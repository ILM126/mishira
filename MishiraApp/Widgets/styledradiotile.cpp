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

#include "styledradiotile.h"
#include "stylehelper.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

StyledRadioTile::StyledRadioTile(QWidget *parent)
	: QWidget(parent)
	, m_mainLayout(this)
	, m_radioWidget(this)
	, m_radioLayout(&m_radioWidget)
	, m_radio(&m_radioWidget)
	, m_selected(false)
	, m_mouseDown(false)
{
	// Setup layouts
	m_mainLayout.setStackingMode(QStackedLayout::StackAll);
	m_radioLayout.setMargin(6);

	// Add radio button to the layout
	m_mainLayout.addWidget(&m_radioWidget);
	m_radioLayout.addWidget(&m_radio, 0, Qt::AlignTop);

	// Clicking the widget is the same as clicking the radio button
	connect(this, &StyledRadioTile::clicked,
		&m_radio, &QRadioButton::click);

	// We change our look based on whether or not the radio button is checked
	connect(&m_radio, &QRadioButton::toggled,
		this, &StyledRadioTile::radioChanged);
}

StyledRadioTile::~StyledRadioTile()
{
}

void StyledRadioTile::select()
{
	if(m_radio.isChecked())
		return;
	m_radio.setChecked(true);
}

void StyledRadioTile::paintEvent(QPaintEvent *ev)
{
	// Draw background frame
	QPainter p(this);
	if(m_selected) {
		StyleHelper::drawFrame(
			&p, rect(), StyleHelper::FrameSelectedColor,
			StyleHelper::FrameSelectedBorderColor, false);
	} else {
		StyleHelper::drawFrame(
			&p, rect(), StyleHelper::FrameNormalColor,
			StyleHelper::FrameNormalBorderColor, false);
	}
}

void StyledRadioTile::mousePressEvent(QMouseEvent *ev)
{
	// We're only interested in the left and right mouse buttons
	if(ev->button() != Qt::LeftButton && ev->button() != Qt::RightButton) {
		ev->ignore();
		return;
	}

	// Only accept the event if it's inside the widget rectangle
	if(!rect().contains(ev->pos())) {
		ev->ignore();
		return;
	}

	m_mouseDown = true;
	ev->accept();
}

void StyledRadioTile::mouseReleaseEvent(QMouseEvent *ev)
{
	// We're only interested in the left and right mouse buttons
	if(ev->button() != Qt::LeftButton && ev->button() != Qt::RightButton) {
		ev->ignore();
		return;
	}

	// Only accept the event if it's inside the widget rectangle. We always
	// receive release events if the user first pressed the mouse button inside
	// of our widget area.
	if(!rect().contains(ev->pos())) {
		ev->ignore();
		return;
	}

	// Only consider the release as a click if the user initially pressed the
	// button on this widget as well
	if(!m_mouseDown) {
		ev->ignore();
		return;
	}

	// The user clicked the widget
	m_mouseDown = false;
	if(ev->button() == Qt::LeftButton)
		emit clicked();
	else
		emit rightClicked();
	ev->accept();
}

void StyledRadioTile::mouseDoubleClickEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	emit doubleClicked();
	ev->accept();
}

void StyledRadioTile::focusOutEvent(QFocusEvent *ev)
{
	m_mouseDown = false;
	QWidget::focusOutEvent(ev);
}

void StyledRadioTile::radioChanged(bool checked)
{
	m_selected = checked;
	repaint();
}
