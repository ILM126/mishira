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

#include "colortogglebutton.h"
#include <QtGui/QPainter>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

ColorToggleButton::ColorToggleButton(QWidget *parent)
	: QPushButton(parent)
	, m_colorA(255, 255, 255)
	, m_colorB(0, 0, 0)
	, m_curIsA(true)
{
	connect(this, &QAbstractButton::clicked,
		this, &ColorToggleButton::toggle);
}

ColorToggleButton::ColorToggleButton(
	const QColor &aCol, const QColor &bCol, QWidget *parent)
	: QPushButton(parent)
	, m_colorA(aCol)
	, m_colorB(bCol)
	, m_curIsA(true)
{
	connect(this, &QAbstractButton::clicked,
		this, &ColorToggleButton::toggle);
}

ColorToggleButton::~ColorToggleButton()
{
}

void ColorToggleButton::setColorA(const QColor &color)
{
	if(m_colorA == color)
		return; // No change
	m_colorA = color;
	if(m_curIsA) {
		repaint();
		emit currentColorChanged(getCurrentColor());
	}
}

void ColorToggleButton::setColorB(const QColor &color)
{
	if(m_colorB == color)
		return; // No change
	m_colorB = color;
	if(!m_curIsA) {
		repaint();
		emit currentColorChanged(getCurrentColor());
	}
}

void ColorToggleButton::setColors(const QColor &aCol, const QColor &bCol)
{
	setColorA(aCol);
	setColorB(bCol);
}

void ColorToggleButton::setIsA(bool isA)
{
	if(m_curIsA == isA)
		return; // No change
	m_curIsA = isA;
	repaint();
	emit currentColorChanged(getCurrentColor());
}

void ColorToggleButton::paintEvent(QPaintEvent *ev)
{
	QPushButton::paintEvent(ev);
	QPainter p(this);

	// Get contents rectangle
	QStyleOptionButton opt;
	initStyleOption(&opt);
	QRect cRect;
	if(isFlat()) {
		// Fill the entire widget area
		cRect = rect();
	} else {
		// Only fill the contents area
		cRect =
			style()->subElementRect(QStyle::SE_PushButtonContents, &opt, this);
	}
	cRect.adjust(1, 1, -1, -1); // A little larger padding please

	// Colour fill
	p.fillRect(cRect, getCurrentColor());

	// Sunken frame
	QStyleOptionFrame fOpt;
	fOpt.initFrom(this);
	fOpt.rect = cRect;
	fOpt.features = QStyleOptionFrame::None;
	fOpt.frameShape = QFrame::Panel;
	fOpt.lineWidth = 1;
	fOpt.midLineWidth = 0;
	fOpt.state |= QStyle::State_Sunken;
	style()->drawControl(QStyle::CE_ShapedFrame, &fOpt, &p, this);
}

void ColorToggleButton::toggle()
{
	setIsA(!m_curIsA);
}
