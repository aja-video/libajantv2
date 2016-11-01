# Makefile for AJA Video NTV2 Linux Classes under linux 2.6+
#
# Copyright (C) 2004, 2005, 2006 AJA Video Systems, Inc.  
# Proprietary and Confidential information.
#
# You need to define NTV2TARGET as either KSD, KHD or XENA2
#

include ../configure.mk

CPP =   g++

CPPFLAGS =	-pedantic -Wall -Wno-long-long

PROJINCS =  ../includes
CLASSES =	../classes
PLATFDIR =	../linuxclasses
DRIVERDIR = ../$(DRIVER_DIR)

INCLUDES =      -I$(PROJINCS) -I$(CLASSES) -I$(PLATFDIR) -I$(DRIVERDIR)

TESTFSTEM = ntv2api
TESTFNAME = $(TESTFSTEM)$(NTV2TARGET)

NTV2APISRCS =	$(CLASSES)/ntv2autocirculate.cpp \
			$(CLASSES)/ntv2devicefeatures.cpp \
			$(CLASSES)/ntv2card.cpp \
			$(CLASSES)/ntv2dma.cpp \
			$(CLASSES)/ntv2nubaccess.cpp \
			$(CLASSES)/ntv2nubpktcom.cpp \
			$(CLASSES)/ntv2driverinterface.cpp \
			$(CLASSES)/ntv2interrupts.cpp \
			$(PLATFDIR)/ntv2linuxdriverinterface.cpp \
			$(PLATFDIR)/ntv2linuxmain.cpp \
			$(CLASSES)/ntv2register.cpp \
			$(CLASSES)/ntv2status.cpp \
			$(CLASSES)/ntv2subscriptions.cpp \
			$(CLASSES)/ntv2utils.cpp \
			$(CLASSES)/ntv2xilinxbitfile.cpp \
			$(CLASSES)/ntv2xilinxprogram.cpp

NTV2APIINCS = $(PROJINCS)/ajatypes.h \
			$(PROJINCS)/audiodefines.h \
			$(PROJINCS)/fixed.h \
			$(CLASSES)/ntv2card.h \
			$(CLASSES)/ntv2nubaccess.h \
			$(CLASSES)/ntv2nubpktcom.h \
			$(CLASSES)/ntv2driverinterface.h \
			$(PLATFDIR)/ntv2linuxdriverinterface.h \
			$(PROJINCS)/ntv2linuxpublicinterface.h \
			$(PROJINCS)/ntv2publicinterface.h \
			$(CLASSES)/ntv2status.h \
			$(CLASSES)/ntv2utils.h \
			$(CLASSES)/ntv2xilinxbitfile.h \
			$(CLASSES)/ntv2xilinxprogram.h \
			$(DRIVERDIR)/registerio.h \
			$(PROJINCS)/videodefines.h

all:	targcheck $(TESTFNAME) 

$(TESTFNAME): $(NTV2APISRCS)  $(NTV2APIINCS) Makefile
	$(CPP) $(INCLUDES) -DAJALinux -D$(NTV2TARGET) $(CPPFLAGS) -g $(NTV2APISRCS) -o $(TESTFNAME)

# You must set NTV2TARGET to determine for which board you are building the
# SDK and driver.  See readme-linux.txt for details.
targcheck:
ifeq ($(NTV2TARGET),)
	$(error Environment variable NTV2TARGET is not defined)
endif

.PHONY:	install clean



clean:
	rm -f *.o *~ $(TESTFSTEM)* 



