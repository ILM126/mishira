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

#include "encodedpacket.h"

EncodedPacket::EncodedPacket()
	: m_data(NULL)
{
}

EncodedPacket::EncodedPacket(
	void *encoder, PktType type, PktPriority priority, PktIdent ident,
	bool isBloat, const char *data, int size)
	: m_data(new PacketData)
{
	m_data->encoder = encoder;
	m_data->type = type;
	m_data->priority = priority;
	m_data->ident = ident;
	m_data->isBloat = isBloat;
	m_data->data = QByteArray::fromRawData(data, size);
	m_data->isPersistent = false;
	m_data->ref = 1;
}

EncodedPacket::EncodedPacket(
	void *encoder, PktType type, PktPriority priority, PktIdent ident,
	bool isBloat, const QByteArray &persistentData)
	: m_data(new PacketData)
{
	m_data->encoder = encoder;
	m_data->type = type;
	m_data->priority = priority;
	m_data->ident = ident;
	m_data->isBloat = isBloat;
	m_data->data = persistentData;
	m_data->isPersistent = true;
	m_data->ref = 1;
}

EncodedPacket::EncodedPacket(const EncodedPacket &pkt)
	: m_data(pkt.m_data)
{
	if(m_data != NULL)
		m_data->ref++;
}

EncodedPacket &EncodedPacket::operator=(const EncodedPacket &pkt)
{
	dereference();
	m_data = pkt.m_data;
	if(m_data != NULL)
		m_data->ref++;
	return *this;
}

EncodedPacket::~EncodedPacket()
{
	dereference();
}

QString EncodedPacket::identAsString() const
{
	if(m_data == NULL)
		return QStringLiteral("Unknown");
	switch(m_data->ident) {
	default:
	case PktUnknownIdent:
		return QStringLiteral("Unknown");
	case PktH264Ident_SLICE:
		return QStringLiteral("SLICE");
	case PktH264Ident_SLICE_DPA:
		return QStringLiteral("DPA");
	case PktH264Ident_SLICE_DPB:
		return QStringLiteral("DPB");
	case PktH264Ident_SLICE_DPC:
		return QStringLiteral("DPC");
	case PktH264Ident_SLICE_IDR:
		return QStringLiteral("IDR");
	case PktH264Ident_SEI:
		return QStringLiteral("SEI");
	case PktH264Ident_SPS:
		return QStringLiteral("SPS");
	case PktH264Ident_PPS:
		return QStringLiteral("PPS");
	case PktH264Ident_AUD:
		return QStringLiteral("AUD");
	case PktH264Ident_FILLER:
		return QStringLiteral("FILLER");
	case PktFdkAacIdent_FRM:
		return QStringLiteral("FRM");
	}
}

void EncodedPacket::dereference()
{
	if(m_data == NULL)
		return;
	m_data->ref--;
	if(m_data->ref)
		return;
	delete m_data;
	m_data = NULL;
}

void EncodedPacket::makePersistent()
{
	if(m_data == NULL || m_data->isPersistent)
		return; // Already persistent

	// Create a deep copy of the original data
	m_data->data = QByteArray(m_data->data.constData(), m_data->data.size());
	m_data->isPersistent = true;
}
