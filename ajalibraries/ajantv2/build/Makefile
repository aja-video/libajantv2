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
	AJA_LIB = ajantv2d
else
	AJA_LIB = ajantv2
endif
AJA_LIB_PATH = $(A_UBER_LIB)/lib$(AJA_LIB).a

KERNEL_INCLUDES	:= /lib/modules/$(shell uname -r)/build

VPATH := $(A_LIB_NTV2_SRC):$(A_LIB_NTV2_SRC_LINUX):$(A_LIB_NTV2_INC):$(A_LIB_BASE_PATH):$(KERNEL_INCLUDES):$(SRCDIR)

CPPFLAGS := -D_REENTRANT -DAJASTATIC

INCLUDES = -I$(KERNEL_INCLUDES)

AJA_LIB_SRCS = ntv2audio.cpp \
		ntv2active.cpp \
		ntv2autocirculate.cpp \
		ntv2bitfile.cpp \
		ntv2card.cpp \
		ntv2colorcorrection.cpp \
		ntv2config2022.cpp \
		ntv2config2110.cpp \
		ntv2configts2022.cpp \
		ntv2cscmatrix.cpp \
		ntv2debug.cpp \
		ntv2devicefeatures.cpp \
		ntv2devicescanner.cpp \
		ntv2discover.cpp \
		ntv2dma.cpp \
		ntv2dpx.cpp	\
		ntv2draw.cpp \
		ntv2hevc.cpp \
		ntv2driverinterface.cpp \
		ntv2enhancedcsc.cpp \
		ntv2formatdescriptor.cpp \
		ntv2interrupts.cpp \
		ntv2konaflashprogram.cpp \
		ntv2linuxdriverinterface.cpp \
		ntv2mailbox.cpp \
		ntv2mbcontroller.cpp \
		ntv2mcsfile.cpp \
		ntv2nubaccess.cpp \
		ntv2nubpktcom.cpp \
		ntv2outputaudio.cpp \
		ntv2procamp.cpp \
		ntv2publicinterface.cpp \
		ntv2register.cpp \
		ntv2registerexpert.cpp \
		ntv2resample.cpp \
		ntv2rp188.cpp \
		ntv2serialcontrol.cpp \
		ntv2signalrouter.cpp \
		ntv2status.cpp \
		ntv2subscriptions.cpp \
		ntv2testpattern.cpp \
		ntv2tonegenerator.cpp \
		ntv2transcode.cpp \
		ntv2utf8.cpp \
		ntv2utils.cpp \
		ntv2verticalfilter.cpp \
		ntv2vidproc.cpp \
		ntv2vpid.cpp \
		ntv2vpidfromspec.cpp \
		performancecounter.cpp \
		ntv2task.cpp \
		testpatterngen.cpp \
		xena2vidproc.cpp \
        $(A_LIB_NTV2_M31_PATH)/ntv2m31.cpp \
        $(A_LIB_NTV2_M31_PATH)/ntv2m31cparam.cpp \
        $(A_LIB_NTV2_M31_PATH)/ntv2m31ehparam.cpp \
        $(A_LIB_NTV2_M31_PATH)/ntv2m31vaparam.cpp \
        $(A_LIB_NTV2_M31_PATH)/ntv2m31vinparam.cpp \
        $(A_LIB_NTV2_M31_PATH)/ntv2m31viparam.cpp 

include $(DIR)/../../../build/common.mk

endif

