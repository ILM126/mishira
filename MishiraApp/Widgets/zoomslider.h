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

#ifndef ZOOMSLIDER_H
#define ZOOMSLIDER_H

#include "stylehelper.h"
#include <QtWidgets/QAbstractSlider>

//=============================================================================
/// <summary>
/// The zoom slider is special in that it is non-linear, is unbounded by the
/// size of the groove, has a special tick mark for 100% zoom (0 point) and has
/// icons in the left and right gutters.
/// </summary>
class ZoomSlider : public QAbstractSlider
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	int						m_tickWidth;
	int						m_minimumTick;
	int						m_maximumTick;
	int						m_handleWidth;
	int						m_handlePos;
	bool					m_handlePressed;
	int						m_handleClickedPos;
	int						m_gutterWidth;
	int						m_snapDistance;
	QPixmap					m_handlePixmap;

public: // Constructor/destructor ---------------------------------------------
	ZoomSlider(QWidget *parent = 0);
	~ZoomSlider();

public: // Methods ------------------------------------------------------------
	virtual QSize	sizeHint() const;
	virtual QSize	minimumSizeHint() const;

	void			setZoom(float zoom, bool byUser);
	float			getZoom() const;

private:
	int				zoomToPos(float zoom) const;
	float			posToZoom(int pos) const;

	void			updateToolTip(const QPoint &globalPos);

	void			drawMarksAndIcons(QPainter &p, int dx, int dy);

protected: // Events ----------------------------------------------------------
	virtual void	mousePressEvent(QMouseEvent *ev);
	virtual void	mouseReleaseEvent(QMouseEvent *ev);
	virtual void	mouseMoveEvent(QMouseEvent *ev);
	virtual void	wheelEvent(QWheelEvent *ev);
	virtual void	paintEvent(QPaintEvent *ev);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void			zoomChanged(float zoom, bool byUser);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			setZoom(float zoom);
};
//=============================================================================

#endif // ZOOMSLIDER_H
