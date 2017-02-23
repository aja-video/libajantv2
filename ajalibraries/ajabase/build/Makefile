#
# Copyright (C) 2004-2010 AJA Video Systems, Inc.
# Proprietary and Confidential information.  
# All Rights Reserved.
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  
  include $(DIR)/../../../targets.mk

else
include $(DIR)/../../../configure.mk

AJA_AJABASE_ROOT = $(AJALIB_DIR)/ajabase

# yes this is redundant
ifeq ($(AJA_DEBUG),1)
  AJA_AJABASE_LIBNAME = ajabased
else
  AJA_AJABASE_LIBNAME = ajabase
endif

LIBNAME = lib$(AJA_AJABASE_LIBNAME)
LIBTARGET = $(LIBNAME)

VPATH    = $(AJA_AJABASE_ROOT)/common:$(AJA_AJABASE_ROOT)/system:$(AJA_AJABASE_ROOT)/system/linux:$(AJA_AJABASE_ROOT)/pnp:$(AJA_AJABASE_ROOT)/pnp/linux:$(AJA_AJABASE_ROOT)/persistence
INCLUDES = -I$(AJALIB_DIR)

CPP =   g++
CPPFLAGS += -DAJALinux -DAJA_LINUX -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-pedantic -Wall -Wno-long-long  -Wwrite-strings -c -pipe -fPIC $(DBG)


LIBCMD 		:= ar crsv
LIBCMD_SO	:= ar crsv

SRCS =\
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

C_SRCS =\
	sqlite3.c

OBJS = $(patsubst %.cpp,%.o,$(SRCS))
OBJS += $(patsubst %.c,%.o,$(C_SRCS))

%.d: %.cpp
	$(CPP) -M $(CPPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $@ $<

%.d: %.c
	$(CC) -M $(CFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<


$(NTV2_LIB_DIR)/$(LIBNAME).a: $(OBJS)
	+@[ -d $(NTV2_LIB_DIR) ] || mkdir -p $(NTV2_LIB_DIR)
	$(LIBCMD) $@ $(OBJS)

all: $(NTV2_LIB_DIR)/$(LIBNAME).a 

.PHONY: clean subclean cleandeps
clean:
	rm -f *.o
	rm -f $(NTV2_LIB_DIR)/$(LIBNAME).a

cleandeps:
	rm -f *.d

subclean:
	rm -f $(LIBTARGET).a $(LIBTARGET).so *.o 
	rm -f $(AJA_API_LIBDIR)/$(LIBNAME).a
	rm -f $(AJA_API_LIBDIR)/$(LIBNAME).so

endif

