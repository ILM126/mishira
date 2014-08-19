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

#include "scaler.h"
#include "application.h"
#include "scene.h"
#include "profile.h"

//=============================================================================
// Helpers

/// <summary>
/// Create a test NV12 image for encoding. Based on an old FFmpeg example:
/// http://ffmpeg.org/doxygen/0.6/output-example_8c-source.html
/// </summary>
static void testNV12Image(
	NV12Frame *img, int frameNum, int width, int height)
{
	// Y
	int off = 0;
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			img->yPlane[off] = x + y + frameNum * 3;
			off++;
		}
	}

	// Cb and Cr
	int hWidth = width / 2;
	int hHeight = height / 2;
	off = 0;
	for(int y = 0; y < hHeight; y++) {
		for(int x = 0; x < hWidth; x++) {
			img->uvPlane[off+0] = (128 + y + frameNum * 2) * 2;
			img->uvPlane[off+1] = (64 + x + frameNum * 5) * 2;
			off += 2;
		}
	}
}

//=============================================================================
// Scaler class

static void gfxInitializedHandler(void *opaque, VidgfxContext *context)
{
	Scaler *scaler = static_cast<Scaler *>(opaque);
	scaler->initializeResources(context);
}

static void gfxDestroyingHandler(void *opaque, VidgfxContext *context)
{
	Scaler *scaler = static_cast<Scaler *>(opaque);
	scaler->destroyResources(context);
}

QVector<Scaler *> Scaler::s_instances;

Scaler *Scaler::getOrCreate(
	Profile *profile, QSize size, SclrScalingMode scaling,
	VidgfxFilter scaleFilter, VidgfxPixFormat pixelFormat)
{
	// If an instance already exists then return it
	for(int i = 0; i < s_instances.count(); i++) {
		Scaler *scaler = s_instances.at(i);
		if(profile == scaler->getProfile() && size == scaler->getSize() &&
			scaling == scaler->getScaling() &&
			scaleFilter == scaler->getScaleFilter())
		{
			scaler->m_ref++;
			return scaler;
		}
	}

	// Create a new instance
	Scaler *scaler = new Scaler(
		profile, size, scaling, scaleFilter, pixelFormat);
	s_instances.push_back(scaler);
	return scaler;
}

void Scaler::graphicsContextInitialized(VidgfxContext *gfx)
{
	if(gfx == NULL)
		return; // Extra safe

	// Forward the signal to all scalers
	for(int i = 0; i < s_instances.count(); i++) {
		Scaler *scaler = s_instances.at(i);
		if(vidgfx_context_is_valid(gfx))
			scaler->initializeResources(gfx);
		else {
			vidgfx_context_add_initialized_callback(
				gfx, gfxInitializedHandler, scaler);
		}
		vidgfx_context_add_destroying_callback(
			gfx, gfxDestroyingHandler, scaler);
	}
}

Scaler::Scaler(
	Profile *profile, QSize size, SclrScalingMode scaling,
	VidgfxFilter scaleFilter, VidgfxPixFormat pixelFormat)
	: QObject()
	, m_profile(profile)
	, m_size(size)
	, m_scaling(scaling)
	, m_scaleFilter(scaleFilter)
	, m_pixelFormat(pixelFormat)
	, m_ref(1)

	, m_quarterWidthBuf(NULL)
	, m_quarterWidthBufBrUv(0.0f, 0.0f)
	, m_nv12Buf(NULL)
	//, m_yuvScratchTex()
	//, m_stagingYTex()
	//, m_stagingUVTex()
	//, m_delayedFrameNum()
	//, m_delayedNumDropped()
	, m_prevStagingTex(0)
	, m_startDelay(SCALER_DELAY)
{
	m_yuvScratchTex[0] = NULL;
	m_yuvScratchTex[1] = NULL;
	m_yuvScratchTex[2] = NULL;
	m_stagingYTex[0] = NULL;
	m_stagingYTex[1] = NULL;
	m_stagingUVTex[0] = NULL;
	m_stagingUVTex[1] = NULL;
	m_delayedFrameNum[0] = 0;
	m_delayedFrameNum[1] = 0;
	m_delayedNumDropped[0] = 0;
	m_delayedNumDropped[1] = 0;

	// Watch the profile for rendered frames
	connect(m_profile, &Profile::frameRendered,
		this, &Scaler::frameRendered);

	// As scalers can be constructed before the graphics context make sure we
	// delay initialization of our hardware resources until it's safe
	VidgfxContext *gfx = App->getGraphicsContext();
	if(gfx != NULL) {
		if(vidgfx_context_is_valid(gfx))
			initializeResources(gfx);
		else {
			vidgfx_context_add_initialized_callback(
				gfx, gfxInitializedHandler, this);
		}
		vidgfx_context_add_destroying_callback(
			gfx, gfxDestroyingHandler, this);
	}
}

