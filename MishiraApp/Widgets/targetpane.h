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

#ifndef TARGETPANE_H
#define TARGETPANE_H

#include "liveindicator.h"
#include "sortablewidget.h"
#include "stylehelper.h"
#include <QtCore/QMap>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QWidget>

class Target;

//=============================================================================
class TargetPane : public QWidget, public SortableWidget
{
	Q_OBJECT

public: // Datatypes ----------------------------------------------------------
	enum PaneState {
		OfflineState = 0,
		LiveState,
		WarningState
	};

protected: // Members ---------------------------------------------------------
	Target *			m_target;
	QColor				m_fgColor;
	QColor				m_shadowColor;
	bool				m_isUserEnabled;
	QGridLayout			m_outerLayout;
	QWidget				m_liveBarLeft;
	QWidget				m_liveBarRight;
	QWidget				m_innerWidget;
	QGridLayout			m_innerLayout;
	QCheckBox			m_userEnabledCheckBox;
	QLabel				m_titleLabel;
	QLabel				m_iconLabel;
	QMap<int, QLabel *>	m_leftText;
	QMap<int, QLabel *>	m_rightText;
	QWidget				m_textWidget;
	QGridLayout			m_textLayout;
	bool				m_mouseDown;

public: // Constructor/destructor ---------------------------------------------
	TargetPane(Target *target, QWidget *parent = 0);
	~TargetPane();

public: // Methods ------------------------------------------------------------
	void				setUserEnabled(bool enable);
	void				setPaneState(PaneState state);
	void				setTitle(const QString &text);
	void				setIconPixmap(const QPixmap &icon);
	void				setItemText(
		int id, const QString &left, const QString &right,
		bool doUpdateLayout);

protected:
	void				updateForeground();
	void				updateChildLabel(QLabel *label, bool isLeft);
	void				updateLayout();
	virtual QWidget *	getWidget();

private: // Events ------------------------------------------------------------
	virtual void		paintEvent(QPaintEvent *ev);
	virtual void		mousePressEvent(QMouseEvent *ev);
	virtual void		mouseReleaseEvent(QMouseEvent *ev);
	virtual void		mouseDoubleClickEvent(QMouseEvent *ev);
	virtual void		focusOutEvent(QFocusEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void				clicked(Target *target);
	void				rightClicked(Target *target);
	void				doubleClicked(Target *target);
	void				activeStateChanged(bool active);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void				activeCheckBoxClicked();
	void				targetEnabledChanged(Target *target, bool enabled);
	void				enableUpdatesTimeout();
};
//=============================================================================

#endif // TARGETPANE_H
