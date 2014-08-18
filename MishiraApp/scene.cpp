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

#include "scene.h"
#include "application.h"
#include "layergroup.h"
#include "profile.h"
#include "sceneitem.h"
#include <Libvidgfx/graphicscontext.h>

const QString LOG_CAT = QStringLiteral("Scene");

Scene::Scene(Profile *profile)
	: QObject()
	, m_isInitializing(true)
	, m_isDestructing(false)
	, m_profile(profile)
	, m_name(tr("Unnamed"))
	, m_groups()
	, m_layerSceneItems()
	, m_isVisible(false)
	, m_activeItem(NULL)
{
}

Scene::~Scene()
{
	// Delete all scene items and groups cleanly
	m_isDestructing = true;
	while(!m_groups.isEmpty())
		removeGroup(m_groups.last().group);
	m_profile->pruneLayerGroups(); // As it's disabled in `removeGroup()`
}

void Scene::setInitialized()
{
	if(!m_isInitializing)
		return; // Already initialized
	m_isInitializing = false;
}

void Scene::setName(const QString &name)
{
	QString str = name;
	if(str.isEmpty())
		str = tr("Unnamed");
	if(m_name == str)
		return;
	QString oldName = m_name;
	m_name = str;
	m_profile->sceneChanged(this); // Remote emit
	if(!m_isInitializing)
		appLog(LOG_CAT) << "Renamed scene " << getIdString();
}

QString Scene::getIdString(bool showName) const
{
	if(showName) {
		return QStringLiteral("%1 (\"%2\")")
			.arg(pointerToString((void *)this))
			.arg(getName());
	}
	return pointerToString((void *)this);
}

/// <summary>
/// Called whenever the profile is going to start or stop using this scene for
/// rendering.
/// </summary>
void Scene::setVisible(bool isVisible)
{
	if(m_isVisible == isVisible)
		return; // No change
	m_isVisible = isVisible;
	if(m_isVisible) {
		// Scene has become visible, notify children
		for(int i = 0; i < m_groups.size(); i++) {
			const GroupInfo &group = m_groups.at(i);
			if(group.isVisible)
				group.group->refVisible();
		}
	} else {
		// Scene has become invisible, notify children
		for(int i = 0; i < m_groups.size(); i++) {
			const GroupInfo &group = m_groups.at(i);
			if(group.isVisible)
				group.group->derefVisible();
		}
	}
}

void Scene::addGroup(LayerGroup *group, bool isVisible, int before)
{
	if(group == NULL)
		return;
	if(indexOfGroup(group) != -1)
		return; // Already added

	if(before < 0) {
		// Position is relative to the right
		before += m_groups.count() + 1;
	}
	before = qBound(0, before, m_groups.count());

	// Was the group previously shared?
	bool wasShared = group->isShared();

	GroupInfo info;
	info.group = group;
	info.sceneItem = new SceneItem(this, group);
	info.isVisible = isVisible;
	m_groups.insert(before, info);

	if(m_isVisible && isVisible)
		group->refVisible();

	// Create scene items for child layers. Must be done completely before
	// emitting any signals to prevent crashes
	const LayerList &layers = group->getLayers();
	for(int i = 0; i < layers.size(); i++) {
		Layer *layer = layers.at(i);
		m_layerSceneItems[layer] = new SceneItem(this, layer);
	}

	// Connect to group signals so we know when to update our scene items
	connect(group, &LayerGroup::groupChanged,
		this, &Scene::groupChanged);
	connect(group, &LayerGroup::layerAdded,
		this, &Scene::layerAdded);
	connect(group, &LayerGroup::destroyingLayer,
		this, &Scene::destroyingLayer);
	connect(group, &LayerGroup::layerMoved,
		this, &Scene::layerMoved);
	connect(group, &LayerGroup::layerChanged,
		this, &Scene::layerChanged);

	// We must emit signals after we have added everything to our lists
	// otherwise slots will access invalid data
	emit itemAdded(info.sceneItem);
	for(int i = 0; i < layers.size(); i++) {
		Layer *layer = layers.at(i);
		emit itemAdded(m_layerSceneItems[layer]);
	}

	// If the group has changed shared state then emit a changed signal
	if(wasShared != group->isShared())
		group->groupChanged(group); // Remote emit
}

