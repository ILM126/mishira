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

#ifndef LAYERPROPERTIES_H
#define LAYERPROPERTIES_H

#include "ui_layerproperties.h"
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QWidget>

class Profile;
class Scene;
class SceneItem;

//=============================================================================
class LayerProperties : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_LayerProperties	m_ui;
	Profile *			m_profile;
	QButtonGroup		m_alignGroup;
	bool				m_ignoreSignals;

public: // Constructor/destructor ---------------------------------------------
	LayerProperties(Profile *profile, QWidget *parent = NULL);
	~LayerProperties();

private:
	void		setAllEnabled(bool enabled);
	void		addShadowEffect(QWidget *widget);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void		lineEditChanged(const QString &text);
	void		scalingChanged(int id);
	void		alignmentChanged(int id);
	void		morePropertiesClicked();

	public
Q_SLOTS:
	void		activeSceneChanged(Scene *scene, Scene *oldScene);
	void		itemChanged(SceneItem *item);
	void		activeItemChanged(SceneItem *item);
};
//=============================================================================

#endif // LAYERPROPERTIES_H
