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

#ifndef COLORBUTTON_H
#define COLORBUTTON_H

#include <QtWidgets/QPushButton>
#include <QtWidgets/QColorDialog>

//=============================================================================
/// <summary>
/// A QPushButton that displays a colour and when pressed opens a colour
/// selection dialog.
/// </summary>
class ColorButton : public QPushButton
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QColor			m_color;
	QColorDialog	m_dialog;

public: // Constructor/destructor ---------------------------------------------
	ColorButton(QWidget *parent = 0);
	ColorButton(const QColor &color, QWidget *parent = 0);
	~ColorButton();

public: // Methods ------------------------------------------------------------
	QColor			getColor() const;

private:
	void			drawCheckerboard(
		QPainter &p, const QRect &rect, int size, const QColor &aCol,
		const QColor &bCol);

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			colorChanged(const QColor &color);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			setColor(const QColor &color);
};
//=============================================================================

inline QColor ColorButton::getColor() const
{
	return m_color;
}

#endif // COLORBUTTON_H
