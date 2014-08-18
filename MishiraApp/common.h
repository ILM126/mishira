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

#ifndef COMMON_H
#define COMMON_H

#include "fraction.h"
#include "log.h"
#include <Libbroadcast/libbroadcast.h>
#include <Libbroadcast/rtmpclient.h>
#include <Libdeskcap/libdeskcap.h>
#include <Libvidgfx/libvidgfx.h>
#include <QtCore/QFileInfo>
#include <QtCore/QRect>
#include <QtGui/QColor>

class Target;
class QSize;
class QLineEdit;

enum LyrAlignment;
enum LyrScalingMode;
enum X264Preset;

//=============================================================================
// Helper functions

QString		getAppBuildVersion();

QString		libavErrorToString(int errnum);
void		imgDataCopy(
	quint8 *dst, quint8 *src, uint dstStride, uint srcStride,
	const QSize &size);
QFileInfo	getUniqueFilename(const QString &filename);
quint64		hashQString64(const QString &str);
quint32		qrand32();
quint64		qrand64();
QString		pointerToString(void *ptr);
QString		numberToHexString(quint64 num);
QString		bytesToHexGrid(const QByteArray &data);
QString		humanBitsBytes(
	quint64 bytes, int numDecimals = 0, bool metric = false);

#ifdef Q_OS_WIN
wchar_t *	QStringToWChar(const QString &str);
#endif

QColor		blendColors(
	const QColor &dst, const QColor &src, float opacity = 1.0f);
bool		doQLineEditValidate(QLineEdit *widget);

QRectF		createScaledRectInBoundsF(
	const QSizeF &size, const QRectF &boundingRect, LyrScalingMode scaling,
	LyrAlignment alignment);
QRect		createScaledRectInBounds(
	const QSize &size, const QRect &boundingRect, LyrScalingMode scaling,
	LyrAlignment alignment);

float		mbToLinear(float mb);
float		linearToMb(float linear);

//=============================================================================
// Enumerations

// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//-----------------------------------------------------------------------------
// Many of these enumerations are used in serialization! This means that they
// need to be modified in a binary-compatible way otherwise user's profiles
// will get corrupted when they upgrade!
//-----------------------------------------------------------------------------
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

//-----------------------------------------------------------------------------
// AudioEncoder

enum AencType {
	AencFdkAacType = 0,

	NUM_AUDIO_ENCODER_TYPES // Must be last
};
static const char * const AencTypeStrings[] = {
	"AAC-LC"
};

//-----------------------------------------------------------------------------
// AudioSource

enum AsrcType {
	AsrcOutputType = 0,
	AsrcInputType,

	NUM_AUDIO_SOURCE_TYPES // Must be last
};

//-----------------------------------------------------------------------------
// EncodedFrame

enum FrmPriority {
	FrmLowestPriority = 0,
	FrmLowPriority,
	FrmHighPriority,
	FrmHighestPriority,

	NUM_FRAME_PRIORITIES
};

//-----------------------------------------------------------------------------
// EncodedPacket

enum PktType {
	PktNullType = 0,
	PktAudioType,
	PktVideoType
};

enum PktPriority {
	PktLowestPriority = 0,
	PktLowPriority,
	PktHighPriority,
	PktHighestPriority
};

enum PktIdent {
	PktUnknownIdent = 0,

	PktH264Ident_SLICE,
	PktH264Ident_SLICE_DPA, // Not used by x264
	PktH264Ident_SLICE_DPB, // Not used by x264
	PktH264Ident_SLICE_DPC, // Not used by x264
	PktH264Ident_SLICE_IDR,
	PktH264Ident_SEI,
	PktH264Ident_SPS,
	PktH264Ident_PPS,
	PktH264Ident_AUD,
	PktH264Ident_FILLER,

	PktFdkAacIdent_FRM
};

//-----------------------------------------------------------------------------
// FdkAacEncoder

