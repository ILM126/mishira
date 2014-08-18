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

#ifndef DSHOWVIDEOSOURCE_H
#define DSHOWVIDEOSOURCE_H

#include "videosource.h"
#include "dshowfilters.h"
#include <QtCore/QMutex>

struct IBaseFilter;
struct IGraphBuilder;
class VideoSourceManager;

struct DShowMode {
	AM_MEDIA_TYPE	type;
	QSize			size;
	float			framerate;
};
typedef QVector<DShowMode> DShowModeList;

//=============================================================================
// Additional GUID definitions

#ifdef INIT_OUR_GUID
#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#else
#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID FAR name
#endif // INIT_OUR_GUID

//-----------------------------------------------------------------------------
// Additional media subtypes that are not defined by default

// 30323449-0000-0010-8000-00AA00389B71  'YV12' ==  MEDIASUBTYPE_I420
OUR_GUID_ENTRY(
	MEDIASUBTYPE_I420,
	0x30323449, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

// 43594448-0000-0010-8000-00AA00389B71  'HDYC' ==  MEDIASUBTYPE_HDYC
OUR_GUID_ENTRY(
	MEDIASUBTYPE_HDYC,
	0x43594448, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

//-----------------------------------------------------------------------------
// Some devices (Such as those made by Roxio) use an alternative pin category
// GUID for video capture

// 6994ad05-93ef-11d0-a3cc-00a0c9223196
OUR_GUID_ENTRY(
	PIN_CATEGORY_CAPTURE_ALT,
	0x6994ad05, 0x93ef, 0x11d0, 0xa3, 0xcc, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96);

//-----------------------------------------------------------------------------

#undef OUR_GUID_ENTRY

//=============================================================================
/// <summary>
/// Provides sample buffering so that they can be used by the applicatioin in a
/// queued manner. As samples are received in a different thread ("Streaming
/// thread") than they are used ("Application thread") the buffer is protected
/// by a mutex. If the graph is stopped or flushed (Caused by a seek) all
/// buffered frames are also flushed.
///
/// WARNING: Right now only the last received sample is buffered!
/// </summary>
class DShowRenderer : public CBaseRenderer
{
private: // Members -----------------------------------------------------------
	QMutex			m_mutex;
	GfxPixelFormat	m_pixelFormat;
	IMediaSample *	m_curSample;
	uint			m_curSampleNum;

public: // Static methods -----------------------------------------------------
	static GfxPixelFormat	formatFromSubtype(const GUID &subtype);
	static int				compareFormats(GfxPixelFormat a, GfxPixelFormat b);
	static void				parseMediaType(
		const AM_MEDIA_TYPE &type, QSize *size, float *framerate,
		bool *negHeight);

public: // Constructor/destructor ---------------------------------------------
	DShowRenderer(GfxPixelFormat pixelFormat, HRESULT *phr);
	virtual ~DShowRenderer();

public: // Methods ------------------------------------------------------------
	IMediaSample *	lockCurSample(uint *sampleNumOut);
	void			unlockCurSample();

public: // Interface ----------------------------------------------------------
	virtual HRESULT	CheckMediaType(const CMediaType *pmt);
	virtual HRESULT	DoRenderSample(IMediaSample *pMediaSample);
	virtual void	OnReceiveFirstSample(IMediaSample *pMediaSample);
	virtual HRESULT	ShouldDrawSampleNow(
		IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime,
		REFERENCE_TIME *pEndTime);
};
//=============================================================================

//=============================================================================
class DShowVideoSource : public VideoSource
{
	Q_OBJECT

private: // Static members ----------------------------------------------------
	static QVector<quint64>	s_activeIds;

private: // Members -----------------------------------------------------------
	quint64				m_id;
	QString				m_friendlyName;
	int					m_ref;
	IBaseFilter *		m_filter;
	DShowRenderer *		m_renderFilter;
	IPin *				m_bestOutPin;
	GfxPixelFormat		m_bestOutPinFormat;
	DShowModeList		m_availModes;
	IMediaControl *		m_control;
	IAMStreamConfig *	m_config;

	float			m_targetFramerate;
	QSize			m_targetSize;

	IGraphBuilder *	m_graph;
	DWORD			m_graphRotId;

	bool			m_isFlipped;
	IMediaSample *	m_curSample;
	uint			m_curSampleNum;
	bool			m_curSampleProcessed;
	GfxPixelFormat	m_curFormat;
	VidgfxTex *		m_curPlaneA;
	VidgfxTex *		m_curPlaneB;
	VidgfxTex *		m_curPlaneC;
	VidgfxTex *		m_curTexture;

public: // Static methods -----------------------------------------------------
	static void			populateSources(
		VideoSourceManager *mgr, bool isRepopulate = false);
	static void			repopulateSources(VideoSourceManager *mgr);

protected: // Constructor/destructor ------------------------------------------
	DShowVideoSource(IBaseFilter *filter, quint64 id, QString friendlyName);
public:
	virtual ~DShowVideoSource();

private: // Methods -----------------------------------------------------------
	bool				initialize();
	void				shutdown();

	bool				recreateFilter();
	void				findBestOutputPin();
	void				fetchAllOutputModes();
	void				convertSampleToTexture(IMediaSample *sample);

	void				clearModeList();
	bool				doesModeAlreadyExist(const DShowMode &mode);
	void				setTargetModeToDefault();
	AM_MEDIA_TYPE *		getClosestTypeToMode(
		float framerate, const QSize &size);
	bool				configureBestPinToTargetMode();

public: // Interface ----------------------------------------------------------
	virtual quint64		getId() const;
	virtual QString		getFriendlyName() const;
	virtual QString		getDebugString() const;
	virtual bool		reference();
	virtual void		dereference();
	virtual int			getRefCount();
	virtual void		prepareFrame(uint frameNum, int numDropped);
	virtual VidgfxTex *	getCurrentFrame();
	virtual bool		isFrameFlipped() const;
	virtual QVector<float>	getFramerates() const;
	virtual QVector<QSize>	getSizesForFramerate(float framerate) const;
	virtual void		setMode(float framerate, const QSize &size);
	virtual float		getModeFramerate();
	virtual QSize		getModeSize();
	virtual void		showConfigureDialog(QWidget *parent = NULL);
};
//=============================================================================

#endif // DSHOWVIDEOSOURCE_H
