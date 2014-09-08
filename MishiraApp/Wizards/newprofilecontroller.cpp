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

#include "newprofilecontroller.h"
#include "application.h"
#include "audioencoder.h"
#include "constants.h"
#include "cpuusage.h"
#include "layergroup.h"
#include "profile.h"
#include "recommendations.h"
#include "scene.h"
#include "stylehelper.h"
#include "target.h"
#include "wizardwindow.h"
#include "x264encoder.h"
#include "Layers/color/colorlayer.h"
#include "Layers/deskcap/windowlayer.h"
#include "Layers/image/imagelayer.h"
#include "Layers/text/textlayer.h"
#include "Targets/file/filetargetsettingspage.h"
#include "Targets/hitbox/hitboxtargetsettingspage.h"
#include "Targets/rtmp/rtmptargetsettingspage.h"
#include "Targets/twitch/twitchtargetsettingspage.h"
#include "Targets/ustream/ustreamtargetsettingspage.h"
#include "Widgets/targettypewidget.h"
#include "Wizards/benchmarkpage.h"
#include "Wizards/benchmarkresultspage.h"
#include "Wizards/newprofilepage.h"
#include "Wizards/preparebenchmarkpage.h"
#include "Wizards/profilecompletepage.h"
#include "Wizards/scenetemplatepage.h"
#include "Wizards/targettypepage.h"
#include "Wizards/uploadpage.h"
#include "Wizards/welcomepage.h"
#include <QtWidgets/QMessageBox>

// How long should each benchmark test take?
const float	BENCHMARK_TEST_DURATION_SECS = 6.1f;

// How many seconds into the test should we start monitoring CPU usage?
const float	BENCHMARK_TEST_START_SECS = 1.0f;

// Reserve x% of the CPU for non-video encoding tasks while benchmarking
const float RESERVED_CPU_AMOUNT = 0.05f; // 5%

NewProfileController::NewProfileController(
	WizController type, WizardWindow *wizWin, WizShared *shared)
	: WizardController(type, wizWin, shared)
	, m_curPage(WizNoPage)
	, m_targetDefaults(&shared->netDefaults)
	, m_targetSettings()
	, m_exitDialog(NULL)
	, m_createBlankProfile(false)
	, m_vidHasHighAction(false)
	, m_recMaxVideoBitrate(0)
	, m_recMaxAudioBitrate(0)
	, m_maxCpuUsage(1.0f)
	, m_prevProfileName()
	, m_chosenResultIndex(0)
	, m_useTutorialScene(false)

	// Benchmark state
	, m_benchmarkActive(false)
	, m_numTotalTests(0)
	, m_numTestsDone(0)
	, m_curFrameNum(0)
	, m_curVidSizeIndex(0)
	, m_curPresetIndex(0)
	, m_curEncoder(NULL)
	, m_curUsage(NULL)
	, m_testResults()
{
	// Listen for frame and tick events
	connect(App, &Application::queuedFrameEvent,
		this, &NewProfileController::queuedFrameEvent);
	connect(App, &Application::realTimeTickEvent,
		this, &NewProfileController::realTimeTickEvent);
}

NewProfileController::~NewProfileController()
{
	if(m_exitDialog != NULL)
		delete m_exitDialog;
}

void NewProfileController::setPageBasedOnTargetType()
{
	switch(m_targetSettings.type) {
	default:
	case TrgtFileType:
		m_curPage = WizFileTargetSettingsPage;
		break;
	case TrgtRtmpType:
		m_curPage = WizRTMPTargetSettingsPage;
		break;
	case TrgtTwitchType:
		m_curPage = WizTwitchTargetSettingsPage;
		break;
	case TrgtUstreamType:
		m_curPage = WizUstreamTargetSettingsPage;
		break;
	case TrgtHitboxType:
		m_curPage = WizHitboxTargetSettingsPage;
		break;
	}
	m_wizWin->setPage(m_curPage);
}

void NewProfileController::resetWizard()
{
	if(m_shared->profIsWelcome)
		m_curPage = WizWelcomePage;
	else
		m_curPage = WizNewProfilePage;
	m_wizWin->setPage(m_curPage);
	m_wizWin->setTitle(tr("Create new profile"));

	// Initialize defaults and reset settings
	m_prevProfileName = QString();
	m_targetDefaults->name = tr("Unnamed");
	m_targetDefaults->setTargetSettingsToDefault();
	m_benchmarkActive = false;
	m_chosenResultIndex = 0;

	// Initialize values for the starting page
	resetPage();
}

bool NewProfileController::cancelWizard()
{
	// If this is the first time setup then we don't want to the user to
	// cancel. If they do treat it the same as exiting the entire application.
	if(m_shared->profIsWelcome) {
		showExitDialog();
		return false; // Don't accept the cancel yet
	}

	// Close the wizard window
	m_wizWin->controllerFinished();

	// Make sure that the benchmark has finished
	endBenchmark();

	leavingTempProfileArea();

	return true; // We have fully processed the cancel
}

void NewProfileController::showExitDialog()
{
	// Create dialogs here so they are always centered above the main window
	QMessageBox *box = new QMessageBox(m_wizWin);
	box->setWindowModality(Qt::ApplicationModal);
	box->setWindowTitle(tr("Exit Mishira?"));
	box->setText(tr(
		"Are you sure you want to exit Mishira?"));
	box->setIcon(QMessageBox::Question);
	box->setStandardButtons(
		QMessageBox::Ok | QMessageBox::Cancel);
	box->setDefaultButton(QMessageBox::Cancel);
	QAbstractButton *btn = box->button(QMessageBox::Ok);
	btn->setText(tr("Exit")); // Rename "OK"
	//btn->setContentsMargins(10, 0, 10, 0);
	connect(box, &QMessageBox::buttonClicked,
		this, &NewProfileController::exitDialogClosed);
	if(m_exitDialog != NULL)
		delete m_exitDialog; // Delayed delete
	m_exitDialog = box;

	box->show();
}

void NewProfileController::exitDialogClosed(QAbstractButton *button)
{
	// Was the cancel button clicked?
	QMessageBox::ButtonRole role = m_exitDialog->buttonRole(button);
	if(role != QMessageBox::AcceptRole)
		return;

	// Close the wizard window
	m_wizWin->controllerFinished();

	// Make sure that the benchmark has finished
	endBenchmark();

	// Mark settings to be deleted on exit
	App->setDeleteSettingsOnExit(true);

	// Exit the application
	App->exit(0);
}

void NewProfileController::resetWelcomePage()
{
	// Get UI object
	WelcomePage *page =
		static_cast<WelcomePage *>(m_wizWin->getActivePage());
	Ui_WelcomePage *ui = page->getUi();

	//page->setUpdatesEnabled(false);

	// Nothing to do on this page

	//page->setUpdatesEnabled(true);

	// Page is always valid
	m_wizWin->setCanContinue(true);

	// Hide the back and reset buttons
	m_wizWin->setVisibleButtons(WizStandardFirstButtons & ~WizResetButton);
}

void NewProfileController::resetNewProfilePage()
{
	// Get UI object
	NewProfilePage *page =
		static_cast<NewProfilePage *>(m_wizWin->getActivePage());
	Ui_NewProfilePage *ui = page->getUi();

	// Just in case the user clicks the "back" button after we already created
	// our temporary profile
	leavingTempProfileArea();
	m_prevProfileName = QString();

	page->setUpdatesEnabled(false);

	// Always default to using the wizard
	ui->wizardRadio->setChecked(true);

	// New profile name is always empty
	ui->nameEdit->setText(QString());

	// Reset validity and connect signal
	doQLineEditValidate(ui->nameEdit);
	m_wizWin->setCanContinue(page->isValid());
	connect(page, &NewProfilePage::validityMaybeChanged,
		m_wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	page->setUpdatesEnabled(true);

	// Hide the back button
	m_wizWin->setVisibleButtons(WizStandardFirstButtons);
}

void NewProfileController::resetTargetTypePage()
{
	// Use common "reset" button code for this page
	TargetTypePage *page =
		static_cast<TargetTypePage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_targetDefaults, true);

	// Show the back button
	m_wizWin->setVisibleButtons(WizStandardButtons);
}

