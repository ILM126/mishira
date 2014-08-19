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

#include "graphicswidget.h"
#include "application.h"
#include "layer.h"
#include "layergroup.h"
#include "profile.h"
#include "scene.h"
#include "sceneitem.h"
#include "stylehelper.h"
#include <Libvidgfx/graphicscontext.h>
#include <QtCore/QTime>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

const float MIN_ZOOM_LEVEL = 1.0f / 64.0f;
const float MAX_ZOOM_LEVEL = 64.0f;
const float RESIZE_HANDLE_SIZE = 6.0f; // Must be even
const int RESIZE_HANDLE_HIT_SIZE = 13; // Must be odd

GraphicsWidget::GraphicsWidget(QWidget *parent)
	: QWidget(parent)
	, m_layout(this)
	, m_dirty(false)
	, m_isAutoZoomEnabled(true)
	, m_canvasZoom(1.0f)
	, m_canvasTopLeft(0.0f, 0.0f)
	, m_isSpacePressed(false)
	, m_mouseDragMode(NotDragging)
	, m_mouseDragHandle(NoHandle)
	, m_mouseDragPos()
	, m_displayResizeRect(false)

	, m_widget(this)

	// Scene buffers
	, m_canvasBuf(NULL)
	, m_canvasBufRect()
	, m_canvasBufBrUv(0.0f, 0.0f)
	, m_outlineBuf(NULL)
	, m_resizeRectBuf(NULL)

	// Right-click layer context menu
	, m_contextLayerMenu(this)
	, m_layerPropertiesAction(NULL)
	, m_fillCanvasLayerAction(NULL)
	, m_condenseLayerAction(NULL)
	, m_duplicateLayerAction(NULL)
	, m_deleteLayerAction(NULL)
	, m_reloadLayerAction(NULL)
	, m_reloadAllLayerAction(NULL)
	, m_unloadLayerAction(NULL)
	, m_unloadAllLayerAction(NULL)

	// Right-click general context menu
	, m_contextGeneralMenu(this)
	, m_reloadAllGeneralAction(NULL)
	, m_unloadAllGeneralAction(NULL)
{
	// Make the actual widget fill the entire area
	m_layout.setMargin(0);
	m_layout.setSpacing(0);
	m_layout.addWidget(&m_widget);

	// We always want receive mouse move events so we can change the mouse
	// cursor whenever we want
	setMouseTracking(true);
	m_widget.setMouseTracking(true);

	// We can receive keyboard focus when the user clicks the widget
	setFocusPolicy(Qt::ClickFocus);

	// Repaint the screen during the application tick
	App->refProcessTicks();
	connect(App, &Application::realTimeTickEvent,
		this, &GraphicsWidget::realTimeTickEvent);

	//-------------------------------------------------------------------------
	// Right-click layer context menu

	m_layerPropertiesAction = m_contextLayerMenu.addAction(tr(
		"Layer options..."));
	m_contextLayerMenu.addSeparator();
	m_fillCanvasLayerAction = m_contextLayerMenu.addAction(tr(
		"Resize layer to fill canvas"));
	m_condenseLayerAction = m_contextLayerMenu.addAction(tr(
		"Resize layer to fit current content"));
	m_contextLayerMenu.addSeparator();
	m_reloadLayerAction = m_contextLayerMenu.addAction(tr(
		"Reload layer"));
	m_reloadAllLayerAction = m_contextLayerMenu.addAction(tr(
		"Reload all loaded layers"));
	m_unloadLayerAction = m_contextLayerMenu.addAction(tr(
		"Unload layer from memory"));
	m_unloadAllLayerAction = m_contextLayerMenu.addAction(tr(
		"Unload all invisible layers from memory"));
	m_contextLayerMenu.addSeparator();
	m_duplicateLayerAction = m_contextLayerMenu.addAction(tr(
		"Duplicate layer"));
	m_deleteLayerAction = m_contextLayerMenu.addAction(tr(
		"Delete layer..."));
	connect(&m_contextLayerMenu, &QMenu::triggered,
		this, &GraphicsWidget::layerContextTriggered);

	//-------------------------------------------------------------------------
	// Right-click general context menu

	m_reloadAllGeneralAction = m_contextGeneralMenu.addAction(tr(
		"Reload all loaded layers"));
	m_unloadAllGeneralAction = m_contextGeneralMenu.addAction(tr(
		"Unload all invisible layers from memory"));
	connect(&m_contextGeneralMenu, &QMenu::triggered,
		this, &GraphicsWidget::generalContextTriggered);
}

GraphicsWidget::~GraphicsWidget()
{
	// No longer need to process application ticks
	App->derefProcessTicks();
}

void GraphicsWidget::initializeScreen(
	VidgfxContext *gfx, const QSize &screenSize)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be usuable
	vidgfx_context_set_render_target(gfx, GfxScreenTarget);

	// Watch profile for canvas changes
	Profile *profile = App->getProfile();
	connect(profile, &Profile::frameRendered,
		this, &GraphicsWidget::canvasChanged);
	connect(profile, &Profile::canvasSizeChanged,
		this, &GraphicsWidget::canvasSizeChanged);

	// Watch all scenes for canvas changes
	const SceneList &scenes = profile->getScenes();
	connect(profile, &Profile::sceneAdded,
		this, &GraphicsWidget::watchScene);
	for(int i = 0; i < scenes.size(); i++) {
		Scene *scene = scenes.at(i);
		watchScene(scene);
	}

	// Initialize zoom and view matrix
	doAutoZoom();

	//-------------------------------------------------------------------------
	// Create scene primitives

	QSize canvasSize = App->getCanvasSize();

	// Canvas preview rectangle
	m_canvasBuf = vidgfx_context_new_vertbuf(
		gfx, GraphicsContext::TexDecalRectBufSize);
	m_canvasBufRect = QRectF();
	m_canvasBufBrUv = QPointF(0.0f, 0.0f);
	updateCanvasBuffer(QPointF(1.0f, 1.0f)); // Assume BR is (1, 1) for now

	// Canvas outline
	m_outlineBuf = vidgfx_context_new_vertbuf(
		gfx, GraphicsContext::SolidRectOutlineBufSize);
	if(m_outlineBuf != NULL) {
		vidgfx_create_solid_rect_outline(
			m_outlineBuf,
			QRectF(-0.5f, -0.5f,
			canvasSize.width() + 1, canvasSize.height() + 1),
			QColor(0, 0, 0), QPointF(0.5f, 0.5f));
	}

	// Resize layer graphic
	m_resizeRectBuf = vidgfx_context_new_vertbuf(
		gfx, GraphicsContext::ResizeRectBufSize);
	if(m_resizeRectBuf != NULL) {
		vidgfx_create_resize_rect(
			m_resizeRectBuf,
			QRectF(-10.5f, -10.5f, 101.0f, 101.0f), // Temporary size
			RESIZE_HANDLE_SIZE, QPointF(0.5f, 0.5f));
	}

	//-------------------------------------------------------------------------

	// Make sure we repaint ASAP
	setDirty(true);
}

