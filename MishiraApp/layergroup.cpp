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

#include "layergroup.h"
#include "layerfactory.h"
#include "profile.h"
#include "scene.h"
#include "sceneitem.h"

const QString LOG_CAT = QStringLiteral("Scene");

LayerGroup::LayerGroup(Profile *profile)
	: QObject()
	, m_isInitializing(true)
	, m_profile(profile)
	, m_name(tr("Unnamed"))
	, m_layers()
	, m_visibleRef(0)
{
}

LayerGroup::~LayerGroup()
{
	// Delete all child layers cleanly
	while(!m_layers.isEmpty())
		destroyLayer(m_layers.first());
}

void LayerGroup::setInitialized()
{
	if(!m_isInitializing)
		return; // Already initialized
	m_isInitializing = false;
	initializedEvent();
}

quint32 LayerGroup::getId()
{
	return m_profile->idOfLayerGroup(this);
}

QString LayerGroup::getIdString(bool showName) const
{
	if(showName) {
		return QStringLiteral("%1 (\"%2\")")
			.arg(pointerToString((void *)this))
			.arg(getName());
	}
	return pointerToString((void *)this);
}

void LayerGroup::setName(const QString &name)
{
	QString str = name;
	if(str.isEmpty())
		str = tr("Unnamed");
	if(m_name == str)
		return;
	QString oldName = m_name;
	m_name = str;
	emit groupChanged(this);
	if(!m_isInitializing)
		appLog(LOG_CAT) << "Renamed layer group " << getIdString();
}

void LayerGroup::refVisible()
{
	m_visibleRef++;
	if(m_visibleRef == 1) {
		// Group has become visible, notify children
		showEvent();
	}
}

void LayerGroup::derefVisible()
{
	if(m_visibleRef > 0) {
		m_visibleRef--;
		if(m_visibleRef == 0) {
			// Group has become invisible, notify children
			hideEvent();
		}
	}
}

/// <summary>
/// Returns true if the group is shared between multiple scenes.
/// </summary>
bool LayerGroup::isShared() const
{
	const SceneList &scenes = m_profile->getScenes();
	int refs = 0;
	for(int i = 0; i < scenes.size(); i++) {
		Scene *scene = scenes.at(i);
		const SceneItemList &items = scene->getGroupSceneItems();
		for(int j = 0; j < items.size(); j++) {
			LayerGroup *group = items.at(j)->getGroup();
			if(group == this) {
				refs++;
				if(refs > 1)
					return true; // Group is shared
			}
		}
	}
	return false; // Group is not shared
}

Layer *LayerGroup::constructLayer(quint32 typeId)
{
	LayerFactory *factory = LayerFactory::findFactory(typeId);
	if(factory == NULL)
		return NULL; // Layer type not found
	return factory->createBlankLayer(this);
}

Layer *LayerGroup::createLayer(quint32 typeId, const QString &name, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_layers.count() + 1;
	}
	before = qBound(0, before, m_layers.count());

	Layer *layer = constructLayer(typeId);
	if(layer == NULL)
		return NULL;
	if(!name.isEmpty())
		layer->setName(name);
	m_layers.insert(before, layer);
	appLog(LOG_CAT) << "Created layer " << layer->getIdString();
	layer->setInitialized();

	emit layerAdded(layer, before);
	return layer;
}

Layer *LayerGroup::createLayerSerialized(
	quint32 typeId, QDataStream *stream, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_layers.count() + 1;
	}
	before = qBound(0, before, m_layers.count());

	Layer *layer = constructLayer(typeId);
	if(layer == NULL)
		return NULL;
	appLog(LOG_CAT)
		<< "Unserializing layer " << layer->getIdString(false) << "...";
	if(!layer->unserialize(stream)) {
		// Failed to unserialize data
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to fully unserialize scene layer data";
		delete layer;
		return NULL;
	}
	m_layers.insert(before, layer);
	appLog(LOG_CAT) << "Created layer " << layer->getIdString();
	layer->setInitialized();

	emit layerAdded(layer, before);
	return layer;
}

