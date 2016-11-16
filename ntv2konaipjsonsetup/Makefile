#
# Copyright (C) 2004-2010 AJA Video Systems, Inc.
# Proprietary and Confidential information.  
# All Rights Reserved.
#

# Makefile to drive qmakes

NTV2_ROOT := $(shell pwd)/../..

QT_PRO_FILE = ntv2konaipjsonsetup.pro
QMAKE_FILE = qMakefile
QMAKE ?= qmake

include $(NTV2_ROOT)/qt.mk

.PHONY: clean

default: $(NTV2_ROOT)/../bin/ntv2konaipjsonsetup

$(NTV2_ROOT)/../bin/ntv2konaipjsonsetup:
	$(QMAKE) -o $(QMAKE_FILE) $(QT_PRO_FILE)
	$(MAKE) -f $(QMAKE_FILE)

clean:
	rm -f $(NTV2_ROOT)/../bin/ntv2konaipjsonsetup
	$(MAKE) -f $(QMAKE_FILE) clean

.PHONY: cleandeps
cleandeps:
	@echo "cleandeps is a nop here"

