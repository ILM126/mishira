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

#ifndef BENCHMARKPAGE_H
#define BENCHMARKPAGE_H

#include "ui_benchmarkpage.h"

//=============================================================================
class BenchmarkPage : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_BenchmarkPage	m_ui;

public: // Constructor/destructor ---------------------------------------------
	BenchmarkPage(QWidget *parent = NULL);
	~BenchmarkPage();

public: // Methods ------------------------------------------------------------
	Ui_BenchmarkPage *	getUi();
};
//=============================================================================

inline Ui_BenchmarkPage *BenchmarkPage::getUi()
{
	return &m_ui;
}

#endif // BENCHMARKPAGE_H
