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

#ifndef WEBCAMLAYER_H
#define WEBCAMLAYER_H

#include "layer.h"
#include "layerfactory.h"

class LayerDialog;
class VideoSource;

//=============================================================================
class WebcamLayer : public Layer
{
	friend class WebcamLayerFactory;

private: // Members -----------------------------------------------------------
	quint64				m_deviceId;
	VidgfxOrientation	m_orientation;

	bool				m_deviceIdChanged;
	VideoSource *		m_vidSource;
	VidgfxVertBuf *		m_vertBuf;
	QRectF				m_vertBufRect;
	QPointF				m_vertBufBrUv;
	VidgfxOrientation	m_vertOrient;
	QSize				m_lastTexSize;

private: // Constructor/destructor --------------------------------------------
	WebcamLayer(LayerGroup *parent);
	~WebcamLayer();

public: // Methods ------------------------------------------------------------
	void				setDeviceId(quint64 id);
	quint64				getDeviceId() const;
	void				setOrientation(VidgfxOrientation orientation);
	VidgfxOrientation	getOrientation() const;

private:
	void	updateVertBuf(
		VidgfxContext *gfx, const QSize &texSize, const QPointF &brUv);

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
	void			sourceAdded(VideoSource *source);
	void			removingSource(VideoSource *source);
};
//=============================================================================

inline quint64 WebcamLayer::getDeviceId() const
{
	return m_deviceId;
}

inline VidgfxOrientation WebcamLayer::getOrientation() const
{
	return m_orientation;
}

//=============================================================================
class WebcamLayerFactory : public LayerFactory
{
public: // Interface ----------------------------------------------------------
	virtual quint32		getTypeId() const;
	virtual QByteArray	getTypeString() const;
	virtual Layer *		createBlankLayer(LayerGroup *parent);
	virtual Layer *		createLayerWithDefaults(LayerGroup *parent);
};
//=============================================================================

#endif // WEBCAMLAYER_H
