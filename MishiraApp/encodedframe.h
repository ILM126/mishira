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

#ifndef ENCODEDFRAME_H
#define ENCODEDFRAME_H

#include "encodedpacket.h"
#include <QtCore/QVector>

class VideoEncoder;

//=============================================================================
/// <summary>
/// WARNING: This class is not thread-safe!
///
/// WARNING: The data in this class MUST be immutable as multiple targets can
/// share the same object. This means that any modifications to the data (E.g.
/// padding dropping) must be done by creating a brand new object or by having
/// a flag that is stored elsewhere.
/// </summary>
class EncodedFrame
{
private: // Datatypes ---------------------------------------------------------
	struct PacketData {
		VideoEncoder *		encoder;
		EncodedPacketList	packets;

		// Can be placed at the very start of the output stream after any
		// required headers (E.g. IDR)
		bool				isKeyframe;

		// The priority of the frame for dropping purposes. All frames of lower
		// priority are dropped before the next higher priority starts to be
		// dropped.
		FrmPriority			priority;

		// The real-time timestamp in milliseconds since the application's
		// "frame origin". This is relative to an arbitrary point in time so
		// targets should only do relative time calculations using it. All
		// encoded video frames and audio segments are relative to the same
		// origin. This should only be used for synchronising video and audio
		// at the beginning of a broadcast target.
		quint64				timestampMsec;

		// Number of `encoder->getTimeBase()` units since encoder start
		qint64				pts;

		// Number of `encoder->getTimeBase()` units since encoder start. Can
		// be negative which might break some muxers.
		qint64				dts;

		int					ref;
	};

private: // Members -----------------------------------------------------------
	PacketData *	m_data;

public: // Constructor/destructor ---------------------------------------------
	EncodedFrame();
	EncodedFrame(
		VideoEncoder *encoder, EncodedPacketList pkts, bool isKeyframe,
		FrmPriority priority, quint64 timestampMsec, qint64 pts, qint64 dts);
	EncodedFrame(const EncodedFrame &frame);
	EncodedFrame &operator=(const EncodedFrame &frame);
	~EncodedFrame();

public: // Methods ------------------------------------------------------------
	bool				isValid() const;
	VideoEncoder *		encoder() const;
	EncodedPacketList	getPackets() const;
	bool				isKeyframe() const;
	FrmPriority			getPriority() const;
	quint64				getTimestampMsec() const;
	qint64				getDTSTimestampMsec() const;
	qint64				getPTS() const;
	qint64				getDTS() const;
	bool				isPersistent() const;
	void				makePersistent();
	QString				getDebugString() const;

private:
	void				dereference();
};
//=============================================================================

typedef QVector<EncodedFrame> EncodedFrameList;

inline bool EncodedFrame::isValid() const
{
	return m_data != NULL;
}

inline VideoEncoder *EncodedFrame::encoder() const
{
	if(m_data == NULL)
		return NULL;
	return m_data->encoder;
}

inline EncodedPacketList EncodedFrame::getPackets() const
{
	if(m_data == NULL)
		return EncodedPacketList();
	return m_data->packets;
}

inline bool EncodedFrame::isKeyframe() const
{
	if(m_data == NULL)
		return false;
	return m_data->isKeyframe;
}

inline FrmPriority EncodedFrame::getPriority() const
{
	if(m_data == NULL)
		return FrmLowestPriority;
	return m_data->priority;
}

inline quint64 EncodedFrame::getTimestampMsec() const
{
	if(m_data == NULL)
		return 0;
	return m_data->timestampMsec;
}

inline qint64 EncodedFrame::getPTS() const
{
	if(m_data == NULL)
		return 0;
	return m_data->pts;
}

inline qint64 EncodedFrame::getDTS() const
{
	if(m_data == NULL)
		return 0;
	return m_data->dts;
}

#endif // ENCODEDFRAME_H
