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

#ifndef RECOMMENDATIONS_H
#define RECOMMENDATIONS_H

#include "fraction.h"
#include <QtCore/QSize>

//=============================================================================
class Recommendations
{
public:
	static void		maxVidAudBitratesFromTotalUpload(
		float uploadBitsPerSec, bool isGamingOnline, int &maxVidBitrateOut,
		int &maxAudBitrateOut);
	static int		capVidBitrateToSaneValue(
		int bitrate, const Fraction &framerate, bool highAction,
		bool hasTranscoder);
	static QSize	bestVidSizeForBitrate(
		int bitrate, const Fraction &framerate, bool highAction);
};
//=============================================================================

#endif // RECOMMENDATIONS_H
