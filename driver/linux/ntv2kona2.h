/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA devices.
//
////////////////////////////////////////////////////////////
//
// Filename: ntv2kona2.h
// Purpose:	 Header Kona2 specific functions.
//
///////////////////////////////////////////////////////////////

#ifndef NTV2KONA2_HEADER
#define NTV2KONA2_HEADER

#include "../ntv2system.h"

bool GetConverterOutFormat(ULWord deviceNumber, NTV2VideoFormat* format);
NTV2VideoFormat GetDeviceVideoFormat(ULWord deviceNumber, NTV2Channel channel);

uint32_t ntv2ReadRegCon32(Ntv2SystemContext* context, uint32_t regNum);
bool ntv2ReadRegMSCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t* regValue, RegisterMask mask, RegisterShift shift);
bool ntv2WriteRegCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue);
bool ntv2WriteRegMSCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue, RegisterMask mask, RegisterShift shift);
uint32_t ntv2ReadVirtRegCon32(Ntv2SystemContext* context, uint32_t regNum);
bool ntv2WriteVirtRegCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t data);

#endif
