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

#ifndef IMAGELAYERDIALOG_H
#define IMAGELAYERDIALOG_H

#include "fileselectdialog.h"
#include "layerdialog.h"
#include "ui_imagelayerdialog.h"

class ImageLayer;

//=============================================================================
class ImageLayerDialog : public LayerDialog
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_ImageLayerDialog	m_ui;
	FileOpenDialog		m_fileDialog;

public: // Constructor/destructor ---------------------------------------------
	ImageLayerDialog(ImageLayer *layer, QWidget *parent = NULL);
	~ImageLayerDialog();

public: // Interface ----------------------------------------------------------
	virtual void	loadSettings();
	virtual void	applySettings();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			filenameEditChanged(const QString &text);
	void			xScrollEditChanged(const QString &text);
	void			yScrollEditChanged(const QString &text);
};
//=============================================================================

#endif // IMAGELAYERDIALOG_H
