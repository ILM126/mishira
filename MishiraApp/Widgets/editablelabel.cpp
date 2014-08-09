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

#include "editablelabel.h"
#include "application.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOption>

EditableLabel::EditableLabel(QWidget *parent)
	: QLineEdit(parent)
	, m_inEditMode(false)
	, m_allowClickEdit(false)
	, m_editTimer(0)
	, m_editTimerEnabled(false)
	, m_mouseDown(false)
	, m_baseColor(palette().base().color())
	, m_origTextColor(palette().text().color())
	, m_borderColor(Qt::black)
	, m_textColor(m_origTextColor)
	, m_fullText()
{
	setFrame(false);
	exitEditMode();
	connect(this, &EditableLabel::editingFinished,
		this, &EditableLabel::internalTextChanged);

	// Move text up 1px while keeping the bottom looking nice
	QMargins margins = textMargins();
	setTextMargins(
		margins.left(), margins.top() - 1,
		margins.right(), margins.bottom());
}

EditableLabel::EditableLabel(const QString &contents, QWidget *parent)
	: QLineEdit(parent)
	, m_inEditMode(false)
	, m_allowClickEdit(false)
	, m_editTimer(0)
	, m_editTimerEnabled(false)
	, m_mouseDown(false)
	, m_baseColor(palette().base().color())
	, m_origTextColor(palette().text().color())
	, m_borderColor(Qt::black)
	, m_textColor(m_origTextColor)
	, m_fullText()
{
	setFrame(false);
	setText(contents);
	exitEditMode();
	connect(this, &EditableLabel::editingFinished,
		this, &EditableLabel::internalTextChanged);

	// Move text up 1px while keeping the bottom looking nice
	QMargins margins = textMargins();
	setTextMargins(
		margins.left(), margins.top() - 1,
		margins.right(), margins.bottom());
}

EditableLabel::~EditableLabel()
{
}

void EditableLabel::setInEditMode(bool editMode)
{
	if(m_inEditMode == editMode)
		return;
	m_inEditMode = editMode;
	m_mouseDown = false;
	if(m_inEditMode)
		enterEditMode();
	else
		exitEditMode();
	repaint();
}

void EditableLabel::setTextColor(const QColor &color)
{
	if(m_textColor == color)
		return;
	m_textColor = color;
	if(!m_inEditMode) {
		QPalette pal = palette();
		pal.setColor(QPalette::Text, m_textColor);
		setPalette(pal);
		repaint();
	}
}

void EditableLabel::setEditColors(
	const QColor &baseCol, const QColor &textCol, const QColor &borderCol)
{
	m_baseColor = baseCol;
	m_origTextColor = textCol;
	m_borderColor = borderCol;
	if(m_inEditMode) {
		QPalette pal = palette();
		pal.setColor(QPalette::Base, m_baseColor);
		pal.setColor(QPalette::Text, m_origTextColor);
		setPalette(pal);
		repaint();
	}
}

void EditableLabel::enterEditMode()
{
	QLineEdit::setText(m_fullText);
	setEnabled(true);
	setFocus();
	selectAll();

	QPalette pal = palette();
	pal.setColor(QPalette::Base, m_baseColor);
	pal.setColor(QPalette::Text, m_origTextColor);
	setPalette(pal);
}

void EditableLabel::exitEditMode()
{
	if(hasFocus()) {
		QWidget *parent = parentWidget();
		if(parent != NULL)
			parent->setFocus();
	}

	setCursorPosition(0);
	setEnabled(false);

	QPalette pal = palette();
	pal.setColor(QPalette::Base, Qt::transparent);
	pal.setColor(QPalette::Text, m_textColor);
	setPalette(pal);
}

/// <summary>
/// Get the position of the text inside the edit box.
/// </summary>
QMargins EditableLabel::getActualTextMargins()
{
	QMargins margins = textMargins();

	QStyleOptionFrameV2 panel;
	initStyleOption(&panel);
	QRect r =
		style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
	r.setX(r.x() + margins.left());
	r.setY(r.y() + margins.top());
	r.setRight(r.right() - margins.right());
	r.setBottom(r.bottom() - margins.bottom());

	// The +2 and +1 is from the `QLineEditPrivate::*Margin` constants
	margins.setLeft(r.left() + 2);
	margins.setTop(r.top() + 1);
	margins.setRight(width() - r.right() - 1 + 2);
	margins.setBottom(height() - r.bottom() - 1 + 1);

	return margins;
}

/// Reimplementation
void EditableLabel::setText(const QString &text)
{
	// Changing the text while in edit mode makes it extremely difficult for
	// the user to edit the text as it undoes everything the user has done
	if(m_inEditMode)
		return;

	m_fullText = text;
	calculateDisplayText();
	//if(m_inEditMode)
	//	QLineEdit::setText(m_fullText);
	//else
	QLineEdit::setText(m_displayText);
}

