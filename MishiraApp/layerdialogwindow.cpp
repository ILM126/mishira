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

#include "layerdialogwindow.h"
#include "constants.h"
#include "layerdialog.h"
#include "application.h"
#include <QtGui/QKeyEvent>
#include <QtWidgets/QPushButton>

LayerDialogWindow::LayerDialogWindow(QWidget *parent)
	: QWidget(parent)
	, m_dialog(NULL)

	// Outer layout
	, m_outerLayout(this)
	, m_header(this)

	// Inner layout
	, m_innerBox(this)
	, m_innerLayout(&m_innerBox)
	, m_line(&m_innerBox)
	, m_buttons(&m_innerBox)
{
	// Prevent the dialog from being resized and give it a minimum size
	m_innerBox.setMinimumSize(640, 400);
	m_outerLayout.setSizeConstraint(QLayout::SetFixedSize);

	// Make the window modal
	setWindowModality(Qt::WindowModal);

	// This is a child window that does not have a minimize or maximize button
	setWindowFlags(
		Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
		Qt::WindowCloseButtonHint);

	// Setup horizontal bar
	m_line.setFrameStyle(QFrame::HLine | QFrame::Sunken);
	//m_line.setLineWidth(0);

	// Set window title
	setWindowTitle(tr("%1 - Modify scene layer").arg(APP_NAME));

	// Set header title
	m_header.setTitle(tr("Modify scene layer"));

	// Setup button bar
	m_buttons.setStandardButtons(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel |
		QDialogButtonBox::Apply | QDialogButtonBox::Reset);
	connect(m_buttons.button(QDialogButtonBox::Ok), &QPushButton::clicked,
		this, &LayerDialogWindow::okEvent);
	connect(m_buttons.button(QDialogButtonBox::Cancel), &QPushButton::clicked,
		this, &LayerDialogWindow::cancelEvent);
	connect(m_buttons.button(QDialogButtonBox::Apply), &QPushButton::clicked,
		this, &LayerDialogWindow::applyEvent);
	connect(m_buttons.button(QDialogButtonBox::Reset), &QPushButton::clicked,
		this, &LayerDialogWindow::resetEvent);

	// Populate "inner" layout
	m_innerBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_innerLayout.addStretch(1); // Only visible when no dialog is visible
	m_innerLayout.addWidget(&m_line);
	m_innerLayout.addWidget(&m_buttons);

	// Populate "outer" layout
	m_outerLayout.setMargin(0); // Fill entire window
	m_outerLayout.setSpacing(0);
	m_outerLayout.addWidget(&m_header);
	m_outerLayout.addWidget(&m_innerBox);
}

LayerDialogWindow::~LayerDialogWindow()
{
	if(m_dialog != NULL) {
		// Should never happen but be extra safe
		m_dialog->cancelSettings();
		delete m_dialog;
	}
}

void LayerDialogWindow::setDialog(LayerDialog *dialog)
{
	const int INSERT_POS = 0;

	// Remove existing item if one exists
	if(m_dialog != NULL) {
		m_innerLayout.removeWidget(m_dialog);
		delete m_dialog;
	} else
		m_innerLayout.removeItem(m_innerLayout.itemAt(INSERT_POS));

	m_dialog = dialog;
	if(m_dialog == NULL) {
		m_innerLayout.insertStretch(INSERT_POS, 1);
		return;
	}

	// Make sure the dialog always fills the window
	m_dialog->setSizePolicy(
		QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	// Insert the item into the window
	m_innerLayout.insertWidget(0, dialog);
}

void LayerDialogWindow::keyPressEvent(QKeyEvent *ev)
{
	switch(ev->key()) {
	default:
		ev->ignore();
		break;
	case Qt::Key_Escape:
		// ESC press is the same as cancel
		cancelEvent();
		break;
	}
}

void LayerDialogWindow::closeEvent(QCloseEvent *ev)
{
	// Close event is the same as cancel
	if(m_dialog != NULL)
		m_dialog->cancelSettings();
}

void LayerDialogWindow::okEvent()
{
	if(m_dialog != NULL)
		m_dialog->applySettings();
	hide();
	setDialog(NULL);
}

void LayerDialogWindow::cancelEvent()
{
	if(m_dialog != NULL)
		m_dialog->cancelSettings();
	hide();
	setDialog(NULL);
}

void LayerDialogWindow::applyEvent()
{
	if(m_dialog != NULL)
		m_dialog->applySettings();
}

void LayerDialogWindow::resetEvent()
{
	if(m_dialog != NULL)
		m_dialog->resetSettings();
}
