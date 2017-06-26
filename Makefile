#
# Top level Makefile for AJA Video demo applications
#
# Copyright (C) 2017 AJA Video Systems, Inc.
# Proprietary and Confidential information.
#

SUBDIRS = ntv2burn \
		  ntv2burn4kquadrant \
		  ntv2capture \
		  ntv2capture4k \
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
		  NVIDIA/dvplowlatencydemo

ifndef AJA_NO_QT
SUBDIRS := $(SUBDIRS) \
          	  ntv2konaipjsonsetup \
		  ntv2konaipj2ksetup \
		  ntv2qtmultiinput \
		  ntv2qtpreview
endif

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@

.PHONY: clean
clean: cleandeps
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

.PHONY: cleandeps
cleandeps:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir cleandeps; \
	done

