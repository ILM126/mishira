@echo off
setlocal

rem ***************************************************************************
rem * Mishira: An audiovisual production tool for broadcasting live video
rem *
rem * Copyright (C) 2014 Lucas Murray <lmurray@undefinedfire.com>
rem * All rights reserved.
rem *
rem * This program is free software; you can redistribute it and/or modify it
rem * under the terms of the GNU General Public License as published by the
rem * Free Software Foundation; either version 2 of the License, or (at your
rem * option) any later version.
rem *
rem * This program is distributed in the hope that it will be useful, but
rem * WITHOUT ANY WARRANTY; without even the implied warranty of
rem * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
rem * Public License for more details.
rem ***************************************************************************

rem Are we build the debug or release version?
if not "%1"=="debug" if not "%1"=="release" (
	echo Usage: PostBuild.bat ^<debug^|release^>
	exit /B 1
)
if "%1"=="debug" (
	set BUILDCONFIG=Debug
) else (
	set BUILDCONFIG=Release
)

rem ---------------------------------------------------------------------------
rem Target and deploy directories

set TARGET32=".\Win32\%BUILDCONFIG%"
set TARGET64=".\x64\%BUILDCONFIG%"
set DEPLOYDIR=".\Deploy\%BUILDCONFIG%"

rem ---------------------------------------------------------------------------
rem Verify that we are in the correct directory

if not exist Mishira.sln (
	echo This script must be executed from the project's root directory
	exit /B 1
)

rem ---------------------------------------------------------------------------
rem Clean the deploy directory. We must first rename otherwise we will error

echo.
echo Cleaning deploy directory...
if not exist .\Deploy mkdir .\Deploy
if not exist %DEPLOYDIR% mkdir %DEPLOYDIR%
move %DEPLOYDIR% %DEPLOYDIR%1
if %errorlevel%==1 (
	echo Access denied errors may be caused by Windows Explorer itself
	echo Please close any open Windows Explorer windows and try again
	exit /B 1
)
rmdir %DEPLOYDIR%1 /S /Q
mkdir %DEPLOYDIR%
rem del %DEPLOYDIR%\* /F /Q

rem ---------------------------------------------------------------------------
rem Sign executables

rem FIXME

rem ---------------------------------------------------------------------------
rem Deploy files (If any of these change make sure to update the clean above as well)

echo Deploying files...

rem Mishira binaries
copy "%TARGET32%\Mishira.exe" %DEPLOYDIR%
copy "%TARGET32%\MishiraApp.dll" %DEPLOYDIR%

rem Mishira text files
copy ".\Media\Legal.html" %DEPLOYDIR%
copy ".\Media\License.html" %DEPLOYDIR%

rem Mishira media files
mkdir %DEPLOYDIR%\Media
rem copy ".\Media\Mishira logo background 540p.jpg" %DEPLOYDIR%\Media
copy ".\Media\Mishira logo background 1080p.jpg" %DEPLOYDIR%\Media

rem Mishira example scripts
mkdir "%DEPLOYDIR%\Example scripts"
copy ".\Media\Countdown and up timer script.js" "%DEPLOYDIR%\Example scripts"
copy ".\Media\File contents script.js" "%DEPLOYDIR%\Example scripts"
copy ".\Media\Website title script.js" "%DEPLOYDIR%\Example scripts"

rem Libbroadcast binaries
if %BUILDCONFIG%==Debug (
	copy "%LIBBROADCAST_DIR%\bin\Libbroadcastd.dll" %DEPLOYDIR%
) else (
	copy "%LIBBROADCAST_DIR%\bin\Libbroadcast.dll" %DEPLOYDIR%
)

rem Libvidgfx binaries
if %BUILDCONFIG%==Debug (
	copy "%LIBVIDGFX_DIR%\bin\Libvidgfxd.dll" %DEPLOYDIR%
) else (
	copy "%LIBVIDGFX_DIR%\bin\Libvidgfx.dll" %DEPLOYDIR%
)

