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

#include "audiosettingspage.h"
#include "common.h"

AudioSettingsPage::AudioSettingsPage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
	, m_btnGroup(this)
{
	m_ui.setupUi(this);

	// Add radio buttons to their appropriate button group
	m_btnGroup.addButton(m_ui.noAudioRadio);
	m_btnGroup.addButton(m_ui.shareRadio);
	m_btnGroup.addButton(m_ui.newRadio);
	void (QButtonGroup:: *fppCB)(QAbstractButton *) =
		&QButtonGroup::buttonClicked;
	connect(&m_btnGroup, fppCB,
		this, &AudioSettingsPage::shareRadioClicked);

	// We populate the "share with" combobox in the controller

	// Populate encoder combobox
	m_ui.encoderCombo->clear();
	for(int i = 0; i < NUM_AUDIO_ENCODER_TYPES; i++)
		m_ui.encoderCombo->addItem(tr(AencTypeStrings[i]));
	m_ui.encoderCombo->setEnabled(false); // Always AAC-LC for now

	// Populate bitrate combobox
	m_ui.bitrateCombo->clear();
	for(int i = 0; FdkAacBitrates[i].bitrate != 0; i++)
		m_ui.bitrateCombo->addItem(tr(FdkAacBitratesStrings[i]));
}

AudioSettingsPage::~AudioSettingsPage()
{
}

bool AudioSettingsPage::isValid() const
{
	return true;
}

void AudioSettingsPage::updateEnabled()
{
	// Which radio button is checked?
	bool inNoAudio = false;
	bool inShare = false;
	bool inNew = false;
	if(m_ui.noAudioRadio->isChecked())
		inNoAudio = true;
	if(m_ui.shareRadio->isChecked())
		inShare = true;
	if(m_ui.newRadio->isChecked())
		inNew = true;

	// Share area
	m_ui.shareCombo->setEnabled(inShare);

	// New area
	//m_ui.encoderCombo->setEnabled(inNew);
	m_ui.bitrateCombo->setEnabled(inNew);
}

void AudioSettingsPage::shareRadioClicked(QAbstractButton *button)
{
	updateEnabled();
}
