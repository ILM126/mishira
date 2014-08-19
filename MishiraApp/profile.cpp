//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 2014 Lucas Murray <lucas@polyflare.com>
// All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//*****************************************************************************

#include "profile.h"
#include "audioinput.h"
#include "audiomixer.h"
#include "application.h"
#include "appsettings.h"
#include "fdkaacencoder.h"
#include "filetarget.h"
#include "hitboxtarget.h"
#include "layergroup.h"
#include "logfilemanager.h"
#include "rtmptarget.h"
#include "scene.h"
#include "sceneitem.h"
#include "twitchtarget.h"
#include "ustreamtarget.h"
#include "x264encoder.h"
#include <Libdeskcap/capturemanager.h>
#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFile>

const QSize DEFAULT_CANVAS_SIZE = QSize(1280, 720); // Never NULL

const QString LOG_CAT_VID = QStringLiteral("Video");
const QString LOG_CAT_AUD = QStringLiteral("Audio");
const QString LOG_CAT_TRGT = QStringLiteral("Target");
const QString LOG_CAT_SCNE = QStringLiteral("Scene");

static void gfxInitializedHandler(void *opaque, VidgfxContext *context)
{
	Profile *profile = static_cast<Profile *>(opaque);
	profile->graphicsContextInitialized(context);
}

static void gfxDestroyingHandler(void *opaque, VidgfxContext *context)
{
	Profile *profile = static_cast<Profile *>(opaque);
	profile->graphicsContextDestroyed(context);
}

/// <summary>
/// Returns a list of profile names that are available to be loaded from the
/// filesystem ordered by their name.
/// </summary>
QVector<QString> Profile::queryAvailableProfiles()
{
	QVector<QString> ret;
	QDir dataDir = App->getDataDirectory();
	QStringList filters;
	filters << "*.profile";
	QStringList entries = dataDir.entryList(
		filters, QDir::Files | QDir::Readable | QDir::Writable, QDir::Name);
	for(int i = 0; i < entries.size(); i++) {
		const QString &entry = entries.at(i);
		QString name = entry.left(entry.size() - 8); // Removes ".profile"
		ret.append(name);
	}
	return ret;
}

/// <summary>
/// Returns the full file path of the profile with the specified name.
/// </summary>
QString Profile::getProfileFilePath(const QString &name)
{
	QDir dataDir = App->getDataDirectory();
	return dataDir.filePath(QStringLiteral("%1.profile").arg(name));
}

/// <summary>
/// Does the profile with the specified name already exist?
/// </summary>
bool Profile::doesProfileExist(const QString &name)
{
	QFileInfo info(getProfileFilePath(name));
	return info.exists();
}

Profile::Profile(const QString &filename, QObject *parent)
	: QObject(parent)
	, m_filename(filename)
	, m_fileExists(false)
	, m_scenes()
	, m_activeScene(NULL)
	, m_prevScene(NULL)
	, m_useFuzzyWinCapture(true)
	, m_videoFramerate(DEFAULT_VIDEO_FRAMERATE)
	, m_mixer(NULL)
	, m_canvasSize(DEFAULT_CANVAS_SIZE)
	, m_audioMode(Prfl441StereoMode)
	, m_videoEncoders()
	, m_audioEncoders()
	, m_layerGroups()
	, m_targets()

	// Transitions
	, m_transition(PrflFadeTransition)
	, m_transitionAni(0.0f, 1.0f, QEasingCurve::Linear, 0.25f)
	, m_fadeVertBuf(NULL)
	, m_activeTransition(0)
	//, m_transitions() // Set below
	//, m_transitionDursMsec() // Set below
{
	// WARNING: This constructor can be called before a graphics context exists
	// or even before the context is fully initialized.

	// Default values
	memset(m_transitions, 0, sizeof(m_transitions));
	memset(m_transitionDursMsec, 0, sizeof(m_transitionDursMsec));
	Q_ASSERT(NumTransitionSettings == 3);
	m_transitions[0] = PrflFadeTransition;
	m_transitions[1] = PrflFadeBlackTransition;
	m_transitions[2] = PrflInstantTransition;
	m_transitionDursMsec[0] = 250;
	m_transitionDursMsec[1] = 750;
	m_transitionDursMsec[2] = 250; // Ignored

	loadFromDisk();

	// We want to be notified when the application wants a frame rendered
	connect(App, &Application::queuedFrameEvent,
		this, &Profile::queuedFrameEvent);

	// Make sure that the profile knows when it can initialize or destroy
	// hardware resources
	VidgfxContext *gfx = App->getGraphicsContext();
	if(gfx != NULL) {
		if(vidgfx_context_is_valid(gfx))
			graphicsContextInitialized(gfx);
		else {
			vidgfx_context_add_initialized_callback(
				gfx, gfxInitializedHandler, this);
		}
		vidgfx_context_add_destroying_callback(
			gfx, gfxDestroyingHandler, this);
	}

	// Test our bitrate suggestions
	//X264Encoder::debugBestBitrates();
}

Profile::~Profile()
{
	appLog()
		<< LOG_SINGLE_LINE << "\n Profile destroy begin\n" << LOG_SINGLE_LINE;

	// Destroy any hardware resources that we have created
	VidgfxContext *gfx = App->getGraphicsContext();
	if(vidgfx_context_is_valid(gfx))
		graphicsContextDestroyed(gfx);

	// Remove callbacks
	if(vidgfx_context_is_valid(gfx)) {
		vidgfx_context_remove_initialized_callback(
			gfx, gfxInitializedHandler, this);
		vidgfx_context_remove_destroying_callback(
			gfx, gfxDestroyingHandler, this);
	}

	// Make sure all targets are not broadcasting
	for(int i = 0; i < m_targets.count(); i++)
		m_targets.at(i)->setActive(false);

	// Do the save
	saveToDisk();

	// Delete all targets cleanly
	while(!m_targets.isEmpty())
		destroyTarget(m_targets.last());

	// Delete all encoders cleanly
	while(!m_videoEncoders.isEmpty())
		destroyVideoEncoder(m_videoEncoders.constBegin().value());
	while(!m_audioEncoders.isEmpty())
		destroyAudioEncoder(m_audioEncoders.constBegin().value());

	// Delete all scenes cleanly
	while(!m_scenes.isEmpty())
		destroyScene(m_scenes.last());
	m_activeScene = NULL;

	// Delete all layer groups cleanly
	while(!m_layerGroups.isEmpty())
		destroyLayerGroup(m_layerGroups.constBegin().value());

	// Delete the audio mixer
	if(m_mixer != NULL)
		delete m_mixer;
	m_mixer = NULL;

	appLog()
		<< LOG_SINGLE_LINE << "\n Profile destroy end\n" << LOG_SINGLE_LINE;
}

