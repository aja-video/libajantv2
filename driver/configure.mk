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

# Vars we need to set
A_ARCH      := $(shell uname -m)
LINUX_DISTRO  := GENERIC
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

A_LIB_NTV2_PATH     := $(A_LIBRARIES_PATH)/libajantv2/ajantv2
A_LIB_NTV2_INC	   	:= $(A_LIB_NTV2_PATH)/includes
A_LIB_NTV2_SRC	   	:= $(A_LIB_NTV2_PATH)/src

A_LIBCMD 		:= ar crsv
A_LIBCMD_SO		:= ar crsv

$(info $(A_LIB_NTV2_PATH))

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

# look for the magic file that says we're RedHat and infer RHELness
ifeq ($(wildcard /etc/redhat-release),/etc/redhat-release)
  IS_REDHAT := 1
  RH_REL := $(shell cat /etc/redhat-release)
  RHEL_REL := $(findstring release 4,$(RH_REL))
  ifeq (release 4,$(RHEL_REL))
    LINUX_DISTRO := RHEL4
  else
    RHEL_REL := $(findstring release 5,$(RH_REL))
    ifeq (release 5, $(RHEL_REL))
      LINUX_DISTRO := RHEL5
    else
      LINUX_DISTRO := RHEL
    endif
  endif
  X11LIBDIR := /usr/X11R6
else ifeq ($(wildcard /etc/debian_version),/etc/debian_version)
  IS_REDHAT := 0
  LINUX_DISTRO := debian
else
  X11LIBDIR    := /usr/X11
  IS_REDHAT    := 0
  LINUX_DISTRO := GENERIC
endif

# set LIB based on 64 bitness or not
ifeq (x86_64,$(A_ARCH))
# no more 32 bit Linux, so no need for a different LIB dir
  LIB := lib
else
  LIB := lib
endif

LIB_OR_LIB64 := $(LIB)
X11LIBDIR := $(X11LIBDIR)/$(LIB)
export X11LIBDIR 

