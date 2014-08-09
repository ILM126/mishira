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

#include "styledbutton.h"
#include <QtGui/QPainter>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyleOption>

const int CONTENTS_MARGIN_X = 10;
const int CONTENTS_MARGIN_Y = 5;

StyledButton::StyledButton(
	const StyleHelper::ColorSet &colorSet, bool lightText, QWidget *parent)
	: QAbstractButton(parent)
	, m_colorSet(colorSet)
	, m_textColor()
	, m_textShadowColor()
	, m_hovered(false)
	, m_normalIcon()
	, m_hoverIcon()
	, m_joinLeft(false)
	, m_joinRight(false)
{
	// Set default contents margins
	setContentsMargins(
		CONTENTS_MARGIN_X, CONTENTS_MARGIN_Y,
		CONTENTS_MARGIN_X, CONTENTS_MARGIN_Y);

	// Determine text colour
	updateLightText(lightText);
}

StyledButton::~StyledButton()
{
}

QSize StyledButton::sizeHint() const
{
	// Based off QPushButton
	// TODO: Cache result

	int w = 0, h = 0;
	ensurePolished();

	// Initialize button style options
	QStyleOptionButton opt;
	initStyleOption(&opt);

	// Add icon size
	if(!icon().isNull()) {
		int ih = opt.iconSize.height();
		int iw = opt.iconSize.width() + 4;
		w += iw;
		h = qMax(h, ih);
	}

	// Add text size
	QString s(text());
	bool empty = s.isEmpty();
	if(empty)
		s = QString::fromLatin1("XXXX");
	QFontMetrics fm = fontMetrics();
	QSize sz = fm.size(Qt::TextShowMnemonic, s);
	if(!empty || !w)
		w += sz.width();
	if(!empty || !h)
		h = qMax(h, sz.height());

	// Add margins and return
	QMargins margins = contentsMargins();
	QSize sizeHint = QSize(
		w + margins.left() + margins.right(),
		h + margins.top() + margins.bottom());
	return sizeHint;
}

void StyledButton::setColorSet(const StyleHelper::ColorSet &colorSet)
{
	m_colorSet = colorSet;
	repaint();
}

void StyledButton::setLightText(bool lightText)
{
	updateLightText(lightText);
	repaint();
}

void StyledButton::setIcon(const QIcon &icon)
{
	m_normalIcon = icon;
	if(!m_hovered)
		QAbstractButton::setIcon(icon);
}

void StyledButton::setHoverIcon(const QIcon &icon)
{
	m_hoverIcon = icon;
	if(m_hovered)
		QAbstractButton::setIcon(icon);
}

void StyledButton::setJoin(bool left, bool right)
{
	m_joinLeft = left;
	m_joinRight = right;
}

void StyledButton::updateLightText(bool lightText)
{
	if(lightText) {
		m_textColor = QColor(0xFF, 0xFF, 0xFF);
		m_textShadowColor = QColor(0x00, 0x00, 0x00, 0x54);
	} else {
		m_textColor = QColor(0x00, 0x00, 0x00);
		m_textShadowColor = QColor(0xFF, 0xFF, 0xFF, 0x54);
	}
}

void StyledButton::initStyleOption(QStyleOptionButton *option) const
{
	option->initFrom(this);
	option->features = QStyleOptionButton::None;
	if(isDown())
		option->state |= QStyle::State_Sunken;
	else
		option->state |= QStyle::State_Raised;
	option->text = text();
	option->icon = icon();
	option->iconSize = iconSize();
}

void StyledButton::enterEvent(QEvent *ev)
{
	m_hovered = true;
	if(!m_hoverIcon.isNull())
		QAbstractButton::setIcon(m_hoverIcon);
}

void StyledButton::leaveEvent(QEvent *ev)
{
	m_hovered = false;
	if(!m_hoverIcon.isNull()) // Test against the hover icon, not normal icon
		QAbstractButton::setIcon(m_normalIcon);
}

