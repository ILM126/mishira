//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 2014 Lucas Murray <lmurray@undefinedfire.com>
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

#ifndef SCENEITEM_H
#define SCENEITEM_H

#include <QtCore/QObject>
#include <QtCore/QString>

class Layer;
class LayerGroup;
class Scene;

//=============================================================================
/// <summary>
/// Abstracts layers and layer groups so that the same interface can be used in
/// UI widgets.
/// </summary>
class SceneItem : public QObject
{
	friend class Scene;

	Q_OBJECT

protected: // Members ---------------------------------------------------------
	Scene *			m_scene;
	bool			m_isGroup;
	Layer *			m_layer;
	LayerGroup *	m_group;

protected: // Constructor/destructor ------------------------------------------
	SceneItem(Scene *scene, Layer *layer);
	SceneItem(Scene *scene, LayerGroup *group);
	virtual ~SceneItem();

public: // Methods ------------------------------------------------------------
	Scene *			getScene() const;
	bool			isGroup();
	Layer *			getLayer();
	LayerGroup *	getGroup();

	void	setName(const QString &name);
	QString	getName() const;
	void	setLoaded(bool loaded);
	bool	isLoaded() const;
	void	setVisible(bool visible);
	bool	isVisible() const;
	bool	isVisibleRecursive() const;
	bool	wouldBeVisibleRecursive() const;
	bool	isShared() const;
	void	deleteItem();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	toggleVisible();
	void	makeActiveItem();
};
//=============================================================================

inline Scene *SceneItem::getScene() const
{
	return m_scene;
}

inline bool SceneItem::isGroup()
{
	return m_isGroup;
}

#endif // SCENEITEM_H
