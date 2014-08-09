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

#include "sortablewidget.h"
#include "Widgets/sortableview.h"
#include <stddef.h>
#include <QtCore/QMimeData>
#include <QtGui/QDrag>

SortableWidget::SortableWidget(int level)
	: m_level(level)
	, m_viewWidget(NULL)
	, m_internalPtr(NULL)
{
}

SortableWidget::~SortableWidget()
{
}

/// <summary>
/// Creates a QDrag object that the view widget will recognise for drag events.
/// </summary>
QDrag *SortableWidget::createDragObject()
{
	QDrag *drag = new QDrag(getWidget());
	QMimeData *mimeData = new QMimeData;
	QByteArray ptrStr = QString::number((quintptr)this).toLatin1();
	mimeData->setData(m_viewWidget->getMimeType(), ptrStr);
	drag->setMimeData(mimeData);
	return drag;
}

void SortableWidget::viewWidgetSet(SortableView *widget)
{
}
