//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 1992-2001 Microsoft Corporation
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
// All of these classes are stripped down and slightly modified ports of
// Microsoft's official DirectShow sample base classes from the Windows 7 SDK.
// We cannot use the classes directly from the SDK as the classes are not
// included in the Windows 8 SDK (DirectShow is deprecated) and we don't want
// to have to depend on the Windows 7 SDK in addition to the Windows 8 SDK.
//*****************************************************************************

#include "dshowfilters.h"

const QString LOG_CAT = QStringLiteral("DirectShow");

const LONGLONG MAX_TIME = 0x7FFFFFFFFFFFFFFF; // Maximum LONGLONG value

//=============================================================================
// Helpers

STDAPI GetInterface(LPUNKNOWN pUnk, void **ppv)
{
	if(ppv == NULL)
		return E_POINTER;
	*ppv = pUnk;
	pUnk->AddRef();
	return NOERROR;
}

// Return a wide string - allocating memory for it
// Returns:
//    S_OK          - no error
//    E_POINTER     - ppszReturn == NULL
//    E_OUTOFMEMORY - can't allocate memory for returned string
STDAPI AMGetWideString(LPCWSTR psz, LPWSTR *ppszReturn)
{
	if(ppszReturn == NULL)
		return E_POINTER;
	*ppszReturn = NULL;
	size_t nameLen;
	HRESULT hr = StringCbLengthW(psz, 100000, &nameLen);
	if(FAILED(hr))
		return hr;
	*ppszReturn = (LPWSTR)CoTaskMemAlloc(nameLen + sizeof(WCHAR));
	if(*ppszReturn == NULL)
		return E_OUTOFMEMORY;
	CopyMemory(*ppszReturn, psz, nameLen + sizeof(WCHAR));
	return NOERROR;
}

STDAPI CreateMemoryAllocator(IMemAllocator **ppAllocator)
{
	return CoCreateInstance(
		CLSID_MemoryAllocator, 0, CLSCTX_INPROC_SERVER,
		IID_IMemAllocator, (void **)ppAllocator);
}

static const TCHAR szOle32Aut[] = TEXT("OleAut32.dll");
HINSTANCE g_hlibOLEAut32 = NULL;
HINSTANCE LoadOLEAut32()
{
	if(g_hlibOLEAut32 == NULL)
		g_hlibOLEAut32 = LoadLibrary(szOle32Aut);
	return g_hlibOLEAut32;
}

// Waits for the HANDLE hObject.  While waiting messages sent
// to windows on our thread by SendMessage will be processed.
// Using this function to do waits and mutual exclusion
// avoids some deadlocks in objects with windows.
// Return codes are the same as for WaitForSingleObject
DWORD WINAPI WaitDispatchingMessages(
	HANDLE hObject, DWORD dwWait, HWND hwnd = NULL, UINT uMsg = 0,
	HANDLE hEvent = NULL)
{
	BOOL bPeeked = FALSE;
	DWORD dwResult;
	DWORD dwStart;
	DWORD dwThreadPriority;

	static UINT uMsgId = 0;

	HANDLE hObjects[2] = { hObject, hEvent };
	if(dwWait != INFINITE && dwWait != 0)
		dwStart = GetTickCount();
	for(;;) {
		DWORD nCount = NULL != hEvent ? 2 : 1;

		// Minimize the chance of actually dispatching any messages
		// by seeing if we can lock immediately.
		dwResult = WaitForMultipleObjects(nCount, hObjects, FALSE, 0);
		if(dwResult < WAIT_OBJECT_0 + nCount)
			break;

		DWORD dwTimeOut = dwWait;
		if(dwTimeOut > 10)
			dwTimeOut = 10;
		dwResult = MsgWaitForMultipleObjects(
			nCount, hObjects, FALSE, dwTimeOut,
			hwnd == NULL ? QS_SENDMESSAGE : QS_SENDMESSAGE + QS_POSTMESSAGE);
		if(dwResult == WAIT_OBJECT_0 + nCount ||
			dwResult == WAIT_TIMEOUT && dwTimeOut != dwWait)
		{
			MSG msg;
			if(hwnd != NULL) {
				while(PeekMessage(&msg, hwnd, uMsg, uMsg, PM_REMOVE))
					DispatchMessage(&msg);
			}
			// Do this anyway - the previous peek doesn't flush out the
			// messages
			PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

			if(dwWait != INFINITE && dwWait != 0) {
				DWORD dwNow = GetTickCount();

				// Working with differences handles wrap-around
				DWORD dwDiff = dwNow - dwStart;
				if(dwDiff > dwWait)
					dwWait = 0;
				else
					dwWait -= dwDiff;
				dwStart = dwNow;
			}
			if(!bPeeked) {
				//  Raise our priority to prevent our message queue
				//  building up
				dwThreadPriority = GetThreadPriority(GetCurrentThread());
				if(dwThreadPriority < THREAD_PRIORITY_HIGHEST) {
					SetThreadPriority(
						GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
				}
				bPeeked = TRUE;
			}
		} else
			break;
	}
	if(bPeeked) {
		SetThreadPriority(GetCurrentThread(), dwThreadPriority);
		if(HIWORD(GetQueueStatus(QS_POSTMESSAGE)) & QS_POSTMESSAGE) {
			if(uMsgId == 0)
				uMsgId = RegisterWindowMessage(TEXT("AMUnblock"));
			if(uMsgId != 0) {
				MSG msg;
				// Remove old ones
				while(PeekMessage(&msg, (HWND)-1, uMsgId, uMsgId, PM_REMOVE))
				{}
			}
			PostThreadMessage(GetCurrentThreadId(), uMsgId, 0, 0);
		}
	}
	return dwResult;
}

//=============================================================================
// Misc. functions

// http://msdn.microsoft.com/en-us/library/ms783692%28v=vs.85%29.aspx
void FreeMediaType(AM_MEDIA_TYPE &mt)
{
	if(mt.cbFormat != 0) {
		CoTaskMemFree((PVOID)mt.pbFormat);
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if(mt.pUnk != NULL) {
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}

// http://msdn.microsoft.com/en-us/library/ms783299%28v=vs.85%29.aspx
void DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if(pmt != NULL) {
		FreeMediaType(*pmt);
		CoTaskMemFree(pmt);
	}
}

// `mtype.cpp` in sample base classes
HRESULT CopyMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource)
{
	//  We'll leak if we copy onto one that already exists - there's one
	//  case we can check like that - copying to itself.
	Q_ASSERT(pmtSource != pmtTarget);
	*pmtTarget = *pmtSource;
	if(pmtSource->cbFormat != 0) {
		Q_ASSERT(pmtSource->pbFormat != NULL);
		pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc(pmtSource->cbFormat);
		if(pmtTarget->pbFormat == NULL) {
			pmtTarget->cbFormat = 0;
			return E_OUTOFMEMORY;
		} else {
			CopyMemory((PVOID)pmtTarget->pbFormat, (PVOID)pmtSource->pbFormat,
				pmtTarget->cbFormat);
		}
	}
	if(pmtTarget->pUnk != NULL)
		pmtTarget->pUnk->AddRef();

	return S_OK;
}

// Use this function to register a graph in the Running Object Table so that it
// can be opened in GraphEdit. Before the graph is visible in GraphEdit you
// must first register a specific DLL by running the following in a command
// prompt that has administrator privileges (Type "cmd" in the start menu and
// then press Ctrl+Shift+Enter):
//
// C:\Windows\SysWOW64\regsvr32.exe "C:\Program Files (x86)\Windows Kits\8.0\bin\x86\proppage.dll"
//
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd390650%28v=vs.85%29.aspx
HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
{
	IMoniker *pMoniker = NULL;
	IRunningObjectTable *pROT = NULL;

	if(FAILED(GetRunningObjectTable(0, &pROT)))
		return E_FAIL;

	const size_t STRING_LENGTH = 256;
	WCHAR wsz[STRING_LENGTH];
	StringCchPrintfW(wsz, STRING_LENGTH, L"FilterGraph %08x pid %08x",
		(DWORD_PTR)pUnkGraph, GetCurrentProcessId());

	HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
	if(SUCCEEDED(hr)) {
		hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph,
			pMoniker, pdwRegister);
		pMoniker->Release();
	}
	pROT->Release();

	return hr;
}

// http://msdn.microsoft.com/en-us/library/windows/desktop/dd390650%28v=vs.85%29.aspx
void RemoveFromRot(DWORD pdwRegister)
{
	IRunningObjectTable *pROT;
	if(SUCCEEDED(GetRunningObjectTable(0, &pROT))) {
		pROT->Revoke(pdwRegister);
		pROT->Release();
	}
}

//=============================================================================
// CAMEvent class

CAMEvent::CAMEvent(BOOL fManualReset, HRESULT *phr)
{
	m_hEvent = CreateEvent(NULL, fManualReset, FALSE, NULL);
	if(m_hEvent != NULL)
		return;
	if(phr != NULL && SUCCEEDED(*phr))
		*phr = E_OUTOFMEMORY;
}

CAMEvent::CAMEvent(HRESULT *phr)
{
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(m_hEvent != NULL)
		return;
	if(phr != NULL && SUCCEEDED(*phr))
		*phr = E_OUTOFMEMORY;
}

CAMEvent::~CAMEvent()
{
	if(m_hEvent == NULL)
		return;
	BOOL ret = CloseHandle(m_hEvent);
	Q_ASSERT(ret);
}

//=============================================================================
// CUnknown class

LONG CUnknown::m_cObjects = 0;

CUnknown::CUnknown(LPCTSTR pName, LPUNKNOWN pUnk)
	: m_cRef(0)
	, m_pUnknown(pUnk)
{
	InterlockedIncrement(&m_cObjects);
	if(pUnk == 0)
		m_pUnknown = static_cast<LPUNKNOWN>(this);
}

CUnknown::CUnknown(LPCTSTR Name, LPUNKNOWN pUnk, HRESULT *phr)
	: m_cRef(0)
	, m_pUnknown(pUnk)
{
	InterlockedIncrement(&m_cObjects);
	if(pUnk == 0)
		m_pUnknown = static_cast<LPUNKNOWN>(this);
}

CUnknown::CUnknown(LPCSTR pName, LPUNKNOWN pUnk)
	: m_cRef(0)
	, m_pUnknown(pUnk)
{
	InterlockedIncrement(&m_cObjects);
	if(pUnk == 0)
		m_pUnknown = static_cast<LPUNKNOWN>(this);
}

CUnknown::CUnknown(LPCSTR pName, LPUNKNOWN pUnk, HRESULT *phr)
	: m_cRef(0)
	, m_pUnknown(pUnk)
{
	InterlockedIncrement(&m_cObjects);
	if(pUnk == 0)
		m_pUnknown = static_cast<LPUNKNOWN>(this);
}

CUnknown::~CUnknown()
{
	if(InterlockedDecrement(&m_cObjects) != 0)
		return;
	if(g_hlibOLEAut32 == NULL)
		return;
	FreeLibrary(g_hlibOLEAut32);
	g_hlibOLEAut32 = NULL;
}

STDMETHODIMP CUnknown::DelegateQueryInterface(REFIID riid, void **ppv)
{
	if(ppv == NULL)
		return E_POINTER;

	// We know only about IUnknown
	if(riid == IID_IUnknown) {
		GetInterface((LPUNKNOWN)this, ppv);
		return NOERROR;
	} else {
		*ppv = NULL;
		return E_NOINTERFACE;
	}
}

// We have to ensure that we DON'T use a max macro, since these will typically
// lead to one of the parameters being evaluated twice. Since we are worried
// about concurrency, we can't afford to access the m_cRef twice since we can't
// afford to run the risk that its value having changed between accesses.
template<class T> inline static T safemax(const T &a, const T &b)
{
	return a > b ? a : b;
}

STDMETHODIMP_(ULONG) CUnknown::DelegateAddRef()
{
	LONG lRef = InterlockedIncrement(&m_cRef);
	Q_ASSERT(lRef > 0);
	return safemax(ULONG(m_cRef), 1ul);
}

STDMETHODIMP_(ULONG) CUnknown::DelegateRelease()
{
	// If the reference count drops to zero delete ourselves
	LONG lRef = InterlockedDecrement(&m_cRef);
	Q_ASSERT(lRef >= 0);

	if(lRef == 0) {
		// COM rules say we must protect against re-entrancy. If we are an
		// aggregator and we hold our own interfaces on the aggregatee, the QI
		// for these interfaces will addref ourselves. So after doing the QI we
		// must release a ref count on ourselves. Then, before releasing the
		// private interface, we must addref ourselves. When we do this from
		// the destructor here it will result in the ref count going to 1 and
		// then back to 0 causing us to re-enter the destructor. Hence we add
		// an extra refcount here once we know we will delete the object.
		m_cRef++;

		delete this;
		return ULONG(0);
	} else {
		// Don't touch m_cRef again even in this leg as the object may have
		// just been released on another thread too
		return safemax(ULONG(lRef), 1ul);
	}
}

//=============================================================================
// CMediaType class

void WINAPI CMediaType::DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
	if(pmt == NULL)
		return;
	FreeMediaType(*pmt);
	CoTaskMemFree((PVOID)pmt);
}

AM_MEDIA_TYPE * WINAPI CMediaType::CreateMediaType(AM_MEDIA_TYPE const *pSrc)
{
	Q_ASSERT(pSrc);

	// Allocate a block of memory for the media type
	AM_MEDIA_TYPE *pMediaType =
		(AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));

	if(pMediaType == NULL)
		return NULL;

	// Copy the variable length format block
	HRESULT hr = CopyMediaType(pMediaType,pSrc);
	if(FAILED(hr)) {
		CoTaskMemFree((PVOID)pMediaType);
		return NULL;
	}

	return pMediaType;
}

HRESULT WINAPI CMediaType::CopyMediaType(
	AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource)
{
	// We'll leak if we copy onto one that already exists - there's one case we
	// can check like that - copying to itself.
	Q_ASSERT(pmtSource != pmtTarget);
	*pmtTarget = *pmtSource;
	if(pmtSource->cbFormat != 0) {
		Q_ASSERT(pmtSource->pbFormat != NULL);
		pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc(pmtSource->cbFormat);
		if (pmtTarget->pbFormat == NULL) {
			pmtTarget->cbFormat = 0;
			return E_OUTOFMEMORY;
		} else {
			CopyMemory((PVOID)pmtTarget->pbFormat, (PVOID)pmtSource->pbFormat,
				pmtTarget->cbFormat);
		}
	}
	if(pmtTarget->pUnk != NULL)
		pmtTarget->pUnk->AddRef();

	return S_OK;
}

void WINAPI CMediaType::FreeMediaType(AM_MEDIA_TYPE& mt)
{
	if(mt.cbFormat != 0) {
		CoTaskMemFree((PVOID)mt.pbFormat);

		// Strictly unnecessary but tidier
		mt.cbFormat = 0;
		mt.pbFormat = NULL;
	}
	if(mt.pUnk != NULL) {
		mt.pUnk->Release();
		mt.pUnk = NULL;
	}
}

CMediaType::CMediaType()
{
	InitMediaType();
}

CMediaType::CMediaType(const GUID * type)
{
	InitMediaType();
	majortype = *type;
}

// Copy constructor does a deep copy of the format block
CMediaType::CMediaType(const AM_MEDIA_TYPE& rt, HRESULT* phr)
{
	HRESULT hr = CopyMediaType(this, &rt);
	if(FAILED(hr) && (NULL != phr))
		*phr = hr;
}

CMediaType::CMediaType(const CMediaType& rt, HRESULT* phr)
{
	HRESULT hr = CopyMediaType(this, &rt);
	if(FAILED(hr) && (NULL != phr))
		*phr = hr;
}

CMediaType::~CMediaType()
{
	FreeMediaType(*this);
}

// this class inherits publicly from AM_MEDIA_TYPE so the compiler could
// generate the following assignment operator itself, however it could
// introduce some memory conflicts and leaks in the process because the
// structure contains a dynamically allocated block (pbFormat) which it will
// not copy correctly
CMediaType &CMediaType::operator=(const AM_MEDIA_TYPE& rt)
{
	Set(rt);
	return *this;
}

CMediaType &CMediaType::operator=(const CMediaType& rt)
{
	*this = (AM_MEDIA_TYPE &) rt;
	return *this;
}

BOOL CMediaType::operator==(const CMediaType& rt) const
{
	// I don't believe we need to check sample size or
	// temporal compression flags, since I think these must
	// be represented in the type, subtype and format somehow. They
	// are pulled out as separate flags so that people who don't understand
	// the particular format representation can still see them, but
	// they should duplicate information in the format block.
	return ((IsEqualGUID(majortype,rt.majortype) == TRUE) &&
		(IsEqualGUID(subtype,rt.subtype) == TRUE) &&
		(IsEqualGUID(formattype,rt.formattype) == TRUE) &&
		(cbFormat == rt.cbFormat) &&
		( (cbFormat == 0) ||
		pbFormat != NULL && rt.pbFormat != NULL &&
		(memcmp(pbFormat, rt.pbFormat, cbFormat) == 0)));
}

BOOL CMediaType::operator!=(const CMediaType& rt) const
{
	if(*this == rt)
		return FALSE;
	return TRUE;
}

HRESULT CMediaType::Set(const CMediaType& rt)
{
	return Set((AM_MEDIA_TYPE &)rt);
}

HRESULT CMediaType::Set(const AM_MEDIA_TYPE& rt)
{
	if(&rt != this) {
		FreeMediaType(*this);
		HRESULT hr = CopyMediaType(this, &rt);
		if(FAILED(hr))
			return E_OUTOFMEMORY;
	}
	return S_OK;
}

BOOL CMediaType::IsValid() const
{
	return (!IsEqualGUID(majortype,GUID_NULL));
}

void CMediaType::SetType(const GUID* ptype)
{
	majortype = *ptype;
}

void CMediaType::SetSubtype(const GUID* ptype)
{
	subtype = *ptype;
}

ULONG CMediaType::GetSampleSize() const
{
	if(IsFixedSize())
		return lSampleSize;
	return 0;
}

void CMediaType::SetSampleSize(ULONG sz)
{
	if(sz == 0)
		SetVariableSize();
	else {
		bFixedSizeSamples = TRUE;
		lSampleSize = sz;
	}
}

void CMediaType::SetVariableSize()
{
	bFixedSizeSamples = FALSE;
}

void CMediaType::SetTemporalCompression(BOOL bCompressed)
{
	bTemporalCompression = bCompressed;
}

BOOL CMediaType::SetFormat(BYTE *pformat, ULONG cb)
{
	if(NULL == AllocFormatBuffer(cb))
		return FALSE;
	Q_ASSERT(pbFormat);
	memcpy(pbFormat, pformat, cb);
	return TRUE;
}

// Set the type of the media type format block, this type defines what you
// will actually find in the format pointer. For example FORMAT_VideoInfo or
// FORMAT_WaveFormatEx. In the future this may be an interface pointer to a
// property set. Before sending out media types this should be filled in.
void CMediaType::SetFormatType(const GUID *pformattype)
{
	formattype = *pformattype;
}

