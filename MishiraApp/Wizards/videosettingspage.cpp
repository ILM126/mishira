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

#include "videosettingspage.h"
#include "application.h"
#include "validators.h"
#include <QtWidgets/QPushButton>

VideoSettingsPage::VideoSettingsPage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
	, m_btnGroup(this)
{
	m_ui.setupUi(this);

	// Setup custom widgets
	m_ui.widthEdit->setUnits(tr("px"));
	m_ui.heightEdit->setUnits(tr("px"));
	m_ui.bitrateEdit->setUnits(tr("Kb/s"));
	m_ui.keyIntervalEdit->setUnits(tr("secs"));

	// Add radio buttons to their appropriate button group
	m_btnGroup.addButton(m_ui.shareRadio);
	m_btnGroup.addButton(m_ui.newRadio);
	void (QButtonGroup:: *fppCB)(QAbstractButton *) =
		&QButtonGroup::buttonClicked;
	connect(&m_btnGroup, fppCB,
		this, &VideoSettingsPage::shareRadioClicked);

	// We populate the "share with" combobox in the controller

	// Populate scaling combobox
	m_ui.scalingCombo->clear();
	for(int i = 0; i < NUM_SCALER_SCALING_MODES; i++)
		m_ui.scalingCombo->addItem(tr(SclrScalingModeStrings[i]));

	// Populate scaling quality combobox
	m_ui.scalingQualityCombo->clear();
	for(int i = 0; i < NUM_STANDARD_TEXTURE_FILTERS; i++)
		m_ui.scalingQualityCombo->addItem(tr(GfxFilterQualityStrings[i]));
	//m_ui.scalingQualityCombo->setEnabled(false); // Always bilinear for now

	// Populate encoder combobox
	m_ui.encoderCombo->clear();
	for(int i = 0; i < NUM_VIDEO_ENCODER_TYPES; i++)
		m_ui.encoderCombo->addItem(tr(VencTypeStrings[i]));
	m_ui.encoderCombo->setEnabled(false); // Always x264 for now

	// Populate CPU usage combobox
	m_ui.cpuUsageCombo->clear();
	for(int i = 0; i < NUM_X264_PRESETS; i++)
		m_ui.cpuUsageCombo->addItem(tr(X264PresetStrings[i]));

	// Setup validators
	m_ui.widthEdit->setValidator(new EvenIntValidator(32, 4096, this));
	m_ui.heightEdit->setValidator(new EvenIntValidator(32, 4096, this));
	m_ui.bitrateEdit->setValidator(
		new QIntValidator(100, 2000000, this)); // x264 maximum = 2 GB/s (!!)
	m_ui.keyIntervalEdit->setValidator(new IntOrEmptyValidator(0, 60, this));
	connect(m_ui.widthEdit, &QLineEdit::textChanged,
		this, &VideoSettingsPage::widthEditChanged);
	connect(m_ui.heightEdit, &QLineEdit::textChanged,
		this, &VideoSettingsPage::heightEditChanged);
	connect(m_ui.bitrateEdit, &QLineEdit::textChanged,
		this, &VideoSettingsPage::bitrateEditChanged);
	connect(m_ui.keyIntervalEdit, &QLineEdit::textChanged,
		this, &VideoSettingsPage::keyIntervalEditChanged);

	// If the user enters "0" for the keyframe interval then automatically
	// display "default" instead by clearing the field
	m_ui.keyIntervalEdit->setPlaceholderText(
		tr("Default (%L1)").arg(DEFAULT_KEYFRAME_INTERVAL));
	connect(m_ui.keyIntervalEdit, &UnitLineEdit::editingFinished,
		this, &VideoSettingsPage::keyIntervalEditFinished);

	// Connect push buttons
	connect(m_ui.bitrateCalcBtn, &QPushButton::clicked,
		App, &Application::showBitrateCalculator);
}

VideoSettingsPage::~VideoSettingsPage()
{
}

bool VideoSettingsPage::isValid() const
{
	if(!m_ui.widthEdit->hasAcceptableInput())
		return false;
	if(!m_ui.heightEdit->hasAcceptableInput())
		return false;
	if(!m_ui.bitrateEdit->hasAcceptableInput())
		return false;
	if(!m_ui.keyIntervalEdit->hasAcceptableInput())
		return false;
	return true;
}

void VideoSettingsPage::updateEnabled()
{
	// Which radio button is checked?
	bool inShare = false;
	if(m_ui.shareRadio->isChecked())
		inShare = true;

	// Share area
	m_ui.shareCombo->setEnabled(inShare);

	// New area
	m_ui.widthEdit->setEnabled(!inShare);
	m_ui.heightEdit->setEnabled(!inShare);
	m_ui.scalingCombo->setEnabled(!inShare);
	m_ui.scalingQualityCombo->setEnabled(!inShare);
	//m_ui.encoderCombo->setEnabled(!inShare);
	m_ui.bitrateEdit->setEnabled(!inShare);
	m_ui.cpuUsageCombo->setEnabled(!inShare);
	m_ui.keyIntervalEdit->setEnabled(!inShare);
}

void VideoSettingsPage::widthEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.widthEdit);
	emit validityMaybeChanged(isValid());
}

void VideoSettingsPage::heightEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.heightEdit);
	emit validityMaybeChanged(isValid());
}

void VideoSettingsPage::bitrateEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.bitrateEdit);
	emit validityMaybeChanged(isValid());
}

void VideoSettingsPage::keyIntervalEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.keyIntervalEdit);
	emit validityMaybeChanged(isValid());
}

void VideoSettingsPage::keyIntervalEditFinished()
{
	if(!m_ui.keyIntervalEdit->hasAcceptableInput())
		return;
	if(m_ui.keyIntervalEdit->text().toInt() == 0)
		m_ui.keyIntervalEdit->setText(QString());
}

void VideoSettingsPage::shareRadioClicked(QAbstractButton *button)
{
	updateEnabled();
}
