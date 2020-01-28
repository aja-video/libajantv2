#
# Copyright (C) 2004-2020 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
	include $(DIR)/../../../../build/targets.mk
else
	## Makefile to drive qmake
	include $(DIR)/../../../../build/configure.mk
	include $(DIR)/../../../../build/qt.mk

	AJA_QT_APP = $(A_UBER_BIN)/ntv2qtmultiinput
	QT_PRO_FILE = $(A_BUILD_PATH)/ntv2demos/qtmultiinput/qtmultiinput.pro

	include $(DIR)/../../../../build/common.mk
endif
