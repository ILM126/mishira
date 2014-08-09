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

#include "asyncio.h"
#include "log.h"
#include "logfilemanager.h"
#include <QtCore/QFile>
#include <QtCore/QThread>

const QString LOG_CAT = QStringLiteral("FileIO");

QThread *AsyncIO::s_thread = NULL;
AsyncIO *AsyncIO::s_worker = NULL;

AsyncIO *AsyncIO::createWorker()
{
	if(s_worker)
		return s_worker;
	s_thread = new QThread();
	s_thread->start();

	s_worker = new AsyncIO();
	s_worker->moveToThread(s_thread);

	return s_worker;
}

void AsyncIO::destroyWorker()
{
	if(!s_thread)
		return;
	s_thread->exit();
	s_thread->wait(); // TODO: Timeout?
	delete s_thread;
	s_thread = NULL;

	delete s_worker;
	s_worker = NULL;
}

AsyncIO::AsyncIO()
	: QObject()
	, m_nextId(1) // Never use "0" so slots can easily ignore events
	, m_mutex(QMutex::Recursive)
{
}

AsyncIO::~AsyncIO()
{
}

/// <summary>
/// Generates a new operation ID that the caller method can use to determine
/// when operations complete.
/// </summary>
int AsyncIO::newOperationId()
{
	m_mutex.lock();
	int id = m_nextId;
	m_nextId++;
	m_mutex.unlock();
	return id;
}

void AsyncIO::saveToFile(
	int id, const QByteArray &data, const QString &filename, bool safeSave)
{
	if(QThread::currentThread() != s_thread) {
		// Method was called from outside of the resource thread. Invoke this
		// method again in the correct thread automatically.
		QMetaObject::invokeMethod(
			this, "saveToFile",
			Q_ARG(int, id),
			Q_ARG(const QByteArray &, data),
			Q_ARG(const QString &, filename),
			Q_ARG(bool, safeSave));
		return;
	}

	// TODO: Use `QSaveFile` if "safeSave == true"
	abort(); // Unimplemented

	emit saveToFileComplete(id, 0);
}

void AsyncIO::loadFromFile(int id, const QString &filename)
{
	if(QThread::currentThread() != s_thread) {
		// Method was called from outside of the resource thread. Invoke this
		// method again in the correct thread automatically.
		QMetaObject::invokeMethod(
			this, "loadFromFile",
			Q_ARG(int, id),
			Q_ARG(const QString &, filename));
		return;
	}

	//_sleep(3000);

	// Read all file data into memory
	int code = 0;
	QByteArray data;
	QFile file(filename);
	if(file.open(QIODevice::ReadOnly)) {
		data = file.readAll();
		file.close();
	} else {
		code = 1;
		appLog(LOG_CAT, Log::Warning)
			<< "Cannot open file \"" << filename << "\" for reading";
	}

	emit loadFromFileComplete(id, code, data);
}

void AsyncIO::flushLogFile()
{
	if(QThread::currentThread() != s_thread) {
		// Method was called from outside of the resource thread. Invoke this
		// method again in the correct thread automatically.
		QMetaObject::invokeMethod(this, "flushLogFile");
		return;
	}

	LogFileManager::getSingleton()->flush();
}
