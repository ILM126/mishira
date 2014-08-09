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

#include "slideshowlayer.h"
#include "application.h"
#include "fileimagetexture.h"
#include "slideshowlayerdialog.h"
#include "layergroup.h"
#include "profile.h"
#include <Libvidgfx/graphicscontext.h>

const QString LOG_CAT = QStringLiteral("Scene");

SlideshowLayer::SlideshowLayer(LayerGroup *parent)
	: Layer(parent)
	, m_vertBufs()
	, m_imgTexs()
	, m_filenamesChanged(true)
	, m_curId(-1)
	, m_prevId(-1)
	, m_transition(1.0f)
	, m_timeSinceLastSwitch(0.0f)

	// Settings
	, m_filenames()
	, m_delayTime(8.0f)
	, m_transitionTime(4.0f)
	, m_transitionStyle(InThenOutStyle)
	, m_order(SequentialOrder)
{
	// As `render()` can be called multiple times on the same layer (Switching
	// scenes with a shared layer) we must do time-based calculations
	// separately
	connect(App, &Application::queuedFrameEvent,
		this, &SlideshowLayer::queuedFrameEvent);
}

SlideshowLayer::~SlideshowLayer()
{
	if(!m_vertBufs.isEmpty()) {
		// Should never happen
		for(int i = 0; i < m_vertBufs.size(); i++)
			delete m_vertBufs.at(i);
		m_vertBufs.clear();
	}
	if(!m_imgTexs.isEmpty()) {
		// Should never happen
		for(int i = 0; i < m_imgTexs.size(); i++)
			delete m_imgTexs.at(i);
		m_imgTexs.clear();
	}
}

