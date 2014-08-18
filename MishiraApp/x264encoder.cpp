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

#include "x264encoder.h"
#include "application.h"
#include <QtCore/qglobal.h>

#define DUMP_STREAM_TO_FILE 0
#if DUMP_STREAM_TO_FILE
#include <QtCore/QFile>
#endif

const QString LOG_CAT = QStringLiteral("Video");

//=============================================================================
// Helpers

static void x264LogHandler(
	void *opaque, int i_level, const char *psz, va_list args)
{
	X264Encoder *enc = static_cast<X264Encoder *>(opaque);

	Log::LogLevel lvl;
	switch(i_level) {
	default:
	case X264_LOG_NONE:
		lvl = Log::Critical;
		break;
	case X264_LOG_ERROR:
		lvl = Log::Critical;
		break;
	case X264_LOG_WARNING:
		lvl = Log::Warning;
		break;
	case X264_LOG_INFO:
		lvl = Log::Notice;
		break;
	case X264_LOG_DEBUG:
		lvl = Log::Notice;
		break;
	}

	// Convert the message into a formatted QString
	QString str;
	int size = 256;
	char *buf = new char[size];
	for(;;) {
		//va_start(args, psz); // Only needed if using "..."
#pragma warning(push)
#pragma warning(disable: 4996)
		int retval = vsnprintf(buf, size, psz, args);
#pragma warning(pop)
		//va_end(args);
		if(retval >= size) {
			// Buffer wasn't large enough, enlarge
			size = retval + 1;
			delete[] buf;
			buf = new char[size];
			continue;
		}

		if(retval >= 0) {
			// Success, use the out buffer as our string
			str = buf;
			break;
		} else {
			// Error occurred, just use the format string
			str = psz;
			break;
		}
	}
	delete[] buf;

	// Output to our log system
	appLog(QStringLiteral("x264"), lvl) << str;
}

//=============================================================================
// X264Encoder class

#if DUMP_STREAM_TO_FILE
static QByteArray dumpBuffer;
#endif

/// <summary>
/// Returns the best bitrate for the specified input in Kb/s. I.e. it does the
/// exact opposite of `determineBestSize()`.
/// </summary>
int X264Encoder::determineBestBitrate(
	const QSize &size, Fraction framerate, bool highAction)
{
	// This formula is slightly HACKy at the moment and was a best-fit of
	// various online recommendations and some very unscientific
	// experimentations. The worksheet for calculating this formula can be
	// found at "P:\Development\Citrine\Bitrate recommendation.xlsx"
	//
	// The general idea is that the final bitrate is based on a "bits per pixel
	// per frame" value that is varied by the framerate and resolution. Due to
	// the way video is encoded you can get away with less data per pixel at
	// higher resolutions and the same can be said with framerate as each frame
	// has a smaller amount of movement since the previous frame.
	//
	// We also take into account the typical internet connection speed of the
	// majority of users which is only a few Mbps at most. This really only
	// affects 720p@60 and 1080p though and is added to the formula by
	// exaggerating the "pixel multiplier" slightly.
	//
	// After we determine the best bitrate we round it to a value that is more
	// readable to humans.
	//
	// Current low action recommendations:
	//
	// 				240p	360p	480p	540p	720p	1080p
	//
	// 15 Hz		150		300		500		600		1000	1900
	// 20 Hz		150		350		600		800		1300	2400
	// 23.976 Hz	200		400		700		900		1500	2800
	// 24 Hz		200		400		700		900		1500	2800
	// 25 Hz		200		400		750		900		1500	2900
	// 30 Hz		200		500		800		1000	1700	3200
	// 50 Hz		250		600		1000	1300	2100	3900
	// 60 Hz		250		550		1000	1200	2100	3800
	//
	// Current high action recommendations:
	//
	// 				240p	360p	480p	540p	720p	1080p
	//
	// 15 Hz		200		450		800		1000	1700	3100
	// 20 Hz		250		600		1000	1300	2100	4000
	// 23.976 Hz	300		650		1200	1500	2400	4500
	// 24 Hz		300		650		1200	1500	2400	4500
	// 25 Hz		300		700		1200	1500	2500	4700
	// 30 Hz		350		800		1300	1700	2800	5200
	// 50 Hz		450		950		1600	2100	3400	6400
	// 60 Hz		400		950		1600	2000	3400	6300

	// PLEASE UPDATE THE TABLES ABOVE IF YOU CHANGE THIS FORMULA! Use the
	// `debugBestBitrates()` function below to aid you.
	float area = (float)(size.width() * size.height());
	float basePerFrame = area * (highAction ? 0.090f : 0.055f);
	float fps = qMax(15.0f, framerate.asFloat());
	float pxMultiplier =
		1.0f - (area - 230400.0f) / (2073600.0f - 230400.0f) * 0.25f;
	float fpsMultiplier =
		(fps - 15.0f) * (0.75f - 1.5f) / (60.0f - 15.0f) + 1.5f;
	float bitPerFrame = basePerFrame * pxMultiplier * fpsMultiplier;
	float bitrate = bitPerFrame * fps * 0.001f; // b/s -> Kb/s
	if(bitrate <= 1000.0f)
		return qRound(bitrate / 50.0f) * 50;
	return qRound(bitrate / 100.0f) * 100;
}

