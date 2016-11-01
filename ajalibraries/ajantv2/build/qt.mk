#
# Copyright (C) 2010 AJA Video Systems, Inc.  
# Proprietary and Confidential information.
#

# Sniff around the system for QT4 and make sure we use it
#
# Using the QT4 qmake to gin up the qMakefile (see the Makefiles in
# the qt<app name> directories for more info) will do the right thing,
# so make sure that we do that.

# WTF?  qmake -v outputs to stderr??
QMAKE_V := $(shell qmake -v 2>&1)
QMAKE_4 := $(findstring 4.,$(QMAKE_V))
QMAKE_5 := $(findstring 5.,$(QMAKE_V))

ifneq (,$(QMAKE_4))
else ifneq (,$(QMAKE_5))
else
    $(error FATAL ERROR: I cannot find qmake version 4 or 5 in your PATH.  QT applications in this SDK require one of those versions.  Please install one of them and ensure that qmake is in your PATH.)
endif

#.PHONY: test
#test:
#	@echo QMAKE_V: "$(QMAKE_V)"
#	@echo QMAKE_4: $(QMAKE_4)

