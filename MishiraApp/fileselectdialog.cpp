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

#include "fileselectdialog.h"
#include "common.h"
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QGuiApplication>
#include <QtGui/5.0.2/QtGui/qpa/qplatformnativeinterface.h>
#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#endif

//=============================================================================
// Helpers

QString getCOMErrorCode(HRESULT res)
{
	switch(res) {
	case E_FAIL:
		return QStringLiteral("E_FAIL");
	case E_INVALIDARG:
		return QStringLiteral("E_INVALIDARG");
	case E_OUTOFMEMORY:
		return QStringLiteral("E_OUTOFMEMORY");
	case S_FALSE:
		return QStringLiteral("S_FALSE");
	case S_OK:
		return QStringLiteral("S_OK");
	default:
		return numberToHexString((uint)res);
	}
}

//=============================================================================
// FileSelectDialogThread class

FileSelectDialogThread::FileSelectDialogThread(FileOpenDialog *dialog)
	: QThread()
	, m_dialog(dialog)
{
	Q_ASSERT(dialog != NULL);
}

FileSelectDialogThread::~FileSelectDialogThread()
{
}

void FileSelectDialogThread::run()
{
	m_dialog->run();
}

//=============================================================================
// FileOpenDialog class

FileOpenDialog::FileOpenDialog(QWidget *parent)
	: QObject(parent)
	, m_filters()
	, m_viewMode(Detailed)
	, m_isSaveDialog(false)
	, m_displayOverwriteMessage(true)
	, m_initialFilename()
	, m_defaultExtensions()
	//, m_modalHack(NULL)
{
#if 0
	// Setup our modal HACK window in order to make Qt recognise that the
	// native dialog should be modal.
	m_modalHack.setWindowModality(Qt::ApplicationModal);
	m_modalHack.setFixedSize(1, 1); // 1x1 in size
	m_modalHack.move(0, 1); // Be 1px away from the corner
	m_modalHack.setWindowFlags(
		Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint |
		Qt::WindowTransparentForInput | Qt::WindowDoesNotAcceptFocus);
#endif // 0

	// Detect when the dialog is closed in a thread-safe manner
	connect(this, &FileOpenDialog::accepted,
		this, &FileOpenDialog::dialogClosed);
	connect(this, &FileOpenDialog::rejected,
		this, &FileOpenDialog::dialogClosed);
}

FileOpenDialog::~FileOpenDialog()
{
}

/// <summary>
/// Filters must be in the format "[Description]([Filters])" where
/// "[Description]" is optional and "[Filters]" is a non-empty list of file
/// patterns separated by one or more semicolons or spaces. For example:
/// "Image files (*.png;*.jpg;*.jpeg;*.gif;*.bmp)".
/// </summary>
void FileOpenDialog::setNameFilters(const QStringList &filters)
{
	m_filters = filters;
}

void FileOpenDialog::setViewMode(ViewMode mode)
{
	m_viewMode = mode;
}

#ifdef Q_OS_WIN
/// <summary>
/// Creates an open file dialog using the Windows Vista API. This method is
/// executed in its own dedicated thread.
/// </summary>
void FileOpenDialog::run()
{
	// Because this is a brand new thread we need to initialize COM manually
	HRESULT res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if(FAILED(res)) {
		appLog(Log::Warning)
			<< "Failed to initialize COM. Reason = " << getCOMErrorCode(res);
		emit rejected();
		return;
	}

	// Create dialog object
	IFileDialog *dialog = NULL;
	res = CoCreateInstance(
		m_isSaveDialog ? CLSID_FileSaveDialog : CLSID_FileOpenDialog,
		NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog));
	if(FAILED(res)) {
		if(m_isSaveDialog) {
			appLog(Log::Warning)
				<< "Failed to create an save file dialog. "
				<< "Reason = " << getCOMErrorCode(res);
		} else {
			appLog(Log::Warning)
				<< "Failed to create an open file dialog. "
				<< "Reason = " << getCOMErrorCode(res);
		}
		emit rejected();
		return;
	}

	// Setup dialog options
	DWORD options;
	dialog->GetOptions(&options);
	options |= FOS_STRICTFILETYPES;
	options |= FOS_FORCEFILESYSTEM;
	if(m_isSaveDialog) {
		if(!m_displayOverwriteMessage)
			options &= ~FOS_OVERWRITEPROMPT;
		//options |= FOS_STRICTFILETYPES;
	}
	//options |= FOS_PATHMUSTEXIST;
	//options |= FOS_FILEMUSTEXIST;
	//options |= FOS_FORCEPREVIEWPANEON;
	dialog->SetOptions(options);

	// Setup file filters. We need to convert all QStrings into LPCWSTRs so
	// it's slightly ugly.
	QVector<FilterSpec> specs = filterSpecs(m_filters);
	QVector<wchar_t *> strings;
	strings.reserve(specs.count() * 2);
	COMDLG_FILTERSPEC *filterSpec = new COMDLG_FILTERSPEC[specs.count()];
	for(int i = 0; i < specs.count(); i++) {
		const FilterSpec &spec = specs.at(i);
		wchar_t *desc = QStringToWChar(spec.desc);
		wchar_t *filter = QStringToWChar(spec.filter);
		strings.append(desc);
		strings.append(filter);
		filterSpec[i].pszName = desc;
		filterSpec[i].pszSpec = filter;
	}
	dialog->SetFileTypes(specs.count(), filterSpec);
	delete[] filterSpec;
	for(int i = 0; i < strings.count(); i++)
		delete[] strings.at(i);
	strings.clear();

	dialog->SetFileTypeIndex(0);
	//dialog->SetDefaultFolder(); // TODO

	// Setup the initial filename and directory if one is set
	if(!m_initialFilename.isEmpty()) {
		QFileInfo info(m_initialFilename);
		QString dir = QDir::toNativeSeparators(info.absolutePath());
		QString file = info.fileName();
		wchar_t *dirname = QStringToWChar(dir);
		wchar_t *filename = QStringToWChar(file);

		IShellItem *item;
		res = SHCreateItemFromParsingName(dirname, NULL, IID_PPV_ARGS(&item));
		if(SUCCEEDED(res)) {
			dialog->SetFolder(item);
			item->Release();
		}
		dialog->SetFileName(filename);

		delete[] dirname;
		delete[] filename;
	}

	// Setup the default extension if one is set.
	// This is disabled as it doesn't work properly for "*.*" filters.
