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

#include "stylehelper.h"
#include "common.h"
#include <QtGui/QBrush>
#include <QtGui/QPainter>
#include <QtGui/QPalette>

// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//*****************************************************************************
// Be very careful when assigning these colours based on mock images. Some
// image editors such as Photoshop use colourspace-corrected colours which
// display differently when used in a linear colourspace (Which is what Qt
// uses). In order to get the linear RGB values from mocks that were made in
// Photoshop you need to take a screenshot of the image in Photoshop and open
// up that screenshot in another program that uses linear colourspace such as
// GIMP.
//*****************************************************************************
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

//-----------------------------------------------------------------------------
// Light UI

QColor StyleHelper::WizardHeaderColor(0x3D, 0x3D, 0x40);

QColor StyleHelper::FrameNormalColor(0xE9, 0xE9, 0xE9);
//QColor StyleHelper::FrameNormalColor(0xE6, 0xE7, 0xEB); // Tinted blue
QColor StyleHelper::FrameNormalBorderColor(0xD4, 0xD4, 0xD4);
//QColor StyleHelper::FrameNormalBorderColor(0xD2, 0xD3, 0xD6); // Tinted blue
QColor StyleHelper::FrameSelectedColor(0xE0, 0xE5, 0xEC);
QColor StyleHelper::FrameSelectedBorderColor(0xB5, 0xCB, 0xEC);
QColor StyleHelper::FrameInfoColor(0xC4, 0xDA, 0xF3);
QColor StyleHelper::FrameInfoBorderColor(0xB5, 0xCB, 0xEC);
QColor StyleHelper::FrameWarningColor(0xF1, 0xE8, 0x96);
QColor StyleHelper::FrameWarningBorderColor(0xDF, 0xD5, 0x84);
QColor StyleHelper::FrameErrorColor(0xF6, 0xAE, 0xAF);
QColor StyleHelper::FrameErrorBorderColor(0xE1, 0x9B, 0x9B);

//-----------------------------------------------------------------------------
// Dark UI

QColor StyleHelper::DarkBg1Color(32, 32, 33);
QColor StyleHelper::DarkBg2Color(44, 44, 46);
QColor StyleHelper::DarkBg3Color(56, 56, 59);
QColor StyleHelper::DarkBg4Color(69, 69, 71);
QColor StyleHelper::DarkBg5Color(81, 81, 84);
QColor StyleHelper::DarkBg6Color(93, 93, 97);
QColor StyleHelper::DarkBg7Color(105, 105, 110);

QColor StyleHelper::DarkFg1Color(225, 225, 230);
QColor StyleHelper::DarkFg2Color(200, 200, 204);
QColor StyleHelper::DarkFg3Color(175, 175, 179);

QColor StyleHelper::DarkBg3SelectedColor(61, 88, 116);
QColor StyleHelper::DarkBg5SelectedColor(73, 100, 128);

QColor StyleHelper::EmbossHighlightColor(255, 255, 255, 38); // 15% opacity
QColor StyleHelper::EmbossShadowColor(0, 0, 0, 102); // 40% opacity
QColor StyleHelper::TextHighlightColor(255, 255, 255, 85); // 33% opacity
QColor StyleHelper::TextShadowColor(0, 0, 0, 85); // 33% opacity

QColor StyleHelper::RedBaseColor(191, 67, 67);
QColor StyleHelper::RedLighterColor(200, 77, 77);
QColor StyleHelper::RedDarkerColor(182, 57, 57);
StyleHelper::ColorSet StyleHelper::RedSet(
	&(StyleHelper::RedLighterColor),
	&(StyleHelper::RedDarkerColor),
	&(StyleHelper::EmbossHighlightColor),
	&(StyleHelper::EmbossShadowColor));

QColor StyleHelper::OrangeBaseColor(217, 136, 22);
QColor StyleHelper::OrangeLighterColor(223, 154, 25);
QColor StyleHelper::OrangeDarkerColor(211, 118, 19);
StyleHelper::ColorSet StyleHelper::OrangeSet(
	&(StyleHelper::OrangeLighterColor),
	&(StyleHelper::OrangeDarkerColor),
	&(StyleHelper::EmbossHighlightColor),
	&(StyleHelper::EmbossShadowColor));

