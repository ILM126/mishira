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

#include "newprofilepage.h"
#include "common.h"
#include "validators.h"

NewProfilePage::NewProfilePage(QWidget *parent)
	: QWidget(parent)
	, m_ui()
{
	m_ui.setupUi(this);

	// Setup validators
	m_ui.nameEdit->setValidator(new ProfileNameValidator(this, true));
	connect(m_ui.nameEdit, &QLineEdit::textChanged,
		this, &NewProfilePage::nameEditChanged);
}

NewProfilePage::~NewProfilePage()
{
}

bool NewProfilePage::isValid() const
{
	if(!m_ui.nameEdit->hasAcceptableInput())
		return false;
	return true;
}

void NewProfilePage::nameEditChanged(const QString &text)
{
	doQLineEditValidate(m_ui.nameEdit);
	emit validityMaybeChanged(isValid());
}