/// <summary>
/// Watch the specified scene for any changes that could require a repaint.
/// </summary>
void GraphicsWidget::watchScene(Scene *scene)
{
	connect(scene, &Scene::activeItemChanged,
		this, &GraphicsWidget::canvasChanged);
	connect(scene, &Scene::itemAdded,
		this, &GraphicsWidget::canvasChanged);
	connect(scene, &Scene::removingItem,
		this, &GraphicsWidget::canvasChanged);
	connect(scene, &Scene::itemChanged,
		this, &GraphicsWidget::canvasChanged);
	connect(scene, &Scene::itemsRearranged,
		this, &GraphicsWidget::canvasChanged);
}

void GraphicsWidget::destroyScreen(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be usuable
	vidgfx_context_set_render_target(gfx, GfxScreenTarget);

	// Destroy scene resources
	vidgfx_context_destroy_vertbuf(gfx, m_canvasBuf);
	vidgfx_context_destroy_vertbuf(gfx, m_outlineBuf);
	vidgfx_context_destroy_vertbuf(gfx, m_resizeRectBuf);
	m_canvasBuf = NULL;
	m_outlineBuf = NULL;
	m_resizeRectBuf = NULL;
}

void GraphicsWidget::screenResized(
	VidgfxContext *gfx, const QSize &oldSize, const QSize &newSize)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be usuable
	vidgfx_context_set_render_target(gfx, GfxScreenTarget);

	// Resize render target to match the widget
	vidgfx_context_resize_screen_target(gfx, newSize);

	// Update projection matrix
	QMatrix4x4 mat;
	mat.ortho(0.0f, newSize.width(), newSize.height(), 0.0f, -1.0f, 1.0f);
	vidgfx_context_set_proj_mat(gfx, mat);

	if(m_isAutoZoomEnabled)
		doAutoZoom();
	else {
		// Make the resize appear as if the center of the widget was pinned
		// instead of the top-left
		m_canvasTopLeft.rx() += (newSize.width() - oldSize.width()) / 2;
		m_canvasTopLeft.ry() += (newSize.height() - oldSize.height()) / 2;
		updateScreenViewMatrix(); // Forces repaint
	}
}

void GraphicsWidget::renderScreen(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be usuable
	vidgfx_context_set_render_target(gfx, GfxScreenTarget);

	// Don't waste resources by repainting when nothing has changed
	// TODO: We need to know when the canvas itself is dirty
	if(!m_dirty)
		return;
	m_dirty = false;

	// TODO: Repaint only when the widget is actually visible

	//-------------------------------------------------------------------------
	// Render the preview scene

	// Clear screen
	vidgfx_context_clear(gfx, StyleHelper::DarkBg1Color);

	// Prepare canvas preview texture for render
#define HIGH_QUALITY_PREVIEW 0
#if HIGH_QUALITY_PREVIEW
	// TODO: Filter mode selection
	QPointF pxSize, botRight;
	VidgfxTex *tex = vidgfx_context_prepare_tex(
		gfx, vidgfx_context_get_target_tex(gfx, GfxCanvas1Target),
		m_canvasBufRect.toAlignedRect().size(), GfxBilinearFilter, true,
		pxSize, botRight);
	if(m_canvasBufBrUv != botRight)
		updateCanvasBuffer(botRight);
	vidgfx_context_set_tex(gfx, tex);
#else
	// It makes more sense that the preview is rendered the same way that video
	// decoders typically scale (It's also faster)
	vidgfx_context_set_tex_filter(gfx, GfxBilinearFilter);
	vidgfx_context_set_tex(
		gfx, vidgfx_context_get_target_tex(gfx, GfxCanvas1Target));
#endif // HIGH_QUALITY_PREVIEW

	// Canvas preview rectangle
	vidgfx_context_set_shader(gfx, GfxTexDecalShader);
	vidgfx_context_set_topology(gfx, GfxTriangleStripTopology);
	vidgfx_context_set_blending(gfx, GfxNoBlending);
	vidgfx_context_draw_buf(gfx, m_canvasBuf);

	// Canvas outline
	vidgfx_context_set_shader(gfx, GfxSolidShader);
	vidgfx_context_set_topology(gfx, GfxTriangleListTopology);
	vidgfx_context_draw_buf(gfx, m_outlineBuf);

	// Resize layer graphic if an editable layer is selected
	if(m_displayResizeRect) {
		vidgfx_context_set_shader(gfx, GfxResizeLayerShader);
		vidgfx_context_set_topology(gfx, GfxTriangleListTopology);
		vidgfx_context_set_tex_filter(gfx, GfxResizeLayerFilter);
		vidgfx_context_set_tex(
			gfx, vidgfx_context_get_target_tex(gfx, GfxCanvas1Target));
		vidgfx_context_draw_buf(gfx, m_resizeRectBuf);
	}

	//-------------------------------------------------------------------------

	// Should we do a buffer swap? In order to reduce ugly UI repainting we
	// disable Qt painting at certain times (E.g. profile load). As Qt cannot
	// disable rendering of our graphics scene we need to do it ourselves.
	bool doSwap = true;
	QWidget *parent = this;
	for(;;) {
		if(!parent->updatesEnabled()) {
			doSwap = false;
			break;
		}
		if(parent->parentWidget() == NULL)
			break;
		parent = parent->parentWidget();
	}

	// Swap buffers
#if 0
	static quint64 prevSwap = App->getUsecSinceExec();
	quint64 now = App->getUsecSinceExec();
	appLog() << "Time since last swap: " << (now - prevSwap) << " usec";
	prevSwap = now;
#endif // 0
	if(doSwap)
		vidgfx_context_swap_screen_bufs(gfx);
}

QPointF GraphicsWidget::mapWidgetToCanvasPos(const QPointF &pos) const
{
	return QPointF(
		(pos.x() - m_canvasTopLeft.x()) / m_canvasZoom,
		(pos.y() - m_canvasTopLeft.y()) / m_canvasZoom);
}

