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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>

class MainEmptyPage;
class MainPage;

//=============================================================================
class MainWindow : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	QVBoxLayout		m_layout;
	MainEmptyPage *	m_emptyPage;
	MainPage *		m_mainPage;

public: // Constructor/destructor ---------------------------------------------
	MainWindow(QWidget *parent = 0);
	~MainWindow();

public: // Methods ------------------------------------------------------------
	MainPage *		getMainPage() const;
	void			useEmptyPage();
	void			useMainPage();

private: // Events ------------------------------------------------------------
	virtual void	closeEvent(QCloseEvent *ev);
};
//=============================================================================

inline MainPage *MainWindow::getMainPage() const
{
	return m_mainPage;
}

#endif // MAINWINDOW_H
