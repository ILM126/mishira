Building Mishira from source (Windows)
======================================

This document describes the recommended procedure for compiling the Windows version of Mishira and all of its dependencies directly from their source code. Official builds of Mishira will follow these instructions to the letter so if you would like to have a 100% identical version of Mishira then it is recommended that you also do the same. That being said it is be possible to compile Mishira with slightly different versions of libraries and build environment than what is outlined below. If you do so however be warned that you may experience different behaviour (Including bugs and performance results) that what is found in the official builds.

Environment variables
=====================

Mishira's build system relies on environment variables in order to locate the correct version of dependencies. This is done in order to allow developers to use different directories for their dependencies if they wish. While this document specifies certain directories for each dependency developers may choose to install to a different directory as long as they correctly set the corresponding environment variables.

Developers should be warned that not all applications support environment variables that contain other variables so it is recommended to specify the full path for each environment variable manually.

**NOTE:** Environment variables are not detected by applications unless the variable is defined before they are launched. This means that you may need to restart applications (Visual Studio being the main one) several times throughout the build process if a later step uses an environment variable that was defined in an earlier step.

Creating and applying patches
=============================

Mishira maintains several patch files for its dependencies in order to fix bugs, improve performance and/or add additional features that don't exist in the original versions of the dependencies. Patches can be created by using `diff -rupN $OLD $NEW` in the "**MinGW Shell**" or similar terminal and can be applied using `patch -p1 < $PATCH_FILENAME`.

Development environment
=======================

Visual Studio 2012 (Update 3)
-----------------------------

Mishira's official builds are compiled with the commercial version of Visual Studio 2012 Update 3. While Express and trial versions should work they have not been tested. If you are an MSDN subscriber then the procedure for installing Visual Studio is as follows:

1.	Download and launch "**Visual Studio Ultimate 2012 (x86) - Web Installer (English)**" from the MSDN subscriber website.
2.	Restart the computer when prompted to do so.
3.	Launch Visual Studio and complete its automated setup process making sure that "**Visual C++ Development Settings**" is selected for "**Default environment settings**".
4.	Exit Visual Studio.
5.	Download and launch "**Visual Studio 2012 Update 3 - English**" from the main Visual Studio page on the official Microsoft website. You may disregard any setup warnings relating to 2010 and 2011 root certificates if they appear.
6.	Restart the computer when prompted to do so.
7.	Launch Visual Studio and then immediately exit. This is to make sure that everything is fully installed and ready to be used for compiling dependencies that call Visual Studio from the command line.

Visual Studio add-ins and extensions
------------------------------------

Mishira **requires** the following Visual Studio add-ins and extensions to be installed:

- **Qt's official add-in** (For automatically compiling Qt MOC, resource and form files), and
- the **WiX toolset v3.x** (For generating the official Mishira installer).

**NOTE:** It is recommended to install Qt's official add-in only after Qt has been installed on the system. Instructions on how to install the Qt add-in are located in the Qt installation instructions later in this document.

The following Visual Studio add-ins and extensions are also recommended to be installed but are **optional**:

- **Switch extension** (Keyboard shortcuts for switching between files),
- **CodeMaid extension** (For automatic code clean up and styling),
- **Editor guidelines extension** (To show where the 80th character column is), and
- **GuidInserter2** (For quickly generating random GUIDs which are used everywhere in Mishira).

Windows SDK for Windows 8.1
---------------------------

The Windows SDK is used by Mishira for accessing Windows-specific APIs and official helper libraries. The official installer is called the "**Windows Software Development Kit (SDK) for Windows 8.1**" and can be found on the MSDN web center website. It is recommended to leave all settings at their defaults except for the "**.NET Framework 4.5.1 Software Development Kit**" setting which can be disabled if not needed.

msysgit (A.k.a. Git for Windows)
--------------------------------

Mishira's build scripts require access to a Git executable in order to embed information about the build into the executable for debugging purposes. While developers can use any Git client they want for accessing the Mishira source code we recommend also installing **msysgit** (Which is also known as Git for Windows) from the official msysgit website for use by the build script, making sure that the Git executable is added to the system's `PATH` environment variable.

MinGW 20120426 (GCC 4.7.2)
--------------------------

While Mishira itself is compiled using Visual Studio many of its dependencies require GCC to be installed in order to build them. Due to the way that MinGW is distributed it is difficult to specify an exact version to install. Official builds of Mishira use GCC 4.7.2 but if you cannot install that particular version you may use any similar compatible version.

The step-by-step procedure for installing MinGW is as follows:

1.	**Download "mingw-get-inst"** from the official MinGW website.
2.	**Install** it with the following settings:
	1.	Ensure that "**Download latest repository catalogues**" is enabled.
	2.	Install to the default directory (`C:\MinGW`).
	3.	Disable all components except "**C Compiler**", "**C++ Compiler**" and "**MSYS Basic System**".
3.	Open the "**MinGW shell**" and type the following to install additional dependencies that are commonly used:
	```
	$ mingw-get install msys-patch
	$ mingw-get install autoconf
	$ mingw-get install automake
	$ mingw-get install libtool
	$ mingw-get install msys-coreutils
	```

NASM 2.10.09
------------

NASM is an x86 and x86-64 assembler and is used by some of Mishira's dependencies such as OpenSSL. The NASM Windows installer can be found on the official [NASM website](http://www.nasm.us/). The installer needs to be executed as an administrator user and it is recommended to leave all settings at their defaults except for "**VS8 integration**" which should be disabled. Once the installer has completed you will need to prepend `C:\Program Files (x86)\nasm\;` to the start of your `PATH` environment variable manually.

Yasm 1.2.0
----------

Yasm is another x86 and x86-64 assembler based on NASM which supposedly produces higher performance output. Yasm is mainly used by x264 for assembling all of its highly optimised inner loops and if it is not installed will result in much slower performance.

