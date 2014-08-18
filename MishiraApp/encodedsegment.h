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

#ifndef ENCODEDSEGMENT_H
#define ENCODEDSEGMENT_H

#include "encodedpacket.h"
#include <QtCore/QVector>

class AudioEncoder;

//=============================================================================
/// <summary>
/// Unlike the `EncodedFrame` which has a separate object for every individual
/// video frame an `EncodedSegment` can contain multiple "audio frames" in a
/// single object. Each packet in this object represents a separate audio frame
/// that has it's own timestamp and PTS value and can be separated from the
/// segment if required (For example for synchronisation with a video stream).
///
/// WARNING: This class is not thread-safe!
///
/// WARNING: The data in this class MUST be immutable as multiple targets can
/// share the same object. This means that any modifications to the data (E.g.
/// padding dropping) must be done by creating a brand new object or by having
/// a flag that is stored elsewhere.
/// </summary>
class EncodedSegment
{
private: // Datatypes ---------------------------------------------------------
	struct PacketData {
		AudioEncoder *		encoder;
		EncodedPacketList	packets;

		// Can be placed at the very start of the output stream after any
		// required headers
		bool				isKeyframe;

		// The real-time timestamp in milliseconds since the application's
		// "frame origin". This is relative to an arbitrary point in time so
		// targets should only do relative time calculations using it. All
		// encoded video frames and audio segments are relative to the same
		// origin. This should only be used for synchronising video and audio
		// at the beginning of a broadcast target.
		quint64				timestampMsec;

		// Number of `encoder->getTimeBase()` units since encoder start for the
		// first packet in the segment. Each packet in the segment has an
		// incrementing pts value.
		qint64				pts;

		int					ref;
	};

private: // Members -----------------------------------------------------------
	PacketData *	m_data;

public: // Constructor/destructor ---------------------------------------------
	EncodedSegment();
	EncodedSegment(
		AudioEncoder *encoder, EncodedPacketList pkts, bool isKeyframe,
		quint64 timestampMsec, qint64 pts);
	EncodedSegment(const EncodedSegment &segment);
	EncodedSegment &operator=(const EncodedSegment &segment);
	~EncodedSegment();

public: // Methods ------------------------------------------------------------
	bool				isValid() const;
	AudioEncoder *		encoder() const;
	EncodedPacketList	getPackets() const;
	bool				isKeyframe() const;
	quint64				getTimestampMsec() const;
	qint64				getPTS() const;
	bool				isPersistent() const;
	void				makePersistent();
	QString				getDebugString() const;

private:
	void				dereference();
};
//=============================================================================

inline bool EncodedSegment::isValid() const
{
	return m_data != NULL;
}

inline AudioEncoder *EncodedSegment::encoder() const
{
	if(m_data == NULL)
		return NULL;
	return m_data->encoder;
}

inline EncodedPacketList EncodedSegment::getPackets() const
{
	if(m_data == NULL)
		return EncodedPacketList();
	return m_data->packets;
}

inline bool EncodedSegment::isKeyframe() const
{
	if(m_data == NULL)
		return false;
	return m_data->isKeyframe;
}

inline quint64 EncodedSegment::getTimestampMsec() const
{
	if(m_data == NULL)
		return 0;
	return m_data->timestampMsec;
}

inline qint64 EncodedSegment::getPTS() const
{
	if(m_data == NULL)
		return 0;
	return m_data->pts;
}

#endif // ENCODEDSEGMENT_H
