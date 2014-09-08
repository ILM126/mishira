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

#include "filetargetsettingspage.h"
#include "common.h"
#include "validators.h"
#include "wizardwindow.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

FileTargetSettingsPage::FileTargetSettingsPage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
	, m_fileDialog(this)
{
	m_ui.setupUi(this);

	// Populate and connect file extension combobox
	m_ui.fileTypeCombo->clear();
	for(int i = 0; i < NUM_FILE_TARGET_TYPES; i++)
		m_ui.fileTypeCombo->addItem(tr(FileTrgtTypeStrings[i]));
	void (QComboBox:: *fpiCB)(int) = &QComboBox::currentIndexChanged;
	connect(m_ui.fileTypeCombo, fpiCB,
		this, &FileTargetSettingsPage::fileTypeChanged);

	// Setup file dialog and connect it to the filename QLineEdit
	updateFilterList();
	m_fileDialog.setShowOverwriteMessage(false);
	//m_fileDialog.setViewMode(FileSaveDialog::List);
	// TODO: Default directory
	connect(&m_fileDialog, &FileSaveDialog::fileSelected,
		this, &FileTargetSettingsPage::fileDialogSelected);
	connect(m_ui.filenameBtn, &QPushButton::clicked,
		&m_fileDialog, &FileSaveDialog::open);

	// Setup validators. Target name must start with a non-whitespace character
	// and be no longer than 24 characters total
	m_ui.nameEdit->setValidator(new QRegExpValidator(
		QRegExp(QStringLiteral("\\S.{,23}")), this));
	m_ui.filenameEdit->setValidator(new SaveFileValidator(this));
	connect(m_ui.nameEdit, &QLineEdit::textChanged,
		this, &FileTargetSettingsPage::nameEditChanged);
	connect(m_ui.filenameEdit, &QLineEdit::textChanged,
		this, &FileTargetSettingsPage::filenameEditChanged);
}

FileTargetSettingsPage::~FileTargetSettingsPage()
{
}

bool FileTargetSettingsPage::isValid() const
{
	if(!m_ui.nameEdit->hasAcceptableInput())
		return false;
	if(!m_ui.filenameEdit->hasAcceptableInput())
		return false;
	return true;
}

/// <summary>
/// Reset button code for the page that is shared between multiple controllers.
/// </summary>
void FileTargetSettingsPage::sharedReset(
	WizardWindow *wizWin, WizTargetSettings *defaults)
{
	setUpdatesEnabled(false);

	// Update fields
	m_ui.nameEdit->setText(defaults->name);
	m_ui.fileTypeCombo->setCurrentIndex(defaults->fileType);
	m_ui.filenameEdit->setText(defaults->fileFilename);

	// Reset validity and connect signal
	doQLineEditValidate(m_ui.nameEdit);
	doQLineEditValidate(m_ui.filenameEdit);
	wizWin->setCanContinue(isValid());
	connect(this, &FileTargetSettingsPage::validityMaybeChanged,
		wizWin, &WizardWindow::setCanContinue,
		Qt::UniqueConnection);

	setUpdatesEnabled(true);
}

/// <summary>
/// Next button code for the page that is shared between multiple controllers.
/// </summary>
void FileTargetSettingsPage::sharedNext(WizTargetSettings *settings)
{
	// Save fields
	settings->name = m_ui.nameEdit->text().trimmed();
	settings->fileType =
		(FileTrgtType)(m_ui.fileTypeCombo->currentIndex());
	settings->fileFilename = m_ui.filenameEdit->text();
}

void FileTargetSettingsPage::updateFilterList()
{
	int index = m_ui.fileTypeCombo->currentIndex();

	QStringList filters;
	filters << FileTrgtTypeFilterStrings[index];
	filters << "All files (*.*)";
	m_fileDialog.setNameFilters(filters);

#if 0
	// Also update default extension
	QStringList extensions;
	extensions << FileTrgtTypeExtStrings[index];
	extensions << "";
	m_fileDialog.setDefaultExtensions(extensions);
#endif // 0
}

void FileTargetSettingsPage::nameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.nameEdit);
	emit validityMaybeChanged(isValid());
}

void FileTargetSettingsPage::fileTypeChanged(int index)
{
	// Change the filename extension as cleanly as possible
	QFileInfo info(m_ui.filenameEdit->text());
	if(!info.isDir()) {
		QString filename = info.dir().filePath(QStringLiteral("%1.%2")
			.arg(info.completeBaseName())
			.arg(FileTrgtTypeExtStrings[index]));
		filename = QDir::toNativeSeparators(filename);
		m_ui.filenameEdit->setText(filename);
		// `filenameEditChanged()` is automatically called
	}

	// Update file select dialog filter
	updateFilterList();
}

void FileTargetSettingsPage::filenameEditChanged(const QString &text)
{
	m_fileDialog.setInitialFilename(text);
	doQLineEditValidate(m_ui.filenameEdit);
	emit validityMaybeChanged(isValid());
}

void FileTargetSettingsPage::fileDialogSelected(const QString &text)
{
	QFileInfo info(text);
	if(info.suffix().isEmpty()) {
		// Add the default file extension if the user didn't enter one
		m_ui.filenameEdit->setText(QStringLiteral("%1.%2")
			.arg(text)
			.arg(FileTrgtTypeExtStrings[m_ui.fileTypeCombo->currentIndex()]));
	} else
		m_ui.filenameEdit->setText(text);
	// `filenameEditChanged()` is automatically called
}
