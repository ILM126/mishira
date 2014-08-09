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

#include "avsynchronizer.h"
#include "application.h"
#include "audioencoder.h"
#include "videoencoder.h"

const QString LOG_CAT = QStringLiteral("AVSync");

// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//*****************************************************************************
// Synchronisation is hard as there is a large amount of situations that you
// need to take into account and there is very little that you can safely
// assume about the inputs. Things to keep in mind:
//  - Just because you receive a video frame after a audio segment doesn't mean
//    that its timestamp is after the audio, likewise for audio segments that
//    are received after video frames.
//  - Audio frames can have a time base that's less than half of the video,
//    likewise video frames can have a time base that's less than half of the
//    audio. This means there can be more than one frame in one stream that
//    maps to a single frame in the other stream making comparisons difficult.
//  - Audio can lag the video or video can lag the audio.
//  - We might or might not already have a video keyframe in our buffer by the
//    time we get to a potential synchronisation point.
//  - We need to synchronise the inputs based on PTS but interleave the outputs
//    based on DTS.
//*****************************************************************************
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

AVSynchronizer::AVSynchronizer(VideoEncoder *vEnc, AudioEncoder *aEnc)
	: QObject()
	, m_videoEnc(vEnc)
	, m_audioEnc(aEnc)
	, m_isActive(false)
	, m_isSynced(false)
	, m_waitingForVideoKeyframe(false)
	, m_waitingForAudioKeyframe(true)
	, m_firstVideoPts(0)
	, m_nextAudioPts(0)
	, m_firstAudioTimestamp(0)
	, m_videoFrames()
	, m_audioPkts()
	, m_syncError(0)
	, m_beginTime(0)
{
	// Connect encoder signals
	if(m_videoEnc != NULL) {
		connect(m_videoEnc, &VideoEncoder::frameEncoded,
			this, &AVSynchronizer::frameEncoded);
	}
	if(m_audioEnc != NULL) {
		connect(m_audioEnc, &AudioEncoder::segmentEncoded,
			this, &AVSynchronizer::segmentEncoded);
	}
}

AVSynchronizer::~AVSynchronizer()
{
	// Make sure to dereference the encoders if they are referenced
	setActive(false);
}

/// <summary>
/// Activate or deactivate the synchronizer output.
/// </summary>
/// <returns>True if successfully changed state</returns>
bool AVSynchronizer::setActive(bool active)
{
	if(m_isActive == active)
		return true; // Nothing to do
	if(active) {
		if(m_videoEnc == NULL)
			return false; // We need a video encoder to work
		if(!m_videoEnc->refActivate())
			return false;
		if(m_audioEnc != NULL) {
			if(!m_audioEnc->refActivate()) {
				m_videoEnc->derefActivate();
				return false;
			}
		}
		m_isActive = true;

		// Reset state
		m_isSynced = false;
		m_waitingForVideoKeyframe = false;
		m_waitingForAudioKeyframe = true;
		m_firstVideoPts = 0;
		m_nextAudioPts = 0;
		m_firstAudioTimestamp = 0;
		m_videoFrames.clear();
		m_audioPkts.clear();
		m_syncError = 0;
		m_beginTime = App->getUsecSinceExec();
	} else {
		m_videoEnc->derefActivate();
		if(m_audioEnc != NULL)
			m_audioEnc->derefActivate();
		m_isActive = false;
	}
	return true;
}

/// <summary>
/// Returns the index of the audio frame that the specified video frame
/// synchronises with or -1 if no such frame was found.
/// </summary>
int AVSynchronizer::findFirstAudioForVideo(EncodedFrame frame)
{
	Fraction vTBase = m_videoEnc->getTimeBase();
	Fraction aTBase = m_audioEnc->getTimeBase();
	quint64 vTimeStart = frame.getTimestampMsec();
	quint64 vTimeEnd = vTimeStart + (1000ULL * (quint64)vTBase.numerator) /
		(quint64)vTBase.denominator;
	for(int i = 0; i < m_audioPkts.count(); i++) {
		const EncodedPacket &pkt = m_audioPkts.at(i);
		quint64 aTime = m_firstAudioTimestamp +
			((quint64)i * (quint64)aTBase.numerator * 1000ULL) /
			(quint64)aTBase.denominator;
		if(aTime >= vTimeStart) {
			if(i == 0) {
				// If this is the first audio frame then there is a chance that
				// it is after this video frame.
				if(aTime >= vTimeEnd)
					return -1;

				// If the audio time base is less than half that of the video
				// time base then we need to check to see if it's possible that
				// we are missing another audio frame before this one.
				qint64 aPrevTime = (qint64)m_firstAudioTimestamp +
					((qint64)(i - 1) * (qint64)aTBase.numerator * 1000LL) /
					(qint64)aTBase.denominator;
				if(aPrevTime >= (qint64)vTimeStart) {
					// We are missing at least one audio frame so we cannot
					// synchronise with this one
					return -1;
				}
			}
			//appLog(LOG_CAT) <<
			//	QStringLiteral("Found %1 for %2").arg(i).arg(frame.getPTS());
			return i;
		}
	}
	return -1;
}

