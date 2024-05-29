The AJA "dvplowlatency" demo requires the "NVIDIA GPUDirect for Video SDK".
Sign up for a free NVIDIA developer account and download the SDK here:

https://developer.nvidia.com/gpudirectforvideo

Download the SDK and install.

- On Windows the SDK will install to "C:\\Program Files (x86)\\NVIDIA Corporation\\NVIDIA GPU Direct for Video SDK". The dvplowlatency demo can be built using the SDK from that location, or the `win` directory can be copied from that directory into the default search path: `<libajantv2-dir>/thirdparty/nvidia/gpudirect`.

- On Linux the SDK is extracted from a .run file in the same directory as the .run file. Copy the `linux` directory from the extracted `dvp2/sdk` dir into the default search path: `<libajantv2-dir>/thirdparty/nvidia/gpudirect`.

Set the CMake variable `NVIDIA_GPUDIRECT_DIR` to override the default search path (`<libajantv2-dir>/thirdparty/nvidia/gpudirect`).

NOTE: The GPUDirect for Video SDK only supports Windows and Linux x86 and x64.
