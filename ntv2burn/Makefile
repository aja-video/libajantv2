#
# Copyright (C) 2004, 2005, 2006, 2007, 2008, 2012 AJA Video Systems, Inc.
# Proprietary and Confidential information.  All Rights Reserved
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../../targets.mk
else
include $(DIR)/../../../../configure.mk

CPP =   g++
CPPFLAGS = -DAJALinux -DAJA_LINUX -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-pedantic -Wall -Wno-long-long  -Wwrite-strings -c -pipe -fPIC $(DBG)

LD = g++
LDFLAGS = 

NTV2_APP = $(NTV2_BIN_DIR)/ntv2burn

VPATH 	 := $(SRCDIR) $(NTV2_DEMO_ROOT)

INCLUDES = -I$(NTV2_INCLUDES) -I$(NTV2_SRC) -I$(NTV2_DEMO_ROOT) -I$(NTV2_LINUX_SRC) -I$(AJALIB_DIR) -I$(AJABASE_DIR)

ifeq ($(AJA_DEBUG),1)
  AJABASE_LIB = ajabased
  RELEASE_OR_DEBUG_LIBS = -l$(AJABASE_LIB)
else
  AJABASE_LIB = ajabase
  RELEASE_OR_DEBUG_LIBS = -l$(AJABASE_LIB)
endif

LIBDIRS = -L$(NTV2_LIB_DIR)

LIBCLASSES = $(NTV2_LIB_DIR)/libajantv2.a
LIBS = -lajantv2 -Wl,-Bstatic $(RELEASE_OR_DEBUG_LIBS) -Wl,-Bdynamic -lpthread -lrt

SRCS = main.cpp ntv2burn.cpp ntv2democommon.cpp

INCS = ntv2burn.h ntv2democommon.h

OBJS = $(patsubst %.cpp,%.o,$(SRCS))

%.d: %.cpp
	$(CPP) -M $(CPPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $@ $<

default: $(AJABASE_LIB).a $(LIBCLASSES) $(NTV2_APP)

-include $(patsubst %.cpp,%.d,$(SRCS))

.PHONY : $(LIBCLASSES)
$(LIBCLASSES): $(SDK_SRCS)
	$(MAKE) -C $(NTV2_AJANTV2_DIR)/build

.PHONY : $(AJABASE_LIB)
$(AJABASE_LIB).a:
	$(MAKE) -C $(AJABASE_DIR)

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
