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

#include "application.h"
#include "aboutwindow.h"
#include "appsettings.h"
#include "appsharedsegment.h"
#include "audiosourcemanager.h"
#include "asyncio.h"
#include "bitratecalcwindow.h"
#include "constants.h"
#include "cpuusage.h"
#include "darkstyle.h"
#include "layer.h"
#include "layerdialogwindow.h"
#include "logfilemanager.h"
#include "mainwindow.h"
#include "profile.h"
#include "scaler.h"
#include "scene.h"
#include "sceneitem.h"
#include "stylehelper.h"
#include "target.h"
#include "videosourcemanager.h"
#include "wizardwindow.h"
#include "Pages/mainpage.h"
#include <Libbroadcast/brolog.h>
#include <Libdeskcap/caplog.h>
#include <Libdeskcap/capturemanager.h>
#include <Libvidgfx/gfxlog.h>
#include <Libvidgfx/graphicscontext.h>
#include <QtCore/QDateTime>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QMessageBox>
extern "C" {
#include <libavformat/avformat.h>
}

//=============================================================================
// Helpers

void broLogHandler(
	const QString &cat, const QString &msg, BroLog::LogLevel lvl)
{
	Log::LogLevel level = Log::Critical;
	switch(lvl) {
	case BroLog::Notice:
		level = Log::Notice;
		break;
	case BroLog::Warning:
		level = Log::Warning;
		break;
	case BroLog::Critical:
	default:
		level = Log::Critical;
		break;
	}
	appLog(cat, level) << msg;
}

void gfxLogHandler(
	const QString &cat, const QString &msg, GfxLog::LogLevel lvl)
{
	Log::LogLevel level = Log::Critical;
	switch(lvl) {
	case GfxLog::Notice:
		level = Log::Notice;
		break;
	case GfxLog::Warning:
		level = Log::Warning;
		break;
	case GfxLog::Critical:
	default:
		level = Log::Critical;
		break;
	}
	appLog(cat, level) << msg;
}

void capLogHandler(
	const QString &cat, const QString &msg, CapLog::LogLevel lvl)
{
	Log::LogLevel level = Log::Critical;
	switch(lvl) {
	case CapLog::Notice:
		level = Log::Notice;
		break;
	case CapLog::Warning:
		level = Log::Warning;
		break;
	case CapLog::Critical:
	default:
		level = Log::Critical;
		break;
	}
	appLog(cat, level) << msg;
}

const int LIBAV_LOG_LEVEL = AV_LOG_INFO;
static void libavLogHandler(
	void *avcl, int level, const char *fmt, va_list args)
{
	Q_UNUSED(avcl);

	// Libav still calls our callback even if it's above our requested level
	if(level > LIBAV_LOG_LEVEL)
		return;

	Log::LogLevel lvl;
	switch(level) {
	default:
	case AV_LOG_PANIC:
	case AV_LOG_FATAL:
	case AV_LOG_ERROR:
		lvl = Log::Critical;
		break;
	case AV_LOG_WARNING:
		lvl = Log::Warning;
		break;
	case AV_LOG_INFO:
	case AV_LOG_VERBOSE:
		lvl = Log::Notice;
		break;
	case AV_LOG_DEBUG:
		lvl = Log::Notice;
		break;
	}

	// Convert the message into a formatted QString
	QString str;
	int size = 256;
	char *buf = new char[size];
	for(;;) {
		//va_start(args, psz); // Only needed if using "..."
#pragma warning(push)
#pragma warning(disable: 4996)
		int retval = vsnprintf(buf, size, fmt, args);
#pragma warning(pop)
		//va_end(args);
		if(retval >= size) {
			// Buffer wasn't large enough, enlarge
			size = retval + 1;
			delete[] buf;
			buf = new char[size];
			continue;
		}

		if(retval >= 0) {
			// Success, use the out buffer as our string
			str = buf;
			break;
		} else {
			// Error occurred, just use the format string
			str = fmt;
			break;
		}
	}
	delete[] buf;

	// Output to our log system
	appLog(QStringLiteral("Libav"), lvl) << str;
}

//=============================================================================
// Application class

Application *Application::s_instance = NULL;

Application::Application(int &argc, char **argv, AppSharedSegment *shm)
	: QApplication(argc, argv)
	, m_shm(shm)
	, m_appSettings(NULL)
	, m_profile(NULL)
	, m_menuBar(NULL)
	, m_profilesMenu(NULL)
	, m_helpMenu(NULL)
	, m_mainWindow(NULL)
	, m_wizardWindow(NULL)
	, m_layerDialogWindow(NULL)
	, m_bitrateCalcWindow(NULL)
	, m_aboutWindow(NULL)
	, m_gfxContext(NULL)
	, m_audioManager(NULL)
	, m_videoManager(NULL)
	, m_activeCursor(Qt::ArrowCursor)
	, m_dataDir()
	, m_isBroadcasting(false)
	, m_isBroadcastingChanging(false)
	, m_broadcastCpuUsage(NULL)
	, m_isInLowCPUMode(false)

	// Main loop
	, m_processTicksRef(0)
	, m_processAllFramesRef(0)
	, m_lowJitterModeRef(0)
	, m_changeMainLoopFreq(false)
	, m_mainLoopFreq(0, 0)
	, m_curVideoFreq(0, 0)
	, m_tickTimeOrigin(0)
	, m_nextRTTickNum(0)
	, m_nextRTFrameNum(0)
	, m_nextQFrameNum(0)
	, m_tickFrameMultiple(1)
	, m_timeSinceLogFileFlush(0)
	, m_timeSinceProfileSave(0)
	, m_averageFrameJitter(0.0f)
	, m_qtExecTimer(this)
	, m_exiting(false)
	, m_exitCode(1)
	, m_deleteSettingsOnExit(false)

	// Menu bar
	, m_profileActions()

	// Common message boxes
	, m_deleteLayerDialog(NULL)
	, m_deleteLayerGroupDialog(NULL)
	, m_deleteTargetDialog(NULL)
	, m_deleteProfileDialog(NULL)
	, m_deleteSceneDialog(NULL)
	, m_runUpdaterDialog(NULL)
	, m_startStopBroadcastDialog(NULL)
	, m_basicWarningDialog(NULL)
{
	// Singleton
	Q_ASSERT(s_instance == NULL);
	s_instance = this;

	// Setup timer
	m_qtExecTimer.setSingleShot(true);
	connect(&m_qtExecTimer, &QTimer::timeout,
		this, &Application::processQtExecTimer);

	// Connect to our own signals to simplify code
	connect(this, &Application::realTimeTickEvent,
		this, &Application::miscRealTimeTickEvent);
	connect(this, &Application::realTimeFrameEvent,
		this, &Application::miscRealTimeFrameEvent);
}

Application::~Application()
{
	// WARNING: This is called after `shutdown()`. Use that method instead!

	s_instance = NULL;
}

