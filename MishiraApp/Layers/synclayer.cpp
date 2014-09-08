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

#define _USE_MATH_DEFINES

#include "synclayer.h"
#include "application.h"
#include "audiomixer.h"
#include "synclayerdialog.h"
#include "layergroup.h"
#include "profile.h"
#include "stylehelper.h"

const QString LOG_CAT = QStringLiteral("Scene");

// The width of the metronome components in pixels
const int MET_WIDTH = 4;

SyncLayer::SyncLayer(LayerGroup *parent)
	: Layer(parent)
	, m_vertBufA(NULL)
	, m_vertBufB(NULL)
	, m_vertBufC(NULL)
	, m_color(255, 255, 255, 255) // Default colour
	, m_refMetronomeDelayed(false)
	, m_metronomeReffed(false)
{
}

SyncLayer::~SyncLayer()
{
}

void SyncLayer::setColor(const QColor &color)
{
	if(m_color == color)
		return; // Nothing to do
	m_color = color;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void SyncLayer::initializeResources(VidgfxContext *gfx)
{
	appLog(LOG_CAT)
		<< "Creating hardware resources for layer " << getIdString();

	m_vertBufA = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_SOLID_RECT_BUF_SIZE);
	m_vertBufB = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_SOLID_RECT_BUF_SIZE);
	m_vertBufC = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_SOLID_RECT_BUF_SIZE);
	updateResources(gfx);
}

/// <summary>
/// Update the position of the moving metronome.
/// </summary>
void SyncLayer::updateMetronome(VidgfxContext *gfx, uint frameNum)
{
	if(m_vertBufA == NULL)
		return;

	// Generate the rectangle at the center of the metronome
	QRect rect(m_rect.center().x() - MET_WIDTH / 2,
		m_rect.top() + m_rect.height() / 4,
		MET_WIDTH, m_rect.height() - m_rect.height() / 4 * 2);

	// Translate the rectangle based on the frame number (Which is in turn
	// based on the frame origin that the sound component is synced to). We be
	// very careful here with the maths to prevent rounding errors at large
	// frame numbers that would make the metronome be out-of-sync.
	Fraction framerate = App->getProfile()->getVideoFramerate();
	quint64 time = (quint64)frameNum * 1000ULL * framerate.denominator /
		framerate.numerator;
	double offset = sin((double)time / 1000.0 * M_PI); // Range: -1.0 to +1.0
	offset *= (double)(m_rect.width() / 2); // Scale to rectangle size
	rect.translate((int)offset, 0);

	// Copy rectangle to the GPU
	QColor color = m_color;
	color.setAlphaF(color.alphaF() * getOpacity());
	vidgfx_create_solid_rect(
		m_vertBufA, rect, color, color, color, color);
}

void SyncLayer::updateResources(VidgfxContext *gfx)
{
	// We update the moving metronome when we render, not now
	QColor color = m_color;
	color.setAlphaF(color.alphaF() * getOpacity());
	if(m_vertBufB != NULL) {
		QRect rect(m_rect.center().x() - MET_WIDTH / 2, m_rect.top(),
			MET_WIDTH, m_rect.height() / 4);
		vidgfx_create_solid_rect(
			m_vertBufB, rect, color, color, color, color);
	}
	if(m_vertBufC != NULL) {
		QRect rect(m_rect.center().x() - MET_WIDTH / 2,
			m_rect.bottom() - m_rect.height() / 4,
			MET_WIDTH, m_rect.height() / 4);
		vidgfx_create_solid_rect(
			m_vertBufC, rect, color, color, color, color);
	}
}

void SyncLayer::destroyResources(VidgfxContext *gfx)
{
	appLog(LOG_CAT)
		<< "Destroying hardware resources for layer " << getIdString();

	vidgfx_context_destroy_vertbuf(gfx, m_vertBufA);
	vidgfx_context_destroy_vertbuf(gfx, m_vertBufB);
	vidgfx_context_destroy_vertbuf(gfx, m_vertBufC);
	m_vertBufA = NULL;
	m_vertBufB = NULL;
	m_vertBufC = NULL;
}

void SyncLayer::showEvent()
{
	if(!isVisibleSomewhere())
		return; // Still hidden
	if(m_metronomeReffed)
		return; // Already referenced

	// Enable audio metronome. As this event can be emitted before the profile
	// has been fully initialized we may need to delay referencing
	Profile *profile = App->getProfile();
	if(profile == NULL) {
		m_refMetronomeDelayed = true;
		return;
	}
	AudioMixer *mixer = profile->getAudioMixer();
	if(mixer == NULL) {
		m_refMetronomeDelayed = true;
		return;
	}
	mixer->refMetronome();
	m_refMetronomeDelayed = false;
	m_metronomeReffed = true;
}

void SyncLayer::hideEvent()
{
	if(isVisibleSomewhere())
		return; // Still visible
	if(!m_metronomeReffed)
		return; // Already dereferenced

	// Disable audio metronome
	AudioMixer *mixer = App->getProfile()->getAudioMixer();
	mixer->derefMetronome();
	m_refMetronomeDelayed = false;
	m_metronomeReffed = false;
}

void SyncLayer::parentShowEvent()
{
	showEvent();
}

void SyncLayer::parentHideEvent()
{
	hideEvent();
}

void SyncLayer::render(
	VidgfxContext *gfx, Scene *scene, uint frameNum, int numDropped)
{
	if(m_refMetronomeDelayed) {
		// Enable audio metronome
		AudioMixer *mixer = App->getProfile()->getAudioMixer();
		mixer->refMetronome();
		m_refMetronomeDelayed = false;
		m_metronomeReffed = true;
	}

	updateMetronome(gfx, frameNum);
	vidgfx_context_set_shader(gfx, GfxSolidShader);
	vidgfx_context_set_topology(gfx, GfxTriangleStripTopology);
	if(m_color.alpha() != 255 || getOpacity() != 1.0f)
		vidgfx_context_set_blending(gfx, GfxAlphaBlending);
	else
		vidgfx_context_set_blending(gfx, GfxNoBlending);
	vidgfx_context_draw_buf(gfx, m_vertBufA);
	vidgfx_context_draw_buf(gfx, m_vertBufB);
	vidgfx_context_draw_buf(gfx, m_vertBufC);
}

quint32 SyncLayer::getTypeId() const
{
	return (quint32)LyrSyncLayerTypeId;
}

bool SyncLayer::hasSettingsDialog()
{
	return true;
}

LayerDialog *SyncLayer::createSettingsDialog(QWidget *parent)
{
	return new SyncLayerDialog(this, parent);
}

void SyncLayer::serialize(QDataStream *stream) const
{
	Layer::serialize(stream);

	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << m_color;
}

bool SyncLayer::unserialize(QDataStream *stream)
{
	if(!Layer::unserialize(stream))
		return false;

	// Read data version number
	quint32 version;
	*stream >> version;

	// Read our data
	if(version == 0) {
		*stream >> m_color;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in sync layer serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

//=============================================================================
// SyncLayerFactory class

quint32 SyncLayerFactory::getTypeId() const
{
	return (quint32)LyrSyncLayerTypeId;
}

QByteArray SyncLayerFactory::getTypeString() const
{
	return QByteArrayLiteral("SyncLayer");
}

Layer *SyncLayerFactory::createBlankLayer(LayerGroup *parent)
{
	return new SyncLayer(parent);
}

Layer *SyncLayerFactory::createLayerWithDefaults(LayerGroup *parent)
{
	return new SyncLayer(parent);
}