void Profile::clear()
{
	// Empty scenes cleanly
	while(!m_scenes.isEmpty())
		destroyScene(m_scenes.last());

	// Empty layer groups cleanly
	while(!m_layerGroups.isEmpty())
		destroyLayerGroup(m_layerGroups.constBegin().value());

	// Empty mixer
	if(m_mixer != NULL) {
		while(AudioInput *input = m_mixer->inputAtIndex(0))
			m_mixer->destroyInput(input);
	}

	// Setup settings
	setCanvasSize(DEFAULT_CANVAS_SIZE);
	setUseFuzzyWinCapture(true);

	// Empty targets cleanly
	while(!m_targets.isEmpty())
		destroyTarget(m_targets.last());

	// Empty encoders cleanly
	while(!m_videoEncoders.isEmpty())
		destroyVideoEncoder(m_videoEncoders.constBegin().value());
	while(!m_audioEncoders.isEmpty())
		destroyAudioEncoder(m_audioEncoders.constBegin().value());
}

void Profile::loadDefaults()
{
	// WARNING: This method can be called before a graphics context exists or
	// even before the context is fully initialized.

	clear();

	// Create the default scene
	createScene(tr("Default"));

	// Set the default framerate
	m_videoFramerate = DEFAULT_VIDEO_FRAMERATE;

	// Add the default speaker and microphone inputs
	if(m_mixer != NULL)
		delete m_mixer;
	m_mixer = new AudioMixer(this);
	m_mixer->createInput(1, tr("Speakers"));
	AudioInput *input = m_mixer->createInput(2, tr("Microphone"));
	input->setMuted(true); // Mute microphone by default

	// Always enter RTMP gamer mode (Decreases lag of games while broadcasting)
	RTMPClient::gamerModeSetEnabled(true);
}

/// <summary>
/// Makes sure there is always at least one active scene. Protects against
/// corrupted files crashing the program. If a new scene is created all known
/// layer groups are added to it so that they don't get pruned
/// </summary>
void Profile::ensureActiveScene()
{
	bool fixed = false;
	if(m_scenes.count() < 1) {
		Scene *scene = createScene(tr("Default"));

		// Add all known groups to the scene so they are not lost
		QHashIterator<quint32, LayerGroup *> it(m_layerGroups);
		while(it.hasNext()) {
			it.next();
			scene->addGroup(it.value(), true);
		}

		fixed = true;
	}
	if(m_activeScene == NULL) {
		m_activeScene = m_scenes.at(0);
		fixed = true;
	}
	if(fixed) {
		appLog(Log::Warning) << QStringLiteral(
			"Corrupted scene in profile, attempting to correct");
	}
}

void Profile::loadFromDisk()
{
	appLog()
		<< LOG_SINGLE_LINE << "\n Profile load begin\n" << LOG_SINGLE_LINE;

	QFile file(m_filename);
	if(!file.exists()) {
		// File doesn't exist, load defaults instead
		m_fileExists = false;
		loadDefaults();
		ensureActiveScene();
		appLog()
			<< LOG_SINGLE_LINE << "\n Profile load end\n" << LOG_SINGLE_LINE;
		return;
	}
	m_fileExists = true;

	file.open(QIODevice::ReadOnly);
	QDataStream stream(&file);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
	stream.setVersion(12);
	bool res = unserialize(&stream);
	if(stream.status() != QDataStream::Ok) {
		appLog(Log::Warning)
			<< "An error occurred while reading the profile settings file";
	}
	file.close();

	if(!res) {
		appLog(Log::Warning)
			<< "Cannot load profile settings file, using defaults";

		// Duplicate the corrupted profile just in case as when the user exits
		// it will automatically override the file!
#define CORRUPTED_FILENAME(name) ((name) + ".corrupt")
		QString newPath = CORRUPTED_FILENAME(m_filename);
		QFile::copy(m_filename, newPath);
		appLog(Log::Warning)
			<< "A backup of the old profile settings can be found at: \""
			<< newPath << "\"";

		// Notify the user so they're not confused. We must delay it as the
		// main window hasn't been created yet
		QTimer::singleShot(0, this, SLOT(unserializeErrorTimeout()));

		loadDefaults();
	}
	ensureActiveScene();

	appLog()
		<< LOG_SINGLE_LINE << "\n Profile load end\n" << LOG_SINGLE_LINE;
}

void Profile::unserializeErrorTimeout()
{
	QString newPath = CORRUPTED_FILENAME(m_filename);
	QString logFilename = LogFileManager::getSingleton()->getFilename();

	// Display a warning in the status bar
	//App->setStatusLabel(
	//	tr("Error loading profile. Backup saved to \"%1\"").arg(newPath));

	// Display a warning dialog to make sure the user isn't confused
	App->showBasicWarningDialog(tr(
		"There was an error loading your profile as it is either corrupted or "
		"was created by an incompatible version of Mishira. A backup of the "
		"profile has been saved to:\n%1\n\nMore details can be found in:\n%2")
		.arg(newPath).arg(logFilename),
		tr("Error loading profile"));
}

/// <summary>
/// Saves the current profile to disk.
/// </summary>
/// <returns>True if the profile was successfully saved</returns>
bool Profile::saveToDisk()
{
	// WARNING: This object can be deleted immediately after this method is
	// executed so if this method is implemented using asynchronous IO then it
	// needs to be able to complete the save without referencing this object

	// Serialize to a temporary buffer just in case this causes the program to
	// crash. If we don't do this then there is a chance that the existing file
	// will be corrupted.
	QByteArray data;
	QBuffer buffer(&data);
	if(!buffer.open(QIODevice::WriteOnly))
		return false; // Should never happen
	QDataStream stream(&buffer);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
	stream.setVersion(12);
	serialize(&stream);
	if(stream.status() != QDataStream::Ok) {
		appLog(Log::Warning)
			<< "An error occurred while serializing profile settings";
		return false;
	}
	buffer.close();

	QFile file(m_filename); // TODO: QSaveFile in Qt 5.1
	if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		appLog(Log::Warning)
			<< "Failed to open profile settings file for saving";
		return false;
	}
	m_fileExists = true; // File has been created
	if(file.write(data) != data.size()) {
		appLog(Log::Warning)
			<< "Failed to write all profile settings data to the filesystem";
	}
	file.close();

	return true;
}

/// <summary>
/// Get the name of the profile from the filename.
/// </summary>
QString Profile::getName() const
{
	QFileInfo info(m_filename);
	return info.completeBaseName();
}

/// <summary>
/// Renames the profile and moves its filesystem file if it exists. If the new
/// profile name already exists it will be overwritten.
/// </summary>
/// <returns>True if the profile was successfully renamed</returns>
bool Profile::setName(const QString &name)
{
	QString newPath = getProfileFilePath(name);
	if(doesFileExist()) {
		// File already exists, rename it
		if(QFile::exists(newPath)) {
			if(!QFile::remove(newPath))
				return false; // Failed to remove existing file
		}
		if(!QFile::rename(m_filename, newPath))
			return false; // Failed to rename
		m_filename = newPath;
	} else {
		// File doesn't exist yet, just change our filename in memory
		m_filename = newPath;
	}
	return true;
}