/// <summary>
/// Trims the beginning of the buffers so that they both start at the same time
/// based on PTS (The first audio frame must be equal to or just after the
/// first video frame). If there is no overlap between video and audio then
/// this method does nothing.
/// </summary>
void AVSynchronizer::trimBuffers()
{
	// Attempt to trim the video stream first as it is the more important one
	// and we may need to drop some audio frames as well this iteration.
	for(int i = 0; i < m_videoFrames.count(); i++) {
		const EncodedFrame &frame = m_videoFrames.at(i);
		int audIndex = findFirstAudioForVideo(frame);
		if(audIndex >= 0) {
			// We found the first valid frame, trim the rest
			if(i > 0) {
				//appLog(LOG_CAT)
				//	<< QStringLiteral("Trimming %1 video frames").arg(i);
				m_videoFrames.remove(0, i);
			}
			break;
		}
	}
	Q_ASSERT(!m_videoFrames.isEmpty());

	// Attempt to trim the audio stream. Even if the video stream begins before
	// the audio we might still need to trim the audio due to the potential
	// one-to-many nature of frames.
	int audIndex = findFirstAudioForVideo(m_videoFrames.first());
	if(audIndex > 0) {
		//appLog(LOG_CAT)
		//	<< QStringLiteral("Trimming %1 audio packets").arg(audIndex);
		Fraction aTBase = m_audioEnc->getTimeBase();
		m_firstAudioTimestamp = m_firstAudioTimestamp +
			((quint64)audIndex * (quint64)aTBase.numerator * 1000ULL) /
			(quint64)aTBase.denominator;
		m_audioPkts.remove(0, audIndex);
	}
	Q_ASSERT(!m_audioPkts.isEmpty());
}

