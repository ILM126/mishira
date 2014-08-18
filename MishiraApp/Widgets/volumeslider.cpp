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

#include "volumeslider.h"
#include "common.h"
#include "constants.h"
#include "stylehelper.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

// The amount of recommended audio headroom, TODO: User customisable
const float	DEFAULT_HEADROOM_MB_F = -900.0f; // -9 dB

// The linearity of the bar graph as used by `mbToPos()` and `posToMb()`. The
// higher value the more linear the graph is.
const float BAR_LINEARITY = 1.5f;

VolumeSlider::VolumeSlider(QWidget *parent)
	: QAbstractSlider(parent)
	, m_handleWidth(5)
	, m_handlePos(0) // Position in pixels from the left of the groove
	, m_handlePressed(false)
	, m_gutterWidth(12)
	, m_isMuted(false)
	, m_attenuation(0)
	, m_headroomMb(DEFAULT_HEADROOM_MB_F)
	, m_rmsVolume(MIN_MB_F)
	, m_peakVolume(MIN_MB_F)
{
}

VolumeSlider::~VolumeSlider()
{
}

QSize VolumeSlider::sizeHint() const
{
	return minimumSizeHint();
}

QSize VolumeSlider::minimumSizeHint() const
{
	return QSize(m_gutterWidth * 2 + 5, 38);
}

void VolumeSlider::setVolume(float rmsLinear, float peakLinear)
{
	float rmsVolume = linearToMb(rmsLinear);
	float peakVolume = linearToMb(peakLinear);
	if(m_rmsVolume != rmsVolume || m_peakVolume != peakVolume) {
		m_rmsVolume = rmsVolume;
		m_peakVolume = peakVolume;

		// Repaint the bar only to improve speed
		repaint(calcBarRect());
	}
}

void VolumeSlider::setIsMuted(bool isMuted)
{
	if(m_isMuted == isMuted)
		return; // No change
	m_isMuted = isMuted;
	repaint(calcBarRect());
}

void VolumeSlider::setAttenuation(int attenuationMb)
{
	if(m_attenuation == attenuationMb)
		return; // No change
	m_attenuation = attenuationMb;
	int newPos = mbToPos(attenuationMb);
	if(m_handlePos == newPos)
		return; // No change
	m_handlePos = newPos;
	repaint();
	//emit attenuationChanged(posToMb(m_handlePos)); // Beware feedback loops!
}

int VolumeSlider::mbToPos(float mb) const
{
	// While the user is only really interested in the upper 50 dB or so of the
	// volume bar we still want to display the full range so the user can
	// figure out if an audio source is working or not. We do this by making
	// the bar logarithmic even if dB is already a logarithmic scale.
	//
	// The worksheet for calculating this formula can be found at
	// "P:\Development\Citrine\Volume bar linearity calculations.xlsx"

	mb = qBound(MIN_MB_F, mb, 0.0f);
	const int barWidth = width() - m_gutterWidth * 2 - 1;
	return (barWidth + BAR_LINEARITY) *
		pow(BAR_LINEARITY / (barWidth + BAR_LINEARITY), mb / MIN_MB_F) -
		BAR_LINEARITY;
}

/// <summary>
/// The opposite of mbToPos().
/// </summary>
float VolumeSlider::posToMb(int pos) const
{
	const int barWidth = width() - m_gutterWidth * 2 - 1;
	pos = qBound(0, pos, barWidth);
	// We use the logarithm base change rule rule:
	//     log_b(s) = log_c(x) / log_c(b)
	return logf((pos + BAR_LINEARITY) / (barWidth + BAR_LINEARITY)) /
		logf(BAR_LINEARITY / (barWidth + BAR_LINEARITY)) * MIN_MB_F;
}

QRect VolumeSlider::calcBarRect() const
{
	return QRect(
		m_gutterWidth, height() / 2 - 3, width() - m_gutterWidth * 2, 6);
}

void VolumeSlider::drawMarks(QPainter &p, int dx, int dy)
{
	// Draw relative to the start of the bar
	dx += m_gutterWidth;
	dy += height() / 2;

	// Draw a tick every 12 dB except the two left-most ones and the right-most
	// one
	for(int mb = -1200; mb > MIN_MB + 1200; mb -= 1200) {
		int x = mbToPos((float)mb);
		p.drawLine(dx + x, dy - 10, dx + x, dy - 8);
		p.drawLine(dx + x, dy + 9, dx + x, dy + 7);
	}
}

