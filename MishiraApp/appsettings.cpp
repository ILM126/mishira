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

#include "appsettings.h"
#include <QtCore/QDataStream>
#include <QtCore/QFile>
#include <QtWidgets/QColorDialog>

AppSettings::AppSettings(const QString &filename)
	: m_dirty(false)
	, m_filename(filename)
	, m_fileExists(false)
	, m_customColors()

	// Settings
	, m_clientId(0)
	, m_mainWinGeom()
	, m_mainWinGeomMaxed(false)
	, m_activeProfile()
{
	loadFromDisk();
	setupColorDialog();
}

AppSettings::~AppSettings()
{
	saveColorDialog(); // We only save the dialog on exit
	saveToDisk();
}

/// <summary>
/// Generates a storable representation of the current settings.
/// </summary>
void AppSettings::serialize(QDataStream *stream) const
{
	// Write header and file version
	*stream << (quint32)0xFB6634A8;
	*stream << (quint32)3; // Version

	// Write settings
	*stream << m_clientId;
	*stream << m_mainWinGeom;
	*stream << m_mainWinGeomMaxed;
	*stream << m_activeProfile;
	*stream << m_customColors;
}

/// <summary>
/// Initializes settings from a stored representation. All settings are
/// validated as they are read and any undefined values are set to their
/// default values.
/// </summary>
bool AppSettings::unserialize(QDataStream *stream)
{
	bool		boolData;
	quint32		uint32Data;
	QByteArray	byteArrayData;
	QString		stringData;

	// Initialize everything to the default settings now just in case there are
	// errors
	loadDefaults();

	// Read header
	*stream >> uint32Data;
	if(uint32Data != 0xFB6634A8) {
		appLog(Log::Warning) << "Invalid application settings file header";
		return false;
	}

	// Read file version
	quint32 version;
	*stream >> version;
	if(version >= 0 && version <= 3) {
		// Read our data
		if(version >= 2) {
			*stream >> m_clientId;
			if(m_clientId == 0)
				generateClientId();
		}
		*stream >> byteArrayData;
		setMainWinGeom(byteArrayData);
		*stream >> boolData;
		setMainWinGeomMaxed(boolData);
		*stream >> stringData;
		setActiveProfile(stringData);
		if(version >= 1 && version <= 2)
			*stream >> uint32Data; // Unused
		*stream >> m_customColors;
	} else {
		appLog(Log::Warning)
			<< "Unknown application settings file version, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

/// <summary>
/// Generates a random client ID that can be used to uniquely identify this
/// installation.
/// </summary>
void AppSettings::generateClientId()
{
	do {
		m_clientId = qrand64();
	} while(m_clientId == 0); // ID of 0 is reserved for error checking
}

void AppSettings::setupColorDialog()
{
	//-------------------------------------------------------------------------
	// Setup standard colours. Colours are listed in an 8x6 grid ordered top to
	// bottom first then left to right second.

	int i = 0;

	// Greyscale
	QColorDialog::setStandardColor(i=0, QColor(0, 0, 0));
	QColorDialog::setStandardColor(i+=6, QColor(255, 255, 255));
	QColorDialog::setStandardColor(i=1, QColor(128, 128, 128));
	QColorDialog::setStandardColor(i+=6, QColor(192, 192, 192));

	// Dark standard colours
	QColorDialog::setStandardColor(i=12, QColor(128, 0, 0));
	QColorDialog::setStandardColor(i+=6, QColor(0, 128, 0));
	QColorDialog::setStandardColor(i+=6, QColor(128, 128, 0));
	QColorDialog::setStandardColor(i+=6, QColor(0, 0, 128));
	QColorDialog::setStandardColor(i+=6, QColor(128, 0, 128));
	QColorDialog::setStandardColor(i+=6, QColor(0, 128, 128));

	// Bright standard colours
	QColorDialog::setStandardColor(i=13, QColor(255, 0, 0));
	QColorDialog::setStandardColor(i+=6, QColor(0, 255, 0));
	QColorDialog::setStandardColor(i+=6, QColor(255, 255, 0));
	QColorDialog::setStandardColor(i+=6, QColor(0, 0, 255));
	QColorDialog::setStandardColor(i+=6, QColor(255, 0, 255));
	QColorDialog::setStandardColor(i+=6, QColor(0, 255, 255));

	// Hues
	for(i = 0; i < 8; i++) {
		for(int j = 0; j < 4; j++) {
			const float div = 256.0f / 5.0f;
			QColorDialog::setStandardColor(
				(i * 6) + j + 2,
				QColor::fromHsl(i * 45, 255, j * div + div));
		}
	}

	//-------------------------------------------------------------------------
	// Set custom colour list from previous application execution

	int numColors = qMin(QColorDialog::customCount(), m_customColors.count());
	for(i = 0; i < numColors; i++)
		QColorDialog::setCustomColor(i, m_customColors.at(i));
}

void AppSettings::saveColorDialog()
{
	// Get custom colour list from the dialog and prepare it for saving to file
	m_customColors.clear();
	int numColors = QColorDialog::customCount();
	for(int i = 0; i < numColors; i++)
		m_customColors.append(QColorDialog::customColor(i));
	m_dirty = true; // Because we don't know if any colours have changed
}

void AppSettings::loadDefaults()
{
	generateClientId();
	m_mainWinGeom = QByteArray();
	m_mainWinGeomMaxed = false;
	m_activeProfile = QStringLiteral("Default");

	m_dirty = false;
}

void AppSettings::loadFromDisk()
{
	QFile file(m_filename);
	if(!file.exists()) {
		// File doesn't exist, load defaults instead
		m_fileExists = false;
		loadDefaults();
		return;
	}
	m_fileExists = true;

	file.open(QIODevice::ReadOnly);
	QDataStream stream(&file);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
	stream.setVersion(12);
	bool res = unserialize(&stream);
	if(stream.status() != QDataStream::Ok) {
		appLog(Log::Warning)
			<< "An error occurred while reading the application settings file";
	}
	file.close();

	if(!res) {
		appLog(Log::Warning)
			<< "Cannot load application settings file, using defaults";
		// TODO: Error dialog?
		loadDefaults();
		return;
	}
	m_dirty = false;
}

/// <summary>
/// Saves the current settings to disk.
/// </summary>
/// <returns>True if the settings were successfully saved</returns>
bool AppSettings::saveToDisk()
{
	if(!m_dirty)
		return true; // Nothing to do

	QFile file(m_filename);
	if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	QDataStream stream(&file);
	stream.setByteOrder(QDataStream::LittleEndian);
	stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
	stream.setVersion(12);
	serialize(&stream);
	if(stream.status() != QDataStream::Ok) {
		appLog(Log::Warning)
			<< "An error occurred while writing to the application settings "
			<< "file";
	}
	file.close();

	m_fileExists = true;
	m_dirty = false;
	return true;
}

//-----------------------------------------------------------------------------
// Settings

void AppSettings::setMainWinGeom(const QByteArray &geom)
{
	if(m_mainWinGeom == geom)
		return; // No change
	// We cannot validate this setting
	m_mainWinGeom = geom;
	m_dirty = true;
}

void AppSettings::setMainWinGeomMaxed(bool maximized)
{
	if(m_mainWinGeomMaxed == maximized)
		return; // No change
	// Booleans are always valid
	m_mainWinGeomMaxed = maximized;
	m_dirty = true;
}

void AppSettings::setActiveProfile(const QString &activeProfile)
{
	if(m_activeProfile == activeProfile)
		return; // No change
	// Strings are always valid
	m_activeProfile = activeProfile;
	m_dirty = true;
}
