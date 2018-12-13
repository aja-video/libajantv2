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

AJA_APP = $(A_UBER_BIN)/ntv2llburn

VPATH	 = $(A_DEMOAPPS_PATH)
INCLUDES = -I$(A_DEMOAPPS_PATH)

SRCS = main.cpp ntv2llburn.cpp ntv2democommon.cpp

INCS = ntv2llburn.h ntv2democommon.h

include $(DIR)/../../../../build/common.mk

endif

