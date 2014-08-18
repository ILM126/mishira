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

#include "editprofilecontroller.h"
#include "application.h"
#include "appsettings.h"
#include "common.h"
#include "profile.h"
#include "wizardwindow.h"
#include "Wizards/editprofilepage.h"

EditProfileController::EditProfileController(
	WizController type, WizardWindow *wizWin, WizShared *shared)
	: WizardController(type, wizWin, shared)
{
}

EditProfileController::~EditProfileController()
{
}

void EditProfileController::resetWizard()
{
	m_wizWin->setPage(WizEditProfilePage);
	m_wizWin->setTitle(tr("Edit profile")); // TODO: "Modify"?
	m_wizWin->setVisibleButtons(
		WizOKButton | WizCancelButton | WizResetButton);

	// Initialize values
	resetPage();
}

bool EditProfileController::cancelWizard()
{
	// Do nothing except close the wizard window
	m_wizWin->controllerFinished();

	return true; // We have fully processed the cancel
}

void EditProfileController::resetPage()
{
	Profile *profile = App->getProfile();
	if(profile == NULL)
		return;

	// Get UI object
	EditProfilePage *page =
		static_cast<EditProfilePage *>(m_wizWin->getActivePage());
	Ui_EditProfilePage *ui = page->getUi();

	page->setUpdatesEnabled(false);

	// Update fields
	ui->nameEdit->setText(profile->getName());
	for(int i = 0; !PrflFpsRates[i].isZero(); i++) {
		if(PrflFpsRates[i] == profile->getVideoFramerate()) {
			ui->fpsCombo->setCurrentIndex(i);
			break;
		}
	}
	ui->audioCombo->setCurrentIndex(profile->getAudioMode());
	ui->widthEdit->setText(
		QString::number(profile->getCanvasSize().width()));
	ui->heightEdit->setText(
		QString::number(profile->getCanvasSize().height()));

	// Reset validity and connect signal
	doQLineEditValidate(ui->nameEdit);
	doQLineEditValidate(ui->widthEdit);
	doQLineEditValidate(ui->heightEdit);
	m_wizWin->setCanContinue(page->isValid());
	connect(page, &EditProfilePage::validityMaybeChanged,
		m_wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	page->setUpdatesEnabled(true);
}

void EditProfileController::backPage()
{
	// Never called
}

void EditProfileController::nextPage()
{
	Profile *profile = App->getProfile();
	if(profile == NULL) {
		// Close the wizard window
		m_wizWin->controllerFinished();
		return;
	}

	// Get UI object
	EditProfilePage *page =
		static_cast<EditProfilePage *>(m_wizWin->getActivePage());
	Ui_EditProfilePage *ui = page->getUi();

	// If the video framerate changed then we need to completely reload the
	// profile
	bool framerateChanged = false;
	Fraction framerate = PrflFpsRates[ui->fpsCombo->currentIndex()];
	if(framerate != profile->getVideoFramerate())
		framerateChanged = true;

	// Save fields
	QString profileName = ui->nameEdit->text().trimmed();
	profile->setName(profileName);
	profile->setVideoFramerate(framerate);
	profile->setAudioMode((PrflAudioMode)(ui->audioCombo->currentIndex()));
	profile->setCanvasSize(QSize(
		ui->widthEdit->text().toInt(),
		ui->heightEdit->text().toInt()));

	// HACK: We assume we're modifying the active profile
	App->getAppSettings()->setActiveProfile(profileName);

	// Refresh profile menu bar and window title
	App->updateMenuBar();
	App->updateWindowTitle();

	// Close the wizard window
	m_wizWin->controllerFinished();

	// Entirely reload the profile if required to
	if(framerateChanged)
		App->changeActiveProfile(profile->getName(), NULL, true);
}
