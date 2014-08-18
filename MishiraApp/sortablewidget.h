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

#ifndef SORTABLEWIDGET_H
#define SORTABLEWIDGET_H

class SortableView;
class QDrag;
class QPaintEvent;
class QWidget;

//=============================================================================
class SortableWidget
{
	friend class SortableView;

protected: // Members ---------------------------------------------------------
	int				m_level;
	SortableView *	m_viewWidget;
	void *			m_internalPtr;

public: // Constructor/destructor ---------------------------------------------
	SortableWidget(int level);
	~SortableWidget();

public: // Methods ------------------------------------------------------------
	void	setLevel(int level);
	int		getLevel() const;
	void	setInternalPtr(void *internalPtr);
	void *	getInternalPtr() const;

protected:
	QDrag *				createDragObject();

	virtual QWidget *	getWidget() = 0;
	virtual void		viewWidgetSet(SortableView *widget);

private:
	void	setViewWidget(SortableView *widget);
};
//=============================================================================

inline void SortableWidget::setLevel(int level)
{
	m_level = level;
}

inline int SortableWidget::getLevel() const
{
	return m_level;
}

inline void SortableWidget::setInternalPtr(void * internalPtr)
{
	m_internalPtr = internalPtr;
}

inline void *SortableWidget::getInternalPtr() const
{
	return m_internalPtr;
}

inline void SortableWidget::setViewWidget(SortableView *widget)
{
	m_viewWidget = widget;
	viewWidgetSet(widget);
}

#endif // SORTABLEWIDGET_H
