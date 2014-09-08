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

#include "rtmptargetbase.h"
#include "application.h"
#include "audioencoder.h"
#include "audiomixer.h"
#include "avsynchronizer.h"
#include "constants.h"
#include "fdkaacencoder.h"
#include "profile.h"
#include "videoencoder.h"
#include "x264encoder.h"
#include "Widgets/infowidget.h"
#include "Widgets/targetpane.h"
#include <Libbroadcast/amf.h>

#define DUMP_STATS_TO_FILE 0
#if DUMP_STATS_TO_FILE
#include "application.h"
#include <QtCore/QFile>
#endif

const QString LOG_CAT = QStringLiteral("RTMP");
const quint64 CALC_UPLOAD_SPEED_OVER_USEC = 5000000; // 5 seconds
const quint64 CALC_STABILITY_OVER_USEC = 120000000; // 2 minutes
const quint64 RECONNECT_DELAY_MSEC = 15000; // 15 seconds

#if DUMP_STATS_TO_FILE
static QByteArray dumpBuffer;
static QString dumpFilename;
static float dumpFrameAvg;
static QVector<int> dumpVbvFrames;
#endif

//=============================================================================
// RTMPTargetBase class

RTMPTargetBase::RTMPTargetBase(
	Profile *profile, TrgtType type, const QString &name)
	: Target(profile, type, name)
	, m_videoEnc(NULL)
	, m_audioEnc(NULL)
	, m_syncer(NULL)
	, m_rtmp(NULL)
	, m_publisher(NULL)
	, m_doPadVideo(false)
	, m_logTimer(this)
	, m_lastFatalError()

	// Logging
	, m_prevDroppedPadding(0)
	, m_prevDroppedFrames(0)

	// State
	, m_syncQueue()
	, m_audioPktsInSyncQueue(0)
	, m_outQueue()
	, m_audioErrorUsec(0)
	, m_wroteHeaders(false)
	, m_deactivating(false)
	, m_isFatalDeactivate(false)
	, m_fatalErrorInProgress(false)
	, m_videoDtsOrigin(INT_MAX)
	, m_uploadStats()
	, m_dropStats()
	, m_numDroppedFrames(0)
	, m_numDroppedPadding(0)
	, m_avgVideoFrameSize(0.0f)
	, m_conSucceededOnce(false)
	, m_firstReconnectAttempt(true)
	, m_allowReconnect(false)
{
	m_syncQueue.reserve(16);
	m_outQueue.reserve(128);
	m_uploadStats.reserve(64);
	m_dropStats.reserve(64);

	// Connect signals
	connect(&m_logTimer, &QTimer::timeout,
		this, &RTMPTargetBase::logTimeout);

#if DUMP_STATS_TO_FILE
	dumpBuffer.reserve(512 * 1024); // 512KB
	dumpVbvFrames.reserve(60);
#endif
}

RTMPTargetBase::~RTMPTargetBase()
{
	m_deactivating = true;

	if(m_syncer != NULL)
		delete m_syncer;
	m_syncer = NULL;

	if(m_publisher != NULL && m_rtmp != NULL)
		m_rtmp->deletePublishStream();
	m_publisher = NULL;

	if(m_rtmp != NULL)
		delete m_rtmp;
	m_rtmp = NULL;
}

void RTMPTargetBase::rtmpInit(
	quint32 videoEncId, quint32 audioEncId, bool padVideo)
{
	// Fetch video and audio encoder
	m_videoEnc = m_profile->getVideoEncoderById(videoEncId);
	m_audioEnc = m_profile->getAudioEncoderById(audioEncId);
	if(m_videoEnc == NULL) { // No audio is okay
		appLog(LOG_CAT, Log::Warning)
			<< "Could not find video encoder " << videoEncId
			<< " for target " << getIdString();
		return;
	}

	// Watch encoders for fatal errors
	connect(m_videoEnc, &VideoEncoder::encodeError,
		this, &RTMPTargetBase::videoEncodeError);

	// Create synchroniser object and connect to its signals
	m_syncer = new AVSynchronizer(m_videoEnc, m_audioEnc);
	connect(m_syncer, &AVSynchronizer::frameReady,
		this, &RTMPTargetBase::frameReady);
	connect(m_syncer, &AVSynchronizer::segmentReady,
		this, &RTMPTargetBase::segmentReady);

	// Initialize RTMP object and connect to its signals
	m_rtmp = new RTMPClient();
	m_rtmp->setVersionString(m_rtmp->getVersionString() +
		QStringLiteral(" %1/%2").arg(APP_NAME).arg(APP_VER_STR));
	connect(m_rtmp, &RTMPClient::connected,
		this, &RTMPTargetBase::rtmpConnected);
	connect(m_rtmp, &RTMPClient::initialized,
		this, &RTMPTargetBase::rtmpInitialized);
	connect(m_rtmp, &RTMPClient::connectedToApp,
		this, &RTMPTargetBase::rtmpConnectedToApp);
	connect(m_rtmp, &RTMPClient::error,
		this, &RTMPTargetBase::rtmpError);

	// We must forward system ticks to the RTMP client if gamer mode is enabled
	connect(App, &Application::realTimeTickEvent,
		this, &RTMPTargetBase::realTimeTickEvent);

	m_doPadVideo = padVideo;
}

bool RTMPTargetBase::rtmpActivate(const RTMPTargetInfo &info)
{
	if(m_isInitializing)
		return false;
	if(m_videoEnc == NULL)
		return false;

	// Reset state
	m_syncQueue.clear();
	m_audioPktsInSyncQueue = 0;
	m_outQueue.clear();
	m_audioErrorUsec = 0;
	m_wroteHeaders = false;
	m_deactivating = false;
	m_isFatalDeactivate = false;
	m_videoDtsOrigin = INT_MAX;
	m_uploadStats.clear();
	m_dropStats.clear();
	m_numDroppedFrames = 0; // TODO: Don't reset when auto-reconnecting
	m_numDroppedPadding = 0; // TODO: Don't reset when auto-reconnecting
	m_avgVideoFrameSize =
		(float)m_videoEnc->getAvgBitrateForCongestion() * 1000.0f / 8.0f /
		m_videoEnc->getFramerate().asFloat();
	m_conSucceededOnce = false;
	m_firstReconnectAttempt = true;
	m_allowReconnect = false;

	// Set average upload speed for gamer mode throttling. Needs to be as close
	// to the actual average as possible for accurate behaviour.
	int avgUpload = m_videoEnc->getAvgBitrateForCongestion();
	if(m_audioEnc != NULL)
		avgUpload += m_audioEnc->getAvgBitrateForCongestion();
	avgUpload *= 1000; // Kb/s -> b/s (1000 for bits, 1024 for bytes)
	avgUpload /= 8; // b/s -> B/s
	m_rtmp->gamerSetAverageUpload(avgUpload);

	// Begin connection process. RTMPClient will automatically initialize the
	// RTMP connection for us based on `info`.
	m_rtmp->setRemoteTarget(info);
	if(!m_rtmp->connect())
		return false;

	// Start log timer
	m_prevDroppedPadding = 0;
	m_prevDroppedFrames = 0;
	m_logTimer.setSingleShot(false);
	m_logTimer.start(1000 * 60 * 5); // 5 minute refresh

#if DUMP_STATS_TO_FILE
	dumpBuffer.clear();
	dumpBuffer.append(
		QStringLiteral("\"PTS\",\"DTS\",\"Ex. pad\",\"Pad\",\"Inc. pad\",\"Inc. avg\"\n"));
	dumpFilename = App->getDataDirectory().filePath("RTMP stats.csv");
	dumpFilename = getUniqueFilename(dumpFilename).filePath();
	dumpFrameAvg = 0.0f;
	dumpVbvFrames.clear();
#endif

	return true;
}

