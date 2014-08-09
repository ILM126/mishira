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

#include "twitchtarget.h"
#include "application.h"
#include "audioencoder.h"
#include "constants.h"
#include "videoencoder.h"
#include "Widgets/infowidget.h"
#include "Widgets/targetpane.h"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QSslSocket>

const QString LOG_CAT = QStringLiteral("Target");

// Twitch API calls require a "client ID" in order to not be rate limited.
// Client IDs can be created in Twitch's user settings panel.
const QByteArray TWITCH_CLIENT_ID =
	QByteArrayLiteral("8n6lcndya55avfaf9w416c2k1rd5w8b");

QVector<QString> TwitchTarget::getIngestListNames()
{
	// Official ingest server list: https://api.twitch.tv/kraken/ingests
	// This list has been sorted alphabetically. The default ingest should be
	// whatever server has the "live.twitch.tv" URL.
	QVector<QString> ret;
	ret.append(QStringLiteral("Asia: Singapore"));
	ret.append(QStringLiteral("EU: Amsterdam, NL"));
	ret.append(QStringLiteral("EU: Frankfurt, Germany"));
	ret.append(QStringLiteral("EU: London, UK"));
	ret.append(QStringLiteral("EU: Paris, FR"));
	ret.append(QStringLiteral("EU: Prague, CZ"));
	ret.append(QStringLiteral("EU: Stockholm, SE"));
	ret.append(QStringLiteral("US Central: Dallas, TX"));
	ret.append(QStringLiteral("US East: Ashburn, VA"));
	ret.append(QStringLiteral("US East: Miami, FL"));
	ret.append(QStringLiteral("US East: New York, NY"));
	ret.append(QStringLiteral("US Midwest: Chicago, IL"));
	ret.append(QStringLiteral("US West: Los Angeles, CA"));
	ret.append(QStringLiteral("US West: San Francisco, CA")); // Default
	return ret;
}

QVector<QString> TwitchTarget::getIngestListURLs()
{
	// Must match `getIngestListNames()`, including order.
	QVector<QString> ret;
	ret.append(QStringLiteral("rtmp://live-sin-backup.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-ams.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-fra.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-lhr.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-cdg.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-prg.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-arn.justin.tv/app")); // sic
	ret.append(QStringLiteral("rtmp://live-dfw.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-iad.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-mia.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-jfk.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-ord.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live-lax.twitch.tv/app"));
	ret.append(QStringLiteral("rtmp://live.twitch.tv/app"));
	return ret;
}

int TwitchTarget::getIngestListDefault()
{
	// Must match `getIngestListNames()`, including order.
	return 13; // Index of default ingest
}

int TwitchTarget::getIngestIndexFromURL(const QString &url)
{
	QVector<QString> urls = getIngestListURLs();
	for(int i = 0; i < urls.size(); i++) {
		if(urls.at(i) == url)
			return i;
	}
	return -1;
}

QString TwitchTarget::getIngestNameFromURL(const QString &url)
{
	int index = getIngestIndexFromURL(url);
	if(index < 0)
		return tr("** Unknown server **");
	QVector<QString> names = getIngestListNames();
	return names.at(index);
}

TwitchTarget::TwitchTarget(
	Profile *profile, const QString &name, const TwitchTrgtOptions &opt)
	: RTMPTargetBase(profile, TrgtTwitchType, name)
	, m_pane(NULL)
	, m_paneTimer(this)
	, m_network(this)
	, m_prevStreamQueryTime(0)
	, m_numViewers(0)

	// Options
	, m_videoEncId(opt.videoEncId)
	, m_audioEncId(opt.audioEncId)
	, m_remoteInfo(RTMPTargetInfo::fromUrl(opt.url, false, opt.streamKey))
	, m_username(opt.username)
{
	// Connect signals
	connect(this, &RTMPTargetBase::rtmpDroppedFrames,
		this, &TwitchTarget::paneOutdated);
	connect(this, &RTMPTargetBase::rtmpDroppedPadding,
		this, &TwitchTarget::paneOutdated);
	connect(this, &RTMPTargetBase::rtmpStateChanged,
		this, &TwitchTarget::connectionStateChanged);
	connect(&m_paneTimer, &QTimer::timeout,
		this, &TwitchTarget::paneTimeout);
	connect(&m_network, &QNetworkAccessManager::finished,
		this, &TwitchTarget::networkFinished);

	// Setup timer to refresh the pane
	m_paneTimer.setSingleShot(false);
	m_paneTimer.start(500); // 500ms refresh

	// Sanity check to make sure Qt was built with SSL support
#ifdef QT_NO_SSL
#error Qt not built with SSL support
#else
	Q_ASSERT(QSslSocket::supportsSsl());
#endif // QT_NO_SSL
}

TwitchTarget::~TwitchTarget()
{
	// Make sure we have released our resources
	setActive(false);
}

void TwitchTarget::initializedEvent()
{
	// Initialize base class
	rtmpInit(m_videoEncId, m_audioEncId, true); // Always pad video stream
}

bool TwitchTarget::activateEvent()
{
	if(m_isInitializing)
		return false;
	if(m_videoEnc == NULL)
		return false;

	appLog(LOG_CAT)
		<< "Activating Twitch target " << getIdString()
		<< " with target URL \"" << m_remoteInfo.asUrl() << "\"...";

	// Immediately initialize RTMP as we know the URL
	if(!rtmpActivate(m_remoteInfo))
		return false;

	// Don't query Twitch immediately and reset any previous results
	m_prevStreamQueryTime = App->getUsecSinceExec();
	m_numViewers = 0;

	if(m_pane != NULL)
		setPaneStateHelper(m_pane);
	appLog(LOG_CAT) << "Twitch target activated";
	return true;
}

