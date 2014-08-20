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

#include "colorlayer.h"
#include "application.h"
#include "colorlayerdialog.h"
#include "layergroup.h"
#include "stylehelper.h"

const QString LOG_CAT = QStringLiteral("Scene");

ColorLayer::ColorLayer(LayerGroup *parent)
	: Layer(parent)
	, m_vertBuf(NULL)
	, m_pattern(SolidPattern)
	, m_aColor(StyleHelper::RedBaseColor) // Default colour
	, m_bColor(m_aColor)
{
}

ColorLayer::~ColorLayer()
{
}

void ColorLayer::setPattern(GradientPattern pat)
{
	if(m_pattern == pat)
		return; // Nothing to do
	m_pattern = pat;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ColorLayer::setAColor(const QColor &color)
{
	if(m_aColor == color)
		return; // Nothing to do
	m_aColor = color;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ColorLayer::setBColor(const QColor &color)
{
	if(m_bColor == color)
		return; // Nothing to do
	m_bColor = color;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void ColorLayer::initializeResources(VidgfxContext *gfx)
{
	appLog(LOG_CAT)
		<< "Creating hardware resources for layer " << getIdString();

	m_vertBuf = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_SOLID_RECT_BUF_SIZE);
	updateResources(gfx);
}

void ColorLayer::updateResources(VidgfxContext *gfx)
{
	if(m_vertBuf != NULL) {
		QColor aColor = m_aColor;
		QColor bColor = m_bColor;
		aColor.setAlphaF(aColor.alphaF() * getOpacity());
		bColor.setAlphaF(bColor.alphaF() * getOpacity());

		switch(m_pattern) {
		default:
		case SolidPattern:
			vidgfx_create_solid_rect(
				m_vertBuf, m_rect, aColor, aColor, aColor, aColor);
			break;
		case VerticalPattern:
			vidgfx_create_solid_rect(
				m_vertBuf, m_rect, aColor, aColor, bColor, bColor);
			break;
		case HorizontalPattern:
			vidgfx_create_solid_rect(
				m_vertBuf, m_rect, aColor, bColor, aColor, bColor);
			break;
		}
	}
}

void ColorLayer::destroyResources(VidgfxContext *gfx)
{
	appLog(LOG_CAT)
		<< "Destroying hardware resources for layer " << getIdString();

	vidgfx_context_destroy_vertbuf(gfx, m_vertBuf);
	m_vertBuf = NULL;
}

void ColorLayer::render(
	VidgfxContext *gfx, Scene *scene, uint frameNum, int numDropped)
{
	vidgfx_context_set_shader(gfx, GfxSolidShader);
	vidgfx_context_set_topology(gfx, GfxTriangleStripTopology);
	if(m_aColor.alpha() != 255 || m_bColor.alpha() != 255 ||
		getOpacity() != 1.0f)
	{
		vidgfx_context_set_blending(gfx, GfxAlphaBlending);
	} else
		vidgfx_context_set_blending(gfx, GfxNoBlending);
	vidgfx_context_draw_buf(gfx, m_vertBuf);
}

LyrType ColorLayer::getType() const
{
	return LyrColorLayerType;
}

bool ColorLayer::hasSettingsDialog()
{
	return true;
}

LayerDialog *ColorLayer::createSettingsDialog(QWidget *parent)
{
	return new ColorLayerDialog(this, parent);
}

void ColorLayer::serialize(QDataStream *stream) const
{
	Layer::serialize(stream);

	// Write data version number
	*stream << (quint32)1;

	// Save our data
	*stream << (quint32)m_pattern;
	*stream << m_aColor;
	*stream << m_bColor;
}

bool ColorLayer::unserialize(QDataStream *stream)
{
	if(!Layer::unserialize(stream))
		return false;

	// Read data version number
	quint32 version;
	*stream >> version;

	// Read our data
	if(version >= 0 && version <= 1) {
		quint32 uint32Data;

		if(version >= 1) {
			*stream >> uint32Data;
			m_pattern = (GradientPattern)uint32Data;
		} else
			m_pattern = VerticalPattern;
		*stream >> m_aColor;
		*stream >> m_bColor;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in color layer serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}
