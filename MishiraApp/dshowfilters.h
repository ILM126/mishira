//*****************************************************************************
// Mishira: An audiovisual production tool for broadcasting live video
//
// Copyright (C) 1992-2001 Microsoft Corporation
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
// All of these classes are stripped down and slightly modified ports of
// Microsoft's official DirectShow sample base classes from the Windows 7 SDK.
// We cannot use the classes directly from the SDK as the classes are not
// included in the Windows 8 SDK (DirectShow is deprecated) and we don't want
// to have to depend on the Windows 7 SDK in addition to the Windows 8 SDK.
//*****************************************************************************

#ifndef DSHOWFILTERS_H
#define DSHOWFILTERS_H

#include "common.h"
#include <dshow.h>

class CBaseFilter;
class CBasePin;
class CBaseRenderer;

void FreeMediaType(AM_MEDIA_TYPE &mt);
void DeleteMediaType(AM_MEDIA_TYPE *pmt);
HRESULT CopyMediaType(
	AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource);
HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
void RemoveFromRot(DWORD pdwRegister);

//=============================================================================
class CCritSec
{
	// Make copy constructor and assignment operator inaccessible
	CCritSec(const CCritSec &refCritSec);
	CCritSec &operator=(const CCritSec &refCritSec);

private: // Members -----------------------------------------------------------
	CRITICAL_SECTION	m_CritSec;

public: // Constructor/destructor ---------------------------------------------
	CCritSec();
	~CCritSec();

public: // Methods ------------------------------------------------------------
	void Lock();
	void Unlock();
};
//=============================================================================

inline CCritSec::CCritSec()
{
	InitializeCriticalSection(&m_CritSec);
};

inline CCritSec::~CCritSec()
{
	DeleteCriticalSection(&m_CritSec);
};

inline void CCritSec::Lock()
{
	EnterCriticalSection(&m_CritSec);
};

inline void CCritSec::Unlock()
{
	LeaveCriticalSection(&m_CritSec);
};

//=============================================================================
class CAutoLock
{
	// Make copy constructor and assignment operator inaccessible
	CAutoLock(const CAutoLock &refAutoLock);
	CAutoLock &operator=(const CAutoLock &refAutoLock);

protected: // Members ---------------------------------------------------------
	CCritSec *	m_pLock;

public: // Constructor/destructor ---------------------------------------------
	CAutoLock(CCritSec *plock);
	~CAutoLock();
};
//=============================================================================

inline CAutoLock::CAutoLock(CCritSec *plock)
{
	m_pLock = plock;
	m_pLock->Lock();
}

inline CAutoLock::~CAutoLock()
{
	m_pLock->Unlock();
}

//=============================================================================
class CAMEvent
{
	// Make copy constructor and assignment operator inaccessible
	CAMEvent(const CAMEvent &refEvent);
	CAMEvent &operator=(const CAMEvent &refEvent);

protected: // Members ---------------------------------------------------------
	HANDLE	m_hEvent;

public: // Constructor/destructor ---------------------------------------------
	CAMEvent(BOOL fManualReset = FALSE, HRESULT *phr = NULL);
	CAMEvent(HRESULT *phr);
	~CAMEvent();

public: // Methods ------------------------------------------------------------
	// Cast to HANDLE - we don't support this as an lvalue
	operator HANDLE() const { return m_hEvent; };

	void	Set();
	BOOL	Wait(DWORD dwTimeout = INFINITE);
	void	Reset();
	BOOL	Check();
};
//=============================================================================

inline void CAMEvent::Set()
{
	BOOL ret = SetEvent(m_hEvent);
	Q_ASSERT(ret);
}

inline BOOL CAMEvent::Wait(DWORD dwTimeout)
{
	return (WaitForSingleObject(m_hEvent, dwTimeout) == WAIT_OBJECT_0);
}

inline void CAMEvent::Reset()
{
	ResetEvent(m_hEvent);
}

inline BOOL CAMEvent::Check()
{
	return Wait(0);
}

//=============================================================================
class CRefTime
{
private: // Constants ---------------------------------------------------------
	static const LONGLONG	MILLISECONDS = (1000);			// 10 ^ 3
	static const LONGLONG	NANOSECONDS = (1000000000);		// 10 ^ 9
	static const LONGLONG	UNITS = (NANOSECONDS / 100);	// 10 ^ 7

public: // Members ------------------------------------------------------------
	// *MUST* be the only data member so that this class is exactly equivalent
	// to a REFERENCE_TIME. Also, must be *no virtual functions*
	REFERENCE_TIME	m_time;

public: // Constructor/destructor ---------------------------------------------
	CRefTime();
	CRefTime(LONG msecs);
	CRefTime(REFERENCE_TIME rt);

	CRefTime &	operator=(const CRefTime& rt);
	CRefTime &	operator=(const LONGLONG ll);

public: // Methods ------------------------------------------------------------
	inline operator REFERENCE_TIME() const { return m_time; };

	inline CRefTime &	operator+=(const CRefTime& rt);
	inline CRefTime &	operator-=(const CRefTime& rt);

	inline LONG			Millisecs(void);
	inline LONGLONG		GetUnits(void);
};
//=============================================================================

inline CRefTime::CRefTime()
{
	m_time = 0;
}

inline CRefTime::CRefTime(LONG msecs)
{
	m_time = Int32x32To64((msecs), (UNITS / MILLISECONDS));
}

inline CRefTime::CRefTime(REFERENCE_TIME rt)
{
	m_time = rt;
}

