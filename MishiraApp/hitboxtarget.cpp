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

#include "hitboxtarget.h"
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

QVector<QString> HitboxTarget::getIngestListNames()
{
	// We get our server list from OBS as the Hitbox API doesn't easily allow
	// us to look at it manually. This list has been sorted alphabetically. The
	// default ingest should be whatever server has the "live.hitbox.tv" URL.
	QVector<QString> ret;
	ret.append(QStringLiteral("Default")); // Default
	ret.append(QStringLiteral("EU Central"));
	ret.append(QStringLiteral("EU East"));
	ret.append(QStringLiteral("EU North"));
	ret.append(QStringLiteral("EU West"));
	ret.append(QStringLiteral("US Central"));
	ret.append(QStringLiteral("US East"));
	ret.append(QStringLiteral("US West"));
	ret.append(QStringLiteral("South America"));
	ret.append(QStringLiteral("South Korea"));
	ret.append(QStringLiteral("United Kingdom"));
	return ret;
}

QVector<QString> HitboxTarget::getIngestListURLs()
{
	// Must match `getIngestListNames()`, including order.
	QVector<QString> ret;
	ret.append(QStringLiteral("rtmp://live.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.nbg.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.vie.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.ams.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.fra.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.den.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.vgn.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.lax.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.gru.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.icn.hitbox.tv/push"));
	ret.append(QStringLiteral("rtmp://live.lhr.hitbox.tv/push"));
	return ret;
}

int HitboxTarget::getIngestListDefault()
{
	// Must match `getIngestListNames()`, including order.
	return 0; // Index of default ingest
}

int HitboxTarget::getIngestIndexFromURL(const QString &url)
{
	QVector<QString> urls = getIngestListURLs();
	for(int i = 0; i < urls.size(); i++) {
		if(urls.at(i) == url)
			return i;
	}
	return -1;
}

QString HitboxTarget::getIngestNameFromURL(const QString &url)
{
	int index = getIngestIndexFromURL(url);
	if(index < 0)
		return tr("** Unknown server **");
	QVector<QString> names = getIngestListNames();
	return names.at(index);
}

HitboxTarget::HitboxTarget(
	Profile *profile, const QString &name, const HitboxTrgtOptions &opt)
	: RTMPTargetBase(profile, TrgtHitboxType, name)
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
		this, &HitboxTarget::paneOutdated);
	connect(this, &RTMPTargetBase::rtmpDroppedPadding,
		this, &HitboxTarget::paneOutdated);
	connect(this, &RTMPTargetBase::rtmpStateChanged,
		this, &HitboxTarget::connectionStateChanged);
	connect(&m_paneTimer, &QTimer::timeout,
		this, &HitboxTarget::paneTimeout);
	connect(&m_network, &QNetworkAccessManager::finished,
		this, &HitboxTarget::networkFinished);

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

HitboxTarget::~HitboxTarget()
{
	// Make sure we have released our resources
	setActive(false);
}

void HitboxTarget::initializedEvent()
{
	// Initialize base class
	rtmpInit(m_videoEncId, m_audioEncId, true); // Always pad video stream
}

bool HitboxTarget::activateEvent()
{
	if(m_isInitializing)
		return false;
	if(m_videoEnc == NULL)
		return false;

	appLog(LOG_CAT)
		<< "Activating Hitbox target " << getIdString()
		<< " with target URL \"" << m_remoteInfo.asUrl() << "\"...";

	// Immediately initialize RTMP as we know the URL
	if(!rtmpActivate(m_remoteInfo))
		return false;

	// Don't query Hitbox immediately and reset any previous results
	m_prevStreamQueryTime = App->getUsecSinceExec();
	m_numViewers = 0;

	if(m_pane != NULL)
		setPaneStateHelper(m_pane);
	appLog(LOG_CAT) << "Hitbox target activated";
	return true;
}

void HitboxTarget::deactivateEvent()
{
	if(m_isInitializing)
		return;
	if(m_videoEnc == NULL)
		return; // Never activated
	appLog(LOG_CAT) << "Deactivating Hitbox target " << getIdString() << "...";

	// Immediately deactivate RTMP
	rtmpDeactivate();

	if(m_pane != NULL)
		m_pane->setPaneState(TargetPane::OfflineState);
	appLog(LOG_CAT) << "Hitbox target deactivated";
}

void HitboxTarget::serialize(QDataStream *stream) const
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

bool HitboxTarget::unserialize(QDataStream *stream)
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
			<< "Unknown version number in Hitbox target serialized data, "
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

void HitboxTarget::setupTargetPane(TargetPane *pane)
{
	m_pane = pane;
	setupTargetPaneHelper(m_pane, ":/Resources/target-18x18-hitbox.png");
	updatePaneText(true);
}

void HitboxTarget::updatePaneText(bool fromTimer)
{
	if(m_pane == NULL)
		return;

	// Should we update the viewer count?
	if(m_isActive) {
		quint64 now = App->getUsecSinceExec();
		const quint64 HITBOX_QUERY_INTERVAL_USEC = 90000000; // 90 seconds
		if(now - m_prevStreamQueryTime > HITBOX_QUERY_INTERVAL_USEC) {
			updateHitboxStats();
			m_prevStreamQueryTime = now;
		}
	}

	int offset = 0;
	//m_pane->setItemText( // TODO: Viewer count
	//	offset++, tr("Viewers:"), QStringLiteral("%L1").arg(m_numViewers),
	//	false);
	offset = updatePaneTextHelper(m_pane, offset, fromTimer);
}

void HitboxTarget::updateHitboxStats()
{
	if(m_username.isEmpty())// || m_username.contains(QChar('/')))
		return; // Invalid username

	// TODO: Request viewer count via the Hitbox API here
}

void HitboxTarget::networkFinished(QNetworkReply *reply)
{
	if(reply->error() != QNetworkReply::NoError) {
		//appLog() << "Network error " << reply->error();
		reply->deleteLater();
		return; // Network error occurred
	}
	QByteArray data = reply->readAll();
	reply->deleteLater();

	// TODO: Process viewer count response and save to `m_numViewers`
}

void HitboxTarget::connectionStateChanged(bool wasError)
{
	if(m_isActive)
		setPaneStateHelper(m_pane);
	else
		m_pane->setPaneState(TargetPane::OfflineState);
}

void HitboxTarget::paneOutdated()
{
	updatePaneText(false);
}

void HitboxTarget::paneTimeout()
{
	updatePaneText(true);
}

void HitboxTarget::resetTargetPaneEnabled()
{
	if(m_pane == NULL)
		return;
	m_pane->setUserEnabled(m_isEnabled);
}

void HitboxTarget::setupTargetInfo(TargetInfoWidget *widget)
{
	int offset = setupTargetInfoHelper(widget, 0,
		":/Resources/target-160x70-hitbox.png");

	widget->setItemText(offset++, tr("Username:"), m_username, true);
	widget->setItemText(
		offset++, tr("Server:"), getIngestNameFromURL(m_remoteInfo.asUrl()),
		true);
}
