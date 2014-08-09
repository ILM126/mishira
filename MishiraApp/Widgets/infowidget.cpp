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

#include "infowidget.h"
#include "stylehelper.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsDropShadowEffect>

//=============================================================================
// InfoWidget class

InfoWidget::InfoWidget(QWidget *parent)
	: QWidget(parent)
	, m_radio(this)
	, m_selected(false)
	, m_titleLabel(this)
	, m_iconLabel(this)
	, m_leftText()
	, m_rightText()
	, m_rightTextMaxWidth(280)
	, m_mainLayout(this)
	, m_textWidget(this)
	, m_textLayout(&m_textWidget)
	, m_mouseDown(false)
{
	// Setup layout margins
	m_mainLayout.setMargin(6);
	//m_mainLayout.setSpacing(0);
	m_textLayout.setMargin(0);
	m_textLayout.setSpacing(4);

	// Setup initial widget states
	QFont font = m_titleLabel.font();
	font.setBold(true);
	m_titleLabel.setFont(font);
	updateChildLabel(&m_titleLabel); // Add shadow

	// Setup size hints
	m_radio.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	m_iconLabel.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	m_textWidget.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	// Add initial widgets to the layouts
	m_mainLayout.addWidget(
		&m_radio, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
	m_mainLayout.addWidget(
		&m_titleLabel, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
	m_mainLayout.addWidget(
		&m_iconLabel, 0, 2, 2, 1, Qt::AlignCenter);
	m_mainLayout.addWidget(
		&m_textWidget, 1, 0, 1, 2, Qt::AlignLeft | Qt::AlignVCenter);
	m_mainLayout.setColumnStretch(0, 1);
	m_mainLayout.setColumnStretch(1, 99);
	m_mainLayout.setColumnStretch(2, 1);
	m_mainLayout.setRowStretch(0, 1);
	m_mainLayout.setRowStretch(1, 99);

	// Make sure all the info text lines up between widgets
	m_textLayout.setColumnMinimumWidth(0, 80);
	m_textLayout.setColumnStretch(0, 1);
	m_textLayout.setColumnStretch(1, 99);

	// Clicking the widget is the same as clicking the radio button
	connect(this, &InfoWidget::clicked,
		&m_radio, &QRadioButton::click);

	// We change our look based on whether or not the radio button is checked
	connect(&m_radio, &QRadioButton::toggled,
		this, &InfoWidget::radioChanged);
}

InfoWidget::~InfoWidget()
{
}

void InfoWidget::select()
{
	if(m_radio.isChecked())
		return;
	m_radio.setChecked(true);
}

void InfoWidget::setTitle(const QString &text)
{
	m_titleLabel.setText(text);
}

void InfoWidget::setIconPixmap(const QPixmap &icon)
{
	m_iconLabel.setPixmap(icon);
}

/// <summary>
/// As updating the layout is a relatively expensive operation only set
/// `doUpdateLayout` to true on the last text item that you add.
/// </summary>
void InfoWidget::setItemText(
	int id, const QString &left, const QString &right, bool doUpdateLayout)
{
	bool doUpdate = false;
	if(left.isNull() && right.isNull()) {
		// Remove items from the hash
		if(m_leftText.contains(id)) {
			QLabel *widget = m_leftText[id];
			m_textLayout.removeWidget(widget);
			delete widget;
			m_leftText.remove(id);
		}
		if(m_rightText.contains(id)) {
			QLabel *widget = m_rightText[id];
			m_textLayout.removeWidget(widget);
			delete widget;
			m_rightText.remove(id);
		}
	} else {
		// Add/modify items in the hash

		//PROFILE_BEGIN(a);
		if(m_leftText.contains(id))
			m_leftText[id]->setText(left);
		else {
			QLabel *widget = new QLabel(left, this);
			//widget->setSizePolicy(
			//	QSizePolicy::Maximum, QSizePolicy::Maximum);
			updateChildLabel(widget);
			m_leftText[id] = widget;
			doUpdate = true;

			// The left-hand text is faded out
			QPalette pal = widget->palette();
			//pal.setColor(widget->foregroundRole(), QColor(0, 0, 0, 127));
			pal.setColor(widget->foregroundRole(), QColor(127, 127, 127));
			widget->setPalette(pal);
		}
		//PROFILE_END(a, "setItemText - Left");

		//PROFILE_BEGIN(b);
		if(m_rightText.contains(id))
			m_rightText[id]->setText(right);
		else {
			QLabel *widget = new QLabel(right, this);
			//widget->setSizePolicy(
			//	QSizePolicy::Expanding, QSizePolicy::Expanding);
			updateChildLabel(widget);
			m_rightText[id] = widget;
			doUpdate = true;

			// The right-hand text can be no wider than 280px
			QFontMetrics metrics = widget->fontMetrics();
			QString newRight = metrics.elidedText(
				right, Qt::ElideMiddle, m_rightTextMaxWidth);
			widget->setText(newRight);
		}
		//PROFILE_END(b, "setItemText - Right");
	}
	if(doUpdate && doUpdateLayout)
		updateLayout();
}

QSize InfoWidget::minimumSizeHint() const
{
	QSize size = m_radio.minimumSizeHint();
	size.setHeight(90);
	return size;
}

void InfoWidget::updateChildLabel(QLabel *label)
{
	// Add a drop shadow effect to the label. The label takes ownership of the
	// effect and will delete it when the label is deleted or when we assign a
	// new effect to the label.
	const QColor shadowCol = QColor(0xFF, 0xFF, 0xFF, 0x54);
	QGraphicsDropShadowEffect *effect =
		static_cast<QGraphicsDropShadowEffect *>(label->graphicsEffect());
	if(effect == NULL) {
		effect = new QGraphicsDropShadowEffect();
		effect->setBlurRadius(1.0f);
		effect->setColor(shadowCol);
		effect->setOffset(1.0f, 1.0f);
		label->setGraphicsEffect(effect);
	} else if(effect->color() != shadowCol)
		effect->setColor(shadowCol);
}

/// <summary>
/// Fill the text string list sorted by ID.
/// </summary>
void InfoWidget::updateLayout()
{
	// Remove all items from the layout
	QMapIterator<int, QLabel *> it(m_leftText);
	while(it.hasNext()) {
		it.next();
		m_textLayout.removeWidget(it.value());
	}
	it = QMapIterator<int, QLabel *>(m_rightText);
	while(it.hasNext()) {
		it.next();
		m_textLayout.removeWidget(it.value());
	}

	// Add everything back again
	int row = 0;
	it = QMapIterator<int, QLabel *>(m_leftText);
	while(it.hasNext()) {
		it.next();
		QLabel *left = it.value();
		QLabel *right = m_rightText[it.key()];
		m_textLayout.addWidget(
			left, row, 0, 1, 1, Qt::AlignRight | Qt::AlignVCenter);
		m_textLayout.addWidget(
			right, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
		row++;
	}
}

void InfoWidget::paintEvent(QPaintEvent *ev)
{
	// Draw background frame
	QPainter p(this);
	if(m_selected) {
		StyleHelper::drawFrame(
			&p, rect(), StyleHelper::FrameSelectedColor,
			StyleHelper::FrameSelectedBorderColor, false);
	} else {
		StyleHelper::drawFrame(
			&p, rect(), StyleHelper::FrameNormalColor,
			StyleHelper::FrameNormalBorderColor, false);
	}
}

void InfoWidget::mousePressEvent(QMouseEvent *ev)
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

void InfoWidget::mouseReleaseEvent(QMouseEvent *ev)
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

void InfoWidget::mouseDoubleClickEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	emit doubleClicked();
	ev->accept();
}

void InfoWidget::focusOutEvent(QFocusEvent *ev)
{
	m_mouseDown = false;
	QWidget::focusOutEvent(ev);
}

void InfoWidget::radioChanged(bool checked)
{
	m_selected = checked;
	repaint();
}

//=============================================================================
// TargetInfoWidget class

TargetInfoWidget::TargetInfoWidget(Target *target, QWidget *parent)
	: InfoWidget(parent)
	, m_target(target)
{
	// We have a fixed height
	setFixedHeight(95); // 78 = 3 text lines, 95 = 4 text lines
}

TargetInfoWidget::~TargetInfoWidget()
{
}

//=============================================================================
// AudioInputInfoWidget class

AudioInputInfoWidget::AudioInputInfoWidget(AudioInput *input, QWidget *parent)
	: InfoWidget(parent)
	, m_input(input)
{
	// We have a fixed height
	setFixedHeight(95); // 78 = 3 text lines, 95 = 4 text lines

	// HACK: Make text fill entire width
	m_rightTextMaxWidth = 999;
}

AudioInputInfoWidget::~AudioInputInfoWidget()
{
}
