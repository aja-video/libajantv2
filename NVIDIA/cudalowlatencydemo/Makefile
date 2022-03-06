#
# Copyright (C) 2004-2022 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq (,$(filter _%,$(notdir $(CURDIR))))
  include $(DIR)/../../../../../build/targets.mk
else
include $(DIR)/../../../../../build/configure.mk

NVCC ?= nvcc
ifeq (,$(wildcard $(NVCC)))
	NVCC := /usr/local/cuda/bin/nvcc
endif

AJA_APP = $(A_UBER_BIN)/cudaapp

CUDA_INC_DIR := $(shell find /usr/local/cuda-* -name cuda.h 2>/dev/null|head -1|xargs dirname 2>/dev/null)
CUDA_LIB_DIR := $(shell find /usr/local/cuda-* -name libcudart.so 2>/dev/null|head -1|xargs dirname 2>/dev/null)

VPATH = $(A_DEMOAPPS_PATH):$(DIR)/../commonCUDA
INCLUDES = -I$(A_DEMOAPPS_PATH) -I$(DIR)/../commonCUDA -I$(A_LIB_GPUSTUFF_PATH)/include -I$(A_LIB_BASE_PATH)/system -I$(CUDA_INC_DIR)
LIBDIRS = -L$(CUDA_LIB_DIR) -Xlinker -rpath=$(CUDA_LIB_DIR) 

ifdef AJA_RDMA
CPPFLAGS += -DAJA_RDMA=$(AJA_RDMA)
ifdef AJA_IGPU
CPPFLAGS += -DAJA_IGPU=$(AJA_IGPU)
endif
endif

SRCS = cudaapp.cpp \
       ntv2democommon.cpp \
       ntv2cudaArrayTransferNV.cpp \
       ntv2gpuTextureTransferNV.cpp \
       oglview.cpp \
       simplegpuvio.cpp \
       ntv2errorlist.cpp \
       ntv2gpucircularbuffer.cpp \
       ntv2rendertotexture.cpp \
       ntv2texture.cpp

EXTRA_OBJS += kernel.o
LIBS += -lX11 -lGL -lGLU -lGLEW -lcuda -lcudart

include $(DIR)/../../../../../build/common.mk

kernel.o: kernel.cu
	$(NVCC) -c -I$(DIR)/../commonCUDA $(DIR)/kernel.cu

endif

