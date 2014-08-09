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

#ifndef BITRATECALCWIDGET_H
#define BITRATECALCWIDGET_H

#include "ui_bitratecalcwidget.h"
#include <QtWidgets/QButtonGroup>

//=============================================================================
class BitrateCalcWidget : public QWidget
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_BitrateCalcWidget	m_ui;
	QButtonGroup			m_btnGroup;
	float					m_unitMultiplier;

	int						m_recVideoBitrate;
	int						m_recAudioBitrate;
	QSize					m_recVideoSize;

public: // Constructor/destructor ---------------------------------------------
	BitrateCalcWidget(QWidget *parent = NULL);
	~BitrateCalcWidget();

public: // Methods ------------------------------------------------------------
	int		getRecommendedVideoBitrate() const;
	int		getRecommendedAudioBitrate() const;
	QSize	getRecommendedVideoSize() const;

private:
	void	recalculate();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	uploadEditChanged(const QString &text);
	void	unitsComboChanged(int index);
	void	advancedBoxChanged(int state);
	void 	actionRadioClicked(QAbstractButton *button);
};
//=============================================================================

inline int BitrateCalcWidget::getRecommendedVideoBitrate() const
{
	return m_recVideoBitrate;
}

inline int BitrateCalcWidget::getRecommendedAudioBitrate() const
{
	return m_recAudioBitrate;
}

inline QSize BitrateCalcWidget::getRecommendedVideoSize() const
{
	return m_recVideoSize;
}

#endif // BITRATECALCWIDGET_H
