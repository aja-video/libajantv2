/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA boards.
//
////////////////////////////////////////////////////////////
//
// Filename: ntv2driverdbgmsgctl.c
// Purpose:	 Header file for dynamic debug message control
// Notes:
//
///////////////////////////////////////////////////////////////

#ifndef NTV2DRIVERDEBUGMSGCTL_H
#define NTV2DRIVERDEBUGMSGCTL_H

bool MsgsEnabled(NTV2_DriverDebugMessageSet msgSet);

int ControlDebugMessages(	NTV2_DriverDebugMessageSet msgSet, 
	  			   		bool enable);

void ShowDebugMessageControl(NTV2_DriverDebugMessageSet msgSet);
#endif

