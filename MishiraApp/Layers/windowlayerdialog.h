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

#ifndef WINDOWLAYERDIALOG_H
#define WINDOWLAYERDIALOG_H

#include "layerdialog.h"
#include "ui_windowlayerdialog.h"

class WindowLayer;

//=============================================================================
class WindowLayerDialog : public LayerDialog
{
	Q_OBJECT

private: // Datatypes ---------------------------------------------------------
	struct WindowSortData {
		QString	name;
		QString	exe;
		QString	title;
	};

private: // Members -----------------------------------------------------------
	Ui_WindowLayerDialog		m_ui;
	QVector<QListWidgetItem *>	m_windows;
	bool						m_ignoreSignals;

public: // Constructor/destructor ---------------------------------------------
	WindowLayerDialog(WindowLayer *layer, QWidget *parent = NULL);
	~WindowLayerDialog();

public: // Interface ----------------------------------------------------------
	virtual void	loadSettings();
	virtual void	applySettings();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			leftCropEditChanged(const QString &text);
	void			rightCropEditChanged(const QString &text);
	void			topCropEditChanged(const QString &text);
	void			botCropEditChanged(const QString &text);

	void			refreshClicked();
	void			addWindowClicked();
	void			removeWindowClicked();
	void			moveUpClicked();
	void			moveDownClicked();

	void			gammaSliderChanged(int value);
	void			gammaBoxChanged(double value);
	void			brightnessChanged(int value);
	void			contrastChanged(int value);
	void			saturationChanged(int value);
};
//=============================================================================

#endif // WINDOWLAYERDIALOG_H
