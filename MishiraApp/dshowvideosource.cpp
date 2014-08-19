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

// Make sure our custom GUIDs are initialized
#define INIT_OUR_GUID 1

#include "dshowvideosource.h"
#include "application.h"
#include "profile.h"
#include "videosourcemanager.h"
#include <dshow.h>
#include <dvdmedia.h>
#include <QtGui/5.0.2/QtGui/qpa/qplatformnativeinterface.h>

const QString LOG_CAT = QStringLiteral("DirectShow");

#define DEBUG_DIRECTSHOW 0

//=============================================================================
// Helpers

QString getDShowErrorCode(HRESULT res)
{
	switch(res) {
	case VFW_S_NO_MORE_ITEMS:
		return QStringLiteral("VFW_S_NO_MORE_ITEMS");
	case VFW_S_DUPLICATE_NAME:
		return QStringLiteral("VFW_S_DUPLICATE_NAME");
	case VFW_S_STATE_INTERMEDIATE:
		return QStringLiteral("VFW_S_STATE_INTERMEDIATE");
	case VFW_S_PARTIAL_RENDER:
		return QStringLiteral("VFW_S_PARTIAL_RENDER");
	case VFW_S_SOME_DATA_IGNORED:
		return QStringLiteral("VFW_S_SOME_DATA_IGNORED");
	case VFW_S_CONNECTIONS_DEFERRED:
		return QStringLiteral("VFW_S_CONNECTIONS_DEFERRED");
	case VFW_S_RESOURCE_NOT_NEEDED:
		return QStringLiteral("VFW_S_RESOURCE_NOT_NEEDED");
	case VFW_S_MEDIA_TYPE_IGNORED:
		return QStringLiteral("VFW_S_MEDIA_TYPE_IGNORED");
	case VFW_S_VIDEO_NOT_RENDERED:
		return QStringLiteral("VFW_S_VIDEO_NOT_RENDERED");
	case VFW_S_AUDIO_NOT_RENDERED:
		return QStringLiteral("VFW_S_AUDIO_NOT_RENDERED");
	case VFW_S_RPZA:
		return QStringLiteral("VFW_S_RPZA");
	case VFW_S_ESTIMATED:
		return QStringLiteral("VFW_S_ESTIMATED");
	case VFW_S_RESERVED:
		return QStringLiteral("VFW_S_RESERVED");
	case VFW_S_STREAM_OFF:
		return QStringLiteral("VFW_S_STREAM_OFF");
	case VFW_S_CANT_CUE:
		return QStringLiteral("VFW_S_CANT_CUE");
	case VFW_S_NOPREVIEWPIN:
		return QStringLiteral("VFW_S_NOPREVIEWPIN");
	case VFW_S_DVD_NON_ONE_SEQUENTIAL:
		return QStringLiteral("VFW_S_DVD_NON_ONE_SEQUENTIAL");
	case VFW_S_DVD_CHANNEL_CONTENTS_NOT_AVAILABLE:
		return QStringLiteral("VFW_S_DVD_CHANNEL_CONTENTS_NOT_AVAILABLE");
	case VFW_S_DVD_NOT_ACCURATE:
		return QStringLiteral("VFW_S_DVD_NOT_ACCURATE");
	case VFW_E_INVALIDMEDIATYPE:
		return QStringLiteral("VFW_E_INVALIDMEDIATYPE");
	case VFW_E_INVALIDSUBTYPE:
		return QStringLiteral("VFW_E_INVALIDSUBTYPE");
	case VFW_E_NEED_OWNER:
		return QStringLiteral("VFW_E_NEED_OWNER");
	case VFW_E_ENUM_OUT_OF_SYNC:
		return QStringLiteral("VFW_E_ENUM_OUT_OF_SYNC");
	case VFW_E_ALREADY_CONNECTED:
		return QStringLiteral("VFW_E_ALREADY_CONNECTED");
	case VFW_E_FILTER_ACTIVE:
		return QStringLiteral("VFW_E_FILTER_ACTIVE");
	case VFW_E_NO_TYPES:
		return QStringLiteral("VFW_E_NO_TYPES");
	case VFW_E_NO_ACCEPTABLE_TYPES:
		return QStringLiteral("VFW_E_NO_ACCEPTABLE_TYPES");
	case VFW_E_INVALID_DIRECTION:
		return QStringLiteral("VFW_E_INVALID_DIRECTION");
	case VFW_E_NOT_CONNECTED:
		return QStringLiteral("VFW_E_NOT_CONNECTED");
	case VFW_E_NO_ALLOCATOR:
		return QStringLiteral("VFW_E_NO_ALLOCATOR");
	case VFW_E_RUNTIME_ERROR:
		return QStringLiteral("VFW_E_RUNTIME_ERROR");
	case VFW_E_BUFFER_NOTSET:
		return QStringLiteral("VFW_E_BUFFER_NOTSET");
	case VFW_E_BUFFER_OVERFLOW:
		return QStringLiteral("VFW_E_BUFFER_OVERFLOW");
	case VFW_E_BADALIGN:
		return QStringLiteral("VFW_E_BADALIGN");
	case VFW_E_ALREADY_COMMITTED:
		return QStringLiteral("VFW_E_ALREADY_COMMITTED");
	case VFW_E_BUFFERS_OUTSTANDING:
		return QStringLiteral("VFW_E_BUFFERS_OUTSTANDING");
	case VFW_E_NOT_COMMITTED:
		return QStringLiteral("VFW_E_NOT_COMMITTED");
	case VFW_E_SIZENOTSET:
		return QStringLiteral("VFW_E_SIZENOTSET");
	case VFW_E_NO_CLOCK:
		return QStringLiteral("VFW_E_NO_CLOCK");
	case VFW_E_NO_SINK:
		return QStringLiteral("VFW_E_NO_SINK");
	case VFW_E_NO_INTERFACE:
		return QStringLiteral("VFW_E_NO_INTERFACE");
	case VFW_E_NOT_FOUND:
		return QStringLiteral("VFW_E_NOT_FOUND");
	case VFW_E_CANNOT_CONNECT:
		return QStringLiteral("VFW_E_CANNOT_CONNECT");
	case VFW_E_CANNOT_RENDER:
		return QStringLiteral("VFW_E_CANNOT_RENDER");
	case VFW_E_CHANGING_FORMAT:
		return QStringLiteral("VFW_E_CHANGING_FORMAT");
	case VFW_E_NO_COLOR_KEY_SET:
		return QStringLiteral("VFW_E_NO_COLOR_KEY_SET");
	case VFW_E_NOT_OVERLAY_CONNECTION:
		return QStringLiteral("VFW_E_NOT_OVERLAY_CONNECTION");
	case VFW_E_NOT_SAMPLE_CONNECTION:
		return QStringLiteral("VFW_E_NOT_SAMPLE_CONNECTION");
	case VFW_E_PALETTE_SET:
		return QStringLiteral("VFW_E_PALETTE_SET");
	case VFW_E_COLOR_KEY_SET:
		return QStringLiteral("VFW_E_COLOR_KEY_SET");
	case VFW_E_NO_COLOR_KEY_FOUND:
		return QStringLiteral("VFW_E_NO_COLOR_KEY_FOUND");
	case VFW_E_NO_PALETTE_AVAILABLE:
		return QStringLiteral("VFW_E_NO_PALETTE_AVAILABLE");
	case VFW_E_NO_DISPLAY_PALETTE:
		return QStringLiteral("VFW_E_NO_DISPLAY_PALETTE");
	case VFW_E_TOO_MANY_COLORS:
		return QStringLiteral("VFW_E_TOO_MANY_COLORS");
	case VFW_E_STATE_CHANGED:
		return QStringLiteral("VFW_E_STATE_CHANGED");
	case VFW_E_NOT_STOPPED:
		return QStringLiteral("VFW_E_NOT_STOPPED");
	case VFW_E_NOT_PAUSED:
		return QStringLiteral("VFW_E_NOT_PAUSED");
	case VFW_E_NOT_RUNNING:
		return QStringLiteral("VFW_E_NOT_RUNNING");
	case VFW_E_WRONG_STATE:
		return QStringLiteral("VFW_E_WRONG_STATE");
	case VFW_E_START_TIME_AFTER_END:
		return QStringLiteral("VFW_E_START_TIME_AFTER_END");
	case VFW_E_INVALID_RECT:
		return QStringLiteral("VFW_E_INVALID_RECT");
	case VFW_E_TYPE_NOT_ACCEPTED:
		return QStringLiteral("VFW_E_TYPE_NOT_ACCEPTED");
	case VFW_E_SAMPLE_REJECTED:
		return QStringLiteral("VFW_E_SAMPLE_REJECTED");
	case VFW_E_SAMPLE_REJECTED_EOS:
		return QStringLiteral("VFW_E_SAMPLE_REJECTED_EOS");
	case VFW_E_DUPLICATE_NAME:
		return QStringLiteral("VFW_E_DUPLICATE_NAME");
	case VFW_E_TIMEOUT:
		return QStringLiteral("VFW_E_TIMEOUT");
	case VFW_E_INVALID_FILE_FORMAT:
		return QStringLiteral("VFW_E_INVALID_FILE_FORMAT");
	case VFW_E_ENUM_OUT_OF_RANGE:
		return QStringLiteral("VFW_E_ENUM_OUT_OF_RANGE");
	case VFW_E_CIRCULAR_GRAPH:
		return QStringLiteral("VFW_E_CIRCULAR_GRAPH");
	case VFW_E_NOT_ALLOWED_TO_SAVE:
		return QStringLiteral("VFW_E_NOT_ALLOWED_TO_SAVE");
	case VFW_E_TIME_ALREADY_PASSED:
		return QStringLiteral("VFW_E_TIME_ALREADY_PASSED");
	case VFW_E_ALREADY_CANCELLED:
		return QStringLiteral("VFW_E_ALREADY_CANCELLED");
	case VFW_E_CORRUPT_GRAPH_FILE:
		return QStringLiteral("VFW_E_CORRUPT_GRAPH_FILE");
	case VFW_E_ADVISE_ALREADY_SET:
		return QStringLiteral("VFW_E_ADVISE_ALREADY_SET");
	case VFW_E_NO_MODEX_AVAILABLE:
		return QStringLiteral("VFW_E_NO_MODEX_AVAILABLE");
	case VFW_E_NO_ADVISE_SET:
		return QStringLiteral("VFW_E_NO_ADVISE_SET");
	case VFW_E_NO_FULLSCREEN:
		return QStringLiteral("VFW_E_NO_FULLSCREEN");
	case VFW_E_IN_FULLSCREEN_MODE:
		return QStringLiteral("VFW_E_IN_FULLSCREEN_MODE");
	case VFW_E_UNKNOWN_FILE_TYPE:
		return QStringLiteral("VFW_E_UNKNOWN_FILE_TYPE");
	case VFW_E_CANNOT_LOAD_SOURCE_FILTER:
		return QStringLiteral("VFW_E_CANNOT_LOAD_SOURCE_FILTER");
	case VFW_E_FILE_TOO_SHORT:
		return QStringLiteral("VFW_E_FILE_TOO_SHORT");
	case VFW_E_INVALID_FILE_VERSION:
		return QStringLiteral("VFW_E_INVALID_FILE_VERSION");
	case VFW_E_INVALID_CLSID:
		return QStringLiteral("VFW_E_INVALID_CLSID");
	case VFW_E_INVALID_MEDIA_TYPE:
		return QStringLiteral("VFW_E_INVALID_MEDIA_TYPE");
	case VFW_E_SAMPLE_TIME_NOT_SET:
		return QStringLiteral("VFW_E_SAMPLE_TIME_NOT_SET");
	case VFW_E_MEDIA_TIME_NOT_SET:
		return QStringLiteral("VFW_E_MEDIA_TIME_NOT_SET");
	case VFW_E_NO_TIME_FORMAT_SET:
		return QStringLiteral("VFW_E_NO_TIME_FORMAT_SET");
	case VFW_E_MONO_AUDIO_HW:
		return QStringLiteral("VFW_E_MONO_AUDIO_HW");
	case VFW_E_NO_DECOMPRESSOR:
		return QStringLiteral("VFW_E_NO_DECOMPRESSOR");
	case VFW_E_NO_AUDIO_HARDWARE:
		return QStringLiteral("VFW_E_NO_AUDIO_HARDWARE");
	case VFW_E_RPZA:
		return QStringLiteral("VFW_E_RPZA");
	case VFW_E_PROCESSOR_NOT_SUITABLE:
		return QStringLiteral("VFW_E_PROCESSOR_NOT_SUITABLE");
	case VFW_E_UNSUPPORTED_AUDIO:
		return QStringLiteral("VFW_E_UNSUPPORTED_AUDIO");
	case VFW_E_UNSUPPORTED_VIDEO:
		return QStringLiteral("VFW_E_UNSUPPORTED_VIDEO");
	case VFW_E_MPEG_NOT_CONSTRAINED:
		return QStringLiteral("VFW_E_MPEG_NOT_CONSTRAINED");
	case VFW_E_NOT_IN_GRAPH:
		return QStringLiteral("VFW_E_NOT_IN_GRAPH");
	case VFW_E_NO_TIME_FORMAT:
		return QStringLiteral("VFW_E_NO_TIME_FORMAT");
	case VFW_E_READ_ONLY:
		return QStringLiteral("VFW_E_READ_ONLY");
	case VFW_E_BUFFER_UNDERFLOW:
		return QStringLiteral("VFW_E_BUFFER_UNDERFLOW");
	case VFW_E_UNSUPPORTED_STREAM:
		return QStringLiteral("VFW_E_UNSUPPORTED_STREAM");
	case VFW_E_NO_TRANSPORT:
		return QStringLiteral("VFW_E_NO_TRANSPORT");
	case VFW_E_BAD_VIDEOCD:
		return QStringLiteral("VFW_E_BAD_VIDEOCD");
	case VFW_S_NO_STOP_TIME:
		return QStringLiteral("VFW_S_NO_STOP_TIME");
	case VFW_E_OUT_OF_VIDEO_MEMORY:
		return QStringLiteral("VFW_E_OUT_OF_VIDEO_MEMORY");
	case VFW_E_VP_NEGOTIATION_FAILED:
		return QStringLiteral("VFW_E_VP_NEGOTIATION_FAILED");
	case VFW_E_DDRAW_CAPS_NOT_SUITABLE:
		return QStringLiteral("VFW_E_DDRAW_CAPS_NOT_SUITABLE");
	case VFW_E_NO_VP_HARDWARE:
		return QStringLiteral("VFW_E_NO_VP_HARDWARE");
	case VFW_E_NO_CAPTURE_HARDWARE:
		return QStringLiteral("VFW_E_NO_CAPTURE_HARDWARE");
	case VFW_E_DVD_OPERATION_INHIBITED:
		return QStringLiteral("VFW_E_DVD_OPERATION_INHIBITED");
	case VFW_E_DVD_INVALIDDOMAIN:
		return QStringLiteral("VFW_E_DVD_INVALIDDOMAIN");
	case VFW_E_DVD_NO_BUTTON:
		return QStringLiteral("VFW_E_DVD_NO_BUTTON");
	case VFW_E_DVD_GRAPHNOTREADY:
		return QStringLiteral("VFW_E_DVD_GRAPHNOTREADY");
	case VFW_E_DVD_RENDERFAIL:
		return QStringLiteral("VFW_E_DVD_RENDERFAIL");
	case VFW_E_DVD_DECNOTENOUGH:
		return QStringLiteral("VFW_E_DVD_DECNOTENOUGH");
	case VFW_E_DDRAW_VERSION_NOT_SUITABLE:
		return QStringLiteral("VFW_E_DDRAW_VERSION_NOT_SUITABLE");
	case VFW_E_COPYPROT_FAILED:
		return QStringLiteral("VFW_E_COPYPROT_FAILED");
	case VFW_E_TIME_EXPIRED:
		return QStringLiteral("VFW_E_TIME_EXPIRED");
	case VFW_E_DVD_WRONG_SPEED:
		return QStringLiteral("VFW_E_DVD_WRONG_SPEED");
	case VFW_E_DVD_MENU_DOES_NOT_EXIST:
		return QStringLiteral("VFW_E_DVD_MENU_DOES_NOT_EXIST");
	case VFW_E_DVD_CMD_CANCELLED:
		return QStringLiteral("VFW_E_DVD_CMD_CANCELLED");
	case VFW_E_DVD_STATE_WRONG_VERSION:
		return QStringLiteral("VFW_E_DVD_STATE_WRONG_VERSION");
	case VFW_E_DVD_STATE_CORRUPT:
		return QStringLiteral("VFW_E_DVD_STATE_CORRUPT");
	case VFW_E_DVD_STATE_WRONG_DISC:
		return QStringLiteral("VFW_E_DVD_STATE_WRONG_DISC");
	case VFW_E_DVD_INCOMPATIBLE_REGION:
		return QStringLiteral("VFW_E_DVD_INCOMPATIBLE_REGION");
	case VFW_E_DVD_NO_ATTRIBUTES:
		return QStringLiteral("VFW_E_DVD_NO_ATTRIBUTES");
	case VFW_E_DVD_NO_GOUP_PGC:
		return QStringLiteral("VFW_E_DVD_NO_GOUP_PGC");
	case VFW_E_DVD_LOW_PARENTAL_LEVEL:
		return QStringLiteral("VFW_E_DVD_LOW_PARENTAL_LEVEL");
	case VFW_E_DVD_NOT_IN_KARAOKE_MODE:
		return QStringLiteral("VFW_E_DVD_NOT_IN_KARAOKE_MODE");
	case VFW_E_FRAME_STEP_UNSUPPORTED:
		return QStringLiteral("VFW_E_FRAME_STEP_UNSUPPORTED");
	case VFW_E_DVD_STREAM_DISABLED:
		return QStringLiteral("VFW_E_DVD_STREAM_DISABLED");
	case VFW_E_DVD_TITLE_UNKNOWN:
		return QStringLiteral("VFW_E_DVD_TITLE_UNKNOWN");
	case VFW_E_DVD_INVALID_DISC:
		return QStringLiteral("VFW_E_DVD_INVALID_DISC");
	case VFW_E_DVD_NO_RESUME_INFORMATION:
		return QStringLiteral("VFW_E_DVD_NO_RESUME_INFORMATION");
	case VFW_E_PIN_ALREADY_BLOCKED_ON_THIS_THREAD:
		return QStringLiteral("VFW_E_PIN_ALREADY_BLOCKED_ON_THIS_THREAD");
	case VFW_E_PIN_ALREADY_BLOCKED:
		return QStringLiteral("VFW_E_PIN_ALREADY_BLOCKED");
	case VFW_E_CERTIFICATION_FAILURE:
		return QStringLiteral("VFW_E_CERTIFICATION_FAILURE");
	case VFW_E_VMR_NOT_IN_MIXER_MODE:
		return QStringLiteral("VFW_E_VMR_NOT_IN_MIXER_MODE");
	case VFW_E_VMR_NO_AP_SUPPLIED:
		return QStringLiteral("VFW_E_VMR_NO_AP_SUPPLIED");
	case VFW_E_VMR_NO_DEINTERLACE_HW:
		return QStringLiteral("VFW_E_VMR_NO_DEINTERLACE_HW");
	case VFW_E_VMR_NO_PROCAMP_HW:
		return QStringLiteral("VFW_E_VMR_NO_PROCAMP_HW");
	case VFW_E_DVD_VMR9_INCOMPATIBLEDEC:
		return QStringLiteral("VFW_E_DVD_VMR9_INCOMPATIBLEDEC");
	case VFW_E_NO_COPP_HW:
		return QStringLiteral("VFW_E_NO_COPP_HW");
	case VFW_E_BAD_KEY:
		return QStringLiteral("VFW_E_BAD_KEY");
	case VFW_E_DVD_NONBLOCKING:
		return QStringLiteral("VFW_E_DVD_NONBLOCKING");
	case VFW_E_DVD_TOO_MANY_RENDERERS_IN_FILTER_GRAPH:
		return QStringLiteral("VFW_E_DVD_TOO_MANY_RENDERERS_IN_FILTER_GRAPH");
	case VFW_E_DVD_NON_EVR_RENDERER_IN_FILTER_GRAPH:
		return QStringLiteral("VFW_E_DVD_NON_EVR_RENDERER_IN_FILTER_GRAPH");
	case VFW_E_DVD_RESOLUTION_ERROR:
		return QStringLiteral("VFW_E_DVD_RESOLUTION_ERROR");
	case VFW_E_CODECAPI_LINEAR_RANGE:
		return QStringLiteral("VFW_E_CODECAPI_LINEAR_RANGE");
	case VFW_E_CODECAPI_ENUMERATED:
		return QStringLiteral("VFW_E_CODECAPI_ENUMERATED");
	case VFW_E_CODECAPI_NO_DEFAULT:
		return QStringLiteral("VFW_E_CODECAPI_NO_DEFAULT");
	case VFW_E_CODECAPI_NO_CURRENT_VALUE:
		return QStringLiteral("VFW_E_CODECAPI_NO_CURRENT_VALUE");
	case E_PROP_ID_UNSUPPORTED:
		return QStringLiteral("E_PROP_ID_UNSUPPORTED");
	case E_PROP_SET_UNSUPPORTED:
		return QStringLiteral("E_PROP_SET_UNSUPPORTED");
	case E_FAIL:
		return QStringLiteral("E_FAIL");
	case E_INVALIDARG:
		return QStringLiteral("E_INVALIDARG");
	case E_OUTOFMEMORY:
		return QStringLiteral("E_OUTOFMEMORY");
	case S_FALSE:
		return QStringLiteral("S_FALSE");
	case S_OK:
		return QStringLiteral("S_OK");
	default:
		return numberToHexString((uint)res);
	}
}