void CMediaType::ResetFormatBuffer()
{
	if(cbFormat)
		CoTaskMemFree((PVOID)pbFormat);
	cbFormat = 0;
	pbFormat = NULL;
}

// Allocate length bytes for the format and return a read/write pointer
// If we cannot allocate the new block of memory we return NULL leaving
// the original block of memory untouched (as does ReallocFormatBuffer)
BYTE *CMediaType::AllocFormatBuffer(ULONG length)
{
	Q_ASSERT(length);
	if(cbFormat == length)
		return pbFormat;

	// allocate the new format buffer
	BYTE *pNewFormat = (PBYTE)CoTaskMemAlloc(length);
	if(pNewFormat == NULL) {
		if(length <= cbFormat)
			return pbFormat; // reuse the old block anyway.
		return NULL;
	}

	// delete the old format
	if(cbFormat != 0) {
		Q_ASSERT(pbFormat);
		CoTaskMemFree((PVOID)pbFormat);
	}

	cbFormat = length;
	pbFormat = pNewFormat;
	return pbFormat;
}

// Reallocate length bytes for the format and return a read/write pointer
// to it. We keep as much information as we can given the new buffer size
// if this fails the original format buffer is left untouched. The caller
// is responsible for ensuring the size of memory required is non zero
BYTE *CMediaType::ReallocFormatBuffer(ULONG length)
{
	Q_ASSERT(length);
	if(cbFormat == length)
		return pbFormat;

	// allocate the new format buffer
	BYTE *pNewFormat = (PBYTE)CoTaskMemAlloc(length);
	if(pNewFormat == NULL) {
		if(length <= cbFormat)
			return pbFormat; // Reuse the old block anyway.
		return NULL;
	}

	// copy any previous format (or part of if new is smaller)
	// delete the old format and replace with the new one
	if(cbFormat != 0) {
		Q_ASSERT(pbFormat);
		memcpy(pNewFormat, pbFormat, qMin(length, cbFormat));
		CoTaskMemFree((PVOID)pbFormat);
	}

	cbFormat = length;
	pbFormat = pNewFormat;
	return pNewFormat;
}

// initialise a media type structure
void CMediaType::InitMediaType()
{
	ZeroMemory((PVOID)this, sizeof(*this));
	lSampleSize = 1;
	bFixedSizeSamples = TRUE;
}

