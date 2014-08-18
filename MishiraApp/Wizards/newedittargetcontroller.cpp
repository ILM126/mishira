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

#include "newedittargetcontroller.h"
#include "application.h"
#include "common.h"
#include "fdkaacencoder.h"
#include "filetarget.h"
#include "hitboxtarget.h"
#include "profile.h"
#include "rtmptarget.h"
#include "target.h"
#include "twitchtarget.h"
#include "ustreamtarget.h"
#include "videoencoder.h"
#include "wizardwindow.h"
#include "x264encoder.h"
#include "Widgets/targettypewidget.h"
#include "Wizards/audiosettingspage.h"
#include "Wizards/targettypepage.h"
#include "Wizards/videosettingspage.h"
#include "Wizards/filetargetsettingspage.h"
#include "Wizards/rtmptargetsettingspage.h"
#include "Wizards/twitchtargetsettingspage.h"
#include "Wizards/ustreamtargetsettingspage.h"
#include "Wizards/hitboxtargetsettingspage.h"

/// <summary>
/// Called just before switching to this controller so that all the settings
/// are properly initialized.
/// </summary>
void NewEditTargetController::prepareCreateTarget(
	WizShared *shared, bool returnToEditTargets)
{
	shared->netCurTarget = NULL;
	shared->netReturnToEditTargets = returnToEditTargets;

	shared->netDefaults = WizTargetSettings(); // Easy zeroing
	WizTargetSettings *settings = &shared->netDefaults;
	settings->name = tr("Unnamed");

	// By default share encoders with another target if possible
	settings->vidShareWith = NULL;
	settings->audShareWith = NULL;
	TargetList targets = App->getProfile()->getTargets();
	for(int i = 0; i < targets.count(); i++) {
		Target *target = targets.at(i);
		if(settings->vidShareWith == NULL && target->getVideoEncoder() != NULL)
			settings->vidShareWith = target;
		if(settings->audShareWith == NULL && target->getAudioEncoder() != NULL)
			settings->audShareWith = target;
	}

	// Default video settings
	settings->vidWidth = App->getCanvasSize().width();
	settings->vidHeight = App->getCanvasSize().height();
	settings->vidScaling = SclrSnapToInnerScale;
	settings->vidScaleQuality = GfxBilinearFilter;
	settings->vidEncoder = VencX264Type;
	settings->vidBitrate = X264Encoder::determineBestBitrate(
		App->getCanvasSize(), App->getProfile()->getVideoFramerate(), true);
	settings->vidPreset = X264SuperFastPreset;

	// Default audio settings
	settings->audHasAudio = true;
	settings->audEncoder = AencFdkAacType;
	settings->audBitrate =
		FdkAacEncoder::determineBestBitrate(settings->vidBitrate);

	// Default target settings
	settings->setTargetSettingsToDefault();
}

