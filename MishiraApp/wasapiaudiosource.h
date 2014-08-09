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

#ifndef WASAPIAUDIOSOURCE_H
#define WASAPIAUDIOSOURCE_H

#include "audiosource.h"
#include <Mmdeviceapi.h>
//#include <QtCore/QMutex>

class AudioSourceManager;
class Resampler;
interface IAudioClient;
interface IAudioCaptureClient;
interface IAudioRenderClient;

// Declare our signal datatypes with Qt
Q_DECLARE_METATYPE(AsrcType);

//=============================================================================
/// <summary>
/// WARNING: The operating system calls these callbacks in a new thread! Any
/// signals emitted by these callbacks will be buffered!
/// </summary>
class MMNotifier : public QObject, public IMMNotificationClient
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	LONG	m_ref;

public: // Constructor/destructor ---------------------------------------------
	MMNotifier();
	~MMNotifier();

public: // Interface ----------------------------------------------------------

	// IUnknown methods
	virtual ULONG STDMETHODCALLTYPE		AddRef();
	virtual ULONG STDMETHODCALLTYPE		Release();
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface(
		REFIID riid, VOID **ppvInterface);

	// IMMNotificationClient methods
	virtual HRESULT STDMETHODCALLTYPE	OnDefaultDeviceChanged(
		EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
	virtual HRESULT STDMETHODCALLTYPE	OnDeviceAdded(LPCWSTR pwstrDeviceId);
	virtual HRESULT STDMETHODCALLTYPE	OnDeviceRemoved(LPCWSTR pwstrDeviceId);
	virtual HRESULT STDMETHODCALLTYPE	OnDeviceStateChanged(
		LPCWSTR pwstrDeviceId, DWORD dwNewState);
	virtual HRESULT STDMETHODCALLTYPE	OnPropertyValueChanged(
		LPCWSTR pwstrDeviceId, const PROPERTYKEY key);

Q_SIGNALS: // Signals ---------------------------------------------------------
	void	defaultDeviceChanged(AsrcType type, const QString &deviceIdStr);
	void	deviceAdded(const QString &deviceIdStr);
	void	deviceRemoved(const QString &deviceIdStr);
	void	deviceStateChanged(const QString &deviceIdStr, uint dwNewState);
};
//=============================================================================

//=============================================================================
/// <summary>
/// Processes the MMNotifier callbacks in the main thread.
/// </summary>
class MMNotifierManager : public QObject
{
	Q_OBJECT

private: // Members -----------------------------------------------------------
	IMMDeviceEnumerator *	m_deviceEnum;

public: // Constructor/destructor ---------------------------------------------
	MMNotifierManager(IMMDeviceEnumerator *deviceEnum);
	virtual ~MMNotifierManager();

private: // Methods -----------------------------------------------------------
	IMMDevice *	getDeviceFromIdString(LPCWSTR pwstrDeviceId) const;
	quint64		getIdOfDefaultDevice(AsrcType type) const;
	EDataFlow	getDataFlowOfDevice(IMMDevice *device) const;
	bool		reconstructSource(
		quint64 sourceId, AsrcType type, quint64 idOfDefault) const;

	private
Q_SLOTS: // Slots -------------------------------------------------------------
	void		defaultDeviceChanged(
		AsrcType type, const QString &deviceIdStr);
	void		deviceAdded(const QString &deviceIdStr);
	void		deviceRemoved(const QString &deviceIdStr);
	void		deviceStateChanged(
		const QString &deviceIdStr, uint dwNewState);
};
//=============================================================================

//=============================================================================
class WASAPIAudioSource : public AudioSource
{
	friend class MMNotifierManager;

	Q_OBJECT

private: // Members -----------------------------------------------------------
	IMMDevice *				m_device;
	IAudioClient *			m_client; // For capture
	IAudioClient *			m_client2; // For render
	IAudioCaptureClient *	m_captureClient;
	IAudioRenderClient *	m_renderClient;
	int						m_renderBufNumFrames;
	Resampler *				m_resampler;
	quint64					m_id;
	QString					m_friendlyName;
	bool					m_isConnected;
	AudioMixer *			m_mixer;
	int						m_frameSize;
	int						m_inSampleRate;
	quint64					m_firstSampleQPC100ns; // Timestamp of the first sample
	quint64					m_numFramesProcessed;
	int						m_ref;
	//QMutex					m_refMutex;

public: // Static methods -----------------------------------------------------
	static void		populateSources(AudioSourceManager *mgr);
	static void		constructAllOfType(
		AudioSourceManager *mgr, IMMDeviceEnumerator *deviceEnum,
		bool isOutput);
	static quint64	getIdFromIdString(wchar_t *wIdStr);
	static quint64	getIdFromIMMDevice(IMMDevice *device);

protected: // Constructor/destructor ------------------------------------------
	WASAPIAudioSource(IMMDevice *device, AsrcType type);
	bool construct(quint64 idOfDefault);
public:
	virtual ~WASAPIAudioSource();

private: // Methods -----------------------------------------------------------
	bool				initialize();
	bool				initializeCapture();
	bool				initializeRender();
	void				shutdown();

	IMMDevice *			getDevice() const;

public: // Interface ----------------------------------------------------------
	virtual quint64		getId() const;
	virtual quint64		getRealId() const;
	virtual QString		getFriendlyName() const;
	virtual QString		getDebugString() const;
	virtual bool		isConnected() const;
	virtual bool		reference(AudioMixer *mixer);
	virtual void		dereference();
	virtual int			getRefCount();
	virtual void		process(int numTicks);
};
//=============================================================================

inline IMMDevice *WASAPIAudioSource::getDevice() const
{
	return m_device;
}

#endif // AUDIOSOURCE_H
