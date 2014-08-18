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

#ifndef SCENEVIEW_H
#define SCENEVIEW_H

#include "sortableview.h"
#include "Widgets/styledsortablebar.h"
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QMenu>

class GripPattern;
class Layer;
class LayerGroup;
class Profile;
class Scene;
class StyledButton;
class QRadioButton;

//=============================================================================
class SceneViewBar : public StyledSortableBar
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Scene *			m_scene;
	QRadioButton *	m_activeBtn;
	StyledButton *	m_deleteBtn;
	GripPattern *	m_dragHandle;
	bool			m_freezeVisual;
	bool			m_sceneDeleted;

public: // Constructor/destructor ---------------------------------------------
	SceneViewBar(
		Scene *scene, const QColor &fgColor, const QColor &bgColor,
		const QColor &shadowColor, QWidget *parent = NULL);
	SceneViewBar(
		Scene *scene, const QColor &fgColor, const QColor &bgColor,
		const QColor &shadowColor, const QString &label,
		QWidget *parent = NULL);
	~SceneViewBar();

private:
	void	construct();

public: // Methods ------------------------------------------------------------
	Scene *			getScene() const;
	void			editName();

protected:
	void			updateActiveCheckState();

	virtual void	viewWidgetSet(SortableView *widget);
	virtual void	keyPressEvent(QKeyEvent *ev);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	sceneChanged(Scene *scene);
	void	activeSceneChanged(Scene *scene, Scene *prev);
	void	destroyingScene(Scene *scene);
	void	deleteClickEvent();
	void	leftClickEvent();
	void	rightClickEvent();
	void	nameEdited(const QString &text);
	void	dragBegun(const QPoint &startPos);
};
//=============================================================================

inline Scene *SceneViewBar::getScene() const
{
	return m_scene;
}

//=============================================================================
class SceneView : public SortableView
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Profile *		m_profile;
	bool			m_mouseDown;
	QButtonGroup	m_radioGroup;

	// Right-click scene context menu
	Scene *			m_contextScene;
	QMenu			m_contextSceneMenu;
	QAction *		m_duplicateSceneAction;
	QAction *		m_renameSceneAction;
	QAction *		m_deleteSceneAction;
	QAction *		m_moveSceneUpAction;
	QAction *		m_moveSceneDownAction;

public: // Constructor/destructor ---------------------------------------------
	SceneView(Profile *profile, QWidget *parent = NULL);
	~SceneView();

private: // Methods -----------------------------------------------------------
	SceneViewBar *	createSceneBar(Scene *scene);
	SceneViewBar *	barForScene(Scene *scene);

public:
	QButtonGroup *	getRadioGroup();
	void			showSceneContextMenu(Scene *scene);
	void			editSceneName(Scene *scene);

protected: // Interface -------------------------------------------------------
	virtual bool	moveChild(SortableWidget *child, int before);

public: // Events -------------------------------------------------------------
	virtual void	keyPressEvent(QKeyEvent *ev);
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	focusOutEvent(QFocusEvent *ev);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			sceneAdded(Scene *scene, int before);
	void			sceneMoved(Scene *scene, int before);
	void			sceneContextTriggered(QAction *action);
};
//=============================================================================

inline QButtonGroup *SceneView::getRadioGroup()
{
	return &m_radioGroup;
}

#endif // SCENEVIEW_H
