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

#include "filetarget.h"
#include "application.h"
#include "audioencoder.h"
#include "audiomixer.h"
#include "avsynchronizer.h"
#include "fdkaacencoder.h"
#include "profile.h"
#include "stylehelper.h"
#include "videoencoder.h"
#include "Widgets/infowidget.h"
#include "Widgets/targetpane.h"
#include <QtCore/QFileInfo>
extern "C" {
#include <libavformat/avformat.h>
}

#define DUMP_STATS_TO_FILE 0
#if DUMP_STATS_TO_FILE
#include <QtCore/QFile>
#endif

const QString LOG_CAT = QStringLiteral("Target");

#if DUMP_STATS_TO_FILE
static QByteArray dumpBuffer;
static QString dumpFilename;
static float dumpFrameAvg;
static QVector<int> dumpVbvFrames;
#endif

FileTarget::FileTarget(
	Profile *profile, const QString &name, const FileTrgtOptions &opt)
	: Target(profile, TrgtFileType, name)
	, m_videoEnc(NULL)
	, m_audioEnc(NULL)
	, m_syncer(NULL)
	, m_pane(NULL)
	, m_paneTimer(this)
	, m_cachedFilesize(0)
	, m_timeOfPrevSizeCalc(0)
	, m_actualFilename()

	// FFmpeg
	, m_outFormat(NULL)
	, m_context(NULL)
	, m_videoStream(NULL)
	, m_audioStream(NULL)
	, m_wroteHeader(false)
	, m_extraVideoData()

	// Options
	, m_videoEncId(opt.videoEncId)
	, m_audioEncId(opt.audioEncId)
	, m_filename(opt.filename)
	, m_fileType(opt.fileType)
{
#if DUMP_STATS_TO_FILE
	dumpBuffer.reserve(512 * 1024); // 512KB
	dumpVbvFrames.reserve(60);
#endif

	// Connect signals
	connect(&m_paneTimer, &QTimer::timeout,
		this, &FileTarget::paneTimeout);

	// Setup timer to refresh the pane
	m_paneTimer.setSingleShot(false);
	m_paneTimer.start(500); // 500ms refresh
}

FileTarget::~FileTarget()
{
	// Make sure we have released our resources
	setActive(false);

	if(m_syncer != NULL)
		delete m_syncer;
}

void FileTarget::initializedEvent()
{
	// Fetch video and audio encoder
	m_videoEnc = m_profile->getVideoEncoderById(m_videoEncId);
	m_audioEnc = m_profile->getAudioEncoderById(m_audioEncId);
	if(m_videoEnc == NULL) { // No audio is okay
		appLog(LOG_CAT, Log::Warning)
			<< "Could not find video encoder " << m_videoEncId
			<< " for target " << getIdString();
		return;
	}

	// Watch encoders for fatal errors
	connect(m_videoEnc, &VideoEncoder::encodeError,
		this, &FileTarget::videoEncodeError);

	// Create synchroniser object and connect to its signals
	m_syncer = new AVSynchronizer(m_videoEnc, m_audioEnc);
	connect(m_syncer, &AVSynchronizer::frameReady,
		this, &FileTarget::frameReady);
	connect(m_syncer, &AVSynchronizer::segmentReady,
		this, &FileTarget::segmentReady);

	// Find the Libavformat output format
	QByteArray fmtStr;
	switch(m_fileType) {
	default:
	case FileTrgtMp4Type:
		fmtStr = QByteArrayLiteral("mp4");
		break;
	case FileTrgtMkvType:
		fmtStr = QByteArrayLiteral("matroska");
		break;
#if 0
	case FileTrgtFlvType:
		fmtStr = QByteArrayLiteral("flv"); // TODO: Not tested
		break;
#endif // 0
	}
	AVOutputFormat *format = av_oformat_next(NULL);
	while(format != NULL) {
		if(QByteArray(format->name) == fmtStr)
			break;
		format = av_oformat_next(format);
	}
	if(format == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "Could not find Libav output format \"" << fmtStr
			<< "\" for target " << getIdString();
		m_videoEnc = NULL; // Prevent activation
		return;
	}
	m_outFormat = format;

#if 0
	// Make sure that H.264 is supported by the format. We should never have a
	// problem with this as we only use containers that we know can store it.
	if(m_outFormat->query_codec(AV_CODEC_ID_H264, FF_COMPLIANCE_NORMAL) == 0) {
		appLog(LOG_CAT, Log::Warning)
			<< "The specified output format cannot store H.264";
		m_videoEnc = NULL; // Prevent activation
		return;
	}
#endif // 0
}

