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

#ifndef USTREAMTARGETSETTINGSPAGE_H
#define USTREAMTARGETSETTINGSPAGE_H

#include "ui_ustreamtargetsettingspage.h"

class WizardWindow;
struct WizTargetSettings;

//=============================================================================
class UstreamTargetSettingsPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_UstreamTargetSettingsPage	m_ui;

public: // Constructor/destructor ---------------------------------------------
	UstreamTargetSettingsPage(QWidget *parent = NULL);
	~UstreamTargetSettingsPage();

public: // Methods ------------------------------------------------------------
	Ui_UstreamTargetSettingsPage *	getUi();
	bool							isValid() const;

	void	sharedReset(WizardWindow *wizWin, WizTargetSettings *defaults);
	void	sharedNext(WizTargetSettings *settings);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	nameEditChanged(const QString &text);
	void	urlEditChanged(const QString &text);
	void	streamKeyEditChanged(const QString &text);
};
//=============================================================================

inline Ui_UstreamTargetSettingsPage *UstreamTargetSettingsPage::getUi()
{
	return &m_ui;
}

#endif // USTREAMTARGETSETTINGSPAGE_H
