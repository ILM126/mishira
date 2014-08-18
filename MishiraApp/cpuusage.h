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

#ifndef CPUUSAGE_H
#define CPUUSAGE_H

#include "common.h"

//=============================================================================
/// <summary>
/// Used to get the average CPU usage of the current process since the creation
/// of this object.
/// </summary>
class CPUUsage
{
public: // Constructor/destructor ---------------------------------------------
	CPUUsage();
	virtual ~CPUUsage();

public: // Interface ----------------------------------------------------------
	virtual float	getAvgUsage(float *totalUsage = NULL) = 0;
};
//=============================================================================

#endif // CPUUSAGE_H
