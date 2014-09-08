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

#ifndef SCENEITEMVIEW_H
#define SCENEITEMVIEW_H

#include "sortableview.h"
#include "Widgets/styledsortablebar.h"
#include <QtWidgets/QMenu>

class GripPattern;
class Layer;
class LayerGroup;
class Scene;
class SceneItem;
class StyledButton;
class VisibleCheckBox;
class QLabel;

typedef QVector<QAction *> QActionList;

//=============================================================================
class SceneItemViewBar : public StyledSortableBar
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	SceneItem *			m_item;
	QWidget *			m_loadedBar;
	VisibleCheckBox *	m_visibleBtn;
	StyledButton *		m_deleteBtn;
	StyledButton *		m_addBtn;
	QLabel *			m_sharedLbl;
	GripPattern *		m_dragHandle;
	bool				m_freezeVisual;
	bool				m_itemDeleted;

public: // Constructor/destructor ---------------------------------------------
	SceneItemViewBar(
		SceneItem *item, const QColor &fgColor, const QColor &bgColor,
		const QColor &shadowColor, int level, QWidget *parent = NULL);
	SceneItemViewBar(
		SceneItem *item, const QColor &fgColor, const QColor &bgColor,
		const QColor &shadowColor, const QString &label, int level,
		QWidget *parent = NULL);
	~SceneItemViewBar();

private:
	void	construct();
	void	constructGroup();
	void	constructLayer();

public: // Methods ------------------------------------------------------------
	SceneItem *		getItem() const;
	void			editName();

protected:
	void			updateLoadedBar();
	void			updateVisibleCheckState();
	Qt::CheckState	getVisibleCheckState() const;

	virtual void	viewWidgetSet(SortableView *widget);
	virtual void	keyPressEvent(QKeyEvent *ev);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	itemChanged(SceneItem *item);
	void	activeItemChanged(SceneItem *item, SceneItem *prev);
	void	removingItem(SceneItem *item);
	void	addClickEvent();
	void	deleteClickEvent();
	void	rightClickEvent();
	void	nameEdited(const QString &text);
	void	dragBegun(const QPoint &startPos);
};
//=============================================================================

inline SceneItem *SceneItemViewBar::getItem() const
{
	return m_item;
}

//=============================================================================
class SceneItemView : public SortableView
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Scene *			m_scene;
	bool			m_mouseDown;

	// "Add layer" popup menu
	LayerGroup *	m_addLayerGroup;
	QMenu			m_addLayerMenu;
	QActionList		m_addLayerActions;

	// Right-click layer context menu
	Layer *			m_contextLayer;
	QMenu			m_contextLayerMenu;
	QAction *		m_layerPropertiesAction;
	QAction *		m_duplicateLayerAction;
	QAction *		m_renameLayerAction;
	QAction *		m_deleteLayerAction;
	QAction *		m_moveLayerUpAction;
	QAction *		m_moveLayerDownAction;
	QAction *		m_reloadLayerAction;
	QAction *		m_reloadAllLayerAction;
	QAction *		m_loadLayerAction;
	QAction *		m_loadGroupLayerAction;
	QAction *		m_loadAllLayerAction;
	QAction *		m_unloadLayerAction;
	QAction *		m_unloadAllLayerAction;

	// Right-click group context menu
	LayerGroup *		m_contextGroup;
	QMenu				m_contextGroupMenu;
	QAction *			m_renameGroupAction;
	QAction *			m_deleteGroupAction;
	QAction *			m_moveGroupUpAction;
	QAction *			m_moveGroupDownAction;
	QAction *			m_reloadAllGroupAction;
	QAction *			m_loadGroupGroupAction;
	QAction *			m_loadAllGroupAction;
	QAction *			m_unloadAllGroupAction;
	QAction *			m_makeLocalGroupAction;
	QMenu *				m_shareWithGroupMenu;
	QVector<QAction *>	m_shareWithActions;

public: // Constructor/destructor ---------------------------------------------
	SceneItemView(Scene *scene, QWidget *parent = NULL);
	~SceneItemView();

private: // Methods -----------------------------------------------------------
	SceneItemViewBar *	createGroupBar(LayerGroup *group);
	SceneItemViewBar *	createLayerBar(Layer *layer);
	SceneItemViewBar *	barForItem(SceneItem *item);

public:
	void				setScene(Scene *scene);

	void				showAddLayerMenu(LayerGroup *group);
	void				showLayerContextMenu(Layer *layer);
	void				showGroupContextMenu(LayerGroup *group);
	void				editLayerName(SceneItem *item);

protected: // Interface -------------------------------------------------------
	virtual bool		moveChild(SortableWidget *child, int before);

public: // Events -------------------------------------------------------------
	virtual void		keyPressEvent(QKeyEvent *ev);
	virtual void		mousePressEvent(QMouseEvent *ev);
	virtual void		mouseReleaseEvent(QMouseEvent *ev);
	virtual void		focusOutEvent(QFocusEvent *ev);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void				itemAdded(SceneItem *item);
	void				itemsRearranged();
	void				addLayerTriggered(QAction *action);
	void				layerContextTriggered(QAction *action);
	void				groupContextTriggered(QAction *action);
	void				groupShareWithTriggered(QAction *action);
};
//=============================================================================

#endif // SCENEITEMVIEW_H
