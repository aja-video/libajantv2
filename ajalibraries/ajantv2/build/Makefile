# Copyright (C) 2004, 2005, 2006, 2007, 2008 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
# Here's the idea behind this makefile approach
# http://make.paulandlesley.org/multi-arch.html

# makefile for classes library
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../targets.mk
else
include $(DIR)/../../../configure.mk

AJA_LIBNAME = libajantv2

KERNEL_INCLUDES 	:= /lib/modules/$(shell uname -r)/build

PYTHON_DEV	:= /usr/include/python2.4

VPATH 	 := $(NTV2_SRC):$(NTV2_LINUX_SRC):$(NTV2_INCLUDES):$(AJABASE_DIR):$(KERNEL_INCLUDES):$(SRCDIR)

DEFINES := -D_REENTRANT -DAJALinux -DAJASTATIC -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS  	= -c -fPIC -pipe -Wall -pedantic -Wno-long-long -Wwrite-strings $(DBG)
INCLUDES	= -I$(NTV2_INCLUDES) -I$(NTV2_SRC) -I$(NTV2_LINUX_SRC) -I$(AJABASE_DIR) -I$(KERNEL_INCLUDES) -I$(QTDIR)/include -I$(PYTHON_DEV)
LIBS    	=
LIBDIRS 	=

ifeq ($(AJA_DEBUG),1)
	DEFINES += -DAJA_DEBUG
endif

LIBCMD 		:= ar crsv
LIBCMD_SO	:= ar crsv

CPP = g++
CPPFLAGS = $(CFLAGS)

CPPFILES = ntv2audio.cpp \
		ntv2active.cpp \
		ntv2autocirculate.cpp \
		ntv2bitfile.cpp \
		ntv2card.cpp \
		ntv2colorcorrection.cpp \
		ntv2config2022.cpp \
		ntv2configts2022.cpp \
		ntv2cscmatrix.cpp \
		ntv2debug.cpp \
		ntv2devicefeatures.cpp \
		ntv2devicescanner.cpp \
		ntv2discover.cpp \
		ntv2dma.cpp \
		ntv2dpx.cpp	\
		ntv2draw.cpp \
		ntv2hevc.cpp \
		ntv2driverinterface.cpp \
		ntv2enhancedcsc.cpp \
		ntv2formatdescriptor.cpp \
		ntv2interrupts.cpp \
		ntv2konaflashprogram.cpp \
		ntv2linuxdriverinterface.cpp \
		ntv2mailbox.cpp \
		ntv2mbcontroller.cpp \
		ntv2mcsfile.cpp \
		ntv2nubaccess.cpp \
		ntv2nubpktcom.cpp \
		ntv2outputaudio.cpp \
		ntv2procamp.cpp \
		ntv2publicinterface.cpp \
		ntv2register.cpp \
		ntv2registerexpert.cpp \
		ntv2resample.cpp \
		ntv2rp188.cpp \
		ntv2serialcontrol.cpp \
		ntv2signalrouter.cpp \
		ntv2status.cpp \
		ntv2subscriptions.cpp \
		ntv2testpattern.cpp \
		ntv2tonegenerator.cpp \
		ntv2transcode.cpp \
		ntv2utf8.cpp \
		ntv2utils.cpp \
		ntv2verticalfilter.cpp \
		ntv2vidproc.cpp \
		ntv2vpid.cpp \
		ntv2vpidfromspec.cpp \
		performancecounter.cpp \
		ntv2task.cpp \
		testpatterngen.cpp \
		xena2vidproc.cpp \
		$(M31_ROOT)/ntv2m31.cpp \
		$(M31_ROOT)/ntv2m31cparam.cpp \
		$(M31_ROOT)/ntv2m31ehparam.cpp \
		$(M31_ROOT)/ntv2m31vaparam.cpp \
		$(M31_ROOT)/ntv2m31vinparam.cpp \
		$(M31_ROOT)/ntv2m31viparam.cpp


