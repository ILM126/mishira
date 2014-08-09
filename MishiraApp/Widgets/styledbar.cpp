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

#include "styledbar.h"
#include "borderspacer.h"
#include "stylehelper.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsDropShadowEffect>

StyledBar::StyledBar(
	const QColor &fgColor, const QColor &bgColor, const QColor &shadowColor,
	QWidget *parent)
	: QWidget(parent)
	, m_fgColor(fgColor)
	, m_bgColor(bgColor)
	, m_shadowColor(shadowColor)
	, m_outerLayout(this)
	, m_innerWidget(this)
	, m_innerLayout(&m_innerWidget)
	, m_labelWidget(&m_innerWidget)
	, m_labelLayout(&m_labelWidget)
	, m_label(&m_labelWidget)
	, m_leftWidgets()
	, m_rightWidgets()
	, m_rightLabelWidgets()
	, m_mouseDown(false)
{
	construct();
}

StyledBar::StyledBar(
	const QColor &fgColor, const QColor &bgColor, const QColor &shadowColor,
	const QString &label, QWidget *parent)
	: QWidget(parent)
	, m_fgColor(fgColor)
	, m_bgColor(bgColor)
	, m_shadowColor(shadowColor)
	, m_outerLayout(this)
	, m_innerWidget(this)
	, m_innerLayout(&m_innerWidget)
	, m_labelWidget(&m_innerWidget)
	, m_labelLayout(&m_labelWidget)
	, m_label(label, &m_labelWidget)
	, m_isLabelEditable(false)
	, m_leftWidgets()
	, m_rightWidgets()
	, m_rightLabelWidgets()
	, m_mouseDown(false)
{
	construct();
}

void StyledBar::construct()
{
	// Remove layout margins so that the child widgets fill the entire area
	m_outerLayout.setMargin(0);
	m_outerLayout.setSpacing(0);
	m_innerLayout.setMargin(0);
	m_innerLayout.setSpacing(0);
	m_labelLayout.setMargin(0);
	m_labelLayout.setSpacing(0);

	// Setup outer layout
	setBorders(0, 0);

	// Setup out label style
	QMargins margins = m_label.getActualTextMargins();
	m_labelLayout.setContentsMargins(
		6 - margins.left(), 0, 0, 0); // 6px indent by default
	updateForeground();

	// Connect label edited signal
	connect(&m_label, &EditableLabel::editingFinished,
		this, &StyledBar::labelChanged);

	// Initialize the layout
	updateLayout();
}

StyledBar::~StyledBar()
{
}

void StyledBar::updateForeground()
{
	m_label.setTextColor(m_fgColor);

	// Add a drop shadow effect to the label. The label takes ownership of the
	// effect and will delete it when the label is deleted or when we assign a
	// new effect to the label.
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(1.0f);
	effect->setColor(m_shadowColor);
	effect->setOffset(1.0f, 1.0f);
	m_label.setGraphicsEffect(effect);
}

void StyledBar::addLeftWidget(QWidget *widget, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_leftWidgets.count() + 1;
	}
	before = qBound(0, before, m_leftWidgets.count());

	m_leftWidgets.insert(before, widget);
	updateLayout();
}

void StyledBar::addRightWidget(QWidget *widget, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_rightWidgets.count() + 1;
	}
	before = qBound(0, before, m_rightWidgets.count());

	m_rightWidgets.insert(before, widget);
	updateLayout();
}

void StyledBar::addRightLabelWidget(QWidget *widget, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_rightLabelWidgets.count() + 1;
	}
	before = qBound(0, before, m_rightLabelWidgets.count());

	m_rightLabelWidgets.insert(before, widget);
	updateLayout();
}

void StyledBar::removeLeftWidget(QWidget *widget)
{
	int index = m_leftWidgets.indexOf(widget);
	if(index < 0)
		return;
	m_leftWidgets.remove(index);
	updateLayout();
}

void StyledBar::removeRightWidget(QWidget *widget)
{
	int index = m_rightWidgets.indexOf(widget);
	if(index < 0)
		return;
	m_rightWidgets.remove(index);
	updateLayout();
}

void StyledBar::removeRightLabelWidget(QWidget *widget)
{
	int index = m_rightLabelWidgets.indexOf(widget);
	if(index < 0)
		return;
	m_rightLabelWidgets.remove(index);
	updateLayout();
}

void StyledBar::setFgColor(const QColor &fgColor, const QColor &shadowColor)
{
	m_fgColor = fgColor;
	m_shadowColor = shadowColor;
	updateForeground();
	repaint();
}

void StyledBar::setBgColor(const QColor &color)
{
	m_bgColor = color;
	repaint();
}

