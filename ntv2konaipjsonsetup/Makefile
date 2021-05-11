# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2021 AJA Video Systems, Inc.
#

DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

# Makefile to drive qmake
include $(DIR)/../../../../build/configure.mk
include $(DIR)/../../../../build/qt.mk

APP_FULL_PATH=$(A_UBER_BIN)/ntv2konaipjsonsetup
QT_PRO_FILE = $(A_BUILD_PATH)/ntv2demos/konaipjsonsetup/konaipjsonsetup.pro

default: $(APP_FULL_PATH)

$(APP_FULL_PATH):
	$(call ensure_dir_exists, $(OBJDIR))
	$(MAKE) -C $(A_LIB_NTV2_PATH)/build
	cd $(OBJDIR) && $(QMAKE) $(QT_PRO_FILE) && $(MAKE)

.PHONY: clean
clean:
	@[ -d $(OBJDIR) ] && rm -rf $(OBJDIR) || true
	@rm -f $(APP_FULL_PATH)
