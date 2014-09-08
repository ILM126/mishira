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

#include "sceneitemview.h"
#include "application.h"
#include "borderspacer.h"
#include "layer.h"
#include "layerfactory.h"
#include "layergroup.h"
#include "profile.h"
#include "scene.h"
#include "sceneitem.h"
#include "Widgets/patternwidgets.h"
#include "Widgets/styledtogglebutton.h"
#include "Widgets/visiblecheckbox.h"
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollBar>

//=============================================================================
// SceneItemViewBar

SceneItemViewBar::SceneItemViewBar(
	SceneItem *item, const QColor &fgColor, const QColor &bgColor,
	const QColor &shadowColor, int level, QWidget *parent)
	: StyledSortableBar(fgColor, bgColor, shadowColor, level, parent)
	, m_item(item)
	, m_loadedBar(NULL)
	, m_visibleBtn(NULL)
	, m_deleteBtn(NULL)
	, m_addBtn(NULL)
	, m_dragHandle(NULL)
	, m_freezeVisual(false)
	, m_itemDeleted(false)
{
	construct();
}

SceneItemViewBar::SceneItemViewBar(
	SceneItem *item, const QColor &fgColor, const QColor &bgColor,
	const QColor &shadowColor, const QString &label, int level,
	QWidget *parent)
	: StyledSortableBar(fgColor, bgColor, shadowColor, label, level, parent)
	, m_item(item)
	, m_loadedBar(NULL)
	, m_visibleBtn(NULL)
	, m_deleteBtn(NULL)
	, m_addBtn(NULL)
	, m_sharedLbl(NULL)
	, m_dragHandle(NULL)
	, m_freezeVisual(false)
	, m_itemDeleted(false)
{
	construct();
}

SceneItemViewBar::~SceneItemViewBar()
{
	delete m_loadedBar;
	delete m_visibleBtn;
	delete m_deleteBtn;
	delete m_addBtn;
	delete m_sharedLbl;
	delete m_dragHandle;
}

void SceneItemViewBar::construct()
{
	// Add a dark line between layers
	setBorders(0, 1);

	// Set the layer rename box colours
	m_label.setEditColors(Qt::white, Qt::black, Qt::black);
#if 0
	m_label.setEditColors(
		StyleHelper::DarkBg3Color.darker(LINE_EDIT_FOCUS_DARKEN_AMOUNT),
		StyleHelper::DarkFg2Color,
		Qt::black);
#endif // 0

	// Set label starting colour
	QColor col = StyleHelper::DarkFg2Color;
	if(!m_item->isVisibleRecursive())
		col.setAlpha(102); // 40%
	m_label.setTextColor(col);

	//-------------------------------------------------------------------------
	// Add widgets that are common to both groups and layers

	// Loaded indicator bar
	m_loadedBar = new QWidget(this);
	m_loadedBar->setFixedWidth(5);
	m_loadedBar->setAutoFillBackground(true);
	updateLoadedBar();
	addLeftWidget(m_loadedBar);

	// Dark line separation and fixed padding
	addLeftWidget(new HBorderSpacer(this, 1));
	QWidget *spacer = new QWidget(this);
	spacer->setFixedSize(5, 0);
	addLeftWidget(spacer);

	// Visible checkbox
	m_visibleBtn = new VisibleCheckBox(m_item, this);
	App->applyDarkStyle(m_visibleBtn);
	updateVisibleCheckState(); // Must be after `applyDarkStyle()`
	addLeftWidget(m_visibleBtn);
	connect(m_visibleBtn, &QAbstractButton::clicked,
		m_item, &SceneItem::toggleVisible);

	// Delete button
	m_deleteBtn = new StyledButton(StyleHelper::TransparentSet, false, this);
	m_deleteBtn->setIcon(QIcon(":/Resources/white-cross.png"));
	m_deleteBtn->setHoverIcon(QIcon(":/Resources/white-cross-hover.png"));
	m_deleteBtn->setIconSize(QSize(10, 10));
	m_deleteBtn->setFixedSize(STYLED_BAR_HEIGHT, STYLED_BAR_HEIGHT);
	m_deleteBtn->hide(); // Hide by default
	if(m_item->isGroup())
		m_deleteBtn->setToolTip(tr("Delete layer group"));
	else
		m_deleteBtn->setToolTip(tr("Delete scene layer"));
	addRightLabelWidget(m_deleteBtn);
	connect(m_deleteBtn, &StyledButton::clicked,
		this, &SceneItemViewBar::deleteClickEvent);

	// Group shared icon. We can't do this at the "type-specific" part below as
	// we need to position it between the delete button and grip widget
	if(m_item->isGroup()) {
		m_sharedLbl = new QLabel(this);
		m_sharedLbl->setPixmap(QPixmap(":/Resources/group-shared.png"));
		m_sharedLbl->setFixedSize(12 + 3, STYLED_BAR_HEIGHT);
		m_sharedLbl->setToolTip(tr("Shared between scenes"));
		if(!m_item->isShared())
			m_sharedLbl->hide();
		addRightLabelWidget(m_sharedLbl);
	}

	// Grip pattern and spacer
	m_dragHandle = new GripPattern(true, this);
	m_dragHandle->setFixedSize(17, 10);
	if(m_item->isGroup())
		m_dragHandle->setToolTip(tr("Drag to rearrange layer groups"));
	else
		m_dragHandle->setToolTip(tr("Drag to rearrange scene layers"));
	addRightLabelWidget(m_dragHandle);
	spacer = new QWidget(this);
	spacer->setFixedSize(5, 0);
	addRightLabelWidget(spacer);
	connect(m_dragHandle, &GripPattern::dragBegun,
		this, &SceneItemViewBar::dragBegun);

	//-------------------------------------------------------------------------

	// Add type-specific bar elements
	if(m_item->isGroup())
		constructGroup();
	else
		constructLayer();

	// Select the active layer if there is already one
	activeItemChanged(m_item->getScene()->getActiveItem(), NULL);

	// Clicking the label selects the item and editing the label changes the
	// layer's name
	connect(this, &SceneItemViewBar::clicked,
		m_item, &SceneItem::makeActiveItem);
	connect(this, &SceneItemViewBar::labelEdited,
		this, &SceneItemViewBar::nameEdited);

	// Watch scene for changes
	Scene *scene = m_item->getScene();
	connect(scene, &Scene::itemChanged,
		this, &SceneItemViewBar::itemChanged);
	connect(scene, &Scene::activeItemChanged,
		this, &SceneItemViewBar::activeItemChanged);
	connect(scene, &Scene::removingItem,
		this, &SceneItemViewBar::removingItem);
}

