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

#ifndef STYLEHELPER_H
#define STYLEHELPER_H

#include <QtCore/QPoint>
#include <QtGui/QColor>

class QPainter;
class QPainterPath;
class QRect;

/// <summary>
/// The amount that a button's background colour is darkened when the user
/// hovers over it with the mouse cursor.
/// </summary>
const int BUTTON_HOVER_DARKEN_AMOUNT = 110;

/// <summary>
/// The amount that a line edit's background colour is darkened when it has
/// keyboard focus.
/// </summary>
const int LINE_EDIT_FOCUS_DARKEN_AMOUNT = 125;

/// <summary>
/// The amount that a checkbox or radio button's background colour is darkened
/// when the user hovers over it with the mouse cursor.
/// </summary>
const int CHECKBOX_HOVER_DARKEN_AMOUNT = 115;
const int SCROLLBAR_HOVER_DARKEN_AMOUNT = 115;

/// <summary>
/// The amount that a checkbox or radio button's background colour is darkened
/// when it is pressed.
/// </summary>
const int CHECKBOX_PRESSED_DARKEN_AMOUNT = 125;
const int SCROLLBAR_PRESSED_DARKEN_AMOUNT = 125;

//=============================================================================
class StyleHelper
{
public: // Datatypes ----------------------------------------------------------
	struct ColorSet {
		QColor *	topColor;
		QColor *	bottomColor;
		QColor *	highlightColor;
		QColor *	shadowColor;

		ColorSet(
			QColor *topCol, QColor *bottomCol, QColor *highlightCol,
			QColor *shadowCol)
			: topColor(topCol)
			, bottomColor(bottomCol)
			, highlightColor(highlightCol)
			, shadowColor(shadowCol)
		{
		}
	};

public: // Static members -----------------------------------------------------

	//-------------------------------------------
	// Light UI

	static QColor	WizardHeaderColor;

	static QColor	FrameNormalColor;
	static QColor	FrameNormalBorderColor;
	static QColor	FrameSelectedColor;
	static QColor	FrameSelectedBorderColor;
	static QColor	FrameInfoColor;
	static QColor	FrameInfoBorderColor;
	static QColor	FrameWarningColor;
	static QColor	FrameWarningBorderColor;
	static QColor	FrameErrorColor;
	static QColor	FrameErrorBorderColor;

	//-------------------------------------------
	// Dark UI

	static QColor	DarkBg1Color; // 13% brightness
	static QColor	DarkBg2Color; // 18%
	static QColor	DarkBg3Color; // 23%
	static QColor	DarkBg4Color; // 28%
	static QColor	DarkBg5Color; // 33%
	static QColor	DarkBg6Color; // 38%
	static QColor	DarkBg7Color; // 43%

	static QColor	DarkFg1Color; // 90% brightness
	static QColor	DarkFg2Color; // 80% (Regular text)
	static QColor	DarkFg3Color; // 70%

	static QColor	DarkBg3SelectedColor;
	static QColor	DarkBg5SelectedColor;

	static QColor	EmbossHighlightColor;
	static QColor	EmbossShadowColor;
	static QColor	TextHighlightColor;
	static QColor	TextShadowColor;

	static QColor	RedBaseColor; // Flat base
	static QColor	RedLighterColor; // Gradient top
	static QColor	RedDarkerColor; // Gradient bottom
	static ColorSet	RedSet;

	static QColor	OrangeBaseColor;
	static QColor	OrangeLighterColor;
	static QColor	OrangeDarkerColor;
	static ColorSet	OrangeSet;

	static QColor	YellowBaseColor;
	static QColor	YellowLighterColor;
	static QColor	YellowDarkerColor;
	static ColorSet	YellowSet;

	static QColor	GreenBaseColor;
	static QColor	GreenLighterColor;
	static QColor	GreenDarkerColor;
	static ColorSet	GreenSet;

	static QColor	BlueBaseColor;
	static QColor	BlueLighterColor;
	static QColor	BlueDarkerColor;
	static ColorSet	BlueSet;

