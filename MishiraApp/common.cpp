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

#include "common.h"
#include "application.h"
#include "constants.h"
#include "stylehelper.h"
#include <QtCore/QSize>
#include <QtCore/QStandardPaths>
#include <QtGui/QPalette>
#include <QtWidgets/QLineEdit>
extern "C" {
#include <libavformat/avformat.h>
}
#ifdef Q_OS_WIN
#include <windows.h>
#endif

//=============================================================================
// Validate library versions

#if LIBDESKCAP_VER_MAJOR != 0 || \
	LIBDESKCAP_VER_MINOR != 5 || \
	LIBDESKCAP_VER_BUILD != 1
#error Mismatched Libdeskcap version!
#endif

#if LIBBROADCAST_VER_MAJOR != 0 || \
	LIBBROADCAST_VER_MINOR != 5 || \
	LIBBROADCAST_VER_BUILD != 0
#error Mismatched Libbroadcast version!
#endif

#if LIBVIDGFX_VER_MAJOR != 0 || \
	LIBVIDGFX_VER_MINOR != 5 || \
	LIBVIDGFX_VER_BUILD != 0
#error Mismatched Libvidgfx version!
#endif

//=============================================================================
// Helper functions

QString getAppBuildVersion()
{
	// In order to get the Git revision for our "build" version number our
	// build script sets an environment variable that is then used as a macro
	// in our Visual Studio project file to set the following definition that
	// we then modify at runtime.
	QString str(GIT_REV);
	if(!str.isEmpty())
		str = str.left(7);
	else
		str = QStringLiteral("unknown");
#ifdef QT_DEBUG
	return QStringLiteral("%1-debug").arg(str);
#else
	return str;
#endif
}

/// <summary>
/// Needed as the built-in `av_err2str()` macro doesn't work in MSVC.
/// </summary>
QString libavErrorToString(int errnum)
{
	char str[AV_ERROR_MAX_STRING_SIZE];
	av_strerror(errnum, str, sizeof(str));
	return QString::fromLatin1(str);
}

/// <summary>
/// An optimized memory copy designed for 2D image transfer that takes into
/// account the row strides of the input and output image buffers. `size` is
/// the width in bytes (NumPixels * BytesPerPixel) but the height in rows.
/// </summary>
void imgDataCopy(
	quint8 *dst, quint8 *src, uint dstStride, uint srcStride,
	const QSize &size)
{
	if(size.width() < 0 || size.height() < 0)
		return; // Width and height must be positive!
	if(dstStride == srcStride) {
		// The input and output buffers are exactly the same size so we can get
		// away with a single memory copy operation
		memcpy(dst, src, dstStride * size.height());
		return;
	}
	// Copy each row separately
	for(int i = 0; i < size.height(); i++) {
		memcpy(dst, src, size.width());
		dst += dstStride;
		src += srcStride;
	}
}

/// <summary>
/// Returns a filename that is guarenteed to be unique based on the specified
/// one. It does this by appending " (x)" to the filename where "x" is a
/// number.
/// </summary>
QFileInfo getUniqueFilename(const QString &filename)
{
	QFileInfo info(filename);
	info.makeAbsolute();
	QString baseName = info.path() + "/" + info.baseName();
	int i = 1;
	while(info.exists()) {
		info.setFile(QStringLiteral("%1 (%2).%3")
			.arg(baseName)
			.arg(i++)
			.arg(info.completeSuffix()));
	}
	return info;
}

/// <summary>
/// Produces a 64-bit hash of the input string that is consistent across
/// different bit widths and Qt versions.
/// </summary>
quint64 hashQString64(const QString &str)
{
	// Based on Qt's string qHash() function which is in-turn based on Java's.
	// See "qtbase/src/corelib/tools/qhash.cpp" for more information. The
	// hashing algorithm includes a salt to prevent denial-of-service attacks
	// that attempt to fill a single bucket of a hash table to cause a large
	// performance decrease.
	const QChar *p = str.unicode();
	quint64 h = 0x54F530E9FDD15445; // Random seed
	for(int i = 0; i < str.size(); i++)
		h = 31ULL * h + (quint64)(p[i].unicode());
	return h;
}