/// <summary>
/// Continues the application startup sequence.
/// </summary>
/// <remarks>
/// WARNING: This method is executed before the main loop has started.
/// </remarks>
/// <returns>True on success and false on failure.</returns>
bool Application::initialize()
{
	// Set organization and application names
	setOrganizationName(ORG_NAME);
	setApplicationName(APP_NAME);

	// Set the default window icon. We cannot reuse the ".ico" file as Qt 5
	// cannot easily read it and we want to use icons with a transparent
	// background.
	QIcon icon;
	icon.addFile(QStringLiteral(":/Resources/icon-256.png"), QSize(256, 256));
	icon.addFile(QStringLiteral(":/Resources/icon-48.png"), QSize(48, 48));
	icon.addFile(QStringLiteral(":/Resources/icon-32.png"), QSize(32, 32));
	icon.addFile(QStringLiteral(":/Resources/icon-16.png"), QSize(16, 16));
	setWindowIcon(icon);

	// Seed RNG
	qsrand(QDateTime::currentMSecsSinceEpoch() % UINT_MAX);

	// Determine the directory that we store most of our application data in
	m_dataDir.setPath(calcDataDirectoryPath());
	m_dataDir.mkpath("."); // Create the directory if it doesn't already exist

	// Archive old log files. Format used: "Log.txt" (Active log file),
	// "Log (Old 1).txt", "Log (Old 2).txt", ..., "Log (Old <MAX>).txt"
	const int NUM_ARCHIVED_LOG_FILES = 9;
	const QString logActiveName = QStringLiteral("Log.txt");
	const QString logArchiveName = QStringLiteral("Log (Old %1).txt");
	m_dataDir.remove(logArchiveName.arg(NUM_ARCHIVED_LOG_FILES));
	for(int i = NUM_ARCHIVED_LOG_FILES - 1; i > 0; i--)
		m_dataDir.rename(logArchiveName.arg(i), logArchiveName.arg(i + 1));
	m_dataDir.rename(logActiveName, logArchiveName.arg(1));

	// Open new log file
	LogFileManager::getSingleton()->openFile(
		m_dataDir.filePath(logActiveName));

	// Install log callbacks for Mishira libraries
	BroLog::setCallback(&broLogHandler);
	GfxLog::setCallback(&gfxLogHandler);
	CapLog::setCallback(&capLogHandler);

#if 0
	// Initialize default palette. All default values are tinted blue by
	// shifting their hue to 220 degrees and giving them a 2% saturation
	QPalette pal = palette();
	const QColor grey105(103, 104, 105);
	const QColor grey120(118, 118, 120);
	const QColor grey160(157, 158, 160);
	const QColor grey227(222, 224, 227);
	const QColor grey240(235, 237, 240);
	const QColor grey246(241, 243, 244);
	const QColor grey247(242, 244, 247);
	pal.setColor(QPalette::Disabled, QPalette::WindowText, grey120);
	pal.setColor(QPalette::Button, grey240);
	pal.setColor(QPalette::Active, QPalette::Midlight, grey227);
	pal.setColor(QPalette::Inactive, QPalette::Midlight, grey227);
	pal.setColor(QPalette::Disabled, QPalette::Midlight, grey247);
	pal.setColor(QPalette::Dark, grey160);
	pal.setColor(QPalette::Mid, grey160);
	pal.setColor(QPalette::Disabled, QPalette::Text, grey120);
	pal.setColor(QPalette::Disabled, QPalette::ButtonText, grey120);
	pal.setColor(QPalette::Disabled, QPalette::Base, grey240);
	pal.setColor(QPalette::Window, grey240);
	pal.setColor(QPalette::Active, QPalette::Shadow, grey105);
	pal.setColor(QPalette::Inactive, QPalette::Shadow, grey105);
	pal.setColor(QPalette::Inactive, QPalette::Highlight, grey240);
	pal.setColor(QPalette::AlternateBase, grey246);
	setPalette(pal);
#endif // 0

	// Initialize default font ourselves as Qt is inconsistent by default. We
	// set the main page's font at a later time.
#ifdef Q_OS_WIN
	QFont font;
	font.setFamily(QStringLiteral("MS Shell Dlg 2"));
	//font.setFamily(QStringLiteral("Segoe UI"));
	font.setPixelSize(11);
	setFont(font);
#endif

	// Initialize dark UI widget style
	m_darkStyle = new DarkStyle();

	// Create asynchronous IO worker thread
	m_asyncIo = AsyncIO::createWorker();

	// Initialize application settings from file
	m_appSettings = new AppSettings(m_dataDir.filePath("Application.config"));

	// Initialize translations, TODO

	// Initialize Libav
	av_log_set_callback(libavLogHandler);
	av_log_set_level(LIBAV_LOG_LEVEL); // Actually ignored by Libav
	av_register_all();
#if 0
	AVOutputFormat *format = av_oformat_next(NULL);
	if(format == NULL) {
		appLog(Log::Warning)
			<< QStringLiteral("No Libav output formats have been registered!");
	} else {
		appLog() << QStringLiteral("Registered Libav output formats:");
		while(format != NULL) {
			appLog() << QStringLiteral("  - %1 (\"%2\")")
				.arg(format->name)
				.arg(format->long_name);
			format = av_oformat_next(format);
		}
	}
#endif // 0
#if 0
	AVCodec *codec = av_codec_next(NULL);
	if(codec == NULL) {
		appLog(Log::Warning)
			<< QStringLiteral("No Libav codecs have been registered!");
	} else {
		appLog() << QStringLiteral("Registered Libav codecs:");
		while(codec != NULL) {
			appLog() << QStringLiteral("  - %1 (\"%2\")")
				.arg(codec->name)
				.arg(codec->long_name);
			codec = av_codec_next(codec);
		}
	}
#endif // 0

	// Log system information
	logSystemInfo();

	// Create audio source manager
	m_audioManager = new AudioSourceManager();
	m_audioManager->initialize();

	// Create video source manager. WARNING: Our video source interface assumes
	// that it'll receive Qt signals for queued frames before the profile
	// actually renders the scene for the same frame. Qt signals are emitted in
	// the order they are connected so we must always always connect profile
	// signals after the video source manager.
	m_videoManager = new VideoSourceManager();
	m_videoManager->initialize();

	// Create capture manager and connect its required signals
	CaptureManager *mgr = CaptureManager::initializeManager();
	if(mgr == NULL)
		return false; // Manager failed to initialize for some reason
	connect(mgr, &CaptureManager::enterLowJitterMode,
		this, &Application::capEnterLowJitterMode);
	connect(mgr, &CaptureManager::exitLowJitterMode,
		this, &Application::capExitLowJitterMode);
	connect(this, &Application::queuedFrameEvent,
		mgr, &CaptureManager::queuedFrameEvent);
	connect(this, &Application::lowJitterRealTimeFrameEvent,
		mgr, &CaptureManager::lowJitterRealTimeFrameEvent);
	connect(this, &Application::realTimeFrameEvent,
		mgr, &CaptureManager::realTimeFrameEvent);
	connect(this, &Application::queuedFrameEvent,
		mgr, &CaptureManager::queuedFrameEvent);
	mgr->setGraphicsContext(m_gfxContext); // Should still be NULL

	//-------------------------------------------------------------------------

	// Get list of profiles and make sure the active profile exists
	QVector<QString> profiles = Profile::queryAvailableProfiles();
	bool profileExists = false;
	for(int i = 0; i < profiles.size(); i++) {
		if(profiles.at(i) == m_appSettings->getActiveProfile()) {
			profileExists = true;
			break;
		}
	}
	if(!profileExists) {
		// Select the first valid profile from the list
		if(profiles.size())
			m_appSettings->setActiveProfile(profiles.at(0));
		else // No profiles exist, create a new one
			m_appSettings->setActiveProfile(QStringLiteral("Default"));
	}

	// Initialize global menu bar
	updateMenuBar();

	// Create main window
	m_mainWindow = new MainWindow();
	m_mainWindow->useEmptyPage();

	// Show the main window
#ifdef Q_OS_WIN
	// `QWidget::saveGeometry()` is broken on Windows. See `MainWindow` for a
	// more detailed explanation.
	if(m_appSettings->getMainWinGeomMaxed())
		m_mainWindow->showMaximized();
	else
		m_mainWindow->show();
#else
	m_mainWindow->show();
#endif

	// Forcefully raise the main window in an attempt to make sure that our
	// application is in the foreground when it is launched from the MSI
	// installer
	m_mainWindow->raise();
	m_mainWindow->activateWindow();

	// Hide the splash screen
	hideLauncherSplash();

	// Load our profile
	bool profileCreated = false;
	changeActiveProfile(m_appSettings->getActiveProfile(), &profileCreated,
		false, false, true);

	// If the profile didn't already exist then we want to show the "create new
	// profile" wizard immediately
	if(profileCreated)
		showNewProfileWizard(true);

	return true;
}

