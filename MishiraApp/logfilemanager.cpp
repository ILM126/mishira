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

#include "logfilemanager.h"
#include "log.h"
#include <QtCore/QFile>

//=============================================================================
// Helpers

static void logStringCallbackHandler(const QByteArray &data)
{
	// Append to log file
	LogFileManager::getSingleton()->append(data + "\n");
}

//=============================================================================
// LogFileManager class

LogFileManager *LogFileManager::s_instance = NULL;

LogFileManager::LogFileManager()
	: m_buffer()
	, m_filename()
	, m_file(NULL)
	, m_mutex(QMutex::Recursive)
{
	// Singleton
	Q_ASSERT(s_instance == NULL);
	s_instance = this;

	// Reserve 48KB for the buffer
	m_buffer.reserve(48 * 1024);

	// Hook logging callback
	Log::setStringCallback(&logStringCallbackHandler);
}

LogFileManager::~LogFileManager()
{
	// Unhook logging callback
	Log::setStringCallback(NULL);

	// Write the remaining data immediately
	if(!m_buffer.isEmpty())
		flush();

	delete m_file;
	m_file = NULL;

	s_instance = NULL;
}

void LogFileManager::openFile(const QString &filename)
{
	m_mutex.lock();

	// If a file is already open flush and close it
	if(m_file) {
		flush();
		m_file->close();
		m_file = NULL;
	}

	m_filename = filename;
	m_file = new QFile(filename);
	if(m_file->open(
		QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
			appLog() << "Using log file: \"" << filename << "\"";
	} else {
		// Failed to open file
		appLog(Log::Warning)
			<< "Failed to open log file: \"" << filename << "\"";
		delete m_file;
		m_file = NULL;
	}

	m_mutex.unlock();
}

void LogFileManager::append(const QByteArray &data)
{
	m_mutex.lock();

	m_buffer += data;

	m_mutex.unlock();
}

void LogFileManager::flush()
{
	m_mutex.lock();

	if(!m_file || m_buffer.isEmpty()) {
		// Nothing to do
		m_mutex.unlock();
		return;
	}

	if(m_file->write(m_buffer) == -1) {
		// If we have an error writing to the log file then logging the error
		// will just result in feedback loop, don't do it
		//appLog() << "Failed to write to log file: " << m_file->errorString();
	}
	m_file->flush(); // Try to actually make it hit the HDD
	m_buffer.clear();

	m_mutex.unlock();
}

bool LogFileManager::isBufferEmpty()
{
	m_mutex.lock();

	bool empty = m_buffer.isEmpty();

	m_mutex.unlock();
	return empty;
}
