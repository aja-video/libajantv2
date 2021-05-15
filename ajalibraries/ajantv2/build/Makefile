# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2021 AJA Video Systems, Inc.
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
#AJA_LIB_PATH_SO = $(A_UBER_LIB)/lib$(AJA_LIB).so

KERNEL_INCLUDES	:= /lib/modules/$(shell uname -r)/build

VPATH := $(A_LIB_NTV2_SRC):$(A_LIB_NTV2_SRC_LINUX):$(A_LIB_NTV2_M31_PATH):$(A_LIB_NTV2_INC):$(A_LIB_ANC_PATH)/src:$(A_LIB_BASE_PATH):$(A_LIB_BASE_PATH)/common:$(A_LIB_BASE_PATH)/system:$(A_LIB_BASE_PATH)/system/linux:$(A_LIB_BASE_PATH)/pnp:$(A_LIB_BASE_PATH)/pnp/linux:$(A_LIB_BASE_PATH)/persistence:$(A_LIB_BASE_PATH)/network:$(KERNEL_INCLUDES):$(SRCDIR)

CPPFLAGS := -D_REENTRANT -DAJASTATIC

INCLUDES = -I$(KERNEL_INCLUDES) -I$(A_LIB_ANC_PATH)/includes

AJA_LIB_SRCS =\
		ancillarydata.cpp \
		ancillarydatafactory.cpp \
		ancillarydata_cea608.cpp \
		ancillarydata_cea608_line21.cpp \
		ancillarydata_cea608_vanc.cpp \
		ancillarydata_cea708.cpp \
		ancillarydata_framestatusinfo524D.cpp \
		ancillarydata_framestatusinfo5251.cpp \
		ancillarydata_hdr_hdr10.cpp \
		ancillarydata_hdr_hlg.cpp \
		ancillarydata_hdr_sdr.cpp \
		ancillarydata_timecode.cpp \
		ancillarydata_timecode_atc.cpp \
		ancillarydata_timecode_vitc.cpp \
		ancillarylist.cpp \
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
		wavewriter.cpp \
		ntv2audio.cpp \
		ntv2anc.cpp \
		ntv2autocirculate.cpp \
		ntv2bitfile.cpp \
		ntv2bitfilemanager.cpp \
		ntv2card.cpp \
		ntv2config2022.cpp \
		ntv2config2110.cpp \
		ntv2configts2022.cpp \
		ntv2csclut.cpp \
		ntv2cscmatrix.cpp \
		ntv2debug.cpp \
		ntv2devicefeatures.cpp \
		ntv2devicescanner.cpp \
		ntv2discover.cpp \
		ntv2dma.cpp \
		ntv2dynamicdevice.cpp \
		ntv2hdmi.cpp \
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
		ntv2publicinterface.cpp \
		ntv2register.cpp \
		ntv2registerexpert.cpp \
		ntv2resample.cpp \
		ntv2routingexpert.cpp \
		ntv2rp188.cpp \
		ntv2serialcontrol.cpp \
		ntv2signalrouter.cpp \
		ntv2spiinterface.cpp \
		ntv2subscriptions.cpp \
		ntv2supportlogger.cpp \
		ntv2transcode.cpp \
		ntv2utf8.cpp \
		ntv2utils.cpp \
		ntv2verticalfilter.cpp \
		ntv2vpid.cpp \
		ntv2vpidfromspec.cpp \
		ntv2task.cpp \
		ntv2testpatterngen.cpp \
        ntv2m31.cpp \
        ntv2m31cparam.cpp \
        ntv2m31ehparam.cpp \
        ntv2m31vaparam.cpp \
        ntv2m31vinparam.cpp \
        ntv2m31viparam.cpp 

ifndef AJA_LIB_PATH_SO
	AJA_LIB_SRCS += persistence.cpp
endif

include $(DIR)/../../../build/common.mk

endif

