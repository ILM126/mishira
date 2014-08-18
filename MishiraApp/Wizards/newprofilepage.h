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

#ifndef NEWPROFILEPAGE_H
#define NEWPROFILEPAGE_H

#include "ui_newprofilepage.h"

//=============================================================================
class NewProfilePage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_NewProfilePage	m_ui;

public: // Constructor/destructor ---------------------------------------------
	NewProfilePage(QWidget *parent = NULL);
	~NewProfilePage();

public: // Methods ------------------------------------------------------------
	Ui_NewProfilePage *	getUi();
	bool				isValid() const;

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	nameEditChanged(const QString &text);
};
//=============================================================================

inline Ui_NewProfilePage *NewProfilePage::getUi()
{
	return &m_ui;
}

#endif // NEWPROFILEPAGE_H
