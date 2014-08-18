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

#ifndef AUDIOUTILS_H
#define AUDIOUTILS_H

#include <QtCore/qglobal.h>

//=============================================================================
// Global helpers

void	applyAttenuationFilter(
	const float *inData, float *outData, int numFloats, float volume);
void	applyMuteFilter(float *outData, int numFloats);
void	applyClipFilter(const float *inData, float *outData, int numFloats);
void	applyMetronomeFilter(
	const float *inData, float *outData, int numFloats, quint64 sampleNum,
	int sampleRate, int numChannels);

//=============================================================================
/// <summary>
/// Calculates the RMS and peak volume levels of an audio stream. Assumes that
/// its input is contiguous.
/// </summary>
class AudioStats
{
protected: // Members ---------------------------------------------------------
	int		m_sampleRate;
	int		m_numChannels;

	// State
	float	m_rmsVolume;
	float	m_peakVolume;
	int		m_peakPauseSamples;

public: // Constructor/destructor ---------------------------------------------
	AudioStats(int sampleRate, int numChannels);
	virtual ~AudioStats();

public: // Methods ------------------------------------------------------------
	void	reset();
	void	calcStats(const float *data, int numFloats);

	float	getRmsVolume() const;
	float	getPeakVolume() const;
};
//=============================================================================

inline float AudioStats::getRmsVolume() const
{
	return m_rmsVolume;
}

inline float AudioStats::getPeakVolume() const
{
	return m_peakVolume;
}

#endif // AUDIOUTILS_H
