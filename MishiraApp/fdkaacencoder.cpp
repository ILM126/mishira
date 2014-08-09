//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 2014 Lucas Murray <lmurray@undefinedfire.com>
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

#include "fdkaacencoder.h"
#include "application.h"
#include "audiomixer.h"
#include "profile.h"
#include "resampler.h"
#include <QtCore/qglobal.h>

#define DUMP_STREAM_TO_FILE 0
#if DUMP_STREAM_TO_FILE
#include <QtCore/QFile>
#endif

const int NUM_CHANNELS = 2;

const QString LOG_CAT = QStringLiteral("Audio");

//=============================================================================
// Helpers

QString getFdkAacErrorCode(AACENC_ERROR res)
{
	switch(res) {
	case AACENC_OK:
		return QStringLiteral("AACENC_OK");
	case AACENC_INVALID_HANDLE:
		return QStringLiteral("AACENC_INVALID_HANDLE");
	case AACENC_MEMORY_ERROR:
		return QStringLiteral("AACENC_MEMORY_ERROR");
	case AACENC_UNSUPPORTED_PARAMETER:
		return QStringLiteral("AACENC_UNSUPPORTED_PARAMETER");
	case AACENC_INVALID_CONFIG:
		return QStringLiteral("AACENC_INVALID_CONFIG");
	case AACENC_INIT_ERROR:
		return QStringLiteral("AACENC_INIT_ERROR");
	case AACENC_INIT_AAC_ERROR:
		return QStringLiteral("AACENC_INIT_AAC_ERROR");
	case AACENC_INIT_SBR_ERROR:
		return QStringLiteral("AACENC_INIT_SBR_ERROR");
	case AACENC_INIT_TP_ERROR:
		return QStringLiteral("AACENC_INIT_TP_ERROR");
	case AACENC_INIT_META_ERROR:
		return QStringLiteral("AACENC_INIT_META_ERROR");
	case AACENC_ENCODE_ERROR:
		return QStringLiteral("AACENC_ENCODE_ERROR");
	case AACENC_ENCODE_EOF:
		return QStringLiteral("AACENC_ENCODE_EOF");
	default:
		return numberToHexString((uint)res);
	}
}

//=============================================================================
// FdkAacEncoder class

#if DUMP_STREAM_TO_FILE
static QByteArray dumpBuffer;
#endif

/// <summary>
/// Returns the best bitrate for the specified input in Kb/s.
/// </summary>
int FdkAacEncoder::determineBestBitrate(int videoBitrate)
{
	//if(videoBitrate < 200)
	//	return 0;
	if(videoBitrate < 300)
		return 32;
	if(videoBitrate < 550)
		return 64;
	if(videoBitrate < 700)
		return 96;
	return 128;
}

int FdkAacEncoder::validateBitrate(int bitrate)
{
	int maxBitrate = 0;
	for(int i = 0; FdkAacBitrates[i].bitrate != 0; i++) {
		maxBitrate = FdkAacBitrates[i].bitrate;
		if(FdkAacBitrates[i].bitrate == bitrate)
			break;
		else if(FdkAacBitrates[i].bitrate > bitrate) {
			appLog(LOG_CAT) <<
				QStringLiteral("%1 Kb/s is an unsupported bitrate, using %2 Kb/s instead")
				.arg(bitrate)
				.arg(FdkAacBitrates[i].bitrate);
			bitrate = FdkAacBitrates[i].bitrate;
			break;
		}
	}
	if(bitrate > maxBitrate) {
		appLog(LOG_CAT) <<
			QStringLiteral("%1 Kb/s is an unsupported bitrate, using %2 Kb/s instead")
			.arg(bitrate)
			.arg(maxBitrate);
		bitrate = maxBitrate;
	}
	return bitrate;
}

