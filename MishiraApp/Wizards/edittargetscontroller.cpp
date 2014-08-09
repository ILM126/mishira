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

#include "edittargetscontroller.h"
#include "application.h"
#include "profile.h"
#include "target.h"
#include "wizardwindow.h"
#include "Widgets/infowidget.h"
#include "Wizards/newedittargetcontroller.h"
#include "Wizards/targetlistpage.h"

EditTargetsController::EditTargetsController(
	WizController type, WizardWindow *wizWin, WizShared *shared)
	: WizardController(type, wizWin, shared)
	, m_targetWidgets()
	, m_btnGroup(this)
	, m_doDelayedRepaint(false)
{
}

EditTargetsController::~EditTargetsController()
{
}

/// <summary>
/// Called whenever we are leaving the current controller page.
/// </summary>
void EditTargetsController::leavingPage()
{
	// Remove all our target radio buttons from the group
	for(int i = 0; i < m_targetWidgets.count(); i++)
		m_btnGroup.removeButton(m_targetWidgets.at(i)->getButton());

	// Deleting our widgets removes them from the UI layout
	for(int i = 0; i < m_targetWidgets.count(); i++)
		delete m_targetWidgets.at(i);
	m_targetWidgets.clear();

	// Stop watching profile for target deletion
	disconnect(App->getProfile(), &Profile::destroyingTarget,
		this, &EditTargetsController::targetDeleted);
}

void EditTargetsController::resetButtons()
{
	// Get UI object
	TargetListPage *page =
		static_cast<TargetListPage *>(m_wizWin->getActivePage());
	Ui_TargetListPage *ui = page->getUi();

	bool btnsEnabled = (m_targetWidgets.count() > 0);
	ui->editBtn->setEnabled(btnsEnabled);

	btnsEnabled = (m_targetWidgets.count() > 1);
	ui->deleteBtn->setEnabled(btnsEnabled); // Cannot delete last target
	ui->moveUpBtn->setEnabled(btnsEnabled);
	ui->moveDownBtn->setEnabled(btnsEnabled);
}

Target *EditTargetsController::getSelectedTarget() const
{
	for(int i = 0; i < m_targetWidgets.count(); i++) {
		TargetInfoWidget *widget = m_targetWidgets.at(i);
		if(widget->isSelected())
			return widget->getTarget();
	}
	return NULL;
}

void EditTargetsController::resetWizard()
{
	m_wizWin->setPage(WizTargetListPage);
	m_wizWin->setTitle(tr("Modify targets"));
	m_wizWin->setVisibleButtons(WizOKButton);

	// Watch profile for target deletion
	connect(App->getProfile(), &Profile::destroyingTarget,
		this, &EditTargetsController::targetDeleted,
		Qt::UniqueConnection);

	// Get UI object
	TargetListPage *page =
		static_cast<TargetListPage *>(m_wizWin->getActivePage());
	Ui_TargetListPage *ui = page->getUi();

	// Prevent ugly repaints when moving targets. We cannot do this always as
	// it'll cause a black box on first display
	if(m_doDelayedRepaint)
		ui->targetList->setUpdatesEnabled(false);

	// Connect buttons
	connect(ui->newTargetBtn, &QAbstractButton::clicked,
		this, &EditTargetsController::createTargetClicked,
		Qt::UniqueConnection);
	connect(ui->editBtn, &QAbstractButton::clicked,
		this, &EditTargetsController::modifyTargetClicked,
		Qt::UniqueConnection);
	connect(ui->duplicateBtn, &QAbstractButton::clicked,
		this, &EditTargetsController::duplicateTargetClicked,
		Qt::UniqueConnection);
	connect(ui->deleteBtn, &QAbstractButton::clicked,
		this, &EditTargetsController::deleteTargetClicked,
		Qt::UniqueConnection);
	connect(ui->moveUpBtn, &QAbstractButton::clicked,
		this, &EditTargetsController::moveTargetUpClicked,
		Qt::UniqueConnection);
	connect(ui->moveDownBtn, &QAbstractButton::clicked,
		this, &EditTargetsController::moveTargetDownClicked,
		Qt::UniqueConnection);

	// Fill target list
	//PROFILE_BEGIN(a);
	QVBoxLayout *layout = ui->targetListLayout;
	TargetList targets = App->getProfile()->getTargets();
	for(int i = 0; i < targets.count(); i++) {
		Target *target = targets.at(i);

		TargetInfoWidget *widget = new TargetInfoWidget(target);
		target->setupTargetInfo(widget);
		m_targetWidgets.append(widget);
		layout->insertWidget(layout->count() - 1, widget);
		m_btnGroup.addButton(widget->getButton());
	}
	//PROFILE_END(a, "Target list");

	// Select the first target if one exists and setup the buttons
	if(m_targetWidgets.count())
		m_targetWidgets.at(0)->select();
	resetButtons();

	// HACK: This must be delayed in order to actually do anything
	if(m_doDelayedRepaint)
		QTimer::singleShot(10, this, SLOT(enableUpdatesTimeout()));
}

