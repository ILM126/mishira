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

#include "wasapiaudiosource.h"
#include "audiomixer.h"
#include "audiosourcemanager.h"
#include "resampler.h"
#include "winapplication.h"
#include <Audioclient.h>
#include <propsys.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <QtCore/QHash>

/// <summary>
/// The size of the audio input and output buffers in nanoseconds. The buffer
/// should be long enough so that if the application freezes due to high CPU
/// load we do not get a buffer overrun.
/// </summary>
const quint64 AUDIO_BUFFER_SIZE_100NS = 60000000LL; // 6 seconds

const QString LOG_CAT = QStringLiteral("WASAPI");

//=============================================================================
// Helpers

QString guidToString(const GUID &guid)
{
	char out[40];
	qsnprintf(out, 40, "{%08X-%04hX-%04hX-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
	return QString::fromLatin1(out);
}

QString sampleFormatToString(AVSampleFormat format)
{
	return QString::fromLatin1(av_get_sample_fmt_name(format));
}

QString channelLayoutToString(uint64_t layout)
{
	switch(layout) {
	case 0:
		return QStringLiteral("** No channels **");
	case AV_CH_LAYOUT_MONO:
		return QStringLiteral("Mono");
	case AV_CH_LAYOUT_STEREO:
		return QStringLiteral("Stereo");
	case AV_CH_LAYOUT_2POINT1:
		return QStringLiteral("2.1");
	case AV_CH_LAYOUT_2_1:
		return QStringLiteral("3.0 (Back)");
	case AV_CH_LAYOUT_SURROUND:
		return QStringLiteral("3.0");
	case AV_CH_LAYOUT_3POINT1:
		return QStringLiteral("3.1");
	case AV_CH_LAYOUT_4POINT0:
		return QStringLiteral("4.0");
	case AV_CH_LAYOUT_4POINT1:
		return QStringLiteral("4.1");
	case AV_CH_LAYOUT_2_2:
		return QStringLiteral("Quad (Side)");
	case AV_CH_LAYOUT_QUAD:
		return QStringLiteral("Quad");
	case AV_CH_LAYOUT_5POINT0:
		return QStringLiteral("5.0 (Side)");
	case AV_CH_LAYOUT_5POINT1:
		return QStringLiteral("5.1 (Side)");
	case AV_CH_LAYOUT_5POINT0_BACK:
		return QStringLiteral("5.0");
	case AV_CH_LAYOUT_5POINT1_BACK:
		return QStringLiteral("5.1");
	case AV_CH_LAYOUT_6POINT0:
		return QStringLiteral("6.0");
	case AV_CH_LAYOUT_6POINT0_FRONT:
		return QStringLiteral("6.0 (Front)");
	case AV_CH_LAYOUT_HEXAGONAL:
		return QStringLiteral("Hexagonal");
	case AV_CH_LAYOUT_6POINT1:
		return QStringLiteral("6.1");
	case AV_CH_LAYOUT_6POINT1_BACK:
		return QStringLiteral("6.1");
	case AV_CH_LAYOUT_6POINT1_FRONT:
		return QStringLiteral("6.1 (Front)");
	case AV_CH_LAYOUT_7POINT0:
		return QStringLiteral("7.0");
	case AV_CH_LAYOUT_7POINT0_FRONT:
		return QStringLiteral("7.0 (Front)");
	case AV_CH_LAYOUT_7POINT1:
		return QStringLiteral("7.1");
	case AV_CH_LAYOUT_7POINT1_WIDE:
		return QStringLiteral("7.1 (Wide-side)");
	case AV_CH_LAYOUT_7POINT1_WIDE_BACK:
		return QStringLiteral("7.1 (Wide)");
	case AV_CH_LAYOUT_OCTAGONAL:
		return QStringLiteral("Octagonal");
	case AV_CH_LAYOUT_STEREO_DOWNMIX:
		return QStringLiteral("Downmix");
	default:
		return numberToHexString(layout);
	}
}

QString getAudioErrorCode(HRESULT res)
{
	switch(res) {
	case AUDCLNT_E_NOT_INITIALIZED:
		return QStringLiteral("AUDCLNT_E_NOT_INITIALIZED");
	case AUDCLNT_E_ALREADY_INITIALIZED:
		return QStringLiteral("AUDCLNT_E_ALREADY_INITIALIZED");
	case AUDCLNT_E_WRONG_ENDPOINT_TYPE:
		return QStringLiteral("AUDCLNT_E_WRONG_ENDPOINT_TYPE");
	case AUDCLNT_E_DEVICE_INVALIDATED:
		return QStringLiteral("AUDCLNT_E_DEVICE_INVALIDATED");
	case AUDCLNT_E_NOT_STOPPED:
		return QStringLiteral("AUDCLNT_E_NOT_STOPPED");
	case AUDCLNT_E_BUFFER_TOO_LARGE:
		return QStringLiteral("AUDCLNT_E_BUFFER_TOO_LARGE");
	case AUDCLNT_E_OUT_OF_ORDER:
		return QStringLiteral("AUDCLNT_E_OUT_OF_ORDER");
	case AUDCLNT_E_UNSUPPORTED_FORMAT:
		return QStringLiteral("AUDCLNT_E_UNSUPPORTED_FORMAT");
	case AUDCLNT_E_INVALID_SIZE:
		return QStringLiteral("AUDCLNT_E_INVALID_SIZE");
	case AUDCLNT_E_DEVICE_IN_USE:
		return QStringLiteral("AUDCLNT_E_DEVICE_IN_USE");
	case AUDCLNT_E_BUFFER_OPERATION_PENDING:
		return QStringLiteral("AUDCLNT_E_BUFFER_OPERATION_PENDING");
	case AUDCLNT_E_THREAD_NOT_REGISTERED:
		return QStringLiteral("AUDCLNT_E_THREAD_NOT_REGISTERED");
	case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED:
		return QStringLiteral("AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED");
	case AUDCLNT_E_ENDPOINT_CREATE_FAILED:
		return QStringLiteral("AUDCLNT_E_ENDPOINT_CREATE_FAILED");
	case AUDCLNT_E_SERVICE_NOT_RUNNING:
		return QStringLiteral("AUDCLNT_E_SERVICE_NOT_RUNNING");
	case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED:
		return QStringLiteral("AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED");
	case AUDCLNT_E_EXCLUSIVE_MODE_ONLY:
		return QStringLiteral("AUDCLNT_E_EXCLUSIVE_MODE_ONLY");
	case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL:
		return QStringLiteral("AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL");
	case AUDCLNT_E_EVENTHANDLE_NOT_SET:
		return QStringLiteral("AUDCLNT_E_EVENTHANDLE_NOT_SET");
	case AUDCLNT_E_INCORRECT_BUFFER_SIZE:
		return QStringLiteral("AUDCLNT_E_INCORRECT_BUFFER_SIZE");
	case AUDCLNT_E_BUFFER_SIZE_ERROR:
		return QStringLiteral("AUDCLNT_E_BUFFER_SIZE_ERROR");
	case AUDCLNT_E_CPUUSAGE_EXCEEDED:
		return QStringLiteral("AUDCLNT_E_CPUUSAGE_EXCEEDED");
	case AUDCLNT_E_BUFFER_ERROR:
		return QStringLiteral("AUDCLNT_E_BUFFER_ERROR");
	case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED:
		return QStringLiteral("AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED");
	case AUDCLNT_E_INVALID_DEVICE_PERIOD:
		return QStringLiteral("AUDCLNT_E_INVALID_DEVICE_PERIOD");
	case AUDCLNT_E_INVALID_STREAM_FLAG:
		return QStringLiteral("AUDCLNT_E_INVALID_STREAM_FLAG");
	case AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE:
		return QStringLiteral("AUDCLNT_E_ENDPOINT_OFFLOAD_NOT_CAPABLE");
	case AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES:
		return QStringLiteral("AUDCLNT_E_OUT_OF_OFFLOAD_RESOURCES");
	case AUDCLNT_E_OFFLOAD_MODE_ONLY:
		return QStringLiteral("AUDCLNT_E_OFFLOAD_MODE_ONLY");
	case AUDCLNT_E_NONOFFLOAD_MODE_ONLY:
		return QStringLiteral("AUDCLNT_E_NONOFFLOAD_MODE_ONLY");
	case AUDCLNT_E_RESOURCES_INVALIDATED:
		return QStringLiteral("AUDCLNT_E_RESOURCES_INVALIDATED");
	case CO_E_NOTINITIALIZED:
		return QStringLiteral("CO_E_NOTINITIALIZED");
	case E_UNEXPECTED:
		return QStringLiteral("E_UNEXPECTED");
	case E_NOTIMPL:
		return QStringLiteral("E_NOTIMPL");
	case E_OUTOFMEMORY:
		return QStringLiteral("E_OUTOFMEMORY");
	case E_INVALIDARG:
		return QStringLiteral("E_INVALIDARG");
	case E_NOINTERFACE:
		return QStringLiteral("E_NOINTERFACE");
	case E_POINTER:
		return QStringLiteral("E_POINTER");
	case E_HANDLE:
		return QStringLiteral("E_HANDLE");
	case E_ABORT:
		return QStringLiteral("E_ABORT");
	case E_FAIL:
		return QStringLiteral("E_FAIL");
	case E_ACCESSDENIED:
		return QStringLiteral("E_ACCESSDENIED");
	case __HRESULT_FROM_WIN32(ERROR_NOT_FOUND):
		return QStringLiteral("E_NOTFOUND");
	case __HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE):
		return QStringLiteral("E_UNSUPPORTED_TYPE");
	case S_FALSE:
		return QStringLiteral("S_FALSE");
	case S_OK:
		return QStringLiteral("S_OK");
	default:
		return numberToHexString((uint)res);
	}
}

