#
# Copyright (C) 2004-2019 AJA Video Systems, Inc.
# Proprietary and Confidential information.  
# All Rights Reserved.
#

# Makefile to drive qmakes

NTV2_ROOT := $(shell pwd)/../../..

QT_PRO_FILE = ntv2qtcudapreview.pro
QT_QRC_FILE = ntv2qtcudapreview.qrc
QMAKE_FILE = qMakefile
QMAKE ?= qmake

include $(NTV2_ROOT)/qt.mk

default: $(NTV2_ROOT)/bin/ntv2qtcudapreview

$(NTV2_ROOT)/bin/ntv2qtcudapreview:
	$(QMAKE) -o $(QMAKE_FILE) $(QT_PRO_FILE)
	-$(MAKE) -f $(QMAKE_FILE)

.PHONY: clean
clean:
	rm -f $(NTV2_ROOT)/bin/ntv2qtcudapreview
	$(MAKE) -f $(QMAKE_FILE) clean
