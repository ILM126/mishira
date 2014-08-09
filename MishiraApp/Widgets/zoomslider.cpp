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

#include "zoomslider.h"
#include "stylehelper.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QToolTip>

ZoomSlider::ZoomSlider(QWidget *parent)
	: QAbstractSlider(parent)
	, m_tickWidth(25)
	, m_minimumTick(-2)
	, m_maximumTick(3)
	, m_handleWidth(9)
	, m_handlePos(0) // Position in pixels from the left of the groove
	, m_handlePressed(false)
	, m_handleClickedPos(0)
	, m_gutterWidth(26)
	, m_snapDistance(5)
	, m_handlePixmap(":/Resources/zoom-handle.png")
{
	// Place the handle at tick 0 (100% zoom) by default
	if(m_minimumTick < 0)
		m_handlePos = m_tickWidth * -m_minimumTick;
}

ZoomSlider::~ZoomSlider()
{
}

QSize ZoomSlider::sizeHint() const
{
	return minimumSizeHint();
}

QSize ZoomSlider::minimumSizeHint() const
{
	return QSize(
		m_gutterWidth * 2 + (m_maximumTick - m_minimumTick) * m_tickWidth + 1,
		17);
}

void ZoomSlider::setZoom(float zoom, bool byUser)
{
	int newPos = qBound(0, zoomToPos(zoom),
		(m_maximumTick - m_minimumTick) * m_tickWidth);
	if(m_handlePos == newPos)
		return; // No change
	m_handlePos = newPos;
	repaint();
	emit zoomChanged(zoom, byUser);
}

void ZoomSlider::setZoom(float zoom)
{
	setZoom(zoom, false);
}

float ZoomSlider::getZoom() const
{
	return posToZoom(m_handlePos);
}

int ZoomSlider::zoomToPos(float zoom) const
{
	float log2 = (logf(zoom) / logf(2)); // log_2(zoom) via change of base
	return qRound(log2 * m_tickWidth - m_minimumTick * m_tickWidth);
}

float ZoomSlider::posToZoom(int pos) const
{
	return powf(2.0f,
		(float)(pos + m_minimumTick * m_tickWidth) / (float)m_tickWidth);
}

void ZoomSlider::updateToolTip(const QPoint &globalPos)
{
	float percent = posToZoom(m_handlePos) * 100.0f;
	QString msg = QStringLiteral("%L1%").arg(percent, 0, 'f', 0);
	QToolTip::showText(globalPos, msg, this);
}

void ZoomSlider::drawMarksAndIcons(QPainter &p, int dx, int dy)
{
	// Draw relative to the center of the left gutter icon
	dx += m_gutterWidth / 2;
	dy += height() / 2;

	// Draw left icon
	dx--; // 1px closer to the edge
	p.drawPoint(dx, dy);
	p.drawLine(dx + 5, dy - 3, dx + 5, dy + 3);
	p.drawLine(dx + 5, dy + 4, dx - 5, dy + 4);
	p.drawLine(dx - 5, dy + 3, dx - 5, dy - 3);
	p.drawLine(dx - 5, dy - 4, dx + 5, dy - 4);
	dx++;

	// Draw relative to groove start
	dx += m_gutterWidth - m_gutterWidth / 2;

	// Draw ticks
	for(int i = m_minimumTick; i <= m_maximumTick; i++) {
		int x = (i - m_minimumTick) * m_tickWidth;
		if(i == 0) {
			// Special arrow tick for 100% zoom
			p.drawPoint(dx + x - 2, dy - 6);
			p.drawLine(dx + x - 1, dy - 6, dx + x - 1, dy - 5);
			p.drawLine(dx + x, dy - 6, dx + x, dy - 4);
			p.drawLine(dx + x + 1, dy - 6, dx + x + 1, dy - 5);
			p.drawPoint(dx + x + 2, dy - 6);
		} else {
			// Plain line tick
			p.drawLine(dx + x, dy - 6, dx + x, dy - 4);
		}
	}

	// Draw relative to the center of the right gutter icon
	int numTicks = m_maximumTick - m_minimumTick;
	dx += numTicks * m_tickWidth + m_gutterWidth / 2;

	// Draw right icon
	dx++; // 1px closer to the edge
	p.fillRect(dx - 3, dy - 2, 7, 5, p.pen().color());
	p.drawLine(dx + 5, dy - 3, dx + 5, dy + 3);
	p.drawLine(dx + 5, dy + 4, dx - 5, dy + 4);
	p.drawLine(dx - 5, dy + 3, dx - 5, dy - 3);
	p.drawLine(dx - 5, dy - 4, dx + 5, dy - 4);
	dx--;
}