//=============================================================================
// MMNotifier class

MMNotifier::MMNotifier()
	: QObject()
	, m_ref(1)
{
}

MMNotifier::~MMNotifier()
{
}

ULONG MMNotifier::AddRef()
{
	return InterlockedIncrement(&m_ref);
}

ULONG MMNotifier::Release()
{
	ULONG ulRef = InterlockedDecrement(&m_ref);
	if(ulRef == 0)
		delete this;
	return ulRef;
}

HRESULT MMNotifier::QueryInterface(REFIID riid, VOID **ppvInterface)
{
	if(riid == IID_IUnknown) {
		AddRef();
		*ppvInterface = (IUnknown *)this;
		return S_OK;
	} else if (riid == __uuidof(IMMNotificationClient)) {
		AddRef();
		*ppvInterface = (IMMNotificationClient *)this;
		return S_OK;
	}
	*ppvInterface = NULL;
	return E_NOINTERFACE;
}

HRESULT MMNotifier::OnDefaultDeviceChanged(
	EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
{
	AsrcType type;
	if(flow == eRender && role == eConsole) {
		// Default output device has changed
		type = AsrcOutputType;
	} else if(flow == eCapture && role == eCommunications) {
		// Default input device has changed
		type = AsrcInputType;
	} else {
		// Don't care
		return S_OK;
	}

	// Forward to main thread. Strings need to be copied as they are freed when
	// this callback returns.
	emit defaultDeviceChanged(type, QString::fromUtf16(pwstrDeviceId));
	return S_OK;
}

HRESULT MMNotifier::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
	// Forward to main thread. Strings need to be copied as they are freed when
	// this callback returns.
	emit deviceAdded(QString::fromUtf16(pwstrDeviceId));
	return S_OK;
};

HRESULT MMNotifier::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
	// Forward to main thread. Strings need to be copied as they are freed when
	// this callback returns.
	emit deviceRemoved(QString::fromUtf16(pwstrDeviceId));
	return S_OK;
}

HRESULT MMNotifier::OnDeviceStateChanged(
	LPCWSTR pwstrDeviceId, DWORD dwNewState)
{
	// Forward to main thread. Strings need to be copied as they are freed when
	// this callback returns.
	emit deviceStateChanged(
		QString::fromUtf16(pwstrDeviceId), (uint)dwNewState);
	return S_OK;
}

