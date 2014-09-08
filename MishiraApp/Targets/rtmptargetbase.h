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

#ifndef RTMPTARGETBASE_H
#define RTMPTARGETBASE_H

#include "encodedframe.h"
#include "encodedsegment.h"
#include "target.h"
#include <Libbroadcast/rtmptargetinfo.h>
#include <QtCore/QTimer>

class AVSynchronizer;
class RTMPClient;

//=============================================================================
class RTMPTargetBase : public Target
{
	Q_OBJECT

private: // Datatypes ---------------------------------------------------------
	struct QueuedData {
		bool			isFrame;
		bool			isDropped; // True = Don't transmit
		EncodedFrame	frame;
		int				bytesPadding;
		EncodedPacket	audioPkt;
		qint64			audioPts;
	};
	struct WriteStats { // Used for determining upload speed
		quint64	timestamp;
		int		bytesWritten;
	};
	struct DropStats { // Used for determining stream stability
		quint64	timestamp;
		bool	wasPadding;
		int		bytesDropped;
	};

	enum StreamStability {
		PerfectStability = 0, // No dropped data
		GoodStability, // Dropped padding only
		MarginalStability, // Below 3% frames dropped
		BadStability, // Below 7.5% frames dropped
		TerribleStability, // Greater than 7.5% frames dropped
		OfflineStability // Not even connected to the remote host
	};

protected: // Members ---------------------------------------------------------
	VideoEncoder *		m_videoEnc;
	AudioEncoder *		m_audioEnc;
	AVSynchronizer *	m_syncer;
	RTMPClient *		m_rtmp;
	RTMPPublisher *		m_publisher;
	bool				m_doPadVideo;
	QTimer				m_logTimer;
	QString				m_lastFatalError;

	// Logging
	int					m_prevDroppedPadding;
	int					m_prevDroppedFrames;

	// State
	QVector<QueuedData>	m_syncQueue; // Data waiting be to synced in sending order
	int					m_audioPktsInSyncQueue;
	QVector<QueuedData>	m_outQueue; // Data to transmit ASAP in sending order
	int					m_audioErrorUsec;
	bool				m_wroteHeaders;
	bool				m_deactivating;
	bool				m_isFatalDeactivate;
	bool				m_fatalErrorInProgress;
	qint64				m_videoDtsOrigin;
	QVector<WriteStats>	m_uploadStats;
	QVector<DropStats>	m_dropStats;
	int					m_numDroppedFrames;
	int					m_numDroppedPadding; // Bytes
	float				m_avgVideoFrameSize; // Used for padding. Bytes
	bool				m_conSucceededOnce; // First connection attempt worked
	bool				m_firstReconnectAttempt;
	bool				m_allowReconnect;

public: // Constructor/destructor ---------------------------------------------
	RTMPTargetBase(Profile *profile, TrgtType type, const QString &name);
	virtual ~RTMPTargetBase();

protected: // Methods ---------------------------------------------------------
	void	rtmpInit(quint32 videoEncId, quint32 audioEncId, bool padVideo);
	bool	rtmpActivate(const RTMPTargetInfo &info);
	void	rtmpDeactivate();

	int				rtmpGetUploadSpeed() const;
	StreamStability	rtmpGetStability() const;
	QString			rtmpGetStabilityAsString() const;
	int				rtmpGetNumDroppedFrames() const;
	int				rtmpGetNumDroppedPadding() const;
	int				rtmpGetBufferUsagePercent() const;
	QString			rtmpGetLastFatalError() const;
	QString			rtmpErrorToString(RTMPClient::RTMPError error) const;

	void			setupTargetPaneHelper(
		TargetPane *pane, const QString &pixmap);
	int				updatePaneTextHelper(
		TargetPane *pane, int offset, bool fromTimer);
	void			setPaneStateHelper(TargetPane *pane);
	int				setupTargetInfoHelper(
		TargetInfoWidget *widget, int offset, const QString &pixmap);

public:
	virtual VideoEncoder *	getVideoEncoder() const;
	virtual AudioEncoder *	getAudioEncoder() const;

private:
	void	fatalRtmpError(
		const QString &logMsg, const QString &usrMsg = QString());
	void	processSyncQueue();
	int		dropBestAudioFrame(int droppedVideoFrame);
	int		dropBestFrame(FrmPriority priority);
	void	processFrameDropping();
	bool	writeHeaders();
	int		writeH264Frame(EncodedFrame frame, int bytesPadding);
	int		writeAACFrame(EncodedPacket pkt, qint64 pts);
	void	writeToPublisher();
	EncodedPacketList	filterFramePackets(EncodedFrame frame) const;
	int		calcSizeOfPackets(const EncodedPacketList &pkts) const;
	int		calcMaximumOutputBufferSize() const;
	int		calcCurrentOutputBufferSize() const;
	quint32	calcVideoTimestampFromDts(qint64 dts);
	quint32	calcAudioTimestampFromPts(qint64 pts);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	rtmpDroppedPadding(int bytesDropped);
	void	rtmpDroppedFrames(int numDropped);
	void	rtmpStateChanged(bool wasError);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void	realTimeTickEvent(int numDropped, int lateByUsec);
	void	frameReady(EncodedFrame frame);
	void	segmentReady(EncodedSegment segment);
	void	videoEncodeError(const QString &error);
	void	logTimeout();

	void	rtmpConnected();
	void	rtmpInitialized();
	void	rtmpConnectedToApp();
	void	rtmpDisconnected();
	void	rtmpError(RTMPClient::RTMPError error);
	void	publisherReady();
	void	publisherDataRequest(uint numBytes);

	private
Q_SLOTS:
	void	delayedSimpleDeactivate();
	void	delayedDeactivate();
	void	delayedReconnect();
};
//=============================================================================

inline QString RTMPTargetBase::rtmpGetLastFatalError() const
{
	return m_lastFatalError;
}

#endif // RTMPTARGETBASE_H