void Application::processOurEvents(bool fromQtExecTimer)
{
	// We have three types of events to dispatch: Real-time ticks, real-time
	// frames and queued frames. The dispatcher needs to handle two different
	// situations: Processing faster than real-time and processing slower than
	// real-time. "Frames" are events that are emitted at the user's specified
	// video framerate while "ticks" are emitted at a frequency as close as
	// 60Hz as possible calculated as an integer multiple of the video
	// framerate. We also need to trigger Qt event processing when it is
	// reasonable to do so.
	//
	// Real-time events are those that should be executed as close as possible
	// to their target frequency but it is okay if some are skipped (Either due
	// to the process not being woken up on time or if we have too much other
	// stuff to process). Queued events are those that should always be emitted
	// but it is okay if they are delayed. Ideally real-time events will never
	// be skipped and queued events will be emitted immediately when they are
	// due.
	//
	// Examples of the three types of events:
	//   - Real-time ticks: Preview rendering and audio capture
	//   - Real-time frames: Audio pane update, audio mixing, hook monitoring
	//     and non-buffered window capture
	//   - Queued frames: Canvas rendering and buffered window capture

	while(!m_exiting && !m_changeMainLoopFreq) {
		bool processedSomething = false;
		quint64 curTime;

		// Process real-time frames. TODO: Other parts of the application rely
		// on frames being processed before ticks so they can guarentee their
		// order of processing. We should add "late" signals that are emitted
		// immediately after the "early" signals to work around this.
		quint64 lateBy = 0;
		curTime = getUsecSinceExec();
		uint latest = calcLatestFrameForTime(curTime, lateBy);
		if(latest >= m_nextRTFrameNum) {
			uint dropped = qMax(0U, latest - m_nextRTFrameNum);
			//if(dropped)
			//	appLog() << dropped << " dropped frames";
			m_nextRTFrameNum = latest + 1; // Next frame to process

			// "Low jitter" slots must execute as quickly as possible to ensure
			// that other "low jitter" slots are actually low jitter as well.
			//appLog() << "*** Real-time frame ***";
			emit lowJitterRealTimeFrameEvent(dropped, lateBy);
			emit realTimeFrameEvent(dropped, lateBy);
			processedSomething = true;

			// Measure average jitter for debug purposes. WARNING: We misuse
			// the term "jitter" as we're actually measuring how late we are
			// instead of the variation of actual values.
#define DEBUG_JITTER 0
#if DEBUG_JITTER
			const int AVERAGE_JITTER_SAMPLES = 150;
			m_averageFrameJitter =
				((double)m_averageFrameJitter *
				(double)(AVERAGE_JITTER_SAMPLES - 1) + (double)lateBy) /
				(double)AVERAGE_JITTER_SAMPLES;
			if(m_nextRTFrameNum % 30 == 0) {
				appLog() << QStringLiteral("Average frame lateness = %L1 usec")
					.arg(m_averageFrameJitter, 0, 'f', 3);
			}
#endif // DEBUG_JITTER
		}

		// Process real-time ticks
		lateBy = 0;
		curTime = getUsecSinceExec();
		latest = calcLatestTickForTime(curTime, lateBy);
		if(latest >= m_nextRTTickNum) {
			uint dropped = qMax(0U, latest - m_nextRTTickNum);
			//if(dropped)
			//	appLog() << dropped << " dropped ticks";
			m_nextRTTickNum = latest + 1; // Next tick to process

			//appLog() << "*** Real-time tick ***";
			emit realTimeTickEvent(dropped, lateBy);
			processedSomething = true;
		}

		//---------------------------------------------------------------------
		// Process queued frames until it's time for the next tick or until we
		// have no more frames to process

		// If there is more than X second worth of frames queued then enter low
		// CPU usage mode in order to catch up with real time. Our video
		// encoders will detect this once they process a frame.
		const float NUM_SECS_UNTIL_LOW_CPU_MODE = 0.333f;
		uint framesToProcess = m_nextRTFrameNum - m_nextQFrameNum;
		//appLog() << framesToProcess << " queued frames to process";
		if((float)framesToProcess > m_curVideoFreq.asFloat() *
			NUM_SECS_UNTIL_LOW_CPU_MODE)
		{
			m_isInLowCPUMode = true;
		}

		// Process as many frames as possible
		for(;;) {
			if(m_nextQFrameNum >= m_nextRTFrameNum)
				break; // No more frames to process

			// Emit signals for every frame that hasn't processed yet if we
			// need to (E.g. rendering a video) or just otherwise the last
			// frame (E.g. previewing on the screen only).
			if(m_processAllFramesRef > 0) {
				// Process every frame
				m_nextQFrameNum++;
				//appLog() << "*** Queued frame ***";
				emit queuedFrameEvent(m_nextQFrameNum - 1, 0);
			} else {
				// Only process the last frame
				uint dropped =
					qMax(0U, m_nextRTFrameNum - m_nextQFrameNum - 1);
				//if(dropped)
				//	appLog() << dropped << " dropped queued frames";
				m_nextQFrameNum = m_nextRTFrameNum;
				//appLog() << "*** Queued frame (With drops) ***";
				emit queuedFrameEvent(m_nextQFrameNum - 1, dropped);
			}
			processedSomething = true;

			// Flush the graphics context after every frame so that shared
			// textures in graphics hooks can be reused without causing
			// synchronisation issues.
			if(m_gfxContext != NULL && m_gfxContext->isValid())
				m_gfxContext->flush();

			// Is it time for the next tick? If so stop processing frames
			lateBy = 0;
			curTime = getUsecSinceExec();
			latest = calcLatestTickForTime(curTime, lateBy);
			if(latest >= m_nextRTTickNum)
				break;
		}

		m_isInLowCPUMode = false;

		//---------------------------------------------------------------------

		// Process all Qt events without blocking. WARNING: Qt blocks when the
		// user resizes or moves a window so this method will create a timer
		// that recurses back here so that the above code executes. We must
		// keep this in mind to prevent infinite recursion.
		if(!fromQtExecTimer)
			processQtNonblocking();

		// Continue looping until we have nothing left to process or if we were
		// executed via our Qt timer
		if(!processedSomething || fromQtExecTimer)
			break;
	}
}

void Application::processQtNonblocking()
{
	beginQtExecTimer();
	processEvents();
	sendPostedEvents(NULL, QEvent::DeferredDelete);
	stopQtExecTimer();
}

/// <summary>
/// Processes the miscellaneous things that should be done once per tick.
/// </summary>
void Application::miscRealTimeTickEvent(int numDropped, int lateByUsec)
{
	// Raise main window if another instance was started
	bool raiseWin = m_shm->getRaiseWindow();
	if(raiseWin) {
		appLog()
			<< "User attempted to start a second instance, raising window";
		m_mainWindow->show();
		m_mainWindow->raise();
		m_mainWindow->activateWindow();
		m_shm->setRaiseWindow(false);
	}

	int msecSinceLastProcess =
		((numDropped + 1) * 1000 * m_mainLoopFreq.denominator) /
		m_mainLoopFreq.numerator;

	// Flush the log every X seconds or so. We don't care for accuracy.
	const int LOG_FLUSH_PERIOD = 5 * 1000; // 5 seconds
	m_timeSinceLogFileFlush += msecSinceLastProcess;
	if(m_timeSinceLogFileFlush > LOG_FLUSH_PERIOD) {
		//appLog() << "*** Log flush ***";
		if(!LogFileManager::getSingleton()->isBufferEmpty())
			m_asyncIo->flushLogFile();
		m_timeSinceLogFileFlush = 0;
	}

	// Automatically save the current profile and global settings every X
	// minutes or so. We don't care for accuracy.
	const int PROFILE_SAVE_PERIOD = 5 * 60 * 1000; // 5 minutes
	m_timeSinceProfileSave += msecSinceLastProcess;
	if(m_timeSinceProfileSave > PROFILE_SAVE_PERIOD) {
		//appLog() << "*** Profile save ***";
		if(m_profile != NULL)
			m_profile->saveToDisk();
		m_appSettings->saveToDisk();
		m_timeSinceProfileSave = 0;
	}

	// When changing profiles or scenes we disable updates on the main window
	// to make it repaint nicer. Once the graphics context is created and is
	// ready to be rendered on we know that the profile is now fully loaded.
	if(m_mainWindow != NULL && !m_mainWindow->updatesEnabled() &&
		m_gfxContext != NULL && m_gfxContext->isValid())
	{
		// HACK: Ensure it's done after the widget has been rendered
		QTimer::singleShot(10, this, SLOT(enableUpdatesTimeout()));
	}
}

void Application::enableUpdatesTimeout()
{
	if(m_mainWindow != NULL && !m_mainWindow->updatesEnabled() &&
		m_gfxContext != NULL && m_gfxContext->isValid())
	{
		m_mainWindow->setUpdatesEnabled(true);
	}
}

/// <summary>
/// Processes the miscellaneous things that should be done once per frame.
/// </summary>
void Application::miscRealTimeFrameEvent(int numDropped, int lateByUsec)
{
	// Recreate the mouse cursor texture no more than once per frame
	resetSystemCursorInfo();
}

/// <summary>
/// Calculates which tick was just before the specified time, how many ticks
/// were missed and how close we are to the latest tick.
/// </summary>
/// <returns>The ID of the latest tick</returns>
uint Application::calcLatestTickForTime(quint64 curTime, quint64 &lateByOut)
{
	// Calculate the latest tick that can be processed right now. We do this
	// by incrementing our tick number until the tick time stamp is in the
	// future then decrement one.
	quint64 nextTick = m_nextRTTickNum; // We do 64-bit maths below
	for(;;nextTick++) {
		quint64 tickTime =
			(nextTick * 1000000 * m_mainLoopFreq.denominator) /
			m_mainLoopFreq.numerator + m_tickTimeOrigin;
		if(tickTime > curTime)
			break;
	}
	nextTick--;
	if(nextTick < m_nextRTTickNum) {
		// No new ticks to process
		lateByOut = 0;
		return m_nextRTTickNum - 1;
	}

	// How many ticks did we miss since the last time we processed?
	int droppedTicks = nextTick - m_nextRTTickNum;
	//if(droppedTicks) {
	//	appLog()
	//		<< "Dropped " << droppedTicks << " realtime ticks in main loop";
	//}

	// Determine how late this tick is. Up to 25% late is okay
	quint64 tickTime =
		(nextTick * 1000000 * m_mainLoopFreq.denominator) /
		m_mainLoopFreq.numerator + m_tickTimeOrigin;
	//quint64 allowedLate =
	//	(1000000 * m_mainLoopFreq.denominator) / m_mainLoopFreq.numerator / 4;
	quint64 lateBy = curTime - tickTime;
	//if(lateBy > allowedLate)
	//	appLog() << "Tick late by: " << lateBy << " usec";

	lateByOut = lateBy;
	return nextTick;
}

