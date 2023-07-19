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

//#define MONITOR_STATE_CHANGES

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

/* control and status */
NTV2_REG(ntv2_reg_control_status,							48);			/* control status */
	NTV2_FLD(ntv2_fld_control_genlock_reset,					1,	6);			/* genlock reset */
	NTV2_FLD(ntv2_fld_control_reference_source,					4,	24);		/* hardware reference source */
	NTV2_FLD(ntv2_fld_control_reference_present,				1,	30);		/* reference source present */
	NTV2_FLD(ntv2_fld_control_genlock_locked,					1,	31);		/* genlock locked */

/* hdmi input status */
NTV2_REG(ntv2_reg_hdmiin_input_status,						126);			/* hdmi input status register */
	NTV2_FLD(ntv2_fld_hdmiin_locked,							1,	0);		
	NTV2_FLD(ntv2_fld_hdmiin_stable,							1,	1);		
	NTV2_FLD(ntv2_fld_hdmiin_rgb,								1,	2);		
	NTV2_FLD(ntv2_fld_hdmiin_deep_color,						1,	3);		
	NTV2_FLD(ntv2_fld_hdmiin_video_code,						6,	4);			/* ntv2 video standard v2 */
	NTV2_FLD(ntv2_fld_hdmiin_audio_8ch,							1,	12);		/* 8 audio channels (vs 2) */
	NTV2_FLD(ntv2_fld_hdmiin_progressive,						1,	13);	
	NTV2_FLD(ntv2_fld_hdmiin_video_sd,							1,	14);		/* video pixel clock sd (not hd or 3g) */
	NTV2_FLD(ntv2_fld_hdmiin_video_74_25,						1,	15);		/* not used */
	NTV2_FLD(ntv2_fld_hdmiin_audio_rate,						4,	16);	
	NTV2_FLD(ntv2_fld_hdmiin_audio_word_length,					4,	20);	
	NTV2_FLD(ntv2_fld_hdmiin_video_format,						3,	24);		/* really ntv2 standard */
	NTV2_FLD(ntv2_fld_hdmiin_dvi,								1,	27);		/* input dvi (vs hdmi) */
	NTV2_FLD(ntv2_fld_hdmiin_video_rate,						4,	28);		/* ntv2 video rate */

/* genlock spi control */
NTV2_REG(ntv2_reg_spi_reset,				0x8010);
NTV2_REG(ntv2_reg_spi_control,				0x8018);
NTV2_REG(ntv2_reg_spi_status,				0x8019);
NTV2_REG(ntv2_reg_spi_write,				0x801a);
NTV2_REG(ntv2_reg_spi_read,					0x801b);
NTV2_REG(ntv2_reg_spi_slave,				0x801c);

static const int64_t c_default_timeout		= 50000;
static const int64_t c_spi_timeout			= 10000;
static const int64_t c_genlock_reset_wait	= 300000;
static const int64_t c_genlock_config_wait	= 500000;

static const uint32_t c_default_lines = 525;
static const uint32_t c_default_rate = ntv2_frame_rate_2997;
static const uint32_t c_configure_error_limit  = 20;

static void ntv2_genlock2_monitor(void* data);
static Ntv2Status ntv2_genlock2_initialize(struct ntv2_genlock2 *ntv2_gen);
static bool has_state_changed(struct ntv2_genlock2 *ntv2_gen);
static struct ntv2_genlock2_data* get_genlock2_config(struct ntv2_genlock2 *ntv2_gen, enum ntv2_genlock2_mode mode);
static bool configure_genlock2(struct ntv2_genlock2 *ntv2_gen, struct ntv2_genlock2_data *config, bool check);
static uint8_t device_id_read(struct ntv2_genlock2 *ntv2_gen);
static bool wait_genlock2(struct ntv2_genlock2 *ntv2_gen, uint32_t numMicrosSeconds);

