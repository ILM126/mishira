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

#include "embosswidget.h"

//=============================================================================
// EmbossWidget class

EmbossWidget::EmbossWidget(int size, bool dark, QWidget *parent)
	: QLabel(parent)
{
	setSizePolicy(
		QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	setAlignment(Qt::AlignCenter);
	if(dark) {
		setPixmap(QPixmap(":/Resources/logo-emboss-dark-60.png"));
	} else {
		if(size == 60)
			setPixmap(QPixmap(":/Resources/logo-emboss-60.png"));
		else
			setPixmap(QPixmap(":/Resources/logo-emboss-100.png"));
	}
}

EmbossWidget::~EmbossWidget()
{
}

//=============================================================================
// EmbossWidgetLarge class

EmbossWidgetLarge::EmbossWidgetLarge(QWidget *parent)
	: EmbossWidget(100, false, parent)
{
}

EmbossWidgetLarge::~EmbossWidgetLarge()
{
}

//=============================================================================
// EmbossWidgetSmall class

EmbossWidgetSmall::EmbossWidgetSmall(QWidget *parent)
	: EmbossWidget(60, false, parent)
{
}

EmbossWidgetSmall::~EmbossWidgetSmall()
{
}

//=============================================================================
// EmbossWidgetSmallDark class

EmbossWidgetSmallDark::EmbossWidgetSmallDark(QWidget *parent)
	: EmbossWidget(60, true, parent)
{
}

EmbossWidgetSmallDark::~EmbossWidgetSmallDark()
{
}