/// <summary>
/// Calculates which frame was just before the specified time, how many frames
/// were missed and how close we are to the latest frame.
/// </summary>
/// <returns>The ID of the latest frame</returns>
uint Application::calcLatestFrameForTime(quint64 curTime, quint64 &lateByOut)
{
	// Calculate the latest frame that can be processed right now. We do this
	// by incrementing our frame number until the frame time stamp is in the
	// future then decrement one.
	quint64 nextFrame = m_nextRTFrameNum; // We do 64-bit maths below
	for(;;nextFrame++) {
		quint64 frameTime =
			(nextFrame * 1000000 * m_curVideoFreq.denominator) /
			m_curVideoFreq.numerator + m_tickTimeOrigin;
		if(frameTime > curTime)
			break;
	}
	nextFrame--;
	if(nextFrame < m_nextRTFrameNum) {
		// No new frames to process
		lateByOut = 0;
		return m_nextRTFrameNum - 1;
	}

	// How many frames did we miss since the last time we processed?
	int droppedFrames = nextFrame - m_nextRTFrameNum;
	//if(droppedFrames) {
	//	appLog()
	//		<< "Dropped " << droppedFrames << " realtime frames in main loop";
	//}

	// Determine how late this frame is. Up to 25% late is okay
	quint64 frameTime =
		(nextFrame * 1000000 * m_curVideoFreq.denominator) /
		m_curVideoFreq.numerator + m_tickTimeOrigin;
	//quint64 allowedLate =
	//	(1000000 * m_curVideoFreq.denominator) / m_curVideoFreq.numerator / 4;
	quint64 lateBy = curTime - frameTime;
	//if(lateBy > allowedLate)
	//	appLog() << "Frame late by: " << lateBy << " usec";

	lateByOut = lateBy;
	return nextFrame;
}

/// <summary>
/// As we use our own custom main loop instead of Qt's we need to handle
/// specific OS oddities ourselves. Sometimes Qt's `processEvents()` blocks for
/// a long period of time for OS reasons but we still want to keep processing
/// our ticks at their scheduled time. We work around this by creating a Qt
/// timer that detects when Qt's processing is most likely blocking and we then
/// use that timer to call back to our custom event handling to keep things
/// responsive. One example of when this happens is when the user drags a
/// window or dialog around the screen with the mouse.
/// </summary>
void Application::beginQtExecTimer()
{
	m_qtExecTimer.start(10); // 10ms for first event
}

void Application::processQtExecTimer()
{
	processOurEvents(true);
	m_qtExecTimer.start(1); // 1ms for second and later events
}

void Application::stopQtExecTimer()
{
	m_qtExecTimer.stop();
}

/// <summary>
/// Executed after the main loop exits. Cleans up any loose ends.
/// </summary>
/// <returns>The return code of the whole application.</returns>
int Application::shutdown(int returnCode)
{
	// Stop broadcasting
	setBroadcasting(false);

	// Destroy wizard window if it exists
	delete m_wizardWindow;
	m_wizardWindow = NULL;

	// Destroy scene layer dialog window if it exists
	delete m_layerDialogWindow;
	m_layerDialogWindow = NULL;

	// Destroy bitrate calculator window if it exists
	delete m_bitrateCalcWindow;
	m_bitrateCalcWindow = NULL;

	// Destroy about window if it exists
	delete m_aboutWindow;
	m_aboutWindow = NULL;

	// Save profile to disk. Must be done while the graphics context still
	// exists.
	if(m_deleteSettingsOnExit) {
		// User cancelled during the first time profile setup wizard. Make sure
		// that the wizard executes the next time Mishira runs by deleting the
		// temporary profile.
		QString filePath = Profile::getProfileFilePath(m_profile->getName());
		delete m_profile;
		m_profile = NULL;
		QFile::remove(filePath);
	} else {
		delete m_profile;
		m_profile = NULL;
	}

	// Destroy the main window. Must be after all of its child windows and
	// anything that uses the graphics context
	delete m_mainWindow;
	m_mainWindow = NULL;

	// Destroy capture manager
	CaptureManager::destroyManager();

	// Destroy video source manager
	delete m_videoManager;
	m_videoManager = NULL;

	// Destroy audio source manager
	delete m_audioManager;
	m_audioManager = NULL;

	// Save application settings to disk
	if(m_deleteSettingsOnExit) {
		// User cancelled during the first time profile setup wizard. Make sure
		// that everything is the same next time by deleting our existing
		// settings.
		QString filePath = m_appSettings->getFilename();
		delete m_appSettings;
		m_appSettings = NULL;
		QFile::remove(filePath);
	} else {
		delete m_appSettings;
		m_appSettings = NULL;
	}

	// Destroy global menu bar
	delete m_menuBar;
	m_menuBar = NULL;

	// Destroy asynchronous IO worker thread
	AsyncIO::destroyWorker();
	m_asyncIo = NULL;

	// Destroy dark UI widget style
	delete m_darkStyle;
	m_darkStyle = NULL;

	// Close log file
	appLog()
		<< LOG_DOUBLE_LINE
		<< "\nCleanly shut down with return code " << returnCode;
	delete LogFileManager::getSingleton(); // Flushes and closes file

	// Free the shared memory segment manager after making sure that the
	// process ID is cleaned up. Note that this doesn't actually delete the
	// segment as it's persistent.
	m_shm->lock();
	m_shm->setMainProcessId(0);
	m_shm->unlock();
	delete m_shm;
	m_shm = NULL;

	return returnCode;
}

/// <summary>
/// Queries the operating system for the directory that the application should
/// store its per-user data in.
/// </summary>
QString Application::calcDataDirectoryPath()
{
	// We don't want to include our company name in the path name. This returns
	// "Local" on Windows instead of "Roaming" so we reimplement this method in
	// `WinApplication` for a custom implementation.
	setOrganizationName(NULL);
	QString str =
		QStandardPaths::writableLocation(QStandardPaths::DataLocation);
	setOrganizationName(ORG_NAME);
	return str;
}

void Application::showLayerDialog(LayerDialog *dialog)
{
	// Recreate the scene layer dialog window so that it nicely animates in and
	// doesn't have visual glitches when the dialog contents have a different
	// size than last time
	if(m_layerDialogWindow != NULL)
		delete m_layerDialogWindow;
	m_layerDialogWindow = new LayerDialogWindow(m_mainWindow);

	m_layerDialogWindow->setDialog(dialog);
	m_layerDialogWindow->show();
}

void Application::showWizard(WizController controller)
{
	// Recreate the wizard window so that it nicely animates in and
	// doesn't have visual glitches when it is shown for the second time.
	if(m_wizardWindow != NULL)
		delete m_wizardWindow;
	m_wizardWindow = new WizardWindow(m_mainWindow);

	m_wizardWindow->setController(controller);
	m_wizardWindow->show();
}

/// <summary>
/// Prepares the wizard window for display similar to `showWizard()` but
/// doesn't actually set the controller or display the window. It is up to the
/// caller to call `setController()` and `show()` on the returned object.
/// </summary>
WizardWindow *Application::showWizardAdvancedHACK()
{
	// Recreate the wizard window so that it nicely animates in and
	// doesn't have visual glitches when it is shown for the second time.
	if(m_wizardWindow != NULL)
		delete m_wizardWindow;
	m_wizardWindow = new WizardWindow(m_mainWindow);
	return m_wizardWindow;
}

void Application::showNewProfileWizard(bool isFirstTime)
{
	// HACK: An identical version of `showWizard()` except sets a shared wizard
	// variable to choose which variation of the wizard to show.

	// Recreate the wizard window so that it nicely animates in and
	// doesn't have visual glitches when it is shown for the second time.
	if(m_wizardWindow != NULL)
		delete m_wizardWindow;
	m_wizardWindow = new WizardWindow(m_mainWindow);

	m_wizardWindow->getShared()->profIsWelcome = isFirstTime;
	m_wizardWindow->setController(WizNewProfileController);
	m_wizardWindow->show();
}

void Application::showBitrateCalculator()
{
	// Recreate the bitrate calculator window so that it nicely animates in and
	// doesn't have visual glitches when it is shown for the second time. Don't
	// add a parent so that other modal windows don't conflict with it.
	if(m_bitrateCalcWindow != NULL)
		delete m_bitrateCalcWindow;
	m_bitrateCalcWindow = new BitrateCalcWindow(NULL);

	m_bitrateCalcWindow->show();
}