QPointF GraphicsWidget::mapCanvasToWidgetPos(const QPointF &pos) const
{
	return QPointF(
		pos.x() * m_canvasZoom + m_canvasTopLeft.x(),
		pos.y() * m_canvasZoom + m_canvasTopLeft.y());
}

/// <summary>
/// Gets the top-most visible layer that contains the world space coordinate
/// `pos`.
/// </summary>
Layer *GraphicsWidget::layerAtPos(const QPoint &pos) const
{
	Scene *scene = App->getActiveScene();
	if(scene == NULL)
		return NULL;
	QSize canvasSize = App->getCanvasSize();
	if(!QRect(QPoint(0, 0), canvasSize).contains(pos))
		return NULL; // Clicked outside the canvas
	SceneItemList groups = scene->getGroupSceneItems();
	for(int i = 0; i < groups.count(); i++) {
		LayerGroup *group = groups.at(i)->getGroup();
		if(!scene->isGroupVisible(group))
			continue;
		SceneItemList layers = scene->getSceneItemsForGroup(group);
		for(int j = 0; j < layers.count(); j++) {
			Layer *layer = layers.at(j)->getLayer();
			if(!layer->isVisible())
				continue;
			if(layer->getVisibleRect().contains(pos))
				return layer;
		}
	}
	return NULL;
}

/// <summary>
/// Given the provided test rectangle `rect` with screen space handle sizes of
/// `handleSize` determine which handle is at the position `testPos`. Returns
/// the relative position of the testing point which is passed to
/// `rectFromHandle()` if `relPosOut` is not NULL .
/// </summary>
GraphicsWidget::DragHandle GraphicsWidget::handleFromRect(
	const QRect &rect, int handleSize, const QPointF &invPxSize,
	const QPointF testPos, QPointF *relPosOut) const
{
	// TODO: Negative width/height rectangles?
	// TODO: There is still a rounding bug somewhere that causes a 1px offset
	// at any zoom level. Users will probably never notice as the handle size
	// is quite lenient.

	const QPointF halfInvPxSize(invPxSize * 0.5f);
	const QRectF innerRect(rect.normalized());
	const QSizeF handleRectSize(
		(float)handleSize * invPxSize.x(), (float)handleSize * invPxSize.y());
	const QPointF handleOffset(
		(float)handleSize * invPxSize.x() * -0.5f,
		(float)handleSize * invPxSize.y() * -0.5f);

	// Determine the centers of the corner handles (Center of screen pixel)
	const QPointF tlCenter(innerRect.topLeft() - halfInvPxSize);
	const QPointF trCenter(
		innerRect.topRight() + QPointF(halfInvPxSize.x(), -halfInvPxSize.y()));
	const QPointF blCenter(
		innerRect.bottomLeft() + QPointF(-halfInvPxSize.x(), halfInvPxSize.y()));
	const QPointF brCenter(innerRect.bottomRight() + halfInvPxSize);

	// Create hitboxes for the corner handles
	const QRectF tlRect(tlCenter + handleOffset, handleRectSize);
	const QRectF trRect(trCenter + handleOffset, handleRectSize);
	const QRectF blRect(blCenter + handleOffset, handleRectSize);
	const QRectF brRect(brCenter + handleOffset, handleRectSize);

	// Fast test to see if the test position is completely inside our bounding
	// rectangle or not
	if(!QRectF(tlRect.topLeft(), brRect.bottomRight()).contains(testPos)) {
		if(relPosOut)
			*relPosOut = QPointF(0.0f, 0.0f);
		return NoHandle;
	}

	// The relative position is always relative to the top-left of the input
	// rectangle as this makes it easier to do calculations.
	if(relPosOut)
		*relPosOut = testPos - innerRect.topLeft();

	// Test to see if the test position is inside our corner handles
	if(tlRect.contains(testPos))
		return TopLeftHandle;
	if(trRect.contains(testPos))
		return TopRightHandle;
	if(blRect.contains(testPos))
		return BottomLeftHandle;
	if(brRect.contains(testPos))
		return BottomRightHandle;

#define FULL_WIDTH_EDGE_HANDLES 1
#if FULL_WIDTH_EDGE_HANDLES
	// Create hitboxes for the edge and center handles
	const QRectF mlRect(tlRect.bottomLeft(), blRect.topRight());
	const QRectF mrRect(trRect.bottomLeft(), brRect.topRight());
	const QRectF tcRect(tlRect.topRight(), trRect.bottomLeft());
	const QRectF bcRect(blRect.topRight(), brRect.bottomLeft());
	//const QRectF mcRect(tlRect.bottomRight(), brRect.topLeft());
#else
	// Determine the centers of the edge and center handles (Center of screen
	// pixel). We do an offset floor() in order to match the rendered
	// rectangles for both odd and even sizes.
	float halfWidth = floorf(innerRect.width() * 0.5f + 0.5f) - 0.5f;
	float halfHeight = floorf(innerRect.height() * 0.5f + 0.5f) - 0.5f;
	const QPointF mlCenter(tlCenter + QPointF(0.0f, halfHeight));
	const QPointF mrCenter(trCenter + QPointF(0.0f, halfHeight));
	const QPointF tcCenter(tlCenter + QPointF(halfWidth, 0.0f));
	const QPointF bcCenter(blCenter + QPointF(halfWidth, 0.0f));
	//const QPointF mcCenter(tlCenter + QPointF(halfWidth, halfHeight));

	// Create hitboxes for the edge and center handles
	const QRectF mlRect(mlCenter + handleOffset, handleRectSize);
	const QRectF mrRect(mrCenter + handleOffset, handleRectSize);
	const QRectF tcRect(tcCenter + handleOffset, handleRectSize);
	const QRectF bcRect(bcCenter + handleOffset, handleRectSize);
	//const QRectF mcRect(mcCenter + handleOffset, handleRectSize);
#endif // FULL_WIDTH_EDGE_HANDLES

	// Test to see if the test position is inside our edge or center handles
	if(mlRect.contains(testPos))
		return MiddleLeftHandle;
	if(mrRect.contains(testPos))
		return MiddleRightHandle;
	if(tcRect.contains(testPos))
		return TopCenterHandle;
	if(bcRect.contains(testPos))
		return BottomCenterHandle;
	//if(mcRect.contains(testPos))
	//	return MiddleCenterHandle;

	// Everything else that's inside the hitbox must be the center
	return MiddleCenterHandle;
}

