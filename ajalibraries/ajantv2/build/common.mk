#
# Copyright (C) 2004, 2005, 2006, 2007, 2008 AJA Video Systems, Inc.
# Proprietary and Confidential information.  All Rights Reserved
#
CPP =   g++
CPPFLAGS += -DAJALinux -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-pedantic -Wall -Wno-long-long  -Wwrite-strings -c -pipe -fPIC $(DBG)

LD = g++
LDFLAGS = 

# Some projects do not require qt, they can use the rules below by setting these
# macros to the empty string before including this file.
QTINC ?= -I$(QTDIR)/include
QTLIB ?= -L$(QTDIR)/lib
QTL ?= -lqt-mt

VPATH 	  := $(VPATH):$(NTV2_INCLUDES):$(NTV2_CLASSES):$(NTV2_LINUX_CLASSES):$(NTV2_LIBDIR):$(SRCDIR)

INCLUDES   := $(INCLUDES) -I$(NTV2_INCLUDES) -I$(NTV2_CLASSES) -I$(NTV2_LINUX_CLASSES) $(QTINC)

LIBDIRS    := $(LIBDIRS) -L$(NTV2_LIBDIR) $(QTLIB)

LIBCLASSES = $(NTV2_LIBDIR)/libaja.a

LIBS       := $(LIBS) -laja $(QTL) -lncurses -lpthread

OBJS = $(patsubst %.cpp,%.o,$(SRCS))
ifdef C_SRCS
OBJS += $(patsubst %.c,%.o,$(C_SRCS))
endif

ifdef SRCS2
  OBJS2 = $(patsubst %.cpp,%.o,$(SRCS2))
endif
ifdef C_SRCS2
  OBJS2 += $(patsubst %.c,%.o,$(C_SRCS2))
endif

ifdef SRCS3
  OBJS3 = $(patsubst %.cpp,%.o,$(SRCS3))
endif
ifdef C_SRCS3
  OBJS3 += $(patsubst %.c,%.o,$(C_SRCS3))
endif

ifdef SRCS4
  OBJS4 = $(patsubst %.cpp,%.o,$(SRCS4))
endif
ifdef C_SRCS4
  OBJS4 += $(patsubst %.c,%.o,$(C_SRCS4))
endif

%.d: %.cpp
	$(CPP) -M $(CPPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $@ $<

%.d: %.c
	$(CC) -M $(CFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<

all: $(LIBCLASSES) $(NTV2_APP) $(NTV2_APP2) $(NTV2_APP3) $(NTV2_APP4)

-include $(patsubst %.cpp,%.d,$(SRCS))

ifdef SRCS2
  -include $(patsubst %.cpp,%.d,$(SRCS2))
endif

ifdef SRCS3
  -include $(patsubst %.cpp,%.d,$(SRCS3))
endif

ifdef SRCS4
  -include $(patsubst %.cpp,%.d,$(SRCS4))
endif

.PHONY : $(LIBCLASSES)
$(LIBCLASSES): $(SDK_SRCS)
	$(MAKE) -C $(NTV2_CLASSES)

$(NTV2_APP): $(OBJS) $(LIBCLASSES) Makefile
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS) -o $(NTV2_APP) $(LIBS)

ifdef NTV2_APP2
  $(NTV2_APP2): $(OBJS2) $(LIBCLASSES) Makefile
	  $(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS2) -o $(NTV2_APP2) $(LIBS)
endif

ifdef NTV2_APP3
  $(NTV2_APP3): $(OBJS3) $(LIBCLASSES) Makefile
	  $(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS3) -o $(NTV2_APP3) $(LIBS)
endif

ifdef NTV2_APP4
  $(NTV2_APP4): $(OBJS4) $(LIBCLASSES) Makefile
	  $(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS4) -o $(NTV2_APP4) $(LIBS)
endif

.PHONY: cleandeps clean realclean

realclean: clean cleandeps

clean:
	rm -f *.o *~  errors.txt
	rm -f $(NTV2_APP)
ifdef NTV2_APP2
	rm -f $(NTV2_APP2)
endif
ifdef NTV2_APP3
	rm -f $(NTV2_APP3)
endif
ifdef NTV2_APP4
	rm -f $(NTV2_APP4)
endif

cleandeps:
	rm -f *.d
