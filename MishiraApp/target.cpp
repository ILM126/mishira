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

#include "target.h"
#include "application.h"
#include "profile.h"

const QString LOG_CAT = QStringLiteral("Target");

Target::Target(Profile *profile, TrgtType type, const QString &name)
	: QObject()
	, m_isInitializing(true)
	, m_profile(profile)
	, m_type(type)
	, m_isEnabled(false)
	, m_isActive(false)
	, m_timeActivated(0)
	, m_prevTimeActive(0)
	, m_name(name)
{
}

Target::~Target()
{
}

void Target::setInitialized()
{
	if(!m_isInitializing)
		return; // Already initialized
	m_isInitializing = false;
	initializedEvent();
	if(m_isActive) {
		if(activateEvent())
			m_timeActivated = App->getUsecSinceExec();
		else {
			// Failed to activate target, TODO: Notify user?
			appLog(LOG_CAT, Log::Warning) <<
				QStringLiteral("Failed to activate target %1")
				.arg(getIdString());
			m_isActive = false;
			return;
		}
	}
}

void Target::setEnabled(bool enable)
{
	if(m_isEnabled == enable)
		return; // Nothing to do

	if(enable) {
		m_isEnabled = true;
	} else {
		if(m_isActive)
			setActive(false);
		m_isEnabled = false;
	}

	emit enabledChanged(this, m_isEnabled);
}

void Target::setActive(bool active)
{
	if(m_isActive == active)
		return; // Nothing to do

	if(active) {
		if(!m_isEnabled)
			setEnabled(true);
		m_isActive = true;
		if(!m_isInitializing) {
			if(activateEvent())
				m_timeActivated = App->getUsecSinceExec();
			else {
				// Failed to activate target, TODO: Notify user?
				appLog(LOG_CAT, Log::Warning) <<
					QStringLiteral("Failed to activate target %1")
					.arg(getIdString());
				m_isActive = false;
				return;
			}
		}
	} else {
		m_prevTimeActive = getTimeActive();
		m_timeActivated = 0;
		m_isActive = false;
		if(!m_isInitializing)
			deactivateEvent();
	}

	emit activeChanged(this, m_isActive);
}

/// <summary>
/// Returns the amount of time, in microseconds, that the target has been
/// active for.
/// </summary>
quint64 Target::getTimeActive() const
{
	if(m_isActive)
		return App->getUsecSinceExec() - m_timeActivated;
	return m_prevTimeActive;
}

/// <summary>
/// Returns the amount of time that the target has been active for as a string
/// in the format "H:MM:SS".
/// </summary>
QString Target::getTimeActiveAsString() const
{
	quint64 secActive = getTimeActive() / 1000000ULL;
	int hours = secActive / 60 / 60;
	int mins = secActive / 60 - hours * 60;
	int secs = secActive - hours * 60 * 60 - mins * 60;
	return tr("%L1:%L2:%L3")
		.arg(hours).arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
}

void Target::setName(const QString &name)
{
	QString str = name;
	if(str.isEmpty())
		str = tr("Unnamed");
	if(m_name == str)
		return;
	QString oldName = m_name;
	m_name = str;
	m_profile->targetChanged(this); // Remote emit
	if(!m_isInitializing)
		appLog(LOG_CAT) << "Renamed target " << getIdString();
}

QString Target::getIdString(bool showName) const
{
	if(showName) {
		return QStringLiteral("%1 (\"%2\")")
			.arg(pointerToString((void *)this))
			.arg(getName());
	}
	return pointerToString((void *)this);
}

void Target::serialize(QDataStream *stream) const
{
	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << m_name;
	*stream << m_isEnabled;
}

bool Target::unserialize(QDataStream *stream)
{
	// Make sure that the target hasn't been initialized yet
	if(!m_isInitializing) {
		appLog(LOG_CAT, Log::Warning)
			<< "Cannot unserialize a target when it has already been "
			<< "initialized";
		return false;
	}

	// Load defaults here if ever needed

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		*stream >> m_name;
		*stream >> m_isEnabled;
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in target serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}
