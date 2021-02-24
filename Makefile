# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2021 AJA Video Systems, Inc.
#

#
# Top level Makefile for AJA Video demo applications
#
# Control Flags for Make
#
# AJA_NO_QT - if this environment variable is defined the demo applications
#             that build against the Qt libraries will be skipped.

SUBDIRS = ntv2burn \
		  ntv2burn4kquadrant \
		  ntv2capture \
		  ntv2capture4k \
		  ntv2capture8k \
		  ntv2ccgrabber \
		  ntv2ccplayer \
		  ntv2encodehevc \
		  ntv2encodehevcfile \
		  ntv2encodehevcvif \
		  ntv2enumerateboards \
		  ntv2fieldburn \
		  ntv2hdrsetup \
		  ntv2llburn \
		  ntv2outputtestpattern \
		  ntv2player \
		  ntv2player4k \
		  ntv2player8k

ifndef AJA_NO_QT
SUBDIRS := $(SUBDIRS) \
		  ntv2konaipjsonsetup \
		  ntv2konaipj2ksetup \
		  ntv2qtmultiinput \
		  ntv2qtpreview
endif

ifndef AJA_NO_GL
SUBDIRS := $(SUBDIRS) \
		  NVIDIA/dvplowlatencydemo
endif

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: clean
clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