void StyledBar::setEditableLabel(bool editable, bool nowAndFocus)
{
	if(m_isLabelEditable == editable) {
		if(editable && nowAndFocus) {
			// Immediately enter edit mode and give the widget focus
			m_label.setInEditMode(true);
		}
		return;
	}
	m_isLabelEditable = editable;
	if(!editable) {
		m_label.setInEditMode(false);
		m_label.forceFocusLost(); // Kill the click timer if enabled
	}
	m_label.setAllowClickEdit(editable);
	if(editable && nowAndFocus) {
		// Immediately enter edit mode and give the widget focus
		m_label.setInEditMode(true);
	}
	repaint();
}

void StyledBar::setBorders(int top, int bottom)
{
	top = qMax(0, top);
	bottom = qMax(0, bottom);

	// Remove all widgets from the outer layout
	if(m_innerLayout.count()) {
		delete m_innerLayout.takeAt(2);
		delete m_innerLayout.takeAt(0);
		while(m_innerLayout.takeAt(0) != 0);
	}

	// Set our maximum height
	setFixedHeight(STYLED_BAR_HEIGHT + top + bottom);

	// Update layout
	m_outerLayout.addWidget(new VBorderSpacer(this, top));
	m_outerLayout.addWidget(&m_innerWidget);
	m_outerLayout.addWidget(new VBorderSpacer(this, bottom));
}

void StyledBar::setLabel(const QString &label)
{
	m_label.setText(label);
	repaint();
}

void StyledBar::setLabelIndent(int indent)
{
	QMargins margins = m_label.getActualTextMargins();
	m_labelLayout.setContentsMargins(indent - margins.left(), 0, 0, 0);
	repaint();
}

void StyledBar::setLabelBold(bool bold)
{
	QFont font = m_label.font();
	if(font.bold() == bold)
		return;
	font.setBold(bold);
	m_label.setFont(font);
}

/// <summary>
/// Recalculates the layout of the child widgets and label.
/// </summary>
void StyledBar::updateLayout()
{
	// Remove all widgets from the main layout
	while(m_innerLayout.takeAt(0) != 0);

	// Re-add all widgets to the main layout in the correct order
	for(int i = 0; i < m_leftWidgets.count(); i++)
		m_innerLayout.addWidget(m_leftWidgets.at(i));
	m_innerLayout.addWidget(&m_labelWidget);
	for(int i = 0; i < m_rightWidgets.count(); i++)
		m_innerLayout.addWidget(m_rightWidgets.at(i));

	// Remove all widgets from the label layout
	while(m_labelLayout.takeAt(0) != 0);

	// Re-add all widgets to the label layout in the correct order
	QMargins margins = m_label.getActualTextMargins();
	m_labelLayout.addWidget(&m_label);
	m_labelLayout.addSpacing(10 - margins.right()); // 10px right margin
	for(int i = 0; i < m_rightLabelWidgets.count(); i++)
		m_labelLayout.addWidget(m_rightLabelWidgets.at(i));
}

void StyledBar::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);
	p.fillRect(QRect(0, 0, width(), height()), m_bgColor);

	// The following only makes sense if the label area has a border around it
	// (This was written back when a styled bar was embossed)
#if 0
	// We only paint the area behind the label
	if(m_bgColor.alpha() > 0) {
		QPainter p(this);
		p.fillRect(m_labelWidget.geometry(), m_bgColor);
	}
#endif // 0
}

void StyledBar::mousePressEvent(QMouseEvent *ev)
{
	// We're only interested in the left and right mouse buttons
	if(ev->button() != Qt::LeftButton && ev->button() != Qt::RightButton) {
		ev->ignore();
		return;
	}

	// Only accept the event if it's inside the widget rectangle
	if(!rect().contains(ev->pos())) {
		ev->ignore();
		return;
	}

	m_mouseDown = true;
	ev->accept();
}

void StyledBar::mouseReleaseEvent(QMouseEvent *ev)
{
	// We're only interested in the left and right mouse buttons
	if(ev->button() != Qt::LeftButton && ev->button() != Qt::RightButton) {
		ev->ignore();
		return;
	}

	// Only accept the event if it's inside the widget rectangle. We always
	// receive release events if the user first pressed the mouse button inside
	// of our widget area.
	if(!rect().contains(ev->pos())) {
		ev->ignore();
		return;
	}

	// Only consider the release as a click if the user initially pressed the
	// button on this widget as well
	if(!m_mouseDown) {
		ev->ignore();
		return;
	}

	// The user clicked the widget
	m_mouseDown = false;
	if(ev->button() == Qt::LeftButton)
		emit clicked();
	else
		emit rightClicked();
	ev->accept();
}

void StyledBar::mouseDoubleClickEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	emit doubleClicked();
	ev->accept();
}

void StyledBar::focusOutEvent(QFocusEvent *ev)
{
	m_mouseDown = false;
	QWidget::focusOutEvent(ev);
}

void StyledBar::labelChanged()
{
	emit labelEdited(m_label.text());
}
