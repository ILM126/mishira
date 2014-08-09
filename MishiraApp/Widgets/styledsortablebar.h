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

#ifndef STYLEDSORTABLEBAR_H
#define STYLEDSORTABLEBAR_H

#include "sortablewidget.h"
#include "Widgets/styledbar.h"

//=============================================================================
class StyledSortableBar : public StyledBar, public SortableWidget
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	StyledSortableBar(
		const QColor &fgColor, const QColor &bgColor,
		const QColor &shadowColor, int level, QWidget *parent = NULL);
	StyledSortableBar(
		const QColor &fgColor, const QColor &bgColor,
		const QColor &shadowColor, const QString &label, int level,
		QWidget *parent = NULL);
	~StyledSortableBar();

private:
	void	construct();

public: // Methods ------------------------------------------------------------

protected:
	virtual QWidget *	getWidget();
};
//=============================================================================

#endif // STYLEDSORTABLEBAR_H
