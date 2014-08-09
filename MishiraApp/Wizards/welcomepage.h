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

#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include "ui_welcomepage.h"

//=============================================================================
class WelcomePage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_WelcomePage	m_ui;

public: // Constructor/destructor ---------------------------------------------
	WelcomePage(QWidget *parent = NULL);
	~WelcomePage();

public: // Methods ------------------------------------------------------------
	Ui_WelcomePage *	getUi();
};
//=============================================================================

inline Ui_WelcomePage *WelcomePage::getUi()
{
	return &m_ui;
}

#endif // WELCOMEPAGE_H