/// <summary>
/// The list of available AAC-LC stereo bitrates and recommended sample rate.
/// </summary>
struct FdkAacBitrateDesc {
	int bitrate;
	int sampleRate;
	FdkAacBitrateDesc(int b, int s) : bitrate(b), sampleRate(s) {}
};
static const FdkAacBitrateDesc FdkAacBitrates[] = {
	//FdkAacBitrateDesc(16, 12000), // Supported: 11025, 12000, 16000
	FdkAacBitrateDesc(32, 22050), // Supported: 16000, 22050, 24000
	FdkAacBitrateDesc(64, 32000), // Supported: 32000
	FdkAacBitrateDesc(96, 32000), // Supported: 32000, 44100, 48000
	FdkAacBitrateDesc(128, 44100),
	FdkAacBitrateDesc(160, 44100),
	FdkAacBitrateDesc(192, 44100),
	//FdkAacBitrateDesc(224, 44100),
	FdkAacBitrateDesc(256, 44100),
	//FdkAacBitrateDesc(288, 44100),
	FdkAacBitrateDesc(320, 44100),
	//FdkAacBitrateDesc(352, 48000), // Supported: 48000
	//FdkAacBitrateDesc(384, 48000),
	//FdkAacBitrateDesc(416, 48000),
	//FdkAacBitrateDesc(448, 48000),
	//FdkAacBitrateDesc(480, 48000),
	//FdkAacBitrateDesc(512, 48000),
	//FdkAacBitrateDesc(544, 48000),
	//FdkAacBitrateDesc(576, 48000),

	FdkAacBitrateDesc(0, 0) // Must be last
};
static const char * const FdkAacBitratesStrings[] = {
	//"16 Kb/s",
	"32 Kb/s",
	"64 Kb/s",
	"96 Kb/s",
	"128 Kb/s (Recommended)",
	"160 Kb/s",
	"192 Kb/s",
	//"224 Kb/s",
	"256 Kb/s",
	//"288 Kb/s",
	"320 Kb/s",
	//"352 Kb/s",
	//"384 Kb/s",
	//"416 Kb/s",
	//"448 Kb/s",
	//"480 Kb/s",
	//"512 Kb/s",
	//"544 Kb/s",
	//"576 Kb/s",
};

struct FdkAacOptions {
	int	bitrate;
};

//-----------------------------------------------------------------------------
// FileTarget

enum FileTrgtType {
	FileTrgtMp4Type = 0,
	FileTrgtMkvType,
	//FileTrgtFlvType,

	NUM_FILE_TARGET_TYPES // Must be last
};
static const char * const FileTrgtTypeStrings[] = {
	".mp4 (MPEG-4 Part 14)",
	".mkv (Matroska)",
	//".flv (Flash video)"
};
static const char * const FileTrgtTypeExtStrings[] = {
	"mp4",
	"mkv",
	//"flv"
};
static const char * const FileTrgtTypeFilterStrings[] = {
	"MPEG-4 Part 14 (*.mp4)",
	"Matroska (*.mkv)",
	//"Flash video (*.flv)"
};

//-----------------------------------------------------------------------------
// Profile

// WARNING: Framerates must be in ascending order!
static const Fraction DEFAULT_VIDEO_FRAMERATE = Fraction(30, 1);
static const Fraction PrflFpsRates[] = {
	Fraction(15, 1),
	Fraction(20, 1),
	Fraction(24000, 1001),
	Fraction(24, 1),
	Fraction(25, 1),
	Fraction(30000, 1001),
	Fraction(30, 1),
	Fraction(50, 1),
	Fraction(60000, 1001),
	Fraction(60, 1),

	Fraction(0, 0) // Must be last (Terminator)
};
static const char * const PrflFpsRatesStrings[] = {
	"15 Hz",
	"20 Hz",
	"23.976 Hz",
	"24 Hz",
	"25 Hz",
	"29.97 Hz",
	"30 Hz (Recommended)",
	"50 Hz",
	"59.94 Hz",
	"60 Hz"
};

enum PrflAudioMode {
	Prfl441StereoMode = 0, // 44.1 kHz stereo
	Prfl48StereoMode, // 48 kHz stereo

	NUM_AUDIO_MODES // Must be last
};
static const char * const PrflAudioModeStrings[] = {
	"44.1 kHz stereo (Recommended)",
	"48 kHz stereo"
};

enum PrflTransition {
	PrflInstantTransition = 0, // Ignores duration setting
	PrflFadeTransition,
	PrflFadeBlackTransition, // Fades through black
	PrflFadeWhiteTransition // Fades through white
};
static const char * const PrflTransitionStrings[] = {
	"Instant",
	"Fade",
	"Fade through black",
	"Fade through white"
};
static const char * const PrflTransitionShortStrings[] = {
	"Instant",
	"Fade",
	"F/black",
	"F/white"
};

//-----------------------------------------------------------------------------
// Scaler

enum SclrScalingMode {
	SclrStretchScale = 0,
	SclrSnapToInnerScale,
	SclrSnapToOuterScale,

