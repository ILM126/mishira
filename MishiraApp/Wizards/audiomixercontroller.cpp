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

#include "audiomixercontroller.h"
#include "application.h"
#include "audioinput.h"
#include "audiomixer.h"
#include "audiosource.h"
#include "audiosourcemanager.h"
#include "profile.h"
#include "wizardwindow.h"
#include "Widgets/infowidget.h"
#include "Wizards/audiomixerpage.h"
#include <QtWidgets/QInputDialog>

AudioMixerController::AudioMixerController(
	WizController type, WizardWindow *wizWin, WizShared *shared)
	: WizardController(type, wizWin, shared)
	, m_inputWidgets()
	, m_btnGroup(this)
	, m_doDelayedRepaint(false)
	, m_renameDialog(NULL)
	, m_renameInput(NULL)
	, m_renameFromCreate(false)
	, m_delayDialog(NULL)
	, m_delayInput(NULL)
{
}

AudioMixerController::~AudioMixerController()
{
}

/// <summary>
/// Called whenever we are leaving the current controller page.
/// </summary>
void AudioMixerController::leavingPage()
{
	// Remove all our input radio buttons from the group
	for(int i = 0; i < m_inputWidgets.count(); i++)
		m_btnGroup.removeButton(m_inputWidgets.at(i)->getButton());

	// Deleting our widgets removes them from the UI layout
	for(int i = 0; i < m_inputWidgets.count(); i++)
		delete m_inputWidgets.at(i);
	m_inputWidgets.clear();

	// Stop watching manager for source list changes
	AudioSourceManager *mgr = App->getAudioSourceManager();
	disconnect(mgr, &AudioSourceManager::sourceAdded,
		this, &AudioMixerController::sourceAdded);
	disconnect(mgr, &AudioSourceManager::removingSource,
		this, &AudioMixerController::sourceDeleted);

	// Stop watching mixer for input deletion
	disconnect(
		App->getProfile()->getAudioMixer(), &AudioMixer::destroyingInput,
		this, &AudioMixerController::inputDeleted);
}

void AudioMixerController::resetButtons()
{
	// Get UI object
	AudioMixerPage *page =
		static_cast<AudioMixerPage *>(m_wizWin->getActivePage());
	Ui_AudioMixerPage *ui = page->getUi();

	bool btnsEnabled = (m_inputWidgets.count() > 0);
	ui->renameBtn->setEnabled(btnsEnabled);
	ui->removeBtn->setEnabled(btnsEnabled);

	btnsEnabled = (m_inputWidgets.count() > 1);
	ui->moveUpBtn->setEnabled(btnsEnabled);
	ui->moveDownBtn->setEnabled(btnsEnabled);
}

AudioInput *AudioMixerController::getSelectedInput() const
{
	for(int i = 0; i < m_inputWidgets.count(); i++) {
		AudioInputInfoWidget *widget = m_inputWidgets.at(i);
		if(widget->isSelected())
			return widget->getInput();
	}
	return NULL;
}