void SceneItemViewBar::constructGroup()
{
	// Make the list look like a tree
	setLabelIndent(5);

	// Groups have a bold font
	setLabelBold(true);

	// Dark line separation
	addRightWidget(new HBorderSpacer(this, 1));

	// Add "add new layer" button
	m_addBtn = new StyledButton(StyleHelper::BlueSet, false);
	m_addBtn->setFixedSize(STYLED_BAR_HEIGHT, STYLED_BAR_HEIGHT);
	m_addBtn->setIcon(QIcon(":/Resources/white-plus.png"));
	m_addBtn->setToolTip(tr("Add a new scene layer"));
	addRightWidget(m_addBtn);
	connect(m_addBtn, &StyledButton::clicked,
		this, &SceneItemViewBar::addClickEvent);
}

void SceneItemViewBar::constructLayer()
{
	// Make the list look like a tree
	setLabelIndent(18);

	// Double clicking the label opens the settings dialog
	connect(this, &SceneItemViewBar::doubleClicked, m_item->getLayer(),
		&Layer::showSettingsDialog);
}

void SceneItemViewBar::editName()
{
	// Immediately enter edit mode and give the widget focus
	setEditableLabel(true, true);
}

void SceneItemViewBar::updateLoadedBar()
{
	QPalette pal = m_loadedBar->palette();
	if(m_item->isLoaded()) {
		pal.setColor(
			m_loadedBar->backgroundRole(), StyleHelper::GreenBaseColor);
		if(m_item->isGroup()) {
			m_loadedBar->setToolTip(tr(
				"Layer group is loaded into memory"));
		} else {
			m_loadedBar->setToolTip(tr(
				"Layer is loaded into memory"));
		}
	} else {
		pal.setColor(
			m_loadedBar->backgroundRole(), StyleHelper::RedBaseColor);
		if(m_item->isGroup()) {
			m_loadedBar->setToolTip(tr(
				"Layer group is not loaded into memory"));
		} else {
			m_loadedBar->setToolTip(tr(
				"Layer is not loaded into memory"));
		}
	}
	m_loadedBar->setPalette(pal);
}

void SceneItemViewBar::updateVisibleCheckState()
{
	Qt::CheckState curState = getVisibleCheckState();
	m_visibleBtn->setCheckState(curState);
	if(m_item->isGroup()) {
		switch(curState) {
		default:
		case Qt::PartiallyChecked:
		case Qt::Unchecked:
			m_visibleBtn->setToolTip(tr(
				"Layer group is not visible"));
			break;
		case Qt::Checked:
			m_visibleBtn->setToolTip(tr(
				"Layer group is visible"));
			break;
		}
	} else {
		switch(curState) {
		default:
		case Qt::Unchecked:
			m_visibleBtn->setToolTip(tr(
				"Layer is not visible"));
			break;
		case Qt::PartiallyChecked:
			m_visibleBtn->setToolTip(tr(
				"Layer is masked by group visibility"));
			break;
		case Qt::Checked:
			m_visibleBtn->setToolTip(tr(
				"Layer is visible"));
			break;
		}
	}
}