/// <summary>
/// Returns the best video size for the specified input in Kb/s. I.e. it does
/// the exact opposite of `determineBestBitrate()`.
/// </summary>
QSize X264Encoder::determineBestSize(
	int bitrate, Fraction framerate, bool highAction)
{
	// Brute force the size based on the output of `determineBestBitrate()` so
	// that we don't need to worry about updating this method whenever the
	// other one changes

	// Recommended output sizes. Only these are ever considered
	const int numSizes = 6;
	const QSize sizes[] = {
		QSize(426, 240),
		QSize(640, 360),
		QSize(854, 480),
		QSize(960, 540),
		QSize(1280, 720),
		QSize(1920, 1080)
	};

	// Determine recommended bitrate for each size
	int bitrates[numSizes];
	for(int i = 0; i < numSizes; i++)
		bitrates[i] = determineBestBitrate(sizes[i], framerate, highAction);

	// Find the closest size bitrate to the input bitrate. We round up only if
	// it'll result in only a minor degradation of visual quality.
	for(int i = numSizes - 1; i >= 0; i--) {
		if(bitrate > bitrates[i] * 0.8f)
			return sizes[i];
	}
	return sizes[0]; // Smallest size
}

/// <summary>
/// Outputs a table of recommended bitrates to the log file for debugging
/// purposes.
/// </summary>
void X264Encoder::debugBestBitrates()
{
	const int numSizes = 6;
	const QSize sizes[] = {
		QSize(426, 240),
		QSize(640, 360),
		QSize(854, 480),
		QSize(960, 540),
		QSize(1280, 720),
		QSize(1920, 1080)
	};
	int numFramerates = 0;
	for(;;numFramerates++) {
		if(PrflFpsRates[numFramerates] == Fraction(0, 0))
			break;
	}

	appLog() << "Low action bitrate recommendations:";
	QString line = QStringLiteral("\t\t");
	for(int i = 0; i < numSizes; i++)
		line += QStringLiteral("\t%1p").arg(sizes[i].height());
	appLog() << line;
	for(int i = 0; i < numFramerates; i++) {
		line = QStringLiteral("%1\t").arg(PrflFpsRatesStrings[i]);
		for(int j = 0; j < numSizes; j++) {
			int bitrate = X264Encoder::determineBestBitrate(
				sizes[j], PrflFpsRates[i], false);
			line += QStringLiteral("\t%1").arg(bitrate);
		}
		appLog() << line;
	}

	appLog() << "High action bitrate recommendations:";
	line = QStringLiteral("\t\t");
	for(int i = 0; i < numSizes; i++)
		line += QStringLiteral("\t%1p").arg(sizes[i].height());
	appLog() << line;
	for(int i = 0; i < numFramerates; i++) {
		line = QStringLiteral("%1\t").arg(PrflFpsRatesStrings[i]);
		for(int j = 0; j < numSizes; j++) {
			int bitrate = X264Encoder::determineBestBitrate(
				sizes[j], PrflFpsRates[i], true);
			line += QStringLiteral("\t%1").arg(bitrate);
		}
		appLog() << line;
	}

	appLog() << "Low action size recommendations:";
	line = QStringLiteral("\t\t");
	for(int i = 0; i < numFramerates; i++)
		line += QStringLiteral("\t%1").arg(PrflFpsRatesStrings[i]);
	appLog() << line;
	for(int i = 0; i < 6000; i += 100) {
		line = QStringLiteral("%1 Kb/s\t").arg(i);
		for(int j = 0; j < numFramerates; j++) {
			QSize size = X264Encoder::determineBestSize(
				i, PrflFpsRates[j], false);
			line += QStringLiteral("\t%1x%2")
				.arg(size.width()).arg(size.height());
		}
		appLog() << line;
	}

	appLog() << "High action size recommendations:";
	line = QStringLiteral("\t\t");
	for(int i = 0; i < numFramerates; i++)
		line += QStringLiteral("\t%1").arg(PrflFpsRatesStrings[i]);
	appLog() << line;
	for(int i = 0; i < 6000; i += 100) {
		line = QStringLiteral("%1 Kb/s\t").arg(i);
		for(int j = 0; j < numFramerates; j++) {
			QSize size = X264Encoder::determineBestSize(
				i, PrflFpsRates[j], true);
			line += QStringLiteral("\t%1x%2")
				.arg(size.width()).arg(size.height());
		}
		appLog() << line;
	}
}