HRESULT MMNotifier::OnPropertyValueChanged(
	LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
{
	// Don't care
	return S_OK;
}

//=============================================================================
// MMNotifierManager class

MMNotifierManager::MMNotifierManager(IMMDeviceEnumerator *deviceEnum)
	: QObject()
	, m_deviceEnum(deviceEnum)
{
	// Register our signal datatypes with Qt
	qRegisterMetaType<AsrcType>();

	// Create notifier object and connect callback signals
	MMNotifier *notifier = new MMNotifier();
	connect(notifier, &MMNotifier::defaultDeviceChanged,
		this, &MMNotifierManager::defaultDeviceChanged);
	connect(notifier, &MMNotifier::deviceAdded,
		this, &MMNotifierManager::deviceAdded);
	connect(notifier, &MMNotifier::deviceRemoved,
		this, &MMNotifierManager::deviceRemoved);
	connect(notifier, &MMNotifier::deviceStateChanged,
		this, &MMNotifierManager::deviceStateChanged);

	// Do the actual register
	HRESULT res = m_deviceEnum->RegisterEndpointNotificationCallback(notifier);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get register audio device callbacks. "
			<< "Reason = " << getAudioErrorCode(res);
		m_deviceEnum->Release();
		m_deviceEnum = NULL;
	}
}

MMNotifierManager::~MMNotifierManager()
{
	if(m_deviceEnum != NULL)
		m_deviceEnum->Release();
}

/// <summary>
/// Returns an IMMDevice object for the specified ID string. It is up to the
/// caller to release the returned object if it is not NULL.
/// </summary>
IMMDevice *MMNotifierManager::getDeviceFromIdString(
	LPCWSTR pwstrDeviceId) const
{
	IMMDevice *device = NULL;
	HRESULT res = m_deviceEnum->GetDevice(pwstrDeviceId, &device);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get IMMDevice from ID string. "
			<< "Reason = " << getAudioErrorCode(res);
		return NULL;
	}
	return device;
}

quint64 MMNotifierManager::getIdOfDefaultDevice(AsrcType type) const
{
	AudioSourceManager *mgr = App->getAudioSourceManager();

	// Determine the ID that the default device will have in our database
	quint64 fakeId = mgr->getDefaultIdOfType(type);

	AudioSourceList sources = mgr->getSourceList();
	for(int i = 0; i < sources.count(); i++) {
		AudioSource *src = sources.at(i);
		if(src->getId() == fakeId) {
			WASAPIAudioSource *wasapiSrc =
				static_cast<WASAPIAudioSource *>(src);
			return WASAPIAudioSource::getIdFromIMMDevice(
				wasapiSrc->getDevice());
		}
	}

	return 0ULL;
}

EDataFlow MMNotifierManager::getDataFlowOfDevice(IMMDevice *device) const
{
	if(device == NULL)
		return eRender;
	IMMEndpoint *endpoint;
	HRESULT res = device->QueryInterface(
		__uuidof(IMMEndpoint), (void **)&endpoint);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get IMMEndpoint from IMMDevice. "
			<< "Reason = " << getAudioErrorCode(res);
		return eRender;
	}
	EDataFlow flow;
	res = endpoint->GetDataFlow(&flow);
	endpoint->Release();
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get data flow direction from IMMEndpoint. "
			<< "Reason = " << getAudioErrorCode(res);
		return eRender;
	}
	return flow;
}

bool MMNotifierManager::reconstructSource(
	quint64 sourceId, AsrcType type, quint64 idOfDefault) const
{
	if(sourceId == 0)
		return false;

	AudioSourceManager *mgr = App->getAudioSourceManager();
	AudioSource *src = mgr->getSource(sourceId);
	if(src != NULL) {
		appLog(LOG_CAT)
			<< "Reconstructing audio device: " << src->getDebugString();

		WASAPIAudioSource *wasapiSrc =
			static_cast<WASAPIAudioSource *>(src);
		IMMDevice *device = wasapiSrc->getDevice();
		device->AddRef();
		mgr->removeSource(src);
		Q_ASSERT(src->getRefCount() == 0);
		delete src;

		WASAPIAudioSource *source = new WASAPIAudioSource(device, type);
		if(source->construct(idOfDefault)) {
			AudioSourceManager *mgr = App->getAudioSourceManager();
			mgr->addSource(source);
		} else {
			appLog(LOG_CAT, Log::Warning) << "Failed to reconstruct";
			delete source;
			device->Release();
			return false;
		}
	}
	return true;
}

void MMNotifierManager::defaultDeviceChanged(
	AsrcType type, const QString &deviceIdStr)
{
	// Get device IDs of the old and new defaults
	quint64 idOfNewDefault = 0;
	if(!deviceIdStr.isEmpty()) {
		IMMDevice *device = getDeviceFromIdString(deviceIdStr.utf16());
		if(device == NULL) {
			appLog(LOG_CAT, Log::Warning)
				<< "Default audio device has changed to an unknown device";
		} else {
			idOfNewDefault = WASAPIAudioSource::getIdFromIMMDevice(device);
			if(device != NULL)
				device->Release();
		}
	}
	quint64 idOfOldDefault = getIdOfDefaultDevice(type);

	// Is this already the default?
	if(idOfNewDefault == idOfOldDefault) {
		// Already the default, nothing to do
		return;
	}

	appLog(LOG_CAT) << "Changing default audio device...";

	// Reconstruct the old default source
	if(idOfOldDefault != 0) {
		reconstructSource(
			AudioSourceManager::getDefaultIdOfType(type), type, idOfNewDefault);
	}

	// Reconstruct the new default source
	if(idOfNewDefault != 0)
		reconstructSource(idOfNewDefault, type, idOfNewDefault);
}

void MMNotifierManager::deviceAdded(const QString &deviceIdStr)
{
	IMMDevice *device = getDeviceFromIdString(deviceIdStr.utf16());
	if(device == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown audio device added, ignoring";
		return;
	}
	bool isOutput = (getDataFlowOfDevice(device) == eRender);
	AsrcType type = isOutput ? AsrcOutputType : AsrcInputType;
	quint64 idOfDefault = getIdOfDefaultDevice(type);

	WASAPIAudioSource *source = new WASAPIAudioSource(device, type);
	if(source->construct(idOfDefault)) {
		AudioSourceManager *mgr = App->getAudioSourceManager();
		mgr->addSource(source);
		appLog(LOG_CAT)
			<< "Added new audio source: " << source->getDebugString();
	} else {
		delete source;
		device->Release();
	}
};

