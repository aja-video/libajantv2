#
# Copyright (C) 2004-2013 AJA Video Systems, Inc.
# Proprietary and Confidential information.  
# All Rights Reserved.
#

# Makefile to drive qmakes

NTV2_ROOT := $(shell pwd)/../../..

QT_PRO_FILE = ntv2qtoglpreview.pro
QT_QRC_FILE = ntv2qtoglpreview.qrc
QMAKE_FILE = qMakefile
QMAKE ?= qmake

include $(NTV2_ROOT)/qt.mk

.PHONY: clean

default: $(NTV2_ROOT)/bin/ntv2qtoglpreview

$(NTV2_ROOT)/bin/ntv2qtoglpreview:
	$(QMAKE) -o $(QMAKE_FILE) $(QT_PRO_FILE)
	$(MAKE) -f $(QMAKE_FILE)

clean:
	rm -f $(NTV2_ROOT)/bin/ntv2qtoglpreview
	$(MAKE) -f $(QMAKE_FILE) clean

.PHONY: cleandeps
cleandeps:
	@echo "cleandeps is a nop here"