X264Encoder::X264Encoder(
	Profile *profile, QSize size, SclrScalingMode scaling,
	GfxFilter scaleFilter, Fraction framerate, const X264Options &opt)
	: VideoEncoder(
	profile, VencX264Type, size, scaling, scaleFilter, framerate)
	, m_scaler(NULL)
	, m_testScaler(NULL)
	, m_x264(NULL)
	, m_params()

	// Options
	, m_preset(opt.preset)
	, m_bitrate(opt.bitrate)
	, m_keyInterval(opt.keyInterval)

	// State
	, m_nextPts(0)
	, m_forceIdr(-1)
	//, m_pics() // Custom x264 allocation method
	, m_prevPic(0)
	, m_picInfoStack()
	, m_numPicInfoAllocated(0)
	, m_inLowCpuMode(false)
	, m_lowCpuModeCount(0)
	, m_lowCpuModeFrameCount(0)
	, m_encodeErrorCount(0)
{
	memset(&m_params, 0, sizeof(m_params));

	//-------------------------------------------------------------------------
	// Memory reuse: As raw video frames can use quite a bit of memory we want
	// to reuse structures while allocating the least amount of space as
	// possible. We also need to allocate an unknown amount of PictureInfo
	// structures to pass our private data through x264's delayed encoding
	// pipeline. On top of all this we also need to keep the previous frame in
	// memory so we can duplicate it at any time.
	//
	// So this is what we do:
	//
	// We allocate two x264_picture_t structures (m_pics) and ping-pong between
	// them. x264 copies the raw pixel data out of them when we pass them to
	// the encode function so all we're doing is keeping the initial memory
	// allocation and not the actual data itself.
	//
	// We allocate our PictureInfos on the heap and add any unused structures
	// to a stack (m_picInfoStack). We don't use a container such as QVector as
	// when they are resized their memory location changes meaning our pointers
	// are no longer valid. We don't need to keep a master list of allocations
	// as all of the structures will eventually exit the x264 pipeline (If we
	// properly flush before terminating) and get added to the stack.
	x264_picture_init(&m_pics[0]); // Init only here, allocate later
	x264_picture_init(&m_pics[1]);
	m_picInfoStack.reserve(16);

#if DUMP_STREAM_TO_FILE
	dumpBuffer.reserve(20 * 1024 * 1024); // 20MB
#endif
}

X264Encoder::~X264Encoder()
{
	shutdownEncoder();

#if DUMP_STREAM_TO_FILE
	QFile dumpFile(App->getDataDirectory().filePath("dump.h264"));
	appLog(Log::Critical)
		<< "Dumping H.264 stream to file: \"" << dumpFile.fileName()
		<< "\" (File size: " << dumpBuffer.size() << ")";
	if(dumpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		dumpFile.write(dumpBuffer);
		dumpFile.close();
		dumpBuffer.clear();
	}
#endif
}

X264Encoder::PictureInfo *X264Encoder::getNextPicInfoFromStack()
{
	if(m_picInfoStack.size() > 0) {
		// We have a free structure already allocated, use it
		return m_picInfoStack.pop();
	}
	// Not enough structures available available, allocate a new one
	PictureInfo *pic = new PictureInfo;
	if(pic == NULL)
		return NULL; // Return immediately
	m_numPicInfoAllocated++;
	return pic;
}

/// <summary>
/// WARNING: Must be called after x264 has been completely flushed.
/// </summary>
void X264Encoder::freeAllPicInfos()
{
	appLog(LOG_CAT)
		<< "Allocated a total of " << m_numPicInfoAllocated
		<< " x264 picture infos";
	m_numPicInfoAllocated =
		qMax(0, m_numPicInfoAllocated - m_picInfoStack.size());
	if(m_numPicInfoAllocated != 0) {
		appLog(LOG_CAT, Log::Warning)
			<< "Cannot unallocate all x264 picture infos (Possible memory "
			<< "leak). " << m_numPicInfoAllocated << " structs not in the "
			<< "stack";
	}
	while(m_picInfoStack.size())
		delete m_picInfoStack.pop();
}

