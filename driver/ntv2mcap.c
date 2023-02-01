/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2mcap.c
//
//==========================================================================

#include "ntv2mcap.h"
#include "ntv2pciconfig.h"
#include "ntv2publicinterface.h"

// debug messages
#define NTV2_DEBUG_INFO				0x00000001
#define NTV2_DEBUG_ERROR			0x00000002
#define NTV2_DEBUG_MCAP_STATE		0x00000004

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	(((ntv2_debug_mask | ntv2_user_mask) & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_MCAP_INFO(string, ...)				NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_MCAP_ERROR(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_MCAP_STATE(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_MCAP_STATE, string, __VA_ARGS__)

static uint32_t ntv2_debug_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR | NTV2_DEBUG_MCAP_STATE;
static uint32_t ntv2_user_mask = 0;

/* swap dword data */
#define ntv2_mcap_swap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

static Ntv2Status ntv2_mcap_request_control(struct ntv2_mcap *ntv2_mc, uint32_t* state);
static Ntv2Status ntv2_mcap_wait_control(struct ntv2_mcap *ntv2_mc);
static Ntv2Status ntv2_mcap_restore_control(struct ntv2_mcap *ntv2_mc, uint32_t state);	
static Ntv2Status ntv2_mcap_release_control(struct ntv2_mcap *ntv2_mc, uint32_t state);

static Ntv2Status ntv2_mcap_write_init(struct ntv2_mcap *ntv2_mc);
static Ntv2Status ntv2_mcap_write_data(struct ntv2_mcap *ntv2_mc,
									   uint32_t* data, uint32_t count, bool swap);
static Ntv2Status ntv2_mcap_write_eos(struct ntv2_mcap *ntv2_mc);
static Ntv2Status ntv2_mcap_wait_done(struct ntv2_mcap *ntv2_mc);

static bool ntv2_mcap_is_request_control(struct ntv2_mcap *ntv2_mc);
static bool ntv2_mcap_is_config_reset(struct ntv2_mcap *ntv2_mc);
static bool ntv2_mcap_is_module_reset(struct ntv2_mcap *ntv2_mc);
static bool ntv2_mcap_is_config_release(struct ntv2_mcap *ntv2_mc);
static bool ntv2_mcap_is_error(struct ntv2_mcap *ntv2_mc);
static bool ntv2_mcap_is_read_complete(struct ntv2_mcap *ntv2_mc);
static bool ntv2_mcap_is_fifo_overflow(struct ntv2_mcap *ntv2_mc);
static bool ntv2_mcap_is_eos(struct ntv2_mcap *ntv2_mc);
#if 0
static int32_t ntv2_mcap_get_read_count(struct ntv2_mcap *ntv2_mc);
#endif
static void ntv2_mcap_write(struct ntv2_mcap *ntv2_mc, int32_t offset, uint32_t value);
static uint32_t ntv2_mcap_read(struct ntv2_mcap *ntv2_mc, int32_t offset);

struct ntv2_mcap *ntv2_mcap_open(Ntv2SystemContext *sys_con,
								 const char *name)
{
	struct ntv2_mcap *ntv2_mc = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_mc = (struct ntv2_mcap *)ntv2MemoryAlloc(sizeof(struct ntv2_mcap));
	if (ntv2_mc == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_mcap instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_mc, 0, sizeof(struct ntv2_mcap));

#if defined(MSWindows)
	sprintf(ntv2_mc->name, "%s", name);
#else
	snprintf(ntv2_mc->name, NTV2_MCAP_STRING_SIZE, "%s", name);
#endif
	ntv2_mc->system_context = sys_con;

	NTV2_MSG_MCAP_INFO("%s: open ntv2_mcap\n", ntv2_mc->name);

	return ntv2_mc;
}

void ntv2_mcap_close(struct ntv2_mcap *ntv2_mc)
{
	if (ntv2_mc == NULL) 
		return;

	NTV2_MSG_MCAP_INFO("%s: close ntv2_mcap\n", ntv2_mc->name);

	memset(ntv2_mc, 0, sizeof(struct ntv2_mcap));
	ntv2MemoryFree(ntv2_mc, sizeof(struct ntv2_mcap));
}

Ntv2Status ntv2_mcap_configure(struct ntv2_mcap *ntv2_mc)
{
	uint32_t offset;

	if (ntv2_mc == NULL)
		return NTV2_STATUS_NO_RESOURCES;

	NTV2_MSG_MCAP_INFO("%s: configure mcap\n", ntv2_mc->name);

	offset = ntv2PciFindExtCapability(ntv2_mc->system_context, MCAP_EXT_CAP_ID);
	if (offset == 0)
	{
		NTV2_MSG_MCAP_ERROR("%s: extended capability not found\n", ntv2_mc->name);
		return NTV2_STATUS_FAIL;
	}

	ntv2_mc->reg_base = offset;
	
	NTV2_MSG_MCAP_INFO("%s: extended capability found  offset %03x\n",
					   ntv2_mc->name, ntv2_mc->reg_base);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_mcap_config_reset(struct ntv2_mcap *ntv2_mc)
{
	uint32_t state;
	uint32_t data;
	Ntv2Status status;
	
	if (ntv2_mc == NULL)
		return NTV2_STATUS_NO_RESOURCES;

	NTV2_MSG_MCAP_STATE("%s: reset config\n", ntv2_mc->name);
	
	status = ntv2_mcap_request_control(ntv2_mc, &state);
	if (status != NTV2_STATUS_SUCCESS)
		return status;

	status = ntv2_mcap_wait_control(ntv2_mc);
	if (status != NTV2_STATUS_SUCCESS)
	{
		NTV2_MSG_MCAP_STATE("%s: request control fail\n", ntv2_mc->name);
		goto bad;
	}
	
	data = ntv2_mcap_read(ntv2_mc, MCAP_CONTROL);
	data |= MCAP_CTRL_CONFIG_ENABLE_MASK | MCAP_CTRL_CONFIG_RESET_MASK;
	ntv2_mcap_write(ntv2_mc, MCAP_CONTROL, data);

	if (ntv2_mcap_is_error(ntv2_mc))
	{
		NTV2_MSG_MCAP_STATE("%s: reset config error\n", ntv2_mc->name);
		goto bad;
	}

	if (!ntv2_mcap_is_config_reset(ntv2_mc))
	{
		NTV2_MSG_MCAP_STATE("%s: reset config fail\n", ntv2_mc->name);
		goto bad;
	}

	ntv2_mcap_restore_control(ntv2_mc, state);

	return NTV2_STATUS_SUCCESS;

bad:	
	ntv2_mcap_restore_control(ntv2_mc, state);
	return NTV2_STATUS_FAIL;
}

Ntv2Status ntv2_mcap_module_reset(struct ntv2_mcap *ntv2_mc)
{
	uint32_t state;
	uint32_t data;
	Ntv2Status status = NTV2_STATUS_SUCCESS;
	
	if (ntv2_mc == NULL)
		return NTV2_STATUS_NO_RESOURCES;

	NTV2_MSG_MCAP_STATE("%s: reset module\n", ntv2_mc->name);

	status = ntv2_mcap_request_control(ntv2_mc, &state);
	if (status != NTV2_STATUS_SUCCESS)
		return status;

	status = ntv2_mcap_wait_control(ntv2_mc);
	if (status != NTV2_STATUS_SUCCESS)
	{
		NTV2_MSG_MCAP_STATE("%s: request control fail\n", ntv2_mc->name);
		goto bad;
	}
	
	data = ntv2_mcap_read(ntv2_mc, MCAP_CONTROL);
	data |= MCAP_CTRL_CONFIG_ENABLE_MASK | MCAP_CTRL_MODULE_RESET_MASK;
	ntv2_mcap_write(ntv2_mc, MCAP_CONTROL, data);

	if (ntv2_mcap_is_error(ntv2_mc))
	{
		NTV2_MSG_MCAP_STATE("%s: reset module error\n", ntv2_mc->name);
		goto bad;
	}

	if (!ntv2_mcap_is_module_reset(ntv2_mc))
	{
		NTV2_MSG_MCAP_STATE("%s: reset module fail\n", ntv2_mc->name);
		goto bad;
	}

	ntv2_mcap_restore_control(ntv2_mc, state);

	return NTV2_STATUS_SUCCESS;

bad:	
	ntv2_mcap_restore_control(ntv2_mc, state);
	return NTV2_STATUS_FAIL;
}

Ntv2Status ntv2_mcap_full_reset(struct ntv2_mcap *ntv2_mc)
{
	uint32_t state;
	uint32_t data;
	Ntv2Status status = NTV2_STATUS_SUCCESS;
	
	if (ntv2_mc == NULL)
		return NTV2_STATUS_NO_RESOURCES;

	NTV2_MSG_MCAP_STATE("%s: reset full\n", ntv2_mc->name);

	status = ntv2_mcap_request_control(ntv2_mc, &state);
	if (status != NTV2_STATUS_SUCCESS)
		return status;

	status = ntv2_mcap_wait_control(ntv2_mc);
	if (status != NTV2_STATUS_SUCCESS)
	{
		NTV2_MSG_MCAP_STATE("%s: request control fail\n", ntv2_mc->name);
		goto bad;
	}
	
	data = ntv2_mcap_read(ntv2_mc, MCAP_CONTROL);
	data |= MCAP_CTRL_CONFIG_ENABLE_MASK | MCAP_CTRL_CONFIG_RESET_MASK | MCAP_CTRL_MODULE_RESET_MASK;
	ntv2_mcap_write(ntv2_mc, MCAP_CONTROL, data);

	if (ntv2_mcap_is_error(ntv2_mc))
	{
		NTV2_MSG_MCAP_STATE("%s: reset full error\n", ntv2_mc->name);
		goto bad;
	}

	if (!ntv2_mcap_is_config_reset(ntv2_mc) || 
		!ntv2_mcap_is_module_reset(ntv2_mc))
	{
		NTV2_MSG_MCAP_STATE("%s: reset full fail\n", ntv2_mc->name);
		goto bad;
	}
	
	ntv2_mcap_restore_control(ntv2_mc, state);

	return NTV2_STATUS_SUCCESS;

bad:	
	ntv2_mcap_restore_control(ntv2_mc, state);
	return NTV2_STATUS_FAIL;
}

Ntv2Status ntv2_mcap_write_bitstream(struct ntv2_mcap *ntv2_mc, void* data,
									 uint32_t bytes, bool fragment, bool swap)
{
	uint32_t state = 0;
	Ntv2Status status;
	
	if (ntv2_mc == NULL)
		return NTV2_STATUS_NO_RESOURCES;
		
	if ((data == NULL) || (bytes == 0))
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_MCAP_STATE("%s: write bitstream  bytes %d  fragment %d  swap %d\n",
						ntv2_mc->name, bytes, fragment, swap);

	/* write status */
	ntv2WriteVirtualRegister(ntv2_mc->system_context, kVRegFlashSize, bytes);
	ntv2WriteVirtualRegister(ntv2_mc->system_context, kVRegFlashStatus , 0);
	ntv2WriteVirtualRegister(ntv2_mc->system_context, kVRegFlashState, kProgramStateProgramFlash);
	
	/* requeset mcap control */
	if(!ntv2_mcap_is_request_control(ntv2_mc))
	{
		NTV2_MSG_MCAP_STATE("%s:   request control\n", ntv2_mc->name);
		status = ntv2_mcap_request_control(ntv2_mc, &state);
		if (status != NTV2_STATUS_SUCCESS) goto bad;
	}

	status = ntv2_mcap_wait_control(ntv2_mc);
	if (status != NTV2_STATUS_SUCCESS)
	{
		NTV2_MSG_MCAP_STATE("%s:   request control fail\n", ntv2_mc->name);
		goto bad;
	}
	
	/* check for failure */
	if (ntv2_mcap_is_error(ntv2_mc) ||
		ntv2_mcap_is_read_complete(ntv2_mc) ||
		ntv2_mcap_is_fifo_overflow(ntv2_mc))
	{
		status = NTV2_STATUS_FAIL;
		goto bad;
	}

	if (!ntv2_mc->fragment)
	{
		NTV2_MSG_MCAP_STATE("%s:   write enable\n", ntv2_mc->name);
		status = ntv2_mcap_write_init(ntv2_mc);
		if (status != NTV2_STATUS_SUCCESS) goto bad;
	}

	NTV2_MSG_MCAP_STATE("%s:   write bitstream data\n", ntv2_mc->name);
	status = ntv2_mcap_write_data(ntv2_mc, (uint32_t*)data, bytes/4, swap);
	if (status != NTV2_STATUS_SUCCESS) goto bad;
	
	if (fragment)
	{
		NTV2_MSG_MCAP_STATE("%s:   write eos\n", ntv2_mc->name);
		status = ntv2_mcap_write_eos(ntv2_mc);
		if (status != NTV2_STATUS_SUCCESS) goto bad;
	}
	else
	{
		status = ntv2_mcap_wait_done(ntv2_mc);
		if (status != NTV2_STATUS_SUCCESS) goto bad;
		ntv2WriteVirtualRegister(ntv2_mc->system_context, kVRegFlashState, kProgramStateFinished);
		NTV2_MSG_MCAP_STATE("%s:   write done\n", ntv2_mc->name);
	}

	if (ntv2_mcap_is_error(ntv2_mc) ||
		ntv2_mcap_is_fifo_overflow(ntv2_mc))
	{
		status = NTV2_STATUS_FAIL;
		goto bad;
	}

	if (!fragment)
	{
		NTV2_MSG_MCAP_STATE("%s:   release control\n", ntv2_mc->name);
		ntv2_mcap_release_control(ntv2_mc, state);
	}

	ntv2_mc->fragment = fragment;

	NTV2_MSG_MCAP_STATE("%s: write bitstream suceeded\n", ntv2_mc->name);
	return NTV2_STATUS_SUCCESS;

bad:

	ntv2_mc->fragment = false;
	ntv2_mcap_restore_control(ntv2_mc, state);
	ntv2_mcap_full_reset(ntv2_mc);
	ntv2WriteVirtualRegister(ntv2_mc->system_context, kVRegFlashState, kProgramStateFinished);
	NTV2_MSG_MCAP_STATE("%s: write bitstream failed  status %08x\n",
						ntv2_mc->name, status);
	return status;
}

Ntv2Status ntv2_mcap_read_register(struct ntv2_mcap *ntv2_mc, uint32_t offset, uint32_t* data)
{
	if (ntv2_mc == NULL)
		return NTV2_STATUS_NO_RESOURCES;
		
	if ((data == NULL) || (offset > MCAP_READ_DATA_3))
		return NTV2_STATUS_BAD_PARAMETER;

	*data = ntv2_mcap_read(ntv2_mc, offset);

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_mcap_request_control(struct ntv2_mcap *ntv2_mc, uint32_t* state)
{
	uint32_t data;

	/* read current control state */
	data = ntv2_mcap_read(ntv2_mc, MCAP_CONTROL);
	*state = data;
	
	/* request control */
	data |= MCAP_CTRL_PCI_REQUEST_MASK;
	ntv2_mcap_write(ntv2_mc, MCAP_CONTROL, data);

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_mcap_wait_control(struct ntv2_mcap *ntv2_mc)
{
	uint32_t i;

	/* wait for access */
	for (i = 0; i < 10000; i++)
	{
		if (!ntv2_mcap_is_config_release(ntv2_mc))
			return NTV2_STATUS_SUCCESS;
		ntv2TimeSleep(100);
	}

	return NTV2_STATUS_TIMEOUT;
}

static Ntv2Status ntv2_mcap_restore_control(struct ntv2_mcap *ntv2_mc, uint32_t state)
{
	ntv2_mcap_write(ntv2_mc, MCAP_CONTROL, state);
	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_mcap_release_control(struct ntv2_mcap *ntv2_mc, uint32_t state)
{
	return ntv2_mcap_restore_control(ntv2_mc, state | MCAP_CTRL_BAR_ENABLE_MASK);
}

static Ntv2Status ntv2_mcap_write_init(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data;

	/* configure for write */
	data = ntv2_mcap_read(ntv2_mc, MCAP_CONTROL);
	data &= ~(MCAP_CTRL_CONFIG_RESET_MASK | MCAP_CTRL_MODULE_RESET_MASK |
			  MCAP_CTRL_READ_ENABLE_MASK | MCAP_CTRL_BAR_ENABLE_MASK);
	data |= MCAP_CTRL_CONFIG_ENABLE_MASK | MCAP_CTRL_WRITE_ENABLE_MASK;
	ntv2_mcap_write(ntv2_mc, MCAP_CONTROL, data);

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_mcap_write_data(struct ntv2_mcap *ntv2_mc,
									   uint32_t* data, uint32_t count, bool swap)
{
	uint32_t i = 0;
	
	if (swap)
	{
		for (i = 0; i < count; i++)
		{
			ntv2_mcap_write(ntv2_mc, MCAP_DATA, ntv2_mcap_swap_32(data[i]));
			ntv2WriteVirtualRegister(ntv2_mc->system_context, kVRegFlashStatus , (i + 1) * 4);
		}
	}
	else
	{
		for (i = 0; i < count; i++)
		{
			ntv2_mcap_write(ntv2_mc, MCAP_DATA, data[i]);
			ntv2WriteVirtualRegister(ntv2_mc->system_context, kVRegFlashStatus , (i + 1) * 4);
		}
	}

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_mcap_write_eos(struct ntv2_mcap *ntv2_mc)
{
	uint32_t i;

	for (i = 0; i < MCAP_EOS_LOOP_COUNT; i++)
		ntv2_mcap_write(ntv2_mc, MCAP_DATA, MCAP_NOOP_VAL);

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_mcap_wait_done(struct ntv2_mcap *ntv2_mc)
{
	uint32_t i;

	/* wait to check eos bit */
	ntv2TimeSleep(5);

	for (i = 0; i < MCAP_EOS_RETRY_COUNT; i++)
	{
		/* check eos bit */
		if (ntv2_mcap_is_eos(ntv2_mc))
			return NTV2_STATUS_SUCCESS;

		/* retry */
		ntv2_mcap_write_eos(ntv2_mc);
		ntv2TimeSleep(5);
	}
		
	return NTV2_STATUS_TIMEOUT;
}

static bool ntv2_mcap_is_request_control(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_CONTROL);
	return (data & MCAP_CTRL_PCI_REQUEST_MASK) != 0;
}

static bool ntv2_mcap_is_config_reset(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_CONTROL);
	return (data & MCAP_CTRL_CONFIG_RESET_MASK) != 0;
}

static bool ntv2_mcap_is_module_reset(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_CONTROL);
	return (data & MCAP_CTRL_MODULE_RESET_MASK) != 0;
}

static bool ntv2_mcap_is_config_release(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_STATUS);
	return (data & MCAP_STS_CONFIG_RELEASE_MASK) != 0;
}

static bool ntv2_mcap_is_error(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_STATUS);
	return (data & MCAP_STS_ERR_MASK) != 0;
}

static bool ntv2_mcap_is_read_complete(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_STATUS);
	return (data & MCAP_STS_REG_READ_CMP_MASK) != 0;
}

static bool ntv2_mcap_is_fifo_overflow(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_STATUS);
	return (data & MCAP_STS_FIFO_OVERFLOW_MASK) != 0;
}

static bool ntv2_mcap_is_eos(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_STATUS);
	return (data & MCAP_STS_EOS_MASK) != 0;
}
#if 0
static int32_t ntv2_mcap_get_read_count(struct ntv2_mcap *ntv2_mc)
{
	uint32_t data = ntv2_mcap_read(ntv2_mc, MCAP_STATUS);
	return (data & MCAP_STS_REG_READ_COUNT_MASK) >> MCAP_STS_REG_READ_COUNT_SHIFT;
}
#endif
static void ntv2_mcap_write(struct ntv2_mcap *ntv2_mc, int32_t offset, uint32_t value)
{
	Ntv2Status status;
	
	status = ntv2WritePciConfig(ntv2_mc->system_context, &value, ntv2_mc->reg_base + offset, 4);
	if (status != NTV2_STATUS_SUCCESS)
	{
		NTV2_MSG_MCAP_ERROR("%s: config write error  status %08x\n", ntv2_mc->name, status);
	}
}
	
static uint32_t ntv2_mcap_read(struct ntv2_mcap *ntv2_mc, int32_t offset)
{
	Ntv2Status status;
	uint32_t data;
	
	status = ntv2ReadPciConfig(ntv2_mc->system_context, &data, ntv2_mc->reg_base + offset, 4);
	if (status != NTV2_STATUS_SUCCESS)
	{
		NTV2_MSG_MCAP_ERROR("%s: config read error  status %08x\n", ntv2_mc->name, status);
		return 0;
	}

	return data;
}
