Build settings
==============

This document contains the base Visual Studio build settings that all Mishira projects should use. If creating a new Visual Studio project file it is recommended to copy and paste an existing one (And then modifying the project's GUID) but if that is not possible then you can use the settings below as a guide. In order to apply these settings through the Visual Studio configuration properties dialog you must do the following:

1. Go to the "all options" page for the section that you are modifying.
2. Go through the list of options and make sure that all values that are not project specific (Defines, directories, etc.) are not bold. If they are then delete the value to use the default.
3. Using the search text box at the top of the page set each switch individually.

Compiler settings
-----------------

**Switches that are common to both debug and release builds:**

Switch         | x86 | x64    | Explanation
-------------- | --- | ------ | -----------
`/arch:SSE2`   | Yes | **No** | Use SSE2 instructions
`/EHsc`        | Yes | Yes    | Catch C++ exceptions and assume `extern` C functions never throw
`/GR-`         | Yes | Yes    | Disable C++ RTTI as we don’t use `dynamic_cast` or `typeid`.
`/nologo`      | Yes | Yes    | Hide the Microsoft copyright banner during compilation
`/W3`          | Yes | Yes    | Enable all warnings that are recommended for production purposes
`/WX`          | Yes | Yes    | Treat all warnings as errors
`/Zc:wchar_t-` | Yes | Yes    | Do not treat `wchar_t` as a built-in type as it causes issues when linking libraries

**Switches that are only enabled in debug builds:**

Switch | x86 | x64 | Explanation
------ | --- | --- | -----------
`/Od`  | Yes | Yes | Disables optimizations (Default behaviour)
`/MDd` | Yes | Yes | Use the debug multithreaded DLL C runtime library
`/Gm`  | Yes | Yes | Enable minimal rebuild mode that caches build information in an `.idb` file
`/Zi`  | Yes | Yes | Produce a program database file (`.pdb`)

**Switches that are only enabled in release builds:**

Switch | x86 | x64    | Explanation
------ | --- | ------ | -----------
`/O2`  | Yes | Yes    | Enable optimizations that maximize speed
`/Oi`  | Yes | Yes    | Enable intrinsic function optimizations for faster memory, string and maths functions
`/Oy-` | Yes | **No** | Explicitly do not omit frame pointers (Improves debugging)
`/GL`  | Yes | Yes    | Enable whole program optimization
`/MD ` | Yes | Yes    | Use the release multithreaded DLL C runtime library

**Switches that are automatically set by MSVC:**

Switch     | Explanation
---------- | -----------
`/Fa"..."` | Sets the filename of the assembly listing file (`.asm`). We don't need this but MSVC does it anyway
`/Fd"..."` | Sets the filename of the program database file (`.pdb`) that is used for debugging
`/Fo"..."` | Sets the filename of the object file (`.obj`)
`/Fp"..."` | Sets the filename of the precompiled header file (`.pch`). Only used when precompiled headers are enabled
`/D...`    | Preprocessor definitions
`/I...`    | Include directories

Linker settings
---------------

**Switches that are common to both debug and release builds:**

Switch         | x86 | x64    | Explanation
-------------- | --- | ------ | -----------
`/SUBSYSTEM:WINDOWS,6.0`<br>`/SUBSYSTEM:CONSOLE,6.0` | Yes | Yes | Specifies which environment that the executable should run in. The "6.0" specifies that the executable will only run in Windows Vista or later (Implies `/NXCOMPAT`).
`/DYNAMICBASE` | Yes | Yes    | Enable the address space layout randomization feature of Windows Vista and later.
`/SAFESEH:NO`  | Yes | **No** | Disable safe exception handlers as the feature is extremely picky about when it wants to work (Switch not available on x64). This feature must be enabled in order to obtain Windows Desktop App Certification.
`/NOLOGO`      | Yes | Yes    | Hide the Microsoft copyright banner during compilation
`/MANIFESTUAC:"level='asInvoker' uiAccess='false'"` | Yes | Yes | Specifies that the executable should be executed with the same UAC permissions as the application that launched it.
`/WX`          | Yes | Yes    | Treat all warnings as errors

**Switches that are only enabled in debug builds:**

Switch   | x86 | x64 | Explanation
-------- | --- | --- | -----------
`/DEBUG` | Yes | Yes | Create program database (`.pdb`) debugging information (Implies `/INCREMENTAL` and `/OPT:NOREF`)

**Switches that are only enabled in release builds:**

Switch            | x86 | x64 | Explanation
----------------- | --- | --- | -----------
`/INCREMENTAL:NO` | Yes | Yes | Disable incremental linking to reduce output file size.

**Switches that are automatically set by MSVC:**

Switch                | Explanation
--------------------- | -----------
`/DLL`                | Creates a DLL instead of an EXE
`/LIBPATH:"..."`      | Library include directories
`/OUT:"..."`          | Sets the filename of the output object
`/PDB:"..."`          | Sets the filename of the program database file (`.pdb`) that is used for debugging 
`/IMPLIB:"..."`       | Sets the filename of the import library file (`.lib`) that is used when the object contains exported symbols
`/PGD:"..."`          | Sets the filename of the profile-guided optimization file (`.pgd`)
`/ManifestFile:"..."` | Sets the filename of the intermediate application manifest file (`.manifest`) that will be embedded into the executable itself
