# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
#

DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../../build/targets.mk
else
include $(DIR)/../../../../build/configure.mk

AJA_APP = $(A_UBER_BIN)/ntv2burn

VPATH	 = $(A_DEMOAPPS_PATH)
INCLUDES = -I$(A_DEMOAPPS_PATH)

SRCS = main.cpp ntv2burn.cpp ntv2democommon.cpp

INCS = ntv2burn.h ntv2democommon.h

include $(DIR)/../../../../build/common.mk

endif