QColor StyleHelper::YellowBaseColor(243, 220, 48);
QColor StyleHelper::YellowLighterColor(245, 225, 55);
QColor StyleHelper::YellowDarkerColor(241, 215, 41);
StyleHelper::ColorSet StyleHelper::YellowSet(
	&(StyleHelper::YellowLighterColor),
	&(StyleHelper::YellowDarkerColor),
	&(StyleHelper::EmbossHighlightColor),
	&(StyleHelper::EmbossShadowColor));

QColor StyleHelper::GreenBaseColor(62, 179, 62);
QColor StyleHelper::GreenLighterColor(71, 190, 71);
QColor StyleHelper::GreenDarkerColor(53, 168, 53);
StyleHelper::ColorSet StyleHelper::GreenSet(
	&(StyleHelper::GreenLighterColor),
	&(StyleHelper::GreenDarkerColor),
	&(StyleHelper::EmbossHighlightColor),
	&(StyleHelper::EmbossShadowColor));

QColor StyleHelper::BlueBaseColor(65, 119, 172);
QColor StyleHelper::BlueLighterColor(75, 137, 184);
QColor StyleHelper::BlueDarkerColor(55, 101, 160);
StyleHelper::ColorSet StyleHelper::BlueSet(
	&(StyleHelper::BlueLighterColor),
	&(StyleHelper::BlueDarkerColor),
	&(StyleHelper::EmbossHighlightColor),
	&(StyleHelper::EmbossShadowColor));

QColor StyleHelper::PurpleBaseColor(146, 80, 179);
QColor StyleHelper::PurpleLighterColor(162, 92, 190);
QColor StyleHelper::PurpleDarkerColor(130, 68, 168);
StyleHelper::ColorSet StyleHelper::PurpleSet(
	&(StyleHelper::PurpleLighterColor),
	&(StyleHelper::PurpleDarkerColor),
	&(StyleHelper::EmbossHighlightColor),
	&(StyleHelper::EmbossShadowColor));

QColor StyleHelper::ButtonLighterColor(121, 121, 126);
QColor StyleHelper::ButtonDarkerColor(90, 90, 94);
StyleHelper::ColorSet StyleHelper::ButtonSet(
	&(StyleHelper::ButtonLighterColor),
	&(StyleHelper::ButtonDarkerColor),
	&(StyleHelper::EmbossHighlightColor),
	&(StyleHelper::EmbossShadowColor));

QColor StyleHelper::TransparentColor(0x00, 0x00, 0x00, 0x00);
StyleHelper::ColorSet StyleHelper::TransparentSet(
	&(StyleHelper::TransparentColor),
	&(StyleHelper::TransparentColor),
	&(StyleHelper::TransparentColor),
	&(StyleHelper::TransparentColor));

// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//*****************************************************************************
// Read the warning message at the top of the file.
//*****************************************************************************
// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

/// <summary>
/// If `isButtonHoverOut` is true then the returned colour should be darkened
/// by `BUTTON_HOVER_DARKEN_AMOUNT`.
/// </summary>
StyleHelper::ColorSet StyleHelper::getColorSetFromBase(
	const QColor &base, bool &isButtonHoverOut)
{
	isButtonHoverOut = false;
	if(base == RedBaseColor)
		return RedSet;
	if(base == OrangeBaseColor)
		return OrangeSet;
	if(base == YellowBaseColor)
		return YellowSet;
	if(base == GreenBaseColor)
		return GreenSet;
	if(base == BlueBaseColor)
		return BlueSet;
	if(base == PurpleBaseColor)
		return PurpleSet;
	if(base == DarkBg7Color)
		return ButtonSet;
	if(base == TransparentColor)
		return TransparentSet;

	// Test for button hovering. HACK: This is ugly
	if(base == RedBaseColor.darker(BUTTON_HOVER_DARKEN_AMOUNT)) {
		isButtonHoverOut = true;
		return RedSet;
	}
	if(base == OrangeBaseColor.darker(BUTTON_HOVER_DARKEN_AMOUNT)) {
		isButtonHoverOut = true;
		return OrangeSet;
	}
	if(base == YellowBaseColor.darker(BUTTON_HOVER_DARKEN_AMOUNT)) {
		isButtonHoverOut = true;
		return YellowSet;
	}
	if(base == GreenBaseColor.darker(BUTTON_HOVER_DARKEN_AMOUNT)) {
		isButtonHoverOut = true;
		return GreenSet;
	}
	if(base == BlueBaseColor.darker(BUTTON_HOVER_DARKEN_AMOUNT)) {
		isButtonHoverOut = true;
		return BlueSet;
	}
	if(base == PurpleBaseColor.darker(BUTTON_HOVER_DARKEN_AMOUNT)) {
		isButtonHoverOut = true;
		return PurpleSet;
	}
	if(base == DarkBg7Color.darker(BUTTON_HOVER_DARKEN_AMOUNT)) {
		isButtonHoverOut = true;
		return ButtonSet;
	}

	return RedSet; // Unknown colour
};