// a partially specified media type can be passed to IPin::Connect
// as a constraint on the media type used in the connection.
// the type, subtype or format type can be null.
BOOL CMediaType::IsPartiallySpecified() const
{
	if((majortype == GUID_NULL) ||
		(formattype == GUID_NULL))
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CMediaType::MatchesPartial(const CMediaType* ppartial) const
{
	if((ppartial->majortype != GUID_NULL) &&
		(majortype != ppartial->majortype))
	{
		return FALSE;
	}
	if((ppartial->subtype != GUID_NULL) &&
		(subtype != ppartial->subtype))
	{
		return FALSE;
	}

	if(ppartial->formattype != GUID_NULL) {
		// if the format block is specified then it must match exactly
		if(formattype != ppartial->formattype)
			return FALSE;
		if(cbFormat != ppartial->cbFormat)
			return FALSE;
		if((cbFormat != 0) &&
			(memcmp(pbFormat, ppartial->pbFormat, cbFormat) != 0))
		{
			return FALSE;
		}
	}

	return TRUE;
}

//=============================================================================

CEnumMediaTypes::CEnumMediaTypes(
	CBasePin *pPin, CEnumMediaTypes *pEnumMediaTypes)
	: m_Position(0)
	, m_pPin(pPin)
	, m_cRef(1)
{
	// We must be owned by a pin derived from CBasePin
	Q_ASSERT(pPin != NULL);

	// Hold a reference count on our pin
	m_pPin->AddRef();

	// Are we creating a new enumerator
	if(pEnumMediaTypes == NULL) {
		m_Version = m_pPin->GetMediaTypeVersion();
		return;
	}

	m_Position = pEnumMediaTypes->m_Position;
	m_Version = pEnumMediaTypes->m_Version;
}

/// <summary>
/// Destructor releases the reference count on our base pin. NOTE since we hold
/// a reference count on the pin who created us we know it is safe to release
/// it, no access can be made to it afterwards though as we might have just
/// caused the last reference count to go and the object to be deleted
/// </summary>
CEnumMediaTypes::~CEnumMediaTypes()
{
	m_pPin->Release();
}

STDMETHODIMP CEnumMediaTypes::QueryInterface(REFIID riid, void **ppv)
{
	if(ppv == NULL)
		return E_POINTER;

	if(riid == IID_IEnumMediaTypes || riid == IID_IUnknown)
		return GetInterface((IEnumMediaTypes *) this, ppv);

	*ppv = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumMediaTypes::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumMediaTypes::Release()
{
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if(cRef == 0)
		delete this;
	return cRef;
}

/// <summary>
/// One of an enumerator's basic member functions allows us to create a cloned
/// interface that initially has the same state. Since we are taking a snapshot
/// of an object (current position and all) we must lock access at the start
/// </summary>
STDMETHODIMP CEnumMediaTypes::Clone(IEnumMediaTypes **ppEnum)
{
	if(ppEnum == NULL)
		return E_POINTER;
	HRESULT hr = NOERROR;

	// Check we are still in sync with the pin
	if(AreWeOutOfSync() == TRUE) {
		*ppEnum = NULL;
		hr = VFW_E_ENUM_OUT_OF_SYNC;
	} else {
		*ppEnum = new CEnumMediaTypes(m_pPin, this);
		if(*ppEnum == NULL)
			hr =  E_OUTOFMEMORY;
	}
	return hr;
}

/// <summary>
/// Enumerate the next pin(s) after the current position. The client using this
/// interface passes in a pointer to an array of pointers each of which will
/// be filled in with a pointer to a fully initialised media type format
/// Return NOERROR if it all works,
/// S_FALSE if fewer than cMediaTypes were enumerated.
/// VFW_E_ENUM_OUT_OF_SYNC if the enumerator has been broken by
/// state changes in the filter
/// The actual count always correctly reflects the number of types in the array.
/// </summary>
STDMETHODIMP CEnumMediaTypes::Next(
	ULONG cMediaTypes, AM_MEDIA_TYPE **ppMediaTypes, ULONG *pcFetched)
{
	if(ppMediaTypes == NULL)
		return E_POINTER;

	// Check we are still in sync with the pin
	if(AreWeOutOfSync() == TRUE)
		return VFW_E_ENUM_OUT_OF_SYNC;

	if(pcFetched != NULL)
		*pcFetched = 0; // default unless we succeed
	else if(cMediaTypes > 1) // pcFetched == NULL
		return E_INVALIDARG;
	ULONG cFetched = 0; // increment as we get each one.

	// Return each media type by asking the filter for them in turn - If we
	// have an error code retured to us while we are retrieving a media type
	// we assume that our internal state is stale with respect to the filter
	// (for example the window size changing) so we return
	// VFW_E_ENUM_OUT_OF_SYNC
	while(cMediaTypes) {
		CMediaType cmt;

		HRESULT hr = m_pPin->GetMediaType(m_Position++, &cmt);
		if(S_OK != hr)
			break;

		// We now have a CMediaType object that contains the next media type
		// but when we assign it to the array position we CANNOT just assign
		// the AM_MEDIA_TYPE structure because as soon as the object goes out of
		// scope it will delete the memory we have just copied. The function
		// we use is CreateMediaType which allocates a task memory block

		// Transfer across the format block manually to save an allocate
		// and free on the format block and generally go faster
		*ppMediaTypes = (AM_MEDIA_TYPE *)CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
		if(*ppMediaTypes == NULL)
			break;

		// Do a regular copy
		**ppMediaTypes = cmt;

		// Make sure the destructor doesn't free these
		cmt.pbFormat = NULL;
		cmt.cbFormat = NULL;
		cmt.pUnk     = NULL;

		ppMediaTypes++;
		cFetched++;
		cMediaTypes--;
	}

	if(pcFetched != NULL)
		*pcFetched = cFetched;

	return (cMediaTypes == 0 ? NOERROR : S_FALSE);
}

/// <summary>
/// Skip over one or more entries in the enumerator
/// </summary>
STDMETHODIMP CEnumMediaTypes::Skip(ULONG cMediaTypes)
{
	// If we're skipping 0 elements we're guaranteed to skip the
	// correct number of elements
	if(cMediaTypes == 0)
		return S_OK;

	// Check we are still in sync with the pin
	if(AreWeOutOfSync() == TRUE)
		return VFW_E_ENUM_OUT_OF_SYNC;

	m_Position += cMediaTypes;

	// See if we're over the end
	CMediaType cmt;
	return S_OK == m_pPin->GetMediaType(m_Position - 1, &cmt) ? S_OK : S_FALSE;
}

/// <summary>
/// Set the current position back to the start
/// Reset has 3 simple steps:
///
/// set position to head of list
/// sync enumerator with object being enumerated
/// return S_OK
/// </summary>
STDMETHODIMP CEnumMediaTypes::Reset()
{
	m_Position = 0;

	// Bring the enumerator back into step with the current state.  This
	// may be a noop but ensures that the enumerator will be valid on the
	// next call.
	m_Version = m_pPin->GetMediaTypeVersion();
	return NOERROR;
}

/// <summary>
/// The media types a filter supports can be quite dynamic so we add to
/// the general IEnumXXXX interface the ability to be signaled when they
/// change via an event handle the connected filter supplies. Until the
/// Reset method is called after the state changes all further calls to
/// the enumerator (except Reset) will return E_UNEXPECTED error code
/// </summary>
BOOL CEnumMediaTypes::AreWeOutOfSync()
{
	return (m_pPin->GetMediaTypeVersion() == m_Version ? FALSE : TRUE);
}

//=============================================================================
// CBaseDispatch class

// HRESULT_FROM_WIN32 converts ERROR_SUCCESS to a success code, but in
// our use of HRESULT_FROM_WIN32, it typically means a function failed
// to call SetLastError(), and we still want a failure code.
#define AmHresultFromWin32(x) (MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, x))

CBaseDispatch::~CBaseDispatch()
{
	if(m_pti)
		m_pti->Release();
}

STDMETHODIMP CBaseDispatch::GetTypeInfoCount(UINT *pctinfo)
{
	if(pctinfo == NULL)
		return E_POINTER;
	*pctinfo = 1;
	return S_OK;
}

typedef HRESULT (STDAPICALLTYPE *LPLOADTYPELIB)(
	const OLECHAR FAR *szFile, ITypeLib FAR* FAR* pptlib);
typedef HRESULT (STDAPICALLTYPE *LPLOADREGTYPELIB)(
	REFGUID rguid, WORD wVerMajor, WORD wVerMinor, LCID lcid,
	ITypeLib FAR* FAR* pptlib);

STDMETHODIMP CBaseDispatch::GetTypeInfo(
	REFIID riid, UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
	if(pptinfo == NULL)
		return E_POINTER;
	HRESULT hr;

	*pptinfo = NULL;

	// We only support one type element
	if(0 != itinfo)
		return TYPE_E_ELEMENTNOTFOUND;

	if(NULL == pptinfo)
		return E_POINTER;

	// Always look for neutral
	if(NULL == m_pti) {
		LPLOADTYPELIB lpfnLoadTypeLib;
		LPLOADREGTYPELIB lpfnLoadRegTypeLib;
		ITypeLib *ptlib;
		HINSTANCE hInst;

		static const char  szTypeLib[]	  = "LoadTypeLib";
		static const char  szRegTypeLib[] = "LoadRegTypeLib";
		static const WCHAR szControl[]	  = L"control.tlb";

		// Try to get the Ole32Aut.dll module handle.
		hInst = LoadOLEAut32();
		if(hInst == NULL) {
			DWORD dwError = GetLastError();
			return AmHresultFromWin32(dwError);
		}
		lpfnLoadRegTypeLib =
			(LPLOADREGTYPELIB)GetProcAddress(hInst, szRegTypeLib);
		if(lpfnLoadRegTypeLib == NULL) {
			DWORD dwError = GetLastError();
			return AmHresultFromWin32(dwError);
		}

		hr = (*lpfnLoadRegTypeLib)(
			LIBID_QuartzTypeLib, 1, 0, lcid, &ptlib); // Version 1.0
		if(FAILED(hr)) {
			// attempt to load directly - this will fill the
			// registry in if it finds it

			lpfnLoadTypeLib = (LPLOADTYPELIB)GetProcAddress(hInst, szTypeLib);
			if(lpfnLoadTypeLib == NULL) {
				DWORD dwError = GetLastError();
				return AmHresultFromWin32(dwError);
			}

			hr = (*lpfnLoadTypeLib)(szControl, &ptlib);
			if(FAILED(hr))
				return hr;
		}

		hr = ptlib->GetTypeInfoOfGuid(riid, &m_pti);
		ptlib->Release();
		if(FAILED(hr))
			return hr;
	}

	*pptinfo = m_pti;
	m_pti->AddRef();
	return S_OK;
}

STDMETHODIMP CBaseDispatch::GetIDsOfNames(
	REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
	// Although the IDispatch riid is dead, we use this to pass from
	// the interface implementation class to us the iid we are talking about.
	ITypeInfo *pti;
	HRESULT hr = GetTypeInfo(riid, 0, lcid, &pti);
	if(SUCCEEDED(hr)) {
		hr = pti->GetIDsOfNames(rgszNames, cNames, rgdispid);
		pti->Release();
	}
	return hr;
}

//=============================================================================
// CMediaPosition class

CMediaPosition::CMediaPosition(LPCTSTR name, LPUNKNOWN pUnk)
	: CUnknown(name, pUnk)
{
}

CMediaPosition::CMediaPosition(LPCTSTR name, LPUNKNOWN pUnk, HRESULT *phr)
	: CUnknown(name, pUnk)
{
	Q_UNUSED(phr);
}

STDMETHODIMP CMediaPosition::DelegateQueryInterface(REFIID riid, void **ppv)
{
	if(riid == IID_IMediaPosition)
		return GetInterface((IMediaPosition *)this, ppv);
	return CUnknown::DelegateQueryInterface(riid, ppv);
}

/// <summary>
/// Returns 1 if we support GetTypeInfo
/// </summary>
STDMETHODIMP CMediaPosition::GetTypeInfoCount(UINT * pctinfo)
{
	return m_basedisp.GetTypeInfoCount(pctinfo);
}

/// <summary>
/// Attempt to find our type library
/// </summary>
STDMETHODIMP CMediaPosition::GetTypeInfo(
	UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
	return m_basedisp.GetTypeInfo(
		IID_IMediaPosition, itinfo, lcid, pptinfo);
}

STDMETHODIMP CMediaPosition::GetIDsOfNames(
	REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
	return m_basedisp.GetIDsOfNames(
		IID_IMediaPosition, rgszNames, cNames, lcid, rgdispid);
}

STDMETHODIMP CMediaPosition::Invoke(
	DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
	DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo,
	UINT *puArgErr)
{
	// This parameter is a dead leftover from an earlier interface
	if(IID_NULL != riid)
		return DISP_E_UNKNOWNINTERFACE;

	ITypeInfo *pti;
	HRESULT hr = GetTypeInfo(0, lcid, &pti);
	if(FAILED(hr))
		return hr;

	hr = pti->Invoke(
		(IMediaPosition *)this, dispidMember, wFlags, pdispparams, pvarResult,
		pexcepinfo, puArgErr);
	pti->Release();
	return hr;
}

//=============================================================================
// CPosPassThru class

CPosPassThru::CPosPassThru(
	LPCTSTR pName, LPUNKNOWN pUnk, HRESULT *phr, IPin *pPin)
	: CMediaPosition(pName, pUnk)
	, m_pPin(pPin)
{
	if(pPin == NULL) {
		*phr = E_POINTER;
		return;
	}
}

STDMETHODIMP CPosPassThru::DelegateQueryInterface(REFIID riid, void **ppv)
{
	if(ppv == NULL)
		return E_POINTER;
	*ppv = NULL;

	if(riid == IID_IMediaSeeking)
		return GetInterface(static_cast<IMediaSeeking *>(this), ppv);
	return CMediaPosition::DelegateQueryInterface(riid,ppv);
}

/// <summary>
/// Return the IMediaPosition interface from our peer
/// </summary>
HRESULT CPosPassThru::GetPeer(IMediaPosition **ppMP)
{
	*ppMP = NULL;

	IPin *pConnected;
	HRESULT hr = m_pPin->ConnectedTo(&pConnected);
	if(FAILED(hr))
		return E_NOTIMPL;
	IMediaPosition * pMP;
	hr = pConnected->QueryInterface(IID_IMediaPosition, (void **)&pMP);
	pConnected->Release();
	if(FAILED(hr))
		return E_NOTIMPL;

	*ppMP = pMP;
	return S_OK;
}

/// <summary>
/// Return the IMediaSeeking interface from our peer
/// </summary>
HRESULT CPosPassThru::GetPeerSeeking(IMediaSeeking **ppMS)
{
	*ppMS = NULL;

	IPin *pConnected;
	HRESULT hr = m_pPin->ConnectedTo(&pConnected);
	if(FAILED(hr))
		return E_NOTIMPL;
	IMediaSeeking * pMS;
	hr = pConnected->QueryInterface(IID_IMediaSeeking, (void **)&pMS);
	pConnected->Release();
	if(FAILED(hr))
		return E_NOTIMPL;

	*ppMS = pMS;
	return S_OK;
}

STDMETHODIMP CPosPassThru::GetCapabilities(DWORD * pCaps)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->GetCapabilities(pCaps);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::CheckCapabilities(DWORD *pCaps)
{
	IMediaSeeking *pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->CheckCapabilities(pCaps);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::IsFormatSupported(const GUID * pFormat)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->IsFormatSupported(pFormat);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::QueryPreferredFormat(GUID *pFormat)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->QueryPreferredFormat(pFormat);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::SetTimeFormat(const GUID * pFormat)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->SetTimeFormat(pFormat);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::GetTimeFormat(GUID *pFormat)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->GetTimeFormat(pFormat);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::IsUsingTimeFormat(const GUID * pFormat)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->IsUsingTimeFormat(pFormat);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::ConvertTimeFormat(
	LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source,
	const GUID * pSourceFormat)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->ConvertTimeFormat(pTarget, pTargetFormat, Source, pSourceFormat);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::SetPositions(
	LONGLONG * pCurrent, DWORD CurrentFlags, LONGLONG * pStop,
	DWORD StopFlags)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->SetPositions(pCurrent, CurrentFlags, pStop, StopFlags);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
{
	IMediaSeeking *pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->GetPositions(pCurrent, pStop);
	pMS->Release();
	return hr;
}

HRESULT CPosPassThru::GetSeekingLongLong(
	HRESULT (__stdcall IMediaSeeking::*pMethod)(LONGLONG *), LONGLONG *pll)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = (pMS->*pMethod)(pll);
	pMS->Release();
	return hr;
}

/// <summary>
/// If we don't have a current position then ask upstream
/// </summary>
STDMETHODIMP CPosPassThru::GetCurrentPosition(LONGLONG *pCurrent)
{
	// Can we report the current position
	HRESULT hr = GetMediaTime(pCurrent, NULL);
	if(SUCCEEDED(hr))
		hr = NOERROR;
	else
		hr = GetSeekingLongLong(&IMediaSeeking::GetCurrentPosition, pCurrent);
	return hr;
}

STDMETHODIMP CPosPassThru::GetStopPosition(LONGLONG *pStop)
{
	return GetSeekingLongLong(&IMediaSeeking::GetStopPosition, pStop);
}

STDMETHODIMP CPosPassThru::GetDuration(LONGLONG *pDuration)
{
	return GetSeekingLongLong(&IMediaSeeking::GetDuration, pDuration);
}

STDMETHODIMP CPosPassThru::GetPreroll(LONGLONG *pllPreroll)
{
	return GetSeekingLongLong(&IMediaSeeking::GetPreroll, pllPreroll);
}

STDMETHODIMP CPosPassThru::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->GetAvailable(pEarliest, pLatest);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::GetRate(double *pdRate)
{
	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->GetRate(pdRate);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::SetRate(double dRate)
{
	if(0.0 == dRate)
		return E_INVALIDARG;

	IMediaSeeking* pMS;
	HRESULT hr = GetPeerSeeking(&pMS);
	if(FAILED(hr))
		return hr;

	hr = pMS->SetRate(dRate);
	pMS->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::get_Duration(REFTIME * plength)
{
	IMediaPosition *pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->get_Duration(plength);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::get_CurrentPosition(REFTIME * pllTime)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->get_CurrentPosition(pllTime);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::put_CurrentPosition(REFTIME llTime)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->put_CurrentPosition(llTime);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::get_StopTime(REFTIME * pllTime)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->get_StopTime(pllTime);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::put_StopTime(REFTIME llTime)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->put_StopTime(llTime);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::get_PrerollTime(REFTIME * pllTime)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->get_PrerollTime(pllTime);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::put_PrerollTime(REFTIME llTime)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->put_PrerollTime(llTime);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::get_Rate(double * pdRate)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->get_Rate(pdRate);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::put_Rate(double dRate)
{
	if(0.0 == dRate)
		return E_INVALIDARG;

	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->put_Rate(dRate);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::CanSeekForward(LONG *pCanSeekForward)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->CanSeekForward(pCanSeekForward);
	pMP->Release();
	return hr;
}

STDMETHODIMP CPosPassThru::CanSeekBackward(LONG *pCanSeekBackward)
{
	IMediaPosition* pMP;
	HRESULT hr = GetPeer(&pMP);
	if(FAILED(hr))
		return hr;

	hr = pMP->CanSeekBackward(pCanSeekBackward);
	pMP->Release();
	return hr;
}

//=============================================================================
// CRendererPosPassThru class

// Media times (eg current frame, field, sample etc) are passed through the
// filtergraph in media samples. When a renderer gets a sample with media
// times in it, it will call one of the RegisterMediaTime methods we expose
// (one takes an IMediaSample, the other takes the media times direct). We
// store the media times internally and return them in GetCurrentPosition.

CRendererPosPassThru::CRendererPosPassThru(
	LPCTSTR pName, LPUNKNOWN pUnk, HRESULT *phr, IPin *pPin)
	: CPosPassThru(pName, pUnk, phr, pPin)
	, m_StartMedia(0)
	, m_EndMedia(0)
	, m_bReset(TRUE)
{
}

/// <summary>
/// Sets the media times the object should report
/// </summary>
HRESULT CRendererPosPassThru::RegisterMediaTime(IMediaSample *pMediaSample)
{
	Q_ASSERT(pMediaSample);
	LONGLONG StartMedia;
	LONGLONG EndMedia;

	CAutoLock cAutoLock(&m_PositionLock);

	// Get the media times from the sample
	HRESULT hr = pMediaSample->GetTime(&StartMedia,&EndMedia);
	if(FAILED(hr)) {
		Q_ASSERT(hr == VFW_E_SAMPLE_TIME_NOT_SET);
		return hr;
	}

	m_StartMedia = StartMedia;
	m_EndMedia = EndMedia;
	m_bReset = FALSE;
	return NOERROR;
}

/// <summary>
/// Sets the media times the object should report
/// </summary>
HRESULT CRendererPosPassThru::RegisterMediaTime(
	LONGLONG StartTime, LONGLONG EndTime)
{
	CAutoLock cAutoLock(&m_PositionLock);
	m_StartMedia = StartTime;
	m_EndMedia = EndTime;
	m_bReset = FALSE;
	return NOERROR;
}

/// <summary>
/// Return the current media times registered in the object
/// </summary>
HRESULT CRendererPosPassThru::GetMediaTime(
	LONGLONG *pStartTime, LONGLONG *pEndTime)
{
	Q_ASSERT(pStartTime);

	CAutoLock cAutoLock(&m_PositionLock);
	if(m_bReset == TRUE)
		return E_FAIL;

	// We don't have to return the end time
	HRESULT hr = ConvertTimeFormat(
		pStartTime, 0, m_StartMedia, &TIME_FORMAT_MEDIA_TIME);
	if(pEndTime && SUCCEEDED(hr)) {
		hr = ConvertTimeFormat(
			pEndTime, 0, m_EndMedia, &TIME_FORMAT_MEDIA_TIME);
	}
	return hr;
}

/// <summary>
/// Resets the media times we hold
/// </summary>
HRESULT CRendererPosPassThru::ResetMediaTime()
{
	CAutoLock cAutoLock(&m_PositionLock);
	m_StartMedia = 0;
	m_EndMedia = 0;
	m_bReset = TRUE;
	return NOERROR;
}

/// <summary>
/// Intended to be called by the owing filter during EOS processing so
/// that the media times can be adjusted to the stop time.  This ensures
/// that the GetCurrentPosition will actully get to the stop position.
/// </summary>
HRESULT CRendererPosPassThru::EOS()
{
	HRESULT hr;
	if(m_bReset == TRUE)
		hr = E_FAIL;
	else {
		LONGLONG llStop;
		hr = GetStopPosition(&llStop);
		if(SUCCEEDED(hr)) {
			CAutoLock cAutoLock(&m_PositionLock);
			m_StartMedia = llStop;
			m_EndMedia = llStop;
		}
	}
	return hr;
}

//=============================================================================
// CEnumPins class

CEnumPins::CEnumPins(CBaseFilter *pFilter, CEnumPins *pEnumPins)
	: m_Position(0)
	, m_PinCount(0)
	, m_pFilter(pFilter)
	, m_cRef(1) // Already ref counted
	, m_PinCache()
{
	// We must be owned by a filter derived from CBaseFilter
	Q_ASSERT(pFilter != NULL);

	// Hold a reference count on our filter
	m_pFilter->AddRef();

	// Are we creating a new enumerator
	if(pEnumPins == NULL) {
		m_Version = m_pFilter->GetPinVersion();
		m_PinCount = m_pFilter->GetPinCount();
	} else {
		Q_ASSERT(m_Position <= m_PinCount);
		m_Position = pEnumPins->m_Position;
		m_PinCount = pEnumPins->m_PinCount;
		m_Version = pEnumPins->m_Version;
		m_PinCache = pEnumPins->m_PinCache;
	}
}

/// <summary>
/// Destructor releases the reference count on our filter NOTE since we hold
/// a reference count on the filter who created us we know it is safe to
/// release it, no access can be made to it afterwards though as we have just
/// caused the last reference count to go and the object to be deleted
/// </summary>
CEnumPins::~CEnumPins()
{
	m_pFilter->Release();
}

STDMETHODIMP CEnumPins::QueryInterface(REFIID riid, void **ppv)
{
	if(ppv == NULL)
		return E_POINTER;

	if(riid == IID_IEnumPins || riid == IID_IUnknown)
		return GetInterface((IEnumPins *)this, ppv);

	*ppv = NULL;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CEnumPins::AddRef()
{
	return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) CEnumPins::Release()
{
	ULONG cRef = InterlockedDecrement(&m_cRef);
	if(cRef == 0)
		delete this;
	return cRef;
}

/// <summary>
/// One of an enumerator's basic member functions allows us to create a cloned
/// interface that initially has the same state. Since we are taking a snapshot
/// of an object (current position and all) we must lock access at the start
/// </summary>
STDMETHODIMP CEnumPins::Clone(IEnumPins **ppEnum)
{
	if(ppEnum == NULL)
		return E_POINTER;
	HRESULT hr = NOERROR;

	// Check we are still in sync with the filter
	if(AreWeOutOfSync() == TRUE) {
		*ppEnum = NULL;
		hr =  VFW_E_ENUM_OUT_OF_SYNC;
	} else {
		*ppEnum = new CEnumPins(m_pFilter, this);
		if(*ppEnum == NULL)
			hr = E_OUTOFMEMORY;
	}
	return hr;
}

/// <summary>
/// Return the next pin after the current position
/// </summary>
STDMETHODIMP CEnumPins::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
	if(ppPins == NULL)
		return E_POINTER;

	Q_ASSERT(ppPins);

	if(pcFetched != NULL)
		*pcFetched = 0; // default unless we succeed
	else if(cPins > 1) // pcFetched == NULL
		return E_INVALIDARG;
	ULONG cFetched = 0; // increment as we get each one.

	// Check we are still in sync with the filter
	if(AreWeOutOfSync() == TRUE) {
		// If we are out of sync, we should refresh the enumerator.
		// This will reset the position and update the other members, but
		// will not clear cache of pins we have already returned.
		Refresh();
	}

	// Return each pin interface NOTE GetPin returns CBasePin * not addrefed
	// so we must QI for the IPin (which increments its reference count)
	// If while we are retrieving a pin from the filter an error occurs we
	// assume that our internal state is stale with respect to the filter
	// (for example someone has deleted a pin) so we
	// return VFW_E_ENUM_OUT_OF_SYNC

	while(cFetched < cPins && m_PinCount > m_Position) {
		// Get the next pin object from the filter

		CBasePin *pPin = m_pFilter->GetPin(m_Position++);
		if(pPin == NULL) {
			// If this happend, and it's not the first time through, then we've
			// got a problem, since we should really go back and release the
			// iPins, which we have previously AddRef'ed.
			Q_ASSERT(cFetched == 0);
			return VFW_E_ENUM_OUT_OF_SYNC;
		}

		// We only want to return this pin, if it is not in our cache
		if(m_PinCache.indexOf(pPin) < 0) {
			// From the object get an IPin interface
			*ppPins = pPin;
			pPin->AddRef();

			cFetched++;
			ppPins++;

			m_PinCache.append(pPin);
		}
	}

	if(pcFetched!=NULL)
		*pcFetched = cFetched;

	return (cPins==cFetched ? NOERROR : S_FALSE);
}

/// <summary>
/// Skip over one or more entries in the enumerator
/// </summary>
STDMETHODIMP CEnumPins::Skip(ULONG cPins)
{
	// Check we are still in sync with the filter
	if(AreWeOutOfSync() == TRUE)
		return VFW_E_ENUM_OUT_OF_SYNC;

	// Work out how many pins are left to skip over
	// We could position at the end if we are asked to skip too many...
	// ..which would match the base implementation for CEnumMediaTypes::Skip
	ULONG PinsLeft = m_PinCount - m_Position;
	if(cPins > PinsLeft)
		return S_FALSE;
	m_Position += cPins;
	return NOERROR;
}

/// <summary>
/// Set the current position back to the start
/// Reset has 4 simple steps:
///
/// Set position to head of list
/// Sync enumerator with object being enumerated
/// Clear the cache of pins already returned
/// return S_OK
/// </summary>
STDMETHODIMP CEnumPins::Reset()
{
	m_Version = m_pFilter->GetPinVersion();
	m_PinCount = m_pFilter->GetPinCount();

	m_Position = 0;

	// Clear the cache
	m_PinCache.clear();

	return S_OK;
}

BOOL CEnumPins::AreWeOutOfSync()
{
	return (m_pFilter->GetPinVersion() == m_Version ? FALSE : TRUE);
}

/// <summary>
/// Set the current position back to the start
/// Refresh has 3 simple steps:
///
/// Set position to head of list
/// Sync enumerator with object being enumerated
/// return S_OK
/// </summary>
STDMETHODIMP CEnumPins::Refresh()
{
	m_Version = m_pFilter->GetPinVersion();
	m_PinCount = m_pFilter->GetPinCount();

	m_Position = 0;
	return S_OK;
}

//=============================================================================
// CBasePin class

CBasePin::CBasePin(
	LPCTSTR pObjectName, CBaseFilter *pFilter, CCritSec *pLock, HRESULT *phr,
	LPCWSTR pName, PIN_DIRECTION dir)
	: CUnknown(pObjectName, NULL)
	, m_pFilter(pFilter)
	, m_pLock(pLock)
	, m_pName(NULL)
	, m_Connected(NULL)
	, m_dir(dir)
	, m_bRunTimeError(FALSE)
	, m_pQSink(NULL)
	, m_TypeVersion(1)
	, m_tStart()
	, m_tStop(MAX_TIME)
	, m_bCanReconnectWhenActive(false)
	, m_bTryMyTypesFirst(false)
	, m_dRate(1.0)
{
	// WARNING - pFilter is often not a properly constituted object at this
	// state (in particular QueryInterface may not work) - this is because its
	// owner is often its containing object and we have been called from the
	// containing object's constructor so the filter's owner has not yet had
	// its CUnknown constructor called
	Q_ASSERT(pFilter != NULL);
	Q_ASSERT(pLock != NULL);

	if(pName) {
		size_t cchName;
		HRESULT hr = StringCchLengthW(pName, STRSAFE_MAX_CCH, &cchName);
		if(SUCCEEDED(hr)) {
			m_pName = new WCHAR[cchName + 1];
			if(m_pName)
				StringCchCopyW(m_pName, cchName + 1, pName);
		}
	}
}

CBasePin::CBasePin(
	LPCSTR pObjectName, CBaseFilter *pFilter, CCritSec *pLock, HRESULT *phr,
	LPCWSTR pName, PIN_DIRECTION dir)
	: CUnknown(pObjectName, NULL)
	, m_pFilter(pFilter)
	, m_pLock(pLock)
	, m_pName(NULL)
	, m_Connected(NULL)
	, m_dir(dir)
	, m_bRunTimeError(FALSE)
	, m_pQSink(NULL)
	, m_TypeVersion(1)
	, m_tStart()
	, m_tStop(MAX_TIME)
	, m_bCanReconnectWhenActive(false)
	, m_bTryMyTypesFirst(false)
	, m_dRate(1.0)
{
	// WARNING - pFilter is often not a properly constituted object at this
	// state (in particular QueryInterface may not work) - this is because its
	// owner is often its containing object and we have been called from the
	// containing object's constructor so the filter's owner has not yet had
	// its CUnknown constructor called
	Q_ASSERT(pFilter != NULL);
	Q_ASSERT(pLock != NULL);

	if(pName) {
		size_t cchName;
		HRESULT hr = StringCchLengthW(pName, STRSAFE_MAX_CCH, &cchName);
		if(SUCCEEDED(hr)) {
			m_pName = new WCHAR[cchName + 1];
			if(m_pName)
				StringCchCopyW(m_pName, cchName + 1, pName);
		}
	}
}

/// <summary>
/// Destructor since a connected pin holds a reference count on us there is
/// no way that we can be deleted unless we are not currently connected.
/// </summary>
CBasePin::~CBasePin()
{
	// We don't call disconnect because if the filter is going away
	// all the pins must have a reference count of zero so they must
	// have been disconnected anyway - (but check the assumption)
	Q_ASSERT(m_Connected == FALSE);

	delete[] m_pName;

	// check the internal reference count is consistent
	Q_ASSERT(m_cRef == 0);
}

STDMETHODIMP CBasePin::DelegateQueryInterface(REFIID riid, void **ppv)
{
	if(riid == IID_IPin)
		return GetInterface((IPin *)this, ppv);
	else if(riid == IID_IQualityControl)
		return GetInterface((IQualityControl *)this, ppv);
	return CUnknown::DelegateQueryInterface(riid, ppv);
}

STDMETHODIMP_(ULONG) CBasePin::DelegateAddRef()
{
	LONG tmp = InterlockedIncrement(&m_cRef);
	Q_ASSERT(tmp > 0);
	return m_pFilter->AddRef();
}

/// <summary>
/// Override to decrement the owning filter's reference count
/// </summary>
STDMETHODIMP_(ULONG) CBasePin::DelegateRelease()
{
	LONG tmp = InterlockedDecrement(&m_cRef);
	Q_ASSERT(tmp >= 0);
	return m_pFilter->Release();
}

BOOL CBasePin::IsStopped()
{
	return (m_pFilter->m_State == State_Stopped);
}

/// <summary>
/// Asked to connect to a pin. A pin is always attached to an owning filter
/// object so we always delegate our locking to that object. We first of all
/// retrieve a media type enumerator for the input pin and see if we accept
/// any of the formats that it would ideally like, failing that we retrieve
/// our enumerator and see if it will accept any of our preferred types
/// </summary>
STDMETHODIMP CBasePin::Connect(
	IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
	if(pReceivePin == NULL)
		return E_POINTER;
	CAutoLock cObjectLock(m_pLock);
	DisplayPinInfo(pReceivePin);

	// See if we are already connected
	if(m_Connected) {
		appLog(LOG_CAT, Log::Warning) << "Already connected";
		return VFW_E_ALREADY_CONNECTED;
	}

	// See if the filter is active
	if(!IsStopped() && !m_bCanReconnectWhenActive) {
		return VFW_E_NOT_STOPPED;
	}

	// Find a mutually agreeable media type -
	// Pass in the template media type. If this is partially specified,
	// each of the enumerated media types will need to be checked against
	// it. If it is non-null and fully specified, we will just try to connect
	// with this.
	const CMediaType * ptype = (CMediaType*)pmt;
	HRESULT hr = AgreeMediaType(pReceivePin, ptype);
	if(FAILED(hr)) {
		appLog(LOG_CAT, Log::Warning) << "Failed to agree on media type";

		// Since the procedure is already returning an error code, there
		// is nothing else this function can do to report the error.
		BOOL suc = SUCCEEDED(BreakConnect());
		Q_ASSERT(suc);

		return hr;
	}

	//appLog(LOG_CAT) << "Connection succeeded";

	return NOERROR;
}

/// <summary>
/// Given a specific media type, attempt a connection (includes checking that
/// the type is acceptable to this pin)
/// </summary>
HRESULT CBasePin::AttemptConnection(
	IPin* pReceivePin, const CMediaType* pmt)
{
	// Check that the connection is valid  -- need to do this for every
	// connect attempt since BreakConnect will undo it.
	HRESULT hr = CheckConnect(pReceivePin);
	if(FAILED(hr)) {
		appLog(LOG_CAT, Log::Warning) << "CheckConnect failed";

		// Since the procedure is already returning an error code, there
		// is nothing else this function can do to report the error.
		BOOL suc = SUCCEEDED(BreakConnect());
		Q_ASSERT(suc);

		return hr;
	}

	DisplayTypeInfo(pReceivePin, pmt);

	// Check we will accept this media type
	hr = CheckMediaType(pmt);
	if(hr == NOERROR) {
		// Make ourselves look connected otherwise ReceiveConnection may not be
		// able to complete the connection
		m_Connected = pReceivePin;
		m_Connected->AddRef();
		hr = SetMediaType(pmt);
		if(SUCCEEDED(hr)) {
			// See if the other pin will accept this type
			hr = pReceivePin->ReceiveConnection((IPin *)this, pmt);
			if(SUCCEEDED(hr)) {
				// Complete the connection
				hr = CompleteConnect(pReceivePin);
				if(SUCCEEDED(hr))
					return hr;
				else {
					appLog(LOG_CAT, Log::Warning)
						<< "Failed to complete connection";
					pReceivePin->Disconnect();
				}
			}
		}
	} else {
		// We cannot use this media type

		// Return a specific media type error if there is one
		// or map a general failure code to something more helpful
		// (in particular S_FALSE gets changed to an error code)
		if (SUCCEEDED(hr) ||
			(hr == E_FAIL) ||
			(hr == E_INVALIDARG)) {
				hr = VFW_E_TYPE_NOT_ACCEPTED;
		}
	}

	// BreakConnect and release any connection here in case CheckMediaType
	// failed, or if we set anything up during a call back during
	// ReceiveConnection.

	// Since the procedure is already returning an error code, there
	// is nothing else this function can do to report the error.
	BOOL suc = SUCCEEDED(BreakConnect());
	Q_ASSERT(suc);

	// If failed then undo our state
	if(m_Connected) {
		m_Connected->Release();
		m_Connected = NULL;
	}

	return hr;
}

/// <summary>
/// Given an enumerator we cycle through all the media types it proposes and
/// firstly suggest them to our derived pin class and if that succeeds try
/// them with the pin in a ReceiveConnection call. This means that if our pin
/// proposes a media type we still check in here that we can support it. This
/// is deliberate so that in simple cases the enumerator can hold all of the
/// media types even if some of them are not really currently available
/// </summary>
HRESULT CBasePin::TryMediaTypes(
	IPin *pReceivePin, const CMediaType *pmt, IEnumMediaTypes *pEnum)
{
	// Reset the current enumerator position
	HRESULT hr = pEnum->Reset();
	if(FAILED(hr))
		return hr;

	CMediaType *pMediaType = NULL;
	ULONG ulMediaCount = 0;

	// attempt to remember a specific error code if there is one
	HRESULT hrFailure = S_OK;

	for(;;) {
		// Retrieve the next media type NOTE each time round the loop the
		// enumerator interface will allocate another AM_MEDIA_TYPE structure
		// If we are successful then we copy it into our output object, if
		// not then we must delete the memory allocated before returning

		hr = pEnum->Next(1, (AM_MEDIA_TYPE**)&pMediaType,&ulMediaCount);
		if(hr != S_OK) {
			if(S_OK == hrFailure)
				hrFailure = VFW_E_NO_ACCEPTABLE_TYPES;
			return hrFailure;
		}

		Q_ASSERT(ulMediaCount == 1);
		Q_ASSERT(pMediaType);

		// Check that this matches the partial type (if any)
		if(pMediaType &&
			((pmt == NULL) ||
			pMediaType->MatchesPartial(pmt)))
		{
			hr = AttemptConnection(pReceivePin, pMediaType);

			// Attempt to remember a specific error code
			if (FAILED(hr) &&
				SUCCEEDED(hrFailure) &&
				(hr != E_FAIL) &&
				(hr != E_INVALIDARG) &&
				(hr != VFW_E_TYPE_NOT_ACCEPTED))
			{
				hrFailure = hr;
			}
		} else {
			hr = VFW_E_NO_ACCEPTABLE_TYPES;
		}

		if(pMediaType) {
			CMediaType::DeleteMediaType(pMediaType);
			pMediaType = NULL;
		}

		if(S_OK == hr)
			return hr;
	}
}

/// <summary>
/// This is called to make the connection, including the taask of finding
/// a media type for the pin connection. pmt is the proposed media type
/// from the Connect call: if this is fully specified, we will try that.
/// Otherwise we enumerate and try all the input pin's types first and
/// if that fails we then enumerate and try all our preferred media types.
/// For each media type we check it against pmt (if non-null and partially
/// specified) as well as checking that both pins will accept it.
/// </summary>
HRESULT CBasePin::AgreeMediaType(
	IPin *pReceivePin, const CMediaType *pmt)
{
	Q_ASSERT(pReceivePin);
	IEnumMediaTypes *pEnumMediaTypes = NULL;

	// If the media type is fully specified then use that
	if((pmt != NULL) && (!pmt->IsPartiallySpecified())) {
		// If this media type fails, then we must fail the connection
		// since if pmt is nonnull we are only allowed to connect
		// using a type that matches it.
		return AttemptConnection(pReceivePin, pmt);
	}

	// Try the other pin's enumerator
	HRESULT hrFailure = VFW_E_NO_ACCEPTABLE_TYPES;
	for(int i = 0; i < 2; i++) {
		HRESULT hr;
		if(i == (int)m_bTryMyTypesFirst)
			hr = pReceivePin->EnumMediaTypes(&pEnumMediaTypes);
		else
			hr = EnumMediaTypes(&pEnumMediaTypes);
		if(FAILED(hr))
			continue;

		Q_ASSERT(pEnumMediaTypes);
		hr = TryMediaTypes(pReceivePin,pmt,pEnumMediaTypes);
		pEnumMediaTypes->Release();
		if(SUCCEEDED(hr))
			return NOERROR;

		// Try to remember specific error codes if there are any
		if((hr != E_FAIL) &&
			(hr != E_INVALIDARG) &&
			(hr != VFW_E_TYPE_NOT_ACCEPTED))
		{
			hrFailure = hr;
		}
	}

	return hrFailure;
}

/// <summary>
/// Called when we want to complete a connection to another filter. Failing
/// this will also fail the connection and disconnect the other pin as well
/// </summary>
HRESULT CBasePin::CompleteConnect(IPin *pReceivePin)
{
	Q_UNUSED(pReceivePin);
	return NOERROR;
}

/// <summary>
/// This is called to set the format for a pin connection - CheckMediaType
/// will have been called to check the connection format and if it didn't
/// return an error code then this (virtual) function will be invoked
/// </summary>
HRESULT CBasePin::SetMediaType(const CMediaType *pmt)
{
	HRESULT hr = m_mt.Set(*pmt);
	if(FAILED(hr))
		return hr;
	return NOERROR;
}

/// <summary>
/// This is called during Connect() to provide a virtual method that can do
/// any specific check needed for connection such as QueryInterface. This
/// base class method just checks that the pin directions don't match
/// </summary>
HRESULT CBasePin::CheckConnect(IPin * pPin)
{
	// Check that pin directions DON'T match
	PIN_DIRECTION pd;
	pPin->QueryDirection(&pd);

	Q_ASSERT((pd == PINDIR_OUTPUT) || (pd == PINDIR_INPUT));
	Q_ASSERT((m_dir == PINDIR_OUTPUT) || (m_dir == PINDIR_INPUT));

	// We should allow for non-input and non-output connections?
	if(pd == m_dir)
		return VFW_E_INVALID_DIRECTION;
	return NOERROR;
}

/// <summary>
/// This is called when we realise we can't make a connection to the pin and
/// must undo anything we did in CheckConnect - override to release QIs done
/// </summary>
HRESULT CBasePin::BreakConnect()
{
	return NOERROR;
}

/// <summary>
/// Called normally by an output pin on an input pin to try and establish a
/// connection.
/// </summary>
STDMETHODIMP CBasePin::ReceiveConnection(
	IPin *pConnector, const AM_MEDIA_TYPE *pmt)
{
	if(pConnector == NULL)
		return E_POINTER;
	if(pmt == NULL)
		return E_POINTER;
	CAutoLock cObjectLock(m_pLock);

	// Are we already connected
	if(m_Connected)
		return VFW_E_ALREADY_CONNECTED;

	// See if the filter is active
	if(!IsStopped() && !m_bCanReconnectWhenActive)
		return VFW_E_NOT_STOPPED;

	HRESULT hr = CheckConnect(pConnector);
	if(FAILED(hr)) {
		// Since the procedure is already returning an error code, there
		// is nothing else this function can do to report the error.
		BOOL suc = SUCCEEDED(BreakConnect());
		Q_ASSERT(suc);

		return hr;
	}

	// Ask derived class if this media type is ok
	CMediaType *pcmt = (CMediaType*)pmt;
	hr = CheckMediaType(pcmt);
	if(hr != NOERROR) {
		// no -we don't support this media type

		// Since the procedure is already returning an error code, there
		// is nothing else this function can do to report the error.
		BOOL suc = SUCCEEDED(BreakConnect());
		Q_ASSERT(suc);

		// return a specific media type error if there is one
		// or map a general failure code to something more helpful
		// (in particular S_FALSE gets changed to an error code)
		if(SUCCEEDED(hr) ||
			(hr == E_FAIL) ||
			(hr == E_INVALIDARG))
		{
			hr = VFW_E_TYPE_NOT_ACCEPTED;
		}

		return hr;
	}

	// Complete the connection
	m_Connected = pConnector;
	m_Connected->AddRef();
	hr = SetMediaType(pcmt);
	if(SUCCEEDED(hr)) {
		hr = CompleteConnect(pConnector);
		if(SUCCEEDED(hr))
			return NOERROR;
	}

	appLog(LOG_CAT, Log::Warning)
		<< "Failed to set the media type or failed to complete the connection";
	m_Connected->Release();
	m_Connected = NULL;

	// Since the procedure is already returning an error code, there
	// is nothing else this function can do to report the error.
	BOOL suc = SUCCEEDED(BreakConnect());
	Q_ASSERT(suc);

	return hr;
}

/// <summary>
/// Called when we want to terminate a pin connection.
/// </summary>
STDMETHODIMP CBasePin::Disconnect()
{
	CAutoLock cObjectLock(m_pLock);

	// See if the filter is active
	if(!IsStopped())
		return VFW_E_NOT_STOPPED;

	return DisconnectInternal();
}

STDMETHODIMP CBasePin::DisconnectInternal()
{
	if(!m_Connected) {
		// No connection - not an error
		return S_FALSE;
	}

	HRESULT hr = BreakConnect();
	if(FAILED(hr)) {
		// There is usually a bug in the program if BreakConnect() fails.
		appLog(LOG_CAT, Log::Warning)
			<< "BreakConnect() failed in CBasePin::Disconnect()";
		return hr;
	}

	m_Connected->Release();
	m_Connected = NULL;

	return S_OK;
}

/// <summary>
/// Return an AddRef()'d pointer to the connected pin if there is one
/// </summary>
STDMETHODIMP CBasePin::ConnectedTo(IPin **ppPin)
{
	if(ppPin == NULL)
		return E_POINTER;
	//
	//  It's pointless to lock here.
	//  The caller should ensure integrity.
	//

	IPin *pPin = m_Connected;
	*ppPin = pPin;
	if(pPin != NULL) {
		pPin->AddRef();
		return S_OK;
	} else {
		Q_ASSERT(*ppPin == NULL);
		return VFW_E_NOT_CONNECTED;
	}
}

/// <summary>
/// Return the media type of the connection
/// </summary>
STDMETHODIMP CBasePin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
	if(pmt == NULL)
		return E_POINTER;
	CAutoLock cObjectLock(m_pLock);

	// Copy constructor of m_mt allocates the memory
	if(IsConnected()) {
		CMediaType::CopyMediaType(pmt, &m_mt);
		return S_OK;
	} else {
		((CMediaType *)pmt)->InitMediaType();
		return VFW_E_NOT_CONNECTED;
	}
}

/// <summary>
/// Return information about the filter we are connect to
/// </summary>
STDMETHODIMP CBasePin::QueryPinInfo(PIN_INFO * pInfo)
{
	if(pInfo == NULL)
		return E_POINTER;

	pInfo->pFilter = m_pFilter;
	if(m_pFilter)
		m_pFilter->AddRef();

	if(m_pName)
		StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	else
		pInfo->achName[0] = L'\0';

	pInfo->dir = m_dir;
	return NOERROR;
}

STDMETHODIMP CBasePin::QueryDirection(PIN_DIRECTION *pPinDir)
{
	if(pPinDir == NULL)
		return E_POINTER;
	*pPinDir = m_dir;
	return NOERROR;
}

/// <summary>
/// Default QueryId to return the pin's name
/// </summary>
STDMETHODIMP CBasePin::QueryId(LPWSTR *Id)
{
	// We're not going away because someone's got a pointer to us
	// so there's no need to lock
	return AMGetWideString(Name(), Id);
}

/// <summary>
/// Does this pin support this media type WARNING this interface function does
/// not lock the main object as it is meant to be asynchronous by nature - if
/// the media types you support depend on some internal state that is updated
/// dynamically then you will need to implement locking in a derived class
/// </summary>
STDMETHODIMP CBasePin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
	if(pmt == NULL)
		return E_POINTER;

	// The CheckMediaType method is valid to return error codes if the media
	// type is horrible, an example might be E_INVALIDARG. What we do here
	// is map all the error codes into either S_OK or S_FALSE regardless
	HRESULT hr = CheckMediaType((CMediaType*)pmt);
	if(FAILED(hr))
		return S_FALSE;

	// Note that the only defined success codes should be S_OK and S_FALSE...
	return hr;
}

/// <summary>
/// This can be called to return an enumerator for the pin's list of preferred
/// media types. An input pin is not obliged to have any preferred formats
/// although it can do. For example, the window renderer has a preferred type
/// which describes a video image that matches the current window size. All
/// output pins should expose at least one preferred format otherwise it is
/// possible that neither pin has any types and so no connection is possible
/// </summary>
STDMETHODIMP CBasePin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
	if(ppEnum == NULL)
		return E_POINTER;

	// Create a new ref counted enumerator
	*ppEnum = new CEnumMediaTypes(this, NULL);
	if(*ppEnum == NULL)
		return E_OUTOFMEMORY;
	return NOERROR;
}

/// <summary>
/// This is a virtual function that returns a media type corresponding with
/// place iPosition in the list. This base class simply returns an error as
/// we support no media types by default but derived classes should override
/// </summary>
HRESULT CBasePin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	Q_UNUSED(iPosition);
	Q_UNUSED(pMediaType);
	return E_UNEXPECTED;
}

/// <summary>
/// This is a virtual function that returns the current media type version.
/// The base class initialises the media type enumerators with the value 1
/// By default we always returns that same value. A Derived class may change
/// the list of media types available and after doing so it should increment
/// the version either in a method derived from this, or more simply by just
/// incrementing the m_TypeVersion base pin variable. The type enumerators
/// call this when they want to see if their enumerations are out of date
/// </summary>
LONG CBasePin::GetMediaTypeVersion()
{
	return m_TypeVersion;
}

/// <summary>
/// Increment the cookie representing the current media type version
/// </summary>
void CBasePin::IncrementTypeVersion()
{
	InterlockedIncrement(&m_TypeVersion);
}

/// <summary>
/// Called by IMediaFilter implementation when the state changes from Stopped
/// to either paused or running and in derived classes could do things like
/// commit memory and grab hardware resource (the default is to do nothing)
/// </summary>
HRESULT CBasePin::Active(void)
{
	return NOERROR;
}

/// <summary>
/// Called by IMediaFilter implementation when the state changes from
/// to either paused to running and in derived classes could do things like
/// commit memory and grab hardware resource (the default is to do nothing)
/// </summary>
HRESULT CBasePin::Run(REFERENCE_TIME tStart)
{
	Q_UNUSED(tStart);
	return NOERROR;
}

/// <summary>
/// Also called by the IMediaFilter implementation when the state changes to
/// Stopped at which point you should decommit allocators and free hardware
/// resources you grabbed in the Active call (default is also to do nothing)
/// </summary>
HRESULT CBasePin::Inactive(void)
{
	m_bRunTimeError = FALSE;
	return NOERROR;
}

/// <summary>
/// Called when no more data will arrive
/// </summary>
STDMETHODIMP CBasePin::EndOfStream(void)
{
	return S_OK;
}

STDMETHODIMP CBasePin::SetSink(IQualityControl * piqc)
{
	CAutoLock cObjectLock(m_pLock);
	m_pQSink = piqc;
	return NOERROR;
}

STDMETHODIMP CBasePin::Notify(IBaseFilter * pSender, Quality q)
{
	Q_UNUSED(q);
	Q_UNUSED(pSender);
	appLog(LOG_CAT, Log::Warning)
		<< "IQualityControl::Notify not over-ridden from CBasePin. "
		<< "(IGNORE is OK)";
	return E_NOTIMPL;
}

/// <summary>
/// NewSegment notifies of the start/stop/rate applying to the data about to be
/// received. Default implementation records data and returns S_OK. Override
/// this to pass downstream.
/// </summary>
STDMETHODIMP CBasePin::NewSegment(
	REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_tStart = tStart;
	m_tStop = tStop;
	m_dRate = dRate;
	return S_OK;
}

//=============================================================================
// CBaseInputPin class

CBaseInputPin::CBaseInputPin(
	LPCTSTR pObjectName, CBaseFilter *pFilter, CCritSec *pLock, HRESULT *phr,
	LPCWSTR pPinName)
	: CBasePin(pObjectName, pFilter, pLock, phr, pPinName, PINDIR_INPUT)
	, m_pAllocator(NULL)
	, m_bReadOnly(FALSE)
	, m_bFlushing(FALSE)
{
	ZeroMemory(&m_SampleProps, sizeof(m_SampleProps));
}

CBaseInputPin::CBaseInputPin(
	LPCSTR pObjectName, CBaseFilter *pFilter, CCritSec *pLock, HRESULT *phr,
	LPCWSTR pPinName)
	: CBasePin(pObjectName, pFilter, pLock, phr, pPinName, PINDIR_INPUT)
	, m_pAllocator(NULL)
	, m_bReadOnly(FALSE)
	, m_bFlushing(FALSE)
{
	ZeroMemory(&m_SampleProps, sizeof(m_SampleProps));
}

// Destructor releases it's reference count on the default allocator
CBaseInputPin::~CBaseInputPin()
{
	if(m_pAllocator != NULL) {
		m_pAllocator->Release();
		m_pAllocator = NULL;
	}
}

STDMETHODIMP CBaseInputPin::DelegateQueryInterface(REFIID riid, void **ppv)
{
	if(riid == IID_IMemInputPin)
		return GetInterface((IMemInputPin *) this, ppv);
	return CBasePin::DelegateQueryInterface(riid, ppv);
}

/// <summary>
/// Return the allocator interface that this input pin would like the output
/// pin to use. NOTE subsequent calls to GetAllocator should all return an
/// interface onto the SAME object so we create one object at the start
///
/// Note:
/// The allocator is Release()'d on disconnect and replaced on
/// NotifyAllocator().
///
/// Override this to provide your own allocator.
/// </summary>
STDMETHODIMP CBaseInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
	if(ppAllocator == NULL)
		return E_POINTER;
	CAutoLock cObjectLock(m_pLock);

	if(m_pAllocator == NULL) {
		HRESULT hr = CreateMemoryAllocator(&m_pAllocator);
		if(FAILED(hr))
			return hr;
	}
	Q_ASSERT(m_pAllocator != NULL);
	*ppAllocator = m_pAllocator;
	m_pAllocator->AddRef();
	return NOERROR;
}

