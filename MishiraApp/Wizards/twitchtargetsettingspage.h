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

#ifndef TWITCHTARGETSETTINGSPAGE_H
#define TWITCHTARGETSETTINGSPAGE_H

#include "ui_twitchtargetsettingspage.h"

class WizardWindow;
struct WizTargetSettings;

#define USE_PING_WIDGET 0
#if USE_PING_WIDGET
#include <QtCore/QThread>
#include <QtNetwork/QHostInfo>

//=============================================================================
class PingWidgetThread : public QThread
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	quint32		m_ipv4Addr;
	int			m_maxTries;
	int			m_avgResult;
	int			m_numResults;

public: // Constructor/destructor ---------------------------------------------
	PingWidgetThread(quint32 ipv4Addr, int maxTries);
	~PingWidgetThread();

public: // Methods ------------------------------------------------------------
	int				getAvgResult() const;

protected:
	virtual void	run();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			avgUpdated(int avgResult);
};
//=============================================================================

inline int PingWidgetThread::getAvgResult() const
{
	return m_avgResult;
}

//=============================================================================
class PingWidget : public QObject
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	bool				m_runOnce;
	QString				m_hostname;
	QTableWidgetItem *	m_item;
	PingWidgetThread *	m_thread;

public: // Constructor/destructor ---------------------------------------------
	PingWidget(const QString &rtmpUrl, QTableWidgetItem *item);
	~PingWidget();

public: // Methods ------------------------------------------------------------
	void	run();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	lookedUp(const QHostInfo &host);
	void	avgUpdated(int avgResult);
};
//=============================================================================
#endif // USE_PING_WIDGET

//=============================================================================
class TwitchTargetSettingsPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_TwitchTargetSettingsPage	m_ui;
	bool						m_isInitialized;
#if USE_PING_WIDGET
	QVector<PingWidget *>		m_pingWidgets;
#endif // USE_PING_WIDGET

public: // Constructor/destructor ---------------------------------------------
	TwitchTargetSettingsPage(QWidget *parent = NULL);
	~TwitchTargetSettingsPage();

public: // Methods ------------------------------------------------------------
	Ui_TwitchTargetSettingsPage *	getUi();
	void							initialize();
	bool							isValid() const;

	void	sharedReset(WizardWindow *wizWin, WizTargetSettings *defaults);
	void	sharedNext(WizTargetSettings *settings);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	nameEditChanged(const QString &text);
	void	usernameEditChanged(const QString &text);
	void	serverSelectionChanged();
	void	streamKeyEditChanged(const QString &text);
};
//=============================================================================

inline Ui_TwitchTargetSettingsPage *TwitchTargetSettingsPage::getUi()
{
	return &m_ui;
}

#endif // TWITCHTARGETSETTINGSPAGE_H