void NewProfileController::resetFileTargetPage()
{
	// Use common "reset" button code for this page
	FileTargetSettingsPage *page =
		static_cast<FileTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_targetDefaults);
}

void NewProfileController::resetRTMPTargetPage()
{
	// Use common "reset" button code for this page
	RTMPTargetSettingsPage *page =
		static_cast<RTMPTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_targetDefaults);
}

void NewProfileController::resetTwitchTargetPage()
{
	// Use common "reset" button code for this page
	TwitchTargetSettingsPage *page =
		static_cast<TwitchTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_targetDefaults);
}

void NewProfileController::resetUstreamTargetPage()
{
	// Use common "reset" button code for this page
	UstreamTargetSettingsPage *page =
		static_cast<UstreamTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_targetDefaults);
}

void NewProfileController::resetHitboxTargetPage()
{
	// Use common "reset" button code for this page
	HitboxTargetSettingsPage *page =
		static_cast<HitboxTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_targetDefaults);
}

void NewProfileController::resetUploadPage()
{
	// Get UI object
	UploadPage *page =
		static_cast<UploadPage *>(m_wizWin->getActivePage());
	Ui_UploadPage *ui = page->getUi();

	page->setUpdatesEnabled(false);

	// No need to reset upload field as it shouldn't change at all once set

	// Select "Mb/s" by default if the upload field is empty
	if(ui->uploadEdit->text().isEmpty())
		ui->uploadUnitsCombo->setCurrentIndex(1);

	// Reset validity and connect signal
	doQLineEditValidate(ui->uploadEdit);
	m_wizWin->setCanContinue(page->isValid());
	connect(page, &UploadPage::validityMaybeChanged,
		m_wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	page->setUpdatesEnabled(true);
}

void NewProfileController::resetPrepareBenchmarkPage()
{
	// Get UI object
	PrepareBenchmarkPage *page =
		static_cast<PrepareBenchmarkPage *>(m_wizWin->getActivePage());
	Ui_PrepareBenchmarkPage *ui = page->getUi();

	page->setUpdatesEnabled(false);

	// Always default to using less than 25% of the CPU
	ui->lowCpuRadio->setChecked(true);

	page->setUpdatesEnabled(true);

	// Page is always valid
	m_wizWin->setCanContinue(true);

	// Show all standard buttons
	m_wizWin->setVisibleButtons(WizStandardButtons);
}

void NewProfileController::resetBenchmarkPage()
{
	// Get UI object
	BenchmarkPage *page =
		static_cast<BenchmarkPage *>(m_wizWin->getActivePage());
	Ui_BenchmarkPage *ui = page->getUi();

	page->setUpdatesEnabled(false);

	// Reset benchmark progress to the beginning
	ui->progressBar->setValue(0);

	page->setUpdatesEnabled(true);

	// Hide all buttons except back and cancel
	m_wizWin->setVisibleButtons(WizBackButton | WizCancelButton);

	// Start the actual benchmark
	startBenchmark();
}

void NewProfileController::resetBenchmarkResultsPage()
{
	// Get UI object
	BenchmarkResultsPage *page =
		static_cast<BenchmarkResultsPage *>(m_wizWin->getActivePage());
	Ui_BenchmarkResultsPage *ui = page->getUi();

	page->setUpdatesEnabled(false);

	// Find the recommended setting. We use the highest resolution that has a
	// "very fast" preset or better, if none is found we slowly lower the
	// preset until one is found. We only ever recommend 30fps settings.
	// TODO: As we are more lenient about bitrate vs size there is a chance
	// that we'll recommend the higher resolution with a lower than recommended
	// bitrate.
	bool foundRecommended = false;
	int bestRecommended = 0;
	int bestHeight = 0;
	for(int i = X264VeryFastPreset; i >= 0; i--) {
		X264Preset searchPreset = (X264Preset)i;
		for(int j = 0; j < m_testResults.size(); j++) {
			const TestResults &results = m_testResults.at(j);
			if(results.vidPreset >= searchPreset &&
				results.vidSize.height() > bestHeight &&
				results.framerate == Fraction(30, 1))
			{
				foundRecommended = true;
				bestRecommended = j;
				bestHeight = results.vidSize.height();
			}
		}
		if(foundRecommended)
			break;
	}
	if(foundRecommended)
		m_testResults[bestRecommended].recommended = true;

	// Populate benchmark results list and select the recommended setting
	ui->listWidget->setEnabled(true);
	ui->listWidget->clearSelection();
	ui->listWidget->clear();
	for(int i = 0; i < m_testResults.size(); i++) {
		const TestResults &results = m_testResults.at(i);
		QString friendly = tr("%1p, %2 fps, %L3 Kb/s, %4 - %5% CPU usage")
			.arg(results.vidSize.height())
			.arg(qRound(results.framerate.asFloat())) // Assume 30 or 60 Hz
			.arg(results.bitrate)
			.arg(tr(X264PresetQualityStrings[results.vidPreset]))
			.arg(qRound((results.ourAvgCpu + RESERVED_CPU_AMOUNT) * 100.0f));
		if(results.recommended)
			friendly = tr("%1 (Recommended)").arg(friendly);

		// Create list widget item
		QListWidgetItem *item = new QListWidgetItem(friendly);
		item->setData(Qt::UserRole, i);

		// Add item to list and select it if it is the recommended setting
		ui->listWidget->insertItem(0, item);
		if(results.recommended) {
			item->setSelected(true);
			ui->listWidget->setCurrentItem(item);
		}
	}

	if(m_testResults.size() == 0) {
		// No valid results were found
		ui->listWidget->setEnabled(false);
		QListWidgetItem *item = new QListWidgetItem(
			tr("No possible settings were found. Please try again with a higher CPU usage limit."));
		ui->listWidget->addItem(item);
	}

	// Reset validity and connect signal
	m_wizWin->setCanContinue(page->isValid());
	connect(page, &BenchmarkResultsPage::validityMaybeChanged,
		m_wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	page->setUpdatesEnabled(true);

	// Show all standard buttons
	m_wizWin->setVisibleButtons(WizStandardButtons);
}

void NewProfileController::resetSceneTemplatePage()
{
	// Get UI object
	SceneTemplatePage *page =
		static_cast<SceneTemplatePage *>(m_wizWin->getActivePage());
	Ui_SceneTemplatePage *ui = page->getUi();

	page->setUpdatesEnabled(false);

	// Always default to using the tutorial layer set
	ui->tutorialRadio->setChecked(true);

	page->setUpdatesEnabled(true);

	// Page is always valid
	m_wizWin->setCanContinue(true);

	// Show all standard buttons
	m_wizWin->setVisibleButtons(WizStandardButtons);
}

void NewProfileController::resetProfileCompletePage()
{
	// Get UI object
	ProfileCompletePage *page =
		static_cast<ProfileCompletePage *>(m_wizWin->getActivePage());
	Ui_ProfileCompletePage *ui = page->getUi();

	//page->setUpdatesEnabled(false);

	// Nothing to do on this page

	//page->setUpdatesEnabled(true);

	// Page is always valid
	m_wizWin->setCanContinue(true);

	// This is always the last page in the wizard and hide the reset button
	m_wizWin->setVisibleButtons(WizStandardLastButtons & ~WizResetButton);
}

void NewProfileController::resetPage()
{
	// Reset whatever page we are currently on
	switch(m_curPage) {
	default:
	case WizWelcomePage:
		resetWelcomePage();
		break;
	case WizNewProfilePage:
		resetNewProfilePage();
		break;
	case WizTargetTypePage:
		resetTargetTypePage();
		break;

	case WizFileTargetSettingsPage:
		resetFileTargetPage();
		break;
	case WizRTMPTargetSettingsPage:
		resetRTMPTargetPage();
		break;
	case WizTwitchTargetSettingsPage:
		resetTwitchTargetPage();
		break;
	case WizUstreamTargetSettingsPage:
		resetUstreamTargetPage();
		break;
	case WizHitboxTargetSettingsPage:
		resetHitboxTargetPage();
		break;

	case WizUploadPage:
		resetUploadPage();
		break;
	case WizPrepareBenchmarkPage:
		resetPrepareBenchmarkPage();
		break;
	case WizBenchmarkPage:
		resetBenchmarkPage();
		break;
	case WizBenchmarkResultsPage:
		resetBenchmarkResultsPage();
		break;
	case WizSceneTemplatePage:
		resetSceneTemplatePage();
		break;
	case WizProfileCompletePage:
		resetProfileCompletePage();
		break;
	}
}

void NewProfileController::backPage()
{
	// Return to the previous page in all cases
	switch(m_curPage) {
	default:
		break;
	case WizWelcomePage:
	case WizNewProfilePage:
		// We don't know where to go!
		break;
	case WizTargetTypePage:
		if(m_shared->profIsWelcome)
			m_curPage = WizWelcomePage;
		else
			m_curPage = WizNewProfilePage;
		m_wizWin->setPage(m_curPage);
		resetPage();
		break;

	case WizFileTargetSettingsPage:
	case WizRTMPTargetSettingsPage:
	case WizTwitchTargetSettingsPage:
	case WizUstreamTargetSettingsPage:
	case WizHitboxTargetSettingsPage:
		m_curPage = WizTargetTypePage;
		m_wizWin->setPage(m_curPage);
		resetPage();
		break;

	case WizUploadPage:
		setPageBasedOnTargetType();
		resetPage();
		break;
	case WizPrepareBenchmarkPage:
		if(m_targetSettings.type == TrgtFileType)
			setPageBasedOnTargetType();
		else {
			m_curPage = WizUploadPage;
			m_wizWin->setPage(m_curPage);
		}
		resetPage();
		break;
	case WizBenchmarkPage:
		endBenchmark();
		m_curPage = WizPrepareBenchmarkPage;
		m_wizWin->setPage(m_curPage);
		resetPage();
		break;
	case WizBenchmarkResultsPage:
		// Skip straight back to the benchmark preparation page
		m_curPage = WizPrepareBenchmarkPage;
		m_wizWin->setPage(m_curPage);
		resetPage();
		break;
	case WizSceneTemplatePage:
		m_curPage = WizBenchmarkResultsPage;
		m_wizWin->setPage(m_curPage);
		resetPage();
		break;
	case WizProfileCompletePage:
		if(m_createBlankProfile && !m_shared->profIsWelcome)
			m_curPage = WizNewProfilePage;
		else
			m_curPage = WizSceneTemplatePage;
		m_wizWin->setPage(m_curPage);
		resetPage();
		break;
	}
}

void NewProfileController::nextWelcomePage()
{
	// Get UI object
	WelcomePage *page =
		static_cast<WelcomePage *>(m_wizWin->getActivePage());
	Ui_WelcomePage *ui = page->getUi();

	// Do nothing as we want to modify the current profile (Which is already
	// blank)

	// Go to the next page
	m_curPage = WizTargetTypePage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextNewProfilePage()
{
	// Get UI object
	NewProfilePage *page =
		static_cast<NewProfilePage *>(m_wizWin->getActivePage());
	Ui_NewProfilePage *ui = page->getUi();

	// Save fields
	m_createBlankProfile = ui->blankRadio->isChecked();

	//-------------------------------------------------------------------------
	// Create the new profile now and switch to it. If the user cancels at a
	// later time then we just delete the profile.

	// Remember the current profile just in case the user cancels
	m_prevProfileName = App->getProfile()->getName();

	// Create new profile. This should always be a brand new profile as we
	// validated it before the user could press the "next" button.
	QString profileName = ui->nameEdit->text().trimmed();
	App->changeActiveProfile(profileName, NULL, false, true);

	//-------------------------------------------------------------------------

	// Go to the next page based on the user's selection
	if(m_createBlankProfile)
		m_curPage = WizProfileCompletePage;
	else
		m_curPage = WizTargetTypePage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextTargetTypePage()
{
	// Use common "next" button code for this page
	TargetTypePage *page =
		static_cast<TargetTypePage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_targetSettings);

	// Go to the target-specific settings page
	setPageBasedOnTargetType();
	resetPage();
}

void NewProfileController::nextFileTargetPage()
{
	// Use common "next" button code for this page
	FileTargetSettingsPage *page =
		static_cast<FileTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_targetSettings);

	//-------------------------------------------------------------------------
	// Define fake "upload speed" parameters so the benchmark code knows what
	// to do.

	m_vidHasHighAction = true;

	// Calculate maximum safe video and audio bitrates
	float uploadBitsPerSec = 100.0f * 1024.0f * 8.0f; // 100 MB/s
	Recommendations::maxVidAudBitratesFromTotalUpload(
		uploadBitsPerSec, false, m_recMaxVideoBitrate,
		m_recMaxAudioBitrate);

	// Cap video bitrate to a sane amount. No need to do this as we want to
	// have higher bitrates for local files
	//m_recMaxVideoBitrate = Recommendations::capVidBitrateToSaneValue(
	//	m_recMaxVideoBitrate, Fraction(60, 1), m_vidHasHighAction, true);

	//-------------------------------------------------------------------------

	// Skip the upload speed testing page and go straight to the benchmark
	// preparation page
	m_curPage = WizPrepareBenchmarkPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextRTMPTargetPage()
{
	// Use common "next" button code for this page
	RTMPTargetSettingsPage *page =
		static_cast<RTMPTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_targetSettings);

	// Go to the upload speed testing page
	m_curPage = WizUploadPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextTwitchTargetPage()
{
	// Use common "next" button code for this page
	TwitchTargetSettingsPage *page =
		static_cast<TwitchTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_targetSettings);

	// Go to the upload speed testing page
	m_curPage = WizUploadPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextUstreamTargetPage()
{
	// Use common "next" button code for this page
	UstreamTargetSettingsPage *page =
		static_cast<UstreamTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_targetSettings);

	// Go to the upload speed testing page
	m_curPage = WizUploadPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextHitboxTargetPage()
{
	// Use common "next" button code for this page
	HitboxTargetSettingsPage *page =
		static_cast<HitboxTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_targetSettings);

	// Go to the upload speed testing page
	m_curPage = WizUploadPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextUploadPage()
{
	// Get UI object
	UploadPage *page =
		static_cast<UploadPage *>(m_wizWin->getActivePage());
	Ui_UploadPage *ui = page->getUi();

	//-------------------------------------------------------------------------
	// Calculate maximum video and audio bitrates and remember content type

	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
	//*************************************************************************
	// Don't forget to update the fake upload information in
	// `nextFileTargetPage()` if you change anything below.
	//*************************************************************************
	// WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

	m_vidHasHighAction = ui->highActionRadio->isChecked();
	bool isOnlineGamer = ui->gamerBox->isChecked();
	bool hasTranscoder = ui->transcodingBox->isChecked();

	// Get upload speed as bits per second
	float unitMultiplier = 1.0f;
	switch(ui->uploadUnitsCombo->currentIndex()) {
	default:
	case 0: // Kb/s
		unitMultiplier = 1.0f;
		break;
	case 1: // Mb/s
		unitMultiplier = 1000.0f;
		break;
	case 2: // KB/s
		unitMultiplier = 1024.0f * 8.0f / 1000.0f;
		break;
	case 3: // MB/s
		unitMultiplier = 1024.0f * 1024.0f * 8.0f / 1000.0f;
		break;
	}
	float uploadBitsPerSec =
		ui->uploadEdit->text().toFloat() * unitMultiplier; // Kb/s

	// Calculate maximum safe video and audio bitrates from upload speed
	Recommendations::maxVidAudBitratesFromTotalUpload(
		uploadBitsPerSec, isOnlineGamer, m_recMaxVideoBitrate,
		m_recMaxAudioBitrate);

	// Cap video bitrate to a sane amount
	m_recMaxVideoBitrate = Recommendations::capVidBitrateToSaneValue(
		m_recMaxVideoBitrate, Fraction(60, 1), m_vidHasHighAction,
		hasTranscoder);

	//-------------------------------------------------------------------------

	// Go to the benchmark preparation page
	m_curPage = WizPrepareBenchmarkPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextPrepareBenchmarkPage()
{
	// Get UI object
	PrepareBenchmarkPage *page =
		static_cast<PrepareBenchmarkPage *>(m_wizWin->getActivePage());
	Ui_PrepareBenchmarkPage *ui = page->getUi();

	// Save max CPU usage. As we're only testing the video encoder usage and
	// not full Mishira usage we reserve an additional 5% CPU for other Mishira
	// tasks that usually occur during broadcasting.
	m_maxCpuUsage = 0.8f; // Use no more than 80% of the CPU just in case
	if(ui->medCpuRadio->isChecked())
		m_maxCpuUsage = 0.4f; // Limit to 40% usage
	else if(ui->lowCpuRadio->isChecked())
		m_maxCpuUsage = 0.2f; // Limit to 20% usage
	m_maxCpuUsage -= RESERVED_CPU_AMOUNT; // Reserve 5% for other Mishira tasks

	// Go to the actual benchmark page
	m_curPage = WizBenchmarkPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextBenchmarkPage()
{
	// Should never be called as there is no "next" button to click
}

void NewProfileController::nextBenchmarkResultsPage()
{
	// Get UI object
	BenchmarkResultsPage *page =
		static_cast<BenchmarkResultsPage *>(m_wizWin->getActivePage());
	Ui_BenchmarkResultsPage *ui = page->getUi();

	// Save selected test result
	m_chosenResultIndex = 0;
	for(int i = 0; i < m_testResults.size(); i++) {
		QListWidgetItem *item = ui->listWidget->item(i);
		if(item->isSelected()) {
			// Use first selected result
			m_chosenResultIndex = item->data(Qt::UserRole).toInt();
			break;
		}
	}

	// Go to the scene template selection page
	m_curPage = WizSceneTemplatePage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextSceneTemplatePage()
{
	// Get UI object
	SceneTemplatePage *page =
		static_cast<SceneTemplatePage *>(m_wizWin->getActivePage());
	Ui_SceneTemplatePage *ui = page->getUi();

	// Save fields
	m_useTutorialScene = ui->tutorialRadio->isChecked();

	// Go to the profile completion page
	m_curPage = WizProfileCompletePage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::nextProfileCompletePage()
{
	// Get UI object
	ProfileCompletePage *page =
		static_cast<ProfileCompletePage *>(m_wizWin->getActivePage());
	Ui_ProfileCompletePage *ui = page->getUi();

	// Create the actual profile
	createProfile();

	// Our wizard has completed, close the wizard window
	m_wizWin->controllerFinished();

	// Completely reload the profile as we might have changed framerates. This
	// also displays the main page in the main window.
	App->changeActiveProfile(App->getProfile()->getName(), NULL, true);
}

void NewProfileController::nextPage()
{
	// Save and continue from whatever page we are currently on
	switch(m_curPage) {
	default:
		break;
	case WizWelcomePage:
		nextWelcomePage();
		break;
	case WizNewProfilePage:
		nextNewProfilePage();
		break;
	case WizTargetTypePage:
		nextTargetTypePage();
		break;

	case WizFileTargetSettingsPage:
		nextFileTargetPage();
		break;
	case WizRTMPTargetSettingsPage:
		nextRTMPTargetPage();
		break;
	case WizTwitchTargetSettingsPage:
		nextTwitchTargetPage();
		break;
	case WizUstreamTargetSettingsPage:
		nextUstreamTargetPage();
		break;
	case WizHitboxTargetSettingsPage:
		nextHitboxTargetPage();
		break;

	case WizUploadPage:
		nextUploadPage();
		break;
	case WizPrepareBenchmarkPage:
		nextPrepareBenchmarkPage();
		break;
	case WizBenchmarkPage:
		// Should never be possible
		nextBenchmarkPage();
		break;
	case WizBenchmarkResultsPage:
		nextBenchmarkResultsPage();
		break;
	case WizSceneTemplatePage:
		nextSceneTemplatePage();
		break;
	case WizProfileCompletePage:
		nextProfileCompletePage();
		break;
	}
}

void NewProfileController::leavingTempProfileArea()
{
	if(m_prevProfileName.isEmpty())
		return;

	// We are currently using a temporary profile, delete it and revert to the
	// old one
	QString curProfileName = App->getProfile()->getName();
	App->changeActiveProfile(m_prevProfileName);
	QFile::remove(Profile::getProfileFilePath(curProfileName));
	App->updateMenuBar();
}

/// <summary>
/// Creates the actual profile based on the wizard's final settings.
/// </summary>
void NewProfileController::createProfile()
{
	appLog() << "New profile wizard complete, creating profile...";
	Profile *profile = App->getProfile();

	// If the user skipped the majority of the wizard by chosing to create a
	// "blank profile" then do exactly that
	if(m_createBlankProfile) {
		// We don't need to set anything here as `Profile::loadDefaults()` was
		// already called and we haven't changed anything. All we want to do is
		// add a slightly nicer background to the scene.
		createBlankScenes();
		return;
	}

	const TestResults &testResults = m_testResults[m_chosenResultIndex];

	// Calculate canvas size from output video size. We prefer 720p and 1080p
	// canvases. We don't go lower than 720p as the aliasing artifacts are
	// extremely ugly.
	QSize canvasSize = testResults.vidSize;
	//if(testResults.vidSize.height() <= 540)
	//	canvasSize = QSize(960, 540);
	//else
	if(testResults.vidSize.height() <= 720)
		canvasSize = QSize(1280, 720);
	else if(testResults.vidSize.height() <= 1080)
		canvasSize = QSize(1920, 1080);
	profile->setCanvasSize(canvasSize);

	// Set selected framerate
	profile->setVideoFramerate(testResults.framerate);

	// Create video encoder
	X264Options vidOpt;
	vidOpt.bitrate = testResults.bitrate;
	vidOpt.preset = testResults.vidPreset;
	vidOpt.keyInterval = 0; // Default keyframe interval
	VideoEncoder *vidEnc = profile->getOrCreateX264VideoEncoder(
		testResults.vidSize, SclrSnapToInnerScale, GfxBilinearFilter, vidOpt);

	// Create audio encoder
	FdkAacOptions audOpt;
	audOpt.bitrate = m_recMaxAudioBitrate;
	AudioEncoder *audEnc = profile->getOrCreateFdkAacAudioEncoder(audOpt);

	// Create target
	Target *target = NULL;
	switch(m_targetSettings.type) {
	default:
	case TrgtFileType: {
		FileTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.filename = m_targetSettings.fileFilename;
		opt.fileType = m_targetSettings.fileType;
		target = profile->createFileTarget(m_targetSettings.name, opt);
		break; }
	case TrgtRtmpType: {
		RTMPTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.url = m_targetSettings.rtmpUrl;
		opt.streamName = m_targetSettings.rtmpStreamName;
		opt.hideStreamName = m_targetSettings.rtmpHideStreamName;
		opt.padVideo = m_targetSettings.rtmpPadVideo;
		target = profile->createRtmpTarget(m_targetSettings.name, opt);
		break; }
	case TrgtTwitchType: {
		TwitchTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.url = m_targetSettings.rtmpUrl;
		opt.streamKey = m_targetSettings.rtmpStreamName;
		opt.username = m_targetSettings.username;
		target = profile->createTwitchTarget(m_targetSettings.name, opt);
		break; }
	case TrgtUstreamType: {
		UstreamTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.url = m_targetSettings.rtmpUrl;
		opt.streamKey = m_targetSettings.rtmpStreamName;
		opt.padVideo = m_targetSettings.rtmpPadVideo;
		target = profile->createUstreamTarget(m_targetSettings.name, opt);
		break; }
	case TrgtHitboxType: {
		HitboxTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.url = m_targetSettings.rtmpUrl;
		opt.streamKey = m_targetSettings.rtmpStreamName;
		opt.username = m_targetSettings.username;
		target = profile->createHitboxTarget(m_targetSettings.name, opt);
		break; }
	}
	target->setEnabled(true);

	// Populate the scene with the chosen layer set
	if(m_useTutorialScene)
		createTutorialScenes();
	else
		createBlankScenes();

	appLog() << "Finished creating new profile";
}

/// <summary>
/// Creates and adds a background wallpaper to the active scene.
/// </summary>
/// <returns>The "Background" group</returns>
LayerGroup *NewProfileController::createBlankScenes()
{
	Profile *profile = App->getProfile();

	// Create containing group
	LayerGroup *group = profile->createLayerGroup(tr("Background"));
	profile->getActiveScene()->addGroup(group, true);

	// Create wallpaper layer
	QDir mediaDir = QDir(App->applicationDirPath() + "/Media");
	ImageLayer *imgLayer = static_cast<ImageLayer *>(
		group->createLayer(LyrImageLayerTypeId, tr("Wallpaper")));
	imgLayer->setVisible(true);
	//if(profile->getCanvasSize().height() <= 540) {
	//	imgLayer->setFilename(
	//		QDir::toNativeSeparators(mediaDir.absoluteFilePath(
	//		"Mishira logo background 540p.jpg")));
	//} else {
	imgLayer->setFilename(
		QDir::toNativeSeparators(mediaDir.absoluteFilePath(
		"Mishira logo background 1080p.jpg")));
	//}
	imgLayer->setScaling(LyrSnapToOuterScale);

	return group;
}

void NewProfileController::createTutorialScenes()
{
	LayerGroup *bgGroup = createBlankScenes();

	Profile *profile = App->getProfile();
	QSize canvasSize = profile->getCanvasSize();

	// Style helpers
	const int fontSize = (canvasSize.height() >= 1080) ? 40 : 26;
	QString EMPTY_LINE_TAGS = QString(
		"<p style=\"-qt-paragraph-type:empty; margin: 0; font-size: %1pt;\">"
		"<br /></p>").arg(fontSize);
	QString P_LEFT_TAG = QString(
		"<p style=\"margin: 0; font-size: %1pt; font-weight: bold; "
		"color: #fff;\">").arg(fontSize);
	QString P_CENTER_TAG = QString(
		"<p align=\"center\" style=\"margin: 0; font-size: %1pt; "
		"font-weight: bold; color: #fff;\">").arg(fontSize);
	QString P_RIGHT_TAG = QString(
		"<p align=\"right\" style=\"margin: 0; font-size: %1pt; "
		"font-weight: bold; color: #fff;\">").arg(fontSize);
	QString SPAN_YELLOW_TAG(
		"<span style=\"color: #ffe699;\">");
	QString SPAN_RED_TAG(
		"<span style=\"color: #ff9999;\">");

	// Common layer rectangles
	QRect FULL_WITH_MARGIN_RECT(
		10, 10, canvasSize.width() - 20, canvasSize.height() - 20);
	QRect WIDE_LEFT_WITH_MARGIN_RECT = FULL_WITH_MARGIN_RECT;
	WIDE_LEFT_WITH_MARGIN_RECT.setWidth(canvasSize.width() * 6 / 11);
	QRect CAPTURE_LEFT_WITH_MARGIN_RECT(
		10, canvasSize.height() * 4 / 15,
		canvasSize.width() * 7 / 12, canvasSize.height() * 4 / 7);
	QRect BIG_EXAMPLE_BOX_RECT(0, 0, 430, 250);
	QRect SMALL_EXAMPLE_BOX_RECT(0, 0, 260, 120);
	if(canvasSize.height() >= 1080) {
		BIG_EXAMPLE_BOX_RECT = QRect(0, 0, 645, 375);
		SMALL_EXAMPLE_BOX_RECT = QRect(0, 0, 390, 180);
	}

	//-------------------------------------------------------------------------
	// "Introduction" scene

	// Rename default scene
	Scene *scene = profile->getActiveScene();
	scene->setName(tr("Introduction"));

	// "Tutorial" group
	LayerGroup *group = profile->createLayerGroup(tr("Tutorial"));
	scene->addGroup(group, true, 0);

	// "Top descriptions" layer
	QString text = P_RIGHT_TAG +
		tr("The broadcast button begins recording! \xe2\x86\x97") +
		QStringLiteral("</p>") +
		EMPTY_LINE_TAGS +

		P_RIGHT_TAG +
		tr("Targets are where you are recording/broadcasting to! "
		"\xe2\x86\x97") +
		QStringLiteral("</p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("\xe2\x86\x96 Scene layers are the stuff that you can see here!") +
		QStringLiteral("</p>");
	createTextLayer(group, tr("Top descriptions"), text,
		FULL_WITH_MARGIN_RECT, LyrTopCenterAlign);

	// "Next step text" layer
	text = P_CENTER_TAG + SPAN_YELLOW_TAG +
		tr("Switch to the &quot;The preview area&quot; scene<br />"
		"in the bottom-left once you are ready to<br />"
		"move on to the next tutorial by clicking on it.") +
		QStringLiteral("</span></p>");
	createTextLayer(group, tr("Next step text"), text,
		FULL_WITH_MARGIN_RECT, LyrMiddleCenterAlign);

	// "Bottom descriptions" layer
	text = P_RIGHT_TAG +
		tr("The audio mixer controls what can be heard in your recordings! "
		"\xe2\x86\x97") +
		QStringLiteral("</p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("\xe2\x86\x90 Scenes allow you to quickly switch between different "
		"sets of scene layers!") +
		QStringLiteral("</p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("\xe2\x86\x93 These control the zoom of this preview area!") +
		QStringLiteral("</p>");
	createTextLayer(group, tr("Bottom descriptions"), text,
		FULL_WITH_MARGIN_RECT, LyrBottomCenterAlign);

	//-------------------------------------------------------------------------
	// "The preview area" scene

	// Create the new scene
	scene = profile->createScene(tr("The preview area"));

	// "Tutorial" group
	group = profile->createLayerGroup(tr("Tutorial"));
	scene->addGroup(group, true);

	// "Tutorial text" layer
	text = P_LEFT_TAG +
		tr("1. You can move the canvas around by clicking and dragging with "
		"the middle mouse button anywhere in this preview area. You can also "
		"hold down the space bar so you can use the left mouse button "
		"instead.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Try moving the canvas now.") +
		QStringLiteral("</span></p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("2. You can zoom in and out by using the mouse wheel or by using "
		"the slider just below this preview area.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Try zooming now.") +
		QStringLiteral("</span></p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("3. If you would like Mishira to automatically make the canvas "
		"fill the entire preview area then make sure that the checkbox "
		"labeled &quot;Automatically zoom to fill window&quot; below this "
		"preview area is checked.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Click this checkbox now to reset the position of the canvas.") +
		QStringLiteral("</span></p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("4.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Switch to the &quot;Layer manipulation&quot; scene in the "
		"bottom-left once you are ready to move on to the next tutorial.") +
		QStringLiteral("</span></p>");
	createTextLayer(group, tr("Tutorial text"), text,
		FULL_WITH_MARGIN_RECT, LyrTopCenterAlign);

	// "Background" group
	scene->addGroup(bgGroup, true);

	//-------------------------------------------------------------------------
	// "Layer manipulation" scene

	// Create the new scene
	scene = profile->createScene(tr("Layer manipulation"));

	// "Tutorial" group
	group = profile->createLayerGroup(tr("Tutorial"));
	scene->addGroup(group, true);

	// "Tutorial text" layer
	text = P_LEFT_TAG +
		tr("1. You can move or resize scene layers by first clicking on them "
		"in this preview area (Or in the layer list to the left) and then "
		"dragging them around.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Try it now with the box to the right \xe2\x86\x92") +
		QStringLiteral("</span></p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("2. Scene layers are always sorted into groups. When you click the "
		"checkbox next to the group name in the layer list to the left it "
		"will show or hide all the layers in that group at once.") +
		QStringLiteral("</p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("3. You can add new layers or layer groups by clicking the "
		"&quot;+&quot; buttons in the scene layer toolbox to the top-left.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Try adding a new &quot;color/gradient layer&quot; to the "
		"&quot;Tutorial&quot; group now!") +
		QStringLiteral("</span></p>");
	createTextLayer(group, tr("Tutorial text"), text,
		WIDE_LEFT_WITH_MARGIN_RECT, LyrTopCenterAlign);

	// "Next step text" layer
	text = P_CENTER_TAG +
		tr("4.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Switch to the &quot;Window/game capture&quot; scene in the "
		"bottom-left once you are ready to move on to the next tutorial.") +
		QStringLiteral("</span></p>");
	QRect rect = FULL_WITH_MARGIN_RECT;
	rect.setTop(canvasSize.height() / 2);
	rect.setLeft(WIDE_LEFT_WITH_MARGIN_RECT.right());
	createTextLayer(group, tr("Next step text"), text,
		rect, LyrMiddleCenterAlign);

	// "Red box" layer
	ColorLayer *boxLayer = static_cast<ColorLayer *>(
		group->createLayer(LyrColorLayerTypeId, tr("Red box")));
	boxLayer->setVisible(true);
	boxLayer->setRect(BIG_EXAMPLE_BOX_RECT.translated(
		canvasSize.width() - BIG_EXAMPLE_BOX_RECT.width() - 40, 40));
	boxLayer->setPattern(VerticalPattern);
	boxLayer->setAColor(StyleHelper::RedLighterColor);
	boxLayer->setBColor(StyleHelper::RedDarkerColor);

	// "Background" group
	scene->addGroup(bgGroup, true);

	//-------------------------------------------------------------------------
	// "Window/game capture" scene

	// Create the new scene
	scene = profile->createScene(tr("Window/game capture"));

	// "Tutorial" group
	group = profile->createLayerGroup(tr("Tutorial"));
	scene->addGroup(group, true);

	// "Tutorial text" layer
	text = P_CENTER_TAG +
		tr("Below is a capture of the Mishira main window. By adding a window "
		"capture to your scene you can record what is visible inside that "
		"window even if you have another window above it.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Try capturing a different window by double-clicking on the "
		"existing capture below!") +
		QStringLiteral("</span></p>");
	createTextLayer(group, tr("Tutorial text"), text,
		FULL_WITH_MARGIN_RECT, LyrTopCenterAlign);

	// "More tutorial text" layer
	text = P_LEFT_TAG +
		tr("It is recommended to use window capture instead of monitor "
		"capture whenever possible as it is faster and results in smoother "
		"recordings.") +
		QStringLiteral(" ") + SPAN_RED_TAG +
		tr("You definitely want to use window capture for video games!") +
		QStringLiteral("</span></p>");
	rect = CAPTURE_LEFT_WITH_MARGIN_RECT;
	rect.setRight(FULL_WITH_MARGIN_RECT.right());
	rect.setLeft(CAPTURE_LEFT_WITH_MARGIN_RECT.right() + 30);
	createTextLayer(group, tr("More tutorial text"), text,
		rect, LyrMiddleCenterAlign);

	// "Next step text" layer
	text = P_CENTER_TAG + SPAN_YELLOW_TAG +
		tr("Switch to the &quot;More about layers&quot; scene in the "
		"bottom-left<br />"
		"once you are ready to move on to the next tutorial.") +
		QStringLiteral("</span></p>");
	createTextLayer(group, tr("Next step text"), text,
		FULL_WITH_MARGIN_RECT, LyrBottomCenterAlign);

	// "Window capture" layer. We must add all variations of the main window
	// title to make sure that the layer actually displays a window during the
	// tutorial.
	WindowLayer *winLayer = static_cast<WindowLayer *>(
		group->createLayer(LyrWindowLayerTypeId, tr("Window capture")));
	winLayer->setVisible(true);
	winLayer->setRect(CAPTURE_LEFT_WITH_MARGIN_RECT);
	winLayer->beginAddingWindows();
	winLayer->addWindow("Mishira.exe", APP_NAME);
	winLayer->addWindow("Mishira.exe", QStringLiteral("%1 - %2")
		.arg(APP_NAME).arg(App->getProfile()->getName()));
	winLayer->finishAddingWindows();

	// "Background" group
	scene->addGroup(bgGroup, true);

	//-------------------------------------------------------------------------
	// "More about layers" scene

	// Create the new scene
	scene = profile->createScene(tr("More about layers"));

	// "Tutorial" group
	group = profile->createLayerGroup(tr("Tutorial"));
	scene->addGroup(group, true);

	// "Tutorial text" layer
	text = P_LEFT_TAG +
		tr("1. Layers can be reorganised quickly in the layer list to the "
		"left by clicking and dragging the grip area to the right of their "
		"name. The same can also be done with scenes.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Try reorganising the three boxes in the layer list now.") +
		QStringLiteral("</span></p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("2. Notice the little circular arrow icon in the layer list to the "
		"left next to the &quot;Background&quot; group? That indicates that "
		"the group is shared between multiple scenes. Any modifications to "
		"the layers in that group will be applied to all of the scenes that "
		"the group is shared with. Groups can be shared between multiple "
		"scenes by right-clicking them and selecting &quot;Share group "
		"with&quot;.") +
		QStringLiteral(" ") + SPAN_RED_TAG +
		tr("Shared groups are great for backgrounds and watermarks!") +
		QStringLiteral("</span></p>") +
		EMPTY_LINE_TAGS +

		P_LEFT_TAG +
		tr("3.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("Switch to the &quot;Conclusion&quot; scene in the bottom-left "
		"once you are ready to move on to the next tutorial.") +
		QStringLiteral("</span></p>");
	createTextLayer(group, tr("Tutorial text"), text,
		FULL_WITH_MARGIN_RECT, LyrTopCenterAlign);

	// "Boxes" group
	group = profile->createLayerGroup(tr("Boxes"));
	scene->addGroup(group, true);

	// "Green box" layer
	rect = SMALL_EXAMPLE_BOX_RECT.translated( // Bottom-right box position
		canvasSize.width() - SMALL_EXAMPLE_BOX_RECT.width() - 20,
		canvasSize.height() - SMALL_EXAMPLE_BOX_RECT.height() - 20);
	boxLayer = static_cast<ColorLayer *>(
		group->createLayer(LyrColorLayerTypeId, tr("Green box")));
	boxLayer->setVisible(true);
	boxLayer->setRect(
		rect.translated(-rect.width() / 4, -rect.height() / 3));
	boxLayer->setPattern(VerticalPattern);
	boxLayer->setAColor(StyleHelper::GreenLighterColor);
	boxLayer->setBColor(StyleHelper::GreenDarkerColor);

	// "Red box" layer
	boxLayer = static_cast<ColorLayer *>(
		group->createLayer(LyrColorLayerTypeId, tr("Red box")));
	boxLayer->setVisible(true);
	boxLayer->setRect(
		rect.translated(-rect.width() * 2 / 3, -rect.height() / 6));
	boxLayer->setPattern(VerticalPattern);
	boxLayer->setAColor(StyleHelper::RedLighterColor);
	boxLayer->setBColor(StyleHelper::RedDarkerColor);

	// "Blue box" layer
	boxLayer = static_cast<ColorLayer *>(
		group->createLayer(LyrColorLayerTypeId, tr("Blue box")));
	boxLayer->setVisible(true);
	boxLayer->setRect(rect);
	boxLayer->setPattern(VerticalPattern);
	boxLayer->setAColor(StyleHelper::BlueLighterColor);
	boxLayer->setBColor(StyleHelper::BlueDarkerColor);

	// "Background" group
	scene->addGroup(bgGroup, true);

	//-------------------------------------------------------------------------
	// "Conclusion" scene

	// Create the new scene
	scene = profile->createScene(tr("Conclusion"));

	// "Tutorial" group
	group = profile->createLayerGroup(tr("Tutorial"));
	scene->addGroup(group, true);

	// "Top text" layer
	text = P_CENTER_TAG +
		tr("You should now know the basics about how Mishira works.") +
		QStringLiteral(" ") + SPAN_YELLOW_TAG +
		tr("It's up to you now!") +
		QStringLiteral("</span></p>") +
		EMPTY_LINE_TAGS +

		P_CENTER_TAG +
		tr("If you require any further assistance the online<br />"
		"manual can be found in the help menu.") +
		QStringLiteral("</p>");
	rect = FULL_WITH_MARGIN_RECT;
	rect.setBottom(canvasSize.height() / 3);
	createTextLayer(group, tr("Top text"), text,
		rect, LyrMiddleCenterAlign);

	// "Bottom text" layer
	text = P_CENTER_TAG +
		tr("If you no longer need the tutorial you can delete all the scenes "
		"in the bottom-left list and all the scene layers in the top-left "
		"list to create a blank canvas.") +
		QStringLiteral("</p>");
	rect = FULL_WITH_MARGIN_RECT;
	rect.setTop(canvasSize.height() * 2 / 3);
	createTextLayer(group, tr("Bottom text"), text,
		rect, LyrMiddleCenterAlign);

	// "Background" group
	scene->addGroup(bgGroup, true);
}

TextLayer *NewProfileController::createTextLayer(
	LayerGroup *group, const QString &name, const QString &text,
	const QRect &rect, LyrAlignment align)
{
	TextLayer *txtLayer = static_cast<TextLayer *>(
		group->createLayer(LyrTextLayerTypeId, name));
	txtLayer->setVisible(true);
	txtLayer->setRect(rect);
	txtLayer->setAlignment(align);
	txtLayer->setDocumentHtml(text);
	txtLayer->setStrokeColor(QColor(0, 0, 0));
	txtLayer->setDialogBgColor(QColor(0, 0, 0));

	// Ideally all the text will have strokes but as generating the stroke
	// takes quite a while we remove it in order to improve the user
	// experience. As the tutorial is for uses that have never used Mishira
	// before we want to give them the best impression possible.
	//txtLayer->setStrokeSize(3);

	return txtLayer;
}

void NewProfileController::startBenchmark()
{
	if(m_benchmarkActive)
		return; // Already active
	if(m_curPage != WizBenchmarkPage)
		return; // Cannot start if we're not on the benchmark page
	m_benchmarkActive = true;

	appLog() << LOG_SINGLE_LINE;
	appLog() << " System benchmark begin";
	appLog() << LOG_SINGLE_LINE;

	// Switch to 30fps if we haven't already (Possible if the user has already
	// done one benchmark)
	if(App->getProfile()->getVideoFramerate() != Fraction(30, 1)) {
		App->getProfile()->setVideoFramerate(Fraction(30, 1));
		App->changeActiveProfile(
			App->getProfile()->getName(), NULL, true, true);
	}

	// Reset state
	m_numTestsDone = 0;
	m_curFrameNum = 0;
	m_curVidSizeIndex = 0;
	m_curPresetIndex = -1;
	m_testResults.clear();

	// We want to process all frames in order to emulate broadcasting
	App->refProcessAllFrames();

	// Begin the first test
	doNextBenchmarkTest();
}

void NewProfileController::endBenchmark()
{
	if(!m_benchmarkActive)
		return; // Already inactive
	m_benchmarkActive = false;

	// Don't process all frames if it isn't required
	App->derefProcessAllFrames();

	// Clean up
	if(m_curEncoder != NULL) {
		if(m_curEncoder->isRunning())
			m_curEncoder->derefActivate();
		delete m_curEncoder;
		m_curEncoder = NULL;
	}
	if(m_curUsage != NULL) {
		delete m_curUsage;
		m_curUsage = NULL;
	}

	appLog() << LOG_SINGLE_LINE;
	appLog() << " System benchmark end";
	appLog() << LOG_SINGLE_LINE;

	// Go to the benchmark results page
	m_curPage = WizBenchmarkResultsPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewProfileController::doNextBenchmarkTest()
{
	// Populate preset vector in order of testing
	QVector<X264Preset> presets;
	presets.reserve(4);
	presets.append(X264FasterPreset);
	presets.append(X264VeryFastPreset);
	presets.append(X264SuperFastPreset);
	presets.append(X264UltraFastPreset);

	// Populate video size vector in order of testing
	QVector<QSize> vidSizes;
	vidSizes.reserve(6);
	vidSizes.append(QSize(426, 240));
	vidSizes.append(QSize(640, 360));
	vidSizes.append(QSize(854, 480));
	vidSizes.append(QSize(960, 540));
	vidSizes.append(QSize(1280, 720));
	vidSizes.append(QSize(1920, 1080));

	m_numTotalTests = presets.size() * vidSizes.size() * 2;

	//-------------------------------------------------------------------------
	// Finish previous test

	// Get CPU usage of previous test
	float totalAvgCpu = 0.0f;
	float ourAvgCpu = 0.0f;
	if(m_curUsage != NULL) {
		// End monitoring CPU usage
		ourAvgCpu = m_curUsage->getAvgUsage(&totalAvgCpu);
		delete m_curUsage;
		m_curUsage = NULL;
	}

	// Did the CPU usage exceed our limit? If the system's CPU usage is above
	// 90% then it's dangerous for us to use the result even if we were below
	// our own limit so we discard the test result.
	bool exceededCpuLimit = false;
	if(ourAvgCpu > m_maxCpuUsage || totalAvgCpu > 0.9f)
		exceededCpuLimit = true;

	if(m_curEncoder != NULL) {
		// Destroy encoder object
		X264Encoder *x264Encoder = static_cast<X264Encoder *>(m_curEncoder);
		int curBitrate = x264Encoder->getBitrate();
		if(m_curEncoder->isRunning())
			m_curEncoder->derefActivate();
		delete m_curEncoder;
		m_curEncoder = NULL;

		X264Preset curPreset = presets[m_curPresetIndex];
		QSize curSize = vidSizes[m_curVidSizeIndex];

		// Remember results if it didn't exceed our limits
		if(!exceededCpuLimit) {
			TestResults results;
			results.vidPreset = curPreset;
			results.vidSize = curSize;
			results.framerate = App->getProfile()->getVideoFramerate();
			results.bitrate = curBitrate;
			results.totalAvgCpu = totalAvgCpu;
			results.ourAvgCpu = ourAvgCpu;
			results.recommended = false;
			m_testResults.append(results);
		}

		// Log results as well
		QString result = QStringLiteral("CPU usage is within limits");
		if(exceededCpuLimit)
			result = QStringLiteral("CPU usage exceeded limits");
		appLog() << QStringLiteral(
			"Our CPU usage = %1%; System CPU usage = %2%; %3")
			.arg(qRound(ourAvgCpu * 100.0f))
			.arg(qRound(totalAvgCpu * 100.0f))
			.arg(result);
	}

	//-------------------------------------------------------------------------

#define EXIT_AFTER_FIRST_BENCHMARK_TEST 0
#if EXIT_AFTER_FIRST_BENCHMARK_TEST
	// Exit immediately after the first test completes
	if(m_curPresetIndex != -1) {
		endBenchmark();
		return; // Not safe to continue
	}
#endif // EXIT_AFTER_FIRST_BENCHMARK_TEST

#define EXIT_AFTER_FIRST_PRESET_TESTS 0
#if EXIT_AFTER_FIRST_PRESET_TESTS
	// Exit immediately after we move on to the second preset
	if(m_curPresetIndex >= 1) {
		endBenchmark();
		return; // Not safe to continue
	}
#endif // EXIT_AFTER_FIRST_PRESET_TESTS

	//-------------------------------------------------------------------------
	// Begin next test

	for(;;) {
		// Increment to the next test. We assume that higher resolutions will
		// always use more CPU and faster presets will always use less CPU. We
		// try to find the highest preset that can be used for any specific
		// resolution.
		//
		// For a given preset we try and find as many resolutions that fit
		// within the CPU requirements. The moment we exceeded our CPU we lower
		// our preset and try again.
		if(m_curPresetIndex == -1) {
			// This is the first test
			m_curPresetIndex = 0;
			m_curVidSizeIndex = 0;
		} else if(exceededCpuLimit) {
			// Try a lower preset on the same video size
			m_curPresetIndex++;
			m_numTestsDone += 1;
		} else {
			// Try a higher video size on the same preset
			m_curVidSizeIndex++;
			m_numTestsDone += presets.size();
		}
		if(m_curPresetIndex >= presets.size() ||
			m_curVidSizeIndex >= vidSizes.size())
		{
			// No more presets or video sizes to test, we are done testing this
			// framerate. Switch to 60fps if we haven't tested it already.
			if(App->getProfile()->getVideoFramerate() != Fraction(60, 1)) {
				App->getProfile()->setVideoFramerate(Fraction(60, 1));
				App->changeActiveProfile(
					App->getProfile()->getName(), NULL, true, true);
				m_curPresetIndex = 0;
				m_curVidSizeIndex = 0;
			} else {
				// Already tested 60fps, nothing else to do
				endBenchmark();
				return; // Not safe to continue
			}
		}
		X264Preset curPreset = presets[m_curPresetIndex];
		QSize curSize = vidSizes[m_curVidSizeIndex];

		// Calculate what bitrate to use. We use the highest possible bitrate
		// that is between 75% and 125% of the recommended bitrate for that
		// video size. If the bitrate is above our maximum limit then we skip
		// this test if it's not the smallest size (We always want at least one
		// result). We are SLIGHTLY more lenient here when it comes to bitrate
		// vs video size than usual as some users may want to use a higher
		// resolution anyway because they think it's better.
		//
		// If the primary target is a file we triple the bitrate as the user
		// will most likely want to edit the file later. If the primary target
		// isn't a file we bump up the maximum bitrate of the lowest resolution
		// as it's a last resort.
		Fraction framerate = App->getProfile()->getVideoFramerate();
		int curBitrate = X264Encoder::determineBestBitrate(
			curSize, framerate, m_vidHasHighAction);
		float curBitrateF = (float)curBitrate * 0.75f;
		if(m_targetSettings.type == TrgtFileType) {
			// If recording to a local file increase the quality a lot
			curBitrateF = (float)curBitrate * 3.0f;
		} else {
			if(m_curVidSizeIndex == 0) {
				// We treat the smallest size differently as it's a last resort
				curBitrateF = (float)curBitrate * 1.45f;
			} else {
				if(curBitrateF > (float)m_recMaxVideoBitrate) {
					appLog() << QStringLiteral(
						"Upload speed is too low for %1p video, skipping")
						.arg(curSize.height());
					continue; // Above our limit even if bitrate is reduced to 75%
				}
				curBitrateF = (float)curBitrate * 1.25f;
			}
		}
		if(curBitrateF <= 1000.0f) // Round to a nice, human-readable number
			curBitrate = qRound(curBitrateF / 50.0f) * 50;
		else
			curBitrate = qRound(curBitrateF / 100.0f) * 100;
		if(curBitrate > m_recMaxVideoBitrate)
			curBitrate = m_recMaxVideoBitrate;

		// Create and initialize our encoder
		X264Options opt;
		opt.bitrate = curBitrate;
		opt.preset = curPreset;
		opt.keyInterval = 2; // We want to encoder to encode some keyframes
		m_curEncoder = new X264Encoder(
			NULL, curSize, SclrSnapToInnerScale, GfxBilinearFilter,
			framerate, opt);
		if(!m_curEncoder->refActivate()) {
			// Failed to initialize, skip this test
			delete m_curEncoder;
			m_curEncoder = NULL;
			continue;
		}

		// We start CPU monitoring a little bit into the test in order to get a
		// more accurate result
		m_curUsage = NULL; // Should already be NULL

		// Successfully began another test
		break;
	}

	// Reset state
	m_curFrameNum = 0;
}

void NewProfileController::queuedFrameEvent(uint frameNum, int numDropped)
{
	if(!m_benchmarkActive || App->getProfile() == NULL)
		return;

	// Request that the video encoder encode a test frame
	if(m_curEncoder != NULL) {
		X264Encoder *x264Encoder = static_cast<X264Encoder *>(m_curEncoder);
		TestScaler *scaler = x264Encoder->getTestScaler();
		if(scaler != NULL)
			scaler->emitFrameRendered(m_curFrameNum, 0);
	}

	// Has the current benchmark completed?
	m_curFrameNum++;
	float curDuration = (float)m_curFrameNum /
		App->getProfile()->getVideoFramerate().asFloat();
	if(curDuration >= BENCHMARK_TEST_DURATION_SECS) {
		// Current benchmark has completed, move on to the next
		doNextBenchmarkTest();
		curDuration = 0.0f;
	}

	// When should we start monitoring CPU usage?
	if(m_curUsage == NULL && curDuration >= BENCHMARK_TEST_START_SECS) {
		// Start monitoring CPU usage
		m_curUsage = App->createCPUUsage();
	}
}

void NewProfileController::realTimeTickEvent(int numDropped, int lateByUsec)
{
	if(!m_benchmarkActive || App->getProfile() == NULL)
		return;

	//-------------------------------------------------------------------------
	// Update the benchmark UI

	// Make sure we haven't moved on to the next page yet
	if(m_curPage != WizBenchmarkPage)
		return;

	// Only update the UI at ~15Hz just in case it results in higher CPU usage
	if(m_curFrameNum % 4 != 0)
		return;

	// Get UI object
	BenchmarkPage *page =
		static_cast<BenchmarkPage *>(m_wizWin->getActivePage());
	Ui_BenchmarkPage *ui = page->getUi();

	// Calculate the percentage of how much the current test is complete
	float curDuration = (float)m_curFrameNum /
		App->getProfile()->getVideoFramerate().asFloat();
	int partialComplete =
		qRound(curDuration / BENCHMARK_TEST_DURATION_SECS * 100.0f);
	partialComplete = qBound(0, partialComplete, 100);

	// Display in progress bar
	ui->progressBar->setRange(0, m_numTotalTests * 100);
	ui->progressBar->setValue(m_numTestsDone * 100 + partialComplete);
}