void Profile::setActiveScene(Scene *scene)
{
	if(m_activeScene == scene)
		return; // No change
	if(indexOfScene(scene) < 0)
		return; // Not in this profile

	// Scenes only become invisible once the transition animation completes
	if(m_prevScene != NULL && !m_transitionAni.atEnd()) {
		// Previous transition did not complete
		m_prevScene->setVisible(false);
	}
	m_transitionAni.progress = 0.0f; // Reset transition
	if(m_transition == PrflInstantTransition) {
		// Ignore duration for instant
		m_transitionAni.progress = m_transitionAni.duration;
	}
	if(m_activeScene != NULL && m_transitionAni.atEnd())
		m_activeScene->setVisible(false);

	m_prevScene = m_activeScene;
	m_activeScene = scene;
	if(scene != NULL)
		m_activeScene->setVisible(true);

	emit activeSceneChanged(m_activeScene, m_prevScene);
}

/// <summary>
/// WARNING: Setting the video framerate does not take affect until the profile
/// is closed and reloaded.
/// </summary>
void Profile::setVideoFramerate(Fraction framerate)
{
	if(m_videoFramerate == framerate)
		return; // No change

	// Validate. If the frequency isn't in our list then it's invalid as well
	if(!framerate.isLegal())
		framerate = DEFAULT_VIDEO_FRAMERATE;
	bool found = false;
	Fraction lastFramerate;
	for(int i = 0; !PrflFpsRates[i].isZero(); i++) {
		lastFramerate = PrflFpsRates[i];
		if(PrflFpsRates[i].asFloat() >= framerate.asFloat()) {
			framerate = PrflFpsRates[i];
			found = true;
			break;
		}
	}
	if(!found) // Higher frequency than what's in our list
		framerate = lastFramerate;

	m_videoFramerate = framerate;
	// We cannot notify the application main loop here as this method is called
	// multiple times during startup. We also will need to recreate all our
	// video encoders if we actually need to apply the new framerate
	// immediately.
}

void Profile::setUseFuzzyWinCapture(bool useFuzzyCapture)
{
	// Make sure the capture manager is always updated
	CaptureManager *mgr = App->getCaptureManager();
	mgr->setFuzzyCapture(useFuzzyCapture);
	if(m_useFuzzyWinCapture == useFuzzyCapture)
		return; // No change
	m_useFuzzyWinCapture = useFuzzyCapture;
	// TODO: Emit changed?
}

void Profile::setTransition(
	int transitionId, PrflTransition mode, uint durationMsec)
{
	if(transitionId < 0 || transitionId >= NumTransitionSettings)
		return; // Invalid ID
	m_transitions[transitionId] = mode;
	m_transitionDursMsec[transitionId] = durationMsec;
	if(transitionId == m_activeTransition)
		setActiveTransition(transitionId);
	emit transitionsChanged();
}

void Profile::getTransition(
	int transitionId, PrflTransition &modeOut, uint &durationMsecOut)
{
	// Set default values
	modeOut = PrflFadeTransition;
	durationMsecOut = 250;

	if(transitionId < 0 || transitionId >= NumTransitionSettings)
		return; // Invalid ID
	modeOut = m_transitions[transitionId];
	durationMsecOut = m_transitionDursMsec[transitionId];
}

void Profile::setActiveTransition(int transitionId)
{
	if(transitionId < 0 || transitionId >= NumTransitionSettings)
		return; // Invalid ID
	m_transition = m_transitions[transitionId];
	float newDur = (float)m_transitionDursMsec[transitionId] * 0.001f;
	if(m_transitionAni.duration != newDur) {
		m_transitionAni.duration = newDur;
		m_transitionAni.progress = newDur;
	}
	m_activeTransition = transitionId;
	emit transitionsChanged();
}

VideoEncoder *Profile::getOrCreateX264VideoEncoder(
	QSize size, SclrScalingMode scaling, GfxFilter scaleFilter,
	const X264Options &opt)
{
	// Find encoder to see if it already exists
	QHashIterator<quint32, VideoEncoder *> it(m_videoEncoders);
	while(it.hasNext()) {
		it.next();
		VideoEncoder *encoder = it.value();
		if(encoder->getType() != VencX264Type)
			continue;
		X264Encoder *enc = static_cast<X264Encoder *>(encoder);
		if(enc->getSize() == size &&
			enc->getScaling() == scaling &&
			enc->getScaleFilter() == scaleFilter &&
			enc->getPreset() == opt.preset &&
			enc->getBitrate() == opt.bitrate &&
			enc->getKeyInterval() == opt.keyInterval)
		{
			// Encoder already exists, return it
			return encoder;
		}
	}

	//-------------------------------------------------------------------------
	// Create new encoder

	// Create a new unique ID for the video encoder
	quint32 id = qrand32();
	while(id == 0 || m_videoEncoders.contains(id))
		id = qrand32();

	// Create and register
	VideoEncoder *encoder = new X264Encoder(
		this, size, scaling, scaleFilter, m_videoFramerate, opt);
	m_videoEncoders[id] = encoder;

	appLog(LOG_CAT_VID) << "Created video encoder " << id;
	return encoder;
}

VideoEncoder *Profile::createVideoEncoderSerialized(
	quint32 id, VencType type, QDataStream *stream)
{
	if(id == 0 || m_videoEncoders.contains(id))
		return NULL; // Invalid ID or already exists

	// Create encoder with dummy values
	VideoEncoder *encoder = NULL;
	switch(type) {
	default:
		appLog(LOG_CAT_VID, Log::Warning)
			<< "Unknown encoder type in video encoder serialized data, "
			<< "cannot load settings";
		break;
	case VencX264Type: {
		X264Options opt;
		opt.bitrate = 100;
		opt.preset = X264SuperFastPreset;
		opt.keyInterval = 0;
		encoder = new X264Encoder(
			this, m_canvasSize, SclrSnapToInnerScale, GfxBilinearFilter,
			m_videoFramerate, opt);
		break; }
	}
	if(encoder == NULL)
		return NULL;

	// Unserialize the real settings and register
	if(!encoder->unserialize(stream)) {
		// Already logged failure reason
		delete encoder;
		return NULL;
	}
	m_videoEncoders[id] = encoder;

	appLog(LOG_CAT_VID)
		<< "Created video encoder " << id << " from serialized data";
	return encoder;
}

void Profile::destroyVideoEncoder(VideoEncoder *encoder)
{
	if(encoder == NULL)
		return;
	delete encoder;
	quint32 id = m_videoEncoders.key(encoder);
	if(id == 0)
		return; // Not registered
	m_videoEncoders.remove(id);
	appLog(LOG_CAT_VID) << "Deleted video encoder " << id;
}

