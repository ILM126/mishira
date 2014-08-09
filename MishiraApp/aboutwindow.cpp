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

#include "aboutwindow.h"
#include "application.h"
#include "appsettings.h"
#include "common.h"
#include "constants.h"
#include "stylehelper.h"
#include "Widgets/borderspacer.h"
#include <QtGui/QKeyEvent>
#include <QtWidgets/QGraphicsDropShadowEffect>
#include <QtWidgets/QPushButton>

AboutWindow::AboutWindow(QWidget *parent)
	: QWidget(parent)

	// Outer layout
	, m_outerLayout(this)

	// Upper layout
	, m_upperBox(this)
	, m_upperLayout(&m_upperBox)
	, m_titleLbl(&m_upperBox)
	, m_infoLbl(&m_upperBox)
	, m_logoLbl(&m_upperBox)

	// Lower layout
	, m_lowerBox(this)
	, m_lowerLayout(&m_lowerBox)
	, m_legalLbl(&m_lowerBox)
	, m_horiLine(&m_lowerBox)
	, m_buttons(&m_lowerBox)
{
	// Prevent the dialog from being resized
	m_outerLayout.setSizeConstraint(QLayout::SetFixedSize);

	// Make the window modal
	setWindowModality(Qt::WindowModal);

	// This is a child window that does not have a minimize or maximize button
	setWindowFlags(
		Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint |
		Qt::WindowCloseButtonHint);

	// Set window title
	setWindowTitle(tr("About %1").arg(APP_NAME));

	//-------------------------------------------------------------------------
	// Setup upper section

	m_titleLbl.setText(APP_NAME);
	m_infoLbl.setText(tr(
		"<html><head/><body>"

		"<p>Version %1 (Build %2)</p>"

		"<p>Copyright \xc2\xa9 2014 Mishira development team</p>"

		"<p><a href=\"http://www.mishira.com/\">"
		"<span style=\"text-decoration: underline; color:#ffffff;\">"
		"http://www.mishira.com</span></a></p>"

		"</body></html>")
		.arg(APP_VER_STR)
		.arg(getAppBuildVersion()));
	m_infoLbl.setOpenExternalLinks(true);
	m_logoLbl.setPixmap(QPixmap(":/Resources/wizard-logo.png"));
	m_upperLayout.setMargin(0);
	m_upperLayout.setSpacing(0);
	m_upperBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	// Row 0
	m_upperLayout.setRowStretch(0, 1);
	m_upperLayout.setRowMinimumHeight(0, 9);

	// Row 1
	m_upperLayout.setColumnMinimumWidth(0, 9);
	m_upperLayout.addWidget(&m_titleLbl, 1, 1);
	m_upperLayout.setColumnStretch(2, 1);
	m_upperLayout.setColumnMinimumWidth(2, 10);
	m_upperLayout.addWidget(&m_logoLbl, 1, 3, 2, 1);
	m_upperLayout.setColumnMinimumWidth(4, 2);

	// Row 2
	m_upperLayout.addWidget(&m_infoLbl, 2, 1);

	// Row 3
	m_upperLayout.setRowStretch(3, 1);
	m_upperLayout.setRowMinimumHeight(3, 12);

	// Row 4
	m_upperLayout.addWidget(new VBorderSpacer(
		this, 3, StyleHelper::FrameNormalBorderColor), 4, 0, 1, 5);

	// Set background colour
	QPalette pal = palette();
	pal.setColor(backgroundRole(), StyleHelper::WizardHeaderColor);
	m_upperBox.setAutoFillBackground(true);
	m_upperBox.setPalette(pal);

	// Set text colours
	pal = m_titleLbl.palette();
	pal.setColor(m_titleLbl.foregroundRole(), Qt::white);
	m_titleLbl.setPalette(pal);
	m_infoLbl.setPalette(pal);

	// Set text fonts
	QFont font = m_titleLbl.font();
	font.setPixelSize(19);
#ifdef Q_OS_WIN
	font.setFamily(QStringLiteral("Calibri"));
#endif
	m_titleLbl.setFont(font);
	// Info text
	font.setPixelSize(11);
#ifdef Q_OS_WIN
	font.setFamily(QStringLiteral("MS Shell Dlg 2"));
#endif
	m_infoLbl.setFont(font);

	// Add a drop shadow effect to the labels. The label takes ownership of the
	// effect and will delete it when the label is deleted or when we assign a
	// new effect to the label.
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(1.0f);
	effect->setColor(QColor(0x00, 0x00, 0x00, 0x54)); // 33%
	effect->setOffset(1.0f, 1.0f);
	m_titleLbl.setGraphicsEffect(effect);
	// Info text
	effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(1.0f);
	effect->setColor(QColor(0x00, 0x00, 0x00, 0x54)); // 33%
	effect->setOffset(1.0f, 1.0f);
	m_infoLbl.setGraphicsEffect(effect);

	//-------------------------------------------------------------------------
	// Setup lower section

	// Setup legal text
	QDir appDir = QDir(App->applicationDirPath());
	m_legalLbl.setText(tr(
		"<html><head/><body>"

		"<p>The use of this software is conditional upon accepting the "
		"<a href=\"file:///%1\">Mishira Software License Agreement</a>. If "
		"you do not agree with this license please exit Mishira immediately."
		"</p>"

		"<p>This software uses intellectual property from third parties. "
		"Please see <a href=\"file:///%2\">this document</a> for a full list "
		"of legal notices.</p>"

		"</body></html>")
		.arg(appDir.absoluteFilePath(QStringLiteral("License.html")))
		.arg(appDir.absoluteFilePath(QStringLiteral("Legal.html"))));
	m_legalLbl.setOpenExternalLinks(true);
	m_legalLbl.setWordWrap(true);

	// Setup horizontal line
	m_horiLine.setFrameStyle(QFrame::HLine | QFrame::Sunken);
	m_horiLine.setLineWidth(1);

	// Setup button bar
	m_buttons.setStandardButtons(
		QDialogButtonBox::Ok);
	connect(m_buttons.button(QDialogButtonBox::Ok), &QPushButton::clicked,
		this, &AboutWindow::okEvent);

	// Populate layout
	m_lowerLayout.addWidget(&m_legalLbl);
	m_lowerLayout.addSpacing(3);
	m_lowerLayout.addWidget(&m_horiLine);
	m_lowerLayout.addWidget(&m_buttons);

	//-------------------------------------------------------------------------

	// Populate "outer" layout
	m_outerLayout.setMargin(0); // Fill entire window
	m_outerLayout.setSpacing(0);
	m_outerLayout.addWidget(&m_upperBox);
	m_outerLayout.addWidget(&m_lowerBox);
}

AboutWindow::~AboutWindow()
{
}

void AboutWindow::keyPressEvent(QKeyEvent *ev)
{
	switch(ev->key()) {
	default:
		ev->ignore();
		break;
	case Qt::Key_Escape:
		// ESC press is the same as "okay"
		okEvent();
		break;
	}
}

void AboutWindow::okEvent()
{
	hide();
}