void MMNotifierManager::deviceRemoved(const QString &deviceIdStr)
{
	// WARNING: By the time this method is executed the device has already been
	// destroyed so we cannot use IMMDevice at all and if the source is in our
	// database it has already been invalidated.

	AudioSourceManager *mgr = App->getAudioSourceManager();

	// Determine the ID that this device will have in our database. As we don't
	// know the type of the device we need to check all the defaults.
	quint64 idOfDevice = WASAPIAudioSource::getIdFromIdString(
		(wchar_t *)deviceIdStr.utf16());
	quint64 idOfDefault = getIdOfDefaultDevice(AsrcOutputType);
	AsrcType type = AsrcOutputType;
	if(idOfDevice != idOfDefault) {
		quint64 idOfDefault = getIdOfDefaultDevice(AsrcInputType);
		AsrcType type = AsrcInputType;
	}
	quint64 id = idOfDevice;
	if(idOfDevice == idOfDefault)
		id = mgr->getDefaultIdOfType(type);

	// Is the device in our database?
	AudioSource *src = mgr->getSource(id);
	if(src == NULL) {
		// Already removed from our database
		return;
	}

	// Device is in the database, destroy it
	appLog(LOG_CAT) << "Removing audio source: " << src->getDebugString();
	mgr->removeSource(src);
	Q_ASSERT(src->getRefCount() == 0);
	delete src;
}

void MMNotifierManager::deviceStateChanged(
	const QString &deviceIdStr, uint dwNewState)
{
	IMMDevice *device = getDeviceFromIdString(deviceIdStr.utf16());
	if(device == NULL) {
		appLog(LOG_CAT, Log::Warning)
			<< "Unknown audio device changed state, ignoring";
		return;
	}
	bool isOutput = (getDataFlowOfDevice(device) == eRender);
	AsrcType type = isOutput ? AsrcOutputType : AsrcInputType;

	AudioSourceManager *mgr = App->getAudioSourceManager();

	// Determine the ID that this device will have in our database
	quint64 idOfDevice = WASAPIAudioSource::getIdFromIMMDevice(device);
	quint64 idOfDefault = getIdOfDefaultDevice(type);
	quint64 id = idOfDevice;
	if(idOfDevice == idOfDefault)
		id = mgr->getDefaultIdOfType(type);

	if(dwNewState & DEVICE_STATE_ACTIVE ||
		dwNewState & DEVICE_STATE_UNPLUGGED)
	{
		// Device should be added to our database if it isn't already in it
		AudioSource *src = mgr->getSource(id);
		if(src != NULL) {
			// Already added to our database
			device->Release();
			return;
		}

		// Device isn't in the database, create it
		WASAPIAudioSource *source = new WASAPIAudioSource(device, type);
		if(source->construct(idOfDefault)) {
			AudioSourceManager *mgr = App->getAudioSourceManager();
			mgr->addSource(source);
			appLog(LOG_CAT)
				<< "Added new audio source: " << source->getDebugString();
		} else {
			delete source;
			device->Release();
			return;
		}
	} else {
		// Release immediately otherwise we get deadlocks
		device->Release();

		// Device should be removed from out database if it is still in it
		AudioSource *src = mgr->getSource(id);
		if(src == NULL) {
			// Already removed from our database
			return;
		}

		// Device is in the database, destroy it
		appLog(LOG_CAT) << "Removing audio source: " << src->getDebugString();
		mgr->removeSource(src);
		Q_ASSERT(src->getRefCount() == 0);
		delete src;
	}
}

//=============================================================================
// WASAPIAudioSource class

/// <summary>
/// Creates all WASAPI-based audio sources and registers them with the manager.
/// </summary>
void WASAPIAudioSource::populateSources(AudioSourceManager *mgr)
{
	// Get MMDevice enumerator object
	IMMDeviceEnumerator *deviceEnum = NULL;
	HRESULT res = CoCreateInstance(
		__uuidof(MMDeviceEnumerator),
		NULL, CLSCTX_ALL, IID_PPV_ARGS(&deviceEnum));
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to create MMDeviceEnumerator. "
			<< "Reason = " << getAudioErrorCode(res);
		return;
	}

	// Construct all available output devices (E.g. speakers) and input devices
	// (E.g. microphones)
	constructAllOfType(mgr, deviceEnum, true);
	constructAllOfType(mgr, deviceEnum, false);

	// Watch operating system for device changes. The manager takes ownership
	// of the enumerator. TODO: This object is never deleted
	new MMNotifierManager(deviceEnum);
}

void WASAPIAudioSource::constructAllOfType(
	AudioSourceManager *mgr, IMMDeviceEnumerator *deviceEnum, bool isOutput)
{
	// Get list of all available devices
	IMMDeviceCollection *devices;
	HRESULT res = deviceEnum->EnumAudioEndpoints(
		isOutput ? eRender : eCapture,
		DEVICE_STATE_ACTIVE | DEVICE_STATE_UNPLUGGED, &devices);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get IMMDeviceCollection. "
			<< "Reason = " << getAudioErrorCode(res);
		return;
	}

	// Get the ID of the default device of this type
	IMMDevice *defaultDev = NULL;
	if(isOutput) {
		// Update notifier code above if this changes
		res = deviceEnum->GetDefaultAudioEndpoint(
			eRender, eConsole, &defaultDev);
	} else {
		// Update notifier code above if this changes
		res = deviceEnum->GetDefaultAudioEndpoint(
			eCapture, eCommunications, &defaultDev);
	}
	if(FAILED(res)) {
		if(res == E_NOTFOUND) {
			if(isOutput) {
				appLog(LOG_CAT, Log::Warning)
					<< "No default audio output device found";
			} else {
				appLog(LOG_CAT, Log::Warning)
					<< "No default audio input device found";
			}
		} else {
			appLog(LOG_CAT, Log::Warning)
				<< "Failed to get default WASAPI device. "
				<< "Reason = " << getAudioErrorCode(res);
		}
		defaultDev = NULL;
		// Continue anyway
	}
	quint64 idOfDefault = 0;
	if(defaultDev != NULL) {
		idOfDefault = getIdFromIMMDevice(defaultDev);
		defaultDev->Release();
	}

	// Iterate over all available devices and construct them
	UINT numDevices = 0;
	if(FAILED(devices->GetCount(&numDevices)))
		numDevices = 0;
	for(uint i = 0; i < numDevices; i++) {
		IMMDevice *device = NULL;
		if(FAILED(devices->Item(i, &device)))
			continue;
		if(device == NULL)
			continue;
		WASAPIAudioSource *source = new WASAPIAudioSource(
			device, isOutput ? AsrcOutputType : AsrcInputType);
		if(source->construct(idOfDefault))
			mgr->addSource(source);
		else {
			delete source;
			device->Release();
		}
	}

	// Clean up
	devices->Release();
}

