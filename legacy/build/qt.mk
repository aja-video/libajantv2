# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
#

# Sniff around the system for QT5 and make sure we use it
#
# Using the QT5 qmake to gin up the qMakefile (see the Makefiles in
# the qt<app name> directories for more info) will do the right thing,
# so make sure that we do that.

QMAKE = $(shell command -v qmake 2>&1)
# check that any found system level qmake actually works
QRET := $(shell $(QMAKE) -v 1>&2 2>/dev/null; echo $$?)
ifneq (0, $(QRET))
	QMAKE = $(shell echo "")
endif

# look for qmake in $PATH
ifeq (,$(QMAKE))
    QMAKE = $(shell command -v qmake-qt5 2>&1)
endif
# look for official Qt (online install version) qmake in $HOME, uses highest numbered 5.x.x version
ifeq (,$(QMAKE))
    QMAKE = $(shell find $(HOME) -path "$(HOME)/Qt/5.*/gcc_64/bin/qmake" 2>/dev/null | sort -r | head -n 1)
endif
# look for official Qt (stand-alone install version) qmake in $HOME, uses highest numbered 5.x.x version
ifeq (,$(QMAKE))
    QMAKE = $(shell find $(HOME) -path "$(HOME)/Qt5.*/5.*/gcc_64/bin/qmake" 2>/dev/null | sort -r | head -n 1)
endif
# look for official Qt qmake in /opt, uses highest numbered 5.x.x version
ifeq (,$(QMAKE))
    QMAKE = $(shell find /opt -path "/opt/Qt5.*/5.*/gcc_64/bin/qmake" 2>/dev/null | sort -r | head -n 1)
endif

# WTF?  qmake -v outputs to stderr??
QMAKE_V := $(shell $(QMAKE) -v 2>&1)
QMAKE_5 := $(findstring 5.,$(QMAKE_V))

ifneq (,$(QMAKE_5))
else
    $(error FATAL ERROR: I cannot find qmake version 5 in your PATH.  QT applications in this SDK require version 5.  Please install one and ensure that qmake is in your PATH.)
endif

export QMAKE

#.PHONY: test
#test:
#	@echo QMAKE: $(QMAKE)
#	@echo QMAKE_V: "$(QMAKE_V)"
#	@echo QMAKE_5: $(QMAKE_5)
