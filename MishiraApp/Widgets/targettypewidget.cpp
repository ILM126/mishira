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

#include "targettypewidget.h"

TargetTypeWidget::TargetTypeWidget(
	TrgtType type, const QPixmap &icon, QWidget *parent)
	: m_type(type)
	, m_iconWidget(this)
	, m_iconLayout(&m_iconWidget)
	, m_iconLabel(&m_iconWidget)
{
	setFixedHeight(120);
	m_iconLabel.setPixmap(icon);
	m_iconLayout.setMargin(0);
	m_iconLayout.addWidget(&m_iconLabel, 0, Qt::AlignCenter);
	m_mainLayout.addWidget(&m_iconWidget);
}

TargetTypeWidget::~TargetTypeWidget()
{
}
