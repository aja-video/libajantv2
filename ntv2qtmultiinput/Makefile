# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2021 AJA Video Systems, Inc.
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
