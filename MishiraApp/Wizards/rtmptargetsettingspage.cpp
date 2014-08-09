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

#include "rtmptargetsettingspage.h"
#include "common.h"
#include "validators.h"
#include "wizardwindow.h"

RTMPTargetSettingsPage::RTMPTargetSettingsPage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
{
	m_ui.setupUi(this);

	// Setup validators. Target name must start with a non-whitespace character
	// and be no longer than 24 characters total and the application URL must
	// be a valid URL that our RTMP library can parse.
	m_ui.nameEdit->setValidator(new QRegExpValidator(
		QRegExp(QStringLiteral("\\S.{,23}")), this));
	m_ui.urlEdit->setValidator(new RTMPURLValidator(this));
	connect(m_ui.nameEdit, &QLineEdit::textChanged,
		this, &RTMPTargetSettingsPage::nameEditChanged);
	connect(m_ui.urlEdit, &QLineEdit::textChanged,
		this, &RTMPTargetSettingsPage::urlEditChanged);
	connect(m_ui.hideStreamNameCheck, &QCheckBox::stateChanged,
		this, &RTMPTargetSettingsPage::hideStreamNameCheckChanged);
}

RTMPTargetSettingsPage::~RTMPTargetSettingsPage()
{
}

bool RTMPTargetSettingsPage::isValid() const
{
	if(!m_ui.nameEdit->hasAcceptableInput())
		return false;
	if(!m_ui.urlEdit->hasAcceptableInput())
		return false;
	return true;
}

/// <summary>
/// Reset button code for the page that is shared between multiple controllers.
/// </summary>
void RTMPTargetSettingsPage::sharedReset(
	WizardWindow *wizWin, WizTargetSettings *defaults)
{
	setUpdatesEnabled(false);

	// Update fields
	m_ui.nameEdit->setText(defaults->name);
	m_ui.urlEdit->setText(defaults->rtmpUrl);
	m_ui.streamNameEdit->setText(defaults->rtmpStreamName);
	m_ui.hideStreamNameCheck->setChecked(defaults->rtmpHideStreamName);
	m_ui.padVideoCheck->setChecked(defaults->rtmpPadVideo);

	// Reset validity and connect signal
	doQLineEditValidate(m_ui.nameEdit);
	doQLineEditValidate(m_ui.urlEdit);
	wizWin->setCanContinue(isValid());
	connect(this, &RTMPTargetSettingsPage::validityMaybeChanged,
		wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	setUpdatesEnabled(true);
}

/// <summary>
/// Next button code for the page that is shared between multiple controllers.
/// </summary>
void RTMPTargetSettingsPage::sharedNext(WizTargetSettings *settings)
{
	// Save fields
	settings->name = m_ui.nameEdit->text().trimmed();
	settings->rtmpUrl = m_ui.urlEdit->text();
	settings->rtmpStreamName = m_ui.streamNameEdit->text();
	settings->rtmpHideStreamName = m_ui.hideStreamNameCheck->isChecked();
	settings->rtmpPadVideo = m_ui.padVideoCheck->isChecked();
}

void RTMPTargetSettingsPage::nameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.nameEdit);
	emit validityMaybeChanged(isValid());
}

void RTMPTargetSettingsPage::urlEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.urlEdit);
	emit validityMaybeChanged(isValid());
}

void RTMPTargetSettingsPage::hideStreamNameCheckChanged(int state)
{
	if(state == Qt::Checked)
		m_ui.streamNameEdit->setEchoMode(QLineEdit::Password);
	else
		m_ui.streamNameEdit->setEchoMode(QLineEdit::Normal);
}
