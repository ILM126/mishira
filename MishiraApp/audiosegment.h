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

#ifndef AUDIOSEGMENT_H
#define AUDIOSEGMENT_H

#include "common.h"
#include <QtCore/QByteArray>

//=============================================================================
/// <summary>
/// WARNING: This class is not thread-safe!
/// </summary>
class AudioSegment
{
private: // Datatypes ---------------------------------------------------------
	struct SegmentData {
		// The time that the first sample was recorded in microseconds after
		// the first video frame (Frame 0). It is up to the method that creates
		// this object to handle negative timestamps (This is unsigned!) and
		// prevent a timestamp value of "0" which has a special meaning to
		// encoders and targets.
		quint64		timestamp;

		QByteArray	data;
		bool		isPersistent;
		int			ref;
	};

private: // Members -----------------------------------------------------------
	SegmentData *	m_data;

public: // Constructor/destructor ---------------------------------------------
	AudioSegment();
	AudioSegment(quint64 timestamp, const float *data, int numFloats);
	AudioSegment(quint64 timestamp, const QByteArray &persistentData);
	AudioSegment(const AudioSegment &pkt);
	AudioSegment &operator=(const AudioSegment &pkt);
	~AudioSegment();

public: // Methods ------------------------------------------------------------
	quint64			timestamp() const;
	QByteArray		data() const;
	const float *	floatData() const;
	int				numFloats() const;
	bool			isPersistent() const;
	void			makePersistent();

private:
	void			dereference();
};
//=============================================================================

inline quint64 AudioSegment::timestamp() const
{
	if(m_data == NULL)
		return true;
	return m_data->timestamp;
}

/// <summary>
/// WARNING: The returned data may contain NUL characters! Always use the
/// length of the array instead of searching for NUL.
/// </summary>
inline QByteArray AudioSegment::data() const
{
	if(m_data == NULL)
		return QByteArray();
	return m_data->data;
}

inline const float *AudioSegment::floatData() const
{
	if(m_data == NULL)
		return NULL;
	return reinterpret_cast<const float *>(m_data->data.constData());
}

inline int AudioSegment::numFloats() const
{
	if(m_data == NULL)
		return 0;
	return m_data->data.size() / sizeof(float);
}

inline bool AudioSegment::isPersistent() const
{
	if(m_data == NULL)
		return false;
	return m_data->isPersistent;
}

#endif // AUDIOSEGMENT_H
