#
# Copyright (C) 2004 - 2017 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

# Makefile to drive qmake
include $(DIR)/../../../../build/configure.mk
include $(DIR)/../../../../build/qt.mk

APP_FULL_PATH=$(A_UBER_BIN)/ntv2qtmultiinput
QT_PRO_FILE = $(A_BUILD_PATH)/ntv2demos/qtmultiinput/qtmultiinput.pro
QMAKE_FILE = qMakefile

.PHONY: clean

default: $(APP_FULL_PATH)

$(APP_FULL_PATH):
	$(QMAKE) -o $(QMAKE_FILE) $(QT_PRO_FILE)
	$(MAKE) -f $(QMAKE_FILE)

clean:
	rm -f $(APP_FULL_PATH)
	$(MAKE) -f $(QMAKE_FILE) clean
	rm -f $(QMAKE_FILE)

.PHONY: cleandeps
cleandeps:
	@echo "cleandeps is a nop here"

