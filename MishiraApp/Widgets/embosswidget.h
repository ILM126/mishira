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

#ifndef EMBOSSWIDGET_H
#define EMBOSSWIDGET_H

#include <QtWidgets/QLabel>

//=============================================================================
class EmbossWidget : public QLabel
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	EmbossWidget(int size = 100, bool dark = false, QWidget *parent = 0);
	~EmbossWidget();
};
//=============================================================================

//=============================================================================
class EmbossWidgetLarge : public EmbossWidget
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	EmbossWidgetLarge(QWidget *parent = 0);
	~EmbossWidgetLarge();
};
//=============================================================================

//=============================================================================
class EmbossWidgetSmall : public EmbossWidget
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	EmbossWidgetSmall(QWidget *parent = 0);
	~EmbossWidgetSmall();
};
//=============================================================================

//=============================================================================
class EmbossWidgetSmallDark : public EmbossWidget
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	EmbossWidgetSmallDark(QWidget *parent = 0);
	~EmbossWidgetSmallDark();
};
//=============================================================================

#endif // EMBOSSWIDGET_H
