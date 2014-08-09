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

#ifndef COLORLAYER_H
#define COLORLAYER_H

#include "layer.h"
#include <QtGui/QColor>

class LayerDialog;
class VertexBuffer;

enum GradientPattern {
	SolidPattern = 0,
	VerticalPattern,
	HorizontalPattern
};

//=============================================================================
class ColorLayer : public Layer
{
	friend class LayerGroup;

private: // Members -----------------------------------------------------------
	VertexBuffer *	m_vertBuf;
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

#endif // COLORLAYER_H
