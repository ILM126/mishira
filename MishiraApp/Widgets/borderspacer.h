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

#ifndef BORDERSPACER_H
#define BORDERSPACER_H

#include "stylehelper.h"
#include <QtWidgets/QWidget>

//=============================================================================
class HBorderSpacer : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	int		m_width;
	QColor	m_color;

public: // Constructor/destructor ---------------------------------------------
	HBorderSpacer(
		QWidget *parent = 0, int width = 1,
		const QColor &color = StyleHelper::DarkBg1Color);
	~HBorderSpacer();

private: // Events ------------------------------------------------------------
	virtual void 	paintEvent(QPaintEvent *ev);
};
//=============================================================================

//=============================================================================
class VBorderSpacer : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	int		m_height;
	QColor	m_color;

public: // Constructor/destructor ---------------------------------------------
	VBorderSpacer(
		QWidget *parent = 0, int height = 1,
		const QColor &color = StyleHelper::DarkBg1Color);
	~VBorderSpacer();

private: // Events ------------------------------------------------------------
	virtual void 	paintEvent(QPaintEvent *ev);
};
//=============================================================================

#endif // BORDERSPACER_H
