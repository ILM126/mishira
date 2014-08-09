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

#include "styledsortablebar.h"

StyledSortableBar::StyledSortableBar(
	const QColor &fgColor, const QColor &bgColor, const QColor &shadowColor,
	int level, QWidget *parent)
	: StyledBar(fgColor, bgColor, shadowColor, parent)
	, SortableWidget(level)
{
	construct();
}

StyledSortableBar::StyledSortableBar(
	const QColor &fgColor, const QColor &bgColor, const QColor &shadowColor,
	const QString &label, int level, QWidget *parent)
	: StyledBar(fgColor, bgColor, shadowColor, label, parent)
	, SortableWidget(level)
{
	construct();
}

void StyledSortableBar::construct()
{
}

StyledSortableBar::~StyledSortableBar()
{
}

QWidget *StyledSortableBar::getWidget()
{
	return static_cast<QWidget *>(this);
}
