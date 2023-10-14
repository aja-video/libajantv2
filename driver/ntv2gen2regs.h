/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2gen2regs_8A34045.h
// Purpose:	 Genlock register config
//
///////////////////////////////////////////////////////////////

#ifndef NTV2GEN2REGS_HEADER
#define NTV2GEN2REGS_HEADER

struct ntv2_genlock2_data {
	uint32_t	size;
	uint16_t	addr;
	char		data[1000];
};

#include "ntv2gen2regs_8a34045.h"
#include "ntv2gen2regs_rc32012a.h"

#endif