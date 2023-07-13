# syntax=docker/dockerfile:1
FROM centos:7

RUN yum update -y \
	&& yum groupinstall -y "Development Tools" \
	&& yum install -y epel-release centos-release-scl \
	&& yum install -y xcb-util-image xcb-util-keysyms xcb-util-wm lm_sensors-devel \
	&& yum install -y rh-git29 ninja-build yasm doxygen graphviz \
	&& yum install -y cmake3 ccache curl wget openssh-clients openssl-devel openssl-static \
	&& yum install -y xkeyboard-config xorg-x11-xkb-utils libX11-devel libxcb-devel libXrender-devel libXi-devel mesa-libGL-devel mesa-libGLU-devel fontconfig-devel libuuid-devel ncurses-devel glew-devel pulseaudio-libs-devel kernel-devel kernel-devel-`uname -r` \
	&& yum install -y ShellCheck \
	&& cd /usr/bin && ln -s cmake3 cmake && cd - \
	&& rm -rf /var/cache/yum
