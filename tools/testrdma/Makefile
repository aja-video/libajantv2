#
# Copyright (C) 2004 - 2017 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../build/targets.mk
else
include $(DIR)/../../../build/configure.mk

AJA_APP = $(A_UBER_BIN)/testrdma

VPATH	 = $(A_DEMOAPPS_PATH)
INCLUDES = -I$(A_DEMOAPPS_PATH)

ifdef AJA_RDMA
CPPFLAGS += -DAJA_RDMA=$(AJA_RDMA)
ifdef AJA_IGPU
CPPFLAGS += -DAJA_IGPU=$(AJA_IGPU)
endif
CUDA_INC_DIR ?= $(shell find /usr/local/cuda-* -name cuda.h 2>/dev/null|head -1|xargs dirname 2>/dev/null)
CUDA_LIB_DIR ?= $(shell find /usr/local/cuda-* -name libcudart.so 2>/dev/null|head -1|xargs dirname 2>/dev/null)
INCLUDES += -I$(CUDA_INC_DIR)
LIBDIRS += -L$(CUDA_LIB_DIR) -Xlinker -rpath=$(CUDA_LIB_DIR) 
LIBS += -lcuda -lcudart
endif

SRCS = testrdma.cpp

include $(DIR)/../../../build/common.mk

endif