void Application::showAboutWindow()
{
	// Recreate the about window so that it nicely animates in and
	// doesn't have visual glitches when it is shown for the second time.
	if(m_aboutWindow != NULL)
		delete m_aboutWindow;
	m_aboutWindow = new AboutWindow(m_mainWindow);

	m_aboutWindow->show();
}

/// <summary>
/// As we're not using Qt's event loop `QCoreApplication::exit()` does
/// absolutely nothing. Because of this we need to make sure this method is
/// always called instead of Qt's.
///
/// Also note that QGuiApplication does not emit any "lastWindowClosed" signals
/// if the Qt event loop isn't running which also means we need to detect when
/// the last window closes ourselves.
/// </summary>
void Application::exit(int returnCode)
{
	appLog()
		<< "Received exit signal with return code " << returnCode
		<< ", shutting down";
	m_exiting = true;
	m_exitCode = returnCode;
	QApplication::exit(returnCode); // Does nothing but do it anyway
}

/// <summary>
/// Same as `exit(0)`.
/// </summary>
void Application::quit()
{
	exit(0);
}

void Application::showDeleteLayerDialog(SceneItem *item)
{
	if(item == NULL)
		return;

	// Create dialogs here so they are always centered above the main window
	QMessageBox *box;
	if(item->isGroup()) {
		box = new QMessageBox(m_mainWindow);
		box->setWindowModality(Qt::ApplicationModal);
		box->setWindowTitle(tr("Confirm deletion"));
		box->setText(
			tr("Are you sure you want to delete \"%1\" and all of its "
			"contents?")
			.arg(item->getName()));
		box->setIcon(QMessageBox::Question);
		box->setStandardButtons(
			QMessageBox::Ok | QMessageBox::Cancel);
		box->setDefaultButton(QMessageBox::Cancel);
		QAbstractButton *btn = box->button(QMessageBox::Ok);
		btn->setText(tr("  Delete group and contents  ")); // Rename "OK"
		connect(box, &QMessageBox::buttonClicked,
			this, &Application::deleteLayerGroupDialogClosed);
		if(m_deleteLayerGroupDialog != NULL)
			delete m_deleteLayerGroupDialog; // Delayed delete
		m_deleteLayerGroupDialog = box;
	} else {
		box = new QMessageBox(m_mainWindow);
		box->setWindowModality(Qt::ApplicationModal);
		box->setWindowTitle(tr("Confirm deletion"));
		box->setText(tr("Are you sure you want to delete \"%1\"?")
			.arg(item->getName()));
		box->setIcon(QMessageBox::Question);
		box->setStandardButtons(
			QMessageBox::Ok | QMessageBox::Cancel);
		box->setDefaultButton(QMessageBox::Cancel);
		QAbstractButton *btn = box->button(QMessageBox::Ok);
		btn->setText(tr("  Delete layer  ")); // Rename "OK"
		btn->setContentsMargins(10, 0, 10, 0);
		connect(box, &QMessageBox::buttonClicked,
			this, &Application::deleteLayerDialogClosed);
		if(m_deleteLayerDialog != NULL)
			delete m_deleteLayerDialog; // Delayed delete
		m_deleteLayerDialog = box;
	}

	box->setProperty(
		"itemPtr", qVariantFromValue(reinterpret_cast<void *>(item)));
	box->show();
}

/// <summary>
/// WARNING: Must only be called from within the wizard window.
/// </summary>
void Application::showDeleteTargetDialog(Target *target)
{
	if(target == NULL)
		return;

	// Create dialogs here so they are always centered above the main window
	QMessageBox *box = new QMessageBox(m_wizardWindow);
	box->setWindowModality(Qt::ApplicationModal);
	box->setWindowTitle(tr("Confirm deletion"));
	box->setText(tr("Are you sure you want to delete \"%1\"?")
		.arg(target->getName()));
	box->setIcon(QMessageBox::Question);
	box->setStandardButtons(
		QMessageBox::Ok | QMessageBox::Cancel);
	box->setDefaultButton(QMessageBox::Cancel);
	QAbstractButton *btn = box->button(QMessageBox::Ok);
	btn->setText(tr("  Delete target  ")); // Rename "OK"
	btn->setContentsMargins(10, 0, 10, 0);
	connect(box, &QMessageBox::buttonClicked,
		this, &Application::deleteTargetDialogClosed);
	if(m_deleteTargetDialog != NULL)
		delete m_deleteTargetDialog; // Delayed delete
	m_deleteTargetDialog = box;

	box->setProperty(
		"targetPtr", qVariantFromValue(reinterpret_cast<void *>(target)));
	box->show();
}

void Application::showDeleteProfileDialog(const QString &name)
{
	// Create dialogs here so they are always centered above the main window
	QMessageBox *box = new QMessageBox(m_mainWindow);
	box->setWindowModality(Qt::ApplicationModal);
	box->setWindowTitle(tr("Confirm deletion"));
	box->setText(tr("Are you sure you want to delete \"%1\"?")
		.arg(name));
	box->setIcon(QMessageBox::Question);
	box->setStandardButtons(
		QMessageBox::Ok | QMessageBox::Cancel);
	box->setDefaultButton(QMessageBox::Cancel);
	QAbstractButton *btn = box->button(QMessageBox::Ok);
	btn->setText(tr("  Delete profile  ")); // Rename "OK"
	btn->setContentsMargins(10, 0, 10, 0);
	connect(box, &QMessageBox::buttonClicked,
		this, &Application::deleteProfileDialogClosed);
	if(m_deleteProfileDialog != NULL)
		delete m_deleteProfileDialog; // Delayed delete
	m_deleteProfileDialog = box;

	box->setProperty("profileName", qVariantFromValue(name));
	box->show();
}

void Application::showDeleteSceneDialog(Scene *scene)
{
	if(scene == NULL)
		return;
	if(scene->getProfile()->getScenes().count() < 2)
		return; // Cannot delete the last scene

	// Create dialogs here so they are always centered above the main window
	QMessageBox *box = new QMessageBox(m_mainWindow);
	box->setWindowModality(Qt::ApplicationModal);
	box->setWindowTitle(tr("Confirm deletion"));
	box->setText(tr("Are you sure you want to delete \"%1\"?")
		.arg(scene->getName()));
	box->setIcon(QMessageBox::Question);
	box->setStandardButtons(
		QMessageBox::Ok | QMessageBox::Cancel);
	box->setDefaultButton(QMessageBox::Cancel);
	QAbstractButton *btn = box->button(QMessageBox::Ok);
	btn->setText(tr("  Delete scene  ")); // Rename "OK"
	btn->setContentsMargins(10, 0, 10, 0);
	connect(box, &QMessageBox::buttonClicked,
		this, &Application::deleteSceneDialogClosed);
	if(m_deleteSceneDialog != NULL)
		delete m_deleteSceneDialog; // Delayed delete
	m_deleteSceneDialog = box;

	box->setProperty(
		"scenePtr", qVariantFromValue(reinterpret_cast<void *>(scene)));
	box->show();
}

void Application::showRunUpdaterDialog()
{
	// Create dialogs here so they are always centered above the main window
	QMessageBox *box = new QMessageBox(m_mainWindow);
	box->setWindowModality(Qt::ApplicationModal);
	box->setWindowTitle(tr("Confirm exit"));
	box->setText(tr(
		"Are you sure you want to exit %1 in order to update to the latest version?")
		.arg(APP_NAME));
	box->setIcon(QMessageBox::Question);
	box->setStandardButtons(
		QMessageBox::Ok | QMessageBox::Cancel);
	box->setDefaultButton(QMessageBox::Cancel);
	QAbstractButton *btn = box->button(QMessageBox::Ok);
	btn->setText(tr("  Exit and update  ")); // Rename "OK"
	btn->setContentsMargins(10, 0, 10, 0);
	connect(box, &QMessageBox::buttonClicked,
		this, &Application::runUpdaterDialogClosed);
	if(m_runUpdaterDialog != NULL)
		delete m_runUpdaterDialog; // Delayed delete
	m_runUpdaterDialog = box;

	box->show();
}