/// <summary>
/// Removes any unreferenced video encoders from the profile.
/// </summary>
void Profile::pruneVideoEncoders()
{
	// Get list of referenced encoders
	QVector<quint32> refList;
	for(int i = 0; i < m_targets.count(); i++) {
		Target *target = m_targets.at(i);
		if(target == NULL)
			continue; // Should never happen
		VideoEncoder *enc = target->getVideoEncoder();
		if(enc == NULL)
			continue;
		refList.append(enc->getId());
	}
	int diff = m_videoEncoders.count() - refList.count();
	if(diff <= 0)
		return; // Nothing to prune

	// Do the actual prune
	appLog(LOG_CAT_VID) <<
		QStringLiteral("Pruning %1 unused video encoders...").arg(diff);
	QHashIterator<quint32, VideoEncoder *> it(m_videoEncoders);
	while(it.hasNext()) {
		it.next();
		VideoEncoder *enc = it.value();
		if(refList.contains(enc->getId()))
			continue; // Encoder is referenced
		// "If the hash is modified while a QHashIterator is active, the
		// QHashIterator will continue iterating over the original hash,
		// ignoring the modified copy." -- QHashIterator documentation
		destroyVideoEncoder(enc);
	}
}

VideoEncoder *Profile::getVideoEncoderById(quint32 id) const
{
	if(m_videoEncoders.contains(id))
		return m_videoEncoders[id];
	return NULL;
}

quint32 Profile::idOfVideoEncoder(VideoEncoder *encoder) const
{
	return m_videoEncoders.key(encoder);
}

AudioEncoder *Profile::getOrCreateFdkAacAudioEncoder(const FdkAacOptions &opt)
{
	// Find encoder to see if it already exists
	QHashIterator<quint32, AudioEncoder *> it(m_audioEncoders);
	while(it.hasNext()) {
		it.next();
		AudioEncoder *encoder = it.value();
		if(encoder->getType() != AencFdkAacType)
			continue;
		FdkAacEncoder *enc = static_cast<FdkAacEncoder *>(encoder);
		if(enc->getBitrate() == opt.bitrate) {
			// Encoder already exists, return it
			return encoder;
		}
	}

	//-------------------------------------------------------------------------
	// Create new encoder

	// Create a new unique ID for the audio encoder
	quint32 id = qrand32();
	while(id == 0 || m_audioEncoders.contains(id))
		id = qrand32();

	// Create and register
	AudioEncoder *encoder = new FdkAacEncoder(this, opt);
	m_audioEncoders[id] = encoder;

	appLog(LOG_CAT_AUD) << "Created audio encoder " << id;
	return encoder;
}

AudioEncoder *Profile::createAudioEncoderSerialized(
	quint32 id, AencType type, QDataStream *stream)
{
	if(id == 0 || m_audioEncoders.contains(id))
		return NULL; // Invalid ID or already exists

	// Create encoder with dummy values
	AudioEncoder *encoder = NULL;
	switch(type) {
	default:
		appLog(LOG_CAT_AUD, Log::Warning)
			<< "Unknown encoder type in audio encoder serialized data, "
			<< "cannot load settings";
		break;
	case AencFdkAacType: {
		FdkAacOptions opt;
		opt.bitrate = 128;
		encoder = new FdkAacEncoder(this, opt);
		break; }
	}
	if(encoder == NULL)
		return NULL;

	// Unserialize the real settings and register
	if(!encoder->unserialize(stream)) {
		// Already logged failure reason
		delete encoder;
		return NULL;
	}
	m_audioEncoders[id] = encoder;

	appLog(LOG_CAT_AUD)
		<< "Created audio encoder " << id << " from serialized data";
	return encoder;
}

void Profile::destroyAudioEncoder(AudioEncoder *encoder)
{
	if(encoder == NULL)
		return;
	delete encoder;
	quint32 id = m_audioEncoders.key(encoder);
	if(id == 0)
		return; // Not registered
	m_audioEncoders.remove(id);
	appLog(LOG_CAT_AUD) << "Deleted audio encoder " << id;
}

/// <summary>
/// Removes any unreferenced audio encoders from the profile.
/// </summary>
void Profile::pruneAudioEncoders()
{
	// Get list of referenced encoders
	QVector<quint32> refList;
	for(int i = 0; i < m_targets.count(); i++) {
		Target *target = m_targets.at(i);
		if(target == NULL)
			continue; // Should never happen
		AudioEncoder *enc = target->getAudioEncoder();
		if(enc == NULL)
			continue;
		refList.append(enc->getId());
	}
	int diff = m_audioEncoders.count() - refList.count();
	if(diff <= 0)
		return; // Nothing to prune

	// Do the actual prune
	appLog(LOG_CAT_AUD) <<
		QStringLiteral("Pruning %1 unused audio encoders...").arg(diff);
	QHashIterator<quint32, AudioEncoder *> it(m_audioEncoders);
	while(it.hasNext()) {
		it.next();
		AudioEncoder *enc = it.value();
		if(refList.contains(enc->getId()))
			continue; // Encoder is referenced
		// "If the hash is modified while a QHashIterator is active, the
		// QHashIterator will continue iterating over the original hash,
		// ignoring the modified copy." -- QHashIterator documentation
		destroyAudioEncoder(enc);
	}
}

AudioEncoder *Profile::getAudioEncoderById(quint32 id) const
{
	if(m_audioEncoders.contains(id))
		return m_audioEncoders[id];
	return NULL;
}

quint32 Profile::idOfAudioEncoder(AudioEncoder *encoder) const
{
	return m_audioEncoders.key(encoder);
}

LayerGroup *Profile::createLayerGroup(const QString &name)
{
	// Create a new unique ID for the group
	quint32 id = qrand32();
	while(id == 0 || m_layerGroups.contains(id))
		id = qrand32();

	// Create and register
	LayerGroup *group = new LayerGroup(this);
	if(!name.isEmpty())
		group->setName(name);
	m_layerGroups[id] = group;

	appLog(LOG_CAT_SCNE) << "Created layer group " << id;
	group->setInitialized();
	return group;
}

/// <summary>
/// If `id` is 0 then a random ID will be generated.
/// </summary>
LayerGroup *Profile::createLayerGroupSerialized(
	quint32 id, QDataStream *stream)
{
	if(m_layerGroups.contains(id))
		return NULL; // Already exists

	// Create a new unique ID for the group if no ID was specified
	if(id == 0) {
		id = qrand32();
		while(id == 0 || m_layerGroups.contains(id))
			id = qrand32();
	}

	// Create group with dummy values
	LayerGroup *group = new LayerGroup(this);

	// Unserialize the real settings and register
	if(!group->unserialize(stream)) {
		// Already logged failure reason
		delete group;
		return NULL;
	}
	m_layerGroups[id] = group;

	appLog(LOG_CAT_SCNE)
		<< "Created layer group " << id << " from serialized data";
	group->setInitialized();
	return group;
}

void Profile::destroyLayerGroup(LayerGroup *group)
{
	if(group == NULL)
		return;
	delete group;
	quint32 id = m_layerGroups.key(group);
	if(id == 0)
		return; // Not registered
	m_layerGroups.remove(id);
	appLog(LOG_CAT_SCNE) << "Deleted layer group " << id;
}

