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

#include "targetpane.h"
#include "application.h"
#include "target.h"
#include "Widgets/borderspacer.h"
#include "Widgets/patternwidgets.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsDropShadowEffect>
#include <QtWidgets/QGraphicsOpacityEffect>

TargetPane::TargetPane(Target *target, QWidget *parent)
	: QWidget(parent)
	, SortableWidget(0)
	, m_target(target)
	, m_fgColor()
	, m_shadowColor()
	, m_isUserEnabled(false)
	, m_outerLayout(this)
	, m_liveBarLeft(this)
	, m_liveBarRight(this)
	, m_innerWidget(this)
	, m_innerLayout(&m_innerWidget)
	, m_userEnabledCheckBox(&m_innerWidget)
	, m_titleLabel(&m_innerWidget)
	, m_iconLabel(&m_innerWidget)
	, m_leftText()
	, m_rightText()
	, m_textWidget(&m_innerWidget)
	, m_textLayout(&m_textWidget)
	, m_mouseDown(false)
{
	// Setup layout margins
	m_outerLayout.setMargin(0);
	m_outerLayout.setSpacing(0);
	m_innerLayout.setContentsMargins(5, 3, 5, 2);
	m_innerLayout.setSpacing(0);
	m_textLayout.setContentsMargins(1, 2, 1, 3);
	m_textLayout.setSpacing(1);

	// Setup live bar indicators
	m_liveBarLeft.setFixedWidth(5);
	m_liveBarLeft.setAutoFillBackground(true);
	m_liveBarRight.setFixedWidth(5);
	m_liveBarRight.setAutoFillBackground(true);

	// Apply dark widget style
	App->applyDarkStyle(&m_userEnabledCheckBox);

	// Setup initial widget states
	QFont font = m_titleLabel.font();
	font.setBold(true);
	m_titleLabel.setFont(font);
	setPaneState(OfflineState); // Offline by default
	m_textWidget.setVisible(false);

	// Create header pattern
	HeaderPattern *pattern = new HeaderPattern(this);
	pattern->setFixedHeight(10);

	// Setup size hints
	m_iconLabel.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	pattern->setSizePolicy(
		QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
	m_textLayout.setColumnStretch(0, 4);
	m_textLayout.setColumnStretch(1, 3);

	// Setup inner layout
	m_innerLayout.addWidget(&m_userEnabledCheckBox, 0, 0);
	m_innerLayout.addItem(
		new QSpacerItem(6, 0, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 1);
	m_innerLayout.addWidget(&m_iconLabel, 0, 2);
	m_innerLayout.addItem(
		new QSpacerItem(4, 0, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 3);
	m_innerLayout.addWidget(&m_titleLabel, 0, 4);
	m_innerLayout.addItem(
		new QSpacerItem(7, 0, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 5);
	m_innerLayout.addWidget(pattern, 0, 6);
	m_innerLayout.addItem(
		new QSpacerItem(2, 0, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 7);
	m_innerLayout.addItem(
		new QSpacerItem(0, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 1, 0);
	m_innerLayout.addWidget(&m_textWidget, 2, 0, 1, 8);

	// Setup outer layout
	m_outerLayout.addWidget(&m_liveBarLeft, 0, 0);
	m_outerLayout.addWidget(new HBorderSpacer(this), 0, 1);
	m_outerLayout.addWidget(&m_innerWidget, 0, 2);
	m_outerLayout.addWidget(new HBorderSpacer(this), 0, 3);
	m_outerLayout.addWidget(&m_liveBarRight, 0, 4);
	m_outerLayout.addWidget(new VBorderSpacer(this), 1, 0, 1, 5);

	// Determine text colour
	updateForeground();

	// Connect signals
	connect(&m_userEnabledCheckBox, &QCheckBox::clicked,
		this, &TargetPane::activeCheckBoxClicked);
	connect(m_target, &Target::enabledChanged,
		this, &TargetPane::targetEnabledChanged);
}

TargetPane::~TargetPane()
{
	// Delete all child labels
	QMapIterator<int, QLabel *> it(m_leftText);
	while(it.hasNext()) {
		it.next();
		delete it.value();
	}
	it = QMapIterator<int, QLabel *>(m_rightText);
	while(it.hasNext()) {
		it.next();
		delete it.value();
	}
}

void TargetPane::setUserEnabled(bool enable)
{
	// Do check box first so `Target::resetTargetPaneEnabled()` works
	m_userEnabledCheckBox.setChecked(enable);
	if(enable) {
		m_userEnabledCheckBox.setToolTip(tr(
			"Target is enabled"));
	} else {
		m_userEnabledCheckBox.setToolTip(tr(
			"Target is disabled"));
	}

	if(m_isUserEnabled == enable)
		return; // Nothing to do
	m_isUserEnabled = enable;

	// Prevent ugly repaints
	parentWidget()->setUpdatesEnabled(false);

	// Update UI
	updateForeground();
	m_textWidget.setVisible(m_isUserEnabled);

	// HACK: The `setUpdatesEnabled()` call must be delayed in order to
	// actually do anything
	parentWidget()->layout()->update();
	QTimer::singleShot(10, this, SLOT(enableUpdatesTimeout()));
}

void TargetPane::enableUpdatesTimeout()
{
	parentWidget()->setUpdatesEnabled(true);
}

void TargetPane::setPaneState(PaneState state)
{
	QPalette pal = m_liveBarLeft.palette();
	switch(state) {
	default:
	case OfflineState:
		pal.setColor(
			m_liveBarLeft.backgroundRole(), StyleHelper::GreenBaseColor);
		m_liveBarLeft.setToolTip(tr("Target is in standby mode"));
		m_liveBarRight.setToolTip(tr("Target is in standby mode"));
		break;
	case LiveState:
		pal.setColor(
			m_liveBarLeft.backgroundRole(), StyleHelper::RedBaseColor);
		m_liveBarLeft.setToolTip(tr("Broadcasting to target"));
		m_liveBarRight.setToolTip(tr("Broadcasting to target"));
		break;
	case WarningState:
		pal.setColor(
			m_liveBarLeft.backgroundRole(), StyleHelper::OrangeBaseColor);
		m_liveBarLeft.setToolTip(tr("Target is attempting to broadcast"));
		m_liveBarRight.setToolTip(tr("Target is attempting to broadcast"));
		break;
	}
	m_liveBarLeft.setPalette(pal);
	m_liveBarRight.setPalette(pal);
}

void TargetPane::setTitle(const QString &text)
{
	m_titleLabel.setText(text);
}

void TargetPane::setIconPixmap(const QPixmap &icon)
{
	m_iconLabel.setPixmap(icon);
}

/// <summary>
/// As updating the layout is a relatively expensive operation only set
/// `doUpdateLayout` to true on the last text item that you add.
/// </summary>
void TargetPane::setItemText(
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
		if(m_leftText.contains(id))
			m_leftText[id]->setText(left);
		else {
			QLabel *widget = new QLabel(left, this);
			//widget->setSizePolicy(
			//	QSizePolicy::Expanding, QSizePolicy::Expanding);
			updateChildLabel(widget, true);
			m_leftText[id] = widget;
			doUpdate = true;
		}
		if(m_rightText.contains(id))
			m_rightText[id]->setText(right);
		else {
			QLabel *widget = new QLabel(right, this);
			//widget->setSizePolicy(
			//	QSizePolicy::Expanding, QSizePolicy::Expanding);
			widget->setIndent(4);
			updateChildLabel(widget, false);
			m_rightText[id] = widget;
			doUpdate = true;
		}
	}
	if(doUpdate && doUpdateLayout)
		updateLayout();
}

void TargetPane::updateForeground()
{
	m_fgColor = StyleHelper::DarkFg2Color;
	if(!m_isUserEnabled)
		m_fgColor.setAlpha(102); // 40%
	m_shadowColor = StyleHelper::TextShadowColor;

	// Update icon opacity
	if(!m_isUserEnabled) {
		if(m_iconLabel.graphicsEffect() == NULL) {
			QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect();
			effect->setOpacity(0.4f); // 40%
			m_iconLabel.setGraphicsEffect(effect);
		}
	} else
		m_iconLabel.setGraphicsEffect(NULL);

	// Update all child labels
	updateChildLabel(&m_titleLabel, false);
	QMapIterator<int, QLabel *> it(m_leftText);
	while(it.hasNext()) {
		it.next();
		updateChildLabel(it.value(), true);
	}
	it = QMapIterator<int, QLabel *>(m_rightText);
	while(it.hasNext()) {
		it.next();
		updateChildLabel(it.value(), false);
	}
}

void TargetPane::updateChildLabel(QLabel *label, bool isLeft)
{
	// Determine colours
	QColor textColor = m_fgColor;
	QColor shadowColor = m_shadowColor;
	if(isLeft) {
		// Labels on the left are half brightness. HACK: Shadow isn't half
		// brightness as it's too faint (Photoshop does shadows differently)
		textColor.setAlpha(textColor.alpha() / 2);
		shadowColor.setAlpha(shadowColor.alpha() * 2 / 3);
	}

	// Set the foreground text colour
	QPalette pal = label->palette();
	if(pal.color(label->foregroundRole()) != textColor) {
		pal.setColor(label->foregroundRole(), textColor);
		label->setPalette(pal);
	}

	// Add a drop shadow effect to the label. The label takes ownership of the
	// effect and will delete it when the label is deleted or when we assign a
	// new effect to the label.
	QGraphicsDropShadowEffect *effect =
		static_cast<QGraphicsDropShadowEffect *>(label->graphicsEffect());
	if(effect == NULL) {
		effect = new QGraphicsDropShadowEffect();
		effect->setBlurRadius(1.0f);
		effect->setColor(shadowColor);
		effect->setOffset(1.0f, 1.0f);
		label->setGraphicsEffect(effect);
	} else if(effect->color() != shadowColor)
		effect->setColor(shadowColor);
}

/// <summary>
/// Fill the text string list sorted by ID.
/// </summary>
void TargetPane::updateLayout()
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

QWidget *TargetPane::getWidget()
{
	return static_cast<QWidget *>(this);
}

void TargetPane::paintEvent(QPaintEvent *ev)
{
	QColor bgColor = m_isUserEnabled ?
		StyleHelper::DarkBg5Color : StyleHelper::DarkBg4Color;
	if(bgColor.alpha() > 0) {
		QPainter p(this);
		p.fillRect(QRect(0, 0, width(), height()), bgColor);
	}
}

void TargetPane::mousePressEvent(QMouseEvent *ev)
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

void TargetPane::mouseReleaseEvent(QMouseEvent *ev)
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
		emit clicked(m_target);
	else
		emit rightClicked(m_target);
	ev->accept();
}

void TargetPane::mouseDoubleClickEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	emit doubleClicked(m_target);
	ev->accept();
}

void TargetPane::focusOutEvent(QFocusEvent *ev)
{
	m_mouseDown = false;
	QWidget::focusOutEvent(ev);
}

void TargetPane::activeCheckBoxClicked()
{
	bool enable = (m_userEnabledCheckBox.checkState() == Qt::Checked);
	if(App->isBroadcasting() && m_target->isActive() != enable) {
		// Show confirmation dialog before changing state
		App->showStartStopBroadcastDialog(enable, m_target);
	} else {
		// No need for confirmation dialog if not broadcasting or if the active
		// state will not change
		setUserEnabled(enable);
		if(m_target != NULL)
			m_target->setEnabled(enable);
	}
}

void TargetPane::targetEnabledChanged(Target *target, bool enabled)
{
	setUserEnabled(enabled);
}
