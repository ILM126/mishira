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

#ifndef ASYNCIO_H
#define ASYNCIO_H

#include <QtCore/QMutex>
#include <QtCore/QObject>

//=============================================================================
class AsyncIO : public QObject
{
	Q_OBJECT

private: // Static members ----------------------------------------------------
	static QThread *	s_thread;
	static AsyncIO *	s_worker;

private: // Members -----------------------------------------------------------
	int					m_nextId;
	QMutex				m_mutex;

public: // Static methods -----------------------------------------------------
	static AsyncIO *	createWorker();
	static void			destroyWorker();

private: // Constructor/destructor --------------------------------------------
	AsyncIO();
	~AsyncIO();

public: // Methods ------------------------------------------------------------
	int					newOperationId();
	Q_INVOKABLE void	saveToFile(
		int id, const QByteArray &data, const QString &filename,
		bool safeSave);
	Q_INVOKABLE void	loadFromFile(int id, const QString &filename);
	Q_INVOKABLE void	flushLogFile();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void				saveToFileComplete(int id, int errorCode);
	void				loadFromFileComplete(
		int id, int errorCode, const QByteArray &data);
};
//=============================================================================

#endif // ASYNCIO_H