/// <summary>
/// Copies a single frame's plane to the specified texture.
/// </summary>
/// <returns>The first byte after the plane</returns>
uchar *copyPlaneToTex(VidgfxTex *tex, uchar *src, const QSize &planeSizeBytes)
{
	uchar *texData = reinterpret_cast<uchar *>(vidgfx_tex_map(tex));
	imgDataCopy(texData, src, vidgfx_tex_get_stride(tex),
		planeSizeBytes.width(), planeSizeBytes);
	vidgfx_tex_unmap(tex);
	src += planeSizeBytes.width() * planeSizeBytes.height();
	return src;
}

//=============================================================================
// DShowRenderer class

static const GUID CLSID_DShowRenderer = {
	0xd285321f, 0xa86f, 0x46a0,
	{0x89, 0x3b, 0xf1, 0xe5, 0x62, 0x0b, 0xe0, 0x64}
};

/// <summary>
/// Returns the matching `VidgfxPixFormat` for the specified video subtype GUID
/// if it is supported by our format conversion system.
/// </summary>
VidgfxPixFormat DShowRenderer::formatFromSubtype(const GUID &subtype)
{
	// Uncompressed RGB with a single packed plane
	if(subtype == MEDIASUBTYPE_RGB24)
		return GfxRGB24Format;
	if(subtype == MEDIASUBTYPE_RGB32) // WARNING: Untested
		return GfxRGB32Format;
	if(subtype == MEDIASUBTYPE_ARGB32) // WARNING: Untested
		return GfxARGB32Format;

	// YUV 4:2:0 formats with 3 separate planes
	if(subtype == MEDIASUBTYPE_YV12)
		return GfxYV12Format;
	if(subtype == MEDIASUBTYPE_IYUV || subtype == MEDIASUBTYPE_I420)
		return GfxIYUVFormat;

	// YUV 4:2:0 formats with 2 separate planes
	// None

	// YUV 4:2:2 formats with a single packed plane
	if(subtype == MEDIASUBTYPE_UYVY) // WARNING: Untested
		return GfxUYVYFormat;
	if(subtype == MEDIASUBTYPE_HDYC) // WARNING: Untested
		return GfxHDYCFormat;
	if(subtype == MEDIASUBTYPE_YUY2)
		return GfxYUY2Format;

	// Not supported
	return GfxNoFormat;
}