/// <summary>
/// Convenience method. Calls `handleFromRect()` using the active scene layer
/// as the input.
/// </summary>
GraphicsWidget::DragHandle GraphicsWidget::handleFromActiveRect(
	const QPointF testPos, QPointF *relPosOut) const
{
	if(!m_displayResizeRect) {
		// No handles visible
		if(relPosOut)
			*relPosOut = QPointF(0.0f, 0.0f);
		return NoHandle;
	}

	// Get layer rectangle
	Layer *layer = App->getActiveLayer();
	if(layer == NULL) {
		// No layer selected
		if(relPosOut)
			*relPosOut = QPointF(0.0f, 0.0f);
		return NoHandle;
	}
	QRect layerRect = layer->getRect();

	// Forward to `handleFromRect()`
	float invPxSize = 1.0f / m_canvasZoom;
	return handleFromRect(
		layerRect, RESIZE_HANDLE_HIT_SIZE, QPointF(invPxSize, invPxSize),
		testPos, relPosOut);
}

/// <summary>
/// Given the provided test rectangle `rect` calculate the new rectangle if
/// `handle` was being dragged at `relPos` to `newPos` (World space). If the
/// new rectangle requires the relative dragged position to be changed it is
/// returned to `newRelPosOut` if it not NULL.
/// </summary>
QRect GraphicsWidget::rectFromHandle(
	const QRect &oldRect, DragHandle handle, const QPointF &relPos,
	const QPointF &newPos, QPointF *newRelPosOut) const
{
	// TODO: Negative width/height rectangles?

	// Calculate the difference between where we clicked and where we currently
	// are
	const QRectF innerRect(oldRect.normalized());
	const QPointF deltaF((newPos - innerRect.topLeft()) - relPos);
	QPoint delta = deltaF.toPoint();

	// No change of the relative dragged position by default
	if(newRelPosOut != NULL)
		*newRelPosOut = relPos;

	// Apply delta to the appropriate corners of the rectangle making sure that
	// the rectangle's width and height will always be >= 1
	QRect newRect = applyHandleResize(oldRect, handle, delta);

	// Apply snapping
	newRect = applyHandleSnapping(newRect, handle, delta);

	// Modify horizontal new relative position
	switch(handle) {
	default:
		break;
	case TopRightHandle:
	case MiddleRightHandle:
	case BottomRightHandle:
		if(newRelPosOut != NULL)
			(*newRelPosOut).rx() += delta.x();
		break;
	}

	// Modify vertical new relative position
	switch(handle) {
	default:
		break;
	case BottomLeftHandle:
	case BottomCenterHandle:
	case BottomRightHandle:
		if(newRelPosOut != NULL)
			(*newRelPosOut).ry() += delta.y();
		break;
	}

	return newRect;
}

/// <summary>
/// Convenience method. Calls `rectFromHandle()` using the active scene layer
/// as the input.
/// </summary>
/// <returns>True if the active scene layer's rectangle was changed.</returns>
bool GraphicsWidget::activeRectFromHandle(
	DragHandle handle, const QPointF &relPos, const QPointF &newPos,
	QPointF *newRelPosOut) const
{
	if(!m_displayResizeRect)
		return false; // No handles visible

	// Get layer rectangle
	Layer *layer = App->getActiveLayer();
	if(layer == NULL)
		return false; // No layer selected
	QRect oldLayerRect = layer->getRect();

	// Forward to `rectFromHandle()`
	QRect newLayerRect = rectFromHandle(
		oldLayerRect, handle, relPos, newPos, newRelPosOut);
	if(newLayerRect == oldLayerRect)
		return false; // No change
	layer->setRect(newLayerRect);
	return true;
}

/// <summary>
/// Applies a movement of `delta` to `oldRect` when the `handle` is active. If
/// the amount of movement is invalid `delta` is modified to return the largest
/// valid change.
/// </summary>
QRect GraphicsWidget::applyHandleResize(
	const QRect &oldRect, DragHandle handle, QPoint &delta) const
{
	QRect newRect(oldRect);

	// Horizontal
	switch(handle) {
	default:
	case NoHandle:
	case TopCenterHandle:
	case MiddleCenterHandle:
	case BottomCenterHandle:
		break;
	case TopLeftHandle:
	case MiddleLeftHandle:
	case BottomLeftHandle:
		delta.rx() += qMin(0, oldRect.width() - delta.x() - 1);
		newRect.adjust(delta.x(), 0, 0, 0);
		break;
	case TopRightHandle:
	case MiddleRightHandle:
	case BottomRightHandle:
		delta.rx() -= qMin(0, oldRect.width() + delta.x() - 1);
		newRect.adjust(0, 0, delta.x(), 0);
		break;
	}

	// Vertical
	switch(handle) {
	default:
	case NoHandle:
	case MiddleLeftHandle:
	case MiddleCenterHandle:
	case MiddleRightHandle:
		break;
	case TopLeftHandle:
	case TopCenterHandle:
	case TopRightHandle:
		delta.ry() += qMin(0, oldRect.height() - delta.y() - 1);
		newRect.adjust(0, delta.y(), 0, 0);
		break;
	case BottomLeftHandle:
	case BottomCenterHandle:
	case BottomRightHandle:
		delta.ry() -= qMin(0, oldRect.height() + delta.y() - 1);
		newRect.adjust(0, 0, 0, delta.y());
		break;
	}
	if(handle == MiddleCenterHandle)
		newRect.translate(delta.x(), delta.y());

	return newRect;
}

/// <summary>
/// Snaps the specified rectangle to the closest edge of another rectangle if
/// it is within the snapping distance.
/// </summary>
void GraphicsWidget::doSnapping(
	const QRect &rect, const QRect &other, DragHandle handle, int snapDist,
	QPoint &deltaOut, bool &foundXOut, bool &foundYOut) const
{
	QPoint closestAbsDelta =
		QPoint(INT_MAX, INT_MAX); // Abs snap distance to closest snap

	deltaOut = QPoint(0, 0);
	foundXOut = false;
	foundYOut = false;

#define APPLY_DELTA_IF_CLOSER_X(delta) { \
	int absDelta = qAbs(delta); \
	if(absDelta <= snapDist && absDelta <= closestAbsDelta.x()) { \
	deltaOut.setX(delta); \
	closestAbsDelta.setX(absDelta); \
	foundXOut = true; \
	} \
	}
