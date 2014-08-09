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

#ifndef LOGFILEMANAGER_H
#define LOGFILEMANAGER_H

#include <QtCore/QByteArray>
#include <QtCore/QMutex>
#include <QtCore/QString>

class QFile;

//=============================================================================
class LogFileManager
{
private: // Static members ----------------------------------------------------
	static LogFileManager *	s_instance;

public: // Static methods -----------------------------------------------------
	static LogFileManager *	getSingleton();

private: // Members -----------------------------------------------------------
	QByteArray	m_buffer;
	QString		m_filename;
	QFile *		m_file;
	QMutex		m_mutex;

public: // Constructor/destructor ---------------------------------------------
	LogFileManager();
	~LogFileManager();

public: // Methods ------------------------------------------------------------
	void	openFile(const QString &filename);
	void	append(const QByteArray &data);
	void	flush();
	bool	isBufferEmpty();
	QString	getFilename() const;
};
//=============================================================================

inline LogFileManager *LogFileManager::getSingleton()
{
	return s_instance;
}

inline QString LogFileManager::getFilename() const
{
	return m_filename;
}

#endif // LOGFILEMANAGER_H
