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

#ifndef PATTERNWIDGETS_H
#define PATTERNWIDGETS_H

#include <QtWidgets/QWidget>

//=============================================================================
class HeaderPattern : public QWidget
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	HeaderPattern(QWidget *parent = 0);
	~HeaderPattern();

private: // Events ------------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

//=============================================================================
/// <summary>
/// Emits a signal when the user initiates a drag operation.
/// </summary>
class GripPattern : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	bool	m_resetOnEmit;
	bool	m_mouseDown;
	bool	m_emittedDrag;
	QPoint	m_mouseDownPos;

public: // Constructor/destructor ---------------------------------------------
	GripPattern(bool resetOnEmit, QWidget *parent = 0);
	~GripPattern();

private: // Events ------------------------------------------------------------
	virtual void	mouseMoveEvent(QMouseEvent *ev);
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	paintEvent(QPaintEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			dragBegun(const QPoint &startPos);
};
//=============================================================================

#endif // PATTERNWIDGETS_H