bool X264Encoder::initializeEncoder()
{
	if(m_isRunning)
		return true;
	appLog(LOG_CAT) << "Initializing x264 (Encoder " << getId() << ")...";

	// Convert preset enumeration into a string
	QByteArray preset;
	switch(m_preset) {
	case X264UltraFastPreset:
		preset = QByteArrayLiteral("ultrafast");
		break;
	default:
	case X264SuperFastPreset:
		preset = QByteArrayLiteral("superfast");
		break;
	case X264VeryFastPreset:
		preset = QByteArrayLiteral("veryfast");
		break;
	case X264FasterPreset:
		preset = QByteArrayLiteral("faster");
		break;
	case X264FastPreset:
		preset = QByteArrayLiteral("fast");
		break;
	case X264MediumPreset:
		preset = QByteArrayLiteral("medium");
		break;
	case X264SlowPreset:
		preset = QByteArrayLiteral("slow");
		break;
	case X264SlowerPreset:
		preset = QByteArrayLiteral("slower");
		break;
	case X264VerySlowPreset:
		preset = QByteArrayLiteral("veryslow");
		break;
#if 0
	case X264PlaceboPreset:
		preset = QByteArrayLiteral("placebo");
		break;
#endif // 0
	}

	// Apply defaults locally so it doesn't modify the UI
	int keyInterval =
		(m_keyInterval > 0 ? m_keyInterval : DEFAULT_KEYFRAME_INTERVAL);

	appLog(LOG_CAT) <<
		QStringLiteral("Encoder settings: Size = %L1x%L2; Scaling = %3; Filter = %4; Preset = %5; Bitrate = %L6; Key int = %L7")
		.arg(m_size.width())
		.arg(m_size.height())
		.arg((int)m_scaling)
		.arg((int)m_scaleFilter)
		.arg(QString::fromLatin1(preset))
		.arg(m_bitrate)
		.arg(keyInterval);

	//-------------------------------------------------------------------------
	// Configure parameters (x264 zeros for us)

	if(x264_param_default_preset(&m_params, preset.constData(), NULL) < 0) {
		appLog(LOG_CAT, Log::Warning)
			<< "x264_param_default_preset() failed, cannot enable encoder";
		return false;
	}

	// Intercept log messages
	m_params.pf_log = x264LogHandler;
	m_params.p_log_private = this;

#define FORCE_SINGLE_THREADED 0
#if FORCE_SINGLE_THREADED
	// Force single threaded operation for testing thread synchronization
	m_params.i_threads = 1;
#endif // FORCE_SINGLE_THREADED

	// Setup rate control method: Constant/average bitrate
	m_params.rc.i_rc_method = X264_RC_ABR;
	m_params.b_vfr_input = 0;
	m_params.rc.i_bitrate = m_bitrate; // Target bitrate (Kb)

	// Setup the "video buffer verifier" (VBV) so that the output stream can
	// have a concept of a "X second buffer". Without this there is no set
	// limit on how much data needs to be buffered before it can be decoded.
	// Larger buffers improves quality as it allows for larger keyframes.
	//
	// While VBV should be able to be summarised as "To decode the next frame I
	// need to never read more than <i_vbv_buffer_size> Kb ahead" this is not
	// actually the case when looking at the output as VBV-constrained video
	// still vastly overshoots the VBV buffer size.
	//
	// Twitch's player uses a 1 second playback buffer so if we never want the
	// stream to freeze after it has buffered once we should have a 1 second
	// VBV buffer ourselves.
	m_params.rc.i_vbv_buffer_size = m_bitrate; // Buffer size (Kb)
	m_params.rc.i_vbv_max_bitrate = m_bitrate; // Fill speed (Kb/s)

	// Setup the "hypothetical reference decoder" (HRD)
	//m_params.i_nal_hrd = X264_NAL_HRD_CBR;

	// Setup size and framerate
	m_params.i_width = m_size.width();
	m_params.i_height = m_size.height();
	m_params.i_fps_num = m_framerate.numerator;
	m_params.i_fps_den = m_framerate.denominator;
	m_params.i_timebase_num = m_params.i_fps_den; // Not sure if needed
	m_params.i_timebase_den = m_params.i_fps_num;

	// Have an IDR keyframe at least once every X seconds instead of at least
	// once every 250 frames. The periodic intra refresh feature
	// ("b_intra_refresh") requires SEI recovery point packets to be
	// transmitted to work therefore shouldn't be used for network
	// broadcasting.
	int keyIntMax = keyInterval * m_params.i_fps_num / m_params.i_fps_den;
	//m_params.i_keyint_max = qMin(keyIntMax, m_params.i_keyint_max);
	m_params.i_keyint_max = keyIntMax;

	// Make sure that the output is playable by Adobe Flash and other common
	// consumer decoders.
	if(x264_param_apply_profile(&m_params, "high") < 0) {
		appLog(LOG_CAT, Log::Warning)
			<< "x264_param_apply_profile() failed, cannot enable encoder";
		return false;
	}

	//-------------------------------------------------------------------------

	// Initialize x264
	m_x264 = x264_encoder_open(&m_params);
	if(m_x264 == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "x264_encoder_open() failed, cannot enable encoder";
		return false;
	}

	// Reset state
	m_nextPts = 0;
	m_forceIdr = -1;
	m_prevPic = 0;
	m_inLowCpuMode = false;
	m_lowCpuModeCount = 0;
	m_lowCpuModeFrameCount = 0;
	m_encodeErrorCount = 0;

	// Allocate picture memory
	if(x264_picture_alloc(
		&m_pics[0], X264_CSP_NV12, m_size.width(), m_size.height()) < 0)
	{
		appLog(LOG_CAT, Log::Warning)
			<< "x264_picture_alloc() failed, cannot enable encoder";
		x264_encoder_close(m_x264);
		m_x264 = NULL;
		return false;
	}
	if(x264_picture_alloc(
		&m_pics[1], X264_CSP_NV12, m_size.width(), m_size.height()) < 0)
	{
		appLog(LOG_CAT, Log::Warning)
			<< "x264_picture_alloc() failed, cannot enable encoder";
		x264_picture_clean(&m_pics[0]);
		x264_encoder_close(m_x264);
		m_x264 = NULL;
		return false;
	}

	appLog(LOG_CAT) << "x264 initialized";

	// Create our scaler or test scaler and connect its output signals
	if(m_profile == NULL) {
		m_testScaler = new TestScaler(m_size);
		connect(m_testScaler, &TestScaler::nv12FrameReady,
			this, &X264Encoder::nv12FrameReady);
	} else {
		m_scaler = Scaler::getOrCreate(
			m_profile, m_size, m_scaling, m_scaleFilter, GfxNV12Format);
		connect(m_scaler, &Scaler::nv12FrameReady,
			this, &X264Encoder::nv12FrameReady);
	}

	// Notify the application that we are encoding and that it should process
	// every frame
	App->refProcessAllFrames();

	m_isRunning = true;
	return true;
}