void VolumeSlider::drawHandle(QPainter &p, int dx, int dy)
{
	// Draw relative to the handle center
	dx += m_gutterWidth + m_handlePos;
	dy += height() / 2;

	// Draw top handle
	p.drawLine(dx - 2, dy - 14, dx - 2, dy - 12);
	p.drawLine(dx - 1, dy - 14, dx - 1, dy - 11);
	p.drawLine(dx + 0, dy - 14, dx + 0, dy - 10);
	p.drawLine(dx + 1, dy - 14, dx + 1, dy - 11);
	p.drawLine(dx + 2, dy - 14, dx + 2, dy - 12);

	// Draw bottom handle
	p.drawLine(dx - 2, dy + 13, dx - 2, dy + 11);
	p.drawLine(dx - 1, dy + 13, dx - 1, dy + 10);
	p.drawLine(dx + 0, dy + 13, dx + 0, dy + 9);
	p.drawLine(dx + 1, dy + 13, dx + 1, dy + 10);
	p.drawLine(dx + 2, dy + 13, dx + 2, dy + 11);
}

void VolumeSlider::mousePressEvent(QMouseEvent *ev)
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

	// Calculate where on the groove the mouse currently is located
	int mouseX = ev->pos().x() - m_gutterWidth;
	mouseX = qBound(0, mouseX, width() - m_gutterWidth * 2 - 1);

	// Immediately jump to the position and enter drag mode
	m_handlePressed = true;
	if(m_handlePos != mouseX) {
		m_handlePos = mouseX;
		m_attenuation = posToMb(m_handlePos);
		emit attenuationChanged(m_attenuation);
		repaint();
	}
}

void VolumeSlider::mouseReleaseEvent(QMouseEvent *ev)
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

void VolumeSlider::mouseMoveEvent(QMouseEvent *ev)
{
	// We only care about move events when the user is dragging the handle
	if(!m_handlePressed) {
		ev->ignore();
		return;
	}
	ev->accept();

	// Calculate where on the groove the mouse currently is located
	int mouseX = ev->pos().x() - m_gutterWidth;
	mouseX = qBound(0, mouseX, width() - m_gutterWidth * 2 - 1);

	// Actually move the handle and issue a repaint
	if(m_handlePos != mouseX) {
		m_handlePos = mouseX;
		m_attenuation = posToMb(m_handlePos);
		emit attenuationChanged(m_attenuation);
		repaint();
	}
}

void VolumeSlider::wheelEvent(QWheelEvent *ev)
{
	int newPos = m_handlePos + ((ev->delta() >= 0) ? 10 : -10);
	newPos = qBound(0, newPos, width() - m_gutterWidth * 2 - 1);
	if(m_handlePos == newPos)
		return; // No change
	m_handlePos = newPos;
	m_attenuation = posToMb(m_handlePos);
	emit attenuationChanged(m_attenuation);
	repaint();
}

void VolumeSlider::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);
	QColor grooveCol = StyleHelper::DarkBg3Color;

	// As repainting the entire widget every time the volume gets updated is
	// expensive always attempt to do an optimized repaint first
	QRect barRect = calcBarRect();
	if(barRect.contains(ev->rect(), false)) {
		// Draw the groove background only
		p.fillRect(barRect, grooveCol);
	} else {
		// Draw groove
		QPalette pal = palette();
		pal.setColor(QPalette::Base, grooveCol);
		StyleHelper::drawDarkPanel(
			&p, pal, barRect.adjusted(-2, -2, 2, 2), false, 0.0f);

		// Draw headroom indicator
		QRect headRect = barRect.adjusted(0, 0, 1, 0);
		headRect.setLeft(barRect.left() + mbToPos(m_headroomMb));
		headRect.setHeight(3);
		p.fillRect(headRect.translated(0, -5), StyleHelper::RedBaseColor);
		p.fillRect(headRect.translated(0, barRect.height() + 2),
			StyleHelper::RedBaseColor);

		// Draw marks
		p.setPen(QColor(255, 255, 255, 20)); // 8% opacity
		drawMarks(p, 1, 1);
		p.setPen(StyleHelper::DarkBg1Color);
		drawMarks(p, 0, 0);

		// Draw handle
		p.setPen(StyleHelper::TextShadowColor);
		drawHandle(p, 1, 1);
		p.setPen(StyleHelper::DarkFg1Color);
		drawHandle(p, 0, 0);
	}

	// Draw peak volume bar
	int width = mbToPos(m_peakVolume);
	if(width > 0) {
		QColor col = StyleHelper::YellowBaseColor;
		col.setAlpha(64); // 25%
		QRect r = barRect;
		r.setWidth(width);
		p.fillRect(r, col);
	}

	// Draw RMS volume bar
	width = mbToPos(m_rmsVolume);
	if(width > 0) {
		QRect r = barRect;
		r.setWidth(width);
		p.fillRect(r, StyleHelper::YellowBaseColor);
	}

#if 0
	// If the audio input is muted then the bars are rendered at half opacity
	if(m_isMuted) {
		QColor col = grooveCol;
		col.setAlpha(127); // 50%
		p.fillRect(barRect, col);
	}
#endif // 0
}

void VolumeSlider::resizeEvent(QResizeEvent *ev)
{
	// We need to recalculate the handle position
	m_attenuation--; // HACK to force recalculation
	setAttenuation(m_attenuation + 1);
}
