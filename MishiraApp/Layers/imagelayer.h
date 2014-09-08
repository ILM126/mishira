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

#ifndef IMAGELAYER_H
#define IMAGELAYER_H

#include "layer.h"
#include "layerfactory.h"

class FileImageTexture;
class LayerDialog;

//=============================================================================
class ImageLayer : public Layer
{
	friend class ImageLayerFactory;
	Q_OBJECT

private: // Members -----------------------------------------------------------
	VidgfxTexDecalBuf *	m_vertBuf;
	FileImageTexture *	m_imgTex;
	bool				m_filenameChanged;

	// Settings
	QString				m_filename;
	QPoint				m_scrollSpeed; // Pixels per second

private: // Constructor/destructor --------------------------------------------
	ImageLayer(LayerGroup *parent);
	~ImageLayer();

public: // Methods ------------------------------------------------------------
	void			setFilename(const QString &filename);
	QString			getFilename() const;
	void			setScrollSpeed(const QPoint &speed);
	QPoint			getScrollSpeed() const;

private:
	void			textureMaybeChanged();

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

protected:
	virtual void	showEvent();
	virtual void	hideEvent();
	virtual void	parentShowEvent();
	virtual void	parentHideEvent();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			queuedFrameEvent(uint frameNum, int numDropped);
};
//=============================================================================

inline QString ImageLayer::getFilename() const
{
	return m_filename;
}

inline QPoint ImageLayer::getScrollSpeed() const
{
	return m_scrollSpeed;
}

//=============================================================================
class ImageLayerFactory : public LayerFactory
{
public: // Interface ----------------------------------------------------------
	virtual quint32		getTypeId() const;
	virtual QByteArray	getTypeString() const;
	virtual Layer *		createBlankLayer(LayerGroup *parent);
	virtual Layer *		createLayerWithDefaults(LayerGroup *parent);
};
//=============================================================================

#endif // IMAGELAYER_H
