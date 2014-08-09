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

#include "styledtogglebutton.h"
#include <QtWidgets/QStyleOption>

StyledToggleButton::StyledToggleButton(
	const StyleHelper::ColorSet &checkedColorSet, bool checkedLightText,
	const StyleHelper::ColorSet &uncheckedColorSet, bool uncheckedLightText,
	QWidget *parent)
	: StyledButton(uncheckedColorSet, uncheckedLightText, parent)
	, m_checkedColorSet(checkedColorSet)
	, m_uncheckedColorSet(uncheckedColorSet)
	, m_checkedTextColor()
	, m_checkedTextShadowColor()
	, m_uncheckedTextColor()
	, m_uncheckedTextShadowColor()
	, m_isTristateButton(false)
	, m_noChange(false)
	, m_checkState(Qt::Unchecked)
	, m_useNextCheckState(false)
	, m_nextCheckState(Qt::Unchecked)
{
	setCheckable(true);

	// Determine text colours
	if(checkedLightText) {
		m_checkedTextColor = QColor(0xFF, 0xFF, 0xFF);
		m_checkedTextShadowColor = QColor(0x00, 0x00, 0x00, 0x54);
	} else {
		m_checkedTextColor = QColor(0x00, 0x00, 0x00);
		m_checkedTextShadowColor = QColor(0xFF, 0xFF, 0xFF, 0x54);
	}
	if(uncheckedLightText) {
		m_uncheckedTextColor = QColor(0xFF, 0xFF, 0xFF);
		m_uncheckedTextShadowColor = QColor(0x00, 0x00, 0x00, 0x54);
	} else {
		m_uncheckedTextColor = QColor(0x00, 0x00, 0x00);
		m_uncheckedTextShadowColor = QColor(0xFF, 0xFF, 0xFF, 0x54);
	}
}

StyledToggleButton::~StyledToggleButton()
{
}

Qt::CheckState StyledToggleButton::getCheckState() const
{
	if(m_isTristateButton && m_noChange)
		return Qt::PartiallyChecked;
	return isChecked() ? Qt::Checked : Qt::Unchecked;
}

void StyledToggleButton::setCheckState(Qt::CheckState state)
{
	if(state == Qt::PartiallyChecked) {
		m_isTristateButton = true;
		m_noChange = true;
	} else
		m_noChange = false;
	//d->blockRefresh = true;
	setChecked(state != Qt::Unchecked);
	//d->blockRefresh = false;
	//d->refresh();
	//if((uint)state != d->publishedState) {
	//	d->publishedState = state;
	//	emit stateChanged(state);
	//}
}

void StyledToggleButton::setNextCheckState(Qt::CheckState state)
{
	m_useNextCheckState = true;
	m_nextCheckState = state;
}

void StyledToggleButton::setCheckedTextColor(QColor text, QColor shadow)
{
	m_checkedTextColor = text;
	m_checkedTextShadowColor = shadow;
	repaint();
}

void StyledToggleButton::setUncheckedTextColor(QColor text, QColor shadow)
{
	m_uncheckedTextColor = text;
	m_uncheckedTextShadowColor = shadow;
	repaint();
}

void StyledToggleButton::initStyleOption(QStyleOptionButton *option) const
{
	StyledButton::initStyleOption(option);
	if(m_isTristateButton && m_noChange)
		option->state |= QStyle::State_NoChange;
	else
		option->state |= isChecked() ? QStyle::State_On : QStyle::State_Off;
}

void StyledToggleButton::checkStateSet()
{
	m_noChange = false;
	//Qt::CheckState state = getCheckState();
	//if((uint)state != d->publishedState) {
	//	d->publishedState = state;
	//	emit stateChanged(state);
	//}
}

void StyledToggleButton::nextCheckState()
{
	if(m_useNextCheckState) {
		setCheckState(m_nextCheckState);
		m_useNextCheckState = false;
	} else if(m_isTristateButton)
		setCheckState((Qt::CheckState)((getCheckState() + 1) % 3));
	else {
		QAbstractButton::nextCheckState();
		checkStateSet();
	}
}

void StyledToggleButton::paintEvent(QPaintEvent *ev)
{
	// Change the active color set based on button state
	switch(getCheckState()) {
	default:
	case Qt::Unchecked:
		m_colorSet = m_uncheckedColorSet;
		m_textColor = m_uncheckedTextColor;
		m_textShadowColor = m_uncheckedTextShadowColor;
		break;
	case Qt::Checked:
		m_colorSet = m_checkedColorSet;
		m_textColor = m_checkedTextColor;
		m_textShadowColor = m_checkedTextShadowColor;
		break;
	case Qt::PartiallyChecked: // TODO: Not implemented
		m_colorSet = m_checkedColorSet;
		m_textColor = m_checkedTextColor;
		m_textShadowColor = m_checkedTextShadowColor;
		break;
	}

	// Forward to super class
	StyledButton::paintEvent(ev);
}
