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

#include "sceneview.h"
#include "application.h"
#include "borderspacer.h"
#include "profile.h"
#include "scene.h"
#include "Widgets/patternwidgets.h"
#include "Widgets/styledtogglebutton.h"
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QRadioButton>

//=============================================================================
// SceneViewBar

SceneViewBar::SceneViewBar(
	Scene *scene, const QColor &fgColor, const QColor &bgColor,
	const QColor &shadowColor, QWidget *parent)
	: StyledSortableBar(fgColor, bgColor, shadowColor, 0, parent)
	, m_scene(scene)
	, m_activeBtn(NULL)
	, m_deleteBtn(NULL)
	, m_dragHandle(NULL)
	, m_freezeVisual(false)
	, m_sceneDeleted(false)
{
	construct();
}

SceneViewBar::SceneViewBar(
	Scene *scene, const QColor &fgColor, const QColor &bgColor,
	const QColor &shadowColor, const QString &label, QWidget *parent)
	: StyledSortableBar(fgColor, bgColor, shadowColor, label, 0, parent)
	, m_scene(scene)
	, m_activeBtn(NULL)
	, m_deleteBtn(NULL)
	, m_dragHandle(NULL)
	, m_freezeVisual(false)
	, m_sceneDeleted(false)
{
	construct();
}

SceneViewBar::~SceneViewBar()
{
	delete m_activeBtn;
	delete m_deleteBtn;
	delete m_dragHandle;
}

void SceneViewBar::construct()
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
	m_label.setTextColor(col);

	// Left spacer
	QWidget *spacer = new QWidget(this);
	spacer->setFixedSize(5, 0);
	addLeftWidget(spacer);

	// Active radio button
	m_activeBtn = new QRadioButton(this);
	App->applyDarkStyle(m_activeBtn);
	updateActiveCheckState(); // Must be after `applyDarkStyle()`
	addLeftWidget(m_activeBtn);
	connect(m_activeBtn, &QAbstractButton::clicked,
		this, &SceneViewBar::leftClickEvent);

	// Delete button
	m_deleteBtn = new StyledButton(StyleHelper::TransparentSet, false, this);
	m_deleteBtn->setIcon(QIcon(":/Resources/white-cross.png"));
	m_deleteBtn->setHoverIcon(QIcon(":/Resources/white-cross-hover.png"));
	m_deleteBtn->setIconSize(QSize(10, 10));
	m_deleteBtn->setFixedSize(STYLED_BAR_HEIGHT, STYLED_BAR_HEIGHT);
	m_deleteBtn->hide(); // Hide by default
	m_deleteBtn->setToolTip(tr("Delete scene"));
	addRightLabelWidget(m_deleteBtn);
	connect(m_deleteBtn, &StyledButton::clicked,
		this, &SceneViewBar::deleteClickEvent);

	// Grip pattern and spacer
	m_dragHandle = new GripPattern(true, this);
	m_dragHandle->setFixedSize(17, 10);
	m_dragHandle->setToolTip(tr("Drag to rearrange scenes"));
	addRightLabelWidget(m_dragHandle);
	spacer = new QWidget(this);
	spacer->setFixedSize(5, 0);
	addRightLabelWidget(spacer);
	connect(m_dragHandle, &GripPattern::dragBegun,
		this, &SceneViewBar::dragBegun);

	// Indent label
	setLabelIndent(5);

	// Make sure that the active scene is immediately selected
	activeSceneChanged(m_scene->getProfile()->getActiveScene(), NULL);

	// Clicking the label activates the scene and editing the label changes
	// the scene's name
	connect(this, &SceneViewBar::clicked,
		this, &SceneViewBar::leftClickEvent);
	connect(this, &SceneViewBar::labelEdited,
		this, &SceneViewBar::nameEdited);

	// Watch profile for changes
	Profile *profile = m_scene->getProfile();
	connect(profile, &Profile::sceneChanged,
		this, &SceneViewBar::sceneChanged);
	connect(profile, &Profile::activeSceneChanged,
		this, &SceneViewBar::activeSceneChanged);
	connect(profile, &Profile::destroyingScene,
		this, &SceneViewBar::destroyingScene);
}