inline CRefTime &CRefTime::operator=(const CRefTime &rt)
{
	m_time = rt.m_time;
	return *this;
}

inline CRefTime &CRefTime::operator=(const LONGLONG ll)
{
	m_time = ll;
	return *this;
}

inline CRefTime &CRefTime::operator+=(const CRefTime& rt)
{
	return (*this = *this + rt);
}

inline CRefTime &CRefTime::operator-=(const CRefTime& rt)
{
	return (*this = *this - rt);
}

inline LONG CRefTime::Millisecs(void)
{
	return (LONG)(m_time / (UNITS / MILLISECONDS));
}

inline LONGLONG CRefTime::GetUnits(void)
{
	return m_time;
}

//=============================================================================
class AM_NOVTABLE CUnknown : public IUnknown
{
private: // Static members ----------------------------------------------------
	static LONG		m_cObjects; // Total number of objects active

private: // Members -----------------------------------------------------------
	LPUNKNOWN		m_pUnknown; // Owner of this object
protected:
	volatile LONG	m_cRef; // Number of reference counts

public: // Static methods -----------------------------------------------------
	static LONG		ObjectsActive();

public: // Constructor/destructor ---------------------------------------------
	CUnknown(LPCTSTR pName, LPUNKNOWN pUnk);
	CUnknown(LPCTSTR Name, LPUNKNOWN pUnk, HRESULT *phr);
	CUnknown(LPCSTR pName, LPUNKNOWN pUnk);
	CUnknown(LPCSTR pName, LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CUnknown();

public: // Methods ------------------------------------------------------------
	LPUNKNOWN				GetOwner() const;

	// IUnknown delegate implementation
	virtual STDMETHODIMP			DelegateQueryInterface(REFIID, void **);
	virtual STDMETHODIMP_(ULONG)	DelegateAddRef();
	virtual STDMETHODIMP_(ULONG)	DelegateRelease();
};
//=============================================================================

// WARNING: All derived classes but use the `Delegate*` CUnknown interface and
// include this macro in the class definition
#define DECLARE_IUNKNOWN \
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { \
	return DelegateQueryInterface(riid, ppv);              \
};                                                         \
	STDMETHODIMP_(ULONG) AddRef() {                        \
	return DelegateAddRef();                               \
};                                                         \
	STDMETHODIMP_(ULONG) Release() {                       \
	return DelegateRelease();                              \
}

inline LONG CUnknown::ObjectsActive()
{
	return m_cObjects;
}

inline LPUNKNOWN CUnknown::GetOwner() const
{
	return m_pUnknown;
}

//=============================================================================
class CMediaType : public _AMMediaType
{
public: // Static methods -----------------------------------------------------
	// General purpose functions to copy and delete a task allocated
	// AM_MEDIA_TYPE structure which is useful when using the IEnumMediaFormats
	// interface as the implementation allocates the structures which you must
	// later delete
	static void WINAPI DeleteMediaType(AM_MEDIA_TYPE *pmt);
	static AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const *pSrc);
	static HRESULT WINAPI CopyMediaType(
		AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource);
	static void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt);

public: // Constructor/destructor ---------------------------------------------
	CMediaType();
	CMediaType(const GUID * majortype);
	CMediaType(const AM_MEDIA_TYPE&, HRESULT* phr = NULL);
	CMediaType(const CMediaType&, HRESULT* phr = NULL);
	~CMediaType();

	CMediaType& operator=(const CMediaType&);
	CMediaType& operator=(const AM_MEDIA_TYPE&);

public: // Methods ------------------------------------------------------------
	BOOL operator==(const CMediaType&) const;
	BOOL operator!=(const CMediaType&) const;

	HRESULT Set(const CMediaType& rt);
	HRESULT Set(const AM_MEDIA_TYPE& rt);

	BOOL IsValid() const;

	const GUID *Type() const { return &majortype; };
	void SetType(const GUID *);
	const GUID *Subtype() const { return &subtype; };
	void SetSubtype(const GUID *);

	BOOL IsFixedSize() const { return bFixedSizeSamples; };
	BOOL IsTemporalCompressed() const { return bTemporalCompression; };
	ULONG GetSampleSize() const;

	void SetSampleSize(ULONG sz);
	void SetVariableSize();
	void SetTemporalCompression(BOOL bCompressed);

	// read/write pointer to format - can't change length without
	// calling SetFormat, AllocFormatBuffer or ReallocFormatBuffer

	BYTE*   Format() const {return pbFormat; };
	ULONG   FormatLength() const { return cbFormat; };

	void SetFormatType(const GUID *);
	const GUID *FormatType() const {return &formattype; };
	BOOL SetFormat(BYTE *pFormat, ULONG length);
	void ResetFormatBuffer();
	BYTE* AllocFormatBuffer(ULONG length);
	BYTE* ReallocFormatBuffer(ULONG length);

	void InitMediaType();

	BOOL MatchesPartial(const CMediaType* ppartial) const;
	BOOL IsPartiallySpecified(void) const;
};
//=============================================================================

//=============================================================================
class CEnumMediaTypes : public IEnumMediaTypes
{
private: // Members -----------------------------------------------------------
	int			m_Position;	// Current ordinal position
	CBasePin *	m_pPin;		// The pin who owns us
	LONG		m_Version;	// Media type version value
	LONG		m_cRef;

public: // Constructor/destructor ---------------------------------------------
	CEnumMediaTypes(CBasePin *pPin, CEnumMediaTypes *pEnumMediaTypes);
	virtual ~CEnumMediaTypes();

public: // Methods ------------------------------------------------------------
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IEnumMediaTypes
	STDMETHODIMP Next(
		ULONG cMediaTypes,             // place this many pins...
		AM_MEDIA_TYPE **ppMediaTypes,  // ...in this array
		ULONG *pcFetched               // actual count passed
		);

