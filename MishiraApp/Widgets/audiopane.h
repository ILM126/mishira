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

#ifndef AUDIOPANE_H
#define AUDIOPANE_H

#include "liveindicator.h"
#include "sortablewidget.h"
#include "stylehelper.h"
#include "Widgets/styledtogglebutton.h"
#include "Widgets/volumeslider.h"
#include <QtCore/QMap>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QWidget>

class AudioInput;

//=============================================================================
class AudioPane : public QWidget, public SortableWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	AudioInput *		m_input;
	QColor				m_fgColor;
	QColor				m_shadowColor;
	QGridLayout			m_outerLayout;
	StyledToggleButton	m_mutedBtn;
	VolumeSlider		m_slider;
	QWidget				m_headerWidget;
	QGridLayout			m_headerLayout;
	QLabel				m_titleLabel;
	bool				m_mouseDown;

public: // Constructor/destructor ---------------------------------------------
	AudioPane(AudioInput *input, QWidget *parent = 0);
	~AudioPane();

protected: // Methods ---------------------------------------------------------
	void				updateForeground();
	void				updateChildLabel(QLabel *label);
	void				updateMuted();
	virtual QWidget *	getWidget();

private: // Events ------------------------------------------------------------
	virtual void		paintEvent(QPaintEvent *ev);
	virtual void		mousePressEvent(QMouseEvent *ev);
	virtual void		mouseReleaseEvent(QMouseEvent *ev);
	virtual void		mouseDoubleClickEvent(QMouseEvent *ev);
	virtual void		focusOutEvent(QFocusEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void				clicked(AudioInput *input);
	void				rightClicked(AudioInput *input);
	void				doubleClicked(AudioInput *input);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void				realTimeFrameEvent(int numDropped, int lateByUsec);
	void				inputChanged(AudioInput *input);
};
//=============================================================================

#endif // AUDIOPANE_H
