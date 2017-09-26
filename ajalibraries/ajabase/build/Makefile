#
# Copyright (C) 2004 - 2017 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../build/targets.mk
else
include $(DIR)/../../../build/configure.mk

ifeq ($(AJA_DEBUG),1)
	AJA_LIB = ajabased
else
	AJA_LIB = ajabase
endif
AJA_LIB_PATH = $(A_UBER_LIB)/lib$(AJA_LIB).a

VPATH    = $(A_LIB_BASE_PATH)/common:$(A_LIB_BASE_PATH)/system:$(A_LIB_BASE_PATH)/system/linux:$(A_LIB_BASE_PATH)/pnp:$(A_LIB_BASE_PATH)/pnp/linux:$(A_LIB_BASE_PATH)/persistence:$(A_LIB_BASE_PATH)/network

CPPFLAGS += -DAJA_LINUX

AJA_LIB_SRCS =\
	atomic.cpp \
	audioutilities.cpp \
	buffer.cpp \
	common.cpp \
	debug.cpp \
	dpx_hdr.cpp \
	dpxfileio.cpp \
	event.cpp \
	eventimpl.cpp \
	file_io.cpp \
	guid.cpp \
	info.cpp \
	infoimpl.cpp \
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
    system.cpp \
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

AJA_LIB_C_SRCS =\
	sqlite3.c

include $(DIR)/../../../build/common.mk

endif

