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

#include "windowlayer.h"
#include "application.h"
#include "layergroup.h"
#include "windowlayerdialog.h"
#include <Libdeskcap/capturemanager.h>
#include <Libdeskcap/captureobject.h>

const QString LOG_CAT = QStringLiteral("Scene");

//=============================================================================
// Serialization helpers

// These helpers exist so that we do not depend on the order of enums in our
// serialization and deserialization methods. This is important as if the order
// changes in a future release then we will be unable to deserialize older
// versions.
// WARNING: ALL CHANGES MADE TO THESE METHODS MUST BE BACKWARDS-COMPATIBLE!

static quint32 capMethodToInt32(CptrMethod method)
{
	switch(method) {
	default:
	case CptrAutoMethod: return 0;
	case CptrStandardMethod: return 1;
	case CptrCompositorMethod: return 2;
	case CptrHookMethod: return 3;
	case CptrDuplicatorMethod: return 4;
	}
	return 0; // Should never be reached
}

static CptrMethod int32ToCapMethod(quint32 method)
{
	switch(method) {
	default:
	case 0: return CptrAutoMethod;
	case 1: return CptrStandardMethod;
	case 2: return CptrCompositorMethod;
	case 3: return CptrHookMethod;
	case 4: return CptrDuplicatorMethod;
	}
	return CptrAutoMethod; // Should never be reached
}

//=============================================================================
// WindowLayer class

WindowLayer::WindowLayer(LayerGroup *parent)
	: Layer(parent)
	, m_captureObj(NULL)
	, m_curSize()
	, m_curFlipped(false)
	, m_vertBuf(NULL)
	, m_vertBufRect()
	, m_vertBufTlUv(0.0f, 0.0f)
	, m_vertBufBrUv(0.0f, 0.0f)
	, m_vertBufFlipped(false)
	, m_cursorVertBuf(NULL)
	, m_cursorVertBufRect()
	, m_cursorVertBufBrUv(0.0f, 0.0f)
	, m_windowListChanged(false)

	// Settings
	, m_exeFilenameList()
	, m_windowTitleList()
	, m_captureMouse(true)
	, m_captureMethod(CptrAutoMethod)
	, m_cropInfo()
	, m_gamma(1.0f)
	, m_brightness(0)
	, m_contrast(0)
	, m_saturation(0)
{
	// Watch for window creation and deletion
	CaptureManager *mgr = App->getCaptureManager();
	connect(mgr, &CaptureManager::windowCreated,
		this, &WindowLayer::windowCreated);
	connect(mgr, &CaptureManager::windowDestroyed,
		this, &WindowLayer::windowDestroyed);
}

WindowLayer::~WindowLayer()
{
	if(m_captureObj != NULL)
		m_captureObj->release();
}

void WindowLayer::beginAddingWindows()
{
	m_exeFilenameList.clear();
	m_windowTitleList.clear();
}

void WindowLayer::addWindow(const QString &exe, const QString &title)
{
	m_exeFilenameList.append(exe);
	m_windowTitleList.append(title);
}

