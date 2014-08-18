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

#ifndef SORTABLEVIEW_H
#define SORTABLEVIEW_H

#include <QtCore/QTimer>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QVBoxLayout>

class SortableWidget;
class QDrag;

//=============================================================================
/// <summary>
/// A very basic widget that is overlayed over the position where a sortable
/// will be inserted when the user drags an item.
/// </summary>
class DragIndicator : public QWidget
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	DragIndicator(QWidget *parent = NULL);
	~DragIndicator();

private: // Events ------------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

//=============================================================================
class SortableView : public QScrollArea
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	QVector<SortableWidget *>	m_children;
	QWidget						m_layoutWidget;
	QVBoxLayout					m_layout;
	QString						m_mimeType; // Used for drag events
	DragIndicator				m_indicator;
	QTimer						m_scrollTimer;
	int							m_scrollDelta;
	int							m_curLevel;
	QPoint						m_curMousePos;

public: // Constructor/destructor ---------------------------------------------
	SortableView(const QString &mimeType, QWidget *parent = NULL);
	~SortableView();

public: // Methods ------------------------------------------------------------
	QString						getMimeType() const;

	void						addSortable(
		SortableWidget *widget, int before = -1);
	void						removeSortable(SortableWidget *widget);
	QVector<SortableWidget *>	getSortables() const;

	Qt::DropAction				dragExec(
		QDrag *drag, Qt::DropActions supportedActions);

protected:
	bool						isIndexValidForLevel(int before, int level);
	void						moveIndicator(int before);
	int							sortableIndexAtPos(
		const QPoint &pos, int level, bool round = false);
	QPoint						getScrollOffset() const;

protected: // Interface -------------------------------------------------------
	/// <summary>
	/// Called when the view wishes to move a child between positions.
	/// </summary>
	/// <returns>True if the move was successful</returns>
	virtual bool	moveChild(SortableWidget *child, int before) = 0;

private: // Events ------------------------------------------------------------
	virtual void	dragEnterEvent(QDragEnterEvent *ev);
	virtual void	dragLeaveEvent(QDragLeaveEvent *ev);
	virtual void	dragMoveEvent(QDragMoveEvent *ev);
	virtual void	dropEvent(QDropEvent *ev);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			scrollTimeout();
};
//=============================================================================

inline QString SortableView::getMimeType() const
{
	return m_mimeType;
}

inline QVector<SortableWidget *> SortableView::getSortables() const
{
	return m_children;
}

#endif // SORTABLEVIEW_H
