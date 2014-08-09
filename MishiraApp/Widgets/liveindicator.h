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

#ifndef LIVEINDICATOR_H
#define LIVEINDICATOR_H

#include "stylehelper.h"
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

//=============================================================================
class LiveIndicator : public QWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	StyleHelper::ColorSet	m_liveColorSet;
	StyleHelper::ColorSet	m_offlineColorSet;
	QColor					m_textColor;
	QColor					m_textShadowColor;
	bool					m_isLive;
	QVBoxLayout				m_layout;
	QLabel					m_label;
	QSize					m_sizeHint;

public: // Constructor/destructor ---------------------------------------------
	LiveIndicator(QWidget *parent = 0);
	~LiveIndicator();

public: // Methods ------------------------------------------------------------
	virtual QSize	minimumSizeHint() const;
	virtual QSize	sizeHint() const;
	void			setLiveColorSet(const StyleHelper::ColorSet &colorSet);
	void			setOfflineColorSet(const StyleHelper::ColorSet &colorSet);
	void			setLightText(bool lightText);
	void			setIsLive(bool live);

protected:
	void			updateLightText(bool lightText);
	void			updateChildLabel(QLabel *label);

private: // Events ------------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

#endif // LIVEINDICATOR_H