/// <summary>
/// Tell the input pin which allocator the output pin is actually going to use
/// Override this if you care - NOTE the locking we do both here and also in
/// GetAllocator is unnecessary but derived classes that do something useful
/// will undoubtedly have to lock the object so this might help remind people
/// </summary>
STDMETHODIMP CBaseInputPin::NotifyAllocator(
	IMemAllocator * pAllocator, BOOL bReadOnly)
{
	if(pAllocator == NULL)
		return E_POINTER;
	CAutoLock cObjectLock(m_pLock);

	IMemAllocator *pOldAllocator = m_pAllocator;
	pAllocator->AddRef();
	m_pAllocator = pAllocator;

	if(pOldAllocator != NULL)
		pOldAllocator->Release();

	// The readonly flag indicates whether samples from this allocator should
	// be regarded as readonly - if true, then inplace transforms will not be
	// allowed.
	m_bReadOnly = (BYTE)bReadOnly;
	return NOERROR;
}

HRESULT CBaseInputPin::BreakConnect()
{
	// We don't need our allocator any more
	if(m_pAllocator) {
		// Always decommit the allocator because a downstream filter may or
		// may not decommit the connection's allocator.  A memory leak could
		// occur if the allocator is not decommited when a pin is disconnected.
		HRESULT hr = m_pAllocator->Decommit();
		if(FAILED(hr))
			return hr;

		m_pAllocator->Release();
		m_pAllocator = NULL;
	}

	return S_OK;
}