void RTMPTargetBase::rtmpDeactivate()
{
	if(m_isInitializing)
		return;
	if(m_videoEnc == NULL)
		return; // Never activated

	// Prevent receiving disconnection errors
	m_deactivating = true;

	// TODO: Soft disconnect (Transmit all queued frames)

	// If the user deactives while we are trying to reconnect stop the attempt
	m_allowReconnect = false;

	// Dereference encoders by disabling the synchroniser
	m_syncer->setActive(false);

	// Cleanly disconnect from the remote host if we are not deactivating due
	// to a fatal error. RTMPClient will automatically delete our publisher for
	// us if one exists.
	m_rtmp->disconnect(!m_isFatalDeactivate);
	m_publisher = NULL;

	// Stop log timer
	m_logTimer.stop();

	// Log frame dropping statistics if we managed to fully connect
	if(m_wroteHeaders) {
		if(m_doPadVideo) {
			appLog(LOG_CAT) << QStringLiteral(
				"Total padding dropped: %1B; Total frames dropped: %2")
				.arg(humanBitsBytes(rtmpGetNumDroppedPadding(), 1))
				.arg(rtmpGetNumDroppedFrames());
		} else {
			appLog(LOG_CAT) << QStringLiteral(
				"Total frames dropped: %1")
				.arg(rtmpGetNumDroppedFrames());
		}
	}

#if DUMP_STATS_TO_FILE
	QFile dumpFile(dumpFilename);
	appLog(Log::Critical)
		<< "Dumping video stats to file: \"" << dumpFile.fileName()
		<< "\" (File size: " << dumpBuffer.size() << ")";
	if(dumpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		dumpFile.write(dumpBuffer);
		dumpFile.close();
		dumpBuffer.clear();
	}
#endif

	m_deactivating = false;
}

int RTMPTargetBase::rtmpGetUploadSpeed() const
{
	quint64 sum = 0;
	for(int i = 0; i < m_uploadStats.size(); i++)
		sum += m_uploadStats.at(i).bytesWritten;
	return sum * 1000000ULL / CALC_UPLOAD_SPEED_OVER_USEC;
}

RTMPTargetBase::StreamStability RTMPTargetBase::rtmpGetStability() const
{
	if(m_rtmp == NULL || !m_rtmp->isSocketConnected())
		return OfflineStability;
	if(m_publisher == NULL || !m_publisher->isReady())
		return OfflineStability;

	StreamStability ret = PerfectStability;
	int numDroppedFrames = 0;
	for(int i = 0; i < m_dropStats.size(); i++) {
		const DropStats &stats = m_dropStats.at(i);
		if(stats.wasPadding) {
			// Dropped padding
			ret = GoodStability; // No longer perfect
			continue;
		}
		// Dropped frame
		numDroppedFrames++;
	}
	float numFrames = m_videoEnc->getFramerate().asFloat() *
		(float)CALC_STABILITY_OVER_USEC / 1000000.0f;
	float ratioDropped = (float)numDroppedFrames / numFrames;
	//if(isActive())
	//	appLog() << "Percent dropped: " << (ratioDropped * 100.0f);
	if(ratioDropped >= 0.075f) // Over 7.5% dropped
		return TerribleStability;
	if(ratioDropped >= 0.03f) // Between 3% and 7.5% dropped
		return BadStability;
	if(numDroppedFrames) // At least one dropped frame
		return MarginalStability;
	return ret; // Perfect or dropped padding only
}

QString RTMPTargetBase::rtmpGetStabilityAsString() const
{
	switch(rtmpGetStability()) {
	default:
	case PerfectStability:
		return tr("Perfect");
	case GoodStability:
		return tr("Good");
	case MarginalStability:
		return tr("Marginal");
	case BadStability:
		return tr("Bad");
	case TerribleStability:
		return tr("Terrible");
	case OfflineStability:
		return tr("Offline");
	}
	// Should never be reached
	return tr("Unknown");
}

int RTMPTargetBase::rtmpGetNumDroppedFrames() const
{
	return m_numDroppedFrames;
}

int RTMPTargetBase::rtmpGetNumDroppedPadding() const
{
	return m_numDroppedPadding;
}

int RTMPTargetBase::rtmpGetBufferUsagePercent() const
{
	return qRound((float)calcCurrentOutputBufferSize() * 100.0f /
		(float)calcMaximumOutputBufferSize());
}

QString RTMPTargetBase::rtmpErrorToString(RTMPClient::RTMPError error) const
{
	return RTMPClient::errorToString(error);
}

void RTMPTargetBase::setupTargetPaneHelper(
	TargetPane *pane, const QString &pixmap)
{
	if(pane == NULL)
		return;
	pane->setUserEnabled(m_isEnabled);
	pane->setPaneState(
		m_isActive ? TargetPane::LiveState : TargetPane::OfflineState);
	pane->setTitle(m_name);
	pane->setIconPixmap(QPixmap(pixmap));
}

/// <summary>
/// Displays the generic RTMP information in the target pane.
/// </summary>
/// <returns>The ID of the next row that can be used.</returns>
int RTMPTargetBase::updatePaneTextHelper(
	TargetPane *pane, int offset, bool fromTimer)
{
	if(pane == NULL)
		return offset;

	pane->setItemText(
		offset++, tr("Broadcast time:"), getTimeActiveAsString(), false);
	if(fromTimer) {
		pane->setItemText(
			offset++, tr("Upload speed:"),
			tr("%1B/s").arg(humanBitsBytes(rtmpGetUploadSpeed())), false);
		pane->setItemText(
			offset++, tr("Output buffer:"),
			tr("%L1% full").arg(rtmpGetBufferUsagePercent()), false);
	} else
		offset += 2;
	if(m_doPadVideo) {
		pane->setItemText(
			offset++, tr("Dropped padding:"),
			tr("%1B").arg(humanBitsBytes(rtmpGetNumDroppedPadding(), 1)),
			false);
	}
	pane->setItemText(
		offset++, tr("Dropped frames:"),
		tr("%L1").arg(rtmpGetNumDroppedFrames()), false);
	pane->setItemText(
		offset++, tr("Stability:"),
		tr("%1").arg(rtmpGetStabilityAsString()), true);

	return offset;
}

/// <summary>
/// Sets the target pane's state based on the RTMP connection state.
/// </summary>
void RTMPTargetBase::setPaneStateHelper(TargetPane *pane)
{
	if(pane == NULL)
		return;
	if(m_publisher != NULL && m_publisher->isReady()) {
		// Connected to remote host and ready to transmit data
		pane->setPaneState(TargetPane::LiveState);
#if 0
	} else if(m_rtmp->isSocketConnected()) {
		// Connected to remote host but not ready to transmit data
		pane->setPaneState(TargetPane::LiveState);
#endif // 0
	} else
		pane->setPaneState(TargetPane::WarningState);
}

