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

#include "sortableview.h"
#include "sortablewidget.h"
#include "stylehelper.h"
#include <QtCore/QMimeData>
#include <QtGui/QDrag>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtWidgets/QScrollBar>

//=============================================================================
// DragIndicator class

DragIndicator::DragIndicator(QWidget *parent)
	: QWidget(parent)
{
	setFixedHeight(3);
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

DragIndicator::~DragIndicator()
{
}

void DragIndicator::paintEvent(QPaintEvent *ev)
{
	QPainter p(this);
	p.setPen(StyleHelper::BlueDarkerColor);
	p.drawLine(0, 0, width(), 0); // Top
	p.drawLine(0, 2, width(), 2); // Bottom
	p.setPen(StyleHelper::BlueBaseColor);
	p.drawLine(0, 1, width(), 1); // Middle
	p.end();
}

//=============================================================================
// SortableView class

SortableView::SortableView(const QString &mimeType, QWidget *parent)
	: QScrollArea(parent)
	, m_children()
	, m_layoutWidget(this)
	, m_layout(&m_layoutWidget) // Scroll area child widget
	, m_mimeType(mimeType)
	, m_indicator(this)
	, m_scrollTimer(this)
	, m_scrollDelta(0)
	, m_curLevel(0)
	, m_curMousePos(0, 0)
{
	// Display the layout widget in the scroll area
	setWidget(&m_layoutWidget);

	// Remove layout margin and spacing
	m_layout.setMargin(0);
	m_layout.setSpacing(0);

	// Add stretch item at the end of the list
	m_layout.addStretch();

	// Allow the scroll area to resize the layout widget dynamically
	setWidgetResizable(true);

	// Hide the indicator widget
	m_indicator.hide();

	// This widget can accept drag-and-drop items
	setAcceptDrops(true);

	// Connect timer signals
	connect(&m_scrollTimer, &QTimer::timeout,
		this, &SortableView::scrollTimeout);
}

SortableView::~SortableView()
{
	// Unlink children from this widget
	for(int i = 0; i < m_children.count(); i++)
		m_children.at(i)->setViewWidget(NULL);
}

void SortableView::addSortable(SortableWidget *widget, int before)
{
	if(widget == NULL)
		return;

	// "m_layout.count() - 1" as we have a spacer at the end
	if(before < 0) {
		// Position is relative to the right
		before += (m_layout.count() - 1) + 1;
	}
	before = qBound(0, before, m_layout.count() - 1);

	m_children.insert(before, widget);
	widget->setViewWidget(this);
	QWidget *qwidget = widget->getWidget();
	if(qwidget != NULL)
		m_layout.insertWidget(before, qwidget);
}

void SortableView::removeSortable(SortableWidget *widget)
{
	if(widget == NULL)
		return;

	int index = m_children.indexOf(widget);
	if(index == -1)
		return; // Not found
	widget->setViewWidget(NULL);
	QWidget *qwidget = widget->getWidget();
	if(qwidget != NULL)
		m_layout.removeWidget(qwidget);
	m_children.remove(index);
}

/// <summary>
/// Calls `QDrag::exec()` in a more robust way. WARNING: This method blocks!
/// </summary>
Qt::DropAction SortableView::dragExec(
	QDrag *drag, Qt::DropActions supportedActions)
{
	Qt::DropAction ret =
		drag->exec(supportedActions); // Blocks until the user drops

	// Make sure that the indicator is hidden and the scroll timer is stopped
	// as we do not receive leave events when the user presses ESC.
	m_indicator.hide();
	m_scrollTimer.stop();

	return ret;
}

/// <summary>
/// Returns true if an item of the specified level can be inserted at the
/// position specified.
/// </summary>
bool SortableView::isIndexValidForLevel(int before, int level)
{
	if(before == 0)
		return level == 0; // Only level 0 items can be inserted at the top
	return true;
}

/// <summary>
/// Moves the indicator widgets so that it is inserted before the specified
/// item index.
/// </summary>
void SortableView::moveIndicator(int before)
{
	// "m_layout.count() - 1" as we have a spacer at the end
	if(before < 0) {
		// Position is relative to the right
		before += (m_layout.count() - 1) + 1;
	}
	before = qBound(0, before, m_layout.count() - 1);

	if(before >= m_layout.count() - 1) {
		// Display after the last item
		const QRect &geom = m_children.last()->getWidget()->geometry();
		m_indicator.move(0, geom.bottom() - getScrollOffset().y() - 1);
	} else {
		// Display before the specified item
		const QRect &geom = m_children.at(before)->getWidget()->geometry();
		if(before > 0)
			m_indicator.move(0, geom.top() - getScrollOffset().y() - 2);
		else {
			// Show 2px of the indicator instead of 1px
			m_indicator.move(0, geom.top() - getScrollOffset().y() - 1);
		}
	}
}

/// <summary>
/// Calculates the index of the sortable at the specified position taking into
/// account the item's level. If `round` is true then the position will round
/// to the closest gap.
/// </summary>
int SortableView::sortableIndexAtPos(const QPoint &pos, int level, bool round)
{
	int prevOnLevel = 0;
	if(round) {
		QRect prevRect;
		for(int i = 0; i < m_children.count(); i++) {
			SortableWidget *child = m_children.at(i);
			QWidget *widget = child->getWidget();
			if(child->getLevel() > level)
				continue;
			if(pos.y() < prevRect.top() +
				(widget->geometry().top() - prevRect.top()) / 2)
			{
				return prevOnLevel;
			}
			prevOnLevel = i;
			prevRect = widget->geometry();
		}
		if(pos.y() < prevRect.top() +
			(m_children.last()->getWidget()->geometry().bottom() -
			prevRect.top()) / 2)
		{
			return prevOnLevel;
		}
	} else {
		for(int i = 0; i < m_children.count(); i++) {
			SortableWidget *child = m_children.at(i);
			QWidget *widget = child->getWidget();
			if(child->getLevel() > level)
				continue;
			if(pos.y() < widget->geometry().top())
				return prevOnLevel;
			prevOnLevel = i;
		}
		if(pos.y() < m_children.last()->getWidget()->geometry().bottom() + 1)
			return prevOnLevel;
	}
	return m_children.count(); // Last position
}

QPoint SortableView::getScrollOffset() const
{
	return QPoint(
		horizontalScrollBar()->value(), verticalScrollBar()->value());
}

void SortableView::dragEnterEvent(QDragEnterEvent *ev)
{
	if(!ev->mimeData()->hasFormat(m_mimeType))
		return;
	SortableWidget *widget =
		(SortableWidget *)ev->mimeData()->data(m_mimeType).toULongLong();

	// Make sure the widget is valid before accepting
	bool found = false;
	for(int i = 0; i < m_children.count(); i++) {
		if(m_children.at(i) == widget) {
			found = true;
			break;
		}
	}
	if(!found)
		return; // Not a valid widget
	ev->acceptProposedAction();

	// Show the indicator and make sure it's above all other widgets
	//m_indicator.raise();
	m_indicator.setFixedWidth(viewport()->width());
	int before = sortableIndexAtPos(
		ev->pos() + getScrollOffset(), widget->getLevel(), true);
	moveIndicator(before);
	if(isIndexValidForLevel(before, widget->getLevel()))
		m_indicator.show();
	else
		m_indicator.hide();
}

void SortableView::dragLeaveEvent(QDragLeaveEvent *ev)
{
	// WARNING: We receive leave events when the mouse cursor is over the
	// indicator widget even if it's invisible to mouse events! This causes
	// issues with autoscrolling so we must not stop the timer.

	// Hide the indicator
	m_indicator.hide();

	// Stop the timer if it's enabled
	//m_scrollTimer.stop();
}

void SortableView::dragMoveEvent(QDragMoveEvent *ev)
{
	SortableWidget *widget =
		(SortableWidget *)ev->mimeData()->data(m_mimeType).toULongLong();
	m_curLevel = widget->getLevel();
	m_curMousePos = ev->pos();

	// If the cursor is near the edge of the widget automatically scroll
	const int SCROLL_DISTANCE = 48;
	m_scrollDelta = 0;
	if(ev->pos().y() < SCROLL_DISTANCE)
		m_scrollDelta -= SCROLL_DISTANCE - ev->pos().y();
	else if(ev->pos().y() > height() - SCROLL_DISTANCE)
		m_scrollDelta += SCROLL_DISTANCE - (height() - ev->pos().y());
	if(m_scrollDelta != 0) {
		m_scrollDelta /= 2;
		if(!m_scrollTimer.isActive()) {
			m_scrollTimer.setSingleShot(false);
			m_scrollTimer.start(100); // 10 Hz
		}
	} else
		m_scrollTimer.stop();

	// Move the indicator
	int before = sortableIndexAtPos(
		ev->pos() + getScrollOffset(), widget->getLevel(), true);
	moveIndicator(before);
	if(isIndexValidForLevel(before, widget->getLevel()))
		m_indicator.show();
	else
		m_indicator.hide();
}

void SortableView::dropEvent(QDropEvent *ev)
{
	SortableWidget *widget =
		(SortableWidget *)ev->mimeData()->data(m_mimeType).toULongLong();

	// Hide the indicator
	m_indicator.hide();

	// Get the position to insert the widget at
	int before = sortableIndexAtPos(
		ev->pos() + getScrollOffset(), widget->getLevel(), true);

	if(!isIndexValidForLevel(before, widget->getLevel()))
		return; // Invalid position

	// Forward the event for processing
	if(!moveChild(widget, before))
		return; // Failed to move

	// Notify the sender that we accepted the drop
	ev->acceptProposedAction();
}

void SortableView::scrollTimeout()
{
	if(m_scrollDelta == 0) {
		// Nothing to do
		m_scrollTimer.stop();
		return;
	}

	QScrollBar *bar = verticalScrollBar();
	bar->setValue(bar->value() + m_scrollDelta);

	// Move the indicator
	int before = sortableIndexAtPos(
		m_curMousePos + getScrollOffset(), m_curLevel, true);
	moveIndicator(before);
	if(isIndexValidForLevel(before, m_curLevel))
		m_indicator.show();
	else
		m_indicator.hide();
}