/// <summary>
/// Make sure the user doesn't start or stop broadcasting to any target
/// accidentally by forcing them to confirm the state change. If `target` is
/// NULL then we are referring to the main broadcast button.
/// </summary>
void Application::showStartStopBroadcastDialog(bool isStart, Target *target)
{
	// Notify the user with a different dialog if no targets are enabled
	TargetList targets = m_profile->getTargets();
	int count = 0;
	for(int i = 0; i < targets.count(); i++) {
		Target *target = targets.at(i);
		if(target->isEnabled()) {
			count++;
			break;
		}
	}
	if(count == 0) {
		showBasicWarningDialog(
			tr("You need at least one enabled target to begin broadcasting."),
			tr("No broadcast targets enabled"));
		return;
	}

	// Create dialogs here so they are always centered above the main window
	QMessageBox *box = new QMessageBox(m_mainWindow);
	box->setWindowModality(Qt::ApplicationModal);
	if(isStart) {
		box->setWindowTitle(tr("Confirm begin broadcast"));
		if(target != NULL) {
			box->setText(
				tr("Are you sure you want to begin broadcasting to \"%1\"?")
				.arg(target->getName()));
		} else
			box->setText(tr("Are you sure you want to begin broadcasting?"));
	} else {
		box->setWindowTitle(tr("Confirm stop broadcast"));
		if(target != NULL) {
			box->setText(
				tr("Are you sure you want to stop broadcasting to \"%1\"?")
				.arg(target->getName()));
		} else
			box->setText(tr("Are you sure you want to stop broadcasting?"));
	}
	box->setIcon(QMessageBox::Question);
	box->setStandardButtons(
		QMessageBox::Ok | QMessageBox::Cancel);
	box->setDefaultButton(QMessageBox::Ok); // Enter = Accept, Esc = Cancel
	QAbstractButton *btn = box->button(QMessageBox::Ok);
	if(isStart)
		btn->setText(tr("  Begin broadcasting  ")); // Rename "OK"
	else
		btn->setText(tr("  Stop broadcasting  ")); // Rename "OK"
	btn->setContentsMargins(10, 0, 10, 0);
	connect(box, &QMessageBox::buttonClicked,
		this, &Application::startStopBroadcastDialogClosed);
	if(m_startStopBroadcastDialog != NULL)
		delete m_startStopBroadcastDialog; // Delayed delete
	m_startStopBroadcastDialog = box;

	// Forward the `isStart` to the closed slot just in case it changes state
	// while the dialog is open
	box->setProperty(
		"targetPtr", qVariantFromValue(reinterpret_cast<void *>(target)));
	box->setProperty("isStart", qVariantFromValue(isStart));
	box->show();
}

void Application::showBasicWarningDialog(
	const QString &message, const QString &title)
{
	QMessageBox *box = new QMessageBox(m_mainWindow);
	box->setWindowModality(Qt::ApplicationModal);
	box->setWindowTitle(title.isEmpty() ? tr("Warning") : title);
	box->setText(message);
	box->setIcon(QMessageBox::Warning);
	box->setStandardButtons(QMessageBox::Ok);
	box->setDefaultButton(QMessageBox::Ok);
	if(m_basicWarningDialog != NULL)
		delete m_basicWarningDialog; // Delayed delete
	m_basicWarningDialog = box;

	box->show();
}

void Application::setGraphicsContext(GraphicsContext *context)
{
	if(context != NULL && m_gfxContext != NULL) {
		appLog(Log::Warning)
			<< "Created a new graphics context when one already exists";
	}
	deleteSystemCursorInfo(); // Clean up
	m_gfxContext = context;

	// Forward to the capture manager
	CaptureManager::getManager()->setGraphicsContext(context);

	if(context == NULL)
		return; // Don't continue if we're destroying the context

	// Make sure that the objects that create hardware resources know when the
	// context has been initialized or destroyed
	if(m_profile != NULL) {
		if(context->isValid())
			m_profile->graphicsContextInitialized(context);
		else {
			connect(context, &GraphicsContext::initialized,
				m_profile, &Profile::graphicsContextInitialized);
		}
		connect(context, &GraphicsContext::destroying,
			m_profile, &Profile::graphicsContextDestroyed);
	}
	Scaler::graphicsContextInitialized(context);
}

/// <summary>
/// Convenience method to return the desktop capture manager.
/// </summary>
CaptureManager *Application::getCaptureManager() const
{
	return CaptureManager::getManager();
}

/// <summary>
/// Changes the mouse cursor so that is has the specified shape. We need this
/// as we cannot change the cursor when the mouse is over the GraphicsWidget
/// with `QWidget::setCursor()` for some reason.
/// </summary>
void Application::setActiveCursor(Qt::CursorShape cursor)
{
	if(m_activeCursor == cursor)
		return; // No change
	if(cursor == Qt::ArrowCursor) {
		// Pop the cursor stack
		restoreOverrideCursor();
		m_activeCursor = cursor;
		return;
	}
	if(m_activeCursor == Qt::ArrowCursor) {
		// Push a new cursor state to the stack
		setOverrideCursor(cursor);
		m_activeCursor = cursor;
		return;
	}
	// Change the current cursor state that's already on the stack
	changeOverrideCursor(cursor);
	m_activeCursor = cursor;
}

void Application::setBroadcasting(bool broadcasting)
{
	if(m_isBroadcastingChanging)
		return; // Prevent recursion
	if(m_isBroadcasting == broadcasting)
		return; // No change
	if(m_profile == NULL)
		return;
	m_isBroadcasting = broadcasting;
	m_isBroadcastingChanging = true;

	if(m_isBroadcasting) {
		appLog()
			<< LOG_SINGLE_LINE << "\n Broadcast begin\n" << LOG_SINGLE_LINE;
		m_broadcastCpuUsage = createCPUUsage();
	}

	// Forward to enabled profile targets
	TargetList targets = m_profile->getTargets();
	int count = 0;
	for(int i = 0; i < targets.count(); i++) {
		Target *target = targets.at(i);
		if(m_isBroadcasting) {
			if(target->isEnabled()) {
				target->setActive(true);
				if(target->isActive()) // Verify that it worked
					count++;
			}
		} else
			target->setActive(false);
	}

	// Must have enabled at least one target
	if(m_isBroadcasting && count == 0) {
		m_isBroadcasting = false;
		appLog(Log::Warning) << "No broadcast targets were enabled";
	} else {
		emit broadcastingChanged(m_isBroadcasting);
		keepSystemAndDisplayActive(m_isBroadcasting);
	}

	if(!m_isBroadcasting) {
		// Get CPU usage statistics
		float totalUsage = 0.0f;
		float procUsage = m_broadcastCpuUsage->getAvgUsage(&totalUsage);
		appLog() << QStringLiteral(
			"Average total CPU usage during broadcast = %1%")
			.arg(qRound(totalUsage * 100.0f));
		appLog() << QStringLiteral(
			"Average Mishira CPU usage during broadcast = %1%")
			.arg(qRound(procUsage * 100.0f));
		delete m_broadcastCpuUsage;
		m_broadcastCpuUsage = NULL;

		appLog()
			<< LOG_SINGLE_LINE << "\n Broadcast end\n" << LOG_SINGLE_LINE;
	}

	// Disable or enable profile menu
	updateMenuBar();

	m_isBroadcastingChanging = false;
}

/// <summary>
/// Convenience method for getting the active scene.
/// </summary>
Scene *Application::getActiveScene() const
{
	Profile *profile = getProfile();
	if(profile == NULL)
		return NULL;
	return profile->getActiveScene();
}

/// <summary>
/// Convenience method for getting the current canvas size.
/// </summary>
QSize Application::getCanvasSize() const
{
	Profile *profile = getProfile();
	if(profile == NULL)
		return QSize(0, 0);
	return profile->getCanvasSize();
}

/// <summary>
/// Convenience method for getting the active scene item.
/// </summary>
SceneItem *Application::getActiveItem() const
{
	Scene *scene = getActiveScene();
	if(scene == NULL)
		return NULL;
	return scene->getActiveItem();
}

/// <summary>
/// Convenience method for getting the active scene layer.
/// </summary>
Layer *Application::getActiveLayer() const
{
	Scene *scene = getActiveScene();
	if(scene == NULL)
		return NULL;
	SceneItem *item = scene->getActiveItem();
	if(item == NULL || item->isGroup())
		return NULL;
	return item->getLayer();
}

/// <summary>
/// Convenience method for setting the status bar label in the main window.
/// </summary>
void Application::setStatusLabel(const QString &msg)
{
	if(m_mainWindow == NULL)
		return;
	MainPage *page = m_mainWindow->getMainPage();
	if(page == NULL)
		return;
	page->setStatusLabel(msg);
}

