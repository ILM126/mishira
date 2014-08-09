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

#include "unitlineedit.h"
#include "application.h"
#include "stylehelper.h"
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>

UnitLineEdit::UnitLineEdit(QWidget *parent)
	: QLineEdit(parent)
	, m_units()
{
}

UnitLineEdit::UnitLineEdit(const QString &contents, QWidget *parent)
	: QLineEdit(contents, parent)
	, m_units()
{
}

UnitLineEdit::UnitLineEdit(
	const QString &contents, const QString &units, QWidget *parent)
	: QLineEdit(contents, parent)
	, m_units(units)
{
}

UnitLineEdit::~UnitLineEdit()
{
}

void UnitLineEdit::paintEvent(QPaintEvent *ev)
{
	QLineEdit::paintEvent(ev);
	if(m_units.isEmpty())
		return;
	// Continue only if the user has specified units

	QPainter p(this);
	QStyleOptionFrameV2 panel;
	initStyleOption(&panel);

	// Constants from QLineEditPrivate
	const int verticalMargin = 1;
	const int horizontalMargin = 2;

	// Get contents rectangle
	QRect r =
		style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
	QMargins margins = textMargins();
	r.setX(r.x() + margins.left());
	r.setY(r.y() + margins.top());
	r.setRight(r.right() - margins.right());
	r.setBottom(r.bottom() - margins.bottom());
	p.setClipRect(r);

	// Determine text rectangle from contents rectangle
	QFontMetrics fm = fontMetrics();
	Qt::Alignment va =
		QStyle::visualAlignment(layoutDirection(), QFlag(alignment()));
	int vscroll;
	switch(va & Qt::AlignVertical_Mask) {
	case Qt::AlignBottom:
		vscroll = r.y() + r.height() - fm.height() - verticalMargin;
		break;
	case Qt::AlignTop:
		vscroll = r.y() + verticalMargin;
		break;
	default: // Center
		vscroll = r.y() + (r.height() - fm.height() + 1) / 2;
		break;
	}
	QRect lineRect(
		r.x() + horizontalMargin, vscroll,
		r.width() - 2 * horizontalMargin, fm.height());

	// Force right alignment of unit text
	va &= ~(Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignJustify);
	va |= Qt::AlignRight;

	// Draw a background behind the unit text extended to the sides of the
	// contents rectangle
	QRect boundingRect = fm.boundingRect(lineRect, va, m_units);
	r.setLeft(boundingRect.left() - 3); // Slight padding
	QColor col = palette().color(QPalette::Base);
	if(App->isDarkStyle(this)) {
		// The dark UI widget style darkens focused line edits. Use the same
		// formula as what is used in `DarkStyle::drawPrimitive()`
		if(hasFocus())
			col = col.darker(125);
	}
	col.setAlpha(218); // 85%
	p.fillRect(r, col);

	// Draw the inner glow
	QVector<QPoint> points;
	if(App->isDarkStyle(this)) {
		// Not needed for the dark UI widget style
#if 0
		// If we are using the dark UI widget style then the glow is actually a
		// shadow and it doesn't touch the corners as they are rounded.

		// HACK: Make sure our glow is in the right spot. This should really be
		// done using metrics and better rectangle calculations
		r.adjust(0, -1, 1, 1);

		points << r.topLeft();
		points << QPoint(r.right() - 2, r.top());
		points << QPoint(r.right(), r.top() + 2);
		points << QPoint(r.right(), r.bottom() - 2);
		points << QPoint(r.right() - 2, r.bottom());
		points << r.bottomLeft();

		// Same colour as `StyleHelper::drawDarkPanel()`
		p.setPen(pal.color(QPalette::Base).darker(112));
#endif // 0
	} else {
		points << r.topLeft();
		points << r.topRight();
		points << r.topRight();
		points << r.bottomRight();
		points << r.bottomRight();
		points << r.bottomLeft();

		// Same colour as `qDrawShadePanel()`
		p.setPen(palette().light().color());
	}
	p.drawLines(points);

	// Draw unit text at half opacity
	col = palette().text().color();
	col.setAlpha(col.alpha() / 2);
	p.setPen(col);
	p.drawText(lineRect, va, m_units);
}