	STDMETHODIMP Skip(ULONG cMediaTypes);
	STDMETHODIMP Reset();
	STDMETHODIMP Clone(IEnumMediaTypes **ppEnum);

private:
	BOOL AreWeOutOfSync();
};
//=============================================================================

//=============================================================================
class CBaseDispatch
{
private: // Members -----------------------------------------------------------
	ITypeInfo *	m_pti;

public: // Constructor/destructor ---------------------------------------------
	CBaseDispatch() : m_pti(NULL) {}
	~CBaseDispatch();

public: // Methods ------------------------------------------------------------
	// IDispatch methods
	STDMETHODIMP GetTypeInfoCount(UINT *pctinfo);
	STDMETHODIMP GetTypeInfo(
		REFIID riid, UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
	STDMETHODIMP GetIDsOfNames(
		REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid,
		DISPID *rgdispid);
};
//=============================================================================

//=============================================================================
class AM_NOVTABLE CMediaPosition : public IMediaPosition, public CUnknown
{
private: // Members -----------------------------------------------------------
	CBaseDispatch	m_basedisp;

public: // Constructor/destructor ---------------------------------------------
	CMediaPosition(LPCTSTR, LPUNKNOWN);
	CMediaPosition(LPCTSTR, LPUNKNOWN, HRESULT *phr);

public: // Methods ------------------------------------------------------------
	DECLARE_IUNKNOWN;
	STDMETHODIMP DelegateQueryInterface(REFIID riid, void **ppv);

	// IDispatch methods
	STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
	STDMETHODIMP GetTypeInfo(
		UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
	STDMETHODIMP GetIDsOfNames(
		REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid,
		DISPID *rgdispid);
	STDMETHODIMP Invoke(
		DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
		DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo,
		UINT *puArgErr);
};
//=============================================================================

//=============================================================================
/// <summary>
/// A utility class that handles IMediaPosition and IMediaSeeking on behalf
/// of single-input pin renderers, or transform filters.
///
/// Renderers will expose this from the filter; transform filters will
/// expose it from the output pin and not the renderer.
///
/// Create one of these, giving it your IPin* for your input pin, and delegate
/// all IMediaPosition methods to it. It will query the input pin for
/// IMediaPosition and respond appropriately.
///
/// Call ForceRefresh if the pin connection changes.
///
/// This class no longer caches the upstream IMediaPosition or IMediaSeeking
/// it acquires it on each method call. This means ForceRefresh is not needed.
/// The method is kept for source compatibility and to minimise the changes
/// if we need to put it back later for performance reasons.
/// </summary>
class CPosPassThru : public IMediaSeeking, public CMediaPosition
{
private: // Members -----------------------------------------------------------
	IPin *	m_pPin;

public: // Constructor/destructor ---------------------------------------------
	CPosPassThru(LPCTSTR, LPUNKNOWN, HRESULT *, IPin *);

private: // Methods -----------------------------------------------------------
	HRESULT GetPeer(IMediaPosition **ppMP);
	HRESULT GetPeerSeeking(IMediaSeeking **ppMS);

public:
	HRESULT ForceRefresh() {
		return S_OK;
	};

	// Override to return an accurate current position
	virtual HRESULT GetMediaTime(LONGLONG *pStartTime, LONGLONG *pEndTime) {
		return E_FAIL;
	}

	DECLARE_IUNKNOWN;
	STDMETHODIMP DelegateQueryInterface(REFIID riid, void **ppv);

	// IMediaSeeking methods
	STDMETHODIMP GetCapabilities(DWORD * pCapabilities);
	STDMETHODIMP CheckCapabilities( DWORD * pCapabilities);
	STDMETHODIMP SetTimeFormat(const GUID * pFormat);
	STDMETHODIMP GetTimeFormat(GUID *pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
	STDMETHODIMP IsFormatSupported( const GUID * pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
	STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget,
		const GUID * pTargetFormat,
		LONGLONG Source,
		const GUID * pSourceFormat );
	STDMETHODIMP SetPositions(LONGLONG * pCurrent, DWORD CurrentFlags,
		LONGLONG * pStop, DWORD StopFlags );

	STDMETHODIMP GetPositions(LONGLONG *pCurrent, LONGLONG *pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG * pCurrent);
	STDMETHODIMP GetStopPosition(LONGLONG * pStop);
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double * pdRate);
	STDMETHODIMP GetDuration(LONGLONG *pDuration);
	STDMETHODIMP GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest);
	STDMETHODIMP GetPreroll(LONGLONG *pllPreroll);