/// <summary>
/// Removes any unreferenced layer groups from the profile.
/// </summary>
void Profile::pruneLayerGroups()
{
	// Get list of referenced encoders
	QVector<quint32> refList;
	for(int i = 0; i < m_scenes.count(); i++) {
		Scene *scene = m_scenes.at(i);
		SceneItemList groups = scene->getGroupSceneItems();
		for(int j = 0; j < groups.size(); j++)
			refList.append(groups.at(j)->getGroup()->getId());
	}
	int diff = m_layerGroups.count() - refList.count();
	if(diff <= 0)
		return; // Nothing to prune

	// Do the actual prune
	appLog(LOG_CAT_SCNE) <<
		QStringLiteral("Pruning %1 unused layer groups...").arg(diff);
	QHashIterator<quint32, LayerGroup *> it(m_layerGroups);
	while(it.hasNext()) {
		it.next();
		LayerGroup *group = it.value();
		if(refList.contains(group->getId()))
			continue; // Group is referenced
		// "If the hash is modified while a QHashIterator is active, the
		// QHashIterator will continue iterating over the original hash,
		// ignoring the modified copy." -- QHashIterator documentation
		destroyLayerGroup(group);
	}
}

LayerGroup *Profile::getLayerGroupById(quint32 id) const
{
	if(m_layerGroups.contains(id))
		return m_layerGroups[id];
	return NULL;
}

quint32 Profile::idOfLayerGroup(LayerGroup *group) const
{
	return m_layerGroups.key(group);
}

/// <summary>
/// Reloads every single loaded layer in the profile, including those in
/// inactive scenes.
/// </summary>
void Profile::reloadAllLayers()
{
	// TODO: This algorithm reloads layers in shared scenes multiple times
	for(int k = 0; k < m_scenes.size(); k++) {
		Scene *scene = m_scenes.at(k);
		SceneItemList groups = scene->getGroupSceneItems();
		for(int i = 0; i < groups.count(); i++) {
			LayerGroup *group = groups.at(i)->getGroup();
			LayerList layers = group->getLayers();
			for(int j = 0; j < layers.count(); j++) {
				Layer *layer = layers.at(j);
				if(!layer->isLoaded())
					continue; // No loaded
				bool wasVisible = layer->isVisible();
				layer->setLoaded(false);
				layer->setLoaded(true);
				if(wasVisible)
					layer->setVisible(true);
			}
		}
	}
}

/// <summary>
/// Load every single layer in the profile into memory, including those in
/// inactive scenes.
/// </summary>
void Profile::loadAllLayers()
{
	for(int k = 0; k < m_scenes.size(); k++) {
		Scene *scene = m_scenes.at(k);
		SceneItemList groups = scene->getGroupSceneItems();
		for(int i = 0; i < groups.count(); i++) {
			LayerGroup *group = groups.at(i)->getGroup();
			LayerList layers = group->getLayers();
			for(int j = 0; j < layers.count(); j++) {
				Layer *layer = layers.at(j);
				layer->setLoaded(true);
			}
		}
	}
}

/// <summary>
/// Unload every single invisible layer in the profile out of memory, including
/// those in inactive scenes.
/// </summary>
void Profile::unloadAllInvisibleLayers()
{
	for(int k = 0; k < m_scenes.size(); k++) {
		Scene *scene = m_scenes.at(k);
		SceneItemList groups = scene->getGroupSceneItems();
		for(int i = 0; i < groups.count(); i++) {
			LayerGroup *group = groups.at(i)->getGroup();
			LayerList layers = group->getLayers();
			for(int j = 0; j < layers.count(); j++) {
				Layer *layer = layers.at(j);
				if(layer->isVisible())
					continue;
				layer->setLoaded(false);
			}
		}
	}
}

Scene *Profile::createScene(const QString &name, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_scenes.count() + 1;
	}
	before = qBound(0, before, m_scenes.count());

	Scene *scene = new Scene(this);
	m_scenes.insert(before, scene);
	if(!name.isEmpty())
		scene->setName(name);
	appLog(LOG_CAT_SCNE) << "Created scene " << scene->getIdString();
	scene->setInitialized();

	emit sceneAdded(scene, before);
	if(m_activeScene == NULL)
		setActiveScene(scene);
	return scene;
}

Scene *Profile::createSceneSerialized(QDataStream *stream, int before)
{
	// Create scene with dummy values
	Scene *scene = new Scene(this);

	// -1 == Append
	if(before < 0)
		before = m_scenes.count();

	// Unserialize the real settings and register
	appLog(LOG_CAT_SCNE)
		<< "Unserializing scene " << scene->getIdString(false) << "...";
	if(!scene->unserialize(stream)) {
		// Already logged failure reason
		delete scene;
		return NULL;
	}
	m_scenes.insert(before, scene);
	appLog(LOG_CAT_SCNE) <<
		QStringLiteral("Created scene %1 from serialized data")
		.arg(scene->getIdString());
	scene->setInitialized();

	emit sceneAdded(scene, before);
	if(m_activeScene == NULL)
		setActiveScene(scene);
	return scene;
}

void Profile::destroyScene(Scene *scene)
{
	int id = m_scenes.indexOf(scene);
	if(id == -1)
		return; // Doesn't exist
	emit destroyingScene(scene);

	QString idString = scene->getIdString();
	if(m_activeScene == scene) {
		scene->setVisible(false);
		for(int i = 0; i < m_scenes.size(); i++) {
			Scene *newScene = m_scenes.at(i);
			if(newScene != scene) {
				setActiveScene(newScene);
				break;
			}
		}
		if(m_activeScene == scene) // No other scenes in profile
			setActiveScene(NULL);
		m_prevScene = NULL; // Never reference a destroyed scene
	}
	m_scenes.remove(id);
	delete scene;
	appLog(LOG_CAT_SCNE) << "Deleted scene " << idString;
}

int Profile::indexOfScene(Scene *scene) const
{
	return m_scenes.indexOf(scene);
}

Scene *Profile::sceneAtIndex(int index) const
{
	if(index < 0 || index >= m_scenes.size())
		return NULL;
	return m_scenes.at(index);
}

void Profile::moveSceneTo(Scene *scene, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_scenes.count() + 1;
	}
	before = qBound(0, before, m_scenes.count());

	// We need to be careful as the `before` index includes ourself
	int index = indexOfScene(scene);
	if(before == index)
		return; // Moving to the same spot
	if(before > index)
		before--; // Adjust to not include ourself
	m_scenes.remove(index);
	m_scenes.insert(before, scene);

	// Emit move signal
	emit sceneMoved(scene, before);
}

Target *Profile::createFileTarget(
	const QString &name, const FileTrgtOptions &opt, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_targets.count() + 1;
	}
	before = qBound(0, before, m_targets.count());

	FileTarget *target = new FileTarget(this, name, opt);
	connect(target, &Target::activeChanged,
		this, &Profile::targetActiveChanged);
	m_targets.insert(before, target);
	appLog(LOG_CAT_TRGT) << "Created file target " << target->getIdString();
	target->setInitialized();

	emit targetAdded(target, before);
	return target;
}

