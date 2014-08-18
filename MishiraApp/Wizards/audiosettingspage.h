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

#ifndef AUDIOSETTINGSPAGE_H
#define AUDIOSETTINGSPAGE_H

#include "ui_audiosettingspage.h"
#include <QtWidgets/QButtonGroup>

//=============================================================================
class AudioSettingsPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_AudioSettingsPage	m_ui;
	QButtonGroup			m_btnGroup;

public: // Constructor/destructor ---------------------------------------------
	AudioSettingsPage(QWidget *parent = NULL);
	~AudioSettingsPage();

public: // Methods ------------------------------------------------------------
	Ui_AudioSettingsPage *	getUi();
	bool					isValid() const;
	void					updateEnabled();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void 	shareRadioClicked(QAbstractButton *button);
};
//=============================================================================

inline Ui_AudioSettingsPage *AudioSettingsPage::getUi()
{
	return &m_ui;
}

#endif // AUDIOSETTINGSPAGE_H
