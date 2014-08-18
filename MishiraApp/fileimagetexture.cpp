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

#include "fileimagetexture.h"
#include "application.h"
#include "asyncio.h"
#include "profile.h"
#include <Libvidgfx/graphicscontext.h>

const QString LOG_CAT = QStringLiteral("Scene");

FileImageTexture::FileImageTexture(const QString &filename)
	: QObject()
	, m_filename(filename)
	, m_animationPaused(false)

	// Image data
	, m_imgData()
	, m_imgLoadOperation(0)
	, m_imgBuffer(&m_imgData)
	, m_imgReader()
	, m_tex(NULL)

	// Image state
	, m_hasAlpha(false)
	, m_hasAnimation(false)
	, m_loopCount(0)
	, m_usecToNextFrame(0)
	, m_isFirstFrameAfterLoad(false)
{
	if(m_filename.isEmpty())
		return; // No file specified, should never happen

	// Read all file data into memory without blocking
	AsyncIO *asyncIo = App->getAsyncIO();
	connect(asyncIo, &AsyncIO::loadFromFileComplete,
		this, &FileImageTexture::imgLoadComplete);
	m_imgLoadOperation = asyncIo->newOperationId();
	asyncIo->loadFromFile(m_imgLoadOperation, m_filename);

	// Continued in `imgLoadComplete()`
}

FileImageTexture::~FileImageTexture()
{
	if(m_tex != NULL) {
		VidgfxContext *gfx = App->getGraphicsContext();
		if(vidgfx_context_is_valid(gfx)) {
			vidgfx_context_destroy_tex(gfx, m_tex);
			m_tex = NULL;
		}
	}

	// Not really required but clean up the image reader for animations
	m_imgReader.setDevice(NULL);
	m_imgBuffer.close();
	m_imgData = QByteArray();
}

void FileImageTexture::setAnimationPaused(bool paused)
{
	m_animationPaused = paused;
}

/// <returns>True of the image's texture has changed</returns>
bool FileImageTexture::resetAnimation(bool fullReset, bool updateTexture)
{
	if(m_tex == NULL || !m_hasAnimation)
		return false; // Nothing to process

	// Qt's internal QGifHandler doesn't support frame jumping meaning the only
	// way we can return to the start of the animation is to completely reload
	// the image reader. :(
	m_imgReader.setDevice(NULL);
	m_imgBuffer.close();
	m_imgReader.setDevice(&m_imgBuffer);

	// A "full reset" behaves like the image was completely reloaded while a
	// partial reset just returns to the first frame.
	if(fullReset)
		m_loopCount = 0;

	// Upload the first frame to the texture if requested
	if(updateTexture) {
		QImage img = m_imgReader.read();
		vidgfx_tex_update_data(m_tex, img);
		m_usecToNextFrame = m_imgReader.nextImageDelay() * 1000;
		return true;
	}

	return false;
}

/// <summary>
/// Should be called in `queuedFrameEvent()` so that animations can be
/// processed correctly.
/// </summary>
/// <returns>True of the image's texture has changed</returns>
bool FileImageTexture::processFrameEvent(uint frameNum, int numDropped)
{
	if(m_tex == NULL)
		return false; // Nothing to do

	// Always return true immediately after our image is loaded for the first
	// time.
	bool ret = false;
	if(m_isFirstFrameAfterLoad) {
		ret = true;
		m_isFirstFrameAfterLoad = false;
	}

	if(!m_hasAnimation || m_animationPaused)
		return ret; // Nothing to process

	// Process animations
	int maxLoops = m_imgReader.loopCount();
	if(maxLoops < 0 || m_loopCount <= maxLoops) {
		// Get the current frame
		Fraction freq = App->getProfile()->getVideoFramerate();
		int numFrames = numDropped + 1;
		m_usecToNextFrame -=
			(numFrames * 1000000 * freq.denominator) / freq.numerator;
		QImage img;
		while(m_usecToNextFrame < 0) {
			img = m_imgReader.read();
			m_usecToNextFrame += m_imgReader.nextImageDelay() * 1000;
			if(!m_imgReader.canRead()) {
				// At end of animation, loop around to the start. We don't want
				// to do a full reset as we need to remember the loop count and
				// we want to keep the current frame visible.
				resetAnimation(false, false);
				m_loopCount++;
			}
		}
		vidgfx_tex_update_data(m_tex, img);
		return true;
	}

	return ret;
}

void FileImageTexture::imgLoadComplete(
	int id, int errorCode, const QByteArray &data)
{
	// Process result
	if(m_imgLoadOperation != id || id == 0)
		return; // Not our signal
	m_imgLoadOperation = 0;
	if(errorCode != 0) {
		appLog(LOG_CAT, Log::Warning)
			<< "Cannot open image file \"" << m_filename << "\". "
			<< "Reason = " << errorCode;
		return;
	}

	// No point continuing processing if we can't create the texture. This
	// should never happen as this object should only exist while a context
	// already exists.
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return;

	// Keep the raw file data in memory until we don't need it anymore
	m_imgData = data;

	// Create image reader
	m_imgReader.setDevice(&m_imgBuffer);
	if(!m_imgReader.canRead()) {
		appLog(LOG_CAT, Log::Warning)
			<< "Unrecognised image file \"" << m_filename << "\"";
		return;
	}
	//QSize texSize = m_imgReader.size();
	m_hasAnimation = (m_imgReader.imageCount() > 1);
	if(m_hasAnimation) {
		appLog(LOG_CAT)
			<< "Image file \"" << m_filename << "\" has animation";
	}

	// Read first frame
	QImage img = m_imgReader.read();
	if(img.format() == QImage::Format_Invalid)
		return; // Silently fail
	m_hasAlpha = img.hasAlphaChannel(); // TODO: Actually test pixels if no animation

	if(m_hasAnimation) {
		// Get delay until the next frame. We don't really care for
		// synchronization so the small timing errors due to doing it this
		// way is acceptable (See main loop to see how to do it properly)
		m_usecToNextFrame = m_imgReader.nextImageDelay() * 1000;
		if(!m_imgReader.canRead())
			m_hasAnimation = false; // 1 frame animation
	}

	// If the image has no animation we can unload the buffer from memory
	if(!m_hasAnimation) {
		m_imgReader.setDevice(NULL);
		m_imgBuffer.close();
		m_imgData = QByteArray();
	}

	// Create new texture object
	if(m_hasAlpha && !m_hasAnimation) {
		// Prevent colour fringing around fully transparent pixels due to
		// bilinear filtering by diluting the colour information
		vidgfx_context_dilute_img(gfx, img);
	}
	m_tex = vidgfx_context_new_tex(gfx, img, m_hasAnimation);
	m_isFirstFrameAfterLoad = true;
}