Target *Profile::createRtmpTarget(
	const QString &name, const RTMPTrgtOptions &opt, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_targets.count() + 1;
	}
	before = qBound(0, before, m_targets.count());

	RTMPTarget *target = new RTMPTarget(this, name, opt);
	connect(target, &Target::activeChanged,
		this, &Profile::targetActiveChanged);
	m_targets.insert(before, target);
	appLog(LOG_CAT_TRGT) << "Created RTMP target " << target->getIdString();
	target->setInitialized();

	emit targetAdded(target, before);
	return target;
}

Target *Profile::createTwitchTarget(
	const QString &name, const TwitchTrgtOptions &opt, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_targets.count() + 1;
	}
	before = qBound(0, before, m_targets.count());

	TwitchTarget *target = new TwitchTarget(this, name, opt);
	connect(target, &Target::activeChanged,
		this, &Profile::targetActiveChanged);
	m_targets.insert(before, target);
	appLog(LOG_CAT_TRGT) << "Created Twitch target " << target->getIdString();
	target->setInitialized();

	emit targetAdded(target, before);
	return target;
}

Target *Profile::createUstreamTarget(
	const QString &name, const UstreamTrgtOptions &opt, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_targets.count() + 1;
	}
	before = qBound(0, before, m_targets.count());

	UstreamTarget *target = new UstreamTarget(this, name, opt);
	connect(target, &Target::activeChanged,
		this, &Profile::targetActiveChanged);
	m_targets.insert(before, target);
	appLog(LOG_CAT_TRGT) << "Created Ustream target " << target->getIdString();
	target->setInitialized();

	emit targetAdded(target, before);
	return target;
}

Target *Profile::createHitboxTarget(
	const QString &name, const HitboxTrgtOptions &opt, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_targets.count() + 1;
	}
	before = qBound(0, before, m_targets.count());

	HitboxTarget *target = new HitboxTarget(this, name, opt);
	connect(target, &Target::activeChanged,
		this, &Profile::targetActiveChanged);
	m_targets.insert(before, target);
	appLog(LOG_CAT_TRGT) << "Created Hitbox target " << target->getIdString();
	target->setInitialized();

	emit targetAdded(target, before);
	return target;
}

Target *Profile::createTargetSerialized(
	TrgtType type, QDataStream *stream, int before)
{
	// -1 == Append
	if(before < 0)
		before = m_targets.count();

	// Create target with dummy values
	Target *target = NULL;
	switch(type) {
	default:
		appLog(LOG_CAT_TRGT, Log::Warning)
			<< "Unknown type in target serialized data, cannot load settings";
		break;
	case TrgtFileType: {
		FileTrgtOptions opt;
		opt.videoEncId = 0;
		opt.audioEncId = 0;
		opt.filename = QStringLiteral("Dummy.mp4");
		opt.fileType = FileTrgtMp4Type;
		target = new FileTarget(this, QStringLiteral("Dummy"), opt);
		break; }
	case TrgtRtmpType: {
		RTMPTrgtOptions opt;
		opt.videoEncId = 0;
		opt.audioEncId = 0;
		opt.url = QString();
		opt.streamName = QString();
		opt.hideStreamName = false;
		opt.padVideo = false;
		target = new RTMPTarget(this, QStringLiteral("Dummy"), opt);
		break; }
	case TrgtTwitchType: {
		TwitchTrgtOptions opt;
		opt.videoEncId = 0;
		opt.audioEncId = 0;
		opt.url = QString();
		opt.streamKey = QString();
		opt.username = QString();
		target = new TwitchTarget(this, QStringLiteral("Dummy"), opt);
		break; }
	case TrgtUstreamType: {
		UstreamTrgtOptions opt;
		opt.videoEncId = 0;
		opt.audioEncId = 0;
		opt.url = QString();
		opt.streamKey = QString();
		opt.padVideo = false;
		target = new UstreamTarget(this, QStringLiteral("Dummy"), opt);
		break; }
	case TrgtHitboxType: {
		HitboxTrgtOptions opt;
		opt.videoEncId = 0;
		opt.audioEncId = 0;
		opt.url = QString();
		opt.streamKey = QString();
		opt.username = QString();
		target = new HitboxTarget(this, QStringLiteral("Dummy"), opt);
		break; }
	}
	if(target == NULL)
		return NULL;

	// Unserialize the real settings and register
	appLog(LOG_CAT_TRGT)
		<< "Unserializing target " << target->getIdString(false) << "...";
	if(!target->unserialize(stream)) {
		// Already logged failure reason
		delete target;
		return NULL;
	}
	connect(target, &Target::activeChanged,
		this, &Profile::targetActiveChanged);
	m_targets.insert(before, target);
	appLog(LOG_CAT_TRGT) <<
		QStringLiteral("Created target %1 from serialized data")
		.arg(target->getIdString());
	target->setInitialized();

	emit targetAdded(target, before);
	return target;
}

void Profile::destroyTarget(Target *target)
{
	int id = m_targets.indexOf(target);
	if(id == -1)
		return; // Doesn't exist
	emit destroyingTarget(target);

	QString idString = target->getIdString();
	m_targets.remove(id);
	delete target;
	appLog(LOG_CAT_TRGT) << "Deleted target " << idString;
}

int Profile::indexOfTarget(Target *target) const
{
	return m_targets.indexOf(target);
}

void Profile::moveTargetTo(Target *target, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_targets.count() + 1;
	}
	before = qBound(0, before, m_targets.count());

	// We need to be careful as the `before` index includes ourself
	int index = indexOfTarget(target);
	if(before == index)
		return; // Moving to the same spot
	if(before > index)
		before--; // Adjust to not include ourself
	m_targets.remove(index);
	m_targets.insert(before, target);

	// Emit move signal
	emit targetMoved(target, before);
}

void Profile::setupContext(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return;

	// Notify the context of our canvas size
	vidgfx_context_resize_canvas_target(gfx, m_canvasSize);

	// Update projection matrix
	vidgfx_context_set_render_target(gfx, GfxCanvas1Target); // Updates all canvas targets
	QMatrix4x4 mat;
	mat.ortho(
		0.0f, m_canvasSize.width(), m_canvasSize.height(), 0.0f,
		-1.0f, 1.0f);
	vidgfx_context_set_proj_mat(gfx, mat);

	// Create transition objects
	if(m_fadeVertBuf != NULL) {
		vidgfx_context_destroy_vertbuf(gfx, m_fadeVertBuf);
		m_fadeVertBuf = NULL;
	}
	m_fadeVertBuf = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_TEX_DECAL_RECT_BUF_SIZE);
	if(m_fadeVertBuf != NULL) {
		vidgfx_create_tex_decal_rect(
			m_fadeVertBuf, QRectF(QPointF(0.0f, 0.0f), m_canvasSize));
	}
}

void Profile::setCanvasSize(const QSize &size)
{
	if(m_canvasSize == size)
		return; // No change
	QSize oldSize = m_canvasSize;
	m_canvasSize = size;

	VidgfxContext *gfx = App->getGraphicsContext();
	if(vidgfx_context_is_valid(gfx)) {
		setupContext(gfx);
		// TODO: Automatically scale layers? Forward layer resize events
	}

	emit canvasSizeChanged(oldSize);
}