void X264Encoder::shutdownEncoder(bool flush)
{
	if(!m_isRunning)
		return;
	appLog(LOG_CAT) << "Shutting down x264 (Encoder " << getId() << ")...";

	// Flush the encoder so we don't get any memory leaks. Must be done first.
	flushFrames();

	// Release the scaler or test scaler and disconnect it so that we no longer
	// receive any frame signals from it.
	if(m_testScaler != NULL) {
		disconnect(m_testScaler, &TestScaler::nv12FrameReady,
			this, &X264Encoder::nv12FrameReady); // Must disconnect first
		delete m_testScaler;
		m_testScaler = NULL;
	}
	if(m_scaler != NULL) {
		disconnect(m_scaler, &Scaler::nv12FrameReady,
			this, &X264Encoder::nv12FrameReady); // Must disconnect first
		m_scaler->release();
		m_scaler = NULL;
	}

	if(m_lowCpuModeCount <= 0) {
		appLog(LOG_CAT) << QStringLiteral(
			"Entered low CPU usage mode %L1 times for a total of %L2 frames")
			.arg(m_lowCpuModeCount).arg(m_lowCpuModeFrameCount);
	}
	freeAllPicInfos();
	x264_picture_clean(&m_pics[0]);
	x264_picture_clean(&m_pics[1]);
	x264_encoder_close(m_x264);
	m_x264 = NULL;

	// Notify the application that we are are no longer encoding and that it
	// should not process every frame if there is no other active encoders.
	App->derefProcessAllFrames();

	appLog(LOG_CAT) << "x264 shut down";
	m_isRunning = false;
}

