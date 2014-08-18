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

#ifndef WIZARDWINDOW_H
#define WIZARDWINDOW_H

#include "common.h"
#include "Widgets/dialogheader.h"
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>

class WizardController;

//=============================================================================
class WizardWindow : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	WizShared			m_shared;

	// Outer layout
	QVBoxLayout			m_outerLayout;
	DialogHeader		m_header;

	// Inner layout
	QWidget				m_innerBox;
	QVBoxLayout			m_innerLayout;
	QFrame				m_line;
	QWidget				m_buttonBox;
	QHBoxLayout			m_buttonLayout;
	QPushButton			m_backBtn; // "< Back" (Windows) or "Go back" (OS X)
	QPushButton			m_nextBtn; // "Next >" (Windows) or "Continue" (OS X)
	QPushButton			m_finishBtn; // "Finish" (Windows) or "Done" (OS X)
	QPushButton			m_okBtn;
	QPushButton			m_cancelBtn;
	QPushButton			m_resetBtn;

	// Controllers and pages
	WizardController *	m_controllers[WIZ_NUM_CONTROLLERS];
	WizardController *	m_activeController;
	QWidget *			m_pages[WIZ_NUM_PAGES];
	QWidget *			m_activePage;

public: // Constructor/destructor ---------------------------------------------
	WizardWindow(QWidget *parent = 0);
	~WizardWindow();

public: // Methods ------------------------------------------------------------
	void			setController(WizController controller);
	void			setPage(WizPage page);
	QWidget *		getActivePage();
	WizShared *		getShared();
	void			setVisibleButtons(WizButtonsMask mask);
	void			setTitle(const QString &text);
	void			controllerFinished();

private:
	void			addButtonToBar(
		WizButtonsMask mask, WizButton btn, QPushButton *widget);

private: // Events ------------------------------------------------------------
	virtual void	keyPressEvent(QKeyEvent *ev);
	virtual void	closeEvent(QCloseEvent *ev);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			backEvent();
	void			nextEvent();
	void			finishEvent();
	void			okEvent();
	void			cancelEvent();
	void			resetEvent();

	void			setCanContinue(bool canContinue);
};
//=============================================================================

inline QWidget *WizardWindow::getActivePage()
{
	return m_activePage;
}

inline WizShared *WizardWindow::getShared()
{
	return &m_shared;
}

#endif // WIZARDWINDOW_H