rem Libdeskcap binaries
if %BUILDCONFIG%==Debug (
	copy "%LIBDESKCAP_DIR%\bin\Libdeskcapd.dll" %DEPLOYDIR%
	copy "%LIBDESKCAP_DIR%\bin\MishiraHelperd.exe" %DEPLOYDIR%
	copy "%LIBDESKCAP_DIR%\bin\MishiraHookd.dll" %DEPLOYDIR%
	copy "%LIBDESKCAP_DIR%\bin\MishiraHelper64d.exe" %DEPLOYDIR%
	copy "%LIBDESKCAP_DIR%\bin\MishiraHook64d.dll" %DEPLOYDIR%
) else (
	copy "%LIBDESKCAP_DIR%\bin\Libdeskcap.dll" %DEPLOYDIR%
	copy "%LIBDESKCAP_DIR%\bin\MishiraHelper.exe" %DEPLOYDIR%
	copy "%LIBDESKCAP_DIR%\bin\MishiraHook.dll" %DEPLOYDIR%
	copy "%LIBDESKCAP_DIR%\bin\MishiraHelper64.exe" %DEPLOYDIR%
	copy "%LIBDESKCAP_DIR%\bin\MishiraHook64.dll" %DEPLOYDIR%
)

rem ffmpeg binaries
copy "%FFMPEG_DIR%\bin\avcodec-54.dll" %DEPLOYDIR%
copy "%FFMPEG_DIR%\bin\avformat-54.dll" %DEPLOYDIR%
copy "%FFMPEG_DIR%\bin\avutil-52.dll" %DEPLOYDIR%
copy "%FFMPEG_DIR%\bin\swresample-0.dll" %DEPLOYDIR%

rem FDK-AAC binaries
copy "%FDKAAC_DIR%\bin\libfdk-aac-0.dll" %DEPLOYDIR%

rem x264 binaries
copy "%X264_DIR%\bin\libx264-133.dll" %DEPLOYDIR%

rem Qt binaries
if %BUILDCONFIG%==Debug (
	rem Root Qt binaries
	copy "%QTDIR%\bin\Qt5Cored.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\Qt5Guid.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\Qt5Networkd.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\Qt5Widgetsd.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\Qt5Scriptd.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\libEGLd.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\libGLESv2d.dll" %DEPLOYDIR%

	rem Qt image format plugins
	mkdir %DEPLOYDIR%\imageformats
	copy "%QTDIR%\plugins\imageformats\qgifd.dll" %DEPLOYDIR%\imageformats
	copy "%QTDIR%\plugins\imageformats\qjpegd.dll" %DEPLOYDIR%\imageformats

	rem Qt platform plugins
	mkdir %DEPLOYDIR%\platforms
	copy "%QTDIR%\plugins\platforms\qwindowsd.dll" %DEPLOYDIR%\platforms

	rem Qt accessible plugins
	mkdir %DEPLOYDIR%\accessible
	copy "%QTDIR%\plugins\accessible\qtaccessiblewidgetsd.dll" %DEPLOYDIR%\accessible
) else (
	rem Root Qt binaries
	copy "%QTDIR%\bin\Qt5Core.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\Qt5Gui.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\Qt5Network.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\Qt5Widgets.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\Qt5Script.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\libEGL.dll" %DEPLOYDIR%
	copy "%QTDIR%\bin\libGLESv2.dll" %DEPLOYDIR%

	rem Qt image format plugins
	mkdir %DEPLOYDIR%\imageformats
	copy "%QTDIR%\plugins\imageformats\qgif.dll" %DEPLOYDIR%\imageformats
	copy "%QTDIR%\plugins\imageformats\qjpeg.dll" %DEPLOYDIR%\imageformats

	rem Qt platform plugins
	mkdir %DEPLOYDIR%\platforms
	copy "%QTDIR%\plugins\platforms\qwindows.dll" %DEPLOYDIR%\platforms

	rem Qt accessible plugins
	mkdir %DEPLOYDIR%\accessible
	copy "%QTDIR%\plugins\accessible\qtaccessiblewidgets.dll" %DEPLOYDIR%\accessible
)

rem DirectX binaries
copy "C:\Program Files (x86)\Windows Kits\8.0\Redist\D3D\x86\d3dcompiler_46.dll" %DEPLOYDIR%

rem ---------------------------------------------------------------------------

echo.
echo Post build script completed successfully!
