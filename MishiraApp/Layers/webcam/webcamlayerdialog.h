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

#ifndef WEBCAMLAYERDIALOG_H
#define WEBCAMLAYERDIALOG_H

#include "layerdialog.h"
#include "ui_webcamlayerdialog.h"

class VideoSource;
class WebcamLayer;

//=============================================================================
class WebcamLayerDialog : public LayerDialog
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_WebcamLayerDialog	m_ui;
	VideoSource *			m_curSource;
	float					m_curFramerate;
	bool					m_ignoreSignals;

public: // Constructor/destructor ---------------------------------------------
	WebcamLayerDialog(WebcamLayer *layer, QWidget *parent = NULL);
	~WebcamLayerDialog();

private: // Methods -----------------------------------------------------------
	void			refreshDeviceList();
	void			refreshFramerateList();
	void			refreshResolutionList();
	void			doEnableDisable();

public: // Interface ----------------------------------------------------------
	virtual void	loadSettings();
	virtual void	applySettings();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	deviceComboChanged(int index);
	void	framerateComboChanged(int index);
	void	configBtnClicked();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			sourceAdded(VideoSource *source);
	void			removingSource(VideoSource *source);
	void			removingSourceTimeout();
};
//=============================================================================

#endif // WEBCAMLAYERDIALOG_H