/// <summary>
/// Generates a random 32-bit unsigned integer using `qrand()`. Use this
/// whenever possible as `qrand()` only returns numbers between 0 and RAND_MAX
/// (0x7FFF).
/// </summary>
quint32 qrand32()
{
	quint32 ret;
	for(int i = 0; i < sizeof(ret); i++)
		ret |= (quint32)((uint)qrand() & 0xFF) << (i * 8);
	return ret;
}

/// <summary>
/// Generates a random 64-bit unsigned integers using `qrand()`. Use this
/// whenever possible as `qrand()` only returns numbers between 0 and RAND_MAX
/// (0x7FFF).
/// </summary>
quint64 qrand64()
{
	quint64 ret;
	for(int i = 0; i < sizeof(ret); i++)
		ret |= (quint64)((uint)qrand() & 0xFF) << (i * 8);
	return ret;
}

QString pointerToString(void *ptr)
{
#if QT_POINTER_SIZE == 4
	return QStringLiteral("0x") + QString::number((quint32)ptr, 16).toUpper();
#elif QT_POINTER_SIZE == 8
	return QStringLiteral("0x") + QString::number((quint64)ptr, 16).toUpper();
#else
#error Unknown pointer size
#endif
}

QString numberToHexString(quint64 num)
{
	return QStringLiteral("0x") + QString::number(num, 16).toUpper();
}

/// <summary>
/// Outputs the input byte array as a string of hexadecimal digits in a grid of
/// 16 columns with an extra gap at the 8th column. I.e. the following
/// format: "XX XX XX XX XX XX XX XX  XX XX XX XX XX XX XX XX"
/// </summary>
QString bytesToHexGrid(const QByteArray &data)
{
	QString str;
	for(int i = 0; i < data.size(); i += 16) {
		if(i != 0)
			str += "\n";
		for(int j = 0; j < 16 && i + j < data.size(); j++) {
			quint8 val = data[i+j];
			if(j == 8)
				str += "  ";
			else if(j != 0)
				str += " ";
			if(val < 16)
				str += "0";
			str += QString::number(val, 16).toUpper();
		}
	}
	return str;
}

/// <summary>
/// Returns the number of bytes adding SI units such as "K" or "M" using orders
/// of 1024 of `metric` is false or 1000 if it's true. It is up to the caller
/// to append "B", "b", "B/s" or "b/s" to the end of the returned string.
/// </summary>
QString humanBitsBytes(quint64 bytes, int numDecimals, bool metric)
{
	// Don't use floating points due to rounding
	QString units;
	quint64 mag = 1024ULL;
	if(metric)
		mag = 1000ULL;
	quint64 order = 1ULL;
	if(bytes < mag) {
		numDecimals = 0;
		units = QStringLiteral(" ");
		goto humanBytesReturn;
	}
	order *= mag;
	if(bytes < mag * mag) {
		numDecimals = qMin(3, numDecimals); // Not really true due to 1024
		units = QStringLiteral(" K");
		goto humanBytesReturn;
	}
	order *= mag;
	if(bytes < mag * mag * mag) {
		numDecimals = qMin(6, numDecimals); // Not really true due to 1024
		units = QStringLiteral(" M");
		goto humanBytesReturn;
	}
	order *= mag;
	numDecimals = qMin(9, numDecimals); // Not really true due to 1024
	units = QStringLiteral(" G");

humanBytesReturn:
	qreal flt = (qreal)bytes / (qreal)order; // Result as a float
	if(numDecimals <= 0)
		return QStringLiteral("%L1%2").arg(qRound(flt)).arg(units);
	quint64 pow = 1ULL;
	for(int i = 0; i < numDecimals; i++)
		pow *= 10ULL;
	quint64 dec = qRound(flt * (qreal)pow);
	quint64 whole = dec / pow; // Whole part
	dec %= pow; // Decimal part as an integer
	return QStringLiteral("%L1.%2%3").arg(whole)
		.arg(dec, numDecimals, 10, QChar('0')).arg(units);
}

