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

#ifndef AUDIOSOURCE_H
#define AUDIOSOURCE_H

#include "audiosegment.h"
#include <QtCore/QObject>

class AudioMixer;

//=============================================================================
/// <summary>
/// Emits contiguous audio segments from a specific audio source in sequential
/// order when referenced by at least one user. As this class handles
/// resampling and format conversion all references must be using the same
/// sampling rate and internal format.
/// </summary>
class AudioSource : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	AsrcType	m_type;

protected: // Constructor/destructor ------------------------------------------
	AudioSource(AsrcType type);
public:
	virtual ~AudioSource();

public: // Methods ------------------------------------------------------------
	AsrcType			getType() const;

public: // Interface ----------------------------------------------------------
	virtual quint64		getId() const = 0;
	virtual quint64		getRealId() const = 0;
	virtual QString		getFriendlyName() const = 0;
	virtual QString		getDebugString() const = 0;
	virtual bool		isConnected() const = 0;
	virtual bool		reference(AudioMixer *mixer) = 0;
	virtual void		dereference() = 0;
	virtual int			getRefCount() = 0;
	virtual void		process(int numTicks) = 0;

Q_SIGNALS: // Signals ---------------------------------------------------------
	void				deviceConnected();
	void				deviceDisconnected();
	void				segmentReady(AudioSegment segment);
};
//=============================================================================

inline AsrcType AudioSource::getType() const
{
	return m_type;
}

#endif // AUDIOSOURCE_H
