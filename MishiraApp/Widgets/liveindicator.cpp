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

#include "liveindicator.h"
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsDropShadowEffect>

LiveIndicator::LiveIndicator(QWidget *parent)
	: QWidget(parent)
	, m_liveColorSet(StyleHelper::RedSet)
	, m_offlineColorSet(StyleHelper::GreenSet)
	, m_textColor()
	, m_textShadowColor()
	, m_isLive(false)
	, m_layout(this)
	, m_label(this)
	, m_sizeHint()
{
	// Setup child items
	m_label.setText(tr("Offline"));
	m_label.setAlignment(Qt::AlignCenter);
	QFont font = m_label.font();
	font.setBold(true);
	m_label.setFont(font);
	m_label.setSizePolicy(
		QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	//m_layout.setMargin(2);
	m_layout.setContentsMargins(2, 1, 2, 3); // Shift text up 1px
	m_layout.setSpacing(0);
	m_layout.addWidget(&m_label);

	// The label is currently in its widest state ("Offline" is wider than
	// "Live"), always use this as the width of the widget. TODO: Not
	// translation safe!
	m_sizeHint = m_label.minimumSizeHint();
	m_sizeHint.rwidth() += 2 * 2 + 12; // Layout margins and extra padding
	m_sizeHint.rheight() += 2 * 2;

	// Determine text colour
	updateLightText(true);
}

LiveIndicator::~LiveIndicator()
{
}

QSize LiveIndicator::minimumSizeHint() const
{
	return m_sizeHint;
}

QSize LiveIndicator::sizeHint() const
{
	return m_sizeHint;
}

void LiveIndicator::setLiveColorSet(const StyleHelper::ColorSet &colorSet)
{
	m_liveColorSet = colorSet;
	if(m_isLive)
		repaint();
}

void LiveIndicator::setOfflineColorSet(const StyleHelper::ColorSet &colorSet)
{
	m_offlineColorSet = colorSet;
	if(!m_isLive)
		repaint();
}

void LiveIndicator::setLightText(bool lightText)
{
	updateLightText(lightText);
	repaint();
}

void LiveIndicator::setIsLive(bool live)
{
	if(m_isLive == live)
		return;
	m_isLive = live;
	if(m_isLive)
		m_label.setText(tr("Live"));
	else
		m_label.setText(tr("Offline"));
	repaint();
}

void LiveIndicator::updateLightText(bool lightText)
{
	if(lightText) {
		m_textColor = QColor(0xFF, 0xFF, 0xFF);
		m_textShadowColor = QColor(0x00, 0x00, 0x00, 0x11);
	} else {
		m_textColor = QColor(0x00, 0x00, 0x00);
		m_textShadowColor = QColor(0xFF, 0xFF, 0xFF, 0x11);
	}
	updateChildLabel(&m_label);
}

void LiveIndicator::updateChildLabel(QLabel *label)
{
	// Set the foreground text colour
	QPalette pal = label->palette();
	if(pal.color(label->foregroundRole()) != m_textColor) {
		pal.setColor(label->foregroundRole(), m_textColor);
		label->setPalette(pal);
	}

#if 0
	// Add a drop shadow effect to the label. The label takes ownership of the
	// effect and will delete it when the label is deleted or when we assign a
	// new effect to the label.
	QGraphicsDropShadowEffect *effect =
		static_cast<QGraphicsDropShadowEffect *>(label->graphicsEffect());
	if(effect == NULL) {
		effect = new QGraphicsDropShadowEffect();
		effect->setBlurRadius(1.0f);
		effect->setColor(m_textShadowColor);
		effect->setOffset(1.0f, 1.0f);
		label->setGraphicsEffect(effect);
	} else if(effect->color() != m_textShadowColor)
		effect->setColor(m_textShadowColor);
#endif // 0
}

void LiveIndicator::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);
	StyleHelper::drawBackground(
		&p, QRect(0, 0, width(), height()),
		m_isLive ? m_liveColorSet : m_offlineColorSet, false, 3.0f);
}