#ifdef Q_OS_WIN
/// <summary>
/// Converts a `QString` into a `wchar_t` array. We cannot use
/// `QString::toStdWString()` as it's not enabled in our Qt build. The caller
/// is responsible for calling `delete[]` on the returned string.
/// </summary>
wchar_t *QStringToWChar(const QString &str)
{
	// We must keep the QByteArray in memory as otherwise its `data()` gets
	// freed and we end up converting undefined data
	QByteArray data = str.toUtf8();
	const char *msg = data.data();
	int len = MultiByteToWideChar(CP_UTF8, 0, msg, -1, NULL, 0);
	wchar_t *wstr = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, msg, -1, wstr, len);
	return wstr;
}
#endif

/// <summary>
/// Mixes the two specified colours.
/// </summary>
QColor blendColors(const QColor &dst, const QColor &src, float opacity)
{
	double colOp = src.alphaF() * (double)opacity;
	return QColor::fromRgbF(
		qBound(0.0, dst.redF() * (1.0 - colOp) + src.redF() * colOp, 1.0),
		qBound(0.0, dst.greenF() * (1.0 - colOp) + src.greenF() * colOp, 1.0),
		qBound(0.0, dst.blueF() * (1.0 - colOp) + src.blueF() * colOp, 1.0),
		qBound(0.0, dst.alphaF() + colOp, 1.0));
}

/// <summary>
/// Tests to see if the line edit has a valid input and if it doesn't visually
/// mark the widget so the user knows.
/// </summary>
bool doQLineEditValidate(QLineEdit *widget)
{
	bool valid = widget->hasAcceptableInput();
	QPalette pal = widget->palette();
	if(App->isDarkStyle(widget)) {
		QColor invalidCol = blendColors(
			StyleHelper::DarkBg3Color, StyleHelper::RedBaseColor, 0.33333f);
		pal.setColor(
			QPalette::Normal, widget->backgroundRole(),
			valid ? StyleHelper::DarkBg3Color : invalidCol);
	} else {
		// TODO: We assume the original colour was black on white
		//pal.setColor(
		//	QPalette::Normal, widget->foregroundRole(),
		//	valid ? Qt::black : QColor(96, 0, 0));
		pal.setColor(
			QPalette::Normal, widget->backgroundRole(),
			valid ? Qt::white : StyleHelper::FrameErrorColor.light(125));

		// HACK: Hover transitions on Windows 8 causes Qt to not use the above
		// palette making the widget temporarily revert to default. Simply
		// disable the transition when the input is invalid until we find a
		// better solution.
		if(valid && widget->property("aniSet").toBool()) {
			widget->setProperty("_q_no_animation", false);
			widget->setProperty("aniSet", false);
		} else if(!valid) {
			widget->setProperty("_q_no_animation", true);
			widget->setProperty("aniSet", true);
		}
	}
	widget->setPalette(pal);
	return valid;
}