#define APPLY_DELTA_IF_CLOSER_Y(delta) { \
	int absDelta = qAbs(delta); \
	if(absDelta <= snapDist && absDelta <= closestAbsDelta.y()) { \
	deltaOut.setY(delta); \
	closestAbsDelta.setY(absDelta); \
	foundYOut = true; \
	} \
	}

	// We want to be able to easily snap the corner of our rectangle to the
	// opposite corner of the other rectangle (E.g. bottom-right to top-left).
	// We must handle this case separately as the two rectangles are not
	// intersecting each other
	bool isNearCorner = false;
	if(rect.top() < other.bottom() + 2 + snapDist &&
		rect.bottom() + 2 + snapDist > other.top() &&
		rect.left() < other.right() + 2 + snapDist &&
		rect.right() + 2 + snapDist > other.left())
	{
		isNearCorner = true;
	}

	// Horizontal snapping
	if((rect.top() < other.bottom() + 2 && rect.bottom() + 2 > other.top()) ||
		isNearCorner)
	{
		// Left snapping
		if(handle == TopLeftHandle ||
			handle == MiddleLeftHandle ||
			handle == BottomLeftHandle ||
			handle == MiddleCenterHandle)
		{
			// Left of other rectangle
			int delta = rect.left() - other.left();
			APPLY_DELTA_IF_CLOSER_X(delta);

			// Right of other rectangle
			delta = rect.left() - other.right() - 1;
			APPLY_DELTA_IF_CLOSER_X(delta);
		}

		// Right snapping
		if(handle == TopRightHandle ||
			handle == MiddleRightHandle ||
			handle == BottomRightHandle ||
			handle == MiddleCenterHandle)
		{
			// Left of other rectangle
			int delta = rect.right() - other.left() + 1;
			APPLY_DELTA_IF_CLOSER_X(delta);

			// Right of other rectangle
			delta = rect.right() - other.right();
			APPLY_DELTA_IF_CLOSER_X(delta);
		}
	}

	// Vertical snapping
	if((rect.left() < other.right() + 2 && rect.right() + 2 > other.left()) ||
		isNearCorner)
	{
		// Top snapping
		if(handle == TopLeftHandle ||
			handle == TopCenterHandle ||
			handle == TopRightHandle ||
			handle == MiddleCenterHandle)
		{
			// Top of other rectangle
			int delta = rect.top() - other.top();
			APPLY_DELTA_IF_CLOSER_Y(delta);

			// Bottom of other rectangle
			delta = rect.top() - other.bottom() - 1;
			APPLY_DELTA_IF_CLOSER_Y(delta);
		}

		// Bottom snapping
		if(handle == BottomLeftHandle ||
			handle == BottomCenterHandle ||
			handle == BottomRightHandle ||
			handle == MiddleCenterHandle)
		{
			// Top of other rectangle
			int delta = rect.bottom() - other.top() + 1;
			APPLY_DELTA_IF_CLOSER_Y(delta);

			// Bottom of other rectangle
			delta = rect.bottom() - other.bottom();
			APPLY_DELTA_IF_CLOSER_Y(delta);
		}
	}
}

/// <summary>
/// Snaps the specified rectangle to nearby snapping points based on the
/// `handle` and canvas zoom level. `delta` is modified if the rectangle is
/// modified.
/// </summary>
QRect GraphicsWidget::applyHandleSnapping(
	const QRect &oldRect, DragHandle handle, QPoint &delta) const
{
	// The snapping distance is in screen space instead of world space. This
	// means that our world space distance is smaller the more zoomed in we
	// are.
	const int SNAPPING_DISTANCE = 10; // Screen pixels
	int snapDist = (float)SNAPPING_DISTANCE / (float)m_canvasZoom;

	QPoint snapDelta(0, 0);
	QPoint closestAbsDelta(INT_MAX, INT_MAX);

	// Snap to canvas edges
	{
		QSize canvasSize = App->getCanvasSize();
		QRect canvasRect(QPoint(0, 0), canvasSize);

		QPoint curDelta;
		bool foundX = false;
		bool foundY = false;
		doSnapping(
			oldRect, canvasRect, handle, snapDist, curDelta, foundX, foundY);
		QPointF absDelta(qAbs(curDelta.x()), qAbs(curDelta.y()));
		if(foundX && absDelta.x() < closestAbsDelta.x()) {
			snapDelta.setX(curDelta.x());
			closestAbsDelta.setX(absDelta.x());
		}
		if(foundY && absDelta.y() < closestAbsDelta.y()) {
			snapDelta.setY(curDelta.y());
			closestAbsDelta.setY(absDelta.y());
		}
	}

	// Snap to visible layer edges
	Scene *scene = App->getActiveScene();
	if(scene == NULL)
		return oldRect; // Nothing to snap to
	SceneItemList groups = scene->getGroupSceneItems();
	for(int i = 0; i < groups.count(); i++) {
		LayerGroup *group = groups.at(i)->getGroup();
		if(!scene->isGroupVisible(group))
			continue;
		SceneItemList layers = scene->getSceneItemsForGroup(group);
		for(int j = 0; j < layers.count(); j++) {
			Layer *layer = layers.at(j)->getLayer();
			if(!layer->isVisible() || layer == App->getActiveLayer())
				continue;

			// Snap to visible rect if it's not empty
			if(!layer->getVisibleRect().isEmpty()) {
				QPoint curDelta;
				bool foundX = false;
				bool foundY = false;
				doSnapping(oldRect, layer->getVisibleRect(), handle, snapDist,
					curDelta, foundX, foundY);
				QPointF absDelta(qAbs(curDelta.x()), qAbs(curDelta.y()));
				if(foundX && absDelta.x() < closestAbsDelta.x()) {
					snapDelta.setX(curDelta.x());
					closestAbsDelta.setX(absDelta.x());
				}
				if(foundY && absDelta.y() < closestAbsDelta.y()) {
					snapDelta.setY(curDelta.y());
					closestAbsDelta.setY(absDelta.y());
				}
			}

			// TODO: Snap to real layer edges? User might be confused
		}
	}

	// Actually apply the snapping delta
	snapDelta = -snapDelta;
	QRect newRect = applyHandleResize(oldRect, handle, snapDelta);
	delta += snapDelta;
	return newRect;
}

