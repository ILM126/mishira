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

#include "winapplication.h"
#include "appsettings.h"
#include "profile.h"
#include "wincpuusage.h"
#include <Dwmapi.h>
#include <mmsystem.h>
#include <Shlobj.h>
#include <Libdeskcap/capturemanager.h>
#include <Libvidgfx/versionhelpers.h>
#include <QtCore/QSettings>
#include <QtGui/QImage>

// Undocumented Qt helper functions that are exported from the QtGui library
//extern QPixmap qt_pixmapFromWinHICON(HICON icon);
extern QImage qt_imageFromWinHBITMAP(HDC hdc, HBITMAP bitmap, int w, int h);

//=============================================================================
// Helpers

/// <summary>
/// Converts a Windows HICON into a QImage. We do this ourselves instead of
/// using Qt's `qt_pixmapFromWinHICON()` as it doesn't properly calculate the
/// icon's size and it outputs a QPixmap instead of a QImage.
/// </summary>
QImage imageFromIcon(HICON icon, ICONINFO info)
{
	QImage img;

	// Create a HDC
	HDC screenHdc = GetDC(NULL);
	HDC hdc = CreateCompatibleDC(screenHdc);
	ReleaseDC(NULL, screenHdc);

	// Determine the icon's size
	QSize size;
	int bpp = 0; // Bytes per pixel
	BITMAP bmp;
	memset(&bmp, 0, sizeof(bmp));
	if(info.hbmColor) {
		if(GetObject(info.hbmColor, sizeof(bmp), &bmp) > 0) {
			size = QSize(bmp.bmWidth, bmp.bmHeight);
			bpp = bmp.bmBitsPixel;
		}
	} else if(info.hbmMask) {
		if(GetObject(info.hbmMask, sizeof(bmp), &bmp) > 0) {
			size = QSize(bmp.bmWidth, bmp.bmHeight / 2);
			bpp = 1;
		}
	}
	if(bpp == 0 || size.isEmpty())
		goto imageFromIconExit1; // No pixel data

	// As icons are difficult to work with due to masking we use GDI to convert
	// it to a bitmap instead
	BITMAPINFO bmpInfo;
	memset(&bmpInfo, 0, sizeof(bmpInfo));
	bmpInfo.bmiHeader.biSize = sizeof(bmpInfo);
	bmpInfo.bmiHeader.biWidth = size.width();
	bmpInfo.bmiHeader.biHeight = size.height();
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	//bmpInfo.bmiHeader.biSizeImage = 0;
	//bmpInfo.bmiHeader.biXPelsPerMeter = 0;
	//bmpInfo.bmiHeader.biYPelsPerMeter = 0;
	//bmpInfo.bmiHeader.biClrUsed = 0;
	//bmpInfo.bmiHeader.biClrImportant = 0;
	uchar *bmpData = NULL;
	HBITMAP hbmp = CreateDIBSection(
		hdc, &bmpInfo, DIB_RGB_COLORS, (void **)&bmpData, NULL, 0);
	if(hbmp == NULL)
		goto imageFromIconExit1;
	HGDIOBJ prevObj = SelectObject(hdc, hbmp);
	DrawIconEx(hdc, 0, 0, icon, 0, 0,
		0, // Animation frame, TODO
		NULL, DI_NORMAL);

	// Convert the bitmap to a QImage
	img = qt_imageFromWinHBITMAP(hdc, hbmp, size.width(), size.height());

	// Does the bitmap have any alpha?
	bool hasAlpha = false;
	for(int y = 0; y < size.height(); y++) {
		const QRgb *line = reinterpret_cast<const QRgb *>(img.scanLine(y));
		for(int x = 0; x < size.width(); x++) {
			if(qAlpha(line[x]) == 0)
				continue;
			hasAlpha = true;
			break;
		}
		if(hasAlpha)
			break;
	}

	// If the bitmap doesn't have any alpha values then we need to merge the
	// mask into the colour data as a separate step.
	if(!hasAlpha) {
		// Get the AND mask
		DrawIconEx(hdc, 0, 0, icon, 0, 0,
			0, // Animation frame, TODO
			NULL, DI_MASK);
		QImage andMaskImg =
			qt_imageFromWinHBITMAP(hdc, hbmp, size.width(), size.height());

		//---------------------------------------------------------------------
		// Get the XOR mask. This is a complete HACK as we're actually
		// rendering the cursor and working backwards using the pixel data and
		// AND mask.

		// Start with a white background
		HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));
		RECT rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = size.width();
		rect.bottom = size.height();
		FillRect(hdc, &rect, brush);
		DeleteObject(brush);

		// Render the cursor normally
		DrawIconEx(hdc, 0, 0, icon, 0, 0,
			0, // Animation frame, TODO
			NULL, DI_MASK | DI_NORMAL);
		QImage xorMaskImg =
			qt_imageFromWinHBITMAP(hdc, hbmp, size.width(), size.height());

		//---------------------------------------------------------------------

		for(int y = 0; y < size.height(); y++) {
			QRgb *imgLine = reinterpret_cast<QRgb *>(img.scanLine(y));
			const QRgb *maskLineAnd = (andMaskImg.isNull() ? 0 :
				reinterpret_cast<const QRgb *>(andMaskImg.scanLine(y)));
			const QRgb *maskLineXor = (xorMaskImg.isNull() ? 0 :
				reinterpret_cast<const QRgb *>(xorMaskImg.scanLine(y)));
			for(int x = 0; x < size.width(); x++) {
				// Debug AND and XOR mask
				//imgLine[x] = 0xFF000000 | (imgLine[x] - maskLineXor[x]);
				//imgLine[x] = 0xFF000000 | (qRed(maskLineAnd[x]) << 16);
				//imgLine[x] = 0xFF000000 | (qRed(maskLineXor[x]) << 16);

				// Ideally cursors should be displayed as shown as follows:
				//
				//    AND | XOR | Display
				//   -----+-----+------------------
				//     0  |  0  | Black
				//     0  |  1  | White
				//     1  |  0  | Screen
				//     1  |  1  | Reverse screen
				//
				// We don't actually handle the XOR mask properly as we cannot
				// do it easily with a single GPU texture. Whenever we are
				// supposed to display the reverse of the screen we actually
				// just display black.
				if(maskLineAnd && qRed(maskLineAnd[x]) != 0) {
					if(maskLineXor && qRed(imgLine[x] - maskLineXor[x]) != 0) {
						// We're supposed to invert the screen but instead just
						// make it black
						imgLine[x] = 0xFF000000;
					} else {
						// Mask the pixel completely
						imgLine[x] &= 0x00FFFFFF;
					}
				} else {
					// Make pixel completely visible
					imgLine[x] |= 0xFF000000;
				}
			}
		}
	}

	//imageFromIconExit2:
	SelectObject(hdc, prevObj);
	DeleteObject(hbmp);