/// <summary>
/// Called just before switching to this controller so that all the settings
/// are properly initialized.
/// </summary>
void NewEditTargetController::prepareModifyTarget(
	WizShared *shared, Target *target, bool returnToEditTargets)
{
	if(target == NULL)
		return;
	VideoEncoder *vidEnc = target->getVideoEncoder();
	AudioEncoder *audEnc = target->getAudioEncoder();

	// There is currently only one type of video and audio encoder
	X264Encoder *x264Enc = static_cast<X264Encoder *>(vidEnc);
	FdkAacEncoder *aacEnc = static_cast<FdkAacEncoder *>(audEnc);

	shared->netCurTarget = target;
	shared->netReturnToEditTargets = returnToEditTargets;
	WizTargetSettings *settings = &shared->netDefaults;
	settings->type = target->getType();
	settings->name = target->getName();

	// By default share encoders with another target if possible
	settings->vidShareWith = NULL;
	settings->audShareWith = NULL;
	TargetList targets = App->getProfile()->getTargets();
	for(int i = 0; i < targets.count(); i++) {
		Target *t = targets.at(i);
		if(target != t && settings->vidShareWith == NULL &&
			t->getVideoEncoder() == vidEnc)
		{
			settings->vidShareWith = t;
		}
		if(target != t && settings->audShareWith == NULL &&
			t->getAudioEncoder() == audEnc)
		{
			settings->audShareWith = t;
		}
	}

	// Video settings
	settings->vidWidth = vidEnc->getSize().width();
	settings->vidHeight = vidEnc->getSize().height();
	settings->vidScaling = vidEnc->getScaling();
	settings->vidScaleQuality = vidEnc->getScaleFilter();
	settings->vidEncoder = vidEnc->getType();
	settings->vidBitrate = x264Enc->getBitrate();
	settings->vidPreset = x264Enc->getPreset();
	settings->vidKeyInterval = x264Enc->getKeyInterval();

	// Default audio settings
	if(aacEnc != NULL) {
		settings->audHasAudio = true;
		settings->audEncoder = aacEnc->getType();
		settings->audBitrate = aacEnc->getBitrate();
	} else {
		settings->audHasAudio = false;
		settings->audEncoder = AencFdkAacType;
		settings->audBitrate =
			FdkAacEncoder::determineBestBitrate(settings->vidBitrate);
	}

	// Type-specific settings
	switch(target->getType()) {
	default:
		break;
	case TrgtFileType: {
		FileTarget *fileTarget = static_cast<FileTarget *>(target);
		settings->fileFilename = fileTarget->getFilename();
		settings->fileType = fileTarget->getFileType();
		break; }
	case TrgtRtmpType: {
		RTMPTarget *rtmpTarget = static_cast<RTMPTarget *>(target);
		settings->rtmpUrl = rtmpTarget->getURL();
		settings->rtmpStreamName = rtmpTarget->getStreamName();
		settings->rtmpHideStreamName = rtmpTarget->getHideStreamName();
		settings->rtmpPadVideo = rtmpTarget->getPadVideo();
		break; }
	case TrgtTwitchType: {
		TwitchTarget *twitchTarget = static_cast<TwitchTarget *>(target);
		settings->rtmpUrl = twitchTarget->getURL();
		settings->rtmpStreamName = twitchTarget->getStreamKey();
		settings->username = twitchTarget->getUsername();
		settings->rtmpHideStreamName = true;
		settings->rtmpPadVideo = true;
		break; }
	case TrgtUstreamType: {
		UstreamTarget *ustreamTarget = static_cast<UstreamTarget *>(target);
		settings->rtmpUrl = ustreamTarget->getURL();
		settings->rtmpStreamName = ustreamTarget->getStreamName();
		settings->rtmpPadVideo = ustreamTarget->getPadVideo();
		break; }
	case TrgtHitboxType: {
		HitboxTarget *hitboxTarget = static_cast<HitboxTarget *>(target);
		settings->rtmpUrl = hitboxTarget->getURL();
		settings->rtmpStreamName = hitboxTarget->getStreamKey();
		settings->username = hitboxTarget->getUsername();
		settings->rtmpHideStreamName = true;
		settings->rtmpPadVideo = true;
		break; }
	}
}

NewEditTargetController::NewEditTargetController(
	WizController type, WizardWindow *wizWin, WizShared *shared)
	: WizardController(type, wizWin, shared)
	, m_curPage(WizNoPage)
	, m_defaults(&shared->netDefaults)
	, m_settings()
{
}

NewEditTargetController::~NewEditTargetController()
{
}

