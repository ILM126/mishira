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

#ifndef STYLEDRADIOTILE_H
#define STYLEDRADIOTILE_H

#include "common.h"
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QVBoxLayout>

//=============================================================================
class StyledRadioTile : public QWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QStackedLayout	m_mainLayout;
	QWidget			m_radioWidget;
	QVBoxLayout		m_radioLayout;
	QRadioButton	m_radio;
	bool			m_selected;
	bool			m_mouseDown;

public: // Constructor/destructor ---------------------------------------------
	StyledRadioTile(QWidget *parent = 0);
	~StyledRadioTile();

public: // Methods ------------------------------------------------------------
	QRadioButton *	getButton();
	void			select();
	bool			isSelected() const;

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	mouseDoubleClickEvent(QMouseEvent *ev);
	virtual void	focusOutEvent(QFocusEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			clicked();
	void			rightClicked();
	void			doubleClicked();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			radioChanged(bool checked);
};
//=============================================================================

inline QRadioButton *StyledRadioTile::getButton()
{
	return &m_radio;
}

inline bool StyledRadioTile::isSelected() const
{
	return m_selected;
}

#endif // STYLEDRADIOTILE_H