void X264Encoder::flushFrames()
{
	if(!m_isRunning)
		return;

	// TODO: Flush scaler as well to make sure that every frame that has been
	// requested to be encoded has been. Right now we possibly drop the last
	// couple of frames.

	// Flush x264 of all buffered frames. Note that once we begin to pass NULL
	// into `x264_encoder_encode()` we can no longer add new frames.
	while(x264_encoder_delayed_frames(m_x264) > 0) {
		x264_picture_t outPic;
		x264_nal_t *nals;
		int numNals;
		x264_picture_init(&outPic); // Zero memory
		if(x264_encoder_encode(m_x264, &nals, &numNals, NULL, &outPic) < 0) {
			appLog(LOG_CAT, Log::Warning)
				<< "x264_encoder_encode() failed during flushing";
			return;
		}
		if(!processNALs(outPic, nals, numNals)) {
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to process x264 NALs during flushing";
		}
	}
}

void X264Encoder::forceKeyframe()
{
	if(!m_isRunning)
		return;
	// We have to delay the force as the API states that it is the next frame
	// to be passed to the encoder that will be the keyframe and not just any
	// frame that's in our pipeline. Targets rely on this to provide fast
	// activation.
	m_forceIdr = SCALER_DELAY;
}

void X264Encoder::setLowCPUUsageMode(bool enable)
{
	if(!m_isRunning)
		return;
	if(enable == m_inLowCpuMode)
		return; // No change
	if(enable) {
		// Enter low CPU usage mode. These settings are based on the
		// "ultrafast" preset with some modifications to allow the settings to
		// be reverted back to the original user settings at a later time.

		x264_param_t params;
		memcpy(&params, &m_params, sizeof(params));

		// "Ultrafast" preset settings
		params.i_frame_reference = 1;
		//params.i_scenecut_threshold = 0;
		params.b_deblocking_filter = 0;
		//params.b_cabac = 0;
		//params.i_bframe = 0;
		params.analyse.intra = 0;
		params.analyse.inter = 0;
		params.analyse.b_transform_8x8 = 0;
		//params.analyse.i_me_method = X264_ME_DIA;
		//params.analyse.i_subpel_refine = 0;
		//params.rc.i_aq_mode = 0;
		params.analyse.b_mixed_references = 0;
		params.analyse.i_trellis = 0;
		//params.i_bframe_adaptive = X264_B_ADAPT_NONE;
		//params.rc.b_mb_tree = 0;
		//params.analyse.i_weighted_pred = X264_WEIGHTP_NONE;
		//params.analyse.b_weighted_bipred = 0;
		params.rc.i_lookahead = 0;

		// Extra modifications
		params.analyse.b_fast_pskip = 1;
		params.analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL;
		params.analyse.i_subpel_refine = 1;

		if(x264_encoder_reconfig(m_x264, &params) < 0) {
			appLog(LOG_CAT, Log::Warning) << QStringLiteral(
				"x264_encoder_reconfig() failed while entering low CPU usage mode");
			// WARNING: Encoder is partially reconfigured!
		}
	} else {
		// Exit low CPU usage mode
		if(x264_encoder_reconfig(m_x264, &m_params) < 0) {
			appLog(LOG_CAT, Log::Warning) << QStringLiteral(
				"x264_encoder_reconfig() failed while exiting low CPU usage mode");
			// WARNING: Encoder is partially reconfigured!
		}
	}

	m_inLowCpuMode = enable;
}

bool X264Encoder::isInLowCPUUsageMode() const
{
	return m_inLowCpuMode;
}

QString X264Encoder::getInfoString() const
{
	// Convert preset enumeration into a string
	QString preset;
	switch(m_preset) {
	case X264UltraFastPreset:
		preset = tr("Ultra fast");
		break;
	default:
	case X264SuperFastPreset:
		preset = tr("Super fast");
		break;
	case X264VeryFastPreset:
		preset = tr("Very fast");
		break;
	case X264FasterPreset:
		preset = tr("Faster");
		break;
	case X264FastPreset:
		preset = tr("Fast");
		break;
	case X264MediumPreset:
		preset = tr("Medium");
		break;
	case X264SlowPreset:
		preset = tr("Slow");
		break;
	case X264SlowerPreset:
		preset = tr("Slower");
		break;
	case X264VerySlowPreset:
		preset = tr("Very slow");
		break;
#if 0
	case X264PlaceboPreset:
		preset = tr("Super slow");
		break;
#endif // 0
	}

	return tr("%L1x%L2, Software H.264, %L3 Kb/s, %4")
		.arg(m_size.width())
		.arg(m_size.height())
		.arg(m_bitrate)
		.arg(preset);
}

int X264Encoder::getAvgBitrateForCongestion() const
{
	return m_bitrate;
}

void X264Encoder::serialize(QDataStream *stream) const
{
	VideoEncoder::serialize(stream);

	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << (quint32)m_preset;
	*stream << (qint32)m_bitrate;
	*stream << (qint32)m_keyInterval;
}

