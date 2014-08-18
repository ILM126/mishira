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

#include "audioinput.h"
#include "application.h"
#include "audiomixer.h"
#include "audiosource.h"
#include "audiosourcemanager.h"
#include "Widgets/infowidget.h"
#include <QtCore/QDataStream>

const QString LOG_CAT = QStringLiteral("Audio");

AudioInput::AudioInput(AudioMixer *mixer, quint64 sourceId, PrflAudioMode mode)
	: QObject()
	, m_isInitializing(true)
	, m_mixer(mixer)
	, m_isActive(false)
	, m_mode(mode)
	, m_source(NULL)
	, m_volume(1.0f)
	, m_buffer()
	, m_firstTimestamp(0)

	// Statistics about the latest input data. NOTE: If new items are added
	// don't forget to add them to the reset code
	, m_isCalcInputStats(false)
	, m_inStats(mixer->getSampleRate(), mixer->getNumChannels())
	, m_outStats(mixer->getSampleRate(), mixer->getNumChannels())

	// Serialized data
	, m_name(tr("Unnamed"))
	, m_sourceId(sourceId)
	, m_attenuation(0)
	, m_isMuted(false)
	, m_delayUsec(0)
{
}

AudioInput::~AudioInput()
{
	if(m_source != NULL) {
		m_source->dereference();
		m_source = NULL;
	}
}

void AudioInput::setInitialized()
{
	if(!m_isInitializing)
		return; // Already initialized
	m_isInitializing = false;
	initializedEvent();
}

void AudioInput::setName(const QString &name)
{
	QString str = name;
	if(str.isEmpty())
		str = tr("Unnamed");
	if(m_name == str)
		return;
	QString oldName = m_name;
	m_name = str;
	m_mixer->inputChanged(this); // Remote emit
	if(!m_isInitializing)
		appLog(LOG_CAT) << "Renamed audio input " << getIdString();
}

void AudioInput::setCalcInputStats(bool calcInput)
{
	m_isCalcInputStats = calcInput;
	if(!m_isCalcInputStats)
		m_inStats.reset(); // Reset stats
}

/// <summary>
/// Returns true if this input uses the default source of any type.
/// </summary>
bool AudioInput::isDefaultOfType() const
{
	if(AudioSourceManager::getTypeOfDefaultId(m_sourceId) <
		NUM_AUDIO_SOURCE_TYPES)
	{
		return true;
	}
	return false;
}

QString AudioInput::getIdString(bool showName) const
{
	if(showName) {
		return QStringLiteral("%1 (\"%2\")")
			.arg(pointerToString((void *)this))
			.arg(getName());
	}
	return pointerToString((void *)this);
}

void AudioInput::setAttenuation(int attenuationMb)
{
	if(m_attenuation == attenuationMb)
		return; // Nothing to do
	m_attenuation = attenuationMb;
	m_volume = mbToLinear((float)m_attenuation);
	m_mixer->inputChanged(this); // Remote emit
}

void AudioInput::setMuted(bool muted)
{
	if(m_isMuted == muted)
		return; // Nothing to do
	m_isMuted = muted;
	m_mixer->inputChanged(this); // Remote emit
}

/// <summary>
/// WARNING: The audio mixer must manually be reset immediately after changing
/// the delay.
/// </summary>
void AudioInput::setDelay(qint64 delayUsec)
{
	if(m_delayUsec == delayUsec)
		return; // Nothing to do
	m_delayUsec = delayUsec;
	m_mixer->inputChanged(this); // Remote emit
}

void AudioInput::initializedEvent()
{
	// Fetch audio source
	AudioSourceManager *mgr = App->getAudioSourceManager();
	AudioSource *source = mgr->getSource(m_sourceId);
	if(source == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "Could not find audio source for input " << getIdString()
			<< ", could be hot-pluggable";
	}

	// Watch manager for add/remove signals
	connect(mgr, &AudioSourceManager::sourceAdded,
		this, &AudioInput::sourceAdded);
	connect(mgr, &AudioSourceManager::removingSource,
		this, &AudioInput::removingSource);

	// Process source immediately if it already exists
	if(source != NULL)
		sourceAdded(source);
}

void AudioInput::sourceAdded(AudioSource *source)
{
	if(source->getId() != m_sourceId && source->getRealId() != m_sourceId)
		return; // Not our source

	// Reference audio source
	if(!source->reference(m_mixer)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Could not reference audio source for input " << getIdString();
		return;
	}
	m_source = source;
	m_isActive = true;

	// Watch source for audio segments
	connect(m_source, &AudioSource::segmentReady,
		this, &AudioInput::segmentReady);

	m_mixer->inputChanged(this); // Remote emit
}