FdkAacEncoder::FdkAacEncoder(
	Profile *profile, const FdkAacOptions &opt)
	: AudioEncoder(profile, AencFdkAacType)
	, m_aacEnc(NULL)
	, m_oob()
	, m_resampler(NULL)
	, m_sampleRate(0)

	// Options
	, m_bitrate(validateBitrate(opt.bitrate))

	// State
	, m_firstTimestampUsec(0)
	, m_nextPts(0)
{
#if DUMP_STREAM_TO_FILE
	dumpBuffer.reserve(20 * 1024 * 1024); // 20MB
#endif
}

FdkAacEncoder::~FdkAacEncoder()
{
	shutdownEncoder();

#if DUMP_STREAM_TO_FILE
	QFile dumpFile(App->getDataDirectory().filePath("dump.aac"));
	appLog(Log::Critical)
		<< "Dumping AAC-LC stream to file: \"" << dumpFile.fileName()
		<< "\" (File size: " << dumpBuffer.size() << ")";
	if(dumpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		dumpFile.write(dumpBuffer);
		dumpFile.close();
		dumpBuffer.clear();
	}
#endif
}

/// <summary>
/// Calls `aacEncoder_SetParam()` and prints an error message if it fails.
/// </summary>
/// <returns>True if no error occurred</returns>
bool FdkAacEncoder::setParamLogged(const AACENC_PARAM param, const UINT value)
{
	AACENC_ERROR res = aacEncoder_SetParam(m_aacEnc, param, value);
	if(res != AACENC_OK) {
		appLog(LOG_CAT, Log::Warning) <<
			QStringLiteral("aacEncoder_SetParam(%1, %2) failed, cannot enable encoder. Reason = %3")
			.arg(param)
			.arg(value)
			.arg(getFdkAacErrorCode(res));
		return false;
	}
	return true;
}

bool FdkAacEncoder::initializeEncoder()
{
	if(m_isRunning)
		return true;
	appLog(LOG_CAT) << "Initializing AAC-LC (Encoder " << getId() << ")...";

	// Get audio mixer
	AudioMixer *mixer = App->getProfile()->getAudioMixer();

	// Determine output sample rate for the specified bitrate
	m_sampleRate = mixer->getSampleRate();
	for(int i = 0; FdkAacBitrates[i].bitrate != 0; i++) {
		if(FdkAacBitrates[i].bitrate == m_bitrate) {
			m_sampleRate = FdkAacBitrates[i].sampleRate;
			break;
		}
	}

	appLog(LOG_CAT) <<
		QStringLiteral("Encoder settings: Bitrate = %L1; Sample rate = %L2")
		.arg(m_bitrate)
		.arg(m_sampleRate);

	// Create FDK AAC encoder instance
	AACENC_ERROR res = aacEncOpen(&m_aacEnc, 1, NUM_CHANNELS);
	if(res != AACENC_OK) {
		appLog(LOG_CAT, Log::Warning)
			<< "aacEncOpen() failed, cannot enable encoder. "
			<< "Reason = " << getFdkAacErrorCode(res);
		return false;
	}

	// Reset state
	m_firstTimestampUsec = 0;
	m_nextPts = 0;

	// Setup parameters in such a way that if there is an error we log it and
	// return immediately
	bool success = true;
#define PARAM_LOGGED(a, b) \
	success = success && (success ? setParamLogged((a), (b)) : true)
	PARAM_LOGGED(AACENC_AOT, AOT_AAC_LC);
	PARAM_LOGGED(AACENC_BITRATE, m_bitrate * 1000);
	PARAM_LOGGED(AACENC_SAMPLERATE, m_sampleRate);
	PARAM_LOGGED(AACENC_CHANNELMODE, MODE_2); // Always stereo
	//PARAM_LOGGED(AACENC_CHANNELORDER, 1);
	PARAM_LOGGED(AACENC_AFTERBURNER, 1); // Higher quality, more CPU
	PARAM_LOGGED(AACENC_TRANSMUX, TT_MP4_RAW);
	//PARAM_LOGGED(AACENC_TRANSMUX, TT_MP4_ADTS); // Needed for dump
	if(!success) {
		aacEncClose(&m_aacEnc);
		m_aacEnc = NULL;
		return false;
	}

	// Initialize the instance
	res = aacEncEncode(m_aacEnc, NULL, NULL, NULL, NULL);
	if(res != AACENC_OK) {
		appLog(LOG_CAT, Log::Warning)
			<< "aacEncEncode(NULL) failed, cannot enable encoder. "
			<< "Reason = " << getFdkAacErrorCode(res);
		aacEncClose(&m_aacEnc);
		m_aacEnc = NULL;
		return false;
	}

	// Cache out-of-band data
	res = aacEncInfo(m_aacEnc, &m_oob);
	if(res != AACENC_OK) {
		appLog(LOG_CAT, Log::Warning)
			<< "aacEncInfo() failed, cannot enable encoder. "
			<< "Reason = " << getFdkAacErrorCode(res);
		aacEncClose(&m_aacEnc);
		m_aacEnc = NULL;
		return false;
	}

	// Initialize resampler. FDK requires its input samples to be interlaced
	// 16-bit integers and have a specific sample rate.
	int64_t inChanLayout = (mixer->getNumChannels() != 1)
		? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO; // TODO
	m_resampler = new Resampler(
		inChanLayout, mixer->getSampleRate(), AV_SAMPLE_FMT_FLT,
		AV_CH_LAYOUT_STEREO, m_sampleRate, AV_SAMPLE_FMT_S16); // Always stereo
	if(!m_resampler->initialize()) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to initialize resampler, cannot enable encoder";
		delete m_resampler;
		m_resampler = NULL;
		aacEncClose(&m_aacEnc);
		m_aacEnc = NULL;
		return false;
	}

	appLog(LOG_CAT) << "AAC-LC initialized";

	// Connect to the audio mixer
	connect(mixer, &AudioMixer::segmentReady,
		this, &FdkAacEncoder::segmentReady);

	// Notify the application that we are encoding and that it should process
	// every frame
	App->refProcessAllFrames();

	m_isRunning = true;
	return true;
}