	// IMediaPosition properties
	STDMETHODIMP get_Duration(REFTIME *plength);
	STDMETHODIMP put_CurrentPosition(REFTIME llTime);
	STDMETHODIMP get_StopTime(REFTIME *pllTime);
	STDMETHODIMP put_StopTime(REFTIME llTime);
	STDMETHODIMP get_PrerollTime(REFTIME *pllTime);
	STDMETHODIMP put_PrerollTime(REFTIME llTime);
	STDMETHODIMP get_Rate(double *pdRate);
	STDMETHODIMP put_Rate(double dRate);
	STDMETHODIMP get_CurrentPosition(REFTIME *pllTime);
	STDMETHODIMP CanSeekForward(LONG *pCanSeekForward);
	STDMETHODIMP CanSeekBackward(LONG *pCanSeekBackward);

private:
	HRESULT GetSeekingLongLong(
		HRESULT (__stdcall IMediaSeeking::*pMethod)(LONGLONG *),
		LONGLONG *pll);
};
//=============================================================================

//=============================================================================
class CRendererPosPassThru : public CPosPassThru
{
private: // Members -----------------------------------------------------------
	CCritSec	m_PositionLock;	// Locks access to our position
	LONGLONG	m_StartMedia;	// Start media time last seen
	LONGLONG	m_EndMedia;		// And likewise the end media
	BOOL		m_bReset;		// Have media times been set

public: // Constructor/destructor ---------------------------------------------
	CRendererPosPassThru(LPCTSTR, LPUNKNOWN, HRESULT *, IPin *);

public: // Methods ------------------------------------------------------------
	HRESULT	RegisterMediaTime(IMediaSample *pMediaSample);
	HRESULT	RegisterMediaTime(LONGLONG StartTime, LONGLONG EndTime);
	HRESULT	GetMediaTime(LONGLONG *pStartTime, LONGLONG *pEndTime);
	HRESULT	ResetMediaTime();
	HRESULT	EOS();
};
//=============================================================================

//=============================================================================
class CEnumPins : public IEnumPins
{
private: // Members -----------------------------------------------------------
	int m_Position;         // Current ordinal position
	int m_PinCount;         // Number of pins available
	CBaseFilter *m_pFilter; // The filter who owns us
	LONG m_Version;         // Pin version information
	LONG m_cRef;

	typedef QVector<CBasePin *> CPinList;

	// These pointers have not been AddRef'ed and
	// so they should not be dereferenced.  They are
	// merely kept to ID which pins have been enumerated.
	CPinList m_PinCache;

public: // Constructor/destructor ---------------------------------------------
	CEnumPins(CBaseFilter *pFilter, CEnumPins *pEnumPins);
	virtual ~CEnumPins();

public: // Methods ------------------------------------------------------------
	// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();

	// IEnumPins
	STDMETHODIMP Next(
		ULONG cPins,      // place this many pins...
		IPin **ppPins,    // ...in this array of IPin*
		ULONG *pcFetched  // actual count passed returned here
		);

	STDMETHODIMP Skip(ULONG cPins);
	STDMETHODIMP Reset();
	STDMETHODIMP Clone(IEnumPins **ppEnum);

private:
	// If while we are retrieving a pin for example from the filter an error
	// occurs we assume that our internal state is stale with respect to the
	// filter (someone may have deleted all the pins). We can check before
	// starting whether or not the operation is likely to fail by asking the
	// filter what it's current version number is. If the filter has not
	// overriden the GetPinVersion method then this will always match
	BOOL AreWeOutOfSync();

	// This method performs the same operations as Reset, except is does not
	// clear the cache of pins already enumerated.
	STDMETHODIMP Refresh();
};
//=============================================================================

//=============================================================================
class AM_NOVTABLE CBasePin : public CUnknown, public IPin, public IQualityControl
{
protected: // Members ---------------------------------------------------------
	WCHAR *         m_pName;		            // This pin's name
	IPin *          m_Connected;                // Pin we have connected to
	PIN_DIRECTION   m_dir;                      // Direction of this pin
	CCritSec *      m_pLock;                    // Object we use for locking
	bool            m_bRunTimeError;            // Run time error generated
	bool            m_bCanReconnectWhenActive;  // OK to reconnect when active
	bool            m_bTryMyTypesFirst;         // When connecting enumerate
	// this pin's types first
	CBaseFilter *   m_pFilter;                  // Filter we were created by
	IQualityControl * m_pQSink;                 // Target for Quality messages
	LONG            m_TypeVersion;              // Holds current type version
	CMediaType      m_mt;                       // Media type of connection

	CRefTime        m_tStart;                   // time from NewSegment call
	CRefTime        m_tStop;                    // time from NewSegment
	double          m_dRate;                    // rate from NewSegment

	// displays pin connection information
	void DisplayPinInfo(IPin *pReceivePin) {};
	void DisplayTypeInfo(IPin *pPin, const CMediaType *pmt) {};

	// used to agree a media type for a pin connection

	// given a specific media type, attempt a connection (includes
	// checking that the type is acceptable to this pin)
	HRESULT AttemptConnection(
		IPin* pReceivePin,      // connect to this pin
		const CMediaType* pmt   // using this type
		);

	// try all the media types in this enumerator - for each that
	// we accept, try to connect using ReceiveConnection.
	HRESULT TryMediaTypes(
		IPin *pReceivePin,       // connect to this pin
		const CMediaType *pmt,   // proposed type from Connect
		IEnumMediaTypes *pEnum); // try this enumerator