/// <summary>
/// Determine and apply the canvas position and zoom values that are needed to
/// make the canvas fill the entire screen area.
/// </summary>
void GraphicsWidget::doAutoZoom()
{
	QSize canvasSize = App->getCanvasSize();
	const int CANVAS_MARGIN = 20;
	float xZoom =
		(float)(width() - CANVAS_MARGIN * 2) / (float)canvasSize.width();
	float yZoom =
		(float)(height() - CANVAS_MARGIN * 2) / (float)canvasSize.height();
	setZoom(qMin(xZoom, yZoom), QPointF(
		(float)(canvasSize.width() / 2),
		(float)(canvasSize.height() / 2)));
}

void GraphicsWidget::updateCanvasBuffer(const QPointF &brUv)
{
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return;
	if(m_canvasBuf == NULL)
		return;
	QSize canvasSize = App->getCanvasSize();
	const QRectF rect = QRectF(0.0f, 0.0f,
		(float)canvasSize.width() * m_canvasZoom,
		(float)canvasSize.height() * m_canvasZoom);
	if(m_canvasBufBrUv == brUv && m_canvasBufRect == rect)
		return;

	m_canvasBufRect = rect;
	m_canvasBufBrUv = brUv;
	vidgfx_create_tex_decal_rect(m_canvasBuf, QRectF(0.0f, 0.0f,
		(float)canvasSize.width(), (float)canvasSize.height()),
		m_canvasBufBrUv);
}

/// <summary>
/// The canvas outline rectangle is rendered using a 1px wide line. As this
/// line width is in screen space and not world space we need to update its
/// position whenever the zoom level changes.
/// </summary>
void GraphicsWidget::updateCanvasOutlineBuffer()
{
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return;
	if(m_outlineBuf == NULL)
		return; // Buffer doesn't exist

	// Recreate rectangle buffer
	QSize canvasSize = App->getCanvasSize();
	float invPxSize = 1.0f / m_canvasZoom;
	float halfInvPxSize = invPxSize * 0.5f;
	vidgfx_create_solid_rect_outline(
		m_outlineBuf,
		QRectF(-halfInvPxSize, -halfInvPxSize,
		(float)canvasSize.width() + invPxSize,
		(float)canvasSize.height() + invPxSize),
		QColor(0, 0, 0), QPointF(halfInvPxSize, halfInvPxSize));
}

/// <summary>
/// Similar to `updateCanvasOutlineBuffer()` but also needs to be called
/// whenever the active layer changes or the canvas changes size.
/// </summary>
void GraphicsWidget::updateResizeRectBuffer()
{
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return;
	if(m_resizeRectBuf == NULL)
		return; // Buffer doesn't exist

	// Update canvas size
	QSize canvasSize = App->getCanvasSize();
	vidgfx_context_set_resize_layer_rect(gfx, QRectF(0.0f, 0.0f,
		(float)canvasSize.width(), (float)canvasSize.height()));

	// Get layer rectangle
	Layer *layer = App->getActiveLayer();
	if(layer == NULL) {
		// No layer selected
		m_displayResizeRect = false;
		return;
	}
	QRect rect = layer->getRect();
	m_displayResizeRect = true;

	// Recreate resize layer graphic
	float invPxSize = 1.0f / m_canvasZoom;
	float halfInvPxSize = invPxSize * 0.5f;
	vidgfx_create_resize_rect(
		m_resizeRectBuf,
		QRectF(rect).adjusted(
		-halfInvPxSize, -halfInvPxSize, halfInvPxSize, halfInvPxSize),
		RESIZE_HANDLE_SIZE * invPxSize,
		QPointF(halfInvPxSize, halfInvPxSize));
}

/// <summary>
/// Update the screen view matrix using our canvas zoom and offset values.
/// </summary>
void GraphicsWidget::updateScreenViewMatrix()
{
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return;

	QMatrix4x4 mat;
	mat.translate(m_canvasTopLeft.x(), m_canvasTopLeft.y());
	mat.scale(m_canvasZoom);
	vidgfx_context_set_screen_view_mat(gfx, mat);
	setDirty(true);
}

/// <summary>
/// Set the active cursor to be whatever makes sense for the provided cursor
/// position.
/// </summary>
void GraphicsWidget::updateMouseCursor(const QPoint &mousePos)
{
	// Display a panning hand if the space bar is pressed
	if(m_isSpacePressed && m_mouseDragMode == NotDragging) {
		App->setActiveCursor(Qt::OpenHandCursor);
		return;
	}

	// Determine which handle, if any, is under the mouse
	QPointF pos = mapWidgetToCanvasPos(mousePos);
	DragHandle handle = handleFromActiveRect(pos, NULL);

	// Update the mouse cursor to match what handle we are hovering over
	switch(handle) {
	default:
	case NoHandle:
		App->setActiveCursor(Qt::ArrowCursor);
		break;
	case TopLeftHandle:
	case BottomRightHandle:
		App->setActiveCursor(Qt::SizeFDiagCursor);
		break;
	case TopRightHandle:
	case BottomLeftHandle:
		App->setActiveCursor(Qt::SizeBDiagCursor);
		break;
	case TopCenterHandle:
	case BottomCenterHandle:
		App->setActiveCursor(Qt::SizeVerCursor);
		break;
	case MiddleLeftHandle:
	case MiddleRightHandle:
		App->setActiveCursor(Qt::SizeHorCursor);
		break;
	case MiddleCenterHandle:
		App->setActiveCursor(Qt::OpenHandCursor);
		break;
	}
}

void GraphicsWidget::keyPressEvent(QKeyEvent *ev)
{
	// Keyboard modifier multiplier for moves
	QPoint layerDelta(0, 0);
	int multi = 1;
	if(ev->modifiers() & Qt::ShiftModifier)
		multi = 100;
	else if(ev->modifiers() & Qt::ControlModifier)
		multi = 10;

	Layer *layer = App->getActiveLayer();
	switch(ev->key()) {
	default:
		ev->ignore();
		break;
	case Qt::Key_Space: {
		if(!ev->isAutoRepeat()) {
			m_isSpacePressed = true;
			App->setActiveCursor(Qt::OpenHandCursor);
		}
		ev->accept();
		break; }
	case Qt::Key_Left: {
		layerDelta = QPoint(-1 * multi, 0);
		ev->accept();
		break; }
	case Qt::Key_Up: {
		layerDelta = QPoint(0, -1 * multi);
		ev->accept();
		break; }
	case Qt::Key_Right: {
		layerDelta = QPoint(1 * multi, 0);
		ev->accept();
		break; }
	case Qt::Key_Down: {
		layerDelta = QPoint(0, 1 * multi);
		ev->accept();
		break; }
	}

	// Apply keyboard movement taking into account the active handle if the
	// user is currently dragging the mouse
	if(layer != NULL && !layerDelta.isNull()) {
		if(m_mouseDragMode == DraggingLeft
			&& m_mouseDragHandle != NoHandle)
		{
			layer->setRect(applyHandleResize(
				layer->getRect(), m_mouseDragHandle, layerDelta));

			// Modify horizontal new relative position
			switch(m_mouseDragHandle) {
			default:
				break;
			case TopRightHandle:
			case MiddleRightHandle:
			case BottomRightHandle:
				m_mouseDragPos.rx() += layerDelta.x();
				break;
			}

			// Modify vertical new relative position
			switch(m_mouseDragHandle) {
			default:
				break;
			case BottomLeftHandle:
			case BottomCenterHandle:
			case BottomRightHandle:
				m_mouseDragPos.ry() += layerDelta.y();
				break;
			}
		} else
			layer->setRect(layer->getRect().translated(layerDelta));
	}
}

