# Dockerfile for creating an image capable of building libajantv2 for CentOS7 x64

# syntax=docker/dockerfile:1
FROM centos:7
ARG AJA_DRIVER_KVERSION=3.10.0-862.11.6.el7.x86_64
RUN yum update -y \
	&& yum groupinstall -y "Development Tools" \
	&& yum install -y epel-release centos-release-scl \
	&& yum install -y kernel-devel kernel-devel-$AJA_DRIVER_KVERSION \
	&& yum install -y avahi-devel cmake3 ccache curl doxygen freeglut-devel fontconfig-devel glew-devel graphviz \
	&& yum install -y libuuid-devel libxcb-devel libX11-devel libXrender-devel libXi-devel libxkbcommon-devel lm_sensors-devel \
	&& yum install -y mesa-libGL-devel mesa-libGLU-devel ncurses-devel ninja-build openssh-clients openssl-devel openssl-static patchelf pulseaudio-libs-devel \
	&& yum install -y wget which xcb-util-image xcb-util-keysyms xcb-util-wm xkeyboard-config xorg-x11-xkb-utils yasm \
	&& yum install -y ShellCheck \
	&& cd /usr/bin && ln -s cmake3 cmake && cd - \
	&& rm -rf /var/cache/yum

# Uncomment below to upload a local copy of the .tar.xz file to the image, for debugging.
# COPY ./container/qt-everywhere-src-5.13.2.tar.xz /
COPY ./container/build_qt_5_13_2.sh /tmp
RUN chmod +x /tmp/build_qt_5_13_2.sh
RUN /tmp/build_qt_5_13_2.sh

# Uncomment the below "COPY" step and comment out the following "RUN" step to upload a local copy of the .deb file to the image, for debugging.
# COPY ./container/cuda-repo-rhel7-12-2-local-12.2.0_535.54.03-1.x86_64.rpm /
RUN wget https://developer.download.nvidia.com/compute/cuda/12.2.0/local_installers/cuda-repo-rhel7-12-2-local-12.2.0_535.54.03-1.x86_64.rpm
RUN rpm -i cuda-repo-rhel7-12-2-local-12.2.0_535.54.03-1.x86_64.rpm
RUN yum clean all
RUN yum -y install nvidia-driver-latest-dkms
RUN yum -y install cuda