/// <summary>
/// Returns a list of targets to share video encoder settings with from the
/// perspective of the current target if there is one.
/// </summary>
ShareMap NewEditTargetController::generateShareMap(bool forVideo) const
{
	ShareMap map;

	TargetList targets = App->getProfile()->getTargets();
	for(int i = 0; i < targets.count(); i++) {
		Target *target = targets.at(i);
		VideoEncoder *vEnc = target->getVideoEncoder();
		AudioEncoder *aEnc = target->getAudioEncoder();
		if((forVideo && vEnc == NULL) || (!forVideo && aEnc == NULL))
			continue; // Skip blank encoders
		if(target == m_shared->netCurTarget)
			continue; // Skip current target

		// Test to see if we already have this encoder in the list and if so
		// skip it
		bool skip = false;
		ShareMapIterator it(map);
		while(it.hasNext()) {
			it.next();
			if(forVideo && vEnc == it.key()->getVideoEncoder()) {
				skip = true;
				break;
			} else if(!forVideo && aEnc == it.key()->getAudioEncoder()) {
				skip = true;
				break;
			}
		}
		if(skip)
			continue;

		// Add the encoder to the list
		map[target] = tr("%1 (%2)")
			.arg(target->getName())
			.arg(forVideo ? vEnc->getInfoString() : aEnc->getInfoString());
	}

	// Make sure there is always at least one item in the returned list
	if(map.count() == 0)
		map[NULL] = tr("** No available targets **");

	// TODO: The result should be shorted alphabetially, right now it's random
	// as we're using a map with pointers as keys
	return map;
}

