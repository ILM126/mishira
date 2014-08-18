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

#ifndef VIDEOSETTINGSPAGE_H
#define VIDEOSETTINGSPAGE_H

#include "ui_videosettingspage.h"
#include <QtWidgets/QButtonGroup>

//=============================================================================
class VideoSettingsPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_VideoSettingsPage	m_ui;
	QButtonGroup			m_btnGroup;

public: // Constructor/destructor ---------------------------------------------
	VideoSettingsPage(QWidget *parent = NULL);
	~VideoSettingsPage();

public: // Methods ------------------------------------------------------------
	Ui_VideoSettingsPage *	getUi();
	bool					isValid() const;
	void					updateEnabled();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	widthEditChanged(const QString &text);
	void	heightEditChanged(const QString &text);
	void	bitrateEditChanged(const QString &text);
	void	keyIntervalEditChanged(const QString &text);
	void	keyIntervalEditFinished();
	void 	shareRadioClicked(QAbstractButton *button);
};
//=============================================================================

inline Ui_VideoSettingsPage *VideoSettingsPage::getUi()
{
	return &m_ui;
}

#endif // VIDEOSETTINGSPAGE_H