/// <summary>
/// Do something with this media sample - this base class checks to see if the
/// format has changed with this media sample and if so checks that the filter
/// will accept it, generating a run time error if not. Once we have raised a
/// run time error we set a flag so that no more samples will be accepted
///
/// It is important that any filter should override this method and implement
/// synchronization so that samples are not processed when the pin is
/// disconnected etc
/// </summary>
STDMETHODIMP CBaseInputPin::Receive(IMediaSample *pSample)
{
	if(pSample == NULL)
		return E_POINTER;
	Q_ASSERT(pSample);

	HRESULT hr = CheckStreaming();
	if(S_OK != hr) {
		return hr;
	}

	// Check for IMediaSample2
	IMediaSample2 *pSample2;
	hr = pSample->QueryInterface(IID_IMediaSample2, (void **)&pSample2);
	if(SUCCEEDED(hr)) {
		hr = pSample2->GetProperties(
			sizeof(m_SampleProps), (PBYTE)&m_SampleProps);
		pSample2->Release();
		if(FAILED(hr))
			return hr;
	} else {
		//  Get the properties the hard way
		m_SampleProps.cbData = sizeof(m_SampleProps);
		m_SampleProps.dwTypeSpecificFlags = 0;
		m_SampleProps.dwStreamId = AM_STREAM_MEDIA;
		m_SampleProps.dwSampleFlags = 0;
		if(S_OK == pSample->IsDiscontinuity())
			m_SampleProps.dwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;
		if(S_OK == pSample->IsPreroll())
			m_SampleProps.dwSampleFlags |= AM_SAMPLE_PREROLL;
		if(S_OK == pSample->IsSyncPoint())
			m_SampleProps.dwSampleFlags |= AM_SAMPLE_SPLICEPOINT;
		if(SUCCEEDED(pSample->GetTime(&m_SampleProps.tStart,
			&m_SampleProps.tStop)))
		{
			m_SampleProps.dwSampleFlags |= AM_SAMPLE_TIMEVALID |
				AM_SAMPLE_STOPVALID;
		}
		if(S_OK == pSample->GetMediaType(&m_SampleProps.pMediaType))
			m_SampleProps.dwSampleFlags |= AM_SAMPLE_TYPECHANGED;
		pSample->GetPointer(&m_SampleProps.pbBuffer);
		m_SampleProps.lActual = pSample->GetActualDataLength();
		m_SampleProps.cbBuffer = pSample->GetSize();
	}

	// Has the format changed in this sample
	if(!(m_SampleProps.dwSampleFlags & AM_SAMPLE_TYPECHANGED))
		return NOERROR;

	// Check the derived class accepts this format
	// This shouldn't fail as the source must call QueryAccept first
	hr = CheckMediaType((CMediaType *)m_SampleProps.pMediaType);
	if(hr == NOERROR)
		return NOERROR;

	// Raise a runtime error if we fail the media type
	m_bRunTimeError = TRUE;
	EndOfStream();
	m_pFilter->NotifyEvent(EC_ERRORABORT, VFW_E_TYPE_NOT_ACCEPTED, 0);
	return VFW_E_INVALIDMEDIATYPE;
}

/// <summary>
/// Receive multiple samples
/// </summary>
STDMETHODIMP CBaseInputPin::ReceiveMultiple(
	IMediaSample **pSamples, long nSamples, long *nSamplesProcessed)
{
	if(pSamples == NULL)
		return E_POINTER;

	HRESULT hr = S_OK;
	*nSamplesProcessed = 0;
	while(nSamples-- > 0) {
		hr = Receive(pSamples[*nSamplesProcessed]);

		// S_FALSE means don't send any more
		if(hr != S_OK)
			break;
		(*nSamplesProcessed)++;
	}
	return hr;
}

/// <summary>
/// See if Receive() might block
/// </summary>
STDMETHODIMP CBaseInputPin::ReceiveCanBlock()
{
	// Ask all the output pins if they block. If there are no output pin assume
	// we do block
	int cPins = m_pFilter->GetPinCount();
	int cOutputPins = 0;
	for(int c = 0; c < cPins; c++) {
		CBasePin *pPin = m_pFilter->GetPin(c);
		if(NULL == pPin)
			break;
		PIN_DIRECTION pd;
		HRESULT hr = pPin->QueryDirection(&pd);
		if(FAILED(hr))
			return hr;

		if(pd != PINDIR_OUTPUT)
			continue;

		IPin *pConnected;
		hr = pPin->ConnectedTo(&pConnected);
		if(SUCCEEDED(hr)) {
			Q_ASSERT(pConnected != NULL);
			cOutputPins++;
			IMemInputPin *pInputPin;
			hr = pConnected->QueryInterface(
				IID_IMemInputPin, (void **)&pInputPin);
			pConnected->Release();
			if(SUCCEEDED(hr)) {
				hr = pInputPin->ReceiveCanBlock();
				pInputPin->Release();
				if(hr != S_FALSE)
					return S_OK;
			} else {
				// There's a transport we don't understand here
				return S_OK;
			}
		}
	}
	return cOutputPins == 0 ? S_OK : S_FALSE;
}

/// <summary>
/// Default handling for BeginFlush - call at the beginning of your
/// implementation (makes sure that all Receive calls fail). After calling
/// this, you need to free any queued data and then call downstream.
/// </summary>
STDMETHODIMP CBaseInputPin::BeginFlush(void)
{
	// BeginFlush is NOT synchronized with streaming but is part of
	// a control action - hence we synchronize with the filter
	CAutoLock lck(m_pLock);

	// If we are already in mid-flush, this is probably a mistake
	// though not harmful - try to pick it up for now so I can think about it
	Q_ASSERT(!m_bFlushing);

	// First thing to do is ensure that no further Receive calls succeed
	m_bFlushing = TRUE;

	// Now discard any data and call downstream - must do that
	// in derived classes
	return S_OK;
}

/// <summary>
/// Fefault handling for EndFlush - call at end of your implementation
/// - before calling this, ensure that there is no queued data and no thread
/// pushing any more without a further receive, then call downstream,
/// then call this method to clear the m_bFlushing flag and re-enable
/// receives
/// </summary>
STDMETHODIMP CBaseInputPin::EndFlush()
{
	// Endlush is NOT synchronized with streaming but is part of
	// a control action - hence we synchronize with the filter
	CAutoLock lck(m_pLock);

	// Almost certainly a mistake if we are not in mid-flush
	Q_ASSERT(m_bFlushing);

	// Before calling, sync with pushing thread and ensure
	// no more data is going downstream, then call EndFlush on
	// downstream pins.

	// Now re-enable Receives
	m_bFlushing = FALSE;

	// No more errors
	m_bRunTimeError = FALSE;

	return S_OK;
}

STDMETHODIMP CBaseInputPin::Notify(IBaseFilter * pSender, Quality q)
{
	Q_UNUSED(q);
	if(pSender == NULL)
		return E_POINTER;
	appLog(LOG_CAT, Log::Warning)
		<< "IQuality::Notify called on an input pin";
	return NOERROR;
}

/// <summary>
/// Free up or unprepare allocator's memory, this is called through
/// IMediaFilter which is responsible for locking the object first
/// </summary>
HRESULT CBaseInputPin::Inactive()
{
	m_bRunTimeError = FALSE;
	if(m_pAllocator == NULL)
		return VFW_E_NO_ALLOCATOR;

	m_bFlushing = FALSE;

	return m_pAllocator->Decommit();
}

/// <summary>
/// What requirements do we have of the allocator - override if you want
/// to support other people's allocators but need a specific alignment
/// or prefix.
/// </summary>
STDMETHODIMP CBaseInputPin::GetAllocatorRequirements(
	ALLOCATOR_PROPERTIES *pProps)
{
	Q_UNUSED(pProps);
	return E_NOTIMPL;
}

/// <summary>
/// Check if it's OK to process data
/// </summary>
HRESULT CBaseInputPin::CheckStreaming()
{
	// Shouldn't be able to get any data if we're not connected!
	Q_ASSERT(IsConnected());

	// Don't process stuff in Stopped state
	if(IsStopped())
		return VFW_E_WRONG_STATE;
	if(m_bFlushing)
		return S_FALSE;
	if(m_bRunTimeError)
		return VFW_E_RUNTIME_ERROR;
	return S_OK;
}

// Pass on the Quality notification q to
// a. Our QualityControl sink (if we have one) or else
// b. to our upstream filter
// and if that doesn't work, throw it away with a bad return code
HRESULT CBaseInputPin::PassNotify(Quality &q)
{
	// We pass the message on, which means that we find the quality sink
	// for our input pin and send it there

	//appLog(LOG_CAT) << "Passing Quality notification through transform";
	if(m_pQSink != NULL)
		return m_pQSink->Notify(m_pFilter, q);

	// No sink set, so pass it upstream
	HRESULT hr;
	IQualityControl * pIQC;

	hr = VFW_E_NOT_FOUND; // Default return value
	if(m_Connected) {
		m_Connected->QueryInterface(IID_IQualityControl, (void **)&pIQC);
		if(pIQC != NULL) {
			hr = pIQC->Notify(m_pFilter, q);
			pIQC->Release();
		}
	}
	return hr;
}

//=============================================================================
// CRendererInputPin class

CRendererInputPin::CRendererInputPin(
	CBaseRenderer *pRenderer, HRESULT *phr, LPCWSTR pPinName)
	: CBaseInputPin(TEXT("Renderer pin"), pRenderer,
	&pRenderer->m_InterfaceLock, (HRESULT *)phr, pPinName)
{
	m_pRenderer = pRenderer;
	Q_ASSERT(m_pRenderer);
}

/// <summary>
/// Signals end of data stream on the input pin
/// </summary>
STDMETHODIMP CRendererInputPin::EndOfStream()
{
	CAutoLock cRendererLock(&m_pRenderer->m_InterfaceLock);
	CAutoLock cSampleLock(&m_pRenderer->m_RendererLock);

	// Make sure we're streaming ok
	HRESULT hr = CheckStreaming();
	if(hr != NOERROR)
		return hr;

	// Pass it onto the renderer
	hr = m_pRenderer->EndOfStream();
	if(SUCCEEDED(hr))
		hr = CBaseInputPin::EndOfStream();
	return hr;
}

/// <summary>
/// Signals start of flushing on the input pin - we do the final reset end of
/// stream with the renderer lock unlocked but with the interface lock locked
/// We must do this because we call timeKillEvent, our timer callback method
/// has to take the renderer lock to serialise our state. Therefore holding a
/// renderer lock when calling timeKillEvent could cause a deadlock condition
/// </summary>
STDMETHODIMP CRendererInputPin::BeginFlush()
{
	CAutoLock cRendererLock(&m_pRenderer->m_InterfaceLock);
	{
		CAutoLock cSampleLock(&m_pRenderer->m_RendererLock);
		CBaseInputPin::BeginFlush();
		m_pRenderer->BeginFlush();
	}
	return m_pRenderer->ResetEndOfStream();
}

/// <summary>
/// Signals end of flushing on the input pin
/// </summary>
STDMETHODIMP CRendererInputPin::EndFlush()
{
	CAutoLock cRendererLock(&m_pRenderer->m_InterfaceLock);
	CAutoLock cSampleLock(&m_pRenderer->m_RendererLock);

	HRESULT hr = m_pRenderer->EndFlush();
	if(SUCCEEDED(hr))
		hr = CBaseInputPin::EndFlush();
	return hr;
}

/// <summary>
/// Pass the sample straight through to the renderer object
/// </summary>
STDMETHODIMP CRendererInputPin::Receive(IMediaSample *pSample)
{
	HRESULT hr = m_pRenderer->Receive(pSample);
	if(SUCCEEDED(hr))
		return hr;

	// WARNING: A deadlock could occur if the caller holds the renderer lock
	// and attempts to acquire the interface lock.

	{
		// The interface lock must be held when the filter is calling
		// IsStopped() or IsFlushing().  The interface lock must also
		// be held because the function uses m_bRunTimeError.
		CAutoLock cRendererLock(&m_pRenderer->m_InterfaceLock);

		// We do not report errors which occur while the filter is stopping,
		// flushing or if the m_bAbort flag is set .  Errors are expected to
		// occur during these operations and the streaming thread correctly
		// handles the errors.
		if(!IsStopped() && !IsFlushing() && !m_pRenderer->m_bAbort &&
			!m_bRunTimeError)
		{
			// EC_ERRORABORT's first parameter is the error which caused
			// the event and its' last parameter is 0.  See the Direct
			// Show SDK documentation for more information.
			m_pRenderer->NotifyEvent(EC_ERRORABORT, hr, 0);

			{
				CAutoLock alRendererLock(&m_pRenderer->m_RendererLock);
				if(m_pRenderer->IsStreaming() &&
					!m_pRenderer->IsEndOfStreamDelivered())
				{
					m_pRenderer->NotifyEndOfStream();
				}
			}

			m_bRunTimeError = TRUE;
		}
	}

	return hr;
}

/// <summary>
/// Called when the input pin is disconnected
/// </summary>
HRESULT CRendererInputPin::BreakConnect()
{
	HRESULT hr = m_pRenderer->BreakConnect();
	if(FAILED(hr))
		return hr;
	return CBaseInputPin::BreakConnect();
}

/// <summary>
/// Called when the input pin is connected
/// </summary>
HRESULT CRendererInputPin::CompleteConnect(IPin *pReceivePin)
{
	HRESULT hr = m_pRenderer->CompleteConnect(pReceivePin);
	if(FAILED(hr))
		return hr;
	return CBaseInputPin::CompleteConnect(pReceivePin);
}

/// <summary>
/// Give the pin id of our one and only pin
/// </summary>
STDMETHODIMP CRendererInputPin::QueryId(LPWSTR *Id)
{
	if(Id == NULL)
		return E_POINTER;

	const WCHAR szIn[] = L"In";
	*Id = (LPWSTR)CoTaskMemAlloc(sizeof(szIn));
	if(*Id == NULL)
		return E_OUTOFMEMORY;
	CopyMemory(*Id, szIn, sizeof(szIn));
	return NOERROR;
}

/// <summary>
/// Will the filter accept this media type
/// </summary>
HRESULT CRendererInputPin::CheckMediaType(const CMediaType *pmt)
{
	return m_pRenderer->CheckMediaType(pmt);
}

/// <summary>
/// Called when we go paused or running
/// </summary>
HRESULT CRendererInputPin::Active()
{
	return m_pRenderer->Active();
}

/// <summary>
/// Called when we go into a stopped state
/// </summary>
HRESULT CRendererInputPin::Inactive()
{
	// The caller must hold the interface lock because
	// this function uses m_bRunTimeError.
	m_bRunTimeError = FALSE;
	return m_pRenderer->Inactive();
}

/// <summary>
/// Tell derived classes about the media type agreed
/// </summary>
HRESULT CRendererInputPin::SetMediaType(const CMediaType *pmt)
{
	HRESULT hr = CBaseInputPin::SetMediaType(pmt);
	if(FAILED(hr))
		return hr;
	return m_pRenderer->SetMediaType(pmt);
}

//=============================================================================
// CBaseFilter class

CBaseFilter::CBaseFilter(
	LPCTSTR pName, LPUNKNOWN pUnk, CCritSec *pLock, REFCLSID clsid)
	: CUnknown(pName, pUnk)
	, m_pLock(pLock)
	, m_clsid(clsid)
	, m_State(State_Stopped)
	, m_pClock(NULL)
	, m_pGraph(NULL)
	, m_pSink(NULL)
	, m_pName(NULL)
	, m_PinVersion(1)
{
	Q_ASSERT(pLock != NULL);
}

// Passes in a redundant HRESULT argument
CBaseFilter::CBaseFilter(
	LPCTSTR pName, LPUNKNOWN pUnk, CCritSec *pLock, REFCLSID clsid,
	HRESULT *phr)
	: CUnknown(pName, pUnk)
	, m_pLock(pLock)
	, m_clsid(clsid)
	, m_State(State_Stopped)
	, m_pClock(NULL)
	, m_pGraph(NULL)
	, m_pSink(NULL)
	, m_pName(NULL)
	, m_PinVersion(1)
{
	Q_ASSERT(pLock != NULL);
	Q_UNUSED(phr);
}

CBaseFilter::CBaseFilter(
	LPCSTR pName, LPUNKNOWN pUnk, CCritSec *pLock, REFCLSID clsid)
	: CUnknown(pName, pUnk)
	, m_pLock(pLock)
	, m_clsid(clsid)
	, m_State(State_Stopped)
	, m_pClock(NULL)
	, m_pGraph(NULL)
	, m_pSink(NULL)
	, m_pName(NULL)
	, m_PinVersion(1)
{
	Q_ASSERT(pLock != NULL);
}

