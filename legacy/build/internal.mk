# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
#

# makefile fragment for internal stuff that gets -indlude ed into the
# main, root level make.  The idea here is that we can hide the
# existence of things that we don't distribute to OEMs by includeing
# this into the main make.

#SUBDIRS += ajaclasses

CLEANDIRS := $(SUBDIRS)

#SUBDIRS += asdcplib