void GraphicsWidget::keyReleaseEvent(QKeyEvent *ev)
{
	switch(ev->key()) {
	default:
		ev->ignore();
		break;
	case Qt::Key_Space: {
		if(!ev->isAutoRepeat()) {
			m_isSpacePressed = false;
			if(m_mouseDragMode == NotDragging) {
				// TODO: Use `updateMouseCursor()`
				App->setActiveCursor(Qt::ArrowCursor);
			}
		}
		ev->accept();
		break; }
	}
}

void GraphicsWidget::mouseMoveEvent(QMouseEvent *ev)
{
	if(m_mouseDragMode == DraggingLeft) {
		// Resize the active layer
		QPointF pos = mapWidgetToCanvasPos(ev->pos());
		QPointF newRelPos;
		activeRectFromHandle(
			m_mouseDragHandle, m_mouseDragPos, pos, &newRelPos);
		m_mouseDragPos = newRelPos;
		ev->accept();
	} else if(m_mouseDragMode == DraggingMiddle ||
		m_mouseDragMode == DraggingLeftPan)
	{
		// Pan the canvas so that the clicked position is under the new mouse
		// position
		setAutoZoomEnabled(false);
		QPointF pos = mapWidgetToCanvasPos(ev->pos());
		QPointF delta = pos - m_mouseDragPos;
		m_canvasTopLeft += delta * m_canvasZoom;
		m_canvasTopLeft = // Align top-left to pixel grid
			QPointF(floorf(m_canvasTopLeft.x()), floorf(m_canvasTopLeft.y()));
		updateScreenViewMatrix();
		ev->accept();
	} else if(m_mouseDragMode == NotDragging) {
		// Update the mouse cursor only when we're not already dragging as we
		// don't want to override the drag cursor
		updateMouseCursor(ev->pos());

		// Ignore the event as cursor changing is passive
		ev->ignore();
	}
}

void GraphicsWidget::mousePressEvent(QMouseEvent *ev)
{
	if(ev->button() == Qt::LeftButton && m_mouseDragMode == NotDragging &&
		!m_isSpacePressed)
	{
		// Left clicking the scene has two possible events: The user wants to
		// move or resize a layer by dragging or the user wants to select a
		// different layer. We test for dragging first and if the user has
		// clicked outside the layer's rectangle then they must want to select
		// another layer instead.

		// Determine which handle, if any, is under the mouse and get the
		// relative position of the cursor on that handle
		QPointF pos = mapWidgetToCanvasPos(ev->pos());
		QPointF relPos;
		DragHandle handle = handleFromActiveRect(pos, &relPos);
		if(handle == NoHandle) {
			// No handle under mouse, the user is selecting another layer
			Layer *layer = layerAtPos(QPoint((int)pos.x(), (int)pos.y()));
			Scene *scene = App->getActiveScene();
			if(layer == NULL) {
				// No layer at point, deselect current layer
				if(scene != NULL)
					scene->setActiveItem(NULL);
				ev->accept();
				return;
			}
			scene->setActiveItemLayer(layer);
			ev->accept();
			return;
		}
		m_mouseDragMode = DraggingLeft;
		m_mouseDragHandle = handle;
		m_mouseDragPos = relPos;
		if(handle == MiddleCenterHandle)
			App->setActiveCursor(Qt::ClosedHandCursor);
		ev->accept();
	} else if((ev->button() == Qt::MiddleButton ||
		(m_isSpacePressed && ev->button() == Qt::LeftButton))
		&& m_mouseDragMode == NotDragging)
	{
		// Pan with the middle mouse button or with the left mouse button when
		// space is held
		QPointF pos = mapWidgetToCanvasPos(ev->pos());
		m_mouseDragMode = (ev->button() == Qt::LeftButton)
			? DraggingLeftPan : DraggingMiddle;
		m_mouseDragPos = pos;
		App->setActiveCursor(Qt::ClosedHandCursor);
		ev->accept();
	} else
		ev->ignore();
}

void GraphicsWidget::mouseReleaseEvent(QMouseEvent *ev)
{
	// WARNING: This method is also called by `mouseDoubleClickEvent()`!

	if(ev->button() == Qt::LeftButton && m_mouseDragMode == DraggingLeft) {
		// Resize layers with the left mouse button
		m_mouseDragMode = NotDragging;
		m_mouseDragHandle = NoHandle;
		updateMouseCursor(ev->pos());
		ev->accept();
	} else if((ev->button() == Qt::MiddleButton
		&& m_mouseDragMode == DraggingMiddle) ||
		(ev->button() == Qt::LeftButton && m_mouseDragMode == DraggingLeftPan))
	{
		// Pan with the middle mouse button or with the left mouse button when
		// space is held
		m_mouseDragMode = NotDragging;
		updateMouseCursor(ev->pos());
		ev->accept();
	} else if(ev->button() == Qt::RightButton) {
		Layer *layer = App->getActiveLayer();
		if(layer == NULL)
			m_contextGeneralMenu.popup(QCursor::pos());
		else
			m_contextLayerMenu.popup(QCursor::pos());
	} else
		ev->ignore();
}

void GraphicsWidget::mouseDoubleClickEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	// Double clicking on a layer opens the layer settings dialog
	QPointF pos = mapWidgetToCanvasPos(ev->pos());
	DragHandle handle = handleFromActiveRect(pos,NULL);
	if(handle == NoHandle) {
		// No handle under mouse
		ev->ignore();
		return;
	}

	// Get active layer
	Layer *layer = App->getActiveLayer();
	if(layer == NULL) {
		// No layer selected (Should never happen as we got a handle)
		ev->ignore();
		return;
	}

	// Open the dialog. Always accept even if it doesn't have a dialog so it is
	// more consistent
	if(layer->hasSettingsDialog()) {
		// As showing the dialog steals focus away from the main window we need
		// to manually emit the mouse release event as well
		mouseReleaseEvent(ev);

		// Actually show the dialog
		layer->showSettingsDialog();
	}
	ev->accept();
}

