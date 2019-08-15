#
# Copyright (C) 2004 - 2019 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#

# Makefile to drive qmakes

NTV2_ROOT := $(shell pwd)/../..

QT_PRO_FILE = ntv2qtplaycorder.pro
QT_QRC_FILE = ntv2qtplaycorder.qrc
QMAKE_FILE = qMakefile
QMAKE ?= qmake

include ../../qt.mk

default: $(NTV2_ROOT)/bin/ntv2qtplaycorder

$(NTV2_ROOT)/bin/ntv2qtplaycorder:
	$(QMAKE) -o $(QMAKE_FILE) $(QT_PRO_FILE)
	$(MAKE) -f $(QMAKE_FILE)

.PHONY: clean
clean:
	rm -f $(NTV2_ROOT)/bin/ntv2qtplaycorder
	$(MAKE) -f $(QMAKE_FILE) clean
