# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2023 AJA Video Systems, Inc.
#

# Makefile for AJA Linux Video NTV2 Driver
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

#DRIVERDIR	?= $(DIR)

# Allow specifying kernel version manually so it is possible to build in a containerized environment.
ifdef AJA_DRIVER_KVERSION
	KVERSION = $(AJA_DRIVER_KVERSION)
else
	KVERSION ?= $(shell uname -r)
endif
KDIR		?= /lib/modules/$(KVERSION)/build

# EXTRA_DEPS = -DNTV2_DEPRECATE_14_0 -DNTV2_DEPRECATE_14_1 

include $(DIR)/../configure.mk
include $(DIR)/../sdkversion.mk

# Create device nodes by default (load_ajantv2 no longer required)
# Install: insmod ajantv2.ko   Remove: rmmod ajantv2
AJA_CREATE_DEVICE_NODES = 1

CC			 ?= gcc

DRIVERTARGET = $(NTV2TARGET).o
INCLUDES 	 = -I$(A_LIB_NTV2_INC) -I$(A_LINUX_DRIVER_PATH) -I$(A_LIB_NTV2_SRC) -I$(A_LIB_NTV2_SRC)/lin -I$(A_DRIVER_PATH)
VERSIONING 	 = -DSDKVER_MAJ=$(SDKVER_MAJ) -DSDKVER_MIN=$(SDKVER_MIN) -DSDKVER_PNT=$(SDKVER_PNT) -DSDKVER_BLD=$(SDKVER_BLD) -DAJA_BETA=$(AJA_BETA) -DAJA_DEBUG=$(AJA_DEBUG)
DISTRO_INFO  = -DDISTRO_TYPE=$(DISTRO_TYPE) -DDISTRO_MAJ_VERSION=$(DISTRO_MAJ_VERSION) -DDISTRO_MIN_VERSION=$(DISTRO_MIN_VERSION) -DDISTRO_IS_RHEL_LIKE=$(DISTRO_IS_RHEL_LIKE)
EXTRA_CFLAGS += -DAJALinux -DXENA2 $(DBG) -D$(NTV2TARGET) -D$(A_ARCH) $(EXTRA_DEPS) $(VERSIONING) $(DISTRO_INFO) $(INCLUDES) -Wall -Wno-implicit-fallthrough

ifdef AJA_HEVC
	EXTRA_CFLAGS += -DAJA_HEVC=$(AJA_HEVC)
endif

ifdef AJA_RDMA
ifeq ($(NVIDIA_SRC_DIR),)
	NVIDIA_SRC_DIR := $(shell find /usr/src/nvidia-* -name nv-p2p.h 2>/dev/null|head -1|xargs dirname 2>/dev/null)
endif
ifeq ($(NVIDIA_SRC_DIR),)
	NVIDIA_SRC_DIR := $(shell find /usr/src/linux-* -name nv-p2p.h 2>/dev/null|head -1|xargs dirname 2>/dev/null)
endif
	EXTRA_CFLAGS += -I$(NVIDIA_SRC_DIR) -DAJA_RDMA=$(AJA_RDMA)
ifdef AJA_IGPU
	EXTRA_CFLAGS += -DAJA_IGPU=$(AJA_IGPU)
else
ifeq ($(NVIDIA_SYMVERS),)
	NVIDIA_KO ?= $(shell find /lib/modules/$(KVERSION)/ -name 'nvidia*.ko'|grep -P 'nvidia(_[0-9]+)?.ko'|head -1)
	NVIDIA_GEN_SYMVERS := ./nvidia-ko-to-module-symvers $(NVIDIA_KO) $(DIR)/nvidia.symvers
	NVIDIA_SYMVERS := $(DIR)/nvidia.symvers
endif
endif
endif

ifdef AJA_CREATE_DEVICE_NODES
	EXTRA_CFLAGS += -DAJA_CREATE_DEVICE_NODES=$(AJA_CREATE_DEVICE_NODES)
endif

DRIVERSRCS = ntv2devicefeatures.c \
			ntv2driver.c \
			ntv2driverautocirculate.c \
			ntv2driverdbgmsgctl.c \
			ntv2dma.c \
			ntv2driverstatus.c \
			ntv2kona2.c \
			ntv2driverprocamp.c \
			ntv2drivertask.c \
			ntv2vpidfromspec.c \
			registerio.c \
			ntv2serial.c \
			../ntv2commonreg.c \
			../ntv2hdmiedid.c \
			../ntv2hdmiin.c \
			../ntv2hdmiin4.c \
			../ntv2infoframe.c \
			../ntv2hdmiout4.c \
			../ntv2displayid.c \
			../ntv2genlock.c \
			../ntv2mcap.c \
			../ntv2pciconfig.c \
			../ntv2system.c \
			../ntv2anc.c \
			../ntv2xpt.c \
			../ntv2kona.c \
			../ntv2vpid.c \
			../ntv2rp188.c \
			../ntv2setup.c \
			../ntv2stream.c \
			../ntv2genlock2.c \
			../ntv2videoraster.c