	NUM_SCALER_SCALING_MODES // Must be last
};
static const char * const SclrScalingModeStrings[] = {
	"Stretch to fit",
	"Show black bars if required",
	"Crop if required"
};

//-----------------------------------------------------------------------------
// Layer

enum LyrType {
	LyrColorLayerType = 0,
	LyrImageLayerType,
	LyrWindowLayerType,
	LyrTextLayerType,
	LyrSyncLayerType,
	LyrWebcamLayerType,
	LyrSlideshowLayerType,
	LyrMonitorLayerType,
	LyrScriptTextLayerType
};

enum LyrScalingMode {
	LyrActualScale = 0,
	LyrStretchScale,
	LyrSnapToInnerScale,
	LyrSnapToOuterScale
};

enum LyrAlignment {
	LyrTopLeftAlign = 0,
	LyrTopCenterAlign,
	LyrTopRightAlign,
	LyrMiddleLeftAlign,
	LyrMiddleCenterAlign,
	LyrMiddleRightAlign,
	LyrBottomLeftAlign,
	LyrBottomCenterAlign,
	LyrBottomRightAlign
};

//-----------------------------------------------------------------------------
// Target

enum TrgtType {
	TrgtFileType = 0,
	TrgtRtmpType = 1,
	TrgtTwitchType = 2,
	//TrgtJustinType, // Removed
	TrgtUstreamType = 4,
	TrgtHitboxType = 5
};

struct FileTrgtOptions {
	quint32			videoEncId;
	quint32			audioEncId;
	QString			filename;
	FileTrgtType	fileType;
};

struct RTMPTrgtOptions {
	quint32	videoEncId;
	quint32	audioEncId;
	QString	url;
	QString	streamName; // "Stream name or key"
	bool	hideStreamName;
	bool	padVideo;
};

struct TwitchTrgtOptions {
	quint32	videoEncId;
	quint32	audioEncId;
	QString	streamKey;
	QString	url;
	QString username;
};

typedef TwitchTrgtOptions HitboxTrgtOptions;

struct UstreamTrgtOptions {
	quint32	videoEncId;
	quint32	audioEncId;
	QString	url;
	QString	streamKey;
	bool	padVideo;
};

//-----------------------------------------------------------------------------
// VideoEncoder

enum VencType {
	VencX264Type = 0,

	NUM_VIDEO_ENCODER_TYPES // Must be last
};
static const char * const VencTypeStrings[] = {
	"Software H.264" // x264
};

//-----------------------------------------------------------------------------
// WizardController

enum WizButton {
	WizBackButton = (1 << 0),
	WizNextButton = (1 << 1),
	WizFinishButton = (1 << 2),
	WizOKButton = (1 << 3),
	WizCancelButton = (1 << 4),
	WizResetButton = (1 << 5)
};
typedef int WizButtonsMask;
const WizButtonsMask WizStandardButtons =
	WizBackButton | WizNextButton | WizCancelButton | WizResetButton;
const WizButtonsMask WizStandardFirstButtons =
	WizNextButton | WizCancelButton | WizResetButton;
const WizButtonsMask WizStandardLastButtons =
	WizBackButton | WizFinishButton | WizCancelButton | WizResetButton;

enum WizController {
	WizNoController = 0,
	WizNewProfileController,
	WizEditProfileController,
	WizEditTargetsController,
	WizNewEditTargetController,
	WizAudioMixerController,

	WIZ_NUM_CONTROLLERS // Must be last
};

enum WizPage {
	WizNoPage = 0,
	WizWelcomePage,
	WizNewProfilePage,
	WizUploadPage,
	WizPrepareBenchmarkPage,
	WizBenchmarkPage,
	WizBenchmarkResultsPage,
	WizSceneTemplatePage,
	WizProfileCompletePage,
	WizEditProfilePage,
	WizTargetListPage,
	WizTargetTypePage,
	WizVideoSettingsPage,
	WizAudioSettingsPage,
	WizAudioMixerPage,
	WizFileTargetSettingsPage,
	WizRTMPTargetSettingsPage,
	WizTwitchTargetSettingsPage,
	WizUstreamTargetSettingsPage,
	WizHitboxTargetSettingsPage,

	WIZ_NUM_PAGES // Must be last
};

/// <summary>
/// Target settings as used by `NewEditTargetController` and partially by
/// `NewProfileController`
/// </summary>
struct WizTargetSettings {
	TrgtType		type;
	QString			name;