void FdkAacEncoder::shutdownEncoder(bool flush)
{
	if(!m_isRunning)
		return;
	appLog(LOG_CAT) << "Shutting down AAC-LC (Encoder " << getId() << ")...";

	// Flush the encoder so we don't get any memory leaks. Must be done first.
	flushSegments();

	// Disconnect from the audio mixer so that we no longer receive any segment
	// signals from it.
	disconnect(App->getProfile()->getAudioMixer(), &AudioMixer::segmentReady,
		this, &FdkAacEncoder::segmentReady); // Must disconnect first

	if(m_resampler != NULL) {
		delete m_resampler;
		m_resampler = NULL;
	}

	aacEncClose(&m_aacEnc);
	m_aacEnc = NULL;

	// Notify the application that we are are no longer encoding and that it
	// should not process every frame if there is no other active encoders.
	App->derefProcessAllFrames();

	appLog(LOG_CAT) << "AAC-LC shut down";
	m_isRunning = false;
}

void FdkAacEncoder::flushSegments()
{
	if(!m_isRunning)
		return;

	// Prepare input buffer description
	AACENC_BufDesc inBufDesc;
	inBufDesc.numBufs = 0;
	inBufDesc.bufs = NULL;
	inBufDesc.bufferIdentifiers = NULL;
	inBufDesc.bufSizes = NULL;
	inBufDesc.bufElSizes = NULL;

	// Do the flush
	EncodedSegment outSegment = encodeSegment(&inBufDesc, -1, 0, true);
	if(outSegment.isValid()) {
		// Debugging stuff
		//appLog() << "Flushed: " << outSegment.getDebugString();

		// Emit our signal
		emit segmentEncoded(outSegment);
	}
}