DRIVEROBJS = $(patsubst %.c,%.o,$(DRIVERSRCS))

DRIVERINCS = $(A_LIB_NTV2_INC)/ntv2audiodefines.h \
			$(A_LIB_NTV2_INC)/ntv2videodefines.h \
			$(A_LIB_NTV2_INC)/ntv2devicefeatures.hh \
			$(A_LIB_NTV2_INC)/ntv2driverprocamp.h \
			$(A_LINUX_DRIVER_PATH)/driverdbg.h \
			$(A_LINUX_DRIVER_PATH)/ntv2driver.h \
			$(A_LINUX_DRIVER_PATH)/ntv2driverautocirculate.h \
			$(A_LINUX_DRIVER_PATH)/ntv2dma.h \
			$(A_LINUX_DRIVER_PATH)/ntv2driverdbgmsgctl.h \
			$(A_LINUX_DRIVER_PATH)/ntv2driverstatus.h \
			$(A_LINUX_DRIVER_PATH)/ntv2drivertask.h \
			$(A_LIB_NTV2_INC)/ntv2vpidfromspec.h \
			$(A_LIB_NTV2_INC)/ntv2enums.h \
			$(A_LIB_NTV2_INC)/ntv2publicinterface.h \
			$(A_LIB_NTV2_SRC)/lin/ntv2linuxpublicinterface.h \
			$(A_LINUX_DRIVER_PATH)/../ntv2xptlookup.h \
			$(A_LINUX_DRIVER_PATH)/registerio.h \
			$(A_LINUX_DRIVER_PATH)/ntv2serial.h \
			$(A_DRIVER_PATH)/ntv2commonreg.h \
			$(A_DRIVER_PATH)/ntv2hdmiin.h \
			$(A_DRIVER_PATH)/ntv2hinreg.h \
			$(A_DRIVER_PATH)/ntv2hdmiin4.h \
			$(A_DRIVER_PATH)/ntv2hin4reg.h \
			$(A_DRIVER_PATH)/ntv2hdmiout4.h \
			$(A_DRIVER_PATH)/ntv2hout4reg.h \
			$(A_DRIVER_PATH)/ntv2hdmiedid.h \
			$(A_DRIVER_PATH)/ntv2infoframe.h \
			$(A_DRIVER_PATH)/ntv2genregs.h \
			$(A_DRIVER_PATH)/ntv2kona.h \
			$(A_DRIVER_PATH)/ntv2mcap.h \
			$(A_DRIVER_PATH)/ntv2pciconfig.h \
			$(A_DRIVER_PATH)/ntv2system.h \
			$(A_DRIVER_PATH)/ntv2vpid.h \
			$(A_DRIVER_PATH)/ntv2xpt.h \
			$(A_DRIVER_PATH)/ntv2anc.h \
			$(A_DRIVER_PATH)/ntv2setup.h

ifdef AJA_HEVC
HEVCSRCS =	hevcdriver.c \
			hevcparams.c \
			hevcregister.c \
			hevcapi.c \
			hevcinterrupt.c \
			hevccommand.c \
			hevcstream.c

HEVCOBJS = $(patsubst %.c,%.o,$(HEVCSRCS))

HEVCINCS =	$(A_LINUX_DRIVER_PATH)/hevcpublic.h \
			$(A_LINUX_DRIVER_PATH)/hevccommon.h \
			$(A_LINUX_DRIVER_PATH)/hevcdriver.h \
			$(A_LINUX_DRIVER_PATH)/hevcparams.h \
			$(A_LINUX_DRIVER_PATH)/hevcconstants.h \
			$(A_LINUX_DRIVER_PATH)/hevcregister.h \
			$(A_LINUX_DRIVER_PATH)/hevcinterrupt.h \
			$(A_LINUX_DRIVER_PATH)/hevccommand.h \
			$(A_LINUX_DRIVER_PATH)/hevcstream.h
else
HEVCSRCS =
HEVCOBJS =
HEVCINCS =
endif

ifdef AJA_RDMA
RDMAINCS =	$(NVIDIA_SRC_DIR)/nv-p2p.h
else
RDMAINCS =
endif

