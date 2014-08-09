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

#ifndef AUDIOSOURCEMANAGER_H
#define AUDIOSOURCEMANAGER_H

#include "common.h"
//#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QVector>

class AudioSource;

typedef QVector<AudioSource *> AudioSourceList;

//=============================================================================
/// <summary>
/// Provides a database of all the available audio sources. This class does not
/// manage the allocation of sources however, it just manages the directory.
/// </summary>
class AudioSourceManager : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	bool			m_initialized;
	AudioSourceList	m_sources;
	//QMutex			m_mutex;

public: // Static methods -----------------------------------------------------
	static quint64	getDefaultIdOfType(AsrcType type);
	static AsrcType	getTypeOfDefaultId(quint64 sourceId);

public: // Constructor/destructor ---------------------------------------------
	AudioSourceManager();
	virtual ~AudioSourceManager();

public: // Methods ------------------------------------------------------------
	void			initialize();
	AudioSourceList	getSourceList() const;
	AudioSource *	getSource(quint64 sourceId);

	// Used by audio source factories only
	void			addSource(AudioSource *source);
	void			removeSource(AudioSource *source);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			sourceAdded(AudioSource *source);
	void			removingSource(AudioSource *source);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			realTimeTickEvent(int numDropped, int lateByUsec);
};
//=============================================================================

inline AudioSourceList AudioSourceManager::getSourceList() const
{
	return m_sources;
}

#endif // AUDIOSOURCEMANAGER_H
