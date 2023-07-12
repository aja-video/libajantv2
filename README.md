# libajantv2
test
## Description

**libajantv2** is an SDK and C++ class library for discovering, interrogating and controlling AJA NTV2 professional video I/O devices.

The NTV2 SDK has the following components:
- `ajabase` - Platform-independent classes for handling threads, mutexes, files, etc.
- `ajaanc` - Ancillary data classes, to aid encoding & decoding of SDI ancillary data packets.
- `ajantv2` - The principal classes, particularly **CNTV2Card**.
- `docs` - Additional documentation for **libajantv2**.
- `plugins` - Plugins that add additional functionality to the NTV2 SDK.

## Docker Builds
Using the Ubuntu 22.04 Dockerfile as an example, here is how to build libajantv2 from a Docker Container, and copy the build artifacts back to the Docker host system.

1. Build Ubuntu 22.04 Docker Image
```
$ docker build -t libajantv2-ubuntu-22.04:latest -f container/ubuntu_22_04/Dockerfile .
```
2. Run Ubuntu 22.04 Docker Container in interactive mode
```
$ docker run -i libajantv2-ubuntu-22.04:latest
```
3. Shell in to Container and build libajantv2
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