/// <summary>
/// Compares the two pixel formats and indicate which one is preferred over the
/// other.
/// </summary>
/// <returns>
/// -1 is `a` is better, 0 if they are the same, 1 if `b` is better
/// </returns>
int DShowRenderer::compareFormats(VidgfxPixFormat a, VidgfxPixFormat b)
{
	// Give each pixel format an order of preference. Higher numbers means a
	// higher preference. Negative preference items are effectively never used.
	// The array has the same order as the `VidgfxPixFormat` enum.
	int order[NUM_PIXEL_FORMAT_TYPES];
	int i = 0;
	order[i++] = 0; // GfxNoFormat

	order[i++] = 20; // GfxRGB24Format. Prefer YUV as this is usually CPU-based
	order[i++] = 100; // GfxRGB32Format
	order[i++] = 100; // GfxARGB32Format

	order[i++] = 50; // GfxYV12Format
	order[i++] = 50; // GfxIYUVFormat
	order[i++] = -1; // GfxNV12Format

	order[i++] = 50; // GfxUYVYFormat
	order[i++] = 50; // GfxHDYCFormat
	order[i++] = 50; // GfxYUY2Format
	Q_ASSERT(i == NUM_PIXEL_FORMAT_TYPES);

	if(order[b] > order[a])
		return 1;
	else if(order[b] < order[a])
		return -1;
	return 0;
}

void DShowRenderer::parseMediaType(
	const AM_MEDIA_TYPE &type, QSize *size, float *framerate, bool *negHeight)
{
	if(size != NULL)
		*size = QSize(0, 0);
	if(framerate != NULL)
		*framerate = 0.0f;
	if(negHeight != NULL)
		*negHeight = false;

	// Get the header information that we require depending on the format
	REFERENCE_TIME avgTimePerFrame;
	BITMAPINFOHEADER *bmiHeader = NULL;
	if(type.formattype == FORMAT_VideoInfo ||
		type.formattype == FORMAT_MPEGVideo)
	{
		VIDEOINFOHEADER *hdr = NULL;
		if(type.formattype == FORMAT_VideoInfo)
			hdr = reinterpret_cast<VIDEOINFOHEADER *>(type.pbFormat);
		else {
			hdr =
				&(reinterpret_cast<MPEG1VIDEOINFO *>(type.pbFormat)->hdr);
		}
		avgTimePerFrame = hdr->AvgTimePerFrame;
		bmiHeader = &hdr->bmiHeader;
	} else if(type.formattype == FORMAT_VideoInfo2 ||
		type.formattype == FORMAT_MPEG2Video)
	{
		VIDEOINFOHEADER2 *hdr = NULL;
		if(type.formattype == FORMAT_VideoInfo)
			hdr = reinterpret_cast<VIDEOINFOHEADER2 *>(type.pbFormat);
		else {
			hdr =
				&(reinterpret_cast<MPEG2VIDEOINFO *>(type.pbFormat)->hdr);
		}
		avgTimePerFrame = hdr->AvgTimePerFrame;
		bmiHeader = &hdr->bmiHeader;
	} else
		return; // We can't determine texture size

	// Prepare return values
	Fraction64 fps;
	if(framerate != NULL) {
		fps.numerator = 10000000ULL; // 100ns units
		fps.denominator = avgTimePerFrame;
		*framerate = fps.asFloat();
	}
	if(size != NULL) {
		size->setWidth(bmiHeader->biWidth);
		int height = bmiHeader->biHeight;
		if(height < 0) {
			if(negHeight != NULL)
				*negHeight = true;
			height = -height;
		}
		size->setHeight(height);
	}
}

