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

#ifndef TARGETTYPEWIDGET_H
#define TARGETTYPEWIDGET_H

#include "styledradiotile.h"
#include <QtWidgets/QLabel>
#include <QtWidgets/QStackedLayout>

//=============================================================================
class TargetTypeWidget : public StyledRadioTile
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	TrgtType	m_type;
	QWidget		m_iconWidget;
	QVBoxLayout	m_iconLayout;
	QLabel		m_iconLabel;

public: // Constructor/destructor ---------------------------------------------
	TargetTypeWidget(TrgtType type, const QPixmap &icon, QWidget *parent = 0);
	~TargetTypeWidget();

public: // Methods ------------------------------------------------------------
	TrgtType	getType() const;
};
//=============================================================================

inline TrgtType TargetTypeWidget::getType() const
{
	return m_type;
}

#endif // TARGETTYPEWIDGET_H