	// establish a connection with a suitable mediatype. Needs to
	// propose a media type if the pmt pointer is null or partially
	// specified - use TryMediaTypes on both our and then the other pin's
	// enumerator until we find one that works.
	HRESULT AgreeMediaType(
		IPin *pReceivePin,      // connect to this pin
		const CMediaType *pmt); // proposed type from Connect

public: // Constructor/destructor ---------------------------------------------
	CBasePin(
		LPCTSTR pObjectName,        // Object description
		CBaseFilter *pFilter,       // Owning filter who knows about pins
		CCritSec *pLock,            // Object who implements the lock
		HRESULT *phr,               // General OLE return code
		LPCWSTR pName,              // Pin name for us
		PIN_DIRECTION dir);         // Either PINDIR_INPUT or PINDIR_OUTPUT
	CBasePin(
		LPCSTR pObjectName,         // Object description
		CBaseFilter *pFilter,       // Owning filter who knows about pins
		CCritSec *pLock,            // Object who implements the lock
		HRESULT *phr,               // General OLE return code
		LPCWSTR pName,              // Pin name for us
		PIN_DIRECTION dir);         // Either PINDIR_INPUT or PINDIR_OUTPUT
	virtual ~CBasePin();

public: // Methods ------------------------------------------------------------
	DECLARE_IUNKNOWN;
	virtual STDMETHODIMP DelegateQueryInterface(REFIID riid, void **ppv);
	virtual STDMETHODIMP_(ULONG) DelegateRelease();
	virtual STDMETHODIMP_(ULONG) DelegateAddRef();

	// --- IPin methods ---

	// take lead role in establishing a connection. Media type pointer
	// may be null, or may point to partially-specified mediatype
	// (subtype or format type may be GUID_NULL).
	STDMETHODIMP Connect(
		IPin * pReceivePin,
		const AM_MEDIA_TYPE *pmt   // optional media type
		);

	// (passive) accept a connection from another pin
	STDMETHODIMP ReceiveConnection(
		IPin * pConnector,         // this is the initiating connecting pin
		const AM_MEDIA_TYPE *pmt   // this is the media type we will exchange
		);

	STDMETHODIMP Disconnect();

	STDMETHODIMP ConnectedTo(IPin **pPin);
	STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE *pmt);
	STDMETHODIMP QueryPinInfo(PIN_INFO *pInfo);
	STDMETHODIMP QueryDirection(PIN_DIRECTION * pPinDir);
	STDMETHODIMP QueryId(LPWSTR *Id);
	// Does the pin support this media type
	STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE *pmt);

	// Return an enumerator for this pins preferred media types
	STDMETHODIMP EnumMediaTypes(IEnumMediaTypes **ppEnum);

	// return an array of IPin* - the pins that this pin internally connects to
	// All pins put in the array must be AddReffed (but no others)
	// Errors: "Can't say" - FAIL, not enough slots - return S_FALSE
	// Default: return E_NOTIMPL
	// The filter graph will interpret NOT_IMPL as any input pin connects to
	// all visible output pins and vice versa.
	// apPin can be NULL if nPin==0 (not otherwise).
	STDMETHODIMP QueryInternalConnections(
		IPin* *apPin,     // array of IPin*
		ULONG *nPin                  // on input, the number of slots
		// on output  the number of pins
		) { return E_NOTIMPL; }

	// Called when no more data will be sent
	STDMETHODIMP EndOfStream(void);

	// Begin/EndFlush still PURE

	// NewSegment notifies of the start/stop/rate applying to the data
	// about to be received. Default implementation records data and
	// returns S_OK.
	// Override this to pass downstream.
	STDMETHODIMP NewSegment(
		REFERENCE_TIME tStart,
		REFERENCE_TIME tStop,
		double dRate);

	//=========================================================================
	// IQualityControl methods
	//=========================================================================

	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

	STDMETHODIMP SetSink(IQualityControl * piqc);

	// --- helper methods ---

	// Returns true if the pin is connected. false otherwise.
	BOOL IsConnected(void) {return (m_Connected != NULL); };
	// Return the pin this is connected to (if any)
	IPin * GetConnected() { return m_Connected; };

	// Check if our filter is currently stopped
	BOOL IsStopped();

	// find out the current type version (used by enumerators)
	virtual LONG GetMediaTypeVersion();
	void IncrementTypeVersion();

	// switch the pin to active (paused or running) mode
	// not an error to call this if already active
	virtual HRESULT Active(void);

	// switch the pin to inactive state - may already be inactive
	virtual HRESULT Inactive(void);

	// Notify of Run() from filter
	virtual HRESULT Run(REFERENCE_TIME tStart);

	// check if the pin can support this specific proposed type and format
	virtual HRESULT CheckMediaType(const CMediaType *) PURE;

	// set the connection to use this format (previously agreed)
	virtual HRESULT SetMediaType(const CMediaType *);

	// check that the connection is ok before verifying it
	// can be overridden eg to check what interfaces will be supported.
	virtual HRESULT CheckConnect(IPin *);

	// Set and release resources required for a connection
	virtual HRESULT BreakConnect();
	virtual HRESULT CompleteConnect(IPin *pReceivePin);

	// returns the preferred formats for a pin
	virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

	// access to NewSegment values
	REFERENCE_TIME CurrentStopTime() {
		return m_tStop;
	}
	REFERENCE_TIME CurrentStartTime() {
		return m_tStart;
	}
	double CurrentRate() {
		return m_dRate;
	}

	//  Access name
	LPWSTR Name() { return m_pName; };

	//  Can reconnectwhen active?
	void SetReconnectWhenActive(bool bCanReconnect)
	{
		m_bCanReconnectWhenActive = bCanReconnect;
	}

	bool CanReconnectWhenActive()
	{
		return m_bCanReconnectWhenActive;
	}

protected:
	STDMETHODIMP DisconnectInternal();
};
//=============================================================================

//=============================================================================
class AM_NOVTABLE CBaseInputPin : public CBasePin, public IMemInputPin
{
protected: // Members ---------------------------------------------------------
	IMemAllocator *m_pAllocator;    // Default memory allocator

	// allocator is read-only, so received samples
	// cannot be modified (probably only relevant to in-place
	// transforms
	BYTE m_bReadOnly;