int RTMPTargetBase::setupTargetInfoHelper(
	TargetInfoWidget *widget, int offset, const QString &pixmap)
{
	widget->setTitle(m_name);
	widget->setIconPixmap(QPixmap(pixmap));

	QString setStr = tr("Error");
	if(m_videoEnc != NULL)
		setStr = m_videoEnc->getInfoString();
	widget->setItemText(offset++, tr("Video settings:"), setStr, false);

	setStr = tr("No audio");
	if(m_audioEnc != NULL)
		setStr = m_audioEnc->getInfoString();
	widget->setItemText(offset++, tr("Audio settings:"), setStr, false);

	return offset;
}

VideoEncoder *RTMPTargetBase::getVideoEncoder() const
{
	return m_videoEnc;
}

AudioEncoder *RTMPTargetBase::getAudioEncoder() const
{
	return m_audioEnc;
}

/// <summary>
/// Called whenever the target can no longer be broadcasted to and should be
/// deactivated.
/// </summary>
void RTMPTargetBase::fatalRtmpError(
	const QString &logMsg, const QString &usrMsg)
{
	if(!m_isActive)
		return; // Already deactivated

	QString userMsg = usrMsg;
	if(userMsg.isEmpty())
		userMsg = logMsg;

	appLog(LOG_CAT, Log::Warning)
		<< QStringLiteral("Fatal RTMP error: %1").arg(logMsg);
	m_lastFatalError = logMsg;
	App->setStatusLabel(QStringLiteral(
		"Network error occured for target \"%1\": %2")
		.arg(getName()).arg(userMsg));

	// Don't deactivate immediately as we might be called from within an
	// encoder output signal and if we dereference them now crashes and other
	// bad things will happen.
	m_fatalErrorInProgress = true;
	QTimer::singleShot(0, this, SLOT(delayedDeactivate()));
}

void RTMPTargetBase::delayedDeactivate()
{
	m_fatalErrorInProgress = false;
	if(!m_isActive)
		return; // Already deactivated

	// If this variable is ever true then it means that we already have a
	// reconnect attempt in progress and if we continue then we'll enter an
	// invalid state
	if(m_allowReconnect)
		return;

	// If we successfully connected to the target once before then assume that
	// the error was a network issue and that we should attempt to reconnect,
	// otherwise we should just deactivate completely.
	if(m_conSucceededOnce) {
		// Attempt to reconnect
		bool wasFirstAttempt = m_firstReconnectAttempt;
		m_isFatalDeactivate = true;
		rtmpDeactivate();
		m_isFatalDeactivate = false;
		if(wasFirstAttempt) {
			appLog(LOG_CAT) << QStringLiteral(
				"Attempting to automatically reconnect to remote host...");
			// Do an immediate reconnect attempt on the first time only
			if(!rtmpActivate(m_rtmp->getRemoteTarget())) {
				appLog(LOG_CAT) << QStringLiteral(
					"Failed to reconnect to remote host, waiting to try again");
				m_allowReconnect = true;
				QTimer::singleShot(
					RECONNECT_DELAY_MSEC, this, SLOT(delayedReconnect()));
			}
		} else {
			// Our first reconnect attempt fail, don't flood the remote server
			// by delaying later reconnect attempts
			appLog(LOG_CAT) << QStringLiteral(
				"Failed to reconnect to remote host, waiting to try again");
			m_allowReconnect = true;
			QTimer::singleShot(
				RECONNECT_DELAY_MSEC, this, SLOT(delayedReconnect()));
		}
		m_conSucceededOnce = true; // Keep through state resets
		m_firstReconnectAttempt = false; // No longer the first attempt
	} else {
		// Deactivate immediately
		m_isFatalDeactivate = true;
		setActive(false);
		m_isFatalDeactivate = false;
	}

	emit rtmpStateChanged(true);
}

void RTMPTargetBase::delayedReconnect()
{
	if(!m_allowReconnect)
		return;
	m_allowReconnect = false;
	appLog(LOG_CAT) << QStringLiteral(
		"Attempting to automatically reconnect to remote host...");
	if(!rtmpActivate(m_rtmp->getRemoteTarget())) {
		m_allowReconnect = true;
		QTimer::singleShot(
			RECONNECT_DELAY_MSEC, this, SLOT(delayedReconnect()));
	}
	m_conSucceededOnce = true; // Keep through state resets
	m_firstReconnectAttempt = false; // If delayed reconnect then keep delayed
}

/// <summary>
/// Write the required video and audio headers to the stream before any actual
/// video frames or audio segments are written.
/// </summary>
/// <returns>True if the headers were successfully written</returns>
bool RTMPTargetBase::writeHeaders()
{
	// We must transmit both the RTMP "@setDataFrame()" message and the video
	// and audio headers before any actual video frames or audio segments are
	// transmitted. To do this we wait until we have one of each type queued
	// and transmit them both together.

	EncodedFrame frame; // First video frame
	EncodedPacket audioPkt; // First audio frame
	for(int i = 0; i < m_outQueue.size(); i++) {
		const QueuedData &data = m_outQueue.at(i);
		if(!frame.isValid() && data.isFrame)
			frame = data.frame;
		else if(!audioPkt.isValid() && !data.isFrame)
			audioPkt = data.audioPkt;
		if(frame.isValid() && audioPkt.isValid())
			break;
	}
	if(!frame.isValid() || (m_audioEnc != NULL && !audioPkt.isValid())) {
		// We cannot transmit the headers yet, wait until later
		//appLog() << "Waiting for frames from both streams";
		return false;
	}
	// Headers are ready to be transmitted

	// Write the following with the least amount of network packets possible
	m_publisher->beginForceBufferWrite();

	//-------------------------------------------------------------------------
	// Transmit the RTMP "@setDataFrame()" message

	AMFObject obj;

	// Video information
	obj["width"] = new AMFNumber((double)m_videoEnc->getSize().width());
	obj["height"] = new AMFNumber((double)m_videoEnc->getSize().height());
	obj["framerate"] =
		new AMFNumber((double)m_videoEnc->getFramerate().asFloat());
	if(m_videoEnc->getType() == VencX264Type) {
		X264Encoder *enc = static_cast<X264Encoder *>(m_videoEnc);
		int keyInt = enc->getKeyInterval();
		if(keyInt == 0)
			keyInt = DEFAULT_KEYFRAME_INTERVAL;
		obj["videocodecid"] = new AMFString("avc1");
		obj["videodatarate"] = new AMFNumber((double)enc->getBitrate());
		// "videokeyframe_frequency" is actually the keyframe interval and not
		// the frequency
		obj["videokeyframe_frequency"] = new AMFNumber((double)keyInt);
		//obj["avclevel"] = new AMFNumber(0.0); // TODO
		//obj["avcprofile"] = new AMFNumber(0.0); // TODO
	} else {
		fatalRtmpError("Unknown video encoder");
		goto exitHeader1;
	}

	// Audio information
	if(m_audioEnc != NULL) {
		AudioMixer *mixer = m_profile->getAudioMixer();
		obj["audiosamplerate"] =
			new AMFNumber((double)m_audioEnc->getSampleRate());
		obj["audiochannels"] = new AMFNumber((double)mixer->getNumChannels());
		if(m_audioEnc->getType() == AencFdkAacType) {
			FdkAacEncoder *enc = static_cast<FdkAacEncoder *>(m_audioEnc);
			obj["audiocodecid"] = new AMFString("mp4a");
			obj["audiodatarate"] = new AMFNumber((double)enc->getBitrate());
		} else {
			fatalRtmpError("Unknown audio encoder");
			goto exitHeader1;
		}
	}

	// Extra information
	obj["encoder"] =
		new AMFString(QStringLiteral("%1/%2").arg(APP_NAME).arg(APP_VER_STR));
	//obj["duration"] = new AMFNumber(0.0);
	//obj["filesize"] = new AMFNumber(0.0);

	if(!m_publisher->writeDataFrame(&obj)) {
		fatalRtmpError("Failed to write data frame");
		goto exitHeader1;
	}

	//-------------------------------------------------------------------------
	// Transmit video header

	if(m_videoEnc->getType() == VencX264Type) {
		// Video stream is H.264, transmit the "AVC decoder configuration
		// record". To do that we first need to find the SPS and PPS NALs.
		const EncodedPacketList &pkts = frame.getPackets();
		EncodedPacket sps, pps;
		for(int i = 0; i < pkts.count(); i++) {
			EncodedPacket pkt = pkts.at(i);
			if(pkt.ident() == PktH264Ident_SPS)
				sps = pkt;
			if(pkt.ident() == PktH264Ident_PPS)
				pps = pkt;
			if(sps.isValid() && pps.isValid())
				break;
		}
		if(!sps.isValid() || !pps.isValid()) {
			// Invalid H.264 stream. The first frame should always be an IDR
			// frame that is prefixed by both an SPS and PPS NAL unit.
			fatalRtmpError("Cannot find SPS and PPS NAL units");
			goto exitHeader1;
		}

		// Actually write the record
		if(!m_publisher->writeAvcConfigRecord(sps.data(), pps.data())) {
			fatalRtmpError("Failed to write AVC configuration record");
			goto exitHeader1;
		}
	} else {
		fatalRtmpError("Unknown video encoder");
		goto exitHeader1;
	}

	//-------------------------------------------------------------------------
	// Transmit audio header if one exists

	if(m_audioEnc != NULL) {
		if(m_audioEnc->getType() == AencFdkAacType) {
			// Audio stream is AAC-LC
			FdkAacEncoder *enc = static_cast<FdkAacEncoder *>(m_audioEnc);
			if(!m_publisher->writeAacSequenceHeader(enc->getOutOfBand())) {
				fatalRtmpError("Failed to write AAC sequence header");
				goto exitHeader1;
			}
		} else {
			fatalRtmpError("Unknown audio encoder");
			goto exitHeader1;
		}
	}

	//-------------------------------------------------------------------------

	m_publisher->endForceBufferWrite();

	return true;

	// Error handling
exitHeader1:
	m_publisher->endForceBufferWrite();
	return false;
}

