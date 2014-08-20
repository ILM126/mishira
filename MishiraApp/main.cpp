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

#include "winapplication.h"
#include "appsharedsegment.h"
#include "common.h"
#include "constants.h"
#include "logfilemanager.h"
#include <QtCore/QSharedMemory>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

// We use Visual Leak Detector for detecting memory leaks as it works for both
// debug and release builds.
#define ENABLE_DETECT_MEM_LEAKS 0
#if ENABLE_DETECT_MEM_LEAKS
#define VLD_FORCE_ENABLE
#include <vld.h>
#endif // ENABLE_DETECT_MEM_LEAKS

//=============================================================================
// Platform-specific message boxes. We need this as we cannot use QMessageBox
// before QApplication has been initialized.

void showErrorBox(QString text, QString infoText)
{
#ifdef Q_OS_WIN
	// Send to log system
	appLog(Log::Critical)
		<< text << "\n" << infoText;

	// Append inputs for display
	text += "\n\n" + infoText;

	// Display message box
	wchar_t *wMessage = QStringToWChar(text);
	wchar_t *wCaption = QStringToWChar(QStringLiteral(APP_NAME));
	MessageBox(NULL, wMessage, wCaption, MB_OK | MB_ICONERROR);
	delete[] wMessage;
	delete[] wCaption;
#else
#error Unsupported platform
#endif
}

//=============================================================================
// Qt logging handler

//void qtMessageHandler(QtMsgType type, const char *msg)
void qtMessageHandler(
	QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	Log::LogLevel lvl;

	switch(type) {
	case QtDebugMsg:
		lvl = Log::Notice;
		break;
	default:
	case QtWarningMsg:
		lvl = Log::Warning;
		break;
	case QtCriticalMsg:
	case QtFatalMsg:
		lvl = Log::Critical;
		break;
	}

	appLog("Qt", lvl)
		<< msg;
	//	<< QStringLiteral(" (%1:%2, %3)").arg(context.file).arg(context.line)
	//		.arg(context.function);

#ifdef QT_DEBUG
	if(type == QtFatalMsg) {
		// Cause a SEGFAULT to make it easier for us to debug
		int *tmp = NULL;
		*tmp = 0;
	}
#endif
}

//=============================================================================
// Windows-specific helpers

#ifdef Q_OS_WIN

/// <summary>
/// Returns true if a process with the specified ID exists.
/// </summary>
bool doesProcessExist(DWORD procId)
{
	// Open the process
	HANDLE process = OpenProcess(
		PROCESS_QUERY_LIMITED_INFORMATION, FALSE, procId);
	if(process == NULL)
		return false;

	bool ret = true; // Assume it exists if the following fails
	DWORD code;
	if(GetExitCodeProcess(process, &code)) {
		if(code != STILL_ACTIVE)
			ret = false;
	}

	// Close the process
	CloseHandle(process);

	return ret;
}

QString getProcExeFilename(DWORD procId, bool fullPath)
{
	// Open the process
	HANDLE process = OpenProcess(
		PROCESS_QUERY_LIMITED_INFORMATION, FALSE, procId);
	if(process == NULL)
		return QString();

	// MSDN recommends using `GetProcessImageFileName()` or
	// `QueryFullProcessImageName()` over `GetModuleFileNameEx()`
	const int strLen = 256;
	wchar_t strBuf[strLen];
	GetProcessImageFileName(process, strBuf, strLen);

	// Close the process
	CloseHandle(process);

	// We just want the ".exe" name without the path. Note that
	// `GetProcessImageFileName()` uses device form paths instead of drive
	// letters
	QString str = QString::fromUtf16(reinterpret_cast<const ushort *>(strBuf));
	if(fullPath)
		return str;
	QStringList strList = str.split(QChar('\\'));
	return strList.last();
}

#endif

//=============================================================================
// Main entry point

#ifdef Q_OS_WIN
extern "C"
	__declspec(dllexport)
	int __cdecl startApp(int argc, char *argv[], void *hideLauncherSplash)
{
	WinApplication::s_hideLauncherSplashPtr =
		(hideLauncherSplash_t)hideLauncherSplash;
	return exec(argc, argv);
}
#else
#error Unsupported platform
#endif

