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

#ifndef AUDIOMIXER_H
#define AUDIOMIXER_H

#include "common.h"
#include "audiosegment.h"
#include "audioutils.h"
#include <QtCore/QObject>
#include <QtCore/QVector>

class AudioInput;
class Profile;

typedef QVector<AudioInput *> AudioInputList;

//=============================================================================
class AudioMixer : public QObject
{
	friend class AudioInput;

	Q_OBJECT

private: // Members -----------------------------------------------------------
	Profile *		m_profile;
	AudioInputList	m_inputs;
	int				m_metronomeEnabledRef;

	// Master state
	AudioStats		m_outStats;
	float			m_masterVolume; // Cached from attenuation
	int				m_masterAttenuation; // mB (Serialized)

	// Synchronisation
	AudioInputList	m_syncedInputs;
	quint64			m_refTimestampUsec; // Usec since frame 0
	qint64			m_minInputDelayUsec; // Lowest negative input delay
	quint64			m_sampleNum; // Number of samples since the reference

public: // Constructor/destructor ---------------------------------------------
	AudioMixer(Profile *profile);
	~AudioMixer();

public: // Methods ------------------------------------------------------------
	Profile *			getProfile() const;
	AudioInputList		getInputs() const;
	AudioInput *		createInput(
		quint64 sourceId, const QString &name = QString(), int before = -1);
	AudioInput *		createInputSerialized(
		QDataStream *stream, int before = -1);
	void				destroyInput(AudioInput *input);
	AudioInput *		inputAtIndex(int index) const;
	int					indexOfInput(AudioInput *input) const;
	void				moveInputTo(AudioInput *input, int before);
	int					inputCount() const;

	void				resetMixer();

	bool				isMetronomeEnabled() const;
	void				refMetronome();
	void				derefMetronome();

	int					getMasterAttenuation() const;
	float				getMasterVolume() const;

	PrflAudioMode		getAudioMode() const;
	int					getSampleRate() const;
	int					getNumChannels() const;
	float				getOutputRmsVolume() const;
	float				getOutputPeakVolume() const;

	void				serialize(QDataStream *stream) const;
	bool				unserialize(QDataStream *stream);

	void				logInputs() const;

private:
	void				calcMinInputDelay();
	AudioInputList &	getInputsMutable();

Q_SIGNALS: // Signals ---------------------------------------------------------
	void				inputChanged(AudioInput *input);
	void				destroyingInput(AudioInput *input);
	void				inputAdded(AudioInput *input, int before);
	void				inputMoved(AudioInput *input, int before);
	void				segmentReady(AudioSegment segment);

	public
Q_SLOTS: // Slots -------------------------------------------------------------
	void				setMasterAttenuation(int attenuationMb);
	void				realTimeFrameEvent(int numDropped, int lateByUsec);
	void				audioModeChanged(PrflAudioMode mode);
};
//=============================================================================

inline Profile *AudioMixer::getProfile() const
{
	return m_profile;
}

inline AudioInputList AudioMixer::getInputs() const
{
	return m_inputs;
}

inline AudioInputList &AudioMixer::getInputsMutable()
{
	return m_inputs;
}

inline bool AudioMixer::isMetronomeEnabled() const
{
	return m_metronomeEnabledRef > 0;
}

inline int AudioMixer::getMasterAttenuation() const
{
	return m_masterAttenuation;
}

inline float AudioMixer::getMasterVolume() const
{
	return m_masterVolume;
}

inline float AudioMixer::getOutputRmsVolume() const
{
	return m_outStats.getRmsVolume();
}

inline float AudioMixer::getOutputPeakVolume() const
{
	return m_outStats.getPeakVolume();
}

#endif // AUDIOMIXER_H
