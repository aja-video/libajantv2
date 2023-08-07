# libajantv2 Dockerfiles

This directory contains Dockerfiles to build a Docker image with the dependencies required to build all of libajantv2.

The following Linux distros are currently supported:
- Ubuntu 22.04
- CentOS7

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