bool FileTarget::activateEvent()
{
	if(m_isInitializing)
		return false;
	if(m_videoEnc == NULL)
		return false;

	// Determine the filename to actually save to. We don't want to override
	// any existing files. TODO: Make asynchronous
	QFileInfo info = getUniqueFilename(m_filename);
	m_actualFilename = info.filePath();
	QByteArray cFilename = m_actualFilename.toUtf8();

	appLog(LOG_CAT)
		<< "Activating file target " << getIdString()
		<< " with target filename \"" << info.filePath() << "\"...";

	// Don't trigger a filesystem query immediately
	m_cachedFilesize = 0;
	m_timeOfPrevSizeCalc = App->getUsecSinceExec();

	// Reference encoders by enabling the synchroniser
	if(!m_syncer->setActive(true))
		return false;

	// Create the FFmpeg context
	int res = avformat_alloc_output_context2(
		&m_context, m_outFormat, NULL, cFilename.data());
	if(res < 0) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to create Libav context: " << libavErrorToString(res);
		goto exitActivate1;
	}

	// Create the video output stream
	m_videoStream = avformat_new_stream(m_context, NULL);
	if(m_videoStream == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to create video Libav stream";
		goto exitActivate2;
	}
	AVCodecContext* codec = m_videoStream->codec;
	codec->codec_id = CODEC_ID_H264; // Hardcoded H264
	codec->codec_type = AVMEDIA_TYPE_VIDEO;
	codec->flags = CODEC_FLAG_GLOBAL_HEADER;
	codec->width = m_videoEnc->getSize().width();
	codec->height = m_videoEnc->getSize().height();
	codec->time_base.num = m_videoEnc->getTimeBase().numerator;
	codec->time_base.den = m_videoEnc->getTimeBase().denominator;

	// Create the audio output stream
	if(m_audioEnc != NULL) {
		m_audioStream = avformat_new_stream(m_context, NULL);
		if(m_audioStream == NULL) {
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to create audio Libav stream";
			goto exitActivate3;
		}
		AudioMixer *mixer = m_profile->getAudioMixer();
		FdkAacEncoder *aacEnc = static_cast<FdkAacEncoder *>(m_audioEnc);
		AVCodecContext* codec = m_audioStream->codec;
		codec->codec_id = CODEC_ID_AAC; // Hardcoded AAC
		codec->profile = FF_PROFILE_AAC_LOW; // AAC-LC
		codec->codec_type = AVMEDIA_TYPE_AUDIO;
		codec->flags = CODEC_FLAG_GLOBAL_HEADER;
		codec->bit_rate = aacEnc->getBitrate() * 1000; // (1000 for bits)
		codec->sample_rate = m_audioEnc->getSampleRate();
		codec->channels = mixer->getNumChannels();
		codec->frame_size = m_audioEnc->getFrameSize();
		codec->delay = m_audioEnc->getDelay();
		codec->time_base.num = m_audioEnc->getTimeBase().numerator;
		codec->time_base.den = m_audioEnc->getTimeBase().denominator;

		// Out-of-band extra data
		QByteArray oob = m_audioEnc->getOutOfBand();
		codec->extradata_size = oob.count();
		codec->extradata = (uint8_t *)av_mallocz(
			codec->extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
		memcpy(codec->extradata, oob.constData(), codec->extradata_size);
	}

	// Log final output format
	//appLog() << "Libav output format dump:";
	//av_dump_format(m_context, 0, cFilename.data(), 1);

	// Open the output file
	if(!(m_outFormat->flags & AVFMT_NOFILE)) {
		res = avio_open(&m_context->pb, cFilename.data(), AVIO_FLAG_WRITE);
		if(res < 0) {
			appLog(LOG_CAT, Log::Warning)
				<< "Could not open file: " << libavErrorToString(res);
			goto exitActivate4;
		}
	}

	// NOTE: We do not write the file header until the first keyframe as we
	// need to know what "extra data" we should use (SPS and PPS in the case of
	// H.264 video)
	m_wroteHeader = false;

#if DUMP_STATS_TO_FILE
	dumpBuffer.clear();
	dumpBuffer.append(
		QStringLiteral("\"PTS\",\"DTS\",\"Ex. bloat\",\"Inc. bloat\",\"Inc. avg\",\"Inc. VBV buf size\"\n"));
	dumpFilename = info.filePath() + ".csv";
	dumpFrameAvg = 0.0f;
	dumpVbvFrames.clear();
#endif

	if(m_pane != NULL)
		m_pane->setPaneState(TargetPane::LiveState);
	appLog(LOG_CAT) << "File target activated";
	return true;

	// Error handling
exitActivate4:
	if(m_audioStream != NULL)
		avcodec_close(m_audioStream->codec);
exitActivate3:
	avcodec_close(m_videoStream->codec);
exitActivate2:
	avformat_free_context(m_context);
	m_context = NULL;
exitActivate1:
	m_videoEnc->derefActivate();
	if(m_audioEnc != NULL)
		m_audioEnc->derefActivate();
	return false;
}

