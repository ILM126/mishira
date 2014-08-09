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

#include "resampler.h"
extern "C" {
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

Resampler::Resampler(
	int64_t inChanLayout, int inSampleRate, AVSampleFormat inSampleFormat,
	int64_t outChanLayout, int outSampleRate, AVSampleFormat outSampleFormat)
	: m_swr(NULL)
	, m_inChanLayout(inChanLayout)
	, m_inSampleRate(inSampleRate)
	, m_inSampleFormat(inSampleFormat)
	, m_inFrameSize(0) // Set below
	, m_outChanLayout(outChanLayout)
	, m_outSampleRate(outSampleRate)
	, m_outSampleFormat(outSampleFormat)
	, m_outFrameSize(0) // Set below
{
	m_inFrameSize =
		av_get_channel_layout_nb_channels(inChanLayout) *
		av_get_bytes_per_sample(inSampleFormat);
	m_outFrameSize =
		av_get_channel_layout_nb_channels(outChanLayout) *
		av_get_bytes_per_sample(outSampleFormat);
}

Resampler::~Resampler()
{
	if(m_swr != NULL)
		swr_free(&m_swr);
}

bool Resampler::initialize()
{
	if(m_swr != NULL)
		return false;
	m_swr = swr_alloc();
	if(m_swr == NULL)
		return false;
	av_opt_set_int(m_swr, "in_channel_layout", m_inChanLayout, 0);
	av_opt_set_int(m_swr, "in_sample_rate", m_inSampleRate, 0);
	av_opt_set_sample_fmt(m_swr, "in_sample_fmt", m_inSampleFormat, 0);
	av_opt_set_int(m_swr, "out_channel_layout", m_outChanLayout, 0);
	av_opt_set_int(m_swr, "out_sample_rate", m_outSampleRate, 0);
	av_opt_set_sample_fmt(m_swr, "out_sample_fmt", m_outSampleFormat, 0);
	if(swr_init(m_swr) < 0)
		return false;
	return true;
}

/// <summary>
/// Resamples the input buffer and outputs it as a persistent QByteArray. If
/// the output is delayed due to resampling then `delayOut` is set to the
/// number of output samples that the input is delayed by in the output buffer.
/// </summary>
QByteArray Resampler::resample(const char *data, int size, int &delayOut)
{
	// Determine delay first as the delay is the number of samples that are
	// still buffered from last time
	delayOut = swr_get_delay(m_swr, m_outSampleRate);

	// Prepare output buffer. We use a QByteArray to prevent a wasteful memory
	// copy when `AudioSegment::makePersistent()` is called.
	QByteArray newData;
	Q_ASSERT(size % m_inFrameSize == 0);
	int numFrames = size / m_inFrameSize;
	int dstMaxNumFrames = av_rescale_rnd(
		swr_get_delay(m_swr, m_inSampleRate) + numFrames,
		m_outSampleRate, m_inSampleRate, AV_ROUND_UP);
	newData.resize(dstMaxNumFrames * m_outFrameSize);
	const uint8_t *inBufs[1] = { reinterpret_cast<const uint8_t *>(data) };
	uint8_t *outBufs[1] = { reinterpret_cast<uint8_t *>(newData.data()) };

	// Do the actual conversion
	int dstNumFrames = swr_convert(
		m_swr, outBufs, dstMaxNumFrames, inBufs, numFrames);
	newData.resize(dstNumFrames * m_outFrameSize);

#if 0
	// Debug output
	appLog()
		<< "Input frames = " << numFrames
		<< "; Max frames = " << dstMaxNumFrames
		<< "; Actual frames = " << dstNumFrames
		<< "; Delay = " << delayedFrames;
#endif

	return newData;
}
