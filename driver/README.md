# The NTV2 Device Driver
The AJA NTV2 device driver runs at the kernel level and handles low-level communication with the device.
It is a required component of the SDK and provides the user-space library with the means to interrogate and/or control the device.

The driverʼs primary job is to facilitate reading and writing device hardware registers, initiate and complete high-speed DMA data
transfers, and respond to interrupts.

## Directory Organization
The **driver** folder contains the following items:
- A number of source files used commonly by all platform drivers.
- CMakeLists.txt — CMake recipe that can build the Linux driver.
- **bin** — Folder containing command-shell scripts for manually loading and unloading the Linux driver.
- **linux** — Folder containing source code for the Linux driver.
  - Makefile — The Linux driver can still be built using ‘makeʼ.
- **peta** — Folder containing source code for the Peta-Linux driver for using NTV2 inside embedded devices.

## Building the Driver

Note that only the Linux and PetaLinux drivers are open-source.
The Windows and MacOS kernel drivers are not currently open-source.


#### Linux: Build with CMake
To build the driver using CMake:
1. Open a terminal window, and enter the **libajantv2/driver** folder:\
`$ cd libajantv2/driver`
2. Generate the build configuration:\
`$ cmake -S .  -B build`
3. Run the build:\
`$ cmake --build build`
4. Look for the finished **ajantv2.ko** kernel object inside **build**.

#### Linux: Build with ‘makeʼ
To build the driver using ‘makeʼ:
1. Open a terminal window, and enter the **libajantv2/driver/linux** folder:\
`$ cd libajantv2/driver/linux`
2. Run the build:\
`$ make`
3. Look for the finished **ajantv2.ko** kernel object inside **libajantv2/bin**.


## Installing the Driver

#### Linux:
To install the driver:
1. Open a terminal window, and enter the **libajantv2/driver** folder:\
`$ cd libajantv2/driver`
2. Generate the build configuration:\
`$ ./bin/load_ajantv2  path/to/ajantv2.ko`
