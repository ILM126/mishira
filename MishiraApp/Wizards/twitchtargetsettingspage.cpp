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

#include "twitchtargetsettingspage.h"
#include "common.h"
#include "twitchtarget.h"
#include "validators.h"
#include "wizardwindow.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtWidgets/QTableWidget>

#if USE_PING_WIDGET

#include <QtCore/QUrl>
#ifdef Q_OS_WIN
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#endif

//=============================================================================
// PingWidgetThread class

PingWidgetThread::PingWidgetThread(quint32 ipv4Addr, int maxTries)
	: QThread()
	, m_ipv4Addr(ipv4Addr)
	, m_maxTries(maxTries)
	, m_avgResult(0)
	, m_numResults(0)
{
}

PingWidgetThread::~PingWidgetThread()
{
}

void PingWidgetThread::run()
{
	// Debug
	QString ipStr = QStringLiteral("%1.%2.%3.%4")
		.arg((m_ipv4Addr >> 24) & 0xFF)
		.arg((m_ipv4Addr >> 16) & 0xFF)
		.arg((m_ipv4Addr >> 8) & 0xFF)
		.arg(m_ipv4Addr & 0xFF);

	for(int tryNum = 0; tryNum < m_maxTries; tryNum++) {
		int result;

		// TODO: ICMP pings don't work, need to actually open TCP connections
#ifdef Q_OS_WIN
		char sendData[32] = "Send data";
		const int REPLY_SIZE = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData);
		char replyBuf[REPLY_SIZE];
		memset(&replyBuf, 0, sizeof(replyBuf));

		// Open ICMP handle
		HANDLE icmpFile = IcmpCreateFile();
		if(icmpFile == INVALID_HANDLE_VALUE)
			return;

		// Do the actual ICMP ping
		appLog() << "Pinging " << ipStr;
		DWORD res = IcmpSendEcho(
			icmpFile, m_ipv4Addr, sendData, sizeof(sendData), NULL, replyBuf,
			sizeof(replyBuf), 5000);
		if(res == 0) {
			// Error codes near 11000 are "IP_" errors (See IP_STATUS_BASE)
			appLog() << ipStr << " error: " << (int)GetLastError();
			IcmpCloseHandle(icmpFile);
			if(res == ERROR_NETWORK_UNREACHABLE)
				return; // No point attempting again
			continue;
		}
		PICMP_ECHO_REPLY echoReply = (PICMP_ECHO_REPLY)replyBuf;
		result = echoReply->RoundTripTime;

		// Close ICMP handle
		IcmpCloseHandle(icmpFile);
#else
#error Unsupported platform
#endif // Q_OS_WIN

		// Aggregate results. TODO: Thread locking
		appLog() << "Received result " << result << " for " << ipStr;
		m_avgResult =
			(m_avgResult * m_numResults + result) / (m_numResults + 1);
		m_numResults++;

		// Notify listeners
		emit avgUpdated(m_avgResult);
	}
}

//=============================================================================
// PingWidget class

PingWidget::PingWidget(const QString &rtmpUrl, QTableWidgetItem *item)
	: QObject()
	, m_runOnce(false)
	, m_hostname()
	, m_item(item)
	, m_thread(NULL)
{
	// Get hostname from URL
	QUrl url(rtmpUrl);
	m_hostname = url.host();
}

PingWidget::~PingWidget()
{
	if(m_thread != NULL)
		delete m_thread;
	m_thread = NULL;
}

void PingWidget::run()
{
	if(m_runOnce)
		return; // Already run
	m_runOnce = true;

	// Get IP address from hostname
	QHostInfo::lookupHost(m_hostname, this, SLOT(lookedUp(const QHostInfo &)));
}

void PingWidget::lookedUp(const QHostInfo &host)
{
	// Do we have an IPv4 address?
	const QList<QHostAddress> &addrs = host.addresses();
	quint32 ipv4Addr = 0;
	for(int i = 0; i < addrs.size(); i++) {
		const QHostAddress &addr = addrs.at(i);
		if(addr.protocol() == QAbstractSocket::IPv4Protocol) {
			ipv4Addr = addr.toIPv4Address();
			break;
		}
	}
	if(ipv4Addr == 0)
		return; // No valid IPv4 address found

	m_thread = new PingWidgetThread(ipv4Addr, 5);
	connect(m_thread, &PingWidgetThread::avgUpdated,
		this, &PingWidget::avgUpdated);
	m_thread->start();
}

void PingWidget::avgUpdated(int avgResult)
{
	// Update the valid in our GUI thread
	m_item->setText(QStringLiteral("%L1 msec").arg(avgResult));
}
#endif // USE_PING_WIDGET

//=============================================================================
// TwitchTargetSettingsPage class

