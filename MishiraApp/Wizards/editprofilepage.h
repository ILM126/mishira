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

#ifndef EDITPROFILEPAGE_H
#define EDITPROFILEPAGE_H

#include "ui_editprofilepage.h"

//=============================================================================
class EditProfilePage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_EditProfilePage	m_ui;

public: // Constructor/destructor ---------------------------------------------
	EditProfilePage(QWidget *parent = NULL);
	~EditProfilePage();

public: // Methods ------------------------------------------------------------
	Ui_EditProfilePage *	getUi();
	bool					isValid() const;

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	nameEditChanged(const QString &text);
	void	widthEditChanged(const QString &text);
	void	heightEditChanged(const QString &text);
};
//=============================================================================

inline Ui_EditProfilePage *EditProfilePage::getUi()
{
	return &m_ui;
}

#endif // EDITPROFILEPAGE_H