int exec(int argc, char *argv[])
{
	// Initialize Mishira libraries
	INIT_LIBBROADCAST();
	INIT_LIBDESKCAP();
	vidgfx_init();

	//-------------------------------------------------------------------------
	// Initialize our logging system and install logging hooks into our
	// libraries. Do not create any files yet as we might immediately quit if
	// another instance of the application is currently running.

	// We must create this before we get multiple threads. The `Application`
	// class is responsible for deleting this object.
	new LogFileManager();

	// Install our Qt message handler so we can intercept messages
	qInstallMessageHandler(qtMessageHandler);

	// Display application version information at the very top of the log
	appLog() << QStringLiteral(
		"%1 v%2 (Build %3)")
		.arg(APP_NAME)
		.arg(APP_VER_STR)
		.arg(getAppBuildVersion());

	//-------------------------------------------------------------------------
	// Create a named shared memory segment so we can test to see if the user
	// already has an instance of the application running.

	// Open or create shared memory segment
	AppSharedSegment *shm = new AppSharedSegment();
	if(!shm->isValid()) {
		// An unexpected error occured

		// Notify the user as they would get no feedback otherwise. We cannot
		// use translatable strings yet as we haven't initialized the
		// translation system.
		showErrorBox(
			"Failed to open or create shared memory segment. Reason:",
			shm->getErrorReason());

		// Terminate
		delete shm;
		return 1; // Error
	}

	// Test if we already have an instance open and if we don't set the process
	// ID to us.
	shm->lock();
#ifdef Q_OS_WIN
	DWORD ourProcId = GetCurrentProcessId();
	DWORD shmProcId = (DWORD)shm->getMainProcessId();
	//appLog() << "Our PID: " << (uint)ourProcId
	//	<< ", SHM PID: " << (uint)shmProcId;
	if(shmProcId != 0 && ourProcId != shmProcId) {
		// Process IDs do not match so it's not us. Test to see if the other
		// process actually exists and if so test to see if it's actually
		// another instance of ourselves
		bool exists = doesProcessExist(shmProcId);
		QString ourPath = getProcExeFilename(ourProcId, true);
		QString shmPath = getProcExeFilename(shmProcId, true);
		//appLog() << "Our path: \"" << ourPath
		//	<< "\", SHM path: \"" << shmPath << "\"";
		if(exists && ourPath == shmPath && !shmPath.isEmpty()) {
			// An instance already exists
			shm->unlock();
			appLog()
				<< "Application instance already exists, raising window and "
				<< "terminating";

			// Notify the other process to raise its main window
			shm->setRaiseWindow(true);

			// Terminate
			delete shm;
			return 0; // Success
		}

		// Our hooks might behave weirdly after a crash so log that it was
		// detected
		appLog(Log::Warning)
			<< "Our shared memory segment wasn't properly cleaned up, Mishira "
			<< "most likely crashed the last time it was run";
	}
	shm->setMainProcessId((uint32_t)ourProcId);
#else
#error Unsupported platform
#endif
	shm->unlock();

	//-------------------------------------------------------------------------
	// Create `Application` instance and continue startup sequence there

	// Force destruction of the `Application` object before returning
	int returnCode = 1;
	QString logFilename;
	{
#ifdef Q_OS_WIN
		WinApplication app(argc, argv, shm);
#else
#error Unsupported platform
#endif

		if(!app.initialize())
			return 1; // Terminate with error

		//---------------------------------------------------------------------
		// Begin main loop

		returnCode = app.execute();

		//---------------------------------------------------------------------
		// Shutdown cleanly

		logFilename = LogFileManager::getSingleton()->getFilename();
		returnCode = app.shutdown(returnCode);
	}

	// If the application exited uncleanly show a useful error dialog
	if(returnCode != 0) {
		QString msg;
		QVector<QString> msgs = Log::getCriticalMessages();
		for(int i = 0; i < msgs.count(); i++)
			msg += QStringLiteral("%1\n").arg(msgs.at(i));
		if(!logFilename.isEmpty()) {
			if(msgs.count())
				msg += QStringLiteral("\n");
			msg += QStringLiteral("More details can be found in:\n%1")
				.arg(logFilename);
		}
		showErrorBox(
			"Application quit unexpectedly with the message below:", msg);
	}

	return returnCode;
}