Scaler::~Scaler()
{
	// Remove callbacks
	VidgfxContext *gfx = App->getGraphicsContext();
	if(vidgfx_context_is_valid(gfx)) {
		vidgfx_context_remove_initialized_callback(
			gfx, gfxInitializedHandler, this);
		vidgfx_context_remove_destroying_callback(
			gfx, gfxDestroyingHandler, this);
	}

	// Destroy hardware resources if required
	if(gfx != NULL)
		destroyResources(gfx);
}

/// <summary>
/// Destroy the scaler instance if no other encoder is referencing it. The
/// pointer that was used to call this method is no longer valid after this
/// method returns.
/// </summary>
void Scaler::release()
{
	m_ref--;
	if(m_ref != 0)
		return;
	int index = s_instances.indexOf(this);
	if(index >= 0)
		s_instances.remove(index);

	// Don't delete immediately as we might be being dereferenced from within
	// a `nv12FrameReady()` signal which will result in crashes
	this->deleteLater();
}

void Scaler::frameRendered(VidgfxTex *tex, uint frameNum, int numDropped)
{
	// If the user is only previewing, don't waste any resources transferring
	// the frames to system memory
	if(!App->processAllFramesEnabled())
		return;

	// We only support NV12 output at the moment
	if(m_pixelFormat != GfxNV12Format)
		return;

	// Get the graphics context
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be usuable

	//-------------------------------------------------------------------------
	// Output a test image to the video encoders instead of using the actual
	// canvas texture data

#if 0
	static quint8 *yBuf = new quint8[m_size.width() * m_size.height()];
	static quint8 *uvBuf = new quint8[m_size.width() * m_size.height() / 2];
	NV12Frame frame;
	frame.yPlane = yBuf;
	frame.uvPlane = uvBuf;
	frame.yStride = m_size.width();
	frame.uvStride = m_size.width();
	testNV12Image(&frame, frameNum, m_size.width(), m_size.height());

	emit nv12FrameReady(frame, frameNum, numDropped);
	// No need to delete[] static buffers
	return;
#endif // 0

	//-------------------------------------------------------------------------
	// Process previous frame

	// In order to guarentee that we never block the CPU we wait 2 frames
	// before attempting to read the staging texture. For a detailed
	// explanation on why 1 frame isn't enough see:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/bb205132%28v=vs.85%29.aspx

	// Which staging texture are we using for this frame? This texture is both
	// read from and written to this iteration
	int curStagingTex = m_prevStagingTex ^ 1;

	// If we have a previous frame in system RAM, map it and forward it to the
	// video encoders
	if(!m_startDelay) {
		// Shorthand
		VidgfxTex *yTex = m_stagingYTex[curStagingTex];
		VidgfxTex *uvTex = m_stagingUVTex[curStagingTex];

		vidgfx_tex_map(yTex);
		vidgfx_tex_map(uvTex);
		if(vidgfx_tex_is_mapped(yTex) && vidgfx_tex_is_mapped(uvTex)) {
			NV12Frame frame;
			frame.yPlane =
				static_cast<quint8 *>(vidgfx_tex_get_data_ptr(yTex));
			frame.uvPlane =
				static_cast<quint8 *>(vidgfx_tex_get_data_ptr(uvTex));
			frame.yStride = vidgfx_tex_get_stride(yTex);
			frame.uvStride = vidgfx_tex_get_stride(uvTex);
			emit nv12FrameReady(
				frame, m_delayedFrameNum[curStagingTex],
				m_delayedNumDropped[curStagingTex]);
		} else {
			// TODO: Should we increment the number of dropped frames if this
			// fails? What about if all the code below fails?
		}

		vidgfx_tex_unmap(yTex);
		vidgfx_tex_unmap(uvTex);
	}
	if(m_startDelay > 0)
		m_startDelay--;

	//-------------------------------------------------------------------------
	// Do scaling and texture preparation for the first pass

	QPointF pxSize, botRight;
	tex = vidgfx_context_prepare_tex(
		gfx, tex, m_size, m_scaleFilter, true, pxSize, botRight);
	if(botRight != m_quarterWidthBufBrUv)
		updateVertBuf(gfx, botRight);
	vidgfx_context_set_rgb_nv16_px_size(gfx, pxSize);

	//-------------------------------------------------------------------------
	// Do RGB->YUV conversion. Useful information relating to YUV formats can
	// be found at:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd206750%28v=vs.85%29.aspx

	const QSize nv16Size = vidgfx_tex_get_size(m_yuvScratchTex[0]);
	const QSize nv12Size = vidgfx_tex_get_size(m_yuvScratchTex[2]);

	// Setup render targets
	vidgfx_context_set_user_render_target(
		gfx, m_yuvScratchTex[0], m_yuvScratchTex[1]);
	vidgfx_context_set_user_render_target_viewport(gfx, nv16Size);
	vidgfx_context_set_render_target(gfx, GfxUserTarget);

	// Update view and projection matrix for first pass
	QMatrix4x4 mat;
	vidgfx_context_set_view_mat(gfx, mat); // Must be set as it's undefined otherwise
	mat.ortho(0.0f, nv16Size.width(), nv16Size.height(), 0.0f, -1.0f, 1.0f);
	vidgfx_context_set_proj_mat(gfx, mat);

	// Render first pass (RGB->NV16)
	vidgfx_context_set_shader(gfx, GfxRgbNv16Shader);
	vidgfx_context_set_topology(gfx, GfxTriangleStripTopology);
	vidgfx_context_set_blending(gfx, GfxNoBlending);
	vidgfx_context_set_tex(gfx, tex);
	vidgfx_context_draw_buf(gfx, m_quarterWidthBuf);

	// Update view and projection matrix for second pass
	mat.setToIdentity();
	vidgfx_context_set_view_mat(gfx, mat);
	mat.setToIdentity();
	mat.ortho(0.0f, nv12Size.width(), nv12Size.height(), 0.0f, -1.0f, 1.0f);
	vidgfx_context_set_proj_mat(gfx, mat);

	// Render second pass (NV16->NV12)
	vidgfx_context_set_user_render_target(gfx, m_yuvScratchTex[2]);
	vidgfx_context_set_user_render_target_viewport(gfx, nv12Size);
	vidgfx_context_set_shader(gfx, GfxTexDecalShader);
	vidgfx_context_set_tex_filter(gfx, GfxBilinearFilter);
	vidgfx_context_set_tex(gfx, m_yuvScratchTex[1]);
	vidgfx_context_draw_buf(gfx, m_nv12Buf);

	//-------------------------------------------------------------------------
	// Copy the output to our staging textures so they are ready to be read at
	// a later time in the future

	vidgfx_context_copy_tex_data(
		gfx, m_stagingYTex[curStagingTex], m_yuvScratchTex[0], QPoint(0, 0),
		QRect(0, 0, nv16Size.width(), nv16Size.height()));
	vidgfx_context_copy_tex_data(
		gfx, m_stagingUVTex[curStagingTex], m_yuvScratchTex[2], QPoint(0, 0),
		QRect(0, 0, nv12Size.width(), nv12Size.height()));

	// We need to delay the frame data as well
	m_delayedFrameNum[curStagingTex] = frameNum;
	m_delayedNumDropped[curStagingTex] = numDropped;

	// Swap staging textures for next time
	m_prevStagingTex = curStagingTex;
}

