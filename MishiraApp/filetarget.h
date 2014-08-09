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

#ifndef FILETARGET_H
#define FILETARGET_H

#include "encodedframe.h"
#include "encodedsegment.h"
#include "target.h"
#include <QtCore/QTimer>

struct AVFormatContext;
struct AVOutputFormat;
struct AVStream;
class AVSynchronizer; // Not a part of FFmpeg, it's our own class

//=============================================================================
class FileTarget : public Target
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	VideoEncoder *		m_videoEnc;
	AudioEncoder *		m_audioEnc;
	AVSynchronizer *	m_syncer;
	TargetPane *		m_pane;
	QTimer				m_paneTimer;
	quint64				m_cachedFilesize;
	quint64				m_timeOfPrevSizeCalc;
	QString				m_actualFilename;

	// FFmpeg
	AVOutputFormat *	m_outFormat;
	AVFormatContext *	m_context;
	AVStream *			m_videoStream;
	AVStream *			m_audioStream;
	bool				m_wroteHeader;
	QByteArray			m_extraVideoData;

	// Options
	quint32				m_videoEncId;
	quint32				m_audioEncId;
	QString				m_filename;
	FileTrgtType		m_fileType;

public: // Constructor/destructor ---------------------------------------------
	FileTarget(
		Profile *profile, const QString &name, const FileTrgtOptions &opt);
	virtual ~FileTarget();

public: // Methods ------------------------------------------------------------
	QString			getFilename() const;
	FileTrgtType	getFileType() const;

private:
	void			updatePaneText(bool fromTimer);

private: // Interface ---------------------------------------------------------
	virtual void	initializedEvent();
	virtual bool	activateEvent();
	virtual void	deactivateEvent();

public:
	virtual VideoEncoder *	getVideoEncoder() const;
	virtual AudioEncoder *	getAudioEncoder() const;

	virtual void	serialize(QDataStream *stream) const;
	virtual bool	unserialize(QDataStream *stream);

	virtual void	setupTargetPane(TargetPane *pane);
	virtual void	resetTargetPaneEnabled();
	virtual void	setupTargetInfo(TargetInfoWidget *widget);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			paneOutdated();
	void			paneTimeout();
	void			frameReady(EncodedFrame frame);
	void			segmentReady(EncodedSegment segment);
	void			videoEncodeError(const QString &error);

	private
Q_SLOTS:
	void			delayedSimpleDeactivate();
};
//=============================================================================

inline QString FileTarget::getFilename() const
{
	return m_filename;
}

inline FileTrgtType FileTarget::getFileType() const
{
	return m_fileType;
}

#endif // FILETARGET_H
