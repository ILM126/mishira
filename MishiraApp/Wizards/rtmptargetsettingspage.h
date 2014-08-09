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

#ifndef RTMPTARGETSETTINGSPAGE_H
#define RTMPTARGETSETTINGSPAGE_H

#include "ui_rtmptargetsettingspage.h"

class WizardWindow;
struct WizTargetSettings;

//=============================================================================
class RTMPTargetSettingsPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_RTMPTargetSettingsPage	m_ui;

public: // Constructor/destructor ---------------------------------------------
	RTMPTargetSettingsPage(QWidget *parent = NULL);
	~RTMPTargetSettingsPage();

public: // Methods ------------------------------------------------------------
	Ui_RTMPTargetSettingsPage *	getUi();
	bool						isValid() const;

	void	sharedReset(WizardWindow *wizWin, WizTargetSettings *defaults);
	void	sharedNext(WizTargetSettings *settings);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	nameEditChanged(const QString &text);
	void	urlEditChanged(const QString &text);
	void	hideStreamNameCheckChanged(int state);
};
//=============================================================================

inline Ui_RTMPTargetSettingsPage *RTMPTargetSettingsPage::getUi()
{
	return &m_ui;
}

#endif // RTMPTARGETSETTINGSPAGE_H