INCFILES = ntv2bitfile.h  \
		ntv2active.h \
		ntv2card.h \
		ntv2colorcorrection.h \
		ntv2config2022.h \
		ntv2configts2022.h \
		ntv2cscmatrix.h \
		ntv2debug.h \
		ntv2devicefeatures.h \
		ntv2devicescanner.h \
		ntv2deviceservices.h \
		ntv2discover.h \
		ntv2diskio.h \
		ntv2driverinterface.h \
		ntv2driverprocamp.h \
		ntv2enhancedcsc.h \
		ntv2formatdescriptor.h \
		ntv2framedelay.h \
		ntv2ioexpressservices.h \
		ntv2iohdservices.h \
		ntv2ioxtservices.h \
		ntv2kona3Gquadservices.h \
		ntv2kona3Gservices.h \
		ntv2kona3services.h \
		ntv2konaflashprogram.h \
		ntv2konalheplusservices.h \
		ntv2konalhiservices.h \
		ntv2lhservices.h \
		ntv2linuxdriverinterface.h \
		ntv2mailbox.h \
		ntv2mbcontroller.h \
		ntv2mcsfile.h \
		ntv2nubaccess.h \
		ntv2nubpktcom.h \
		ntv2nubtypes.h \
		ntv2procamp.h \
		ntv2registerexpert.h \
		ntv2registers2022.h \
		ntv2resample.h \
		ntv2rp188.h \
		ntv2serialcontrol.h \
		ntv2signalrouter.h \
		ntv2status.h \
		ntv2task.h \
		ntv2testpattern.h \
		ntv2transcode.h \
		ntv2tshelper.h \
		ntv2utf8.h \
		ntv2utils.h \
		ntv2verticalfilter.h \
		ntv2vidproc.h \
		ntv2vidprocmasks.h \
		ntv2vpid.h \
		ntv2vpidfromspec.h \
		ntv2dpx.h \
		performancecounter.h \
		testpatterndata.h \
		testpatterngen.h \
		xena2vidproc.h \
		$(M31_ROOT)/ntv2m31.h \
		$(M31_ROOT)/ntv2m31cparam.h \
		$(M31_ROOT)/ntv2m31ehparam.h \
		$(M31_ROOT)/ntv2m31vaparam.h \
		$(M31_ROOT)/ntv2m31vinparam.h \
		$(M31_ROOT)/ntv2m31viparam.h

OBJFILES = $(patsubst %.cpp,%.o,$(CPPFILES))

# rule to generate . d files that list the dependencies for a given .cpp file
%.d: %.cpp
	$(CPP) -M $(INCLUDES) $(DEFINES) $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(DEFINES) $(INCLUDES) -o $@ $<

all: cleanold $(NTV2_LIB_DIR)/$(AJA_LIBNAME).a

-include $(patsubst %.cpp,%.d,$(CPPFILES))

$(NTV2_LIB_DIR)/$(AJA_LIBNAME).a: $(OBJFILES) Makefile
#	@echo $(OBJFILES)
	+@[ -d $(NTV2_LIB_DIR) ] || mkdir -p $(NTV2_LIB_DIR)
	$(LIBCMD) $@ $(OBJFILES)

$(NTV2_LIB_DIR)/$(AJA_LIBNAME).so: $(OBJFILES) Makefile
	+@[ -d $(NTV2_LIB_DIR) ] || mkdir -p $(NTV2_LIB_DIR)
	@echo
	@echo "WARNING: this is a FAKE target and shared object libraries are not is use (yet)"
	@echo
	$(LIBCMD_SO) $@ $(OBJFILES)

.PHONY: clean cleanold cleandeps realclean
clean:
	rm -f *.o
	rm -f $(M31_ROOT)/*.o
	rm -f $(NTV2_LIB_DIR)/$(AJA_LIBNAME).a

cleanold:
	rm -f $(AJA_CLASSES)/*.o
	rm -f $(NTV2_SRC)/*.o
	rm -f $(NTV2_LINUX_SRC)/*.o

cleandeps:
	rm -f *.d

realclean: clean cleanold cleandeps


endif

