# Dockerfile for creating an image capable of building libajantv2 for Ubuntu 22.04 x64

# syntax=docker/dockerfile:1
FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive
ARG AJA_DRIVER_KVERSION=5.15.0-73
ENV TZ=America/Los_Angeles

RUN apt-get update \
    && apt-get install -y bash bash-completion make ccache cmake wget ninja-build python3 \
       linux-headers-$AJA_DRIVER_KVERSION linux-tools-$AJA_DRIVER_KVERSION linux-tools-generic \
       llvm libclang-dev clang-11 \
       libfontconfig1-dev libfreetype6-dev libx11-dev libx11-xcb-dev libxext-dev \
       libxfixes-dev libxi-dev libxrender-dev libxcb1-dev libxcb-glx0-dev \
       libxcb-keysyms1-dev libxcb-cursor-dev libxcb-image0-dev libxcb-shm0-dev libxcb-icccm4-dev \
       libxcb-sync0-dev libxcb-xfixes0-dev libxcb-shape0-dev libxcb-randr0-dev \
       libxcb-render-util0-dev libxcb-xinerama0-dev libxcb-xinput-dev libxkbcommon-dev \
       libxkbcommon-x11-dev libasound2-dev libpulse-dev libgtk-3-dev libsdl2-dev \
       libudev-dev libinput-dev libwayland-egl-backend-dev libgbm-dev libcups2-dev \
       libcupsimage2-dev unixodbc-dev libssl-dev \
       vim-tiny tzdata \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists*

# Uncomment below to upload a local copy of the .tar.xz file to the image, for debugging.
# COPY ./container/qt-everywhere-opensource-src-5.15.8.tar.xz /
COPY ./container/build_qt_5_15_8.sh /tmp
RUN chmod +x /tmp/build_qt_5_15_8.sh
RUN /bin/sh -c "/tmp/build_qt_5_15_8.sh"

# Uncomment the below "COPY" step and comment out the following "RUN" step to upload a local copy of the .deb file to the image, for debugging.
# COPY ./container/cuda-repo-ubuntu2204-12-2-local_12.2.0-535.54.03-1_amd64.deb /
RUN wget https://developer.download.nvidia.com/compute/cuda/12.2.0/local_installers/cuda-repo-ubuntu2204-12-2-local_12.2.0-535.54.03-1_amd64.deb
RUN wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-ubuntu2204.pin
RUN mv cuda-ubuntu2204.pin /etc/apt/preferences.d/cuda-repository-pin-600
RUN dpkg -i cuda-repo-ubuntu2204-12-2-local_12.2.0-535.54.03-1_amd64.deb
RUN cp /var/cuda-repo-ubuntu2204-12-2-local/cuda-*-keyring.gpg /usr/share/keyrings/
RUN apt-get update
RUN apt-get -y install cuda
