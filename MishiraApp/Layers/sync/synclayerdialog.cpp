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

#include "synclayerdialog.h"
#include "synclayer.h"

SyncLayerDialog::SyncLayerDialog(SyncLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
{
	m_ui.setupUi(this);

	// Notify the dialog when settings change
	connect(m_ui.colorBtn, &ColorButton::colorChanged,
		this, &LayerDialog::settingModified);
}

SyncLayerDialog::~SyncLayerDialog()
{
}

void SyncLayerDialog::loadSettings()
{
	SyncLayer *layer = static_cast<SyncLayer *>(m_layer);
	m_ui.colorBtn->setColor(layer->getColor());
}

void SyncLayerDialog::applySettings()
{
	SyncLayer *layer = static_cast<SyncLayer *>(m_layer);
	layer->setColor(m_ui.colorBtn->getColor());
}
