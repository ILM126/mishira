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

#include "encodedsegment.h"
#include <QtCore/QStringBuilder>

EncodedSegment::EncodedSegment()
	: m_data(NULL)
{
}

EncodedSegment::EncodedSegment(
	AudioEncoder *encoder, EncodedPacketList pkts, bool isKeyframe,
	quint64 timestampMsec, qint64 pts)
	: m_data(new PacketData)
{
	m_data->encoder = encoder;
	m_data->packets = pkts;
	m_data->isKeyframe = isKeyframe;
	m_data->timestampMsec = timestampMsec;
	m_data->pts = pts;
	m_data->ref = 1;
}

EncodedSegment::EncodedSegment(const EncodedSegment &segment)
	: m_data(segment.m_data)
{
	if(m_data != NULL)
		m_data->ref++;
}

EncodedSegment &EncodedSegment::operator=(const EncodedSegment &segment)
{
	dereference();
	m_data = segment.m_data;
	if(m_data != NULL)
		m_data->ref++;
	return *this;
}

EncodedSegment::~EncodedSegment()
{
	dereference();
}

bool EncodedSegment::isPersistent() const
{
	if(m_data == NULL)
		return false;
	for(int i = 0; i < m_data->packets.count(); i++) {
		if(!m_data->packets.at(i).isPersistent())
			return false;
	}
	return true;
}

void EncodedSegment::makePersistent()
{
	if(m_data == NULL)
		return;
	for(int i = 0; i < m_data->packets.count(); i++)
		m_data->packets[i].makePersistent();
}

QString EncodedSegment::getDebugString() const
{
	// Segment(Key=0 TS=12345678 PTS=1234 Pkts=[FRM FRM FRM] Size=12345 MinSize=12345)
	if(m_data == NULL)
		return QStringLiteral("Segment(NULL)");
	int maxSize = 0;
	int minSize = 0;
	QString pkts;
	for(int i = 0; i < m_data->packets.count(); i++) {
		const EncodedPacket &pkt = m_data->packets.at(i);
		int pktSize = pkt.data().size();
		maxSize += pktSize;
		if(!pkt.isBloat())
			minSize += pktSize;
		if(pkts.isEmpty())
			pkts = pkt.identAsString();
		else
			pkts = pkts % QStringLiteral(" ") % pkt.identAsString();
	}
	return QStringLiteral(
		"Segment(Key=%L1 TS=%L2 PTS=%L3 Pkts=[%5] MaxSize=%L6 MinSize=%L7)")
		.arg(m_data->isKeyframe ? 1 : 0)
		.arg(m_data->timestampMsec)
		.arg(m_data->pts)
		.arg(pkts)
		.arg(maxSize)
		.arg(minSize);
}

void EncodedSegment::dereference()
{
	if(m_data == NULL)
		return;
	m_data->ref--;
	if(m_data->ref)
		return;
	delete m_data;
	m_data = NULL;
}
