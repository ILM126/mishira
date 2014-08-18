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

#include "videosourcemanager.h"
#include "application.h"
#include "dshowvideosource.h"

//=============================================================================
// DeviceChangeReceiver class

DeviceChangeReceiver::DeviceChangeReceiver()
	: QWidget()
	, m_timerActive(false)
{
	setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
	setWindowTitle(QStringLiteral("Device change receiver"));
	show(); // Force creation of native window handle
	hide();
}

DeviceChangeReceiver::~DeviceChangeReceiver()
{
}

bool DeviceChangeReceiver::nativeEvent(
	const QByteArray &eventType, void *message, long *result)
{
	if(eventType != QByteArrayLiteral("windows_generic_MSG"))
		return false; // Forward to Qt
	MSG *msg = static_cast<MSG *>(message);
	LRESULT *res = static_cast<LRESULT *>(result);
	if(msg->message != WM_DEVICECHANGE)
		return false; // Forward to Qt
	// This is a `WM_DEVICECHANGE` event

	// Don't spam signal emits as often by only triggering after a short amount
	// of time
	//appLog() << "Device changed event received";
	if(!m_timerActive) {
		m_timerActive = true;
		QTimer::singleShot(100, this, SLOT(timeout()));
	}

	return false; // Forward to Qt as well
}

void DeviceChangeReceiver::timeout()
{
	//appLog() << "Device changed event emitted";
	m_timerActive = false;
	emit deviceChanged();
}

//=============================================================================
// VideoSourceManager class

VideoSourceManager::VideoSourceManager()
	: QObject()
	, m_initialized(false)
	, m_sources()
	, m_receiver(new DeviceChangeReceiver())
	//, m_mutex(QMutex::Recursive)
{
	connect(m_receiver, &DeviceChangeReceiver::deviceChanged,
		this, &VideoSourceManager::deviceChanged);
}

VideoSourceManager::~VideoSourceManager()
{
	// Cleanly delete sources
	while(!m_sources.isEmpty())
		removeSource(m_sources.first());

	// Delete event receiver
	delete m_receiver;
}

void VideoSourceManager::initialize()
{
	if(m_initialized)
		return; // Already initialized
#ifdef Q_OS_WIN
	DShowVideoSource::populateSources(this);
#endif

	// Log available video sources
	appLog() << "Available video sources:";
	if(m_sources.count()) {
		for(int i = 0; i < m_sources.count(); i++) {
			appLog() << "  - " << m_sources.at(i)->getDebugString();
		}
	} else
		appLog() << "    ** None **";

	// Connect to main loop
	connect(App, &Application::queuedFrameEvent,
		this, &VideoSourceManager::queuedFrameEvent);
}

VideoSource *VideoSourceManager::getSource(quint64 sourceId)
{
	//m_mutex.lock();
	for(int i = 0; i < m_sources.count(); i++) {
		VideoSource *src = m_sources.at(i);
		if(src->getId() == sourceId) {
			//m_mutex.unlock();
			return src;
		}
	}
	//m_mutex.unlock();
	return NULL;
}

/// <summary>
/// Registers an video source with the manager.
/// </summary>
void VideoSourceManager::addSource(VideoSource *source)
{
	if(source == NULL)
		return;
	//m_mutex.lock();
	for(int i = 0; i < m_sources.count(); i++) {
		if(m_sources.at(i) == source) {
			// Already added
			//m_mutex.unlock();
			return;
		}
	}
	m_sources.append(source);
	//m_mutex.unlock();
	emit sourceAdded(source);
}

/// <summary>
/// Unregisters an video source from the manager.
/// </summary>
void VideoSourceManager::removeSource(VideoSource *source)
{
	if(source == NULL)
		return;
	//m_mutex.lock();
	for(int i = 0; i < m_sources.count(); i++) {
		if(m_sources.at(i) == source) {
			// WARNING/TODO: A deadlock will occur if any slot listening to
			// this signal attempts to call another VideoSourceManager method.
			emit removingSource(source);
			m_sources.remove(i);
			break;
		}
	}
	//m_mutex.unlock();
}

void VideoSourceManager::queuedFrameEvent(uint frameNum, int numDropped)
{
	// Process all video sources. WARNING: We assume that we are always
	// executed before the profile renders this frame which is the case if we
	// properly connected our Qt signals in order.

	//m_mutex.lock();
	for(int i = 0; i < m_sources.count(); i++)
		m_sources.at(i)->prepareFrame(frameNum, numDropped);
	//m_mutex.unlock();
}

void VideoSourceManager::deviceChanged()
{
#ifdef Q_OS_WIN
	DShowVideoSource::repopulateSources(this);
#endif
}
