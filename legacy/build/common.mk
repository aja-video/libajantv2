# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
#

CXX ?= g++
CPP := $(CXX)

ifeq ($(AJA_USE_CCACHE),1)
	CXX := ccache $(CXX)
	CPP := ccache $(CPP)
	CC := ccache $(CC)
endif

CPPFLAGS += -DAJALinux -DAJA_LINUX -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 \
			-pedantic -Wall -Wno-long-long -Wwrite-strings -c -pipe -fPIC -std=c++11 $(DBG)

LD = $(CXX)
LDFLAGS = 

VPATH	:= $(VPATH):$(A_LIB_NTV2_INC):$(A_LIB_NTV2_SRC):$(A_LIB_NTV2_SRC_LINUX):$(A_UBER_LIB):$(SRCDIR)
INCLUDES:= $(INCLUDES) -I$(A_LIB_NTV2_INC) -I$(A_LIB_NTV2_SRC) -I$(A_LIB_NTV2_SRC_LINUX) -I$(A_LIBRARIES_PATH) -I$(A_LIB_BASE_PATH)
LIBDIRS	:= $(LIBDIRS) -L$(A_UBER_LIB)

rm_if_file_exists = @[ -f $1 ] && rm -f $1 && echo "rm -f $1" || true
rm_if_dir_exists = @[ -d $1 ] && rm -rf $1 && echo "rm -rf $1" || true
mkdir_if_not_exists = @[ -d $1 ] || mkdir -p $1 && echo "mkdir -p $1" || true

ifeq ($(AJA_DEBUG),1)
	LIB_AJANTV2_FILENAME = ajantv2d
else
	LIB_AJANTV2_FILENAME = ajantv2
endif
LIB_AJANTV2 = $(A_UBER_LIB)/lib$(LIB_AJANTV2_FILENAME).a
LIB_AJANTV2_SO = $(A_UBER_LIB)/lib$(LIB_AJANTV2_FILENAME).so

ifdef AJA_LIB_PATH
	BUILD_AND_LINK_AGAINST_AJALIBS = 0
else ifdef AJA_LIB_PATH_SO
	BUILD_AND_LINK_AGAINST_AJALIBS = 0
else
	BUILD_AND_LINK_AGAINST_AJALIBS = 1
endif

ifeq ($(BUILD_AND_LINK_AGAINST_AJALIBS),1)
	LIBS := $(LIBS) -l$(LIB_AJANTV2_FILENAME) -lpthread -lrt -ldl
	LIB_AJANTV2_DEP := $(LIB_AJANTV2)
else
	LIBS := $(LIBS) -lpthread -lrt -ldl
	LIB_AJANTV2_DEP := 
endif

OBJS = $(patsubst %.cpp,%.o,$(SRCS)) $(EXTRA_OBJS)
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

ifdef AJA_LIB_SRCS
	LIBOBJS = $(patsubst %.cpp,%.o,$(AJA_LIB_SRCS))
	ifdef AJA_LIB_C_SRCS
	LIBOBJS += $(patsubst %.c,%.o,$(AJA_LIB_C_SRCS))
	endif
endif

%.o: %.cpp
	$(CPP) $(CPPFLAGS) $(INCLUDES) -o $@ $<
	@$(CPP) -MM $(CPPFLAGS) $(INCLUDES) -MF $*.d $<

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCLUDES) -o $@ $<
	@$(CC) -MM $(CFLAGS) $(INCLUDES) -MF $*.d $<

ifeq ($(BUILD_AND_LINK_AGAINST_AJALIBS),1)
all: $(LIB_AJANTV2) $(AJA_APP) $(AJA_APP2) $(AJA_APP3) $(AJA_APP4) $(AJA_QT_APP)
else
all: $(AJA_APP) $(AJA_APP2) $(AJA_APP3) $(AJA_APP4) $(AJA_QT_APP) $(AJA_LIB_PATH) $(AJA_LIB_PATH_SO)
endif

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

ifdef AJA_LIB_SRCS
    -include $(patsubst %.cpp,%.d,$(AJA_LIB_SRCS))
endif

ifeq ($(BUILD_AND_LINK_AGAINST_AJALIBS),1)
.PHONY : $(LIB_AJANTV2)
$(LIB_AJANTV2): $(SDK_SRCS)
		$(MAKE) -C $(A_LIB_NTV2_PATH)/build
endif

$(AJA_APP): $(OBJS) $(LIB_AJANTV2_DEP)
	$(call mkdir_if_not_exists,$(A_UBER_BIN))
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS) -o $(AJA_APP) $(LIBS)

ifdef AJA_APP2
$(AJA_APP2): $(OBJS2) $(LIB_AJANTV2_DEP)
	$(call mkdir_if_not_exists,$(A_UBER_BIN))
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS2) -o $(AJA_APP2) $(LIBS)
endif

ifdef AJA_APP3
$(AJA_APP3): $(OBJS3) $(LIB_AJANTV2_DEP)
	$(call mkdir_if_not_exists,$(A_UBER_BIN))
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS3) -o $(AJA_APP3) $(LIBS)
endif

ifdef AJA_APP4
$(AJA_APP4): $(OBJS4) $(LIB_AJANTV2_DEP)
	$(call mkdir_if_not_exists,$(A_UBER_BIN))
	$(LD) $(LDFLAGS) $(LIBDIRS) $(OBJS4) -o $(AJA_APP4) $(LIBS)
endif

ifdef AJA_LIB_PATH
.PHONY : $(AJA_LIB_PATH)
$(AJA_LIB_PATH): $(LIBOBJS)
	$(call mkdir_if_not_exists,$(A_UBER_LIB))
	ar crs $@ $(LIBOBJS)
endif

ifdef AJA_LIB_PATH_SO
.PHONY : $(AJA_LIB_PATH_SO)
$(AJA_LIB_PATH_SO): $(LIBOBJS)
	$(call mkdir_if_not_exists,$(A_UBER_LIB))
	$(CXX) $(LDFLAGS) -shared -Wl,-soname,lib$(AJA_LIB).so -o $@ $(LIBOBJS) $(LIBS)
endif

ifdef AJA_QT_APP
.PHONY: $(AJA_QT_APP)
$(AJA_QT_APP):
	$(call mkdir_if_not_exists,$(OBJDIR))
	cd $(OBJDIR) && $(QMAKE) $(QT_PRO_FILE) && $(MAKE)
endif

.PHONY: clean
clean:
ifdef AJA_APP
	$(call rm_if_file_exists,$(AJA_APP))	
endif
ifdef AJA_APP2
	$(call rm_if_file_exists,$(AJA_APP2))	
endif
ifdef AJA_APP3
	$(call rm_if_file_exists,$(AJA_APP3))	
endif
ifdef AJA_APP4
	$(call rm_if_file_exists,$(AJA_APP4))	
endif
ifdef AJA_QT_APP
	$(call rm_if_file_exists,$(AJA_QT_APP))	
endif
ifdef AJA_LIB_PATH
	$(call rm_if_file_exists,$(AJA_LIB_PATH))	
endif
ifdef AJA_LIB_PATH_SO
	$(call rm_if_file_exists,$(AJA_LIB_PATH_SO))	
endif
ifeq ($(BUILD_AND_LINK_AGAINST_AJALIBS),1)
	$(call rm_if_file_exists,$(LIB_AJANTV2_SO))	
	$(call rm_if_file_exists,$(LIB_AJANTV2))	
	$(call rm_if_dir_exists,"$(A_LIB_NTV2_PATH)/build/$(OBJDIR)")
endif
	$(call rm_if_dir_exists,"`pwd`")

