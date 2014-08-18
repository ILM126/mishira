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

#ifndef BITRATECALCWINDOW_H
#define BITRATECALCWINDOW_H

#include "Widgets/dialogheader.h"
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>

class BitrateCalcWidget;

//=============================================================================
class BitrateCalcWindow : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	BitrateCalcWidget *	m_calcWidget;

	// Outer layout
	QVBoxLayout			m_outerLayout;
	DialogHeader		m_header;

	// Inner layout
	QWidget				m_innerBox;
	QVBoxLayout			m_innerLayout;
	QFrame				m_line;
	QDialogButtonBox	m_buttons;

public: // Constructor/destructor ---------------------------------------------
	BitrateCalcWindow(QWidget *parent = 0);
	~BitrateCalcWindow();

private: // Events ------------------------------------------------------------
	virtual void	keyPressEvent(QKeyEvent *ev);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			okEvent();
};
//=============================================================================

#endif // BITRATECALCWINDOW_H
