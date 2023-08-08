# libajantv2
**libajantv2** is AJA’s open-source SDK for discovering, interrogating and 
controlling AJA NTV2 professional video I/O devices.

## Directory Organization
The **libajantv2** folder contains the following items:
- **ajaanc** — Classes for encoding & decoding SDI ancillary data packets.
- **ajabase** — Utility classes (e.g. threads, mutexes, files, etc.).
- **ajantv2** — Principal classes, especially **CNTV2Card**.
  - **includes** — Header files.
  - **src** — Source files.
  - **test** — Unit test source code.
  - **utilityfiles** — Additional sources needed to build some command-line utilities.
- **ci** — Continuous integration (CI) scripts.
- **cmake** — Cmake build-related stuff.
- **container** — Docker-related stuff.
- **demos** — Demonstration programs and applications.
- **driver** — Platform-specific device drivers.
- **plugins** — Dynamically-loadable libraries that augment SDK behavior.
- **thirdparty** — Required non-AJA packages (e.g. **doctest**)
- **tools** — Useful command-line utilities.

## Obtaining libajantv2

Clone the libajantv2 repository from GitHub:
```
> git clone git@github.com:aja-video/libajantv2.git
```

## Building libajantv2
Starting in the NTV2 SDK version 17.0, AJA has standardized on [CMake](https://cmake.org/) for describing how to build the libraries, demonstration applications, command-line tools, unit tests, and plugins. AJA requires using CMake version 3.15 or later.

The instructions for building the default static library are generally the same on each supported platform (Windows, macOS, Linux on x64 and aarch64). Note that the default "CMake Generator" varies by platform.

**NOTE: By default — absent any parameters — only the target for the ajantv2 static library is built.**

To build additional targets, the following CMake variables must be set to 'ON' in your CMake build environment:
- `AJANTV2_BUILD_DEMOS` — If enabled, builds the demonstration programs (e.g. **ntv2capture**, **ntv2player**, …).\
By default, demo apps are _not_ built.
- `AJANTV2_BUILD_TOOLS` — If enabled, builds the command-line tools (e.g. **ntv2thermo**, **regio**, **supportlog**, …).\
By default, tools are _not_ built.
- `AJANTV2_BUILD_TESTS` — If enabled, builds any/all unit test(s).\
By default, these test programs are _not_ built.
- `AJANTV2_BUILD_PLUGINS` — If enabled, builds the standard NTV2 plugins (i.e. **nublegacy**, **swdevice**, …).\
By default, plugins are _not_ built.

Please follow the instructions below to build libajantv2 on the supported platform and development environment of your preference.

## Windows
### Option 1: Developer Command Prompt for Visual Studio

Follow these instructions to build libajantv2 from a Developer Command Prompt for Visual Studio:

1. Open a command prompt window (`cmd`) and initialize your Visual Studio environment by running the `vcvarsall.cmd` script from the desired Microsoft Visual Studio directory. The location of vcvarsall.cmd may vary depending on the version of Visual Studio installed on the development system.

   For example, if using Visual Studio 2019 Community Edition:
   ```
   > SET VS_YEAR=2019
   > SET VS_EDITION=Community
   > call "C:\Program Files (x86)\Microsoft Visual Studio\%VS_YEAR%\%VS_EDITION%\VC\Auxiliary\Build\vcvarsall.bat" x64
   ```
2. Run cmake to generate the libajantv2 Visual Studio Solution file in a directory called `build`:
   ```
   > cd libajantv2
   > cmake -S . -B build
   ```
3. Build libajantv2 from the command line, via the generated Visual Studio Solution:
   ```
   > cmake --build build
   ```
4. If the build completes without errors, the static library should be output to `out\build\<arch>-<build type>\ajantv2` under the libajantv2 directory.

   Other build target outputs (demos, tools, etc.) will be available in subdirectories under the build directory mirroring their original location in the libajantv2 source tree.

   For example, the `ntv2enumerateboards.exe` demo app will be located in: `out\build\demos\ntv2enumerateboards`.
   
Alternatively, the generated Visual Studio solution from `build/libajantv2.sln` may be opened in Visual Studio, where libajantv2 can be built via the usual mechanisms.

### Option 2: Visual Studio CMake Integration

Follow these instructions to build libajantv2 via the Microsoft Visual Studio CMake integration. This requires Visual Studio 2019 or 2022.
1. Open Microsoft Visual Studio 2019.
2. From the initial splash page, select "Open a local folder..." and navigate to the libajantv2 repo directory.
3. If CMake is installed and configured properly, the Output window should show the CMake configuration logs and end with a message saying "`CMake generation finished`".
4. From the Build menu select "Build All".
5. If the build completes without errors, the static library should be output to `out\build\<arch>-<build type>\ajantv2` under the libajantv2 directory.

   Other build target outputs (demos, tools, etc.) will be available in subdirectories under the build directory mirroring their original location in the libajantv2 source tree.

   For example, the `ntv2enumerateboards.exe` demo app will be located in: `out\build\demos\ntv2enumerateboards`.

## macOS
### CMake Xcode Project Generation

1. Open a Terminal and generate the XCode project files:
   ```
   $ cd libajantv2
   $ cmake -S . -B build -G Xcode
   ```
2. Build libajantv2 static library from the terminal, via the generated XCode Project:
   ```
   $ cmake --build build
   ```

   NOTE: It is also possible to build the generated XCode Project via the typical `xcrun` command:
   ```
   $ xcrun xcodebuild -project build/libajantv2.xcodeproj -target ALL_BUILD
   ```
3. If the build completes without errors, the static library should be output to `build/ajantv2/<Debug|Release>/libajantv2d.a` under the libajantv2 directory.

   Other build target outputs (demos, tools, etc.) will be available in subdirectories under the build directory mirroring their original location in the libajantv2 source tree.

   For example, the `ntv2enumerateboards` demo app will be located in: `build/demos/ntv2enumerateboards`.

Alternatively, the generated XCode project from `build/libajantv2.xcodeproj` may be opened in XCode, where the static library can be build via the usual mechanisms.

## macOS and Linux
### GNU Makefiles
1. Open a Terminal window and run cmake to generate the GNU Makefiles in a directory called `build`.
   ```
   $ cd libajantv2
   $ cmake -S . -B build
   ```
2. Build the libajantv2 static library from the generated GNU Makefiles:
   ```
   $ cmake --build build
   ```
3. If the build completes without errors, the static library should be output to `out\build\<arch>-<build type>\ajantv2` under the libajantv2 directory.

   Other build target outputs (demos, tools, etc.) will be available in subdirectories under the build directory mirroring their original location in the libajantv2 source tree.

   For example, the `ntv2enumerateboards` demo app will be located in: `out\build\demos\ntv2enumerateboards`.

## All Platforms
### Ninja Build

If Ninja Build is installed in the PATH it is possible to generate .ninja build configuration files with CMake.

Ninja Build is available from GitHub, or via the package manager of your preference.

Download: https://github.com/ninja-build/ninja/releases

NOTE: The compiler toolset used by Ninja Build will vary depending which compiler CMake finds in the PATH by default. On macOS and Linux this is clang or gcc, and on Windows this is usually the cl compiler available under the current Developer Command Prompt for Visual Studio Environment.

1. Open a Terminal or Command Prompt window and run cmake with the Ninja generator specified, to created .ninja files in a directory called `build`.
   ```
   $ cd libajantv2
   $ cmake -S . -B build -GNinja
   ```
2. Build all configured libajantv2 targets:
   ```
   $ cmake --build build
   ```
3. If the build completes without errors, the libajantv2 static library should be available in `build/ajantv2`.

   Other build target outputs (demos, tools, etc.) will be available in subdirectories under the build directory mirroring their original location in the libajantv2 source tree.

### Visual Studio Code
The libajantv2 repository can be opened as a directory in Visual Studio Code and built with the optional Microsoft CMake Extension for VSCode.

1. Launch Visual Studio Code
2. From the File menu select Open (or Open Folder on macOS) and navigate to the libajantv2 repo directory.
3. In the left-hand sidebar click on the Extensions button or press Ctrl+Shift+P (Cmd+Shift+P on macOS) to open the Command Palette and search for "Extensions: Install Extensions".
4. In the left-hand sidebar search for the "cmake" extension from developer "twxs" (also available in the CMake Tools extension from Microsoft) and install it.
5. Open the Command Palette once again and search for "CMake: Configure".
6. If the configuration completes successfully, open the Command Palette and search for "CMake: Build".
7. If the build completes without errors, the libajantv2 static library should be available in `build/ajantv2`.

   Other build target outputs (demos, tools, etc.) will be available in subdirectories under the build directory mirroring their original location in the libajantv2 source tree.

### Qt Creator
The libajantv2 repository can be opened as a directory in Qt Creator and built with the Qt Creator CMake integration.
