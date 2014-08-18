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

#ifndef LAYERDIALOGWINDOW_H
#define LAYERDIALOGWINDOW_H

#include "Widgets/dialogheader.h"
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>

class LayerDialog;

//=============================================================================
class LayerDialogWindow : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	LayerDialog *		m_dialog;

	// Outer layout
	QVBoxLayout			m_outerLayout;
	DialogHeader		m_header;

	// Inner layout
	QWidget				m_innerBox;
	QVBoxLayout			m_innerLayout;
	QFrame				m_line;
	QDialogButtonBox	m_buttons;

public: // Constructor/destructor ---------------------------------------------
	LayerDialogWindow(QWidget *parent = 0);
	~LayerDialogWindow();

public: // Methods ------------------------------------------------------------
	void			setDialog(LayerDialog *dialog);

private: // Events ------------------------------------------------------------
	virtual void	keyPressEvent(QKeyEvent *ev);
	virtual void	closeEvent(QCloseEvent *ev);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			okEvent();
	void			cancelEvent();
	void			applyEvent();
	void			resetEvent();
};
//=============================================================================

#endif // LAYERDIALOGWINDOW_H
