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

#ifndef LAYER_H
#define LAYER_H

#include "common.h"
#include <QtCore/QByteArray>
#include <QtCore/QRect>

class LayerDialog;
class Scene;
class QDataStream;

//=============================================================================
/// <summary>
/// WARNING: While layers have a visibility flag they don't know if they are
/// actually visible as it depends on which scene is currently being rendered.
/// </summary>
class Layer : public QObject
{
	friend class LayerGroup;

	Q_OBJECT

protected: // Members ---------------------------------------------------------
	bool			m_isInitializing;
	LayerGroup *	m_parent;
	QString			m_name;
	bool			m_isLoaded;
	bool			m_isVisible;

	QRect			m_rect;
	QRect			m_visibleRect;
	int				m_rotationDeg;
	float			m_rotationRad;
	float			m_opacity;
	LyrScalingMode	m_scaling;
	LyrAlignment	m_alignment;

protected: // Constructor/destructor ------------------------------------------
	Layer(LayerGroup *parent);
	~Layer();

public: // Methods ------------------------------------------------------------
	void			setInitialized();
	bool			isInitializing() const;
	LayerGroup *	getParent() const;
	void			setName(const QString &name);
	QString			getName() const;
	QString			getIdString(bool showName = true) const;
	void			setLoaded(bool loaded);
	bool			isLoaded() const;
	void			reload();
	void			setVisible(bool visible);
	bool			isVisible() const;
	bool			isVisibleSomewhere() const;

	void			setRect(const QRect &rect);
	QRect			getRect() const;
	void			setVisibleRect(const QRect &rect);
	QRect			getVisibleRect() const;
	void			setRotationDeg(int degrees);
	int				getRotationDeg() const;
	float			getRotationRad() const;
	void			setOpacity(float opacity);
	float			getOpacity() const;
	void			setScaling(LyrScalingMode scaling);
	LyrScalingMode	getScaling() const;
	void			setAlignment(LyrAlignment alignment);
	LyrAlignment	getAlignment() const;

	void			moveTo(LayerGroup *group, int before = -1);

protected:
	QSize			getCanvasSize() const;
	QRect			scaledRectFromActualSize(const QSize &size) const;
	void			updateResourcesIfLoaded();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			showSettingsDialog();

public: // Interface ----------------------------------------------------------
	virtual void	initializeResources(VidgfxContext *gfx);
	virtual void	updateResources(VidgfxContext *gfx);
	virtual void	destroyResources(VidgfxContext *gfx);
	virtual void	render(
		VidgfxContext *gfx, Scene *scene, uint frameNum, int numDropped);

	virtual LyrType	getType() const = 0;

	virtual bool			hasSettingsDialog();
	virtual LayerDialog *	createSettingsDialog(QWidget *parent = NULL);

	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

protected:
	virtual void	initializedEvent();
	virtual void	loadEvent();
	virtual void	unloadEvent();
	virtual void	showEvent();
	virtual void	hideEvent();
	virtual void	parentShowEvent();
	virtual void	parentHideEvent();
};
//=============================================================================

inline bool Layer::isInitializing() const
{
	return m_isInitializing;
}

inline LayerGroup *Layer::getParent() const
{
	return m_parent;
}

inline QString Layer::getName() const
{
	return m_name;
}

inline bool Layer::isLoaded() const
{
	return m_isLoaded;
}

inline bool Layer::isVisible() const
{
	return m_isVisible;
}

inline QRect Layer::getRect() const
{
	return m_rect;
}

inline QRect Layer::getVisibleRect() const
{
	return m_visibleRect;
}

inline int Layer::getRotationDeg() const
{
	return m_rotationDeg;
}

inline float Layer::getRotationRad() const
{
	return m_rotationRad;
}

inline float Layer::getOpacity() const
{
	return m_opacity;
}

inline LyrScalingMode Layer::getScaling() const
{
	return m_scaling;
}

inline LyrAlignment Layer::getAlignment() const
{
	return m_alignment;
}

#endif // LAYER_H