void AudioMixerController::refreshSourceBox()
{
	// Get UI object
	AudioMixerPage *page =
		static_cast<AudioMixerPage *>(m_wizWin->getActivePage());
	Ui_AudioMixerPage *ui = page->getUi();

	// TODO: Select previously selected source? They might rearrange

	ui->sourceBox->clear();
	int itemIndex = 0;

	// Add any default source that isn't already in the mixer's list
	AudioInputList inputs = App->getProfile()->getAudioMixer()->getInputs();
	AudioSourceManager *mgr = App->getAudioSourceManager();
	for(int i = 0; i < NUM_AUDIO_SOURCE_TYPES; i++) {
		AsrcType type = (AsrcType)i;
		quint64 defaultId = mgr->getDefaultIdOfType(type);
		AudioSource *defaultSource = mgr->getSource(defaultId);

		// Is it already in the mixer?
		bool skip = false;
		for(int j = 0; j < inputs.size(); j++) {
			AudioInput *input = inputs.at(j);
			AudioSource *inputSource = input->getSource();
			if(defaultId == input->getSourceId() ||
				(inputSource != NULL && defaultId == inputSource->getId()))
			{
				skip = true;
				break;
			}
		}
		if(skip)
			continue;

		// Generate device name
		QString name = tr("** No default device **");
		if(defaultSource != NULL)
			name = defaultSource->getFriendlyName();
		switch(type) {
		default:
		case AsrcOutputType:
			name = tr("Default playback device (\"%1\")").arg(name);
			break;
		case AsrcInputType:
			name = tr("Default recording device (\"%1\")").arg(name);
			break;
		}

		// Add to combo box
		ui->sourceBox->addItem(name);
		ui->sourceBox->setItemData(
			itemIndex, qVariantFromValue(defaultId), Qt::UserRole);
		itemIndex++;
	}

	// Add any source that isn't already in the mixer's list
	AudioSourceList sources = mgr->getSourceList();
	for(int i = 0; i < sources.size(); i++) {
		AudioSource *source = sources.at(i);

		// Is it already in the mixer?
		bool skip = false;
		for(int j = 0; j < inputs.size(); j++) {
			AudioInput *input = inputs.at(j);
			AudioSource *inputSource = input->getSource();
			if((inputSource != NULL &&
				source->getRealId() == inputSource->getRealId()) ||
				(inputSource == NULL &&
				source->getRealId() == input->getSourceId()))
			{
				skip = true;
				break;
			}
		}
		if(skip)
			continue;

		// Generate device name
		QString name;
		switch(source->getType()) {
		default:
		case AsrcOutputType:
			name = tr("Playback device: ");
			break;
		case AsrcInputType:
			name = tr("Recording device: ");
			break;
		}
		name += source->getFriendlyName();
		if(!source->isConnected())
			name += tr(" (Unplugged)");

		// Add to combo box
		ui->sourceBox->addItem(name);
		ui->sourceBox->setItemData(
			itemIndex, qVariantFromValue(source->getRealId()), Qt::UserRole);
		itemIndex++;
	}
}

void AudioMixerController::refreshInputList()
{
	// Remove all our input radio buttons from the group
	for(int i = 0; i < m_inputWidgets.count(); i++)
		m_btnGroup.removeButton(m_inputWidgets.at(i)->getButton());

	// Deleting our widgets removes them from the UI layout
	for(int i = 0; i < m_inputWidgets.count(); i++)
		delete m_inputWidgets.at(i);
	m_inputWidgets.clear();

	// Get UI object
	AudioMixerPage *page =
		static_cast<AudioMixerPage *>(m_wizWin->getActivePage());
	Ui_AudioMixerPage *ui = page->getUi();

	// Prevent ugly repaints when moving inputs. We cannot do this always as
	// it'll cause a black box on first display
	if(m_doDelayedRepaint)
		ui->inputList->setUpdatesEnabled(false);

	// Populate input list
	QVBoxLayout *layout = ui->inputListLayout;
	AudioInputList inputs = App->getProfile()->getAudioMixer()->getInputs();
	for(int i = 0; i < inputs.size(); i++) {
		AudioInput *input = inputs.at(i);

		AudioInputInfoWidget *widget = new AudioInputInfoWidget(input);
		input->setupInputInfo(widget);
		m_inputWidgets.append(widget);
		layout->insertWidget(layout->count() - 1, widget);
		m_btnGroup.addButton(widget->getButton());
	}

	// Select the first input if one exists and setup the buttons
	if(m_inputWidgets.count())
		m_inputWidgets.at(0)->select();
	resetButtons();

	// HACK: This must be delayed in order to actually do anything
	if(m_doDelayedRepaint)
		QTimer::singleShot(10, this, SLOT(enableUpdatesTimeout()));
}

