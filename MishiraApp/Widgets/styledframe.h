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

#ifndef STYLEDFRAME_H
#define STYLEDFRAME_H

#include <QtWidgets/QFrame>

//=============================================================================
class StyledFrame : public QFrame
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	StyledFrame(QWidget *parent = 0);
	~StyledFrame();

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

//=============================================================================
class StyledInfoFrame : public StyledFrame
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	StyledInfoFrame(QWidget *parent = 0);
	~StyledInfoFrame();

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

//=============================================================================
class StyledWarningFrame : public StyledFrame
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	StyledWarningFrame(QWidget *parent = 0);
	~StyledWarningFrame();

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

//=============================================================================
class StyledErrorFrame : public StyledFrame
{
	Q_OBJECT

public: // Constructor/destructor ---------------------------------------------
	StyledErrorFrame(QWidget *parent = 0);
	~StyledErrorFrame();

protected: // Events ----------------------------------------------------------
	virtual void	paintEvent(QPaintEvent *ev);
};
//=============================================================================

#endif // STYLEDFRAME_H