imageFromIconExit1:
	DeleteDC(hdc);
	return img;
}

/// <summary>
/// Gets the installed Windows version using the version helper API.
/// </summary>
void getRealWindowsVersion(
	unsigned int *major, unsigned int *minor, unsigned int *servpack)
{
	if(major == NULL || minor == NULL || servpack == NULL)
		return;
	*major = 6;
	*minor = 0;
	*servpack = 0;

	// Determine major version number
	for(;;) {
		if(!IsWindowsVersionOrGreater(*major, *minor, *servpack))
			break;
		(*major)++;
	}
	(*major)--;

	// Determine minor version number
	for(;;) {
		if(!IsWindowsVersionOrGreater(*major, *minor, *servpack))
			break;
		(*minor)++;
	}
	(*minor)--;

	// Determine minor service pack number
	for(;;) {
		if(!IsWindowsVersionOrGreater(*major, *minor, *servpack))
			break;
		(*servpack)++;
	}
	(*servpack)--;
}

/// <summary>
/// Returns the currently installed version of Windows as a human-readable
/// string. Returns an empty string if there was an error.
/// </summary>
QString getWindowsVersionName()
{
	// Log Windows version. See the remarks of this MSDN article for how to
	// identify which version of Windows is running:
	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724833%28v=vs.85%29.aspx
	//
	// `GetVersionEx()` has been deprecated in Windows 8.1 meaning the latest
	// version that the function will return is the version number of Windows
	// 8. In order to determine later versions of Windows we must use the
	// version helper API which is available in the Windows 8.1 SDK.
	// http://msdn.microsoft.com/en-us/library/windows/desktop/dn424972(v=vs.85).aspx

	unsigned int major, minor, servpack;
	getRealWindowsVersion(&major, &minor, &servpack);

	QString osName;
	if(major == 6) {
		switch(minor) {
		case 0:
			if(!IsWindowsServer())
				osName = QStringLiteral("Windows Vista");
			else
				osName = QStringLiteral("Windows Server 2008");
			break;
		case 1:
			if(!IsWindowsServer())
				osName = QStringLiteral("Windows 7");
			else
				osName = QStringLiteral("Windows Server 2008 R2");
			break;
		case 2:
			if(!IsWindowsServer())
				osName = QStringLiteral("Windows 8");
			else
				osName = QStringLiteral("Windows Server 2012");
			break;
		case 3:
			if(!IsWindowsServer())
				osName = QStringLiteral("Windows 8.1");
			else
				osName = QStringLiteral("Windows Server 2012 R2");
			break;
		default:
			// Set later
			break;
		}
	}
	if(osName.isEmpty()) {
		if(!IsWindowsServer()) {
			osName = QStringLiteral("Version %1.%2")
				.arg(major)
				.arg(minor);
		} else {
			osName = QStringLiteral("Version %1.%2 (Server)")
				.arg(major)
				.arg(minor);
		}
	}

	// Get service pack information
	if(servpack > 0) {
		osName = QStringLiteral("%1 Service Pack %2")
			.arg(osName).arg(servpack);
	}

	return osName;
}