/// <summary>
/// Interpolate between the two specified colours. Note that this is different
/// to `blendColors` as it also interpolates the alpha channel.
/// </summary>
QColor lerpColors(const QColor &a, const QColor &b, float t)
{
	double colOp = (double)t;
	return QColor::fromRgbF(
		qBound(0.0, a.redF() * (1.0 - colOp) + b.redF() * colOp, 1.0),
		qBound(0.0, a.greenF() * (1.0 - colOp) + b.greenF() * colOp, 1.0),
		qBound(0.0, a.blueF() * (1.0 - colOp) + b.blueF() * colOp, 1.0),
		qBound(0.0, a.alphaF() * (1.0 - colOp) + b.alphaF() * colOp, 1.0));
}

void StyleHelper::drawPathedGradient(
	QPainter *p, const QPainterPath &path, QPoint aPos, QPoint bPos,
	QColor aCol, QColor bCol, bool antialias)
{
	if(aCol.alpha() == 0 && bCol.alpha() == 0)
		return; // Completely transparent

	// Enable or disable antialiasing on the painter
	bool prevHint = p->testRenderHint(QPainter::Antialiasing);
	p->setRenderHint(QPainter::Antialiasing, antialias);

#if 1
	// Create gradient brush
	QLinearGradient gradient;
	gradient.setStart((float)aPos.x(), (float)aPos.y());
	gradient.setFinalStop((float)bPos.x(), (float)bPos.y());
#define SHARP_GRADIENT 1
#if SHARP_GRADIENT
	gradient.setColorAt(0.0f, aCol);
	gradient.setColorAt(0.49f, lerpColors(aCol, bCol, 0.4f));
	gradient.setColorAt(0.51f, lerpColors(aCol, bCol, 0.6f));
	gradient.setColorAt(1.0f, bCol);
#else
	gradient.setColorAt(0.0f, aCol);
	gradient.setColorAt(1.0f, bCol);
#endif // SHARP_GRADIENT
	QBrush gradientBrush(gradient);

	// Draw gradient
	p->fillPath(path, gradientBrush);
#else
	// Render as a solid colour for performance testing purposes
	p->fillPath(path, aCol);
#endif

	// Restore previous painter hints
	p->setRenderHint(QPainter::Antialiasing, prevHint);
}

void StyleHelper::drawGradient(
	QPainter *p, const QRect &rect, QPoint aPos, QPoint bPos, QColor aCol,
	QColor bCol)
{
	QPainterPath path;
	path.addRect(rect);
	drawPathedGradient(
		p, path, aPos, bPos, aCol, bCol, false);
}

