/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2025 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2mailbox.h
// Purpose:	 Mailbox buffer send and receive
//
///////////////////////////////////////////////////////////////

#ifndef NTV2MAILBOX_HEADER
#define NTV2MAILBOX_HEADER

#include "ntv2system.h"

#define NTV2_MAILBOX_STRING_SIZE	80


struct ntv2_mailbox {
	int						index;
	char					name[NTV2_MAILBOX_STRING_SIZE];
	Ntv2SystemContext* 		system_context;
    uint32_t                reg_base;
};

struct ntv2_mailbox *ntv2_mailbox_open(Ntv2SystemContext* sys_con,
									   const char *name, int index);
void ntv2_mailbox_close(struct ntv2_mailbox *ntv2_hin);

Ntv2Status ntv2_mailbox_configure(struct ntv2_mailbox* ntv2_mail,
								  uint32_t reg_base);

Ntv2Status ntv2_mailbox_send(struct ntv2_mailbox *ntv2_mail,
                             uint8_t* buffer, uint32_t size, uint32_t* offset);

Ntv2Status ntv2_mailbox_recv(struct ntv2_mailbox *ntv2_mail,
                             uint8_t* buffer, uint32_t size, uint32_t* offset);

Ntv2Status ntv2_packet_send(struct ntv2_mailbox *ntv2_mail,
                            uint8_t* buffer, uint32_t size, uint32_t* offset,
                            uint32_t delay, uint32_t timeout);

Ntv2Status ntv2_packet_recv(struct ntv2_mailbox *ntv2_mail,
                            uint8_t* buffer, uint32_t size, uint32_t* offset,
                            uint32_t delay, uint32_t timeout);

#endif
