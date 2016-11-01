#
# Copyright (C) 2004, 2005, 2006, 2007, 2008 AJA Video Systems, Inc.
# Proprietary and Confidential information.  All Rights Reserved
#
# You no longer need to define NTV2TARGET as either KSD, KHD or XENA2

NTV2_ROOT	:= $(shell pwd)/../..

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(NTV2_ROOT)/targets.mk
else

CPP =   g++
CPPFLAGS = -DAJALinux -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-pedantic -Wall -Wno-long-long  -Wwrite-strings -c -pipe -fPIC -w $(DBG)

LD = g++
LDFLAGS = 

NTV2_APP   = $(NTV2_BINDIR)/qauart

VPATH 	  := $(NTV2_INCLUDES):$(NTV2_CLASSES):$(NTV2_LINUX_CLASSES):$(NTV2_LIBDIR):$(SRCDIR)

INCLUDES   = -I$(NTV2_INCLUDES) -I$(NTV2_CLASSES) -I$(NTV2_LINUX_CLASSES)

LIBDIRS    = -L$(NTV2_LIBDIR)

LIBCLASSES = $(NTV2_LIBDIR)/libaja.a
LIBS       = -lpthread -laja

SRCS = main.cpp

OBJS = $(patsubst %.cpp,%.o,$(SRCS))

%.d: %.cpp
	$(CPP) -M $(CPPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $@ $<

all: $(LIBCLASSES) $(NTV2_APP)

-include $(patsubst %.cpp,%.d,$(SRCS)) 

.PHONY : $(LIBCLASSES)
$(LIBCLASSES): $(SDK_SRCS)
	$(MAKE) -C $(NTV2_CLASSES)

$(NTV2_APP): $(OBJS) $(LIBCLASSES) Makefile
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS) -o $(NTV2_APP) $(LIBS)

.PHONY: cleandeps clean realclean

realclean: clean cleandeps

clean:
	rm -f *.o *~  errors.txt
	rm -f $(NTV2_APP)

cleandeps:
	rm -d *.d

endif
