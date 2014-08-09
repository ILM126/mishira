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

#ifndef SCRIPTTEXTLAYERDIALOG_H
#define SCRIPTTEXTLAYERDIALOG_H

#include "layerdialog.h"
#include "ui_scripttextlayerdialog.h"

class ScriptTextLayer;

//=============================================================================
class ScriptTextLayerDialog : public LayerDialog
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_ScriptTextLayerDialog	m_ui;
	bool						m_ignoreSignals;

public: // Constructor/destructor ---------------------------------------------
	ScriptTextLayerDialog(ScriptTextLayer *layer, QWidget *parent = NULL);
	~ScriptTextLayerDialog();

public: // Interface ----------------------------------------------------------
	virtual void	loadSettings();
	virtual void	applySettings();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			strokeSizeEditChanged(const QString &text);
	void			xScrollEditChanged(const QString &text);
	void			yScrollEditChanged(const QString &text);
	void			resetEditorScrollPos();
};
//=============================================================================

#endif // SCRIPTTEXTLAYERDIALOG_H
