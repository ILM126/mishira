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

#define _USE_MATH_DEFINES

#include "layer.h"
#include "application.h"
#include "layerdialog.h"
#include "layergroup.h"
#include "profile.h"
#include "scene.h"
#include <Libvidgfx/graphicscontext.h>

const QString LOG_CAT = QStringLiteral("Scene");

Layer::Layer(LayerGroup *parent)
	: QObject()
	, m_isInitializing(true)
	, m_parent(parent)
	, m_name(tr("Unnamed"))
	, m_isLoaded(false)
	, m_isVisible(false)

	//, m_rect() // See below
	, m_visibleRect(0, 0, 0, 0) // Invisible
	, m_rotationDeg(0)
	, m_rotationRad(0.0f)
	, m_opacity(1.0f)
	, m_scaling(LyrSnapToInnerScale)
	, m_alignment(LyrMiddleCenterAlign)
{
	// Default layer rectangle is the full canvas
	QSize canvasSize = getCanvasSize();
	m_rect = QRect(0, 0, canvasSize.width(), canvasSize.height());
}

Layer::~Layer()
{
	// WARNING: The inheriting class's destructor is called before this one
	// meaning that cleaning up here doesn't actually work. Use
	// `LayerGroup::destroyLayer()` instead
}

void Layer::setInitialized()
{
	if(!m_isInitializing)
		return; // Already initialized
	if(m_parent->isInitializing())
		return; // Parent isn't fully initialized yet (Unserializing)

	m_isInitializing = false;
	initializedEvent();
	if(m_isLoaded)
		loadEvent();
	if(m_isVisible)
		showEvent();
}

void Layer::setName(const QString &name)
{
	QString str = name;
	if(str.isEmpty())
		str = tr("Unnamed");
	if(m_name == str)
		return;
	QString oldName = m_name;
	m_name = str;
	m_parent->layerChanged(this); // Remote emit
	if(!m_isInitializing)
		appLog(LOG_CAT) << "Renamed layer " << getIdString();
}

QString Layer::getIdString(bool showName) const
{
	if(showName) {
		return QStringLiteral("%1 (\"%2\")")
			.arg(pointerToString((void *)this))
			.arg(getName());
	}
	return pointerToString((void *)this);
}

void Layer::setLoaded(bool loaded)
{
	if(m_isLoaded == loaded)
		return; // Nothing to do

	if(loaded) {
		m_isLoaded = true;
		if(!m_isInitializing)
			loadEvent();
	} else {
		if(m_isVisible)
			setVisible(false);
		m_isLoaded = false;
		if(!m_isInitializing)
			unloadEvent();
	}
	m_parent->layerChanged(this); // Remote emit
}

void Layer::reload()
{
	bool wasVisible = isVisible();
	setLoaded(false);
	setLoaded(true);
	if(wasVisible)
		setVisible(true);
}

void Layer::setVisible(bool visible)
{
	if(m_isVisible == visible)
		return; // Nothing to do

	if(visible) {
		if(!m_isLoaded)
			setLoaded(true); // TODO: What if loading fails?
		m_isVisible = true;
		if(!m_isInitializing)
			showEvent();
	} else {
		m_isVisible = false;
		if(!m_isInitializing)
			hideEvent();
	}
	m_parent->layerChanged(this); // Remote emit
}

/// <summary>
/// Returns true if the layer is actually visible somewhere, either in the UI
/// or rendered on a scene. WARNING: Don't use this to determine if the layer
/// is visible in a specific scene!
/// </summary>
bool Layer::isVisibleSomewhere() const
{
	return m_isVisible && m_parent->isVisibleSomewhere();
}

