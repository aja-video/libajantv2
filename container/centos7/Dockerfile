# syntax=docker/dockerfile:1
FROM centos:7

RUN yum update -y \
	&& yum groupinstall -y "Development Tools" \
	&& yum install -y epel-release centos-release-scl \
    && yum install -y avahi-devel \
	&& yum install -y kernel-devel kernel-devel-`uname -r` \
	&& yum install -y libxcb-devel xcb-util-image xcb-util-keysyms xcb-util-wm lm_sensors-devel \
	&& yum install -y cmake3 ccache curl doxygen freeglut-devel graphviz ninja-build openssh-clients openssl-devel openssl-static wget which yasm \
	&& yum install -y libxkbcommon-devel xkeyboard-config xorg-x11-xkb-utils libX11-devel libXrender-devel libXi-devel mesa-libGL-devel mesa-libGLU-devel fontconfig-devel libuuid-devel ncurses-devel glew-devel pulseaudio-libs-devel \
	&& yum install -y ShellCheck \
	&& cd /usr/bin && ln -s cmake3 cmake && cd - \
	&& rm -rf /var/cache/yum

# COPY ./container/qt-everywhere-src-5.13.2.tar.xz /
COPY ./container/build_qt_5_13_2.sh /tmp
RUN chmod +x /tmp/build_qt_5_13_2.sh
RUN /tmp/build_qt_5_13_2.sh

# COPY ./container/cuda-repo-rhel7-12-2-local-12.2.0_535.54.03-1.x86_64.rpm /
RUN wget https://developer.download.nvidia.com/compute/cuda/12.2.0/local_installers/cuda-repo-rhel7-12-2-local-12.2.0_535.54.03-1.x86_64.rpm
rpm -i cuda-repo-rhel7-12-2-local-12.2.0_535.54.03-1.x86_64.rpm
yum clean all
yum -y install nvidia-driver-latest-dkms
yum -y install cuda
