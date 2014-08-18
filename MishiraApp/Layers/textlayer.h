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

#ifndef TEXTLAYER_H
#define TEXTLAYER_H

#include "layer.h"
#include <Libvidgfx/libvidgfx.h>
#include <QtGui/QColor>
#include <QtGui/QTextDocument>

class LayerDialog;

//=============================================================================
class TextLayer : public Layer
{
	friend class LayerGroup;
	friend class ScriptTextLayer;

private: // Members -----------------------------------------------------------
	VidgfxTexDecalBuf *	m_vertBuf;
	VidgfxTex *			m_texture;
	bool				m_isTexDirty;
	QTextDocument		m_document;
	int					m_strokeSize;
	QColor				m_strokeColor;
	bool				m_wordWrap;
	QColor				m_dialogBgColor;
	QPoint				m_scrollSpeed; // Pixels per second

private: // Constructor/destructor --------------------------------------------
	TextLayer(LayerGroup *parent);
	~TextLayer();

public: // Methods ------------------------------------------------------------
	void			setDocumentHtml(const QString &html);
	QString			getDocumentHtml();
	void			setStrokeSize(int size);
	int				getStrokeSize() const;
	void			setStrokeColor(const QColor &color);
	QColor			getStrokeColor() const;
	void			setWordWrap(bool wordWrap);
	bool			getWordWrap() const;
	void			setDialogBgColor(const QColor &color);
	QColor			getDialogBgColor() const;
	void			setScrollSpeed(const QPoint &speed);
	QPoint			getScrollSpeed() const;

	QFont			getDefaultFont() const;
	void			setDefaultFontSettings(QTextDocument *document);

private:
	void			recreateTexture(VidgfxContext *gfx);

public: // Interface ----------------------------------------------------------
	virtual void	initializeResources(VidgfxContext *gfx);
	virtual void	updateResources(VidgfxContext *gfx);
	virtual void	destroyResources(VidgfxContext *gfx);
	virtual void	render(
		VidgfxContext *gfx, Scene *scene, uint frameNum, int numDropped);

	virtual LyrType	getType() const;

	virtual bool			hasSettingsDialog();
	virtual LayerDialog *	createSettingsDialog(QWidget *parent = NULL);

	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			queuedFrameEvent(uint frameNum, int numDropped);
};
//=============================================================================

inline QString TextLayer::getDocumentHtml()
{
	return m_document.toHtml();
}

inline int TextLayer::getStrokeSize() const
{
	return m_strokeSize;
}

inline QColor TextLayer::getStrokeColor() const
{
	return m_strokeColor;
}

inline bool TextLayer::getWordWrap() const
{
	return m_wordWrap;
}

inline void TextLayer::setDialogBgColor(const QColor &color)
{
	m_dialogBgColor = color;
}

inline QColor TextLayer::getDialogBgColor() const
{
	return m_dialogBgColor;
}

inline QPoint TextLayer::getScrollSpeed() const
{
	return m_scrollSpeed;
}

#endif // TEXTLAYER_H