quint64 WASAPIAudioSource::getIdFromIdString(wchar_t *wIdStr)
{
	QString idStr = QString::fromUtf16(wIdStr);
	return hashQString64(idStr);
}

quint64 WASAPIAudioSource::getIdFromIMMDevice(IMMDevice *device)
{
	wchar_t *wIdStr = NULL;
	HRESULT res = device->GetId(&wIdStr);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get unique device ID string. "
			<< "Reason = " << getAudioErrorCode(res);
		return false;
	}
	return getIdFromIdString(wIdStr);
}

WASAPIAudioSource::WASAPIAudioSource(IMMDevice *device, AsrcType type)
	: AudioSource(type)
	, m_device(device)
	, m_client(NULL)
	, m_client2(NULL)
	, m_captureClient(NULL)
	, m_renderClient(NULL)
	, m_renderBufNumFrames(0)
	, m_resampler(NULL)
	, m_id(0)
	, m_friendlyName(tr("** Unknown **"))
	, m_isConnected(false)
	, m_mixer(NULL)
	, m_frameSize(0)
	, m_inSampleRate(0)
	, m_numFramesProcessed(0)
	, m_timestampAdjust(0LL)
	, m_firstSampleQPC100ns(UINT64_MAX)
	, m_ref(0)
	//, m_refMutex(QMutex::Recursive)
{
}

bool WASAPIAudioSource::construct(quint64 idOfDefault)
{
	// Get unique ID string
	m_id = getIdFromIMMDevice(m_device);
	if(m_id == idOfDefault) {
		// Device is a system default, give it a special ID
		m_id = AudioSourceManager::getDefaultIdOfType(m_type);
	}

	// Get friendly name
	IPropertyStore *store;
	if(SUCCEEDED(m_device->OpenPropertyStore(STGM_READ, &store))) {
		PROPVARIANT varName;
		PropVariantInit(&varName);
		if(SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &varName)))
			m_friendlyName = QString::fromUtf16(varName.pwszVal);
		store->Release();
	}

	// Get connected/enabled state
	DWORD state;
	if(SUCCEEDED(m_device->GetState(&state)))
		m_isConnected = (state & DEVICE_STATE_ACTIVE) ? true : false;

	// Successfully constructed audio source
	return true;
}

WASAPIAudioSource::~WASAPIAudioSource()
{
	if(m_ref != 0) {
		appLog(LOG_CAT, Log::Warning) <<
			QStringLiteral("Destroying referenced audio source \"%1\" (%L2 references)")
			.arg(getDebugString())
			.arg(m_ref);
		shutdown(); // Make sure everything is released
	}

	m_device->Release();
}

bool WASAPIAudioSource::initialize()
{
	if(!initializeCapture())
		return false;
	if(m_type == AsrcOutputType) {
		// Due to the way WASAPI works if we are using a loopback capture on an
		// output device and nothing is outputting to that same device then we
		// will never actually receive any audio data at all (I.e.
		// `IAudioCaptureClient::GetNextPacketSize()` always returns 0). In
		// order to work around this we create our own output stream and keep
		// it full of "silence".
		if(!initializeRender()) {
			shutdown();
			return false;
		}
	}
	return true;
}