/// <summary>
/// Synchronise the first audio frame with the first video frame.
/// </summary>
/// <returns>True if the streams have been successfully synchronised.</returns>
bool AVSynchronizer::doInitialSync()
{
	if(m_isSynced)
		return true; // Already synchronised
	if(m_audioEnc == NULL) {
		// We have no audio so we are always "synchronised"
		m_isSynced = true;
		return true;
	}
	if(m_videoFrames.isEmpty() || m_audioPkts.isEmpty())
		return false; // We require both streams to be able to sync

	// If the video and audio overlap at any point trim the beginning so that
	// they both start at the same time.
	trimBuffers();

	// If the first video frame doesn't have a corrosponding audio frame then
	// it means we cannot synchronise right now as there is no overlap
	if(findFirstAudioForVideo(m_videoFrames.first()) == -1)
		return false;

	// If we reach here then it means that we are essentially synchronised but
	// we need to make sure that the first video frame is a keyframe. The
	// buffer trimming above guarantees that none of our buffered frames begin
	// before the audio so the first keyframe that we receive will be the one
	// that we emit. If there is no keyframe in our buffer already then we need
	// to force one. We need to be careful here as this method is called
	// several times and we only want to ever force a single keyframe.
	bool hasKeyframe = false;
	for(int i = 0; i < m_videoFrames.count(); i++) {
		EncodedFrame frame = m_videoFrames.at(i);
		if(frame.isKeyframe()) {
			hasKeyframe = true;
			m_waitingForVideoKeyframe = false;
			m_firstVideoPts = frame.getPTS();
			//appLog(LOG_CAT) << "Found keyframe at " << m_firstVideoPts;
			if(i > 0) {
				//appLog(LOG_CAT) <<
				//	QStringLiteral("Trimming %1 video frames for keyframe")
				//	.arg(i);
				m_videoFrames.remove(0, i);
			}
			break;
		}
	}
	if(!hasKeyframe) {
		// No keyframe in buffer, request one if we haven't already
		if(!m_waitingForVideoKeyframe) {
			//appLog(LOG_CAT) << "Requesting keyframe";
			m_waitingForVideoKeyframe = true;
			m_videoEnc->forceKeyframe();
		}
		return false; // Cannot synchronise yet
	}

	// The keyframe search above could have made the video and audio streams no
	// longer overlap. Test if this happened. While we do this we can also see
	// if we need to do any additional trimming on the audio stream so the
	// first audio frame in the stream corrosponds to the beginning keyframe.
	int audIndex = findFirstAudioForVideo(m_videoFrames.first());
	if(audIndex == -1)
		return false; // No longer overlapping
	else if(audIndex > 0) {
		// We need to trim the audio some more
		trimBuffers();
	}

	// If we reach here then it means that the first video frame in our buffer
	// is a keyframe and the first audio frame corrosponds to that keyframe. We
	// have successfully synchronised!
	m_isSynced = true;

	// Determine synchronisation error in nanoseconds. That is the amount of
	// time that the audio will be moved forward so that PTS 0 of both channels
	// occur at the exact same time during playback. We also store the error in
	// case any targets want to take it into account for frame dropping.
	m_syncError =
		(m_firstAudioTimestamp - m_videoFrames.first().getTimestampMsec()) *
		1000000ULL;
	appLog(LOG_CAT) <<
		QStringLiteral("Audio/video synchronisation error = %L1 ms")
		.arg(m_syncError / 1000000LL);

	// Approximately how long are we buffering one of the streams before we can
	// emit it? Usually we buffer the audio stream but that's not always the
	// case.
	Fraction aTBase = m_audioEnc->getTimeBase();
	quint64 lastAudioTimestamp = m_firstAudioTimestamp +
		((quint64)m_audioPkts.count() * (quint64)aTBase.numerator * 1000ULL) /
		(quint64)aTBase.denominator;
	appLog(LOG_CAT) <<
		QStringLiteral("Video pipeline lags %L1 ms behind the audio pipeline")
		.arg((qint64)lastAudioTimestamp -
		(qint64)m_videoFrames.last().getTimestampMsec());

	// How long did it take to synchronise?
	appLog(LOG_CAT) <<
		QStringLiteral("It took %L1 ms to synchronise video and audio streams")
		.arg((App->getUsecSinceExec() - m_beginTime) / 1000ULL);

	return true;
}

