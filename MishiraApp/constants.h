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

#ifndef CONSTANTS_H
#define CONSTANTS_H

// Application name and version. Version numbers are in the format
// "X.Y.Z[-WWW]" where "X" is the major version number, "Y" is the minor
// version number, "Z" is the patch version number and "-WWW" is an optional
// tag to indicate pre-releases. Tags are typically one of "-alphaX", "-betaX"
// and "-rcX".
//
// NOTE: Don't forget to update the values in the resource files as well
// ("Mishira.rc" and "MishiraApp.rc"). When limited to numerical values in the
// format "X.Y.Z.W" then "X", "Y" and "Z" match the version number below and
// "W" begins at "0" for the first release and is incremented for every public
// release without skipping any numbers. E.g. "-rc1" is "0", "-rc2" is "1" and
// final is "2". If there is no pre-releases then the final would be "0".
#define ORG_NAME "Mishira"
#define APP_NAME "Mishira"
#define APP_VER_STR "1.0.0"

// Set this define to `1` in order to enable the automatic updater
#define IS_UPDATER_ENABLED 1

// The dynamic range of 24-bit audio is theoretically 144 dB
const int	MIN_MB = -14400;
const float	MIN_MB_F = -14400.0f;

#endif // CONSTANTS_H
