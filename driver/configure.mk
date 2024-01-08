# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2023 AJA Video Systems, Inc.
#

# Here's the idea behind this makefile approach
# http://make.paulandlesley.org/multi-arch.html
#
#
# a makefile to do all the sniffing, configuring and path setting for
# the assorted flavors of distros and bitedness

# Allow specifying kernel version manually so it is possible to build in a containerized environment.
ifdef AJA_DRIVER_KVERSION
	KVERSION = $(AJA_DRIVER_KVERSION)
else
	KVERSION ?= $(shell uname -r)
endif
KDIR		?= /lib/modules/$(KVERSION)/build

# Vars we need to set
A_ARCH      := $(shell uname -m)
AJA_BETA 		  ?= 0
AJA_DEBUG 		  ?= 0
AJA_CROSS_COMPILE ?= 0

ifeq ($(AJA_CROSS_COMPILE),1)
  OBJ := _$(ARCH)
else
  OBJ := _host
endif

ifeq ($(AJA_DEBUG),1)
  OBJ := $(OBJ)-DEBUG
  DBG := -ggdb
# Use same directory with debug lib names that end in "d"
else
  OBJ := $(OBJ)
  DBG := 
endif

OBJDIR := $(OBJ)

NTV2TARGET ?= ajantv2

# some common paths to use in all the makefiles
DIR_HERE 			:= $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))
A_ROOT              := $(abspath $(DIR_HERE)/../)
A_UBER_BIN          := $(A_ROOT)/bin
A_UBER_LIB          := $(A_ROOT)/lib
A_LIBRARIES_PATH    := $(A_ROOT)

A_DRIVER_PATH       := $(DIR_HERE)
A_LINUX_DRIVER_PATH := $(DIR_HERE)/linux
A_PETA_DRIVER_PATH := $(DIR_HERE)/peta

A_LIB_NTV2_PATH     := $(A_LIBRARIES_PATH)/ajantv2
A_LIB_NTV2_INC	   	:= $(A_LIB_NTV2_PATH)/includes
A_LIB_NTV2_SRC	   	:= $(A_LIB_NTV2_PATH)/src

A_LIBCMD 		:= ar crsv
A_LIBCMD_SO		:= ar crsv

# helper functions
# these can be invoked in make rules like so:
# $(call ensure_dir_exists, somepath/value)

# create the passed in dir (and parents if needed) if they do not already exist
ensure_dir_exists = @if [ ! -d $1 ]; then mkdir -p $1; fi

# if the passed dir exists run the makefile in that dir
make_if_dir_exists = @if [ -d $1 ]; then $(MAKE) -C $1 ;fi

# if the passed dir exists run the makefile in that dir with the clean rule
clean_if_dir_exists = @if [ -d $1 ]; then $(MAKE) -C $1 clean ;fi

export A_ARCH
export AJA_BETA
export AJA_CROSS_COMPILE
export AJA_DEBUG
export A_LIB_NTV2_INC
export A_LIB_NTV2_PATH
export A_LIB_NTV2_SRC
export A_LIBRARIES_PATH
export A_LINUX_DRIVER_PATH
export A_ROOT
export A_UBER_BIN
export A_UBER_LIB
export DBG
export A_LIBCMD
export A_LIBCMD_SO
export OBJDIR
export KVERSION
export KDIR

# Figure out info about the distro, specifically if a RHEL like: (CentOS, Rocky, Alma, RHEL, etc)
ifeq ($(wildcard /etc/os-release),/etc/os-release)
	DISTRO_TYPE := $(shell awk 'BEGIN {FS = "="} $$1 == "ID" {gsub("\"",""); print $$2}' /etc/os-release)
	DISTRO_IS_RHEL_LIKE := $(shell awk 'BEGIN {FS = "="} $$1 == "ID_LIKE" && $$2 ~ /rhel/ {print 1}' /etc/os-release)
	DISTRO_MAJ_VERSION := $(shell awk 'BEGIN {FS = "="} $$1 == "VERSION_ID" {split($$2, subfield, "."); gsub("\"","",subfield[1]); printf("%d", subfield[1])}' /etc/os-release)
	DISTRO_MIN_VERSION := $(shell awk 'BEGIN {FS = "="} $$1 == "VERSION_ID" {split($$2, subfield, "."); gsub("\"","",subfield[2]); printf("%d", subfield[2])}' /etc/os-release)
endif

DISTRO_KERNEL_PKG_MAJ := $(shell echo $(KVERSION) | awk 'BEGIN {FS = "-"} {split($$2, pkg, "."); printf("%d", pkg[1])}')
DISTRO_KERNEL_PKG_MIN := $(shell echo $(KVERSION) | awk 'BEGIN {FS = "-"} {split($$2, pkg, "."); printf("%d", pkg[2])}')
DISTRO_KERNEL_PKG_PNT := $(shell echo $(KVERSION) | awk 'BEGIN {FS = "-"} {split($$2, pkg, "."); printf("%d", pkg[3])}')

# set distro defaults if needed
ifeq ($(DISTRO_TYPE),)
	DISTRO_TYPE := generic
endif
ifeq ($(DISTRO_TYPE),rhel)
	DISTRO_IS_RHEL_LIKE := 1
endif
ifeq ($(DISTRO_IS_RHEL_LIKE),)
	DISTRO_IS_RHEL_LIKE := 0
endif
ifeq ($(DISTRO_MAJ_VERSION),)
	DISTRO_MAJ_VERSION := 0
endif
ifeq ($(DISTRO_MIN_VERSION),)
	DISTRO_MIN_VERSION := 0
endif
ifeq ($(DISTRO_KERNEL_PKG_MAJ),)
	DISTRO_KERNEL_PKG_MAJ := 0
endif
ifeq ($(DISTRO_KERNEL_PKG_MIN),)
	DISTRO_KERNEL_PKG_MIN := 0
endif
ifeq ($(DISTRO_KERNEL_PKG_PNT),)
	DISTRO_KERNEL_PKG_PNT := 0
endif

export DISTRO_TYPE
export DISTRO_IS_RHEL_LIKE
export DISTRO_MAJ_VERSION
export DISTRO_MIN_VERSION
export DISTRO_KERNEL_PKG_MAJ
export DISTRO_KERNEL_PKG_MIN
export DISTRO_KERNEL_PKG_PNT

# set LIB based on 64 bitness or not
ifeq (x86_64,$(A_ARCH))
# no more 32 bit Linux, so no need for a different LIB dir
  LIB := lib
else
  LIB := lib
endif

LIB_OR_LIB64 := $(LIB)

