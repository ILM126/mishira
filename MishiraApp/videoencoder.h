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

#ifndef VIDEOENCODER_H
#define VIDEOENCODER_H

#include "encodedframe.h"
#include "fraction.h"
#include "scaler.h"
#include <Libvidgfx/graphicscontext.h>
#include <QtCore/QObject>
#include <QtCore/QSize>

class Profile;

//=============================================================================
class VideoEncoder : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	Profile *		m_profile;
	VencType		m_type;
	int				m_ref;
	bool			m_isRunning;
	QSize			m_size;
	SclrScalingMode	m_scaling;
	GfxFilter		m_scaleFilter;
	Fraction		m_framerate; // Must not change after construction

public: // Constructor/destructor ---------------------------------------------
	VideoEncoder(
		Profile *profile, VencType type, QSize size, SclrScalingMode scaling,
		GfxFilter scaleFilter, Fraction framerate);
	virtual ~VideoEncoder();

public: // Methods ------------------------------------------------------------
	Profile *		getProfile() const;
	VencType		getType() const;
	quint32			getId();
	bool			refActivate();
	void			derefActivate();
	bool			isRunning() const;
	QSize			getSize() const;
	SclrScalingMode	getScaling() const;
	GfxFilter		getScaleFilter() const;
	Fraction		getFramerate() const;
	Fraction		getTimeBase() const;

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
	/// Forces that the next frame that will be passed in to the encoder can be
	/// placed at the beginning of a stream.
	/// </summary>
	virtual void forceKeyframe() = 0;

	/// <summary>
	/// When in low CPU usage mode the encoder is forced to use the least
	/// amount of CPU possible, overriding the user settings. This is used when
	/// the application is CPU-bound and cannot keep up with real time.
	/// </summary>
	virtual void setLowCPUUsageMode(bool enable) = 0;
	virtual bool isInLowCPUUsageMode() const = 0;

	/// <summary>
	/// Returns the settings of the encoder as a string for use in the UI.
	/// </summary>
	virtual QString getInfoString() const = 0;

	/// <summary>
	/// Returns the average bitrate that should be used for calculating buffer
	/// sizes for network congestion and low-lag network output in Kb/s.
	/// </summary>
	virtual int getAvgBitrateForCongestion() const = 0;

	virtual void serialize(QDataStream *stream) const;
	virtual bool unserialize(QDataStream *stream);

Q_SIGNALS: // Signals ---------------------------------------------------------
	//void	packetReady(EncodedPacket pkt);
	void	frameEncoded(EncodedFrame frame);
	void	encodeError(const QString &error);
};
//=============================================================================

inline Profile *VideoEncoder::getProfile() const
{
	return m_profile;
}

inline VencType VideoEncoder::getType() const
{
	return m_type;
}

inline bool VideoEncoder::isRunning() const
{
	return m_isRunning;
}

inline QSize VideoEncoder::getSize() const
{
	return m_size;
}

inline SclrScalingMode VideoEncoder::getScaling() const
{
	return m_scaling;
}

inline GfxFilter VideoEncoder::getScaleFilter() const
{
	return m_scaleFilter;
}

inline Fraction VideoEncoder::getFramerate() const
{
	return m_framerate;
}

inline Fraction VideoEncoder::getTimeBase() const
{
	return Fraction(m_framerate.denominator, m_framerate.numerator);
}

#endif // VIDEOENCODER_H
