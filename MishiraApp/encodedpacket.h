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

#ifndef ENCODEDPACKET_H
#define ENCODEDPACKET_H

#include "common.h"
#include <QtCore/QByteArray>
#include <QtCore/QVector>

class AudioEncoder;
class VideoEncoder;

//=============================================================================
/// <summary>
/// WARNING: This class is not thread-safe!
/// </summary>
class EncodedPacket
{
private: // Datatypes ---------------------------------------------------------
	struct PacketData {
		void *		encoder;
		PktType		type;
		PktPriority	priority;
		PktIdent	ident;
		bool		isBloat;
		QByteArray	data;
		bool		isPersistent;
		int			ref;
	};

private: // Members -----------------------------------------------------------
	PacketData *	m_data;

public: // Constructor/destructor ---------------------------------------------
	EncodedPacket();
	EncodedPacket(
		void *encoder, PktType type, PktPriority priority,
		PktIdent ident, bool isBloat, const char *data, int size);
	EncodedPacket(
		void *encoder, PktType type, PktPriority priority,
		PktIdent ident, bool isBloat, const QByteArray &persistentData);
	EncodedPacket(const EncodedPacket &pkt);
	EncodedPacket &operator=(const EncodedPacket &pkt);
	~EncodedPacket();

public: // Methods ------------------------------------------------------------
	bool			isValid() const;
	VideoEncoder *	videoEncoder() const;
	AudioEncoder *	audioEncoder() const;
	PktType			type() const;
	PktPriority		priority() const;
	PktIdent		ident() const;
	QString			identAsString() const;
	bool			isBloat() const;
	QByteArray		data() const;
	bool			isPersistent() const;
	void			makePersistent();

private:
	void			dereference();
};
//=============================================================================

typedef QVector<EncodedPacket> EncodedPacketList;

inline bool EncodedPacket::isValid() const
{
	return m_data != NULL;
}

inline VideoEncoder *EncodedPacket::videoEncoder() const
{
	if(m_data == NULL)
		return NULL;
	return static_cast<VideoEncoder *>(m_data->encoder);
}

inline AudioEncoder *EncodedPacket::audioEncoder() const
{
	if(m_data == NULL)
		return NULL;
	return static_cast<AudioEncoder *>(m_data->encoder);
}

inline PktType EncodedPacket::type() const
{
	if(m_data == NULL)
		return PktNullType;
	return m_data->type;
}

inline PktPriority EncodedPacket::priority() const
{
	if(m_data == NULL)
		return PktLowestPriority;
	return m_data->priority;
}

inline PktIdent EncodedPacket::ident() const
{
	if(m_data == NULL)
		return PktUnknownIdent;
	return m_data->ident;
}

inline bool EncodedPacket::isBloat() const
{
	if(m_data == NULL)
		return true;
	return m_data->isBloat;
}

/// <summary>
/// WARNING: The returned data may contain NUL characters! Always use the
/// length of the array instead of searching for NUL.
/// </summary>
inline QByteArray EncodedPacket::data() const
{
	if(m_data == NULL)
		return QByteArray();
	return m_data->data;
}

inline bool EncodedPacket::isPersistent() const
{
	if(m_data == NULL)
		return false;
	return m_data->isPersistent;
}

#endif // ENCODEDPACKET_H
