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

#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "common.h"
#include <QtGui/QColor>

//=============================================================================
class AppSettings
{
private: // Members -----------------------------------------------------------
	bool			m_dirty;
	QString			m_filename;
	bool			m_fileExists;
	QVector<QColor>	m_customColors;

	// Settings
	quint64			m_clientId;
	QByteArray		m_mainWinGeom;
	bool			m_mainWinGeomMaxed;
	QString			m_activeProfile;

public: // Constructor/destructor ---------------------------------------------
	AppSettings(const QString &filename);
	~AppSettings();

public: // Methods ------------------------------------------------------------
	QString		getFilename() const;
	bool		doesFileExist() const;
	void		loadDefaults();
	void		loadFromDisk();
	bool		saveToDisk();

private:
	void		serialize(QDataStream *stream) const;
	bool		unserialize(QDataStream *stream);

	void		generateClientId();
	void		setupColorDialog();
	void		saveColorDialog();

public: // Accessors ----------------------------------------------------------
	quint64		getClientId() const;

	QByteArray	getMainWinGeom() const;
	void		setMainWinGeom(const QByteArray &geom);

	bool		getMainWinGeomMaxed() const;
	void		setMainWinGeomMaxed(bool maximized);

	QString		getActiveProfile() const;
	void		setActiveProfile(const QString &activeProfile);
};
//=============================================================================

inline QString AppSettings::getFilename() const
{
	return m_filename;
}

inline bool AppSettings::doesFileExist() const
{
	return m_fileExists;
}

inline quint64 AppSettings::getClientId() const
{
	return m_clientId;
}

inline QByteArray AppSettings::getMainWinGeom() const
{
	return m_mainWinGeom;
}

inline bool AppSettings::getMainWinGeomMaxed() const
{
	return m_mainWinGeomMaxed;
}

inline QString AppSettings::getActiveProfile() const
{
	return m_activeProfile;
}

#endif // APPSETTINGS_H
