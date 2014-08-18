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

#ifndef EDITTARGETSCONTROLLER_H
#define EDITTARGETSCONTROLLER_H

#include "wizardcontroller.h"
#include <QtCore/QVector>
#include <QtWidgets/QButtonGroup>

class TargetInfoWidget;

//=============================================================================
class EditTargetsController : public WizardController
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QVector<TargetInfoWidget *>	m_targetWidgets;
	QButtonGroup				m_btnGroup;
	bool						m_doDelayedRepaint;

public: // Constructor/destructor ---------------------------------------------
	EditTargetsController(
		WizController type, WizardWindow *wizWin, WizShared *shared);
	~EditTargetsController();

private: // Methods -----------------------------------------------------------
	void			leavingPage();
	void			resetButtons();
	Target *		getSelectedTarget() const;

public: // Interface ----------------------------------------------------------
	virtual void	resetWizard();
	virtual bool	cancelWizard();
	virtual void	resetPage();
	virtual void	backPage();
	virtual void	nextPage();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			createTargetClicked();
	void			modifyTargetClicked();
	void			duplicateTargetClicked();
	void			deleteTargetClicked();
	void			moveTargetUpClicked();
	void			moveTargetDownClicked();

	void			targetDeleted(Target *target);

	void			enableUpdatesTimeout();
};
//=============================================================================

#endif // EDITTARGETSCONTROLLER_H