DShowRenderer::DShowRenderer(VidgfxPixFormat pixelFormat, HRESULT *phr)
	: CBaseRenderer(CLSID_DShowRenderer, TEXT("DShowRenderer"), NULL, phr)
	, m_mutex()
	, m_pixelFormat(pixelFormat)
	, m_curSample(NULL)
	, m_curSampleNum(0)
{
}

DShowRenderer::~DShowRenderer()
{
	m_mutex.lock();
	if(m_curSample != NULL) {
		m_curSample->Release();
		m_curSample = NULL;
	}
	m_mutex.unlock();
}

/// <summary>
/// Locks the current sample so that it can be processed by the application.
/// `sampleNumOut` can be used to identify if the sample has changed since the
/// last time it was accessed so that the same sample doesn't need to be
/// processed multiple times.
/// </summary>
IMediaSample *DShowRenderer::lockCurSample(uint *sampleNumOut)
{
	if(m_curSample == NULL)
		return NULL;
	m_mutex.lock();
	if(sampleNumOut != NULL)
		*sampleNumOut = m_curSampleNum;
	return m_curSample;
}

void DShowRenderer::unlockCurSample()
{
	if(m_curSample == NULL)
		return;
	m_mutex.unlock();
}

HRESULT DShowRenderer::CheckMediaType(const CMediaType *pmt)
{
	// We only accept video samples
	if(pmt == NULL)
		return S_FALSE;
	if(*(pmt->Type()) != MEDIATYPE_Video)
		return S_FALSE;

	// If we have an expected pixel format then force that one as we chose it
	// for a reason, otherwise only accept pixel formats that we can process.
	if(m_pixelFormat != GfxNoFormat) {
		if(formatFromSubtype(*(pmt->Subtype())) != m_pixelFormat)
			return S_FALSE;
	} else {
		if(formatFromSubtype(*(pmt->Subtype())) == GfxNoFormat)
			return S_FALSE;
	}

	// This is a supported media type!
	return S_OK;
}

HRESULT DShowRenderer::DoRenderSample(IMediaSample *pMediaSample)
{
	m_mutex.lock();
	if(m_curSample != NULL)
		m_curSample->Release();
	m_curSample = pMediaSample;
	m_curSample->AddRef(); // Keep the sample around after returning
	m_curSampleNum++;
	m_mutex.unlock();

	return S_OK; // Return code is unused by the base class
}

void DShowRenderer::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
	// TODO: If the graph is paused we should use this sample to display a
	// still frame to the user. Without this the user will be presented with a
	// blank or incorrect frame whenever they seek.
}

HRESULT DShowRenderer::ShouldDrawSampleNow(
	IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime,
	REFERENCE_TIME *pEndTime)
{
	// TODO: We don't actually implement buffering yet so rely on the base
	// class to handle scheduling
	return S_FALSE;

	// By always returning `S_OK` we are requesting that the base class forward
	// all samples that it receives immediately to us so we can buffer it
	// ourselves. The default implementation of this method is to always return
	// `S_FALSE` which forces the streaming thread to block until the sample is
	// ready to be rendered.
	//return S_OK;
}

//=============================================================================
// DShowVideoSource class

QVector<quint64> DShowVideoSource::s_activeIds;

/// <summary>
/// Creates all DirectShow-based video sources and registers them with the
/// manager.
/// </summary>
void DShowVideoSource::populateSources(
	VideoSourceManager *mgr, bool isRepopulate)
{
	// Create the enumerator for all video capture devices
	ICreateDevEnum *devEnum = NULL;
	HRESULT res = CoCreateInstance(
		CLSID_SystemDeviceEnum, NULL, CLSCTX_ALL, IID_PPV_ARGS(&devEnum));
	if(FAILED(res) || devEnum == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to create ICreateDevEnum. "
			<< "Reason = " << getDShowErrorCode(res);
		return;
	}
	IEnumMoniker *monikerEnum = NULL;
	res = devEnum->CreateClassEnumerator(
		CLSID_VideoInputDeviceCategory, &monikerEnum, 0);
	if(FAILED(res) || monikerEnum == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to create IEnumMoniker. "
			<< "Reason = " << getDShowErrorCode(res);
		return;
	}

	// Iterate over each device and create a source object from it
	QVector<quint64> foundIds;
	IMoniker *moniker = NULL;
	while(monikerEnum->Next(1, &moniker, NULL) == S_OK) {
#if DEBUG_DIRECTSHOW
		appLog(LOG_CAT) << "---- Next device ----";
#endif // DEBUG_DIRECTSHOW

		// Get the unique moniker name that we use just in case we fail to get
		// the device path from the property bag
		QString fullIdStr;
		IBindCtx *context = NULL;
		res = CreateBindCtx(0, &context);
		if(SUCCEEDED(res)) {
			LPOLESTR str = NULL;
			res = moniker->GetDisplayName(context, NULL, &str);
			if(SUCCEEDED(res) && str != NULL)
				fullIdStr = QString::fromUtf16(str);
			context->Release();
		}
#if DEBUG_DIRECTSHOW
		appLog(LOG_CAT) << fullIdStr;
#endif // DEBUG_DIRECTSHOW

		// Open the property bag for this device
		IPropertyBag *propBag = NULL;
		res = moniker->BindToStorage(NULL, NULL, IID_IPropertyBag,
			(void**)(&propBag));
		if(FAILED(res) || propBag == NULL) {
			// Should never happen
#if DEBUG_DIRECTSHOW
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to get property bag from moniker. "
				<< "Reason = " << getDShowErrorCode(res);
#endif // DEBUG_DIRECTSHOW
			moniker->Release();
			continue;
		}
		VARIANT propStr;
		VariantInit(&propStr);

#define USE_DEVICE_PATH_FOR_ID 0
#if USE_DEVICE_PATH_FOR_ID
		// Get the device's unique ID. This fails with the FFSplit virtual
		// camera and AmaRecTV. In order for us to allow these we instead use
		// the full moniker ID if we succeeded in fetching it.
		QString idStr = fullIdStr;
		res = propBag->Read(L"DevicePath", &propStr, NULL);
		if(SUCCEEDED(res))
			idStr = QString::fromUtf16(propStr.bstrVal);
		else if(fullIdStr.isEmpty()) {
#if DEBUG_DIRECTSHOW
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to read device path from property bag. "
				<< "Reason = " << getDShowErrorCode(res);
#endif // DEBUG_DIRECTSHOW
			moniker->Release();
			continue;
		}
		quint64 id = hashQString64(idStr);
		VariantClear(&propStr);
		foundIds.append(id);
#else
		// Always use the full moniker ID string as our unique identifier
		QString idStr = fullIdStr;
		quint64 id = hashQString64(idStr);
		foundIds.append(id);
#endif // USE_DEVICE_PATH_FOR_ID

		// Is the current device already in our manager?
		if(mgr->getSource(id) != NULL) {
			moniker->Release();
			continue;
		}

		// Blacklist certain filters as they are known to cause issues
		if(idStr.contains(QStringLiteral("VHSplit"))) { // XSplit virtual cam
#if DEBUG_DIRECTSHOW
			appLog(LOG_CAT, Log::Warning)
				<< "Blacklisted filter, skipping";
#endif // DEBUG_DIRECTSHOW
			moniker->Release();
			continue;
		}

		// Get the actual filter interface
		IBaseFilter *filter = NULL;
		res = moniker->BindToObject(
			NULL, NULL, IID_IBaseFilter, (void**)&filter);
		if(FAILED(res) || filter == NULL) {
			// Occurs with the XSplit virtual camera as it uses a conflicting
			// version of FFmpeg (Missing symbols from "avcodec-54.dll")
#if DEBUG_DIRECTSHOW
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to get IBaseFilter from moniker. "
				<< "Reason = " << getDShowErrorCode(res);
#endif // DEBUG_DIRECTSHOW
			moniker->Release();
			continue;
		}

		// Get the device's friendly name. We prefer "Description" as it's
		// apparently more accurate according to MSDN.
		QString friendlyName = tr("** Unknown **");
		res = propBag->Read(L"Description", &propStr, NULL);
		if(FAILED(res))
			res = propBag->Read(L"FriendlyName", &propStr, NULL);
		if(SUCCEEDED(res)) {
			friendlyName = QString::fromUtf16(propStr.bstrVal);
			VariantClear(&propStr);
		}

		// Construct the source object and register it with the manager
		DShowVideoSource *source = new DShowVideoSource(
			filter, id, friendlyName);
		mgr->addSource(source);

		// Log when a new device is connected
		if(isRepopulate) {
			appLog(LOG_CAT) << QStringLiteral(
				"Adding new video source: %1").arg(source->getDebugString());
		}

		// Clean up
		propBag->Release();
		moniker->Release();
	}

	// Clean up
	monikerEnum->Release();
	devEnum->Release();

	// Remove any sources that are no longer available
	for(int i = 0; i < s_activeIds.size(); i++) {
		quint64 id = s_activeIds.at(i);

		int found = false;
		for(int j = 0; j < foundIds.size(); j++) {
			if(foundIds.at(j) == id) {
				found = true;
				break;
			}
		}
		if(found)
			continue; // Still exists

		// Device has been removed, delete it
		VideoSource *src = mgr->getSource(id);
		if(src == NULL)
			continue; // Should never happen

		// Log when a device is disconnected
		if(isRepopulate) {
			appLog(LOG_CAT) << QStringLiteral(
				"Removing video source: %1").arg(src->getDebugString());
		}

		mgr->removeSource(src);
		delete src;
	}
}

