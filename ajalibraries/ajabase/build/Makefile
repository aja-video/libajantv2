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

# yes this is redundant
ifeq ($(AJA_DEBUG),1)
  AJA_AJABASE_LIBNAME = ajabased
else
  AJA_AJABASE_LIBNAME = ajabase
endif

LIBNAME = lib$(AJA_AJABASE_LIBNAME)
LIBTARGET = $(LIBNAME)

VPATH    = $(A_LIB_BASE_PATH)/common:$(A_LIB_BASE_PATH)/system:$(A_LIB_BASE_PATH)/system/linux:$(A_LIB_BASE_PATH)/pnp:$(A_LIB_BASE_PATH)/pnp/linux:$(A_LIB_BASE_PATH)/persistence
INCLUDES = -I$(A_LIBRARIES_PATH)

CPP =   g++
CPPFLAGS += -DAJALinux -DAJA_LINUX -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-pedantic -Wall -Wno-long-long  -Wwrite-strings -c -pipe -fPIC $(DBG)

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


$(A_UBER_LIB)/$(LIBNAME).a: $(OBJS)
	+@[ -d $(A_UBER_LIB) ] || mkdir -p $(A_UBER_LIB)
	$(A_LIBCMD) $@ $(OBJS)

all: $(A_UBER_LIB)/$(LIBNAME).a

.PHONY: clean subclean cleandeps
clean:
	rm -f *.o
	rm -f $(A_UBER_LIB)/$(LIBNAME).a

cleandeps:
	rm -f *.d

subclean:
	rm -f $(LIBTARGET).a $(LIBTARGET).so *.o 
	rm -f $(A_UBER_LIB)/$(LIBNAME).a
	rm -f $(A_UBER_LIB)/$(LIBNAME).so

endif

