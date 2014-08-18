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

#ifndef VIDEOSOURCEMANAGER_H
#define VIDEOSOURCEMANAGER_H

#include "common.h"
//#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtWidgets/QWidget>

class VideoSource;

typedef QVector<VideoSource *> VideoSourceList;

//=============================================================================
/// <summary>
/// A simple hidden top-level window that processes `WM_DEVICECHANGE` events.
/// </summary>
class DeviceChangeReceiver : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	bool	m_timerActive;

public: // Constructor/destructor ---------------------------------------------
	DeviceChangeReceiver();
	virtual ~DeviceChangeReceiver();

private: // Methods -----------------------------------------------------------
	bool	nativeEvent(
		const QByteArray &eventType, void *message, long *result);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	deviceChanged();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	timeout();
};
//=============================================================================

//=============================================================================
/// <summary>
/// Provides a database of all the available video sources. This class does not
/// manage the allocation of sources however, it just manages the directory.
/// </summary>
class VideoSourceManager : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	bool					m_initialized;
	VideoSourceList			m_sources;
	DeviceChangeReceiver *	m_receiver;
	//QMutex					m_mutex;

public: // Constructor/destructor ---------------------------------------------
	VideoSourceManager();
	virtual ~VideoSourceManager();

public: // Methods ------------------------------------------------------------
	void			initialize();
	VideoSourceList	getSourceList() const;
	VideoSource *	getSource(quint64 sourceId);

	// Used by video source factories only
	void			addSource(VideoSource *source);
	void			removeSource(VideoSource *source);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			sourceAdded(VideoSource *source);
	void			removingSource(VideoSource *source);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			queuedFrameEvent(uint frameNum, int numDropped);
	void			deviceChanged();
};
//=============================================================================

inline VideoSourceList VideoSourceManager::getSourceList() const
{
	return m_sources;
}

#endif // VIDEOSOURCEMANAGER_H
