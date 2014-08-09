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

//=============================================================================
// The sole purpose of this executable is to display a splash window as quickly
// as possible so the user knows something is happening. We do this by making
// the application automatically link to the last amount of libraries possible
// and load no external files so that it can be read from the HDD in a single
// request. After this executable is in memory we display the window and then
// dynamically link in the main application. Once the application has fully
// initialised it calls a function that hides the splash window.
//=============================================================================

#include <stdlib.h>
#include <windows.h>
#include <wincodec.h>
#include "resource.h"

// Define what the DLL entry point should be
extern "C"
	typedef int (__cdecl *startApp_t)(
	int argc, char *argv[], void *hideLauncherSplash);

IStream *createIStreamFromResource(LPCWSTR name, LPCWSTR type)
{
	IStream *stream = NULL;

	// Find and load the resource
	HRSRC hrsrc = FindResource(NULL, name, type);
	if(hrsrc == NULL)
		return NULL;
	DWORD resSize = SizeofResource(NULL, hrsrc);
	HGLOBAL hResData = LoadResource(NULL, hrsrc);
	if(hResData == NULL)
		return NULL;

	// Map resource to memory
	void *srcData = LockResource(hResData);
	if(srcData == NULL)
		return NULL;

	// Allocate memory on the "global" heap as IStream uses OLE objects
	HGLOBAL globalMem = GlobalAlloc(GMEM_MOVEABLE, resSize);
	if(globalMem == NULL)
		return NULL;

	// Map the allocated memory
	void *dstData = GlobalLock(globalMem);
	if(dstData == NULL) {
		// couldn't create stream; free the memory
		GlobalFree(dstData);
		return NULL;
	}

	// Copy the resource data to the global heap allocation and unmap all
	// memory blocks
	memcpy(dstData, srcData, resSize);
	GlobalUnlock(globalMem);
	UnlockResource(hResData);

	// Create an IStream from the "global" memory block. The returned object
	// will free the memory block once it is released
	HRESULT res = CreateStreamOnHGlobal(globalMem, TRUE, &stream);
	if(FAILED(res))
		return NULL;
	return stream;
}

IWICBitmapSource *getWICBitmapFromIStream(IStream *stream)
{
	IWICBitmapSource *bitmap = NULL;

	// Load the WIC PNG decoder
	IWICBitmapDecoder *decoder = NULL;
	HRESULT res = CoCreateInstance(
		CLSID_WICPngDecoder,
		NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&decoder));
	if(FAILED(res))
		return NULL;

	// Initialize the decoder from the IStream
	res = decoder->Initialize(stream, WICDecodeMetadataCacheOnLoad);
	if(FAILED(res))
		goto exitBitmapFromIStream1;

	// Load the first frame of the PNG
	IWICBitmapFrameDecode *frame = NULL;
	res = decoder->GetFrame(0, &frame);
	if(FAILED(res))
		goto exitBitmapFromIStream1;

	// Convert the frame to a 32-bit BGRA bitmap with pre-multiplied alpha
	res = WICConvertBitmapSource(
		GUID_WICPixelFormat32bppPBGRA, frame, &bitmap);
	if(FAILED(res))
		goto exitBitmapFromIStream2;

	// Clean up and return the bitmap
	frame->Release();
	return bitmap;

	// Error handling
exitBitmapFromIStream2:
	frame->Release();
exitBitmapFromIStream1:
	decoder->Release();
	return NULL;
}

HBITMAP getHBITMAPFromWICBitmap(IWICBitmapSource *wicBitmap)
{
	HBITMAP bitmap = NULL;

	// Get image size
	UINT width = 0;
	UINT height = 0;
	HRESULT res = wicBitmap->GetSize(&width, &height);
	if(FAILED(res) || width == 0 || height == 0)
		return NULL;

	// Define bitmap header
	BITMAPINFO bminfo;
	memset(&bminfo, 0, sizeof(bminfo));
	bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bminfo.bmiHeader.biWidth = width;
	bminfo.bmiHeader.biHeight = -((LONG)height); // Negative = "Top-down" DIB
	bminfo.bmiHeader.biPlanes = 1;
	bminfo.bmiHeader.biBitCount = 32;
	bminfo.bmiHeader.biCompression = BI_RGB;

	// Create the DIB section for pixel data
	void *bitData = NULL;
	HDC hdc = GetDC(NULL);
	bitmap = CreateDIBSection(hdc, &bminfo, DIB_RGB_COLORS, &bitData, NULL, 0);
	ReleaseDC(NULL, hdc);
	if(bitmap == NULL)
		return NULL;

	// Copy pixel data to the HBITMAP
	const UINT stride = width * 4;
	const UINT bufSize = stride * height;
	res = wicBitmap->CopyPixels(
		NULL, stride, bufSize, static_cast<BYTE *>(bitData));
	if(FAILED(res)) {
		DeleteObject(bitmap);
		return NULL;
	}

	return bitmap;
}

