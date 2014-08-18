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

#ifndef UNITLINEEDIT_H
#define UNITLINEEDIT_H

#include <QtWidgets/QLineEdit>

//=============================================================================
/// <summary>
/// A QLineEdit that has greyed-out units to the right of the user input.
/// </summary>
class UnitLineEdit : public QLineEdit
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	QString		m_units;

public: // Constructor/destructor ---------------------------------------------
	UnitLineEdit(QWidget *parent = 0);
	UnitLineEdit(const QString &contents, QWidget *parent = 0);
	UnitLineEdit(
		const QString &contents, const QString &units, QWidget *parent = 0);
	~UnitLineEdit();

public: // Methods ------------------------------------------------------------
	void			setUnits(const QString &units);
	QString			getUnits() const;

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

inline void UnitLineEdit::setUnits(const QString &units)
{
	m_units = units;
}

inline QString UnitLineEdit::getUnits() const
{
	return m_units;
}

#endif // UNITLINEEDIT_H
