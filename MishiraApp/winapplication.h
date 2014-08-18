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

#ifndef WINAPPLICATION_H
#define WINAPPLICATION_H

#include "application.h"
#include <windows.h>

typedef void (*hideLauncherSplash_t)();

//=============================================================================
class WinApplication : public Application
{
	Q_OBJECT

public: // Static members -----------------------------------------------------
	static hideLauncherSplash_t	s_hideLauncherSplashPtr;

private: // Members -----------------------------------------------------------
	int				m_disableAeroRef;
	bool			m_disabledAero;

	// Performance timer
	DWORD			m_startTick;
	LONGLONG		m_lastTime;
	LARGE_INTEGER	m_startTime;
	LARGE_INTEGER	m_frequency;
	DWORD_PTR		m_timerMask;

	// OS Scheduler
	uint			m_schedulerPeriod; // ms

	// System cursor
	bool			m_cursorDirty;
	HCURSOR			m_cursorCached;
	Texture *		m_cursorTexture;
	QPoint			m_cursorPos;
	QPoint			m_cursorOffset;
	bool			m_cursorVisible;

public: // Constructor/destructor ---------------------------------------------
	WinApplication(int &argc, char **argv, AppSharedSegment *shm);
	~WinApplication();

public: // Methods ------------------------------------------------------------
	qint64			qpcPosToUsecSinceFrameOrigin(UINT64 u64QPCPosition) const;
	void			refDisableAero();
	void			derefDisableAero();

public: // Interface ----------------------------------------------------------
	virtual void		logSystemInfo();
	virtual void		hideLauncherSplash();
	virtual Texture *	getSystemCursorInfo(
		QPoint *globalPos, QPoint *offset, bool *isVisible);
private:
	virtual void		resetSystemCursorInfo();
	virtual void		deleteSystemCursorInfo();
	virtual void		keepSystemAndDisplayActive(bool enable);
public:
	virtual int			execute();

	// Performance timer
	virtual quint64	getUsecSinceExec();
	virtual quint64	getUsecSinceFrameOrigin();
private:
	virtual bool	beginPerformanceTimer();

	// OS scheduler
	virtual int		detectOsSchedulerPeriod();
	virtual void	setOsSchedulerPeriod(uint desiredMinPeriod);
	virtual void	releaseOsSchedulerPeriod();

	// Miscellaneous OS-specific stuff
	virtual CPUUsage *	createCPUUsage();
protected:
	virtual QString	calcDataDirectoryPath();
};
//=============================================================================

#endif // WINAPPLICATION_H
