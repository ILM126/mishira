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

#include "wincpuusage.h"
#include "winapplication.h"

WinCPUUsage::WinCPUUsage()
	: CPUUsage()
	, m_startSysIdle()
	, m_startSysKernel()
	, m_startSysUser()
	, m_startProcKernel()
	, m_startProcUser()
{
	// Remember current CPU statistics
	FILETIME createTime, exitTime;
	GetSystemTimes(&m_startSysIdle, &m_startSysKernel, &m_startSysUser);
	GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime,
		&m_startProcKernel, &m_startProcUser);
}

WinCPUUsage::~WinCPUUsage()
{
}

/// <summary>
/// Returns the average CPU usage of the current process in the range [0, 1].
/// Also returns the average total CPU usage if `totalUsage` is non-NULL.
/// </summary>
float WinCPUUsage::getAvgUsage(float *totalUsage)
{
	// Fetch current CPU statistics
	FILETIME curSysIdle, curSysKernel, curSysUser, curProcKernel, curProcUser;
	FILETIME createTime, exitTime;
	GetSystemTimes(&curSysIdle, &curSysKernel, &curSysUser);
	GetProcessTimes(GetCurrentProcess(), &createTime, &exitTime,
		&curProcKernel, &curProcUser);

	// Calculate the change over time
	quint64 diffSysIdle =
		fileTimeToUInt64(curSysIdle) - fileTimeToUInt64(m_startSysIdle);
	quint64 diffSysKernel =
		fileTimeToUInt64(curSysKernel) - fileTimeToUInt64(m_startSysKernel);
	quint64 diffSysUser =
		fileTimeToUInt64(curSysUser) - fileTimeToUInt64(m_startSysUser);
	quint64 diffProcKernel =
		fileTimeToUInt64(curProcKernel) - fileTimeToUInt64(m_startProcKernel);
	quint64 diffProcUser =
		fileTimeToUInt64(curProcUser) - fileTimeToUInt64(m_startProcUser);

	// Calculate the average total CPU usage if the caller requested it
	if(totalUsage != NULL) {
		quint64 totalTime = diffSysKernel + diffSysUser;
		quint64 totalIdleTime = diffSysIdle;
		*totalUsage =
			1.0f - (float)((double)totalIdleTime / (double)totalTime);
	}

	// Calculate and return the ratio of processor time vs total time. Note
	// that the system kernel time includes idle time
	quint64 totalTime = diffSysKernel + diffSysUser;
	quint64 totalProcTime = diffProcKernel + diffProcUser;
	return (float)((double)totalProcTime / (double)totalTime);
}

quint64 WinCPUUsage::fileTimeToUInt64(const FILETIME &fileTime)
{
	ULARGE_INTEGER ret;
	ret.HighPart = fileTime.dwHighDateTime;
	ret.LowPart = fileTime.dwLowDateTime;
	return ret.QuadPart;
}
