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

#ifndef D3DWIDGET_H
#define D3DWIDGET_H

#include <Libvidgfx/libvidgfx.h>
#include <QtWidgets/QWidget>

class GraphicsWidget;

//=============================================================================
class D3DWidget : public QWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	GraphicsWidget *	m_parent;
	int					m_timesToPaint;
	bool				m_initialized;
	VidgfxD3DContext *	m_context;
	QSize				m_curSize;

public: // Constructor/destructor ---------------------------------------------
	D3DWidget(GraphicsWidget *parent = 0);
	~D3DWidget();

public: // Methods ------------------------------------------------------------
	virtual QPaintEngine *	paintEngine() const;

protected: // Events ----------------------------------------------------------
	virtual void	showEvent(QShowEvent *ev);
	virtual void	resizeEvent(QResizeEvent *ev);
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

#endif // D3DWIDGET_H