/// <summary>
/// We determine the timestamps of all output `EncodedSegments` by recording
/// the timestamp of the first input sample and counting the number of output
/// packets we have emitted. We can do it this way as all segments are
/// guaranteed to be in sequential order and contiguous.
/// </summary>
quint64 FdkAacEncoder::ptsToTimestampMsec(qint64 pts) const
{
	Fraction timeBase = getTimeBase();
	return (m_firstTimestampUsec +
		((quint64)pts * (quint64)timeBase.numerator * 1000000ULL) /
		(quint64)timeBase.denominator) / 1000ULL;
}

EncodedSegment FdkAacEncoder::encodeSegment(
	AACENC_BufDesc *inBufDesc, int numInSamples, int estimatedPkts,
	bool isFlushing)
{
	// Output buffer must be at least `768 * numChannels` bytes in size as that
	// is the maximum AAC packet size
	AACENC_BufDesc outBufDesc;
	void *outBufs[] = { NULL }; // We set this below
	INT outBufIds[] = { OUT_BITSTREAM_DATA };
	INT outBufSizes[] = { 768 * NUM_CHANNELS }; // Always stereo
	INT outBufElSizes[] = { sizeof(UCHAR) };
	outBufDesc.numBufs = 1;
	outBufDesc.bufs = outBufs;
	outBufDesc.bufferIdentifiers = outBufIds;
	outBufDesc.bufSizes = outBufSizes;
	outBufDesc.bufElSizes = outBufElSizes;

	// Allocate memory for output
	EncodedPacketList pkts;
	if(estimatedPkts > 0)
		pkts.reserve(estimatedPkts);

	// Do the encoding. We repeat the encoding step multiple times to make sure
	// we have processed all the input data
	AACENC_InArgs inArgs;
	AACENC_OutArgs outArgs;
	inArgs.numInSamples = isFlushing ? -1 : numInSamples;
	inArgs.numAncBytes = 0;
	int numPts = 0;
	for(;;) {
		// FDK allows us to define where the output will be written. In order
		// to reduce the amount of memory copies make the output go straight
		// into a QByteArray so that Qt can manage it.
		QByteArray outBuf;
		outBuf.resize(outBufDesc.bufSizes[0]);
		outBufDesc.bufs[0] = outBuf.data();

		AACENC_ERROR res =
			aacEncEncode(m_aacEnc, inBufDesc, &outBufDesc, &inArgs, &outArgs);
		if(res != AACENC_OK && (!isFlushing || res != AACENC_ENCODE_EOF)) {
			appLog(LOG_CAT, Log::Warning)
				<< "aacEncEncode() failed. "
				<< "Reason = " << getFdkAacErrorCode(res);
			return EncodedSegment();
		}

		// Create a packet of the output if there is any
		if(outArgs.numOutBytes > 0) {
			outBuf.resize(outArgs.numOutBytes); // So `.size()` is correct
			EncodedPacket pkt(
				this, PktAudioType, PktHighestPriority, PktFdkAacIdent_FRM,
				false, outBuf);
			pkts.append(pkt);
			numPts++;
		}

		if(isFlushing) {
			// We must test for EOF if we're flushing as it's possible that the
			// encoder will have some samples in its buffer that it won't
			// return until another call
			if(res == AACENC_ENCODE_EOF)
				break;
			continue;
		}
		// Not flushing, do normal loop test

		// Do we need to continue to encode?
		if(outArgs.numInSamples > 0) {
			// Decrease input buffer size by the amount of bytes that the
			// encoder read from the buffer
			INT_PCM *curBuf = static_cast<INT_PCM *>(inBufDesc->bufs[0]);
			inBufDesc->bufs[0] = &(curBuf[outArgs.numInSamples]);
			inArgs.numInSamples -= outArgs.numInSamples;
			if(inArgs.numInSamples <= 0) {
				// Nothing left to process, exit the loop
				break;
			}
		} else if(outArgs.numOutBytes <= 0) {
			// If the encoder didn't read any data and didn't output anything
			// then be safe and exit the loop to prevent possible freezes
			break;
		}
	}

	// Create the EncodedSegment
	EncodedSegment outSegment(
		this, pkts, true, ptsToTimestampMsec(m_nextPts), m_nextPts);
	m_nextPts += numPts;

	// Debugging stuff
#if DUMP_STREAM_TO_FILE
	for(int i = 0; i < pkts.size(); i++) {
		const EncodedPacket &pkt = pkts.at(i);
		dumpBuffer.append(pkt.data());
	}
#endif

	return outSegment;
}