void SceneViewBar::editName()
{
	// Immediately enter edit mode and give the widget focus
	setEditableLabel(true, true);
}

void SceneViewBar::updateActiveCheckState()
{
	m_activeBtn->setChecked(App->getActiveScene() == m_scene);
}

void SceneViewBar::viewWidgetSet(SortableView *widget)
{
	if(widget == NULL)
		return;

	// Connect right-click signal to open the context menu making sure we never
	// connect the same signal multiple times
	connect(this, &SceneViewBar::rightClicked,
		this, &SceneViewBar::rightClickEvent, Qt::UniqueConnection);

	// Put our radio button into a group with all the others so the OS can
	// handle exclusiveness
	SceneView *view = static_cast<SceneView *>(m_viewWidget);
	view->getRadioGroup()->addButton(m_activeBtn);
}

void SceneViewBar::keyPressEvent(QKeyEvent *ev)
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

void SceneViewBar::sceneChanged(Scene* scene)
{
	if(m_freezeVisual)
		return; // UI is frozen for changes
	if(scene != m_scene)
		return; // Not us

	// Update widgets
	setLabel(scene->getName());
	updateActiveCheckState();
}

void SceneViewBar::activeSceneChanged(Scene *scene, Scene *prev)
{
	if(m_freezeVisual)
		return; // UI is frozen for changes
	if(m_scene == scene) {
		// We were selected
		updateActiveCheckState();
		setEditableLabel(true);
		if(m_scene->getProfile()->getScenes().size() >= 2)
			m_deleteBtn->show();
		setBgColor(StyleHelper::DarkBg5SelectedColor);

		// Make sure we are visible to the user
		if(m_viewWidget != NULL)
			m_viewWidget->ensureWidgetVisible(this, 0, 0);
	} else if(m_scene == prev) {
		// We were deselected
		updateActiveCheckState();
		setEditableLabel(false);
		m_deleteBtn->hide();
		setBgColor(StyleHelper::DarkBg5Color);
	}
}

void SceneViewBar::destroyingScene(Scene *scene)
{
	if(scene != m_scene)
		return; // Not us

	// Prevent the UI from changing states as the item is destroyed
	m_freezeVisual = true;

	// Begin delete animation, TODO: Never actually done
	m_sceneDeleted = true;

	// Just remove immediately from the view for now, TODO
	m_viewWidget->removeSortable(this);
	delete this;
}

void SceneViewBar::deleteClickEvent()
{
	// Show confirmation dialog
	App->showDeleteSceneDialog(m_scene);
}

void SceneViewBar::leftClickEvent()
{
	// WARNING: Also trigged by toggling the radio button
	SceneView *view = static_cast<SceneView *>(m_viewWidget);
	App->getProfile()->setActiveScene(m_scene);
	updateActiveCheckState();
}

void SceneViewBar::rightClickEvent()
{
	SceneView *view = static_cast<SceneView *>(m_viewWidget);
	view->showSceneContextMenu(m_scene);
}

void SceneViewBar::nameEdited(const QString &text)
{
	m_scene->setName(text.trimmed());
	setLabel(m_scene->getName()); // Just in case it was an invalid name
}

/// <summary>
/// Called when the user has initiated a drag using the grip widget.
/// </summary>
void SceneViewBar::dragBegun(const QPoint &startPos)
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
// SceneView

