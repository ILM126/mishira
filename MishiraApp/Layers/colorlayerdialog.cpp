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

#include "colorlayerdialog.h"
#include "colorlayer.h"

ColorLayerDialog::ColorLayerDialog(ColorLayer *layer, QWidget *parent)
	: LayerDialog(layer, parent)
	, m_ui()
{
	m_ui.setupUi(this);

	// Connect button signals
	connect(m_ui.solidRadio, &QPushButton::clicked,
		this, &ColorLayerDialog::patternChanged);
	connect(m_ui.vertRadio, &QPushButton::clicked,
		this, &ColorLayerDialog::patternChanged);
	connect(m_ui.horiRadio, &QPushButton::clicked,
		this, &ColorLayerDialog::patternChanged);

	// Notify the dialog when settings change
	connect(m_ui.solidRadio, &QPushButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.vertRadio, &QPushButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.horiRadio, &QPushButton::clicked,
		this, &LayerDialog::settingModified);
	connect(m_ui.aColorBtn, &ColorButton::colorChanged,
		this, &LayerDialog::settingModified);
	connect(m_ui.bColorBtn, &ColorButton::colorChanged,
		this, &LayerDialog::settingModified);
}

ColorLayerDialog::~ColorLayerDialog()
{
}

GradientPattern ColorLayerDialog::getGradientPattern()
{
	if(m_ui.solidRadio->isChecked())
		return SolidPattern;
	if(m_ui.vertRadio->isChecked())
		return VerticalPattern;
	if(m_ui.horiRadio->isChecked())
		return HorizontalPattern;
	return SolidPattern;
}

void ColorLayerDialog::updateWidgets()
{
	switch(getGradientPattern()) {
	default:
	case SolidPattern:
		m_ui.aColorLbl->setText(tr("Main color:"));
		m_ui.aColorLbl->setEnabled(true);
		m_ui.aColorBtn->setEnabled(true);
		m_ui.bColorLbl->setText(tr("Unused color:"));
		m_ui.bColorLbl->setEnabled(false);
		m_ui.bColorBtn->setEnabled(false);
		break;
	case VerticalPattern:
		m_ui.aColorLbl->setText(tr("Top color:"));
		m_ui.aColorLbl->setEnabled(true);
		m_ui.aColorBtn->setEnabled(true);
		m_ui.bColorLbl->setText(tr("Bottom color:"));
		m_ui.bColorLbl->setEnabled(true);
		m_ui.bColorBtn->setEnabled(true);
		break;
	case HorizontalPattern:
		m_ui.aColorLbl->setText(tr("Left color:"));
		m_ui.aColorLbl->setEnabled(true);
		m_ui.aColorBtn->setEnabled(true);
		m_ui.bColorLbl->setText(tr("Right color:"));
		m_ui.bColorLbl->setEnabled(true);
		m_ui.bColorBtn->setEnabled(true);
		break;
	}
}

void ColorLayerDialog::loadSettings()
{
	ColorLayer *layer = static_cast<ColorLayer *>(m_layer);
	switch(layer->getPattern()) {
	default:
	case SolidPattern:
		m_ui.solidRadio->setChecked(true);
		break;
	case VerticalPattern:
		m_ui.vertRadio->setChecked(true);
		break;
	case HorizontalPattern:
		m_ui.horiRadio->setChecked(true);
		break;
	}
	m_ui.aColorBtn->setColor(layer->getAColor());
	m_ui.bColorBtn->setColor(layer->getBColor());
	updateWidgets();
}

void ColorLayerDialog::applySettings()
{
	ColorLayer *layer = static_cast<ColorLayer *>(m_layer);
	layer->setPattern(getGradientPattern());
	layer->setAColor(m_ui.aColorBtn->getColor());
	layer->setBColor(m_ui.bColorBtn->getColor());
}

void ColorLayerDialog::patternChanged()
{
	updateWidgets();
}