void Profile::setAudioMode(PrflAudioMode mode)
{
	if(m_audioMode == mode)
		return; // No change
	m_audioMode = mode;
	appLog(LOG_CAT_AUD)
		<< QStringLiteral("Audio processing mode set to: %1")
		.arg(PrflAudioModeStrings[mode]);
	emit audioModeChanged(m_audioMode);
}

void Profile::render(VidgfxContext *gfx, uint frameNum, int numDropped)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be usuable

	GfxRenderTarget target = GfxCanvas1Target;
	if(m_transitionAni.atEnd()) {
		// No transition animation is currently active, just render a single
		// scene as efficiently as possible
		vidgfx_context_set_render_target(gfx, target);
		vidgfx_context_clear(gfx, QColor(0, 0, 0));
		if(m_activeScene != NULL)
			m_activeScene->render(gfx, frameNum, numDropped);
	} else {
		// We are currently transitioning between two scenes
		switch(m_transition) {
		default:
		case PrflInstantTransition: // Should never need to transition
		case PrflFadeTransition: {
			// Render the previous scene first
			vidgfx_context_set_render_target(gfx, target);
			vidgfx_context_clear(gfx, QColor(0, 0, 0));
			if(m_prevScene != NULL)
				m_prevScene->render(gfx, frameNum, numDropped);

			// Render the current scene to another target
			vidgfx_context_set_render_target(gfx, GfxCanvas2Target);
			vidgfx_context_clear(gfx, QColor(0, 0, 0));
			if(m_activeScene != NULL)
				m_activeScene->render(gfx, frameNum, numDropped);

			// Render the current scene's texture on top of the old scene
			vidgfx_context_set_render_target(gfx, target);
			vidgfx_context_set_tex(
				gfx, vidgfx_context_get_target_tex(gfx, GfxCanvas2Target));
			vidgfx_context_set_shader(gfx, GfxTexDecalRgbShader);
			vidgfx_context_set_topology(gfx, GfxTriangleStripTopology);
			vidgfx_context_set_blending(gfx, GfxAlphaBlending);
			QColor oldCol = vidgfx_context_get_tex_decal_mod_color(gfx);
			vidgfx_context_set_tex_decal_mod_color(gfx, QColor(255, 255, 255,
				(int)(m_transitionAni.currentValue() * 255.0f)));
			vidgfx_context_draw_buf(gfx, m_fadeVertBuf);
			vidgfx_context_set_tex_decal_mod_color(gfx, oldCol);

			break; }
		case PrflFadeBlackTransition:
		case PrflFadeWhiteTransition: {
			// Determine which canvas to render and rescale duration
			Scene *scene = NULL;
			float opacity = m_transitionAni.currentValue() * 2.0f;
			if(opacity >= 1.0f) {
				// Rendering the active scene fading in
				opacity -= 1.0f;
				scene = m_activeScene;
			} else {
				// Rendering the previous scene fading out
				opacity = 1.0f - opacity;
				scene = m_prevScene;
			}

			// Render the scene to another target
			vidgfx_context_set_render_target(gfx, GfxCanvas2Target);
			vidgfx_context_clear(gfx, QColor(0, 0, 0));
			if(scene != NULL)
				scene->render(gfx, frameNum, numDropped);

			// Clear with the appropriate background colour
			vidgfx_context_set_render_target(gfx, target);
			if(m_transition == PrflFadeBlackTransition)
				vidgfx_context_clear(gfx, QColor(0, 0, 0));
			else
				vidgfx_context_clear(gfx, QColor(255, 255, 255));

			// Render the scene's texture
			vidgfx_context_set_tex(
				gfx, vidgfx_context_get_target_tex(gfx, GfxCanvas2Target));
			vidgfx_context_set_shader(gfx, GfxTexDecalRgbShader);
			vidgfx_context_set_topology(gfx, GfxTriangleStripTopology);
			vidgfx_context_set_blending(gfx, GfxAlphaBlending);
			QColor oldCol = vidgfx_context_get_tex_decal_mod_color(gfx);
			vidgfx_context_set_tex_decal_mod_color(gfx, QColor(255, 255, 255,
				(int)(opacity * 255.0f)));
			vidgfx_context_draw_buf(gfx, m_fadeVertBuf);
			vidgfx_context_set_tex_decal_mod_color(gfx, oldCol);

			break; }
		}
	}
	VidgfxTex *targetTex = vidgfx_context_get_target_tex(gfx, target);
	emit frameRendered(targetTex, frameNum, numDropped);
}

void Profile::serialize(QDataStream *stream) const
{
	// Write header and file version
	*stream << (quint32)0x6AF64620;
	*stream << (quint32)2;

	// Write top-level data
	*stream << m_canvasSize;
	*stream << (quint32)m_audioMode;

	// Write the layer groups
	QHashIterator<quint32, LayerGroup *> itA(m_layerGroups);
	*stream << (quint32)m_layerGroups.count();
	while(itA.hasNext()) {
		itA.next();
		*stream << itA.key();
		itA.value()->serialize(stream);
	}

	// Write scenes
	*stream << (quint32)m_scenes.count();
	for(int i = 0; i < m_scenes.count(); i++) {
		Scene *scene = m_scenes.at(i);
		scene->serialize(stream);
	}
	*stream << (qint32)indexOfScene(m_activeScene);

	// Write scene transitions
	Q_ASSERT(NumTransitionSettings == 3);
	*stream << (quint32)m_transitions[0];
	*stream << (quint32)m_transitions[1];
	*stream << (quint32)m_transitions[2];
	*stream << (quint32)m_transitionDursMsec[0];
	*stream << (quint32)m_transitionDursMsec[1];
	*stream << (quint32)m_transitionDursMsec[2];
	*stream << (quint32)m_activeTransition;

	// Write the video framerate
	*stream << (quint32)m_videoFramerate.numerator;
	*stream << (quint32)m_videoFramerate.denominator;

	// Write the audio mixer
	m_mixer->serialize(stream);

	// Write video encoders
	QHashIterator<quint32, VideoEncoder *> itB(m_videoEncoders);
	*stream << (quint32)m_videoEncoders.count();
	while(itB.hasNext()) {
		itB.next();
		*stream << itB.key();
		*stream << (quint32)itB.value()->getType();
		itB.value()->serialize(stream);
	}

	// Write audio encoders
	QHashIterator<quint32, AudioEncoder *> itC(m_audioEncoders);
	*stream << (quint32)m_audioEncoders.count();
	while(itC.hasNext()) {
		itC.next();
		*stream << itC.key();
		*stream << (quint32)itC.value()->getType();
		itC.value()->serialize(stream);
	}

	// Write targets
	*stream << (quint32)m_targets.count();
	for(int i = 0; i < m_targets.count(); i++) {
		Target *target = m_targets.at(i);
		*stream << (quint32)target->getType();
		target->serialize(stream);
	}
}