void Scaler::updateVertBuf(VidgfxContext *gfx, const QPointF &brUv)
{
	if(m_quarterWidthBuf == NULL || m_yuvScratchTex[0] == NULL)
		return;
	if(m_quarterWidthBufBrUv == brUv)
		return;

	// Calculate canvas rectangle based on scaling mode
	int halfWidth = (m_size.width() + 1) / 2; // Int ceil()
	const QRectF boundRect = QRectF(0.0f, 0.0f,
		(float)halfWidth * 0.5f, (float)m_size.height());
	QSizeF canvasSize = m_profile->getCanvasSize();
	canvasSize.setWidth(canvasSize.width() * 0.25f);
	QRectF rect = createScaledRectInBoundsF(
		canvasSize, boundRect, convertScaling(m_scaling),
		LyrMiddleCenterAlign);

	// Round to the nearest packed texel to prevent blurring
	rect.setLeft((float)qRound(rect.left() * 2.0f) * 0.5f);
	rect.setRight((float)qRound(rect.right() * 2.0f) * 0.5f);
	rect.setTop((float)qRound(rect.top()));
	rect.setBottom((float)qRound(rect.bottom()));

	// Ensure that if the output rectangle only fills half a pixel it will
	// still be rendered by the graphics card by making it overlap the sampling
	// point.
	if(qFuzzyCompare((float)rect.left() - floorf((float)rect.left()), 0.5f))
		rect.setLeft((float)rect.left() - 0.005f);
	if(qFuzzyCompare((float)rect.right() - floorf((float)rect.right()), 0.5f))
		rect.setRight((float)rect.right() + 0.005f);

	m_quarterWidthBufBrUv = brUv;
	vidgfx_create_tex_decal_rect(
		m_quarterWidthBuf, rect, m_quarterWidthBufBrUv);
}