/// <summary>
/// The synchroniser guarantees that all output video frames and audio segments
/// are interleaved in the following pattern (WARNING: DTS order!):
///
///        -------------+-------------------------+-------------
/// Video:  V frame n-1 |        V frame n        | V frame n+1
///        -----+-----+-+---+-----+-----+-----+---+-+-----+-----
/// Audio:  n-1 | n-1 | n-1 |  n  |  n  |  n  |  n  | n+1 | n+1
///        -----+-----+-----+-----+-----+-----+-----+-----+-----
///
///        -------------+-------------------------+-------------
/// Video:              |            0            |      5
///        -----+-----+-+---+-----+-----+-----+---+-+-----+-----
/// Audio:  -3  | -2  | -1  |  1  |  2  |  3  |  4  |  6  |  7
///        -----+-----+-----+-----+-----+-----+-----+-----+-----
///
///        -----------+-----------+-----------+-----------+-----
/// Video:     -1     |     0     |     2     |     3     |  5
///        -----------+-+---------+-----------+---+-------+-----
/// Audio:              |            1            |      4
///        -------------+-------------------------+-------------
/// </summary>
/// <returns>True if the last video frame in our buffer was emitted</returns>
void AVSynchronizer::emitDataInterleaved()
{
	// If we have no audio to synchronise with then we can output all our video
	// data immediately
	if(m_audioEnc == NULL) {
		for(int i = 0; i < m_videoFrames.count(); i++)
			emit frameReady(m_videoFrames.at(i));
		m_videoFrames.clear();
		return;
	}

	// In order to guarantee that the packet timestamps are always increasing
	// we must have at least one packet from each stream in our buffers.
	// TODO: This is not true as we know what the next DTS will be
	Fraction vTBase = m_videoEnc->getTimeBase();
	Fraction aTBase = m_audioEnc->getTimeBase();
	while(m_videoFrames.count() != 0 && m_audioPkts.count() != 0) {
		// All timestamps are in microseconds since video start. If this isn't
		// enough resolution then we need to redesign this loop as we're using
		// the entire 64-bit unsigned integer. Imagine someone broadcasting
		// non-stop for over a week (Awesome Games Done Quick is 5 days
		// straight) at maximum framerate and audio frequency.
		quint64 vTime = 0;
		quint64 nextATime = ((quint64)m_nextAudioPts * 1000000ULL *
			(quint64)aTBase.numerator) / (quint64)aTBase.denominator;

		// Emit all video frames that begin before the next audio packet begins
		int processedFrames = 0;
		for(; processedFrames < m_videoFrames.count(); processedFrames++) {
			EncodedFrame frame = m_videoFrames.at(processedFrames);
			qint64 adjDTS = frame.getDTS() - m_firstVideoPts;
			if(adjDTS > 0) {
				// Don't use `getDTSTimestampMsec()` as it's not reliable
				vTime = ((quint64)adjDTS * 1000000ULL *
					(quint64)vTBase.numerator) /
					(quint64)vTBase.denominator;
				if(vTime > nextATime)
					break; // Cannot emit this frame
			}
			// Emit the frame with adjusted DTS and PTS
			emit frameReady(EncodedFrame(frame.encoder(), frame.getPackets(),
				frame.isKeyframe(), frame.getPriority(),
				frame.getTimestampMsec(), frame.getPTS() - m_firstVideoPts,
				frame.getDTS() - m_firstVideoPts));
		}
		if(processedFrames > 0) {
			m_videoFrames.remove(0, processedFrames);
			if(m_videoFrames.count() == 0)
				break; // Not safe to continue
		}

		// Emit all audio packets that begin before the next video frame begins
		EncodedPacketList pkts;
		for(int i = 0; i < m_audioPkts.count(); i++) {
			quint64 aTime = ((quint64)(m_nextAudioPts + i) *
				1000000ULL * (quint64)aTBase.numerator) /
				(quint64)aTBase.denominator;
			if(aTime > vTime)
				break; // Cannot emit this packet
			pkts.append(m_audioPkts.at(i));
		}
		if(pkts.count() > 0) {
			// Emit reconstructed audio segment object
			EncodedSegment segment(
				m_audioEnc, pkts, true,
				m_firstAudioTimestamp + nextATime / 1000ULL, m_nextAudioPts);
			emit segmentReady(segment);
			m_audioPkts.remove(0, pkts.count());
			m_nextAudioPts += pkts.count();
			if(m_audioPkts.count() == 0)
				break; // Not safe to continue
		}
	}
}

void AVSynchronizer::frameEncoded(EncodedFrame frame)
{
	if(!m_isActive)
		return;
	//appLog() << "Synchroniser: " << frame.getDebugString();

	// Add the video frame to our buffer
	m_videoFrames += frame;

	if(doInitialSync()) {
		// Our streams are synchronised, do interleaving
		emitDataInterleaved();
	}
	if(!m_videoFrames.isEmpty()) {
		// The video frame that we just added to the buffer wasn't emitted,
		// we need to make it persistent.
		frame.makePersistent();
	}
}

void AVSynchronizer::segmentEncoded(EncodedSegment segment)
{
	if(!m_isActive)
		return;
	// Our code assumes that every segment is a keyframe
	//appLog() << "Synchroniser: " << segment.getDebugString();

	// Add the audio segment to our buffer
	m_audioPkts += segment.getPackets();

	// As we only buffer the actual packet data we need to keep track of the
	// initial timing data so we can reconstruct it later based on the packet's
	// PTS value. The stored timestamp is the timestamp at PTS = 0, until we
	// synchronise with the video stream this is always the first packet in the
	// buffer.
	if(m_waitingForAudioKeyframe) {
		m_waitingForAudioKeyframe = false;
		m_firstAudioTimestamp = segment.getTimestampMsec();
	}

	if(doInitialSync()) {
		// Our streams are synchronised, do interleaving
		emitDataInterleaved();
	}
	if(!m_audioPkts.isEmpty()) {
		// The audio data that we just added to the buffer wasn't emitted,
		// we need to make it persistent.
		segment.makePersistent();
	}
}