void NewEditTargetController::setPageBasedOnTargetType()
{
	switch(m_settings.type) {
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

void NewEditTargetController::resetWizard()
{
	if(m_shared->netCurTarget == NULL) {
		m_curPage = WizTargetTypePage;
		m_wizWin->setPage(m_curPage);
		m_wizWin->setTitle(tr("Create new target"));
	} else {
		// Skip target type selection if we are modifying an existing target
		m_curPage = WizVideoSettingsPage;
		m_wizWin->setPage(m_curPage);
		m_wizWin->setTitle(tr("Modify existing target"));

		// We must set the type as the type page isn't being displayed
		m_settings.type = m_defaults->type;
	}
	m_wizWin->setVisibleButtons(WizStandardButtons);

	// Initialize values for the starting page
	resetPage();
}

bool NewEditTargetController::cancelWizard()
{
	// Close the wizard window or return to previous controller depending on
	// the shared settings
	if(m_shared->netReturnToEditTargets)
		m_wizWin->setController(WizEditTargetsController);
	else
		m_wizWin->controllerFinished();

	return true; // We have fully processed the cancel
}

void NewEditTargetController::resetTargetTypePage()
{
	// Use common "reset" button code for this page
	TargetTypePage *page =
		static_cast<TargetTypePage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_defaults, false);
}

void NewEditTargetController::resetVideoPage()
{
	// Get UI object
	VideoSettingsPage *page =
		static_cast<VideoSettingsPage *>(m_wizWin->getActivePage());
	Ui_VideoSettingsPage *ui = page->getUi();

	page->setUpdatesEnabled(false);

	// Populate "share with" combobox
	ShareMap map = generateShareMap(true);
	ShareMapIterator it(map);
	ui->shareCombo->clear();
	while(it.hasNext()) {
		it.next();
		ui->shareCombo->addItem(
			it.value(), qVariantFromValue(reinterpret_cast<void *>(it.key())));
	}

	// If the share list is empty then prevent the user from being able to
	// select it
	if(map.contains(NULL)) {
		ui->shareCombo->setEnabled(false);
		ui->shareRadio->setEnabled(false);
		ui->newRadio->setEnabled(false);
	} else {
		ui->shareCombo->setEnabled(true);
		ui->shareRadio->setEnabled(true);
		ui->newRadio->setEnabled(true);
	}

	// Update fields
	if(m_defaults->vidShareWith != NULL && !map.contains(NULL)) {
		ui->shareRadio->setChecked(true);

		// WARNING/TODO: Because the target-encoder mapping is many-to-one it
		// is possible that "vidShareWith" can refer to a target that isn't in
		// our list. We should probably do a more advanced search here.
		int i = 0;
		ShareMapIterator it(map);
		while(it.hasNext()) {
			it.next();
			if(it.key() == m_defaults->vidShareWith)
				break;
			i++;
		}
		ui->shareCombo->setCurrentIndex(i);
	} else {
		ui->newRadio->setChecked(true);
		ui->shareCombo->setCurrentIndex(0);
	}
	ui->widthEdit->setText(QString::number(m_defaults->vidWidth));
	ui->heightEdit->setText(QString::number(m_defaults->vidHeight));
	ui->scalingCombo->setCurrentIndex(m_defaults->vidScaling);
	ui->scalingQualityCombo->setCurrentIndex(m_defaults->vidScaleQuality);
	ui->encoderCombo->setCurrentIndex(m_defaults->vidEncoder);
	ui->bitrateEdit->setText(QString::number(m_defaults->vidBitrate));
	ui->cpuUsageCombo->setCurrentIndex(m_defaults->vidPreset);
	if(m_defaults->vidKeyInterval > 0) {
		ui->keyIntervalEdit->setText(
			QString::number(m_defaults->vidKeyInterval));
	} else
		ui->keyIntervalEdit->setText(QString()); // Display placeholder

	// Update which widgets are enabled and disabled
	page->updateEnabled();

	// Reset validity and connect signal
	doQLineEditValidate(ui->widthEdit);
	doQLineEditValidate(ui->heightEdit);
	doQLineEditValidate(ui->bitrateEdit);
	doQLineEditValidate(ui->keyIntervalEdit);
	m_wizWin->setCanContinue(page->isValid());
	connect(page, &VideoSettingsPage::validityMaybeChanged,
		m_wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	page->setUpdatesEnabled(true);
}

void NewEditTargetController::resetAudioPage()
{
	// Get UI object
	AudioSettingsPage *page =
		static_cast<AudioSettingsPage *>(m_wizWin->getActivePage());
	Ui_AudioSettingsPage *ui = page->getUi();

	page->setUpdatesEnabled(false);

	// Populate "share with" combobox
	ShareMap map = generateShareMap(false);
	ShareMapIterator it(map);
	ui->shareCombo->clear();
	while(it.hasNext()) {
		it.next();
		ui->shareCombo->addItem(
			it.value(), qVariantFromValue(reinterpret_cast<void *>(it.key())));
	}

	// If the share list is empty then prevent the user from being able to
	// select it
	if(map.contains(NULL)) {
		ui->shareCombo->setEnabled(false);
		ui->shareRadio->setEnabled(false);
	} else {
		ui->shareCombo->setEnabled(true);
		ui->shareRadio->setEnabled(true);
	}

	//-------------------------------------------------------------------------
	// Update fields

	// Radio buttons and share combo selection
	if(!m_defaults->audHasAudio) {
		ui->noAudioRadio->setChecked(true);
		ui->shareCombo->setCurrentIndex(0);
	} else if(m_defaults->audShareWith != NULL && !map.contains(NULL)) {
		ui->shareRadio->setChecked(true);

		// WARNING/TODO: Because the target-encoder mapping is many-to-one it
		// is possible that "audShareWith" can refer to a target that isn't in
		// our list. We should probably do a more advanced search here.
		int i = 0;
		ShareMapIterator it(map);
		while(it.hasNext()) {
			it.next();
			if(it.key() == m_defaults->audShareWith)
				break;
			i++;
		}
		ui->shareCombo->setCurrentIndex(i);
	} else {
		ui->newRadio->setChecked(true);
		ui->shareCombo->setCurrentIndex(0);
	}

	// Bitrate combo
	int bitrate = FdkAacEncoder::validateBitrate(m_defaults->audBitrate);
	for(int i = 0; FdkAacBitrates[i].bitrate != 0; i++) {
		if(bitrate != FdkAacBitrates[i].bitrate)
			continue;
		ui->bitrateCombo->setCurrentIndex(i);
		break;
	}

	//-------------------------------------------------------------------------

	// Update which widgets are enabled and disabled
	page->updateEnabled();

	// Reset validity and connect signal
	m_wizWin->setCanContinue(true); // Page is always valid

	page->setUpdatesEnabled(true);
}

void NewEditTargetController::resetFileTargetPage()
{
	// Use common "reset" button code for this page
	FileTargetSettingsPage *page =
		static_cast<FileTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_defaults);
}

void NewEditTargetController::resetRTMPTargetPage()
{
	// Use common "reset" button code for this page
	RTMPTargetSettingsPage *page =
		static_cast<RTMPTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_defaults);
}