Qt::CheckState SceneItemViewBar::getVisibleCheckState() const
{
	if(!m_item->isVisible())
		return Qt::Unchecked;
	if(m_item->isVisibleRecursive())
		return Qt::Checked;
	return Qt::PartiallyChecked;
}

void SceneItemViewBar::viewWidgetSet(SortableView *widget)
{
	if(widget == NULL)
		return;

	// Connect right-click signal to open the context menu making sure we never
	// connect the same signal multiple times
	connect(this, &SceneItemViewBar::rightClicked,
		this, &SceneItemViewBar::rightClickEvent, Qt::UniqueConnection);
}

void SceneItemViewBar::keyPressEvent(QKeyEvent *ev)
{
	ev->ignore();
	switch(ev->key()) {
	default:
		break;
	case Qt::Key_Up:
	case Qt::Key_Down:
		// Eat up/down keys when in edit mode so our parent view doesn't act up
		// when the focus changes uncleanly
		if(m_label.isInEditMode())
			ev->accept();
		break;
	}
}

void SceneItemViewBar::itemChanged(SceneItem *item)
{
	if(m_freezeVisual)
		return; // UI is frozen for changes
	if(item != m_item)
		return; // Not us

	// Update label
	setLabel(item->getName());

	// Update shared state
	if(m_sharedLbl != NULL) {
		if(m_item->isShared())
			m_sharedLbl->show();
		else
			m_sharedLbl->hide();
	}

	// Common widgets
	updateLoadedBar();
	updateVisibleCheckState();
	QColor col = StyleHelper::DarkFg2Color;
	if(!item->isVisibleRecursive())
		col.setAlpha(102); // 40%
	m_label.setTextColor(col);
}

void SceneItemViewBar::activeItemChanged(SceneItem *item, SceneItem *prev)
{
	if(m_freezeVisual)
		return; // UI is frozen for changes
	if(m_item == item) {
		// We were selected
		setEditableLabel(true);
		m_deleteBtn->show();

		// Type-specific elements
		if(m_item->isGroup())
			setBgColor(StyleHelper::DarkBg5SelectedColor);
		else
			setBgColor(StyleHelper::DarkBg3SelectedColor);

		// Make sure we are visible to the user
		if(m_viewWidget != NULL)
			m_viewWidget->ensureWidgetVisible(this, 0, 0);
	} else if(m_item == prev) {
		// We were deselected
		setEditableLabel(false);
		m_deleteBtn->hide();

		// Type-specific elements
		if(m_item->isGroup())
			setBgColor(StyleHelper::DarkBg5Color);
		else
			setBgColor(StyleHelper::DarkBg3Color);
	}
}

void SceneItemViewBar::removingItem(SceneItem *item)
{
	if(item != m_item)
		return; // Not us

	// Prevent the UI from changing states as the item is destroyed
	m_freezeVisual = true;

	// Begin delete animation, TODO: Never actually done
	m_itemDeleted = true;

	// Just remove immediately from the view for now, TODO
	m_viewWidget->removeSortable(this);
	delete this;
}

void SceneItemViewBar::addClickEvent()
{
	SceneItemView *view = static_cast<SceneItemView *>(m_viewWidget);
	view->showAddLayerMenu(m_item->getGroup());
}

void SceneItemViewBar::deleteClickEvent()
{
	// Show confirmation dialog
	App->showDeleteLayerDialog(m_item);
}

void SceneItemViewBar::rightClickEvent()
{
	SceneItemView *view = static_cast<SceneItemView *>(m_viewWidget);
	if(m_item->isGroup())
		view->showGroupContextMenu(m_item->getGroup());
	else
		view->showLayerContextMenu(m_item->getLayer());
}

void SceneItemViewBar::nameEdited(const QString &text)
{
	m_item->setName(text.trimmed());
	setLabel(m_item->getName()); // Just in case it was an invalid name
}

/// <summary>
/// Called when the user has initiated a drag using the grip widget.
/// </summary>
void SceneItemViewBar::dragBegun(const QPoint &startPos)
{
	// Create a new drag object that our view will recognise
	QDrag *drag = createDragObject();

	// Render the widget as a pixmap with transparency
	QPixmap grabbed = grab();
	QPixmap pixmap = QPixmap(grabbed.size() - QSize(0, 1));
	pixmap.fill(QColor(0, 0, 0, 0));
	QPainter p;
	p.begin(&pixmap);
	p.setOpacity(0.5f);
	p.drawPixmap(0, 0, grabbed);
	p.end();
	drag->setPixmap(pixmap);
	drag->setHotSpot(m_dragHandle->pos() + m_labelWidget.pos() + startPos);

	// Execute the drag. WARNING: This blocks until the user drops!
	m_viewWidget->dragExec(drag, Qt::MoveAction);
	// The view has now already processed the drop if it was valid
}

//=============================================================================
// SceneItemView

