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

#include "validators.h"
#include "application.h"
#include "profile.h"
#include <Libbroadcast/rtmptargetinfo.h>
#include <QtCore/QFileInfo>

//-----------------------------------------------------------------------------
// EvenIntValidator class

EvenIntValidator::EvenIntValidator(QObject *parent)
	: QIntValidator(parent)
{
}

EvenIntValidator::EvenIntValidator(int minimum, int maximum, QObject *parent)
	: QIntValidator(minimum, maximum, parent)
{
}

EvenIntValidator::~EvenIntValidator()
{
}

void EvenIntValidator::fixup(QString &input) const
{
	QIntValidator::fixup(input);
	//int pos = 0;
	//if(validate(input, pos) == Invalid)
	//	return;
}

QValidator::State EvenIntValidator::validate(QString &input, int &pos) const
{
	State state = QIntValidator::validate(input, pos);
	if(state != Acceptable)
		return state;
	int num = locale().toInt(input); // As it's `Acceptable` we know it's valid
	if(num % 2)
		return Intermediate; // It's odd
	return Acceptable; // It's even
}

//-----------------------------------------------------------------------------
// IntOrEmptyValidator class

IntOrEmptyValidator::IntOrEmptyValidator(QObject *parent)
	: QIntValidator(parent)
{
}

IntOrEmptyValidator::IntOrEmptyValidator(
	int minimum, int maximum, QObject *parent)
	: QIntValidator(minimum, maximum, parent)
{
}

IntOrEmptyValidator::~IntOrEmptyValidator()
{
}

QValidator::State IntOrEmptyValidator::validate(QString &input, int &pos) const
{
	if(input.isEmpty())
		return Acceptable;
	return QIntValidator::validate(input, pos);
}

//-----------------------------------------------------------------------------
// ProfileNameValidator class

ProfileNameValidator::ProfileNameValidator(QObject *parent, bool isNewProfile)
	: QRegExpValidator(
	QRegExp(QStringLiteral("[^\\\\/:*?\"<>|]{1,24}")), parent)
	, m_isNewProfile(isNewProfile)
{
}

ProfileNameValidator::~ProfileNameValidator()
{
}

QValidator::State ProfileNameValidator::validate(
	QString &input, int &pos) const
{
	if(input.isEmpty())
		return Intermediate;
	bool exists = Profile::doesProfileExist(input.trimmed());
	if(m_isNewProfile) {
		// Don't allow a name when creating a new profile if it already exists
		if(exists)
			return Intermediate;
	} else {
		// HACK: We assume we only ever edit the active profile
		if(exists && input.trimmed() != App->getProfile()->getName())
			return Intermediate;
	}
	return QRegExpValidator::validate(input, pos);
}

//-----------------------------------------------------------------------------
// SaveFileValidator class

SaveFileValidator::SaveFileValidator(QObject *parent)
	: QValidator(parent)
{
}

SaveFileValidator::~SaveFileValidator()
{
}

void SaveFileValidator::fixup(QString &input) const
{
	// Convert to absolute
	//QFileInfo info(input);
	//input = info.absoluteFilePath();
}

// TODO: If the text begins with two forward or back slashes then the program
// freezes for a few seconds.
QValidator::State SaveFileValidator::validate(QString &input, int &pos) const
{
	if(input.isEmpty())
		return Intermediate;
	QFileInfo info(input);

	// The user input must be an absolute path
	if(!info.isAbsolute())
		return Intermediate;

	// Prevent "C:example" from being accepted as a valid path
	if(input.length() >= 2 && input.at(1) == ':') {
		if(input.length() >= 3) {
			// "C:example"
			if(input.at(2) != '\\' && input.at(2) != '/')
				return Intermediate;
		} else
			return Intermediate; // "C:" only
	}

	// We must not be trying to write to a directory directly
	if(info.isDir())
		return Intermediate;

	// If the file doesn't already exists then use the directory for the
	// following tests
	if(!info.exists())
		info = QFileInfo(info.absolutePath());

	// We must be able to write to the file or directory
	if(!info.isWritable())
		return Intermediate;

	return Acceptable;
}

//-----------------------------------------------------------------------------
// OpenFileValidator class

OpenFileValidator::OpenFileValidator(QObject *parent)
	: QValidator(parent)
{
}

OpenFileValidator::~OpenFileValidator()
{
}

void OpenFileValidator::fixup(QString &input) const
{
	// Convert to absolute
	//QFileInfo info(input);
	//input = info.absoluteFilePath();
}

QValidator::State OpenFileValidator::validate(QString &input, int &pos) const
{
	if(input.isEmpty())
		return Intermediate;
	QFileInfo info(input);

	// The user input must be an absolute path
	if(!info.isAbsolute())
		return Intermediate;

	// Prevent "C:example" from being accepted as a valid path
	if(input.length() >= 2 && input.at(1) == ':') {
		if(input.length() >= 3) {
			// "C:example"
			if(input.at(2) != '\\' && input.at(2) != '/')
				return Intermediate;
		} else
			return Intermediate; // "C:" only
	}

	// We must not be trying to read from a directory directly
	if(info.isDir())
		return Intermediate;

	// We must be able to read from the file
	if(!info.isReadable())
		return Intermediate;

	return Acceptable;
}

//-----------------------------------------------------------------------------
// RTMPURLValidator class

RTMPURLValidator::RTMPURLValidator(QObject *parent)
	: QValidator(parent)
{
}

RTMPURLValidator::~RTMPURLValidator()
{
}

void RTMPURLValidator::fixup(QString &input) const
{
}

QValidator::State RTMPURLValidator::validate(QString &input, int &pos) const
{
	if(input.isEmpty())
		return Intermediate;
	RTMPTargetInfo info = RTMPTargetInfo::fromUrl(input, false);
	if(!info.isValid())
		return Intermediate;
	return Acceptable;
}
