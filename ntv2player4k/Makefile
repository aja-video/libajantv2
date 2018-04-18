#
# Copyright (C) 2004 - 2017 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../../build/targets.mk
else
include $(DIR)/../../../../build/configure.mk

CPPFLAGS += -DAJA_LINUX

AJA_APP = $(A_UBER_BIN)/ntv2player4k

VPATH	 = $(A_DEMOAPPS_PATH)
INCLUDES = -I$(A_DEMOAPPS_PATH)

SRCS = main.cpp ntv2player4k.cpp ntv2democommon.cpp

INCS = ntv2player4k.h ntv2democommon.h

LIBS = -lajaanc -lajacc

include $(DIR)/../../../../build/common.mk

endif

