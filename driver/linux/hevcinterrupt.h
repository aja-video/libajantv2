/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// HEVC Linux Device Driver for AJA HEVC devices.
//
////////////////////////////////////////////////////////////
//
// Filename: hevcinterrupt.h
// Purpose:	 HEVC interrupt service routines
// Notes:	 
//
///////////////////////////////////////////////////////////////

#ifndef HEVCINTERRUPT_H
#define HEVCINTERRUPT_H

#include "hevccommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

// initialize and release interrupt resources
void hevcInterruptInitialize(uint32_t devNum);
void hevcInterruptRelease(uint32_t devNum);

// the interrupt handler
bool hevcInterrupt(uint32_t devNum);

#ifdef __cplusplus
}
#endif

#endif
