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

#include "appsharedsegment.h"

AppSharedSegment::AppSharedSegment()
	: m_shm("MishiraSHM")
	, m_isValid(false)
	, m_errorReason()
{
	bool testVersion = false;
	if(m_shm.attach(QSharedMemory::ReadWrite)) {
		// The shared segment already exists. Test to see if it actually
		// references an existing process.
		testVersion = true;
	} else {
		// Shared segment doesn't already exist
		if(m_shm.error() != QSharedMemory::NotFound) {
			// An unexpected error occurred
			m_errorReason = m_shm.errorString();
			return;
		}
		if(!m_shm.create(SEGMENT_SIZE, QSharedMemory::ReadWrite)) {
			// Failed to create shared memory segment
			m_errorReason = m_shm.errorString();
			return;
		}
	}

	lock();

	// Get pointers to our data
	char *data = static_cast<char *>(m_shm.data());
	m_version = reinterpret_cast<quint32 *>(data);
	data += sizeof(quint32);
	m_mainProcId = reinterpret_cast<quint32 *>(data);
	data += sizeof(quint32);
	m_raiseWindow = reinterpret_cast<char *>(data);
	data += sizeof(char);

	if(testVersion) {
		// Test version number to make sure we haven't upgraded Mishira on OS's
		// that have persistent shared segments and the segment has a different
		// format.
		if(*m_version > 1) {
			m_errorReason = QStringLiteral(
				"Newer version of the Mishira shared memory segment already exists");
			unlock();
			return;
		}
	}
	*m_version = 1;

	unlock();

	m_isValid = true;
}

AppSharedSegment::~AppSharedSegment()
{
}

void AppSharedSegment::lock()
{
	m_shm.lock();
}

void AppSharedSegment::unlock()
{
	m_shm.unlock();
}

quint32 AppSharedSegment::getMainProcessId()
{
	if(m_mainProcId == NULL)
		return 0;
	return *m_mainProcId;
}

void AppSharedSegment::setMainProcessId(quint32 procId)
{
	if(m_mainProcId == NULL)
		return;
	*m_mainProcId = procId;
}

bool AppSharedSegment::getRaiseWindow()
{
	if(m_raiseWindow == NULL)
		return false;
	return (*m_raiseWindow != 0) ? true : false;
}

void AppSharedSegment::setRaiseWindow(bool raise)
{
	if(m_raiseWindow == NULL)
		return;
	*m_raiseWindow = (raise ? 1 : 0);
}