CBaseFilter::CBaseFilter(
	LPCSTR pName, LPUNKNOWN pUnk, CCritSec *pLock, REFCLSID clsid,
	HRESULT *phr)
	: CUnknown(pName, pUnk)
	, m_pLock(pLock)
	, m_clsid(clsid)
	, m_State(State_Stopped)
	, m_pClock(NULL)
	, m_pGraph(NULL)
	, m_pSink(NULL)
	, m_pName(NULL)
	, m_PinVersion(1)
{
	Q_ASSERT(pLock != NULL);
	Q_UNUSED(phr);
}

CBaseFilter::~CBaseFilter()
{
	// NOTE we do NOT hold references on the filtergraph for m_pGraph or
	// m_pSink. When we did we had the circular reference problem. Nothing
	// would go away.

	delete[] m_pName;

	// Must be stopped, but can't call Stop here since
	// our critsec has been destroyed.

	// Release any clock we were using
	if(m_pClock) {
		m_pClock->Release();
		m_pClock = NULL;
	}
}

STDMETHODIMP CBaseFilter::DelegateQueryInterface(REFIID riid, void **ppv)
{
	if(riid == IID_IBaseFilter)
		return GetInterface((IBaseFilter *) this, ppv);
	else if(riid == IID_IMediaFilter)
		return GetInterface((IMediaFilter *) this, ppv);
	else if(riid == IID_IPersist)
		return GetInterface((IPersist *) this, ppv);
	else if(riid == IID_IAMovieSetup)
		return GetInterface((IAMovieSetup *) this, ppv);
	return CUnknown::DelegateQueryInterface(riid, ppv);
}

/// <summary>
/// Return the filter's clsid
/// </summary>
STDMETHODIMP CBaseFilter::GetClassID(CLSID *pClsID)
{
	if(pClsID == NULL)
		return E_POINTER;
	*pClsID = m_clsid;
	return NOERROR;
}

/// <summary>
/// Override this if your state changes are not done synchronously
/// </summary>
STDMETHODIMP CBaseFilter::GetState(DWORD dwMSecs, FILTER_STATE *State)
{
	Q_UNUSED(dwMSecs);
	if(State == NULL)
		return E_POINTER;
	*State = m_State;
	return S_OK;
}

/// <summary>
/// Set the clock we will use for synchronisation
/// </summary>
STDMETHODIMP CBaseFilter::SetSyncSource(IReferenceClock *pClock)
{
	CAutoLock cObjectLock(m_pLock);

	// Ensure the new one does not go away - even if the same as the old
	if(pClock)
		pClock->AddRef();

	// If we have a clock, release it
	if(m_pClock)
		m_pClock->Release();

	// Set the new reference clock (might be NULL)
	// Should we query it to ensure it is a clock?  Consider for a debug build.
	m_pClock = pClock;

	return NOERROR;
}

/// <summary>
/// Return the clock we are using for synchronisation
/// </summary>
STDMETHODIMP CBaseFilter::GetSyncSource(IReferenceClock **pClock)
{
	if(pClock == NULL)
		return E_POINTER;
	CAutoLock cObjectLock(m_pLock);

	if(m_pClock)
		m_pClock->AddRef();
	*pClock = (IReferenceClock*)m_pClock;
	return NOERROR;
}

/// <summary>
/// Override CBaseMediaFilter Stop method, to deactivate any pins this
/// filter has.
/// </summary>
STDMETHODIMP CBaseFilter::Stop()
{
	CAutoLock cObjectLock(m_pLock);
	HRESULT hr = NOERROR;

	// Notify all pins of the state change
	if(m_State != State_Stopped) {
		int cPins = GetPinCount();
		for(int c = 0; c < cPins; c++) {
			CBasePin *pPin = GetPin(c);
			if(NULL == pPin)
				break;

			// Disconnected pins are not activated - this saves pins worrying
			// about this state themselves. We ignore the return code to make
			// sure everyone is inactivated regardless. The base input pin
			// class can return an error if it has no allocator but Stop can
			// be used to resync the graph state after something has gone bad

			if(pPin->IsConnected()) {
				HRESULT hrTmp = pPin->Inactive();
				if(FAILED(hrTmp) && SUCCEEDED(hr))
					hr = hrTmp;
			}
		}
	}

	m_State = State_Stopped;
	return hr;
}

/// <summary>
/// Override CBaseMediaFilter Pause method to activate any pins
/// this filter has (also called from Run)
/// </summary>
STDMETHODIMP CBaseFilter::Pause()
{
	CAutoLock cObjectLock(m_pLock);

	// Notify all pins of the change to active state
	if(m_State == State_Stopped) {
		int cPins = GetPinCount();
		for(int c = 0; c < cPins; c++) {
			CBasePin *pPin = GetPin(c);
			if(NULL == pPin)
				break;

			// Disconnected pins are not activated - this saves pins
			// worrying about this state themselves
			if(pPin->IsConnected()) {
				HRESULT hr = pPin->Active();
				if(FAILED(hr))
					return hr;
			}
		}
	}

	m_State = State_Paused;
	return S_OK;
}

/// <summary>
/// Put the filter into a running state.
///
/// The time parameter is the offset to be added to the samples'
/// stream time to get the reference time at which they should be presented.
///
/// you can either add these two and compare it against the reference clock,
/// or you can call CBaseFilter::StreamTime and compare that against
/// the sample timestamp.
/// </summary>
STDMETHODIMP CBaseFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cObjectLock(m_pLock);

	// Remember the stream time offset
	m_tStart = tStart;

	if(m_State == State_Stopped) {
		HRESULT hr = Pause();
		if(FAILED(hr))
			return hr;
	}

	// Notify all pins of the change to active state
	if(m_State != State_Running) {
		int cPins = GetPinCount();
		for(int c = 0; c < cPins; c++) {
			CBasePin *pPin = GetPin(c);
			if(NULL == pPin)
				break;

			// Disconnected pins are not activated - this saves pins
			// worrying about this state themselves
			if(pPin->IsConnected()) {
				HRESULT hr = pPin->Run(tStart);
				if(FAILED(hr))
					return hr;
			}
		}
	}

	m_State = State_Running;
	return S_OK;
}

/// <summary>
/// Return the current stream time - samples with start timestamps of this
/// time or before should be rendered by now
/// <summary>
HRESULT CBaseFilter::StreamTime(CRefTime& rtStream)
{
	// Caller must lock for synchronization
	// We can't grab the filter lock because we want to be able to call
	// this from worker threads without deadlocking
	if(m_pClock == NULL)
		return VFW_E_NO_CLOCK;

	// Get the current reference time
	HRESULT hr = m_pClock->GetTime((REFERENCE_TIME *)&rtStream);
	if(FAILED(hr))
		return hr;

	// subtract the stream offset to get stream time
	rtStream -= m_tStart;

	return S_OK;
}

/// <summary>
/// Create an enumerator for the pins attached to this filter
/// </summary>
STDMETHODIMP CBaseFilter::EnumPins(IEnumPins **ppEnum)
{
	if(ppEnum == NULL)
		return E_POINTER;

	// Create a new ref counted enumerator
	*ppEnum = new CEnumPins(this, NULL);
	return *ppEnum == NULL ? E_OUTOFMEMORY : NOERROR;
}

/// <summary>
/// Default behaviour of FindPin is to assume pins are named
/// by their pin names
/// </summary>
STDMETHODIMP CBaseFilter::FindPin(LPCWSTR Id, IPin **ppPin)
{
	if(ppPin == NULL)
		return E_POINTER;

	// We're going to search the pin list so maintain integrity
	CAutoLock lck(m_pLock);
	int iCount = GetPinCount();
	for(int i = 0; i < iCount; i++) {
		CBasePin *pPin = GetPin(i);
		if(NULL == pPin)
			break;

		if(0 == lstrcmpW(pPin->Name(), Id)) {
			// Found one that matches. AddRef() and return it
			*ppPin = pPin;
			pPin->AddRef();
			return S_OK;
		}
	}
	*ppPin = NULL;
	return VFW_E_NOT_FOUND;
}

/// <summary>
/// Return information about this filter
/// </summary>
STDMETHODIMP CBaseFilter::QueryFilterInfo(FILTER_INFO * pInfo)
{
	if(pInfo == NULL)
		return E_POINTER;

	if(m_pName)
		StringCchCopyW(pInfo->achName, NUMELMS(pInfo->achName), m_pName);
	else
		pInfo->achName[0] = L'\0';
	pInfo->pGraph = m_pGraph;
	if(m_pGraph)
		m_pGraph->AddRef();
	return NOERROR;
}

/// <summary>
/// Provide the filter with a filter graph
/// </summary>
STDMETHODIMP CBaseFilter::JoinFilterGraph(
	IFilterGraph * pGraph, LPCWSTR pName)
{
	CAutoLock cObjectLock(m_pLock);

	// NOTE: We no longer hold references on the graph (m_pGraph, m_pSink)
	m_pGraph = pGraph;
	if(m_pGraph) {
		HRESULT hr = m_pGraph->QueryInterface(
			IID_IMediaEventSink, (void **)&m_pSink);
		if(FAILED(hr)) {
			Q_ASSERT(m_pSink == NULL);
		} else
			m_pSink->Release(); // we do NOT keep a reference on it.
	} else {
		// If graph pointer is null, then we should
		// also release the IMediaEventSink on the same object - we don't
		// refcount it, so just set it to null
		m_pSink = NULL;
	}

	if(m_pName) {
		delete[] m_pName;
		m_pName = NULL;
	}

	if(pName) {
		size_t namelen;
		HRESULT hr = StringCchLengthW(pName, STRSAFE_MAX_CCH, &namelen);
		if(FAILED(hr))
			return hr;
		m_pName = new WCHAR[namelen + 1];
		if(m_pName)
			StringCchCopyW(m_pName, namelen + 1, pName);
		else
			return E_OUTOFMEMORY;
	}

	return NOERROR;
}

/// <summary>
/// Return a Vendor information string. Optional - may return E_NOTIMPL.
/// memory returned should be freed using CoTaskMemFree
/// default implementation returns E_NOTIMPL
/// </summary>
STDMETHODIMP CBaseFilter::QueryVendorInfo(LPWSTR* pVendorInfo)
{
	Q_UNUSED(pVendorInfo);
	return E_NOTIMPL;
}

/// <summary>
/// Send an event notification to the filter graph if we know about it.
/// returns S_OK if delivered, S_FALSE if the filter graph does not sink
/// events, or an error otherwise.
/// </summary>
HRESULT CBaseFilter::NotifyEvent(
	long EventCode, LONG_PTR EventParam1, LONG_PTR EventParam2)
{
	// Snapshot so we don't have to lock up
	IMediaEventSink *pSink = m_pSink;
	if(pSink) {
		if(EC_COMPLETE == EventCode)
			EventParam2 = (LONG_PTR)(IBaseFilter*)this;
		return pSink->Notify(EventCode, EventParam1, EventParam2);
	}
	return E_NOTIMPL;
}

/// <summary>
/// Request reconnect
/// pPin is the pin to reconnect
/// pmt is the type to reconnect with - can be NULL
/// Calls ReconnectEx on the filter graph
/// </summary>
HRESULT CBaseFilter::ReconnectPin(IPin *pPin, AM_MEDIA_TYPE const *pmt)
{
	IFilterGraph2 *pGraph2;
	if(m_pGraph == NULL)
		return E_NOINTERFACE;

	HRESULT hr = m_pGraph->QueryInterface(
		IID_IFilterGraph2, (void **)&pGraph2);
	if(SUCCEEDED(hr)) {
		hr = pGraph2->ReconnectEx(pPin, pmt);
		pGraph2->Release();
		return hr;
	}
	return m_pGraph->Reconnect(pPin);
}

/// <summary>
/// This is the same idea as the media type version does for type enumeration
/// on pins but for the list of pins available. So if the list of pins you
/// provide changes dynamically then either override this virtual function
/// to provide the version number, or more simply call IncrementPinVersion
/// </summary>
LONG CBaseFilter::GetPinVersion()
{
	return m_PinVersion;
}

/// <summary>
/// Increment the current pin version cookie
/// </summary>
void CBaseFilter::IncrementPinVersion()
{
	InterlockedIncrement(&m_PinVersion);
}

//=============================================================================
// CBaseRenderer class