bool WASAPIAudioSource::initializeCapture()
{
	// Get audio client object
	HRESULT res = m_device->Activate(
		__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&m_client);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get audio client object. "
			<< "Reason = " << getAudioErrorCode(res);
		m_client = NULL;
		return false;
	}

	//-------------------------------------------------------------------------
	// Get audio client mix format and convert it to something that
	// libswresample understands

	// Libswresample inputs
	int64_t			chanLayout = 0; // AV_CH_LAYOUT_*
	AVSampleFormat	sampleFormat = AV_SAMPLE_FMT_NONE; // AV_SAMPLE_FMT_*

	WAVEFORMATEX *format = NULL;
	res = m_client->GetMixFormat(&format);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get the audio client mix format. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize1;
	}
	m_inSampleRate = format->nSamplesPerSec;
	m_frameSize = format->nBlockAlign;
	if(format->wFormatTag == WAVE_FORMAT_EXTENSIBLE && format->cbSize >= 22) {
		WAVEFORMATEXTENSIBLE *formatEx =
			reinterpret_cast<WAVEFORMATEXTENSIBLE *>(format);

		// Determine sample format
		if(formatEx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) {
			// Samples are stored in a floating point format
			sampleFormat = AV_SAMPLE_FMT_FLT;
		} else if(formatEx->SubFormat == KSDATAFORMAT_SUBTYPE_PCM) {
			// Samples are stored in an integer PCM format
			if(format->wBitsPerSample == 8)
				sampleFormat = AV_SAMPLE_FMT_U8;
			else if(format->wBitsPerSample == 16)
				sampleFormat = AV_SAMPLE_FMT_S16;
			else if(format->wBitsPerSample == 32)
				sampleFormat = AV_SAMPLE_FMT_S32;
			else {
				appLog(LOG_CAT, Log::Warning) <<
					QStringLiteral("Unsupported PCM sample size %1, cannot enable source")
					.arg(format->wBitsPerSample);
				CoTaskMemFree(format);
				goto exitInitialize2;
			}
		} else {
			appLog(LOG_CAT, Log::Warning) <<
				QStringLiteral("Unsupported audio client mix format %1, cannot enable source")
				.arg(guidToString(formatEx->SubFormat));
			CoTaskMemFree(format);
			goto exitInitialize2;
		}

		// Determine channel layout
		switch(formatEx->dwChannelMask) {
		case KSAUDIO_SPEAKER_MONO:
			chanLayout = AV_CH_LAYOUT_MONO;
			break;
		case KSAUDIO_SPEAKER_STEREO:
			chanLayout = AV_CH_LAYOUT_STEREO;
			break;
		case KSAUDIO_SPEAKER_QUAD:
			chanLayout = AV_CH_LAYOUT_QUAD;
			break;
		case KSAUDIO_SPEAKER_SURROUND:
			chanLayout = AV_CH_LAYOUT_4POINT0;
			break;
		case KSAUDIO_SPEAKER_5POINT1:
			chanLayout = AV_CH_LAYOUT_5POINT1_BACK;
			break;
		case KSAUDIO_SPEAKER_7POINT1:
			chanLayout = AV_CH_LAYOUT_7POINT1_WIDE_BACK;
			break;
		case KSAUDIO_SPEAKER_5POINT1_SURROUND:
			chanLayout = AV_CH_LAYOUT_5POINT1;
			break;
		case KSAUDIO_SPEAKER_7POINT1_SURROUND:
			chanLayout = AV_CH_LAYOUT_7POINT1;
			break;
		default:
			break; // We'll determine it later from number of channels
		}
	} else {
		// Determine sample format
		if(format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
			// Samples are stored in a floating point format
			sampleFormat = AV_SAMPLE_FMT_FLT;
		} else if(format->wFormatTag == WAVE_FORMAT_PCM) {
			// Samples are stored in an integer PCM format
			if(format->wBitsPerSample == 8)
				sampleFormat = AV_SAMPLE_FMT_U8;
			else if(format->wBitsPerSample == 16)
				sampleFormat = AV_SAMPLE_FMT_S16;
			else if(format->wBitsPerSample == 32)
				sampleFormat = AV_SAMPLE_FMT_S32;
			else {
				appLog(LOG_CAT, Log::Warning) <<
					QStringLiteral("Unsupported PCM sample size %1, cannot enable source")
					.arg(format->wBitsPerSample);
				CoTaskMemFree(format);
				goto exitInitialize2;
			}
		} else {
			appLog(LOG_CAT, Log::Warning) <<
				QStringLiteral("Unsupported audio client mix format %1, cannot enable source")
				.arg(numberToHexString(format->wFormatTag));
			CoTaskMemFree(format);
			goto exitInitialize2;
		}
	}

	// Determine channel layout from number of channels if it hasn't been set
	// yet
	if(chanLayout == 0) {
		appLog(LOG_CAT, Log::Warning) <<
			QStringLiteral("Guessing channel layout from number of channels (%1)")
			.arg(format->nChannels);
#define USE_AVUTIL_LAYOUT_GUESS 1
#if USE_AVUTIL_LAYOUT_GUESS
		chanLayout = av_get_default_channel_layout(format->nChannels);
		if(chanLayout == 0) {
			appLog(LOG_CAT, Log::Warning)
				<< "Unknown channel layout, cannot enable source";
			CoTaskMemFree(format);
			goto exitInitialize2;
		}
#else
		switch(format->nChannels) {
		case 1:
			chanLayout = AV_CH_LAYOUT_MONO;
			break;
		case 2:
			chanLayout = AV_CH_LAYOUT_STEREO;
			break;
		case 3:
			chanLayout = AV_CH_LAYOUT_SURROUND;
			break;
		case 4:
			chanLayout = AV_CH_LAYOUT_QUAD;
			break;
		case 5:
			chanLayout = AV_CH_LAYOUT_5POINT0_BACK;
			break;
		case 6:
			chanLayout = AV_CH_LAYOUT_5POINT1_BACK;
			break;
		case 7:
			chanLayout = AV_CH_LAYOUT_6POINT1_BACK;
			break;
		case 8:
			chanLayout = AV_CH_LAYOUT_7POINT1_WIDE_BACK;
			break;
		default:
			appLog(LOG_CAT, Log::Warning)
				<< "Unknown channel layout, cannot enable source";
			CoTaskMemFree(format);
			goto exitInitialize2;
		}
#endif // USE_AVUTIL_LAYOUT_GUESS
	}

	// Log format in case something went wrong and the user complains
	appLog(LOG_CAT) <<
		QStringLiteral("Input audio format: Sample rate = %L1; Sample format = %2; Channel layout = %3")
		.arg(m_inSampleRate)
		.arg(sampleFormatToString(sampleFormat))
		.arg(channelLayoutToString(chanLayout));

	//-------------------------------------------------------------------------

	// Initialize audio client with an X second buffer
	res = m_client->Initialize(
		AUDCLNT_SHAREMODE_SHARED,
		(m_type == AsrcOutputType) ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0,
		AUDIO_BUFFER_SIZE_100NS, 0, format, NULL);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to initialize audio client. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize2;
	}

	// Get audio capture client object
	res = m_client->GetService(
		__uuidof(IAudioCaptureClient), (void**)&m_captureClient);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get audio capture client object. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize3;
	}

#if 0
	// Get audio capture clock object for debugging synchronisation
	IAudioClock *captureClock = NULL;
	res = m_client->GetService(
		__uuidof(IAudioClock), (void**)&captureClock);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get audio capture clock object. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize4;
	}
	UINT64 clkFreq = 0;
	captureClock->GetFrequency(&clkFreq);
	appLog(LOG_CAT) << QStringLiteral(
		"Reported clock frequency: %L1").arg(clkFreq);
	captureClock->Release();
#endif // 0

	// Create Libswresample context
	// TODO: We assume that the default input channel mapping is correct
	int64_t outChanLayout = (m_mixer->getNumChannels() != 1)
		? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO; // TODO
	m_resampler = new Resampler(
		chanLayout, m_inSampleRate, sampleFormat, outChanLayout,
		m_mixer->getSampleRate(), AV_SAMPLE_FMT_FLT);
	if(!m_resampler->initialize()) {
		appLog(LOG_CAT, Log::Warning) << "Failed to create audio resampler";
		goto exitInitialize4;
	}

	// Start capturing data
	res = m_client->Start();
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to start capturing audio data. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize4;
	}
	m_numFramesProcessed = 0;
	m_timestampAdjust = 0LL;
	m_firstSampleQPC100ns = UINT64_MAX;

	// Clean up
	CoTaskMemFree(format);

	// Successfully initialized audio source!
	return true;

	// Error handling
exitInitialize4:
	if(m_resampler != NULL) {
		delete m_resampler;
		m_resampler = NULL;
	}
exitInitialize3:
	if(m_captureClient != NULL)
		m_captureClient->Release();
	m_captureClient = NULL;
exitInitialize2:
	CoTaskMemFree(format);
exitInitialize1:
	if(m_client != NULL)
		m_client->Release();
	m_client = NULL;
	return false;
}