SceneItemView::SceneItemView(Scene *scene, QWidget *parent)
	: SortableView("application/x-mishira-scene-item", parent)
	, m_scene(NULL)
	, m_mouseDown(false)

	// "Add layer" popup menu
	, m_addLayerGroup(NULL)
	, m_addLayerMenu(this)
	, m_addLayerActions()

	// Right-click layer context menu
	, m_contextLayer(NULL)
	, m_contextLayerMenu(this)
	, m_layerPropertiesAction(NULL)
	, m_duplicateLayerAction(NULL)
	, m_renameLayerAction(NULL)
	, m_deleteLayerAction(NULL)
	, m_moveLayerUpAction(NULL)
	, m_moveLayerDownAction(NULL)
	, m_reloadLayerAction(NULL)
	, m_reloadAllLayerAction(NULL)
	, m_loadLayerAction(NULL)
	, m_loadGroupLayerAction(NULL)
	, m_loadAllLayerAction(NULL)
	, m_unloadLayerAction(NULL)
	, m_unloadAllLayerAction(NULL)

	// Right-click group context menu
	, m_contextGroup(NULL)
	, m_contextGroupMenu(this)
	, m_renameGroupAction(NULL)
	, m_deleteGroupAction(NULL)
	, m_moveGroupUpAction(NULL)
	, m_moveGroupDownAction(NULL)
	, m_reloadAllGroupAction(NULL)
	, m_loadGroupGroupAction(NULL)
	, m_loadAllGroupAction(NULL)
	, m_unloadAllGroupAction(NULL)
	, m_makeLocalGroupAction(NULL)
	, m_shareWithGroupMenu(NULL)
	, m_shareWithActions()
{
	App->applyDarkStyle(this);
	App->applyDarkStyle(verticalScrollBar());
	App->applyDarkStyle(horizontalScrollBar());

	// Populate "add layer" popup menu
	LayerFactoryList factories = App->getLayerFactories();
	for(int i = 0; i < factories.size(); i++) {
		LayerFactory *factory = factories.at(i);
		QAction *action =
			m_addLayerMenu.addAction(factory->getAddLayerString());
		m_addLayerActions.append(action);
	}
	connect(&m_addLayerMenu, &QMenu::triggered,
		this, &SceneItemView::addLayerTriggered);

	// Populate right-click layer context menu
	m_layerPropertiesAction = m_contextLayerMenu.addAction(tr(
		"Layer options..."));
	m_contextLayerMenu.addSeparator();
	m_moveLayerUpAction = m_contextLayerMenu.addAction(tr(
		"Move layer up"));
	m_moveLayerDownAction = m_contextLayerMenu.addAction(tr(
		"Move layer down"));
	m_contextLayerMenu.addSeparator();
	m_reloadLayerAction = m_contextLayerMenu.addAction(tr(
		"Reload layer"));
	m_reloadAllLayerAction = m_contextLayerMenu.addAction(tr(
		"Reload all loaded layers"));
	m_loadLayerAction = m_contextLayerMenu.addAction(tr(
		"Load layer into memory"));
	m_unloadLayerAction = m_contextLayerMenu.addAction(tr(
		"Unload layer from memory"));
	m_loadGroupLayerAction = m_contextLayerMenu.addAction(tr(
		"Load all layers in this group into memory"));
	m_loadAllLayerAction = m_contextLayerMenu.addAction(tr(
		"Load all layers into memory"));
	m_unloadAllLayerAction = m_contextLayerMenu.addAction(tr(
		"Unload all invisible layers from memory"));
	m_contextLayerMenu.addSeparator();
	m_duplicateLayerAction = m_contextLayerMenu.addAction(tr(
		"Duplicate layer"));
	m_renameLayerAction = m_contextLayerMenu.addAction(tr(
		"Rename layer"));
	m_deleteLayerAction = m_contextLayerMenu.addAction(tr(
		"Delete layer..."));
	connect(&m_contextLayerMenu, &QMenu::triggered,
		this, &SceneItemView::layerContextTriggered);

	// Populate right-click group context menu
	m_moveGroupUpAction = m_contextGroupMenu.addAction(tr(
		"Move group up"));
	m_moveGroupDownAction = m_contextGroupMenu.addAction(tr(
		"Move group down"));
	m_contextGroupMenu.addSeparator();
	m_reloadAllGroupAction = m_contextGroupMenu.addAction(tr(
		"Reload all loaded layers"));
	m_loadGroupGroupAction = m_contextGroupMenu.addAction(tr(
		"Load all layers in this group into memory"));
	m_loadAllGroupAction = m_contextGroupMenu.addAction(tr(
		"Load all layers into memory"));
	m_unloadAllGroupAction = m_contextGroupMenu.addAction(tr(
		"Unload all invisible layers from memory"));
	m_contextGroupMenu.addSeparator();
	m_makeLocalGroupAction = m_contextGroupMenu.addAction(tr(
		"Make local copy of shared group"));
	m_shareWithGroupMenu = m_contextGroupMenu.addMenu(tr(
		"Share group with..."));
	m_contextGroupMenu.addSeparator();
	m_renameGroupAction = m_contextGroupMenu.addAction(tr(
		"Rename group"));
	m_deleteGroupAction = m_contextGroupMenu.addAction(tr(
		"Delete group"));
	connect(&m_contextGroupMenu, &QMenu::triggered,
		this, &SceneItemView::groupContextTriggered);
	connect(m_shareWithGroupMenu, &QMenu::triggered,
		this, &SceneItemView::groupShareWithTriggered);

	setScene(scene);
}

