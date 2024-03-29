
PWD			= $(shell pwd)
DRIVERDIR 		?= $(PWD)
KDIR  	  		?= /lib/modules/$(shell uname -r)/build
BINDIR 		  	= ../bin
DRIVERTARGET  		= ajardma.o
INCLUDES      		= -I. -I$(DRIVERDIR) -I$(DRIVERDIR)/..
EXTRA_CFLAGS    	+= $(INCLUDES) -Wall
LDFLAGS 		:= 
export LDFLAGS
NTV2_SYMVERS	= $(DRIVERDIR)/../linux/Module.symvers

DRIVERSRCS = main.c
DRIVEROBJS = main.o
#DRIVERINCS = 

obj-m		:= $(DRIVERTARGET)
ajardma-y	:= $(DRIVEROBJS)

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
	NVIDIA_GEN_SYMVERS := ./nvidia-ko-to-module-symvers $(NVIDIA_KO) $(DRIVERDIR)/nvidia.symvers
	NVIDIA_SYMVERS := $(DRIVERDIR)/nvidia.symvers
endif
endif
endif

ifdef AJA_RDMA
RDMAINCS = $(NVIDIA_SRC_DIR)/nv-p2p.h
else
RDMAINCS =
endif

default: $(DRIVERSRCS) $(DRIVERINCS) 
	@echo linux distro: $(DISTRO_TYPE)
	@echo linux distro flags: '$(DISTRO_INFO)'
	@echo lib: $(LIB)
ifdef AJA_RDMA
ifdef AJA_IGPU
	$(MAKE) -C $(KDIR) M=$(DRIVERDIR) DRIVERDIR=$(DRIVERDIR) modules
else
	${NVIDIA_GEN_SYMVERS}
	$(MAKE) -C $(KDIR) M=$(DRIVERDIR) DRIVERDIR=$(DRIVERDIR) modules KBUILD_EXTRA_SYMBOLS='$(NVIDIA_SYMVERS) $(NTV2_SYMVERS)'
endif
else
	$(MAKE) -C $(KDIR) M=$(DRIVERDIR) DRIVERDIR=$(DRIVERDIR) modules
endif
	$(call ensure_dir_exists, $(A_DRIVER_PATH)/bin)
	cp $(NTV2TARGET).ko $(A_DRIVER_PATH)/bin

all: default

.PHONY: clean reload all

clean:
	rm -f $(BINDIR)/*.ko *.ko *.o .*o.cmd *.mod *.mod.c *~ errors.txt semantic.cache
	rm -rf .tmp_versions
	rm -f ../*.o ../.*o.cmd
	rm -f *.markers *.symvers *.order

