#
# Copyright (C) 2004, 2005, 2006, 2007, 2008, 2012 AJA Video Systems, Inc.
# Proprietary and Confidential information.  All Rights Reserved
#
# You no longer need to define NTV2TARGET as either KSD, KHD or XENA2

NTV2_ROOT	?= $(shell pwd)/../..
AJA_API_ROOT ?= $(NTV2_ROOT)/../../ajaapi

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(NTV2_ROOT)/targets.mk
else

CPP =   g++
CPPFLAGS = -DAJALinux -DAJA_LINUX -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-pedantic -Wall -Wno-long-long  -Wwrite-strings -c -pipe -fPIC $(DBG)

LD = g++
LDFLAGS = 

NTV2_APP = $(NTV2_BINDIR)/qabankandmailbox

VPATH 	 := $(SRCDIR) $(NTV2_ROOT)/../qaclasses $(AJA_API_ROOT)

INCLUDES = -I$(NTV2_INCLUDES) -I$(NTV2_CLASSES) -I$(NTV2_ROOT)/../qaclasses -I$(NTV2_ROOT)/../linuxclasses -I$(AJA_API_ROOT)

ifeq ($(AJA_DEBUG),1)
  AJA_STUFF_LIB = ajabased
  RELEASE_OR_DEBUG_LIBS = -l$(AJA_STUFF_LIB)
else
  AJA_STUFF_LIB = ajabase
  RELEASE_OR_DEBUG_LIBS = -l$(AJA_STUFF_LIB)
endif

LIBDIRS = -L$(NTV2_LIBDIR) -L$(AJA_API_ROOT)/lib64 

LIBCLASSES = $(NTV2_LIBDIR)/libaja.a
LIBS = -laja -Wl,-Bstatic $(RELEASE_OR_DEBUG_LIBS) -Wl,-Bdynamic -lpthread -lrt

SRCS = main.cpp

INCS = 

OBJS = $(patsubst %.cpp,%.o,$(SRCS))

%.d: %.cpp
	$(CPP) -M $(CPPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $@ $<

default: $(AJA_STUFF_LIB).a $(LIBCLASSES) $(NTV2_APP)

-include $(patsubst %.cpp,%.d,$(SRCS))

.PHONY : $(LIBCLASSES)
$(LIBCLASSES): $(SDK_SRCS)
	$(MAKE) -C $(NTV2_CLASSES)

.PHONY : $(AJA_STUFF_LIB)
$(AJA_STUFF_LIB).a:
	$(MAKE) -C $(AJA_API_ROOT)/ajabase

$(NTV2_APP): $(OBJS) $(LIBCLASSES) Makefile
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS) -o $(NTV2_APP) $(LIBS)

.PHONY: clean cleandeps  realclean

clean:
	rm -f *.o *~  errors.txt
	rm -f $(NTV2_APP)

cleandeps:
	rm -f *.d

realclean: clean cleandeps

endif