/// <summary>
/// Changes the active profile to the one specified. If the profile doesn't
/// exist it is created.
/// </summary>
/// <returns>True if the profile was changed</returns>
bool Application::changeActiveProfile(
	const QString &name, bool *createdOut, bool force, bool stayOnEmptyPage,
	bool stayOnEmptyPageIfCreated)
{
	if(createdOut != NULL)
		*createdOut = false;
	if(m_isBroadcasting)
		return false; // Cannot change while broadcasting
	if(!force && m_profile != NULL && m_profile->getName() == name)
		return true; // Already active

	// Uncheck our currently active profile in the menu bar
	for(int i = 0; i < m_profileActions.size(); i++) {
		QAction *action = m_profileActions.at(i);
		if(action->text() == m_appSettings->getActiveProfile()) {
			action->setChecked(false);
			break;
		}
	}

	// Save existing profile to disk if one exists. Must be done while the
	// graphics context (I.e. main window page) still exists.
	if(m_profile != NULL)
		delete m_profile;
	m_profile = NULL;

	// Destroy the main window page as it cannot cleanly handle a complete
	// change of profile. We must process Qt in order for the main window to
	// repaint nicely. We don't use `processQtNonblocking()` as it may crash
	// when loading the first profile as we haven't fully initialized yet.
	m_mainWindow->useEmptyPage();
	processEvents();

	// Notify the main loop that it needs to update its frequency to match the
	// profile settings. We must do this before creating the profile object as
	// its audio mixer will immediately attempt to get a reference time and we
	// do not know if the main loop will need to change frequency until the
	// profile has been fully unserialized.
	m_changeMainLoopFreq = true;

	// Load new profile
	m_appSettings->setActiveProfile(name);
	m_profile = new Profile(
		Profile::getProfileFilePath(m_appSettings->getActiveProfile()));
	if(createdOut != NULL && !m_profile->doesFileExist())
		*createdOut = true; // Brand new profile was created

	// Save the profile immediately to the disk if it's new in order for it to
	// show up in the menu bar
	if(!m_profile->doesFileExist())
		m_profile->saveToDisk();

	if(stayOnEmptyPage ||
		(stayOnEmptyPageIfCreated && createdOut != NULL && *createdOut))
	{
		m_mainWindow->setUpdatesEnabled(true); // HACK
		return true;
	}

	// Return to the main window page. We disable updates to prevent the window
	// from flashing the Window's default window colour
	m_mainWindow->setUpdatesEnabled(false);
	m_mainWindow->useMainPage();

	// Completely update the menu bar and window title. Adds new profiles to
	// the menu and checks it in the list
	updateMenuBar();
	updateWindowTitle();

	return true;
}

/// <summary>
/// Makes the specified widget use the dark UI widget style without affecting
/// any other widget.
/// </summary>
void Application::applyDarkStyle(QWidget *widget) const
{
	QPalette pal = widget->palette();
	pal.setColor(QPalette::Active, QPalette::WindowText, StyleHelper::DarkFg2Color);
	pal.setColor(QPalette::Inactive, QPalette::WindowText, StyleHelper::DarkFg2Color);
	//pal.setColor(QPalette::Disabled, QPalette::WindowText, grey120);
	pal.setColor(QPalette::Button, StyleHelper::DarkBg7Color);
	//pal.setColor(QPalette::Active, QPalette::Midlight, grey227);
	//pal.setColor(QPalette::Inactive, QPalette::Midlight, grey227);
	//pal.setColor(QPalette::Disabled, QPalette::Midlight, grey247);
	//pal.setColor(QPalette::Dark, grey160);
	//pal.setColor(QPalette::Mid, grey160);
	pal.setColor(QPalette::Active, QPalette::Text, StyleHelper::DarkFg2Color);
	pal.setColor(QPalette::Inactive, QPalette::Text, StyleHelper::DarkFg2Color);
	//pal.setColor(QPalette::Disabled, QPalette::Text, grey120);
	pal.setColor(QPalette::Active, QPalette::ButtonText, StyleHelper::DarkFg2Color);
	pal.setColor(QPalette::Inactive, QPalette::ButtonText, StyleHelper::DarkFg2Color);
	//pal.setColor(QPalette::Disabled, QPalette::ButtonText, grey120);
	pal.setColor(QPalette::Active, QPalette::Base, StyleHelper::DarkBg3Color);
	pal.setColor(QPalette::Inactive, QPalette::Base, StyleHelper::DarkBg3Color);
	//pal.setColor(QPalette::Disabled, QPalette::Base, grey240);
	pal.setColor(QPalette::Window, StyleHelper::DarkBg5Color);
	//pal.setColor(QPalette::Active, QPalette::Shadow, grey105);
	//pal.setColor(QPalette::Inactive, QPalette::Shadow, grey105);
	//pal.setColor(QPalette::Inactive, QPalette::Highlight, grey240);
	//pal.setColor(QPalette::AlternateBase, grey246);
	widget->setPalette(pal);

	// Must be after `setPalette()` as the style's `polish()` function makes
	// its own changes to the palette that we don't want to override
	widget->setStyle(m_darkStyle);
}

/// <summary>
/// Returns true if the specified widget is using the dark UI widget style.
/// </summary>
bool Application::isDarkStyle(QWidget *widget) const
{
	if(widget == NULL)
		return false;
	if(widget->style() == m_darkStyle)
		return true;
	return false;
}

/// <summary>
/// Updates the global menu bar to match the current application state. E.g. if
/// a wizard is opened then the user cannot change profiles. Also used to
/// initialize the menu bar during application startup.
/// </summary>
void Application::updateMenuBar()
{
	QMenu *menu;
	QAction *action;

	if(m_menuBar == NULL) {
		// We create a parentless menu bar so that the bar is shared between
		// all of our windows on OS X.
		m_menuBar = new StyledMenuBar();

		//---------------------------------------------------------------------
		// Add the "File" menu

		menu = m_menuBar->addMenu(tr("&File"));

		//action = menu->addAction(tr("&Global settings..."));
		//action->setEnabled(false);

		//action = menu->addAction(tr("&Video sources..."));
		//action->setEnabled(false);

		//action = menu->addAction(tr("&Audio sources..."));
		//action->setEnabled(false);

		//menu->addSeparator();

		action = menu->addAction(tr("E&xit"));
		connect(action, SIGNAL(triggered()),
			this, SLOT(closeAllWindows())); // Slot is static, use old syntax

		//---------------------------------------------------------------------
		// Add the "Profiles" menu

		m_profilesMenu = m_menuBar->addMenu(tr("&Profiles"));
		// Contents are added below

		//---------------------------------------------------------------------
		// Add the "Help" menu

		m_helpMenu = m_menuBar->addMenu(tr("&Help"));
		// Contents are added below
	}

	//-------------------------------------------------------------------------
	// Update the "Profiles" menu contents

	// Clear the profiles menu completely
	menu = m_profilesMenu;
	menu->clear();

	action = menu->addAction(tr("Create &new profile..."));
	if(m_isBroadcasting)
		action->setEnabled(false);
	connect(action, &QAction::triggered,
		this, &Application::newProfileClicked);

	menu->addSeparator();

	// Add all our profiles
	QVector<QString> profiles = Profile::queryAvailableProfiles();
	m_profileActions.clear();
	m_profileActions.reserve(profiles.size());
	for(int i = 0; i < profiles.size(); i++) {
		const QString &name = profiles.at(i);
		action = menu->addAction(name);
		m_profileActions.append(action);
		if(m_isBroadcasting)
			action->setEnabled(false);
		action->setCheckable(true);
		if(name == m_appSettings->getActiveProfile())
			action->setChecked(true);
		connect(action, &QAction::triggered,
			this, &Application::changeProfileClicked);
	}

	menu->addSeparator();

	action = menu->addAction(tr("&Edit current profile..."));
	if(m_isBroadcasting)
		action->setEnabled(false);
	connect(action, &QAction::triggered,
		this, &Application::editProfileClicked);

	action = menu->addAction(tr("&Duplicate current profile"));
	if(m_isBroadcasting)
		action->setEnabled(false);
	connect(action, &QAction::triggered,
		this, &Application::duplicateProfileClicked);

	action = menu->addAction(tr("De&lete current profile..."));
	if(m_isBroadcasting)
		action->setEnabled(false);
	if(profiles.size() < 2)
		action->setEnabled(false);
	connect(action, &QAction::triggered,
		this, &Application::deleteProfileClicked);

	//-------------------------------------------------------------------------
	// Update the "Help" menu contents

	// Clear the profiles menu completely
	menu = m_helpMenu;
	menu->clear();

	action =
		menu->addAction(tr("&Video and audio settings calculator..."));
	connect(action, &QAction::triggered,
		this, &Application::showBitrateCalculator);

	menu->addSeparator();

	action = menu->addAction(tr("View &online manual..."));
	connect(action, &QAction::triggered,
		this, &Application::onlineManualClicked);

	action = menu->addAction(tr("&About Mishira..."));
	connect(action, &QAction::triggered,
		this, &Application::showAboutWindow);
}

