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

#define _USE_MATH_DEFINES

#include "audiomixer.h"
#include "audioinput.h"
#include "application.h"
#include "profile.h"
#include <QtCore/QBuffer>

const QString LOG_CAT = QStringLiteral("Audio");

//=============================================================================
// Helpers

/// <summary>
/// Loops `num` within the range [0..`max`) (0.0 to just below `max`).
/// Negative numbers are not inverted, i.e. "-0.1"  becomes "`max` - 0.1".
/// </summary>
double dblRepeat(double num, double max)
{
	double tmp;
	tmp = num - (double)((int)(num / max)) * max;
	if(tmp < 0.0)
		return max + tmp;
	return tmp;
}

/// <summary>
/// Fills the specified stereo buffer with an identical sine wave on both
/// channels.
/// </summary>
/// <returns>The phase of the sample after the last one in the buffer</returns>
double fillWithSineStereo(
	float *data, int numSamples, int sampleRate, float freq,
	double phase = 0.0)
{
	if(data == NULL)
		return phase;
	double delta = (2.0 * M_PI * (double)freq) / (double)sampleRate;
	for(int i = 0; i < numSamples * 2; i += 2) {
		data[i] = (float)sin(phase);// * (float)sin(phase / 32.0);
		data[i+1] = data[i];// * 0.5f;
		phase += delta;
	}
	return dblRepeat(phase, 2.0 * M_PI);
}

//=============================================================================
// AudioMixer class

AudioMixer::AudioMixer(Profile *profile)
	: QObject()
	, m_profile(profile)
	, m_inputs()
	, m_metronomeEnabledRef(0)

	// Master state
	, m_outStats(getSampleRate(), getNumChannels())
	, m_masterVolume(1.0f)
	, m_masterAttenuation(0)

	// Synchronisation
	, m_syncedInputs()
	, m_refTimestampUsec(App->getUsecSinceFrameOrigin())
	, m_minInputDelayUsec(0)
	, m_sampleNum(0)
{
	// We process audio at the same frequency as the video framerate as it
	// allows us to do more bulk processing
	connect(App, &Application::realTimeFrameEvent,
		this, &AudioMixer::realTimeFrameEvent);

	// Watch profile for audio mode changes
	connect(profile, &Profile::audioModeChanged,
		this, &AudioMixer::audioModeChanged);
}

AudioMixer::~AudioMixer()
{
	if(m_metronomeEnabledRef > 0) {
		appLog(LOG_CAT, Log::Warning)
			<< QStringLiteral("Audio mixer destroyed while metronome still refernced");
	}

	// Delete all child inputs cleanly
	while(!m_inputs.isEmpty())
		destroyInput(m_inputs.first());
}

/// <summary>
/// Determines the most negative audio input delay that the mixer needs to
/// handle. We need this value in order to delay out output and is guarenteed
/// to always be negative or zero.
/// </summary>
void AudioMixer::calcMinInputDelay()
{
	m_minInputDelayUsec = 0;
	for(int i = 0; i < m_inputs.count(); i++) {
		qint64 delay = m_inputs.at(i)->getDelay();
		if(delay < m_minInputDelayUsec)
			m_minInputDelayUsec = delay;
	}
}

AudioInput *AudioMixer::createInput(
	quint64 sourceId, const QString &name, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_inputs.count() + 1;
	}
	before = qBound(0, before, m_inputs.count());

	AudioInput *input =
		new AudioInput(this, sourceId, m_profile->getAudioMode());
	if(!name.isEmpty())
		input->setName(name);
	m_inputs.insert(before, input);
	appLog(LOG_CAT) << "Created audio input " << input->getIdString();
	input->setInitialized();
	calcMinInputDelay();

	emit inputAdded(input, before);
	return input;
}

AudioInput *AudioMixer::createInputSerialized(
	QDataStream *stream, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_inputs.count() + 1;
	}
	before = qBound(0, before, m_inputs.count());

	AudioInput *input = new AudioInput(this, 0, m_profile->getAudioMode());
	appLog(LOG_CAT)
		<< "Unserializing audio input " << input->getIdString(false)
		<< "...";
	if(!input->unserialize(stream)) {
		// Failed to unserialize data
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to fully unserialize audio input data";
		delete input;
		return NULL;
	}
	m_inputs.insert(before, input);
	appLog(LOG_CAT) << "Created audio input " << input->getIdString();
	input->setInitialized();
	calcMinInputDelay();

	emit inputAdded(input, before);
	return input;
}