void LayerGroup::destroyLayer(Layer *layer)
{
	int id = m_layers.indexOf(layer);
	if(id == -1)
		return; // Doesn't exist
	emit destroyingLayer(layer);

	// Make sure all hardware resources are unloaded before deleting
	layer->setLoaded(false);

	QString idString = layer->getIdString();
	m_layers.remove(id);
	delete layer;
	appLog(LOG_CAT) << "Deleted layer " << idString;
}

Layer *LayerGroup::layerAtIndex(int index) const
{
	if(index < 0 || index >= m_layers.count())
		return NULL;
	return m_layers.at(index);
}

/// <summary>
/// Returns true if all the child layers are loaded.
/// </summary>
bool LayerGroup::areAllLayersLoaded() const
{
	for(int i = 0; i < m_layers.count(); i++) {
		if(!m_layers.at(i)->isLoaded())
			return false;
	}
	return true;
}

/// <summary>
/// Loads all child layers in the group.
/// </summary>
void LayerGroup::loadAllLayers()
{
	for(int i = 0; i < m_layers.count(); i++)
		m_layers.at(i)->setLoaded(true);
}

/// <summary>
/// Duplicates the specified layer.
/// </summary>
/// <returns>True if the layer was successfully duplicated</returns>
bool LayerGroup::duplicateLayer(Layer *layer)
{
	LayerGroup *group = layer->getParent();
	if(group != this)
		return false;
	int index = indexOfLayer(layer);

	// Duplicate layer by serializing, unserializing and renaming
	QBuffer buf;
	buf.open(QIODevice::ReadWrite);
	QDataStream stream(&buf);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
	stream.setVersion(12);
	layer->serialize(&stream);
	if(stream.status() != QDataStream::Ok) {
		appLog(Log::Warning)
			<< "An error occurred while duplicating (Serializing)";
		return false;
	}
	buf.seek(0);
	Layer *newLayer = createLayerSerialized(
		layer->getTypeId(), &stream, index);
	if(stream.status() != QDataStream::Ok) {
		appLog(Log::Warning)
			<< "An error occurred while duplicating (Unserializing)";
		return false;
	}
	newLayer->setName(tr("%1 (Copy)").arg(layer->getName()));
	return true;
}

void LayerGroup::serialize(QDataStream *stream) const
{
	// Write data version number
	*stream << (quint32)0;

	// Save top-level data
	*stream << m_name;

	// Save child layers
	*stream << (quint32)m_layers.count();
	for(int i = 0; i < m_layers.count(); i++) {
		Layer *layer = m_layers.at(i);
		*stream << layer->getTypeId();
		layer->serialize(stream);
	}
}

bool LayerGroup::unserialize(QDataStream *stream)
{
	QString	strData;
	quint32	uint32Data;

	// Make sure that the group hasn't been initialized yet
	if(!m_isInitializing) {
		appLog(LOG_CAT, Log::Warning)
			<< "Cannot unserialize a scene layer group when it has already "
			<< "been initialized";
		return false;
	}

	// Load defaults here if ever needed

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		// Read top-level data
		*stream >> strData;
		setName(strData);

		// Read child layers
		*stream >> uint32Data;
		int count = uint32Data;
		for(int i = 0; i < count; i++) {
			*stream >> uint32Data;
			quint32 typeId = uint32Data;
			if(createLayerSerialized(typeId, stream) == NULL) {
				// The reason has already been logged
				return false;
			}
		}
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in scene layer group serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

void LayerGroup::initializedEvent()
{
	// Make sure all our children are also initialized (Due to unserialization)
	for(int i = 0; i < m_layers.count(); i++) {
		Layer *layer = m_layers.at(i);
		layer->setInitialized();
	}
}

void LayerGroup::showEvent()
{
	// Notify children of visibility change
	for(int i = 0; i < m_layers.count(); i++) {
		Layer *layer = m_layers.at(i);
		layer->parentShowEvent();
	}
}

void LayerGroup::hideEvent()
{
	// Notify children of visibility change
	for(int i = 0; i < m_layers.count(); i++) {
		Layer *layer = m_layers.at(i);
		layer->parentHideEvent();
	}
}