void WindowLayer::finishAddingWindows()
{
	m_windowListChanged = true;
	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WindowLayer::setCaptureMouse(bool capture)
{
	if(m_captureMouse == capture)
		return; // Nothing to do
	m_captureMouse = capture;
	//updateResourcesIfLoaded(); // Not needed
	m_parent->layerChanged(this); // Remote emit
}

void WindowLayer::setCaptureMethod(CptrMethod method)
{
	if(m_captureMethod == method)
		return; // Nothing to do
	m_captureMethod = method;
	if(m_captureObj != NULL)
		m_captureObj->setMethod(m_captureMethod);
	//updateResourcesIfLoaded(); // Not needed
	m_parent->layerChanged(this); // Remote emit
}

void WindowLayer::setCropInfo(const CropInfo &info)
{
	if(m_cropInfo == info)
		return; // Nothing changed
	m_cropInfo = info;
	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WindowLayer::setGamma(float gamma)
{
	if(m_gamma == gamma)
		return; // Nothing to do
	m_gamma = gamma;
	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WindowLayer::setBrightness(int brightness)
{
	if(m_brightness == brightness)
		return; // Nothing to do
	m_brightness = brightness;
	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WindowLayer::setContrast(int contrast)
{
	if(m_contrast == contrast)
		return; // Nothing to do
	m_contrast = contrast;
	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WindowLayer::setSaturation(int saturation)
{
	if(m_saturation == saturation)
		return; // Nothing to do
	m_saturation = saturation;
	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void WindowLayer::updateVertBuf(
	VidgfxContext *gfx, const QPointF &tlUv, const QPointF &brUv)
{
	if(m_vertBuf == NULL)
		return;
	if(m_captureObj == NULL)
		return; // Nothing visible

	// Has anything changed that would result in us having to recreate the
	// vertex buffer?
	QRect cropRect = m_cropInfo.calcCroppedRectForSize(m_curSize);
	const QRectF rect = scaledRectFromActualSize(cropRect.size());
	setVisibleRect(rect.toAlignedRect());
	if(m_vertBufTlUv == tlUv && m_vertBufBrUv == brUv &&
		m_vertBufRect == rect && m_vertBufFlipped == m_curFlipped)
	{
		return;
	}

	m_vertBufRect = rect;
	m_vertBufTlUv = tlUv;
	m_vertBufBrUv = brUv;
	m_vertBufFlipped = m_curFlipped;
	if(m_vertBufFlipped) {
		// Texture is flipped vertically
		vidgfx_create_tex_decal_rect(m_vertBuf, m_vertBufRect,
			QPointF(tlUv.x(), brUv.y()), brUv,
			tlUv, QPointF(brUv.x(), tlUv.y()));
	} else {
		vidgfx_create_tex_decal_rect(m_vertBuf, m_vertBufRect,
			tlUv, QPointF(brUv.x(), tlUv.y()),
			QPointF(tlUv.x(), brUv.y()), brUv);
	}
}

void WindowLayer::updateCursorVertBuf(
	VidgfxContext *gfx, const QPointF &brUv, const QRect &relRect)
{
	if(m_cursorVertBuf == NULL)
		return;

	// Scale cursor rectangle nicely to match the scaled window
	QRect cropRect = m_cropInfo.calcCroppedRectForSize(m_curSize);
	const QPointF scale(
		m_vertBufRect.width() / (float)cropRect.width(),
		m_vertBufRect.height() / (float)cropRect.height());
	QRectF rect(
		m_vertBufRect.x() + qRound((float)(relRect.x() - cropRect.x()) * scale.x()),
		m_vertBufRect.y() + qRound((float)(relRect.y() - cropRect.y()) * scale.y()),
		qRound((float)relRect.width() * scale.x()),
		qRound((float)relRect.height() * scale.y()));

	if(m_cursorVertBufBrUv == brUv && m_cursorVertBufRect == rect)
		return; // Buffer hasn't changed

	m_cursorVertBufRect = rect;
	m_cursorVertBufBrUv = brUv;
	vidgfx_create_tex_decal_rect(
		m_cursorVertBuf, m_cursorVertBufRect, m_cursorVertBufBrUv);
}

void WindowLayer::initializeResources(VidgfxContext *gfx)
{
	appLog(LOG_CAT)
		<< "Creating hardware resources for layer " << getIdString();

	// Allocate resources
	m_vertBuf = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_TEX_DECAL_RECT_BUF_SIZE);
	m_cursorVertBuf = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_TEX_DECAL_RECT_BUF_SIZE);

	// Reset caching and update all resources
	m_windowListChanged = true;
	m_vertBufRect = QRectF();
	m_vertBufTlUv = QPointF(0.0f, 0.0f);
	m_vertBufBrUv = QPointF(0.0f, 0.0f);
	m_cursorVertBufRect = QRectF();
	m_cursorVertBufBrUv = QPointF(0.0f, 0.0f);
	updateResources(gfx);
}

void WindowLayer::updateResources(VidgfxContext *gfx)
{
	// Layer is invisible by default
	setVisibleRect(QRect());

	if(m_windowListChanged) {
		m_windowListChanged = false;

		// Release the existing object if it exists
		if(m_captureObj != NULL) {
			m_captureObj->release();
			m_captureObj = NULL;
		}

		// Find the first matching window in our list
		CaptureManager *mgr = App->getCaptureManager();
		mgr->cacheWindowList();
		for(int i = 0; i < m_exeFilenameList.count(); i++) {
			const QString exe = m_exeFilenameList.at(i);
			const QString title = m_windowTitleList.at(i);
			WinId winId = mgr->findWindow(exe, title);
			if(winId == NULL)
				continue;
			m_captureObj = mgr->captureWindow(winId, m_captureMethod);
			break;
		}
		mgr->uncacheWindowList();
	}

	m_curSize = QSize();
	if(m_captureObj != NULL) {
		m_curSize = m_captureObj->getSize();
		m_curFlipped = m_captureObj->isFlipped();
	}

	// Assume that the left-left UV is at (0, 0) and bottom-right UV is at
	// (1, 1) for now
	updateVertBuf(gfx, QPointF(0.0f, 0.0f), QPointF(1.0f, 1.0f));
}

void WindowLayer::destroyResources(VidgfxContext *gfx)
{
	appLog(LOG_CAT)
		<< "Destroying hardware resources for layer " << getIdString();

	vidgfx_context_destroy_vertbuf(gfx, m_vertBuf);
	vidgfx_context_destroy_vertbuf(gfx, m_cursorVertBuf);
	m_vertBuf = NULL;
	m_cursorVertBuf = NULL;

	if(m_captureObj != NULL) {
		m_captureObj->release();
		m_captureObj = NULL;
	}

	m_curSize = QSize();
}

void WindowLayer::render(
	VidgfxContext *gfx, Scene *scene, uint frameNum, int numDropped)
{
	if(m_captureObj == NULL || !m_captureObj->isTextureValid())
		return; // Nothing to render

	// Has the capture object changed size since last frame?
	if(m_captureObj->getSize() != m_curSize ||
		m_captureObj->isFlipped() != m_curFlipped)
	{
		updateResources(gfx);
	}

	// Prepare texture for render
	// TODO: Filter mode selection
	QPointF pxSize, topLeft, botRight;
	QRect cropRect = m_cropInfo.calcCroppedRectForSize(m_curSize);
	VidgfxTex *tex = vidgfx_context_prepare_tex(
		gfx, m_captureObj->getTexture(), cropRect,
		m_vertBufRect.toAlignedRect().size(), GfxBilinearFilter, true, pxSize,
		topLeft, botRight);
	if(m_vertBufTlUv != topLeft || m_vertBufBrUv != botRight)
		updateVertBuf(gfx, topLeft, botRight);
	vidgfx_context_set_tex(gfx, tex);

	// sRGB back buffer correction HACK
	// TODO: 2.233333 gamma isn't really accurate, see:
	// http://chilliant.blogspot.com.au/2012/08/srgb-approximations-for-hlsl.html
	float gamma = m_gamma;
	if(vidgfx_tex_is_srgb_hack(tex))
		gamma *= 2.233333f;

	// Do the actual render
	if(vidgfx_context_set_tex_decal_effects_helper(
		gfx, gamma, m_brightness, m_contrast, m_saturation))
	{
		vidgfx_context_set_shader(gfx, GfxTexDecalGbcsShader);
	} else
		vidgfx_context_set_shader(gfx, GfxTexDecalRgbShader);
	vidgfx_context_set_topology(gfx, GfxTriangleStripTopology);
	QColor prevCol = vidgfx_context_get_tex_decal_mod_color(gfx);
	vidgfx_context_set_tex_decal_mod_color(
		gfx, QColor(255, 255, 255, (int)(getOpacity() * 255.0f)));
	if(getOpacity() != 1.0f)
		vidgfx_context_set_blending(gfx, GfxAlphaBlending);
	else
		vidgfx_context_set_blending(gfx, GfxNoBlending);
	vidgfx_context_draw_buf(gfx, m_vertBuf);
	vidgfx_context_set_tex_decal_mod_color(gfx, prevCol);

	//-------------------------------------------------------------------------
	// Render the mouse cursor if it's enabled

	// Get cursor information and return if we have nothing to do
	if(!m_captureMouse)
		return;
	QPoint globalPos;
	QPoint offset;
	bool isVisible;
	VidgfxTex *cursorTex =
		App->getSystemCursorInfo(&globalPos, &offset, &isVisible);
	if(cursorTex == NULL || !isVisible)
		return;

	// Is the cursor above our window?
	QPoint localPos = m_captureObj->mapScreenPosToLocal(globalPos);
	if(localPos.x() < cropRect.left() || localPos.x() >= cropRect.right() ||
		localPos.y() < cropRect.top() || localPos.y() >= cropRect.bottom())
	{
		// Cursor is outside our window
		return;
	}

	// Prepare texture for render
	// TODO: Filter mode selection?
	tex = vidgfx_context_prepare_tex(
		gfx, cursorTex, m_cursorVertBufRect.toAlignedRect().size(),
		GfxBilinearFilter, true, pxSize, botRight);
	updateCursorVertBuf( // Always update
		gfx, botRight, QRect(localPos + offset,
		vidgfx_tex_get_size(cursorTex)));
	vidgfx_context_set_tex(gfx, tex);

	// Do the actual render
	if(vidgfx_context_set_tex_decal_effects_helper(
		gfx, m_gamma, m_brightness, m_contrast, m_saturation))
	{
		vidgfx_context_set_shader(gfx, GfxTexDecalGbcsShader);
	} else
		vidgfx_context_set_shader(gfx, GfxTexDecalShader);
	vidgfx_context_set_topology(gfx, GfxTriangleStripTopology);
	vidgfx_context_set_tex_decal_mod_color( // Incorrect blending but we don't care
		gfx, QColor(255, 255, 255, (int)(getOpacity() * 255.0f)));
	vidgfx_context_set_blending(gfx, GfxAlphaBlending);
	vidgfx_context_draw_buf(gfx, m_cursorVertBuf);
	vidgfx_context_set_tex_decal_mod_color(gfx, prevCol);
}

quint32 WindowLayer::getTypeId() const
{
	return (quint32)LyrWindowLayerTypeId;
}

bool WindowLayer::hasSettingsDialog()
{
	return true;
}

LayerDialog *WindowLayer::createSettingsDialog(QWidget *parent)
{
	return new WindowLayerDialog(this, parent);
}

void WindowLayer::serialize(QDataStream *stream) const
{
	Layer::serialize(stream);

	// Write data version number
	*stream << (quint32)2;

	// Save our data
	*stream << (quint32)m_exeFilenameList.count();
	for(int i = 0; i < m_exeFilenameList.count(); i++) {
		*stream << m_exeFilenameList.at(i);
		*stream << m_windowTitleList.at(i);
	}
	*stream << m_captureMouse;
	*stream << capMethodToInt32(m_captureMethod);
	*stream << m_cropInfo;
	*stream << m_gamma;
	*stream << (qint32)m_brightness;
	*stream << (qint32)m_contrast;
	*stream << (qint32)m_saturation;
}

bool WindowLayer::unserialize(QDataStream *stream)
{
	if(!Layer::unserialize(stream))
		return false;

	// Read data version number
	quint32 version;
	*stream >> version;

	// Read our data
	if(version >= 0 && version <= 2) {
		qint32 int32Data;
		quint32 uint32Data;
		QString stringData;

		m_exeFilenameList.clear();
		m_windowTitleList.clear();
		*stream >> uint32Data;
		for(uint i = 0; i < uint32Data; i++) {
			*stream >> stringData;
			m_exeFilenameList.append(stringData);
			*stream >> stringData;
			m_windowTitleList.append(stringData);
		}

		*stream >> m_captureMouse;
		*stream >> uint32Data;
		m_captureMethod = int32ToCapMethod(uint32Data);

		if(version >= 1) {
			*stream >> m_cropInfo;
		} else {
			m_cropInfo.setMargins(QMargins(0, 0, 0, 0));
			m_cropInfo.setAnchors(
				CropInfo::TopLeft, CropInfo::BottomRight,
				CropInfo::TopLeft, CropInfo::BottomRight);
		}

		if(version >= 2) {
			*stream >> m_gamma;
			*stream >> int32Data;
			m_brightness = int32Data;
			*stream >> int32Data;
			m_contrast = int32Data;
			*stream >> int32Data;
			m_saturation = int32Data;
		} else {
			m_gamma = 1.0f;
			m_brightness = 0;
			m_contrast = 0;
			m_saturation = 0;
		}
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in window capture layer serialized "
			<< "data, cannot load settings";
		return false;
	}

	return true;
}

void WindowLayer::windowCreated(WinId winId)
{
	if(!isLoaded())
		return;
	if(m_captureObj != NULL)
		return;

	// A window was created and we are not capturing anything. Test the new
	// window to see if it's in our list and, if it is, capture it
	CaptureManager *mgr = App->getCaptureManager();
	const QString bExe = mgr->getWindowExeFilename(winId);
	const QString bTitle = mgr->getWindowTitle(winId);
	for(int i = 0; i < m_exeFilenameList.count(); i++) {
		const QString aExe = m_exeFilenameList.at(i);
		const QString aTitle = m_windowTitleList.at(i);
		if(!mgr->doWindowsMatch(aExe, aTitle, bExe, bTitle, true))
			continue;

		// The new window is on our list! Capture it
		m_captureObj = mgr->captureWindow(winId, m_captureMethod);
		updateResourcesIfLoaded();
		return;
	}
	// Window was not on our list
}

void WindowLayer::windowDestroyed(WinId winId)
{
	if(m_captureObj == NULL)
		return;
	if(winId != m_captureObj->getWinId())
		return;
	// Our window was deleted

	// Our object is now invalid, release it
	m_captureObj->release();
	m_captureObj = NULL;

	// HACK to find another window in our list
	m_windowListChanged = true;
	updateResourcesIfLoaded();
}

//=============================================================================
// WindowLayerFactory class

quint32 WindowLayerFactory::getTypeId() const
{
	return (quint32)LyrWindowLayerTypeId;
}

QByteArray WindowLayerFactory::getTypeString() const
{
	return QByteArrayLiteral("WindowLayer");
}

Layer *WindowLayerFactory::createBlankLayer(LayerGroup *parent)
{
	return new WindowLayer(parent);
}

Layer *WindowLayerFactory::createLayerWithDefaults(LayerGroup *parent)
{
	return new WindowLayer(parent);
}

QString WindowLayerFactory::getAddLayerString() const
{
	return QObject::tr("Add window/game capture layer");
}