void AudioMixer::destroyInput(AudioInput *input)
{
	int id = m_inputs.indexOf(input);
	if(id == -1)
		return; // Doesn't exist
	emit destroyingInput(input); // Do first so we can freeze the UI

	QString idString = input->getIdString();
	m_inputs.remove(id);
	delete input;
	appLog(LOG_CAT) << "Deleted audio input " << idString;

	// Remove from synchonised list if it exists
	id = m_syncedInputs.indexOf(input);
	if(id >= 0)
		m_syncedInputs.remove(id);
	calcMinInputDelay();
}

AudioInput *AudioMixer::inputAtIndex(int index) const
{
	if(index < 0 || index >= m_inputs.count())
		return NULL;
	return m_inputs.at(index);
}

int AudioMixer::indexOfInput(AudioInput *input) const
{
	return m_inputs.indexOf(input);
}

void AudioMixer::moveInputTo(AudioInput *input, int before)
{
	if(before < 0) {
		// Position is relative to the right
		before += m_inputs.count() + 1;
	}
	before = qBound(0, before, m_inputs.count());

	// We need to be careful as the `before` index includes ourself
	int index = indexOfInput(input);
	if(before == index)
		return; // Moving to the same spot
	if(before > index)
		before--; // Adjust to not include ourself
	m_inputs.remove(index);
	m_inputs.insert(before, input);

	// Emit move signal
	emit inputMoved(input, before);
}

int AudioMixer::inputCount() const
{
	return m_inputs.count();
}

/// <summary>
/// Completely resets the mixer. This must be called whenever an audio input
/// or the output format changes in a way that cannot be done on-the-fly.
/// WARNING: This causes an output discontinuity which means that it cannot be
/// called while broadcasting!
/// </summary>
void AudioMixer::resetMixer()
{
	// Serialize to a temporary buffer
	QByteArray data;
	QBuffer buffer(&data);
	buffer.open(QIODevice::ReadWrite);
	{
		QDataStream stream(&buffer);
		stream.setByteOrder(QDataStream::LittleEndian);
		stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
		stream.setVersion(12);
		serialize(&stream);
		if(stream.status() != QDataStream::Ok) {
			// TODO
		}
	}

	// Completely reset mixer
	while(!m_inputs.isEmpty())
		destroyInput(m_inputs.first());
	m_outStats = AudioStats(getSampleRate(), getNumChannels());
	m_refTimestampUsec = App->getUsecSinceFrameOrigin();
	m_minInputDelayUsec = 0;
	m_sampleNum = 0;

	// Restore previous state
	buffer.seek(0);
	{
		QDataStream stream(&buffer);
		stream.setByteOrder(QDataStream::LittleEndian);
		stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
		stream.setVersion(12);
		unserialize(&stream);
		if(stream.status() != QDataStream::Ok) {
			// TODO
		}
	}
}

void AudioMixer::refMetronome()
{
	m_metronomeEnabledRef++;
}

void AudioMixer::derefMetronome()
{
	if(m_metronomeEnabledRef <= 0)
		return; // Already disabled
	m_metronomeEnabledRef--;
}

void AudioMixer::setMasterAttenuation(int attenuationMb)
{
	if(m_masterAttenuation == attenuationMb)
		return; // Nothing to do
	m_masterAttenuation = attenuationMb;
	m_masterVolume = mbToLinear((float)m_masterAttenuation);
	emit inputChanged(NULL); // HACK: NULL input = master output
}

/// <summary>
/// Convenience method
/// </summary>
inline PrflAudioMode AudioMixer::getAudioMode() const
{
	return m_profile->getAudioMode();
}

/// <summary>
/// Convenience method
/// </summary>
int AudioMixer::getSampleRate() const
{
	switch(getAudioMode()) {
	default:
	case Prfl441StereoMode:
		return 44100;
	case Prfl48StereoMode:
		return 48000;
	}
	return 44100; // Never reached
}

/// <summary>
/// Convenience method
/// </summary>
int AudioMixer::getNumChannels() const
{
	switch(getAudioMode()) {
	default:
	case Prfl441StereoMode:
		return 2;
	case Prfl48StereoMode:
		return 2;
	}
	return 2; // Never reached
}

void AudioMixer::serialize(QDataStream *stream) const
{
	// Write data version number
	*stream << (quint32)0;

	// Save our data
	*stream << (qint32)m_masterAttenuation;
	*stream << (quint32)m_inputs.count();
	for(int i = 0; i < m_inputs.count(); i++) {
		AudioInput *input = m_inputs.at(i);
		input->serialize(stream);
	}
}

bool AudioMixer::unserialize(QDataStream *stream)
{
	qint32	int32Data;
	quint32	uint32Data;

	// Read data version number
	quint32 version;
	*stream >> version;
	if(version == 0) {
		// Read our data
		*stream >> int32Data;
		setMasterAttenuation(int32Data);
		*stream >> uint32Data;
		int count = uint32Data;
		for(int i = 0; i < count; i++) {
			if(createInputSerialized(stream) == NULL) {
				// The reason has already been logged
				return false;
			}
		}
	} else {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown version number in audio mixer serialized data, "
			<< "cannot load settings";
		return false;
	}

	return true;
}

