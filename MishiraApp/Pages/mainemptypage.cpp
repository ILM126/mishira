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

#include "mainemptypage.h"
#include "stylehelper.h"
#include <QtWidgets/QGraphicsDropShadowEffect>

MainEmptyPage::MainEmptyPage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
{
	m_ui.setupUi(this);

	// Set the base window colour
	QPalette pal = palette();
	pal.setColor(backgroundRole(), StyleHelper::DarkBg5Color);
	setPalette(pal);
	setAutoFillBackground(true);

	// Set text colours
	pal = m_ui.loadingLbl->palette();
	pal.setColor(m_ui.loadingLbl->foregroundRole(), StyleHelper::DarkBg3Color);
	m_ui.loadingLbl->setPalette(pal);

	// Add a drop shadow effect to the loading text. The label takes ownership
	// of the effect and will delete it when the label is deleted or when we
	// assign a new effect to the label.
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
	effect->setBlurRadius(1.0f);
	effect->setColor(QColor(255, 255, 255, 28));
	effect->setOffset(1.0f, 1.0f);
	m_ui.loadingLbl->setGraphicsEffect(effect);
}

MainEmptyPage::~MainEmptyPage()
{
}
