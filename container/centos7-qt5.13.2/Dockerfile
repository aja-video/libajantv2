# syntax=docker/dockerfile:1
FROM centos:7

RUN yum update -y \
	&& yum groupinstall -y "Development Tools" \
	&& yum install -y epel-release centos-release-scl \
	&& yum install -y kernel-devel kernel-devel-`uname -r` \
	&& yum install -y libxcb-devel xcb-util-image xcb-util-keysyms xcb-util-wm lm_sensors-devel \
	&& yum install -y cmake3 ccache curl doxygen graphviz ninja-build openssh-clients openssl-devel openssl-static wget which yasm \
	&& yum install -y libxkbcommon-devel xkeyboard-config xorg-x11-xkb-utils libX11-devel libXrender-devel libXi-devel mesa-libGL-devel mesa-libGLU-devel fontconfig-devel libuuid-devel ncurses-devel glew-devel pulseaudio-libs-devel \
	&& yum install -y ShellCheck \
	&& rm -rf /var/cache/yum

COPY ./container/build_qt_5_13_2.sh /tmp
RUN chmod +x /tmp/build_qt_5_13_2.sh
RUN /tmp/build_qt_5_13_2.sh
