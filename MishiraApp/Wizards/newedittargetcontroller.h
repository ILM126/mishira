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

#ifndef NEWEDITTARGETCONTROLLER_H
#define NEWEDITTARGETCONTROLLER_H

#include "wizardcontroller.h"
#include <QtCore/QMap>

class AudioEncoder;
class VideoEncoder;

typedef QMap<Target *, QString> ShareMap;
typedef QMapIterator<Target *, QString> ShareMapIterator;

//=============================================================================
class NewEditTargetController : public WizardController
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	WizPage				m_curPage;
	WizTargetSettings *	m_defaults;
	WizTargetSettings	m_settings;

public: // Static methods -----------------------------------------------------
	static void		prepareCreateTarget(
		WizShared *shared, bool returnToEditTargets);
	static void		prepareModifyTarget(
		WizShared *shared, Target *target, bool returnToEditTargets);

public: // Constructor/destructor ---------------------------------------------
	NewEditTargetController(
		WizController type, WizardWindow *wizWin, WizShared *shared);
	~NewEditTargetController();

private: // Methods -----------------------------------------------------------
	ShareMap		generateShareMap(bool forVideo) const;
	void			setPageBasedOnTargetType();

	void			resetTargetTypePage();
	void			resetVideoPage();
	void			resetAudioPage();
	void			resetFileTargetPage();
	void			resetRTMPTargetPage();
	void			resetTwitchTargetPage();
	void			resetUstreamTargetPage();
	void			resetHitboxTargetPage();

	void			nextTargetTypePage();
	void			nextVideoPage();
	void			nextAudioPage();
	void			nextFileTargetPage();
	void			nextRTMPTargetPage();
	void			nextTwitchTargetPage();
	void			nextUstreamTargetPage();
	void			nextHitboxTargetPage();

	void			recreateTarget();
	VideoEncoder *	getOrCreateVideoEncoder();
	AudioEncoder *	getOrCreateAudioEncoder();

public: // Interface ----------------------------------------------------------
	virtual void	resetWizard();
	virtual bool	cancelWizard();
	virtual void	resetPage();
	virtual void	backPage();
	virtual void	nextPage();
};
//=============================================================================

#endif // NEWEDITTARGETCONTROLLER_H
