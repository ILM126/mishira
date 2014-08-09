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

#include "elidedlabel.h"
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>

ElidedLabel::ElidedLabel(QWidget *parent, Qt::WindowFlags f)
	: QLabel(parent, f)
	, m_elideMode(Qt::ElideRight)
	, m_elidedText()
{
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(
	const QString &txt, QWidget *parent, Qt::WindowFlags f)
	: QLabel(txt, parent, f)
	, m_elideMode(Qt::ElideRight)
	, m_elidedText()
{
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(
	const QString &txt, Qt::TextElideMode elideMode, QWidget *parent,
	Qt::WindowFlags f)
	: QLabel(txt, parent, f)
	, m_elideMode(elideMode)
	, m_elidedText()
{
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

ElidedLabel::~ElidedLabel()
{
}

void ElidedLabel::setText(const QString &txt)
{
	QLabel::setText(txt);
	calcElidedText(geometry().width());
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
}

void ElidedLabel::calcElidedText(int width) {
	m_elidedText = fontMetrics().elidedText(
		text(), m_elideMode, width, Qt::TextShowMnemonic);

	// Display a tooltip of the complete text only if the text is elided
	if(text() != m_elidedText)
		setToolTip(text());
	else
		setToolTip(QString());
}

void ElidedLabel::resizeEvent(QResizeEvent *ev)
{
	QLabel::resizeEvent(ev);
	calcElidedText(ev->size().width());
}

void ElidedLabel::paintEvent(QPaintEvent *ev)
{
	if(m_elideMode == Qt::ElideNone) {
		QLabel::paintEvent(ev);
		return;
	}
	QPainter p(this);
	p.drawText(0, 0, geometry().width(), geometry().height(),
		alignment(), m_elidedText);
}