	// Video settings
	Target *		vidShareWith;
	int				vidWidth;
	int				vidHeight;
	SclrScalingMode	vidScaling;
	GfxFilter		vidScaleQuality;
	VencType		vidEncoder;
	int				vidBitrate;
	X264Preset		vidPreset;
	int				vidKeyInterval;

	// Audio settings
	bool			audHasAudio;
	Target *		audShareWith;
	AencType		audEncoder;
	int				audBitrate;

	// File target settings
	QString			fileFilename;
	FileTrgtType	fileType;

	// Other RTMP target settings
	QString			rtmpUrl;
	QString			rtmpStreamName;
	bool			rtmpHideStreamName;
	bool			rtmpPadVideo;

	// Twitch/Hitbox target settings
	QString			username;

	// Constructor (Needed for QStrings)
	inline WizTargetSettings()
		: type((TrgtType)0)
		, name()
		, vidShareWith(NULL)
		, vidWidth(0)
		, vidHeight(0)
		, vidScaling((SclrScalingMode)0)
		, vidScaleQuality((GfxFilter)0)
		, vidEncoder((VencType)0)
		, vidBitrate(0)
		, vidPreset((X264Preset)0)
		, vidKeyInterval(0)
		, audHasAudio(false)
		, audShareWith(NULL)
		, audEncoder((AencType)0)
		, audBitrate(0)

		// These can be set to more sane defaults by using the
		// `setTargetSettingsToDefault()` method
		, fileFilename()
		, fileType((FileTrgtType)0)
		, rtmpUrl()
		, rtmpStreamName()
		, rtmpHideStreamName(false)
		, rtmpPadVideo(false)
		, username()
	{}

	void setTargetSettingsToDefault();
};
struct WizShared {
	// `NewProfileController` shared values
	bool				profIsWelcome; // true = First time setup

	// `NewEditTargetController` shared values
	Target *			netCurTarget; // NULL if creating a new target
	WizTargetSettings	netDefaults;
	bool				netReturnToEditTargets;

	inline WizShared()
		: profIsWelcome(false)
		, netCurTarget(NULL)
		, netDefaults()
		, netReturnToEditTargets(false)
	{}
};

//-----------------------------------------------------------------------------
// X264Encoder

// Preset speeds are the rounded average of the following benchmark results:
// http://blogs.motokado.com/yoshi/2011/06/25/comparison-of-x264-presets/
// http://doom10.org/index.php?topic=1656.0
enum X264Preset {
	X264UltraFastPreset = 0,
	X264SuperFastPreset,
	X264VeryFastPreset,
	X264FasterPreset,
	X264FastPreset,
	X264MediumPreset,
	X264SlowPreset,
	X264SlowerPreset,
	X264VerySlowPreset,
	//X264PlaceboPreset, // Crashes on Macbook Pro and Tim Oliver's PC

	NUM_X264_PRESETS // Must be last
};
static const char * const X264PresetStrings[] = {
	"Ultra fast (Approx. 0.4x)", // 0.43x
	"Super fast (Approx. 0.5x)", // 0.50x
	"Very fast (Approx. 0.6x)", // 0.64x
	"Faster (1.0x)",
	"Fast (Approx. 1.4x)", // 1.38x
	"Medium (Approx. 1.6x)", // 1.58x
	"Slow (Approx. 2.3x)", // 2.29x
	"Slower (Approx. 6.0x)", // 5.83x
	"Very slow (Approx. 9.0x)"//, // 8.78x
	//"Super slow (Approx. 21.0x)" // 20.59x
};
static const char * const X264PresetQualityStrings[] = {
	"Very low quality",
	"Low quality",
	"Good quality",
	"Great quality",
	"High quality",
	"Very high quality",
	"Super high quality",
	"Extreme quality",
	// If "placebo" is disabled then calling "very slow" "near best" will be
	// confusing to users. Instead call "very slow" "best". If "placebo" is
	// ever enabled again then we'll rename "very slow" to "near best" again.
	//"Near best quality",
	"Best quality"
};

const int DEFAULT_KEYFRAME_INTERVAL = 2;
struct X264Options {
	X264Preset	preset;
	int			bitrate;

	// 0 = DEFAULT_KEYFRAME_INTERVAL
	// We only convert the "0" to the default when initializing the encoder so
	// that if the user leaves the value in the UI as "default" it will
	// continue to display "default" when the UI is reloaded instead of having
	// it replaced by the actual default.
	int			keyInterval;
};

//=============================================================================

#endif // COMMON_H
