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

#ifndef COLORLAYERDIALOG_H
#define COLORLAYERDIALOG_H

#include "layerdialog.h"
#include "ui_colorlayerdialog.h"
#include "colorlayer.h"

//=============================================================================
class ColorLayerDialog : public LayerDialog
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_ColorLayerDialog	m_ui;

public: // Constructor/destructor ---------------------------------------------
	ColorLayerDialog(ColorLayer *layer, QWidget *parent = NULL);
	~ColorLayerDialog();

private: // Methods -----------------------------------------------------------
	GradientPattern	getGradientPattern();
	void			updateWidgets();

public: // Interface ----------------------------------------------------------
	virtual void	loadSettings();
	virtual void	applySettings();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			patternChanged();
};
//=============================================================================

#endif // COLORLAYERDIALOG_H