//=============================================================================
// WinApplication class

hideLauncherSplash_t WinApplication::s_hideLauncherSplashPtr = NULL;

WinApplication::WinApplication(
	int &argc, char **argv, AppSharedSegment *shm)
	: Application(argc, argv, shm)
	, m_disableAeroRef(0)
	, m_disabledAero(false)

	// Performance timer
	//, m_startTick()
	//, m_lastTime()
	//, m_startTime()
	//, m_frequency()
	, m_timerMask(0)

	// OS Scheduler
	, m_schedulerPeriod(0)

	// System cursor
	, m_cursorDirty(true)
	, m_cursorCached(NULL)
	, m_cursorTexture(NULL)
	, m_cursorPos(0, 0)
	, m_cursorOffset(0, 0)
	, m_cursorVisible(false)
{
}

WinApplication::~WinApplication()
{
	if(m_disableAeroRef > 0) {
		appLog(Log::Warning) << QStringLiteral(
			"Application shutting down while disable Aero still referenced");
		while(m_disableAeroRef > 0)
			derefDisableAero();
	}
}

void WinApplication::refDisableAero()
{
	m_disableAeroRef++;
	if(m_disableAeroRef == 1) {
		// Disable Aero if required

		// Remember if Aero is already disabled so that we don't enable it once
		// we lose all our references.
		BOOL aeroEnabled;
		HRESULT res = DwmIsCompositionEnabled(&aeroEnabled);
		if(SUCCEEDED(res)) {
			if(aeroEnabled)
				m_disabledAero = true;
			else
				m_disabledAero = false;
		}

		if(m_disabledAero && !IsWindows8OrGreater()) {
			appLog() << QStringLiteral("Disabling Windows Aero");
			DwmEnableComposition(DWM_EC_DISABLECOMPOSITION);
		}
	}
}

void WinApplication::derefDisableAero()
{
	if(m_disableAeroRef <= 0)
		return; // Already disabled
	m_disableAeroRef--;
	if(m_disableAeroRef == 0) {
		// Enable Aero
		if(m_disabledAero && !IsWindows8OrGreater()) {
			appLog() << QStringLiteral("Enabling Windows Aero");
			DwmEnableComposition(DWM_EC_ENABLECOMPOSITION);
		}
	}
}

