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

#ifndef TEXTLAYERDIALOG_H
#define TEXTLAYERDIALOG_H

#include "layerdialog.h"
#include "ui_textlayerdialog.h"

class TextLayer;

//=============================================================================
class TextLayerDialog : public LayerDialog
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	Ui_TextLayerDialog	m_ui;
	bool				m_ignoreSignals;

public: // Constructor/destructor ---------------------------------------------
	TextLayerDialog(TextLayer *layer, QWidget *parent = NULL);
	~TextLayerDialog();

private: // Methods -----------------------------------------------------------
	QString			prepareForEdit(const QString &input);
	QString			prepareForDisplay(const QString &input);
	bool			ensureSelectionNotEmpty(QTextCursor &cursor);

public: // Interface ----------------------------------------------------------
	virtual void	loadSettings();
	virtual void	applySettings();

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void			charFormatChanged(const QTextCharFormat &format);

	void			fontChanged(const QFont &font);
	void			fontSizeChanged(int i);
	void			boldToggled(bool checked);
	void			italicToggled(bool checked);
	void			underlineToggled(bool checked);
	void			leftAlignToggled(bool checked);
	void			centerAlignToggled(bool checked);
	void			rightAlignToggled(bool checked);
	void			justifyAlignToggled(bool checked);
	void			fontColorChanged(const QColor &color);
	void			bgColorChanged(const QColor &color);

	void			strokeSizeEditChanged(const QString &text);
	void			xScrollEditChanged(const QString &text);
	void			yScrollEditChanged(const QString &text);
};
//=============================================================================

#endif // TEXTLAYERDIALOG_H
