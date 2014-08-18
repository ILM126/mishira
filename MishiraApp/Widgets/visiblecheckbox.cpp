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

#include "visiblecheckbox.h"
#include "application.h"
#include "sceneitem.h"
#include "darkstyle.h"
#include <QtGui/QMouseEvent>

VisibleCheckBox::VisibleCheckBox(SceneItem *item, QWidget *parent)
	: QCheckBox(parent)
	, m_item(item)
{
}

VisibleCheckBox::~VisibleCheckBox()
{
}

void VisibleCheckBox::setCheckState(Qt::CheckState state)
{
	QCheckBox::setCheckState(state);

	// HACK: We want to display the correct "next state" when the user clicks
	// the widget. Read the checkbox drawing code in DarkStyle for more info.
	if(!App->isDarkStyle(this))
		return;
	Qt::CheckState nextState =
		(state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
	if(!m_item->isVisible() && !m_item->wouldBeVisibleRecursive())
		nextState = Qt::PartiallyChecked;
	if(nextState == Qt::PartiallyChecked)
		App->getDarkStyle()->addNextCheckStateIsTri(this);
	else
		App->getDarkStyle()->removeNextCheckStateIsTri(this);
}

void VisibleCheckBox::nextCheckState()
{
	if(m_item->isVisible())
		setCheckState(Qt::Unchecked);
	else if(m_item->wouldBeVisibleRecursive())
		setCheckState(Qt::Checked);
	else
		setCheckState(Qt::PartiallyChecked);
}

void VisibleCheckBox::mouseDoubleClickEvent(QMouseEvent *ev)
{
	// Eat double-click events so the user can show and hide the layer quickly
	// without accidentally opening the properties dialog
	ev->accept();
}