SceneItemView::~SceneItemView()
{
}

void SceneItemView::setScene(Scene *scene)
{
	if(m_scene == scene)
		return; // No change

	// Disconnect from previous scene and delete all items
	if(m_scene != NULL) {
		disconnect(m_scene, &Scene::itemAdded,
			this, &SceneItemView::itemAdded);
		disconnect(m_scene, &Scene::itemsRearranged,
			this, &SceneItemView::itemsRearranged);

		// Delete all items
		QVector<SortableWidget *> sortables = getSortables();
		for(int i = 0; i < sortables.count(); i++) {
			SceneItemViewBar *widget =
				static_cast<SceneItemViewBar *>(sortables.at(i));
			removeSortable(widget);
			delete widget;
		}
	}
	m_scene = scene;

	// Populate view from existing scene
	SceneItemList groups = m_scene->getGroupSceneItems();
	for(int i = 0; i < groups.count(); i++) {
		LayerGroup *group = groups.at(i)->getGroup();
		addSortable(createGroupBar(group));

		LayerList layers = group->getLayers();
		for(int j = 0; j < layers.count(); j++) {
			Layer *layer = layers.at(j);
			addSortable(createLayerBar(layer));
		}
	}

	// Watch scene for additions and moves
	connect(m_scene, &Scene::itemAdded,
		this, &SceneItemView::itemAdded);
	connect(m_scene, &Scene::itemsRearranged,
		this, &SceneItemView::itemsRearranged);
}

SceneItemViewBar *SceneItemView::createGroupBar(LayerGroup *group)
{
	SceneItem *item = m_scene->itemForGroup(group);
	if(item == NULL)
		return NULL;
	SceneItemViewBar *widget = new SceneItemViewBar(
		item, StyleHelper::DarkFg2Color, StyleHelper::DarkBg5Color,
		StyleHelper::TextShadowColor, group->getName(), 0, this);
	return widget;
}

SceneItemViewBar *SceneItemView::createLayerBar(Layer *layer)
{
	SceneItem *item = m_scene->itemForLayer(layer);
	if(item == NULL)
		return NULL;
	SceneItemViewBar *widget = new SceneItemViewBar(
		item, StyleHelper::DarkFg2Color, StyleHelper::DarkBg3Color,
		StyleHelper::TextShadowColor, layer->getName(), 1, this);
	return widget;
}

SceneItemViewBar *SceneItemView::barForItem(SceneItem *item)
{
	QVector<SortableWidget *> sortables = getSortables();
	for(int i = 0; i < sortables.count(); i++) {
		SceneItemViewBar *widget =
			static_cast<SceneItemViewBar *>(sortables.at(i));
		if(widget->getItem() != item)
			continue;
		return widget;
	}
	return NULL;
}

void SceneItemView::showAddLayerMenu(LayerGroup *group)
{
	m_addLayerGroup = group;
	m_addLayerMenu.popup(QCursor::pos());
}

void SceneItemView::showLayerContextMenu(Layer *layer)
{
	m_contextLayer = layer;
	if(layer->isLoaded()) {
		m_reloadLayerAction->setVisible(true);
		m_loadLayerAction->setVisible(false);
		m_unloadLayerAction->setVisible(true);
	} else {
		m_reloadLayerAction->setVisible(false);
		m_loadLayerAction->setVisible(true);
		m_unloadLayerAction->setVisible(false);
	}
	m_contextLayerMenu.popup(QCursor::pos());
}

void SceneItemView::showGroupContextMenu(LayerGroup *group)
{
	m_contextGroup = group;
	if(group->isShared())
		m_makeLocalGroupAction->setVisible(true);
	else
		m_makeLocalGroupAction->setVisible(false);

	// Populate "share with" menu
	m_shareWithGroupMenu->clear();
	m_shareWithActions.clear();
	const SceneList &scenes = App->getProfile()->getScenes();
	m_shareWithActions.reserve(scenes.count());
	for(int i = 0; i < scenes.size(); i++) {
		Scene *scene = scenes.at(i);
		QAction *action =
			m_shareWithGroupMenu->addAction(scene->getName());
		m_shareWithActions.append(action);

		// Disable actions for scenes that already have the group in them
		const SceneItemList &items = scene->getGroupSceneItems();
		for(int i = 0; i < items.size(); i++) {
			if(items.at(i)->getGroup() == m_contextGroup) {
				// Group already in selected scene
				action->setEnabled(false);
				break;
			}
		}
	}

	m_contextGroupMenu.popup(QCursor::pos());
}

