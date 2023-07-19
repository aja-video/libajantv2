/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2genlock2.h
// Purpose:	 Genlock2 monitor
//
///////////////////////////////////////////////////////////////

#ifndef NTV2GENLOCK2_HEADER
#define NTV2GENLOCK2_HEADER

#include "ntv2system.h"

#define NTV2_GENLOCK2_STRING_SIZE	80

enum ntv2_genlock2_mode {
	ntv2_genlock2_mode_unknown,
	ntv2_genlock2_mode_zero,
	ntv2_genlock2_mode_broadcast_1485,
	ntv2_genlock2_mode_size
};

struct ntv2_genlock2 {
	int					index;
	char				name[NTV2_GENLOCK2_STRING_SIZE];
	Ntv2SystemContext* 	system_context;
	Ntv2SpinLock		state_lock;

	Ntv2Thread 			monitor_task;
	bool				monitor_enable;
	Ntv2Event			monitor_event;

	uint32_t			ref_source;
	uint32_t			gen_source;
	bool				ref_locked;
	bool				gen_locked;
	uint32_t			ref_lines;
	uint32_t			ref_rate;

	uint32_t			gen_lines;
	uint32_t			gen_rate;

	uint8_t				page_address;
};

struct ntv2_genlock2 *ntv2_genlock2_open(Ntv2SystemContext* sys_con,
									   const char *name, int index);
void ntv2_genlock2_close(struct ntv2_genlock2 *ntv2_gen);

Ntv2Status ntv2_genlock2_configure(struct ntv2_genlock2 *ntv2_gen);

Ntv2Status ntv2_genlock2_enable(struct ntv2_genlock2 *ntv2_gen);
Ntv2Status ntv2_genlock2_disable(struct ntv2_genlock2 *ntv2_gen);

Ntv2Status ntv2_genlock2_program(struct ntv2_genlock2 *ntv2_gen,
								enum ntv2_genlock2_mode mode);

#endif
