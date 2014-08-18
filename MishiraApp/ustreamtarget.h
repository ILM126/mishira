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

#ifndef USTREAMTARGET_H
#define USTREAMTARGET_H

#include "rtmptargetbase.h"

//=============================================================================
class UstreamTarget : public RTMPTargetBase
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	TargetPane *		m_pane;
	QTimer				m_paneTimer;

	// Options
	quint32				m_videoEncId;
	quint32				m_audioEncId;
	RTMPTargetInfo		m_remoteInfo;
	bool				m_padVideo;

public: // Constructor/destructor ---------------------------------------------
	UstreamTarget(
		Profile *profile, const QString &name, const UstreamTrgtOptions &opt);
	virtual ~UstreamTarget();

public: // Methods ------------------------------------------------------------
	RTMPTargetInfo		getRemoteInfo() const;
	RTMPProtocolType	getProtocol() const;
	QString				getHost() const;
	int					getPort() const;
	QString				getAppName() const;
	QString				getAppInstance() const;
	QString				getURL() const;
	QString				getStreamName() const;
	bool				getPadVideo() const;

private:
	void			updatePaneText(bool fromTimer);

private: // Interface ---------------------------------------------------------
	virtual void	initializedEvent();
	virtual bool	activateEvent();
	virtual void	deactivateEvent();

public:
	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

	virtual void	setupTargetPane(TargetPane *pane);
	virtual void	resetTargetPaneEnabled();
	virtual void	setupTargetInfo(TargetInfoWidget *widget);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	connectionStateChanged(bool wasError);
	void	paneOutdated();
	void	paneTimeout();
};
//=============================================================================

inline RTMPTargetInfo UstreamTarget::getRemoteInfo() const
{
	return m_remoteInfo;
}

inline RTMPProtocolType UstreamTarget::getProtocol() const
{
	return m_remoteInfo.protocol;
}

inline QString UstreamTarget::getHost() const
{
	return m_remoteInfo.host;
}

inline int UstreamTarget::getPort() const
{
	return m_remoteInfo.port;
}

inline QString UstreamTarget::getAppName() const
{
	return m_remoteInfo.appName;
}

inline QString UstreamTarget::getAppInstance() const
{
	return m_remoteInfo.appInstance;
}

inline QString UstreamTarget::getURL() const
{
	return m_remoteInfo.asUrl();
}

inline QString UstreamTarget::getStreamName() const
{
	return m_remoteInfo.streamName;
}

inline bool UstreamTarget::getPadVideo() const
{
	return m_padVideo;
}

#endif // USTREAMTARGET_H