void SceneItemView::editLayerName(SceneItem *item)
{
	SceneItemViewBar *widget = barForItem(item);
	if(widget == NULL)
		return;
	widget->editName();
}

bool SceneItemView::moveChild(SortableWidget *child, int before)
{
	// Called when the view requests an item be moved via drag-and-drop.

	SceneItemViewBar *widget = static_cast<SceneItemViewBar *>(child);
	SceneItem *item = widget->getItem();
	SceneItem *beforeItem = m_scene->itemAtListIndex(before);
	if(item->isGroup()) {
		// Moving a group
		LayerGroup *group = item->getGroup();
		if(beforeItem == NULL) {
			// Moving to the end of the list
			m_scene->moveGroupTo(group, -1);
		} else if(beforeItem->isGroup()) {
			// Moving before a group
			LayerGroup *beforeGroup = beforeItem->getGroup();
			m_scene->moveGroupTo(group, m_scene->indexOfGroup(beforeGroup));
		} else {
			// Should never be able to insert a group within another group
			return false;
		}
	} else {
		// Moving a layer
		Layer *layer = item->getLayer();
		if(beforeItem == NULL) {
			// Moving to the end of the list
			LayerGroup *group =
				m_scene->groupAtIndex(m_scene->groupCount() - 1);
			layer->moveTo(group, -1);
		} else if(beforeItem->isGroup()) {
			// Moving before a group which means we want to be added to the end
			// of the previous group
			LayerGroup *beforeGroup = beforeItem->getGroup();
			int groupIndex = m_scene->indexOfGroup(beforeGroup);
			if(groupIndex == 0)
				return false; // Should not be able to insert at the top
			layer->moveTo(m_scene->groupAtIndex(groupIndex - 1), -1);
		} else {
			// Moving before another layer
			Layer *beforeLayer = beforeItem->getLayer();
			LayerGroup *group = beforeLayer->getParent();
			layer->moveTo(group, group->indexOfLayer(beforeLayer));
		}
	}
	return true;
}

void SceneItemView::keyPressEvent(QKeyEvent *ev)
{
	ev->ignore();
	switch(ev->key()) {
	default:
		break;
	case Qt::Key_Up: {
		// Select the next item above the current one
		SceneItem *item = m_scene->getActiveItem();
		if(item == NULL)
			break;
		ev->accept();
		if(item->isGroup()) {
			// Select the last item in the previous group
			int index = m_scene->indexOfGroup(item->getGroup());
			if(index == 0)
				break; // We are the top group
			LayerGroup *group = m_scene->groupAtIndex(index - 1);
			if(group->layerCount())
				m_scene->setActiveItemLayer(group->getLayers().last());
			else // No layers in group, select the group itself
				m_scene->setActiveItemGroup(group);
		} else {
			// Select the layer above or the parent group
			Layer *layer = item->getLayer();
			LayerGroup *group = layer->getParent();
			int index = group->indexOfLayer(layer);
			if(index > 0)
				m_scene->setActiveItemLayer(group->layerAtIndex(index - 1));
			else // We are the top layer, select the group itself
				m_scene->setActiveItemGroup(group);
		}
		break; }
	case Qt::Key_Down: {
		// Select the next item below the current one
		SceneItem *item = m_scene->getActiveItem();
		if(item == NULL)
			break;
		ev->accept();
		if(item->isGroup()) {
			// Select the first item in this group
			LayerGroup *group = item->getGroup();
			if(group->layerCount() > 0)
				m_scene->setActiveItemLayer(group->getLayers().first());
			else {
				// No layers in group, select the next group
				int index = m_scene->indexOfGroup(group);
				if(index >= m_scene->groupCount() - 1)
					break; // We are the bottom group
				m_scene->setActiveItemGroup(m_scene->groupAtIndex(index + 1));
			}
		} else {
			// Select the layer below or the next group item
			Layer *layer = item->getLayer();
			LayerGroup *group = layer->getParent();
			int index = group->indexOfLayer(layer);
			if(index < group->getLayers().count() - 1)
				m_scene->setActiveItemLayer(group->layerAtIndex(index + 1));
			else {
				// We are the bottom layer, select the next group if one exists
				int index = m_scene->indexOfGroup(group);
				if(index >= m_scene->groupCount() - 1)
					break; // We are the bottom group
				m_scene->setActiveItemGroup(m_scene->groupAtIndex(index + 1));
			}
		}
		break; }
	case Qt::Key_F2:
		// Edit the selected layer's name
		SceneItem *item = m_scene->getActiveItem();
		if(item == NULL)
			break;
		SceneItemViewBar *bar = barForItem(item);
		bar->editName();
		ev->accept();
		break;
	}
}

void SceneItemView::mousePressEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	// Only accept the event if it's inside the widget rectangle
	if(!rect().contains(ev->pos())) {
		ev->ignore();
		return;
	}

	m_mouseDown = true;
	ev->accept();
}

