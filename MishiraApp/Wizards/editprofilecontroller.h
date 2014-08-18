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

#ifndef EDITPROFILECONTROLLER_H
#define EDITPROFILECONTROLLER_H

#include "wizardcontroller.h"

//=============================================================================
class EditProfileController : public WizardController
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	EditProfileController(
		WizController type, WizardWindow *wizWin, WizShared *shared);
	~EditProfileController();

public: // Interface ----------------------------------------------------------
	virtual void	resetWizard();
	virtual bool	cancelWizard();
	virtual void	resetPage();
	virtual void	backPage();
	virtual void	nextPage();
};
//=============================================================================

#endif // EDITPROFILECONTROLLER_H
