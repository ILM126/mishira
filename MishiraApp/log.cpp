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

#include "log.h"
#include <QtCore/QDateTime>
#include <QtCore/QRect>
#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <QtCore/QStringList>
#include <iostream>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

const char *LOG_SINGLE_LINE =
	"---------------------------------------------------------";
const char *LOG_DOUBLE_LINE =
	"=========================================================";

//=============================================================================
// LogData class

class LogData
{
public: // Members ------------------------------------------------------------
	int				ref;
	QString			cat;
	Log::LogLevel	lvl;
	QString			msg;

public: // Constructor/destructor ---------------------------------------------
	LogData()
		: ref(0)
		, cat()
		, lvl(Log::Notice)
		, msg()
	{
	}
};

//=============================================================================
// Log class

LogStringCallback *Log::s_stringCallback = NULL;
bool Log::s_timestampsEnabled = true;
QVector<QString> Log::s_criticalMsgs;

Log::Log()
	: d(new LogData())
{
	d->ref++;
}

Log::Log(const Log &log)
	: d(log.d)
{
	d->ref++;
}

Log::~Log()
{
	QString		time;
	QStringList	msgs;

	d->ref--;
	if(d->ref)
		return; // Temporary object

	// HACK: Prevent Qt's built-in colour dialog from spamming our log.
	// Hopefully this gets fixed as our code can cause the same error and we
	// will never know about it.
	if(d->msg.startsWith(
		QStringLiteral("QWindowsNativeInterface::nativeResourceForWindow")))
	{
		delete d;
		return;
	}

	// HACK: Prevent Qt's QSocketNotify from warning about multiple notifiers
	// on the same socket. There's nothing we can do about this as we need
	// access to a QTcpSocket's notifier for our RTMP client but it's not in
	// the public API.
	if(d->msg.startsWith(
		QStringLiteral("QSocketNotifier: Multiple socket notifiers for same socket")))
	{
		delete d;
		return;
	}

	// Split up the string so we can remove empty lines and give each line a
	// timestamp and category
	msgs = d->msg.split("\n", QString::SkipEmptyParts);
	time = QDateTime::currentDateTime().toString(Qt::ISODate);

	for(int i = 0; i < msgs.size(); i++) {
		QString	line;
		QString	formattedLine;

		// Clean up line
		line = msgs.at(i);
		if(line.trimmed().isEmpty())
			continue; // Skip empty lines
		line.replace("\r", "");

		// Generate log entry string
		QString prefix;
		if(s_timestampsEnabled)
			prefix = QStringLiteral("[%1]").arg(time);
		if(!d->cat.isEmpty())
			prefix.append(QStringLiteral("[%1]").arg(d->cat));
		switch(d->lvl) {
		default:
		case Notice:
			if(!prefix.isEmpty())
				formattedLine = QStringLiteral("%1 %2").arg(prefix).arg(line);
			else
				formattedLine = line;
			break;
		case Warning:
			formattedLine = QStringLiteral("%1[!!] %2").arg(prefix).arg(line);
			break;
		case Critical:
			formattedLine =
				QStringLiteral("%1[!!!!!] %2").arg(prefix).arg(line);
			break;
		}

		// Output to the terminal
		QByteArray output = formattedLine.toLocal8Bit();
		std::cout << output.constData() << std::endl;

		// Visual Studio does not display stdout in the debug console so we
		// need to use a special Windows API
#if defined(Q_OS_WIN) && defined(QT_DEBUG)
		// Output to the Visual Studio or system debugger in debug builds only
		OutputDebugStringA(output.constData());
		OutputDebugStringA("\r\n");
#endif

		// Remember critical messages so they can be used in an error dialog on
		// application exit
		if(d->lvl == Critical) {
			prefix.clear();
			if(!d->cat.isEmpty())
				prefix = QStringLiteral("[%1] ").arg(d->cat);
			s_criticalMsgs.append(
				QStringLiteral("%1%2").arg(prefix).arg(line));
		}

		// Forward to the callback
		if(s_stringCallback != NULL)
			s_stringCallback(output);
	}

	delete d;
}

Log operator<<(Log log, const QString &msg)
{
	log.d->msg.append(msg);
	return log;
}

Log operator<<(Log log, const QByteArray &msg)
{
	return log << QString::fromUtf8(msg);
}

Log operator<<(Log log, const char *msg)
{
	return log << QString(msg);
}

Log operator<<(Log log, int msg)
{
	return log << QString::number(msg);
}

Log operator<<(Log log, unsigned int msg)
{
	return log << QString::number(msg);
}

Log operator<<(Log log, qint64 msg)
{
	return log << QString::number(msg);
}

Log operator<<(Log log, quint64 msg)
{
	return log << QString::number(msg);
}

Log operator<<(Log log, qreal msg)
{
	return log << QString::number(msg);
}

Log operator<<(Log log, float msg)
{
	return log << QString::number(msg);
}

Log operator<<(Log log, bool msg)
{
	if(msg)
		return log << QStringLiteral("true");
	return log << QStringLiteral("false");
}

Log operator<<(Log log, const QPoint &msg)
{
	return log << QStringLiteral("Point(%1, %2)")
		.arg(msg.x())
		.arg(msg.y());
}

Log operator<<(Log log, const QPointF &msg)
{
	return log << QStringLiteral("Point(%1, %2)")
		.arg(msg.x())
		.arg(msg.y());
}

Log operator<<(Log log, const QRect &msg)
{
	return log << QStringLiteral("Rect(%1, %2, %3, %4)")
		.arg(msg.left())
		.arg(msg.top())
		.arg(msg.width())
		.arg(msg.height());
}

Log operator<<(Log log, const QRectF &msg)
{
	return log << QStringLiteral("Rect(%1, %2, %3, %4)")
		.arg(msg.left())
		.arg(msg.top())
		.arg(msg.width())
		.arg(msg.height());
}

Log operator<<(Log log, const QSize &msg)
{
	return log << QStringLiteral("Size(%1, %2)")
		.arg(msg.width())
		.arg(msg.height());
}

Log operator<<(Log log, const QSizeF &msg)
{
	return log << QStringLiteral("Size(%1, %2)")
		.arg(msg.width())
		.arg(msg.height());
}

Log appLog(const QString &category, Log::LogLevel lvl)
{
	Log log;
	log.d->cat = category;
	log.d->lvl = lvl;
	return log;
}

Log appLog(Log::LogLevel lvl)
{
	return appLog("", lvl);
}
