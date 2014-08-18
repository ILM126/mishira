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

#ifndef X264ENCODER_H
#define X264ENCODER_H

#include "videoencoder.h"
#include <QtCore/QByteArray>
#include <QtCore/QStack>

// x264 headers
#include <stdint.h>
#define _STDINT_H
extern "C" {
#include <x264.h>
}

//=============================================================================
class X264Encoder : public VideoEncoder
{
	Q_OBJECT

private: // Datatypes ---------------------------------------------------------
	struct PictureInfo {
		int		frameNum;
		//quint64	timestampMsec;
	};

protected: // Members ---------------------------------------------------------
	Scaler *				m_scaler;
	TestScaler *			m_testScaler;
	x264_t *				m_x264;
	x264_param_t			m_params;

	// Options
	X264Preset				m_preset;
	int						m_bitrate;
	int						m_keyInterval;

	// State
	qint64					m_nextPts;
	int						m_forceIdr; // -1 = Ignore, 0+ = Delay until force
	x264_picture_t			m_pics[2];
	int						m_prevPic;
	QStack<PictureInfo *>	m_picInfoStack; // More efficient than a queue
	int						m_numPicInfoAllocated;
	bool					m_inLowCpuMode;
	int						m_lowCpuModeCount;
	int						m_lowCpuModeFrameCount;
	int						m_encodeErrorCount;

public: // Static methods -----------------------------------------------------
	static int		determineBestBitrate(
		const QSize &size, Fraction framerate, bool highAction);
	static QSize	determineBestSize(
		int bitrate, Fraction framerate, bool highAction);
	static void		debugBestBitrates();

public: // Constructor/destructor ---------------------------------------------
	X264Encoder(
		Profile *profile, QSize size, SclrScalingMode scaling,
		GfxFilter scaleFilter, Fraction framerate, const X264Options &opt);
	virtual ~X264Encoder();

public: // Methods ------------------------------------------------------------
	TestScaler *	getTestScaler() const;
	X264Preset		getPreset() const;
	int				getBitrate() const;
	int				getKeyInterval() const;

private:
	void			flushFrames();
	bool			encodeNV12Frame(const NV12Frame &frame, uint frameNum);
	void			dupPrevFrame(int amount = 1);
	bool			processNALs(
		x264_picture_t &pic, x264_nal_t *nals, int numNals);
	PictureInfo *	getNextPicInfoFromStack();
	void			freeAllPicInfos();

private: // Interface ---------------------------------------------------------
	virtual bool	initializeEncoder();
	virtual void	shutdownEncoder(bool flush = false);

public:
	virtual void	forceKeyframe();
	virtual void	setLowCPUUsageMode(bool enable);
	virtual bool	isInLowCPUUsageMode() const;
	virtual QString	getInfoString() const;
	virtual int		getAvgBitrateForCongestion() const;
	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			nv12FrameReady(
		const NV12Frame &frame, uint frameNum, int numDropped);
};
//=============================================================================

inline TestScaler *X264Encoder::getTestScaler() const
{
	return m_testScaler;
}

inline X264Preset X264Encoder::getPreset() const
{
	return m_preset;
}

inline int X264Encoder::getBitrate() const
{
	return m_bitrate;
}

inline int X264Encoder::getKeyInterval() const
{
	return m_keyInterval;
}

#endif // X264ENCODER_H
