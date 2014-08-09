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

#include "editprofilepage.h"
#include "application.h"
#include "validators.h"
#include <QtWidgets/QPushButton>

EditProfilePage::EditProfilePage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
{
	m_ui.setupUi(this);

	// Setup custom widgets
	m_ui.widthEdit->setUnits(tr("px"));
	m_ui.heightEdit->setUnits(tr("px"));

	// Populate framerate combobox
	m_ui.fpsCombo->clear();
	for(int i = 0; !PrflFpsRates[i].isZero(); i++)
		m_ui.fpsCombo->addItem(tr(PrflFpsRatesStrings[i]));

	// Populate audio mode combobox
	m_ui.audioCombo->clear();
	for(int i = 0; i < NUM_AUDIO_MODES; i++)
		m_ui.audioCombo->addItem(tr(PrflAudioModeStrings[i]));

	// Setup validators
	m_ui.nameEdit->setValidator(new ProfileNameValidator(this));
	m_ui.widthEdit->setValidator(new EvenIntValidator(32, INT_MAX, this));
	m_ui.heightEdit->setValidator(new EvenIntValidator(32, INT_MAX, this));
	connect(m_ui.nameEdit, &QLineEdit::textChanged,
		this, &EditProfilePage::nameEditChanged);
	connect(m_ui.widthEdit, &QLineEdit::textChanged,
		this, &EditProfilePage::widthEditChanged);
	connect(m_ui.heightEdit, &QLineEdit::textChanged,
		this, &EditProfilePage::heightEditChanged);

	// Connect push buttons
	connect(m_ui.bitrateCalcBtn, &QPushButton::clicked,
		App, &Application::showBitrateCalculator);
}

EditProfilePage::~EditProfilePage()
{
}

bool EditProfilePage::isValid() const
{
	if(!m_ui.nameEdit->hasAcceptableInput())
		return false;
	if(!m_ui.widthEdit->hasAcceptableInput())
		return false;
	if(!m_ui.heightEdit->hasAcceptableInput())
		return false;
	return true;
}

void EditProfilePage::nameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.nameEdit);
	emit validityMaybeChanged(isValid());
}

void EditProfilePage::widthEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.widthEdit);
	emit validityMaybeChanged(isValid());
}

void EditProfilePage::heightEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.heightEdit);
	emit validityMaybeChanged(isValid());
}