/// <summary>
/// Creates a rectangle of size `size` inside of `boundingRect` based on the
/// specified rules of scaling and alignment.
/// </summary>
QRectF createScaledRectInBoundsF(
	const QSizeF &size, const QRectF &boundingRect, LyrScalingMode scaling,
	LyrAlignment alignment)
{
	QRectF rect = boundingRect;

	// Calculate display size
	QSizeF newSize;
	switch(scaling) {
	default:
	case LyrActualScale:
		newSize = size; // No change
		break;
	case LyrSnapToInnerScale: {
		qreal sAspect = size.width() / size.height();
		qreal rAspect = rect.width() / rect.height();
		if(sAspect > rAspect) {
			// Will touch the left and right
			newSize.setWidth(rect.width());
			newSize.setHeight(rect.width() / sAspect);
		} else {
			// Will touch the top and bottom
			newSize.setWidth(rect.height() * sAspect);
			newSize.setHeight(rect.height());
		}
		break; }
	case LyrSnapToOuterScale: {
		qreal sAspect = size.width() / size.height();
		qreal rAspect = rect.width() / rect.height();
		if(sAspect < rAspect) {
			// Will touch the top and bottom
			newSize.setWidth(rect.width());
			newSize.setHeight(rect.width() / sAspect);
		} else {
			// Will touch the left and right
			newSize.setWidth(rect.height() * sAspect);
			newSize.setHeight(rect.height());
		}
		break; }
	}

	// Calculate rectangle from alignment
	switch(scaling) {
	case LyrSnapToInnerScale:
	case LyrSnapToOuterScale:
	case LyrActualScale: {
		// Horizontal
		switch(alignment) {
		case LyrTopLeftAlign:
		case LyrMiddleLeftAlign:
		case LyrBottomLeftAlign:
			rect.setWidth(newSize.width());
			break;
		case LyrTopCenterAlign:
		case LyrMiddleCenterAlign:
		case LyrBottomCenterAlign:
			rect.setRight(rect.center().x() + newSize.width() * 0.5f);
			// Fall through
		case LyrTopRightAlign:
		case LyrMiddleRightAlign:
		case LyrBottomRightAlign:
			rect.setLeft(rect.right() - newSize.width());
			break;
		}

		// Vertical
		switch(alignment) {
		case LyrTopLeftAlign:
		case LyrTopCenterAlign:
		case LyrTopRightAlign:
			rect.setHeight(newSize.height());
			break;
		case LyrMiddleLeftAlign:
		case LyrMiddleCenterAlign:
		case LyrMiddleRightAlign:
			rect.setBottom(rect.center().y() + newSize.height() * 0.5f);
			// Fall through
		case LyrBottomLeftAlign:
		case LyrBottomCenterAlign:
		case LyrBottomRightAlign:
			rect.setTop(rect.bottom() - newSize.height());
			break;
		}

		break; }
	default:
	case LyrStretchScale:
		break; // Return the rectangle unmodified
	}

	// Nullify floating point maths error
	if(qFuzzyCompare(rect.left(), boundingRect.left()))
		rect.setLeft(boundingRect.left());
	if(qFuzzyCompare(rect.right(), boundingRect.right()))
		rect.setRight(boundingRect.right());
	if(qFuzzyCompare(rect.top(), boundingRect.top()))
		rect.setTop(boundingRect.top());
	if(qFuzzyCompare(rect.bottom(), boundingRect.bottom()))
		rect.setBottom(boundingRect.bottom());

	return rect;
}

/// <summary>
/// Creates a rectangle of size `size` inside of `boundingRect` based on the
/// specified rules of scaling and alignment.
/// </summary>
QRect createScaledRectInBounds(
	const QSize &size, const QRect &boundingRect, LyrScalingMode scaling,
	LyrAlignment alignment)
{
	QRectF rect = createScaledRectInBoundsF(
		size, boundingRect, scaling, alignment);
	return rect.toRect();
}

float mbToLinear(float mb)
{
	return pow(10.0f, mb / 2000.0f);
}

float linearToMb(float linear)
{
	float mb = 2000.0f * log10(linear);
	if(!_finite(mb))
		return MIN_MB_F;
	return mb;
}

//-----------------------------------------------------------------------------
// WizTargetSettings struct

/// <summary>
/// Resets all values relating to targets to sane defaults.
/// </summary>
void WizTargetSettings::setTargetSettingsToDefault()
{
	// Default filename for local files
	fileType = FileTrgtMp4Type;
	QDir defaultDir(
		QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
	fileFilename = QStringLiteral("%1.%2")
		.arg(name)
		.arg(FileTrgtTypeExtStrings[fileType]);
	fileFilename = QDir::toNativeSeparators(
		defaultDir.absoluteFilePath(fileFilename));

	// Default other RTMP settings
	rtmpUrl = QStringLiteral("rtmp://");
	rtmpStreamName = QString();
	rtmpHideStreamName = false;
	rtmpPadVideo = false;

	// Default Twitch/Hitbox settings
	username = QString();
}
