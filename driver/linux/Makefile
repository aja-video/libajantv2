# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2023 AJA Video Systems, Inc.
#

# Makefile for AJA Linux Video NTV2 Driver
DIR := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))

#DRIVERDIR	?= $(DIR)

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
DISTRO_INFO  = -DDISTRO_TYPE=$(DISTRO_TYPE) -DDISTRO_IS_RHEL_LIKE=$(DISTRO_IS_RHEL_LIKE) \
			   -DDISTRO_MAJ_VERSION=$(DISTRO_MAJ_VERSION) -DDISTRO_MIN_VERSION=$(DISTRO_MIN_VERSION)  \
			   -DDISTRO_KERNEL_PKG_MAJ=$(DISTRO_KERNEL_PKG_MAJ) -DDISTRO_KERNEL_PKG_MIN=$(DISTRO_KERNEL_PKG_MIN) -DDISTRO_KERNEL_PKG_PNT=$(DISTRO_KERNEL_PKG_PNT)
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

AJANTV2INCS = $(A_LIB_NTV2_INC)/ajaexport.h \
			$(A_LIB_NTV2_INC)/ajatypes.h \
			$(A_LIB_NTV2_INC)/ntv2audiodefines.h \
			$(A_LIB_NTV2_INC)/ntv2devicefeatures.h \
			$(A_LIB_NTV2_INC)/ntv2devicefeatures.hh \
			$(A_LIB_NTV2_INC)/ntv2driverprocamp.h \
			$(A_LIB_NTV2_INC)/ntv2enums.h \
			$(A_LIB_NTV2_INC)/ntv2fixed.h \
			$(A_LIB_NTV2_INC)/ntv2publicinterface.h \
			$(A_LIB_NTV2_INC)/ntv2version.h \
			$(A_LIB_NTV2_INC)/ntv2videodefines.h \
			$(A_LIB_NTV2_INC)/ntv2virtualregisters.h \
			$(A_LIB_NTV2_INC)/ntv2vpidfromspec.h

AJANTV2SRCLIN = $(A_LIB_NTV2_SRC)/lin/ntv2linuxpublicinterface.h

AJANTV2SRCS = $(A_LIB_NTV2_SRC)/ntv2devicefeatures.cpp \
			$(A_LIB_NTV2_SRC)/ntv2devicefeatures.hpp \
			$(A_LIB_NTV2_SRC)/ntv2driverprocamp.cpp \
			$(A_LIB_NTV2_SRC)/ntv2vpidfromspec.cpp

DRIVERMK = ../configure.mk \
			../sdkversion.mk

DRIVERINCS = ../ntv2anc.h \
			../ntv2autofunc.h \
			../ntv2aux.h \
			../ntv2commonreg.h \
			../ntv2displayid.h \
			../ntv2gen2regs_8a34045.h \
			../ntv2gen2regs.h \
			../ntv2gen2regs_rc32012a.h \
			../ntv2genlock2.h \
			../ntv2genlock.h \
			../ntv2genregs.h \
			../ntv2hdmiedid.h \
			../ntv2hdmiin4.h \
			../ntv2hdmiin.h \
			../ntv2hdmiout4.h \
			../ntv2hin4reg.h \
			../ntv2hinreg.h \
			../ntv2hout4reg.h \
			../ntv2infoframe.h \
			../ntv2kona.h \
			../ntv2mcap.h \
			../ntv2pciconfig.h \
			../ntv2rp188.h \
			../ntv2setup.h \
			../ntv2stream.h \
			../ntv2system.h \
			../ntv2videoraster.h \
			../ntv2videorasterreg.h \
			../ntv2vpid.h \
			../ntv2xpt.h \
			../ntv2xptlookup.h

DRIVERSRCS = ../ntv2anc.c \
			../ntv2aux.c \
			../ntv2commonreg.c \
			../ntv2displayid.c \
			../ntv2genlock2.c \
			../ntv2genlock.c \
			../ntv2hdmiedid.c \
			../ntv2hdmiin4.c \
			../ntv2hdmiin.c \
			../ntv2hdmiout4.c \
			../ntv2infoframe.c \
			../ntv2kona.c \
			../ntv2mcap.c \
			../ntv2pciconfig.c \
			../ntv2rp188.c \
			../ntv2setup.c \
			../ntv2stream.c \
			../ntv2system.c \
			../ntv2videoraster.c \
			../ntv2vpid.c \
			../ntv2xpt.c

DRIVERLINMK = Makefile \
				nvidia-ko-to-module-symvers

