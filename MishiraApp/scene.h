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

#ifndef SCENE_H
#define SCENE_H

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QVector>

class GraphicsContext;
class Layer;
class LayerGroup;
class Profile;
class SceneItem;

typedef QVector<LayerGroup *> LayerGroupList;
typedef QVector<SceneItem *> SceneItemList;
typedef QHash<Layer *, SceneItem *> LayerSceneItemHash;

//=============================================================================
class Scene : public QObject
{
	friend class SceneItem;

	Q_OBJECT

private: // Datatypes ---------------------------------------------------------
	struct GroupInfo {
		LayerGroup *	group;
		SceneItem *		sceneItem;
		bool			isVisible;
	};
	typedef QVector<GroupInfo> GroupInfoList;

private: // Members -----------------------------------------------------------
	bool				m_isInitializing;
	bool				m_isDestructing;
	Profile *			m_profile;
	QString				m_name;
	GroupInfoList		m_groups;
	LayerSceneItemHash	m_layerSceneItems;
	bool				m_isVisible;
	SceneItem *			m_activeItem;

public: // Constructor/destructor ---------------------------------------------
	Scene(Profile *profile);
	~Scene();

public: // Methods ------------------------------------------------------------
	void			setInitialized();
	bool			isInitializing() const;
	Profile *		getProfile() const;
	void			setName(const QString &name);
	QString			getName() const;
	QString			getIdString(bool showName = true) const;
	void			setVisible(bool isVisible);
	bool			isVisible() const;

	// Layer groups
	void			addGroup(
		LayerGroup *group, bool isVisible, int before = -1);
	void			removeGroup(LayerGroup *group);
	LayerGroup *	groupAtIndex(int index) const;
	int				indexOfGroup(LayerGroup *group) const;
	int				groupCount() const;
	void			moveGroupTo(LayerGroup *group, int before = -1);
	void			setGroupVisible(LayerGroup *group, bool visible);
	bool			isGroupVisible(LayerGroup *group) const;

	// Scene items
	SceneItemList	getGroupSceneItems() const;
	SceneItemList	getSceneItemsForGroup(LayerGroup *group) const;
	SceneItem *		itemForGroup(LayerGroup *group) const;
	SceneItem *		itemForLayer(Layer *layer) const;
	int				listIndexOfItem(SceneItem *item) const;
	SceneItem *		itemAtListIndex(int index) const;
	int				listItemCount() const;
	void			setActiveItem(SceneItem *item);
	void			setActiveItemGroup(LayerGroup *group);
	void			setActiveItemLayer(Layer *layer);
	SceneItem *		getActiveItem() const;

	void			serialize(QDataStream *stream) const;
	bool			unserialize(QDataStream *stream);

	void			render(
		GraphicsContext *gfx, uint frameNum, int numDropped);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			activeItemChanged(SceneItem *item, SceneItem *prev);

	void			itemAdded(SceneItem *item);
	void			removingItem(SceneItem *item);
	void			itemChanged(SceneItem *item);
	void			itemsRearranged();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			groupChanged(LayerGroup *group);
	void			layerAdded(Layer *layer, int before);
	void			destroyingLayer(Layer *layer);
	void			layerMoved(Layer *layer, int before, bool isAdd);
	void			layerChanged(Layer *layer);
};
//=============================================================================

inline bool Scene::isInitializing() const
{
	return m_isInitializing;
}

inline Profile *Scene::getProfile() const
{
	return m_profile;
}

inline QString Scene::getName() const
{
	return m_name;
}

inline bool Scene::isVisible() const
{
	return m_isVisible;
}

inline void Scene::setActiveItem(SceneItem *item)
{
	if(m_activeItem == item)
		return; // No change
	SceneItem *prev = m_activeItem;
	m_activeItem = item;
	emit activeItemChanged(item, prev);
}

inline SceneItem *Scene::getActiveItem() const
{
	return m_activeItem;
}

#endif // SCENE_H
