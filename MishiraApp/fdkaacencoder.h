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

#ifndef FDKAACENCODER_H
#define FDKAACENCODER_H

#include "audioencoder.h"
#include "audiosegment.h"
#include <fdk-aac/aacenc_lib.h>

class Resampler;

//=============================================================================
class FdkAacEncoder : public AudioEncoder
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	HANDLE_AACENCODER	m_aacEnc;
	AACENC_InfoStruct	m_oob; // Out-of-band data
	Resampler *			m_resampler;
	int					m_sampleRate;

	// Options
	int					m_bitrate;

	// State
	quint64				m_firstTimestampUsec;
	qint64				m_nextPts;

public: // Static methods -----------------------------------------------------
	static int			determineBestBitrate(int videoBitrate);
	static int			validateBitrate(int bitrate);

public: // Constructor/destructor ---------------------------------------------
	FdkAacEncoder(
		Profile *profile, const FdkAacOptions &opt);
	virtual ~FdkAacEncoder();

public: // Methods ------------------------------------------------------------
	int					getBitrate() const;

private:
	bool				setParamLogged(
		const AACENC_PARAM param, const UINT value);
	void				flushSegments();
	quint64				ptsToTimestampMsec(qint64 pts) const;
	EncodedSegment		encodeSegment(
		AACENC_BufDesc *inBufDesc, int numInSamples, int estimatedPkts,
		bool isFlushing);

private: // Interface ---------------------------------------------------------
	virtual bool		initializeEncoder();
	virtual void		shutdownEncoder(bool flush = false);

public:
	virtual void		forceKeyframe();
	virtual QString		getInfoString() const;
	virtual int			getSampleRate() const;
	virtual Fraction	getTimeBase() const;
	virtual QByteArray	getOutOfBand() const;
	virtual int			getFrameSize() const;
	virtual int			getDelay() const;
	virtual int			getAvgBitrateForCongestion() const;
	virtual void		serialize(QDataStream *stream) const;
	virtual bool		unserialize(QDataStream *stream);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void				segmentReady(AudioSegment segment);
};
//=============================================================================

inline int FdkAacEncoder::getBitrate() const
{
	return m_bitrate;
}

#endif // FDKAACENCODER_H
