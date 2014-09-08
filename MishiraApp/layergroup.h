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

#ifndef LAYERGROUP_H
#define LAYERGROUP_H

#include "layer.h"
#include <QtCore/QByteArray>
#include <QtCore/QVector>

class Profile;

typedef QVector<Layer *> LayerList;

//=============================================================================
class LayerGroup : public QObject
{
	friend class Layer;
	friend class Profile;

	Q_OBJECT

private: // Members -----------------------------------------------------------
	bool		m_isInitializing;
	Profile *	m_profile;
	QString		m_name;
	LayerList	m_layers;
	int			m_visibleRef;

private: // Constructor/destructor --------------------------------------------
	LayerGroup(Profile *profile);
	~LayerGroup();

public: // Methods ------------------------------------------------------------
	void		setInitialized();
	bool		isInitializing() const;
	Profile *	getProfile() const;
	quint32		getId();
	QString		getIdString(bool showName = true) const;
	void		setName(const QString &name);
	QString		getName() const;
	void		refVisible();
	void		derefVisible();
	bool		isVisibleSomewhere() const;
	bool		isShared() const;

	LayerList	getLayers() const;
	Layer *		createLayer(
		quint32 typeId, const QString &name = QString(), int before = -1);
	Layer *		createLayerSerialized(
		quint32 typeId, QDataStream *stream, int before = -1);
	void		destroyLayer(Layer *layer);
	Layer *		layerAtIndex(int index) const;
	int			indexOfLayer(Layer *layer) const;
	int			layerCount() const;
	bool		areAllLayersLoaded() const;
	void		loadAllLayers();
	bool		duplicateLayer(Layer *layer);

	void		serialize(QDataStream *stream) const;
	bool		unserialize(QDataStream *stream);

private:
	LayerList &	getLayersMutable();
	Layer *		constructLayer(quint32 typeId);

	void		initializedEvent();
	void		showEvent();
	void		hideEvent();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void		groupChanged(LayerGroup *group);
	void		layerAdded(Layer *layer, int before);
	void		destroyingLayer(Layer *layer);
	void		layerMoved(Layer *layer, int before, bool isAdd);
	void		layerChanged(Layer *layer);
};
//=============================================================================

inline bool LayerGroup::isInitializing() const
{
	return m_isInitializing;
}

inline Profile *LayerGroup::getProfile() const
{
	return m_profile;
}

inline QString LayerGroup::getName() const
{
	return m_name;
}

/// <summary>
/// WARNING: Don't use this to determine if the group is visible in a specific
/// scene.
/// </summary>
inline bool LayerGroup::isVisibleSomewhere() const
{
	return m_visibleRef > 0;
}

inline LayerList LayerGroup::getLayers() const
{
	return m_layers;
}

inline LayerList &LayerGroup::getLayersMutable()
{
	return m_layers;
}

inline int LayerGroup::indexOfLayer(Layer *layer) const
{
	return m_layers.indexOf(layer);
}

inline int LayerGroup::layerCount() const
{
	return m_layers.count();
}

#endif // LAYERGROUP_H
