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

#ifndef VALIDATORS_H
#define VALIDATORS_H

#include <QtGui/QValidator>

//=============================================================================
class EvenIntValidator : public QIntValidator
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	EvenIntValidator(QObject *parent = NULL);
	EvenIntValidator(int minimum, int maximum, QObject *parent = NULL);
	~EvenIntValidator();

public: // Methods ------------------------------------------------------------
	virtual void	fixup(QString &input) const;
	virtual State	validate(QString &input, int &pos) const;
};
//=============================================================================

//=============================================================================
class IntOrEmptyValidator : public QIntValidator
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	IntOrEmptyValidator(QObject *parent = NULL);
	IntOrEmptyValidator(int minimum, int maximum, QObject *parent = NULL);
	~IntOrEmptyValidator();

public: // Methods ------------------------------------------------------------
	virtual State	validate(QString &input, int &pos) const;
};
//=============================================================================

//=============================================================================
class ProfileNameValidator : public QRegExpValidator
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	bool	m_isNewProfile;

public: // Constructor/destructor ---------------------------------------------
	ProfileNameValidator(QObject *parent = NULL, bool isNewProfile = false);
	~ProfileNameValidator();

public: // Methods ------------------------------------------------------------
	virtual State	validate(QString &input, int &pos) const;
};
//=============================================================================

//=============================================================================
class SaveFileValidator : public QValidator
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	SaveFileValidator(QObject *parent = NULL);
	~SaveFileValidator();

public: // Methods ------------------------------------------------------------
	virtual void	fixup(QString &input) const;
	virtual State	validate(QString &input, int &pos) const;
};
//=============================================================================

//=============================================================================
class OpenFileValidator : public QValidator
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	OpenFileValidator(QObject *parent = NULL);
	~OpenFileValidator();

public: // Methods ------------------------------------------------------------
	virtual void	fixup(QString &input) const;
	virtual State	validate(QString &input, int &pos) const;
};
//=============================================================================

//=============================================================================
class RTMPURLValidator : public QValidator
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	RTMPURLValidator(QObject *parent = NULL);
	~RTMPURLValidator();

public: // Methods ------------------------------------------------------------
	virtual void	fixup(QString &input) const;
	virtual State	validate(QString &input, int &pos) const;
};
//=============================================================================

#endif // VALIDATORS_H
