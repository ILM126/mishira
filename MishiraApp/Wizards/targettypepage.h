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

#ifndef TARGETTYPEPAGE_H
#define TARGETTYPEPAGE_H

#include "common.h"
#include "ui_targettypepage.h"
#include <QtWidgets/QButtonGroup>

class TargetTypeWidget;
class QGridLayout;
class WizardWindow;
struct WizTargetSettings;

//=============================================================================
class TargetTypePage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_TargetTypePage			m_ui;
	QVector<TargetTypeWidget *>	m_typeWidgets;
	QButtonGroup				m_typeBtnGroup;

public: // Constructor/destructor ---------------------------------------------
	TargetTypePage(QWidget *parent = NULL);
	~TargetTypePage();

public: // Methods ------------------------------------------------------------
	Ui_TargetTypePage *	getUi();

	void	cleanUpTypePage();
	void	addTargetType(
		QGridLayout *layout, TrgtType type, const QString &tooltip,
		const QPixmap &icon, int row, int column);

	void	sharedReset(
		WizardWindow *wizWin, WizTargetSettings *defaults, bool isNewProfile);
	void	sharedNext(WizTargetSettings *settings);
};
//=============================================================================

inline Ui_TargetTypePage *TargetTypePage::getUi()
{
	return &m_ui;
}

#endif // TARGETTYPEPAGE_H