void AudioInput::removingSource(AudioSource *source)
{
	if(source->getId() != m_sourceId && source->getRealId() != m_sourceId)
		return; // Not our source
	if(m_source == NULL)
		return; // Would happen if we failed to reference the source

	// Stop watching source for audio segments
	disconnect(m_source, &AudioSource::segmentReady,
		this, &AudioInput::segmentReady);

	// Dereference audio source
	m_source->dereference();
	m_source = NULL;
	m_isActive = false;

	// Reset statistics
	m_inStats.reset();
	m_outStats.reset();

	m_mixer->inputChanged(this); // Remote emit
}

void AudioInput::moveTo(int before)
{
	AudioInputList &inputs = m_mixer->getInputsMutable();
	if(before < 0) {
		// Position is relative to the right
		before += inputs.count() + 1;
	}
	before = qBound(0, before, inputs.count());

	// We need to be careful as the `before` index includes ourself
	int index = m_mixer->indexOfInput(this);
	if(before == index)
		return; // Moving to the same spot
	if(before > index)
		before--; // Adjust to not include ourself
	inputs.remove(index);
	inputs.insert(before, this);

	// Emit move signal
	m_mixer->inputMoved(this, before); // Remote emit
}

void AudioInput::serialize(QDataStream *stream) const
{
	// Write data version number
	*stream << (quint32)1;

	// Save our data
	*stream << m_name;
	*stream << m_sourceId;
	*stream << (qint32)m_attenuation;
	*stream << m_isMuted;
	*stream << m_delayUsec;
}

