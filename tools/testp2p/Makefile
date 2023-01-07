#
# Copyright (C) 2004, 2005, 2006, 2007, 2008 AJA Video Systems, Inc.
# Proprietary and Confidential information.  All Rights Reserved
#
# You no longer need to define NTV2TARGET as either KSD, KHD or XENA2

DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../build/targets.mk
else
include $(DIR)/../../../build/configure.mk

AJA_APP = $(A_UBER_BIN)/testp2p

VPATH	 = $(A_DEMOAPPS_PATH)
INCLUDES = -I$(A_DEMOAPPS_PATH)

SRCS = testp2p.cpp

include $(DIR)/../../../build/common.mk

endif