CBaseRenderer::CBaseRenderer(
	REFCLSID RenderClass, LPCTSTR pName, LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(pName,pUnk,&m_InterfaceLock,RenderClass)
	, m_evComplete(TRUE, phr)
	, m_RenderEvent(FALSE, phr)
	, m_bAbort(FALSE)
	, m_pPosition(NULL)
	, m_ThreadSignal(TRUE, phr)
	, m_bStreaming(FALSE)
	, m_bEOS(FALSE)
	, m_bEOSDelivered(FALSE)
	, m_pMediaSample(NULL)
	, m_dwAdvise(0)
	, m_pQSink(NULL)
	, m_pInputPin(NULL)
	, m_bRepaintStatus(TRUE)
	, m_SignalTime(0)
	, m_bInReceive(FALSE)
	, m_EndOfStreamTimer(0)
{
	if(SUCCEEDED(*phr))
		Ready();
}

/// <summary>
/// Delete the dynamically allocated IMediaPosition and IMediaSeeking helper
/// object. The object is created when somebody queries us. These are standard
/// control interfaces for seeking and setting start/stop positions and rates.
/// We will probably also have made an input pin based on CRendererInputPin
/// that has to be deleted, it's created when an enumerator calls our GetPin
/// </summary>
CBaseRenderer::~CBaseRenderer()
{
	Q_ASSERT(m_bStreaming == FALSE);
	Q_ASSERT(m_EndOfStreamTimer == 0);
	StopStreaming();
	ClearPendingSample();

	// Delete any IMediaPosition implementation
	if(m_pPosition) {
		m_pPosition->Release();
		m_pPosition = NULL;
	}

	// Delete any input pin created
	if(m_pInputPin) {
		delete m_pInputPin;
		m_pInputPin = NULL;
	}

	// Release any Quality sink
	Q_ASSERT(m_pQSink == NULL);
}

/// <summary>
/// This returns the IMediaPosition and IMediaSeeking interfaces
/// </summary>
HRESULT CBaseRenderer::GetMediaPositionInterface(REFIID riid, void **ppv)
{
	CAutoLock cObjectCreationLock(&m_ObjectCreationLock);
	if(m_pPosition)
		return m_pPosition->DelegateQueryInterface(riid, ppv);

	CBasePin *pPin = GetPin(0);
	if(NULL == pPin)
		return E_OUTOFMEMORY;

	HRESULT hr = NOERROR;

	// Create implementation of this dynamically since sometimes we may
	// never try and do a seek. The helper object implements a position
	// control interface (IMediaPosition) which in fact simply takes the
	// calls normally from the filter graph and passes them upstream
	m_pPosition = new CRendererPosPassThru(
		TEXT("Renderer CPosPassThru"), CBaseFilter::GetOwner(),
		(HRESULT *)&hr, pPin);
	if(m_pPosition == NULL)
		return E_OUTOFMEMORY;
	if(FAILED(hr)) {
		delete m_pPosition;
		m_pPosition = NULL;
		return E_NOINTERFACE;
	}

	// Make sure that we are the one that decides when the object is deleted
	m_pPosition->DelegateAddRef();

	hr = GetMediaPositionInterface(riid, ppv);
	return hr;
}

STDMETHODIMP CBaseRenderer::DelegateQueryInterface(REFIID riid, void **ppv)
{
	if(riid == IID_IMediaPosition || riid == IID_IMediaSeeking)
		return GetMediaPositionInterface(riid, ppv);
	return CBaseFilter::DelegateQueryInterface(riid, ppv);
}

/// <summary>
/// This is called whenever we change states, we have a manual reset event that
/// is signalled whenever we don't won't the source filter thread to wait in us
/// (such as in a stopped state) and likewise is not signalled whenever it can
/// wait (during paused and running) this function sets or resets the thread
/// event. The event is used to stop source filter threads waiting in Receive
/// </summary>
HRESULT CBaseRenderer::SourceThreadCanWait(BOOL bCanWait)
{
	if(bCanWait == TRUE)
		m_ThreadSignal.Reset();
	else
		m_ThreadSignal.Set();
	return NOERROR;
}

/// <summary>
/// Wait until the clock sets the timer event or we're otherwise signalled. We
/// set an arbitrary timeout for this wait and if it fires then we display the
/// current renderer state on the debugger. It will often fire if the filter's
/// left paused in an application however it may also fire during stress tests
/// if the synchronisation with application seeks and state changes is faulty
/// </summary>
HRESULT CBaseRenderer::WaitForRenderTime()
{
#define RENDER_TIMEOUT 10000

	HANDLE WaitObjects[] = { m_ThreadSignal, m_RenderEvent };
	DWORD Result = WAIT_TIMEOUT;

	// Wait for either the time to arrive or for us to be stopped

	OnWaitStart();
	while(Result == WAIT_TIMEOUT)
		Result = WaitForMultipleObjects(2, WaitObjects, FALSE, RENDER_TIMEOUT);
	OnWaitEnd();

	// We may have been awoken without the timer firing
	if(Result == WAIT_OBJECT_0)
		return VFW_E_STATE_CHANGED;

	SignalTimerFired();
	return NOERROR;
}

/// <summary>
/// Poll waiting for Receive to complete.  This really matters when
/// Receive may set the palette and cause window messages
/// The problem is that if we don't really wait for a renderer to
/// stop processing we can deadlock waiting for a transform which
/// is calling the renderer's Receive() method because the transform's
/// Stop method doesn't know to process window messages to unblock
/// the renderer's Receive processing
/// </summary>
void CBaseRenderer::WaitForReceiveToComplete()
{
	for(;;) {
		if(!m_bInReceive)
			break;

		// Receive all interthread sendmessages
		MSG msg;
		PeekMessage(&msg, NULL, WM_NULL, WM_NULL, PM_NOREMOVE);

		Sleep(1);
	}

	// If the wakebit for QS_POSTMESSAGE is set, the PeekMessage call
	// above just cleared the changebit which will cause some messaging
	// calls to block (waitMessage, MsgWaitFor...) now.
	// Post a dummy message to set the QS_POSTMESSAGE bit again
	if(HIWORD(GetQueueStatus(QS_POSTMESSAGE)) & QS_POSTMESSAGE) {
		//  Send dummy message
		PostThreadMessage(GetCurrentThreadId(), WM_NULL, 0, 0);
	}
}

/// <summary>
/// A filter can have four discrete states, namely Stopped, Running, Paused,
/// Intermediate. We are in an intermediate state if we are currently trying
/// to pause but haven't yet got the first sample (or if we have been flushed
/// in paused state and therefore still have to wait for a sample to arrive)
///
/// This class contains an event called m_evComplete which is signalled when
/// the current state is completed and is not signalled when we are waiting to
/// complete the last state transition. As mentioned above the only time we
/// use this at the moment is when we wait for a media sample in paused state
/// If while we are waiting we receive an end of stream notification from the
/// source filter then we know no data is imminent so we can reset the event
/// This means that when we transition to paused the source filter must call
/// end of stream on us or send us an image otherwise we'll hang indefinately
///
/// Simple internal way of getting the real state
/// </summary>
FILTER_STATE CBaseRenderer::GetRealState()
{
	return m_State;
}

/// <summary>
/// The renderer doesn't complete the full transition to paused states until
/// it has got one media sample to render. If you ask it for its state while
/// it's waiting it will return the state along with VFW_S_STATE_INTERMEDIATE
/// </summary>
STDMETHODIMP CBaseRenderer::GetState(DWORD dwMSecs,FILTER_STATE *State)
{
	if(State == NULL)
		return E_POINTER;

	if(WaitDispatchingMessages(m_evComplete, dwMSecs) == WAIT_TIMEOUT) {
		*State = m_State;
		return VFW_S_STATE_INTERMEDIATE;
	}
	*State = m_State;
	return NOERROR;
}

/// <summary>
/// If we're pausing and we have no samples we don't complete the transition
/// to State_Paused and we return S_FALSE. However if the m_bAbort flag has
/// been set then all samples are rejected so there is no point waiting for
/// one. If we do have a sample then return NOERROR. We will only ever return
/// VFW_S_STATE_INTERMEDIATE from GetState after being paused with no sample
/// (calling GetState after either being stopped or Run will NOT return this)
/// </summary>
HRESULT CBaseRenderer::CompleteStateChange(FILTER_STATE OldState)
{
	// Allow us to be paused when disconnected
	if(m_pInputPin->IsConnected() == FALSE) {
		Ready();
		return S_OK;
	}

	// Have we run off the end of stream
	if(IsEndOfStream() == TRUE) {
		Ready();
		return S_OK;
	}

	// Make sure we get fresh data after being stopped
	if(HaveCurrentSample() == TRUE) {
		if(OldState != State_Stopped) {
			Ready();
			return S_OK;
		}
	}
	NotReady();
	return S_FALSE;
}

/// <summary>
/// When we stop the filter the things we do are:-
///
///      Decommit the allocator being used in the connection
///      Release the source filter if it's waiting in Receive
///      Cancel any advise link we set up with the clock
///      Any end of stream signalled is now obsolete so reset
///      Allow us to be stopped when we are not connected
/// </summary>
STDMETHODIMP CBaseRenderer::Stop()
{
	CAutoLock cRendererLock(&m_InterfaceLock);

	// Make sure there really is a state change
	if(m_State == State_Stopped)
		return NOERROR;

	// Is our input pin connected
	if(m_pInputPin->IsConnected() == FALSE) {
		//appLog(LOG_CAT) << "Input pin is not connected";
		m_State = State_Stopped;
		return NOERROR;
	}

	CBaseFilter::Stop();

	// If we are going into a stopped state then we must decommit whatever
	// allocator we are using it so that any source filter waiting in the
	// GetBuffer can be released and unlock themselves for a state change
	if(m_pInputPin->Allocator())
		m_pInputPin->Allocator()->Decommit();

	// Cancel any scheduled rendering
	SetRepaintStatus(TRUE);
	StopStreaming();
	SourceThreadCanWait(FALSE);
	ResetEndOfStream();
	CancelNotification();

	// There should be no outstanding clock advise
	HRESULT hr = CancelNotification();
	Q_ASSERT(hr == S_FALSE);
	Q_ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
	Q_ASSERT(m_EndOfStreamTimer == 0);

	Ready();
	WaitForReceiveToComplete();
	m_bAbort = FALSE;

	return NOERROR;
}

/// <summary>
/// When we pause the filter the things we do are:-
///
///      Commit the allocator being used in the connection
///      Allow a source filter thread to wait in Receive
///      Cancel any clock advise link (we may be running)
///      Possibly complete the state change if we have data
///      Allow us to be paused when we are not connected
/// </summary>
STDMETHODIMP CBaseRenderer::Pause()
{
	CAutoLock cRendererLock(&m_InterfaceLock);
	FILTER_STATE OldState = m_State;
	Q_ASSERT(m_pInputPin->IsFlushing() == FALSE);

	// Make sure there really is a state change
	if(m_State == State_Paused)
		return CompleteStateChange(State_Paused);

	// Has our input pin been connected
	if(m_pInputPin->IsConnected() == FALSE) {
		//appLog(LOG_CAT) << "Input pin is not connected";
		m_State = State_Paused;
		return CompleteStateChange(State_Paused);
	}

	// Pause the base filter class
	HRESULT hr = CBaseFilter::Pause();
	if(FAILED(hr)) {
		//appLog(LOG_CAT) << "Pause failed";
		return hr;
	}

	// Enable EC_REPAINT events again
	SetRepaintStatus(TRUE);
	StopStreaming();
	SourceThreadCanWait(TRUE);
	CancelNotification();
	ResetEndOfStreamTimer();

	// If we are going into a paused state then we must commit whatever
	// allocator we are using it so that any source filter can call the
	// GetBuffer and expect to get a buffer without returning an error
	if(m_pInputPin->Allocator())
		m_pInputPin->Allocator()->Commit();

	// There should be no outstanding advise
	hr = CancelNotification();
	Q_ASSERT(hr == S_FALSE);
	Q_ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
	Q_ASSERT(m_EndOfStreamTimer == 0);
	Q_ASSERT(m_pInputPin->IsFlushing() == FALSE);

	// When we come out of a stopped state we must clear any image we were
	// holding onto for frame refreshing. Since renderers see state changes
	// first we can reset ourselves ready to accept the source thread data
	// Paused or running after being stopped causes the current position to
	// be reset so we're not interested in passing end of stream signals
	if(OldState == State_Stopped) {
		m_bAbort = FALSE;
		ClearPendingSample();
	}
	return CompleteStateChange(OldState);
}

/// <summary>
/// When we run the filter the things we do are:-
///
///      Commit the allocator being used in the connection
///      Allow a source filter thread to wait in Receive
///      Signal the render event just to get us going
///      Start the base class by calling StartStreaming
///      Allow us to be run when we are not connected
///      Signal EC_COMPLETE if we are not connected
/// </summary>
STDMETHODIMP CBaseRenderer::Run(REFERENCE_TIME StartTime)
{
	CAutoLock cRendererLock(&m_InterfaceLock);
	FILTER_STATE OldState = m_State;

	// Make sure there really is a state change
	if(m_State == State_Running)
		return NOERROR;

	// Send EC_COMPLETE if we're not connected
	if(m_pInputPin->IsConnected() == FALSE) {
		NotifyEvent(EC_COMPLETE,S_OK,(LONG_PTR)(IBaseFilter *)this);
		m_State = State_Running;
		return NOERROR;
	}

	Ready();

	// Pause the base filter class
	HRESULT hr = CBaseFilter::Run(StartTime);
	if(FAILED(hr)) {
		//appLog(LOG_CAT) << "Run failed";
		return hr;
	}

	// Allow the source thread to wait
	Q_ASSERT(m_pInputPin->IsFlushing() == FALSE);
	SourceThreadCanWait(TRUE);
	SetRepaintStatus(FALSE);

	// There should be no outstanding advise
	hr = CancelNotification();
	Q_ASSERT(hr == S_FALSE);
	Q_ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
	Q_ASSERT(m_EndOfStreamTimer == 0);
	Q_ASSERT(m_pInputPin->IsFlushing() == FALSE);

	// If we are going into a running state then we must commit whatever
	// allocator we are using it so that any source filter can call the
	// GetBuffer and expect to get a buffer without returning an error
	if(m_pInputPin->Allocator())
		m_pInputPin->Allocator()->Commit();

	// When we come out of a stopped state we must clear any image we were
	// holding onto for frame refreshing. Since renderers see state changes
	// first we can reset ourselves ready to accept the source thread data
	// Paused or running after being stopped causes the current position to
	// be reset so we're not interested in passing end of stream signals
	if(OldState == State_Stopped) {
		m_bAbort = FALSE;
		ClearPendingSample();
	}
	return StartStreaming();
}

/// <summary>
/// Return the number of input pins we support
/// </summary>
int CBaseRenderer::GetPinCount()
{
	if(m_pInputPin == NULL) {
		// Try to create it
		GetPin(0);
	}
	return m_pInputPin != NULL ? 1 : 0;
}

/// <summary>
/// We only support one input pin and it is numbered zero
/// </summary>
CBasePin *CBaseRenderer::GetPin(int n)
{
	CAutoLock cObjectCreationLock(&m_ObjectCreationLock);

	// Should only ever be called with zero
	Q_ASSERT(n == 0);

	if(n != 0)
		return NULL;

	// Create the input pin if not already done so
	if(m_pInputPin == NULL) {
		// hr must be initialized to NOERROR because
		// CRendererInputPin's constructor only changes
		// hr's value if an error occurs.
		HRESULT hr = NOERROR;

		m_pInputPin = new CRendererInputPin(this, &hr, L"In");
		if(NULL == m_pInputPin)
			return NULL;
		if(FAILED(hr)) {
			delete m_pInputPin;
			m_pInputPin = NULL;
			return NULL;
		}
	}
	return m_pInputPin;
}

/// <summary>
/// If "In" then return the IPin for our input pin, otherwise NULL and error
/// </summary>
STDMETHODIMP CBaseRenderer::FindPin(LPCWSTR Id, IPin **ppPin)
{
	if(ppPin == NULL)
		return E_POINTER;

	if(0 == lstrcmpW(Id, L"In")) {
		*ppPin = GetPin(0);
		if(*ppPin)
			(*ppPin)->AddRef();
		else
			return E_OUTOFMEMORY;
	} else {
		*ppPin = NULL;
		return VFW_E_NOT_FOUND;
	}
	return NOERROR;
}

/// <summary>
/// Called when the input pin receives an EndOfStream notification. If we have
/// not got a sample, then notify EC_COMPLETE now. If we have samples, then set
/// m_bEOS and check for this on completing samples. If we're waiting to pause
/// then complete the transition to paused state by setting the state event
/// </summary>
HRESULT CBaseRenderer::EndOfStream()
{
	// Ignore these calls if we are stopped
	if(m_State == State_Stopped)
		return NOERROR;

	// If we have a sample then wait for it to be rendered
	m_bEOS = TRUE;
	if(m_pMediaSample)
		return NOERROR;

	// If we are waiting for pause then we are now ready since we cannot now
	// carry on waiting for a sample to arrive since we are being told there
	// won't be any. This sets an event that the GetState function picks up
	Ready();

	// Only signal completion now if we are running otherwise queue it until
	// we do run in StartStreaming. This is used when we seek because a seek
	// causes a pause where early notification of completion is misleading
	if(m_bStreaming)
		SendEndOfStream();
	return NOERROR;
}

/// <summary>
/// When we are told to flush we should release the source thread
/// </summary>
HRESULT CBaseRenderer::BeginFlush()
{
	// If paused then report state intermediate until we get some data
	if(m_State == State_Paused)
		NotReady();

	SourceThreadCanWait(FALSE);
	CancelNotification();
	ClearPendingSample();
	// Wait for Receive to complete
	WaitForReceiveToComplete();

	return NOERROR;
}

/// <summary>
/// After flushing the source thread can wait in Receive again
/// </summary>
HRESULT CBaseRenderer::EndFlush()
{
	// Reset the current sample media time
	if(m_pPosition)
		m_pPosition->ResetMediaTime();

	// There should be no outstanding advise
	HRESULT hr = CancelNotification();
	Q_ASSERT(hr == S_FALSE);
	SourceThreadCanWait(TRUE);
	return NOERROR;
}

/// <summary>
/// We can now send EC_REPAINTs if so required
/// </summary>
HRESULT CBaseRenderer::CompleteConnect(IPin *pReceivePin)
{
	// The caller should always hold the interface lock because
	// the function uses CBaseFilter::m_State
	m_bAbort = FALSE;

	if(State_Running == GetRealState()) {
		HRESULT hr = StartStreaming();
		if(FAILED(hr))
			return hr;

		SetRepaintStatus(FALSE);
	} else
		SetRepaintStatus(TRUE);

	return NOERROR;
}

/// <summary>
/// Called when we go paused or running
/// </summary>
HRESULT CBaseRenderer::Active()
{
	return NOERROR;
}

/// <summary>
/// Called when we go into a stopped state
/// </summary>
HRESULT CBaseRenderer::Inactive()
{
	if(m_pPosition)
		m_pPosition->ResetMediaTime();

	//  People who derive from this may want to override this behaviour
	//  to keep hold of the sample in some circumstances
	ClearPendingSample();

	return NOERROR;
}

/// <summary>
/// Tell derived classes about the media type agreed
/// </summary>
HRESULT CBaseRenderer::SetMediaType(const CMediaType *pmt)
{
	return NOERROR;
}

/// <summary>
/// When we break the input pin connection we should reset the EOS flags. When
/// we are asked for either IMediaPosition or IMediaSeeking we will create a
/// CPosPassThru object to handles media time pass through. When we're handed
/// samples we store (by calling CPosPassThru::RegisterMediaTime) their media
/// times so we can then return a real current position of data being rendered
/// </summary>
HRESULT CBaseRenderer::BreakConnect()
{
	// Do we have a quality management sink
	if(m_pQSink) {
		m_pQSink->Release();
		m_pQSink = NULL;
	}

	// Check we have a valid connection
	if(m_pInputPin->IsConnected() == FALSE)
		return S_FALSE;

	//Check we are stopped before disconnecting
	if(m_State != State_Stopped && !m_pInputPin->CanReconnectWhenActive())
		return VFW_E_NOT_STOPPED;

	SetRepaintStatus(FALSE);
	ResetEndOfStream();
	ClearPendingSample();
	m_bAbort = FALSE;

	if(State_Running == m_State)
		StopStreaming();

	return NOERROR;
}

/// <summary>
/// Retrieves the sample times for this samples (note the sample times are
/// passed in by reference not value). We return S_FALSE to say schedule this
/// sample according to the times on the sample. We also return S_OK in
/// which case the object should simply render the sample data immediately
/// </summary>
HRESULT CBaseRenderer::GetSampleTimes(
	IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime,
	REFERENCE_TIME *pEndTime)
{
	Q_ASSERT(m_dwAdvise == 0);
	Q_ASSERT(pMediaSample);

	// If the stop time for this sample is before or the same as start time,
	// then just ignore it (release it) and schedule the next one in line
	// Source filters should always fill in the start and end times properly!

	if(SUCCEEDED(pMediaSample->GetTime(pStartTime, pEndTime))) {
		if(*pEndTime < *pStartTime)
			return VFW_E_START_TIME_AFTER_END;
	} else {
		// No time set in the sample... draw it now?
		return S_OK;
	}

	// Can't synchronise without a clock so we return S_OK which tells the
	// caller that the sample should be rendered immediately without going
	// through the overhead of setting a timer advise link with the clock
	if(m_pClock == NULL)
		return S_OK;
	return ShouldDrawSampleNow(pMediaSample,pStartTime,pEndTime);
}

/// <summary>
/// By default all samples are drawn according to their time stamps so we
/// return S_FALSE. Returning S_OK means draw immediately, this is used
/// by the derived video renderer class in its quality management.
/// </summary>
HRESULT CBaseRenderer::ShouldDrawSampleNow(
	IMediaSample *pMediaSample, REFERENCE_TIME *ptrStart,
	REFERENCE_TIME *ptrEnd)
{
	return S_FALSE;
}

/// <summary>
/// We must always reset the current advise time to zero after a timer fires
/// because there are several possible ways which lead us not to do any more
/// scheduling such as the pending image being cleared after state changes
/// </summary>
void CBaseRenderer::SignalTimerFired()
{
	m_dwAdvise = 0;
}

/// <summary>
/// Cancel any notification currently scheduled. This is called by the owning
/// window object when it is told to stop streaming. If there is no timer link
/// outstanding then calling this is benign otherwise we go ahead and cancel
/// We must always reset the render event as the quality management code can
/// signal immediate rendering by setting the event without setting an advise
/// link. If we're subsequently stopped and run the first attempt to setup an
/// advise link with the reference clock will find the event still signalled
/// </summary>
HRESULT CBaseRenderer::CancelNotification()
{
	Q_ASSERT(m_dwAdvise == 0 || m_pClock);
	DWORD_PTR dwAdvise = m_dwAdvise;

	// Have we a live advise link
	if(m_dwAdvise) {
		m_pClock->Unadvise(m_dwAdvise);
		SignalTimerFired();
		Q_ASSERT(m_dwAdvise == 0);
	}

	// Clear the event and return our status
	m_RenderEvent.Reset();
	return (dwAdvise ? S_OK : S_FALSE);
}

/// <summary>
/// Responsible for setting up one shot advise links with the clock
/// Return FALSE if the sample is to be dropped (not drawn at all)
/// Return TRUE if the sample is to be drawn and in this case also
/// arrange for m_RenderEvent to be set at the appropriate time
/// </summary>
BOOL CBaseRenderer::ScheduleSample(IMediaSample *pMediaSample)
{
	REFERENCE_TIME StartSample, EndSample;

	// Is someone pulling our leg
	if(pMediaSample == NULL)
		return FALSE;

	// Get the next sample due up for rendering.  If there aren't any ready
	// then GetNextSampleTimes returns an error.  If there is one to be done
	// then it succeeds and yields the sample times. If it is due now then
	// it returns S_OK other if it's to be done when due it returns S_FALSE
	HRESULT hr = GetSampleTimes(pMediaSample, &StartSample, &EndSample);
	if(FAILED(hr))
		return FALSE;

	// If we don't have a reference clock then we cannot set up the advise
	// time so we simply set the event indicating an image to render. This
	// will cause us to run flat out without any timing or synchronisation
	if(hr == S_OK) {
		BOOL tmp = SetEvent((HANDLE) m_RenderEvent);
		Q_ASSERT(tmp);
		return TRUE;
	}

	Q_ASSERT(m_dwAdvise == 0);
	Q_ASSERT(m_pClock);
	Q_ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));

	// We do have a valid reference clock interface so we can ask it to
	// set an event when the image comes due for rendering. We pass in
	// the reference time we were told to start at and also the current
	// stream time which is the offset from the start reference time
	hr = m_pClock->AdviseTime(
		(REFERENCE_TIME) m_tStart,		// Start run time
		StartSample,					// Stream time
		(HEVENT)(HANDLE) m_RenderEvent,	// Render notification
		&m_dwAdvise);					// Advise cookie
	if(SUCCEEDED(hr))
		return TRUE;

	// We could not schedule the next sample for rendering despite the fact
	// we have a valid sample here. This is a fair indication that either
	// the system clock is wrong or the time stamp for the sample is duff
	Q_ASSERT(m_dwAdvise == 0);
	return FALSE;
}