DRIVERLININCS = buildenv.h \
				driverdbg.h \
				fs1wait.h \
				ntv2dma.h \
				ntv2driverautocirculate.h \
				ntv2driverbigphysarea.h \
				ntv2driverdbgmsgctl.h \
				ntv2driver.h \
				ntv2driverstatus.h \
				ntv2drivertask.h \
				ntv2kona2.h \
				ntv2serial.h \
				registerio.h

DRIVERLINSRCS = ntv2dma.c \
				ntv2driverautocirculate.c \
				ntv2driver.c \
				ntv2driverdbgmsgctl.c \
				ntv2driverstatus.c \
				ntv2drivertask.c \
				ntv2kona2.c \
				ntv2serial.c \
				registerio.c

ifdef AJA_HEVC
HEVCSRCS = hevcdriver.c \
			hevcparams.c \
			hevcregister.c \
			hevcapi.c \
			hevcinterrupt.c \
			hevccommand.c \
			hevcstream.c

HEVCINCS = $(A_LINUX_DRIVER_PATH)/hevcpublic.h \
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
HEVCINCS =
endif

ifdef AJA_RDMA
RDMAINCS = $(NVIDIA_SRC_DIR)/nv-p2p.h
else
RDMAINCS =
endif

ALLDRIVERINCS =$(AJANTV2_INCS) $(AJANTV2SRCLIN) $(DRIVERINCS) $(DRIVERLININCS) $(HEVCSINCS) $(RDMAINCS)
ALLDRIVERSRCS =$(DRIVERSRCS) $(DRIVERLINSRCS) $(HEVCSRCS) ntv2devicefeatures.c ntv2driverprocamp.c ntv2vpidfromspec.c
ALLDRIVEROBJS =$(patsubst %.c,%.o,$(ALLDRIVERSRCS))

$(NTV2TARGET)-objs := $(ALLDRIVEROBJS)
obj-m				:= $(DRIVERTARGET)

DKMSVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT)
DKMSDIR = ajantv2-$(DKMSVER)
ifeq ($(wildcard /usr/sbin/dkms), /usr/sbin/dkms)
	DKMSINSTALLED = 1
else
	DKMSINSTALLED = $(shell command -v dkms 2> /dev/null)
endif

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
all: ntv2version_gen $(ALLDRIVERINCS) $(ALLDRIVERSRCS)

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
	rm -rf tmp

cleandeps: clean
realclean: clean

etags:
	etags *.h *.c ../*.h ../*.c $(A_LIB_NTV2_INC)/*.h

dkms-pkg:
	@printf "\n## packaging up DKMS driver\n"
	@rm -rf tmp/$(DKMSDIR)
	@mkdir -p tmp/$(DKMSDIR)/libajantv2/driver/linux
	@mkdir -p tmp/$(DKMSDIR)/libajantv2/ajantv2/includes
	@mkdir -p tmp/$(DKMSDIR)/libajantv2/ajantv2/src/lin
	@for i in "$(AJANTV2INCS)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/ajantv2/includes/ ; \
	done
	@for i in "$(AJANTV2SRCLIN)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/ajantv2/src/lin/ ; \
	done
	@for i in "$(AJANTV2SRCS)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/ajantv2/src/ ; \
	done
	@for i in "$(DRIVERMK)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/driver ; \
	done
	@for i in "$(DRIVERINCS)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/driver ; \
	done
	@for i in "$(DRIVERSRCS)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/driver ; \
	done
	@for i in "$(DRIVERLINMK)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/driver/linux ; \
	done
	@for i in "$(DRIVERLININCS)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/driver/linux ; \
	done
	@for i in "$(DRIVERLINSRCS)" ; do \
		cp $$i tmp/$(DKMSDIR)/libajantv2/driver/linux ; \
	done
	@awk '\
		sub("@AJA_DKMS_PKG_VERSION@", "$(DKMSVER)") {print;next} \
		{print} \
	' dkms.conf > "tmp/$(DKMSDIR)/dkms.conf"

dkms-install: dkms-pkg
	@if [ -n "$(DKMSINSTALLED)" ]; then \
		printf "\n## installing DKMS driver:\n"; \
		sudo cp -a tmp/$(DKMSDIR) /usr/src/; \
		sudo dkms install ajantv2/$(DKMSVER); \
		sudo modprobe ajantv2; \
	else \
		echo "!! DKMS system is not installed"; \
	fi

dkms-uninstall:
	@if [ -n "$(DKMSINSTALLED)" ]; then \
		printf "\n## uninstalling DKMS driver:\n"; \
		sudo dkms remove ajantv2/$(DKMSVER) --all; \
		sudo rm -rf /usr/src/ajantv2-$(DKMSVER); \
		sudo rmmod ajantv2; \
	else \
		echo "!! DKMS system is not installed"; \
	fi
