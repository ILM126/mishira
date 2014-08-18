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

#include "audiosegment.h"

AudioSegment::AudioSegment()
	: m_data(NULL)
{
}

AudioSegment::AudioSegment(
	quint64 timestamp, const float *data, int numFloats)
	: m_data(new SegmentData)
{
	m_data->timestamp = timestamp;
	m_data->data = QByteArray::fromRawData(
		reinterpret_cast<const char *>(data), numFloats * sizeof(float));
	m_data->isPersistent = false;
	m_data->ref = 1;
}

AudioSegment::AudioSegment(
	quint64 timestamp, const QByteArray &persistentData)
	: m_data(new SegmentData)
{
	m_data->timestamp = timestamp;
	m_data->data = persistentData;
	m_data->isPersistent = true;
	m_data->ref = 1;
}

AudioSegment::AudioSegment(const AudioSegment &pkt)
	: m_data(pkt.m_data)
{
	if(m_data != NULL)
		m_data->ref++;
}

AudioSegment &AudioSegment::operator=(const AudioSegment &pkt)
{
	dereference();
	m_data = pkt.m_data;
	return *this;
}

AudioSegment::~AudioSegment()
{
	dereference();
}

void AudioSegment::dereference()
{
	if(m_data == NULL)
		return;
	m_data->ref--;
	if(m_data->ref)
		return;
	delete m_data;
	m_data = NULL;
}

void AudioSegment::makePersistent()
{
	if(m_data == NULL || m_data->isPersistent)
		return; // Already persistent

	// Create a deep copy of the original data
	m_data->data = QByteArray(m_data->data.constData(), m_data->data.size());
	m_data->isPersistent = true;
}