bool Profile::unserialize(QDataStream *stream)
{
	// WARNING: This method can be called before a graphics context exists or
	// even before the context is fully initialized.

	qint32	int32Data;
	quint32	uint32Data, uint32Data2;
	QSize	sizeData;

	// Clear the entire profile
	clear();

	// Read header
	*stream >> uint32Data;
	if(uint32Data != 0x6AF64620) {
		appLog(Log::Warning) << "Invalid profile settings file header";
		return false;
	}

	// Read file version
	quint32 version;
	*stream >> version;
	if(version >= 0 && version <= 2) {
		// Read top-level data
		*stream >> sizeData;
		setCanvasSize(sizeData);
		*stream >> uint32Data;
		setAudioMode((PrflAudioMode)uint32Data);

		// Read layer groups
		*stream >> uint32Data;
		int count = uint32Data;
		for(int i = 0; i < count; i++) {
			*stream >> uint32Data;
			quint32 id = uint32Data;
			if(createLayerGroupSerialized(id, stream) == NULL) {
				// The reason has already been logged
				return false;
			}
		}

		// Read scenes
		*stream >> uint32Data;
		count = uint32Data;
		for(int i = 0; i < count; i++) {
			if(createSceneSerialized(stream) == NULL) {
				// The reason has already been logged
				return false;
			}
		}
		*stream >> int32Data; // Active scene index
		if(int32Data >= 0 && int32Data < m_scenes.size())
			setActiveScene(m_scenes.at(int32Data));
		else
			setActiveScene(0);

		// Read scene transitions
		if(version >= 2) {
			Q_ASSERT(NumTransitionSettings == 3);
			*stream >> uint32Data;
			m_transitions[0] = (PrflTransition)uint32Data;
			*stream >> uint32Data;
			m_transitions[1] = (PrflTransition)uint32Data;
			*stream >> uint32Data;
			m_transitions[2] = (PrflTransition)uint32Data;
			*stream >> uint32Data;
			m_transitionDursMsec[0] = uint32Data;
			*stream >> uint32Data;
			m_transitionDursMsec[1] = uint32Data;
			*stream >> uint32Data;
			m_transitionDursMsec[2] = uint32Data;
			*stream >> uint32Data;
			setActiveTransition(uint32Data);
		} else {
			Q_ASSERT(NumTransitionSettings == 3);
			m_transitions[0] = PrflFadeTransition;
			m_transitions[1] = PrflFadeBlackTransition;
			m_transitions[2] = PrflInstantTransition;
			m_transitionDursMsec[0] = 250;
			m_transitionDursMsec[1] = 750;
			m_transitionDursMsec[2] = 250; // Ignored
			setActiveTransition(0);
		}

		if(version >= 1) {
			// Read video framerate
			*stream >> uint32Data;
			*stream >> uint32Data2;
			setVideoFramerate(Fraction(uint32Data, uint32Data2));
		} else
			setVideoFramerate(DEFAULT_VIDEO_FRAMERATE);

		// Read audio mixer
		if(m_mixer != NULL)
			delete m_mixer;
		m_mixer = new AudioMixer(this);
		if(!m_mixer->unserialize(stream)) {
			// The reason has already been logged
			return false;
		}

		// Read video encoders
		*stream >> uint32Data;
		count = uint32Data;
		for(int i = 0; i < count; i++) {
			*stream >> uint32Data;
			quint32 id = uint32Data;
			*stream >> uint32Data;
			VencType type = (VencType)uint32Data;
			if(createVideoEncoderSerialized(id, type, stream) == NULL) {
				// The reason has already been logged
				return false;
			}
		}

		// Read audio encoders
		*stream >> uint32Data;
		count = uint32Data;
		for(int i = 0; i < count; i++) {
			*stream >> uint32Data;
			quint32 id = uint32Data;
			*stream >> uint32Data;
			AencType type = (AencType)uint32Data;
			if(createAudioEncoderSerialized(id, type, stream) == NULL) {
				// The reason has already been logged
				return false;
			}
		}

		// Read targets
		*stream >> uint32Data;
		count = uint32Data;
		for(int i = 0; i < count; i++) {
			*stream >> uint32Data;
			TrgtType type = (TrgtType)uint32Data;
			if(createTargetSerialized(type, stream) == NULL) {
				// The reason has already been logged
				return false;
			}
		}

		// Always enter RTMP gamer mode (Decreases lag of games while
		// broadcasting)
		RTMPClient::gamerModeSetEnabled(true);
	} else {
		appLog(Log::Warning)
			<< "Unknown profile settings file version, cannot load settings";
		return false;
	}

	//appLog() << "Initial profile scene graph:";
	//m_activeScene->logItems();

	//appLog() << "Initial profile audio inputs:";
	//m_mixer->logInputs();

	return true;
}

void Profile::graphicsContextInitialized(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be useable

	setupContext(gfx);

	// Forward to loaded layers only
	QHashIterator<quint32, LayerGroup *> it(m_layerGroups);
	while(it.hasNext()) {
		it.next();
		LayerGroup *group = it.value();
		LayerList layers = group->getLayers();
		for(int j = 0; j < layers.count(); j++) {
			Layer *layer = layers.at(j);
			if(layer->isLoaded())
				layer->initializeResources(gfx);
		}
	}
}

void Profile::graphicsContextDestroyed(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be useable

	vidgfx_context_set_render_target(gfx, GfxCanvas1Target);

	// Destroy transition objects
	vidgfx_context_destroy_vertbuf(gfx, m_fadeVertBuf);
	m_fadeVertBuf = NULL;

	// Forward to loaded layers only
	QHashIterator<quint32, LayerGroup *> it(m_layerGroups);
	while(it.hasNext()) {
		it.next();
		LayerGroup *group = it.value();
		LayerList layers = group->getLayers();
		for(int j = 0; j < layers.count(); j++) {
			Layer *layer = layers.at(j);
			if(layer->isLoaded())
				layer->destroyResources(gfx);
		}
	}
}

void Profile::queuedFrameEvent(uint frameNum, int numDropped)
{
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return;

	//appLog(LOG_CAT)
	//	<< "Rendering frame " << frameNum << " (Dropped " << numDropped << ")";

	// Progress transition animation if one is active
	if(!m_transitionAni.atEnd()) {
		float period = 1.0f / m_videoFramerate.asFloat();
		m_transitionAni.addTime(period * (float)(numDropped + 1));

		// If a transition has completed then the previous scene is no longer
		// visible
		if(m_prevScene != NULL && m_transitionAni.atEnd())
			m_prevScene->setVisible(false);
	}

	// Repaint the canvas
	render(gfx, frameNum, numDropped);
}

void Profile::targetActiveChanged(Target *target, bool active)
{
	// The state of one of our targets changed, do we need to update the state
	// of the broadcast button?
	bool anyActive = false;
	for(int i = 0; i < m_targets.size(); i++) {
		if(m_targets.at(i)->isActive()) {
			anyActive = true;
			break;
		}
	}
	if(anyActive != App->isBroadcasting())
		App->setBroadcasting(anyActive);
}
