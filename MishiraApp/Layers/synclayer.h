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

//=============================================================================
class SyncLayer : public Layer
{
	friend class LayerGroup;

private: // Members -----------------------------------------------------------
	VidgfxVertBuf *	m_vertBufA; // Moving metronome
	VidgfxVertBuf *	m_vertBufB; // Static center
	VidgfxVertBuf *	m_vertBufC; // Static center
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
	void			updateMetronome(VidgfxContext *gfx, uint frameNum);

public: // Interface ----------------------------------------------------------
	virtual void	initializeResources(VidgfxContext *gfx);
	virtual void	updateResources(VidgfxContext *gfx);
	virtual void	destroyResources(VidgfxContext *gfx);
	virtual void	render(
		VidgfxContext *gfx, Scene *scene, uint frameNum, int numDropped);

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