void Layer::setRect(const QRect &rect)
{
	if(m_rect == rect)
		return; // Nothing to do
	m_rect = rect;

	// By default the visibility rectangle is identical to the layer rectangle
	setVisibleRect(rect);

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

/// <summary>
/// Sets the "visibility rectangle" which is the rectangle that the layer
/// is actually displaying something in. This is used by the preview widget to
/// allow for a more human-friendly layer selection. This allows the user to
/// "click through" to lower layers if this layer doesn't have anything visible
/// at the specified coordinate.
/// </summary>
void Layer::setVisibleRect(const QRect &rect)
{
	if(m_visibleRect == rect)
		return; // Nothing to do
	m_visibleRect = rect;
	// No need to update resources or emit signals
}

void Layer::setRotationDeg(int degrees)
{
	if(m_rotationDeg == degrees)
		return; // Nothing to do
	m_rotationDeg = degrees;
	m_rotationRad = (float)degrees / 180.0f * M_PI;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void Layer::setOpacity(float opacity)
{
	// Make sure the opacity is always valid
	opacity = qBound(0.0f, opacity, 1.0f);
	if(qFuzzyCompare(opacity, 0.0f))
		opacity = 0.0f;
	if(qFuzzyCompare(opacity, 1.0f))
		opacity = 1.0f;

	if(m_opacity == opacity)
		return; // Nothing to do
	m_opacity = opacity;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void Layer::setScaling(LyrScalingMode scaling)
{
	if(m_scaling == scaling)
		return; // Nothing to do
	m_scaling = scaling;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

void Layer::setAlignment(LyrAlignment alignment)
{
	if(m_alignment == alignment)
		return; // Nothing to do
	m_alignment = alignment;

	updateResourcesIfLoaded();
	m_parent->layerChanged(this); // Remote emit
}

/// <summary>
/// Move this layer in the scene.
/// </summary>
void Layer::moveTo(LayerGroup *group, int before)
{
	if(group == NULL)
		return;

	LayerList &groupLayers = group->getLayersMutable();
	if(before < 0) {
		// Position is relative to the right
		before += groupLayers.count() + 1;
	}
	before = qBound(0, before, groupLayers.count());

	LayerGroup *prevGroup = m_parent;
	if(group == m_parent) {
		// Moving within the same group. We need to be careful as the `before`
		// index includes ourself
		int index = m_parent->indexOfLayer(this);
		if(before == index)
			return; // Moving to the same spot
		if(before > index)
			before--; // Adjust to not include ourself
		groupLayers.remove(index);
		groupLayers.insert(before, this);
	} else {
		// Moving between groups. We hide the layer before moving it just in
		// case the visibility of the parent is different
		bool wasVisible = isVisible();
		setVisible(false);
		int index = m_parent->indexOfLayer(this);
		m_parent->getLayersMutable().remove(index);
		groupLayers.insert(before, this);
		m_parent = group;
		setVisible(wasVisible);
	}

	// Emit move signal
	prevGroup->layerMoved(this, before, false); // Remote emit
	m_parent->layerMoved(this, before, true); // Remote emit
}

void Layer::serialize(QDataStream *stream) const
{
	// Write data version number
	*stream << (quint32)1;

	// Save our data
	*stream << m_isLoaded;
	*stream << m_isVisible;
	*stream << m_name;
	*stream << m_rect;
	*stream << (qint32)m_rotationDeg;
	*stream << m_opacity;
	*stream << (quint32)m_scaling;
	*stream << (quint32)m_alignment;
}

bool Layer::unserialize(QDataStream *stream)
{
	QRect	rectData;
	QString	strData;
	qint32	int32Data;
	quint32	uint32Data;
	float	floatData;
	bool	boolData;

	// Make sure that the layer hasn't been initialized yet
	if(!m_isInitializing) {
		appLog(LOG_CAT, Log::Warning)
			<< "Cannot unserialize a scene layer when it has already been "
			<< "initialized";
		return false;
	}

	// Load defaults here if ever needed

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version >= 0 && version <= 1) {
		// Read top-level data
		*stream >> boolData;
		//setLoaded(boolData); Always begin unloaded
		*stream >> boolData;
		setVisible(boolData);
		*stream >> strData;
		setName(strData);
		*stream >> rectData;
		setRect(rectData);
		*stream >> int32Data;
		m_rotationDeg = int32Data;
		if(version >= 1) {
			*stream >> floatData;
			setOpacity(floatData);
		} else
			setOpacity(1.0f);
		*stream >> uint32Data;
		m_scaling = (LyrScalingMode)uint32Data;
		*stream >> uint32Data;
		m_alignment = (LyrAlignment)uint32Data;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in scene layer serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

void Layer::showSettingsDialog()
{
	if(!hasSettingsDialog())
		return; // No dialog to display

	LayerDialog *dialog = createSettingsDialog();
	dialog->loadSettings();
	App->showLayerDialog(dialog);
}

/// <summary>
/// A helper method to get the canvas size of the profile that this layer
/// belongs to. We need this as `Application::getProfile()` might still be NULL
/// or point to a different profile to the one that we actually want.
/// </summary>
QSize Layer::getCanvasSize() const
{
	return m_parent->getProfile()->getCanvasSize();
}

/// <summary>
/// Calculate the scaled layer rectangle based on the provided "actual size" of
/// the layer contents. Takes into account the user's specified layer scaling
/// method and alignment.
/// </summary>
QRect Layer::scaledRectFromActualSize(const QSize &size) const
{
	return createScaledRectInBounds(
		size, m_rect, m_scaling, m_alignment);
}

/// <summary>
/// Calls `updateResources()` only if it makes sense
/// </summary>
/// <returns></returns>
void Layer::updateResourcesIfLoaded()
{
	if(m_isLoaded && !m_isInitializing) {
		GraphicsContext *gfx = App->getGraphicsContext();
		if(gfx != NULL && gfx->isValid())
			updateResources(gfx);
	}
}

void Layer::initializedEvent()
{
}

void Layer::loadEvent()
{
	GraphicsContext *gfx = App->getGraphicsContext();
	if(gfx != NULL && gfx->isValid())
		initializeResources(gfx);
}

void Layer::unloadEvent()
{
	GraphicsContext *gfx = App->getGraphicsContext();
	if(gfx != NULL && gfx->isValid())
		destroyResources(gfx);
}

void Layer::showEvent()
{
}

void Layer::hideEvent()
{
}

void Layer::parentShowEvent()
{
}

void Layer::parentHideEvent()
{
}

void Layer::initializeResources(GraphicsContext *gfx)
{
	Q_UNUSED(gfx);
}

void Layer::updateResources(GraphicsContext *gfx)
{
	Q_UNUSED(gfx);
}

void Layer::destroyResources(GraphicsContext *gfx)
{
	Q_UNUSED(gfx);
}

void Layer::render(
	GraphicsContext *gfx, Scene *scene, uint frameNum, int numDropped)
{
	Q_UNUSED(gfx);
	Q_UNUSED(scene);
	Q_UNUSED(frameNum);
	Q_UNUSED(numDropped);
}

bool Layer::hasSettingsDialog()
{
	return false;
}

LayerDialog *Layer::createSettingsDialog(QWidget *parent)
{
	Q_UNUSED(parent);
	return NULL;
}