void EditableLabel::calculateDisplayText()
{
	// Get display rectangle (Based off `QLineEdit::paintEvent()`)
	QMargins margins = textMargins();
	QStyleOptionFrameV2 panel;
	initStyleOption(&panel);
	QRect r =
		style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
	r.setX(r.x() + margins.left());
	r.setY(r.y() + margins.top());
	r.setRight(r.right() - margins.right());
	r.setBottom(r.bottom() - margins.bottom());
	QFontMetrics fm = fontMetrics();
	Qt::Alignment va = QStyle::visualAlignment(layoutDirection(), alignment());
	int vscroll;
	switch (va & Qt::AlignVertical_Mask) {
	case Qt::AlignBottom:
		vscroll = r.y() + r.height() - fm.height() - 1;
		break;
	case Qt::AlignTop:
		vscroll = r.y() + 1;
		break;
	default: // Center
		vscroll = r.y() + (r.height() - fm.height() + 1) / 2;
		break;
	}
	QRect lineRect(r.x() + 2, vscroll, r.width() - 2 * 2, fm.height());

	// Calculate elided display text. We make the rectangle slightly smaller to
	// make sure it is always left aligned
	lineRect.setRight(qMax(lineRect.left(), lineRect.right() - 2));
	m_displayText =
		fm.elidedText(m_fullText, Qt::ElideRight, lineRect.width());
}

bool EditableLabel::event(QEvent *ev)
{
	// Receive mouse press and release events even when disabled
	if(!isEnabled()) {
		switch(ev->type()) {
		case QEvent::MouseButtonPress:
			mousePressEvent((QMouseEvent *)ev);
			return true;
		case QEvent::MouseButtonRelease:
			mouseReleaseEvent((QMouseEvent *)ev);
			return true;
		case QEvent::MouseButtonDblClick:
			mouseDoubleClickEvent((QMouseEvent *)ev);
			return true;
		default:
			break;
		}
	}
	return QLineEdit::event(ev);
}

void EditableLabel::keyPressEvent(QKeyEvent *ev)
{
	if(!m_inEditMode) {
		ev->ignore();
		return;
	}
	switch(ev->key()) {
	default:
		// Forward most keys to the line edit itself
		QLineEdit::keyPressEvent(ev);
		return;
	case Qt::Key_Enter:
	case Qt::Key_Return:
		// Forward (To emit change signal) then exit edit mode
		QLineEdit::keyPressEvent(ev);
		setInEditMode(false);
		ev->accept();
		return;
	case Qt::Key_Escape:
		// Exit edit mode
		setInEditMode(false);
		ev->accept();
		return;
	}
	ev->ignore();
}

void EditableLabel::mousePressEvent(QMouseEvent *ev)
{
	if(m_inEditMode) {
		// Ignore mouse presses when in edit mode
		QLineEdit::mousePressEvent(ev);
		return;
	}
	if(!m_allowClickEdit) {
		ev->ignore();
		return;
	}

	//-------------------------------------------------------------------------
	// Detect when the widget is clicked to enter edit mode

	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
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

void EditableLabel::mouseReleaseEvent(QMouseEvent *ev)
{
	if(m_inEditMode) {
		// Ignore mouse presses when in edit mode
		QLineEdit::mouseReleaseEvent(ev);
		return;
	}
	if(!m_allowClickEdit) {
		ev->ignore();
		return;
	}

	//-------------------------------------------------------------------------
	// Detect when the widget is clicked to enter edit mode

	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
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

	// The user clicked the widget, enter edit mode after a short delay
	m_mouseDown = false;
	ev->accept();
	if(!m_editTimerEnabled) {
		m_editTimerEnabled = true;
		m_editTimer = startTimer(App->doubleClickInterval());
	}
}

void EditableLabel::mouseDoubleClickEvent(QMouseEvent *ev)
{
	if(m_editTimerEnabled) {
		m_editTimerEnabled = false;
		killTimer(m_editTimer);
	}
	ev->ignore();
}

/// <summary>
/// Notify the widget that it has lost keyboard focus even if it never received
/// the Qt event. This is because we sometimes fake focus (E.g. layer list) and
/// the item doesn't actually have keyboard focus.
/// </summary>
void EditableLabel::forceFocusLost()
{
	setInEditMode(false); // Exit edit mode when we lose keyboard focus
	m_mouseDown = false;
	if(m_editTimerEnabled) {
		m_editTimerEnabled = false;
		killTimer(m_editTimer);
	}
}

void EditableLabel::focusOutEvent(QFocusEvent *ev)
{
	forceFocusLost();
	QLineEdit::focusOutEvent(ev);
}

void EditableLabel::timerEvent(QTimerEvent *ev)
{
	if(m_editTimerEnabled)
		killTimer(m_editTimer);
	m_editTimerEnabled = false;
	setInEditMode(true);
}

void EditableLabel::paintEvent(QPaintEvent *ev)
{
	QLineEdit::paintEvent(ev);

	// Draw a contrasting outline around the box in edit mode
	if(!m_inEditMode)
		return;
	QPainter p(this);
	p.setPen(m_borderColor);
	p.drawRect(rect().adjusted(0, 0, -1, -1));
}

void EditableLabel::resizeEvent(QResizeEvent *ev)
{
	Q_UNUSED(ev);

	// Recalculate the display text
	if(m_inEditMode)
		return; // Nothing to do
	calculateDisplayText();
	QLineEdit::setText(m_displayText);
}

void EditableLabel::internalTextChanged()
{
	// Regenerate display text and cache full text
	setText(QLineEdit::text());
}
