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

#include "recommendations.h"
#include "common.h"
#include "fdkaacencoder.h"
#include "x264encoder.h"

/// <summary>
/// Determine the maximum video and audio bitrates that can be safely used for
/// the specified total network upload speed.
/// </summary>
void Recommendations::maxVidAudBitratesFromTotalUpload(
	float uploadBitsPerSec, bool isGamingOnline, int &maxVidBitrateOut,
	int &maxAudBitrateOut)
{
	// Calculate recommended video bitrate. This is 55-70% of the specified
	// upload speed rounded down to a nice, human-readable number.
	float bitrate = uploadBitsPerSec;
	if(isGamingOnline)
		bitrate *= 0.55f; // Guessed amount
	else
		bitrate *= 0.7f; // Guessed amount
	maxVidBitrateOut = qRound(bitrate / 100.0f) * 100;
	if(bitrate <= 1000.0f)
		maxVidBitrateOut = qRound(bitrate / 50.0f) * 50;

	// No point recommending below 100 Kb/s as it's below our minimum video
	// bitrate
	if(maxVidBitrateOut < 100)
		maxVidBitrateOut = 100;

	// Calculate recommended audio bitrate based off the video bitrate
	maxAudBitrateOut =
		FdkAacEncoder::determineBestBitrate(maxVidBitrateOut);
}

/// <summary>
/// Cap the video bitrate to the maximum sane value for the specified
/// conditions.
/// </summary>
int Recommendations::capVidBitrateToSaneValue(
	int bitrate, const Fraction &framerate, bool highAction,
	bool hasTranscoder)
{
	// If the user doesn't have a transcoder to lower the bitrate of the video
	// automatically do not recommend a bitrate that exceeds the download speed
	// of the majority of internet users.
	if(!hasTranscoder && bitrate > 2500)
		bitrate = 2500;

	// Cap the video bitrate to 105% of our recommendation for our maximum
	// resolution for the specified framerate and content rounded to a nice,
	// human-readable number.
	int maxRecBitrate = X264Encoder::determineBestBitrate(
		QSize(1920, 1080), framerate, highAction);
	maxRecBitrate *= 1.05f;
	if(maxRecBitrate <= 1000)
		maxRecBitrate = qRound((float)maxRecBitrate / 50.0f) * 50;
	else
		maxRecBitrate = qRound((float)maxRecBitrate / 100.0f) * 100;
	if(maxRecBitrate < bitrate)
		bitrate = maxRecBitrate;

	return bitrate;
}

QSize Recommendations::bestVidSizeForBitrate(
	int bitrate, const Fraction &framerate, bool highAction)
{
	// Calculate recommended size using the encoder's recommendation
	return X264Encoder::determineBestSize(bitrate, framerate, highAction);
}
