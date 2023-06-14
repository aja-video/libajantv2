# syntax=docker/dockerfile:1
FROM centos:7
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles
ADD . /mnt/libajantv2
RUN yum update -y \
    && yum groupinstall  -y "Development Tools" \
    && yum install -y epel-release centos-release-scl \
    && yum install -y rh-python36 rh-git29 graphviz \
    && yum install -y wget \
    && yum install -y kernel-devel kernel-devel-`uname -r`
RUN cd /mnt/libajantv2 && cmake -S. -Bbuild -GNinja -DAJANTV2_BUILD_OPENSOURCE=ON
RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.0-rc2/cmake-3.27.0-rc2-linux-x86_64.tar.gz -o cmake.tar.gz
RUN tar xvf cmake-3.27.0-rc2-linux-x86_64.tar.gz
RUN wget https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip
RUN unzip ninja-linux.zip