void StyleHelper::drawEmboss(
	QPainter *p, const QRect &rect, QColor highlightCol, QColor shadowCol,
	bool pressed, bool joinLeft, bool joinRight)
{
	if(highlightCol.alpha() == 0 && shadowCol.alpha() == 0)
		return; // Completely transparent

	// Disable antialiasing on the painter
	bool prevHint = p->testRenderHint(QPainter::Antialiasing);
	p->setRenderHint(QPainter::Antialiasing, false);

	// Draw shadow without painting the bottom-right pixel twice
	p->setPen(pressed ? highlightCol : shadowCol);
	if(joinRight)
		p->drawLine(rect.bottomLeft(), rect.bottomRight());
	else {
		p->drawLine(rect.bottomLeft(), rect.bottomRight() + QPoint(-1, 0));
		p->drawLine(rect.topRight(), rect.bottomRight());
	}

	// Draw highlight without painting the top-left pixel twice
	p->setPen(pressed ? shadowCol : highlightCol);
	if(joinLeft)
		p->drawLine(rect.topLeft(), rect.topRight());
	else {
		p->drawLine(rect.topLeft() + QPoint(1, 0), rect.topRight());
		p->drawLine(rect.topLeft(), rect.bottomLeft());
	}

	// Draw "joiner" if requested. The joiner is a small line that separates
	// two parts of a visually larger button. E.g. A custom combobox widget
	// that behaves differently if you click the arrow instead of the text.
	const QPoint joinDist(0, 4);
	if(joinLeft) {
		p->setPen(highlightCol);
		p->drawLine(rect.topLeft() + joinDist, rect.bottomLeft() - joinDist);
	}
	if(joinRight) {
		p->setPen(shadowCol);
		p->drawLine(rect.topRight() + joinDist, rect.bottomRight() - joinDist);
	}

	// Restore previous painter hints
	p->setRenderHint(QPainter::Antialiasing, prevHint);
}

void createLightClippingAreas(
	const QPainterPath &path, QPolygon &topHalfOut, QPolygon &bottomHalfOut)
{
	// Calculate clipping areas based on the Qt example:
	// http://qt-project.org/doc/qt-5.0/qtwidgets/widgets-styles.html
	QRect rect = path.boundingRect().toAlignedRect();
	int halfSize = qMin(rect.width() / 2, rect.height() / 2);
	topHalfOut = QPolygon();
	topHalfOut.append(
		QPoint(rect.x(), rect.y()));
	topHalfOut.append(
		QPoint(rect.x() + rect.width(), rect.y()));
	topHalfOut.append(
		QPoint(rect.x() + rect.width() - halfSize, rect.y() + halfSize));
	topHalfOut.append(
		QPoint(rect.x() + halfSize, rect.y() + rect.height() - halfSize));
	topHalfOut.append(
		QPoint(rect.x(), rect.y() + rect.height()));
	bottomHalfOut = topHalfOut;
	bottomHalfOut[0] =
		QPoint(rect.x() + rect.width(), rect.y() + rect.height());
}

void StyleHelper::drawPathedEmboss(
	QPainter *p, const QPainterPath &path, QColor highlightCol,
	QColor shadowCol, bool pressed)
{
	if(highlightCol.alpha() == 0 && shadowCol.alpha() == 0)
		return; // Completely transparent

	// Enable antialiasing on the painter
	bool prevHint = p->testRenderHint(QPainter::Antialiasing);
	p->setRenderHint(QPainter::Antialiasing, true);

	// Calculate clipping areas for emulating light
	QPolygon topHalf;
	QPolygon bottomHalf;
	createLightClippingAreas(path, topHalf, bottomHalf);

	// Draw highlight path
	p->setClipPath(path);
	p->setClipRegion(topHalf, Qt::IntersectClip);
	p->setPen(Qt::NoPen);
	p->setBrush(pressed ? shadowCol : highlightCol);
	p->drawPath(path);

	// Draw shadow path
	p->setClipPath(path);
	p->setClipRegion(bottomHalf, Qt::IntersectClip);
	p->setBrush(pressed ? highlightCol : shadowCol);
	p->drawPath(path);

	// Restore previous painter hints
	p->setClipping(false);
	p->setRenderHint(QPainter::Antialiasing, prevHint);
}

QPainterPath StyleHelper::createRoundedRect(const QRect &rect, float radius)
{
	// Create the rounded rectangle if we have a radius. Based on the Qt
	// example: http://qt-project.org/doc/qt-5.0/qtwidgets/widgets-styles.html
	QPainterPath path;
	if(radius > 0.0f) {
		float diam = 2.0f * radius;
		int x1, y1, x2, y2;
		rect.getCoords(&x1, &y1, &x2, &y2);

		// Compensate for 0.5 pixel offset
		x2++;
		y2++;

		path.moveTo(x2, y1 + radius);
		path.arcTo(QRectF(x2 - diam, y1, diam, diam), 0.0f, 90.0f);
		path.lineTo(x1 + radius, y1);
		path.arcTo(QRectF(x1, y1, diam, diam), 90.0f, 90.0f);
		path.lineTo(x1, y2 - radius);
		path.arcTo(QRectF(x1, y2 - diam, diam, diam), 180.0f, 90.0f);
		path.lineTo(x1 + radius, y2);
		path.arcTo(QRectF(x2 - diam, y2 - diam, diam, diam), 270.0f, 90.0f);
		path.closeSubpath();
	} else
		path.addRect(rect);
	return path;
}

