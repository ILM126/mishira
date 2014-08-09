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
	echo Usage: DeployStep2.bat ^<debug^|release^> [noupdater]
	echo.
	echo Use "noupdater" only when creating internal builds. Do NOT use it for public releases!
	exit /B 1
)
if "%1"=="debug" (
	set BUILDCONFIG=Debug
) else (
	set BUILDCONFIG=Release
)

rem Do we want to skip deploying wyUpdate
if "%2"=="noupdater" (
	set INCUPDATER=0
) else (
	set INCUPDATER=1
)

echo.
echo ========================================================================
echo  Mishira build script: Step 2 ^(%BUILDCONFIG%^)
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

rem Verify that step 1 of the build script has previously been executed
if not exist %DEPLOYDIR%\Mishira.exe (
	echo This script must be executed after `BuildStep1.bat`
	exit /B 1
)

rem Verify that wyUpdate has been copied to the deploy directory or create the
rem dummy files if "noupdater" has been set
if %INCUPDATER%==1 (
	if not exist %DEPLOYDIR%\client.wyc (
		echo This script must be executed after wyUpdate has been deployed
		exit /B 1
	)
) else (
	echo. 2>%DEPLOYDIR%\client.wyc
	echo. 2>%DEPLOYDIR%\wyUpdate.exe
)

rem Clean the deploy directory
echo.
echo Cleaning deploy directory...
if %BUILDCONFIG%==Debug (
	del .\Deploy\MishiraSetup32-debug.msi /F /Q
	del .\Deploy\MishiraSetup64-debug.msi /F /Q
) else (
	del .\Deploy\MishiraSetup32.msi /F /Q
	del .\Deploy\MishiraSetup64.msi /F /Q
)

rem Sign `wyUpdate.exe` as our wyBuild settings doesn't allow it to be signed
rem FIXME

rem Build installers
echo.
echo --------------------------------------------------------------------------
echo  Building installers...
echo --------------------------------------------------------------------------
echo.
%MSBUILDBIN% Mishira.sln /m /t:Rebuild /p:Configuration="%BUILDCONFIG%";Platform="x86 Installer"
if %errorlevel%==1 exit /B 1
%MSBUILDBIN% Mishira.sln /m /t:Rebuild /p:Configuration="%BUILDCONFIG%";Platform="x64 Installer"
if %errorlevel%==1 exit /B 1

rem Sign installers
rem FIXME

rem ---------------------------------------------------------------------------
rem Deploy files (If any of these change make sure to update the clean above as well)

echo Deploying files...

rem Installers
if %BUILDCONFIG%==Debug (
	copy %TARGET32%\en-US\MishiraSetup.msi .\Deploy\MishiraSetup32-debug.msi
	copy %TARGET64%\en-US\MishiraSetup.msi .\Deploy\MishiraSetup64-debug.msi
) else (
	copy %TARGET32%\en-US\MishiraSetup.msi .\Deploy\MishiraSetup32.msi
	copy %TARGET64%\en-US\MishiraSetup.msi .\Deploy\MishiraSetup64.msi
)

rem ---------------------------------------------------------------------------

echo.
echo Step 2 completed successfully!
