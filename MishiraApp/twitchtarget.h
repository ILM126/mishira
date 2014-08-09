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

#ifndef TWITCHTARGET_H
#define TWITCHTARGET_H

#include "rtmptargetbase.h"
#include <QtNetwork/QNetworkAccessManager>

//=============================================================================
class TwitchTarget : public RTMPTargetBase
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	TargetPane *			m_pane;
	QTimer					m_paneTimer;
	QNetworkAccessManager	m_network;
	quint64					m_prevStreamQueryTime; // Last Twitch.tv API query
	int						m_numViewers;

	// Options
	quint32				m_videoEncId;
	quint32				m_audioEncId;
	RTMPTargetInfo		m_remoteInfo;
	QString 			m_username;

public: // Static methods -----------------------------------------------------
	static QVector<QString>	getIngestListNames();
	static QVector<QString>	getIngestListURLs();
	static int				getIngestListDefault();
	static int				getIngestIndexFromURL(const QString &url);
	static QString			getIngestNameFromURL(const QString &url);

public: // Constructor/destructor ---------------------------------------------
	TwitchTarget(
		Profile *profile, const QString &name, const TwitchTrgtOptions &opt);
	virtual ~TwitchTarget();

public: // Methods ------------------------------------------------------------
	RTMPTargetInfo	getRemoteInfo() const;
	QString			getURL() const;
	QString			getStreamKey() const;
	QString			getUsername() const;

private:
	void			updatePaneText(bool fromTimer);
	void			updateTwitchStats();

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
	void	networkFinished(QNetworkReply *reply);
};
//=============================================================================

inline RTMPTargetInfo TwitchTarget::getRemoteInfo() const
{
	return m_remoteInfo;
}

inline QString TwitchTarget::getURL() const
{
	return m_remoteInfo.asUrl();
}

inline QString TwitchTarget::getUsername() const
{
	return m_username;
}

inline QString TwitchTarget::getStreamKey() const
{
	return m_remoteInfo.streamName;
}

#endif // TWITCHTARGET_H