/// <summary>
/// Creates a 1px pixel-aligned stroke for the specified rounded rectangle. The
/// stroke extends from the edge of the shape inwards. The resulting path
/// should be rendered with a QBrush only (No QPen).
///
/// This is needed as Qt's QPen-based stroke rendering is not pixel exact.
/// </summary>
QPainterPath StyleHelper::createRoundedRectStroke(
	const QRect &rect, float radius)
{
	QPainterPath path = createRoundedRect(rect.adjusted(0, 0, -1, -1), radius);
	QPainterPathStroker stroker;
	path = stroker.createStroke(path);
	path.translate(0.5, 0.5);
	return path;
}

/// <summary>
/// The same as `createRoundedRectStroke()` but for ellipses.
/// </summary>
QPainterPath createEllipseStroke(const QRect &boundRect)
{
	// HACK: As Qt renders ellipses slightly different to Photoshop we fudge
	// the offsets slightly in order to create a similar output. This means the
	// output is no longer perfectly pixel aligned.
	QPainterPath path;
	path.addEllipse(QRectF(boundRect).adjusted(0.0, 0.0, -0.95, -0.95));
	QPainterPathStroker stroker;
	path = stroker.createStroke(path);
	path.translate(0.475, 0.475);
	return path;
}

void StyleHelper::drawBackground(
	QPainter *p, const QRect &rect, QColor topCol, QColor bottomCol,
	QColor highlightCol, QColor shadowCol, bool pressed, float radius,
	bool joinLeft, bool joinRight)
{
	// Draw background gradient
	QPainterPath path = createRoundedRect(rect, radius);
	drawPathedGradient(
		p, path, rect.left(), rect.top(), rect.left(), rect.bottom(),
		topCol, bottomCol, radius > 0.0f);

	// Draw border emboss
	if(radius > 0.0f) {
		// TODO: Ability to join rounded buttons (Joined edges are straight)
		path = createRoundedRectStroke(rect, radius);
		drawPathedEmboss(p, path, highlightCol, shadowCol, pressed);
	} else {
		drawEmboss(
			p, rect, highlightCol, shadowCol, pressed, joinLeft, joinRight);
	}
}

void StyleHelper::drawFrame(
	QPainter *p, const QRect &rect, QColor backCol, QColor borderCol,
	bool hasShadow, float radius)
{
	// The rectangle that we use is actually 1px smaller than the input on both
	// axis if we have a "shadow".
	QRect newRect = hasShadow ? rect.adjusted(0, 0, -1, -1) : rect;
	if(newRect.width() <= 0 || newRect.height() <= 0)
		return; // Invisible rectangle

	// Create rectangle paths
	QPainterPath bgPath = createRoundedRect(newRect, radius);
	QPainterPath path = createRoundedRectStroke(newRect, radius);
	QPainterPath adjPath = path.translated(1.0f, 1.0f);

	// Enable or disable antialiasing based on the radius
	bool prevHint = p->testRenderHint(QPainter::Antialiasing);
	p->setRenderHint(QPainter::Antialiasing, radius > 0.0f);

	// Draw the frame. If `hasShadow` is not set then we don't want to render
	// the "shadow" (Which is actually a highlight) to the bottom and right of
	// the frame. As the inner shadow and outer shadow is drawn at the same
	// time we need to use a clip region to hide the bottom-right part.
	p->fillPath(bgPath, backCol); // Background
	p->setClipping(false);
	if(!hasShadow)
		p->setClipPath(bgPath);
	p->setPen(Qt::NoPen);
	p->setBrush(QColor(255, 255, 255, 84)); // 33% transparent white
	p->drawPath(adjPath); // "Shadow" (Actually a highlight)
	if(!hasShadow)
		p->setClipping(false);
	p->setBrush(borderCol);
	p->drawPath(path); // Border stroke

	// Restore previous painter hints
	p->setRenderHint(QPainter::Antialiasing, prevHint);
}

