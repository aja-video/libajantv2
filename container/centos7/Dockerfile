# syntax=docker/dockerfile:1
FROM centos:7
RUN yum update -y \
	&& yum groupinstall -y "Development Tools" \
	&& yum install -y epel-release centos-release-scl \
	&& yum update -y \
	&& yum install -y cmake3 ccache curl wget python3 rh-git29 yasm ninja-build \
	&& cd /usr/bin && ln -s cmake3 cmake && cd - \
	&& rm -rf /var/cache/yum