	// in flushing state (between BeginFlush and EndFlush)
	// if TRUE, all Receives are returned with S_FALSE
	BYTE m_bFlushing;

	// Sample properties - initalized in Receive
	AM_SAMPLE2_PROPERTIES m_SampleProps;

public: // Constructor/destructor ---------------------------------------------
	CBaseInputPin(
		LPCTSTR pObjectName,
		CBaseFilter *pFilter,
		CCritSec *pLock,
		HRESULT *phr,
		LPCWSTR pName);
	CBaseInputPin(
		LPCSTR pObjectName,
		CBaseFilter *pFilter,
		CCritSec *pLock,
		HRESULT *phr,
		LPCWSTR pName);
	virtual ~CBaseInputPin();

public: // Methods ------------------------------------------------------------
	DECLARE_IUNKNOWN;
	virtual STDMETHODIMP DelegateQueryInterface(REFIID riid, void **ppv);

	// return the allocator interface that this input pin
	// would like the output pin to use
	STDMETHODIMP GetAllocator(IMemAllocator ** ppAllocator);

	// tell the input pin which allocator the output pin is actually
	// going to use.
	STDMETHODIMP NotifyAllocator(
		IMemAllocator * pAllocator,
		BOOL bReadOnly);

	// do something with this media sample
	STDMETHODIMP Receive(IMediaSample *pSample);

	// do something with these media samples
	STDMETHODIMP ReceiveMultiple (
		IMediaSample **pSamples,
		long nSamples,
		long *nSamplesProcessed);

	// See if Receive() blocks
	STDMETHODIMP ReceiveCanBlock();

	// Default handling for BeginFlush - call at the beginning
	// of your implementation (makes sure that all Receive calls
	// fail). After calling this, you need to free any queued data
	// and then call downstream.
	STDMETHODIMP BeginFlush(void);

	// default handling for EndFlush - call at end of your implementation
	// - before calling this, ensure that there is no queued data and no thread
	// pushing any more without a further receive, then call downstream,
	// then call this method to clear the m_bFlushing flag and re-enable
	// receives
	STDMETHODIMP EndFlush(void);

	// this method is optional (can return E_NOTIMPL).
	// default implementation returns E_NOTIMPL. Override if you have
	// specific alignment or prefix needs, but could use an upstream
	// allocator
	STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES*pProps);

	// Release the pin's allocator.
	HRESULT BreakConnect();

	// helper method to check the read-only flag
	BOOL IsReadOnly();

	// helper method to see if we are flushing
	BOOL IsFlushing();

	//  Override this for checking whether it's OK to process samples
	//  Also call this from EndOfStream.
	virtual HRESULT CheckStreaming();

	// Pass a Quality notification on to the appropriate sink
	HRESULT PassNotify(Quality& q);

	//=========================================================================
	// IQualityControl methods (from CBasePin)
	//=========================================================================

	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

	// no need to override:
	// STDMETHODIMP SetSink(IQualityControl * piqc);

	// switch the pin to inactive state - may already be inactive
	virtual HRESULT Inactive();

	// Return sample properties pointer
	AM_SAMPLE2_PROPERTIES * SampleProps() {
		Q_ASSERT(m_SampleProps.cbData != 0);
		return &m_SampleProps;
	}
};
//=============================================================================

inline BOOL CBaseInputPin::IsReadOnly()
{
	return m_bReadOnly;
}

inline BOOL CBaseInputPin::IsFlushing()
{
	return m_bFlushing;
}

//=============================================================================
class CRendererInputPin : public CBaseInputPin
{
protected: // Members ---------------------------------------------------------
	CBaseRenderer *m_pRenderer;

public: // Constructor/destructor ---------------------------------------------
	CRendererInputPin(
		CBaseRenderer *pRenderer, HRESULT *phr, LPCWSTR Name);

public: // Methods ------------------------------------------------------------
	// Overriden from the base pin classes

	HRESULT BreakConnect();
	HRESULT CompleteConnect(IPin *pReceivePin);
	HRESULT SetMediaType(const CMediaType *pmt);
	HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT Active();
	HRESULT Inactive();

	// Add rendering behaviour to interface functions

	STDMETHODIMP QueryId(LPWSTR *Id);
	STDMETHODIMP EndOfStream();
	STDMETHODIMP BeginFlush();
	STDMETHODIMP EndFlush();
	STDMETHODIMP Receive(IMediaSample *pMediaSample);

	// Helper
	IMemAllocator inline *Allocator() const
	{
		return m_pAllocator;
	}
};
//=============================================================================

