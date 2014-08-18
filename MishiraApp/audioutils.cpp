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

#include "audioutils.h"
#include <QtCore/qglobal.h>

//=============================================================================
// Global helpers

void applyAttenuationFilter(
	const float *inData, float *outData, int numFloats, float volume)
{
	// WARNING: `inData` and `outData` can point to the same address!
	if(qFuzzyCompare(volume, 1.0f)) {
		// No change in volume
		if(inData != outData)
			memcpy(outData, inData, numFloats * sizeof(float));
		return;
	}

	// SSE is most likely not worth it as we are probably memory bandwidth
	// limited and our buffers are not 16-byte aligned
	for(int i = 0; i < numFloats; i++)
		outData[i] = inData[i] * volume;
}

void applyMuteFilter(float *outData, int numFloats)
{
	memset(outData, 0, numFloats * sizeof(float));
}

void applyClipFilter(const float *inData, float *outData, int numFloats)
{
	// WARNING: `inData` and `outData` can point to the same address!

	// SSE is most likely not worth it as we are probably memory bandwidth
	// limited and our buffers are not 16-byte aligned
	for(int i = 0; i < numFloats; i++)
		outData[i] = qBound(-1.0f, inData[i], 1.0f);
}

void applyMetronomeFilter(
	const float *inData, float *outData, int numFloats, quint64 sampleNum,
	int sampleRate, int numChannels)
{
	// WARNING: `inData` and `outData` can point to the same address!

	// Output a short square wave burst every second. We rely on the timestamp
	// being synchronised to the frame origin so that it matches the metronome
	// scene layer.

	const float VOLUME = 0.25f; // -6 dBFS
	const int FREQ = 32 * numChannels;
	const uint LENGTH = FREQ * 30;

	// Determine how many samples between "ticks"
	const uint interval = sampleRate * numChannels;

	// Convert sample number to a more efficient relative number
	uint relSampleNum = sampleNum * numChannels % (quint64)interval;

	// TODO: This can be more efficient but as the metronome isn't always used
	// we can probably live with it
	for(int i = 0; i < numFloats; i++) {
		const uint mod = relSampleNum % interval;
		if(mod < LENGTH) {
			// Tick should be heard during this time
			float vol = (float)(mod / FREQ % 2 * 2 - 1) * VOLUME;
			outData[i] = inData[i] + vol;
		} else
			outData[i] = inData[i];
		relSampleNum++;
	}
}

//=============================================================================
// AudioStats class

AudioStats::AudioStats(int sampleRate, int numChannels)
	: m_sampleRate(sampleRate)
	, m_numChannels(numChannels)

	// State
	, m_rmsVolume(0.0f)
	, m_peakVolume(0.0f)
	, m_peakPauseSamples(0)
{
}

AudioStats::~AudioStats()
{
}

void AudioStats::reset()
{
	m_rmsVolume = 0.0f;
	m_peakVolume = 0.0f;
	m_peakPauseSamples = 0;
}

/// <summary>
/// Calculates the RMS and peak volume levels of the input buffer in a manner
/// which takes into account variable input buffer sizes.
/// </summary>
void AudioStats::calcStats(const float *data, int numFloats)
{
	// RMS value is calculated as a running average over 100ms of the sum of
	// squares. As we're doing an average of non-linear inputs our output
	// signal has a much faster attack rate compared to the decay rate. This is
	// mostly okay for our purposes and it seems that foobar2000 does exactly
	// the same thing with its VU meter.
	const float RMS_AVERAGE_SAMPLES = m_sampleRate * m_numChannels / 10;

	// "Freeze" the peak indicator for 250ms before making it fall
	const int PEAK_PAUSE_SAMPLES = m_sampleRate * m_numChannels / 4;

	// Calculate our starting values from the inputs
	float sumSquares = m_rmsVolume * m_rmsVolume * RMS_AVERAGE_SAMPLES;

	// WARNING: This loop will be executed on 264,600 samples per second for
	// most users. It has to be fast!
	for(int i = 0; i < numFloats; i++) {
		// Calculate RMS
		sumSquares =
			sumSquares * (RMS_AVERAGE_SAMPLES - 1.0f) /
			RMS_AVERAGE_SAMPLES;
		sumSquares += data[i] * data[i];

		// Calculate peak
		if(m_peakPauseSamples > 0)
			m_peakPauseSamples--;
		else
			m_peakVolume *= 0.9999f;
		float val = qAbs(data[i]);
		if(val > m_peakVolume) {
			m_peakVolume = val;
			m_peakPauseSamples = PEAK_PAUSE_SAMPLES;
		}
	}

	// Determine our output values
	sumSquares /= RMS_AVERAGE_SAMPLES;
	m_rmsVolume = sqrtf(sumSquares);
}
