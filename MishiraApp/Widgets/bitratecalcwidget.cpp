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

#include "bitratecalcwidget.h"
#include "application.h"
#include "profile.h"
#include "recommendations.h"
#include <QtWidgets/QCheckBox>

BitrateCalcWidget::BitrateCalcWidget(QWidget *parent)
	: QWidget(parent)
	, m_ui()
	, m_btnGroup(this)
	, m_unitMultiplier(1.0f)

	, m_recVideoBitrate(0)
	, m_recAudioBitrate(0)
	, m_recVideoSize(0, 0)
{
	m_ui.setupUi(this);

	// Add radio buttons to their appropriate button group
	m_btnGroup.addButton(m_ui.highActionRadio);
	m_btnGroup.addButton(m_ui.lowActionRadio);
	void (QButtonGroup:: *fppCB)(QAbstractButton *) =
		&QButtonGroup::buttonClicked;
	connect(&m_btnGroup, fppCB,
		this, &BitrateCalcWidget::actionRadioClicked);

	// Setup validators
	m_ui.uploadEdit->setValidator(new QDoubleValidator(this));
	connect(m_ui.uploadEdit, &QLineEdit::textChanged,
		this, &BitrateCalcWidget::uploadEditChanged);
	doQLineEditValidate(m_ui.uploadEdit);

	// Select "Mb/s" by default
	m_ui.uploadUnitsCombo->setCurrentIndex(1);
	unitsComboChanged(1); // Calls `recalculate()`

	// Connect other widget signals
	void (QComboBox:: *fpiCB)(int) = &QComboBox::currentIndexChanged;
	connect(m_ui.uploadUnitsCombo, fpiCB,
		this, &BitrateCalcWidget::unitsComboChanged);
	connect(m_ui.gamerBox, &QCheckBox::stateChanged,
		this, &BitrateCalcWidget::advancedBoxChanged);
	connect(m_ui.transcodingBox, &QCheckBox::stateChanged,
		this, &BitrateCalcWidget::advancedBoxChanged);
}

BitrateCalcWidget::~BitrateCalcWidget()
{
}

void BitrateCalcWidget::recalculate()
{
	Fraction framerate = App->getProfile()->getVideoFramerate();
	bool isHighAction = m_ui.highActionRadio->isChecked();
	bool isOnlineGamer = m_ui.gamerBox->isChecked();
	bool hasTranscoder = m_ui.transcodingBox->isChecked();

	// Calculate maximum safe video and audio bitrates from upload speed
	float uploadBitsPerSec =
		m_ui.uploadEdit->text().toFloat() * m_unitMultiplier;
	Recommendations::maxVidAudBitratesFromTotalUpload(
		uploadBitsPerSec, isOnlineGamer, m_recVideoBitrate, m_recAudioBitrate);

	// Cap video bitrate to a sane amount
	m_recVideoBitrate = Recommendations::capVidBitrateToSaneValue(
		m_recVideoBitrate, framerate, isHighAction, hasTranscoder);

	// Calculate recommended video size from bitrate
	m_recVideoSize = Recommendations::bestVidSizeForBitrate(
		m_recVideoBitrate, framerate, isHighAction);

	// Update the UI
	m_ui.recVidBitrateResult->setText(
		QStringLiteral("%L1 Kb/s").arg(m_recVideoBitrate));
	m_ui.recAudBitrateResult->setText(
		QStringLiteral("%L1 Kb/s").arg(m_recAudioBitrate));
	m_ui.recWidthResult->setText(
		QStringLiteral("%L1 px").arg(m_recVideoSize.width()));
	m_ui.recHeightResult->setText(
		QStringLiteral("%L1 px").arg(m_recVideoSize.height()));
}

void BitrateCalcWidget::uploadEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.uploadEdit);
	recalculate();
}

void BitrateCalcWidget::unitsComboChanged(int index)
{
	switch(index) {
	default:
	case 0: // Kb/s
		m_unitMultiplier = 1.0f;
		break;
	case 1: // Mb/s
		m_unitMultiplier = 1000.0f;
		break;
	case 2: // KB/s
		m_unitMultiplier = 1024.0f * 8.0f / 1000.0f;
		break;
	case 3: // MB/s
		m_unitMultiplier = 1024.0f * 1024.0f * 8.0f / 1000.0f;
		break;
	}
	recalculate();
}

void BitrateCalcWidget::advancedBoxChanged(int state)
{
	recalculate();
}

void BitrateCalcWidget::actionRadioClicked(QAbstractButton *button)
{
	recalculate();
}