void FdkAacEncoder::forceKeyframe()
{
	// Every segment is a keyframe
}

QString FdkAacEncoder::getInfoString() const
{
	return tr("AAC-LC, %L3 Kb/s")
		.arg(m_bitrate);
}

int FdkAacEncoder::getSampleRate() const
{
	return m_sampleRate;
}

Fraction FdkAacEncoder::getTimeBase() const
{
	return Fraction(getFrameSize(), m_sampleRate).reduced();
}

QByteArray FdkAacEncoder::getOutOfBand() const
{
	QByteArray data(
		reinterpret_cast<const char *>(m_oob.confBuf), m_oob.confSize);
	return data;
}

int FdkAacEncoder::getFrameSize() const
{
	return m_oob.frameLength;
}

int FdkAacEncoder::getDelay() const
{
	return m_oob.encoderDelay;
}

int FdkAacEncoder::getAvgBitrateForCongestion() const
{
	return m_bitrate;
}

void FdkAacEncoder::serialize(QDataStream *stream) const
{
	AudioEncoder::serialize(stream);

	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << (qint32)m_bitrate;
}

bool FdkAacEncoder::unserialize(QDataStream *stream)
{
	if(!AudioEncoder::unserialize(stream))
		return false;

	qint32	int32Data;

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		*stream >> int32Data;
		m_bitrate = validateBitrate(int32Data);
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in AAC-LC audio encoder serialized "
			<< "data, cannot load settings";
		return false;
	}

	return true;
}

/// <summary>
/// WARNING: Invalidates any non-permanent EncodedPackets.
/// </summary>
void FdkAacEncoder::segmentReady(AudioSegment segment)
{
	if(!m_isRunning)
		return;
	//appLog() << "**SEGMENT**";

	// Resample to the format that FDK requires (Interlaced 16-bit integer) and
	// change to the recommended sample rate at the same time.
	int delayedFrames;
	QByteArray inBuf = m_resampler->resample(
		segment.data(), segment.data().size(), delayedFrames);
	int numInSamples = inBuf.size() / sizeof(INT_PCM);

	// Record the timestamp of the first sample we ever receive so that we can
	// calculate the timestamps of the output EncodedSegments.
	// See `ptsToTimestamp()`
	if(m_firstTimestampUsec == 0) {
		m_firstTimestampUsec =
			segment.timestamp() - (delayedFrames * 1000000LL) /
			(qint64)m_sampleRate;
	}

	// Prepare input buffer description
	AACENC_BufDesc inBufDesc;
	void *inBufs[] = { inBuf.data() };
	INT inBufIds[] = { IN_AUDIO_DATA };
	INT inBufSizes[] = { inBuf.size() };
	INT inBufElSizes[] = { sizeof(INT_PCM) };
	inBufDesc.numBufs = 1;
	inBufDesc.bufs = inBufs;
	inBufDesc.bufferIdentifiers = inBufIds;
	inBufDesc.bufSizes = inBufSizes;
	inBufDesc.bufElSizes = inBufElSizes;

	// Do the encoding
	int estimatedPkts = numInSamples / (getFrameSize() * NUM_CHANNELS) + 1;
	EncodedSegment outSegment =
		encodeSegment(&inBufDesc, numInSamples, estimatedPkts, false);
	if(outSegment.isValid()) {
		// Debugging stuff
		//appLog() << outSegment.getDebugString();

		// Emit our signal
		emit segmentEncoded(outSegment);
	}
}
