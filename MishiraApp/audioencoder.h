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

#ifndef AUDIOENCODER_H
#define AUDIOENCODER_H

#include "encodedsegment.h"
#include "fraction.h"
#include <QtCore/QObject>

class Profile;

//=============================================================================
class AudioEncoder : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	Profile *		m_profile;
	AencType		m_type;
	int				m_ref;
	bool			m_isRunning;

public: // Constructor/destructor ---------------------------------------------
	AudioEncoder(Profile *profile, AencType type);
	virtual ~AudioEncoder();

public: // Methods ------------------------------------------------------------
	Profile *		getProfile() const;
	AencType		getType() const;
	quint32			getId();
	bool			refActivate();
	void			derefActivate();
	bool			isRunning() const;

private: // Interface ---------------------------------------------------------

	/// <summary>
	/// Initialize the encoder and begin emitting encoded frames as soon as
	/// possible. Called once whenever the encoder is referenced by a target
	/// for the first time. If the encoder is still in the process of shutting
	/// down it will block until it is ready to be initialized.
	/// </summary>
	/// <returns>True if the encoder is ready to encode.</returns>
	virtual bool initializeEncoder() = 0;

	/// <summary>
	/// Shut down the encoder and free any system resources that it uses. If
	/// any frames are currently in the encoder's pipeline they will be
	/// discarded unless `flush` is true in which case this method will block
	/// until all frames have been transfered to system memory and fully
	/// encoded (WARNING: There can be over a second of frames in the
	/// pipeline!).
	/// </summary>
	virtual void shutdownEncoder(bool flush = false) = 0;

public:
	/// <summary>
	/// Forces the encoder to create a packet that can be placed at the
	/// beginning of a stream as soon as possible.
	/// </summary>
	virtual void forceKeyframe() = 0;

	/// <summary>
	/// Returns the settings of the encoder as a string for use in the UI.
	/// </summary>
	virtual QString getInfoString() const = 0;

	/// <summary>
	/// Returns the actual sample rate of the output data. Can be different
	/// than the input data. Valid only when the encoder is referenced.
	/// </summary>
	virtual int getSampleRate() const = 0;

	/// <summary>
	/// Returns the time base that each segment packet represents.
	/// </summary>
	virtual Fraction getTimeBase() const = 0;

	/// <summary>
	/// Returns the out-of-band extra data that is required for the decoder to
	/// be able to decode the stream.
	/// </summary>
	virtual QByteArray getOutOfBand() const = 0;

	/// <summary>
	/// Returns the number of samples per output frame (Samples per packet).
	/// </summary>
	virtual int getFrameSize() const = 0;

	/// <summary>
	/// Returns the number of samples that the output is delayed by.
	/// </summary>
	virtual int getDelay() const = 0;

	/// <summary>
	/// Returns the average bitrate that should be used for calculating buffer
	/// sizes for network congestion and low-lag network output in Kb/s.
	/// </summary>
	virtual int getAvgBitrateForCongestion() const = 0;

	virtual void serialize(QDataStream *stream) const;
	virtual bool unserialize(QDataStream *stream);

Q_SIGNALS: // Signals ---------------------------------------------------------
	//void	packetReady(EncodedPacket pkt);
	void	segmentEncoded(EncodedSegment segment);
};
//=============================================================================

inline Profile *AudioEncoder::getProfile() const
{
	return m_profile;
}

inline AencType AudioEncoder::getType() const
{
	return m_type;
}

inline bool AudioEncoder::isRunning() const
{
	return m_isRunning;
}

#endif // AUDIOENCODER_H