#if 0
	if(m_defaultExtensions.count() > 0) {
		QString defaults;
		for(int i = 0; i < m_defaultExtensions.count(); i++) {
			if(i != 0)
				defaults += ";";
			defaults += m_defaultExtensions.at(i);
		}
		wchar_t *ext = QStringToWChar(defaults);
		dialog->SetDefaultExtension(ext);
		delete[] ext;
	}
#endif // 0

	// Get the HWND of the parent window. If we do not provide a HWND then
	// Windows will not return focus to the previous window.
	QPlatformNativeInterface *native =
		QGuiApplication::platformNativeInterface();
	HWND hwnd = static_cast<HWND>(native->nativeResourceForWindow(
		"handle", static_cast<QWidget *>(parent())->window()->windowHandle()));

	// Show the dialog
	res = dialog->Show(hwnd);
	if(FAILED(res)) {
		// We receive a failed result when the user clicks cancels
		//appLog(Log::Warning)
		//	<< "Failed to display an open file dialog. "
		//	<< "Reason = " << getCOMErrorCode(res);
		emit rejected();
		return;
	}

	// Get result filename
	IShellItem *result;
	res = dialog->GetResult(&result);
	if(FAILED(res)) {
		appLog(Log::Warning)
			<< "Failed to get the IShellItem from the dialog. "
			<< "Reason = " << getCOMErrorCode(res);
		emit rejected();
		return;
	}
	wchar_t *wFilename = NULL;
	res = result->GetDisplayName(SIGDN_FILESYSPATH, &wFilename);
	if(FAILED(res)) {
		appLog(Log::Warning)
			<< "Failed to get the display name from the IShellItem. "
			<< "Reason = " << getCOMErrorCode(res);
		result->Release();
		emit rejected();
		return;
	}

	// Convert filename to a QString
	QString filename = QString::fromUtf16(wFilename);

	// Clean up
	CoTaskMemFree(wFilename);
	result->Release();
	dialog->Release();

	// Emit success signals
	emit fileSelected(filename);
	emit accepted();
}
#else
#error Unsupported system
#endif

void FileOpenDialog::open()
{
	//m_modalHack.show();
	FileSelectDialogThread *thread = new FileSelectDialogThread(this);
	connect(thread, &FileSelectDialogThread::finished,
		thread, &FileSelectDialogThread::deleteLater);
	thread->start();
}

void FileOpenDialog::dialogClosed()
{
	//m_modalHack.close();
}

/// <summary>
/// Separate the filter descriptions from the actual filters.
/// </summary>
QVector<FileOpenDialog::FilterSpec>
	FileOpenDialog::filterSpecs(QStringList filters) const
{
	QRegExp filterDescPattern("([^(]*)\\(([^)]+)\\)");
	const QRegExp filterSepPattern("[;\\s]+");

	QVector<FilterSpec> specs;
	for(int i = 0; i < filters.count(); i++) {
		const QString filter = filters.at(i);
		int index = filterDescPattern.indexIn(filter);
		if(index < 0)
			continue; // Invalid filter string
		FilterSpec spec;
		spec.desc = filterDescPattern.cap(1);
		spec.filter = filterDescPattern.cap(2);
		spec.filter = spec.filter.replace(filterSepPattern, ";");
		specs.append(spec);
	}
	return specs;
}

//=============================================================================
// FileSaveDialog class

FileSaveDialog::FileSaveDialog(QWidget *parent)
	: FileOpenDialog(parent)
{
	m_isSaveDialog = true;
}

FileSaveDialog::~FileSaveDialog()
{
}