bool WASAPIAudioSource::initializeRender()
{
	// Get audio client object
	HRESULT res = m_device->Activate(
		__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&m_client2);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get audio client object for renderer. "
			<< "Reason = " << getAudioErrorCode(res);
		m_client2 = NULL;
		return false;
	}

	// Get audio client mix format
	WAVEFORMATEX *format = NULL;
	res = m_client2->GetMixFormat(&format);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get the audio client mix format for renderer. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize1;
	}

	// Initialize audio client with an X second buffer
	res = m_client2->Initialize(
		AUDCLNT_SHAREMODE_SHARED, 0, AUDIO_BUFFER_SIZE_100NS, 0, format, NULL);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to initialize audio client for renderer. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize2;
	}

	// Get the actual buffer size in frames
	UINT32 numFrames;
	res = m_client2->GetBufferSize(&numFrames);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get audio buffer size for renderer. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize2;
	}
	m_renderBufNumFrames = numFrames;

	// Get audio render client object
	res = m_client2->GetService(
		__uuidof(IAudioRenderClient), (void**)&m_renderClient);
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to get audio render client object. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize3;
	}

	// Pre-fill buffer with silence
	UINT32 numPaddingFrames;
	res = m_client2->GetCurrentPadding(&numPaddingFrames);
	if(SUCCEEDED(res)) {
		BYTE *data;
		UINT32 bufSize = (UINT32)m_renderBufNumFrames - numPaddingFrames;
		res = m_renderClient->GetBuffer(bufSize, &data);
		if(SUCCEEDED(res))
			m_renderClient->ReleaseBuffer(bufSize, AUDCLNT_BUFFERFLAGS_SILENT);
	}

	// Start rendering data
	res = m_client2->Start();
	if(FAILED(res)) {
		appLog(LOG_CAT, Log::Warning)
			<< "Failed to start capturing audio data. "
			<< "Reason = " << getAudioErrorCode(res);
		goto exitInitialize3;
	}

	// Clean up
	CoTaskMemFree(format);

	// Successfully initialized audio renderer!
	return true;

	// Error handling
exitInitialize3:
	if(m_renderClient != NULL)
		m_renderClient->Release();
	m_renderClient = NULL;
exitInitialize2:
	CoTaskMemFree(format);
exitInitialize1:
	if(m_client2 != NULL)
		m_client2->Release();
	m_client2 = NULL;
	return false;
}

void WASAPIAudioSource::process(int numTicks)
{
	if(m_ref <= 0)
		return; // Not enabled
	if(m_captureClient == NULL)
		return; // Should never happen

	// TODO: Handle `AUDCLNT_E_DEVICE_INVALIDATED`

	// If we have a fake output device make sure it's always full of "silence"
	if(m_client2 != NULL && m_renderClient != NULL) {
		UINT32 numPaddingFrames;
		HRESULT res = m_client2->GetCurrentPadding(&numPaddingFrames);
		if(SUCCEEDED(res)) {
			BYTE *data;
			UINT32 bufSize = (UINT32)m_renderBufNumFrames - numPaddingFrames;
			res = m_renderClient->GetBuffer(bufSize, &data);
			if(SUCCEEDED(res)) {
				m_renderClient->ReleaseBuffer(
					bufSize, AUDCLNT_BUFFERFLAGS_SILENT);
			}
		}
	}

	// Loop until we completely empty the buffer
	for(;;) {
		// Get ready for the next iteration
		UINT32 numFrames;
		HRESULT res = m_captureClient->GetNextPacketSize(&numFrames);
		if(FAILED(res)) {
			//appLog(LOG_CAT, Log::Warning)
			//	<< "Failed to get next buffer packet size. "
			//	<< "Reason = " << getAudioErrorCode(res);
			return; // Don't log as it'll spam the log file
		}
		if(numFrames <= 0)
			break;

		//appLog(LOG_CAT) << "Processing " << numFrames << " audio frames";

		// Read the next packet of data from the operating system
		BYTE *	data;
		DWORD	flags;
		UINT64	devPos;
		UINT64	qpcPos;
		res = m_captureClient->GetBuffer(
			&data, &numFrames, &flags, &devPos, &qpcPos);
		UINT32 releaseNumFrames = numFrames;
		if(FAILED(res)) {
			//appLog(LOG_CAT, Log::Warning)
			//	<< "Failed to get audio buffer. "
			//	<< "Reason = " << getAudioErrorCode(res);
			return; // Don't log as it'll spam the log file
		}

		// Convert timestamp to something that we understand
		WinApplication *winApp = static_cast<WinApplication *>(App);
		qint64 timestamp = winApp->qpcPosToUsecSinceFrameOrigin(qpcPos);

		// Sanitise the timestamp. Some devices have timestamps that are far in
		// into the past or into the future. For these devices we just adjust
		// them as if the first sample received was captured at that exact time
		qint64 now = App->getUsecSinceFrameOrigin();
		if(m_firstSampleQPC100ns == UINT64_MAX &&
			qAbs(timestamp - now) > 10000000LL)
		{
			// More than 10 seconds difference, assuming invalid timestamp. If
			// a user reports a bug about sound being out-of-sync by a second
			// or two then this is most likely the cause.
			m_timestampAdjust = now - timestamp;
			appLog(LOG_CAT, Log::Warning) << QStringLiteral(
				"Audio device timestamps may be invalid, adjusting by %L1 ms")
				.arg(m_timestampAdjust / 1000LL);
		}
		timestamp += m_timestampAdjust;

		// Throw away samples that were recorded before our mainloop started
		if(timestamp < 0LL) {
			res = m_captureClient->ReleaseBuffer(releaseNumFrames);
			if(FAILED(res)) {
				//appLog(LOG_CAT, Log::Warning)
				//	<< "Failed to release audio buffer. "
				//	<< "Reason = " << getAudioErrorCode(res);
				return; // Don't log as it'll spam the log file
			}
			continue;
		}

		// Compensate for sample frequency drift (Inaccurate sample frequency)
		// and discontinuities (Buffer overrun, etc). Drift does occur and
		// causes slight desynchronisation over long periods of time (Takes
		// 30-45 minutes to become noticable). By doing it this way we do not
		// need to watch for the discontinuity flag which is not available in
		// Windows Vista anyway.
		const qint64 MAX_ACCEPTABLE_DESYNC_100NS = 500000; // 50 msec
		//const qint64 MAX_ACCEPTABLE_DESYNC_100NS = 10000; // 1 msec
		QByteArray disconBuf;
		if(m_firstSampleQPC100ns == UINT64_MAX) {
			// This is the first frame
			m_firstSampleQPC100ns = qpcPos;
		} else {
			// Calculate the expected QPC of the current packet
			quint64 qpcExpected = m_firstSampleQPC100ns +
				(m_numFramesProcessed * 10000000ULL) / (quint64)m_inSampleRate;
			qint64 diff100ns = qpcPos - qpcExpected;
			int diffFrames =
				(int)(diff100ns * (qint64)m_inSampleRate / 10000000LL);
			// TODO: Handle negative desync (We need to remove samples)
			if(diff100ns > MAX_ACCEPTABLE_DESYNC_100NS) {
				// TODO: swr_inject_silence() ?
				//disconBuf.reserve((numFrames + diffFrames) * m_frameSize);
				disconBuf.fill(0, diffFrames * m_frameSize);
				disconBuf.append((char *)data, numFrames * m_frameSize);
				data = reinterpret_cast<BYTE *>(disconBuf.data());
				timestamp -=
					((qint64)diffFrames * 1000000LL) / (qint64)m_inSampleRate;
				appLog(LOG_CAT) << QStringLiteral(
					"Resynchronised audio by %L1 samples for \"%2\"")
					.arg(diffFrames).arg(m_friendlyName);
				numFrames += diffFrames;
			}
		}
		m_numFramesProcessed += numFrames;
		// WARNING: Number of frames cannot be modified after this line

		// If the frame is silence we should discard the data in the buffer and
		// create a truly silent buffer. We use QByteArray to simplify memory
		// deallocation.
		// TODO: swr_inject_silence() ?
		QByteArray silence;
		if(flags & AUDCLNT_BUFFERFLAGS_SILENT) {
			silence.fill(0, numFrames * m_frameSize);
			data = reinterpret_cast<BYTE *>(silence.data());
		}

		// Resample the data and adjust our timestamp to take into account
		// resampling delay
		int delayedFrames;
		QByteArray newData = m_resampler->resample(
			reinterpret_cast<const char *>(data), numFrames * m_frameSize,
			delayedFrames);
		timestamp -=
			(delayedFrames * 1000000LL) / (qint64)m_mixer->getSampleRate();

		// This should always be true but it does VERY rarely happen. Why?
		//Q_ASSERT(timestamp >= 0);

		// Emit audio segment
		if(timestamp >= 0)
			emit segmentReady(AudioSegment((quint64)timestamp, newData));

		// Release the buffer
		res = m_captureClient->ReleaseBuffer(releaseNumFrames);
		if(FAILED(res)) {
			//appLog(LOG_CAT, Log::Warning)
			//	<< "Failed to release audio buffer. "
			//	<< "Reason = " << getAudioErrorCode(res);
			return; // Don't log as it'll spam the log file
		}
	}
}

