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

#ifndef STYLEDTOGGLEBUTTON_H
#define STYLEDTOGGLEBUTTON_H

#include "styledbutton.h"

//=============================================================================
class StyledToggleButton : public StyledButton
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	StyleHelper::ColorSet	m_checkedColorSet;
	StyleHelper::ColorSet	m_uncheckedColorSet;
	QColor					m_checkedTextColor;
	QColor					m_checkedTextShadowColor;
	QColor					m_uncheckedTextColor;
	QColor					m_uncheckedTextShadowColor;
	bool					m_isTristateButton;
	bool					m_noChange;
	Qt::CheckState			m_checkState;
	bool					m_useNextCheckState;
	Qt::CheckState			m_nextCheckState;

public: // Constructor/destructor ---------------------------------------------
	StyledToggleButton(
		const StyleHelper::ColorSet &checkedColorSet, bool checkedLightText,
		const StyleHelper::ColorSet &uncheckedColorSet, bool uncheckedLightText,
		QWidget *parent = 0);
	~StyledToggleButton();

public: // Methods ------------------------------------------------------------
	bool			isTristateButton() const;
	void			setTristateButton(bool y = true);
	Qt::CheckState	getCheckState() const;
	void			setCheckState(Qt::CheckState state);
	void			setNextCheckState(Qt::CheckState state);

	void			setCheckedTextColor(QColor text, QColor shadow);
	void			setUncheckedTextColor(QColor text, QColor shadow);

protected:
	virtual void	initStyleOption(QStyleOptionButton *option) const;

private: // Events ------------------------------------------------------------
	virtual void	checkStateSet();
	virtual void	nextCheckState();

protected:
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

inline bool StyledToggleButton::isTristateButton() const
{
	return m_isTristateButton;
}

inline void StyledToggleButton::setTristateButton(bool y)
{
	m_isTristateButton = y;
}

#endif // STYLEDTOGGLEBUTTON_H