void NewEditTargetController::resetTwitchTargetPage()
{
	// Use common "reset" button code for this page
	TwitchTargetSettingsPage *page =
		static_cast<TwitchTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_defaults);
}

void NewEditTargetController::resetUstreamTargetPage()
{
	// Use common "reset" button code for this page
	UstreamTargetSettingsPage *page =
		static_cast<UstreamTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_defaults);
}

void NewEditTargetController::resetHitboxTargetPage()
{
	// Use common "reset" button code for this page
	HitboxTargetSettingsPage *page =
		static_cast<HitboxTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedReset(m_wizWin, m_defaults);
}

void NewEditTargetController::resetPage()
{
	// Reset whatever page we are currently on
	switch(m_curPage) {
	default:
		break;
	case WizTargetTypePage:
		resetTargetTypePage();
		break;
	case WizVideoSettingsPage:
		resetVideoPage();
		break;
	case WizAudioSettingsPage:
		resetAudioPage();
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
	}
}

void NewEditTargetController::backPage()
{
	// Return to the previous page in all cases
	switch(m_curPage) {
	default:
		break;
	case WizTargetTypePage:
		if(m_shared->netReturnToEditTargets)
			m_wizWin->setController(WizEditTargetsController);
		else {
			// We don't know where to go!
		}
		break;
	case WizVideoSettingsPage:
		if(m_shared->netCurTarget == NULL) {
			m_curPage = WizTargetTypePage;
			m_wizWin->setPage(m_curPage);
			resetPage();
		} else if(m_shared->netReturnToEditTargets)
			m_wizWin->setController(WizEditTargetsController);
		else {
			// We don't know where to go!
		}
		break;
	case WizAudioSettingsPage:
		m_curPage = WizVideoSettingsPage;
		m_wizWin->setPage(m_curPage);
		resetPage();
		break;

	case WizFileTargetSettingsPage:
	case WizRTMPTargetSettingsPage:
	case WizTwitchTargetSettingsPage:
	case WizUstreamTargetSettingsPage:
	case WizHitboxTargetSettingsPage:
		m_curPage = WizAudioSettingsPage;
		m_wizWin->setPage(m_curPage);
		resetPage();
		m_wizWin->setVisibleButtons(WizStandardButtons);
		break;
	}
}

void NewEditTargetController::nextTargetTypePage()
{
	// Use common "next" button code for this page
	TargetTypePage *page =
		static_cast<TargetTypePage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_settings);

	// Go to the video settings page
	m_curPage = WizVideoSettingsPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewEditTargetController::nextVideoPage()
{
	// Get UI object
	VideoSettingsPage *page =
		static_cast<VideoSettingsPage *>(m_wizWin->getActivePage());
	Ui_VideoSettingsPage *ui = page->getUi();

	// Save fields
	m_settings.vidShareWith = NULL;
	if(ui->shareRadio->isChecked()) {
		const QVariant &data =
			ui->shareCombo->itemData(ui->shareCombo->currentIndex());
		Target *target = reinterpret_cast<Target *>(data.value<void *>());
		m_settings.vidShareWith = target;
	}
	m_settings.vidWidth = ui->widthEdit->text().toInt();
	m_settings.vidHeight = ui->heightEdit->text().toInt();
	m_settings.vidScaling =
		(SclrScalingMode)(ui->scalingCombo->currentIndex());
	m_settings.vidScaleQuality =
		(GfxFilter)(ui->scalingQualityCombo->currentIndex());
	m_settings.vidEncoder = (VencType)(ui->encoderCombo->currentIndex());
	m_settings.vidBitrate = ui->bitrateEdit->text().toInt();
	m_settings.vidPreset = (X264Preset)(ui->cpuUsageCombo->currentIndex());
	m_settings.vidKeyInterval = ui->keyIntervalEdit->text().toInt();

	// Go to the audio settings page
	m_curPage = WizAudioSettingsPage;
	m_wizWin->setPage(m_curPage);
	resetPage();
}

