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

#ifndef VISIBLECHECKBOX_H
#define VISIBLECHECKBOX_H

#include <QtWidgets/QCheckBox>

class SceneItem;

//=============================================================================
/// <summary>
/// A special QCheckBox that prevents single-frame visual glitches when a scene
/// layer in a hidden group is set visible.
/// </summary>
class VisibleCheckBox : public QCheckBox
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	SceneItem *	m_item;

public: // Constructor/destructor ---------------------------------------------
	VisibleCheckBox(SceneItem *item, QWidget *parent = 0);
	~VisibleCheckBox();

public: // Methods ------------------------------------------------------------
	void			setCheckState(Qt::CheckState state);

protected:
	virtual void	nextCheckState();
	virtual void	mouseDoubleClickEvent(QMouseEvent *ev);
};
//=============================================================================

#endif // VISIBLECHECKBOX_H
