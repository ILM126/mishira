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

#ifndef APPSHAREDSEGMENT_H
#define APPSHAREDSEGMENT_H

#include "common.h"
#include <QtCore/QSharedMemory>

//=============================================================================
/// <summary>
/// Represents the shared memory segment for interprocess communication. Used
/// mainly for detecting if the application is already running and, if it is,
/// raising the window of the existing instance.
/// </summary>
class AppSharedSegment
{
public: // Constants ----------------------------------------------------------
	static const int SEGMENT_SIZE = 1024; // 1 KB

private: // Members -----------------------------------------------------------
	QSharedMemory	m_shm;
	bool			m_isValid;
	QString			m_errorReason;

	// Data
	quint32 *		m_version;
	quint32 *		m_mainProcId;
	char *			m_raiseWindow;

public: // Constructor/destructor ---------------------------------------------
	AppSharedSegment();
	virtual ~AppSharedSegment();

public: // Methods ------------------------------------------------------------
	bool	isValid() const;
	QString	getErrorReason() const;

	void	lock();
	void	unlock();

	quint32	getMainProcessId();
	void	setMainProcessId(quint32 procId);

	bool	getRaiseWindow();
	void	setRaiseWindow(bool raise);
};
//=============================================================================

inline bool AppSharedSegment::isValid() const
{
	return m_isValid;
}

inline QString AppSharedSegment::getErrorReason() const
{
	return m_errorReason;
}

#endif // APPSHAREDSEGMENT_H
