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

#ifndef SCALER_H
#define SCALER_H

#include "common.h"
#include <QtCore/QObject>

class Profile;

/// <summary>
/// The number of frames that the scaler delays by in order to prevent blocking
/// the CPU.
/// </summary>
const int SCALER_DELAY = 2;

//=============================================================================
struct NV12Frame
{
	/// <summary>
	/// A buffer of the packed Y components for this frame. The buffer has the
	/// same width and height as the output size.
	/// </summary>
	quint8 *	yPlane;

	/// <summary>
	/// A buffer of the packed U and V components for this frame. The buffer
	/// has the same width as the output in bytes but only half the height.
	/// </summary>
	quint8 *	uvPlane;

	/// <summary>
	/// The row stride of the Y buffer. This is the number of bytes between
	/// adjacent rows in the buffer.
	/// </summary>
	int			yStride;

	/// <summary>
	/// The row stride of the UV buffer. This is the number of bytes between
	/// adjacent rows in the buffer.
	/// </summary>
	int			uvStride;
};
//=============================================================================

//=============================================================================
class Scaler : public QObject
{
	Q_OBJECT

private: // Static members ----------------------------------------------------
	static QVector<Scaler *>	s_instances;

private: // Members -----------------------------------------------------------
	Profile *		m_profile;
	QSize			m_size;
	SclrScalingMode	m_scaling;
	GfxFilter		m_scaleFilter;
	GfxPixelFormat	m_pixelFormat;
	int				m_ref;

	VidgfxVertBuf *	m_quarterWidthBuf;
	QPointF			m_quarterWidthBufBrUv;
	VidgfxVertBuf *	m_nv12Buf;
	VidgfxTex *		m_yuvScratchTex[3];
	VidgfxTex *		m_stagingYTex[2];
	VidgfxTex *		m_stagingUVTex[2];
	uint			m_delayedFrameNum[2];
	int				m_delayedNumDropped[2];
	int				m_prevStagingTex;
	int				m_startDelay;

public: // Static methods -----------------------------------------------------
	static Scaler *	getOrCreate(
		Profile *profile, QSize size, SclrScalingMode scaling,
		GfxFilter scaleFilter, GfxPixelFormat pixelFormat);
	static void		graphicsContextInitialized(VidgfxContext *gfx);

private: // Constructor/destructor ---------------------------------------------
	Scaler(
		Profile *profile, QSize size, SclrScalingMode scaling,
		GfxFilter scaleFilter, GfxPixelFormat pixelFormat);
	~Scaler();

public: // Methods ------------------------------------------------------------
	Profile *		getProfile() const;
	QSize			getSize() const;
	SclrScalingMode	getScaling() const;
	GfxFilter		getScaleFilter() const;
	GfxPixelFormat	getPixelFormat() const;

	void			release();

private:
	void			updateVertBuf(VidgfxContext *gfx, const QPointF &brUv);
	LyrScalingMode	convertScaling(SclrScalingMode scaling) const;

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	nv12FrameReady(
		const NV12Frame &frame, uint frameNum, int numDropped);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	frameRendered(VidgfxTex *tex, uint frameNum, int numDropped);
	void	initializeResources(VidgfxContext *gfx);
	void	destroyResources(VidgfxContext *gfx);
};
//=============================================================================

inline Profile *Scaler::getProfile() const
{
	return m_profile;
}

inline QSize Scaler::getSize() const
{
	return m_size;
}

inline SclrScalingMode Scaler::getScaling() const
{
	return m_scaling;
}

inline GfxFilter Scaler::getScaleFilter() const
{
	return m_scaleFilter;
}

inline GfxPixelFormat Scaler::getPixelFormat() const
{
	return m_pixelFormat;
}

//=============================================================================
/// <summary>
/// A fake `Scaler`-like object that emits procedurally generated frames when
/// requested to.
/// </summary>
class TestScaler : public QObject
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	QSize		m_size;
	quint8 *	m_yBuf;
	quint8 *	m_uvBuf;

public: // Constructor/destructor ---------------------------------------------
	TestScaler(QSize size);
	~TestScaler();

public: // Methods ------------------------------------------------------------
	QSize	getSize() const;
	void	emitFrameRendered(uint frameNum, int numDropped);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	nv12FrameReady(
		const NV12Frame &frame, uint frameNum, int numDropped);
};
//=============================================================================

#endif // SCALER_H
