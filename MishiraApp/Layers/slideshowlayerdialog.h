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

#ifndef SLIDESHOWLAYERDIALOG_H
#define SLIDESHOWLAYERDIALOG_H

#include "fileselectdialog.h"
#include "layerdialog.h"
#include "ui_slideshowlayerdialog.h"

class SlideshowLayer;

//=============================================================================
class SlideshowLayerDialog : public LayerDialog
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_SlideshowLayerDialog	m_ui;
	FileOpenDialog			m_fileDialog;

public: // Constructor/destructor ---------------------------------------------
	SlideshowLayerDialog(SlideshowLayer *layer, QWidget *parent = NULL);
	~SlideshowLayerDialog();

public: // Interface ----------------------------------------------------------
	virtual void	loadSettings();
	virtual void	applySettings();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			fileSelected(const QString &filename);
	void			removeImageClicked();
	void			moveUpClicked();
	void			moveDownClicked();

	void			delayEditChanged(const QString &text);
	void			transitionEditChanged(const QString &text);
};
//=============================================================================

#endif // SLIDESHOWLAYERDIALOG_H