LyrScalingMode Scaler::convertScaling(SclrScalingMode mode) const
{
	switch(mode) {
	case SclrStretchScale:
		return LyrStretchScale;
		break;
	case SclrSnapToInnerScale:
		return LyrSnapToInnerScale;
		break;
	case SclrSnapToOuterScale:
		return LyrSnapToOuterScale;
		break;
	}
	// Should never be reached
	Q_ASSERT(false);
	return LyrSnapToInnerScale;
}

void Scaler::initializeResources(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be useable

	// TODO: Detect failure

	// Integer ceil() = (x + y - 1) / y;
	const QSize nv16Size = QSize(
		(m_size.width() + 3) / 4, m_size.height());
	const QSize nv12Size = QSize(
		(m_size.width() + 3) / 4, m_size.height() / 2);

	// Scratch textures
	// 0 = packed Y, 1 = packed UV in NV16 format, 2 = packed UV in NV12 format
	m_yuvScratchTex[0] = vidgfx_context_new_tex(gfx, nv16Size, false, true);
	m_yuvScratchTex[1] = vidgfx_context_new_tex(gfx, nv16Size, false, true);
	m_yuvScratchTex[2] = vidgfx_context_new_tex(gfx, nv12Size, false, true);

	// Initialize textures with YUV black
	vidgfx_context_set_user_render_target(gfx, m_yuvScratchTex[0]);
	vidgfx_context_set_render_target(gfx, GfxUserTarget);
	vidgfx_context_clear(gfx, QColor(0, 0, 0, 0));
	//vidgfx_context_clear(gfx, QColor(16, 16, 16, 16)); // BT.601 black?
	//vidgfx_context_clear(gfx, QColor(255, 255, 255, 255)); // White
	vidgfx_context_set_user_render_target(gfx, m_yuvScratchTex[1]);
	vidgfx_context_clear(gfx, QColor(127, 127, 127, 127));

	// Staging textures
	m_stagingYTex[0] = vidgfx_context_new_staging_tex(gfx, nv16Size);
	m_stagingYTex[1] = vidgfx_context_new_staging_tex(gfx, nv16Size);
	m_stagingUVTex[0] = vidgfx_context_new_staging_tex(gfx, nv12Size);
	m_stagingUVTex[1] = vidgfx_context_new_staging_tex(gfx, nv12Size);

	// RGB->NV16 vertex buffer
	m_quarterWidthBuf = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_TEX_DECAL_RECT_BUF_SIZE);
	m_quarterWidthBufBrUv = QPointF(0.0f, 0.0f);
	// Assume that the bottom-right UV is at (1, 1) for now
	updateVertBuf(gfx, QPointF(1.0f, 1.0f));

	// NV16->NV12 vertex buffer
	m_nv12Buf = vidgfx_context_new_vertbuf(
		gfx, VIDGFX_TEX_DECAL_RECT_BUF_SIZE);
	if(m_nv12Buf != NULL) {
		vidgfx_create_tex_decal_rect(
			m_nv12Buf,
			QRectF(QPoint(0.0f, 0.0f),
			vidgfx_tex_get_size(m_yuvScratchTex[2])));
	}
}