/// <returns>Approximate bytes written to the network buffer.</returns>
int RTMPTargetBase::writeH264Frame(EncodedFrame frame, int bytesPadding)
{
	// Create FLV "VideoTagHeader" structure
	char header[5];
	if(frame.isKeyframe())
		header[0] = 0x17; // AVC keyframe
	else
		header[0] = 0x27; // AVC interframe
	header[1] = 0x01; // AVC NALU
	// Composition time = PTS - DTS in msec
	Fraction timeBase = m_videoEnc->getTimeBase();
	amfEncodeUInt24(&header[2],
		(quint64)(frame.getPTS() - frame.getDTS()) *
		(quint64)timeBase.numerator * 1000ULL / (quint64)timeBase.denominator);
	QByteArray headerOut = QByteArray::fromRawData(header, sizeof(header));

	// Get raw NAL unit data
	int rawSize = 0;
	QVector<QByteArray> rawPkts;
	const EncodedPacketList &pkts = filterFramePackets(frame);
	for(int i = 0; i < pkts.size(); i++) {
		const EncodedPacket &pkt = pkts.at(i);
		rawPkts.append(pkt.data());
		rawSize += pkt.data().size();
	}

	// Append padding NAL. The H.264 "filler" NAL has the following structure:
	//   - NAL header = 0x0C:
	//       - 1 bit forbidden zero = 0b0
	//       - 2 bits for `nal_ref_idc` = 0b00
	//       - 5 bits for `nal_unit_type` = 0b01100
	//   - Zero or more 0xFF
	//   - 0x80
	const int FILLER_OVERHEAD = 2;
	bool padAdded = false;
	if(bytesPadding < FILLER_OVERHEAD)
		bytesPadding = 0;
	if(bytesPadding > 0) {
		QByteArray filler(bytesPadding, (char)0xFF);
		filler[0] = (char)0x0C; // NAL header
		filler[filler.size()-1] = (char)0x80; // Tailing bits
		rawPkts.append(filler);
		padAdded = true;
	}

	// Actually write the frame
	bool wrote = m_publisher->writeVideoFrame(
		calcVideoTimestampFromDts(frame.getDTS()), headerOut, rawPkts);
	if(!wrote) {
		// If we failed to write the frame assume it is because the remote host
		// closed the connection (Qt doesn't notify us if this happens)
		fatalRtmpError(
			"Failed to write video frame", "Remote host closed connection");
		return 0;
	}

	// Debugging stuff
#if DUMP_STATS_TO_FILE
	int exSize = 0;
	int incSize = 0;
	for(int i = 0; i < rawPkts.size(); i++) {
		if(!padAdded || (padAdded && i < rawPkts.size() - 1))
			exSize += rawPkts.at(i).size();
		incSize += rawPkts.at(i).size();
	}
	if(padAdded)
		bytesPadding = rawPkts.at(rawPkts.size() - 1).size();

	// Running average
	const float DUMP_AVG_NUM_FRAMES = m_videoEnc->getFramerate().asFloat();
	dumpFrameAvg = (dumpFrameAvg * (DUMP_AVG_NUM_FRAMES - 1.0f) +
		(float)incSize) / DUMP_AVG_NUM_FRAMES;

	dumpBuffer.append(QStringLiteral("%1,%2,%3,%4,%5,%6\n")
		.arg(frame.getPTS())
		.arg(frame.getDTS())
		.arg(exSize)
		.arg(bytesPadding)
		.arg(incSize)
		.arg(dumpFrameAvg));
#endif

	// Doesn't take into account RTMP overheads
	return sizeof(header) + rawSize + bytesPadding;
}

/// <returns>Approximate bytes written to the network buffer.</returns>
int RTMPTargetBase::writeAACFrame(EncodedPacket pkt, qint64 pts)
{
	// Create FLV "AudioTagHeader" structure
	char header[2];
	header[0] = (char)0xAF; // AAC format (Constant)
	header[1] = (char)0x01; // 0 = AAC sequence header, 1 = AAC data
	QByteArray headerOut = QByteArray::fromRawData(header, sizeof(header));

	// Actually write the frame
	bool wrote = m_publisher->writeAudioFrame(
		calcAudioTimestampFromPts(pts), headerOut, pkt.data());
	if(!wrote) {
		// If we failed to write the frame assume it is because the remote host
		// closed the connection (Qt doesn't notify us if this happens)
		fatalRtmpError(
			"Failed to write audio frame", "Remote host closed connection");
		return 0;
	}

	// Doesn't take into account RTMP overheads
	return sizeof(header) + pkt.data().size();
}

/// <summary>
/// Moves synchronised video and audio frames to the output queue.
/// </summary>
void RTMPTargetBase::processSyncQueue()
{
	while(m_syncQueue.size()) {
		if(m_audioPktsInSyncQueue < 1)
			break; // Must always have 1 audio frame to guarentee order
		const QueuedData &data = m_syncQueue.at(0);
		m_outQueue.append(data);
		m_syncQueue.remove(0);
		if(!data.isFrame)
			m_audioPktsInSyncQueue--;
	}
}