void ZoomSlider::mousePressEvent(QMouseEvent *ev)
{
	// Determine the position of the handle extended to the top and bottom of
	// the widget to make it easier to click
	QRect handleRect(m_gutterWidth + m_handlePos - m_handleWidth / 2, 0,
		m_handleWidth, height());

	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}
	ev->accept();

	// Determine where the user clicked
	if(handleRect.contains(ev->pos())) {
		// User clicked on the handle
		m_handlePressed = true;
		m_handleClickedPos = ev->pos().x() - handleRect.x();
		updateToolTip(ev->globalPos());
	} else if(ev->pos().x() < handleRect.left()) {
		// User clicked to the left of the handle, move to the nearest tick
		// below the current position
		if(m_handlePos % m_tickWidth > 0)
			m_handlePos = (m_handlePos / m_tickWidth) * m_tickWidth;
		else
			m_handlePos = (m_handlePos / m_tickWidth - 1) * m_tickWidth;
		m_handlePos = qBound(
			0, m_handlePos, (m_maximumTick - m_minimumTick) * m_tickWidth);
		repaint();
		emit zoomChanged(posToZoom(m_handlePos), true);
	} else {
		// User clicked to the right of the handle, move to the nearest tick
		// above the current position
		m_handlePos = (m_handlePos / m_tickWidth + 1) * m_tickWidth;
		m_handlePos = qBound(
			0, m_handlePos, (m_maximumTick - m_minimumTick) * m_tickWidth);
		repaint();
		emit zoomChanged(posToZoom(m_handlePos), true);
	}
}

void ZoomSlider::mouseReleaseEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}
	ev->accept();

	// The only thing that can happen is the user released the handle
	m_handlePressed = false;
}

void ZoomSlider::mouseMoveEvent(QMouseEvent *ev)
{
	// We only care about move events when the user is dragging the handle
	if(!m_handlePressed) {
		ev->ignore();
		return;
	}
	ev->accept();

	// Calculate where on the groove the mouse currently is located taking into
	// account where on the handle the user is dragging from
	int mouseX = ev->pos().x() - m_gutterWidth + m_handleWidth / 2
		- m_handleClickedPos;
	mouseX = qBound(0, mouseX, (m_maximumTick - m_minimumTick) * m_tickWidth);

	// Apply snapping
	int relToTick = (mouseX + m_tickWidth / 2) % m_tickWidth
		- m_tickWidth / 2;
	if(relToTick > -m_snapDistance && relToTick < m_snapDistance)
		mouseX -= relToTick;

	// Actually move the handle and issue a repaint
	if(m_handlePos != mouseX) {
		m_handlePos = mouseX;
		repaint();
		emit zoomChanged(posToZoom(m_handlePos), true);
		updateToolTip(ev->globalPos());
	}
}

void ZoomSlider::wheelEvent(QWheelEvent *ev)
{
	float zoomAmount = 1.125f; // Same as in GraphicsWidget
	zoomAmount = (ev->delta() >= 0) ? zoomAmount : 1.0f / zoomAmount;
	setZoom(getZoom() * zoomAmount, true); // TODO: Unbounded zoom
}

void ZoomSlider::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);

	// Draw groove
	QPalette pal = palette();
	pal.setColor(QPalette::Base, StyleHelper::DarkBg3Color);
	StyleHelper::drawDarkPanel(&p, pal, QRect(
		m_gutterWidth - 1, height() / 2,
		width() - m_gutterWidth * 2 + 2, 6), false, 1.5f);

	// Draw marks and icons
	p.setPen(QColor(255, 255, 255, 20)); // 8% opacity
	drawMarksAndIcons(p, 1, 1);
	p.setPen(StyleHelper::DarkBg1Color);
	drawMarksAndIcons(p, 0, 0);

	// Draw handle
	p.drawPixmap(
		m_gutterWidth + m_handlePos - m_handlePixmap.width() / 2,
		height() / 2 - 2, m_handlePixmap);
}