//=============================================================================
class AM_NOVTABLE CBaseFilter : public CUnknown,        // Handles an IUnknown
	public IBaseFilter     // The Filter Interface
{
	friend class CBasePin;

protected: // Members ---------------------------------------------------------
	FILTER_STATE    m_State;            // current state: running, paused
	IReferenceClock *m_pClock;          // this graph's ref clock
	CRefTime        m_tStart;           // offset from stream time to reference time
	CLSID	    m_clsid;            // This filters clsid
	// used for serialization
	CCritSec        *m_pLock;           // Object we use for locking

	WCHAR           *m_pName;           // Full filter name
	IFilterGraph    *m_pGraph;          // Graph we belong to
	IMediaEventSink *m_pSink;           // Called with notify events
	LONG            m_PinVersion;       // Current pin version

public: // Constructor/destructor ---------------------------------------------
	CBaseFilter(
		LPCTSTR pName,   // Object description
		LPUNKNOWN pUnk,  // IUnknown of delegating object
		CCritSec  *pLock,    // Object who maintains lock
		REFCLSID   clsid);        // The clsid to be used to serialize this filter
	CBaseFilter(
		LPCTSTR pName,    // Object description
		LPUNKNOWN pUnk,  // IUnknown of delegating object
		CCritSec  *pLock,    // Object who maintains lock
		REFCLSID   clsid,         // The clsid to be used to serialize this filter
		HRESULT   *phr);  // General OLE return code
	CBaseFilter(
		LPCSTR pName,    // Object description
		LPUNKNOWN pUnk,  // IUnknown of delegating object
		CCritSec  *pLock,    // Object who maintains lock
		REFCLSID   clsid);        // The clsid to be used to serialize this filter
	CBaseFilter(
		LPCSTR pName,     // Object description
		LPUNKNOWN pUnk,  // IUnknown of delegating object
		CCritSec  *pLock,    // Object who maintains lock
		REFCLSID   clsid,         // The clsid to be used to serialize this filter
		HRESULT   *phr);  // General OLE return code
	~CBaseFilter();

public: // Methods ------------------------------------------------------------
	DECLARE_IUNKNOWN;
	virtual STDMETHODIMP DelegateQueryInterface(REFIID riid, void ** ppv);

	//
	// --- IPersist method ---
	//

	STDMETHODIMP GetClassID(CLSID *pClsID);

	// --- IMediaFilter methods ---

	STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);

	STDMETHODIMP SetSyncSource(IReferenceClock *pClock);

	STDMETHODIMP GetSyncSource(IReferenceClock **pClock);

	// override Stop and Pause so we can activate the pins.
	// Note that Run will call Pause first if activation needed.
	// Override these if you want to activate your filter rather than
	// your pins.
	STDMETHODIMP Stop();
	STDMETHODIMP Pause();

	// the start parameter is the difference to be added to the
	// sample's stream time to get the reference time for
	// its presentation
	STDMETHODIMP Run(REFERENCE_TIME tStart);

	// --- helper methods ---

	// return the current stream time - ie find out what
	// stream time should be appearing now
	virtual HRESULT StreamTime(CRefTime& rtStream);

	// Is the filter currently active?
	BOOL IsActive() {
		m_pLock->Lock();
		BOOL ret = ((m_State == State_Paused) || (m_State == State_Running));
		m_pLock->Unlock();
		return ret;
	};

	// Is this filter stopped (without locking)
	BOOL IsStopped() {
		return (m_State == State_Stopped);
	};

	//
	// --- IBaseFilter methods ---
	//

	// pin enumerator
	STDMETHODIMP EnumPins(
		IEnumPins ** ppEnum);

	// default behaviour of FindPin assumes pin ids are their names
	STDMETHODIMP FindPin(
		LPCWSTR Id,
		IPin ** ppPin
		);

	STDMETHODIMP QueryFilterInfo(
		FILTER_INFO * pInfo);

	STDMETHODIMP JoinFilterGraph(
		IFilterGraph * pGraph,
		LPCWSTR pName);

	// return a Vendor information string. Optional - may return E_NOTIMPL.
	// memory returned should be freed using CoTaskMemFree
	// default implementation returns E_NOTIMPL
	STDMETHODIMP QueryVendorInfo(
		LPWSTR* pVendorInfo
		);

	// --- helper methods ---

	// send an event notification to the filter graph if we know about it.
	// returns S_OK if delivered, S_FALSE if the filter graph does not sink
	// events, or an error otherwise.
	HRESULT NotifyEvent(
		long EventCode,
		LONG_PTR EventParam1,
		LONG_PTR EventParam2);

	// return the filter graph we belong to
	IFilterGraph *GetFilterGraph() {
		return m_pGraph;
	}

	// Request reconnect
	// pPin is the pin to reconnect
	// pmt is the type to reconnect with - can be NULL
	// Calls ReconnectEx on the filter graph
	HRESULT ReconnectPin(IPin *pPin, AM_MEDIA_TYPE const *pmt);

	// find out the current pin version (used by enumerators)
	virtual LONG GetPinVersion();
	void IncrementPinVersion();

	// you need to supply these to access the pins from the enumerator
	// and for default Stop and Pause/Run activation.
	virtual int GetPinCount() PURE;
	virtual CBasePin *GetPin(int n) PURE;
};
//=============================================================================