/// <summary>
/// Drops the closest audio frame to the specified video frame.
/// </summary>
/// <returns>The amount of bytes dropped</returns>
int RTMPTargetBase::dropBestAudioFrame(int droppedVideoFrame)
{
	if(m_audioEnc == NULL)
		return 0; // No audio to drop
	int bytesDropped = 0;

	// Calculate how many audio frames we need to drop to resynchronise with
	// the video. TODO: This might be susceptible to drift over large periods
	// of time due to rounding.
	Fraction timebase = m_audioEnc->getTimeBase();
	uint timebaseUsec = 1000000U * timebase.numerator / timebase.denominator;
	int numFrames = m_audioErrorUsec / timebaseUsec;
	if(numFrames == 0)
		return 0; // Still synchronised

	while(numFrames) {
		numFrames--;
		int bestDrop = -1;

		// Find the first non-dropped audio frame after the dropped video frame
		for(int i = droppedVideoFrame; i < m_outQueue.size(); i++) {
			const QueuedData &data = m_outQueue.at(i);
			if(data.isFrame)
				continue;
			if(data.isDropped)
				continue;
			bestDrop = i;
			break;
		}
		if(bestDrop != -1) {
			// Found a frame to drop, do so
			m_outQueue[bestDrop].isDropped = true;
			bytesDropped += m_outQueue[bestDrop].audioPkt.data().size();
			m_audioErrorUsec -= timebaseUsec;
			continue;
		}

		// If no frame was found then find the first non-dropped audio frame
		// before the dropped video frame.
		for(int i = droppedVideoFrame; i >= 0; i--) {
			const QueuedData &data = m_outQueue.at(i);
			if(data.isFrame)
				continue;
			if(data.isDropped)
				continue;
			bestDrop = i;
			break;
		}
		if(bestDrop != -1) {
			// Found a frame to drop, do so
			m_outQueue[bestDrop].isDropped = true;
			bytesDropped += m_outQueue[bestDrop].audioPkt.data().size();
			m_audioErrorUsec -= timebaseUsec;
			continue;
		}

		// No frame was found for dropping. As we only ever attempt to drop
		// audio when a video frame is dropped if we get here then the audio
		// could potentially remain out-of-sync for a while. Log a warning.
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to find an audio frame to drop, potentially lost synchronisation";
		break;
	}

	return bytesDropped;
}

/// <summary>
/// Drops a single frame of the specified type. Also drops an audio frame if
/// it's too far out-of-sync.
/// </summary>
/// <returns>The amount of bytes dropped</returns>
int RTMPTargetBase::dropBestFrame(FrmPriority priority)
{
	// The algorithm used before is described in `processFrameDropping()`.

	int bytesDropped = 0;

	// "Search from the end of the queue to the start for any frame of the same
	// type is already dropped and remember the first one found."
	int bestDrop = -1;
	int foundDropped = -1;
	for(int i = m_outQueue.size() - 1; i >= 0; i--) {
		const QueuedData &data = m_outQueue.at(i);
		if(!data.isFrame)
			continue;
		if(data.frame.getPriority() != priority)
			continue;
		if(bestDrop == -1)
			bestDrop = i;
		if(data.isDropped) {
			foundDropped = i;
			break;
		}
	}

	// "If no frames of the same type were already dropped then drop the last
	// frame of that type in the queue."
	if(foundDropped == -1)
		goto bestFrameDrop;

	// "If a frame was found then find the first frame of the same type before
	// it that is not already dropped."
	bestDrop = -1;
	for(int i = foundDropped; i >= 0; i--) {
		const QueuedData &data = m_outQueue.at(i);
		if(!data.isFrame)
			continue;
		if(data.frame.getPriority() != priority)
			continue;
		if(data.isDropped)
			continue;
		bestDrop = i;
		break;
	}
	if(bestDrop != -1)
		goto bestFrameDrop; // Found a frame to drop

	// "If all frames before the found one are already dropped then find the
	// first frame of the same type after it that is not already dropped."
	for(int i = foundDropped; i < m_outQueue.size(); i++) {
		const QueuedData &data = m_outQueue.at(i);
		if(!data.isFrame)
			continue;
		if(data.frame.getPriority() != priority)
			continue;
		if(data.isDropped)
			continue;
		bestDrop = i;
		break;
	}

bestFrameDrop:
	// "If any frame was found then mark it as dropped and restart the
	// algorithm beginning at the same type otherwise move on to the next
	// type."
	if(bestDrop == -1)
		return 0; // No frames of this type at all, move to the next type
	m_outQueue[bestDrop].isDropped = true;
	bytesDropped += calcSizeOfPackets(filterFramePackets(
		m_outQueue[bestDrop].frame));

	// Resynchronise the audio if required by dropping one or more audio frames
	// TODO: This might be susceptible to drift over large periods of time due
	// to rounding.
	Fraction timebase = m_videoEnc->getTimeBase();
	m_audioErrorUsec +=
		1000000ULL * timebase.numerator / timebase.denominator;
	bytesDropped += dropBestAudioFrame(bestDrop);

	return bytesDropped;
}