void DShowVideoSource::repopulateSources(VideoSourceManager *mgr)
{
	populateSources(mgr, true);
}

DShowVideoSource::DShowVideoSource(
	IBaseFilter *filter, quint64 id, QString friendlyName)
	: VideoSource()
	, m_id(id)
	, m_friendlyName(friendlyName)
	, m_ref(0)
	, m_filter(filter)
	, m_renderFilter(NULL)
	, m_bestOutPin(NULL)
	, m_bestOutPinFormat(GfxNoFormat)
	, m_availModes()
	, m_control(NULL)

	, m_targetFramerate(0.0f)
	, m_targetSize(0, 0)

	, m_graph(NULL)
	, m_graphRotId(0)

	, m_isFlipped(false)
	, m_curSample(NULL)
	, m_curSampleNum(0)
	, m_curSampleProcessed(false)
	, m_curFormat(GfxNoFormat)
	, m_curPlaneA(NULL)
	, m_curPlaneB(NULL)
	, m_curPlaneC(NULL)
	, m_curTexture(NULL)
{
	// Track DirectShow sources
	s_activeIds.append(m_id);

	findBestOutputPin();
	fetchAllOutputModes();
}

DShowVideoSource::~DShowVideoSource()
{
	if(m_ref != 0) {
		appLog(LOG_CAT, Log::Warning) <<
			QStringLiteral("Destroying referenced video source \"%1\" (%L2 references)")
			.arg(getDebugString())
			.arg(m_ref);
		shutdown(); // Make sure everything is released
	}

	if(m_bestOutPin != NULL)
		m_bestOutPin->Release();
	m_bestOutPin = NULL;
	m_bestOutPinFormat = GfxNoFormat;

	clearModeList();

	m_filter->Release();

	// Untrack DirectShow source
	int index = s_activeIds.indexOf(m_id);
	if(index >= 0)
		s_activeIds.remove(index);
}

bool DShowVideoSource::initialize()
{
	if(m_bestOutPin == NULL) {
		// Reattempt to find the best pin
		findBestOutputPin();
		fetchAllOutputModes();

		if(m_bestOutPin == NULL) {
			appLog(LOG_CAT, Log::Warning)
				<< "Could not find a pin to connect to";
			return false;
		}
	}

	// Create filter graph with graph builder
	m_graph = NULL;
	HRESULT res = CoCreateInstance(CLSID_FilterGraph,
		NULL, CLSCTX_ALL, IID_PPV_ARGS(&m_graph));
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to create IGraphBuilder. "
			<< "Reason = " << getDShowErrorCode(res);
		return false;
	}

#ifdef QT_DEBUG
	// Register graph in the running object table so that we can open it in
	// GraphEdit
	res = AddToRot(m_graph, &m_graphRotId);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to register graph in the running object table. "
			<< "Reason = " << getDShowErrorCode(res);
	}
#endif // QT_DEBUG

	// Create renderer filter. We pass the best pixel format to the renderer so
	// that it only accepts our preferred format instead of whatever the graph
	// builder decides to use during the connection process.
	m_renderFilter = new DShowRenderer(m_bestOutPinFormat, &res);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to create DShowRenderer. "
			<< "Reason = " << getDShowErrorCode(res);
		goto exitInitialize1;
	}
	m_renderFilter->AddRef(); // Only we can delete the filter

	// Add filters to graph
	wchar_t *name = QStringToWChar(getFriendlyName());
	res = m_graph->AddFilter(m_filter, name);
	delete[] name;
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to add source filter to graph. "
			<< "Reason = " << getDShowErrorCode(res);
		goto exitInitialize1;
	}
	res = m_graph->AddFilter(m_renderFilter, TEXT("Renderer"));
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to add render filter to graph. "
			<< "Reason = " << getDShowErrorCode(res);
		goto exitInitialize2;
	}

	//-------------------------------------------------------------------------
	// Configure the source filter output size and framerate

	m_bestOutPin->QueryInterface(IID_PPV_ARGS(&m_config));
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get IAMStreamConfig. "
			<< "Reason = " << getDShowErrorCode(res);
		goto exitInitialize3;
	}

	if(m_targetFramerate == 0.0f && m_targetSize.isEmpty())
		setTargetModeToDefault();
	configureBestPinToTargetMode();

	//-------------------------------------------------------------------------

	// Connect the filters together
	res = m_graph->Connect(m_bestOutPin, m_renderFilter->GetPin(0));
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to connect source and render filters. "
			<< "Reason = " << getDShowErrorCode(res);
		goto exitInitialize4;
	}

	// Get the graph media controller
	res = m_graph->QueryInterface(IID_PPV_ARGS(&m_control));
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get IMediaControl. "
			<< "Reason = " << getDShowErrorCode(res);
		goto exitInitialize4;
	}

	// Start all filters in the graph
	res = m_control->Run();
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to start entire graph. "
			<< "Reason = " << getDShowErrorCode(res);
		goto exitInitialize5;
	}

	// Reset current frame data
	m_isFlipped = false;
	m_curSampleNum = 0;
	m_curSampleProcessed = false;

	return true;

	// Error handling
exitInitialize5:
	if(m_control != NULL)
		m_control->Release();
	m_control = NULL;
exitInitialize4:
	if(m_config != NULL)
		m_config->Release();
	m_config = NULL;
exitInitialize3:
	m_graph->RemoveFilter(m_renderFilter);
exitInitialize2:
	m_graph->RemoveFilter(m_filter);
exitInitialize1:
	if(m_renderFilter != NULL)
		m_renderFilter->Release();
	m_renderFilter = NULL;

	if(m_graph != NULL)
		m_graph->Release();
	m_graph = NULL;

	return false;
}

void DShowVideoSource::shutdown()
{
	// Reset current frame data if we have one
	VidgfxContext *gfx = App->getGraphicsContext();
	if(vidgfx_context_is_valid(gfx)) {
		if(m_curPlaneA != NULL)
			vidgfx_context_destroy_tex(gfx, m_curPlaneA);
		if(m_curPlaneB != NULL)
			vidgfx_context_destroy_tex(gfx, m_curPlaneB);
		if(m_curPlaneC != NULL)
			vidgfx_context_destroy_tex(gfx, m_curPlaneC);
		if(m_curTexture != NULL)
			vidgfx_context_destroy_tex(gfx, m_curTexture);
		m_curPlaneA = NULL;
		m_curPlaneB = NULL;
		m_curPlaneC = NULL;
		m_curTexture = NULL;
	}
	if(m_curSample != NULL) {
		m_curSample->Release();
		m_curSample = NULL;
	}
	m_curSampleNum = 0;
	m_curSampleProcessed = false;
	m_curFormat = GfxNoFormat;

	m_control->Stop();

	if(m_control != NULL)
		m_control->Release();
	m_control = NULL;

	if(m_config != NULL)
		m_config->Release();
	m_config = NULL;

	m_graph->RemoveFilter(m_renderFilter);
	m_graph->RemoveFilter(m_filter);

	if(m_renderFilter != NULL)
		m_renderFilter->Release();
	m_renderFilter = NULL;

#ifdef QT_DEBUG
	RemoveFromRot(m_graphRotId);
	m_graphRotId = 0;
#endif // QT_DEBUG

	if(m_graph != NULL)
		m_graph->Release();
	m_graph = NULL;

	// Recreate the filter just in case the output modes have changed so that
	// when a user reloads a webcam layer they get the new settings.
	recreateFilter();
}

/// <summary>
/// Some filters, such as AmaRecTV, change their available modes during
/// operation. In order to automatically reconnect to the filter with the new
/// modes we must completely recreate the filter from scratch.
/// </summary>
/// <returns>True if the filter was recreated</returns>
bool DShowVideoSource::recreateFilter()
{
	// Get the unique class ID (CLSID) of the filter
	CLSID clsid;
	HRESULT res = m_filter->GetClassID(&clsid);
	if(FAILED(res))
		return false;

	// Get the new filter that's identical to the original
	IBaseFilter *newFilter = NULL;
	res = CoCreateInstance(
		clsid, NULL, CLSCTX_ALL, IID_PPV_ARGS(&newFilter));
	if(FAILED(res))
		return false;

	// Replace the current filter with the new one
	if(m_bestOutPin != NULL)
		m_bestOutPin->Release();
	m_bestOutPin = NULL;
	m_bestOutPinFormat = GfxNoFormat;
	clearModeList();
	m_filter->Release();
	m_filter = newFilter;

	// Refind best pin and all output modes
	findBestOutputPin();
	fetchAllOutputModes();

	return true;
}