bool AudioInput::unserialize(QDataStream *stream)
{
	QString	strData;
	qint32	int32Data;
	qint64	int64Data;
	quint64	uint64Data;
	bool	boolData;

	// Make sure that the input hasn't been initialized yet
	if(!m_isInitializing) {
		appLog(LOG_CAT, Log::Warning)
			<< "Cannot unserialize a audio input when it has already been "
			<< "initialized";
		return false;
	}

	// Load defaults here if ever needed

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version >= 0 && version <= 1) {
		// Read our data
		*stream >> strData;
		setName(strData);
		*stream >> uint64Data;
		m_sourceId = uint64Data;
		*stream >> int32Data;
		setAttenuation(int32Data);
		*stream >> boolData;
		setMuted(boolData);
		if(version >= 1) {
			*stream >> int64Data;
			setDelay(int64Data);
		} else
			setDelay(0LL);
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in audio input serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

/// <summary>
/// Returns the approximate timestamp of the first sample in the buffer taking
/// into account any manual audio delays.
/// </summary>
qint64 AudioInput::getBufferTimestamp() const
{
	// "0" is a special timestamp for audio segments
	if(m_firstTimestamp == 0ULL)
		return 0LL;
	qint64 ts = (qint64)m_firstTimestamp + m_delayUsec;
	if(ts == 0LL)
		ts = 1LL;
	return ts;
}

/// <summary>
/// Notify the audio input that `numReadFloats` floats in the buffer have been
/// read by the mixer and that they can be removed from the internal buffer.
/// </summary>
void AudioInput::reduceBufferBy(int numReadFloats)
{
	if(numReadFloats <= 0)
		return;
	int numBytes = numReadFloats * sizeof(float);
	if(numReadFloats > INT_MAX / sizeof(float))
		numBytes = INT_MAX; // The above maths overflowed
	if(numBytes >= m_buffer.size()) {
		// The entire buffer has been read
		m_buffer.clear();
		m_firstTimestamp = 0;
		return;
	}
	Q_ASSERT(numBytes >= 0);

	// Only part of the buffer has been read, remove the read portion only. We
	// abuse QByteArray's `data()` here as we want to reuse the same memory
	// allocation for as long as possible.
	// TODO: Use a ring buffer to prevent wasteful memory copies instead?
	int newSize = m_buffer.size() - numBytes;
	char *dst = m_buffer.data();
	char *src = dst + numBytes;
	memmove(dst, src, newSize);
	m_buffer.resize(newSize); // Keeps old size in "reserve"

	// Update timestamp. WARNING: This is approximate and causes drift. We
	// update it more accurately when we get the next segment from the source.
	m_firstTimestamp +=
		(quint64)(numReadFloats / m_mixer->getNumChannels()) * 1000000ULL /
		m_mixer->getSampleRate();
}

void AudioInput::setupInputInfo(AudioInputInfoWidget *widget) const
{
	// We do not use `m_source` as it won't exist if the device is unplugged.
	AudioSource *source = App->getAudioSourceManager()->getSource(m_sourceId);

	widget->setTitle(m_name);
	//widget->setIconPixmap(QPixmap(":/Resources/mixer-160x70-input.png"));

	int offset = 0;

	// Device type
	QString setStr = tr("Unknown");
	if(source != NULL) {
		switch(source->getType()) {
		default:
		case AsrcOutputType:
			setStr = tr("Playback device");
			break;
		case AsrcInputType:
			setStr = tr("Recording device");
			break;
		}
	}
	widget->setItemText(offset++, tr("Device type:"), setStr, false);

	// Device name
	setStr = tr("** Device disconnected **");
	if(source != NULL)
		setStr = source->getFriendlyName();
	if(isDefaultOfType()) {
		AsrcType type = AudioSourceManager::getTypeOfDefaultId(m_sourceId);
		switch(type) {
		default:
		case AsrcOutputType:
			setStr = tr("Default playback device (\"%1\")")
				.arg(setStr);
			break;
		case AsrcInputType:
			setStr = tr("Default recording device (\"%1\")")
				.arg(setStr);
			break;
		}
	}
	widget->setItemText(offset++, tr("Device:"), setStr, false);

	// Device state
	setStr = tr("Unplugged");
	if(source == NULL)
		setStr = tr("Unavailable");
	else if(source->isConnected())
		setStr = tr("Enabled");
	widget->setItemText(offset++, tr("State:"), setStr, true);

	// Audio delay
	qint64 msecDelay = getDelay() / 1000LL;
	if(msecDelay >= 0)
		setStr = tr("+%L1 msec").arg(getDelay() / 1000LL);
	else
		setStr = tr("%L1 msec").arg(getDelay() / 1000LL);
	widget->setItemText(offset++, tr("Delay:"), setStr, true);
}

/// <summary>
/// Called whenever the user clicks the mute/unmute button.
/// </summary>
void AudioInput::toggleMuted()
{
	setMuted(!m_isMuted);
}

void AudioInput::segmentReady(AudioSegment segment)
{
	if(m_source == NULL)
		return;

	// HACK: Discard the first 100ms of audio as our compensation code might
	// cause negative timestamps or discontinuities. Both of these are VERY bad
	// and can cause crashes.
	const quint64 INITIAL_DISCARD_DURATION_USEC = 100000; // 100ms
	if(segment.timestamp() < INITIAL_DISCARD_DURATION_USEC)
		return;

	// Get the timestamp of the first sample in the segment. In order to
	// prevent our timestamp drifting we always update it even if there are
	// still samples in the buffer as the audio source handles the drift in a
	// more robust way.
	m_firstTimestamp = segment.timestamp();
	if(!m_buffer.isEmpty()) {
		quint64 curDuration =
			(quint64)(getBufferSize() / m_mixer->getNumChannels()) *
			1000000ULL / (quint64)m_mixer->getSampleRate();
		if(curDuration >= m_firstTimestamp) {
			// Just in case our HACK above doesn't catch all cases. This may or
			// may not cause desynchronisation as it has never been tested.
			m_firstTimestamp = 1ULL; // "0" is special
		} else
			m_firstTimestamp -= curDuration;
	}

	// Calculate the volume of the input data only if required
	int numFloats = segment.numFloats();
	if(m_isCalcInputStats)
		m_inStats.calcStats(segment.floatData(), numFloats);

	// Increase our buffer's size and get a pointer to the end of the buffer
	int oldSize = m_buffer.size();
	m_buffer.resize(m_buffer.size() + numFloats * sizeof(float));
	float *buf = (float *)(m_buffer.data() + oldSize);

	//--------------------------------------------------------------------------
	// Pass input through our filters in a way that limits the amount of memory
	// copies. We do this even when muted so that our UI still shows the volume
	// bars.

	// Attenuation filter
	applyAttenuationFilter(segment.floatData(), buf, numFloats, m_volume);

	//--------------------------------------------------------------------------

	// Calculate the volume of the output data
	m_outStats.calcStats(buf, numFloats);

	// Apply muting after calculating the output statistics. TODO: Fade out?
	if(m_isMuted)
		applyMuteFilter(buf, numFloats);

	//appLog(LOG_CAT) <<
	//	QStringLiteral("In=%1, Out=%2").arg(m_rmsVolumeIn).arg(m_rmsVolumeOut);
}