//-----------------------------------------------------------------------------
// Dark UI

void StyleHelper::drawDarkPanel(
	QPainter *p, const QPalette &pal, const QRect &rect, bool ellipse,
	float radius)
{
	// Create paths
	QPainterPath outerPath;
	QPainterPath innerPath;
	QPainterPath innerBgPath;
	if(ellipse) {
		outerPath = createEllipseStroke(rect);
		innerPath = createEllipseStroke(rect.adjusted(1, 1, -1, -1));
		innerBgPath.addEllipse(rect.adjusted(1, 1, -1, -1));
	} else {
		outerPath = createRoundedRectStroke(rect, radius);
		innerPath =
			createRoundedRectStroke(rect.adjusted(1, 1, -1, -1), radius);
		innerBgPath = createRoundedRect(rect.adjusted(1, 1, -1, -1), radius);
	}

	// Enable antialiasing
	bool prevHint = p->testRenderHint(QPainter::Antialiasing);
	p->setRenderHint(QPainter::Antialiasing, true);

	// Calculate clipping areas for emulating light
	QPolygon topHalf;
	QPolygon bottomHalf;
	createLightClippingAreas(outerPath, topHalf, bottomHalf);

	// Draw inner background
	p->setPen(Qt::NoPen);
	p->setBrush(pal.color(QPalette::Base));
	p->drawPath(innerBgPath);

	// Draw inner stroke
	p->setBrush(pal.color(QPalette::Base).darker(112));
	p->drawPath(innerPath);

	// Draw top-left outer stroke
	p->setClipPath(outerPath);
	p->setClipRegion(topHalf, Qt::IntersectClip);
	p->setBrush(QColor(255, 255, 255, 13)); // 5%
	p->drawPath(outerPath);

	// Draw bottom-right outer stroke
	p->setClipPath(outerPath);
	p->setClipRegion(bottomHalf, Qt::IntersectClip);
	p->setBrush(QColor(255, 255, 255, 38)); // 15%
	p->drawPath(outerPath);

	// Restore previous painter hints
	p->setClipping(false);
	p->setRenderHint(QPainter::Antialiasing, prevHint);
}

void StyleHelper::drawDarkButton(
	QPainter *p, const QPalette &pal, const QRect &rect, bool pressed)
{
	// Create rectangle paths
	QPainterPath outerPath = createRoundedRectStroke(rect, 2.0f);

	// Enable antialiasing
	bool prevHint = p->testRenderHint(QPainter::Antialiasing);
	p->setRenderHint(QPainter::Antialiasing, true);

	// Calculate clipping areas for emulating light
	QPolygon topHalf;
	QPolygon bottomHalf;
	createLightClippingAreas(outerPath, topHalf, bottomHalf);

	// Draw top-left outer stroke
	p->setClipPath(outerPath);
	p->setClipRegion(topHalf, Qt::IntersectClip);
	p->setPen(Qt::NoPen);
	p->setBrush(QColor(0, 0, 0, 64)); // 25%
	p->drawPath(outerPath);

	// Draw bottom-right outer stroke
	p->setClipPath(outerPath);
	p->setClipRegion(bottomHalf, Qt::IntersectClip);
	p->setBrush(QColor(255, 255, 255, 26)); // 10%
	p->drawPath(outerPath);

	// Draw the actual button
	p->setClipping(false);
	bool isButtonHover = false;
	QColor topCol, botCol; // A ColorSet is pointers to QColors
	ColorSet set =
		getColorSetFromBase(pal.color(QPalette::Button), isButtonHover);
	if(isButtonHover) {
		topCol = set.topColor->darker(BUTTON_HOVER_DARKEN_AMOUNT);
		botCol = set.bottomColor->darker(BUTTON_HOVER_DARKEN_AMOUNT);
		set = ColorSet(&topCol, &botCol, set.highlightColor, set.shadowColor);
	}
	drawBackground(p, rect.adjusted(1, 1, -1, -1), set, pressed, 2.0f);

	// Restore previous painter hints
	p->setRenderHint(QPainter::Antialiasing, prevHint);
}
