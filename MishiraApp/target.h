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

#ifndef TARGET_H
#define TARGET_H

#include "common.h"
#include <QtCore/QObject>

class Profile;
class TargetInfoWidget;
class AudioEncoder;
class TargetPane;
class VideoEncoder;

//=============================================================================
class Target : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	bool		m_isInitializing;
	Profile *	m_profile;
	TrgtType	m_type;
	bool		m_isEnabled;
	bool		m_isActive;
	quint64		m_timeActivated;
	quint64		m_prevTimeActive;
	QString		m_name;

protected: // Constructor/destructor ------------------------------------------
	Target(Profile *profile, TrgtType type, const QString &name);
public:
	virtual ~Target();

public: // Methods ------------------------------------------------------------
	void			setInitialized();
	bool			isInitializing() const;
	Profile *		getProfile() const;
	TrgtType		getType() const;
	void			setEnabled(bool enabled);
	bool			isEnabled() const;
	void			setActive(bool active);
	bool			isActive() const;
	quint64			getTimeActive() const;
	QString			getTimeActiveAsString() const;
	void			setName(const QString &name);
	QString			getName() const;
	QString			getIdString(bool showName = true) const;

private: // Interface ---------------------------------------------------------
	virtual void	initializedEvent() = 0;
	virtual bool	activateEvent() = 0;
	virtual void	deactivateEvent() = 0;

public:
	virtual VideoEncoder *	getVideoEncoder() const = 0;
	virtual AudioEncoder *	getAudioEncoder() const = 0;

	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

	virtual void	setupTargetPane(TargetPane *pane) = 0;
	virtual void	resetTargetPaneEnabled() = 0;
	virtual void	setupTargetInfo(TargetInfoWidget *widget) = 0;

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			activeChanged(Target *target, bool active);
	void			enabledChanged(Target *target, bool enabled);
};
//=============================================================================

inline bool Target::isInitializing() const
{
	return m_isInitializing;
}

inline Profile *Target::getProfile() const
{
	return m_profile;
}

inline TrgtType Target::getType() const
{
	return m_type;
}

inline bool Target::isEnabled() const
{
	return m_isEnabled;
}

inline bool Target::isActive() const
{
	return m_isActive;
}

inline QString Target::getName() const
{
	return m_name;
}

#endif // TARGET_H
