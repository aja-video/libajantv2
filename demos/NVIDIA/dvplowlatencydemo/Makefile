# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
#

DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../../../build/targets.mk
else
include $(DIR)/../../../../../build/configure.mk

AJA_APP = $(A_UBER_BIN)/dvplowlatencydemo

VPATH	 = $(A_DEMOAPPS_PATH)/NVIDIA/common
INCLUDES = -I$(A_DEMOAPPS_PATH)/NVIDIA/common -I$(A_LIB_GPUSTUFF_PATH)/include
LIBDIRS = -L$(A_LIB_GPUSTUFF_PATH)/lib/linux/lib64

SRCS = oglapp.cpp \
       oglview.cpp \
       simplegpuvio.cpp \
       ntv2errorlist.cpp \
       ntv2glTextureTransferNV.cpp \
       ntv2gpucircularbuffer.cpp \
       ntv2rendertotexture.cpp \
       ntv2texture.cpp 

LIBS = -ldvp -lX11 -lGL -lGLU -lGLEW

include $(DIR)/../../../../../build/common.mk

endif

