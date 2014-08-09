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

#ifndef VIDEOSOURCE_H
#define VIDEOSOURCE_H

#include <QtCore/QObject>

class Texture;

//=============================================================================
/// <summary>
/// Provides an interface to pull the latest video frame from a specific video
/// source. The source must be referenced in order to receive frame data. This
/// class handles colourspace conversion to RGB.
/// </summary>
class VideoSource : public QObject
{
	Q_OBJECT

protected: // Constructor/destructor ------------------------------------------
	VideoSource();
public:
	virtual ~VideoSource();

public: // Interface ----------------------------------------------------------
	virtual quint64		getId() const = 0;
	virtual QString		getFriendlyName() const = 0;
	virtual QString		getDebugString() const = 0;
	virtual bool		reference() = 0;
	virtual void		dereference() = 0;
	virtual int			getRefCount() = 0;
	virtual void		prepareFrame(uint frameNum, int numDropped) = 0;
	virtual Texture *	getCurrentFrame() = 0;
	virtual bool		isFrameFlipped() const = 0;
	virtual QVector<float>	getFramerates() const = 0;
	virtual QVector<QSize>	getSizesForFramerate(float framerate) const = 0;
	virtual void		setMode(float framerate, const QSize &size) = 0;
	virtual float		getModeFramerate() = 0;
	virtual QSize		getModeSize() = 0;
	virtual void		showConfigureDialog(QWidget *parent = NULL) = 0;
};
//=============================================================================

#endif // VIDEOSOURCE_H
