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

#ifndef TARGETLISTPAGE_H
#define TARGETLISTPAGE_H

#include "ui_targetlistpage.h"

//=============================================================================
class TargetListPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_TargetListPage	m_ui;

public: // Constructor/destructor ---------------------------------------------
	TargetListPage(QWidget *parent = NULL);
	~TargetListPage();

public: // Methods ------------------------------------------------------------
	Ui_TargetListPage *	getUi();
};
//=============================================================================

inline Ui_TargetListPage *TargetListPage::getUi()
{
	return &m_ui;
}

#endif // TARGETLISTPAGE_H
