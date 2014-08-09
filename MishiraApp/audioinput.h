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

#ifndef AUDIOINPUT_H
#define AUDIOINPUT_H

#include "audiosegment.h"
#include "audioutils.h"
#include "common.h"
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

class AudioInputInfoWidget;
class AudioMixer;
class AudioSource;

//=============================================================================
/// <summary>
/// This class handles the buffering and filtering/DSP of audio data from an
/// audio source. It will automatically activate and deactivate itself when the
/// source it is following is added and removed. An audio input will only fill
/// its buffer if it is active and it is up to the "downstream" receiver
/// (E.g. mixer) to handle what happens when an audio input is deactivated.
/// </summary>
class AudioInput : public QObject
{
	Q_OBJECT

protected: // Members ---------------------------------------------------------
	bool				m_isInitializing;
	AudioMixer *		m_mixer;
	bool				m_isActive; // Is the input receiving samples?
	PrflAudioMode		m_mode;
	AudioSource *		m_source;
	float				m_volume; // Cached from attenuation
	QByteArray			m_buffer;
	quint64				m_firstTimestamp; // Usec since frame 0 for the first sample in the buffer

	// Statistics about the latest input data
	bool				m_isCalcInputStats;
	AudioStats			m_inStats;
	AudioStats			m_outStats;

	// Serialized data
	QString				m_name;
	quint64				m_sourceId;
	int					m_attenuation; // mB
	bool				m_isMuted;
	qint64				m_delayUsec; // Manual delay amount in usec

public: // Constructor/destructor ---------------------------------------------
	AudioInput(AudioMixer *mixer, quint64 sourceId, PrflAudioMode mode);
	virtual ~AudioInput();

public: // Methods ------------------------------------------------------------
	void			setInitialized();
	bool			isInitializing() const;
	AudioMixer *	getMixer() const;
	void			setCalcInputStats(bool calcInput);
	bool			getCalcInputStats() const;
	void			setName(const QString &name);
	QString			getName() const;
	AudioSource *	getSource() const;
	quint64			getSourceId() const;
	bool			isDefaultOfType() const;
	int				getAttenuation() const;
	float			getVolume() const;
	void			setMuted(bool muted);
	bool			isMuted() const;
	void			setDelay(qint64 delayUsec);
	qint64			getDelay() const;
	QString			getIdString(bool showName = true) const;

	void			moveTo(int before = -1);

	void			serialize(QDataStream *stream) const;
	bool			unserialize(QDataStream *stream);

	// Methods intended to be used from the "downstream" receiver
	bool			isActive() const;
	float			getInputRmsVolume() const;
	float			getInputPeakVolume() const;
	float			getOutputRmsVolume() const;
	float			getOutputPeakVolume() const;
	const float *	getBuffer() const;
	int				getBufferSize() const;
	qint64			getBufferTimestamp() const;
	void			reduceBufferBy(int numReadFloats);

	void			setupInputInfo(AudioInputInfoWidget *widget) const;

private:
	void			initializedEvent();

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void			setAttenuation(int attenuationMb);
	void			toggleMuted();
	void			sourceAdded(AudioSource *source);
	void			removingSource(AudioSource *source);
	void			segmentReady(AudioSegment segment);
};
//=============================================================================

inline bool AudioInput::isInitializing() const
{
	return m_isInitializing;
}

inline AudioMixer *AudioInput::getMixer() const
{
	return m_mixer;
}

inline bool AudioInput::getCalcInputStats() const
{
	return m_isCalcInputStats;
}

inline QString AudioInput::getName() const
{
	return m_name;
}

/// <summary>
/// Returns the audio source if one exists. WARNING: Can return NULL!
/// </summary>
inline AudioSource *AudioInput::getSource() const
{
	return m_source;
}

inline quint64 AudioInput::getSourceId() const
{
	return m_sourceId;
}

inline int AudioInput::getAttenuation() const
{
	return m_attenuation;
}

inline float AudioInput::getVolume() const
{
	return m_volume;
}

inline bool AudioInput::isMuted() const
{
	return m_isMuted;
}

inline qint64 AudioInput::getDelay() const
{
	return m_delayUsec;
}

inline bool AudioInput::isActive() const
{
	return m_isActive;
}

inline float AudioInput::getInputRmsVolume() const
{
	return m_inStats.getRmsVolume();
}

inline float AudioInput::getInputPeakVolume() const
{
	return m_inStats.getPeakVolume();
}

inline float AudioInput::getOutputRmsVolume() const
{
	return m_outStats.getRmsVolume();
}

inline float AudioInput::getOutputPeakVolume() const
{
	return m_outStats.getPeakVolume();
}

inline const float *AudioInput::getBuffer() const
{
	return reinterpret_cast<const float *>(m_buffer.constData());
}

inline int AudioInput::getBufferSize() const
{
	return m_buffer.size() / sizeof(float);
}

#endif // AUDIOINPUT_H