void WASAPIAudioSource::shutdown()
{
	// WARNING: This method can be called half way through `initialize()`

	// Stop capturing data. We don't care if this call fails
	if(m_client != NULL)
		m_client->Stop();
	if(m_client2 != NULL)
		m_client2->Stop();

	// Release resampler object
	if(m_resampler != NULL) {
		delete m_resampler;
		m_resampler = NULL;
	}

	// Release WASAPI objects
	if(m_renderClient != NULL)
		m_renderClient->Release();
	if(m_captureClient != NULL)
		m_captureClient->Release();
	if(m_client2 != NULL)
		m_client2->Release();
	if(m_client != NULL)
		m_client->Release();
	m_renderClient = NULL;
	m_captureClient = NULL;
	m_client2 = NULL;
	m_client = NULL;
}

/// <summary>
/// Returns the unique ID of this audio source. IDs 1 and 2 are special as they
/// are the default speakers and microphone respectively. We want the defaults
/// to be constant so it's easier to copy profiles between computers and not
/// have to reselect the default devices manually.
/// </summary>
quint64 WASAPIAudioSource::getId() const
{
	return m_id;
}

/// <summary>
/// Returns the unique ID of this audio source ignoring if it is the default
/// device or not.
/// </summary>
quint64 WASAPIAudioSource::getRealId() const
{
	return getIdFromIMMDevice(m_device);
}

QString WASAPIAudioSource::getFriendlyName() const
{
	return m_friendlyName;
}

QString WASAPIAudioSource::getDebugString() const
{
	return QStringLiteral("%1 [%2]%3")
		.arg(m_friendlyName)
		.arg(numberToHexString(m_id))
		.arg(m_isConnected ? QString() : QStringLiteral(" (Unplugged)"));
}

bool WASAPIAudioSource::isConnected() const
{
	return m_isConnected;
}

bool WASAPIAudioSource::reference(AudioMixer *mixer)
{
	//m_refMutex.lock();
	if(m_mixer != NULL) {
		if(m_mixer->getAudioMode() != mixer->getAudioMode() ||
			m_mixer->getSampleRate() != mixer->getSampleRate())
		{
			appLog(LOG_CAT, Log::Warning) <<
				QStringLiteral("Attempting to reference audio source \"%1\" with different output settings")
				.arg(getDebugString());
		}
	} else if(mixer == NULL) {
		//m_refMutex.unlock();
		return false;
	}
	m_mixer = mixer;
	m_ref++;
	if(m_ref == 1) {
		if(!initialize()) {
			// Already logged reason
			m_ref--;
			//m_refMutex.unlock();
			return false;
		}
	}
	//m_refMutex.unlock();
	return true;
}

void WASAPIAudioSource::dereference()
{
	//m_refMutex.lock();
	if(m_ref <= 0) {
		appLog(LOG_CAT, Log::Warning) <<
			QStringLiteral("Audio source \"%1\" was dereferenced more times than it was referenced")
			.arg(getDebugString());
		//m_refMutex.unlock();
		return;
	}
	m_ref--;
	if(m_ref == 0) {
		shutdown();
		m_mixer = NULL;
	}
	//m_refMutex.unlock();
}

int WASAPIAudioSource::getRefCount()
{
	//m_refMutex.lock();
	int ref = m_ref;
	//m_refMutex.unlock();
	return ref;
}
