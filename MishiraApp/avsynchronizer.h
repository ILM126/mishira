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

#ifndef AVSYNCHRONIZER_H
#define AVSYNCHRONIZER_H

#include "encodedframe.h"
#include "encodedsegment.h"
#include <QtCore/QObject>

//=============================================================================
/// <summary>
/// A class that handles the synchronising of a single encoded video and audio
/// stream based on their real-time timestamps. It guarantees that video and
/// audio data are correctly interweaved based on their DTS and that the first
/// video frame is a keyframe with a PTS of 0.
/// </summary>
class AVSynchronizer : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	VideoEncoder *		m_videoEnc;
	AudioEncoder *		m_audioEnc;
	bool				m_isActive;
	bool				m_isSynced;
	bool				m_waitingForVideoKeyframe;
	bool				m_waitingForAudioKeyframe;
	qint64				m_firstVideoPts;
	qint64				m_nextAudioPts;
	quint64				m_firstAudioTimestamp;
	EncodedFrameList	m_videoFrames; // TODO: Use a more efficient queue
	EncodedPacketList	m_audioPkts; // TODO: Use a more efficient queue
	qint64				m_syncError; // Number of nsec that audio is behind video
	quint64				m_beginTime; // How long does it take to sync?

public: // Constructor/destructor ---------------------------------------------
	AVSynchronizer(VideoEncoder *vEnc, AudioEncoder *aEnc);
	virtual ~AVSynchronizer();

public: // Methods ------------------------------------------------------------
	bool	isActive() const;
	bool	setActive(bool active);
	bool	isSynchronized() const;
	bool	isEmitting() const;

private:
	int		findFirstAudioForVideo(EncodedFrame frame);
	void	trimBuffers();
	bool	doInitialSync();
	void	emitDataInterleaved();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	frameReady(EncodedFrame frame);
	void	segmentReady(EncodedSegment segment);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	frameEncoded(EncodedFrame frame);
	void	segmentEncoded(EncodedSegment segment);
};
//=============================================================================

inline bool AVSynchronizer::isActive() const
{
	return m_isActive;
}

inline bool AVSynchronizer::isSynchronized() const
{
	return m_isActive && m_isSynced;
}

inline bool AVSynchronizer::isEmitting() const
{
	return m_isActive && m_isSynced && !m_waitingForVideoKeyframe;
}

#endif // AVSYNCHRONIZER_H
