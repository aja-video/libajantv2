/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2vpid.h
// Purpose:	 Common VPID
//
///////////////////////////////////////////////////////////////

#ifndef NTV2VPID_HEADER
#define NTV2VPID_HEADER

#include "ntv2system.h"
#include "ntv2vpidfromspec.h"
#include "ntv2xptlookup.h"
#include "ntv2kona.h"

VPIDChannel GetChannelFrom425XPT(ULWord index);
	
bool ReadSDIInVPID(Ntv2SystemContext* context, NTV2Channel channel, ULWord* valueA, ULWord* valueB);

bool SetSDIOutVPID(Ntv2SystemContext* context, NTV2Channel channel, ULWord valueA, ULWord valueB);

bool AdjustFor4KDC(Ntv2SystemContext* context, VPIDControl* pControl);

bool FindVPID(Ntv2SystemContext* context, NTV2OutputXptID startingXpt, VPIDControl* pControl);

bool SetVPIDOutput(Ntv2SystemContext* context, NTV2Channel channel);


#endif

