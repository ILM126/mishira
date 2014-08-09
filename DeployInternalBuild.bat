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

if "%1"=="debug" (
	call DeployStep1.bat debug fast
	call DeployStep2.bat debug noupdater
) else (
	call DeployStep1.bat release fast
	call DeployStep2.bat release noupdater
)
