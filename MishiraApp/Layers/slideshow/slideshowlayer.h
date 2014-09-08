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

#ifndef SLIDESHOWLAYER_H
#define SLIDESHOWLAYER_H

#include "animatedfloat.h"
#include "layer.h"
#include "layerfactory.h"

class FileImageTexture;
class LayerDialog;

// WARNING: This is used in user profiles. Treat this as a public API.
enum SlideTransStyle {
	InThenOutStyle = 0,
	InAndOutStyle = 1
};

// WARNING: This is used in user profiles. Treat this as a public API.
enum SlideshowOrder {
	SequentialOrder = 0,
	RandomOrder = 1
};

//=============================================================================
class SlideshowLayer : public Layer
{
	friend class SlideshowLayerFactory;

private: // Datatypes ---------------------------------------------------------
	typedef QVector<VidgfxTexDecalBuf *> TexDecalVertBufList;
	typedef QVector<FileImageTexture *> FileImgTexList;

private: // Members -----------------------------------------------------------
	TexDecalVertBufList	m_vertBufs;
	FileImgTexList		m_imgTexs;
	bool				m_filenamesChanged;
	int					m_curId;
	int					m_prevId;
	AnimatedFloat		m_transition;
	float				m_timeSinceLastSwitch;

	// Settings
	QStringList			m_filenames;
	float				m_delayTime;
	float				m_transitionTime;
	SlideTransStyle		m_transitionStyle;
	SlideshowOrder		m_order;

private: // Constructor/destructor --------------------------------------------
	SlideshowLayer(LayerGroup *parent);
	~SlideshowLayer();

public: // Methods ------------------------------------------------------------
	void			setFilenames(const QStringList &filenames);
	QStringList		getFilenames() const;
	void			setDelayTime(float delay);
	float			getDelayTime() const;
	void			setTransitionTime(float transition);
	float			getTransitionTime() const;
	void			setTransitionStyle(SlideTransStyle style);
	SlideTransStyle	getTransitionStyle() const;
	void			setOrder(SlideshowOrder order);
	SlideshowOrder	getOrder() const;

private:
	void			switchToFirstImage();
	void			switchToNextImage(bool immediately = false);
	bool			switchToImage(int id, bool immediately);
	void			textureMaybeChanged(int id);
	void			renderImage(VidgfxContext *gfx, int id, float opacity);

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

inline QStringList SlideshowLayer::getFilenames() const
{
	return m_filenames;
}

inline float SlideshowLayer::getDelayTime() const
{
	return m_delayTime;
}

inline float SlideshowLayer::getTransitionTime() const
{
	return m_transitionTime;
}

inline SlideTransStyle SlideshowLayer::getTransitionStyle() const
{
	return m_transitionStyle;
}

inline SlideshowOrder SlideshowLayer::getOrder() const
{
	return m_order;
}

//=============================================================================
class SlideshowLayerFactory : public LayerFactory
{
public: // Interface ----------------------------------------------------------
	virtual quint32		getTypeId() const;
	virtual QByteArray	getTypeString() const;
	virtual Layer *		createBlankLayer(LayerGroup *parent);
	virtual Layer *		createLayerWithDefaults(LayerGroup *parent);
	virtual QString		getAddLayerString() const;
};
//=============================================================================

#endif // SLIDESHOWLAYER_H