void NewEditTargetController::nextAudioPage()
{
	// Get UI object
	AudioSettingsPage *page =
		static_cast<AudioSettingsPage *>(m_wizWin->getActivePage());
	Ui_AudioSettingsPage *ui = page->getUi();

	// Save fields
	m_settings.audHasAudio = !ui->noAudioRadio->isChecked();
	m_settings.audShareWith = NULL;
	if(ui->shareRadio->isChecked()) {
		const QVariant &data =
			ui->shareCombo->itemData(ui->shareCombo->currentIndex());
		Target *target = reinterpret_cast<Target *>(data.value<void *>());
		m_settings.audShareWith = target;
	}
	m_settings.audBitrate =
		FdkAacBitrates[ui->bitrateCombo->currentIndex()].bitrate;

	// Go to the target type-specific settings page
	setPageBasedOnTargetType();
	m_wizWin->setVisibleButtons(WizStandardLastButtons);
	resetPage();
}

void NewEditTargetController::nextFileTargetPage()
{
	// Use common "next" button code for this page
	FileTargetSettingsPage *page =
		static_cast<FileTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_settings);

	// Create/edit actual target
	recreateTarget();

	// Our wizard has completed, close the wizard window or return to previous
	// controller depending on the shared settings
	if(m_shared->netReturnToEditTargets)
		m_wizWin->setController(WizEditTargetsController);
	else
		m_wizWin->controllerFinished();
}

void NewEditTargetController::nextRTMPTargetPage()
{
	// Use common "next" button code for this page
	RTMPTargetSettingsPage *page =
		static_cast<RTMPTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_settings);

	// Create/edit actual target
	recreateTarget();

	// Our wizard has completed, close the wizard window or return to previous
	// controller depending on the shared settings
	if(m_shared->netReturnToEditTargets)
		m_wizWin->setController(WizEditTargetsController);
	else
		m_wizWin->controllerFinished();
}

void NewEditTargetController::nextTwitchTargetPage()
{
	// Use common "next" button code for this page
	TwitchTargetSettingsPage *page =
		static_cast<TwitchTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_settings);

	// Create/edit actual target
	recreateTarget();

	// Our wizard has completed, close the wizard window or return to previous
	// controller depending on the shared settings
	if(m_shared->netReturnToEditTargets)
		m_wizWin->setController(WizEditTargetsController);
	else
		m_wizWin->controllerFinished();
}

void NewEditTargetController::nextUstreamTargetPage()
{
	// Use common "next" button code for this page
	UstreamTargetSettingsPage *page =
		static_cast<UstreamTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_settings);

	// Create/edit actual target
	recreateTarget();

	// Our wizard has completed, close the wizard window or return to previous
	// controller depending on the shared settings
	if(m_shared->netReturnToEditTargets)
		m_wizWin->setController(WizEditTargetsController);
	else
		m_wizWin->controllerFinished();
}

void NewEditTargetController::nextHitboxTargetPage()
{
	// Use common "next" button code for this page
	HitboxTargetSettingsPage *page =
		static_cast<HitboxTargetSettingsPage *>(m_wizWin->getActivePage());
	page->sharedNext(&m_settings);

	// Create/edit actual target
	recreateTarget();

	// Our wizard has completed, close the wizard window or return to previous
	// controller depending on the shared settings
	if(m_shared->netReturnToEditTargets)
		m_wizWin->setController(WizEditTargetsController);
	else
		m_wizWin->controllerFinished();
}

void NewEditTargetController::nextPage()
{
	// Save and continue from whatever page we are currently on
	switch(m_curPage) {
	default:
		break;
	case WizTargetTypePage:
		nextTargetTypePage();
		break;
	case WizVideoSettingsPage:
		nextVideoPage();
		break;
	case WizAudioSettingsPage:
		nextAudioPage();
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
	}
}

