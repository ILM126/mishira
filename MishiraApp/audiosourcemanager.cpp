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

#include "audiosourcemanager.h"
#include "application.h"
#include "wasapiaudiosource.h"

quint64 AudioSourceManager::getDefaultIdOfType(AsrcType type)
{
	switch(type) {
	case AsrcOutputType:
		return 1; // Default speakers
	case AsrcInputType:
		return 2; // Default microphone
	default:
		return 0;
	}
	return 0;
}

/// <summary>
/// Returns the audio source type if the specified source ID is a special ID
/// that represents a default device or `NUM_AUDIO_SOURCE_TYPES` otherwise.
/// </summary>
AsrcType AudioSourceManager::getTypeOfDefaultId(quint64 sourceId)
{
	switch(sourceId) {
	case 1: // Default speakers
		return AsrcOutputType;
	case 2: // Default microphone
		return AsrcInputType;
	default:
		return NUM_AUDIO_SOURCE_TYPES;
	}
	return NUM_AUDIO_SOURCE_TYPES;
}

AudioSourceManager::AudioSourceManager()
	: QObject()
	, m_initialized(false)
	, m_sources()
	//, m_mutex(QMutex::Recursive)
{
}

AudioSourceManager::~AudioSourceManager()
{
	// Cleanly delete sources
	while(!m_sources.isEmpty())
		removeSource(m_sources.first());
}

void AudioSourceManager::initialize()
{
	if(m_initialized)
		return; // Already initialized
#ifdef Q_OS_WIN
	WASAPIAudioSource::populateSources(this);
#endif

	// Log available audio sources
	appLog() << "Available audio sources:";
	for(int i = 0; i < m_sources.count(); i++) {
		appLog() << "  - " << m_sources.at(i)->getDebugString();
	}

	// Connect to main loop
	connect(App, &Application::realTimeTickEvent,
		this, &AudioSourceManager::realTimeTickEvent);
}

AudioSource *AudioSourceManager::getSource(quint64 sourceId)
{
	//m_mutex.lock();
	for(int i = 0; i < m_sources.count(); i++) {
		AudioSource *src = m_sources.at(i);
		if(src->getId() == sourceId || src->getRealId() == sourceId) {
			//m_mutex.unlock();
			return src;
		}
	}
	//m_mutex.unlock();
	return NULL;
}

/// <summary>
/// Registers an audio source with the manager.
/// </summary>
void AudioSourceManager::addSource(AudioSource *source)
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
/// Unregisters an audio source from the manager.
/// </summary>
void AudioSourceManager::removeSource(AudioSource *source)
{
	if(source == NULL)
		return;
	//m_mutex.lock();
	for(int i = 0; i < m_sources.count(); i++) {
		if(m_sources.at(i) == source) {
			// WARNING/TODO: A deadlock will occur if any slot listening to
			// this signal attempts to call another AudioSourceManager method.
			emit removingSource(source);
			m_sources.remove(i);
			break;
		}
	}
	//m_mutex.unlock();
}

void AudioSourceManager::realTimeTickEvent(int numDropped, int lateByUsec)
{
	// Process all audio sources
	//m_mutex.lock();
	for(int i = 0; i < m_sources.count(); i++)
		m_sources.at(i)->process(numDropped + 1);
	//m_mutex.unlock();
}