/// <summary>
/// Monitors the output queue and if too much data is buffered then determines
/// the best way to reduce the queue and does so. Video padding is the first to
/// go, then B frames, then P frames and then I frames. Takes into account
/// audio synchronisation when dropping video frames.
/// </summary>
void RTMPTargetBase::processFrameDropping()
{
	// Frame dropping is an iterative process where a target video frame is
	// found and marked as dropped, calculating the new audio offset error, if
	// the offset error is greater than the audio frame time base then the
	// closest audio frame to the dropped video frame is also dropped, and then
	// the entire algorithm iterates until the output buffer is of suitable
	// length.
	//
	// To limit the amount of viewer disturbance the data with the least amount
	// of influence on the stream is dropped before the data that has a greater
	// influence is dropped. The general order of dropping is: Video padding,
	// B-frames, P-frames and then I-frames. As video encoding can result in
	// more frame types that just these (Reference B-frames for example) we
	// rely on the encoder to determine the dropping order for us (See the
	// frame priority flag). The algorithm for each type is as follows:
	//
	// Search from the end of the queue to the start for any frame of the same
	// type is already dropped and remember the first one found. If no frames
	// of the same type were already dropped then drop the last frame of that
	// type in the queue. If a frame was found then find the first frame of the
	// same type before it that is not already dropped. If all frames before
	// the found one are already dropped then find the first frame of the same
	// type after it that is not already dropped. If any frame was found then
	// mark it as dropped and restart the algorithm beginning at the same type
	// otherwise move on to the next type.

	// Determine maximum buffer size before we need to start dropping data
	int targetBufSize = calcMaximumOutputBufferSize();

	// Calculate current output buffer size. We ignore RTMP overhead.
	int bufSize = calcCurrentOutputBufferSize();

	//appLog() << QStringLiteral("Target output buffer size=%L1, Current=%L2")
	//	.arg(targetBufSize).arg(bufSize);

	// Only continue if we need to drop frames
	if(bufSize <= targetBufSize)
		return;

	//-------------------------------------------------------------------------
	// Drop padding as a separate step as it's not a part of the standard video
	// frames. We should always only drop the minimum amount of padding
	// required in an attempt to keep a "strict CBR" state even in a network
	// congested environment. We don't really care where the padding is removed
	// from in the queue.

	int i = -1;
	int droppedBytesTotal = 0;
	while(bufSize > targetBufSize) {
		i++;
		if(i >= m_outQueue.size())
			break; // No more padding anywhere
		const QueuedData &data = m_outQueue.at(i);
		if(!data.isFrame)
			continue;
		if(data.bytesPadding <= 0)
			continue;

		// Found some padding to drop, do so
		int droppedBytes = qMin(data.bytesPadding, bufSize - targetBufSize);
		m_outQueue[i].bytesPadding -= droppedBytes;
		bufSize -= droppedBytes;
		droppedBytesTotal += droppedBytes;
		m_numDroppedPadding += droppedBytes;

		// Record statistics so we can calculate stability
		DropStats stats;
		stats.timestamp = App->getUsecSinceExec();
		stats.wasPadding = true;
		stats.bytesDropped = droppedBytes;
		m_dropStats.append(stats);
	}

	// Emit padding dropped signal
	if(droppedBytesTotal > 0) {
#if 0
		appLog() << QStringLiteral("Dropped %L1 bytes of padding")
			.arg(droppedBytesTotal);
#endif // 0

		emit rtmpDroppedPadding(droppedBytesTotal);
	}

	//-------------------------------------------------------------------------

	// Only continue if we need to drop frames
	if(bufSize <= targetBufSize)
		return;

	// Increase the viewing experience slightly by applying hysteresis to the
	// dropping algorithm so that frame dropping occurs in chunks instead of
	// being consistent. We do this after dropping padding as padding doesn't
	// affect the viewer in any way.
	const float HYSTERESIS_AMOUNT = 0.8f;
	targetBufSize = (int)((float)targetBufSize * HYSTERESIS_AMOUNT);

	// Drop frames while recording the number of dropped frames of each type.
	int numDropped[NUM_FRAME_PRIORITIES];
	int numDroppedTotal = 0;
	for(int prior = 0; prior < NUM_FRAME_PRIORITIES; prior++)
		numDropped[prior] = 0;
	for(int prior = 0; prior < NUM_FRAME_PRIORITIES; prior++) {
		FrmPriority priority = (FrmPriority)prior;
		bool breakLoop = true;
		while(bufSize > targetBufSize) {
			//appLog()
			//	<< QStringLiteral("Attempting to drop frame. Target=%L1, Current=%L2")
			//	.arg(targetBufSize).arg(bufSize);
			int droppedBytes = dropBestFrame(priority);
			if(droppedBytes == 0) {
				// No more frames of this type in the buffer, move on to the
				// next type
				breakLoop = false;
				break;
			}
			bufSize -= droppedBytes;
			numDropped[prior]++;
			numDroppedTotal++;
			m_numDroppedFrames++;

			// Record statistics so we can calculate stability
			DropStats stats;
			stats.timestamp = App->getUsecSinceExec();
			stats.wasPadding = false;
			stats.bytesDropped = droppedBytes;
			m_dropStats.append(stats);
		}
		if(breakLoop)
			break;
	}

	// Emit frame dropped signal
	if(numDroppedTotal > 0) {
#if 0
		// Debug more detailed statistics
		Q_ASSERT(NUM_FRAME_PRIORITIES == 4);
		appLog() << QStringLiteral("Dropped frames: %1, %2, %3, %4")
			.arg(numDropped[0])
			.arg(numDropped[1])
			.arg(numDropped[2])
			.arg(numDropped[3]);
#endif // 0

		emit rtmpDroppedFrames(numDroppedTotal);
	}
}

/// <summary>
/// Write as much video and audio data as possible to the RTMP publisher
/// object keeping everything in sync.
/// </summary>
void RTMPTargetBase::writeToPublisher()
{
	if(!m_isActive)
		return;

	// Move data from the synchronisation queue to the output queue. We have
	// two queues so that the frame dropping algorithm doesn't need to worry
	// about input synchronisation. The output queue will always have data
	// appended to the end and never inserted.
	processSyncQueue();

	// Headers must be written before any actual video frames or audio segments
	// are written
	if(!m_wroteHeaders) {
		if(!writeHeaders())
			return; // Cannot continue until the headers are written
		m_wroteHeaders = true;
	}

	// Handle network congestion
	processFrameDropping();

#define DO_HACKY_FAKE_CONGESTION 0
#if DO_HACKY_FAKE_CONGESTION
	// Test network congestion by randomly deciding that the output buffer is
	// full
	static bool hackDroppingFrames = false;
	if(hackDroppingFrames) {
		if(rand() < RAND_MAX * 0.00055f) {
			hackDroppingFrames = false;
			appLog() << "Stopping fake network congestion";
		}
	} else {
		if(rand() < RAND_MAX * 0.002f) {
			hackDroppingFrames = true;
			appLog() << "Starting fake network congestion";
		}
	}
#endif // DO_HACKY_FAKE_CONGESTION

	// Write queued data to the socket one at a time so we make sure the least
	// amount of data is buffered by the OS so we can more easily detect
	// network congestion.
	int numWrote = 0;
	int bytesWritten = 0;
	while(m_outQueue.size()) {
#if DO_HACKY_FAKE_CONGESTION
		if(hackDroppingFrames && rand() < RAND_MAX * 0.6f)
			break;
#endif // DO_HACKY_FAKE_CONGESTION
		if(m_publisher == NULL || m_publisher->willWriteBuffer())
			break; // OS buffer is full
		if(m_fatalErrorInProgress)
			break; // We are shutting down, don't write anything to the network
		const QueuedData &data = m_outQueue.at(0);
		if(data.isDropped) {
			// Don't output dropped frames
			m_outQueue.remove(0);
			continue;
		}
		if(data.isFrame) {
			//appLog() << data.frame.getDebugString();
			if(m_videoEnc->getType() == VencX264Type)
				bytesWritten += writeH264Frame(data.frame, data.bytesPadding);
			else {
				fatalRtmpError("Unknown video encoder");
				return;
			}
		} else {
			//appLog() << QStringLiteral("Audio frame: PTS=%L1")
			//	.arg(data.audioPts);
			if(m_audioEnc->getType() == AencFdkAacType)
				bytesWritten += writeAACFrame(data.audioPkt, data.audioPts);
			else {
				fatalRtmpError("Unknown audio encoder");
				return;
			}
		}
		m_outQueue.remove(0);
		numWrote++;
	}
	//appLog(LOG_CAT)
	//	<< QStringLiteral("Wrote %1 packets (%L2 bytes) to network")
	//	.arg(numWrote).arg(bytesWritten);

	// Record bytes written to the network so we can determine upload speed
	// after trimming the statistics log
	quint64 now = App->getUsecSinceExec();
	int numToTrim = 0;
	for(; numToTrim < m_uploadStats.size(); numToTrim++) {
		const WriteStats &stats = m_uploadStats.at(numToTrim);
		if(CALC_UPLOAD_SPEED_OVER_USEC > now)
			break; // App hasn't even been running for that long
		if(stats.timestamp >= now - CALC_UPLOAD_SPEED_OVER_USEC)
			break;
	}
	if(numToTrim > 0)
		m_uploadStats.remove(0, numToTrim);
	WriteStats stats;
	stats.timestamp = now;
	stats.bytesWritten = bytesWritten;
	m_uploadStats.append(stats);

	// Trim drop statistics. Semi-HACK as this doesn't really belong here
	numToTrim = 0;
	for(; numToTrim < m_dropStats.size(); numToTrim++) {
		const DropStats &stats = m_dropStats.at(numToTrim);
		if(CALC_STABILITY_OVER_USEC > now)
			break; // App hasn't even been running for that long
		if(stats.timestamp >= now - CALC_STABILITY_OVER_USEC)
			break;
	}
	if(numToTrim > 0)
		m_dropStats.remove(0, numToTrim);
}

