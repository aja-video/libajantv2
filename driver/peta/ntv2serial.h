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
// Filename: ntv2serial.h
// Purpose:	 Serial port control header.
//
///////////////////////////////////////////////////////////////

#ifndef NTV2SERIAL_HEADER
#define NTV2SERIAL_HEADER

#include "ntv2system.h"

#define NTV2_TTY_NAME				"ttyNTV"

struct ntv2_serial {
	int								index;
	char							name[80];
	Ntv2SystemContext* 				system_context;
	Ntv2Register					uart_reg;
	Ntv2Register					route_reg;
	u32								route_mask;

	bool							uart_enable;
	spinlock_t 						state_lock;

	struct uart_port 				uart_port;
	bool							busy;
};

struct ntv2_serial *ntv2_serial_open(Ntv2SystemContext* sys_con,
									 const char *name, int index);
void ntv2_serial_close(struct ntv2_serial *ntv2_ser);

int ntv2_serial_configure(struct ntv2_serial *ntv2_ser,
						  Ntv2Register uart_reg,
						  Ntv2Register route_reg,
						  u32 route_mask);

int ntv2_serial_enable(struct ntv2_serial *ntv2_ser);
int ntv2_serial_disable(struct ntv2_serial *ntv2_ser);
bool ntv2_serial_active(struct ntv2_serial *ntv2_ser);

int ntv2_serial_interrupt(struct ntv2_serial *ntv2_ser);

#endif