TwitchTargetSettingsPage::TwitchTargetSettingsPage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
	, m_isInitialized(false)
#if USE_PING_WIDGET
	, m_pingWidgets()
#endif // USE_PING_WIDGET
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
		this, &TwitchTargetSettingsPage::nameEditChanged);
	connect(m_ui.usernameEdit, &QLineEdit::textChanged,
		this, &TwitchTargetSettingsPage::usernameEditChanged);
	connect(m_ui.serverList, &QTableWidget::itemSelectionChanged,
		this, &TwitchTargetSettingsPage::serverSelectionChanged);
	connect(m_ui.streamKeyEdit, &QLineEdit::textChanged,
		this, &TwitchTargetSettingsPage::streamKeyEditChanged);

	m_ui.streamKeyEdit->setEchoMode(QLineEdit::Password);

	// Populate server list and create ping widgets
	QVector<QString> names = TwitchTarget::getIngestListNames();
	QVector<QString> urls = TwitchTarget::getIngestListURLs();
	for(int i = 0; i < urls.size(); i++) {
		QTableWidgetItem *item = new QTableWidgetItem(names.at(i));
		item->setData(Qt::UserRole, urls.at(i));
		m_ui.serverList->insertRow(i);
		m_ui.serverList->setItem(i, 0, item);

#if USE_PING_WIDGET
		item = new QTableWidgetItem("? msec");
		PingWidget *ping = new PingWidget(urls.at(i), item);
		m_pingWidgets.append(ping);
		m_ui.serverList->setItem(i, 1, item);
#endif // USE_PING_WIDGET
	}

	// Change server list header text alignment
	QTableWidgetItem *item = m_ui.serverList->horizontalHeaderItem(0);
	item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
#if USE_PING_WIDGET
	item = m_ui.serverList->horizontalHeaderItem(1);
	item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
#endif // USE_PING_WIDGET
}

TwitchTargetSettingsPage::~TwitchTargetSettingsPage()
{
#if USE_PING_WIDGET
	// Delete ping widgets
	while(!m_pingWidgets.isEmpty()) {
		delete m_pingWidgets.last();
		m_pingWidgets.pop_back();
	}
#endif // USE_PING_WIDGET
}

/// <summary>
/// Called whenever this page is displayed.
/// </summary>
void TwitchTargetSettingsPage::initialize()
{
	if(m_isInitialized)
		return; // Nothing to do
	m_isInitialized = true;

#if USE_PING_WIDGET
	// Start pinging our servers
	for(int i = 0; i < m_pingWidgets.size(); i++)
		m_pingWidgets.at(i)->run();
#endif // USE_PING_WIDGET
}

bool TwitchTargetSettingsPage::isValid() const
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
void TwitchTargetSettingsPage::sharedReset(
	WizardWindow *wizWin, WizTargetSettings *defaults)
{
	setUpdatesEnabled(false);
	initialize();

	// Update text fields
	m_ui.nameEdit->setText(defaults->name);
	m_ui.usernameEdit->setText(defaults->username);
	m_ui.streamKeyEdit->setText(defaults->rtmpStreamName);

	// Update server list selection
	int index = TwitchTarget::getIngestIndexFromURL(defaults->rtmpUrl);
	if(index < 0)
		index = TwitchTarget::getIngestListDefault();
	int numItems = TwitchTarget::getIngestListURLs().size();
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
	connect(this, &TwitchTargetSettingsPage::validityMaybeChanged,
		wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	setUpdatesEnabled(true);
}

/// <summary>
/// Next button code for the page that is shared between multiple controllers.
/// </summary>
void TwitchTargetSettingsPage::sharedNext(WizTargetSettings *settings)
{
	// Save text fields
	settings->name = m_ui.nameEdit->text().trimmed();
	settings->username = m_ui.usernameEdit->text().trimmed();
	settings->rtmpStreamName = m_ui.streamKeyEdit->text();
	settings->rtmpHideStreamName = true;
	settings->rtmpPadVideo = true;

	// Save server list selection
	settings->rtmpUrl = QString();
	int numItems = TwitchTarget::getIngestListURLs().size();
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
		QVector<QString> urls = TwitchTarget::getIngestListURLs();
		settings->rtmpUrl = urls.at(TwitchTarget::getIngestListDefault());
	}
}

void TwitchTargetSettingsPage::nameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.nameEdit);
	emit validityMaybeChanged(isValid());
}

void TwitchTargetSettingsPage::usernameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.usernameEdit);
	emit validityMaybeChanged(isValid());
}

void TwitchTargetSettingsPage::serverSelectionChanged()
{
	emit validityMaybeChanged(isValid());
}

void TwitchTargetSettingsPage::streamKeyEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.streamKeyEdit);
	emit validityMaybeChanged(isValid());
}
