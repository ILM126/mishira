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

#ifndef LOG_H
#define LOG_H

#include <QtCore/QVector>

class LogData;
class QPoint;
class QPointF;
class QRect;
class QRectF;
class QSize;
class QSizeF;

// Separators that fill an entire line to the 79th column (With timestamp)
extern const char *LOG_SINGLE_LINE;
extern const char *LOG_DOUBLE_LINE;

typedef void LogStringCallback(const QByteArray &data);

//=============================================================================
class Log
{
public: // Datatypes ----------------------------------------------------------
	enum LogLevel {
		Notice = 0,
		Warning,
		Critical
	};

protected: // Static members --------------------------------------------------
	static LogStringCallback *	s_stringCallback;
	static bool					s_timestampsEnabled;
	static QVector<QString>		s_criticalMsgs;

public: // Members ------------------------------------------------------------
	LogData *	d;

public: // Static methods -----------------------------------------------------
	static void	setStringCallback(LogStringCallback *funcPtr);
	static void	setTimestampsEnabled(bool enabled);
	static QVector<QString>	getCriticalMessages();

public: // Constructor/destructor ---------------------------------------------
	Log();
	Log(const Log &log);
	~Log();
};
Log operator<<(Log log, const QString &msg);
Log operator<<(Log log, const QByteArray &msg);
Log operator<<(Log log, const char *msg);
Log operator<<(Log log, int msg);
Log operator<<(Log log, unsigned int msg);
Log operator<<(Log log, qint64 msg);
Log operator<<(Log log, quint64 msg);
Log operator<<(Log log, qreal msg);
Log operator<<(Log log, float msg);
Log operator<<(Log log, bool msg);
Log operator<<(Log log, const QPoint &msg);
Log operator<<(Log log, const QPointF &msg);
Log operator<<(Log log, const QRect &msg);
Log operator<<(Log log, const QRectF &msg);
Log operator<<(Log log, const QSize &msg);
Log operator<<(Log log, const QSizeF &msg);
Log appLog(const QString &category, Log::LogLevel lvl = Log::Notice);
Log appLog(Log::LogLevel lvl = Log::Notice);
//=============================================================================

inline void Log::setStringCallback(LogStringCallback *funcPtr)
{
	s_stringCallback = funcPtr;
}

inline void Log::setTimestampsEnabled(bool enabled)
{
	s_timestampsEnabled = enabled;
}

/// <summary>
/// Returns a list of all the critical error messages that occured during the
/// execution of the application in a format that can be used in a message box.
/// </summary>
inline QVector<QString> Log::getCriticalMessages()
{
	return s_criticalMsgs;
}

#endif // LOG_H
