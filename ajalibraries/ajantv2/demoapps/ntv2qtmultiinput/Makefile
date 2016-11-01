#
# Copyright (C) 2004-2010 AJA Video Systems, Inc.
# Proprietary and Confidential information.  
# All Rights Reserved.
#

# Makefile to drive qmakes

# have to redefine these here
CPU_ARCH        	:= $(shell uname -m)
ifeq ($(CPU_ARCH),x86_64)
  AJA_API_LIBDIR	= $$(AJA_API_ROOT)/lib64
  AJA_APP_BINDIR	= $$(AJA_API_ROOT)/bin64
else
  AJA_API_LIBDIR	= $$(AJA_API_ROOT)/lib32
  AJA_APP_BINDIR	= $$(AJA_API_ROOT)/bin32
endif

export AJA_API_LIBDIR
export AJA_APP_BINDIR

QT_PRO_FILE = ntv2qtmultiinput.pro
QT_QRC_FILE = ntv2qtmultiinput.qrc
QMAKE_FILE = qMakefile
QMAKE ?= qmake

include ../../qt.mk

AJA_API_ROOT ?= ../../../../

.PHONY: clean

default: $(AJA_NTV2_ROOT)/bin/ntv2qtmultiinput

$(AJA_NTV2_ROOT)/bin/ntv2qtmultiinput:
	$(QMAKE) -o $(QMAKE_FILE) $(QT_PRO_FILE)
	$(MAKE) -f $(QMAKE_FILE)

clean:
	rm -f $(AJA_NTV2_ROOT)/bin/ntv2qtmultiinput
	$(MAKE) -f $(QMAKE_FILE) clean

.PHONY: cleandeps
cleandeps:
	@echo "cleandeps is a nop here"

