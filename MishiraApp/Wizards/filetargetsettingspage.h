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

#ifndef FILETARGETSETTINGSPAGE_H
#define FILETARGETSETTINGSPAGE_H

#include "fileselectdialog.h"
#include "ui_filetargetsettingspage.h"

class WizardWindow;
struct WizTargetSettings;

//=============================================================================
class FileTargetSettingsPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_FileTargetSettingsPage	m_ui;
	FileSaveDialog				m_fileDialog;

public: // Constructor/destructor ---------------------------------------------
	FileTargetSettingsPage(QWidget *parent = NULL);
	~FileTargetSettingsPage();

public: // Methods ------------------------------------------------------------
	Ui_FileTargetSettingsPage *	getUi();
	bool						isValid() const;

	void	sharedReset(WizardWindow *wizWin, WizTargetSettings *defaults);
	void	sharedNext(WizTargetSettings *settings);

private:
	void	updateFilterList();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	validityMaybeChanged(bool isValid);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	nameEditChanged(const QString &text);
	void	fileTypeChanged(int index);
	void	filenameEditChanged(const QString &text);
	void	fileDialogSelected(const QString &text);
};
//=============================================================================

inline Ui_FileTargetSettingsPage *FileTargetSettingsPage::getUi()
{
	return &m_ui;
}

#endif // FILETARGETSETTINGSPAGE_H
