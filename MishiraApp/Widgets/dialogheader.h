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

#ifndef DIALOGHEADER_H
#define DIALOGHEADER_H

#include <QtWidgets/QLabel>
#include <QtWidgets/QGridLayout>

//=============================================================================
class DialogHeader : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	QGridLayout	m_layout;
	QLabel		m_titleLbl;
	QLabel		m_logoLbl;

public: // Constructor/destructor ---------------------------------------------
	DialogHeader(QWidget *parent = 0);
	~DialogHeader();

public: // Methods ------------------------------------------------------------
	QString			getTitle() const;
	void			setTitle(const QString &text);
};
//=============================================================================

inline QString DialogHeader::getTitle() const
{
	return m_titleLbl.text();
}

#endif // DIALOGHEADER_H
