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

#include "bitratecalcwindow.h"
#include "common.h"
#include "constants.h"
#include "Widgets/bitratecalcwidget.h"
#include <QtGui/QKeyEvent>
#include <QtWidgets/QPushButton>

BitrateCalcWindow::BitrateCalcWindow(QWidget *parent)
	: QWidget(parent)
	, m_calcWidget(NULL)

	// Outer layout
	, m_outerLayout(this)
	, m_header(this)

	// Inner layout
	, m_innerBox(this)
	, m_innerLayout(&m_innerBox)
	, m_line(&m_innerBox)
	, m_buttons(&m_innerBox)
{
	// Prevent the dialog from being resized
	m_outerLayout.setSizeConstraint(QLayout::SetFixedSize);

	// This is a child window that does not have a minimize or maximize button
	setWindowFlags(
		Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
		Qt::WindowCloseButtonHint);

	// Set window title
	setWindowTitle(tr("%1 - Video and audio settings calculator")
		.arg(APP_NAME));

	// Setup horizontal bar
	m_line.setFrameStyle(QFrame::HLine | QFrame::Sunken);
	//m_line.setLineWidth(0);

	// Set header title
	m_header.setTitle(tr("Video and audio settings calculator"));

	// Setup button bar
	m_buttons.setStandardButtons(
		QDialogButtonBox::Ok);
	connect(m_buttons.button(QDialogButtonBox::Ok), &QPushButton::clicked,
		this, &BitrateCalcWindow::okEvent);

	// Setup calculator widget
	m_calcWidget = new BitrateCalcWidget(this);

	// Populate "inner" layout
	m_innerBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_innerLayout.addWidget(m_calcWidget);
	m_innerLayout.addWidget(&m_line);
	m_innerLayout.addWidget(&m_buttons);

	// Populate "outer" layout
	m_outerLayout.setMargin(0); // Fill entire window
	m_outerLayout.setSpacing(0);
	m_outerLayout.addWidget(&m_header);
	m_outerLayout.addWidget(&m_innerBox);
}

BitrateCalcWindow::~BitrateCalcWindow()
{
	delete m_calcWidget;
}

void BitrateCalcWindow::keyPressEvent(QKeyEvent *ev)
{
	switch(ev->key()) {
	default:
		ev->ignore();
		break;
	case Qt::Key_Escape:
		// ESC press is the same as "okay"
		okEvent();
		break;
	}
}

void BitrateCalcWindow::okEvent()
{
	hide();
}