void TwitchTarget::deactivateEvent()
{
	if(m_isInitializing)
		return;
	if(m_videoEnc == NULL)
		return; // Never activated
	appLog(LOG_CAT) << "Deactivating Twitch target " << getIdString() << "...";

	// Immediately deactivate RTMP
	rtmpDeactivate();

	if(m_pane != NULL)
		m_pane->setPaneState(TargetPane::OfflineState);
	appLog(LOG_CAT) << "Twitch target deactivated";
}

void TwitchTarget::serialize(QDataStream *stream) const
{
	Target::serialize(stream);

	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << m_videoEncId;
	*stream << m_audioEncId;
	*stream << m_remoteInfo.asUrl();
	*stream << m_remoteInfo.streamName;
	*stream << m_username;
}

bool TwitchTarget::unserialize(QDataStream *stream)
{
	if(!Target::unserialize(stream))
		return false;

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		QString stringData;

		// Read our data
		*stream >> m_videoEncId;
		*stream >> m_audioEncId;
		*stream >> stringData;
		m_remoteInfo = RTMPTargetInfo::fromUrl(stringData);
		*stream >> m_remoteInfo.streamName;
		*stream >> m_username;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in Twitch target serialized data, "
			<< "cannot load settings";
		return false;
	}

	// Verify that the specified URL is actually valid. If it isn't replace it
	// with the default. This is just in case the user attempts to load an old
	// profile with an outdated server selected.
	QString prevUrl = m_remoteInfo.asUrl();
	if(getIngestIndexFromURL(prevUrl) < 0) {
		QVector<QString> urls = getIngestListURLs();
		QString defaultUrl = urls.at(getIngestListDefault());
		m_remoteInfo = RTMPTargetInfo::fromUrl(
			defaultUrl, false, m_remoteInfo.streamName);
		appLog(LOG_CAT, Log::Warning) << QStringLiteral(
			"Specified URL \"%1\" doesn't exist in server list, using \"%2\" instead")
			.arg(prevUrl).arg(defaultUrl);
	}

	return true;
}

void TwitchTarget::setupTargetPane(TargetPane *pane)
{
	m_pane = pane;
	setupTargetPaneHelper(m_pane, ":/Resources/target-18x18-twitch.png");
	updatePaneText(true);
}

void TwitchTarget::updatePaneText(bool fromTimer)
{
	if(m_pane == NULL)
		return;

	// Should we update the viewer count?
	if(m_isActive) {
		quint64 now = App->getUsecSinceExec();
		const quint64 TWITCH_QUERY_INTERVAL_USEC = 90000000; // 90 seconds
		if(now - m_prevStreamQueryTime > TWITCH_QUERY_INTERVAL_USEC) {
			updateTwitchStats();
			m_prevStreamQueryTime = now;
		}
	}

	int offset = 0;
	m_pane->setItemText(
		offset++, tr("Viewers:"), QStringLiteral("%L1").arg(m_numViewers),
		false);
	offset = updatePaneTextHelper(m_pane, offset, fromTimer);
}

void TwitchTarget::updateTwitchStats()
{
	if(m_username.isEmpty())// || m_username.contains(QChar('/')))
		return; // Invalid username

	// Request Twitch stream object which contains the viewer count. Note that
	// this request doesn't work for private streams.
	QNetworkRequest req;
	QString url = QStringLiteral(
		"https://api.twitch.tv/kraken/streams/%1").arg(
		QString::fromLatin1(QUrl::toPercentEncoding(m_username.toLower())));
	req.setUrl(QUrl(url));
	req.setRawHeader("Accept", "application/vnd.twitchtv.v2+json");
	req.setRawHeader("User-Agent",
		QStringLiteral("%1/%2").arg(APP_NAME).arg(APP_VER_STR).toUtf8());
	req.setRawHeader("Client-ID", TWITCH_CLIENT_ID);
	m_network.get(req);
}

void TwitchTarget::networkFinished(QNetworkReply *reply)
{
	if(reply->error() != QNetworkReply::NoError) {
		//appLog() << "Network error " << reply->error();
		reply->deleteLater();
		return; // Network error occurred
	}
	QByteArray data = reply->readAll();
	reply->deleteLater();

	// Process reply as JSON to get viewer count
	QJsonDocument doc = QJsonDocument::fromJson(data);
	QJsonObject obj = doc.object();
	QJsonValue val = obj.value(QStringLiteral("stream"));
	if(!val.isObject())
		return; // Undefined, null or invalid response
	obj = val.toObject();
	val = obj.value(QStringLiteral("viewers"));
	if(!val.isDouble())
		return; // Undefined, null or invalid response
	m_numViewers = qRound(val.toDouble());
}

void TwitchTarget::connectionStateChanged(bool wasError)
{
	if(m_isActive)
		setPaneStateHelper(m_pane);
	else
		m_pane->setPaneState(TargetPane::OfflineState);
}

void TwitchTarget::paneOutdated()
{
	updatePaneText(false);
}

void TwitchTarget::paneTimeout()
{
	updatePaneText(true);
}

void TwitchTarget::resetTargetPaneEnabled()
{
	if(m_pane == NULL)
		return;
	m_pane->setUserEnabled(m_isEnabled);
}

void TwitchTarget::setupTargetInfo(TargetInfoWidget *widget)
{
	int offset = setupTargetInfoHelper(widget, 0,
		":/Resources/target-160x70-twitch.png");

	widget->setItemText(offset++, tr("Username:"), m_username, true);
	widget->setItemText(
		offset++, tr("Server:"), getIngestNameFromURL(m_remoteInfo.asUrl()),
		true);
}
