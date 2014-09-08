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

#ifndef RTMPTARGET_H
#define RTMPTARGET_H

#include "../rtmptargetbase.h"

//=============================================================================
class RTMPTarget : public RTMPTargetBase
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	TargetPane *		m_pane;
	QTimer				m_paneTimer;

	// Options
	quint32				m_videoEncId;
	quint32				m_audioEncId;
	RTMPTargetInfo		m_remoteInfo;
	bool				m_hideStreamName;
	bool				m_padVideo;

public: // Constructor/destructor ---------------------------------------------
	RTMPTarget(
		Profile *profile, const QString &name, const RTMPTrgtOptions &opt);
	virtual ~RTMPTarget();

public: // Methods ------------------------------------------------------------
	RTMPTargetInfo		getRemoteInfo() const;
	RTMPProtocolType	getProtocol() const;
	QString				getHost() const;
	int					getPort() const;
	QString				getAppName() const;
	QString				getAppInstance() const;
	QString				getURL() const;
	QString				getStreamName() const;
	bool				getHideStreamName() const;
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

inline RTMPTargetInfo RTMPTarget::getRemoteInfo() const
{
	return m_remoteInfo;
}

inline RTMPProtocolType RTMPTarget::getProtocol() const
{
	return m_remoteInfo.protocol;
}

inline QString RTMPTarget::getHost() const
{
	return m_remoteInfo.host;
}

inline int RTMPTarget::getPort() const
{
	return m_remoteInfo.port;
}

inline QString RTMPTarget::getAppName() const
{
	return m_remoteInfo.appName;
}

inline QString RTMPTarget::getAppInstance() const
{
	return m_remoteInfo.appInstance;
}

inline QString RTMPTarget::getURL() const
{
	return m_remoteInfo.asUrl();
}

inline QString RTMPTarget::getStreamName() const
{
	return m_remoteInfo.streamName;
}

inline bool RTMPTarget::getHideStreamName() const
{
	return m_hideStreamName;
}

inline bool RTMPTarget::getPadVideo() const
{
	return m_padVideo;
}

#endif // RTMPTARGET_H