void GraphicsWidget::wheelEvent(QWheelEvent *ev)
{
	// Zoom towards or away from the mouse cursor position
	setAutoZoomEnabled(false);
	float zoomAmount = 1.125f; // Same as in ZoomSlider
	zoomAmount = ev->delta() >= 0.0f ? zoomAmount : 1.0f / zoomAmount;
	float oldZoom = m_canvasZoom;
	m_canvasZoom *= zoomAmount;
	m_canvasZoom = qBound(MIN_ZOOM_LEVEL, m_canvasZoom, MAX_ZOOM_LEVEL);
	if(oldZoom == m_canvasZoom)
		return; // No change
	zoomAmount = m_canvasZoom / oldZoom;
	m_canvasTopLeft -= ev->pos();
	m_canvasTopLeft *= zoomAmount;
	m_canvasTopLeft += ev->pos();
	m_canvasTopLeft = // Align top-left to pixel grid
		QPointF(floorf(m_canvasTopLeft.x()), floorf(m_canvasTopLeft.y()));
	updateScreenViewMatrix();
	updateCanvasBuffer(m_canvasBufBrUv);
	updateCanvasOutlineBuffer();
	updateResizeRectBuffer();
	ev->accept();
	emit zoomChanged(m_canvasZoom);
}

void GraphicsWidget::leaveEvent(QEvent *ev)
{
	// Make sure that the mouse cursor is reset to normal
	App->setActiveCursor(Qt::ArrowCursor);
}

void GraphicsWidget::setZoom(float zoom, const QPointF &center)
{
	zoom = qBound(MIN_ZOOM_LEVEL, zoom, MAX_ZOOM_LEVEL);
	QPointF newTL(
		(float)(width() / 2) - center.x() * zoom,
		(float)(height() / 2) - center.y() * zoom);
	if(m_canvasZoom == zoom && m_canvasTopLeft == newTL)
		return; // No change
	m_canvasZoom = zoom;
	m_canvasTopLeft = newTL;

	// Apply zoom
	updateScreenViewMatrix();
	updateCanvasBuffer(m_canvasBufBrUv);
	updateCanvasOutlineBuffer();
	updateResizeRectBuffer();

	emit zoomChanged(m_canvasZoom);
}

void GraphicsWidget::setAutoZoomEnabled(bool enabled)
{
	if(m_isAutoZoomEnabled == enabled)
		return;
	m_isAutoZoomEnabled = enabled;
	if(m_isAutoZoomEnabled)
		doAutoZoom();
	emit autoZoomEnabledChanged(m_isAutoZoomEnabled);
}

void GraphicsWidget::realTimeTickEvent(int numDropped, int lateByUsec)
{
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return;

	// Repaint the screen
	renderScreen(gfx);
}

void GraphicsWidget::canvasChanged()
{
	updateResizeRectBuffer(); // TODO: This is called too often
	setDirty(true);
}

void GraphicsWidget::canvasSizeChanged(const QSize &oldSize)
{
	VidgfxContext *gfx = App->getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return;

	QSize canvasSize = App->getCanvasSize();
	if(m_canvasBuf != NULL) {
		vidgfx_create_tex_decal_rect(
			m_canvasBuf,
			QRectF(0.0f, 0.0f, canvasSize.width(), canvasSize.height()));
	}

	if(m_isAutoZoomEnabled)
		doAutoZoom();
	else {
		// Change the current camera zoom and position so that the canvas
		// remains approximately the same size on the screen.
		// TODO: Handle aspect ratio changes better
		float xRatio = (float)oldSize.width() / (float)canvasSize.width();
		//float yRatio = (float)oldSize.height() / (float)canvasSize.height();
		//if(xRatio > yRatio)
		//	m_canvasZoom *= yRatio;
		//else
		m_canvasZoom *= xRatio;

		updateCanvasBuffer(m_canvasBufBrUv);
		updateScreenViewMatrix(); // Sets dirty
		updateCanvasOutlineBuffer();
		updateResizeRectBuffer();

		emit zoomChanged(m_canvasZoom);
	}
}

void GraphicsWidget::graphicsContextInitialized(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be usable

	// Forward to our actual initialize method
	initializeScreen(gfx, m_widget.size());
}

void GraphicsWidget::graphicsContextDestroyed(VidgfxContext *gfx)
{
	if(!vidgfx_context_is_valid(gfx))
		return; // Context must exist and be usable

	// Forward to our actual destroy method
	destroyScreen(gfx);
}

void GraphicsWidget::layerContextTriggered(QAction *action)
{
	Layer *contextLayer = App->getActiveLayer();
	if(contextLayer == NULL)
		return; // Should never happen

	if(action == m_layerPropertiesAction) {
		if(contextLayer->hasSettingsDialog())
			contextLayer->showSettingsDialog();
	} else if(action == m_fillCanvasLayerAction)
		contextLayer->setRect(QRect(QPoint(), App->getCanvasSize()));
	else if(action == m_condenseLayerAction) {
		const QRect &visibleRect = contextLayer->getVisibleRect();
		if(!visibleRect.isEmpty())
			contextLayer->setRect(visibleRect);
	} else if(action == m_duplicateLayerAction)
		contextLayer->getParent()->duplicateLayer(contextLayer);
	else if(action == m_deleteLayerAction) {
		App->showDeleteLayerDialog(
			App->getActiveScene()->itemForLayer(contextLayer));
	} else if(action == m_reloadLayerAction)
		contextLayer->reload();
	else if(action == m_reloadAllLayerAction)
		App->getProfile()->reloadAllLayers();
	else if(action == m_unloadLayerAction)
		contextLayer->setLoaded(false);
	else if(action == m_unloadAllLayerAction)
		App->getProfile()->unloadAllInvisibleLayers();
}

void GraphicsWidget::generalContextTriggered(QAction *action)
{
	if(action == m_reloadAllGeneralAction)
		App->getProfile()->reloadAllLayers();
	else if(action == m_unloadAllGeneralAction)
		App->getProfile()->unloadAllInvisibleLayers();
}