HBITMAP loadBitmapResource(LPCWSTR name, LPCWSTR type)
{
	HBITMAP bitmap = NULL;

	// Load specified resource as an IStream
	IStream *stream = createIStreamFromResource(name, type);
	if(stream == NULL)
		return NULL;

	// Convert the IStream to a WIC bitmap
	IWICBitmapSource *wicBitmap = getWICBitmapFromIStream(stream);
	if(wicBitmap == NULL)
		goto exitLoadBitmapResource1;

	// Convert the WIC bitmap to a HBITMAP
	bitmap = getHBITMAPFromWICBitmap(wicBitmap);

	// Clean up and return
	wicBitmap->Release();
	stream->Release();
	return bitmap;

	// Error handling
exitLoadBitmapResource1:
	stream->Release();
	return NULL;
}

HWND createSplashWindow(HINSTANCE hInstance)
{
	// Register window class
	LPCWSTR winClass = TEXT("MishiraSplash");
	WNDCLASS wc;
	memset(&wc, 0, sizeof(wc));
	//wc.style = CS_OWNDC;
	wc.lpfnWndProc = DefWindowProc; // Default blackhole
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = winClass;
	RegisterClass(&wc);

	// Create a "layered" window that does not appear in alt-tab or the taskbar
	return CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TOOLWINDOW, winClass, NULL,
		WS_POPUP | WS_VISIBLE, 0, 0, 1, 1, NULL, NULL, hInstance, NULL);
}

HWND g_splashWinHwnd = NULL;
void createAndShowLauncherSplash(HINSTANCE hInstance)
{
	// Load bitmap and create the splash window
	HBITMAP bitmap =
		loadBitmapResource(MAKEINTRESOURCE(IDB_PNG1), TEXT("PNG"));
	if(bitmap == NULL)
		return;
	g_splashWinHwnd = createSplashWindow(hInstance);

	// Calculate what size the splash window should be
	BITMAP bmp;
	SIZE size;
	GetObject(bitmap, sizeof(bmp), &bmp);
	size.cx = bmp.bmWidth;
	size.cy = bmp.bmHeight;

	// Get the primary monitor size and position
	POINT posZero; // Reused later
	posZero.x = 0;
	posZero.y = 0;
	HMONITOR monitor = MonitorFromPoint(posZero, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitorInfo;
	monitorInfo.cbSize = sizeof(monitorInfo);
	GetMonitorInfo(monitor, &monitorInfo);

	// Calculate the position that would place the splash window in the middle
	// of the primary monitor
	const RECT &wrk = monitorInfo.rcWork;
	POINT posDst;
	posDst.x = wrk.left + (wrk.right - wrk.left - size.cx) / 2;
	posDst.y = wrk.top + (wrk.bottom - wrk.top - size.cy) / 2;

	// Specified the blending function to use
	BLENDFUNCTION blend;
	memset(&blend, 0, sizeof(blend));
	blend.AlphaFormat = AC_SRC_ALPHA;
	blend.BlendOp = AC_SRC_OVER;
	blend.SourceConstantAlpha = 255;

	// Set the position, size, shape, content, and translucency of the splash
	// window
	HDC hdcDst = GetDC(NULL);
	HDC hdcSrc = CreateCompatibleDC(hdcDst);
	HBITMAP prevObj = (HBITMAP)SelectObject(hdcSrc, bitmap);
	UpdateLayeredWindow(
		g_splashWinHwnd, hdcDst, &posDst, &size, hdcSrc, &posZero,
		RGB(0, 0, 0), &blend, ULW_ALPHA);
	SelectObject(hdcSrc, prevObj);
	DeleteDC(hdcSrc);
	ReleaseDC(NULL, hdcDst);

	// Forcefully focus the splash window as when launched from our MSI
	// installer it opens in the background. NOTE: Doesn't work.
	//SetWindowPos(g_splashWinHwnd, HWND_TOP, 0, 0, 0, 0,
	//	SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	//SetForegroundWindow(g_splashWinHwnd);
	//SetFocus(g_splashWinHwnd);
}

void hideLauncherSplash()
{
	if(g_splashWinHwnd == NULL)
		return; // Already destroyed or never created
	DestroyWindow(g_splashWinHwnd);
	g_splashWinHwnd = NULL;
}

int loadAndExecuteApp(LPCWSTR filename)
{
	HMODULE module = LoadLibrary(filename);
	if(module == NULL) {
		// TODO: Display error dialog
		return 1;
	}
	startApp_t startApp = (startApp_t)GetProcAddress(module, "startApp");
	int ret = 1;
	if(startApp != NULL) {
		ret = startApp(__argc, __argv, &hideLauncherSplash);
	} else {
		// TODO: Display error dialog
	}
	//FreeLibrary(module); // Crashes Qt
	return ret;
}

extern "C"
	int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// If this is not `COINIT_APARTMENTTHREADED` then Qt's clipboard code will
	// not work
	HRESULT res = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	if(SUCCEEDED(res))
		createAndShowLauncherSplash(hInstance);
	return loadAndExecuteApp(TEXT("MishiraApp.dll"));
}
