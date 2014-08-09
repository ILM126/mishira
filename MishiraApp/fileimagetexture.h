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

#ifndef FILEIMAGETEXTURE_H
#define FILEIMAGETEXTURE_H

#include <QtCore/QByteArray>
#include <QtCore/QBuffer>
#include <QtCore/QString>
#include <QtGui/QImageReader>

class Texture;

//=============================================================================
/// <summary>
/// A helper class for managing textures that are loaded directly out of a file
/// on the filesystem. Handles filesystem IO, animations and memory management.
/// This object should only be created when a graphics context already exists
/// and should be deleted before the graphics context is invalidated.
/// </summary>
class FileImageTexture : public QObject
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	QString			m_filename;
	bool			m_animationPaused;

	// Image data
	QByteArray		m_imgData;
	int				m_imgLoadOperation;
	QBuffer			m_imgBuffer;
	QImageReader	m_imgReader;
	Texture *		m_tex;

	// Image state
	bool			m_hasAlpha;
	bool			m_hasAnimation;
	int				m_loopCount;
	int				m_usecToNextFrame;
	bool			m_isFirstFrameAfterLoad;

public: // Constructor/destructor ---------------------------------------------
	FileImageTexture(const QString &filename);
	~FileImageTexture();

public: // Methods ------------------------------------------------------------
	QString	getFilename() const;
	void	setAnimationPaused(bool paused);
	bool	isAnimationPaused() const;
	bool	resetAnimation(bool fullReset = true, bool updateTexture = true);

	bool		isLoaded() const;
	bool		hasTransparency() const;
	bool		hasAnimation() const;
	Texture *	getTexture() const;

	bool	processFrameEvent(uint frameNum, int numDropped);

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void	imgLoadComplete(
		int id, int errorCode, const QByteArray &data);
};
//=============================================================================

inline QString FileImageTexture::getFilename() const
{
	return m_filename;
}

inline bool FileImageTexture::isAnimationPaused() const
{
	return m_animationPaused;
}

inline bool FileImageTexture::isLoaded() const
{
	return m_tex != NULL;
}

inline bool FileImageTexture::hasTransparency() const
{
	return m_hasAlpha;
}

inline bool FileImageTexture::hasAnimation() const
{
	return m_hasAnimation;
}

inline Texture *FileImageTexture::getTexture() const
{
	return m_tex;
}

#endif // FILEIMAGETEXTURE_H