void Scene::removeGroup(LayerGroup *group)
{
	if(group == NULL)
		return;

	int index = indexOfGroup(group);
	if(index < 0)
		return; // Not in scene
	const GroupInfo &info = m_groups.at(index);

	// Clean up
	SceneItem *groupItem = info.sceneItem;
	if(getActiveItem() == groupItem)
		setActiveItem(NULL);
	if(m_isVisible && info.isVisible)
		group->derefVisible();

	// Disconnect to group signals. If the object is destructing then
	// disconnecting results in crashing.
	if(!m_isDestructing) {
		disconnect(group, &LayerGroup::groupChanged,
			this, &Scene::groupChanged);
		disconnect(group, &LayerGroup::layerAdded,
			this, &Scene::layerAdded);
		disconnect(group, &LayerGroup::destroyingLayer,
			this, &Scene::destroyingLayer);
		disconnect(group, &LayerGroup::layerMoved,
			this, &Scene::layerMoved);
		disconnect(group, &LayerGroup::layerChanged,
			this, &Scene::layerChanged);
	}

	// Remove group and layer scene items
	emit removingItem(groupItem); // Expected to be first
	delete groupItem;
	const LayerList &layers = group->getLayers();
	for(int i = 0; i < layers.size(); i++) {
		Layer *layer = layers.at(i);
		emit removingItem(m_layerSceneItems[layer]);
		m_layerSceneItems.remove(layer);
	}

	// Actually remove from the group list
	m_groups.remove(index);

	// As we cannot detect if the shared state has changed when a scene is
	// deleted we just blindly assume that if a group is no longer shared then
	// its shared state has changed.
	if(!group->isShared())
		group->groupChanged(group); // Remote emit

	// Delete any group that is no longer in a scene. We don't do this while
	// destructing as this scene object has already been removed from the
	// profile meaning the second and later groups will be pruned before we
	// dereference them here.
	if(!m_isDestructing)
		m_profile->pruneLayerGroups();
}

LayerGroup *Scene::groupAtIndex(int index) const
{
	if(index < 0 || index >= m_groups.count())
		return NULL;
	return m_groups.at(index).group;
}

int Scene::indexOfGroup(LayerGroup *group) const
{
	for(int i = 0; i < m_groups.size(); i++) {
		if(m_groups.at(i).group == group)
			return i;
	}
	return -1;
}

int Scene::groupCount() const
{
	return m_groups.count();
}

void Scene::moveGroupTo(LayerGroup *group, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_groups.count() + 1;
	}
	before = qBound(0, before, m_groups.count());

	// We need to be careful as the `before` index includes ourself
	int index = indexOfGroup(group);
	if(index == -1)
		return; // Group isn't in this scene
	if(before == index)
		return; // Moving to the same spot
	if(before > index)
		before--; // Adjust to not include ourself
	GroupInfo info = m_groups.at(index);
	m_groups.remove(index);
	m_groups.insert(before, info);

	emit itemsRearranged();
}

void Scene::setGroupVisible(LayerGroup *group, bool visible)
{
	int index = indexOfGroup(group);
	if(index < 0)
		return; // Not in scene
	if(m_groups[index].isVisible == visible)
		return; // No change
	m_groups[index].isVisible = visible;
	if(m_isVisible) {
		// Occurs when the user clicks the checkbox on the active scene
		if(visible)
			group->refVisible();
		else
			group->derefVisible();
	}

	// Emit signals for the group itself and all its children as layer
	// visibility depends on its parent
	emit itemChanged(m_groups[index].sceneItem);
	const LayerList &layers = group->getLayers();
	for(int i = 0; i < layers.size(); i++) {
		SceneItem *item = itemForLayer(layers.at(i));
		if(item != NULL)
			emit itemChanged(item);
	}
}

bool Scene::isGroupVisible(LayerGroup *group) const
{
	int index = indexOfGroup(group);
	if(index < 0)
		return false; // Not in scene
	return m_groups.at(index).isVisible;
}

/// <summary>
/// Returns a list of all the layer group scene items.
/// </summary>
SceneItemList Scene::getGroupSceneItems() const
{
	SceneItemList ret;
	ret.reserve(m_groups.size());
	for(int i = 0; i < m_groups.size(); i++)
		ret.append(m_groups.at(i).sceneItem);
	return ret;
}

/// <summary>
/// Returns a list of all the layer scene items inside of the specified group.
/// </summary>
SceneItemList Scene::getSceneItemsForGroup(LayerGroup *group) const
{
	SceneItemList ret;
	const LayerList &layers = group->getLayers();
	ret.reserve(layers.size());
	for(int i = 0; i < layers.size(); i++) {
		Layer *layer = layers.at(i);
		ret.append(m_layerSceneItems[layer]);
	}
	return ret;
}

/// <summary>
/// Returns the `SceneItem` that represents the specified layer group if it is
/// in this scene.
/// </summary>
SceneItem *Scene::itemForGroup(LayerGroup *group) const
{
	if(group == NULL)
		return NULL;
	int index = indexOfGroup(group);
	if(index < 0)
		return NULL; // Not in scene
	return m_groups.at(index).sceneItem;
}

/// <summary>
/// Returns the `SceneItem` that represents the specified layer if it is in
/// this scene.
/// </summary>
SceneItem *Scene::itemForLayer(Layer *layer) const
{
	if(!m_layerSceneItems.contains(layer))
		return NULL;
	return m_layerSceneItems[layer];
}

/// <summary>
/// Returns what index the specified item would be if the entire scene was
/// represented in a linear list.
/// </summary>
int Scene::listIndexOfItem(SceneItem *item) const
{
	int index = 0;
	for(int i = 0; i < m_groups.count(); i++) {
		const GroupInfo &group = m_groups.at(i);
		if(item == group.sceneItem)
			return index;
		index++;
		LayerList layers = group.group->getLayers();
		for(int j = 0; j < layers.count(); j++) {
			if(item == itemForLayer(layers.at(j)))
				return index;
			index++;
		}
	}
	return -1; // Not found
}

