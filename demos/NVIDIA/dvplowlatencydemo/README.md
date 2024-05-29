The AJA "dvplowlatency" demo requires the "NVIDIA GPUDirect for Video SDK".
Sign up for a free NVIDIA developer account and download the SDK here:

https://developer.nvidia.com/gpudirectforvideo

Download the SDK and copy to libajantv2/thirdparty/NVIDIA.

The directory structure should look like this:

libajantv2/thirdparty/NVIDIA/gpudirect
|- bin
  |- win32
    |- dvp.dll
  |- x64
    |- dvp.dll
|- docs
|- include
|- lib
  |- linux
    |- lib64
      |- libdvp.a
      |- libdvp.so
  |- Win32
    |- dvp_static.lib
    |- dvp.lib
  |- x64
    |- dvp_static.lib
    |- dvp.lib

NOTE: The GPUDirect for Video SDK only supports Windows and Linux x86 and x64.
