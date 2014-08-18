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

#include "audioview.h"
#include "application.h"
#include "audiomixer.h"
#include "profile.h"
#include "audioinput.h"
#include "Widgets/audiopane.h"

AudioView::AudioView(QWidget *parent)
	: QWidget(parent)
	, m_panes()
	, m_layout(this)

	// Right-click context menu
	, m_contextInput(NULL)
	, m_contextMenu(this)
	//, m_renameSourceAction(NULL)
	, m_moveSourceUpAction(NULL)
	, m_moveSourceDownAction(NULL)
{
	m_layout.setMargin(0);
	m_layout.setSpacing(0);

	// Populate view from existing audio mixer
	repopulateList();

	// Populate right-click context menu
	//m_renameSourceAction = m_contextMenu.addAction(tr(
	//	"Rename audio source..."));
	//m_contextMenu.addSeparator();
	m_moveSourceUpAction = m_contextMenu.addAction(tr(
		"Move audio source up"));
	m_moveSourceDownAction = m_contextMenu.addAction(tr(
		"Move audio source down"));
	connect(&m_contextMenu, &QMenu::triggered,
		this, &AudioView::audioPaneContextTriggered);

	// Watch audio mixer for additions, deletions and moves
	AudioMixer *mixer = App->getProfile()->getAudioMixer();
	connect(mixer, &AudioMixer::inputAdded,
		this, &AudioView::inputAdded);
	connect(mixer, &AudioMixer::destroyingInput,
		this, &AudioView::destroyingInput);
	connect(mixer, &AudioMixer::inputMoved,
		this, &AudioView::inputMoved);
}

AudioView::~AudioView()
{
	for(int i = 0; i < m_panes.count(); i++)
		delete m_panes.at(i);
	m_panes.clear();
}

void AudioView::repopulateList()
{
	// Clear existing list
	for(int i = 0; i < m_panes.count(); i++)
		delete m_panes.at(i);
	m_panes.clear();

	//-------------------------------------------------------------------------
	// Repopulate list

	// Add master volume
	AudioPane *pane = new AudioPane(NULL, this);
	//input->setupAudioPane(pane);
	m_panes.append(pane);
	m_layout.addWidget(pane);

	// Add audio inputs
	AudioMixer *mixer = App->getProfile()->getAudioMixer();
	AudioInputList inputs = mixer->getInputs();
	for(int i = 0; i < inputs.count(); i++) {
		AudioInput *input = inputs.at(i);
		pane = new AudioPane(input, this);
		//input->setupAudioPane(pane);
		m_panes.append(pane);
		m_layout.addWidget(pane);
		connect(pane, &AudioPane::rightClicked,
			this, &AudioView::audioPaneRightClicked,
			Qt::UniqueConnection);
	}
}

void AudioView::showInputContextMenu(AudioInput *input)
{
	m_contextInput = input;
	m_contextMenu.popup(QCursor::pos());
}

void AudioView::inputAdded(AudioInput *input, int before)
{
	repopulateList();
}

void AudioView::destroyingInput(AudioInput *input)
{
	// We receive the signal before the audio input is actually removed so this
	// doesn't work
	//repopulateList();

	// Deleting our widget also removes it from the UI layout
	int index = App->getProfile()->getAudioMixer()->indexOfInput(input);
	delete m_panes.at(index + 1); // Master volume = 0
	m_panes.remove(index + 1);
}

void AudioView::inputMoved(AudioInput *input, int before)
{
	repopulateList();
}

void AudioView::audioPaneRightClicked(AudioInput *input)
{
	showInputContextMenu(input);
}

void AudioView::audioPaneContextTriggered(QAction *action)
{
	if(m_contextInput == NULL)
		return;
	if(action == m_moveSourceUpAction) {
		// Move audio source up
		AudioMixer *mixer = App->getProfile()->getAudioMixer();
		int index = mixer->indexOfInput(m_contextInput);
		if(index == 0)
			return; // Already at the top of the stack
		mixer->moveInputTo(m_contextInput, index - 1);
	} else if(action == m_moveSourceDownAction) {
		// Move audio source down
		AudioMixer *mixer = App->getProfile()->getAudioMixer();
		int index = mixer->indexOfInput(m_contextInput);
		if(index == mixer->getInputs().count() - 1)
			return; // Already at the bottom of the stack
		mixer->moveInputTo(m_contextInput, index + 2);
	}
}