Yasm is very easy to install. Just download the **Yasm Win64 executable** from the official [Yasm website](http://yasm.tortall.net/), copy the file into MinGW's bin directory (`C:\MinGW\bin`) and rename it to `yasm.exe`.

ActivePerl 5.16.3.1603
----------------------

Qt and a couple of other dependencies of Mishira use Perl in their build scripts. Simply download the **ActivePerl Community Edition x64 MSI** from the official ActiveState website and install it with all options left at their defaults with the exception of "**Create Perl file extension association**" which can be disabled if it's not required.

Python 3.3.2
------------

Qt and a couple of other dependencies of Mishira also use Python in their build scripts. Simply download the **Python 3 "Windows X86-64 MSI Installer"** from the official Python website and install it with all of its options left at their defaults except "**Add python.exe to Path**" which should be enabled.

Building Qt
===========

The main dependency of Mishira is Qt. It is the single largest library that is used and is around 5 times larger than Mishira itself when fully compiled in a release configuration. As there are many different ways to compile Qt Mishira uses the following method.

OpenSSL 1.0.1e
--------------

The only external dependency that Qt requires is OpenSSL, all of its other dependencies are either distributed within Qt internally or are disabled by us.

Download the **OpenSSL 1.0.1e** source code tarball from the official [OpenSSL website](http://www.openssl.org/source/) and extract it to `C:\src\openssl-1.0.1e`.

Navigate to the above directory with the "**Visual Studio 2012 Command Prompt**" and execute the following to build OpenSSL as a static library only:

```
$ perl Configure VC-WIN32 enable-static-engine --prefix=C:\src\openssl-1.0.1e\build
$ ms\do_nasm
$ nmake -f ms\nt.mak
$ nmake -f ms\nt.mak install
```

Once that is completed set the following environment variable:

- `OPENSSL_DIR` = `C:\src\openssl-1.0.1e\build`

Qt 5.0.2
--------

Mishira currently depends on an older version of Qt which makes it difficult to find source code and documentation. The following procedure is temporary until Mishira is updated to the latest version of Qt.

Even on the official Qt website the source code of Qt is distributed in many different formats and each download has different files included within it. We want the unmodified **Windows** source code for Qt 5.0.2 which is distributed only in one file: `qt-everywhere-opensource-src-5.0.2.7z`. **Do not** download any other file as even the `.zip` and `.tar.gz` versions contain different files to the `.7z`. As of writing this document (3 August 2014) the file that we want can be located at [here](http://download.qt-project.org/archive/qt/5.0/5.0.2/single/).

Once you have download the source code extract it to `C:\src` and rename the directory from `qt-everywhere-opensource-src-5.0.2` to `qt-5.0.2`.

Mishira requires several patches to be applied to the Qt source code before it is compiled. These patches can be found in the `Patches` directory and are applied using the method outlined at the beginning of this document.

Once the patches have been applied navigate to the Qt source root directory with the "**Visual Studio 2012 Command Prompt**" and execute the following to compile it. This may take a very long time depending on the speed of your CPU.

```
$ mkdir build
$ cd build
$ ..\configure -debug-and-release -prefix C:\Qt\5.0.2 -opensource -platform win32-msvc2012 -nomake tests -nomake examples -skip qtwebkit-examples-and-demos -qt-zlib -qt-pcre -qt-libpng -qt-libjpeg -qt-freetype -openssl -openssl-linked -I %OPENSSL_DIR%\include -L %OPENSSL_DIR%\lib OPENSSL_LIBS="-lUser32 -lAdvapi32 -lGdi32 -lssleay32 -llibeay32"
$ nmake
$ nmake install
```

Once Qt has been compiled and installed to `C:\Qt\5.0.2` copy the `d3dcompiler_46.dll` DLL from `C:\Program Files (x86)\Windows Kits\8.0\bin\x86` into `C:\Qt\5.0.2\bin`. This file is required by ANGLE (A dependency of Qt which is distributed inside of its source code) which implements the OpenGL API on top of DirectX.

As Qt is now installed on the system it is now safe to install Qt's "**Visual Studio Add-in**" which can be found in the [downloads section](http://qt-project.org/downloads) of the official Qt website.

Once everything is completed you can set the following environment variable and prepend `C:\Qt\5.0.2\bin;` to your `PATH`:

- `QTDIR` = `C:\Qt\5.0.2`

### Recompiling Qt

If you ever require to reconfigure or recompile Qt it is highly recommended to do so with a completely clean source code tree. Qt's build scripts are massive and sometimes they don't properly handle stale configuration files during recompiling which results in incorrect output files. If you followed the build instructions above exactly then you can clean Qt very quickly by just deleting all the files found in the `build` directory. Alternatively you can navigate to the Qt source root directory using the "**Visual Studio 2012 Command Prompt**" and then execute the following:

```
$ nmake clean
$ nmake distclean
```

The initial `nmake clean` might not actually be required but it hasn't been tested and it's better to be safe.

Unit testing framework
======================

Mishira and its internal libraries use Google Test for unit testing. While the unit tests are not distributed to users they are still required in order to properly build Mishira's Visual Studio solutions.

Google Test 1.6.0
-----------------

As the default compilation settings in Google Test cause some issues with the version of Visual Studio that Mishira uses we need to modify them slightly in order to use the framework. First **download** the testing framework source code from the [official website](https://code.google.com/p/googletest/downloads/list) and extract the archive into `C:\src`. After doing so follow the following procedure to modify the compilation settings and build the framework:

1.	Open the `msvc/gtest.sln` solution in **Visual Studio** and upgrade it to the latest version.
2.	Open up the project settings for the "**gtest**" project, navigate to "**Configuration Properties -> C/C++ -> Preprocessor**" and add `_VARIADIC_MAX=10` to the "**Preprocessor Definitions**".
3.	Repeat step 2 for the other 3 projects in the solution.
4.	Open up the project settings for the "**gtest**" project, navigate to "**Configuration Properties -> C/C++ -> Code Generation**" and change "**Runtime Library**" to "**Multi-threaded Debug DLL (/MDd)**".
5.	Repeat step 4 for the other 3 projects in the solution.
6.	Build the entire solution by pressing F7.
7.	Switch to the release configuration and repeat steps 2-6 except using "**Multi-threaded DLL (/MD)**" as the "**Runtime Library**".

Once everything is built set the following environment variable:

- `GTEST_DIR` = `C:\src\gtest-1.6.0`

Libdeskcap dependencies
=======================

In addition to Qt and Google Test, Mishira's Libdeskcap library relies on the following additional dependencies being present on the development system.

Boost 1.54.0
------------

Boost is used by Libdeskcap in order to more easily transfer data between processes using shared memory segments. To install Boost first **download** the source code from the official [Boost website](http://www.boost.org/users/download/) and then extract it to `C:\src`. After that is one navigate to the created Boost directory with the "**Visual Studio 2012 Command Prompt**" and execute the following to build and install it on the system:

```
$ rmdir /S /Q C:\Boost\1.54.0\lib
$ bootstrap.bat
$ b2 --prefix=C:\Boost\1.54.0 --build-dir=.\build\ --build-type=complete address-model=32 link=static runtime-link=shared threading=multi install
$ rename C:\Boost\1.54.0\lib lib32
$ b2 --prefix=C:\Boost\1.54.0 --build-dir=.\build\ --build-type=complete address-model=64 link=static runtime-link=shared threading=multi install
$ rename C:\Boost\1.54.0\lib lib64
$ mkdir C:\Boost\1.54.0\lib
$ move C:\Boost\1.54.0\lib32 C:\Boost\1.54.0\lib\Win32
$ move C:\Boost\1.54.0\lib64 C:\Boost\1.54.0\lib\x64
```

Once everything is built set the following environment variable:

- `BOOST_DIR` = `C:\Boost\1.54.0`

GLEW 1.10.0
-----------

Libdeskcap uses GLEW in order to more easily capture back buffer pixel data from OpenGL applications. As we want to be able to optionally dynamically link to OpenGL in our injected hooks we must make some small changes to the GLEW source code before building it. The procedure for building GLEW is as follows:

1.	**Download** the GLEW source code from the [official website](http://glew.sourceforge.net/) and extract it to `C:\src`.
2.	**Apply the custom GLEW patches** that can be found in the `Patches` directory in the Libdeskcap repository using the method outlined at the beginning of this document.
3.	**Open `build\glew.rc` in any standard text editor** and break up the extremely long string literal into multiple pieces by inserting `""` at several locations. If we do not make this change then Visual Studio will fail to compile the project due to the string literal being too long.
4.	**Open `build\vc10\glew.sln` in Visual Studio 2012** and update the solution to the latest version.
5.	Right-click on the `glew_static` project, select "**Properties**" and change the "**Configuration Properties -> C/C++ -> Code Generation -> Runtime Library**" setting to "**Multi-threaded DLL (/MD)**" for the "**Win32**" and "**x64**" platforms of the "**Release MX**" configuration.
6.	Change the build configuration to "**Debug MX**"/"**Win32**", right-click on the `glew_static` project and select "**Build**".
7.	Change the build configuration to "**Debug MX**"/"**x64**", right-click on the `glew_static` project and select "**Build**".
8.	Change the build configuration to "**Release MX**"/"**x64**", right-click on the `glew_static` project and select "**Build**".
9.	Change the build configuration to "**Release MX**"/"**Win32**", right-click on the `glew_static` project and select "**Build**".

Once all of that is done set the following environment variable:

- `GLEW_DIR` = `C:\src\glew-1.10.0`

Main application dependencies
=============================

Official Mishira libraries
--------------------------

Libbroadcast, Libvidgfx and Libdeskcap are all official libraries of the Mishira project. All the libraries are built exactly the same way as the main Mishira application. Any additional build instructions for the libraries can be found in their appropriate repository if they are needed. The repositories for these libraries can be found in the official [Mishira GitHub organisation](https://www.github.com/mishira).

Assuming that everything is installed into `C:\src` then set the following environment variables to the values listed below:

- `LIBBROADCAST_DIR` = `C:\src\Libbroadcast\Win32`
- `LIBVIDGFX_DIR` = `C:\src\Libvidgfx\Win32`
- `LIBDESKCAP_DIR` = `C:\src\Libdeskcap\Win32`

Fraunhofer FDK AAC (Commit: fcb5f1b692)
---------------------------------------

FDK AAC is the AAC encoder that is used by Mishira for producing high quality audio output. As the library is actually a part of the Android operating system we use an unofficial stand-alone port that is maintained by [Martin Storsjö on GitHub](https://github.com/mstorsjo). As we do not require the AAC decoder that is inside the library we disable it manually with a patch. The procedure for building the library is as follows:

1.	Clone [Martin's Git repository](https://github.com/mstorsjo/fdk-aac) into `C:\src\fdk-aac` and switch to the above Git commit.
2.	**Apply the custom FDK AAC patches** from the `Patches` directory using the method outlined at the beginning of this document.
3.	Navigate to library's root directory using the "**MinGW shell**" and execute the following in order to build it:
	```
	$ ./autogen.sh
	$ configure --prefix=/c/src/fdk-aac/build/ --disable-static CFLAGS=-Wl,--output-def=libfdk-aac-0.def
	$ make
	$ make install
	```
4.	Navigate to library's root directory using the "**Visual Studio 2012 Command Prompt**" and execute the following to generate the export definition file that Visual Studio requires to link to the library:
	```
	$ lib /MACHINE:X86 /DEF:libfdk-aac-0.def /OUT:build\lib\libfdk-aac-0.lib
	```

Once the above is done set the following environment variable:

- `FDKAAC_DIR` = `C:\src\fdk-aac\build`

x264 stable build 133 (Commit: 585324fee3)
------------------------------------------

Mishira uses x264 in order to encode H.264 video. We use the library directly instead of going through an abstraction library such as FFmpeg as we use some advance functionality of x264 that just isn't exposed through FFmpeg's simplified interface. Mishira contains a custom H.264 bitstream transcoder that inserts padding bits just before transmitting them to remote hosts that require a constant video bitrate while allowing the padding bits to not be added to local recordings which can waste significant hard drive space. This transcoder needs access to the raw H.264 NAL metadata that is stripped away by FFmpeg as FFmpeg actually concatenates multiple NAL units together before returning the data to the application as a single "opaque" block that should not be modified.

The procedure for building x264 from Git is as follows:

1.	Clone the [official Git repository](http://www.videolan.org/developers/x264.html) into `C:\src\x264` and switch to the above Git commit.
2.	Navigate to x264's root directory using the "**MinGW shell**" and execute the following in order to build it. Note that any error relating to Git not being installed can be safely ignored.
	```
	$ configure --prefix=./build/ --enable-shared --enable-win32thread --disable-interlaced --extra-ldflags=-Wl,--output-def=libx264-133.def
	$ mingw32-make
	$ mingw32-make install
	```
3.	Navigate to x264's root directory using the "**Visual Studio 2012 Command Prompt**" and execute the following to generate the export definition file that Visual Studio requires to link to the library:
	```
	$ lib /MACHINE:X86 /DEF:libx264-133.def /OUT:build\lib\libx264-133.lib
	```

Once the above is done set the following environment variable:

- `X264_DIR` = `C:\src\x264\build`

### Future consideration (TODO)

We should investigate whether using the following configuration switches are beneficial or not to Mishira:

```
--enable-debug --enable-strip
--disable-avs --disable-swscale --disable-lavf --disable-ffms --disable-gpac
```

FFmpeg 1.2.2
------------

Mishira uses FFmpeg for audio resampling, audio mixing (E.g. surround sound to stereo) and AV muxing for saving to local files. The procedure for building the library is as follows:

1.	**Download the FFmpeg source code tarball** from the [official website](http://www.ffmpeg.org/releases/) and extract it to `C:\src`. Note that if you ever check out FFmpeg on a Windows machine from Git you must immediately execute `git config core.autocrlf false` and then `git clean -x -f` in order to fix the "missing separator. Stop." messages during `make install`.
2.	Navigate to FFmpeg's root directory with "**MinGW shell**" and execute the following to build it. Note that is may be needed to kill Perl twice during configuration. The reason for this is so far unknown and it doesn't seem to have any negative side effects.
	```
	$ configure --prefix=./build/ --disable-static --enable-shared --enable-memalign-hack --disable-programs --disable-doc --disable-avfilter --disable-postproc --disable-everything --enable-muxer=matroska --enable-muxer=mp4 --enable-protocol=file
	$ mingw32-make -j2
	$ mingw32-make install
	```
3.	Open up `build/include/libavutil/common.h` in a text editor and replace `#include <inttypes.h>` with `#include <stdint.h>` so that it works with Visual Studio.

Once the above is done set the following environment variable:

- `FFMPEG_DIR` = `C:\src\ffmpeg-1.2.2\build`

### Future consideration (TODO)

We should investigate whether using the following configuration switches are beneficial or not to Mishira:

```
--disable-avdevice --enable-libspeex
--enable-encoder=flv --enable-muxer=flv
```

We should also investigate whether we should disable SSSE3 support or not.

Building Mishira
================

After all of the above is done (Phew!) it is finally time to build Mishira itself. Right now development builds of Mishira are compiled entirely within the main Visual Studio solution which is the `Mishira.sln` file in the root of the repository. Please do not upgrade the solution or project files to later Visual Studio versions if asked.

The main Mishira executable is the "Mishira" project inside of the solution and should be set to the "StartUp Project" (sic) if compiling Mishira itself. This project contains a custom build step that is executed after building which copies all the required files to run Mishira into a directory called `Deploy` in the root of the repository. If you debug or launch the application from within Visual Studio it will execute the binary found in that deploy directory.

Most of the Mishira source code is compiled as a DLL called `MishiraApp.dll` and loaded dynamically at run time. We do it this way in order to be more responsive on application launch as it means the operating system doesn't need to load as much from the hard drive before we can display a splash screen. Basically `Mishira.exe` is an extremely small and low level application that only depends on the main Windows system libraries that are always loaded into RAM with the sole purpose of displaying a splash screen **before** dynamically linking to `MishiraApp.dll` and then executing the real application code.

Creating release builds
-----------------------

In order to maintain high quality releases Mishira uses a mostly automatic build system outside of Visual Studio for building the official binaries and installer. These scripts should not be used for general development as they are slower to compile and overall more difficult to use. Instructions for using these scripts are not publicly available.