obj-m				:= $(DRIVERTARGET)
$(NTV2TARGET)-objs 	:= $(DRIVEROBJS) $(HEVCOBJS)

default: all
	@echo linux distro: $(DISTRO_TYPE)
	@echo linux distro flags: '$(DISTRO_INFO)'
	@echo lib: $(LIB)
ifdef AJA_RDMA
ifdef AJA_IGPU
	$(MAKE) -C $(KDIR) M=$(A_LINUX_DRIVER_PATH) DRIVERDIR=$(A_LINUX_DRIVER_PATH) modules
else
	${NVIDIA_GEN_SYMVERS}
	$(MAKE) -C $(KDIR) M=$(A_LINUX_DRIVER_PATH) DRIVERDIR=$(A_LINUX_DRIVER_PATH) modules KBUILD_EXTRA_SYMBOLS=$(NVIDIA_SYMVERS)
endif
else
	$(MAKE) -C $(KDIR) M=$(A_LINUX_DRIVER_PATH) DRIVERDIR=$(A_LINUX_DRIVER_PATH) modules
endif
	$(call ensure_dir_exists, $(A_DRIVER_PATH)/bin)
	cp $(NTV2TARGET).ko $(A_DRIVER_PATH)/bin

modules_install:
	$(MAKE) -C $(KDIR) M=$(A_LINUX_DRIVER_PATH) modules_install

ntv2version_gen:
	@if [ ! -e "$(A_LIB_NTV2_INC)/ntv2version.h" ]; then \
		printf "\ngenerating: $(A_LIB_NTV2_INC)/ntv2version.h\n"; \
		if [ -e "$(A_LIB_NTV2_INC)/ntv2version.h.in" ]; then \
			awk '\
				sub("@AJA_NTV2_SDK_VERSION_MAJOR@", "$(SDKVER_MAJ)") {print;next} \
				sub("@AJA_NTV2_SDK_VERSION_MINOR@", "$(SDKVER_MIN)") {print;next} \
				sub("@AJA_NTV2_SDK_VERSION_POINT@", "$(SDKVER_PNT)") {print;next} \
				sub("@AJA_NTV2_SDK_BUILD_NUMBER@", "$(SDKVER_BLD)") {print;next} \
				sub("@AJA_NTV2_SDK_BUILD_TYPE@", "$(SDKVER_STR)") {print;next} \
				sub("@AJA_NTV2_SDK_BUILD_DATETIME@", "$(shell date -u +%Y-%m-%dT%H:%M:%SZ)") {print;next} \
				{print} \
			' "$(A_LIB_NTV2_INC)/ntv2version.h.in" > "$(A_LIB_NTV2_INC)/ntv2version.h"; \
		fi \
	fi

.PHONY: all
all: ntv2version_gen $(DRIVERINCS) $(DRIVERSRCS) $(HEVCINCS) $(HEVCSRCS) $(RDMAINCS)

# Make a symlink to the C-only CPP files we need in classes.

ntv2devicefeatures.c:
	ln -sf $(A_LIB_NTV2_SRC)/ntv2devicefeatures.cpp $(A_LINUX_DRIVER_PATH)/ntv2devicefeatures.c

ntv2driverprocamp.c: 
	ln -sf $(A_LIB_NTV2_SRC)/ntv2driverprocamp.cpp $(A_LINUX_DRIVER_PATH)/ntv2driverprocamp.c

ntv2vpidfromspec.c: 
	ln -sf $(A_LIB_NTV2_SRC)/ntv2vpidfromspec.cpp $(A_LINUX_DRIVER_PATH)/ntv2vpidfromspec.c

.PHONY: clean cleandeps realclean

clean:
	rm -f $(A_DRIVER_PATH)/bin/$(NTV2TARGET).ko $(A_UBER_BIN)/*.ko $(A_DRIVER_PATH)/*.o $(A_DRIVER_PATH)/.*o.cmd *.ko *.o .*o.cmd *.mod.c *~ errors.txt semantic.cache
	rm -f *.markers *.symvers *.order *.o.ur-safe *.mod
	rm -f ntv2devicefeatures.*
	rm -f ntv2driverprocamp.c
	rm -f ntv2vpidfromspec.c
	rm -f TAGS
	rm -rf .tmp_versions
	rm -rf .cache.mk
	rm -rf nvidia.symvers

cleandeps: clean
realclean: clean

etags:
	etags *.h *.c ../*.h ../*.c $(A_LIB_NTV2_INC)/*.h