//=============================================================================
class CBaseRenderer : public CBaseFilter
{
protected:
	friend class CRendererInputPin;
	friend void CALLBACK EndOfStreamTimer(
		UINT uID,      // Timer identifier
		UINT uMsg,     // Not currently used
		DWORD_PTR dwUser,  // User information
		DWORD_PTR dw1,     // Windows reserved
		DWORD_PTR dw2);    // Is also reserved

protected: // Members ---------------------------------------------------------
	CRendererPosPassThru *m_pPosition;  // Media seeking pass by object
	CAMEvent m_RenderEvent;             // Used to signal timer events
	CAMEvent m_ThreadSignal;            // Signalled to release worker thread
	CAMEvent m_evComplete;              // Signalled when state complete
	BOOL m_bAbort;                      // Stop us from rendering more data
	BOOL m_bStreaming;                  // Are we currently streaming
	DWORD_PTR m_dwAdvise;                   // Timer advise cookie
	IMediaSample *m_pMediaSample;       // Current image media sample
	BOOL m_bEOS;                        // Any more samples in the stream
	BOOL m_bEOSDelivered;               // Have we delivered an EC_COMPLETE
	CRendererInputPin *m_pInputPin;     // Our renderer input pin object
	CCritSec m_InterfaceLock;           // Critical section for interfaces
	CCritSec m_RendererLock;            // Controls access to internals
	IQualityControl * m_pQSink;         // QualityControl sink
	BOOL m_bRepaintStatus;              // Can we signal an EC_REPAINT
	//  Avoid some deadlocks by tracking filter during stop
	volatile BOOL  m_bInReceive;        // Inside Receive between PrepareReceive
	// And actually processing the sample
	REFERENCE_TIME m_SignalTime;        // Time when we signal EC_COMPLETE
	UINT m_EndOfStreamTimer;            // Used to signal end of stream
	CCritSec m_ObjectCreationLock;      // This lock protects the creation and
	// of m_pPosition and m_pInputPin.  It
	// ensures that two threads cannot create
	// either object simultaneously.

public: // Constructor/destructor ---------------------------------------------
	CBaseRenderer(REFCLSID RenderClass, // CLSID for this renderer
		LPCTSTR pName,         // Debug ONLY description
		LPUNKNOWN pUnk,       // Aggregated owner object
		HRESULT *phr);        // General OLE return code
	~CBaseRenderer();

public: // Methods ------------------------------------------------------------
	// Overriden to say what interfaces we support and where

	virtual HRESULT GetMediaPositionInterface(REFIID riid, void **ppv);

	DECLARE_IUNKNOWN;
	virtual STDMETHODIMP DelegateQueryInterface(REFIID, void **);

	virtual HRESULT SourceThreadCanWait(BOOL bCanWait);

#ifdef DEBUG
	// Debug only dump of the renderer state
	void DisplayRendererState();
#endif
	virtual HRESULT WaitForRenderTime();
	virtual HRESULT CompleteStateChange(FILTER_STATE OldState);

	// Return internal information about this filter

	BOOL IsEndOfStream() { return m_bEOS; };
	BOOL IsEndOfStreamDelivered() { return m_bEOSDelivered; };
	BOOL IsStreaming() { return m_bStreaming; };
	void SetAbortSignal(BOOL bAbort) { m_bAbort = bAbort; };
	virtual void OnReceiveFirstSample(IMediaSample *pMediaSample) { };
	CAMEvent *GetRenderEvent() { return &m_RenderEvent; };

	// Permit access to the transition state

	void Ready() { m_evComplete.Set(); };
	void NotReady() { m_evComplete.Reset(); };
	BOOL CheckReady() { return m_evComplete.Check(); };

	virtual int GetPinCount();
	virtual CBasePin *GetPin(int n);
	FILTER_STATE GetRealState();
	void SendRepaint();
	void SendNotifyWindow(IPin *pPin,HWND hwnd);
	BOOL OnDisplayChange();
	void SetRepaintStatus(BOOL bRepaint);

	// Override the filter and pin interface functions

	STDMETHODIMP Stop();
	STDMETHODIMP Pause();
	STDMETHODIMP Run(REFERENCE_TIME StartTime);
	STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);
	STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin);

	// These are available for a quality management implementation

	virtual void OnRenderStart(IMediaSample *pMediaSample);
	virtual void OnRenderEnd(IMediaSample *pMediaSample);
	virtual HRESULT OnStartStreaming() { return NOERROR; };
	virtual HRESULT OnStopStreaming() { return NOERROR; };
	virtual void OnWaitStart() { };
	virtual void OnWaitEnd() { };
	virtual void PrepareRender() { };

	// Quality management implementation for scheduling rendering

	virtual BOOL ScheduleSample(IMediaSample *pMediaSample);
	virtual HRESULT GetSampleTimes(IMediaSample *pMediaSample,
		REFERENCE_TIME *pStartTime,
		REFERENCE_TIME *pEndTime);

	virtual HRESULT ShouldDrawSampleNow(IMediaSample *pMediaSample,
		REFERENCE_TIME *ptrStart,
		REFERENCE_TIME *ptrEnd);

	// Lots of end of stream complexities

	void TimerCallback();
	void ResetEndOfStreamTimer();
	HRESULT NotifyEndOfStream();
	virtual HRESULT SendEndOfStream();
	virtual HRESULT ResetEndOfStream();
	virtual HRESULT EndOfStream();

	// Rendering is based around the clock

	void SignalTimerFired();
	virtual HRESULT CancelNotification();
	virtual HRESULT ClearPendingSample();

	// Called when the filter changes state

	virtual HRESULT Active();
	virtual HRESULT Inactive();
	virtual HRESULT StartStreaming();
	virtual HRESULT StopStreaming();
	virtual HRESULT BeginFlush();
	virtual HRESULT EndFlush();

	// Deal with connections and type changes

	virtual HRESULT BreakConnect();
	virtual HRESULT SetMediaType(const CMediaType *pmt);
	virtual HRESULT CompleteConnect(IPin *pReceivePin);

	// These look after the handling of data samples

	virtual HRESULT PrepareReceive(IMediaSample *pMediaSample);
	virtual HRESULT Receive(IMediaSample *pMediaSample);
	virtual BOOL HaveCurrentSample();
	virtual IMediaSample *GetCurrentSample();
	virtual HRESULT Render(IMediaSample *pMediaSample);

	// Derived classes MUST override these
	virtual HRESULT DoRenderSample(IMediaSample *pMediaSample) PURE;
	virtual HRESULT CheckMediaType(const CMediaType *) PURE;

	// Helper
	void WaitForReceiveToComplete();
};
//=============================================================================

#endif // DSHOWFILTERS_H
