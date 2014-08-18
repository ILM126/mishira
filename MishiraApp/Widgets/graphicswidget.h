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

#ifndef GRAPHICSWIDGET_H
#define GRAPHICSWIDGET_H

#include "d3dwidget.h"
#include <QtWidgets/QMenu>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

class GraphicsContext;
class Layer;
class Scene;
class VertexBuffer;

//=============================================================================
/// <summary>
/// A platform-independent widget that contains the 3D viewport. This is not an
/// abstract class as we use QGLWidget on OS X while our own custom widget on
/// Windows.
/// </summary>
class GraphicsWidget : public QWidget
{
	Q_OBJECT

private: // Datatypes ---------------------------------------------------------
	enum DragMode {
		NotDragging,
		DraggingLeft,
		DraggingLeftPan,
		DraggingMiddle
	};
	enum DragHandle {
		NoHandle = 0,
		TopLeftHandle,
		TopCenterHandle,
		TopRightHandle,
		MiddleLeftHandle,
		MiddleCenterHandle,
		MiddleRightHandle,
		BottomLeftHandle,
		BottomCenterHandle,
		BottomRightHandle
	};

protected: // Members ---------------------------------------------------------
	QVBoxLayout		m_layout;
	bool			m_dirty;
	bool			m_isAutoZoomEnabled;
	float			m_canvasZoom;
	QPointF			m_canvasTopLeft;
	bool			m_isSpacePressed;
	DragMode		m_mouseDragMode;
	DragHandle		m_mouseDragHandle;
	QPointF			m_mouseDragPos;
	bool			m_displayResizeRect;

#ifdef Q_OS_WIN
	D3DWidget		m_widget;
#else
	QGLWidget		m_widget;
#endif

	// Scene buffers
	VertexBuffer *	m_canvasBuf;
	QRectF			m_canvasBufRect;
	QPointF			m_canvasBufBrUv;
	VertexBuffer *	m_outlineBuf;
	VertexBuffer *	m_resizeRectBuf;

	// Right-click layer context menu
	QMenu			m_contextLayerMenu;
	QAction *		m_layerPropertiesAction;
	QAction *		m_fillCanvasLayerAction;
	QAction *		m_condenseLayerAction;
	QAction *		m_duplicateLayerAction;
	QAction *		m_deleteLayerAction;
	QAction *		m_reloadLayerAction;
	QAction *		m_reloadAllLayerAction;
	QAction *		m_unloadLayerAction;
	QAction *		m_unloadAllLayerAction;

	// Right-click general context menu
	QMenu			m_contextGeneralMenu;
	QAction *		m_reloadAllGeneralAction;
	QAction *		m_unloadAllGeneralAction;

public: // Constructor/destructor ---------------------------------------------
	GraphicsWidget(QWidget *parent = 0);
	~GraphicsWidget();

public: // Methods ------------------------------------------------------------
	void		setDirty(bool dirty = true);
	bool		isAutoZoomEnabled() const;

	void		initializeScreen(
		GraphicsContext *gfx, const QSize &screenSize);
	void		destroyScreen(GraphicsContext *gfx);
	void		screenResized(
		GraphicsContext *gfx, const QSize &oldSize, const QSize &newSize);
	void		renderScreen(GraphicsContext *gfx);

	QPointF			mapWidgetToCanvasPos(const QPointF &pos) const;
	QPointF			mapCanvasToWidgetPos(const QPointF &pos) const;

private:
	Layer *			layerAtPos(const QPoint &pos) const;

	DragHandle		handleFromRect(
		const QRect &rect, int handleSize, const QPointF &invPxSize,
		const QPointF testPos, QPointF *relPosOut) const;
	DragHandle		handleFromActiveRect(
		const QPointF testPos, QPointF *relPosOut) const;
	QRect			rectFromHandle(
		const QRect &oldRect, DragHandle handle, const QPointF &relPos,
		const QPointF &newPos, QPointF *newRelPosOut) const;
	bool			activeRectFromHandle(
		DragHandle handle, const QPointF &relPos, const QPointF &newPos,
		QPointF *newRelPosOut) const;
	QRect			applyHandleResize(
		const QRect &oldRect, DragHandle handle, QPoint &delta) const;
	QRect			applyHandleSnapping(
		const QRect &oldRect, DragHandle handle, QPoint &delta) const;
	void			doSnapping(
		const QRect &rect, const QRect &other, DragHandle handle, int snapDist,
		QPoint &deltaOut, bool &foundXOut, bool &foundYOut) const;

	void			doAutoZoom();
	void			updateCanvasBuffer(const QPointF &brUv);
	void			updateCanvasOutlineBuffer();
	void			updateResizeRectBuffer();
	void			updateScreenViewMatrix();
	void			updateMouseCursor(const QPoint &mousePos);

protected: // Events ----------------------------------------------------------
	virtual void	keyPressEvent(QKeyEvent *ev);
	virtual void	keyReleaseEvent(QKeyEvent *ev);
	virtual void	mouseMoveEvent(QMouseEvent *ev);
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	mouseDoubleClickEvent(QMouseEvent *ev);
	virtual void	wheelEvent(QWheelEvent *ev);
	virtual void	leaveEvent(QEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	zoomChanged(float zoom);
	void	autoZoomEnabledChanged(bool enabled);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	watchScene(Scene *scene);
	void	setZoom(float zoom, const QPointF &center);
	void	setAutoZoomEnabled(bool enabled);
	void	realTimeTickEvent(int numDropped, int lateByUsec);
	void	canvasChanged();
	void	canvasSizeChanged(const QSize &oldSize);
	void	graphicsContextInitialized(GraphicsContext *gfx);
	void	graphicsContextDestroyed(GraphicsContext *gfx);
	void	layerContextTriggered(QAction *action);
	void	generalContextTriggered(QAction *action);
};
//=============================================================================

inline void GraphicsWidget::setDirty(bool dirty)
{
	m_dirty = true;
}

inline bool GraphicsWidget::isAutoZoomEnabled() const
{
	return m_isAutoZoomEnabled;
}

#endif // GRAPHICSWIDGET_H