void AudioMixerController::resetWizard()
{
	m_wizWin->setPage(WizAudioMixerPage);
	m_wizWin->setTitle(tr("Modify audio mixer"));
	m_wizWin->setVisibleButtons(WizOKButton);

	// Get UI object
	AudioMixerPage *page =
		static_cast<AudioMixerPage *>(m_wizWin->getActivePage());
	Ui_AudioMixerPage *ui = page->getUi();

	// Make sure that our source list is always up-to-date
	AudioSourceManager *mgr = App->getAudioSourceManager();
	connect(mgr, &AudioSourceManager::sourceAdded,
		this, &AudioMixerController::sourceAdded,
		Qt::UniqueConnection);
	connect(mgr, &AudioSourceManager::removingSource,
		this, &AudioMixerController::sourceDeleted,
		Qt::UniqueConnection);

	// Watch mixer for input deletion
	connect(App->getProfile()->getAudioMixer(), &AudioMixer::destroyingInput,
		this, &AudioMixerController::inputDeleted,
		Qt::UniqueConnection);

	// Connect buttons
	connect(ui->addBtn, &QAbstractButton::clicked,
		this, &AudioMixerController::addClicked,
		Qt::UniqueConnection);
	connect(ui->removeBtn, &QAbstractButton::clicked,
		this, &AudioMixerController::removeClicked,
		Qt::UniqueConnection);
	connect(ui->renameBtn, &QAbstractButton::clicked,
		this, &AudioMixerController::renameClicked,
		Qt::UniqueConnection);
	connect(ui->delayBtn, &QAbstractButton::clicked,
		this, &AudioMixerController::delayClicked,
		Qt::UniqueConnection);
	connect(ui->moveUpBtn, &QAbstractButton::clicked,
		this, &AudioMixerController::moveUpClicked,
		Qt::UniqueConnection);
	connect(ui->moveDownBtn, &QAbstractButton::clicked,
		this, &AudioMixerController::moveDownClicked,
		Qt::UniqueConnection);

	// Populate sources combo box
	refreshSourceBox();

	// Populate input list
	refreshInputList();
}

void AudioMixerController::enableUpdatesTimeout()
{
	AudioMixerPage *page =
		static_cast<AudioMixerPage *>(m_wizWin->getActivePage());
	if(page == NULL)
		return;
	Ui_AudioMixerPage *ui = page->getUi();
	if(ui == NULL)
		return;
	ui->inputList->setUpdatesEnabled(true);
}

bool AudioMixerController::cancelWizard()
{
	// Closing the window is the same as clicking "OK"
	nextPage();

	return true; // We have fully processed the cancel
}

void AudioMixerController::resetPage()
{
	// Never called
}

void AudioMixerController::backPage()
{
	// Never called
}

void AudioMixerController::nextPage()
{
	// Close the wizard window
	leavingPage();
	m_wizWin->controllerFinished();
}

void AudioMixerController::addClicked()
{
	// Get UI object
	AudioMixerPage *page =
		static_cast<AudioMixerPage *>(m_wizWin->getActivePage());
	Ui_AudioMixerPage *ui = page->getUi();

	// Get the currently selected source
	int i = ui->sourceBox->currentIndex();
	if(i < 0)
		return;
	Q_ASSERT(sizeof(qulonglong) == sizeof(quint64)); // We assume this
	quint64 sourceId = ui->sourceBox->itemData(i, Qt::UserRole).toULongLong();

	// Add source to mixer at the top of the list
	AudioMixer *mixer = App->getProfile()->getAudioMixer();
	AudioInput *input = mixer->createInput(sourceId, tr("Unnamed"), 0);

	// Refresh everything
	refreshSourceBox();
	refreshInputList();

	// Immediately open rename dialog
	m_renameFromCreate = true;
	showRenameDialog(input);
}

void AudioMixerController::removeClicked()
{
	AudioInput *input = getSelectedInput();
	if(input == NULL)
		return;

	AudioMixer *mixer = App->getProfile()->getAudioMixer();
	mixer->destroyInput(input);

	refreshSourceBox();
}

void AudioMixerController::renameClicked()
{
	AudioInput *input = getSelectedInput();
	if(input == NULL)
		return;
	m_renameFromCreate = false;
	showRenameDialog(input);
}

void AudioMixerController::delayClicked()
{
	AudioInput *input = getSelectedInput();
	if(input == NULL)
		return;
	showDelayDialog(input);
}

void AudioMixerController::showRenameDialog(AudioInput *input)
{
	m_renameInput = input;

	// Create rename dialog
	m_renameDialog = new QInputDialog(m_wizWin);
	connect(m_renameDialog, &QInputDialog::finished,
		this, &AudioMixerController::renameDialogClosed);
	m_renameDialog->setInputMode(QInputDialog::TextInput);
	m_renameDialog->setLabelText(tr("Audio source name:"));
	m_renameDialog->setTextValue(m_renameInput->getName());
	m_renameDialog->setModal(true);
	m_renameDialog->show();
}