void SlideshowLayer::setFilenames(const QStringList &filenames)
{
	// Do not check if the filenames are the same as the file contents might
	// have changed and the user wants to reload it.
	m_filenames = filenames;
	m_filenamesChanged = true;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void SlideshowLayer::setDelayTime(float delay)
{
	if(m_delayTime == delay)
		return; // No change
	m_delayTime = delay;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void SlideshowLayer::setTransitionTime(float transition)
{
	if(m_transitionTime == transition)
		return; // No change
	m_transitionTime = transition;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void SlideshowLayer::setTransitionStyle(SlideTransStyle style)
{
	if(m_transitionStyle == style)
		return; // No change
	m_transitionStyle = style;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void SlideshowLayer::setOrder(SlideshowOrder order)
{
	if(m_order == order)
		return; // No change
	m_order = order;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void SlideshowLayer::showEvent()
{
	// Reset to the "first" image and reset all timers and animation

	// Reset state
	m_curId = -1;
	m_prevId = -1;
	m_transition = 1.0f;

	// Immediately switch to an image
	switchToFirstImage();
}

void SlideshowLayer::hideEvent()
{
	// Pause all active timers. We do this instead of only disabling the active
	// images to be extra safe
	for(int i = 0; i < m_imgTexs.size(); i++) {
		FileImageTexture *imgTex = m_imgTexs.at(i);
		imgTex->setAnimationPaused(true);
	}
}

void SlideshowLayer::parentShowEvent()
{
	showEvent();
}

void SlideshowLayer::parentHideEvent()
{
	hideEvent();
}

void SlideshowLayer::initializeResources(GraphicsContext *gfx)
{
	appLog(LOG_CAT)
		<< "Creating hardware resources for layer " << getIdString();

	// Start invisible
	setVisibleRect(QRect());

	// Reset caching and update all resources
	m_filenamesChanged = true;
	if(!m_vertBufs.isEmpty()) {
		// Should never happen
		for(int i = 0; i < m_vertBufs.size(); i++)
			delete m_vertBufs.at(i);
		m_vertBufs.clear();
	}
	if(!m_imgTexs.isEmpty()) {
		// Should never happen
		for(int i = 0; i < m_imgTexs.size(); i++)
			delete m_imgTexs.at(i);
		m_imgTexs.clear();
	}
	updateResources(gfx);
}

void SlideshowLayer::updateResources(GraphicsContext *gfx)
{
	// Recreate textures only if they have changed
	if(m_filenamesChanged) {
		m_filenamesChanged = false;

		// Destroy existing buffers and textures
		for(int i = 0; i < m_vertBufs.size(); i++)
			delete m_vertBufs.at(i);
		m_vertBufs.clear();
		for(int i = 0; i < m_imgTexs.size(); i++)
			delete m_imgTexs.at(i);
		m_imgTexs.clear();

		// Reset state
		m_curId = -1;
		m_prevId = -1;
		m_transition = 1.0f;
		m_timeSinceLastSwitch = 0.0f;

		if(m_filenames.isEmpty())
			return; // No files specified

		// Create new buffers and textures
		for(int i = 0; i < m_filenames.size(); i++) {
			m_vertBufs.append(new TexDecalVertBuf(gfx));
			FileImageTexture *imgTex = new FileImageTexture(m_filenames.at(i));
			imgTex->setAnimationPaused(true);
			m_imgTexs.append(imgTex);
		}

		// Immediately switch to an image
		switchToFirstImage();
	}

	// Update vertex buffers when the user resizes or changes layer properties
	textureMaybeChanged(m_prevId);
	textureMaybeChanged(m_curId);

	// All other settings auto-update within the timer code
}

/// <summary>
/// Attempts to immediately switch to the "first" image.
/// </summary>
void SlideshowLayer::switchToFirstImage()
{
	switchToNextImage(true);
}

/// <summary>
/// Begin switching to the next image taking into account transition time and
/// display order.
/// </summary>
void SlideshowLayer::switchToNextImage(bool immediately)
{
	const int numImgs = m_imgTexs.size();
	switch(m_order) {
	default:
	case SequentialOrder: {
		// Switch to the next loaded image sequentially
		int nextId = m_curId;
		for(;;) {
			nextId++;
			if(nextId >= numImgs) {
				nextId = 0;
				if(m_curId < 0)
					break; // No valid image found
			}
			if(nextId == m_curId)
				break; // No valid image found
			if(switchToImage(nextId, immediately))
				break;
		}
		break; }
	case RandomOrder: {
		// Switch to a random image that isn't the current one that is already
		// loaded. If we have more than two images then also don't choose the
		// previous one.

		// Generate a list of all valid images that we can switch to except the
		// current and previous ones
		QVector<int> validImgs;
		validImgs.reserve(numImgs);
		for(int i = 0; i < numImgs; i++) {
			if(i != m_curId && i != m_prevId)
				validImgs.append(i);
		}

		// Try to switch to one of the images in the list
		bool switched = false;
		while(!validImgs.isEmpty()) {
			// Choose a random image from the list
			int index = qrand32() % validImgs.size();
			int nextId = validImgs.at(index);

			// Try switching to that image
			if(switchToImage(nextId, immediately)) {
				// Successfully switched image
				switched = true;
				break;
			}

			// We failed to switch to the image, remove it from the list and
			// try the next one
			validImgs.remove(index);
		}

		// If we didn't successfully switch to another image then we only have
		// one choice left: The previous image. We can't just add this to the
		// list above as we might have more than two images in our list but
		// only two are actually loaded due to errors.
		if(!switched) {
			if(m_prevId >= 0 && m_prevId < numImgs)
				switchToImage(m_prevId, immediately);
			// If the above fails then we just keep the current image displayed
		}

		break; }
	}
}

/// <summary>
/// Begin switching to the specified image taking into account transition time
/// if `immediately` is false or immediately if it is true.
/// </summary>
/// <returns>False if the specified image is invalid</returns>
bool SlideshowLayer::switchToImage(int id, bool immediately)
{
	if(m_curId == id)
		return false; // Already displaying that image
	if(id < 0 || id >= m_imgTexs.size())
		return false; // Invalid image
	FileImageTexture *imgTex = m_imgTexs.at(id);
	if(!imgTex->isLoaded())
		return false; // Image isn't loaded yet

	// An immediate transition time is the same as always immediately
	if(m_transitionTime <= 0.0f)
		immediately = true;

	// Make extra sure that the image previous to the current one has had its
	// animations disabled. This is mostly for safety.
	if(m_prevId >= 0 && m_prevId < m_imgTexs.size()) {
		FileImageTexture *prevImgTex = m_imgTexs.at(m_prevId);
		prevImgTex->setAnimationPaused(true);
	}

	// Do the actual switch
	m_prevId = m_curId;
	m_curId = id;
	m_timeSinceLastSwitch = 0.0f;

	// Enable animation for the image to make visible if it has any and fully
	// reset it to the first frame.
	imgTex->setAnimationPaused(false);
	imgTex->resetAnimation(true, true);

	// Reset our transition timer
	if(immediately) {
		m_transition = 1.0f;
		m_timeSinceLastSwitch += m_transitionTime;
	} else {
		m_transition = AnimatedFloat(
			0.0f, 1.0f, QEasingCurve::Linear, m_transitionTime);
	}

	// Update visible rectangle and vertex buffer as required
	textureMaybeChanged(id);

	// If we immediately changed image then disable the animation of the
	// previous image immediately as well if it has any. If we're not
	// immediately switching then the animation will be disabled once the
	// transition completes.
	if(immediately && m_prevId >= 0 && m_prevId < m_imgTexs.size()) {
		FileImageTexture *prevImgTex = m_imgTexs.at(m_prevId);
		prevImgTex->setAnimationPaused(true);
	}

	return true;
}

void SlideshowLayer::textureMaybeChanged(int id)
{
	if(id < 0 || id >= m_imgTexs.size())
		return; // Invalid image
	FileImageTexture *imgTex = m_imgTexs.at(id);
	TexDecalVertBuf *vertBuf = m_vertBufs.at(id);

	// Update vertex buffer and visible rectangle
	if(imgTex == NULL) {
		if(id == m_curId)
			setVisibleRect(QRect()); // Layer is invisible
		return;
	}
	Texture *tex = imgTex->getTexture();
	if(tex == NULL) {
		if(id == m_curId)
			setVisibleRect(QRect()); // Layer is invisible
		return;
	}
	const QRectF rect = scaledRectFromActualSize(tex->getSize());
	vertBuf->setRect(rect);
	if(id == m_curId)
		setVisibleRect(rect.toAlignedRect());
}

void SlideshowLayer::destroyResources(GraphicsContext *gfx)
{
	appLog(LOG_CAT)
		<< "Destroying hardware resources for layer " << getIdString();

	for(int i = 0; i < m_vertBufs.size(); i++)
		delete m_vertBufs.at(i);
	m_vertBufs.clear();
	for(int i = 0; i < m_imgTexs.size(); i++)
		delete m_imgTexs.at(i);
	m_imgTexs.clear();
}

void SlideshowLayer::renderImage(GraphicsContext *gfx, int id, float opacity)
{
	if(opacity <= 0.0f)
		return; // Invisible image
	if(id < 0 || id >= m_imgTexs.size())
		return; // Invalid image
	TexDecalVertBuf *texVertBuf = m_vertBufs.at(id);
	FileImageTexture *imgTex = m_imgTexs.at(id);
	Texture *tex = imgTex->getTexture();
	if(tex == NULL)
		return; // Image not loaded yet or an error occurred during load

	// Prepare texture for render. TODO: If the image doesn't have animation
	// then we can optimize rendering by caching the prepared texture data. We
	// still need to keep the original though just in case the user resizes the
	// layer.
	// TODO: Filter mode selection
	QPointF pxSize, botRight;
	tex = gfx->prepareTexture(
		tex, getVisibleRect().size(), GfxBilinearFilter, true,
		pxSize, botRight);
	texVertBuf->setTextureUv(QPointF(), botRight, GfxUnchangedOrient);
	gfx->setTexture(tex);

	// Do the actual render
	VertexBuffer *vertBuf = texVertBuf->getVertBuf();
	if(vertBuf != NULL) {
		gfx->setShader(GfxTexDecalShader);
		gfx->setTopology(texVertBuf->getTopology());
		if(imgTex->hasTransparency() || opacity < 1.0f)
			gfx->setBlending(GfxAlphaBlending);
		else
			gfx->setBlending(GfxNoBlending);
		QColor oldCol = gfx->getTexDecalModColor();
		gfx->setTexDecalModColor(
			QColor(255, 255, 255, (int)(opacity * getOpacity() * 255.0f)));
		gfx->drawBuffer(vertBuf);
		gfx->setTexDecalModColor(oldCol);
	}
}

void SlideshowLayer::render(
	GraphicsContext *gfx, Scene *scene, uint frameNum, int numDropped)
{
	// Switch to the first image the moment that it loads
	if(m_curId < 0) {
		switchToFirstImage();
		if(m_curId < 0)
			return; // Nothing to display
	}

	if(m_transition.atEnd()) {
		// No transition animation is currently active, just render a single
		// image as efficiently as possible
		renderImage(gfx, m_curId, 1.0f);
	} else {
		// We are currently transitioning between two images
		switch(m_transitionStyle) {
		default:
		case InThenOutStyle: {
			float oldOpacity =
				1.0f - qMax(0.0f, m_transition.currentValue() * 2.0f - 1.0f);
			float newOpacity =
				qMin(1.0f, m_transition.currentValue() * 2.0f);
			renderImage(gfx, m_prevId, oldOpacity);
			renderImage(gfx, m_curId, newOpacity);
			break; }
		case InAndOutStyle: {
			float oldOpacity = 1.0f - m_transition.currentValue();
			float newOpacity = m_transition.currentValue();
			renderImage(gfx, m_prevId, oldOpacity);
			renderImage(gfx, m_curId, newOpacity);
			break; }
		}
	}
}

LyrType SlideshowLayer::getType() const
{
	return LyrSlideshowLayerType;
}

bool SlideshowLayer::hasSettingsDialog()
{
	return true;
}

LayerDialog *SlideshowLayer::createSettingsDialog(QWidget *parent)
{
	return new SlideshowLayerDialog(this, parent);
}

void SlideshowLayer::serialize(QDataStream *stream) const
{
	Layer::serialize(stream);

	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << m_filenames;
	*stream << m_delayTime;
	*stream << m_transitionTime;
	*stream << (quint32)m_transitionStyle;
	*stream << (quint32)m_order;
}

bool SlideshowLayer::unserialize(QDataStream *stream)
{
	if(!Layer::unserialize(stream))
		return false;

	quint32 version;

	// Read data version number
	*stream >> version;

	// Read our data
	if(version == 0) {
		quint32 uint32Data;

		*stream >> m_filenames;
		*stream >> m_delayTime;
		*stream >> m_transitionTime;
		*stream >> uint32Data;
		m_transitionStyle = (SlideTransStyle)uint32Data;
		*stream >> uint32Data;
		m_order = (SlideshowOrder)uint32Data;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in slideshow layer serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

void SlideshowLayer::queuedFrameEvent(uint frameNum, int numDropped)
{
	if(!isVisibleSomewhere())
		return;

	// Forward to image texture helpers in order for animations to process.
	// TODO: We most likely process this AFTER the frame we want due to Qt
	// signal processing order
	for(int i = 0; i < m_imgTexs.size(); i++) {
		FileImageTexture *imgTex = m_imgTexs.at(i);
		if(imgTex->processFrameEvent(frameNum, numDropped))
			textureMaybeChanged(i);
	}

	// Increment our transition timer
	Fraction framerate = App->getProfile()->getVideoFramerate();
	float period = 1.0f / framerate.asFloat();
	m_transition.addTime((float)(numDropped + 1) * period);

	// If the previous image is no longer visible then disable its animation if
	// it has any
	if(m_transition.atEnd() && m_prevId >= 0)
		m_imgTexs.at(m_prevId)->setAnimationPaused(true);

	// If it's time to switch to the next image then do so
	m_timeSinceLastSwitch += (float)(numDropped + 1) * period;
	if(m_timeSinceLastSwitch >= m_transitionTime + m_delayTime) {
		if(m_curId < 0)
			switchToFirstImage();
		else
			switchToNextImage();
	}
}