void NewEditTargetController::recreateTarget()
{
	// Delete the old target if one exists
	Profile *profile = App->getProfile();
	bool wasEnabled = true;
	int before = -1;
	if(m_shared->netCurTarget != NULL) {
		wasEnabled = m_shared->netCurTarget->isEnabled();
		before = profile->indexOfTarget(m_shared->netCurTarget);
		profile->destroyTarget(m_shared->netCurTarget);
	}

	VideoEncoder *vidEnc = getOrCreateVideoEncoder();
	AudioEncoder *audEnc = getOrCreateAudioEncoder();
	Target *target = NULL;
	switch(m_settings.type) {
	default:
	case TrgtFileType: {
		FileTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.filename = m_settings.fileFilename;
		opt.fileType = m_settings.fileType;
		target = profile->createFileTarget(m_settings.name, opt, before);
		break; }
	case TrgtRtmpType: {
		RTMPTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.url = m_settings.rtmpUrl;
		opt.streamName = m_settings.rtmpStreamName;
		opt.hideStreamName = m_settings.rtmpHideStreamName;
		opt.padVideo = m_settings.rtmpPadVideo;
		target = profile->createRtmpTarget(m_settings.name, opt, before);
		break; }
	case TrgtTwitchType: {
		TwitchTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.url = m_settings.rtmpUrl;
		opt.streamKey = m_settings.rtmpStreamName;
		opt.username = m_settings.username;
		target = profile->createTwitchTarget(m_settings.name, opt, before);
		break; }
	case TrgtUstreamType: {
		UstreamTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.url = m_settings.rtmpUrl;
		opt.streamKey = m_settings.rtmpStreamName;
		opt.padVideo = m_settings.rtmpPadVideo;
		target = profile->createUstreamTarget(m_settings.name, opt, before);
		break; }
	case TrgtHitboxType: {
		HitboxTrgtOptions opt;
		opt.videoEncId = (vidEnc == NULL) ? 0 : vidEnc->getId();
		opt.audioEncId = (audEnc == NULL) ? 0 : audEnc->getId();
		opt.url = m_settings.rtmpUrl;
		opt.streamKey = m_settings.rtmpStreamName;
		opt.username = m_settings.username;
		target = profile->createHitboxTarget(m_settings.name, opt, before);
		break; }
	}
	if(target != NULL) {
		if(wasEnabled)
			target->setEnabled(true);
	}

	// Prune encoders last just in case nothing changed
	profile->pruneVideoEncoders();
	profile->pruneAudioEncoders();

	// Update the shared data with the new target reference
	m_shared->netCurTarget = target;
}

/// <summary>
/// Get or create the video encoder as specified in the current settings.
/// </summary>
VideoEncoder *NewEditTargetController::getOrCreateVideoEncoder()
{
	// Return shared video encoder object if required
	if(m_settings.vidShareWith != NULL)
		return m_settings.vidShareWith->getVideoEncoder();

	// Create a new video encoder
	Profile *profile = App->getProfile();
	switch(m_settings.vidEncoder) {
	default:
		return NULL;
	case VencX264Type: {
		X264Options opt;
		opt.bitrate = m_settings.vidBitrate;
		opt.preset = m_settings.vidPreset;
		opt.keyInterval = m_settings.vidKeyInterval;
		return profile->getOrCreateX264VideoEncoder(
			QSize(m_settings.vidWidth, m_settings.vidHeight),
			m_settings.vidScaling, m_settings.vidScaleQuality, opt); }
	}
	return NULL;
}

/// <summary>
/// Get or create the audio encoder as specified in the current settings.
/// </summary>
AudioEncoder *NewEditTargetController::getOrCreateAudioEncoder()
{
	if(!m_settings.audHasAudio)
		return NULL;

	// Return shared audio encoder object if required
	if(m_settings.audShareWith != NULL)
		return m_settings.audShareWith->getAudioEncoder();

	// Create a new audio encoder
	Profile *profile = App->getProfile();
	switch(m_settings.audEncoder) {
	default:
		return NULL;
	case AencFdkAacType: {
		FdkAacOptions opt;
		opt.bitrate = m_settings.audBitrate;
		return profile->getOrCreateFdkAacAudioEncoder(opt); }
	}
	return NULL;
}
