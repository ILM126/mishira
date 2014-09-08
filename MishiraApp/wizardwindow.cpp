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

#include "wizardwindow.h"
#include "constants.h"
#include "wizardcontroller.h"
#include "Targets/file/filetargetsettingspage.h"
#include "Targets/hitbox/hitboxtargetsettingspage.h"
#include "Targets/rtmp/rtmptargetsettingspage.h"
#include "Targets/twitch/twitchtargetsettingspage.h"
#include "Targets/ustream/ustreamtargetsettingspage.h"
#include "Wizards/audiomixercontroller.h"
#include "Wizards/audiomixerpage.h"
#include "Wizards/audiosettingspage.h"
#include "Wizards/benchmarkpage.h"
#include "Wizards/benchmarkresultspage.h"
#include "Wizards/editprofilecontroller.h"
#include "Wizards/editprofilepage.h"
#include "Wizards/edittargetscontroller.h"
#include "Wizards/newedittargetcontroller.h"
#include "Wizards/newprofilecontroller.h"
#include "Wizards/newprofilepage.h"
#include "Wizards/preparebenchmarkpage.h"
#include "Wizards/profilecompletepage.h"
#include "Wizards/scenetemplatepage.h"
#include "Wizards/uploadpage.h"
#include "Wizards/targetlistpage.h"
#include "Wizards/targettypepage.h"
#include "Wizards/videosettingspage.h"
#include "Wizards/welcomepage.h"
#include <QtGui/QKeyEvent>

