# syntax=docker/dockerfile:1
FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles

RUN apt-get update \
    && apt-get install -y bash bash-completion make ccache cmake wget ninja-build python3 \
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

COPY ./container/build_qt_5_15_8.sh /tmp
RUN chmod +x /tmp/build_qt_5_15_8.sh
RUN /bin/sh -c "/tmp/build_qt_5_15_8.sh"
