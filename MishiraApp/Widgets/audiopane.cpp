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

#include "audiopane.h"
#include "application.h"
#include "audioinput.h"
#include "audiomixer.h"
#include "profile.h"
#include "Widgets/borderspacer.h"
#include "Widgets/patternwidgets.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QGraphicsDropShadowEffect>

AudioPane::AudioPane(AudioInput *input, QWidget *parent)
	: QWidget(parent)
	, SortableWidget(0)
	, m_input(input)
	, m_fgColor()
	, m_shadowColor()
	, m_outerLayout(this)
	, m_mutedBtn(StyleHelper::RedSet, true, StyleHelper::GreenSet, true, this)
	, m_slider(this)
	, m_headerWidget(this)
	, m_headerLayout(&m_headerWidget)
	, m_titleLabel(&m_headerWidget)
	, m_mouseDown(false)
{
	AudioMixer *mixer = App->getProfile()->getAudioMixer();

	// Setup layout margins
	m_outerLayout.setMargin(0);
	m_outerLayout.setSpacing(0);
	m_headerLayout.setContentsMargins(5, 5, 6, 0);
	m_headerLayout.setSpacing(0);

	// Setup muted button
	QIcon icon(":/Resources/audio-enabled.png");
	icon.addFile(
		":/Resources/audio-disabled.png", QSize(19, 12),
		QIcon::Normal, QIcon::On);
	m_mutedBtn.setIcon(icon);
	m_mutedBtn.setIconSize(QSize(19, 12));
	m_mutedBtn.setFixedSize(37, 18);

	// Setup initial widget states
	QFont font = m_titleLabel.font();
	font.setBold(true);
	m_titleLabel.setFont(font);
	if(m_input == NULL) {
		m_titleLabel.setText(tr("Master volume"));
		m_slider.setAttenuation(mixer->getMasterAttenuation());
		m_mutedBtn.setVisible(false);
	} else {
		m_titleLabel.setText(m_input->getName());
		m_slider.setAttenuation(m_input->getAttenuation());
		m_mutedBtn.setChecked(m_input->isMuted());
	}
	m_slider.setToolTip(tr("Volume, VU and peak meter"));

	// Create header pattern
	HeaderPattern *pattern = new HeaderPattern(this);
	pattern->setFixedHeight(10);

	// Setup size hints
	pattern->setSizePolicy(
		QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);

	// Setup header layout
	m_headerLayout.addWidget(&m_titleLabel, 0, 0);
	m_headerLayout.addItem(
		new QSpacerItem(7, 0, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 1);
	m_headerLayout.addWidget(pattern, 0, 2);

	// Setup outer layout
	if(m_input == NULL) {
		// Master volume has no muted button
		m_outerLayout.addWidget(&m_headerWidget, 0, 0);
		m_outerLayout.addWidget(
			new VBorderSpacer(this, 1, Qt::transparent), 1, 0);
		m_outerLayout.addWidget(&m_slider, 2, 0);
		m_outerLayout.addWidget(new VBorderSpacer(this), 3, 0);
	} else {
		m_outerLayout.addWidget(&m_headerWidget, 0, 0, 2, 1);
		m_outerLayout.addWidget(new HBorderSpacer(this), 0, 1, 2, 1);
		m_outerLayout.addWidget(&m_mutedBtn, 0, 2);
		m_outerLayout.addWidget(new VBorderSpacer(this), 1, 1, 1, 2);
		m_outerLayout.addWidget(&m_slider, 2, 0, 1, 3);
		m_outerLayout.addWidget(new VBorderSpacer(this), 3, 0, 1, 3);
	}

	// Determine text colour and muted button state
	updateForeground();
	updateMuted();

	// Connect signals
	if(m_input == NULL) {
		connect(&m_slider, &VolumeSlider::attenuationChanged,
			mixer, &AudioMixer::setMasterAttenuation);
	} else {
		connect(&m_mutedBtn, &StyledToggleButton::clicked,
			m_input, &AudioInput::toggleMuted);
		connect(m_input->getMixer(), &AudioMixer::inputChanged,
			this, &AudioPane::inputChanged);
		connect(&m_slider, &VolumeSlider::attenuationChanged,
			m_input, &AudioInput::setAttenuation);
	}

	// Update the volume levels every tick
	connect(App, &Application::realTimeFrameEvent,
		this, &AudioPane::realTimeFrameEvent);
}

AudioPane::~AudioPane()
{
}

void AudioPane::updateForeground()
{
	m_fgColor = StyleHelper::DarkFg2Color;
	if(m_input != NULL && !m_input->isActive()) {
		m_fgColor.setAlpha(102); // 40%
		m_titleLabel.setToolTip(tr("Device is unavailable"));
	} else
		m_titleLabel.setToolTip(QString());
	m_shadowColor = StyleHelper::TextShadowColor;

	// Update all child labels
	updateChildLabel(&m_titleLabel);
}

void AudioPane::updateChildLabel(QLabel *label)
{
	// Determine colours
	QColor textColor = m_fgColor;
	QColor shadowColor = m_shadowColor;

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

void AudioPane::updateMuted()
{
	if(m_input == NULL)
		return; // Master volume has no mute button
	if(m_input->isMuted())
		m_mutedBtn.setToolTip(tr("Device is muted"));
	else
		m_mutedBtn.setToolTip(tr("Device is not muted"));
}

QWidget *AudioPane::getWidget()
{
	return static_cast<QWidget *>(this);
}

void AudioPane::paintEvent(QPaintEvent *ev)
{
	QColor bgColor = (m_input == NULL) ?
		StyleHelper::DarkBg5Color : StyleHelper::DarkBg4Color;
	if(bgColor.alpha() > 0) {
		QPainter p(this);
		p.fillRect(QRect(0, 0, width(), height()), bgColor);
	}
}

void AudioPane::mousePressEvent(QMouseEvent *ev)
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

void AudioPane::mouseReleaseEvent(QMouseEvent *ev)
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
		emit clicked(m_input);
	else
		emit rightClicked(m_input);
	ev->accept();
}

void AudioPane::mouseDoubleClickEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	emit doubleClicked(m_input);
	ev->accept();
}

void AudioPane::focusOutEvent(QFocusEvent *ev)
{
	m_mouseDown = false;
	QWidget::focusOutEvent(ev);
}

void AudioPane::realTimeFrameEvent(int numDropped, int lateByUsec)
{
	// Update the volume levels every tick
	if(m_input == NULL) {
		AudioMixer *mixer = App->getProfile()->getAudioMixer();
		m_slider.setVolume(
			mixer->getOutputRmsVolume(), mixer->getOutputPeakVolume());
	} else {
		m_slider.setIsMuted(m_input->isMuted());
		m_slider.setVolume(
			m_input->getOutputRmsVolume(), m_input->getOutputPeakVolume());
	}
}

void AudioPane::inputChanged(AudioInput *input)
{
	if(input != m_input)
		return; // Not our input
	m_titleLabel.setText(m_input->getName());
	if(m_input == NULL) {
		AudioMixer *mixer = App->getProfile()->getAudioMixer();
		m_slider.setAttenuation(mixer->getMasterAttenuation());
	} else
		m_slider.setAttenuation(m_input->getAttenuation());
	updateForeground();
	updateMuted();
}