/// <summary>
/// Finds the best video output pin to use and the best output format.
/// </summary>
void DShowVideoSource::findBestOutputPin()
{
	if(m_bestOutPin != NULL) {
		m_bestOutPin->Release();
		m_bestOutPin = NULL;
	}
	m_bestOutPinFormat = GfxNoFormat;

	// Create pin enumeration object
	IEnumPins *pins = NULL;
	HRESULT res = m_filter->EnumPins(&pins);
	if(FAILED(res)) {
#if DEBUG_DIRECTSHOW
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to enumerate output pins. "
			<< "Reason = " << getDShowErrorCode(res);
#endif // DEBUG_DIRECTSHOW
		return;
	}

	// Enumerate over each pin to find the best one
	IPin *pin = NULL;
	while(pins->Next(1, &pin, NULL) == S_OK) {
		GUID pinCat = PIN_CATEGORY_CAPTURE;

		// We only want output pins
		PIN_DIRECTION pinDir;
		res = pin->QueryDirection(&pinDir);
		if(FAILED(res)) {
#if DEBUG_DIRECTSHOW
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to query pin direction. "
				<< "Reason = " << getDShowErrorCode(res);
#endif // DEBUG_DIRECTSHOW
			goto continueFindBestOutputPin;
		}
		if(pinDir != PINDIR_OUTPUT)
			goto continueFindBestOutputPin;

		// We only want "capture" pins and not anything else such as stills or
		// closed captioning. If we fail to get the property set object then
		// just assume it's a capture pin.
		IKsPropertySet *propSet = NULL;
		res = pin->QueryInterface(IID_PPV_ARGS(&propSet));
		if(SUCCEEDED(res)) {
			DWORD retSize = 0;
			res = propSet->Get(AMPROPSETID_Pin, AMPROPERTY_PIN_CATEGORY, NULL,
				0, &pinCat, sizeof(pinCat), &retSize);
			if(FAILED(res) || retSize != sizeof(pinCat))
				pinCat = PIN_CATEGORY_CAPTURE;
			propSet->Release();
		}
		if(pinCat != PIN_CATEGORY_CAPTURE &&
			pinCat != PIN_CATEGORY_CAPTURE_ALT)
		{
			goto continueFindBestOutputPin;
		}

		//---------------------------------------------------------------------
		// We only want pins that output video

		// Create media type enumeration object
		IEnumMediaTypes *types = NULL;
		HRESULT res = pin->EnumMediaTypes(&types);
		if(FAILED(res)) {
#if DEBUG_DIRECTSHOW
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to enumerate output pins. "
				<< "Reason = " << getDShowErrorCode(res);
#endif // DEBUG_DIRECTSHOW
			goto continueFindBestOutputPin;
		}

		// Enumerate over each media type to find the best one
		AM_MEDIA_TYPE *type = NULL;
		while(types->Next(1, &type, NULL) == S_OK) {
			if(type->majortype != MEDIATYPE_Video) {
				DeleteMediaType(type);
				continue;
			}

			// If we get this far then this pin has some sort of video output.
			// If we don't have a better pin already then use this as the
			// fallback as there might be intermediate filters that can convert
			// the output to something that we can use (E.g. H.264 decoders).
			if(m_bestOutPin == NULL) {
				pin->AddRef();
				m_bestOutPin = pin;
			}

			// Does this pin output one of our supported pixel formats?
			VidgfxPixFormat format =
				DShowRenderer::formatFromSubtype(type->subtype);
			if(format != GfxNoFormat) {
				// This pin outputs one of our supported formats! Use this pin
				// if we don't already have something better.
				if(DShowRenderer::compareFormats(
					m_bestOutPinFormat, format) > 0)
				{
					// This is the better format, use it. `m_bestOutPin` should
					// already be set by now so we don't need to test it.
					m_bestOutPin->Release();
					pin->AddRef();
					m_bestOutPin = pin;
					m_bestOutPinFormat = format;
				}
			}
		}
		types->Release();

		//---------------------------------------------------------------------

		// Clean up
continueFindBestOutputPin:
		pin->Release();
		pin = NULL;
	}

	// Clean up
	pins->Release();
	pins = NULL;
}

/// <summary>
/// Generates a cached list of all available output framerates and resolutions.
/// </summary>
void DShowVideoSource::fetchAllOutputModes()
{
	if(m_bestOutPin == NULL)
		return; // We require an output pin first

	// Clear the available modes list in a safe manner
	clearModeList();
	m_availModes.reserve(16);

	// TODO: Should we use `m_config->GetStreamCaps()` instead of
	// `m_bestOutPin->EnumMediaTypes()`? Apparently some cards such as Elgato
	// do not support it so we'll need to fallback to the current method. Just
	// note that `m_config` is currently not defined until the source is
	// actually initiated (We can move it to the `findBestOutputPin()` method
	// no worries).

	//-------------------------------------------------------------------------

	// Create media type enumeration object
	IEnumMediaTypes *types = NULL;
	HRESULT res = m_bestOutPin->EnumMediaTypes(&types);
	if(FAILED(res)) {
#if DEBUG_DIRECTSHOW
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to enumerate output pins. "
			<< "Reason = " << getDShowErrorCode(res);
#endif // DEBUG_DIRECTSHOW
		return;
	}

	// Enumerate over each media type
	AM_MEDIA_TYPE *type = NULL;
	while(types->Next(1, &type, NULL) == S_OK) {
		// We are only interested in video media types that have the same
		// output format as what we're going to use
		if(type->majortype != MEDIATYPE_Video) {
			DeleteMediaType(type);
			continue;
		}
		VidgfxPixFormat format =
			DShowRenderer::formatFromSubtype(type->subtype);
		if(format != m_bestOutPinFormat) {
			DeleteMediaType(type);
			continue;
		}

		// Get output size and framerate
		DShowMode mode;
		mode.size = QSize(0, 0);
		mode.framerate = 0.0f;
		DShowRenderer::parseMediaType(
			*type, &mode.size, &mode.framerate, NULL);

		// Append all unique modes to our list
		if(doesModeAlreadyExist(mode)) {
			DeleteMediaType(type);
			continue;
		}
		if(FAILED(CopyMediaType(&mode.type, type))) {
			DeleteMediaType(type);
			continue;
		}
		m_availModes.append(mode);
	}
	types->Release();
}

