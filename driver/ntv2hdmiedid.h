/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2hdmiedid.h
// Purpose:	 HDMI edid repository
//
///////////////////////////////////////////////////////////////

#ifndef NTV2HDMIEDID_HEADER
#define NTV2HDMIEDID_HEADER

#include "ntv2system.h"

#define NTV2_HDMIEDID_SIZE			256
#define NTV2_HDMIEDID_STRING_SIZE	80

enum ntv2_edid_type {
	ntv2_edid_type_unknown,
	ntv2_edid_type_konahdmi_20,
	ntv2_edid_type_konahdmi_13,
	ntv2_edid_type_corvidhbr,
	ntv2_edid_type_io4k,
	ntv2_edid_type_io4kplus,
	ntv2_edid_type_iox3,
	ntv2_edid_type_size
};

struct ntv2_hdmiedid {
	int						index;
	char					name[NTV2_HDMIEDID_STRING_SIZE];
	Ntv2SystemContext* 		system_context;

	enum ntv2_edid_type		edid_type;
	int						port_index;

	uint8_t					edid_data[NTV2_HDMIEDID_SIZE];
	uint32_t				edid_size;
};

struct ntv2_hdmiedid *ntv2_hdmiedid_open(Ntv2SystemContext* sys_con,
										 const char *name, int index);
void ntv2_hdmiedid_close(struct ntv2_hdmiedid *ntv2_hed);

Ntv2Status ntv2_hdmiedid_configure(struct ntv2_hdmiedid *ntv2_hed,
								   enum ntv2_edid_type type,
								   int port_index);

uint8_t *ntv2_hdmi_get_edid_data(struct ntv2_hdmiedid *ntv2_hed);
uint32_t ntv2_hdmi_get_edid_size(struct ntv2_hdmiedid *ntv2_hed);

#endif

