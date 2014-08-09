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

#include "encodedframe.h"
#include "videoencoder.h"
#include <QtCore/QStringBuilder>

EncodedFrame::EncodedFrame()
	: m_data(NULL)
{
}

EncodedFrame::EncodedFrame(
	VideoEncoder *encoder, EncodedPacketList pkts, bool isKeyframe,
	FrmPriority priority, quint64 timestampMsec, qint64 pts, qint64 dts)
	: m_data(new PacketData)
{
	m_data->encoder = encoder;
	m_data->packets = pkts;
	m_data->isKeyframe = isKeyframe;
	m_data->priority = priority;
	m_data->timestampMsec = timestampMsec;
	m_data->pts = pts;
	m_data->dts = dts;
	m_data->ref = 1;
}

EncodedFrame::EncodedFrame(const EncodedFrame &frame)
	: m_data(frame.m_data)
{
	if(m_data != NULL)
		m_data->ref++;
}

EncodedFrame &EncodedFrame::operator=(const EncodedFrame &frame)
{
	dereference();
	m_data = frame.m_data;
	if(m_data != NULL)
		m_data->ref++;
	return *this;
}

EncodedFrame::~EncodedFrame()
{
	dereference();
}

qint64 EncodedFrame::getDTSTimestampMsec() const
{
	if(m_data == NULL)
		return 0;
	Fraction tbase = m_data->encoder->getTimeBase();
	// TODO: This currently doesn't produce the exact same timestamps as the
	// PTS for the time time due to rounding
	return m_data->timestampMsec +
		(((qint64)m_data->dts * 1000LL * (qint64)tbase.numerator) /
		(qint64)tbase.denominator -
		((qint64)m_data->pts * 1000LL * (qint64)tbase.numerator) /
		(qint64)tbase.denominator);
}

bool EncodedFrame::isPersistent() const
{
	if(m_data == NULL)
		return false;
	for(int i = 0; i < m_data->packets.count(); i++) {
		if(!m_data->packets.at(i).isPersistent())
			return false;
	}
	return true;
}

void EncodedFrame::makePersistent()
{
	if(m_data == NULL)
		return;
	for(int i = 0; i < m_data->packets.count(); i++)
		m_data->packets[i].makePersistent();
}

QString EncodedFrame::getDebugString() const
{
	// Frame(Key=0 TS=12345678 PTS=1234 DTS=1234 Prior=Highest Pkts=[SPS PPS SEI SLICE] Size=12345 MinSize=12345)
	if(m_data == NULL)
		return QStringLiteral("Frame(NULL)");
	int maxSize = 0;
	int minSize = 0;

	QString prior;
	switch(m_data->priority) {
	default:
	case FrmLowestPriority:
		prior = QStringLiteral("Lowest");
		break;
	case FrmLowPriority:
		prior = QStringLiteral("Low");
		break;
	case FrmHighPriority:
		prior = QStringLiteral("High");
		break;
	case FrmHighestPriority:
		prior = QStringLiteral("Highest");
		break;
	}

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
		"Frame(Key=%L1 TS=%L2/%L3 PTS=%L4 DTS=%L5 Prior=%6 Pkts=[%7] MaxSize=%L8 MinSize=%L9)")
		.arg(m_data->isKeyframe ? 1 : 0)
		.arg(m_data->timestampMsec)
		.arg(getDTSTimestampMsec())
		.arg(m_data->pts)
		.arg(m_data->dts)
		.arg(prior)
		.arg(pkts)
		.arg(maxSize)
		.arg(minSize);
}

void EncodedFrame::dereference()
{
	if(m_data == NULL)
		return;
	m_data->ref--;
	if(m_data->ref)
		return;
	delete m_data;
	m_data = NULL;
}