/// <summary>
/// Converts the specified `IMediaSample` to an RGB texture inside the graphics
/// context, saving it to `m_curTexture`.
/// </summary>
void DShowVideoSource::convertSampleToTexture(IMediaSample *sample)
{
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return; // We don't have a valid graphics context yet

	// We delay determining the final sample pixel format and size until now
	// just to be extra sure we know what it is.
	QSize texSize;
	if(m_curFormat == GfxNoFormat) {
		// Parse media sample type structure
		AM_MEDIA_TYPE *type = NULL;
		HRESULT res = m_config->GetFormat(&type);
		//HRESULT res = sample->GetMediaType(&type);
		if(FAILED(res) || type == NULL)
			return;
		m_curFormat = DShowRenderer::formatFromSubtype(type->subtype);
		float framerate = 0.0f;
		bool negHeight = false;
		DShowRenderer::parseMediaType(*type, &texSize, &framerate, &negHeight);
		DeleteMediaType(type);

		// Determine if the sample is vertically flipped or not. See `biHeight`
		// on this MSDN page:
		// http://msdn.microsoft.com/en-us/library/windows/desktop/dd318229%28v=vs.85%29.aspx
		if(m_curFormat == GfxRGB24Format ||
			m_curFormat == GfxRGB32Format ||
			m_curFormat == GfxARGB32Format)
		{
			// "For uncompressed RGB bitmaps, if biHeight is positive, the
			// bitmap is a bottom-up DIB with the origin at the lower left
			// corner. If biHeight is negative, the bitmap is a top-down DIB
			// with the origin at the upper left corner."
			if(negHeight)
				m_isFlipped = false;
			else
				m_isFlipped = true;
		} else {
			// "For YUV bitmaps, the bitmap is always top-down, regardless of
			// the sign of biHeight."
			m_isFlipped = false;
		}

		appLog(LOG_CAT) << QStringLiteral(
			"\"%1\" is outputting at size %2x%3 and framerate %4")
			.arg(getDebugString())
			.arg(texSize.width())
			.arg(texSize.height())
			.arg(framerate, 0, 'f', 2);
	}
	if(m_curFormat == GfxNoFormat)
		return; // Invalid format

	// Create our textures if we haven't already
	if(m_curTexture == NULL) {
		switch(m_curFormat) {
		default:
			return; // Invalid format
		case GfxRGB24Format:
		case GfxRGB32Format: // DXGI_FORMAT_B8G8R8X8_UNORM
		case GfxARGB32Format: // DXGI_FORMAT_B8G8R8A8_UNORM
			// We don't use any intermediate textures for these formats, we
			// write directly to the texture from the CPU.
			m_curTexture = vidgfx_context_new_tex(
				gfx, texSize, true, false, true);
			break;
		case GfxYV12Format: // NxM Y, (N/2)x(M/2) V, (N/2)x(M/2) U
		case GfxIYUVFormat: { // NxM Y, (N/2)x(M/2) U, (N/2)x(M/2) V
			// We require 3 separate planes for these formats
			QSize sizeA(texSize.width() / 4, texSize.height());
			QSize sizeBC(sizeA.width() / 2, sizeA.height() / 2);
			m_curPlaneA = vidgfx_context_new_tex(
				gfx, sizeA, true, false, false);
			m_curPlaneB = vidgfx_context_new_tex(
				gfx, sizeBC, true, false, false);
			m_curPlaneC = vidgfx_context_new_tex(
				gfx, sizeBC, true, false, false);
			m_curTexture = vidgfx_context_new_tex(
				gfx, texSize, false, false, false);
			break; }
		case GfxNV12Format: { // NxM Y, Nx(M/2) interleaved UV
			// We require 2 separate planes for this formats
			QSize sizeA(texSize.width() / 4, texSize.height());
			QSize sizeB(sizeA.width(), sizeA.height() / 2);
			m_curPlaneA = vidgfx_context_new_tex(
				gfx, sizeA, true, false, false);
			m_curPlaneB = vidgfx_context_new_tex(
				gfx, sizeB, true, false, false);
			m_curTexture = vidgfx_context_new_tex(
				gfx, texSize, false, false, false);
			break; }
		case GfxUYVYFormat: // UYVY
		case GfxHDYCFormat: // UYVY with BT.709
		case GfxYUY2Format: { // YUYV
			// We only require a single plane for these formats
			QSize sizeA(texSize.width() / 2, texSize.height());
			m_curPlaneA = vidgfx_context_new_tex(
				gfx, sizeA, true, false, false);
			m_curTexture = vidgfx_context_new_tex(
				gfx, texSize, false, false, false);
			break; }
		}
	} else {
		// Make sure the size variable is defined
		texSize = vidgfx_tex_get_size(m_curTexture);
	}

	// Do the actual conversion
	BYTE *sampleData = NULL;
	HRESULT res = sample->GetPointer(&sampleData);
	if(FAILED(res) || sampleData == NULL)
		return; // Failed to get sample data
	switch(m_curFormat) {
	default:
	case GfxRGB24Format: {
		// We must convert from 24-bit RGB to 32-bit RGB
		uchar *texData =
			reinterpret_cast<uchar *>(vidgfx_tex_map(m_curTexture));
		int texStride = vidgfx_tex_get_stride(m_curTexture);
		uchar *in = sampleData;
		for(int y = 0; y < texSize.height(); y++) {
			uchar *out = &texData[y*texStride];
			for(int x = 0; x < texSize.width(); x++) {
				*(out++) = *(in++);
				*(out++) = *(in++);
				*(out++) = *(in++);
				*(out++) = 0xFF;
			}
		}
		vidgfx_tex_unmap(m_curTexture);
		break; }
	case GfxRGB32Format: // DXGI_FORMAT_B8G8R8X8_UNORM
	case GfxARGB32Format: { // DXGI_FORMAT_B8G8R8A8_UNORM
		copyPlaneToTex(m_curTexture, sampleData,
			QSize(texSize.width() * 4, texSize.height()));
		break; }
	case GfxYV12Format: // NxM Y, (N/2)x(M/2) V, (N/2)x(M/2) U
	case GfxIYUVFormat: { // NxM Y, (N/2)x(M/2) U, (N/2)x(M/2) V
		// Copy raw data to the graphics card
		QSize halfSize = QSize(texSize.width() / 2, texSize.height() / 2);
		uchar *in = sampleData;
		in = copyPlaneToTex(m_curPlaneA, in, texSize);
		in = copyPlaneToTex(m_curPlaneB, in, halfSize);
		in = copyPlaneToTex(m_curPlaneC, in, halfSize);

		// Convert to BGRX
		VidgfxTex *tmpTex = vidgfx_context_convert_to_bgrx(
			gfx, m_curFormat, m_curPlaneA, m_curPlaneB, m_curPlaneC);
		if(tmpTex != NULL) {
			vidgfx_context_copy_tex_data(gfx, m_curTexture, tmpTex,
				QPoint(0, 0), QRect(QPoint(0, 0), texSize));
		}
		break; }
	case GfxNV12Format: { // NxM Y, Nx(M/2) interleaved UV
		// Copy raw data to the graphics card
		QSize halfHeight = QSize(texSize.width(), texSize.height() / 2);
		uchar *in = sampleData;
		in = copyPlaneToTex(m_curPlaneA, in, texSize);
		in = copyPlaneToTex(m_curPlaneB, in, halfHeight);

		// Convert to BGRX
		VidgfxTex *tmpTex = vidgfx_context_convert_to_bgrx(
			gfx, m_curFormat, m_curPlaneA, m_curPlaneB, NULL);
		if(tmpTex != NULL) {
			vidgfx_context_copy_tex_data(gfx, m_curTexture, tmpTex,
				QPoint(0, 0), QRect(QPoint(0, 0), texSize));
		}
		break; }
	case GfxUYVYFormat: // UYVY
	case GfxHDYCFormat: // UYVY with BT.709
	case GfxYUY2Format: { // YUYV
		// Copy raw data to the graphics card
		QSize packedWidth = QSize(texSize.width() / 2 * 4, texSize.height());
		copyPlaneToTex(m_curPlaneA, sampleData, packedWidth);

		// Convert to BGRX
		VidgfxTex *tmpTex = vidgfx_context_convert_to_bgrx(
			gfx, m_curFormat, m_curPlaneA, NULL, NULL);
		if(tmpTex != NULL) {
			vidgfx_context_copy_tex_data(gfx, m_curTexture, tmpTex,
				QPoint(0, 0), QRect(QPoint(0, 0), texSize));
		}
		break; }
	}
}

/// <summary>
/// Returns the unique ID of this video source.
/// </summary>
quint64 DShowVideoSource::getId() const
{
	return m_id;
}

QString DShowVideoSource::getFriendlyName() const
{
	return m_friendlyName;
}

QString DShowVideoSource::getDebugString() const
{
	QString compatible;
	if(m_bestOutPin != NULL) {
		if(m_bestOutPinFormat == GfxNoFormat)
			compatible = QStringLiteral(" (Might not be compatible)");
	} else
		compatible = QStringLiteral(" (Not compatible)");
	if(!compatible.isNull()) {
		return QStringLiteral("%1 [%2]%3")
			.arg(m_friendlyName)
			.arg(numberToHexString(m_id))
			.arg(compatible);
	}
	return QStringLiteral("%1 (%2) [%3]")
		.arg(m_friendlyName)
		.arg(VidgfxPixFormatStrings[m_bestOutPinFormat])
		.arg(numberToHexString(m_id));
}

bool DShowVideoSource::reference()
{
	m_ref++;
	if(m_ref == 1) {
		if(!initialize()) {
			// Already logged reason
			m_ref--;
			return false;
		}
	}
	return true;
}

void DShowVideoSource::dereference()
{
	if(m_ref <= 0) {
		appLog(LOG_CAT, Log::Warning) << QStringLiteral(
			"Video source \"%1\" was dereferenced more times than it was referenced")
			.arg(getDebugString());
		return;
	}
	m_ref--;
	if(m_ref == 0)
		shutdown();
}

int DShowVideoSource::getRefCount()
{
	return m_ref;
}

void DShowVideoSource::prepareFrame(uint frameNum, int numDropped)
{
	if(m_ref <= 0)
		return; // Not enabled

	// TODO: Properly calculate which frame to use instead of just using the
	// latest one. As we will be processing multiple frames at once quite often
	// just using the latest frame will result in temporal artifacts. For most
	// people this won't matter but if this is for a capture card or a 60fps
	// device then it'll become very noticeable.

	// Fetch the current sample from the renderer
	uint sampleNum = 0;
	IMediaSample *sample = m_renderFilter->lockCurSample(&sampleNum);
	if(sampleNum == m_curSampleNum) {
		// Nothing has changed, immediately return
		m_renderFilter->unlockCurSample();
		return;
	}

	// Release the previous frame's sample if we have one
	if(m_curSample != NULL) {
		m_curSample->Release();
		m_curSample = NULL;
	}

	// Reference the current sample so it doesn't get released until after this
	// entire frame has finished processing
	m_curSample = sample;
	m_curSample->AddRef();
	m_curSampleNum = sampleNum;
	m_renderFilter->unlockCurSample();

	// Clear our cache so the next call to `getCurrentFrame()` actually
	// processes the frame. We don't process the frame here as it may not
	// actually be required (Layer might be hidden).
	m_curSampleProcessed = false;
}

VidgfxTex *DShowVideoSource::getCurrentFrame()
{
	if(m_ref <= 0)
		return NULL; // Not enabled
	if(m_curSampleProcessed)
		return m_curTexture; // Texture already processed and has been cached
	m_curSampleProcessed = true;

	// Convert our DirectShow sample into a texture and then immediately
	// release the sample as it's no longer required
	if(m_curSample != NULL) {
		convertSampleToTexture(m_curSample);
		m_curSample->Release();
		m_curSample = NULL;
	}

	return m_curTexture;
}

bool DShowVideoSource::isFrameFlipped() const
{
	return m_isFlipped;
}

void DShowVideoSource::clearModeList()
{
	for(int i = 0; i < m_availModes.size(); i++) {
		DShowMode *mode = &m_availModes[i];
		FreeMediaType(mode->type);
	}
	m_availModes.clear();
}

bool DShowVideoSource::doesModeAlreadyExist(const DShowMode &mode)
{
	for(int i = 0; i < m_availModes.size(); i++) {
		const DShowMode &mod = m_availModes.at(i);
		if(mod.framerate == mode.framerate && mod.size == mode.size)
			return true;
	}
	return false;
}

