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

#ifndef COLORLAYER_H
#define COLORLAYER_H

#include "layer.h"
#include "layerfactory.h"
#include <QtGui/QColor>

class LayerDialog;

// WARNING: This is used in user profiles. Treat this as a public API.
enum GradientPattern {
	SolidPattern = 0,
	VerticalPattern = 1,
	HorizontalPattern = 2
};

//=============================================================================
class ColorLayer : public Layer
{
	friend class ColorLayerFactory;

private: // Members -----------------------------------------------------------
	VidgfxVertBuf *	m_vertBuf;
	GradientPattern	m_pattern;
	QColor			m_aColor; // Base/top/left
	QColor			m_bColor; // Bottom/right

private: // Constructor/destructor --------------------------------------------
	ColorLayer(LayerGroup *parent);
	~ColorLayer();

public: // Methods ------------------------------------------------------------
	void			setPattern(GradientPattern pat);
	GradientPattern	getPattern() const;
	void			setAColor(const QColor &color);
	QColor			getAColor() const;
	void			setBColor(const QColor &color);
	QColor			getBColor() const;

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
};
//=============================================================================

inline GradientPattern ColorLayer::getPattern() const
{
	return m_pattern;
}

inline QColor ColorLayer::getAColor() const
{
	return m_aColor;
}

inline QColor ColorLayer::getBColor() const
{
	return m_bColor;
}

//=============================================================================
class ColorLayerFactory : public LayerFactory
{
public: // Interface ----------------------------------------------------------
	virtual quint32		getTypeId() const;
	virtual QByteArray	getTypeString() const;
	virtual Layer *		createBlankLayer(LayerGroup *parent);
	virtual Layer *		createLayerWithDefaults(LayerGroup *parent);
	virtual QString		getAddLayerString() const;
};
//=============================================================================

#endif // COLORLAYER_H
