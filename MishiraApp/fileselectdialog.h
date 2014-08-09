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

#ifndef FILESELECTDIALOG_H
#define FILESELECTDIALOG_H

#include <QtCore/QThread>
#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class FileOpenDialog;

//=============================================================================
class FileSelectDialogThread : public QThread
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	FileOpenDialog *	m_dialog;

public: // Constructor/destructor ---------------------------------------------
	FileSelectDialogThread(FileOpenDialog *dialog);
	~FileSelectDialogThread();

protected: // Methods ---------------------------------------------------------
	virtual void	run();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			resultReady(const QString &filename);
};
//=============================================================================

//=============================================================================
/// <summary>
/// Provides a native file open dialog that is non-blocking. This is needed as
/// Qt only provides blocking file dialogs. Qt doesn't even allow a QFileDialog
/// to be created in a separate thread so we must do everything ourselves.
/// </summary>
class FileOpenDialog : public QObject
{
	friend class FileSelectDialogThread;

	Q_OBJECT

public: // Datatypes ----------------------------------------------------------
	enum ViewMode {
		List = 0,
		Detailed
	};

private:
	struct FilterSpec {
		QString	desc;
		QString	filter;
	};

protected: // Members ---------------------------------------------------------
	QStringList	m_filters;
	ViewMode	m_viewMode;
	bool		m_isSaveDialog;
	bool		m_displayOverwriteMessage;
	QString		m_initialFilename;
	QStringList	m_defaultExtensions;
	//QWidget		m_modalHack;

public: // Constructor/destructor ---------------------------------------------
	FileOpenDialog(QWidget *parent = NULL);
	virtual ~FileOpenDialog();

public: // Methods ------------------------------------------------------------
	void	setNameFilters(const QStringList &filters);
	void	setViewMode(ViewMode mode);
	void	setInitialFilename(const QString &filename);

private:
	void				run();
	QVector<FilterSpec>	filterSpecs(QStringList filters) const;

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	accepted();
	void	rejected();
	void	fileSelected(const QString &filename);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	open();

	private
Q_SLOTS: // Private slots -----------------------------------------------------
	void	dialogClosed();
};
//=============================================================================

inline void FileOpenDialog::setInitialFilename(const QString &filename)
{
	m_initialFilename = filename;
}

//=============================================================================
class FileSaveDialog : public FileOpenDialog
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	FileSaveDialog(QWidget *parent = NULL);
	virtual ~FileSaveDialog();

public: // Methods ------------------------------------------------------------
	void	setShowOverwriteMessage(bool show);
	//void	setDefaultExtensions(const QStringList &extensions);
};
//=============================================================================

inline void FileSaveDialog::setShowOverwriteMessage(bool show)
{
	m_displayOverwriteMessage = show;
};

#if 0
/// <summary>
/// Sets the default extension for each item in the filter list.
/// </summary>
inline void FileSaveDialog::setDefaultExtensions(const QStringList &extensions)
{
	m_defaultExtensions = extensions;
};
#endif // 0

#endif // FILESELECTDIALOG_H
