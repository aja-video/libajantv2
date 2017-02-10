#
# Copyright (C) 2004, 2005, 2006, 2007, 2008 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
# Here's the idea behind this makefile approach
# http://make.paulandlesley.org/multi-arch.html

# a makefile to do all the sniffing, configuring and path setting for
# the assorted flavors of distros and bitedness

# Vars we need to set
CPU_ARCH      := $(shell uname -m)
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
  NTV2_LIBDIR := $(NTV2_ROOT)/../lib
else
  OBJ := $(OBJ)
  DBG := 
  NTV2_LIBDIR := $(NTV2_ROOT)/../lib
endif

ifeq ($(NTV2_DEPRECATE),1)
  DBG += -DNTV2_DEPRECATE
endif

OBJDIR := $(OBJ)

DRIVER_DIR = driver/linuxdriver

NTV2TARGET ?= ajantv2

NTV2_INCLUDES	   	:= $(NTV2_ROOT)/includes
NTV2_CLASSES       	:= $(NTV2_ROOT)/classes
NTV2_LINUX_CLASSES 	:= $(NTV2_ROOT)/linuxclasses
NTV2_BINDIR        	:= $(NTV2_ROOT)/../bin
AJA_CLASSES		   	:= $(NTV2_ROOT)/ajaclasses
AJA_ASDCP_DIR		:= $(NTV2_ROOT)/asdcplib
AJA_ASDCP_LIBDIR	:= $(NTV2_ROOT)/asdcplib/Linux
FLTKDIR 			:= $(NTV2_ROOT)/fltk
FLTKLIBDIR 			:= $(FLTKDIR)/lib


export AJA_BETA
export AJA_DEBUG
export DBG
export AJA_CROSS_COMPILE
export OBJDIR
export DRIVER_DIR
export NTV2_INCLUDES
export NTV2_CLASSES
export NTV2_LINUX_CLASSES
export NTV2_LIBDIR
export NTV2_BINDIR
export AJA_CLASSES
export AJA_ASDCP_DIR
export AJA_ASDCP_LIBDIR
export FLTKDIR
export FLTKLIBDIR
export CPU_ARCH

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
else
  X11LIBDIR    := /usr/X11
  IS_REDHAT    := 0
  LINUX_DISTRO := GENERIC
endif

# set LIB based on 64 bitness or not
ifeq (x86_64,$(CPU_ARCH))
# no more 32 bit Linux, so no need for a different LIB dir
  LIB := lib
else
  LIB := lib
endif

LIB_OR_LIB64 := $(LIB)
X11LIBDIR := $(X11LIBDIR)/$(LIB)
export X11LIBDIR 

# setup the QT Dirs
ifeq ($(QTDIR),)
    QTDIR := $(dir $(lastword $(shell qmake -v 2>&1)))
    ifeq ($(QTDIR),)
        $(warning QTDIR environment variable not set, skipping applications that require Qt. Try installing the appropriate Qt package.)
    endif
endif
ifneq ($(QTDIR),)
  ifeq ($(QTBIN),)
    QTBIN := $(QTDIR)/bin
  endif
  ifeq ($(QTLIB),)
    QTLIB := $(QTDIR)/lib
  endif
  ifeq ($(QTLIBDIR),)
    QTLIBDIR := $(QTDIR)/lib
  endif
endif	

#.PHONY : setup_vars
#setup_vars: 
#	@echo "Building for LINUX_DISTRO $(LINUX_DISTRO)"
#	@echo "             CPU          $(CPU_ARCH)"
#	@echo "             LIB          $(LIB)"
#	@echo "             X11LIBDIR    $(X11LIBDIR)"
#	@echo "             QTLIBDIR     $(QTLIBDIR)"
#	@echo "             FLTKLIBDIR   $(FLTKLIBDIR)"
#	export LINUX_DISTRO CPU LIB X11LIBDIR QTLIBDIR FLTKLIBDIR


#.PHONY: get_dist
#get_dist:
#   @echo "IS_REDHAT = $(IS_REDHAT)"
#   @echo "RH_REL    = $(RH_REL)"
#   @echo "RHEL      = $(RHEL)"
#   @echo "RHEL_REL  = $(RHEL_REL)"

#.PHONY: get_qt_libdir
#get_qt_libdir:


# get_x11_libdir:

