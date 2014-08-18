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

#include "sceneitem.h"
#include "layer.h"
#include "layergroup.h"
#include "profile.h"
#include "scene.h"

SceneItem::SceneItem(Scene *scene, Layer *layer)
	: QObject()
	, m_scene(scene)
	, m_isGroup(false)
	, m_layer(layer)
	, m_group(NULL)
{
}

SceneItem::SceneItem(Scene *scene, LayerGroup *group)
	: QObject()
	, m_scene(scene)
	, m_isGroup(true)
	, m_layer(NULL)
	, m_group(group)
{
}

SceneItem::~SceneItem()
{
}

Layer *SceneItem::getLayer()
{
	if(m_isGroup)
		return NULL;
	return m_layer;
}

LayerGroup *SceneItem::getGroup()
{
	if(!m_isGroup)
		return NULL;
	return m_group;
}

void SceneItem::setName(const QString &name)
{
	if(m_isGroup)
		m_group->setName(name);
	else
		m_layer->setName(name);
}

QString SceneItem::getName() const
{
	if(m_isGroup)
		return m_group->getName();
	return m_layer->getName();
}

void SceneItem::setLoaded(bool loaded)
{
	if(!m_isGroup)
		m_layer->setLoaded(loaded);
}

bool SceneItem::isLoaded() const
{
	if(m_isGroup)
		return true; // Always "loaded"
	return m_layer->isLoaded();
}

void SceneItem::setVisible(bool visible)
{
	if(m_isGroup)
		m_scene->setGroupVisible(m_group, visible);
	else
		m_layer->setVisible(visible);
}

bool SceneItem::isVisible() const
{
	if(m_isGroup)
		return m_scene->isGroupVisible(m_group);
	return m_layer->isVisible();
}

/// <summary>
/// Returns true if this item and all its parents are visible.
/// </summary>
bool SceneItem::isVisibleRecursive() const
{
	if(m_isGroup)
		return isVisible();
	return isVisible() && m_scene->isGroupVisible(m_layer->getParent());
}

/// <summary>
/// Returns true if this item would be actually visible if `setVisible(true)`
/// was called. Takes into account parent group visibilities.
/// </summary>
bool SceneItem::wouldBeVisibleRecursive() const
{
	if(m_isGroup)
		return true;
	return m_scene->isGroupVisible(m_layer->getParent());
}

bool SceneItem::isShared() const
{
	if(!m_isGroup)
		return false;
	return m_group->isShared();
}

void SceneItem::deleteItem()
{
	if(m_isGroup)
		m_scene->removeGroup(m_group);
	else
		m_layer->getParent()->destroyLayer(m_layer);
}

/// <summary>
/// Called whenever the user clicks the show/hide button.
/// </summary>
void SceneItem::toggleVisible()
{
	setVisible(!isVisible());
}

/// <summary>
/// Called whenever the user clicks the item's label.
/// </summary>
void SceneItem::makeActiveItem()
{
	m_scene->setActiveItem(this);
}
