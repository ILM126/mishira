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

#include "ustreamtarget.h"
#include "application.h"
#include "audioencoder.h"
#include "videoencoder.h"
#include "Widgets/infowidget.h"
#include "Widgets/targetpane.h"

const QString LOG_CAT = QStringLiteral("Target");

// Ustreams API calls require a "data API key" in order to not be rate limited.
// API keys can be created from the Ustream development website
// ( http://developer.ustream.tv ).
//const QByteArray USTREAM_API_KEY =
//	QByteArrayLiteral("FB159317E004BEF3ECDF6BFCF5584C7D");

UstreamTarget::UstreamTarget(
	Profile *profile, const QString &name, const UstreamTrgtOptions &opt)
	: RTMPTargetBase(profile, TrgtUstreamType, name)
	, m_pane(NULL)
	, m_paneTimer(this)

	// Options
	, m_videoEncId(opt.videoEncId)
	, m_audioEncId(opt.audioEncId)
	, m_remoteInfo(RTMPTargetInfo::fromUrl(opt.url, false, opt.streamKey))
	, m_padVideo(opt.padVideo)
{
	// Connect signals
	connect(this, &RTMPTargetBase::rtmpDroppedFrames,
		this, &UstreamTarget::paneOutdated);
	connect(this, &RTMPTargetBase::rtmpDroppedPadding,
		this, &UstreamTarget::paneOutdated);
	connect(this, &RTMPTargetBase::rtmpStateChanged,
		this, &UstreamTarget::connectionStateChanged);
	connect(&m_paneTimer, &QTimer::timeout,
		this, &UstreamTarget::paneTimeout);

	// Setup timer to refresh the pane
	m_paneTimer.setSingleShot(false);
	m_paneTimer.start(500); // 500ms refresh
}

UstreamTarget::~UstreamTarget()
{
	// Make sure we have released our resources
	setActive(false);
}

void UstreamTarget::initializedEvent()
{
	// Initialize base class
	rtmpInit(m_videoEncId, m_audioEncId, m_padVideo);
}

bool UstreamTarget::activateEvent()
{
	if(m_isInitializing)
		return false;
	if(m_videoEnc == NULL)
		return false;

	appLog(LOG_CAT)
		<< "Activating Ustream target " << getIdString()
		<< " with target URL \"" << m_remoteInfo.asUrl() << "\"...";

	// Immediately initialize RTMP as we know the URL
	if(!rtmpActivate(m_remoteInfo))
		return false;

	if(m_pane != NULL)
		setPaneStateHelper(m_pane);
	appLog(LOG_CAT) << "Ustream target activated";
	return true;
}

void UstreamTarget::deactivateEvent()
{
	if(m_isInitializing)
		return;
	if(m_videoEnc == NULL)
		return; // Never activated
	appLog(LOG_CAT)
		<< "Deactivating Ustream target " << getIdString() << "...";

	// Immediately deactivate RTMP
	rtmpDeactivate();

	if(m_pane != NULL)
		m_pane->setPaneState(TargetPane::OfflineState);
	appLog(LOG_CAT) << "Ustream target deactivated";
}

void UstreamTarget::serialize(QDataStream *stream) const
{
	Target::serialize(stream);

	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << m_videoEncId;
	*stream << m_audioEncId;
	*stream << m_remoteInfo.asUrl();
	*stream << m_remoteInfo.streamName;
	*stream << m_padVideo;
}

bool UstreamTarget::unserialize(QDataStream *stream)
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
		*stream >> m_padVideo;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in Ustream target serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

void UstreamTarget::setupTargetPane(TargetPane *pane)
{
	m_pane = pane;
	// TODO: Ustream icon
	setupTargetPaneHelper(m_pane, ":/Resources/target-18x18-rtmp.png");
	updatePaneText(true);
}

void UstreamTarget::updatePaneText(bool fromTimer)
{
	if(m_pane == NULL)
		return;
	updatePaneTextHelper(m_pane, 0, fromTimer);
}

void UstreamTarget::connectionStateChanged(bool wasError)
{
	if(m_isActive)
		setPaneStateHelper(m_pane);
	else
		m_pane->setPaneState(TargetPane::OfflineState);
}

void UstreamTarget::paneOutdated()
{
	updatePaneText(false);
}

void UstreamTarget::paneTimeout()
{
	updatePaneText(true);
}

void UstreamTarget::resetTargetPaneEnabled()
{
	if(m_pane == NULL)
		return;
	m_pane->setUserEnabled(m_isEnabled);
}

void UstreamTarget::setupTargetInfo(TargetInfoWidget *widget)
{
	int offset = setupTargetInfoHelper(
		widget, 0, ":/Resources/target-160x70-ustream.png");

	widget->setItemText(offset++, tr("URL:"), m_remoteInfo.asUrl(), true);
}
