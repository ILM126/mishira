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

#include "borderspacer.h"
#include <QtGui/QPainter>

//=============================================================================
// HBorderSpacer class

HBorderSpacer::HBorderSpacer(QWidget *parent, int width, const QColor &color)
	: QWidget(parent)
	, m_width(width)
	, m_color(color)
{
	setMinimumSize(width, 0);
	setMaximumWidth(width);
}

HBorderSpacer::~HBorderSpacer()
{
}

void HBorderSpacer::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);
	p.fillRect(0, 0, width(), height(), m_color);
}

//=============================================================================
// VBorderSpacer class

VBorderSpacer::VBorderSpacer(QWidget *parent, int height, const QColor &color)
	: QWidget(parent)
	, m_height(height)
	, m_color(color)
{
	setMinimumSize(0, height);
	setMaximumHeight(height);
}

VBorderSpacer::~VBorderSpacer()
{
}

void VBorderSpacer::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);
	p.fillRect(0, 0, width(), height(), m_color);
}
