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

class LayerDialog;
class Texture;
class VertexBuffer;
class VideoSource;

//=============================================================================
class WebcamLayer : public Layer
{
	friend class LayerGroup;

private: // Members -----------------------------------------------------------
	quint64			m_deviceId;
	GfxOrientation	m_orientation;

	bool			m_deviceIdChanged;
	VideoSource *	m_vidSource;
	VertexBuffer *	m_vertBuf;
	QRectF			m_vertBufRect;
	QPointF			m_vertBufBrUv;
	GfxOrientation	m_vertOrient;
	QSize			m_lastTexSize;

private: // Constructor/destructor --------------------------------------------
	WebcamLayer(LayerGroup *parent);
	~WebcamLayer();

public: // Methods ------------------------------------------------------------
	void			setDeviceId(quint64 id);
	quint64			getDeviceId() const;
	void			setOrientation(GfxOrientation orientation);
	GfxOrientation	getOrientation() const;

private:
	void	updateVertBuf(
		GraphicsContext *gfx, const QSize &texSize, const QPointF &brUv);

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
	void			sourceAdded(VideoSource *source);
	void			removingSource(VideoSource *source);
};
//=============================================================================

inline quint64 WebcamLayer::getDeviceId() const
{
	return m_deviceId;
}

inline GfxOrientation WebcamLayer::getOrientation() const
{
	return m_orientation;
}

#endif // WEBCAMLAYER_H
