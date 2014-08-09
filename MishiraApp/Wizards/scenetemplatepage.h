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

#ifndef SCENETEMPLATEPAGE_H
#define SCENETEMPLATEPAGE_H

#include "ui_scenetemplatepage.h"

//=============================================================================
class SceneTemplatePage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_SceneTemplatePage	m_ui;

public: // Constructor/destructor ---------------------------------------------
	SceneTemplatePage(QWidget *parent = NULL);
	~SceneTemplatePage();

public: // Methods ------------------------------------------------------------
	Ui_SceneTemplatePage *	getUi();
};
//=============================================================================

inline Ui_SceneTemplatePage *SceneTemplatePage::getUi()
{
	return &m_ui;
}

#endif // SCENETEMPLATEPAGE_H