/// <summary>
/// This is called when a sample comes due for rendering. We pass the sample
/// on to the derived class. After rendering we will initialise the timer for
/// the next sample, NOTE signal that the last one fired first, if we don't
/// do this it thinks there is still one outstanding that hasn't completed
/// </summary>
HRESULT CBaseRenderer::Render(IMediaSample *pMediaSample)
{
	// If the media sample is NULL then we will have been notified by the
	// clock that another sample is ready but in the mean time someone has
	// stopped us streaming which causes the next sample to be released
	if(pMediaSample == NULL)
		return S_FALSE;

	// If we have stopped streaming then don't render any more samples, the
	// thread that got in and locked us and then reset this flag does not
	// clear the pending sample as we can use it to refresh any output device
	if(m_bStreaming == FALSE)
		return S_FALSE;

	// Time how long the rendering takes
	OnRenderStart(pMediaSample);
	DoRenderSample(pMediaSample);
	OnRenderEnd(pMediaSample);

	return NOERROR;
}

/// <summary>
/// Checks if there is a sample waiting at the renderer
/// </summary>
BOOL CBaseRenderer::HaveCurrentSample()
{
	CAutoLock cRendererLock(&m_RendererLock);
	return (m_pMediaSample == NULL ? FALSE : TRUE);
}

/// <summary>
/// Returns the current sample waiting at the video renderer. We AddRef the
/// sample before returning so that should it come due for rendering the
/// person who called this method will hold the remaining reference count
/// that will stop the sample being added back onto the allocator free list
/// </summary>
IMediaSample *CBaseRenderer::GetCurrentSample()
{
	CAutoLock cRendererLock(&m_RendererLock);
	if(m_pMediaSample)
		m_pMediaSample->AddRef();
	return m_pMediaSample;
}

/// <summary>
/// Called when the source delivers us a sample. We go through a few checks to
/// make sure the sample can be rendered. If we are running (streaming) then we
/// have the sample scheduled with the reference clock, if we are not streaming
/// then we have received an sample in paused mode so we can complete any state
/// transition. On leaving this function everything will be unlocked so an app
/// thread may get in and change our state to stopped (for example) in which
/// case it will also signal the thread event so that our wait call is stopped
/// </summary>
HRESULT CBaseRenderer::PrepareReceive(IMediaSample *pMediaSample)
{
	CAutoLock cInterfaceLock(&m_InterfaceLock);
	m_bInReceive = TRUE;

	// Check our flushing and filter state

	// This function must hold the interface lock because it calls
	// CBaseInputPin::Receive() and CBaseInputPin::Receive() uses
	// CBasePin::m_bRunTimeError.
	HRESULT hr = m_pInputPin->CBaseInputPin::Receive(pMediaSample);
	if(hr != NOERROR) {
		m_bInReceive = FALSE;
		return E_FAIL;
	}

	// Has the type changed on a media sample. We do all rendering
	// synchronously on the source thread, which has a side effect
	// that only one buffer is ever outstanding. Therefore when we
	// have Receive called we can go ahead and change the format
	// Since the format change can cause a SendMessage we just don't
	// lock
	if(m_pInputPin->SampleProps()->pMediaType) {
		hr = m_pInputPin->SetMediaType(
			(CMediaType *)m_pInputPin->SampleProps()->pMediaType);
		if(FAILED(hr)) {
			m_bInReceive = FALSE;
			return hr;
		}
	}

	CAutoLock cSampleLock(&m_RendererLock);

	Q_ASSERT(IsActive() == TRUE);
	Q_ASSERT(m_pInputPin->IsFlushing() == FALSE);
	Q_ASSERT(m_pInputPin->IsConnected() == TRUE);
	Q_ASSERT(m_pMediaSample == NULL);

	// Return an error if we already have a sample waiting for rendering
	// source pins must serialise the Receive calls - we also check that
	// no data is being sent after the source signalled an end of stream
	if(m_pMediaSample || m_bEOS || m_bAbort) {
		Ready();
		m_bInReceive = FALSE;
		return E_UNEXPECTED;
	}

	// Store the media times from this sample
	if(m_pPosition)
		m_pPosition->RegisterMediaTime(pMediaSample);

	// Schedule the next sample if we are streaming
	if((m_bStreaming == TRUE) && (ScheduleSample(pMediaSample) == FALSE)) {
		Q_ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
		hr = CancelNotification();
		Q_ASSERT(hr == S_FALSE);
		m_bInReceive = FALSE;
		return VFW_E_SAMPLE_REJECTED;
	}

	// Store the sample end time for EC_COMPLETE handling
	m_SignalTime = m_pInputPin->SampleProps()->tStop;

	// BEWARE we sometimes keep the sample even after returning the thread to
	// the source filter such as when we go into a stopped state (we keep it
	// to refresh the device with) so we must AddRef it to keep it safely. If
	// we start flushing the source thread is released and any sample waiting
	// will be released otherwise GetBuffer may never return (see BeginFlush)
	m_pMediaSample = pMediaSample;
	m_pMediaSample->AddRef();

	if(m_bStreaming == FALSE)
		SetRepaintStatus(TRUE);
	return NOERROR;
}

/// <summary>
/// Called by the source filter when we have a sample to render. Under normal
/// circumstances we set an advise link with the clock, wait for the time to
/// arrive and then render the data using the PURE virtual DoRenderSample that
/// the derived class will have overriden. After rendering the sample we may
/// also signal EOS if it was the last one sent before EndOfStream was called
/// </summary>
HRESULT CBaseRenderer::Receive(IMediaSample *pSample)
{
	Q_ASSERT(pSample);

	// It may return VFW_E_SAMPLE_REJECTED code to say don't bother
	HRESULT hr = PrepareReceive(pSample);
	Q_ASSERT(m_bInReceive == SUCCEEDED(hr));
	if(FAILED(hr)) {
		if(hr == VFW_E_SAMPLE_REJECTED)
			return NOERROR;
		return hr;
	}

	// We realize the palette in "PrepareRender()" so we have to give away the
	// filter lock here.
	if(m_State == State_Paused) {
		PrepareRender();
		// no need to use InterlockedExchange
		m_bInReceive = FALSE;
		{
			// We must hold both these locks
			CAutoLock cRendererLock(&m_InterfaceLock);
			if(m_State == State_Stopped)
				return NOERROR;

			m_bInReceive = TRUE;
			CAutoLock cSampleLock(&m_RendererLock);
			OnReceiveFirstSample(pSample);
		}
		Ready();
	}

	// Having set an advise link with the clock we sit and wait. We may be
	// awoken by the clock firing or by a state change. The rendering call
	// will lock the critical section and check we can still render the data
	hr = WaitForRenderTime();
	if(FAILED(hr)) {
		m_bInReceive = FALSE;
		return NOERROR;
	}

	PrepareRender();

	// Set this here and poll it until we work out the locking correctly
	// It can't be right that the streaming stuff grabs the interface
	// lock - after all we want to be able to wait for this stuff
	// to complete
	m_bInReceive = FALSE;

	// We must hold both these locks
	CAutoLock cRendererLock(&m_InterfaceLock);

	// Since we gave away the filter wide lock, the sate of the filter could
	// have chnaged to Stopped
	if(m_State == State_Stopped)
		return NOERROR;

	CAutoLock cSampleLock(&m_RendererLock);

	// Deal with this sample
	Render(m_pMediaSample);
	ClearPendingSample();
	SendEndOfStream();
	CancelNotification();
	return NOERROR;
}

/// <summary>
/// This is called when we stop or are inactivated to clear the pending sample
/// We release the media sample interface so that they can be allocated to the
/// source filter again, unless of course we are changing state to inactive in
/// which case GetBuffer will return an error. We must also reset the current
/// media sample to NULL so that we know we do not currently have an image
/// </summary>
HRESULT CBaseRenderer::ClearPendingSample()
{
	CAutoLock cRendererLock(&m_RendererLock);
	if(m_pMediaSample) {
		m_pMediaSample->Release();
		m_pMediaSample = NULL;
	}
	return NOERROR;
}

/// <summary>
/// Used to signal end of stream according to the sample end time
/// </summary>
void CALLBACK EndOfStreamTimer(
	UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2)
{
	CBaseRenderer *pRenderer = (CBaseRenderer *)dwUser;
	//appLog(LOG_CAT) << QStringLiteral(
	//	"EndOfStreamTimer called (%1)").arg(uID);
	pRenderer->TimerCallback();
}

/// <summary>
/// Do the timer callback work
/// </summary>
void CBaseRenderer::TimerCallback()
{
	// Lock for synchronization (but don't hold this lock when calling
	// timeKillEvent)
	CAutoLock cRendererLock(&m_RendererLock);

	// See if we should signal end of stream now
	if(m_EndOfStreamTimer) {
		m_EndOfStreamTimer = 0;
		SendEndOfStream();
	}
}

/// <summary>
/// If we are at the end of the stream signal the filter graph but do not set
/// the state flag back to FALSE. Once we drop off the end of the stream we
/// leave the flag set (until a subsequent ResetEndOfStream). Each sample we
/// get delivered will update m_SignalTime to be the last sample's end time.
/// We must wait this long before signalling end of stream to the filtergraph
/// </summary>
HRESULT CBaseRenderer::SendEndOfStream()
{
#define TIMEOUT_DELIVERYWAIT 50
#define TIMEOUT_RESOLUTION 10

	if(m_bEOS == FALSE || m_bEOSDelivered || m_EndOfStreamTimer)
		return NOERROR;

	// If there is no clock then signal immediately
	if(m_pClock == NULL)
		return NotifyEndOfStream();

	// How long into the future is the delivery time
	REFERENCE_TIME Signal = m_tStart + m_SignalTime;
	REFERENCE_TIME CurrentTime;
	m_pClock->GetTime(&CurrentTime);
	LONG Delay = LONG((Signal - CurrentTime) / 10000);

#if 0
	// Dump the timing information to the debugger
	appLog(LOG_CAT) << QStringLiteral("Delay until end of stream delivery %1")
		.arg(Delay);
	appLog(LOG_CAT) << QStringLiteral("Current %L1")
		.arg(CurrentTime);
	appLog(LOG_CAT) << QStringLiteral("Signal %L1")
		.arg(Signal);
#endif // 0

	// Wait for the delivery time to arrive
	if(Delay < TIMEOUT_DELIVERYWAIT)
		return NotifyEndOfStream();

	// Signal a timer callback on another worker thread
	m_EndOfStreamTimer = timeSetEvent(
		(UINT)Delay,		// Period of timer
		TIMEOUT_RESOLUTION,	// Timer resolution
		EndOfStreamTimer,	// Callback function
		(DWORD_PTR)this,	// Used information
		TIME_ONESHOT | TIME_KILL_SYNCHRONOUS); // Type of callback
	if(m_EndOfStreamTimer == 0)
		return NotifyEndOfStream();
	return NOERROR;
}

/// <summary>
/// Signals EC_COMPLETE to the filtergraph manager
/// </summary>
HRESULT CBaseRenderer::NotifyEndOfStream()
{
	CAutoLock cRendererLock(&m_RendererLock);
	Q_ASSERT(m_bEOSDelivered == FALSE);
	Q_ASSERT(m_EndOfStreamTimer == 0);

	// Has the filter changed state
	if (m_bStreaming == FALSE) {
		Q_ASSERT(m_EndOfStreamTimer == 0);
		return NOERROR;
	}

	// Reset the end of stream timer
	m_EndOfStreamTimer = 0;

	// If we've been using the IMediaPosition interface, set it's start
	// and end media "times" to the stop position by hand.  This ensures
	// that we actually get to the end, even if the MPEG guestimate has
	// been bad or if the quality management dropped the last few frames
	if(m_pPosition)
		m_pPosition->EOS();
	m_bEOSDelivered = TRUE;
	//appLog(LOG_CAT) << "Sending EC_COMPLETE...";
	return NotifyEvent(EC_COMPLETE, S_OK, (LONG_PTR)(IBaseFilter *)this);
}

/// <summary>
/// Reset the end of stream flag, this is typically called when we transfer to
/// stopped states since that resets the current position back to the start so
/// we will receive more samples or another EndOfStream if there aren't any. We
/// keep two separate flags one to say we have run off the end of the stream
/// (this is the m_bEOS flag) and another to say we have delivered EC_COMPLETE
/// to the filter graph. We need the latter otherwise we can end up sending an
/// EC_COMPLETE every time the source changes state and calls our EndOfStream
/// </summary>
HRESULT CBaseRenderer::ResetEndOfStream()
{
	ResetEndOfStreamTimer();
	CAutoLock cRendererLock(&m_RendererLock);

	m_bEOS = FALSE;
	m_bEOSDelivered = FALSE;
	m_SignalTime = 0;

	return NOERROR;
}

/// <summary>
/// Kills any outstanding end of stream timer
/// </summary>
void CBaseRenderer::ResetEndOfStreamTimer()
{
	if(m_EndOfStreamTimer) {
		timeKillEvent(m_EndOfStreamTimer);
		m_EndOfStreamTimer = 0;
	}
}

/// <summary>
/// This is called when we start running so that we can schedule any pending
/// image we have with the clock and display any timing information. If we
/// don't have any sample but we have queued an EOS flag then we send it. If
/// we do have a sample then we wait until that has been rendered before we
/// signal the filter graph otherwise we may change state before it's done
/// </summary>
HRESULT CBaseRenderer::StartStreaming()
{
	CAutoLock cRendererLock(&m_RendererLock);
	if(m_bStreaming == TRUE)
		return NOERROR;

	// Reset the streaming times ready for running
	m_bStreaming = TRUE;

	//timeBeginPeriod(1); // Should already be done
	OnStartStreaming();

	// There should be no outstanding advise
	Q_ASSERT(WAIT_TIMEOUT == WaitForSingleObject((HANDLE)m_RenderEvent,0));
	HRESULT hr = CancelNotification();
	Q_ASSERT(hr == S_FALSE);

	// If we have an EOS and no data then deliver it now
	if (m_pMediaSample == NULL)
		return SendEndOfStream();

	// Have the data rendered
	Q_ASSERT(m_pMediaSample);
	if(!ScheduleSample(m_pMediaSample))
		m_RenderEvent.Set();

	return NOERROR;
}

/// <summary>
/// This is called when we stop streaming so that we can set our internal flag
/// indicating we are not now to schedule any more samples arriving. The state
/// change methods in the filter implementation take care of cancelling any
/// clock advise link we have set up and clearing any pending sample we have
/// </summary>
HRESULT CBaseRenderer::StopStreaming()
{
	CAutoLock cRendererLock(&m_RendererLock);
	m_bEOSDelivered = FALSE;

	if(m_bStreaming == TRUE) {
		m_bStreaming = FALSE;
		OnStopStreaming();
		//timeEndPeriod(1);
	}
	return NOERROR;
}

/// <summary>
/// We have a boolean flag that is reset when we have signalled EC_REPAINT to
/// the filter graph. We set this when we receive an image so that should any
/// conditions arise again we can send another one. By having a flag we ensure
/// we don't flood the filter graph with redundant calls. We do not set the
/// event when we receive an EndOfStream call since there is no point in us
/// sending further EC_REPAINTs. In particular the AutoShowWindow method and
/// the DirectDraw object use this method to control the window repainting
/// </summary>
void CBaseRenderer::SetRepaintStatus(BOOL bRepaint)
{
	CAutoLock cSampleLock(&m_RendererLock);
	m_bRepaintStatus = bRepaint;
}

/// <summary>
/// Pass the window handle to the upstream filter
/// </summary>
void CBaseRenderer::SendNotifyWindow(IPin *pPin,HWND hwnd)
{
	IMediaEventSink *pSink;

	// Does the pin support IMediaEventSink
	HRESULT hr = pPin->QueryInterface(IID_IMediaEventSink, (void **)&pSink);
	if(SUCCEEDED(hr)) {
		pSink->Notify(EC_NOTIFY_WINDOW, LONG_PTR(hwnd), 0);
		pSink->Release();
	}
	NotifyEvent(EC_NOTIFY_WINDOW, LONG_PTR(hwnd), 0);
}

/// <summary>
/// Signal an EC_REPAINT to the filter graph. This can be used to have data
/// sent to us. For example when a video window is first displayed it may
/// not have an image to display, at which point it signals EC_REPAINT. The
/// filtergraph will either pause the graph if stopped or if already paused
/// it will call put_CurrentPosition of the current position. Setting the
/// current position to itself has the stream flushed and the image resent
/// </summary>
void CBaseRenderer::SendRepaint()
{
	CAutoLock cSampleLock(&m_RendererLock);
	Q_ASSERT(m_pInputPin);

	// We should not send repaint notifications when...
	//    - An end of stream has been notified
	//    - Our input pin is being flushed
	//    - The input pin is not connected
	//    - We have aborted a video playback
	//    - There is a repaint already sent

	if(m_bAbort != FALSE)
		return;
	if(m_pInputPin->IsConnected() != TRUE)
		return;
	if(m_pInputPin->IsFlushing() != FALSE)
		return;
	if(IsEndOfStream() != FALSE)
		return;
	if(m_bRepaintStatus != TRUE)
		return;
	IPin *pPin = (IPin *) m_pInputPin;
	NotifyEvent(EC_REPAINT,(LONG_PTR) pPin,0);
	SetRepaintStatus(FALSE);
	//appLog(LOG_CAT) << "Sending repaint";
}

/// <summary>
/// When a video window detects a display change (WM_DISPLAYCHANGE message) it
/// can send an EC_DISPLAY_CHANGED event code along with the renderer pin. The
/// filtergraph will stop everyone and reconnect our input pin. As we're then
/// reconnected we can accept the media type that matches the new display mode
/// since we may no longer be able to draw the current image type efficiently
/// </summary>
BOOL CBaseRenderer::OnDisplayChange()
{
	// Ignore if we are not connected yet

	CAutoLock cSampleLock(&m_RendererLock);
	if(m_pInputPin->IsConnected() == FALSE)
		return FALSE;

	//appLog(LOG_CAT) << "Notification of EC_DISPLAY_CHANGE";

	// Pass our input pin as parameter on the event
	IPin *pPin = (IPin *) m_pInputPin;
	m_pInputPin->AddRef();
	NotifyEvent(EC_DISPLAY_CHANGED,(LONG_PTR) pPin,0);
	SetAbortSignal(TRUE);
	ClearPendingSample();
	m_pInputPin->Release();

	return TRUE;
}

/// <summary>
/// Called just before we start drawing.
/// Store the current time in m_trRenderStart to allow the rendering time to be
/// logged.  Log the time stamp of the sample and how late it is (neg is early)
/// </summary>
void CBaseRenderer::OnRenderStart(IMediaSample *pMediaSample)
{
}

/// <summary>
/// Called directly after drawing an image.
/// calculate the time spent drawing and log it.
/// </summary>
void CBaseRenderer::OnRenderEnd(IMediaSample *pMediaSample)
{
}
