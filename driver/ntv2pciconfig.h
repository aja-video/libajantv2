/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2pciconfig.h
// Purpose:	 PCI configuration space utility
//
///////////////////////////////////////////////////////////////

#ifndef NTV2PCICONFIG_HEADER
#define NTV2PCICONFIG_HEADER

#include "ntv2system.h"

int32_t		ntv2PciFindCapability(Ntv2SystemContext* pSysCon, uint32_t cap_id);
int32_t		ntv2PciFindExtCapability(Ntv2SystemContext* pSysCon, uint32_t ext_id);
uint32_t	ntv2ReadPciMaxReadRequestSize(Ntv2SystemContext* pSysCon);
Ntv2Status	ntv2WritePciMaxReadRequestSize(Ntv2SystemContext* pSysCon, uint32_t reqSize);

#endif
