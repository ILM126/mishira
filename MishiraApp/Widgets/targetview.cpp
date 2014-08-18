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

#include "targetview.h"
#include "application.h"
#include "profile.h"
#include "target.h"
#include "wizardwindow.h"
#include "Widgets/targetpane.h"
#include "Wizards/newedittargetcontroller.h"
#include <QtWidgets/QScrollBar>

TargetView::TargetView(QWidget *parent)
	: QScrollArea(parent)
	, m_panes()
	, m_layoutWidget(this)
	, m_layout(&m_layoutWidget) // Scroll area child widget

	// Right-click context menu
	, m_contextTarget(NULL)
	, m_contextMenu(this)
	, m_modifyTargetAction(NULL)
	, m_moveTargetUpAction(NULL)
	, m_moveTargetDownAction(NULL)
{
	App->applyDarkStyle(this);
	App->applyDarkStyle(verticalScrollBar());
	App->applyDarkStyle(horizontalScrollBar());

	// Display the layout widget in the scroll area
	setWidget(&m_layoutWidget);
	m_layoutWidget.installEventFilter(this);
	m_layoutWidget.setSizePolicy(
		QSizePolicy::Preferred, QSizePolicy::Maximum);

	// Remove layout margin and spacing
	m_layout.setMargin(0);
	m_layout.setSpacing(0);

	// Allow the scroll area to resize the layout widget dynamically
	setWidgetResizable(true);

	// Populate right-click context menu
	m_modifyTargetAction = m_contextMenu.addAction(tr(
		"Modify target..."));
	m_contextMenu.addSeparator();
	m_moveTargetUpAction = m_contextMenu.addAction(tr(
		"Move target up"));
	m_moveTargetDownAction = m_contextMenu.addAction(tr(
		"Move target down"));
	connect(&m_contextMenu, &QMenu::triggered,
		this, &TargetView::targetContextTriggered);

	// Populate view from existing profile
	repopulateList();

	// Watch profile for additions, deletions and moves
	Profile *profile = App->getProfile();
	connect(profile, &Profile::targetAdded,
		this, &TargetView::targetAdded);
	connect(profile, &Profile::destroyingTarget,
		this, &TargetView::destroyingTarget);
	connect(profile, &Profile::targetChanged,
		this, &TargetView::targetChanged);
	connect(profile, &Profile::targetMoved,
		this, &TargetView::targetMoved);
}

TargetView::~TargetView()
{
	for(int i = 0; i < m_panes.count(); i++)
		delete m_panes.at(i);
	m_panes.clear();
}

void TargetView::repopulateList()
{
	// Prevent ugly repaints
	setUpdatesEnabled(false);

	// Clear existing list
	for(int i = 0; i < m_panes.count(); i++)
		delete m_panes.at(i);
	m_panes.clear();

	// Repopulate list
	Profile *profile = App->getProfile();
	TargetList targets = profile->getTargets();
	for(int i = 0; i < targets.count(); i++) {
		Target *target = targets.at(i);
		TargetPane *pane = new TargetPane(target, this);
		target->setupTargetPane(pane);
		m_panes.append(pane);
		m_layout.addWidget(pane);
		connect(pane, &TargetPane::rightClicked,
			this, &TargetView::targetPaneRightClicked,
			Qt::UniqueConnection);
	}

	// HACK: This must be delayed in order to actually do anything
	QTimer::singleShot(10, this, SLOT(enableUpdatesTimeout()));
}

void TargetView::enableUpdatesTimeout()
{
	setUpdatesEnabled(true);
}

bool TargetView::eventFilter(QObject *watched, QEvent *ev)
{
	if(ev->type() != QEvent::Resize)
		return false;
	setUpdatesEnabled(false);
	setMaximumHeight(m_layoutWidget.minimumSizeHint().height());

	// HACK: This must be delayed in order to actually do anything
	QTimer::singleShot(10, this, SLOT(enableUpdatesTimeout()));

	return false;
}

void TargetView::showTargetContextMenu(Target *target)
{
	m_contextTarget = target;
	m_contextMenu.popup(QCursor::pos());
}

void TargetView::targetAdded(Target *target, int before)
{
	repopulateList();
}

void TargetView::destroyingTarget(Target *target)
{
	// We receive the signal before the target is actually removed so this
	// doesn't work
	//repopulateList();

	// Deleting our widget also removes it from the UI layout
	int index = App->getProfile()->indexOfTarget(target);
	delete m_panes.at(index);
	m_panes.remove(index);
}

void TargetView::targetChanged(Target *target)
{
	repopulateList();
}

void TargetView::targetMoved(Target *target, int before)
{
	repopulateList();
}

void TargetView::targetPaneRightClicked(Target *target)
{
	showTargetContextMenu(target);
}

void TargetView::targetContextTriggered(QAction *action)
{
	if(m_contextTarget == NULL)
		return;
	if(action == m_modifyTargetAction) {
		// Modify target dialog
		if(App->isBroadcasting()) {
			App->showBasicWarningDialog(
				tr("Cannot modify targets while broadcasting."));
			return;
		}
		WizardWindow *wizWin = App->showWizardAdvancedHACK();
		NewEditTargetController::prepareModifyTarget(
			wizWin->getShared(), m_contextTarget, false);
		wizWin->setController(WizNewEditTargetController);
		wizWin->show();
	} else if(action == m_moveTargetUpAction) {
		// Move target up
		Profile *profile = App->getProfile();
		int index = profile->indexOfTarget(m_contextTarget);
		if(index == 0)
			return; // Already at the top of the stack
		profile->moveTargetTo(m_contextTarget, index - 1);
	} else if(action == m_moveTargetDownAction) {
		// Move target down
		Profile *profile = App->getProfile();
		int index = profile->indexOfTarget(m_contextTarget);
		if(index == profile->getTargets().count() - 1)
			return; // Already at the bottom of the stack
		profile->moveTargetTo(m_contextTarget, index + 2);
	}
}