/// <summary>
/// A debug method that prints the entire mixer structure to the log file.
/// </summary>
void AudioMixer::logInputs() const
{
	if(m_inputs.count() == 0) {
		appLog() << "--: No audio inputs defined";
		return;
	}
	for(int i = 0; i < m_inputs.count(); i++) {
		AudioInput *input = m_inputs.at(i);
		appLog()
			<< QStringLiteral("%1: ").arg(i, 2) << input->getIdString();
	}
}

void AudioMixer::realTimeFrameEvent(int numDropped, int lateByUsec)
{
	// Process audio inputs no more than once per video frame. We want to
	// always process the audio even if the user isn't broadcasting as we need
	// to update the mixer UI to let the user know that audio is being
	// received.
	//
	// We rely on frame events being processed after system tick events to make
	// sure that our audio inputs already have data available.
	//
	// Output audio segments must be in sequential order and contiguous.
	//
	// As audio inputs might have different amounts of lag we need to
	// synchronise them before mixing. Once we emit a single segment we must
	// always make sure that sequent segments are always contiguous. We must
	// also make sure that we are outputting data even if there is no active
	// inputs.

	// If no time has passed since the frame origin then the main loop is still
	// resetting its frequency from a profile change. As the code in this
	// method doesn't properly handle this case and as no time has passed we
	// just return immediately.
	if(App->getUsecSinceFrameOrigin() == 0ULL)
		return;

	// Determine the timestamp of our next output segment accurately
	int sampleRate = getSampleRate();
	int numChannels = getNumChannels();
	qint64 outTimestamp =
		m_refTimestampUsec + (m_sampleNum * 1000000ULL) / sampleRate;
	outTimestamp += m_minInputDelayUsec;

	// Trim any samples from unsynchronised inputs that are before our current
	// output timestamp and detect when they have synchronised
	for(int i = 0; i < m_inputs.count(); i++) {
		AudioInput *input = m_inputs.at(i);
		if(m_syncedInputs.contains(input))
			continue; // Input is synchronised
		if(!input->isActive()) {
			// If an input is inactive and not synchronised then we should
			// wipe its buffer if it has one as we can't reliably do anything
			// with it.
			input->reduceBufferBy(input->getBufferSize());
			continue;
		}
		if(input->getBufferSize() <= 0) {
			// If the buffer is empty then its timestamp is undefined and can
			// cause the code below to crash.
			continue;
		}
		qint64 ts = input->getBufferTimestamp();
		if(ts >= outTimestamp) {
			// Input is sometime in the future, we'll synchronise at a later
			// time. We could potentially speed up synchronisation by detecting
			// it when we are actually mixing but that is more complex and
			// prone to more bugs. Right now we'll lose some samples at the
			// beginning but that is okay.
			continue;
		}

		// Calculate number of samples that are before our current time
		// rounding up. Using 64-bit maths as audio sources get their
		// timestamps from the operating system and they are not 100% reliable.
		quint64 usecDelta = (quint64)(outTimestamp - ts);
		quint64 sampleDelta =
			((quint64)sampleRate * usecDelta + 999999ULL) / 1000000ULL;

		// Do the actual trim
		input->reduceBufferBy(sampleDelta * numChannels);

		// If we still have samples left then we are synchronised
		if(input->getBufferSize() > 0)
			m_syncedInputs.append(input);
	}

	// Determine the maximum amount of samples we can output right now. This is
	// equal to the least number of samples that any synchronised input has or,
	// if we don't have any synchronised inputs, the amount of samples needed
	// to bring the output up to the real-time "now". If an input is
	// deactivated then it means everything after the end of its buffer is
	// silence and we are allowed to go past its end.
	int numOutSamples = INT_MAX;
	for(int i = 0; i < m_syncedInputs.count(); i++) {
		AudioInput *input = m_syncedInputs.at(i);
		int numSamples = input->getBufferSize() / numChannels;
		//appLog() << ">> Num samples this input ready: " << numSamples;
		if(numSamples < numOutSamples && input->isActive())
			numOutSamples = numSamples;
	}
	if(numOutSamples == 0)
		return; // We are waiting for new data
	if(numOutSamples == INT_MAX) {
		qint64 nowUsec = App->getUsecSinceFrameOrigin() + m_minInputDelayUsec;
		qint64 usecDelta =
			nowUsec > outTimestamp ? nowUsec - outTimestamp : 0;
		numOutSamples = ((quint64)sampleRate * usecDelta) / 1000000ULL;
		if(numOutSamples == 0) {
			// Our audio output is in the future which can only happen if the
			// audio sources are emitting timestamps incorrectly.
#ifdef QT_DEBUG
			appLog(LOG_CAT, Log::Critical)
				<< "We've recorded audio from the future. "
				<< "This shouldn't happen";
#endif
		}
		//appLog() << ">> Num samples to now: " << numOutSamples;
	}
	//appLog() << "Outputting " << numOutSamples << " samples this iteration";

	// Do the actual mixing and detect when we lose synchronisation of
	// deactivated inputs.
	QByteArray outBuf;
	int numFloats = numOutSamples * numChannels;
	outBuf.fill(0, numFloats * sizeof(float));
	float *buf = reinterpret_cast<float *>(outBuf.data());
	AudioInputList desyncedInputs;
	for(int i = 0; i < m_syncedInputs.count(); i++) {
		AudioInput *input = m_syncedInputs.at(i);
		const float *inBuf = input->getBuffer();
		int inNumFloats = qMin(numFloats, input->getBufferSize());

		// SSE is most likely not worth it as we are probably memory bandwidth
		// limited and our buffers are not 16-byte aligned
		for(int i = 0; i < inNumFloats; i++)
			buf[i] += inBuf[i];

		input->reduceBufferBy(inNumFloats);
		if(inNumFloats < numFloats) {
			// We've lost synchronisation of this input as it's been
			// deactivated. Remove it from our list when it's safe to do so.
			Q_ASSERT(!input->isActive());
			desyncedInputs.append(input);
		}
	}
	for(int i = 0; i < desyncedInputs.count(); i++) {
		int index = m_syncedInputs.indexOf(desyncedInputs.at(i));
		m_syncedInputs.remove(index);
	}

	// TODO: Why do we have separate input and output buffers below if they
	// both point to the same address? I forgot why I did it that way...

	if(isMetronomeEnabled()) {
		// Adjust the metronome's sample number to take into account delay and
		// a non-zero origin. This is because it assumes that sample number 0
		// has a timestamp of 0 as well.
		qint64 sampleNum = (qint64)m_sampleNum +
			(((qint64)m_refTimestampUsec + m_minInputDelayUsec) *
			(qint64)sampleRate) / 1000000LL;
		if(sampleNum >= 0) {
			applyMetronomeFilter(
				buf, buf, numFloats, (quint64)sampleNum, getSampleRate(),
				getNumChannels());
		}
	}

	// Apply master volume
	applyAttenuationFilter(buf, buf, numFloats, m_masterVolume);

	// Clip output to the range [-1.0, 1.0]. Probably not required but it's
	// better to be safe.
	applyClipFilter(buf, buf, numFloats);

	// Calculate RMS and peak volume levels
	m_outStats.calcStats(buf, numFloats);

	// Create and emit the audio segment and update our state. As audio
	// segments cannot support negative timestamps we just eat the segments.
	if(outTimestamp >= 0) {
		if(outTimestamp == 0)
			outTimestamp = 1; // 0 is special
		emit segmentReady(AudioSegment((quint64)outTimestamp, outBuf));
	}
	m_sampleNum += numOutSamples;

#define OUTPUT_TEST_SIGNAL 0
#if OUTPUT_TEST_SIGNAL
	//-------------------------------------------------------------------------
	// Output test signal. As we're generating samples from the past the
	// timestamp should also be in the past.
	int sampleRate = getSampleRate();
	int frameRate = ceilf(App->getProfile()->getVideoFramerate().asFloat());
	int samplesPerFrame = ((sampleRate + frameRate - 1) / frameRate) * 2;
	int numSamples = samplesPerFrame * (numDropped + 1);
	qint64 timestamp = (qint64)App->getUsecSinceFrameOrigin();
	//timestamp -= 1500000LL; // For lagged video/audio testing only
	timestamp -= ((qint64)(samplesPerFrame * numDropped) * 1000000LL) /
		(qint64)sampleRate;
	if(timestamp >= 0) {
		float *data = new float[numSamples];
		static double phase = 0.0;
		phase = fillWithSineStereo(
			data, numSamples / 2, sampleRate, 1000.0f, phase);
		emit segmentReady(AudioSegment(timestamp, data, numSamples));
		delete[] data;
	}
#endif // OUTPUT_TEST_SIGNAL
}

void AudioMixer::audioModeChanged(PrflAudioMode mode)
{
	// As we are constantly mixing even when we are not broadcasting the only
	// safe way to change audio modes is to completely reset the mixer. This
	// means we need to remove all inputs, reset the buffers and add all the
	// inputs back again. WARNING: This causes an output discontinuity!
	resetMixer();
}
