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

AJA_APP = $(A_UBER_BIN)/ntv2encodehevc

VPATH	 = $(A_DEMOAPPS_PATH)
INCLUDES = -I$(A_DEMOAPPS_PATH) -I$(A_LIB_NTV2_M31_PATH)

SRCS = main.cpp ntv2encodehevc.cpp ntv2demohevccommon.cpp

INCS = ntv2encodehevc.h ntv2demohevccommon.h

include $(DIR)/../../../../build/common.mk

endif

