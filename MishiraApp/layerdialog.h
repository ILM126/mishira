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

#ifndef LAYERDIALOG_H
#define LAYERDIALOG_H

#include <QtWidgets/QWidget>

class Layer;

//=============================================================================
class LayerDialog : public QWidget
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	Layer *	m_layer;
	bool	m_isModified;

protected: // Constructor/destructor ------------------------------------------
	LayerDialog(Layer *layer, QWidget *parent = NULL);
public:
	~LayerDialog();

protected: // Methods ---------------------------------------------------------
	void			setModified(bool modified = true);
	bool			isModified();

public: // Interface ----------------------------------------------------------
	virtual void	loadSettings();
	virtual void	applySettings();
	virtual void	cancelSettings();
	virtual void	resetSettings();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			settingModified();
};
//=============================================================================

inline bool LayerDialog::isModified()
{
	return m_isModified;
}

#endif // LAYERDIALOG_H
