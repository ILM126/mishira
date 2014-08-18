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

#ifndef AUDIOMIXERCONTROLLER_H
#define AUDIOMIXERCONTROLLER_H

#include "wizardcontroller.h"
#include <QtCore/QVector>
#include <QtWidgets/QButtonGroup>

class AudioInput;
class AudioInputInfoWidget;
class AudioSource;
class QInputDialog;

//=============================================================================
class AudioMixerController : public WizardController
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QVector<AudioInputInfoWidget *>	m_inputWidgets;
	QButtonGroup					m_btnGroup;
	bool							m_doDelayedRepaint;
	QInputDialog *					m_renameDialog;
	AudioInput *					m_renameInput;
	bool							m_renameFromCreate;
	QInputDialog *					m_delayDialog;
	AudioInput *					m_delayInput;

public: // Constructor/destructor ---------------------------------------------
	AudioMixerController(
		WizController type, WizardWindow *wizWin, WizShared *shared);
	~AudioMixerController();

private: // Methods -----------------------------------------------------------
	void			leavingPage();
	void			resetButtons();
	AudioInput *	getSelectedInput() const;
	void			refreshSourceBox();
	void			refreshInputList();
	void			showRenameDialog(AudioInput *input);
	void			showDelayDialog(AudioInput *input);

public: // Interface ----------------------------------------------------------
	virtual void	resetWizard();
	virtual bool	cancelWizard();
	virtual void	resetPage();
	virtual void	backPage();
	virtual void	nextPage();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			addClicked();
	void			removeClicked();
	void			renameClicked();
	void			delayClicked();
	void			moveUpClicked();
	void			moveDownClicked();

	void			renameDialogClosed(int result);
	void			delayDialogClosed(int result);

	void			inputDeleted(AudioInput *input);
	void			sourceAdded(AudioSource *source);
	void			sourceDeleted(AudioSource *source);

	void			enableUpdatesTimeout();
};
//=============================================================================

#endif // AUDIOMIXERCONTROLLER_H
