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

#ifndef VOLUMESLIDER_H
#define VOLUMESLIDER_H

#include "stylehelper.h"
#include <QtWidgets/QAbstractSlider>

//=============================================================================
class VolumeSlider : public QAbstractSlider
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	int						m_handleWidth;
	int						m_handlePos;
	bool					m_handlePressed;
	int						m_handleClickedPos;
	int						m_gutterWidth;
	bool					m_isMuted;
	int						m_attenuation;
	float					m_headroomMb; // mB
	float					m_rmsVolume; // mB
	float					m_peakVolume; // mB

public: // Constructor/destructor ---------------------------------------------
	VolumeSlider(QWidget *parent = 0);
	~VolumeSlider();

public: // Methods ------------------------------------------------------------
	virtual QSize	sizeHint() const;
	virtual QSize	minimumSizeHint() const;

	void			setVolume(float rmsLinear, float peakLinear);
	void			setIsMuted(bool isMuted);
	void			setAttenuation(int attenuationMb);

private:
	int				mbToPos(float mb) const;
	float			posToMb(int pos) const;

	QRect			calcBarRect() const;
	void			drawMarks(QPainter &p, int dx, int dy);
	void			drawHandle(QPainter &p, int dx, int dy);

protected: // Events ----------------------------------------------------------
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	mouseMoveEvent(QMouseEvent *ev);
	virtual void	wheelEvent(QWheelEvent *ev);
	virtual void	paintEvent(QPaintEvent *ev);
	virtual void	resizeEvent(QResizeEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			attenuationChanged(int attenuationMb);
};
//=============================================================================

#endif // VOLUMESLIDER_H
