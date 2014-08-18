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

#ifndef WINCPUUSAGE_H
#define WINCPUUSAGE_H

#include "cpuusage.h"
#include <windows.h>

//=============================================================================
class WinCPUUsage : public CPUUsage
{
private: // Members -----------------------------------------------------------
	FILETIME	m_startSysIdle;
	FILETIME	m_startSysKernel;
	FILETIME	m_startSysUser;
	FILETIME	m_startProcKernel;
	FILETIME	m_startProcUser;

public: // Constructor/destructor ---------------------------------------------
	WinCPUUsage();
	virtual ~WinCPUUsage();

public: // Methods ------------------------------------------------------------
	virtual float	getAvgUsage(float *totalUsage = NULL);

private:
	quint64			fileTimeToUInt64(const FILETIME &fileTime);
};
//=============================================================================

#endif // WINCPUUSAGE_H
