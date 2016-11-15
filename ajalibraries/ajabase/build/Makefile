#
# Copyright (C) 2004-2010 AJA Video Systems, Inc.
# Proprietary and Confidential information.  
# All Rights Reserved.
#

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include ../../targets.mk
else

AJA_API_ROOT      ?= ../../..
AJA_AJABASE_ROOT = $(AJA_API_ROOT)/ajabase

# yes this is redundant
ifeq ($(AJA_DEBUG),1)
  AJA_AJABASE_LIBNAME = ajabased
else
  AJA_AJABASE_LIBNAME = ajabase
endif

LIBNAME = lib$(AJA_AJABASE_LIBNAME)
LIBTARGET = $(LIBNAME)

VPATH    = $(AJA_AJABASE_ROOT)/common:$(AJA_AJABASE_ROOT)/system:$(AJA_AJABASE_ROOT)/system/linux:$(AJA_AJABASE_ROOT)/pnp:$(AJA_AJABASE_ROOT)/pnp/linux:$(AJA_AJABASE_ROOT)/persistence
INCLUDES = -I$(AJA_API_ROOT)

AJA_APP_SRCS =\
	atomic.cpp \
	audioutilities.cpp \
	buffer.cpp \
	cli_utils.cpp \
	debug.cpp \
	dpx_hdr.cpp \
	dpxfileio.cpp \
	event.cpp \
	eventimpl.cpp \
	file_io.cpp \
	guid.cpp \
	ip_socket.cpp \
	lock.cpp \
	lockimpl.cpp \
	memory.cpp \
	options_popt.cpp \
	performance.cpp \
	persistence.cpp \
	pixelformat.cpp \
	pnp.cpp \
	pnpimpl.cpp \
	process.cpp \
	processimpl.cpp \
	systemtime.cpp \
	testpatterngen.cpp \
	thread.cpp \
	threadimpl.cpp \
	timebase.cpp \
	timecode.cpp \
	timecodeburn.cpp \
	timer.cpp \
	udp_socket.cpp \
	videoutilities.cpp \
	wavewriter.cpp

C_AJA_APP_SRCS =\
	sqlite3.c

default: $(LIBNAME).a $(LIBNAME).so copy-a copy-so 

include ../../../common.mk

subclean:
	rm -f $(LIBTARGET).a $(LIBTARGET).so *.o 
	rm -f $(AJA_API_LIBDIR)/$(LIBNAME).a
	rm -f $(AJA_API_LIBDIR)/$(LIBNAME).so

endif