void SceneItemView::mouseReleaseEvent(QMouseEvent *ev)
{
	// We're only interested in the left mouse button
	if(ev->button() != Qt::LeftButton) {
		ev->ignore();
		return;
	}

	// Only accept the event if it's inside the widget rectangle. We always
	// receive release events if the user first pressed the mouse button inside
	// of our widget area.
	if(!rect().contains(ev->pos())) {
		ev->ignore();
		return;
	}

	// Only consider the release as a click if the user initially pressed the
	// button on this widget as well
	if(!m_mouseDown) {
		ev->ignore();
		return;
	}

	// The user clicked the widget, deselect the active layer
	m_mouseDown = false;
	ev->accept();
	m_scene->setActiveItem(NULL);
}

void SceneItemView::focusOutEvent(QFocusEvent *ev)
{
	m_mouseDown = false;
	SortableView::focusOutEvent(ev);
}

void SceneItemView::itemAdded(SceneItem *item)
{
	// Get linear list index
	int before = m_scene->listIndexOfItem(item);

	if(item->isGroup()) {
		LayerGroup *group = item->getGroup();
		addSortable(createGroupBar(group), before);
	} else {
		Layer *layer = item->getLayer();
		addSortable(createLayerBar(layer), before);
	}
	item->makeActiveItem();
	// TODO: Edit name immediately
}

void SceneItemView::itemsRearranged()
{
	// As moving individual items is error-prone we just regenerate the entire
	// list at once.

	// Get widget list with the new order
	QVector<SortableWidget *> sortables = getSortables();
	QVector<SceneItemViewBar *> newSortables;
	newSortables.resize(sortables.count());
	for(int i = 0; i < sortables.count(); i++) {
		SceneItemViewBar *widget =
			static_cast<SceneItemViewBar *>(sortables.at(i));
		SceneItem *item = widget->getItem();
		int index = item->getScene()->listIndexOfItem(item);
		newSortables[index] = widget;
	}

	// Regenerate list
	for(int i = 0; i < sortables.count(); i++)
		removeSortable(sortables.at(i));
	for(int i = 0; i < newSortables.count(); i++)
		addSortable(newSortables.at(i));
}

void SceneItemView::addLayerTriggered(QAction *action)
{
	// Determine position to insert layer at
	int before;
	SceneItem *item = App->getActiveItem();
	if(item != NULL) {
		LayerGroup *group;
		if(item->isGroup()) {
			// Selected layer is a group, ignore and insert at the top
			before = 0;
		} else {
			Layer *layer = item->getLayer();
			group = layer->getParent();
			if(m_addLayerGroup == group) {
				// Selected layer is in our specified group, insert the new
				// layer above the selected one
				before = group->indexOfLayer(layer);
			} else {
				// Selected layer isn't in our specified group, ignore and
				// insert at the top
				before = 0;
			}
		}
	} else {
		// Nothing is selected, insert at top of the group by default
		before = 0;
	}

	// Create a new layer of the appropriate type by searching for the QAction
	// and then calling the matching factory
	Layer *layer = NULL;
	for(int i = 0; i < m_addLayerActions.size(); i++) {
		if(action == m_addLayerActions.at(i)) {
			LayerFactoryList factories = App->getLayerFactories();
			LayerFactory *factory = factories.at(i);
			layer = m_addLayerGroup->createLayer(
				factory->getTypeId(), QString(), before);
			break;
		}
	}
	if(layer == NULL)
		return;

	m_scene->setActiveItemLayer(layer);
	layer->setVisible(true); // Visible by default
	if(layer->hasSettingsDialog())
		layer->showSettingsDialog();
	// TODO: Edit name immediately after closing settings dialog
}

void SceneItemView::layerContextTriggered(QAction *action)
{
	if(action == m_layerPropertiesAction) {
		// Show layer properties
		if(m_contextLayer->hasSettingsDialog())
			m_contextLayer->showSettingsDialog();
	} else if(action == m_duplicateLayerAction)
		m_contextLayer->getParent()->duplicateLayer(m_contextLayer);
	else if(action == m_renameLayerAction)
		editLayerName(m_scene->itemForLayer(m_contextLayer));
	else if(action == m_deleteLayerAction)
		App->showDeleteLayerDialog(m_scene->itemForLayer(m_contextLayer));
	else if(action == m_moveLayerUpAction) {
		// Move layer up
		LayerGroup *group = m_contextLayer->getParent();
		int index = group->indexOfLayer(m_contextLayer);
		if(index > 0) {
			// Move within the group
			m_contextLayer->moveTo(group, index - 1);
		} else {
			// Move between groups
			int groupIndex = m_scene->indexOfGroup(group);
			if(groupIndex == 0)
				return; // Already at the top of the stack
			m_contextLayer->moveTo(m_scene->groupAtIndex(groupIndex - 1), -1);
		}
	} else if(action == m_moveLayerDownAction) {
		// Move layer down
		LayerGroup *group = m_contextLayer->getParent();
		int index = group->indexOfLayer(m_contextLayer);
		if(index < group->layerCount() - 1) {
			// Move within the group
			m_contextLayer->moveTo(group, index + 2);
		} else {
			// Move between groups
			int groupIndex = m_scene->indexOfGroup(group);
			if(groupIndex == m_scene->groupCount() - 1)
				return; // Already at the bottom of the stack
			m_contextLayer->moveTo(m_scene->groupAtIndex(groupIndex + 1), 0);
		}
	} else if(action == m_reloadLayerAction)
		m_contextLayer->reload();
	else if(action == m_reloadAllLayerAction)
		m_scene->getProfile()->reloadAllLayers();
	else if(action == m_loadLayerAction)
		m_contextLayer->setLoaded(true);
	else if(action == m_loadGroupLayerAction)
		m_contextLayer->getParent()->loadAllLayers();
	else if(action == m_loadAllLayerAction)
		m_scene->getProfile()->loadAllLayers();
	else if(action == m_unloadLayerAction)
		m_contextLayer->setLoaded(false);
	else if(action == m_unloadAllLayerAction)
		m_scene->getProfile()->unloadAllInvisibleLayers();
}

void SceneItemView::groupContextTriggered(QAction *action)
{
	if(action == m_renameGroupAction) {
		// Rename group
		editLayerName(m_scene->itemForGroup(m_contextGroup));
	} else if(action == m_deleteGroupAction) {
		// Delete group (Show confirmation dialog)
		App->showDeleteLayerDialog(m_scene->itemForGroup(m_contextGroup));
	} else if(action == m_moveGroupUpAction) {
		// Move group up
		int index = m_scene->indexOfGroup(m_contextGroup);
		if(index == 0)
			return; // Already at the top of the stack
		m_scene->moveGroupTo(m_contextGroup, index - 1);
	} else if(action == m_moveGroupDownAction) {
		// Move group down
		int index = m_scene->indexOfGroup(m_contextGroup);
		if(index == m_scene->groupCount() - 1)
			return; // Already at the bottom of the stack
		m_scene->moveGroupTo(m_contextGroup, index + 2);
	} else if(action == m_reloadAllGroupAction) {
		m_scene->getProfile()->reloadAllLayers();
	} else if(action == m_loadGroupGroupAction) {
		m_contextGroup->loadAllLayers();
	} else if(action == m_loadAllGroupAction) {
		m_scene->getProfile()->loadAllLayers();
	} else if(action == m_unloadAllGroupAction) {
		m_scene->getProfile()->unloadAllInvisibleLayers();
	} else if(action == m_makeLocalGroupAction) {
		// Make a local copy of a shared group by serializing and unserializing
		QBuffer buf;
		buf.open(QIODevice::ReadWrite);
		QDataStream stream(&buf);
		stream.setByteOrder(QDataStream::LittleEndian);
		stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
		stream.setVersion(12);
		m_contextGroup->serialize(&stream);
		if(stream.status() != QDataStream::Ok) {
			appLog(Log::Warning)
				<< "An error occurred while detaching group (Serializing)";
			return;
		}
		buf.seek(0);
		LayerGroup *group =
			m_scene->getProfile()->createLayerGroupSerialized(0, &stream);
		if(stream.status() != QDataStream::Ok) {
			appLog(Log::Warning)
				<< "An error occurred while detaching group (Unserializing)";
			return;
		}

		// Replace the shared group with the newly created one. We must remove
		// the old group after adding the new one to prevent pruning it.
		int index = m_scene->indexOfGroup(m_contextGroup);
		if(index < 0)
			return; // Should never happen
		bool wasVisible = m_scene->isGroupVisible(m_contextGroup);
		App->disableMainWindowUpdates(); // Prevent ugly repaints
		m_scene->addGroup(group, wasVisible, index);
		m_scene->removeGroup(m_contextGroup);

		// Unset the active item as it's by default set to the last layer in
		// the new group
		m_scene->setActiveItem(NULL);
	}
}

void SceneItemView::groupShareWithTriggered(QAction *action)
{
	// Determine which scene was selected
	int index = -1;
	for(int i = 0; i < m_shareWithActions.size(); i++) {
		if(action == m_shareWithActions.at(i)) {
			index = i;
			break;
		}
	}
	if(index == -1)
		return; // Invalid scene selected
	Scene *scene = App->getProfile()->sceneAtIndex(index);
	if(scene == NULL)
		return; // Unknown scene selected

	// Share the selected group with the selected scene if the group is not
	// already in that scene. The group is added to the top of the list.
	const SceneItemList &items = scene->getGroupSceneItems();
	for(int i = 0; i < items.size(); i++) {
		if(items.at(i)->getGroup() == m_contextGroup)
			return; // Group already in selected scene
	}
	scene->addGroup(
		m_contextGroup, m_scene->isGroupVisible(m_contextGroup), 0);
}