void StyledButton::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);

	// Initialize button style options
	QStyleOptionButton opt;
	initStyleOption(&opt);

	// Determine colour set to paint with
	QColor topCol(*(m_colorSet.topColor));
	QColor bottomCol(*(m_colorSet.bottomColor));
	QColor highlightCol(*(m_colorSet.highlightColor));
	QColor shadowCol(*(m_colorSet.shadowColor));
	StyleHelper::ColorSet set(&topCol, &bottomCol, &highlightCol, &shadowCol);
	if(m_hovered) {
		topCol = topCol.darker(BUTTON_HOVER_DARKEN_AMOUNT);
		bottomCol = bottomCol.darker(BUTTON_HOVER_DARKEN_AMOUNT);
		highlightCol = highlightCol.darker(BUTTON_HOVER_DARKEN_AMOUNT);
		shadowCol = shadowCol.darker(BUTTON_HOVER_DARKEN_AMOUNT);
	}
	opt.palette.setColor(QPalette::ButtonText, m_textColor);

	// Draw background
	StyleHelper::drawBackground(
		&p, QRect(0, 0, width(), height()), set,
		opt.state & QStyle::State_Sunken, 0.0f, m_joinLeft, m_joinRight);

	//-------------------------------------------------------------------------
	// This section duplicated in `DarkStyle` (CE_PushButtonLabel)
	// The only difference is that we hard-code the pressed button offset

	// Get contents rectangle
	QMargins margins = contentsMargins();
	QStyleOptionButton subopt = opt;
	subopt.rect = opt.rect.adjusted(
		margins.left(), margins.top(),
		-margins.right(), -margins.bottom());

	QRect textRect = subopt.rect;
	uint tf = Qt::AlignVCenter | Qt::TextShowMnemonic;
	if(!style()->styleHint(QStyle::SH_UnderlineShortcut, &subopt, this))
		tf |= Qt::TextHideMnemonic;

	// Draw the icon if one exists. We need to modify the text rectangle as
	// well as we want both the icon and the text centered on the button
	QRect iconRect;
	if(!subopt.icon.isNull()) {
		// Determine icon state
		QIcon::Mode mode = (subopt.state & QStyle::State_Enabled) ?
			QIcon::Normal : QIcon::Disabled;
		if(mode == QIcon::Normal && subopt.state & QStyle::State_HasFocus)
			mode = QIcon::Active;
		QIcon::State state = QIcon::Off;
		if(subopt.state & QStyle::State_On)
			state = QIcon::On;

		// Determine metrics
		QPixmap pixmap = subopt.icon.pixmap(subopt.iconSize, mode, state);
		int labelWidth = pixmap.width();
		int labelHeight = pixmap.height();
		int iconSpacing = 4; // See `sizeHint()`
		int textWidth = subopt.fontMetrics.boundingRect(
			subopt.rect, tf, subopt.text).width();
		if(!subopt.text.isEmpty())
			labelWidth += textWidth + iconSpacing;

		// Determine icon rectangle
		iconRect = QRect(textRect.x() + (textRect.width() - labelWidth) / 2,
			textRect.y() + (textRect.height() - labelHeight) / 2,
			pixmap.width(), pixmap.height());
		iconRect = QStyle::visualRect(subopt.direction, textRect, iconRect);

		// Change where the text will be displayed
		tf |= Qt::AlignLeft; // Left align, we adjust the text rect instead
		if(subopt.direction == Qt::RightToLeft)
			textRect.setRight(iconRect.left() - iconSpacing);
		else
			textRect.setLeft(iconRect.left() + iconRect.width() + iconSpacing);

		// Translate the contents slightly when the button is pressed
		if(subopt.state & QStyle::State_Sunken)
			iconRect.translate(1, 1);

		// Draw pixmap
		p.drawPixmap(iconRect, pixmap);
	} else
		tf |= Qt::AlignHCenter;

	// HACK: Move the text up 1px so that it is vertically centered
	textRect.translate(0, -1);

	// Translate the contents slightly when the button is pressed
	if(subopt.state & QStyle::State_Sunken)
		textRect.translate(1, 1);

	// Draw text shadow only if the button is enabled
	if(subopt.state & QStyle::State_Enabled) {
		QPalette pal(subopt.palette);
		pal.setColor(QPalette::ButtonText, m_textShadowColor);
		style()->drawItemText(
			&p, textRect.translated(2, 1).adjusted(-1, -1, 1, 1), tf, pal,
			true, subopt.text, QPalette::ButtonText);
	}

	// Draw text. HACK: We offset the text by one pixel to the right as the
	// font metrics include the character spacing to the right of the last
	// character making the text appear slightly to the left. We also increase
	// the rectangle by 1px in every direction in order for the text to not get
	// clipped with some fonts.
	style()->drawItemText(
		&p, textRect.translated(1, 0).adjusted(-1, -1, 1, 1), tf,
		subopt.palette, (subopt.state & QStyle::State_Enabled), subopt.text,
		QPalette::ButtonText);

	//-------------------------------------------------------------------------
	// This section is based off QCommonStyle (CE_PushButton)
	// We change the way the focus rectangle is calculated as the original
	// algorithm has some bugs

	// Draw focus rectangle
	if(hasFocus()) {
		int pad =
			style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this) + 1;

		QStyleOptionFocusRect fropt;
		fropt.initFrom(this);
		fropt.QStyleOption::operator=(opt);

		// Determine bounding rectangle
		if(!subopt.icon.isNull()) {
			QRegion region(subopt.fontMetrics.boundingRect(
				textRect, tf, subopt.text));
			region += iconRect;
			fropt.rect = region.boundingRect();
			fropt.rect = fropt.rect.adjusted(-pad, -pad, pad, pad);
		} else if(!subopt.text.isEmpty()) {
			fropt.rect = subopt.fontMetrics.boundingRect(
				textRect, tf, subopt.text);
			fropt.rect = fropt.rect.adjusted(-pad, -pad, pad, pad);
		} else {
			fropt.rect = subopt.rect;

			// Translate the contents slightly when the button is pressed
			if(subopt.state & QStyle::State_Sunken)
				fropt.rect.translate(1, 1);
		}

		style()->drawPrimitive(QStyle::PE_FrameFocusRect, &fropt, &p, this);
		//p.drawRect(fropt.rect);
	}

	//-------------------------------------------------------------------------
}
