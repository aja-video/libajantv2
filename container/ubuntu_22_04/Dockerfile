# syntax=docker/dockerfile:1
FROM ubuntu:22.04
ADD . /mnt/libajantv2
RUN apt-get update \
    && apt-get install -y ubuntu-dev-tools bash bash-completion make cmake wget git ninja-build python3 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists*