void AudioMixerController::renameDialogClosed(int result)
{
	if(result == QDialog::Accepted)
		m_renameInput->setName(m_renameDialog->textValue());

	// User cancelled when creating a new input, remove it from the mixer
	if(m_renameFromCreate && result == QDialog::Rejected) {
		AudioMixer *mixer = App->getProfile()->getAudioMixer();
		mixer->destroyInput(m_renameInput);
	}
	m_renameFromCreate = false;

	// Destroy rename dialog
	m_renameDialog->deleteLater();
	m_renameDialog = NULL;
	m_renameInput = NULL;

	// Refresh everything
	refreshSourceBox();
	refreshInputList();
}

void AudioMixerController::showDelayDialog(AudioInput *input)
{
	m_delayInput = input;

	// Create delay dialog
	m_delayDialog = new QInputDialog(m_wizWin);
	connect(m_delayDialog, &QInputDialog::finished,
		this, &AudioMixerController::delayDialogClosed);
	m_delayDialog->setInputMode(QInputDialog::IntInput);
	m_delayDialog->setIntMinimum(-60 * 1000); // -60 seconds
	m_delayDialog->setIntMaximum(60 * 1000); // +60 seconds
	m_delayDialog->setLabelText(tr("Audio source delay (Milliseconds):"));
	m_delayDialog->setIntValue(m_delayInput->getDelay() / 1000LL);
	m_delayDialog->setModal(true);
	m_delayDialog->show();
}

void AudioMixerController::delayDialogClosed(int result)
{
	if(result == QDialog::Accepted) {
		m_delayInput->setDelay((qint64)m_delayDialog->intValue() * 1000LL);
		AudioMixer *mixer = App->getProfile()->getAudioMixer();
		mixer->resetMixer(); // WARNING: Causes output discontinuity
	}

	// Destroy delay dialog
	m_delayDialog->deleteLater();
	m_delayDialog = NULL;
	m_delayInput = NULL;

	// Refresh everything
	refreshSourceBox();
	refreshInputList();
}

void AudioMixerController::moveUpClicked()
{
	AudioInput *input = getSelectedInput();
	if(input == NULL)
		return;
	AudioMixer *mixer = App->getProfile()->getAudioMixer();
	int index = mixer->indexOfInput(input);
	if(index == 0)
		return; // Already at the top of the stack
	mixer->moveInputTo(input, index - 1);

	// Refresh input list and reselect the original input
	m_doDelayedRepaint = true;
	leavingPage();
	resetWizard();
	m_doDelayedRepaint = false;
	m_inputWidgets.at(index - 1)->select();
}

void AudioMixerController::moveDownClicked()
{
	AudioInput *input = getSelectedInput();
	if(input == NULL)
		return;
	AudioMixer *mixer = App->getProfile()->getAudioMixer();
	int index = mixer->indexOfInput(input);
	if(index == mixer->getInputs().count() - 1)
		return; // Already at the bottom of the stack
	mixer->moveInputTo(input, index + 2);

	// Refresh input list and reselect the original input
	m_doDelayedRepaint = true;
	leavingPage();
	resetWizard();
	m_doDelayedRepaint = false;
	m_inputWidgets.at(index + 1)->select();
}

void AudioMixerController::inputDeleted(AudioInput *input)
{
	// The input that was deleted should always be the one that is currently
	// selected. Make the next input in the list the selected one.
	AudioMixer *mixer = App->getProfile()->getAudioMixer();
	int index = mixer->indexOfInput(input);

	// Remove the input's radio button from the group
	m_btnGroup.removeButton(m_inputWidgets.at(index)->getButton());

	// Deleting our widget also removes it from the UI layout
	delete m_inputWidgets.at(index);
	m_inputWidgets.remove(index);

	// Select the next input
	if(m_inputWidgets.count() > 0)
		m_inputWidgets.at(qMin(index, m_inputWidgets.count() - 1))->select();

	// Reset the button state
	resetButtons();
}

void AudioMixerController::sourceAdded(AudioSource *source)
{
	refreshSourceBox();
}

void AudioMixerController::sourceDeleted(AudioSource *source)
{
	refreshSourceBox();
}
