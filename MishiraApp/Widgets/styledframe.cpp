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

#include "styledframe.h"
#include "stylehelper.h"
#include <QtGui/QPainter>

//=============================================================================
// StyledFrame class

StyledFrame::StyledFrame(QWidget *parent)
	: QFrame(parent)
{
	//setContentsMargins(10, 10, 10, 10);
}

StyledFrame::~StyledFrame()
{
}

void StyledFrame::paintEvent(QPaintEvent *ev)
{
	// Draw background frame
	QPainter p(this);
	StyleHelper::drawFrame(
		&p, rect(), StyleHelper::FrameNormalColor,
		StyleHelper::FrameNormalBorderColor, true);
}

//=============================================================================
// StyledInfoFrame class

StyledInfoFrame::StyledInfoFrame(QWidget *parent)
	: StyledFrame(parent)
{
}

StyledInfoFrame::~StyledInfoFrame()
{
}

void StyledInfoFrame::paintEvent(QPaintEvent *ev)
{
	// Draw background frame
	QPainter p(this);
	StyleHelper::drawFrame(
		&p, rect(), StyleHelper::FrameInfoColor,
		StyleHelper::FrameInfoBorderColor, true);
}

//=============================================================================
// StyledWarningFrame class

StyledWarningFrame::StyledWarningFrame(QWidget *parent)
	: StyledFrame(parent)
{
}

StyledWarningFrame::~StyledWarningFrame()
{
}

void StyledWarningFrame::paintEvent(QPaintEvent *ev)
{
	// Draw background frame
	QPainter p(this);
	StyleHelper::drawFrame(
		&p, rect(), StyleHelper::FrameWarningColor,
		StyleHelper::FrameWarningBorderColor, true);
}

//=============================================================================
// StyledErrorFrame class

StyledErrorFrame::StyledErrorFrame(QWidget *parent)
	: StyledFrame(parent)
{
}

StyledErrorFrame::~StyledErrorFrame()
{
}

void StyledErrorFrame::paintEvent(QPaintEvent *ev)
{
	// Draw background frame
	QPainter p(this);
	StyleHelper::drawFrame(
		&p, rect(), StyleHelper::FrameErrorColor,
		StyleHelper::FrameErrorBorderColor, true);
}
