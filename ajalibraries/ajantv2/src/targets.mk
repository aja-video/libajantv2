# Copyright (C) 2004, 2005, 2006, 2007, 2008 AJA Video Systems, Inc.
# Proprietary and Confidential information.
# All righs reserved
#
# Here's the idea behind this makefile approach
# http://make.paulandlesley.org/multi-arch.html
#

SUFFIXES:

include $(NTV2_ROOT)/configure.mk

MAKETARGET = $(MAKE) -C $@ -f $(CURDIR)/Makefile \
                  SRCDIR=$(CURDIR) $(MAKECMDGOALS)

.PHONY: $(OBJDIR)
$(OBJDIR):
	+@[ -d $@ ] || mkdir -p $@
	+@$(MAKETARGET)

Makefile : ;
%.mk :: ;

% :: $(OBJDIR) ; :

#.PHONY: clean
#clean:
#	rm -rf $(OBJDIR)

