# SPDX-License-Identifier: MIT
#
# Copyright (C) 2004 - 2023 AJA Video Systems, Inc.
#

#
# This file controls the SDK versioning
#
# Major, minor and point come from libajantv2/VERSION.txt. Bump that, not this.
# The fallback is for standalone driver builds with no VERSION.txt; keep it in step.
#
# The build number will be set by the TeamCity automated builder
#

# Resolves to libajantv2/VERSION.txt, relative to this file rather than the cwd.
NTV2_VERSION_FILE := $(strip $(shell dirname $(abspath $(lastword $(MAKEFILE_LIST)))))/../VERSION.txt
NTV2_VERSION := $(strip $(shell cat $(NTV2_VERSION_FILE) 2>/dev/null))

ifeq ($(NTV2_VERSION),)
  SDKVER_MAJ ?= 18
  SDKVER_MIN ?=  0
  SDKVER_PNT ?=  1
else
  SDKVER_MAJ ?= $(word 1,$(subst ., ,$(NTV2_VERSION)))
  SDKVER_MIN ?= $(word 2,$(subst ., ,$(NTV2_VERSION)))
  SDKVER_PNT ?= $(word 3,$(subst ., ,$(NTV2_VERSION)))
endif

ifeq ($(TC_BUILD_COUNTER),)
  SDKVER_BLD = 0
  OLD_SDK_VERSION_FORMAT = 1 
  export OLD_SDK_VERSION_FORMAT
  ifeq ($(AJA_DEBUG),1)
    RELEASEVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT)D
	SDKVER_STR = "d"
  else
    RELEASEVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT)
    ifeq ($(AJA_BETA),1)
		SDKVER_STR = "b"
    else
		SDKVER_STR = ""
    endif
  endif
else
  SDKVER_BLD = $(TC_BUILD_COUNTER)
  ifeq ($(AJA_DEBUG),1)
    RELEASEVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT).$(SDKVER_BLD)D
	SDKVER_STR = "d"
  else
    RELEASEVER = $(SDKVER_MAJ).$(SDKVER_MIN).$(SDKVER_PNT).$(SDKVER_BLD)
    ifeq ($(AJA_BETA),1)
		SDKVER_STR = "b"
    else
		SDKVER_STR = ""
    endif
  endif
endif

export SDKVER_MAJ
export SDKVER_MIN
export SDKVER_PNT
export SDKVER_BLD
export SDKVER_STR