/// <summary>
/// Returns the list of packets from a frame that should actually be
/// transmitted to the remote host.
/// </summary>
EncodedPacketList RTMPTargetBase::filterFramePackets(EncodedFrame frame) const
{
	EncodedPacketList newPkts;
	if(m_videoEnc->getType() == VencX264Type) {
		const EncodedPacketList &pkts = frame.getPackets();
		for(int i = 0; i < pkts.size(); i++) {
			const EncodedPacket &pkt = pkts.at(i);
			switch(pkt.ident()) {
			case PktH264Ident_SLICE:
			case PktH264Ident_SLICE_IDR:
			case PktH264Ident_SEI:
			case PktH264Ident_FILLER:
				// We should write this packet to the socket
				newPkts.append(pkt);
				break;
			default:
				// Don't write this packet to the socket
				continue;
			}
		}
	} else
		newPkts = frame.getPackets();
	return newPkts;
}

/// <summary>
/// Returns the sum of the sizes of the packets in the specified list.
/// </summary>
int RTMPTargetBase::calcSizeOfPackets(const EncodedPacketList &pkts) const
{
	int size = 0;
	for(int i = 0; i < pkts.size(); i++) {
		const EncodedPacket &pkt = pkts.at(i);
		size += pkt.data().size();
	}
	return size;
}

/// <summary>
/// Returns the maximum size of our output buffer before we begin dropping
/// data.
/// </summary>
int RTMPTargetBase::calcMaximumOutputBufferSize() const
{
	// The buffer size is a balancing act of limiting the amount of frame drops
	// due to short drops in network throughput and the maximum amount of delay
	// our service provider can handle before causing the end-user's player to
	// buffer. We also need to make sure that we don't drop frames through the
	// normal usage because of the variable size of each frame.

	// The minimum number of seconds to be buffered before dropping frames.
	const float DROP_TRIGGER_MIN_SECS = 2.0f;

	// Set the buffer size to be the maximum of the above constant or 75% of
	// the keyframe interval
	int keyInt = 0;
	if(m_videoEnc->getType() == VencX264Type) {
		X264Encoder *enc = static_cast<X264Encoder *>(m_videoEnc);
		keyInt = enc->getKeyInterval();
	}
	if(keyInt == 0)
		keyInt = DEFAULT_KEYFRAME_INTERVAL;
	float dropBufLength = qMax(DROP_TRIGGER_MIN_SECS, (float)keyInt * 0.75f);

	int bufSize = m_videoEnc->getAvgBitrateForCongestion();
	if(m_audioEnc != NULL)
		bufSize += m_audioEnc->getAvgBitrateForCongestion();
	bufSize *= 1000; // Kb/s -> b/s (1000 for bits, 1024 for bytes)
	bufSize /= 8; // b/s -> B/s
	return (int)((float)bufSize * dropBufLength);
}

/// <summary>
/// Returns the current amount of data in our output buffer.
/// </summary>
int RTMPTargetBase::calcCurrentOutputBufferSize() const
{
	// Calculate current output buffer size. We ignore RTMP overhead.
	int bufSize = 0;
	for(int i = 0; i < m_outQueue.size(); i++) {
		const QueuedData &data = m_outQueue.at(i);
		if(data.isDropped)
			continue; // Already dropped
		if(data.isFrame)
			bufSize += calcSizeOfPackets(filterFramePackets(data.frame));
		else
			bufSize += data.audioPkt.data().size();
		bufSize += data.bytesPadding;
	}
	return bufSize;
}

/// <summary>
/// Returns the timestamp of a video frame based on its DTS as the amount of
/// milliseconds since the start of the stream.
/// </summary>
quint32 RTMPTargetBase::calcVideoTimestampFromDts(qint64 dts)
{
	//appLog() << "Frame DTS = " << dts;
	Fraction timebase = m_videoEnc->getTimeBase();
	quint64 relDts = dts - m_videoDtsOrigin; // First frame has a DTS = 0
	return relDts * 1000ULL * (quint64)timebase.numerator /
		(quint64)timebase.denominator;
}

/// <summary>
/// Returns the timestamp of an audio frame based on its PTS as the amount of
/// milliseconds since the start of the stream.
/// </summary>
quint32 RTMPTargetBase::calcAudioTimestampFromPts(qint64 pts)
{
	//appLog() << "Audio PTS = " << pts;
	// The synchroniser guarentees that our first segment has a PTS of 0
	Fraction timebase = m_audioEnc->getTimeBase();
	return (quint64)pts * 1000ULL * (quint64)timebase.numerator /
		(quint64)timebase.denominator;
}

void RTMPTargetBase::realTimeTickEvent(int numDropped, int lateByUsec)
{
	// Forward system ticks to the RTMP client if gamer mode is enabled
	m_rtmp->gamerTickEvent(numDropped);
}

void RTMPTargetBase::frameReady(EncodedFrame frame)
{
	if(!m_isActive)
		return;
	//appLog() << frame.getDebugString();

	// If this is the first frame ever received record its DTS so we can give
	// this frame a timestamp of 0 and properly queue audio frames.
	if(m_videoDtsOrigin == INT_MAX) {
		m_videoDtsOrigin = frame.getDTS();
		//appLog()
		//	<< QStringLiteral("First video DTS=%1").arg(m_videoDtsOrigin);
	}

	// Create queue data structure
	frame.makePersistent();
	QueuedData data;
	data.isFrame = true;
	data.isDropped = false;
	data.frame = frame;
	data.bytesPadding = 0;

	//-------------------------------------------------------------------------
	// Calculate padding amount

	int rawSize = calcSizeOfPackets(filterFramePackets(frame));
	data.bytesPadding = 0;
	if(m_doPadVideo) {
		// If we are currently under the target average bitrate then fill in
		// the gap.
		float trgtFrmSize =
			(float)m_videoEnc->getAvgBitrateForCongestion() * 1000.0f / 8.0f /
			m_videoEnc->getFramerate().asFloat();
		data.bytesPadding = (int)((trgtFrmSize - m_avgVideoFrameSize) *
			m_videoEnc->getFramerate().asFloat());
		data.bytesPadding -= rawSize;
		if(data.bytesPadding < 0)
			data.bytesPadding = 0;

		// Update running average over the keyframe interval range or 2
		// seconds, whichever is smaller. The purpose of strict CBR is to make
		// sure that the TCP socket doesn't reduce its window size during low
		// bitrate periods but some broadcast services such as Twitch most
		// likely test for compliance by checking "HLS fragment" size variation
		// which is based on the keyframe interval.
		// TODO: Should we use VBV buffer size instead?
		int keyInt = 0;
		if(m_videoEnc->getType() == VencX264Type) {
			X264Encoder *enc = static_cast<X264Encoder *>(m_videoEnc);
			keyInt = enc->getKeyInterval();
		}
		if(keyInt == 0)
			keyInt = DEFAULT_KEYFRAME_INTERVAL;
		if(keyInt < 2)
			keyInt = 2;
		const float AVG_FRAMES =
			m_videoEnc->getFramerate().asFloat() * (float)keyInt;
		m_avgVideoFrameSize = (m_avgVideoFrameSize * (AVG_FRAMES - 1.0f) +
			(float)(rawSize + data.bytesPadding)) / AVG_FRAMES;
	}

	//-------------------------------------------------------------------------

	// Queue frame and attempt to write it to the network if possible
	m_syncQueue.append(data);
	writeToPublisher();
}

