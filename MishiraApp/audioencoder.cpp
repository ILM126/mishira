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

#include "audioencoder.h"
#include "profile.h"
#include <QtCore/QDataStream>

const QString LOG_CAT = QStringLiteral("Audio");

AudioEncoder::AudioEncoder(
	Profile *profile, AencType type)
	: m_profile(profile)
	, m_type(type)
	, m_ref(0)
	, m_isRunning(false)
{
}

AudioEncoder::~AudioEncoder()
{
	if(m_ref != 0) {
		appLog(LOG_CAT, Log::Warning) <<
			QStringLiteral("Destroying referenced audio encoder %1 (%L2 references)")
			.arg(getId())
			.arg(m_ref);
	}
}

quint32 AudioEncoder::getId()
{
	return m_profile->idOfAudioEncoder(this);
}

/// <summary>
/// Reference this audio encoder. If an encoder has one or more references it
/// activates and begins encoding segments.
/// </summary>
/// <returns>True if the encoder successfully activated</returns>
bool AudioEncoder::refActivate()
{
	m_ref++;
	if(m_ref == 1) {
		if(!initializeEncoder()) {
			m_ref--;
			return false;
		}
	}
	return true;
}

void AudioEncoder::derefActivate()
{
	if(m_ref <= 0) {
		appLog(LOG_CAT, Log::Warning) <<
			QStringLiteral("Audio encoder %1 was dereferenced more times than it was referenced")
			.arg(getId());
		return;
	}
	m_ref--;
	if(m_ref == 0)
		shutdownEncoder(); // TODO: Flush?
}

void AudioEncoder::serialize(QDataStream *stream) const
{
	// Write data version number
	*stream << (quint32)0;

	// We have no data to save
}

bool AudioEncoder::unserialize(QDataStream *stream)
{
	//quint32	uint32Data;

	// TODO: Make sure we cannot unserialize when the encoder is in use

	// Load defaults here if ever needed

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		// We have no data to load
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in audio encoder serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}
