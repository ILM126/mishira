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

#include "hitboxtargetsettingspage.h"
#include "common.h"
#include "hitboxtarget.h"
#include "validators.h"
#include "wizardwindow.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QTableWidget>

HitboxTargetSettingsPage::HitboxTargetSettingsPage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
	, m_isInitialized(false)
{
	m_ui.setupUi(this);

	// Setup validators. Target name must start with a non-whitespace character
	// and be no longer than 24 characters total and the application URL must
	// be a valid URL that our RTMP library can parse.
	m_ui.nameEdit->setValidator(new QRegExpValidator(
		QRegExp(QStringLiteral("\\S.{,23}")), this));
	m_ui.usernameEdit->setValidator(new QRegExpValidator(
		QRegExp(QStringLiteral("\\S+")), this));
	m_ui.streamKeyEdit->setValidator(new QRegExpValidator(
		QRegExp(QStringLiteral("\\S+")), this));
	connect(m_ui.nameEdit, &QLineEdit::textChanged,
		this, &HitboxTargetSettingsPage::nameEditChanged);
	connect(m_ui.usernameEdit, &QLineEdit::textChanged,
		this, &HitboxTargetSettingsPage::usernameEditChanged);
	connect(m_ui.serverList, &QTableWidget::itemSelectionChanged,
		this, &HitboxTargetSettingsPage::serverSelectionChanged);
	connect(m_ui.streamKeyEdit, &QLineEdit::textChanged,
		this, &HitboxTargetSettingsPage::streamKeyEditChanged);

	m_ui.streamKeyEdit->setEchoMode(QLineEdit::Password);

	// Populate server list and create ping widgets
	QVector<QString> names = HitboxTarget::getIngestListNames();
	QVector<QString> urls = HitboxTarget::getIngestListURLs();
	for(int i = 0; i < urls.size(); i++) {
		QTableWidgetItem *item = new QTableWidgetItem(names.at(i));
		item->setData(Qt::UserRole, urls.at(i));
		m_ui.serverList->insertRow(i);
		m_ui.serverList->setItem(i, 0, item);
	}

	// Change server list header text alignment
	QTableWidgetItem *item = m_ui.serverList->horizontalHeaderItem(0);
	item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);

	// Detect when the user wants to get their stream key. We need to insert
	// the user's username into the URL to get to the Hitbox dashboard.
	connect(m_ui.dashLbl, &QLabel::linkActivated,
		this, &HitboxTargetSettingsPage::dashboardClicked);
}

HitboxTargetSettingsPage::~HitboxTargetSettingsPage()
{
}

/// <summary>
/// Called whenever this page is displayed.
/// </summary>
void HitboxTargetSettingsPage::initialize()
{
	if(m_isInitialized)
		return; // Nothing to do
	m_isInitialized = true;
}

bool HitboxTargetSettingsPage::isValid() const
{
	if(!m_ui.nameEdit->hasAcceptableInput())
		return false;
	if(!m_ui.usernameEdit->hasAcceptableInput())
		return false;
	if(m_ui.serverList->selectedItems().count() != 1)
		return false;
	if(!m_ui.streamKeyEdit->hasAcceptableInput())
		return false;
	return true;
}

/// <summary>
/// Reset button code for the page that is shared between multiple controllers.
/// </summary>
void HitboxTargetSettingsPage::sharedReset(
	WizardWindow *wizWin, WizTargetSettings *defaults)
{
	setUpdatesEnabled(false);
	initialize();

	// Update text fields
	m_ui.nameEdit->setText(defaults->name);
	m_ui.usernameEdit->setText(defaults->username);
	m_ui.streamKeyEdit->setText(defaults->rtmpStreamName);

	// Update server list selection
	int index = HitboxTarget::getIngestIndexFromURL(defaults->rtmpUrl);
	if(index < 0)
		index = HitboxTarget::getIngestListDefault();
	int numItems = HitboxTarget::getIngestListURLs().size();
	for(int i = 0; i < numItems; i++) {
		QTableWidgetItem *item = m_ui.serverList->item(i, 0);
		if(i == index) {
			item->setSelected(true);
			m_ui.serverList->setCurrentItem(item);
		} else
			item->setSelected(false);
	}

	// Reset validity and connect signal
	doQLineEditValidate(m_ui.nameEdit);
	doQLineEditValidate(m_ui.usernameEdit);
	doQLineEditValidate(m_ui.streamKeyEdit);
	wizWin->setCanContinue(isValid());
	connect(this, &HitboxTargetSettingsPage::validityMaybeChanged,
		wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	setUpdatesEnabled(true);
}

/// <summary>
/// Next button code for the page that is shared between multiple controllers.
/// </summary>
void HitboxTargetSettingsPage::sharedNext(WizTargetSettings *settings)
{
	// Save text fields
	settings->name = m_ui.nameEdit->text().trimmed();
	settings->username = m_ui.usernameEdit->text().trimmed();
	settings->rtmpStreamName = m_ui.streamKeyEdit->text();
	settings->rtmpHideStreamName = true;
	settings->rtmpPadVideo = true;

	// Save server list selection
	settings->rtmpUrl = QString();
	int numItems = HitboxTarget::getIngestListURLs().size();
	for(int i = 0; i < numItems; i++) {
		QTableWidgetItem *item = m_ui.serverList->item(i, 0);
		if(item->isSelected()) {
			// Use first selected server
			settings->rtmpUrl = item->data(Qt::UserRole).toString();
			break;
		}
	}
	if(settings->rtmpUrl.isEmpty()) {
		// Nothing was selected
		QVector<QString> urls = HitboxTarget::getIngestListURLs();
		settings->rtmpUrl = urls.at(HitboxTarget::getIngestListDefault());
	}
}

void HitboxTargetSettingsPage::dashboardClicked(const QString &link)
{
	// Insert username into URL and open in the browser
	QString user = m_ui.usernameEdit->text();
	QString url = QStringLiteral(
		"http://www.hitbox.tv/settings/%1/livestreams").arg(
		QString::fromLatin1(QUrl::toPercentEncoding(user)));
	QDesktopServices::openUrl(QUrl(url));
}

void HitboxTargetSettingsPage::nameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.nameEdit);
	emit validityMaybeChanged(isValid());
}

void HitboxTargetSettingsPage::usernameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.usernameEdit);
	emit validityMaybeChanged(isValid());
}

void HitboxTargetSettingsPage::serverSelectionChanged()
{
	emit validityMaybeChanged(isValid());
}

void HitboxTargetSettingsPage::streamKeyEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.streamKeyEdit);
	emit validityMaybeChanged(isValid());
}
