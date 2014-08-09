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

#include "patternwidgets.h"
#include "application.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

//=============================================================================
// HeaderPattern class

HeaderPattern::HeaderPattern(QWidget *parent)
	: QWidget(parent)
{
}

HeaderPattern::~HeaderPattern()
{
}

void HeaderPattern::paintEvent(QPaintEvent *ev)
{
	// Vertically center the pattern while making sure that we never render
	// half a line at the top or bottom
	QRect r = rect();
	r.setHeight(((height() - 1 + 7) / 8 + (height() + 7) / 8 - 1) * 4);
	r.translate(0, ((height() - 2) % 8) / 2);

	// Render the lines
	QPainter p(this);
	for(int y = r.top(); y < r.bottom(); y += 4) {
		p.setPen(QColor(0, 0, 0, 64)); // 25%
		p.drawLine(0, y, width(), y);
		p.setPen(QColor(255, 255, 255, 18)); // 7%
		p.drawLine(0, y + 1, width(), y + 1);
	}
}

//=============================================================================
// GripPattern class

GripPattern::GripPattern(bool resetOnEmit, QWidget *parent)
	: QWidget(parent)
	, m_resetOnEmit(resetOnEmit)
	, m_mouseDown(false)
	, m_emittedDrag(false)
	, m_mouseDownPos(0, 0)
{
	setCursor(Qt::OpenHandCursor);
}

GripPattern::~GripPattern()
{
}

void GripPattern::mouseMoveEvent(QMouseEvent *ev)
{
	ev->ignore();
	if(!m_mouseDown || m_emittedDrag) {
		QWidget::mouseMoveEvent(ev);
		return;
	}

	// Only trigger once we exceed the OS's drag distance
	if((ev->pos() - m_mouseDownPos).manhattanLength() <
		App->startDragDistance())
	{
		QWidget::mouseMoveEvent(ev);
		return;
	}

	emit dragBegun(m_mouseDownPos);
	m_emittedDrag = true;

	// If the drag event trigged a `QDrag::exec()` then we will never receive
	// our mouse button release event. Process the release here instead of the
	// creater requested
	if(m_resetOnEmit) {
		m_mouseDown = false;
		m_emittedDrag = false;
		setCursor(Qt::OpenHandCursor);
	}
}

void GripPattern::mousePressEvent(QMouseEvent *ev)
{
	ev->ignore();
	if(ev->button() == Qt::LeftButton) {
		m_mouseDown = true;
		m_emittedDrag = false;
		m_mouseDownPos = ev->pos();
		setCursor(Qt::ClosedHandCursor);
		// We emit the event only when the cursor moves while pressed
	}
}

void GripPattern::mouseReleaseEvent(QMouseEvent *ev)
{
	ev->ignore();
	if(ev->button() == Qt::LeftButton) {
		m_mouseDown = false;
		m_emittedDrag = false;
		setCursor(Qt::OpenHandCursor);
	}
}

void GripPattern::paintEvent(QPaintEvent *ev)
{
	// Vertically and horizontally center the pattern while making sure that we
	// never render half a dot at any edge
	QRect r = rect();
	r.setWidth(((width() - 1 + 7) / 8 + (width() + 7) / 8 - 1) * 4);
	r.setHeight(((height() - 1 + 7) / 8 + (height() + 7) / 8 - 1) * 4);
	r.translate(((width() - 2) % 8) / 2, ((height() - 2) % 8) / 2);

	// Render the dots
	QPainter p(this);
	for(int y = r.top(); y < r.bottom(); y += 4) {
		for(int x = r.left(); x < r.right(); x += 4) {
			p.setPen(QColor(0, 0, 0, 255)); // 100%
			p.drawPoint(x, y);
			p.setPen(QColor(0, 0, 0, 85)); // 33%
			p.drawPoint(x + 1, y);
			p.drawPoint(x, y + 1);
			p.setPen(QColor(0, 0, 0, 31)); // 12%
			p.drawPoint(x + 1, y + 1);
		}
	}
}