void FileTarget::deactivateEvent()
{
	if(m_isInitializing)
		return;
	if(m_videoEnc == NULL)
		return; // Never activated
	appLog(LOG_CAT) << "Deactivating file target " << getIdString() << "...";

	// We only write the trailer if we wrote the header (Which is done at the
	// first keyframe)
	if(m_wroteHeader)
		av_write_trailer(m_context);

	// Close the stream codecs. We must also unset our video extra data as it
	// wasn't allocated with `av_malloc()` which will result in
	// `avformat_free_context()` crashing. TODO: Use `av_malloc()`
	avcodec_close(m_videoStream->codec);
	if(m_audioStream != NULL)
		avcodec_close(m_audioStream->codec);
	m_videoStream->codec->extradata = NULL;
	m_videoStream->codec->extradata_size = 0;

	// Close the output file
	if(!(m_outFormat->flags & AVFMT_NOFILE))
		avio_close(m_context->pb);

	// Release the FFmpeg context
	avformat_free_context(m_context);
	m_context = NULL;

	// Dereference encoders by disabling the synchroniser
	m_syncer->setActive(false);

	m_extraVideoData.clear();
	if(m_pane != NULL)
		m_pane->setPaneState(TargetPane::OfflineState);
	appLog(LOG_CAT) << "File target deactivated";

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
}

VideoEncoder *FileTarget::getVideoEncoder() const
{
	return m_videoEnc;
}

AudioEncoder *FileTarget::getAudioEncoder() const
{
	return m_audioEnc;
}

void FileTarget::serialize(QDataStream *stream) const
{
	Target::serialize(stream);

	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << m_videoEncId;
	*stream << m_audioEncId;
	*stream << m_filename;
	*stream << (quint32)m_fileType;
}

