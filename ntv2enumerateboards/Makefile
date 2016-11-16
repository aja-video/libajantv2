#
# Copyright (C) 2004, 2005, 2006, 2007, 2008, 2012 AJA Video Systems, Inc.
# Proprietary and Confidential information.  All Rights Reserved
#

NTV2_ROOT	:= $(shell pwd)/../..

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(NTV2_ROOT)/targets.mk
else

NTV2_APP   = $(NTV2_BINDIR)/ntv2enumerateboards

VPATH    := $(NTV2_ROOT)/../democlasses

INCLUDES = -I.. -I$(NTV2_INCLUDES) -I$(NTV2_CLASSES) -I$(NTV2_ROOT)/../democlasses

LIBS = -laja
LIBDIRS = -L$(NTV2_LIBDIR)

# Please keep the list of source files alphabetical
SRCS =	main.cpp ntv2enumerateboards.cpp

CPPFLAGS += -DAJALinux -DHAVE_PTHREAD_H -DNO_WGLIB

default: $(NTV2_APP)

QTINC :=
QTLIB :=
QTL :=

include $(NTV2_ROOT)/../common.mk

endif