void WinApplication::logSystemInfo()
{
	// Get the name of the operating system
	QString osName = getWindowsVersionName();
	if(osName.isEmpty())
		appLog() << "Operating system: **Unknown**";
	else
		appLog() << "Operating system: " << osName;

	// Is Windows Aero enabled?
	BOOL aeroEnabled;
	HRESULT res = DwmIsCompositionEnabled(&aeroEnabled);
	if(FAILED(res)) {
		appLog(Log::Warning)
			<< "Failed to determine if Windows Aero was enabled or not";
	} else {
		if(aeroEnabled)
			appLog() << "Windows Aero: Enabled at application launch";
		else
			appLog() << "Windows Aero: Disabled at application launch";
	}
	// TODO: Log when we receive a `WM_DWMCOMPOSITIONCHANGED` event

	// Read CPU information via the Windows registry
	QSettings reg(
		"HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor",
		QSettings::NativeFormat);
	QString procName = reg.value(
		"0/ProcessorNameString", QStringLiteral("** Unknown **")).toString();
	QString procIdent = reg.value(
		"0/Identifier", QStringLiteral("** Unknown **")).toString();
	quint64 procHz = reg.value("0/~MHz", 0).toInt();
	int numCores = 0;
	for(;;) {
		if(!reg.contains(
			QStringLiteral("%1/ProcessorNameString").arg(numCores)))
		{
			break;
		}
		numCores++;
	}
	appLog() << "Processor information:";
	appLog() << "    Name: " << procName;
	appLog() << "    Identifier: " << procIdent;
	appLog() << "    Speed: ~"
		<< humanBitsBytes(procHz * 1000000ULL, 2, true) << "Hz";
	appLog() << "    Logical cores: " << numCores;

	// Log memory status
	MEMORYSTATUSEX memStats;
	memset(&memStats, 0, sizeof(memStats));
	memStats.dwLength = sizeof(memStats);
	if(GlobalMemoryStatusEx(&memStats) == 0) {
		appLog(Log::Warning) << "Failed to get memory information. "
			<< "Reason = " << (int)GetLastError();
	} else {
		appLog() << "Memory information:";
		appLog() << QStringLiteral("    Total: %1B")
			.arg(humanBitsBytes(memStats.ullTotalPhys, 2));
		appLog() << QStringLiteral("    Free: %1B (%2% used)")
			.arg(humanBitsBytes(memStats.ullAvailPhys, 2))
			.arg(qRound((1.0f - (float)memStats.ullAvailPhys /
			(float)memStats.ullTotalPhys) * 100.0f));
	}

	// Log all available display adapters
	vidgfx_d3d_log_display_adapters();
}

/// <summary>
/// Hides the splash window that was created by our small launcher executable.
/// </summary>
void WinApplication::hideLauncherSplash()
{
	if(s_hideLauncherSplashPtr != NULL)
		(*s_hideLauncherSplashPtr)();
}

/// <summary>
/// Gets the texture, position and state of the current system mouse cursor.
/// Position is in screen coordinates.
/// </summary>
VidgfxTex *WinApplication::getSystemCursorInfo(
	QPoint *globalPos, QPoint *offset, bool *isVisible)
{
	if(!m_cursorDirty)
		goto systemCursorInfoDone; // Skip straight to the end of the method

	// Make sure we have a graphics context
	VidgfxContext *gfx = getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		goto systemCursorInfoDone; // Cannot create texture

	// Query basic cursor information from the OS
	CURSORINFO info;
	memset(&info, 0, sizeof(info));
	info.cbSize = sizeof(info);
	if(!GetCursorInfo(&info)) {
		m_cursorVisible = false;
		goto systemCursorInfoDone;
	}
	m_cursorPos.setX(info.ptScreenPos.x);
	m_cursorPos.setY(info.ptScreenPos.y);
	m_cursorVisible = (info.flags & CURSOR_SHOWING);

	// Get additional information from the cursor bitmap if it's different
	// to our cached bitmap
	if(m_cursorCached != info.hCursor) {
		ICONINFO iconInfo;
		memset(&iconInfo, 0, sizeof(iconInfo));
		if(!GetIconInfo(info.hCursor, &iconInfo)) {
			m_cursorVisible = false;
			goto systemCursorInfoDone;
		}
		m_cursorOffset.setX(-(int)iconInfo.xHotspot);
		m_cursorOffset.setY(-(int)iconInfo.yHotspot);
		m_cursorCached = info.hCursor;

		// Get pixmap data
		QImage img = imageFromIcon(info.hCursor, iconInfo);
		img = img.convertToFormat(QImage::Format_ARGB32);
		if(m_cursorTexture != NULL)
			vidgfx_context_destroy_tex(gfx, m_cursorTexture);
		m_cursorTexture = vidgfx_context_new_tex(gfx, img, false, false);
		if(m_cursorTexture == NULL)
			m_cursorVisible = false;

		// It is our responsibility to delete the bitmaps from `GetIconInfo()`
		DeleteObject(iconInfo.hbmColor);
		DeleteObject(iconInfo.hbmMask);
	}

	m_cursorDirty = false;

systemCursorInfoDone:
	if(globalPos != NULL)
		*globalPos = m_cursorPos;
	if(offset != NULL)
		*offset = m_cursorOffset;
	if(isVisible != NULL)
		*isVisible = m_cursorVisible;
	return m_cursorTexture;
}

/// <summary>
/// As we recreate the cursor texture every single frame we only want to create
/// it no more than once per frame. This method is called whenever the main
/// loop believes it is most efficient to recreate the texture.
/// </summary>
void WinApplication::resetSystemCursorInfo()
{
	m_cursorDirty = true;
}

void WinApplication::deleteSystemCursorInfo()
{
	VidgfxContext *gfx = getGraphicsContext();
	if(!vidgfx_context_is_valid(gfx))
		return; // Cannot delete texture
	vidgfx_context_destroy_tex(gfx, m_cursorTexture);
	m_cursorTexture = NULL;
	m_cursorCached = NULL;
	m_cursorDirty = true;
}

/// <summary>
/// If `enable` is set to true then notify the OS that the system should never
/// sleep or turn the display off. If `enable` is set to false then revert the
/// settings back to normal.
/// </summary>
void WinApplication::keepSystemAndDisplayActive(bool enable)
{
	if(enable) {
		SetThreadExecutionState(
			ES_SYSTEM_REQUIRED | // Prevent system from sleeping
			ES_DISPLAY_REQUIRED | // Prevent display from sleeping
			ES_CONTINUOUS);
	} else
		SetThreadExecutionState(ES_CONTINUOUS);
}

/// <summary>
/// Begins the main/GUI thread's main loop.
/// </summary>
int WinApplication::execute()
{
	// Test to see if the application exited during initialization
	if(m_exiting)
		return m_exitCode;

	// It takes a while to process Qt events for the first time so do it before
	// we start timing the ticks
	processEvents();
	sendPostedEvents(NULL, QEvent::DeferredDelete);

	// Set the OS scheduler to a higher frequency. While we theoretically only
	// need a 2ms scheduler period it was proven during tests that using a 1ms
	// period resulted in ~4% less CPU usage on a dual core Intel E8500.
	setOsSchedulerPeriod(1);

	// Begin performance counter
	if(!beginPerformanceTimer()) {
		appLog(Log::Critical) << "Unsupported system: No performance timer";
		releaseOsSchedulerPeriod();
		return 1;
	}

	// Begin main loop
	appLog() << LOG_DOUBLE_LINE << "\n Main loop begin\n" << LOG_DOUBLE_LINE;
	while(!m_exiting) {
		// In order to accurately time ticks we have to calculate all tick
		// time stamps from the same origin. This means that the frequency must
		// remain constant. We must also make sure that the frequency that we
		// update the screen at is an integer multiple of the video framerate
		// in order to simplify the sleep code.
		// 60Hz = Ideal screen update frequency, TODO: Monitor frequency
		m_curVideoFreq = DEFAULT_VIDEO_FRAMERATE;
		if(m_profile != NULL)
			m_curVideoFreq = m_profile->getVideoFramerate();
		m_mainLoopFreq = m_curVideoFreq;
		m_tickFrameMultiple =
			60 * m_curVideoFreq.denominator / m_curVideoFreq.numerator;
		m_mainLoopFreq.numerator *= m_tickFrameMultiple;
		m_tickTimeOrigin = getUsecSinceExec();
		m_nextRTTickNum = 0;
		m_nextRTFrameNum = 0;
		m_nextQFrameNum = 0;

		appLog() <<
			QStringLiteral("Main loop frequency = %1/%2 = ~%3 Hz")
			.arg(m_mainLoopFreq.numerator)
			.arg(m_mainLoopFreq.denominator)
			.arg(m_mainLoopFreq.asFloat());
		appLog() <<
			QStringLiteral("Video frame frequency = %1/%2 = ~%3 Hz")
			.arg(m_curVideoFreq.numerator)
			.arg(m_curVideoFreq.denominator)
			.arg(m_curVideoFreq.asFloat());

		// Notify RTMP client of new frequency
		RTMPClient::gamerSetTickFreq(m_mainLoopFreq.asFloat());

		// Notify capture manager of new frequency
		CaptureManager::getManager()->setVideoFrequency(
			m_curVideoFreq.numerator, m_curVideoFreq.denominator);

		m_changeMainLoopFreq = false;
		while(!m_exiting && !m_changeMainLoopFreq) {
			if(m_processTicksRef == 0 && m_processAllFramesRef == 0) {
				// We are only processing Qt so we are allowed to lower CPU
				// usage by waiting for new events in a blocking fashion.
				beginQtExecTimer();
				processEvents(QEventLoop::WaitForMoreEvents);
				sendPostedEvents(NULL, QEvent::DeferredDelete);
				stopQtExecTimer();
				continue;
			}

			// Process all our events immediately after waking up
			processOurEvents();

			// If it's possible for us to sleep do so. If "low jitter mode" is
			// enabled then we want to wake up at least
			// `[ActualSchedulerPeriod] * 1.5` milliseconds before our next
			// scheduled tick so that we don't miss the exact time, otherwise
			// we sleep until after the scheduled time to prevent wasting CPU.
			// WARNING: Remember that we are dealing with unsigned numbers!
			//continue; // Spin
			quint64 sleepTarget =
				((quint64)m_nextRTTickNum * 1000000 *
				m_mainLoopFreq.denominator) / m_mainLoopFreq.numerator +
				m_tickTimeOrigin;
			quint64 targetBuf = (quint64)m_schedulerPeriod * 1500; // ms->usec
			if(!isInLowJitterMode())
				targetBuf = 0; // Always sleep if it isn't time yet
			quint64 curTime = getUsecSinceExec();
			//appLog() << "Target=" << sleepTarget << " Buffer=" << targetBuf
			//	<< " Now=" << curTime;
			if(sleepTarget < curTime)
				continue; // We've already missed the target time
			sleepTarget -= curTime; // Make the target relative
			if(sleepTarget < targetBuf)
				continue; // If we sleep now we might not wake up in time
			sleepTarget -= targetBuf; // Now the max usec we can safely sleep
			int sleepMs;
			if(isInLowJitterMode())
				sleepMs = sleepTarget / 1000; // Always wake up early
			else
				sleepMs = (sleepTarget + 999) / 1000; // Always wake up late
			Sleep(sleepMs);

#define LOG_SLEEP_BEHAVIOUR 0
#if LOG_SLEEP_BEHAVIOUR
			// Calculate how early we awoke compared to our target
			if(sleepMs == 0) {
				//appLog() << "Slept for 0ms";
			} else {
				sleepTarget =
					(m_nextRTTickNum * 1000000 * m_mainLoopFreq.denominator) /
					m_mainLoopFreq.numerator + m_tickTimeOrigin;
				curTime = getUsecSinceExec();
				if(sleepTarget < curTime) {
					appLog() << "  Overslept by "
						<< (curTime - sleepTarget) << " usec! Slept for "
						<< sleepMs << " msec";
				} else {
					appLog() << "Awoke " << (sleepTarget - curTime)
						<< " usec early. Slept for " << sleepMs << " msec";
				}
			}
#endif
		}
	}
	appLog() << LOG_DOUBLE_LINE << "\n Main loop end\n" << LOG_DOUBLE_LINE;

	// Return the OS scheduler to its original frequency
	releaseOsSchedulerPeriod();

	return m_exitCode;
}

