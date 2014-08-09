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

#include "ustreamtargetsettingspage.h"
#include "common.h"
#include "validators.h"
#include "wizardwindow.h"

UstreamTargetSettingsPage::UstreamTargetSettingsPage(QWidget *parent)
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
	m_ui.streamKeyEdit->setValidator(new QRegExpValidator(
		QRegExp(QStringLiteral("\\S+")), this));
	connect(m_ui.nameEdit, &QLineEdit::textChanged,
		this, &UstreamTargetSettingsPage::nameEditChanged);
	connect(m_ui.urlEdit, &QLineEdit::textChanged,
		this, &UstreamTargetSettingsPage::urlEditChanged);
	connect(m_ui.streamKeyEdit, &QLineEdit::textChanged,
		this, &UstreamTargetSettingsPage::streamKeyEditChanged);

	m_ui.streamKeyEdit->setEchoMode(QLineEdit::Password);
}

UstreamTargetSettingsPage::~UstreamTargetSettingsPage()
{
}

bool UstreamTargetSettingsPage::isValid() const
{
	if(!m_ui.nameEdit->hasAcceptableInput())
		return false;
	if(!m_ui.urlEdit->hasAcceptableInput())
		return false;
	if(!m_ui.streamKeyEdit->hasAcceptableInput())
		return false;
	return true;
}

/// <summary>
/// Reset button code for the page that is shared between multiple controllers.
/// </summary>
void UstreamTargetSettingsPage::sharedReset(
	WizardWindow *wizWin, WizTargetSettings *defaults)
{
	setUpdatesEnabled(false);

	// Update fields
	m_ui.nameEdit->setText(defaults->name);
	m_ui.urlEdit->setText(defaults->rtmpUrl);
	m_ui.streamKeyEdit->setText(defaults->rtmpStreamName);
	m_ui.padVideoCheck->setChecked(defaults->rtmpPadVideo);

	// Reset validity and connect signal
	doQLineEditValidate(m_ui.nameEdit);
	doQLineEditValidate(m_ui.urlEdit);
	doQLineEditValidate(m_ui.streamKeyEdit);
	wizWin->setCanContinue(isValid());
	connect(this, &UstreamTargetSettingsPage::validityMaybeChanged,
		wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	setUpdatesEnabled(true);
}

/// <summary>
/// Next button code for the page that is shared between multiple controllers.
/// </summary>
void UstreamTargetSettingsPage::sharedNext(WizTargetSettings *settings)
{
	// Save fields
	settings->name = m_ui.nameEdit->text().trimmed();
	settings->rtmpUrl = m_ui.urlEdit->text();
	settings->rtmpStreamName = m_ui.streamKeyEdit->text();
	settings->rtmpPadVideo = m_ui.padVideoCheck->isChecked();
}

void UstreamTargetSettingsPage::nameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.nameEdit);
	emit validityMaybeChanged(isValid());
}

void UstreamTargetSettingsPage::urlEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.urlEdit);
	emit validityMaybeChanged(isValid());
}

void UstreamTargetSettingsPage::streamKeyEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.streamKeyEdit);
	emit validityMaybeChanged(isValid());
}