	static QColor	PurpleBaseColor;
	static QColor	PurpleLighterColor;
	static QColor	PurpleDarkerColor;
	static ColorSet	PurpleSet;

	static QColor	ButtonLighterColor;
	static QColor	ButtonDarkerColor;
	static ColorSet	ButtonSet;

	static QColor	TransparentColor;
	static ColorSet	TransparentSet;

public: // Static methods -----------------------------------------------------
	static void	colorToFloats(const QColor &color, float *output);
	static ColorSet getColorSetFromBase(
		const QColor &base, bool &isButtonHoverOut);

	static QPainterPath createRoundedRect(const QRect &rect, float radius);
	static QPainterPath createRoundedRectStroke(
		const QRect &rect, float radius);

	static void	drawPathedGradient(
		QPainter *p, const QPainterPath &path, QPoint aPos, QPoint bPos,
		QColor aCol, QColor bCol, bool antialias);
	static void	drawPathedGradient(
		QPainter *p, const QPainterPath &path, int aX, int aY, int bX, int bY,
		QColor aCol, QColor bCol, bool antialias);
	static void	drawGradient(
		QPainter *p, const QRect &rect, QPoint aPos, QPoint bPos, QColor aCol,
		QColor bCol);
	static void	drawGradient(
		QPainter *p, const QRect &rect, int aX, int aY, int bX, int bY,
		QColor aCol, QColor bCol);

	static void	drawEmboss(
		QPainter *p, const QRect &rect, QColor highlightCol, QColor shadowCol,
		bool pressed = false, bool joinLeft = false, bool joinRight = false);
	static void	drawPathedEmboss(
		QPainter *p, const QPainterPath &path, QColor highlightCol,
		QColor shadowCol, bool pressed = false);

	static void	drawBackground(
		QPainter *p, const QRect &rect, QColor topCol, QColor bottomCol,
		QColor highlightCol, QColor shadowCol, bool pressed = false,
		float radius = 0.0f, bool joinLeft = false, bool joinRight = false);
	static void	drawBackground(
		QPainter *p, const QRect &rect, ColorSet &set, bool pressed = false,
		float radius = 0.0f, bool joinLeft = false, bool joinRight = false);

	static void	drawFrame(
		QPainter *p, const QRect &rect, QColor backCol, QColor borderCol,
		bool hasShadow = true, float radius = 5.0f);

	// Dark UI
	static void	drawDarkPanel(
		QPainter *p, const QPalette &pal, const QRect &rect,
		bool ellipse = false, float radius = 2.0f);
	static void	drawDarkButton(
		QPainter *p, const QPalette &pal, const QRect &rect, bool pressed);
};
//=============================================================================

/// <summary>
/// Converts a `QColor` into a `float[4]` array with each value in the range
/// [0..1].
/// </summary>
inline void	StyleHelper::colorToFloats(const QColor &color, float *output)
{
	output[0] = color.redF();
	output[1] = color.greenF();
	output[2] = color.blueF();
	output[3] = color.alphaF();
}

inline void StyleHelper::drawPathedGradient(
	QPainter *p, const QPainterPath &path, int aX, int aY, int bX, int bY,
	QColor aCol, QColor bCol, bool antialias)
{
	drawPathedGradient(
		p, path, QPoint(aX, aY), QPoint(bX, bY),
		aCol, bCol, antialias);
}

inline void StyleHelper::drawGradient(
	QPainter *p, const QRect &rect, int aX, int aY, int bX, int bY,
	QColor aCol, QColor bCol)
{
	drawGradient(
		p, rect, QPoint(aX, aY), QPoint(bX, bY),
		aCol, bCol);
}

inline void StyleHelper::drawBackground(
	QPainter *p, const QRect &rect, ColorSet &set, bool pressed, float radius,
	bool joinLeft, bool joinRight)
{
	drawBackground(
		p, rect, *(set.topColor),
		*(set.bottomColor), *(set.highlightColor),
		*(set.shadowColor), pressed, radius, joinLeft, joinRight);
}

#endif // STYLEHELPER_H
