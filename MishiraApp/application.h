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

#ifndef APPLICATION_H
#define APPLICATION_H

#include "common.h"
#include "Widgets/styledmenubar.h"
#include <QtCore/QDir>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

class AboutWindow;
class AppSettings;
class AppSharedSegment;
class AudioSourceManager;
class AsyncIO;
class BitrateCalcWindow;
class CaptureManager;
class CPUUsage;
class DarkStyle;
class Layer;
class LayerDialog;
class LayerDialogWindow;
class MainWindow;
class Profile;
class Scene;
class SceneItem;
class Texture;
class VideoSourceManager;
class WizardWindow;
class QAbstractButton;
class QMessageBox;

// Convenience
#define App Application::getSingleton()
#define PROFILE_BEGIN(var) quint64 time_##var = App->getUsecSinceExec()
#define PROFILE_END(var, name) \
	appLog() << QStringLiteral("Profile (%1) = %2 usec") \
	.arg(name).arg(App->getUsecSinceExec() - time_##var)

//=============================================================================
class Application : public QApplication
{
	friend int exec(int argc, char *argv[]);
	Q_OBJECT

private: // Static members ----------------------------------------------------
	static Application *	s_instance;

protected: // Members ---------------------------------------------------------
	AppSharedSegment *		m_shm;
	AppSettings *			m_appSettings;
	Profile *				m_profile;
	StyledMenuBar *			m_menuBar;
	QMenu *					m_profilesMenu;
	QMenu *					m_helpMenu;
	DarkStyle *				m_darkStyle;
	MainWindow *			m_mainWindow;
	WizardWindow *			m_wizardWindow;
	LayerDialogWindow *		m_layerDialogWindow;
	BitrateCalcWindow *		m_bitrateCalcWindow;
	AboutWindow *			m_aboutWindow;
	VidgfxContext *			m_gfxContext;
	AudioSourceManager *	m_audioManager;
	VideoSourceManager *	m_videoManager;
	AsyncIO *				m_asyncIo;
	Qt::CursorShape			m_activeCursor;
	QDir					m_dataDir;
	bool					m_isBroadcasting;
	bool					m_isBroadcastingChanging;
	CPUUsage *				m_broadcastCpuUsage;
	bool					m_isInLowCPUMode;

	// Main loop
	int						m_processTicksRef; // Processes frames as required
	int						m_processAllFramesRef;
	int						m_lowJitterModeRef;
	bool					m_changeMainLoopFreq;
	Fraction				m_mainLoopFreq;
	Fraction				m_curVideoFreq;
	quint64					m_tickTimeOrigin;
	uint					m_nextRTTickNum; // Number of next real-time tick to process
	uint					m_nextRTFrameNum; // Number of next real-time frame to process
	uint					m_nextQFrameNum; // Number of next queued frame to process
	int						m_tickFrameMultiple;
	int						m_timeSinceLogFileFlush; // msec
	int						m_timeSinceProfileSave; // msec
	float					m_averageFrameJitter; // usec
	QTimer					m_qtExecTimer;
	bool					m_exiting;
	int						m_exitCode;
	bool					m_deleteSettingsOnExit; // Used by first time setup

	// Menu bar
	QVector<QAction *>		m_profileActions;

	// Common message boxes
	QMessageBox *			m_deleteLayerDialog;
	QMessageBox *			m_deleteLayerGroupDialog;
	QMessageBox *			m_deleteTargetDialog;
	QMessageBox *			m_deleteProfileDialog;
	QMessageBox *			m_deleteSceneDialog;
	QMessageBox *			m_runUpdaterDialog;
	QMessageBox *			m_startStopBroadcastDialog;
	QMessageBox *			m_basicWarningDialog;

public: // Static methods -----------------------------------------------------
	static Application *	getSingleton();

public: // Constructor/destructor ---------------------------------------------
	Application(int &argc, char **argv, AppSharedSegment *shm);
	~Application();

public: // Methods ------------------------------------------------------------
	void				showLayerDialog(LayerDialog *dialog);
	void				showWizard(WizController controller);
	WizardWindow *		showWizardAdvancedHACK();
	void				showNewProfileWizard(bool isFirstTime);
	void				exit(int returnCode);

	// Message boxes
	void				showDeleteLayerDialog(SceneItem *item);
	void				showDeleteTargetDialog(Target *target);
	void				showDeleteProfileDialog(const QString &name);
	void				showDeleteSceneDialog(Scene *scene);
	void				showRunUpdaterDialog();
	void				showStartStopBroadcastDialog(
		bool isStart, Target *target = NULL);
	void				showBasicWarningDialog(
		const QString &message, const QString &title = QString());

	void					setGraphicsContext(VidgfxContext *context);
	VidgfxContext *			getGraphicsContext() const;
	CaptureManager *		getCaptureManager() const;
	AudioSourceManager *	getAudioSourceManager() const;
	VideoSourceManager *	getVideoSourceManager() const;
	AsyncIO *				getAsyncIO() const;

	void				setActiveCursor(Qt::CursorShape cursor);
	Qt::CursorShape		getActiveCursor() const;
	QDir				getDataDirectory() const;
	Fraction			getMainLoopFrequency() const;
	quint64				frameNumToMsecTimestamp(int frameNum) const;
	void				refProcessTicks();
	void				derefProcessTicks();
	void				refProcessAllFrames();
	void				derefProcessAllFrames();
	void				refLowJitterMode();
	void				derefLowJitterMode();
	bool				isInLowJitterMode();
	bool				processAllFramesEnabled() const;
	void				setBroadcasting(bool broadcasting);
	bool				isBroadcasting() const;
	bool				isInLowCPUMode() const;
	void				setDeleteSettingsOnExit(bool deleteOnExit);

	AppSettings *		getAppSettings() const;
	Profile *			getProfile() const;
	bool				changeActiveProfile(
		const QString &name, bool *createdOut = NULL, bool force = false,
		bool stayOnEmptyPage = false, bool stayOnEmptyPageIfCreated = false);
	QMenuBar *			getMenuBar() const;
	void				updateMenuBar();
	void				updateWindowTitle();
	void				disableMainWindowUpdates();

	DarkStyle *			getDarkStyle() const;
	void				applyDarkStyle(QWidget *widget) const;
	bool				isDarkStyle(QWidget *widget) const;

	// Convenience methods
	Scene *				getActiveScene() const;
	QSize				getCanvasSize() const;
	SceneItem *			getActiveItem() const;
	Layer *				getActiveLayer() const;
	void				setStatusLabel(const QString &msg);

protected:

	// main()
	bool				initialize();
	void				processOurEvents(bool fromQtExecTimer = false);
	void				processQtNonblocking();
	uint				calcLatestTickForTime(
		quint64 curTime, quint64 &lateByOut);
	uint				calcLatestFrameForTime(
		quint64 curTime, quint64 &lateByOut);
	void				beginQtExecTimer();
	void				processQtExecTimer();
	void				stopQtExecTimer();
	int					shutdown(int returnCode);

public: // Interface ----------------------------------------------------------
	virtual void		logSystemInfo() = 0;
	virtual void		hideLauncherSplash() = 0;
	virtual Texture *	getSystemCursorInfo(
		QPoint *globalPos, QPoint *offset, bool *isVisible) = 0;
private:
	virtual void		resetSystemCursorInfo() = 0;
	virtual void		deleteSystemCursorInfo() = 0;
	virtual void		keepSystemAndDisplayActive(bool enable) = 0;
public:
	virtual int			execute() = 0;

	// Performance timer
	virtual quint64		getUsecSinceExec() = 0; // Only use for debugging!
	virtual quint64		getUsecSinceFrameOrigin() = 0;
private:
	virtual bool		beginPerformanceTimer() = 0;

	// OS scheduler
	virtual int			detectOsSchedulerPeriod() = 0;
	virtual void		setOsSchedulerPeriod(uint desiredMinPeriod) = 0;
	virtual void		releaseOsSchedulerPeriod() = 0;

	// Miscellaneous OS-specific stuff
public:
	virtual CPUUsage *	createCPUUsage() = 0;
protected:
	virtual QString		calcDataDirectoryPath();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void				realTimeTickEvent(int numDropped, int lateByUsec);
	void				lowJitterRealTimeFrameEvent(
		int numDropped, int lateByUsec);
	void				realTimeFrameEvent(int numDropped, int lateByUsec);
	void				queuedFrameEvent(uint frameNum, int numDropped);

	void				broadcastingChanged(bool broadcasting);

	public
Q_SLOTS: // Public slots ------------------------------------------------------
	void				showBitrateCalculator();
	void				showAboutWindow();

	private
Q_SLOTS: // Private slots -----------------------------------------------------
	void				quit();
	void				miscRealTimeTickEvent(int numDropped, int lateByUsec);
	void				miscRealTimeFrameEvent(int numDropped, int lateByUsec);
	void				enableUpdatesTimeout();

	void				newProfileClicked();
	void				changeProfileClicked();
	void				editProfileClicked();
	void				duplicateProfileClicked();
	void				deleteProfileClicked();
	void				deleteLayerDialogClosed(QAbstractButton *button);
	void				deleteLayerGroupDialogClosed(QAbstractButton *button);
	void				deleteTargetDialogClosed(QAbstractButton *button);
	void				deleteProfileDialogClosed(QAbstractButton *button);
	void				deleteSceneDialogClosed(QAbstractButton *button);
	void				runUpdaterDialogClosed(QAbstractButton *button);
	void				startStopBroadcastDialogClosed(
		QAbstractButton *button);
	void				logDirectoryClicked();
	void				onlineManualClicked();

	void				capEnterLowJitterMode();
	void				capExitLowJitterMode();
};
//=============================================================================

inline Application *Application::getSingleton()
{
	return s_instance;
}

inline VidgfxContext *Application::getGraphicsContext() const
{
	return m_gfxContext;
}

inline AudioSourceManager *Application::getAudioSourceManager() const
{
	return m_audioManager;
}

inline VideoSourceManager *Application::getVideoSourceManager() const
{
	return m_videoManager;
}

inline AsyncIO *Application::getAsyncIO() const
{
	return m_asyncIo;
}

inline Qt::CursorShape Application::getActiveCursor() const
{
	return m_activeCursor;
}

inline QDir Application::getDataDirectory() const
{
	return m_dataDir;
}

inline Fraction Application::getMainLoopFrequency() const
{
	return m_mainLoopFreq;
}

/// <summary>
/// Returns the timestamp of the specified frame number in milliseconds past
/// an arbitrary, but constant, origin.
/// </summary>
inline quint64 Application::frameNumToMsecTimestamp(int frameNum) const
{
	return (quint64)frameNum * 1000ULL * (quint64)m_curVideoFreq.denominator /
		(quint64)m_curVideoFreq.numerator;
}

/// <summary>
/// The application will emit tick events only if one or more objects reference
/// this flag. The tick frequency is as close to the monitor framerate as
/// possible and is designed to be used for screen rendering.
/// </summary>
inline void Application::refProcessTicks()
{
	m_processTicksRef++;
}

inline void Application::derefProcessTicks()
{
	if(m_processTicksRef > 0)
		m_processTicksRef--;
}

/// <summary>
/// If one or more objects reference this flag then the application will emit
/// a separate queued frame event for every frame instead of dropping frames if
/// we cannot keep up with realtime. Dropping frames at this stage of the
/// pipeline should only be done to save system resources if the user is only
/// previewing the output on the screen and not broadcasting. NOTE: This is
/// also used to enable the output video scalers and, by extension, the video
/// encoders themselves.
/// </summary>
inline void Application::refProcessAllFrames()
{
	m_processAllFramesRef++;
}

inline void Application::derefProcessAllFrames()
{
	if(m_processAllFramesRef > 0)
		m_processAllFramesRef--;
}

inline void Application::refLowJitterMode()
{
	m_lowJitterModeRef++;
}

inline void Application::derefLowJitterMode()
{
	if(m_lowJitterModeRef > 0)
		m_lowJitterModeRef--;
}

inline bool Application::isInLowJitterMode()
{
	return m_lowJitterModeRef > 0;
}

inline bool Application::processAllFramesEnabled() const
{
	return m_processAllFramesRef > 0;
}

inline bool Application::isBroadcasting() const
{
	return m_isBroadcasting;
}

inline bool Application::isInLowCPUMode() const
{
	return m_isInLowCPUMode;
}

inline void Application::setDeleteSettingsOnExit(bool deleteOnExit)
{
	m_deleteSettingsOnExit = deleteOnExit;
}

inline AppSettings *Application::getAppSettings() const
{
	return m_appSettings;
}

inline Profile *Application::getProfile() const
{
	return m_profile;
}

inline QMenuBar *Application::getMenuBar() const
{
	return m_menuBar;
}

inline DarkStyle *Application::getDarkStyle() const
{
	return m_darkStyle;
}

#endif // APPLICATION_H