/// <summary>
/// Sets the target mode to a sane default. We use the highest resolution that
/// doesn't exceed our canvas size of the highest, most popular framerate.
/// </summary>
void DShowVideoSource::setTargetModeToDefault()
{
	QVector<float> framerates = getFramerates();
	float bestFramerate = 0.0f;
	QVector<QSize> bestFramerateSizes;
	for(int i = 0; i < framerates.size(); i++) {
		float curFramerate = framerates.at(i);
		QVector<QSize> sizes = getSizesForFramerate(curFramerate);
		if(sizes.size() < bestFramerateSizes.size())
			continue; // Less popular than our current best
		if(sizes.size() == bestFramerateSizes.size() &&
			curFramerate < bestFramerate)
		{
			// Lower framerate than our current best
			continue;
		}

		// This is the new best
		bestFramerate = curFramerate;
		bestFramerateSizes = sizes;
	}

	// If we have a profile active then don't use a frame size that's larger
	// than our canvas by default.
	QSize maxSize(INT_MAX, INT_MAX);
	Profile *profile = App->getProfile();
	if(profile != NULL)
		maxSize = profile->getCanvasSize();

	// Find the highest frame size for the best framerate
	QSize bestSize;
	for(int i = 0; i < bestFramerateSizes.size(); i++) {
		const QSize &size = bestFramerateSizes.at(i);
		if(size.height() < bestSize.height())
			continue; // Shorter than our current best
		if(size.width() < bestSize.width())
			continue; // Thinner than our current best
		if(size.width() > maxSize.width() || size.height() > maxSize.height())
			continue; // Exceeds our canvas size

		// This is the new best
		bestSize = size;
	}

	// Update our target mode
	m_targetFramerate = bestFramerate;
	m_targetSize = bestSize;

#if 0
	// Debugging
	appLog(LOG_CAT) << QStringLiteral(
		"Default for \"%1\" is size %2x%3 and framerate %4")
		.arg(getDebugString())
		.arg(m_targetSize.width())
		.arg(m_targetSize.height())
		.arg(m_targetFramerate, 0, 'f', 2);
#endif // 0
}

/// <summary>
/// Finds the exact DirectShow media type that most closely matches the target
/// mode.
/// </summary>
AM_MEDIA_TYPE *DShowVideoSource::getClosestTypeToMode(
	float framerate, const QSize &size)
{
	// Find the closest framerate from our known supported mode
	QVector<float> framerates = getFramerates();
	float bestFramerate = -10000.0f;
	for(int i = 0; i < framerates.size(); i++) {
		if(qAbs(framerate - framerates.at(i)) >
			qAbs(framerate - bestFramerate))
		{
			continue;
		}
		bestFramerate = framerates.at(i);
	}

	// Find the closest size from our known supported sizes based on area if we
	// don't have an exact match
	QVector<QSize> sizes = getSizesForFramerate(bestFramerate);
	QSize bestSize;
	int bestArea = -10000;
	int trgtArea = size.width() * size.height();
	for(int i = 0; i < sizes.size(); i++) {
		const QSize curSize = sizes.at(i);
		int curArea = curSize.width() * curSize.height();
		if(size == curSize) {
			bestSize = curSize;
			bestArea = curArea;
			break;
		}
		if(qAbs(trgtArea - curArea) > qAbs(trgtArea - bestArea))
			continue;
		bestSize = curSize;
		bestArea = curArea;
	}

	// Find and return our exact media type
	for(int i = 0; i < m_availModes.size(); i++) {
		DShowMode *mode = &m_availModes[i];
		if(mode->framerate == bestFramerate && mode->size == bestSize)
			return &(mode->type);
	}
	return NULL;
}

QVector<float> DShowVideoSource::getFramerates() const
{
	QVector<float> ret;
	ret.reserve(8);
	for(int i = 0; i < m_availModes.size(); i++) {
		const DShowMode &mode = m_availModes.at(i);

		// Prevent adding the same framerate multiple times
		bool exists = false;
		for(int j = 0; j < ret.size(); j++) {
			if(mode.framerate == ret.at(j)) {
				exists = true;
				break;
			}
		}
		if(exists)
			continue;

		ret.append(mode.framerate);
	}
	return ret;
}

QVector<QSize> DShowVideoSource::getSizesForFramerate(float framerate) const
{
	QVector<QSize> ret;
	ret.reserve(16);
	for(int i = 0; i < m_availModes.size(); i++) {
		const DShowMode &mode = m_availModes.at(i);
		if(mode.framerate != framerate)
			continue;
		ret.append(mode.size);
	}
	return ret;
}

/// <summary>
/// Configures the source to use the closest available mode to the specified
/// one. If the source is referenced then it will reset the source.
/// </summary>
void DShowVideoSource::setMode(float framerate, const QSize &size)
{
	// Log to file so that we can figure out that the user is explicitly
	// changing modes and that it didn't just randomly fail
	appLog(LOG_CAT) << QStringLiteral(
		"\"%1\" is changing mode to size %2x%3 and framerate %4")
		.arg(getDebugString())
		.arg(size.width())
		.arg(size.height())
		.arg(framerate, 0, 'f', 2);

	// Remember the original state
	float oldFramerate = m_targetFramerate;
	QSize oldSize = m_targetSize;

	if(m_targetFramerate == framerate && m_targetSize == size)
		return; // No change
	m_targetFramerate = framerate;
	m_targetSize = size;
	if(m_ref <= 0)
		return;

	// If we are active then `m_config->SetFormat()` can fail. Set the mode by
	// completely resetting the source.
	shutdown();
	if(!initialize()) {
		// Revert to the old mode if we failed. WARNING: This can also fail! If
		// it does then everything will break as other parts of the program
		// assume the source is referenced.
		m_targetFramerate = oldFramerate;
		m_targetSize = oldSize;
		if(!initialize()) {
			appLog(LOG_CAT, Log::Critical) << QStringLiteral(
				"Failed to reinitialize after a mode change, things will break!");
			m_ref = 0;
		}
	}
}

/// <summary>
/// Configures the best pin to use the closest available mode to the target
/// one.
/// </summary>
/// <returns>True if the pin was successfully configured</returns>
bool DShowVideoSource::configureBestPinToTargetMode()
{
	if(m_config == NULL)
		return false;
	AM_MEDIA_TYPE *type =
		getClosestTypeToMode(m_targetFramerate, m_targetSize);
	if(type == NULL)
		return false; // Should never happen
	HRESULT res = m_config->SetFormat(type);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to configure pin output mode, will use device default. "
			<< "Reason = " << getDShowErrorCode(res);
		//return false; // Return true even if we failed
	}

	// Log debug message
	QSize actualSize;
	float actualFramerate;
	DShowRenderer::parseMediaType(*type, &actualSize, &actualFramerate, NULL);
	//actualSize = m_targetSize;
	//actualFramerate = m_targetFramerate;
	appLog(LOG_CAT) << QStringLiteral(
		"Configured \"%1\" to use size %2x%3 and framerate %4")
		.arg(getDebugString())
		.arg(actualSize.width())
		.arg(actualSize.height())
		.arg(actualFramerate, 0, 'f', 2);

	return true;
}

float DShowVideoSource::getModeFramerate()
{
	if(m_targetFramerate == 0.0f || m_targetSize.isEmpty())
		setTargetModeToDefault();
	return m_targetFramerate;
}

QSize DShowVideoSource::getModeSize()
{
	if(m_targetFramerate == 0.0f || m_targetSize.isEmpty())
		setTargetModeToDefault();
	return m_targetSize;
}

void DShowVideoSource::showConfigureDialog(QWidget *parent)
{
	// Originally based off:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dd375480%28v=vs.85%29.aspx

	ISpecifyPropertyPages *pProp;
	HRESULT hr = m_filter->QueryInterface(
		IID_ISpecifyPropertyPages, (void **)&pProp);
	if(FAILED(hr) || pProp == NULL)
		return;

	// Get the filter's name and IUnknown pointer
	FILTER_INFO filterInfo;
	hr = m_filter->QueryFilterInfo(&filterInfo);
	if(FAILED(hr))
		return;
	IUnknown *unknown;
	hr = m_filter->QueryInterface(IID_IUnknown, (void **)&unknown);
	if(FAILED(hr) || unknown == NULL) {
		// Should never fail
		filterInfo.pGraph->Release();
		return;
	}

	// Get the HWND of the QWidget
	HWND hwnd = NULL;
	if(parent != NULL) {
		QPlatformNativeInterface *native =
			QGuiApplication::platformNativeInterface();
		HWND hwnd = static_cast<HWND>(
			native->nativeResourceForWindow("handle", parent->windowHandle()));
	}

	// Show the page
	CAUUID caGUID;
	pProp->GetPages(&caGUID);
	pProp->Release();
	OleCreatePropertyFrame(
		hwnd, 0, 0, filterInfo.achName, 1, &unknown, caGUID.cElems,
		caGUID.pElems, 0, 0, NULL);

	// Clean up
	unknown->Release();
	if(filterInfo.pGraph != NULL)
		filterInfo.pGraph->Release();
	CoTaskMemFree(caGUID.pElems);
}