WizardWindow::WizardWindow(QWidget *parent)
	: QWidget(parent)
	, m_shared()

	// Outer layout
	, m_outerLayout(this)
	, m_header(this)

	// Inner layout
	, m_innerBox(this)
	, m_innerLayout(&m_innerBox)
	, m_line(&m_innerBox)
	, m_buttonBox(&m_innerBox)
	, m_buttonLayout(&m_buttonBox)
	, m_backBtn(&m_buttonBox)
	, m_nextBtn(&m_buttonBox)
	, m_finishBtn(&m_buttonBox)
	, m_okBtn(&m_buttonBox)
	, m_cancelBtn(&m_buttonBox)
	, m_resetBtn(&m_buttonBox)

	// Controllers and pages
	//, m_controllers()
	, m_activeController(NULL)
	//, m_pages()
	, m_activePage(NULL)
{
	// All wizards have the same fixed size
	setFixedSize(750, 550);

	// Make the window modal
	setWindowModality(Qt::WindowModal);

	// This is a child window that does not have a minimize or maximize button
	setWindowFlags(
		Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
		Qt::WindowCloseButtonHint);

	// Setup horizontal bar
	m_line.setFrameStyle(QFrame::HLine | QFrame::Sunken);
	//m_line.setLineWidth(0);

	// Setup button bar
#ifdef Q_OS_WIN
	m_backBtn.setText(tr("< Back"));
	m_nextBtn.setText(tr("Next >"));
	m_finishBtn.setText(tr("Finish"));
#endif
#ifdef Q_OS_MAC
	m_backBtn.setText(tr("Go back"));
	m_nextBtn.setText(tr("Continue"));
	m_finishBtn.setText(tr("Done"));
#endif
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
#error Unsupported platform
#endif
	m_okBtn.setText(tr("OK"));
	m_cancelBtn.setText(tr("Cancel"));
	m_resetBtn.setText(tr("Reset"));
	m_buttonLayout.setMargin(0);
	setVisibleButtons(WizStandardFirstButtons);
	connect(&m_backBtn, &QPushButton::clicked,
		this, &WizardWindow::backEvent);
	connect(&m_nextBtn, &QPushButton::clicked,
		this, &WizardWindow::nextEvent);
	connect(&m_finishBtn, &QPushButton::clicked,
		this, &WizardWindow::finishEvent);
	connect(&m_okBtn, &QPushButton::clicked,
		this, &WizardWindow::okEvent);
	connect(&m_cancelBtn, &QPushButton::clicked,
		this, &WizardWindow::cancelEvent);
	connect(&m_resetBtn, &QPushButton::clicked,
		this, &WizardWindow::resetEvent);

	// Populate "inner" layout
	m_innerBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_innerLayout.addStretch(1); // Only visible when no page is visible
	m_innerLayout.addWidget(&m_line);
	m_innerLayout.addWidget(&m_buttonBox);

	// Populate "outer" layout
	m_outerLayout.setMargin(0); // Fill entire window
	m_outerLayout.setSpacing(0);
	m_outerLayout.addWidget(&m_header);
	m_outerLayout.addWidget(&m_innerBox);

	// Initialize controllers. WARNING: These must be listed in the same order
	// that they are defined in the `WizController` enumeration!
	int index = 0;
	m_controllers[index++] = NULL;
	m_controllers[index++] = new NewProfileController(
		WizNewProfileController, this, &m_shared);
	m_controllers[index++] = new EditProfileController(
		WizEditProfileController, this, &m_shared);
	m_controllers[index++] = new EditTargetsController(
		WizEditTargetsController, this, &m_shared);
	m_controllers[index++] = new NewEditTargetController(
		WizNewEditTargetController, this, &m_shared);
	m_controllers[index++] = new AudioMixerController(
		WizAudioMixerController, this, &m_shared);
	Q_ASSERT(index == WIZ_NUM_CONTROLLERS);

	// Initialize pages. WARNING: These must be listed in the same order that
	// they are defined in the `WizPage` enumeration!
	index = 0;
	m_pages[index++] = NULL;
	m_pages[index++] = new WelcomePage(this);
	m_pages[index++] = new NewProfilePage(this);
	m_pages[index++] = new UploadPage(this);
	m_pages[index++] = new PrepareBenchmarkPage(this);
	m_pages[index++] = new BenchmarkPage(this);
	m_pages[index++] = new BenchmarkResultsPage(this);
	m_pages[index++] = new SceneTemplatePage(this);
	m_pages[index++] = new ProfileCompletePage(this);
	m_pages[index++] = new EditProfilePage(this);
	m_pages[index++] = new TargetListPage(this);
	m_pages[index++] = new TargetTypePage(this);
	m_pages[index++] = new VideoSettingsPage(this);
	m_pages[index++] = new AudioSettingsPage(this);
	m_pages[index++] = new AudioMixerPage(this);
	m_pages[index++] = new FileTargetSettingsPage(this);
	m_pages[index++] = new RTMPTargetSettingsPage(this);
	m_pages[index++] = new TwitchTargetSettingsPage(this);
	m_pages[index++] = new UstreamTargetSettingsPage(this);
	m_pages[index++] = new HitboxTargetSettingsPage(this);
	Q_ASSERT(index == WIZ_NUM_PAGES);
	for(int i = 0; i < WIZ_NUM_PAGES; i++) {
		if(m_pages[i] != NULL)
			m_pages[i]->hide();
	}
}

WizardWindow::~WizardWindow()
{
	if(m_activeController != NULL) {
		// Should never happen but be extra safe
		m_activeController->cancelWizard();
	}

	for(int i = 0; i < WIZ_NUM_CONTROLLERS; i++)
		delete m_controllers[i];
	for(int i = 0; i < WIZ_NUM_PAGES; i++)
		delete m_pages[i];
}

void WizardWindow::setController(WizController controller)
{
	if(controller < 0 || controller >= WIZ_NUM_CONTROLLERS)
		controller = WizNoController;
	m_activeController = m_controllers[controller];
	setCanContinue(true);
	if(m_activeController == NULL) {
		setPage(WizNoPage);
		setVisibleButtons(WizFinishButton);
		return;
	}
	m_activeController->resetWizard();
}

void WizardWindow::setPage(WizPage page)
{
	if(page < 0 || page >= WIZ_NUM_PAGES)
		page = WizNoPage;

	const int INSERT_POS = 0;

	// Remove existing page from the layout
	if(m_activePage != NULL) {
		m_innerLayout.removeWidget(m_activePage);
		m_activePage->hide();
	} else
		m_innerLayout.removeItem(m_innerLayout.itemAt(INSERT_POS));

	m_activePage = m_pages[page];
	if(m_activePage == NULL) {
		m_innerLayout.insertStretch(INSERT_POS, 1);
		return;
	}

	// Insert the new page into the layout and display it
	m_innerLayout.insertWidget(INSERT_POS, m_activePage);
	m_activePage->show();
}