void RTMPTargetBase::segmentReady(EncodedSegment segment)
{
	if(!m_isActive)
		return;
	//appLog() << segment.getDebugString();

	// As we shift the DTS of the video so the first frame has a DTS of 0 we
	// need to resynchronise the audio to the new DTS timestamps of the video.
	// As each audio segment has multiple audio frames we need to split the
	// frames from the container. In order to guarentee correct transmit order
	// we need to always have a single audio frame in the queue.
	// TODO/HACK: Does this cause noticeable audio lag? Large B-frame counts?

	// Queue segment and attempt to write it to the network if possible
	segment.makePersistent();
	QueuedData data;
	data.isFrame = false;
	data.isDropped = false;
	data.bytesPadding = 0;
	const EncodedPacketList &pkts = segment.getPackets();
	for(int i = 0; i < pkts.size(); i++) {
		const EncodedPacket &pkt = pkts.at(i);
		data.audioPkt = pkt;
		data.audioPts = segment.getPTS() + i;

		// Find position in queue to insert the audio frame. We insert before
		// the first video fame that has a later timestamp than us. We never
		// need to check audio timestamps as our synchroniser guarentees that
		// they will always be in order.
		quint32 timestamp = calcAudioTimestampFromPts(data.audioPts);
		int j = 0;
		for(; j < m_syncQueue.size(); j++) {
			const QueuedData &item = m_syncQueue.at(j);
			if(!item.isFrame)
				continue;
			quint32 frameTimestamp =
				calcVideoTimestampFromDts(item.frame.getDTS());
			//appLog() << QStringLiteral("Audio TS=%L1, Video TS=%L2")
			//	.arg(timestamp).arg(frameTimestamp);
			if(frameTimestamp > timestamp)
				break; // Insert before this item
		}
		//appLog()
		//	<< QStringLiteral("Inserting audio PTS=%1 before %2 (Size=%3)")
		//	.arg(data.audioPts).arg(j).arg(m_syncQueue.size());
		m_syncQueue.insert(j, data);
		m_audioPktsInSyncQueue++;
	}
	writeToPublisher();
}

void RTMPTargetBase::videoEncodeError(const QString &error)
{
	if(!isActive())
		return;
	// Immediately stop encoding and notify the user
	App->setStatusLabel(error);
	QTimer::singleShot(0, this, SLOT(delayedSimpleDeactivate()));
}

void RTMPTargetBase::delayedSimpleDeactivate()
{
	if(!isActive())
		return;
	setActive(false);
}

void RTMPTargetBase::logTimeout()
{
	// Query stats
	int uploadSpeed = rtmpGetUploadSpeed();
	int droppedPadding = rtmpGetNumDroppedPadding();
	int droppedFrames = rtmpGetNumDroppedFrames();

	// Log stats
	int relDroppedPadding = droppedPadding - m_prevDroppedPadding;
	int relDroppedFrames = droppedFrames - m_prevDroppedFrames;
	if(m_doPadVideo) {
		appLog(LOG_CAT) << QStringLiteral(
			"Current upload speed = %1B/s; Padding dropped in previous 5 mins: %2B; Frames dropped in previous 5 mins: %3")
			.arg(humanBitsBytes(uploadSpeed))
			.arg(humanBitsBytes(relDroppedPadding, 1))
			.arg(relDroppedFrames);
	} else {
		appLog(LOG_CAT) << QStringLiteral(
			"Current upload speed = %1B/s; Frames dropped in previous 5 mins: %2")
			.arg(humanBitsBytes(uploadSpeed))
			.arg(relDroppedFrames);
	}
	m_prevDroppedPadding = droppedPadding;
	m_prevDroppedFrames = droppedFrames;
}

void RTMPTargetBase::rtmpConnected()
{
	appLog(LOG_CAT) << QStringLiteral("Connected to remote host");

	// Determine TCP send buffer size. Data stays in the send buffer until it
	// has been acknowledged by the remote those, this means that we need to
	// keep in mind network latency when choosing the buffer size. Ideally it
	// should be as small as possible so we can more quickly respond to network
	// congestion. TODO: Windows has a function called
	// `idealsendbacklogquery()` that returns the recommended send buffer size
	// for a certain socket during usage, we should probably use that.
	const float SEND_BUF_SIZE_SECS = 1.5f;
	int sendBufSize = m_videoEnc->getAvgBitrateForCongestion();
	if(m_audioEnc != NULL)
		sendBufSize += m_audioEnc->getAvgBitrateForCongestion();
	sendBufSize *= 1000; // Kb/s -> b/s (1000 for bits, 1024 for bytes)
	sendBufSize /= 8; // b/s -> B/s
	sendBufSize = (int)((float)sendBufSize * SEND_BUF_SIZE_SECS);
	if(m_rtmp->setOSWriteBufferSize(sendBufSize) == 0) {
		appLog(LOG_CAT)
			<< QStringLiteral("Set OS TCP send buffer size to %L1 bytes")
			.arg(sendBufSize);
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< QStringLiteral("Failed to set OS TCP send buffer size to %L1 bytes, upload speed will be suboptimal")
			.arg(sendBufSize);
	}

	emit rtmpStateChanged(false);
}

void RTMPTargetBase::rtmpInitialized()
{
	// Signal used for notification only
	appLog(LOG_CAT) << QStringLiteral("RTMP connection initialized");
	emit rtmpStateChanged(false);
}

void RTMPTargetBase::rtmpConnectedToApp()
{
	appLog(LOG_CAT) << QStringLiteral("Connected to RTMP application");

	// Create publisher object and connect to its signals
	m_publisher = m_rtmp->createPublishStream();
	connect(m_publisher, &RTMPPublisher::ready,
		this, &RTMPTargetBase::publisherReady);
	connect(m_publisher, &RTMPPublisher::socketDataRequest,
		this, &RTMPTargetBase::publisherDataRequest);
	if(!m_publisher->beginPublishing()) {
		fatalRtmpError("Failed to begin publishing");
		return;
	}

	emit rtmpStateChanged(false);
}

void RTMPTargetBase::rtmpDisconnected()
{
	if(m_deactivating) {
		emit rtmpStateChanged(false);
		return; // Ignore self-inflicted disconnections
	}
	fatalRtmpError("Disconnected");
}

void RTMPTargetBase::rtmpError(RTMPClient::RTMPError error)
{
	if(m_deactivating) {
		emit rtmpStateChanged(false);
		return; // Ignore self-inflicted errors
	}
	//if(m_isActive)
	//	return; // Ignore errors if not active
	fatalRtmpError(RTMPClient::errorToString(error));
}

void RTMPTargetBase::publisherReady()
{
	// We have successfully connected to the RTMP remote host and the
	// connection is now ready to accept video data.
	appLog(LOG_CAT) << QStringLiteral("Ready to begin transmitting data");

	// Reference encoders by enabling the synchroniser
	if(!m_syncer->setActive(true)) {
		fatalRtmpError("Failed to activate synchroniser");
		return;
	}

	// As we have proven that this connection is valid if we get disconnected
	// then we want to attempt to automatically reconnect.
	m_conSucceededOnce = true;
	m_firstReconnectAttempt = true;

	emit rtmpStateChanged(false);
}

void RTMPTargetBase::publisherDataRequest(uint numBytes)
{
	Q_UNUSED(numBytes);
	writeToPublisher();
}
