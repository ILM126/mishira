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

#ifndef WIZARDCONTROLLER_H
#define WIZARDCONTROLLER_H

#include "common.h"
#include <QtCore/QObject>

class WizardWindow;

//=============================================================================
class WizardController : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	WizController	m_type;
	WizardWindow *	m_wizWin;
	WizShared *		m_shared;

protected: // Constructor/destructor ------------------------------------------
	WizardController(
		WizController type, WizardWindow *wizWin, WizShared *shared);
public:
	~WizardController();

public: // Methods ------------------------------------------------------------
	WizController	getType() const;

public: // Interface ----------------------------------------------------------
	virtual void	resetWizard() = 0;
	virtual bool	cancelWizard() = 0;
	virtual void	resetPage() = 0;
	virtual void	backPage() = 0;
	virtual void	nextPage() = 0;
};
//=============================================================================

inline WizController WizardController::getType() const
{
	return m_type;
}

#endif // WIZARDCONTROLLER_H
