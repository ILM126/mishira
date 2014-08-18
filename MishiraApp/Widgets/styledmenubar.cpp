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

#include "styledmenubar.h"
#include "stylehelper.h"
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QStyleOption>

StyledMenuBar::StyledMenuBar(QWidget *parent)
	: QMenuBar(parent)
{
}

StyledMenuBar::~StyledMenuBar()
{
}

void StyledMenuBar::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);

	// Draw background
	p.fillRect(QRect(0, 0, width(), height()), StyleHelper::DarkBg5Color);

	// Draw bottom border
	p.setPen(StyleHelper::DarkBg1Color);
	p.drawLine(rect().bottomLeft(), rect().bottomRight());

	// Draw actions
	QList<QAction *> acts = actions();
	for(int i = 0; i < acts.count(); ++i) {
		// Get clickable rectangle of the action
		QRect actRect = actionGeometry(acts[i]);

		// Only continue if this action is actually visible. TODO: Take into
		// account "hidden" actions which Qt places in a separate drop down
		// menu instead of displaying them on the bar
		if(actRect.isEmpty() || !(ev->rect().intersects(actRect)))
			continue;

		// Use the exact same style options as the native style
		QStyleOptionMenuItem opt;
		initStyleOption(&opt, acts[i]);
		opt.rect = actRect;
		p.setClipRect(actRect);

		//---------------------------------------------------------------------
		// This section is based off QWindowsStyle

		bool active = opt.state & QStyle::State_Selected;
		bool hasFocus = opt.state & QStyle::State_HasFocus;
		bool down = opt.state & QStyle::State_Sunken;
		if(active && hasFocus) {
			// Highlight the active element
			p.fillRect(actRect, StyleHelper::DarkBg7Color);
		}

		//---------------------------------------------------------------------
		// This section is based off QCommonStyle

		uint alignment =
			Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip |
			Qt::TextSingleLine;
		if(!style()->styleHint(QStyle::SH_UnderlineShortcut, &opt, this))
			alignment |= Qt::TextHideMnemonic;
		QPixmap pix = opt.icon.pixmap(
			style()->pixelMetric(QStyle::PM_SmallIconSize),
			(opt.state & QStyle::State_Enabled) ?
			QIcon::Normal : QIcon::Disabled);
		if(!pix.isNull())
			style()->drawItemPixmap(&p, opt.rect, alignment, pix);
		else {
			if(opt.state & QStyle::State_Enabled) {
				// Draw a faint shadow if the action is enabled
				p.setPen(StyleHelper::TextShadowColor);
				style()->drawItemText(
					&p, opt.rect.translated(1, 0), alignment, opt.palette,
					true, opt.text, QPalette::NoRole);
			}
			p.setPen(StyleHelper::DarkFg2Color);
			style()->drawItemText(
				&p, opt.rect.translated(0, -1), alignment, opt.palette,
				opt.state & QStyle::State_Enabled, opt.text,
				QPalette::NoRole); // Use pen instead of "ButtonText" role
		}

		//---------------------------------------------------------------------
	}
}
