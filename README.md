<center>
  <a href="https://www.aja.com/">
    <img src="aja_logo.svg" alt="AJA Video Systems" width="33%"/>
  </a>
</center>

# libajantv2

This is the open-source SDK for discovering, interrogating and controlling NTV2 professional video I/O devices from AJA Video Systems, Inc. All code herein is licensed under the MIT license.

# Table of Contents
1. [Directory Layout](#directory-layout)
1. [Obtaining libajantv2](#obtaining)
1. [Building libajantv2](#building)
1. [Building the Kernel Module Driver (Linux)](#building-kernel-module)
1. [Verifying the Kernel Module Driver (Linux)](#verifying-kernel-module)
1. [Customizing NTV2](#customizing-ntv2)

## Directory Layout
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
Starting in the NTV2 SDK version 17.0, AJA has standardized on [CMake](https://cmake.org/) for describing how to build the libraries, demonstration applications, command-line tools, unit tests, and plugins. AJA requires CMake version 3.15 or later.

The instructions for building the default static library are generally the same on each supported platform (Windows, macOS, Linux). Note that the default "CMake Generator" varies by platform.

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
4. To reconfigure the build with custom settings for certain CMake variables, additional CMake or build flags, etc. go to the Project menu and select "CMake settings for libajantv2". Make any changes to the build flags, CMake flags, variables, etc. and then save the CMakeSettings.json document. The CMake configure step should automatically be re-run upon saving.
   
   _NOTE: To manually re-run the CMake configure step with new settings, go to the Project menu and select "Configure libajantv2". If you've added libajantv2 as a sub-folder in another project you may see a different project name in the Configure menu item.
5. From the Build menu select "Build All".
6. If the build completes without errors, the static library should be output to `out\build\<arch>-<build type>\ajantv2` under the libajantv2 directory.

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

## Building the Kernel Module Driver (Linux) <a name="building-kernel-module"></a>

### Prerequisites
Before building the driver please ensure that you have installed the Linux kernel headers for your current distro:

#### Ubuntu 20.04/22.04
```
$ sudo apt install -y linux-headers-$(uname -r) linux-tools-$(uname -r)
```

#### CentOS7
```
$ sudo yum install -y kernel-devel kernel-devel-$(uname -r)
```

### Building
1. Open a terminal window and cd into the linux driver directory:
```
$ cd libajantv2/driver/linux
```
2. Run make to build the driver
```
$ make clean && make
```
3. If the kernel module build succeeded, the ajantv2.ko file should appear in `libajantv2/driver/bin`. The kernel module can now be installed via the **load_ajantv2** script from the same directory:
```
$ sudo ../bin/load_ajantv2
```

Note that on hosts with **Secure Boot** enabled, you’ll need to sign the **ajantv2.ko** kernel module after it’s been built.
Check your Linux distro’s documentation for **Secure Boot** information.

Uninstallation of the kernel module can be accomplished via the **unload_ajantv2** script:
```
$ sudo ../bin/unload_ajantv2
```


## Verifying the Kernel Module Driver (Linux) <a name="verifying-kernel-module"></a>

To confirm that the driver is loaded and running on a host that has an AJA NTV2 device installed or connected, issue
an `lsmod` command, and look for **ajantv2** in the list.
You can also issue an `ls /dev` command, and look for devices with names that start with **ajantv2**.

If `lsmod` doesn’t report the device, or it doesn’t appear in `/dev`:
-  Try disabling any/all “fast boot” options in the host BIOS.
-  Try disabling any/all power management options in the host BIOS (e.g. ASPM).
-  Be sure the AJA device shows up in `lspci -nn  -d f1d0:`.
-  Be sure the installed AJA board(s) each have two green LEDs lit after host power-on.
-  Check the `dmesg` log for error messages from the AJA NTV2 kernel driver.
-  Try installing the AJA device in a different PCIe slot on the host motherboard.


## Customizing libajantv2 <a name="customizing-ntv2"></a>
There are a number of macros that control certain aspects of NTV2:
- `NTV2_USE_CPLUSPLUS11` (in `ajantv2/includes/ajatypes.h`) — If defined (the default), assumes a C++11 compiler (or later) is being used, and C++11 language features will be used in 'ajantv2'.
Note that this macro will automatically be defined or undefined as necessary by CMake depending on the `CMAKE_CXX_STANDARD` that's in use at build-time.
Also note that if this macro is defined, so must `AJA_USE_CPLUSPLUS11` (see below) … and vice-versa.
- `AJA_USE_CPLUSPLUS11` (in `ajabase/common/types.h`) — If defined (the default), assumes a C++11 compiler (or later) is being used, and C++11 language features will be used in 'ajabase'.
Note that this macro will automatically be defined or undefined as necessary by CMake depending on the `CMAKE_CXX_STANDARD` that's in use at build-time.
Also note that if this macro is defined, so must `NTV2_USE_CPLUSPLUS11` (see above) … and vice-versa.
- `NTV2_NULL_DEVICE` (in `ajantv2/includes/ajatypes.h`) — If defined, removes all linkage to the NTV2 kernel driver. This is used, for example, to build a “sandboxed” MacOS X application with no linkage to Apple’s IOKit framework. This has the side effect of having `CNTV2DriverInterface::OpenLocalPhysical` always fail, thus permitting only remote devices to be accessed. This macro is undefined by default.
- `NTV2_NUB_CLIENT_SUPPORT` (in `ajantv2/includes/ajatypes.h`) — If defined (the default), the SDK will load plugins (DLLs, dylibs, .so’s) as necessary to connect to remote or virtual devices.
For applications requiring higher security, this macro can be undefined to prevent dynamic plugin loading.
- `NTV2_WRITEREG_PROFILING` (in `ajantv2/includes/ajatypes.h`) — If defined (the default), the `WriteRegister` profiling API in `CNTV2Card` is available.