void EditTargetsController::enableUpdatesTimeout()
{
	TargetListPage *page =
		static_cast<TargetListPage *>(m_wizWin->getActivePage());
	if(page == NULL)
		return;
	Ui_TargetListPage *ui = page->getUi();
	if(ui == NULL)
		return;
	ui->targetList->setUpdatesEnabled(true);
}

bool EditTargetsController::cancelWizard()
{
	// Closing the window is the same as clicking "OK"
	nextPage();

	return true; // We have fully processed the cancel
}

void EditTargetsController::resetPage()
{
	// Never called
}

void EditTargetsController::backPage()
{
	// Never called
}

void EditTargetsController::nextPage()
{
	// Close the wizard window
	leavingPage();
	m_wizWin->controllerFinished();
}

void EditTargetsController::createTargetClicked()
{
	// Populate settings for the next controller
	NewEditTargetController::prepareCreateTarget(m_shared, true);

	// Switch to the next controller
	leavingPage();
	m_wizWin->setController(WizNewEditTargetController);
}

void EditTargetsController::modifyTargetClicked()
{
	Target *target = getSelectedTarget();
	if(target == NULL)
		return;

	// Populate settings for the next controller
	NewEditTargetController::prepareModifyTarget(m_shared, target, true);

	// Switch to the next controller
	leavingPage();
	m_wizWin->setController(WizNewEditTargetController);
}

void EditTargetsController::duplicateTargetClicked()
{
	Target *target = getSelectedTarget();
	if(target == NULL)
		return;
	Profile *profile = App->getProfile();
	int index = profile->indexOfTarget(target);

	// Duplicate scene by serializing, unserializing and renaming
	QBuffer buf;
	buf.open(QIODevice::ReadWrite);
	QDataStream stream(&buf);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
	stream.setVersion(12);
	target->serialize(&stream);
	if(stream.status() != QDataStream::Ok) {
		appLog(Log::Warning)
			<< "An error occurred while duplicating (Serializing)";
		return;
	}
	buf.seek(0);
	Target *newTarget =
		profile->createTargetSerialized(target->getType(), &stream, index);
	if(stream.status() != QDataStream::Ok) {
		appLog(Log::Warning)
			<< "An error occurred while duplicating (Unserializing)";
		return;
	}
	newTarget->setName(tr("%1 (Copy)").arg(target->getName()));

	// Refresh target list and select the new target
	m_doDelayedRepaint = true;
	leavingPage();
	resetWizard();
	m_doDelayedRepaint = false;
	m_targetWidgets.at(index)->select();
}

void EditTargetsController::deleteTargetClicked()
{
	Target *target = getSelectedTarget();
	if(target == NULL)
		return;

	// Delete target (Show confirmation dialog)
	App->showDeleteTargetDialog(target);
}

void EditTargetsController::moveTargetUpClicked()
{
	Target *target = getSelectedTarget();
	if(target == NULL)
		return;
	Profile *profile = App->getProfile();
	int index = profile->indexOfTarget(target);
	if(index == 0)
		return; // Already at the top of the stack
	profile->moveTargetTo(target, index - 1);

	// Refresh target list and reselect the original target
	m_doDelayedRepaint = true;
	leavingPage();
	resetWizard();
	m_doDelayedRepaint = false;
	m_targetWidgets.at(index - 1)->select();
}

void EditTargetsController::moveTargetDownClicked()
{
	Target *target = getSelectedTarget();
	if(target == NULL)
		return;
	Profile *profile = App->getProfile();
	int index = profile->indexOfTarget(target);
	if(index == profile->getTargets().count() - 1)
		return; // Already at the bottom of the stack
	profile->moveTargetTo(target, index + 2);

	// Refresh target list and reselect the original target
	m_doDelayedRepaint = true;
	leavingPage();
	resetWizard();
	m_doDelayedRepaint = false;
	m_targetWidgets.at(index + 1)->select();
}

void EditTargetsController::targetDeleted(Target *target)
{
	// The target that was deleted should always be the one that is currently
	// selected. Make the next target in the list the selected one.
	int index = App->getProfile()->indexOfTarget(target);

	// Remove the target's radio button from the group
	m_btnGroup.removeButton(m_targetWidgets.at(index)->getButton());

	// Deleting our widget also removes it from the UI layout
	delete m_targetWidgets.at(index);
	m_targetWidgets.remove(index);

	// Select the next target
	if(m_targetWidgets.count() > 0)
		m_targetWidgets.at(qMin(index, m_targetWidgets.count() - 1))->select();

	// Reset the button state
	resetButtons();
}