bool X264Encoder::unserialize(QDataStream *stream)
{
	if(!VideoEncoder::unserialize(stream))
		return false;

	qint32	int32Data;
	quint32	uint32Data;

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		*stream >> uint32Data;
		m_preset = (X264Preset)uint32Data;
		*stream >> int32Data;
		m_bitrate = int32Data;
		*stream >> int32Data;
		m_keyInterval = int32Data;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in x264 video encoder serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

/// <summary>
/// WARNING: Invalidates any non-permanent EncodedPackets.
/// </summary>
/// <returns>True if the frame was accepted by the encoder.</returns>
bool X264Encoder::encodeNV12Frame(const NV12Frame &frame, uint frameNum)
{
	if(!m_isRunning)
		return false;
	//appLog() << "**FRAME**";

	// Is the application in low CPU usage mode? If so obey it
	bool appLowCpu = App->isInLowCPUMode();
	if(m_inLowCpuMode != appLowCpu) {
		if(appLowCpu) {
			m_lowCpuModeCount++;
			//if(m_lowCpuModeCount <= 1) {
			//	appLog(LOG_CAT) << QStringLiteral(
			//		"Entering low CPU usage mode for the first time");
			//}
		} else {
			//if(m_lowCpuModeCount <= 1) {
			//	appLog(LOG_CAT) << QStringLiteral(
			//		"Exiting low CPU usage mode for the first time (%L1 frames)"
			//		).arg(m_lowCpuModeFrameCount);
			//}
		}
		setLowCPUUsageMode(appLowCpu);
	}
	if(m_inLowCpuMode)
		m_lowCpuModeFrameCount++;

	//-------------------------------------------------------------------------
	// Prepare our `x264_picture_t`. WARNING: Due to memory sharing the picture
	// initially has partially undefined settings! Make sure that every
	// parameter that gets changed is changed in every possible branch.

	// Get our picture structure
	m_prevPic ^= 1; // Swap pictures
	x264_picture_t *inPic = &m_pics[m_prevPic];

	// Define x264 parameters
	if(m_forceIdr >= 0) {
		m_forceIdr--;
		if(m_forceIdr == -1) {
			// Make this frame a keyframe
			inPic->i_type = X264_TYPE_IDR;
		}
	} else
		inPic->i_type = X264_TYPE_AUTO;
	inPic->i_pts = m_nextPts;
	m_nextPts++;

	// Define our user parameters
	PictureInfo *info = getNextPicInfoFromStack();
	if(info == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to allocated a x264 picture info struct, "
			<< "skipping frame";
		m_prevPic ^= 1; // Swap pictures back as this one is invalid
		return false;
	}
	inPic->opaque = info;
	info->frameNum = frameNum;

	// Copy the frame data into the encoder's memory aligned input buffers.
	// x264's API is currently wasteful as it will do an additional (Optimized)
	// memory copy of this data when we call `x264_encoder_encode`. It needs to
	// do this because of frame delaying but it's still not optimal.
	const QSize uvSize = QSize(m_size.width(), m_size.height() / 2);
	imgDataCopy(
		inPic->img.plane[0], frame.yPlane, inPic->img.i_stride[0],
		frame.yStride, m_size);
	imgDataCopy(
		inPic->img.plane[1], frame.uvPlane, inPic->img.i_stride[1],
		frame.uvStride, uvSize);

	//-------------------------------------------------------------------------

	// Do the encoding
	x264_picture_t outPic;
	x264_nal_t *nals;
	int numNals;
	x264_picture_init(&outPic); // Zero memory
	if(x264_encoder_encode(m_x264, &nals, &numNals, inPic, &outPic) < 0) {
		appLog(LOG_CAT, Log::Warning)
			<< "x264_encoder_encode() failed, skipping frame";

		// Return the picture info structure to the stack ready for reuse
		m_picInfoStack.push(info);

		// Don't swap `m_prevPic` back as the frame data is still valid

		// If this happens multiple times in a row then it's likely not going
		// to succeed, immediately stop encoding
		m_encodeErrorCount++;
		if(m_encodeErrorCount >= 10) {
			emit encodeError(tr(
				"Fatal error while encoding video frame. Please try different video settings."));
		}

		return false;
	}
	m_encodeErrorCount = 0; // Successful encode
	return processNALs(outPic, nals, numNals);
}

bool X264Encoder::processNALs(
	x264_picture_t &pic, x264_nal_t *nals, int numNals)
{
	if(numNals <= 0)
		return true;
	FrmPriority framePriority = FrmLowestPriority;

	// Turn NALs into packets
	EncodedPacketList pkts;
	pkts.reserve(numNals);
	for(int i = 0; i < numNals; i++) {
		x264_nal_t &nal = nals[i];

		// Decode NAL priority
		PktPriority priority;
		switch(nal.i_ref_idc) {
		default:
		case NAL_PRIORITY_DISPOSABLE:
			priority = PktLowestPriority;
			break;
		case NAL_PRIORITY_LOW:
			priority = PktLowPriority;
			break;
		case NAL_PRIORITY_HIGH:
			priority = PktHighPriority;
			break;
		case NAL_PRIORITY_HIGHEST:
			priority = PktHighestPriority;
			break;
		}

		// Decode NAL identity
		PktIdent ident;
		bool isBloat;
		switch(nal.i_type) {
		default:
		case NAL_UNKNOWN:
			ident = PktUnknownIdent;
			isBloat = true;
			break;
		case NAL_SLICE:
			ident = PktH264Ident_SLICE;
			isBloat = false;
			break;
		case NAL_SLICE_DPA:
			ident = PktH264Ident_SLICE_DPA;
			isBloat = true;
			break;
		case NAL_SLICE_DPB:
			ident = PktH264Ident_SLICE_DPB;
			isBloat = true;
			break;
		case NAL_SLICE_DPC:
			ident = PktH264Ident_SLICE_DPC;
			isBloat = true;
			break;
		case NAL_SLICE_IDR:
			ident = PktH264Ident_SLICE_IDR;
			isBloat = false;
			break;
		case NAL_SEI:
			ident = PktH264Ident_SEI;
			isBloat = true;
			break;
		case NAL_SPS:
			ident = PktH264Ident_SPS;
			isBloat = true;
			break;
		case NAL_PPS:
			ident = PktH264Ident_PPS;
			isBloat = true;
			break;
		case NAL_AUD:
			ident = PktH264Ident_AUD;
			isBloat = true;
			break;
		case NAL_FILLER:
			ident = PktH264Ident_FILLER;
			isBloat = true;
			break;
		}

		// Create packet
		EncodedPacket pkt(
			static_cast<VideoEncoder *>(this), PktVideoType,
			priority, ident, isBloat,
			reinterpret_cast<const char *>(nal.p_payload), nal.i_payload);
		pkts.append(pkt);

		// Determine frame dropping priority. We simply just give the entire
		// frame the same priority as the highest priority packet that is in
		// the frame.
		FrmPriority newFrmPriority;
		switch(priority) {
		default:
		case PktLowestPriority:
			newFrmPriority = FrmLowestPriority;
			break;
		case PktLowPriority:
			newFrmPriority = FrmLowPriority;
			break;
		case PktHighPriority:
			newFrmPriority = FrmHighPriority;
			break;
		case PktHighestPriority:
			newFrmPriority = FrmHighestPriority;
			break;
		}
		if(newFrmPriority > framePriority)
			framePriority = newFrmPriority;
	}

	// Create the EncodedFrame
	PictureInfo *info = (PictureInfo *)pic.opaque;
	EncodedFrame frame(
		this, pkts, pic.b_keyframe, framePriority,
		App->frameNumToMsecTimestamp(info->frameNum), pic.i_pts, pic.i_dts);

	// Debugging stuff
	//appLog() << frame.getDebugString();
#if DUMP_STREAM_TO_FILE
	for(int i = 0; i < pkts.size(); i++) {
		const EncodedPacket &pkt = pkts.at(i);
		dumpBuffer.append(pkt.data());
	}
#endif

	// Emit our signal
	emit frameEncoded(frame);

	// Return the picture info structure to the stack ready for reuse
	m_picInfoStack.push(info);

	return true;
}

/// <summary>
/// Duplicate the previous frame `amount` times. WARNING: Unimplemented
/// </summary>
void X264Encoder::dupPrevFrame(int amount)
{
	if(!m_isRunning)
		return;
	if(m_nextPts == 0)
		return; // Haven't encoded a frame yet!
	//appLog() << "**DUPE FRAME " << amount << "**";

	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	//*************************************************************************
	// This method is unimplemented as it has never been required.
	//*************************************************************************
	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

	Q_ASSERT(false);
}

void X264Encoder::nv12FrameReady(
	const NV12Frame &frame, uint frameNum, int numDropped)
{
	if(!m_isRunning)
		return;
	if(numDropped > 0)
		dupPrevFrame(numDropped);
	encodeNV12Frame(frame, frameNum);
}
