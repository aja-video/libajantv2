#
# Copyright (C) 2004 - 2017 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../../targets.mk
else
include $(DIR)/../../../../configure.mk

CPPFLAGS += -DAJA_LINUX

NTV2_APP = $(A_UBER_BIN)/ntv2burn

VPATH	 = $(A_DEMOAPPS_PATH)
INCLUDES = -I$(A_DEMOAPPS_PATH)

SRCS = main.cpp ntv2burn.cpp ntv2democommon.cpp

INCS = ntv2burn.h ntv2democommon.h

include $(DIR)/../../../../common.mk

endif

