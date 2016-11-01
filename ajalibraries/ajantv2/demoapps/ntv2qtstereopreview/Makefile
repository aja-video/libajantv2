#
# Copyright (C) 2004-2010 AJA Video Systems, Inc.
# Proprietary and Confidential information.  
# All Rights Reserved.
#

# Makefile to drive qmakes

NTV2_ROOT := $(shell pwd)/../..

QT_PRO_FILE = ntv2qtstereopreview.pro
QT_QRC_FILE = ntv2qtstereopreview.qrc
QMAKE_FILE = qMakefile
QMAKE ?= qmake

include ../../qt.mk

.PHONY: clean

default: $(NTV2_ROOT)/bin/ntv2qtstereopreview

$(NTV2_ROOT)/bin/ntv2qtstereopreview:
	$(QMAKE) -o $(QMAKE_FILE) $(QT_PRO_FILE)
	$(MAKE) -f $(QMAKE_FILE)

clean:
	rm -f $(NTV2_ROOT)/bin/ntv2qtstereopreview
	$(MAKE) -f $(QMAKE_FILE) clean

.PHONY: cleandeps
cleandeps:
	@echo "cleandeps is a nop here"