bool FileTarget::unserialize(QDataStream *stream)
{
	if(!Target::unserialize(stream))
		return false;

	quint32 uint32Data;

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		// Read our data
		*stream >> m_videoEncId;
		*stream >> m_audioEncId;
		*stream >> m_filename;
		*stream >> uint32Data;
		m_fileType = (FileTrgtType)uint32Data;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in file target serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

void FileTarget::setupTargetPane(TargetPane *pane)
{
	m_pane = pane;
	m_pane->setUserEnabled(m_isEnabled);
	m_pane->setPaneState(
		m_isActive ? TargetPane::LiveState : TargetPane::OfflineState);
	m_pane->setTitle(m_name);
	m_pane->setIconPixmap(QPixmap(":/Resources/target-18x18-file.png"));
	updatePaneText(true);
}

void FileTarget::updatePaneText(bool fromTimer)
{
	if(m_pane == NULL)
		return;

	// Calculate filesize once every X seconds to limit filesystem queries
	const quint64 FILESIZE_UPDATE_PERIOD_USEC = 15000000; // 15 seconds
	if(isActive()) {
		quint64 now = App->getUsecSinceExec();
		if(now > m_timeOfPrevSizeCalc + FILESIZE_UPDATE_PERIOD_USEC) {
			QFileInfo info(m_actualFilename);
			m_cachedFilesize = qMax(0LL, info.size());
			m_timeOfPrevSizeCalc = now;
		}
	}

	m_pane->setItemText(
		0, tr("Recording time:"), getTimeActiveAsString(), false);
	m_pane->setItemText(
		1, tr("Filesize:"),
		tr("%1B").arg(humanBitsBytes(m_cachedFilesize)), true);
	//m_pane->setItemText(
	//	2, tr("Free space:"), tr("%L1 GB").arg(0), false);
	//m_pane->setItemText(
	//	3, tr("Current bitrate:"), tr("%L1 Kb/s").arg(0), true);
}

void FileTarget::paneOutdated()
{
	updatePaneText(false);
}

void FileTarget::paneTimeout()
{
	updatePaneText(true);
}

void FileTarget::resetTargetPaneEnabled()
{
	if(m_pane == NULL)
		return;
	m_pane->setUserEnabled(m_isEnabled);
}

void FileTarget::setupTargetInfo(TargetInfoWidget *widget)
{
	//PROFILE_BEGIN(a);

	widget->setTitle(m_name);
	widget->setIconPixmap(QPixmap(":/Resources/target-160x70-file.png"));

	QString setStr = tr("Error");
	if(m_videoEnc != NULL)
		setStr = m_videoEnc->getInfoString();
	widget->setItemText(0, tr("Video settings:"), setStr, false);

	setStr = tr("No audio");
	if(m_audioEnc != NULL)
		setStr = m_audioEnc->getInfoString();
	widget->setItemText(1, tr("Audio settings:"), setStr, false);

	widget->setItemText(2, tr("Filename:"), m_filename, true);

	//PROFILE_END(a, "FileTarget::setupTargetInfo");
}

void FileTarget::frameReady(EncodedFrame frame)
{
	if(!m_isActive)
		return;
	//appLog() << frame.getDebugString();

	// We begin the file at the first keyframe
	const EncodedPacketList &pkts = frame.getPackets();
	if(!m_wroteHeader) {
		// Some file formats want "extra data" to be stored in the header of
		// the file. In the case of H.264 this extra data is the SPS and PPS
		// NAL units. Even if these units are in the header they should still
		// be kept in the actual stream otherwise decoders will have troubles
		// seeking. NOTE: We only need to do this if the
		// "CODEC_FLAG_GLOBAL_HEADER" flag is set for the codec.
		EncodedPacketList extraPkts;
		extraPkts.reserve(2);
		for(int i = 0; i < pkts.count(); i++) {
			EncodedPacket pkt = pkts.at(i);
			if(pkt.ident() == PktH264Ident_SPS ||
				pkt.ident() == PktH264Ident_PPS)
			{
				extraPkts.append(pkt);
			}
		}

		// Serialize the extra data and pass it to FFmpeg
		m_extraVideoData.clear();
		for(int i = 0; i < extraPkts.count(); i++)
			m_extraVideoData += extraPkts.at(i).data();
		m_videoStream->codec->extradata =
			reinterpret_cast<uint8_t *>(m_extraVideoData.data());
		m_videoStream->codec->extradata_size = m_extraVideoData.size();

		// Write the file header (Must be done after the "extra data" has been
		// determined)
		int res = avformat_write_header(m_context, NULL);
		if(res < 0) {
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to write file headers: " << libavErrorToString(res);
			QTimer::singleShot(0, this, SLOT(delayedSimpleDeactivate()));
			return;
		}
		m_wroteHeader = true;
	}

	//-------------------------------------------------------------------------
	// Add video data to the file

	// Serialize our frame data
	QByteArray pktData;
	for(int i = 0; i < pkts.count(); i++)
		pktData += pkts.at(i).data();

	// Debugging stuff
	//appLog() << bytesToHexGrid(pktData);
#if DUMP_STATS_TO_FILE
	int maxSize = 0;
	int minSize = 0;
	for(int i = 0; i < pkts.size(); i++) {
		const EncodedPacket &pkt = pkts.at(i);
		int pktSize = pkt.data().size();
		maxSize += pktSize;
		if(!pkt.isBloat())
			minSize += pktSize;
	}

	// Running average
	const float DUMP_AVG_NUM_FRAMES = m_videoEnc->getFramerate().asFloat();
	dumpFrameAvg = (dumpFrameAvg * (DUMP_AVG_NUM_FRAMES - 1.0f) +
		(float)maxSize) / DUMP_AVG_NUM_FRAMES;

	// VBV buffer size (Not exactly correct as we use frames instead of bytes)
	const int DUMP_VBV_NUM_FRAMES = (int)DUMP_AVG_NUM_FRAMES;
	dumpVbvFrames.append(maxSize);
	if(dumpVbvFrames.size() > DUMP_VBV_NUM_FRAMES)
		dumpVbvFrames.remove(0);
	int vbvSize = 0;
	for(int i = 0; i < dumpVbvFrames.size(); i++)
		vbvSize += dumpVbvFrames.at(i);

	dumpBuffer.append(QStringLiteral("%1,%2,%3,%4,%5,%6\n")
		.arg(frame.getPTS())
		.arg(frame.getDTS())
		.arg(minSize)
		.arg(maxSize)
		.arg(dumpFrameAvg)
		.arg(vbvSize));
#endif

	// Create the Libav video packet. Note that the time base of the output
	// format might not match the time base of our codec so we must always
	// rescale it in order to prevent incorrect playback speed.
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.pts = av_rescale_q(
		frame.getPTS(), m_videoStream->codec->time_base,
		m_videoStream->time_base);
	pkt.dts = av_rescale_q(
		frame.getDTS(), m_videoStream->codec->time_base,
		m_videoStream->time_base);
	pkt.data = reinterpret_cast<uint8_t *>(pktData.data());
	pkt.size = pktData.size();
	pkt.stream_index = m_videoStream->index;
	pkt.flags = frame.isKeyframe() ? AV_PKT_FLAG_KEY : 0;

	// Write the video packet to the file. While the synchroniser object does
	// its own interleaving we still use FFmpeg's interleaving method as well
	// just to be extra sure as the target file format might use a different
	// interleaving system. This does use more processing power and memory
	// copies though.
	int res = av_interleaved_write_frame(m_context, &pkt);
	if(res < 0) {
		QString reason = libavErrorToString(res);
		if(res == -28)
			reason = tr("Out of disk space");
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to write video stream packet: "
			<< reason;
		App->setStatusLabel(tr("Failed to write to file (%1)")
			.arg(reason));
		QTimer::singleShot(0, this, SLOT(delayedSimpleDeactivate()));
		return;
	}
}

void FileTarget::segmentReady(EncodedSegment segment)
{
	if(!m_isActive)
		return;
	if(!m_wroteHeader) {
		// Should never happen as the synchroniser object will always output a
		// video frame before the first audio segment
		return;
	}
	//appLog() << segment.getDebugString();

	//-------------------------------------------------------------------------
	// Add audio data to the file

	// We must pass each packet to FFmpeg separately
	const EncodedPacketList &pkts = segment.getPackets();
	for(int i = 0; i < pkts.count(); i++) {
		// We must copy the byte array to the stack otherwise the memory gets
		// deallocated before FFmpeg can use it
		// TODO: This doesn't make sense as we should be managing the memory
		// ourselves through EncodedPacket?
		QByteArray pktData = pkts.at(i).data();

		// Debugging stuff
		//appLog() << bytesToHexGrid(pktData);
		//appLog() << "-----";

		// Create the Libav audio packet. Note that the time base of the output
		// format might not match the time base of our codec so we must always
		// rescale it in order to prevent incorrect playback speed.
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.pts = av_rescale_q(
			segment.getPTS() + i, m_audioStream->codec->time_base,
			m_audioStream->time_base);
		pkt.dts = pkt.pts;
		pkt.data = reinterpret_cast<uint8_t *>(pktData.data());
		pkt.size = pktData.size();
		pkt.stream_index = m_audioStream->index;
		pkt.flags = 0; // No need for AV_PKT_FLAG_KEY

		// Write the audio packet to the file. While the synchroniser object does
		// its own interleaving we still use FFmpeg's interleaving method as well
		// just to be extra sure as the target file format might use a different
		// interleaving system. This does use more processing power and memory
		// copies though.
		int res = av_interleaved_write_frame(m_context, &pkt);
		if(res < 0) {
			QString reason = libavErrorToString(res);
			if(res == -28)
				reason = tr("Out of disk space");
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to write video stream packet: "
				<< reason;
			App->setStatusLabel(tr("Failed to write to file (%1)")
				.arg(reason));
			QTimer::singleShot(0, this, SLOT(delayedSimpleDeactivate()));
			return;
		}
	}
}

void FileTarget::videoEncodeError(const QString &error)
{
	if(!isActive())
		return;
	// Immediately stop encoding and notify the user
	App->setStatusLabel(error);
	QTimer::singleShot(0, this, SLOT(delayedSimpleDeactivate()));
}

/// <summary>
/// If we deactive the target from within an encoder signal then it may result
/// in the encoder being deleted by the time we exit the signal. For this
/// reason we must delay all deactivations.
/// </summary>
void FileTarget::delayedSimpleDeactivate()
{
	if(!isActive())
		return;
	setActive(false);
}
