
PWD			= $(shell pwd)
DRIVERDIR 		?= $(PWD)
KDIR  	  		?= /lib/modules/$(shell uname -r)/build
BINDIR 		  	= ../bin
DRIVERTARGET  		= ntv2_rdma.o
INCLUDES      		= -I. -I$(DRIVERDIR) -I$(DRIVERDIR)/..
EXTRA_CFLAGS    	+= $(INCLUDES) -Wall
LDFLAGS 		:= 
export LDFLAGS
KBUILD_EXTRA_SYMBOLS	= $(DRIVERDIR)/../linux/Module.symvers

DRIVERSRCS = main.c
DRIVEROBJS = main.o
#DRIVERINCS = 

obj-m		:= $(DRIVERTARGET)
ntv2_rdma-y	:= $(DRIVEROBJS)

default: $(DRIVERSRCS) $(DRIVERINCS) 
	$(MAKE) -C $(KDIR) M=$(DRIVERDIR) DRIVERDIR=$(DRIVERDIR) modules

all: default

.PHONY: clean reload all

clean:
	rm -f $(BINDIR)/*.ko *.ko *.o .*o.cmd *.mod.c *~ errors.txt semantic.cache
	rm -rf .tmp_versions
	rm -f ../*.o ../.*o.cmd
	rm -f *.markers *.symvers *.order

