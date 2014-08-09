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

#include "mainwindow.h"
#include "application.h"
#include "appsettings.h"
#include "Pages/mainemptypage.h"
#include "Pages/mainpage.h"
#include <QtGui/QPainter>
#include <QtWidgets/QDesktopWidget>

MainWindow::MainWindow(QWidget *parent)
	: QWidget(parent)
	, m_layout(this)
	, m_emptyPage(NULL)
	, m_mainPage(NULL)
{
	// Set the minimum window size to be 20px less than the minimum target
	// resolution (1024x720) minus the window decoration size (16x38) and the
	// largest Windows taskbar size (Both in the horizontal and vertical
	// positions which is 62x40) which is 926x622. As we don't actually require
	// the full vertical space reduce the height by another 100px (926x522).
	//setMinimumSize(1024 - 16 - 62 - 20, 720 - 38 - 40 - 20); // 926x622
	setMinimumSize(926, 622 - 100); // 926x522

	// Restore window state from the application settings file. We cannot
	// maximize yet as the application is still initializing
	AppSettings *settings = App->getAppSettings();
	const QByteArray &geom = settings->getMainWinGeom();
	if(!geom.isEmpty()) {
		restoreGeometry(geom);
#ifdef Q_OS_WIN
		//if(settings->getMainWinGeomMaxed())
		//	showMaximized();
#endif
	} else {
		// This is the first time that the window has been displayed, make it
		// fill the majority of the screen by default. To make sure this works
		// on multi-monitor setups we use the size of the smallest monitor.
		QDesktopWidget *desktop = App->desktop();
		int numScreens = desktop->screenCount();
		QSize size(INT_MAX, INT_MAX);
		for(int i = 0; i < numScreens; i++) {
			const QRect &rect = desktop->availableGeometry(i);
			if(rect.width() < size.width())
				size.setWidth(rect.width());
			if(rect.height() < size.height())
				size.setHeight(rect.height());
		}
		size.setWidth(size.width() - 200); // Outside padding
		size.setHeight(size.height() - 200);
		if(size.width() < minimumWidth()) // Obey minimum size to be safe
			size.setWidth(minimumWidth());
		if(size.height() < minimumHeight())
			size.setHeight(minimumHeight());
		resize(size);
	}

	// Remove layout margins so that the displayed page fills the entire window
	m_layout.setMargin(0);
}

MainWindow::~MainWindow()
{
	if(m_emptyPage != NULL)
		delete m_emptyPage;
	if(m_mainPage != NULL)
		delete m_mainPage;
	m_emptyPage = NULL;
	m_mainPage = NULL;
}

void MainWindow::useEmptyPage()
{
	if(m_emptyPage != NULL)
		return; // Already visible

	// Delete any existing visible pages from the window
	if(m_mainPage != NULL) {
		m_layout.removeWidget(m_mainPage);
		delete m_mainPage;
		m_mainPage = NULL;
	}

	// Create and display the requested page
	m_emptyPage = new MainEmptyPage(this);
	m_layout.addWidget(m_emptyPage);
}

void MainWindow::useMainPage()
{
	if(m_mainPage != NULL)
		return; // Already visible

	// Delete any existing visible pages from the window
	if(m_emptyPage != NULL) {
		m_layout.removeWidget(m_emptyPage);
		delete m_emptyPage;
		m_emptyPage = NULL;
	}

	// Create and display the requested page
	m_mainPage = new MainPage(this);
	m_layout.addWidget(m_mainPage);
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
	// Save window state in the application settings file
	AppSettings *settings = App->getAppSettings();
	settings->setMainWinGeomMaxed(isMaximized());
#ifdef Q_OS_WIN
	if(isMaximized()) {
		// `QWidget::saveGeometry()` is broken on Windows when it comes to
		// window maximization as it uses the result of `normalGeometry()` for
		// determining the geometry of the restored state which is incorrectly
		// set to the geometry of the maximized window instead. In order to
		// make `normalGeometry()` return the correct geometry we need to
		// restore the window and process Qt's internal event loop. When using
		// this method we need to manually store if the window was maximized
		// on close and use `showMaximized()` the next time the program is run.
		//
		// If we don't use this method at all then we not only do we lose the
		// restored window geometry but the window is also always maximized on
		// the primary monitor on a multi-monitor setup if the window was
		// maximized when it was previously closed.
		setWindowState(windowState() & ~Qt::WindowMaximized);
		App->processEvents();
	}
#endif
	settings->setMainWinGeom(saveGeometry());

	// As Qt won't emit a "lastWindowClosed" event if we're not using Qt's
	// event loop
	App->exit(0);
}
