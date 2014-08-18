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

#ifndef MONITORLAYER_H
#define MONITORLAYER_H

#include "common.h"
#include "cropinfo.h"
#include "layer.h"
#include <Libvidgfx/graphicscontext.h>
#include <QtCore/QStringList>
#include <QtCore/QMargins>

class CaptureObject;
class LayerDialog;

//=============================================================================
class MonitorLayer : public Layer
{
	friend class LayerGroup;

private: // Members -----------------------------------------------------------
	CaptureObject *	m_captureObj;
	QSize			m_curSize;
	bool			m_curFlipped;
	VertexBuffer *	m_vertBuf;
	QRectF			m_vertBufRect;
	QPointF			m_vertBufTlUv;
	QPointF			m_vertBufBrUv;
	bool			m_vertBufFlipped;
	VertexBuffer *	m_cursorVertBuf;
	QRectF			m_cursorVertBufRect;
	QPointF			m_cursorVertBufBrUv;
	bool			m_monitorChanged;
	bool			m_aeroDisableReffed;

	// Settings
	int				m_monitor;
	bool			m_captureMouse;
	bool			m_disableAero;
	CptrMethod		m_captureMethod;
	CropInfo		m_cropInfo;
	float			m_gamma; // [0.1, 10.0]
	int				m_brightness; // [-250, 250]
	int				m_contrast; // [-100, 400]
	int				m_saturation; // [-100, 400]

private: // Constructor/destructor --------------------------------------------
	MonitorLayer(LayerGroup *parent);
	~MonitorLayer();

public: // Methods ------------------------------------------------------------
	void		setMonitor(int monitor);
	int			getMonitor() const;
	void		setCaptureMouse(bool capture);
	bool		getCaptureMouse() const;
	void		setDisableAero(bool disableAero);
	bool		getDisableAero() const;
	void		setCaptureMethod(CptrMethod method);
	CptrMethod	getCaptureMethod() const;
	void		setCropInfo(const CropInfo &info);
	CropInfo	getCropInfo() const;
	void		setGamma(float gamma);
	float		getGamma() const;
	void		setBrightness(int brightness);
	int			getBrightness() const;
	void		setContrast(int contrast);
	int			getContrast() const;
	void		setSaturation(int saturation);
	int			getSaturation() const;

private:
	void		updateVertBuf(
		GraphicsContext *gfx, const QPointF &tlUv, const QPointF &brUv);
	void		updateCursorVertBuf(
		GraphicsContext *gfx, const QPointF &brUv, const QRect &relRect);
	void		updateDisableAero();

public: // Interface ----------------------------------------------------------
	virtual void	initializeResources(GraphicsContext *gfx);
	virtual void	updateResources(GraphicsContext *gfx);
	virtual void	destroyResources(GraphicsContext *gfx);
	virtual void	render(
		GraphicsContext *gfx, Scene *scene, uint frameNum, int numDropped);

	virtual LyrType	getType() const;

	virtual bool			hasSettingsDialog();
	virtual LayerDialog *	createSettingsDialog(QWidget *parent = NULL);

	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			monitorInfoChanged();
};
//=============================================================================

inline int MonitorLayer::getMonitor() const
{
	return m_monitor;
}

inline bool MonitorLayer::getCaptureMouse() const
{
	return m_captureMouse;
}

inline bool MonitorLayer::getDisableAero() const
{
	return m_disableAero;
}

inline CptrMethod MonitorLayer::getCaptureMethod() const
{
	return m_captureMethod;
}

inline CropInfo MonitorLayer::getCropInfo() const
{
	return m_cropInfo;
}

inline float MonitorLayer::getGamma() const
{
	return m_gamma;
}

inline int MonitorLayer::getBrightness() const
{
	return m_brightness;
}

inline int MonitorLayer::getContrast() const
{
	return m_contrast;
}

inline int MonitorLayer::getSaturation() const
{
	return m_saturation;
}

#endif // MONITORLAYER_H
