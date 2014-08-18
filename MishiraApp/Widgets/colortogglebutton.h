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

#ifndef COLORTOGGLEBUTTON_H
#define COLORTOGGLEBUTTON_H

#include <QtWidgets/QPushButton>
#include <QtWidgets/QColorDialog>

//=============================================================================
/// <summary>
/// A QPushButton that displays a colour and when pressed toggles to another
/// colour.
/// </summary>
class ColorToggleButton : public QPushButton
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QColor			m_colorA;
	QColor			m_colorB;
	bool			m_curIsA;

public: // Constructor/destructor ---------------------------------------------
	ColorToggleButton(QWidget *parent = 0);
	ColorToggleButton(
		const QColor &aCol, const QColor &bCol, QWidget *parent = 0);
	~ColorToggleButton();

public: // Methods ------------------------------------------------------------
	void			setColorA(const QColor &color);
	QColor			getColorA() const;
	void			setColorB(const QColor &color);
	QColor			getColorB() const;
	void			setColors(const QColor &aCol, const QColor &bCol);
	QColor			getCurrentColor() const;
	void			setIsA(bool isA);
	bool			getIsA() const;

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			currentColorChanged(const QColor &color);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			toggle();
};
//=============================================================================

inline QColor ColorToggleButton::getColorA() const
{
	return m_colorA;
}

inline QColor ColorToggleButton::getColorB() const
{
	return m_colorB;
}

inline QColor ColorToggleButton::getCurrentColor() const
{
	return m_curIsA ? m_colorA : m_colorB;
}

inline bool ColorToggleButton::getIsA() const
{
	return m_curIsA;
}

#endif // COLORTOGGLEBUTTON_H
