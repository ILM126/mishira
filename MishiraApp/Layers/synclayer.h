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

#ifndef SYNCLAYER_H
#define SYNCLAYER_H

#include "layer.h"
#include <QtGui/QColor>

class LayerDialog;
class VertexBuffer;

//=============================================================================
class SyncLayer : public Layer
{
	friend class LayerGroup;

private: // Members -----------------------------------------------------------
	VertexBuffer *	m_vertBufA; // Moving metronome
	VertexBuffer *	m_vertBufB; // Static center
	VertexBuffer *	m_vertBufC; // Static center
	QColor			m_color;
	bool			m_refMetronomeDelayed;
	bool			m_metronomeReffed;

private: // Constructor/destructor --------------------------------------------
	SyncLayer(LayerGroup *parent);
	~SyncLayer();

public: // Methods ------------------------------------------------------------
	void			setColor(const QColor &color);
	QColor			getColor() const;

private:
	void			updateMetronome(GraphicsContext *gfx, uint frameNum);

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

protected:
	virtual void	showEvent();
	virtual void	hideEvent();
	virtual void	parentShowEvent();
	virtual void	parentHideEvent();
};
//=============================================================================

inline QColor SyncLayer::getColor() const
{
	return m_color;
}

#endif // SYNCLAYER_H
