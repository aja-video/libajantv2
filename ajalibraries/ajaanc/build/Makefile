# Copyright (C) 2013 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
# Here's the idea behind this makefile approach
# http://make.paulandlesley.org/multi-arch.html

# makefile for libajaanc library

NTV2_ROOT	?= $(shell pwd)/../../../ntv2projects


ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(NTV2_ROOT)/targets.mk
else

NTV2_ROOT	:= $(shell pwd)/../../../../ntv2projects
AJAANC_ROOT	:= $(shell pwd)/../..
AJA_API_ROOT := $(NTV2_ROOT)/../ajaapi

AJA_LIBNAME = libajaanc

AJAANC_SRC := $(AJAANC_ROOT)/src
AJAANC_INC := $(AJAANC_ROOT)/includes
UX_CLASSES := $(NTV2_ROOT)/linuxclasses

VPATH		:= $(AJAANC_SRC) $(NTV2_CLASSES)

DEFINES		:= -D_REENTRANT -DAJALinux -DAJA_LINUX -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS		= -c -fPIC -pipe -Wall -pedantic -Wno-long-long -Wwrite-strings $(DBG) -Wno-variadic-macros
INCLUDES	= -I$(AJAANC_INC) -I$(NTV2_INCLUDES) -I$(NTV2_CLASSES) -I$(UX_CLASSES) -I$(AJA_API_ROOT)

LIBCMD		:= ar crsv
LIBCMD_SO	:= ar crsv

CPP = g++
CPPFLAGS = $(CFLAGS)

CPPFILES	= ancillarydata.cpp \
			  ancillarydatafactory.cpp \
			  ancillarydata_cea608.cpp  \
			  ancillarydata_cea608_line21.cpp \
			  ancillarydata_cea608_vanc.cpp \
			  ancillarydata_cea708.cpp \
			  ancillarydata_framestatusinfo524D.cpp	\
			  ancillarydata_framestatusinfo5251.cpp	\
			  ancillarydata_timecode.cpp	\
			  ancillarydata_timecode_atc.cpp	\
			  ancillarydata_timecode_vitc.cpp	\
			  ancillarylist.cpp

INCFILES	= ancillarydata.h \
			  ancillarydatafactory.h \
			  ancillarydata_cea608.h  \
			  ancillarydata_cea608_line21.h \
			  ancillarydata_cea608_vanc.h \
			  ancillarydata_cea708.h \
			  ancillarydata_framestatusinfo524D.h	\
			  ancillarydata_framestatusinfo5251.h	\
			  ancillarydata_timecode.h	\
			  ancillarydata_timecode_atc.h	\
			  ancillarydata_timecode_vitc.h	\
			  ancillarylist.h

OBJFILES = $(patsubst %.cpp,%.o,$(CPPFILES))

# rule to generate . d files that list the dependencies for a given .cpp file
%.d: %.cpp
	$(CPP) -M $(INCLUDES) $(CPPFLAGS) $(DEFINES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(DEFINES) $(INCLUDES) -o $@ $<

all: cleanold $(NTV2_LIBDIR)/$(AJA_LIBNAME).a $(NTV2_LIBDIR)/$(AJA_LIBNAME).so

-include $(patsubst %.cpp,%.d,$(CPPFILES))

$(NTV2_LIBDIR)/$(AJA_LIBNAME).a: $(OBJFILES) Makefile
#	@echo $(OBJFILES)
	+@[ -d $(NTV2_LIBDIR) ] || mkdir -p $(NTV2_LIBDIR)
	$(LIBCMD) $@ $(OBJFILES)

$(NTV2_LIBDIR)/$(AJA_LIBNAME).so: $(OBJFILES) Makefile
	+@[ -d $(NTV2_LIBDIR) ] || mkdir -p $(NTV2_LIBDIR)
	$(LIBCMD_SO) $@ $(OBJFILES)

.PHONY: clean cleanold cleandeps realclean

clean:
	rm -f *.o
	rm -f $(NTV2_LIBDIR)/$(AJA_LIBNAME).a
	rm -f $(NTV2_LIBDIR)/$(AJA_LIBNAME).so

cleanold:
	@echo root is $(AJAANC_ROOT)
	@echo NTV2 dir is $(NTV2_ROOT)
	rm -f $(NTV2_AJA_CLASSES)/*.o

cleandeps:
	rm -f *.d

realclean: clean cleanold cleandeps

endif