/// <summary>
/// Begins the high-performance timer for the main loop. Also sets thread
/// affinity for timer stability if it is required.
/// </summary>
/// <returns>True if a timer was successfully created.</returns>
bool WinApplication::beginPerformanceTimer()
{
	// This method is based on `Ogre::Timer`

	DWORD_PTR	procMask;
	DWORD_PTR	sysMask;

	// Get the current process core mask
	GetProcessAffinityMask(GetCurrentProcess(), &procMask, &sysMask);
	if(procMask == 0)
		procMask = 1; // Assume there is only one core available

	// Find the highest core that this process uses
	if(m_timerMask == 0) {
		m_timerMask = (DWORD_PTR)1 << (sizeof(m_timerMask) * 8 - 1);
		while((m_timerMask & procMask) == 0) {
			m_timerMask >>= 1;
			if(m_timerMask == 0) {
				appLog(Log::Warning)
					<< "Cannot determine process affinity";
				return false;
			}
		}
	}

#define SET_THREAD_AFFINITY 0
#if SET_THREAD_AFFINITY
	// Set current thread affinity to the highest core
	oldMask = SetThreadAffinityMask(GetCurrentThread(), m_timerMask);
#endif

	// Get the frequency of the performance counter
	QueryPerformanceFrequency(&m_frequency);
	if(m_frequency.QuadPart == 0) {
		// System doesn't has a performance timer
		appLog(Log::Warning)
			<< "No performance timer available on system";
		return false;
	}
	appLog() <<
		QStringLiteral("Performance timer frequency = %L1 Hz")
		.arg(m_frequency.QuadPart);
	if(m_frequency.QuadPart < 200) {
		// Performance timer has a resolution of less than 5ms
		appLog(Log::Warning)
			<< "Performance timer isn't performant enough to use";
		return false;
	}

	// Query the timer
	QueryPerformanceCounter(&m_startTime);
	m_startTick = GetTickCount();
	m_lastTime = 0;

	return true;
}

