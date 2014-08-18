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

#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QtWidgets/QLabel>

//=============================================================================
/// <summary>
/// A QLabel variation that can automatically elide single-line text.
/// </summary>
class ElidedLabel : public QLabel
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Qt::TextElideMode	m_elideMode;
	QString				m_elidedText;

public: // Constructor/destructor ---------------------------------------------
	ElidedLabel(QWidget *parent = NULL, Qt::WindowFlags f = 0);
	ElidedLabel(
		const QString &txt, QWidget *parent = NULL, Qt::WindowFlags f = 0);
	ElidedLabel(
		const QString &txt, Qt::TextElideMode elideMode = Qt::ElideRight,
		QWidget *parent = NULL, Qt::WindowFlags f = 0);
	~ElidedLabel();

public: // Methods ------------------------------------------------------------
	void				setElideMode(Qt::TextElideMode elideMode);
	Qt::TextElideMode	getElideMode() const;

	void				setText(const QString &txt); // QLabel override

private:
	void				calcElidedText(int width);

private: // Events ------------------------------------------------------------
	virtual void		paintEvent(QPaintEvent *ev);
	virtual void		resizeEvent(QResizeEvent *ev);
};
//=============================================================================

inline void ElidedLabel::setElideMode(Qt::TextElideMode elideMode)
{
	m_elideMode = elideMode;
	updateGeometry();
}

inline Qt::TextElideMode ElidedLabel::getElideMode() const
{
	return m_elideMode;
}

#endif // ELIDEDLABEL_H