/// <summary>
/// Returns the item at the specified index if the entire scene was represented
/// in a linear list.
/// </summary>
SceneItem *Scene::itemAtListIndex(int index) const
{
	int item = 0;
	for(int i = 0; i < m_groups.count(); i++) {
		const GroupInfo &group = m_groups.at(i);
		if(item == index)
			return group.sceneItem;
		item++;
		LayerList layers = group.group->getLayers();
		for(int j = 0; j < layers.count(); j++) {
			Layer *layer = layers.at(j);
			if(item == index)
				return itemForLayer(layer);
			item++;
		}
	}
	return NULL; // Not found
}

/// <summary>
/// Returns the total number of items that are in this scene.
/// </summary>
int Scene::listItemCount() const
{
	int num = 0;
	for(int i = 0; i < m_groups.count(); i++) {
		num++;
		num += m_groups.at(i).group->layerCount();
	}
	return num;
}

/// <summary>
/// Convenience method.
/// </summary>
void Scene::setActiveItemGroup(LayerGroup *group)
{
	setActiveItem(itemForGroup(group));
}

/// <summary>
/// Convenience method.
/// </summary>
void Scene::setActiveItemLayer(Layer *layer)
{
	setActiveItem(itemForLayer(layer));
}

void Scene::serialize(QDataStream *stream) const
{
	// Write data version number
	*stream << (quint32)0;

	// Save name
	*stream << m_name;

	// Save group list
	*stream << (quint32)m_groups.count();
	for(int i = 0; i < m_groups.count(); i++) {
		const GroupInfo &info = m_groups.at(i);
		*stream << (quint32)info.group->getId();
		*stream << info.isVisible;
	}
}

bool Scene::unserialize(QDataStream *stream)
{
	quint32	uint32Data;
	bool	boolData;
	QString	strData;

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		// Read name
		*stream >> strData;
		setName(strData);

		// Read group list
		*stream >> uint32Data;
		int count = uint32Data;
		for(int i = 0; i < count; i++) {
			*stream >> uint32Data;
			*stream >> boolData;
			addGroup(m_profile->getLayerGroupById(uint32Data), boolData);
		}
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in scene serialized data, cannot load "
			<< "settings";
		return false;
	}

	return true;
}

void Scene::render(GraphicsContext *gfx, uint frameNum, int numDropped)
{
	if(gfx == NULL || !gfx->isValid())
		return; // Context must exist and be usuable

	// Forward to visible layers only in reverse order
	for(int i = m_groups.count() - 1; i >= 0; i--) {
		const GroupInfo &group = m_groups.at(i);
		if(!group.isVisible)
			continue;
		LayerList layers = group.group->getLayers();
		for(int j = layers.count() - 1; j >= 0; j--) {
			Layer *layer = layers.at(j);
			if(layer->isVisible())
				layer->render(gfx, this, frameNum, numDropped);
		}
	}
}

void Scene::groupChanged(LayerGroup *group)
{
	// One of our groups was modified
	SceneItem *item = itemForGroup(group);
	if(item == NULL)
		return;
	emit itemChanged(item);
}

void Scene::layerAdded(Layer *layer, int before)
{
	// A new layer was added to one of our groups
	SceneItem *item = itemForLayer(layer);
	if(item != NULL)
		return; // Already in scene
	m_layerSceneItems[layer] = new SceneItem(this, layer);
	emit itemAdded(m_layerSceneItems[layer]);
}

void Scene::destroyingLayer(Layer *layer)
{
	// A layer was removed from one of our groups
	if(!m_layerSceneItems.contains(layer))
		return;
	SceneItem *item = m_layerSceneItems[layer];
	emit removingItem(item);
	if(getActiveItem() == item)
		setActiveItem(NULL);
	delete item;
	m_layerSceneItems.remove(layer);
}

/// <summary>
/// Called twice when a layer is moved: Once from the group it was removed from
/// (`isAdd` is false) and once from the group it was moved to (`isAdd` is
/// true).
/// </summary>
void Scene::layerMoved(Layer *layer, int before, bool isAdd)
{
	// WARNING: We assume that layers can only be moved between groups that are
	// all in the same scene. I.e. a layer cannot be moved out of our
	// referenced groups and cannot be moved in.
	if(isAdd == false) {
		// Has the layer moved to another group that is not in our scene?
		if(itemForGroup(layer->getParent()) == NULL) {
			// The layer moved from a shared group in our scene to a non-shared
			// group in another scene
			destroyingLayer(layer);
		}
		return; // Wait for the layer add signal
	}
	if(!m_layerSceneItems.contains(layer)) {
		// A layer was moved from a non-shared group in another scene to a
		// shared group that's in this scene
		layerAdded(layer, before);
		return;
	}
	emit itemsRearranged();
}

void Scene::layerChanged(Layer *layer)
{
	// A layer inside one of our groups was modified
	SceneItem *item = itemForLayer(layer);
	if(item == NULL)
		return;
	emit itemChanged(item);
}
