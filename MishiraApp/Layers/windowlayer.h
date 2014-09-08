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

#ifndef WINDOWLAYER_H
#define WINDOWLAYER_H

#include "common.h"
#include "cropinfo.h"
#include "layer.h"
#include "layerfactory.h"
#include <QtCore/QStringList>
#include <QtCore/QMargins>

class CaptureObject;
class LayerDialog;

//=============================================================================
class WindowLayer : public Layer
{
	friend class WindowLayerFactory;

private: // Members -----------------------------------------------------------
	CaptureObject *	m_captureObj;
	QSize			m_curSize;
	bool			m_curFlipped;
	VidgfxVertBuf *	m_vertBuf;
	QRectF			m_vertBufRect;
	QPointF			m_vertBufTlUv;
	QPointF			m_vertBufBrUv;
	bool			m_vertBufFlipped;
	VidgfxVertBuf *	m_cursorVertBuf;
	QRectF			m_cursorVertBufRect;
	QPointF			m_cursorVertBufBrUv;
	bool			m_windowListChanged;

	// Settings
	QStringList		m_exeFilenameList;
	QStringList		m_windowTitleList;
	bool			m_captureMouse;
	CptrMethod		m_captureMethod;
	CropInfo		m_cropInfo;
	float			m_gamma; // [0.1, 10.0]
	int				m_brightness; // [-250, 250]
	int				m_contrast; // [-100, 400]
	int				m_saturation; // [-100, 400]

private: // Constructor/destructor --------------------------------------------
	WindowLayer(LayerGroup *parent);
	~WindowLayer();

public: // Methods ------------------------------------------------------------
	void		beginAddingWindows();
	void		addWindow(const QString &exe, const QString &title);
	void		finishAddingWindows();
	QStringList	getExeFilenameList() const;
	QStringList	getWindowTitleList() const;
	void		setCaptureMouse(bool capture);
	bool		getCaptureMouse() const;
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
		VidgfxContext *gfx, const QPointF &tlUv, const QPointF &brUv);
	void		updateCursorVertBuf(
		VidgfxContext *gfx, const QPointF &brUv, const QRect &relRect);

public: // Interface ----------------------------------------------------------
	virtual void	initializeResources(VidgfxContext *gfx);
	virtual void	updateResources(VidgfxContext *gfx);
	virtual void	destroyResources(VidgfxContext *gfx);
	virtual void	render(
		VidgfxContext *gfx, Scene *scene, uint frameNum, int numDropped);

	virtual quint32	getTypeId() const;

	virtual bool			hasSettingsDialog();
	virtual LayerDialog *	createSettingsDialog(QWidget *parent = NULL);

	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			windowCreated(WinId winId);
	void			windowDestroyed(WinId winId);
};
//=============================================================================

inline QStringList WindowLayer::getExeFilenameList() const
{
	return m_exeFilenameList;
}

inline QStringList WindowLayer::getWindowTitleList() const
{
	return m_windowTitleList;
}

inline bool WindowLayer::getCaptureMouse() const
{
	return m_captureMouse;
}

inline CptrMethod WindowLayer::getCaptureMethod() const
{
	return m_captureMethod;
}

inline CropInfo WindowLayer::getCropInfo() const
{
	return m_cropInfo;
}

inline float WindowLayer::getGamma() const
{
	return m_gamma;
}

inline int WindowLayer::getBrightness() const
{
	return m_brightness;
}

inline int WindowLayer::getContrast() const
{
	return m_contrast;
}

inline int WindowLayer::getSaturation() const
{
	return m_saturation;
}

//=============================================================================
class WindowLayerFactory : public LayerFactory
{
public: // Interface ----------------------------------------------------------
	virtual quint32		getTypeId() const;
	virtual QByteArray	getTypeString() const;
	virtual Layer *		createBlankLayer(LayerGroup *parent);
	virtual Layer *		createLayerWithDefaults(LayerGroup *parent);
};
//=============================================================================

#endif // WINDOWLAYER_H