static void spi_reset(struct ntv2_genlock2 *ntv2_gen);
static void spi_reset_fifos(struct ntv2_genlock2 *ntv2_gen);
static bool spi_wait_write_empty(struct ntv2_genlock2 *ntv2_gen);
static bool spi_genlock2_write(struct ntv2_genlock2 *ntv2_gen, uint32_t size, uint8_t offset, uint8_t* data, bool triggerWait);
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

	//ntv2SpinLockOpen(&ntv2_gen->state_lock, sys_con);
	//ntv2ThreadOpen(&ntv2_gen->monitor_task, sys_con, "genlock2 monitor");
	//ntv2EventOpen(&ntv2_gen->monitor_event, sys_con);

	NTV2_MSG_GENLOCK_INFO("%s: open ntv2_genlock2\n", ntv2_gen->name);

	return ntv2_gen;
}

void ntv2_genlock2_close(struct ntv2_genlock2 *ntv2_gen)
{
	if (ntv2_gen == NULL) 
		return;

	NTV2_MSG_GENLOCK_INFO("%s: close ntv2_genlock2\n", ntv2_gen->name);

	//ntv2_genlock2_disable(ntv2_gen);

	//ntv2EventClose(&ntv2_gen->monitor_event);
	//ntv2ThreadClose(&ntv2_gen->monitor_task);
	//ntv2SpinLockClose(&ntv2_gen->state_lock);

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
	bool success ;

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
		struct ntv2_genlock2_data* configData = get_genlock2_config(ntv2_gen, mode);
		if (!configure_genlock2(ntv2_gen, configData, false))
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
		}

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
			if ((ntv2_gen->gen_lines != c_default_lines) ||
				(ntv2_gen->gen_rate != c_default_rate))
			{
				unlock_wait++;
				if (unlock_wait > 5)
				{
					ntv2_gen->gen_lines = c_default_lines;
					ntv2_gen->gen_rate = c_default_rate;
					update = true;
				}
			}
		}

		if (update) {
			//config = get_genlock2_config(ntv2_gen, ntv2_gen->gen_lines, ntv2_gen->gen_rate);
			if (config != NULL) {
				NTV2_MSG_GENLOCK_CONFIG("%s: configure genlock2 lines %d  rate %s\n", 
										ntv2_gen->name, ntv2_gen->gen_lines, ntv2_frame_rate_name(ntv2_gen->gen_rate));
				if (!configure_genlock2(ntv2_gen, config, false)) goto sleep;
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
#ifndef MONITOR_STATE_CHANGES
	(void)ntv2_gen;
	return false;
#else
	uint32_t global;
	uint32_t control;
	uint32_t input;
	uint32_t standard;
	uint32_t ref_source;
	uint32_t gen_source;
	uint32_t ref_lines;
	uint32_t ref_rate;
	bool ref_locked;
	bool gen_locked;
	bool changed = false;

	global = reg_read(ntv2_gen, ntv2_reg_global_control);
	ref_source = NTV2_FLD_GET(ntv2_fld_global_reference_source, global);

	control = reg_read(ntv2_gen, ntv2_reg_control_status);
	gen_source = NTV2_FLD_GET(ntv2_fld_control_reference_source, control);
	ref_locked = (NTV2_FLD_GET(ntv2_fld_control_reference_present, control) == 1);
	gen_locked = (NTV2_FLD_GET(ntv2_fld_control_genlock_locked, control) == 1);

	switch (ref_source)
	{
	case ntv2_ref_source_external:
		input = reg_read(ntv2_gen, ntv2_reg_sdiin_input_status);
		standard = NTV2_FLD_GET(ntv2_fld_sdiin_standard_ref, input);
		ref_lines = ntv2_ref_standard_lines(standard);
		ref_rate = NTV2_FLD_GET(ntv2_fld_sdiin_frame_rate_ref, input);
		break;
	case ntv2_ref_source_hdmi:
		input = reg_read(ntv2_gen, ntv2_reg_hdmiin_input_status);
		standard = NTV2_FLD_GET(ntv2_fld_hdmiin_video_code, input);
		ref_lines = ntv2_video_standard_lines(standard);
		ref_rate = NTV2_FLD_GET(ntv2_fld_hdmiin_video_rate, input);
		// special case 625i generates 525i ref
		if ((ref_lines == 625) &&
			(ref_rate == ntv2_frame_rate_2500))
		{
			ref_lines = 525;
			ref_rate = ntv2_frame_rate_2997;
		}
		break;
	default:
		ref_lines = c_default_lines;
		ref_rate = c_default_rate;
		break;
	}

	if ((ref_lines == 0) || (ref_rate == ntv2_frame_rate_none))
		ref_locked = false;

	if ((ref_source != ntv2_gen->ref_source) ||
		(gen_source != ntv2_gen->gen_source) ||
		(ref_locked != ntv2_gen->ref_locked) ||
		(gen_locked != ntv2_gen->gen_locked) ||
		(ref_lines != ntv2_gen->ref_lines) || 
		(ref_rate != ntv2_gen->ref_rate)) {
		changed = true;
	}
	ntv2_gen->ref_source = ref_source;
	ntv2_gen->gen_source = gen_source;
	ntv2_gen->ref_locked = ref_locked;
	ntv2_gen->gen_locked = gen_locked;
	ntv2_gen->ref_lines = ref_lines;
	ntv2_gen->ref_rate = ref_rate;

	return changed;
#endif
}

static struct ntv2_genlock2_data* get_genlock2_config(struct ntv2_genlock2 *ntv2_gen, enum ntv2_genlock2_mode mode)
{
	struct ntv2_genlock2_data* config = NULL;
	uint32_t genlockDeviceID = device_id_read(ntv2_gen);

	switch (mode)
	{
	default:
	case ntv2_genlock2_mode_broadcast_1485:
		switch (genlockDeviceID)
		{
		default:
		case 0x45:
			config = s_8a34045_broadcast_1485;
			break;
		case 0x12:
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
	uint32_t count = 0;
	uint32_t value;
	uint32_t mask;
	uint8_t writeBytes[256];

	if (NTV2_DEBUG_ACTIVE(NTV2_DEBUG_GENLOCK_CHECK)) check = true;

	spi_reset(ntv2_gen);

	if (check) {
		NTV2_MSG_GENLOCK_CHECK("%s: genlock write registers\n", ntv2_gen->name);
	}

	while ((gdat->size != 0))
	{
		NTV2_MSG_GENLOCK_INFO("Writing offset %02X %s", gdat->offset, gdat->data);
		hex_to_bytes(gdat->data+2, writeBytes, gdat->size);
		if (!spi_genlock2_write(ntv2_gen, gdat->size, gdat->offset, writeBytes, true)) {
			NTV2_MSG_ERROR("%s: genlock spi write failed\n", ntv2_gen->name);
		return false;
		}
		count++;
		gdat++;
	}

	// reset genlock
	value = NTV2_FLD_SET(ntv2_fld_control_genlock_reset, 1);
	mask = NTV2_FLD_MASK(ntv2_fld_control_genlock_reset);
	ntv2_reg_rmw(ntv2_gen->system_context, ntv2_reg_control_status, ntv2_gen->index, value, mask);
	value = NTV2_FLD_SET(ntv2_fld_control_genlock_reset, 0);
	ntv2_reg_rmw(ntv2_gen->system_context, ntv2_reg_control_status, ntv2_gen->index, value, mask);

	if (check) {
		NTV2_MSG_GENLOCK_CHECK("%s: genlock write complete\n", ntv2_gen->name);
	}

	return true;
}

static uint8_t device_id_read(struct ntv2_genlock2 *ntv2_gen)
{
	uint8_t rxID[1];
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
	reg_write(ntv2_gen, ntv2_reg_spi_control, 0x1e6);
}

static void spi_reset_fifos(struct ntv2_genlock2 *ntv2_gen)
{
    reg_write(ntv2_gen, ntv2_reg_spi_control, 0x1e6);
}

static bool spi_wait_write_empty(struct ntv2_genlock2 *ntv2_gen)
{
	uint32_t status = 0;
	uint32_t count = 0;

	status = reg_read(ntv2_gen, ntv2_reg_spi_status);
    while ((status & GENL_SPI_WRITE_FIFO_EMPTY) == 0)
	{
		if (count++ > c_spi_timeout) return false;
		if (!ntv2_gen->monitor_enable) return false;
		status = reg_read(ntv2_gen, ntv2_reg_spi_status);
		//NTV2_MSG_GENLOCK_INFO("FIFO NOT EMPTY!");
	}
	//NTV2_MSG_GENLOCK_INFO("FIFO EMPTY! %d", count);
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

static bool spi_genlock2_write(struct ntv2_genlock2 *ntv2_gen, uint32_t size, uint8_t offset, uint8_t* data, bool triggerWait)
{
    uint32_t controlVal;
    uint32_t i;
    
    if (!spi_wait_write_empty(ntv2_gen)) return false;

	// Step 1 reset FIFOs
	spi_reset_fifos(ntv2_gen);

	// Step 2 load data
	reg_write(ntv2_gen, ntv2_reg_spi_write, offset);
	NTV2_MSG_GENLOCK_INFO("Wrote %02X", offset);
	for (i = 0; i < size; i++)
	{
		reg_write(ntv2_gen, ntv2_reg_spi_write, data[i]);
		NTV2_MSG_GENLOCK_INFO("Wrote %02X", data[i]);
	}

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

	if (!spi_wait_write_empty(ntv2_gen))
		return false;

	// Step 1 reset FIFOs
	spi_reset_fifos(ntv2_gen);

    if (addr != 0x7c)
    {
        // set page
        tx_buffer[0] = addr & 0x80;
        tx_buffer[1] = addr >> 8;
        tx_buffer[2] = 0x10;
        tx_buffer[3] = 0x20;

		NTV2_MSG_GENLOCK_INFO("Read: Set Page");
		spi_genlock2_write(ntv2_gen, 4, 0x7C, tx_buffer, true);
    }

#if 1

	reg_write(ntv2_gen, ntv2_reg_spi_control, 0xe6);
	wait_genlock2(ntv2_gen, 1000);
	reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x00);
	wait_genlock2(ntv2_gen, 1000);

	reg_write(ntv2_gen, ntv2_reg_spi_write, 0x80 | (addr & 0x7f));
	wait_genlock2(ntv2_gen, 1000);

	for (i = 0; i < numBytes; i++)
		reg_write(ntv2_gen, ntv2_reg_spi_write, 0);

	reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x01);
	wait_genlock2(ntv2_gen, 10000);

	if(!spi_wait_read_not_empty(ntv2_gen))
		return false;

	//wait_genlock2(ntv2_gen, 1000);
    
	val = reg_read(ntv2_gen, ntv2_reg_spi_read); // dummy read for address sent
	NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
    for (i = 0; i < numBytes; i++)
    {
        status = reg_read(ntv2_gen, ntv2_reg_spi_status);
		if(!spi_wait_read_not_empty(ntv2_gen))
			return false;
        val = reg_read(ntv2_gen, ntv2_reg_spi_read);
		NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
        data[i] = (uint8_t)val;
    }

	reg_write(ntv2_gen, ntv2_reg_spi_control, 0xe6);

#else
	

	//Send address and dumy bytes
	memset(tx_buffer, 0, 10);
	NTV2_MSG_GENLOCK_INFO("Read: Send address and dummy bytes");
	spi_genlock2_write(ntv2_gen, numBytes,  (0x80 | (addr & 0x7f)), tx_buffer, true);
    
	// read bytes
	if(!spi_wait_read_not_empty(ntv2_gen))
		return false;

	//wait_genlock2(ntv2_gen, 1000);
    
	val = reg_read(ntv2_gen, ntv2_reg_spi_read); // dummy read for address sent
	NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
    for (uint32_t i=0; i < numBytes; i++)
    {
        status = reg_read(ntv2_gen, ntv2_reg_spi_status);
		if(!spi_wait_read_not_empty(ntv2_gen))
			return false;
        val = reg_read(ntv2_gen, ntv2_reg_spi_read);
		NTV2_MSG_GENLOCK_INFO("Read: Val: %02X", val);
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