void Scaler::destroyResources(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be useable

	// Vertex buffers
	if(m_quarterWidthBuf != NULL)
		vidgfx_context_destroy_vertbuf(gfx, m_quarterWidthBuf);
	if(m_nv12Buf != NULL)
		vidgfx_context_destroy_vertbuf(gfx, m_nv12Buf);
	m_quarterWidthBuf = NULL;
	m_nv12Buf = NULL;

	// Scratch textures
	if(m_yuvScratchTex[0] != NULL)
		vidgfx_context_destroy_tex(gfx, m_yuvScratchTex[0]);
	if(m_yuvScratchTex[1] != NULL)
		vidgfx_context_destroy_tex(gfx, m_yuvScratchTex[1]);
	if(m_yuvScratchTex[2] != NULL)
		vidgfx_context_destroy_tex(gfx, m_yuvScratchTex[2]);
	m_yuvScratchTex[0] = NULL;
	m_yuvScratchTex[1] = NULL;
	m_yuvScratchTex[2] = NULL;

	// Staging textures
	if(m_stagingYTex[0] != NULL)
		vidgfx_context_destroy_tex(gfx, m_stagingYTex[0]);
	if(m_stagingYTex[1] != NULL)
		vidgfx_context_destroy_tex(gfx, m_stagingYTex[1]);
	if(m_stagingUVTex[0] != NULL)
		vidgfx_context_destroy_tex(gfx, m_stagingUVTex[0]);
	if(m_stagingUVTex[1] != NULL)
		vidgfx_context_destroy_tex(gfx, m_stagingUVTex[1]);
	m_stagingYTex[0] = NULL;
	m_stagingYTex[1] = NULL;
	m_stagingUVTex[0] = NULL;
	m_stagingUVTex[1] = NULL;
}

//=============================================================================
// TestScaler class

TestScaler::TestScaler(QSize size)
	: QObject()
	, m_size(size)
	, m_yBuf(NULL)
	, m_uvBuf(NULL)
{
	m_yBuf = new quint8[m_size.width() * m_size.height()];
	m_uvBuf = new quint8[m_size.width() * m_size.height() / 2];
}

TestScaler::~TestScaler()
{
	delete[] m_yBuf;
	delete[] m_uvBuf;
}

void TestScaler::emitFrameRendered(uint frameNum, int numDropped)
{
	// Generate test image
	NV12Frame frame;
	frame.yPlane = m_yBuf;
	frame.uvPlane = m_uvBuf;
	frame.yStride = m_size.width();
	frame.uvStride = m_size.width();
	testNV12Image(&frame, frameNum, m_size.width(), m_size.height());

	// Emit actual signal
	emit nv12FrameReady(frame, frameNum, numDropped);
}