void WizardWindow::setVisibleButtons(WizButtonsMask mask)
{
	// Clear layout
	while(m_buttonLayout.count() > 0)
		m_buttonLayout.removeItem(m_buttonLayout.itemAt(0));

	// Repopulate layout in the appropriate order for the target platform
#ifdef Q_OS_WIN
	addButtonToBar(mask, WizResetButton, &m_resetBtn);
	m_buttonLayout.addStretch();
	addButtonToBar(mask, WizBackButton, &m_backBtn);
	addButtonToBar(mask, WizNextButton, &m_nextBtn);
	addButtonToBar(mask, WizFinishButton, &m_finishBtn);
	addButtonToBar(mask, WizOKButton, &m_okBtn);
	addButtonToBar(mask, WizCancelButton, &m_cancelBtn);
#endif
#ifdef Q_OS_MAC
	addButtonToBar(mask, WizCancelButton, &m_cancelBtn);
	addButtonToBar(mask, WizOKButton, &m_okBtn);
	addButtonToBar(mask, WizResetButton, &m_resetBtn);
	m_buttonLayout.addStretch();
	addButtonToBar(mask, WizBackButton, &m_backBtn);
	addButtonToBar(mask, WizNextButton, &m_nextBtn);
	addButtonToBar(mask, WizFinishButton, &m_finishBtn);
#endif
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
#error Unsupported platform
#endif
}

/// <summary>
/// Enable the next, finish and ok buttons only if the user can continue.
/// </summary>
void WizardWindow::setCanContinue(bool canContinue)
{
	m_nextBtn.setEnabled(canContinue);
	m_finishBtn.setEnabled(canContinue);
	m_okBtn.setEnabled(canContinue);
}

void WizardWindow::setTitle(const QString &text)
{
	m_header.setTitle(text);
	setWindowTitle(QStringLiteral("%1 - %2").arg(APP_NAME).arg(text));
}

/// <summary>
/// Called by the controller whenever it wants to hide the wizard window.
/// </summary>
void WizardWindow::controllerFinished()
{
	hide();
	setController(WizNoController);
}

void WizardWindow::addButtonToBar(
	WizButtonsMask mask, WizButton btn, QPushButton *widget)
{
	if(mask & btn) {
		m_buttonLayout.addWidget(widget);
		widget->show();
	} else if(btn == WizNextButton &&
		!((mask & WizFinishButton) || (mask & WizOKButton)))
	{
		// If this is the next button and we are not going to be displaying any
		// other "completed" button then fill this area with a spacer. This is
		// to make sure that the "back" button, if it exists, remains at the
		// same location.
		m_buttonLayout.addSpacing(
			widget->sizeHint().width() + m_buttonLayout.spacing());
		widget->hide();
	} else
		widget->hide();
}

void WizardWindow::keyPressEvent(QKeyEvent *ev)
{
	switch(ev->key()) {
	default:
		ev->ignore();
		break;
	case Qt::Key_Escape:
		// ESC press is the same as cancel
		cancelEvent();
		break;
	}
}

void WizardWindow::closeEvent(QCloseEvent *ev)
{
	// Close event is the same as cancel
	if(m_activeController != NULL) {
		bool res = m_activeController->cancelWizard();
		if(!res) {
			// The controller rejected the event, keep the window open.
			ev->ignore();
		}
	}
}

void WizardWindow::backEvent()
{
	if(m_activeController != NULL)
		m_activeController->backPage();
}

void WizardWindow::nextEvent()
{
	if(m_activeController != NULL)
		m_activeController->nextPage();
}

void WizardWindow::finishEvent()
{
	if(m_activeController != NULL)
		m_activeController->nextPage();
	else
		controllerFinished();
}

void WizardWindow::okEvent()
{
	if(m_activeController != NULL)
		m_activeController->nextPage();
	else
		controllerFinished();
}

void WizardWindow::cancelEvent()
{
	if(m_activeController != NULL)
		m_activeController->cancelWizard();
	else
		controllerFinished();
}

void WizardWindow::resetEvent()
{
	if(m_activeController != NULL)
		m_activeController->resetPage();
}
