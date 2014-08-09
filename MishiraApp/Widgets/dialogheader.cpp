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

#include "dialogheader.h"
#include "stylehelper.h"
#include "Widgets/borderspacer.h"
#include <QtWidgets/QGraphicsDropShadowEffect>

DialogHeader::DialogHeader(QWidget *parent)
	: QWidget(parent)
	, m_layout(this)
	, m_titleLbl(this)
	, m_logoLbl(this)
{
	setFixedHeight(50);

	m_titleLbl.setText("???");
	m_logoLbl.setPixmap(QPixmap(":/Resources/wizard-logo.png"));
	m_layout.setMargin(0);
	m_layout.setSpacing(0);

	// Row 0
	m_layout.setRowStretch(0, 1);

	// Row 1
	m_layout.setColumnMinimumWidth(0, 9);
	m_layout.addWidget(&m_titleLbl, 1, 1);
	m_layout.setColumnStretch(2, 1);
	m_layout.addWidget(&m_logoLbl, 1, 3);

	// Row 2
	m_layout.setRowStretch(2, 1);

	// Row 3
	m_layout.addWidget(new VBorderSpacer(
		this, 3, StyleHelper::FrameNormalBorderColor), 3, 0, 1, 4);

	// Set background colour
	QPalette pal = palette();
	pal.setColor(backgroundRole(), StyleHelper::WizardHeaderColor);
	setAutoFillBackground(true);
	setPalette(pal);

	// Set title text colour
	pal = m_titleLbl.palette();
	pal.setColor(m_titleLbl.foregroundRole(), Qt::white);
	m_titleLbl.setPalette(pal);

	// Setup title font
	QFont font = m_titleLbl.font();
	font.setPixelSize(19);
#ifdef Q_OS_WIN
	font.setFamily(QStringLiteral("Calibri"));
#endif
	m_titleLbl.setFont(font);

	// Add a drop shadow effect to the label. The label takes ownership of the
	// effect and will delete it when the label is deleted or when we assign a
	// new effect to the label.
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(1.0f);
	effect->setColor(QColor(0x00, 0x00, 0x00, 0x54)); // 33%
	effect->setOffset(1.0f, 1.0f);
	m_titleLbl.setGraphicsEffect(effect);
}

DialogHeader::~DialogHeader()
{
}

void DialogHeader::setTitle(const QString &text)
{
	m_titleLbl.setText(text);
}
