/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2setup.h
// Purpose:	 Monitor and setup outputs, vpid and other stuff
//
///////////////////////////////////////////////////////////////

#ifndef NTV2SETUP_HEADER
#define NTV2SETUP_HEADER

#include "ntv2system.h"

#define NTV2_SETUP_STRING_SIZE	80

struct ntv2_setup {
	int					index;
	char				name[NTV2_SETUP_STRING_SIZE];
	Ntv2SystemContext* 	system_context;
	Ntv2SpinLock		state_lock;

	Ntv2Thread 			monitor_task;
	bool				monitor_enable;
	Ntv2Event			monitor_event;
};

struct ntv2_setup *ntv2_setup_open(Ntv2SystemContext* sys_con, const char *name);
void ntv2_setup_close(struct ntv2_setup *ntv2_setterupper);
Ntv2Status ntv2_setup_configure(struct ntv2_setup *ntv2_setterupper);
Ntv2Status ntv2_setup_enable(struct ntv2_setup *ntv2_setterupper);
Ntv2Status ntv2_setup_disable(struct ntv2_setup *ntv2_setterupper);

#endif
