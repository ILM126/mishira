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

#include "benchmarkresultspage.h"

BenchmarkResultsPage::BenchmarkResultsPage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
{
	m_ui.setupUi(this);

	// Listen to when the list selection changes
	connect(m_ui.listWidget, &QListWidget::itemSelectionChanged,
		this, &BenchmarkResultsPage::listSelectionChanged);
}

BenchmarkResultsPage::~BenchmarkResultsPage()
{
}

bool BenchmarkResultsPage::isValid() const
{
	if(m_ui.listWidget->selectedItems().count() != 1)
		return false;
	return true;
}

void BenchmarkResultsPage::listSelectionChanged()
{
	emit validityMaybeChanged(isValid());
}