/// <summary>
/// Returns the number of microseconds that have passed since the main loop
/// began.
/// </summary>
quint64 WinApplication::getUsecSinceExec()
{
	// This method is based on `Ogre::Timer`

	LARGE_INTEGER	curTime;
	LONGLONG		timeSinceStart;

	if(m_timerMask == 0)
		return 0; // Haven't started timer yet

	// Query the timer
	QueryPerformanceCounter(&curTime);
	timeSinceStart = curTime.QuadPart - m_startTime.QuadPart;

	// Convert to milliseconds for `GetTickCount()` comparison
	unsigned long newTicks =
		(unsigned long)(1000LL * timeSinceStart / m_frequency.QuadPart);

	// Compensate for performance counter leaps (See Microsoft KB: Q274323)
	unsigned long check = GetTickCount() - m_startTick;
	signed long msecOff = (signed long)(newTicks - check);
	if(msecOff < -100 || msecOff > 100) {
		// Anomaly detected, compensate
		LONGLONG adjust = qMin(
			msecOff * m_frequency.QuadPart / 1000,
			timeSinceStart - m_lastTime);
		m_startTime.QuadPart += adjust;
		timeSinceStart -= adjust;
	}
	m_lastTime = timeSinceStart;

	// Convert to microseconds and return
	return (quint64)(1000000LL * timeSinceStart / m_frequency.QuadPart);
}

/// <summary>
/// Returns the number of microseconds that have passed since video frame 0.
/// </summary>
quint64 WinApplication::getUsecSinceFrameOrigin()
{
	if(m_timerMask == 0)
		return 0ULL; // Haven't started timer yet
	if(m_changeMainLoopFreq) {
		// The main loop is currently processing the last iteration of the loop
		// at the previous video frequency. During this time the frame origin
		// has currently not been reset yet we'll be reinitializing classes
		// that use the frame origin as a reference (E.g. audio mixer). Because
		// of this pretend that we have just immediately restarted the main
		// loop at the new frequency with a new origin.
		return 0ULL;
	}
	return getUsecSinceExec() - m_tickTimeOrigin;
}

/// <summary>
/// Converts the `pu64QPCPosition` parameter of
/// `IAudioCaptureClient::GetBuffer()` into a time that is relative to our
/// main loop origin (Frame 0).
///
/// WARNING: This is a signed integer as it's possible for us to receive audio
/// from the OS that was recorded before our main loop started (In the case of
/// a profile being loaded immediately on application start).
/// </summary>
qint64 WinApplication::qpcPosToUsecSinceFrameOrigin(
	UINT64 u64QPCPosition) const
{
	if(m_timerMask == 0)
		return 0; // Haven't started timer yet
	quint64 start100Nsec =
		(quint64)(10000000LL * m_startTime.QuadPart / m_frequency.QuadPart);
	return ((qint64)u64QPCPosition - (qint64)start100Nsec) / 10LL -
		m_tickTimeOrigin;
}

