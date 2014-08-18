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

#ifndef RESAMPLER_H
#define RESAMPLER_H

#include <QtCore/QByteArray>
extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

struct SwrContext;

//=============================================================================
/// <summary>
/// Helper class for doing audio resampling and conversion.
/// </summary>
class Resampler
{
private: // Members -----------------------------------------------------------
	SwrContext *	m_swr;
	int64_t			m_inChanLayout;
	int				m_inSampleRate;
	AVSampleFormat	m_inSampleFormat;
	int				m_inFrameSize;
	int64_t			m_outChanLayout;
	int				m_outSampleRate;
	AVSampleFormat	m_outSampleFormat;
	int				m_outFrameSize;

public: // Constructor/destructor ---------------------------------------------
	Resampler(
		int64_t inChanLayout, int inSampleRate, AVSampleFormat inSampleFormat,
		int64_t outChanLayout, int outSampleRate,
		AVSampleFormat outSampleFormat);
	~Resampler();

public: // Methods ------------------------------------------------------------
	bool		initialize();
	QByteArray	resample(const char *data, int size, int &delayOut);
};
//=============================================================================

#endif // RESAMPLER_H
