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

#include "colorbutton.h"
#include <QtGui/QPainter>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

ColorButton::ColorButton(QWidget *parent)
	: QPushButton(parent)
	, m_color(0, 0, 0)
	, m_dialog(this)
{
	connect(this, &QAbstractButton::clicked,
		&m_dialog, &QWidget::show);
	connect(&m_dialog, &QColorDialog::colorSelected,
		this, &ColorButton::setColor);
	m_dialog.setCurrentColor(m_color);
	m_dialog.setOptions(QColorDialog::ShowAlphaChannel);
}

ColorButton::ColorButton(const QColor &color, QWidget *parent)
	: QPushButton(parent)
	, m_color(color)
	, m_dialog(this)
{
	connect(this, &QAbstractButton::clicked,
		&m_dialog, &QWidget::show);
	connect(&m_dialog, &QColorDialog::colorSelected,
		this, &ColorButton::setColor);
	m_dialog.setCurrentColor(m_color);
	m_dialog.setOptions(QColorDialog::ShowAlphaChannel);
}

ColorButton::~ColorButton()
{
}

void ColorButton::setColor(const QColor &color)
{
	m_color = color;
	m_dialog.setCurrentColor(color);
	repaint();
	emit colorChanged(color);
}

void ColorButton::drawCheckerboard(
	QPainter &p, const QRect &rect, int size, const QColor &aCol,
	const QColor &bCol)
{
	// Fill the background with the secondary colour
	p.fillRect(rect, bCol);

	// Draw the primary colour rectangles
	int line = 0;
	for(int y = rect.y(); y <= rect.bottom(); y += size) {
		bool useB = (line++ % 2 == 1);
		for(int x = rect.x(); x <= rect.right(); x += size) {
			if(!useB) {
				const QRect r(x, y,
					qMin(size, rect.right() - x + 1),
					qMin(size, rect.bottom() - y + 1));
				p.fillRect(r, aCol);
			}
			useB = !useB;
		}
	}
}

void ColorButton::paintEvent(QPaintEvent *ev)
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
	if(m_color.alpha() == 255)
		p.fillRect(cRect, m_color);
	else {
		// Separate left and right rectangles
		QRect lRect = cRect;
		lRect.setWidth(lRect.width() / 2 + 1);
		QRect rRect = cRect;
		rRect.setLeft(lRect.right());

		// Fill the left with the solid colour
		p.fillRect(
			lRect, QColor(m_color.red(), m_color.green(), m_color.blue()));

		// Fill the right with the transparent colour on a checkerboard
		drawCheckerboard(p, rRect, 5, Qt::white, QColor(0xCC, 0xCC, 0xCC));
		p.fillRect(rRect, m_color);
	}

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

	// Fade out when disabled
	if(!isEnabled()) {
		QBrush brush = palette().base();
		QColor col = brush.color();
		col.setAlpha(127); // 50% transparency
		brush.setColor(col);
		p.fillRect(cRect, brush);
	}
}