SceneView::SceneView(Profile *profile, QWidget *parent)
	: SortableView("application/x-mishira-scene", parent)
	, m_profile(profile)
	, m_mouseDown(false)
	, m_radioGroup(this)

	// Right-click scene context menu
	, m_contextScene(NULL)
	, m_contextSceneMenu(this)
	, m_duplicateSceneAction(NULL)
	, m_renameSceneAction(NULL)
	, m_deleteSceneAction(NULL)
	, m_moveSceneUpAction(NULL)
	, m_moveSceneDownAction(NULL)
{
	App->applyDarkStyle(this);
	App->applyDarkStyle(verticalScrollBar());
	App->applyDarkStyle(horizontalScrollBar());

	// Populate view from existing profile
	SceneList scenes = profile->getScenes();
	for(int i = 0; i < scenes.count(); i++) {
		Scene *scene = scenes.at(i);
		addSortable(createSceneBar(scene));
	}

	// Right-click scene context menu
	m_moveSceneUpAction =
		m_contextSceneMenu.addAction(tr("Move scene up"));
	m_moveSceneDownAction =
		m_contextSceneMenu.addAction(tr("Move scene down"));
	m_contextSceneMenu.addSeparator();
	m_duplicateSceneAction =
		m_contextSceneMenu.addAction(tr("Duplicate scene"));
	m_renameSceneAction =
		m_contextSceneMenu.addAction(tr("Rename scene"));
	m_deleteSceneAction =
		m_contextSceneMenu.addAction(tr("Delete scene"));
	connect(&m_contextSceneMenu, &QMenu::triggered,
		this, &SceneView::sceneContextTriggered);

	// Watch profile for additions and moves
	connect(profile, &Profile::sceneAdded,
		this, &SceneView::sceneAdded);
	connect(profile, &Profile::sceneMoved,
		this, &SceneView::sceneMoved);
}

SceneView::~SceneView()
{
}

SceneViewBar *SceneView::createSceneBar(Scene *scene)
{
	SceneViewBar *widget = new SceneViewBar(
		scene, StyleHelper::DarkFg2Color,
		StyleHelper::DarkBg5Color, StyleHelper::TextShadowColor,
		scene->getName(), this);
	return widget;
}

SceneViewBar *SceneView::barForScene(Scene *scene)
{
	QVector<SortableWidget *> sortables = getSortables();
	for(int i = 0; i < sortables.count(); i++) {
		SceneViewBar *widget =
			static_cast<SceneViewBar *>(sortables.at(i));
		if(widget->getScene() != scene)
			continue;
		return widget;
	}
	return NULL;
}

void SceneView::showSceneContextMenu(Scene *scene)
{
	m_contextScene = scene;
	if(m_profile->getScenes().size() < 2) {
		// Prevent deleting the last scene
		m_deleteSceneAction->setEnabled(false);
	} else
		m_deleteSceneAction->setEnabled(true);
	m_contextSceneMenu.popup(QCursor::pos());
}

void SceneView::editSceneName(Scene *scene)
{
	SceneViewBar *widget = barForScene(scene);
	if(widget == NULL)
		return;
	widget->editName();
}

bool SceneView::moveChild(SortableWidget *child, int before)
{
	// Called when the view requests a scene be moved via drag-and-drop.

	SceneViewBar *widget = static_cast<SceneViewBar *>(child);
	Scene *scene = widget->getScene();
	Scene *beforeScene = m_profile->sceneAtIndex(before);

	if(beforeScene == NULL) {
		// Moving to the end of the list
		m_profile->moveSceneTo(scene, -1);
	} else {
		// Moving before another scene
		m_profile->moveSceneTo(scene, m_profile->indexOfScene(beforeScene));
	}

	return true;
}

