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

#include "webcamlayer.h"
#include "application.h"
#include "webcamlayerdialog.h"
#include "layergroup.h"
#include "profile.h"
#include "videosource.h"
#include "videosourcemanager.h"
#include <Libvidgfx/graphicscontext.h>

const QString LOG_CAT = QStringLiteral("Scene");

WebcamLayer::WebcamLayer(LayerGroup *parent)
	: Layer(parent)
	, m_deviceId(0)
	, m_orientation(GfxUnchangedOrient)

	, m_deviceIdChanged(false)
	, m_vidSource(NULL)
	, m_vertBuf(NULL)
	, m_vertBufRect()
	, m_vertBufBrUv(0.0f, 0.0f)
	, m_vertOrient(GfxUnchangedOrient)
	, m_lastTexSize(0, 0)
{
	// Watch manager for source changes
	VideoSourceManager *mgr = App->getVideoSourceManager();
	connect(mgr, &VideoSourceManager::sourceAdded,
		this, &WebcamLayer::sourceAdded);
	connect(mgr, &VideoSourceManager::removingSource,
		this, &WebcamLayer::removingSource);
}

WebcamLayer::~WebcamLayer()
{
}

void WebcamLayer::setDeviceId(quint64 id)
{
	if(m_deviceId == id)
		return; // Nothing to do
	m_deviceId = id;
	m_deviceIdChanged = true;
	if(m_vidSource != NULL)
		m_vidSource->dereference();
	m_vidSource = NULL;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WebcamLayer::setOrientation(GfxOrientation orientation)
{
	if(m_orientation == orientation)
		return; // Nothing to do
	m_orientation = orientation;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WebcamLayer::initializeResources(GraphicsContext *gfx)
{
	appLog(LOG_CAT)
		<< "Creating hardware resources for layer " << getIdString();

	// Allocate resources
	m_vertBuf = gfx->createVertexBuffer(GraphicsContext::TexDecalRectBufSize);

	// Reset state and update all resources
	m_vertBufRect = QRectF();
	m_vertBufBrUv = QPointF(0.0f, 0.0f);
	m_deviceIdChanged = true;
	updateResources(gfx);
}

void WebcamLayer::updateVertBuf(
	GraphicsContext *gfx, const QSize &texSize, const QPointF &brUv)
{
	setVisibleRect(QRect()); // Invisible by default
	if(m_vertBuf == NULL)
		return;
	if(texSize.isEmpty())
		return; // Nothing to display
	const QRectF rect = scaledRectFromActualSize(texSize);
	setVisibleRect(rect.toAlignedRect());
	if(m_vertBufBrUv == brUv && m_vertBufRect == rect &&
		m_vertOrient == m_orientation)
	{
		return;
	}

	m_vertBufRect = rect;
	m_vertBufBrUv = brUv;
	m_vertOrient = m_orientation;

	// Calculate orientation
	bool flipFrame = false;
	bool mirrorFrame = false;
	if(m_vidSource != NULL)
		flipFrame = m_vidSource->isFrameFlipped();
	switch(m_orientation) {
	default:
	case GfxUnchangedOrient:
		break;
	case GfxFlippedOrient:
		flipFrame = !flipFrame;
		break;
	case GfxMirroredOrient:
		mirrorFrame = !mirrorFrame;
		break;
	case GfxFlippedMirroredOrient:
		flipFrame = !flipFrame;
		mirrorFrame = !mirrorFrame;
		break;
	}

	// Calculate texture coordinates based on orientation
	QPointF tl, tr, bl, br;
	if(flipFrame && mirrorFrame) {
		tl = brUv;
		tr = QPointF(0.0, brUv.y());
		bl = QPointF(brUv.x(), 0.0);
		br = QPointF(0.0, 0.0);
	} else if(flipFrame) {
		tl = QPointF(0.0, brUv.y());
		tr = brUv;
		bl = QPointF(0.0, 0.0);
		br = QPointF(brUv.x(), 0.0);
	} else if(mirrorFrame) {
		tl = QPointF(brUv.x(), 0.0f);
		tr = QPointF(0.0f, 0.0f);
		bl = brUv;
		br = QPointF(0.0f, brUv.y());
	} else {
		tl = QPointF(0.0f, 0.0f);
		tr = QPointF(brUv.x(), 0.0f);
		bl = QPointF(0.0f, brUv.y());
		br = brUv;
	}

	// Actually update the vertex data
	gfx->createTexDecalRect(m_vertBuf, m_vertBufRect, tl, tr, bl, br);
}

void WebcamLayer::updateResources(GraphicsContext *gfx)
{
	// Update the visible rectangle and vertex buffer assuming that the size of
	// the next frame will be identical to the previous frame
	updateVertBuf(gfx, m_lastTexSize, m_vertBufBrUv);
}

void WebcamLayer::destroyResources(GraphicsContext *gfx)
{
	appLog(LOG_CAT)
		<< "Destroying hardware resources for layer " << getIdString();

	gfx->deleteVertexBuffer(m_vertBuf);
	m_vertBuf = NULL;

	// Dereference the video source as we no longer need it
	if(m_vidSource != NULL)
		m_vidSource->dereference();
	m_vidSource = NULL;
}

void WebcamLayer::render(
	GraphicsContext *gfx, Scene *scene, uint frameNum, int numDropped)
{
	// Fetch the video source if it's changed and make sure it's valid
	if(m_deviceIdChanged) {
		m_vidSource = App->getVideoSourceManager()->getSource(m_deviceId);
		if(m_vidSource != NULL) {
			if(!m_vidSource->reference()) {
				appLog(LOG_CAT, Log::Warning) << QStringLiteral(
					"Failed to reference video source");
				m_vidSource = NULL;
			}
		}
		m_deviceIdChanged = false;
	}
	if(m_vidSource == NULL)
		return; // Nothing to render

	// Get the latest video frame from the video source. The returned texture
	// is valid only for the duration of this render method.
	Texture *tex = m_vidSource->getCurrentFrame();
	if(tex == NULL || tex->getSize().isEmpty())
		return; // Nothing to render
	bool texSizeChanged = false;
	if(m_lastTexSize != tex->getSize()) {
		texSizeChanged = true;
		m_lastTexSize = tex->getSize();
	}

	// Prepare texture for render. TODO: Filter mode selection?
	QPointF pxSize, botRight;
	QSize visSize = scaledRectFromActualSize(tex->getSize()).size();
	tex = gfx->prepareTexture(
		tex, visSize, GfxBilinearFilter, true, pxSize, botRight);
	if(m_vertBufBrUv != botRight || texSizeChanged)
		updateVertBuf(gfx, m_lastTexSize, botRight);
	gfx->setTexture(tex);

	// Do the actual render
	gfx->setShader(GfxTexDecalRgbShader);
	gfx->setTopology(GfxTriangleStripTopology);
	QColor prevCol = gfx->getTexDecalModColor();
	gfx->setTexDecalModColor(
		QColor(255, 255, 255, (int)(getOpacity() * 255.0f)));
	if(getOpacity() != 1.0f)
		gfx->setBlending(GfxAlphaBlending);
	else
		gfx->setBlending(GfxNoBlending);
	gfx->drawBuffer(m_vertBuf);
	gfx->setTexDecalModColor(prevCol);
}

LyrType WebcamLayer::getType() const
{
	return LyrWebcamLayerType;
}

bool WebcamLayer::hasSettingsDialog()
{
	return true;
}

LayerDialog *WebcamLayer::createSettingsDialog(QWidget *parent)
{
	return new WebcamLayerDialog(this, parent);
}

void WebcamLayer::serialize(QDataStream *stream) const
{
	Layer::serialize(stream);

	// Write data version number
	*stream << (quint32)1;

	// Save our data
	*stream << m_deviceId;
	*stream << (quint32)m_orientation;
}

bool WebcamLayer::unserialize(QDataStream *stream)
{
	if(!Layer::unserialize(stream))
		return false;

	quint32 version;

	// Read data version number
	*stream >> version;

	// Read our data
	if(version == 0 || version == 1) {
		quint32 uint32Data;
		quint64 uint64Data;

		*stream >> uint64Data;
		setDeviceId(uint64Data);
		if(version >= 1) {
			*stream >> uint32Data;
			setOrientation((GfxOrientation)uint32Data);
		}
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in webcam layer serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

void WebcamLayer::sourceAdded(VideoSource *source)
{
	if(m_vidSource != NULL || source->getId() != m_deviceId)
		return; // Not our source

	// Our device has been added, use it
	m_deviceIdChanged = true;
	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WebcamLayer::removingSource(VideoSource *source)
{
	if(source != m_vidSource || m_vidSource == NULL)
		return; // Not our source

	// Our device has been removed, dereference it
	m_vidSource->dereference();
	m_vidSource = NULL;
	m_lastTexSize = QSize(0, 0);
	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}
