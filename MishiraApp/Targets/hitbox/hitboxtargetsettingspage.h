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

#ifndef HITBOXTARGETSETTINGSPAGE_H
#define HITBOXTARGETSETTINGSPAGE_H

#include "ui_hitboxtargetsettingspage.h"

class WizardWindow;
struct WizTargetSettings;

//=============================================================================
class HitboxTargetSettingsPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_HitboxTargetSettingsPage	m_ui;
	bool						m_isInitialized;
#if USE_PING_WIDGET
	QVector<PingWidget *>		m_pingWidgets;
#endif // USE_PING_WIDGET

public: // Constructor/destructor ---------------------------------------------
	HitboxTargetSettingsPage(QWidget *parent = NULL);
	~HitboxTargetSettingsPage();

public: // Methods ------------------------------------------------------------
	Ui_HitboxTargetSettingsPage *	getUi();
	void							initialize();
	bool							isValid() const;

	void	sharedReset(WizardWindow *wizWin, WizTargetSettings *defaults);
	void	sharedNext(WizTargetSettings *settings);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	dashboardClicked(const QString &link);
	void	nameEditChanged(const QString &text);
	void	usernameEditChanged(const QString &text);
	void	serverSelectionChanged();
	void	streamKeyEditChanged(const QString &text);
};
//=============================================================================

inline Ui_HitboxTargetSettingsPage *HitboxTargetSettingsPage::getUi()
{
	return &m_ui;
}

#endif // HITBOXTARGETSETTINGSPAGE_H
