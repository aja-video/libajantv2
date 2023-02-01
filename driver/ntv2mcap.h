/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2mcap.h
// Purpose:	 Xilinx bitfile loading
//
///////////////////////////////////////////////////////////////

#ifndef NTV2MCAP_HEADER
#define NTV2MCAP_HEADER

#include "ntv2system.h"

/* mcap register map */
#define MCAP_EXT_CAP_HEADER		0x00
#define MCAP_VEND_SPEC_HEADER	0x04
#define MCAP_FPGA_JTAG_ID		0x08
#define MCAP_FPGA_BIT_VERSION	0x0C
#define MCAP_STATUS				0x10
#define MCAP_CONTROL			0x14
#define MCAP_DATA				0x18
#define MCAP_READ_DATA_0		0x1C
#define MCAP_READ_DATA_1		0x20
#define MCAP_READ_DATA_2		0x24
#define MCAP_READ_DATA_3		0x28

/* mcap control register bit map */
#define MCAP_CTRL_CONFIG_ENABLE_MASK	(1 << 0)
#define MCAP_CTRL_READ_ENABLE_MASK		(1 << 1)
#define MCAP_CTRL_CONFIG_RESET_MASK		(1 << 4)
#define MCAP_CTRL_MODULE_RESET_MASK		(1 << 5)
#define MCAP_CTRL_PCI_REQUEST_MASK		(1 << 8)
#define MCAP_CTRL_BAR_ENABLE_MASK		(1 << 12)
#define MCAP_CTRL_WRITE_ENABLE_MASK		(1 << 16)

/* mcap status register bit map */
#define MCAP_STS_ERR_MASK				(1 << 0)
#define MCAP_STS_EOS_MASK				(1 << 1)
#define MCAP_STS_REG_READ_CMP_MASK		(1 << 4)
#define MCAP_STS_REG_READ_COUNT_MASK	(7 << 5)
#define MCAP_STS_REG_READ_COUNT_SHIFT	(5)
#define MCAP_STS_FIFO_OVERFLOW_MASK		(1 << 8)
#define MCAP_STS_FIFO_OCCUPANCY_MASK	(15 << 12)
#define MCAP_STS_FIFO_OCCUPANCY_SHIFT	(12)
#define MCAP_STS_CONFIG_RELEASE_MASK	(1 << 24)

/* mcap pci extended capability id */
#define MCAP_EXT_CAP_ID			0xB

/* mcap timeout counts */
#define MCAP_EOS_RETRY_COUNT 	10
#define MCAP_EOS_LOOP_COUNT 	100

/* mcap bit data fill */	
#define MCAP_NOOP_VAL			0x2000000

#define NTV2_MCAP_STRING_SIZE	80


struct ntv2_mcap {
	char					name[NTV2_MCAP_STRING_SIZE];
	Ntv2SystemContext* 		system_context;
	int32_t					reg_base;
	bool					fragment;
};

struct ntv2_mcap *ntv2_mcap_open(Ntv2SystemContext *sys_con, const char *name);
void ntv2_mcap_close(struct ntv2_mcap *ntv2_mc);

Ntv2Status ntv2_mcap_configure(struct ntv2_mcap *ntv2_mc);

Ntv2Status ntv2_mcap_config_reset(struct ntv2_mcap *ntv2_mc);
Ntv2Status ntv2_mcap_module_reset(struct ntv2_mcap *ntv2_mc);
Ntv2Status ntv2_mcap_full_reset(struct ntv2_mcap *ntv2_mc);

Ntv2Status ntv2_mcap_write_bitstream(struct ntv2_mcap *ntv2_mc, void* data,
									 uint32_t bytes, bool fragment, bool swap);
	
Ntv2Status ntv2_mcap_read_register(struct ntv2_mcap *ntv2_mc, uint32_t offset, uint32_t* data);

#endif