/// <summary>
/// Sets the title of the main window.
/// </summary>
void Application::updateWindowTitle()
{
	if(m_mainWindow == NULL)
		return;

	// WARNING: If you change this make sure you update the tutorial's window
	// capture layer as well!

	bool multiProfiles = (Profile::queryAvailableProfiles().size() > 1);
	if(multiProfiles) {
		m_mainWindow->setWindowTitle(QStringLiteral("%1 - %2")
			.arg(APP_NAME).arg(m_appSettings->getActiveProfile()));
	} else
		m_mainWindow->setWindowTitle(APP_NAME);
}

/// <summary>
/// Disables repainting of the main window temporarily. Updates will be
/// automatically enabled after a few milliseconds.
/// </summary>
void Application::disableMainWindowUpdates()
{
	if(m_mainWindow != NULL)
		m_mainWindow->setUpdatesEnabled(false);
}

/// <summary>
/// Triggered when the user clicks a profile name in the profile menu.
/// </summary>
void Application::changeProfileClicked()
{
	// As we don't know what profile was clicked we need to figure it out
	// ourselves based on which item is checked.
	QAction *checkedAction = NULL;
	for(int i = 0; i < m_profileActions.size(); i++) {
		QAction *action = m_profileActions.at(i);
		if(action->isChecked() &&
			action->text() != m_appSettings->getActiveProfile())
		{
			checkedAction = action;
			break;
		}
	}
	if(checkedAction == NULL) {
		// If none are checked then the user clicked our active profile which
		// unchecked it, revert the user action and do nothing else
		for(int i = 0; i < m_profileActions.size(); i++) {
			QAction *action = m_profileActions.at(i);
			if(action->text() == m_appSettings->getActiveProfile()) {
				action->setChecked(true);
				break;
			}
		}
		return;
	}

	// Does the checked profile actually exist?
	bool exists = Profile::doesProfileExist(checkedAction->text());
	if(!exists) {
		// Profile doesn't exist, revert the user action and do nothing else
		checkedAction->setChecked(false);
		return;
	}

	// Changing the active profile handles everything from updating the menu
	// bar to destroying the existing profile
	changeActiveProfile(checkedAction->text());
}

void Application::newProfileClicked()
{
	if(isBroadcasting()) {
		showBasicWarningDialog(
			tr("Cannot create a new profile while broadcasting."));
		return;
	}
	showNewProfileWizard(false);
}

void Application::editProfileClicked()
{
	if(isBroadcasting()) {
		showBasicWarningDialog(
			tr("Cannot modify profile settings while broadcasting."));
		return;
	}
	showWizard(WizEditProfileController);
}

void Application::duplicateProfileClicked()
{
	// Simply append " - Copy" to the profile name. We don't care if it exceeds
	// the maximum profile name length. If the file already exists make it
	// unique
	QString name = m_profile->getName();
	QString oldPath = Profile::getProfileFilePath(name);
	name = tr("%1 - Copy").arg(name);
	QString newPath = Profile::getProfileFilePath(name);
	QFileInfo info = getUniqueFilename(newPath);
	newPath = info.absoluteFilePath();

	// Copy the profile file
	QFile::copy(oldPath, newPath);

	// Update menu bar
	updateMenuBar();
}

void Application::deleteProfileClicked()
{
	showDeleteProfileDialog(m_profile->getName());
}

void Application::deleteLayerDialogClosed(QAbstractButton *button)
{
	// Was the cancel button clicked?
	QMessageBox::ButtonRole role = m_deleteLayerDialog->buttonRole(button);
	if(role != QMessageBox::AcceptRole)
		return;

	// Delete the layer
	SceneItem *item = reinterpret_cast<SceneItem *>(
		m_deleteLayerDialog->property("itemPtr").value<void *>());
	item->deleteItem();

	// Don't delete the dialog here or we will crash
}

void Application::deleteLayerGroupDialogClosed(QAbstractButton *button)
{
	// Was the cancel button clicked?
	QMessageBox::ButtonRole role =
		m_deleteLayerGroupDialog->buttonRole(button);
	if(role != QMessageBox::AcceptRole)
		return;

	// Delete the layer group
	SceneItem *item = reinterpret_cast<SceneItem *>(
		m_deleteLayerGroupDialog->property("itemPtr").value<void *>());
	item->deleteItem();

	// Don't delete the dialog here or we will crash
}

void Application::deleteTargetDialogClosed(QAbstractButton *button)
{
	// Was the cancel button clicked?
	QMessageBox::ButtonRole role = m_deleteTargetDialog->buttonRole(button);
	if(role != QMessageBox::AcceptRole)
		return;

	// Delete the target
	Target *target = reinterpret_cast<Target *>(
		m_deleteTargetDialog->property("targetPtr").value<void *>());
	m_profile->destroyTarget(target);
	m_profile->pruneVideoEncoders();
	//m_profile->pruneAudioEncoders();

	// Don't delete the dialog here or we will crash
}

void Application::deleteProfileDialogClosed(QAbstractButton *button)
{
	// Was the cancel button clicked?
	QMessageBox::ButtonRole role = m_deleteProfileDialog->buttonRole(button);
	if(role != QMessageBox::AcceptRole)
		return;

	QString name =
		m_deleteProfileDialog->property("profileName").value<QString>();

	// If we're deleting the active profile then change to another one first
	if(name == m_profile->getName()) {
		// Change the current profile to the first profile in the list that is
		// not the current one
		QVector<QString> profiles = Profile::queryAvailableProfiles();
		if(profiles.size() < 2)
			return; // We must have at least two profiles
		QString newProfile = profiles.at(0);
		if(newProfile == name)
			newProfile = profiles.at(1);
		changeActiveProfile(newProfile);
	}

	// Delete the specified profile
	QFile::remove(Profile::getProfileFilePath(name));

	// Update the menu bar
	updateMenuBar();

	// Don't delete the dialog here or we will crash
}

void Application::deleteSceneDialogClosed(QAbstractButton *button)
{
	// Was the cancel button clicked?
	QMessageBox::ButtonRole role =
		m_deleteSceneDialog->buttonRole(button);
	if(role != QMessageBox::AcceptRole)
		return;

	// Delete the scene
	Scene *scene = reinterpret_cast<Scene *>(
		m_deleteSceneDialog->property("scenePtr").value<void *>());
	scene->getProfile()->destroyScene(scene);

	// Don't delete the dialog here or we will crash
}

void Application::runUpdaterDialogClosed(QAbstractButton *button)
{
	// Was the cancel button clicked?
	QMessageBox::ButtonRole role = m_runUpdaterDialog->buttonRole(button);
	if(role != QMessageBox::AcceptRole)
		return;

	// Configure program executable and arguments
#ifdef Q_OS_WIN
	QString prog =
		QStringLiteral("%1/wyUpdate.exe").arg(App->applicationDirPath());
	QStringList args;
	//args << "/skipinfo";
#else
#error Unsupported platform
#endif // Q_OS_WIN

	// Exit Mishira and run the updater
	if(QProcess::startDetached(prog, args)) {
		appLog() << QStringLiteral("Exiting %1 in order to start updater")
			.arg(APP_NAME);
		exit(0);
	} else {
		appLog(Log::Warning) << QStringLiteral(
			"Failed to start updater executable");
		setStatusLabel(tr("Failed to start updater executable"));
	}
}

void Application::startStopBroadcastDialogClosed(QAbstractButton *button)
{
	Target *target = reinterpret_cast<Target *>(
		m_startStopBroadcastDialog->property("targetPtr").value<void *>());
	bool isStart =
		m_startStopBroadcastDialog->property("isStart").value<bool>();

	// Was the cancel button clicked?
	QMessageBox::ButtonRole role =
		m_startStopBroadcastDialog->buttonRole(button);
	if(role != QMessageBox::AcceptRole) {
		// If a target is set then we need to reset the check box if the user
		// cancelled
		if(target != NULL)
			target->resetTargetPaneEnabled();
		return;
	}

	// Actually change state
	if(target != NULL) {
		target->setEnabled(isStart);
		if(isStart && m_isBroadcasting)
			target->setActive(true);
		else if(m_isBroadcasting) {
			// Did we disable the only enabled target?
			TargetList targets = m_profile->getTargets();
			bool stillBroadcasting = false;
			for(int i = 0; i < targets.count(); i++) {
				Target *target = targets.at(i);
				if(target->isActive()) {
					stillBroadcasting = true;
					break;
				}
			}
			if(!stillBroadcasting)
				setBroadcasting(false);
		}
	} else
		setBroadcasting(isStart);

	// Don't delete the dialog here or we will crash
}

void Application::onlineManualClicked()
{
	QDesktopServices::openUrl(
		QUrl(QStringLiteral("http://www.mishira.com/manual.html")));
}

void Application::capEnterLowJitterMode()
{
	refLowJitterMode();
}

void Application::capExitLowJitterMode()
{
	derefLowJitterMode();
}
