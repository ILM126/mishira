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

#ifndef STYLEDBUTTON_H
#define STYLEDBUTTON_H

#include "stylehelper.h"
#include <QtWidgets/QAbstractButton>

class QStyleOptionButton;

//=============================================================================
class StyledButton : public QAbstractButton
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	StyleHelper::ColorSet	m_colorSet;
	QColor					m_textColor;
	QColor					m_textShadowColor;
	bool					m_hovered;
	QIcon					m_normalIcon;
	QIcon					m_hoverIcon;
	bool					m_joinLeft;
	bool					m_joinRight;

public: // Constructor/destructor ---------------------------------------------
	StyledButton(
		const StyleHelper::ColorSet &colorSet, bool lightText,
		QWidget *parent = 0);
	~StyledButton();

public: // Methods ------------------------------------------------------------
	virtual QSize	sizeHint() const;
	void			setColorSet(const StyleHelper::ColorSet &colorSet);
	void			setLightText(bool lightText);
	void			setIcon(const QIcon &icon);
	void			setHoverIcon(const QIcon &icon);
	void			setJoin(bool left, bool right);

protected:
	void			updateLightText(bool lightText);
	virtual void	initStyleOption(QStyleOptionButton *option) const;

private: // Events ------------------------------------------------------------
	virtual void	enterEvent(QEvent *ev);
	virtual void	leaveEvent(QEvent *ev);

protected:
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

#endif // STYLEDBUTTON_H
