# SPDX-License-Identifier: MIT
# Makefile for ntv42dummy standalone kernel module

KVERSION ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVERSION)/build

# Paths relative to this directory
DRIVER_DIR := $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST))))/..

ccflags-y += -DAJALinux -DAJA_NTV42=1 -I$(DRIVER_DIR)

obj-m := ntv42dummy.o
ntv42dummy-objs := ntv42dummy_main.o ntv42device.o ntv42ioctl.o ntv42ioctl_event.o ntv42ioctl_dma.o ntv42regbatch.o

# Create symlinks to shared source files
ntv42dummy_main.c:
	ln -sf ntv42dummy.c ntv42dummy_main.c

ntv42device.c:
	ln -sf $(DRIVER_DIR)/ntv42device.c ntv42device.c

ntv42ioctl.c:
	ln -sf $(DRIVER_DIR)/ntv42ioctl.c ntv42ioctl.c

ntv42ioctl_event.c:
	ln -sf $(DRIVER_DIR)/ntv42ioctl_event.c ntv42ioctl_event.c

ntv42ioctl_dma.c:
	ln -sf $(DRIVER_DIR)/ntv42ioctl_dma.c ntv42ioctl_dma.c

ntv42regbatch.c:
	ln -sf $(DRIVER_DIR)/ntv42regbatch.c ntv42regbatch.c

.PHONY: all clean

all: ntv42dummy_main.c ntv42device.c ntv42ioctl.c ntv42ioctl_event.c ntv42ioctl_dma.c ntv42regbatch.c
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f ntv42dummy_main.c ntv42device.c ntv42ioctl.c ntv42ioctl_event.c ntv42ioctl_dma.c ntv42regbatch.c
