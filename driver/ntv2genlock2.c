/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2genlock2.c
//
//==========================================================================

#include "ntv2genlock2.h"
#include "ntv2commonreg.h"
#include "ntv2kona.h"
#include "ntv2gen2regs.h"

/* debug messages */
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002
#define NTV2_DEBUG_GENLOCK_STATE		0x00000004
#define NTV2_DEBUG_GENLOCK_DETECT		0x00000008
#define NTV2_DEBUG_GENLOCK_CONFIG		0x00000010
#define NTV2_DEBUG_GENLOCK_CHECK		0x00000020

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	((ntv2_debug_mask & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_GENLOCK_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_GENLOCK_ERROR(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_GENLOCK_STATE(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_GENLOCK_STATE, string, __VA_ARGS__)
#define NTV2_MSG_GENLOCK_DETECT(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_GENLOCK_DETECT, string, __VA_ARGS__)
#define NTV2_MSG_GENLOCK_CONFIG(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_GENLOCK_CONFIG, string, __VA_ARGS__)
#define NTV2_MSG_GENLOCK_CHECK(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_GENLOCK_CHECK, string, __VA_ARGS__)

static uint32_t ntv2_debug_mask =
	NTV2_DEBUG_INFO |
	NTV2_DEBUG_ERROR;

#define GENL_SPI_READ_FIFO_EMPTY			0x01
#define GENL_SPI_READ_FIFO_FULL				0x02
#define GENL_SPI_WRITE_FIFO_EMPTY			0x04
#define GENL_SPI_WRITE_FIFO_FULL			0x08

#define GENLOCK2_FRAME_RATE_SIZE		11

#define DTR_EMPTY	0x04

/* global control */
NTV2_REG(ntv2_reg_global_control,							0, 267, 377, 378, 379, 380, 381, 382, 383);	/* global control */
	NTV2_FLD(ntv2_fld_global_frame_rate_012,					3,	0);			/* frame rate 0-2 */
	NTV2_FLD(ntv2_fld_global_frame_geometry,					4,	3);			/* frame geometry */
	NTV2_FLD(ntv2_fld_global_video_standard,					3,	7);			/* video standard */
	NTV2_FLD(ntv2_fld_global_reference_source,					3,	10);		/* user reference source */
	NTV2_FLD(ntv2_fld_global_register_sync,						2,	20);		/* register updat sync */
	NTV2_FLD(ntv2_fld_global_frame_rate_3,						1,	21);		/* frame rate 3 */

/* control and status */
NTV2_REG(ntv2_reg_control_status,							48);			/* control status */
	NTV2_FLD(ntv2_fld_control_genlock_reset,					1,	6);			/* genlock reset */
	NTV2_FLD(ntv2_fld_control_reference_source,					4,	24);		/* hardware reference source */
	NTV2_FLD(ntv2_fld_control_reference_present,				1,	30);		/* reference source present */
	NTV2_FLD(ntv2_fld_control_genlock_locked,					1,	31);		/* genlock locked */

/* genlock spi control */
NTV2_REG(ntv2_reg_spi_ip_status,						0x8008);
	NTV2_FLD(ntv2_fld_ip_status_dtr_empty,					1,	2);
//NTV2_REG(ntv2_reg_spi_ip_control,						0x800a);
//	NTV2_FLD(ntv2_fld_ip_control_dtr_empty,					1,	2);
NTV2_REG(ntv2_reg_spi_reset,				0x8010);
NTV2_REG(ntv2_reg_spi_control,				0x8018);
NTV2_REG(ntv2_reg_spi_status,				0x8019);
NTV2_REG(ntv2_reg_spi_write,				0x801a);
NTV2_REG(ntv2_reg_spi_read,					0x801b);
NTV2_REG(ntv2_reg_spi_slave,				0x801c);

NTV2_REG(ntv2_reg_out_freq1, 0x36c1);
NTV2_REG(ntv2_reg_out_freq2, 0x36c2);
NTV2_REG(ntv2_reg_out_freq3, 0x36c3);
NTV2_REG(ntv2_reg_out_freq4, 0x36c4);
NTV2_REG(ntv2_reg_out_freq5, 0x36c5);

static const int64_t c_default_timeout		= 50000;
static const int64_t c_spi_timeout			= 10000;
static const int64_t c_genlock_config_wait	= 500000;

static const uint32_t c_ntsc_lines = 525;
static const uint32_t c_ntsc_rate = ntv2_frame_rate_2997;

static void ntv2_genlock2_monitor(void* data);
static Ntv2Status ntv2_genlock2_initialize(struct ntv2_genlock2 *ntv2_gen);
static bool has_state_changed(struct ntv2_genlock2 *ntv2_gen);
static struct ntv2_genlock2_data* get_genlock2_config(struct ntv2_genlock2 *ntv2_gen, uint32_t lines, uint32_t rate);
static bool configure_genlock2(struct ntv2_genlock2 *ntv2_gen, struct ntv2_genlock2_data *config, bool check);
static uint8_t device_id_read(struct ntv2_genlock2 *ntv2_gen);
static bool wait_genlock2(struct ntv2_genlock2 *ntv2_gen, uint32_t numMicrosSeconds);

static void spi_reset(struct ntv2_genlock2 *ntv2_gen);
static void spi_reset_fifos(struct ntv2_genlock2 *ntv2_gen);
static bool spi_wait_write_empty(struct ntv2_genlock2 *ntv2_gen);
static bool spi_genlock2_write(struct ntv2_genlock2 *ntv2_gen, uint32_t size, uint16_t addr, uint8_t* data, bool triggerWait);
static bool spi_genlock2_read(struct ntv2_genlock2 *ntv2_gen, uint16_t addr, uint8_t* data, uint32_t numBytes);
static void hex_to_bytes(char *hex, uint8_t *output, uint32_t array_length);

static uint32_t reg_read(struct ntv2_genlock2 *ntv2_gen, const uint32_t *reg);
static void reg_write(struct ntv2_genlock2 *ntv2_gen, const uint32_t *reg, uint32_t data);

struct ntv2_genlock2 *ntv2_genlock2_open(Ntv2SystemContext* sys_con,
									   const char *name, int index)
{
	struct ntv2_genlock2 *ntv2_gen = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_gen = (struct ntv2_genlock2 *)ntv2MemoryAlloc(sizeof(struct ntv2_genlock2));
	if (ntv2_gen == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_genlock instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_gen, 0, sizeof(struct ntv2_genlock2));

	ntv2_gen->index = index;
#if defined(MSWindows)
	sprintf(ntv2_gen->name, "%s%d", name, index);
#else
	snprintf(ntv2_gen->name, NTV2_GENLOCK2_STRING_SIZE, "%s%d", name, index);
#endif
	ntv2_gen->system_context = sys_con;

	ntv2SpinLockOpen(&ntv2_gen->state_lock, sys_con);
	ntv2ThreadOpen(&ntv2_gen->monitor_task, sys_con, "genlock2 monitor");
	ntv2EventOpen(&ntv2_gen->monitor_event, sys_con);

	NTV2_MSG_GENLOCK_INFO("%s: open ntv2_genlock2\n", ntv2_gen->name);

	return ntv2_gen;
}

void ntv2_genlock2_close(struct ntv2_genlock2 *ntv2_gen)
{
	if (ntv2_gen == NULL) 
		return;

	NTV2_MSG_GENLOCK_INFO("%s: close ntv2_genlock2\n", ntv2_gen->name);

	ntv2_genlock2_disable(ntv2_gen);

	ntv2EventClose(&ntv2_gen->monitor_event);
	ntv2ThreadClose(&ntv2_gen->monitor_task);
	ntv2SpinLockClose(&ntv2_gen->state_lock);

	memset(ntv2_gen, 0, sizeof(struct ntv2_genlock2));
	ntv2MemoryFree(ntv2_gen, sizeof(struct ntv2_genlock2));
}

Ntv2Status ntv2_genlock2_configure(struct ntv2_genlock2 *ntv2_gen)
{
	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_GENLOCK_INFO("%s: configure genlock2 device\n", ntv2_gen->name);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_genlock2_enable(struct ntv2_genlock2 *ntv2_gen)
{
	bool success;

	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (ntv2_gen->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_GENLOCK_STATE("%s: enable genlock2 monitor\n", ntv2_gen->name);

	ntv2EventClear(&ntv2_gen->monitor_event);
	ntv2_gen->monitor_enable = true;

	success = ntv2ThreadRun(&ntv2_gen->monitor_task, ntv2_genlock2_monitor, (void*)ntv2_gen);
	if (!success) {
		return NTV2_STATUS_FAIL;
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_genlock2_disable(struct ntv2_genlock2 *ntv2_gen)
{	
	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (!ntv2_gen->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_GENLOCK_STATE("%s: disable genlock2 monitor\n", ntv2_gen->name);

	ntv2_gen->monitor_enable = false;
	ntv2EventSignal(&ntv2_gen->monitor_event);

	ntv2ThreadStop(&ntv2_gen->monitor_task);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_genlock2_program(struct ntv2_genlock2 *ntv2_gen, enum ntv2_genlock2_mode mode)
{
	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	ntv2_gen->monitor_enable = true;
	
	switch (mode)
	{
	case ntv2_genlock2_mode_broadcast_1485:
	{
		struct ntv2_genlock2_data* configData = get_genlock2_config(ntv2_gen, c_ntsc_lines, c_ntsc_rate);
		if (!configure_genlock2(ntv2_gen, configData, true))
			return NTV2_STATUS_FAIL;
		break;
	}
	default:
		return NTV2_STATUS_BAD_PARAMETER;
	}

	return NTV2_STATUS_SUCCESS;
}

static void ntv2_genlock2_monitor(void* data)
{
	struct ntv2_genlock2 *ntv2_gen = (struct ntv2_genlock2 *)data;
	struct ntv2_genlock2_data *config = NULL;
	bool update = false;
	uint32_t lock_wait = 0;
	uint32_t unlock_wait = 0;
	Ntv2Status status;

	if (ntv2_gen == NULL)
		return;

	NTV2_MSG_GENLOCK_STATE("%s: genlock2 input monitor task start\n", ntv2_gen->name);

	status = ntv2_genlock2_initialize(ntv2_gen);
	if (status != NTV2_STATUS_SUCCESS) {
		NTV2_MSG_ERROR("%s: genlock2 initialization failed\n", ntv2_gen->name);
		goto exit;
	}

	while (!ntv2ThreadShouldStop(&ntv2_gen->monitor_task) && ntv2_gen->monitor_enable) 
	{
//		if ((reg_read(ntv2_gen, ntv2_reg_control_status) & 0x1) == 0)  goto sleep;

		if (has_state_changed(ntv2_gen)) 
		{
			NTV2_MSG_GENLOCK_DETECT("%s: new genlock2 state   ref %s %s   gen %s %s   lines %d  rate %s\n", 
									ntv2_gen->name,
									ntv2_ref_source_name(ntv2_gen->ref_source),
									ntv2_gen->ref_locked?"locked":"unlocked",
									ntv2_ref_source_name(ntv2_gen->gen_source),
									ntv2_gen->gen_locked?"locked":"unlocked",
									ntv2_gen->ref_lines,
									ntv2_frame_rate_name(ntv2_gen->ref_rate));
			update = true;
		}
#if 0
		if (ntv2_gen->ref_locked)
		{
			unlock_wait = 0;
			if ((ntv2_gen->gen_lines != ntv2_gen->ref_lines) ||
				(ntv2_gen->gen_rate != ntv2_gen->ref_rate))
			{
				lock_wait++;
				if (lock_wait > 5)
				{
					ntv2_gen->gen_lines = ntv2_gen->ref_lines;
					ntv2_gen->gen_rate = ntv2_gen->ref_rate;
					update = true;
				}
			}
		}
		else
		{
			lock_wait = 0;
			if ((ntv2_gen->gen_lines != c_ntsc_lines) ||
				(ntv2_gen->gen_rate != c_ntsc_rate))
			{
				unlock_wait++;
				if (unlock_wait > 5)
				{
					ntv2_gen->gen_lines = c_ntsc_lines;
					ntv2_gen->gen_rate = c_ntsc_rate;
					update = true;
				}
			}
		}
#endif

		if (update) {
			config = get_genlock2_config(ntv2_gen, ntv2_gen->gen_lines, ntv2_gen->gen_rate);
			if (config != NULL) {
				NTV2_MSG_GENLOCK_CONFIG("%s: configure genlock2 lines %d  rate %s\n", 
										ntv2_gen->name, ntv2_gen->gen_lines, ntv2_frame_rate_name(ntv2_gen->gen_rate));
				if (!configure_genlock2(ntv2_gen, config, true)) goto sleep;
				ntv2EventWaitForSignal(&ntv2_gen->monitor_event, c_genlock_config_wait, true);
				update = false;
				lock_wait = 0;
				unlock_wait = 0;
			}
		}

	sleep:
		ntv2EventWaitForSignal(&ntv2_gen->monitor_event, c_default_timeout, true);
	}

exit:
	NTV2_MSG_GENLOCK_STATE("%s: genlock2 monitor task stop\n", ntv2_gen->name);
	ntv2ThreadExit(&ntv2_gen->monitor_task);
	return;
}

static Ntv2Status ntv2_genlock2_initialize(struct ntv2_genlock2 *ntv2_gen)
{
	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	ntv2_gen->ref_source = ntv2_ref_source_freerun;
	ntv2_gen->gen_source = ntv2_ref_source_freerun;
	ntv2_gen->ref_locked = false;
	ntv2_gen->gen_locked = false;
	ntv2_gen->ref_lines = 0;
	ntv2_gen->ref_rate = ntv2_frame_rate_none;
	ntv2_gen->gen_lines = 0;
	ntv2_gen->gen_rate = ntv2_frame_rate_none;
	ntv2_gen->page_address = 0xff;

	return NTV2_STATUS_SUCCESS;
}

static bool has_state_changed(struct ntv2_genlock2 *ntv2_gen)
{
	uint32_t global;
	uint32_t ref_source;
	uint32_t ref_lines;
	uint32_t ref_rate;
	bool ref_locked = false;
	bool changed = false;

	global = reg_read(ntv2_gen, ntv2_reg_global_control);
	ref_source = NTV2_FLD_GET(ntv2_fld_global_reference_source, global);

	switch (ref_source)
	{
	default:
		ref_lines = c_ntsc_lines;
		ref_rate = c_ntsc_rate;
		break;
	}

	if ((ref_lines == 0) || (ref_rate == ntv2_frame_rate_none))
		ref_locked = false;

	if ((ref_lines != ntv2_gen->ref_lines) ||
		(ref_rate != ntv2_gen->ref_rate))
	{
		changed = true;
	}
	ntv2_gen->ref_source = ref_source;
	ntv2_gen->ref_lines = ref_lines;
	ntv2_gen->ref_rate = ref_rate;

	return changed;
}

static struct ntv2_genlock2_data* get_genlock2_config(struct ntv2_genlock2 *ntv2_gen, uint32_t lines, uint32_t rate)
{
	struct ntv2_genlock2_data* config;
	uint32_t genlockDeviceID = device_id_read(ntv2_gen);
	(void)rate;
	
	switch (lines)
	{
	default:
	case 525:
		switch (genlockDeviceID)
		{
		case 0x45:
			NTV2_MSG_GENLOCK_INFO("%s: configure genlock2 device %02X: 8a34045\n", ntv2_gen->name, genlockDeviceID);
			config = s_8a34045_broadcast_1485;
			break;
		case 0x12:
        default:
			NTV2_MSG_GENLOCK_INFO("%s: configure genlock2 device %02X: rc32012a\n", ntv2_gen->name, genlockDeviceID);
			config = s_rc32012a_broadcast_1485;
			break;
		}
	break;
	}
		
	return config;
}

static bool configure_genlock2(struct ntv2_genlock2 *ntv2_gen, struct ntv2_genlock2_data *config, bool check)
{
	struct ntv2_genlock2_data* gdat = config;
	uint32_t count = 0, errorCount = 0, totalBytes = 0;
	uint32_t value;
	uint32_t mask;
	uint8_t writeBytes[256];
	uint32_t outFreq1 = 0, outFreq2 = 0, outFreq3 = 0, outFreq4 = 0, outFreq5 = 0;
	uint8_t dpll0Status[1] = { 0 };
    uint32_t gpioValue;
    
	if (NTV2_DEBUG_ACTIVE(NTV2_DEBUG_GENLOCK_CHECK)) check = true;

	spi_reset(ntv2_gen);

	if (check)
	{
		NTV2_MSG_GENLOCK_INFO("%s: genlock2 write registers\n", ntv2_gen->name);
	}

	while ((gdat->size != 0))
	{
		//NTV2_MSG_GENLOCK_INFO("Writing offset %02X %s", gdat->offset, gdat->data);
		hex_to_bytes(gdat->data+2, writeBytes, gdat->size);
		if (!spi_genlock2_write(ntv2_gen, gdat->size, gdat->addr, writeBytes, true))
		{
			NTV2_MSG_ERROR("%s: genlock spi write failed\n", ntv2_gen->name);
			return false;
		}
		totalBytes += gdat->size;

		if (check)
		{
			uint8_t readBytes[256];
            uint32_t i;
			if (spi_genlock2_read(ntv2_gen, gdat->addr, readBytes, gdat->size))
			{
				for (i = 0; i < gdat->size; i++)
				{
					spi_genlock2_read(ntv2_gen, gdat->addr+i, readBytes+i, 1);
					if (readBytes[i] != writeBytes[i])
					{
						errorCount++;
						NTV2_MSG_GENLOCK_INFO("%s: Set: %d, Offset: %04X, size: %d, data: %s", ntv2_gen->name, count, gdat->addr, gdat->size, gdat->data);
						NTV2_MSG_GENLOCK_INFO("%s: Bytes did not match i : %d read : %02X write : %02X\n", ntv2_gen->name, i, readBytes[i], writeBytes[i]);
					}
					else
					{
						//NTV2_MSG_GENLOCK_INFO("%s: Bytes matched\n", ntv2_gen->name);
					}
				}
			}
		}
		else
		{
			NTV2_MSG_GENLOCK_INFO("%s: Set: %d, Offset: %04X No check", ntv2_gen->name, count, gdat->addr);
		}
		count++;
		gdat++;
	}
	NTV2_MSG_GENLOCK_INFO("%s: Total genlock error count: %d of %d", ntv2_gen->name, errorCount, totalBytes);

	dpll0Status[0] = 0;
	spi_genlock2_read(ntv2_gen, 0xC054, dpll0Status, 1);

	gpioValue = NTV2_FLD_GET(ntv2_fld_control_genlock_locked, reg_read(ntv2_gen, ntv2_reg_control_status));

	NTV2_MSG_GENLOCK_INFO("%s: Genlock dpll0: %d, gpio: %d", ntv2_gen->name, +dpll0Status[0], gpioValue);

	wait_genlock2(ntv2_gen, 1000);
	count = 0;
	outFreq1 = reg_read(ntv2_gen, ntv2_reg_out_freq1);
	outFreq2 = reg_read(ntv2_gen, ntv2_reg_out_freq2);
	outFreq3 = reg_read(ntv2_gen, ntv2_reg_out_freq3);
	outFreq4 = reg_read(ntv2_gen, ntv2_reg_out_freq4);
	outFreq5 = reg_read(ntv2_gen, ntv2_reg_out_freq5);
	NTV2_MSG_GENLOCK_INFO("%s: Freq0: %08X, Freq1: %08X, Freq2: %08X, Freq3: %08X, Freq4: %08X\n", ntv2_gen->name, outFreq1, outFreq2, outFreq3, outFreq4, outFreq5);
	while (	(outFreq1 & 0xffffff00) != 0x08D9EE00 &&
			(outFreq2 & 0xffffff00) != 0x08D7AA00 &&
			(outFreq3 & 0xffffff00) != 0x019BFC00 &&
			(outFreq4 & 0xffffff00) != 0x00BB8000 &&
			(outFreq5 & 0xffffff00) != 0x08D9EE00 &&
			count < 10000)
	{
		wait_genlock2(ntv2_gen, 1000);
		outFreq1 = reg_read(ntv2_gen, ntv2_reg_out_freq1);
		outFreq2 = reg_read(ntv2_gen, ntv2_reg_out_freq2);
		outFreq3 = reg_read(ntv2_gen, ntv2_reg_out_freq3);
		outFreq4 = reg_read(ntv2_gen, ntv2_reg_out_freq4);
		outFreq5 = reg_read(ntv2_gen, ntv2_reg_out_freq5);
		//NTV2_MSG_GENLOCK_INFO("%s: Freq0: %08X, Freq1: %08X, Freq2: %08X, Freq3: %08X, Freq4: %08X\n", ntv2_gen->name, outFreq1, outFreq2, outFreq3, outFreq4, outFreq5);
		count++;
	}

	if (count >= 10000)
	{
		NTV2_MSG_GENLOCK_INFO("%s: genlock2 initial lock check timeout\n", ntv2_gen->name);
	}
	else
	{
		NTV2_MSG_GENLOCK_INFO("%s: Freq0: %08X, Freq1: %08X, Freq2: %08X, Freq3: %08X, Freq4: %08X\n", ntv2_gen->name, outFreq1, outFreq2, outFreq3, outFreq4, outFreq5);
		NTV2_MSG_GENLOCK_INFO("%s: genlock2 initial lock check locked count: %d\n", ntv2_gen->name, count);
	}



	// reset genlock
	value = NTV2_FLD_SET(ntv2_fld_control_genlock_reset, 1);
	mask = NTV2_FLD_MASK(ntv2_fld_control_genlock_reset);
	ntv2_reg_rmw(ntv2_gen->system_context, ntv2_reg_control_status, ntv2_gen->index, value, mask);
	value = NTV2_FLD_SET(ntv2_fld_control_genlock_reset, 0);
	ntv2_reg_rmw(ntv2_gen->system_context, ntv2_reg_control_status, ntv2_gen->index, value, mask);

	if (check)
	{
		NTV2_MSG_GENLOCK_INFO("%s: genlock2 write complete\n", ntv2_gen->name);
	}

	return true;
}

static uint8_t device_id_read(struct ntv2_genlock2 *ntv2_gen)
{
	uint8_t rxID[1] = {0};
	spi_genlock2_read(ntv2_gen, 0xC032, rxID, 1);
	return rxID[0];

}

static bool wait_genlock2(struct ntv2_genlock2 *ntv2_gen, uint32_t numMicrosSeconds)
{
	uint32_t usTicks = 0;
	uint32_t timeoutCount = 0;

	ntv2_regnum_write(ntv2_gen->system_context, 0x3606, 0);
	while (usTicks < numMicrosSeconds)
	{
		usTicks = ntv2_regnum_read(ntv2_gen->system_context, 0x3606);
#if 1
		if(timeoutCount++ > 100) return false;
#else
		if (timeoutCount++ > 100)
		{
			NTV2_MSG_GENLOCK_INFO("Genlock2 200us wait TIMEDOUT");
			return false;
		}
		else
		{
			NTV2_MSG_GENLOCK_INFO("Genlock2 timeout count: %d", timeoutCount);
			return true;
		}
#endif
	}
	return true;
}

static void spi_reset(struct ntv2_genlock2 *ntv2_gen)
{
	ntv2_gen->page_address = 0xff;

	// reset spi hardware
    reg_write(ntv2_gen, ntv2_reg_spi_reset, 0x0a);

	// configure spi & reset fifos
    reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x1);
	reg_write(ntv2_gen, ntv2_reg_spi_control, 0x1fe);//0x1e6);
}

static void spi_reset_fifos(struct ntv2_genlock2 *ntv2_gen)
{
    //reg_write(ntv2_gen, ntv2_reg_spi_control, 0x1e6);
	reg_write(ntv2_gen, ntv2_reg_spi_control, 0x1fe);
}

static bool spi_wait_write_empty(struct ntv2_genlock2 *ntv2_gen)
{
	//uint32_t status = 0;
	uint32_t dtrStatus = 0;
	uint32_t count = 0;

	/*status = reg_read(ntv2_gen, ntv2_reg_spi_status);
    while ((status & GENL_SPI_WRITE_FIFO_EMPTY) == 0)
	{
		if (count++ > c_spi_timeout) return false;
		if (!ntv2_gen->monitor_enable) return false;
		status = reg_read(ntv2_gen, ntv2_reg_spi_status);
		NTV2_MSG_GENLOCK_INFO("%s: FIFO NOT EMPTY!\n", ntv2_gen->name);
	}
*/
	dtrStatus = reg_read(ntv2_gen, ntv2_reg_spi_ip_status);
	count = 0;
	while ((dtrStatus & DTR_EMPTY) == 0)
	{
		if (count++ > c_spi_timeout) return false;
		if (!ntv2_gen->monitor_enable) return false;
		dtrStatus = reg_read(ntv2_gen, ntv2_reg_spi_ip_status);
		//NTV2_MSG_GENLOCK_INFO("%s: DTR NOT EMPTY!\n", ntv2_gen->name);
	}

	//NTV2_MSG_GENLOCK_INFO("%s: Transfer Complete! %d\n", ntv2_gen->name, count);
	return true;
}

static bool spi_wait_read_not_empty(struct ntv2_genlock2 *ntv2_gen)
{
	uint32_t status = 0;
	uint32_t count = 0;

	status = reg_read(ntv2_gen, ntv2_reg_spi_status);
    while ((status & GENL_SPI_READ_FIFO_EMPTY) != 0)
	{
		if (count++ > c_spi_timeout) return false;
		if (!ntv2_gen->monitor_enable) return false;
		status = reg_read(ntv2_gen, ntv2_reg_spi_status);
		//NTV2_MSG_GENLOCK_INFO("READ FIFO EMPTY!");
	}
	//NTV2_MSG_GENLOCK_INFO("READ FIFO NOT EMPTY! %d", count);
	return true;
}

static bool reset_dtr_status(struct ntv2_genlock2 *ntv2_gen)
{
	uint32_t dtrStatus = reg_read(ntv2_gen, ntv2_reg_spi_ip_status);
	if ((dtrStatus & DTR_EMPTY) != 0)
	{
		uint32_t dtrReset = NTV2_FLD_SET(ntv2_fld_ip_status_dtr_empty, 1);
		uint32_t dtrMask = NTV2_FLD_MASK(ntv2_fld_ip_status_dtr_empty);
		//NTV2_MSG_GENLOCK_INFO("%s: DTR High!\n", ntv2_gen->name);
		ntv2_reg_rmw(ntv2_gen->system_context, ntv2_reg_spi_ip_status, ntv2_gen->index, dtrReset, dtrMask);
		dtrStatus = reg_read(ntv2_gen, ntv2_reg_spi_ip_status);
		//NTV2_MSG_GENLOCK_INFO("%s: DTR Reset Value: %08X!\n", ntv2_gen->name, dtrStatus);
		return true;
	}
	return false;
}

static bool spi_genlock2_write(struct ntv2_genlock2 *ntv2_gen, uint32_t size, uint16_t addr, uint8_t* data, bool triggerWait)
{
    uint32_t controlVal;
	uint8_t   page_select_buffer[10];
    uint32_t i;
    
    //if (!spi_wait_write_empty(ntv2_gen)) return false;

	// Step 1 reset FIFOs
	spi_reset_fifos(ntv2_gen);

	if (addr != 0x7C)
	{
		// set page
		page_select_buffer[0] = addr & 0x80;
		page_select_buffer[1] = addr >> 8;
		page_select_buffer[2] = 0x10;
		page_select_buffer[3] = 0x20;

		//NTV2_MSG_GENLOCK_INFO("Write: Set Page");
		spi_genlock2_write(ntv2_gen, 4, 0x7C, page_select_buffer, true);
	}

	// Step 2 load data
	reg_write(ntv2_gen, ntv2_reg_spi_write, addr & 0x7f);
	//NTV2_MSG_GENLOCK_INFO("Wrote %02X", offset);
	for (i = 0; i < size; i++)
	{
		reg_write(ntv2_gen, ntv2_reg_spi_write, data[i]);
		//NTV2_MSG_GENLOCK_INFO("Wrote %02X", data[i]);
	}
	
	// Reset DRT Shift bit
	reset_dtr_status(ntv2_gen);
	
	// Step 3 chip select low
	reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x0);

	// Step 4 enable master transactions
	controlVal = reg_read(ntv2_gen, ntv2_reg_spi_control);
	controlVal &= ~0x100;
	reg_write(ntv2_gen, ntv2_reg_spi_control, controlVal);

	spi_wait_write_empty(ntv2_gen);

	// Step 5 deassert chip select
	reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x01);

	// Step 6 disable master transactions
	controlVal = reg_read(ntv2_gen, ntv2_reg_spi_control);
	controlVal |= 0x100;
	reg_write(ntv2_gen, ntv2_reg_spi_control, controlVal);

	if (triggerWait)
		wait_genlock2(ntv2_gen, 200);
	
	return true;
}

static bool spi_genlock2_read(struct ntv2_genlock2 *ntv2_gen, uint16_t addr, uint8_t* data, uint32_t numBytes)
{
	uint32_t  val, status;
    uint8_t   tx_buffer[10];
	uint32_t i;

	//if (!spi_wait_write_empty(ntv2_gen))
		//return false;

	// Step 1 reset FIFOs
	spi_reset_fifos(ntv2_gen);

    if (addr != 0x7c)
    {
        // set page
        tx_buffer[0] = addr & 0x80;
        tx_buffer[1] = addr >> 8;
        tx_buffer[2] = 0x10;
        tx_buffer[3] = 0x20;

		//NTV2_MSG_GENLOCK_INFO("Read: Set Page");
		spi_genlock2_write(ntv2_gen, 4, 0x7C, tx_buffer, true);
    }

#if 1

	reg_write(ntv2_gen, ntv2_reg_spi_control, 0xfe);//0xe6);
	wait_genlock2(ntv2_gen, 1000);
	reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x00);
	wait_genlock2(ntv2_gen, 1000);

	reg_write(ntv2_gen, ntv2_reg_spi_write, 0x80 | (addr & 0x7f));
	wait_genlock2(ntv2_gen, 1000);

	for (i = 0; i < numBytes; i++)
		reg_write(ntv2_gen, ntv2_reg_spi_write, 0);
    
	if (!spi_wait_write_empty(ntv2_gen))
		return false;

	reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x01);
	wait_genlock2(ntv2_gen, 10000);

	if(!spi_wait_read_not_empty(ntv2_gen))
		return false;

	//wait_genlock2(ntv2_gen, 1000);
    
	val = reg_read(ntv2_gen, ntv2_reg_spi_read); // dummy read for address sent
	//NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
    for (i = 0; i < numBytes; i++)
    {
        status = reg_read(ntv2_gen, ntv2_reg_spi_status);
		if(!spi_wait_read_not_empty(ntv2_gen))
			return false;
        val = reg_read(ntv2_gen, ntv2_reg_spi_read);
		//NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
        data[i] = (uint8_t)val;
    }

	reg_write(ntv2_gen, ntv2_reg_spi_control, 0xfe);//0xe6);

#else
	

	//Send address and dumy bytes
	memset(tx_buffer, 0, 10);
	//NTV2_MSG_GENLOCK_INFO("Read: Send address and dummy bytes");
	spi_genlock2_write(ntv2_gen, numBytes,  (0x80 | (addr & 0x7f)), tx_buffer, true);
    
	// read bytes
	if(!spi_wait_read_not_empty(ntv2_gen))
		return false;

	//wait_genlock2(ntv2_gen, 1000);
    
	val = reg_read(ntv2_gen, ntv2_reg_spi_read); // dummy read for address sent
	//NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
    for (uint32_t i=0; i < numBytes; i++)
    {
        status = reg_read(ntv2_gen, ntv2_reg_spi_status);
		if(!spi_wait_read_not_empty(ntv2_gen))
			return false;
        val = reg_read(ntv2_gen, ntv2_reg_spi_read);
		//NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
        data[i] = (uint8_t)val;
    }

	spi_reset_fifos(ntv2_gen);
#endif
	return true;
}

void hex_to_bytes(char *hex, uint8_t *output, uint32_t array_length)
{
    uint32_t i;
    uint32_t j;
    
    for (i = 0, j = 0; i < array_length; i++, j+=2)
    {
        uint8_t bottom = hex[j+1] - (hex[j+1] > '9' ? 'A' - 10 : '0');
        uint8_t top = hex[j] - (hex[j] > '9' ? 'A' - 10 : '0');
        output[i] = (top * 16) + bottom;
    }
}

static uint32_t reg_read(struct ntv2_genlock2 *ntv2_gen, const uint32_t *reg)
{
    return ntv2_reg_read(ntv2_gen->system_context, reg, ntv2_gen->index);
}

static void reg_write(struct ntv2_genlock2 *ntv2_gen, const uint32_t *reg, uint32_t data)
{
    ntv2_reg_write(ntv2_gen->system_context, reg, ntv2_gen->index, data);
}
