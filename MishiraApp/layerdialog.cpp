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

#include "layerdialog.h"

LayerDialog::LayerDialog(Layer *layer, QWidget *parent)
	: QWidget(parent)
	, m_layer(layer)
	, m_isModified(false)
{
	Q_ASSERT(layer != NULL);
}

LayerDialog::~LayerDialog()
{
}

void LayerDialog::setModified(bool modified)
{
	m_isModified = modified;
	//setWindowModified(modified);
}

void LayerDialog::loadSettings()
{
}

void LayerDialog::applySettings()
{
}

void LayerDialog::cancelSettings()
{
}

void LayerDialog::resetSettings()
{
	cancelSettings();
	loadSettings();
}

void LayerDialog::settingModified()
{
	setModified(true);
}
