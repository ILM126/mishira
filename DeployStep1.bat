@echo off
setlocal

rem ***************************************************************************
rem * Mishira: An audiovisual production tool for broadcasting live video
rem *
rem * Copyright (C) 2014 Lucas Murray <lucas@polyflare.com>
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
	echo Usage: DeployStep1.bat ^<debug^|release^> [fast]
	echo.
	echo Use "fast" only when creating internal builds. Do NOT use it for public releases!
	exit /B 1
)
if "%1"=="debug" (
	set BUILDCONFIG=Debug
) else (
	set BUILDCONFIG=Release
)

rem Do we want to be safe and do a full rebuild or just build what's needed?
if "%2"=="fast" (
	set DOREBUILD=0
) else (
	set DOREBUILD=1
)

echo.
echo ========================================================================
echo  Mishira build script: Step 1 ^(%BUILDCONFIG%^)
echo ========================================================================
echo.

rem ---------------------------------------------------------------------------
rem Determine build tool locations

set MSBUILDBIN="%WINDIR%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"
if not exist %MSBUILDBIN% (
	echo Cannot find MSBuild!
	exit /B 1
)
echo MSBuild location: %MSBUILDBIN%

set GITBIN="C:\Program Files (x86)\Git\bin\git.exe"
if not exist %GITBIN%  (
	echo Cannot find Git!
	exit /B 1
)
echo Git location: %GITBIN%

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

rem Get Git commit revision
echo.
echo Fetching Git commit revision...
for /F "delims=" %%i in ('%GITBIN% rev-list HEAD -n 1') do set GITREV=%%i
echo Git revision: %GITREV%

rem Build binaries
echo.
echo --------------------------------------------------------------------------
if %DOREBUILD%==1 (
	echo  Rebuilding all binaries...
) else (
	echo  Building modified binaries...
)
echo --------------------------------------------------------------------------
echo.
if %DOREBUILD%==1 (
	%MSBUILDBIN% Mishira.sln /m /t:Rebuild /p:Configuration="%BUILDCONFIG%";Platform="Mixed Platforms"
) else (
	%MSBUILDBIN% Mishira.sln /m /p:Configuration="%BUILDCONFIG%";Platform="Mixed Platforms"
)
if %errorlevel%==1 exit /B 1

rem ---------------------------------------------------------------------------

echo.
echo Step 1 completed successfully!
