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

AJA_LIB = ajaanc
AJA_LIB_PATH = $(A_UBER_LIB)/lib$(AJA_LIB).a

VPATH    = $(A_LIB_ANC_PATH)/src
INCLUDES = -I$(A_LIB_ANC_PATH)/includes

CPPFLAGS += -DAJA_LINUX

AJA_LIB_SRCS =ancillarydata.cpp \
			  ancillarydatafactory.cpp \
			  ancillarydata_cea608.cpp \
			  ancillarydata_cea608_line21.cpp \
			  ancillarydata_cea608_vanc.cpp \
			  ancillarydata_cea708.cpp \
			  ancillarydata_framestatusinfo524D.cpp	\
			  ancillarydata_framestatusinfo5251.cpp	\
			  ancillarydata_timecode.cpp \
			  ancillarydata_timecode_atc.cpp \
			  ancillarydata_timecode_vitc.cpp \
			  ancillarylist.cpp

include $(DIR)/../../../build/common.mk

endif

