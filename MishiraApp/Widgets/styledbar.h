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

#ifndef STYLEDBAR_H
#define STYLEDBAR_H

#include "stylehelper.h"
#include "Widgets/editablelabel.h"
#include <QtCore/QVector>
#include <QtWidgets/QWidget>
#include <QtWidgets/QHBoxLayout>

// We must use `QWidgetVector` as `QWidgetList` is already defined
typedef QVector<QWidget *> QWidgetVector;

const int STYLED_BAR_HEIGHT = 22;

//=============================================================================
class StyledBar : public QWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QColor			m_fgColor;
	QColor			m_bgColor;
	QColor			m_shadowColor;
	QVBoxLayout		m_outerLayout;
	QWidget			m_innerWidget;
	QHBoxLayout		m_innerLayout;
	QWidget			m_labelWidget;
	QHBoxLayout		m_labelLayout;
	EditableLabel	m_label;
	bool			m_isLabelEditable;
	QWidgetVector	m_leftWidgets;
	QWidgetVector	m_rightWidgets;
	QWidgetVector	m_rightLabelWidgets;
	bool			m_mouseDown;

public: // Constructor/destructor ---------------------------------------------
	StyledBar(
		const QColor &fgColor, const QColor &bgColor,
		const QColor &shadowColor, QWidget *parent = 0);
	StyledBar(
		const QColor &fgColor, const QColor &bgColor,
		const QColor &shadowColor, const QString &label, QWidget *parent = 0);
	~StyledBar();

private:
	void			construct();

public: // Methods ------------------------------------------------------------
	void			addLeftWidget(QWidget *widget, int before = -1);
	void			addRightWidget(QWidget *widget, int before = -1);
	void			addRightLabelWidget(QWidget *widget, int before = -1);
	void			removeLeftWidget(QWidget *widget);
	void			removeRightWidget(QWidget *widget);
	void			removeRightLabelWidget(QWidget *widget);
	QWidgetVector	getLeftWidgets() const;
	QWidgetVector	getRightWidgets() const;
	QWidgetVector	getRightLabelWidgets() const;
	void			setFgColor(
		const QColor &fgColor, const QColor &shadowColor);
	void			setBgColor(const QColor &color);
	void			setEditableLabel(bool editable, bool nowAndFocus = false);
	void			setBorders(int top, int bottom);
	void			setLabel(const QString &label);
	void			setLabelIndent(int indent);
	void			setLabelBold(bool bold);

private:
	void			updateForeground();
	void			updateLayout();

private: // Events ------------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	mouseDoubleClickEvent(QMouseEvent *ev);
	virtual void	focusOutEvent(QFocusEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			clicked();
	void			rightClicked();
	void			doubleClicked();
	void			labelEdited(const QString &text);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			labelChanged();
};
//=============================================================================

inline QWidgetVector StyledBar::getLeftWidgets() const
{
	return m_leftWidgets;
}

inline QWidgetVector StyledBar::getRightWidgets() const
{
	return m_rightWidgets;
}

inline QWidgetVector StyledBar::getRightLabelWidgets() const
{
	return m_rightLabelWidgets;
}

#endif // STYLEDBAR_H