void SceneView::keyPressEvent(QKeyEvent *ev)
{
	ev->ignore();
	switch(ev->key()) {
	default:
		break;
	case Qt::Key_Up: {
		// Select the next scene above the current one
		Scene *scene = m_profile->getActiveScene();
		if(scene == NULL)
			break;
		ev->accept();
		int index = m_profile->indexOfScene(scene);
		if(index == 0)
			break; // We are the top scene
		scene = m_profile->sceneAtIndex(index - 1);
		m_profile->setActiveScene(scene);
		break; }
	case Qt::Key_Down: {
		// Select the next scene below the current one
		Scene *scene = m_profile->getActiveScene();
		if(scene == NULL)
			break;
		ev->accept();
		int index = m_profile->indexOfScene(scene);
		if(index >= m_profile->getScenes().size() - 1)
			break; // We are the bottom scene
		scene = m_profile->sceneAtIndex(index + 1);
		m_profile->setActiveScene(scene);
		break; }
	case Qt::Key_F2:
		// Edit the selected scene's name
		Scene *scene = m_profile->getActiveScene();
		if(scene == NULL)
			break;
		SceneViewBar *bar = barForScene(scene);
		bar->editName();
		ev->accept();
		break;
	}
}

void SceneView::mousePressEvent(QMouseEvent *ev)
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

void SceneView::mouseReleaseEvent(QMouseEvent *ev)
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

	// The user clicked an inactive part of the widget, do nothing
	m_mouseDown = false;
	ev->accept();
}

void SceneView::focusOutEvent(QFocusEvent *ev)
{
	m_mouseDown = false;
	SortableView::focusOutEvent(ev);
}

void SceneView::sceneAdded(Scene *scene, int before)
{
	addSortable(createSceneBar(scene), before);
	m_profile->setActiveScene(scene);
	// TODO: Edit name immediately
}

void SceneView::sceneMoved(Scene *scene, int before)
{
	// As moving individual items is error-prone we just regenerate the entire
	// list at once.

	// Get widget list with the new order
	QVector<SortableWidget *> sortables = getSortables();
	QVector<SceneViewBar *> newSortables;
	newSortables.resize(sortables.count());
	for(int i = 0; i < sortables.count(); i++) {
		SceneViewBar *widget =
			static_cast<SceneViewBar *>(sortables.at(i));
		Scene *scene = widget->getScene();
		int index = m_profile->indexOfScene(scene);
		newSortables[index] = widget;
	}

	// Regenerate list
	for(int i = 0; i < sortables.count(); i++)
		removeSortable(sortables.at(i));
	for(int i = 0; i < newSortables.count(); i++)
		addSortable(newSortables.at(i));
}

void SceneView::sceneContextTriggered(QAction *action)
{
	if(action == m_duplicateSceneAction) {
		// Duplicate scene by serializing, unserializing and renaming
		int index = m_profile->indexOfScene(m_contextScene);

		QBuffer buf;
		buf.open(QIODevice::ReadWrite);
		QDataStream stream(&buf);
		stream.setByteOrder(QDataStream::LittleEndian);
		stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
		stream.setVersion(12);
		m_contextScene->serialize(&stream);
		if(stream.status() != QDataStream::Ok) {
			appLog(Log::Warning)
				<< "An error occurred while duplicating (Serializing)";
			return;
		}
		buf.seek(0);
		Scene *scene = m_profile->createSceneSerialized(&stream, index);
		if(stream.status() != QDataStream::Ok) {
			appLog(Log::Warning)
				<< "An error occurred while duplicating (Unserializing)";
			return;
		}
		scene->setName(tr("%1 (Copy)").arg(m_contextScene->getName()));
	} else if(action == m_renameSceneAction) {
		// Rename scene
		editSceneName(m_contextScene);
	} else if(action == m_deleteSceneAction) {
		// Delete scene (Show confirmation dialog)
		App->showDeleteSceneDialog(m_contextScene);
	} else if(action == m_moveSceneUpAction) {
		// Move group up
		int index = m_profile->indexOfScene(m_contextScene);
		if(index == 0)
			return; // Already at the top of the stack
		m_profile->moveSceneTo(m_contextScene, index - 1);
	} else if(action == m_moveSceneDownAction) {
		// Move group down
		int index = m_profile->indexOfScene(m_contextScene);
		if(index == m_profile->getScenes().size() - 1)
			return; // Already at the bottom of the stack
		m_profile->moveSceneTo(m_contextScene, index + 2);
	}
}
