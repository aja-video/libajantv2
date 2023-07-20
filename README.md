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

## Build Instructions
Starting in SDK 17.0, AJA has standardized on [CMake](https://cmake.org/) for describing how to build the libraries, demonstration applications, command-line tools, unit tests, and plugins.

AJA recommends using CMake version 3.15 or later.

#### CMake Build Parameters
By default — absent any parameters — only the static NTV2 library gets built.\
To build additional items, use these parameters:
- **AJANTV2_BUILD_DEMOS** — If defined, builds the demonstration programs (e.g. **ntv2capture**, **ntv2player**, …). By default, demo apps are _not_ built.
- **AJANTV2_BUILD_TOOLS** — If defined, builds the command-line tools (e.g. **ntv2thermo**, **regio**, **supportlog**, …). By default, tools are _not_ built.
- **AJANTV2_BUILD_TESTS** — If defined, builds any/all unit test(s). By default, these test programs are _not_ built.
- **AJANTV2_BUILD_PLUGINS** — If defined, builds the standard NTV2 plugins (i.e. **nublegacy**, **swdevice**, …). By default, plugins are _not_ built.

#### Examples

###### Linux: Build the Static Library
To build only the static library from the command line:
1. Open a terminal window, and enter the **libajantv2** folder:\
`$ cd libajantv2`
2. Generate the build configuration:\
`$ cmake -S .  -B _bld`
3. Run the build:\
`$ cmake --build _bld`
4. Look for the finished **libajantv2.a** library inside **_bld/ajantv2**.

###### MacOS: Build SDK & Demos Using Xcode
To generate the Xcode project file, and then build from it:
1. From the Terminal, enter the **libajantv2** folder:\
`$ cd libajantv2`
2. Generate the Xcode project:\
`$ cmake -S .  -B _bld`
3. Either open the project in **Xcode**:\
`$ open _bld/libajantv2.xcodeproj`\
…or build directly from the command line:\
`$ xcrun xcodebuild -project _bld/libajantv2.xcodeproj -target ALL_BUILD`
4. Look for the finished executables inside the **_bld** folder.\
For example, to run **ntv2enumerateboards**:\
`$ _bld/demos/ntv2enumerateboards/Debug/ntv2enumerateboards`

By default, **Xcode** will produce **Debug** artifacts.\
To build **Release** artifacts, add `-configuration Release` to the **xcodebuild** command line.

###### Docker:
Using the Ubuntu 22.04 Dockerfile as an example, here is how to build libajantv2 from a Docker Container, and copy the build artifacts back to the Docker host system.

1. Build Ubuntu 22.04 Docker Image:
```
$ docker build -t libajantv2-ubuntu-22.04:latest -f container/ubuntu_22_04/Dockerfile .
```
2. Run Ubuntu 22.04 Docker Container in interactive mode:
```
$ docker run -i libajantv2-ubuntu-22.04:latest
```
3. Shell in to Container and build **libajantv2**:
```
$ docker exec -it <container-id> bash
# cd /mnt/libajantv2
# cmake -S. -Bbuild -GNinja -DAJANTV2_BUILD_OPENSOURCE=ON
# cmake --build build
# exit
```
4. Copy build artifacts out of container to host
```
$ docker cp <container-id>:/mnt/libajantv2/build libajantv2-build
```