int WinApplication::detectOsSchedulerPeriod()
{
	// Force scheduler refresh by sleeping for more than 1/64 sec (Which is the
	// default Windows scheduler frequency)
	Sleep(20);

	// Brute force detection
	uint schedPeriod = UINT_MAX;
	uint schedPrev = timeGetTime();
	int schedCount = 0;
	for(;;) {
		uint next = timeGetTime();
		if(next == schedPrev)
			continue;
		uint diff = next - schedPrev;

		// Did the time change by the same amount?
		if(diff == schedPeriod) {
			if(++schedCount == 5) // Identical results = Certain
				break;
		}

		// Is the difference faster than our previous guess?
		if(diff < schedPeriod) {
			schedPeriod = diff; // Difference is
			schedCount = 0;
		}
	}

	return schedPeriod;
}

void WinApplication::setOsSchedulerPeriod(uint desiredMinPeriod)
{
	TIMECAPS schedPtc;

	m_schedulerPeriod = desiredMinPeriod;

	// Log current resolution
	uint previousPeriod = detectOsSchedulerPeriod();
	appLog() <<
		QStringLiteral("Previous OS scheduler period = %L1 ms")
		.arg(previousPeriod);

	// Get supported range
	if(timeGetDevCaps(&schedPtc, sizeof(schedPtc)) == TIMERR_NOERROR) {
		appLog() <<
			QStringLiteral("Supported OS scheduler period range = [%L1 .. %L2] ms")
			.arg(schedPtc.wPeriodMin)
			.arg(schedPtc.wPeriodMax);

		// Stay within the valid range
		m_schedulerPeriod = qMax(m_schedulerPeriod, schedPtc.wPeriodMin);

#define MATCH_PREVIOUS_SCHEDULER_PERIOD 1
#if MATCH_PREVIOUS_SCHEDULER_PERIOD
		// As our main loop uses the scheduler period to calculate sleep
		// periods we must make sure that the period never becomes larger than
		// what we specify. If the previous period is less than what we want
		// then there is a chance that the other program that set it will
		// call its `timeEndPeriod()` and the period will increase. Prevent
		// this from happening by matching the existing previous period
		m_schedulerPeriod = qMin(m_schedulerPeriod, previousPeriod);
#endif

		// Set as close to our target as possible
		if(timeBeginPeriod(m_schedulerPeriod) == TIMERR_NOERROR) {
			appLog() <<
				QStringLiteral("Set OS scheduler period to %L1 ms")
				.arg(m_schedulerPeriod);

			// Verify that we actually changed the resolution
			// TODO: Tests have shown that the verified period can sometimes be
			// less than requested period. If this is the case we probably want
			// to force the lower period so we can more reliably calculate
			// timing values in our main loop
			appLog() <<
				QStringLiteral("Verified current OS scheduler period = %1 ms")
				.arg(detectOsSchedulerPeriod());
		} else {
			appLog(Log::Warning) <<
				QStringLiteral("Failed to set OS scheduler period to %1 ms")
				.arg(m_schedulerPeriod);
			m_schedulerPeriod = 0;
		}
	} else {
		appLog(Log::Warning)
			<< "Failed to get supported OS scheduler periods";
		m_schedulerPeriod = 0;
	}
}

void WinApplication::releaseOsSchedulerPeriod()
{
	if(m_schedulerPeriod)
		timeEndPeriod(m_schedulerPeriod);
}

CPUUsage *WinApplication::createCPUUsage()
{
	return new WinCPUUsage();
}

QString WinApplication::calcDataDirectoryPath()
{
	// Query OS for root "roaming" directory
	wchar_t wPath[MAX_PATH];
	memset(wPath, 0, sizeof(wPath));
	HRESULT res = SHGetFolderPath(
		NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, wPath);
	if(FAILED(res))
		return Application::calcDataDirectoryPath();

	// Append application name to the path
	QString path = QString::fromUtf16(wPath);
	//if(!organizationName().isEmpty())
	//	path += QLatin1Char('/') + organizationName();
	if(!applicationName().isEmpty())
		path += QLatin1Char('/') + applicationName();

	return path;
}
