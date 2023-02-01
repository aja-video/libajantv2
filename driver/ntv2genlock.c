/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2genlock.c
//
//==========================================================================

#include "ntv2genlock.h"
#include "ntv2commonreg.h"
#include "ntv2genregs.h"


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
//	NTV2_DEBUG_GENLOCK_STATE |
//	NTV2_DEBUG_GENLOCK_DETECT | 
//	NTV2_DEBUG_GENLOCK_CONFIG |
//	NTV2_DEBUG_GENLOCK_CHECK |
	NTV2_DEBUG_INFO |
	NTV2_DEBUG_ERROR;

// genlock spi registers
#define GENL_SPI_SET_ADDR_CMD				0x00
#define GENL_SPI_WRITE_CMD					0x40
#define GENL_SPI_READ_CMD					0x80

#define GENL_SPI_READ_FIFO_EMPTY			0x01
#define GENL_SPI_READ_FIFO_FULL				0x02
#define GENL_SPI_WRITE_FIFO_EMPTY			0x04
#define GENL_SPI_WRITE_FIFO_FULL			0x08

/* global control */
NTV2_REG(ntv2_reg_global_control,							0, 267, 377, 378, 379, 380, 381, 382, 383);	/* global control */
	NTV2_FLD(ntv2_fld_global_frame_rate_012,					3,	0);			/* frame rate 0-2 */
	NTV2_FLD(ntv2_fld_global_frame_geometry,					4,	3);			/* frame geometry */
	NTV2_FLD(ntv2_fld_global_video_standard,					3,	7);			/* video standard */
	NTV2_FLD(ntv2_fld_global_reference_source,					3,	10);		/* user reference source */
	NTV2_FLD(ntv2_fld_global_register_sync,						2,	20);		/* register updat sync */
	NTV2_FLD(ntv2_fld_global_frame_rate_3,						1,	21);		/* frame rate 3 */

/* input status */
NTV2_REG(ntv2_reg_sdiin_input_status,						22);			/* input status */
	NTV2_FLD(ntv2_fld_sdiin_frame_rate_1_012,					3,	0);			/* sdi 1 frame rate 0-2 */
	NTV2_FLD(ntv2_fld_sdiin_autotiming_1,						1,	3);			/* sdi 1 auto timing */
	NTV2_FLD(ntv2_fld_sdiin_frame_geometry_1_012,				3,	4);			/* sdi 1 frame geometry 0-2 */
	NTV2_FLD(ntv2_fld_sdiin_progressive_1,						1,	7);			/* sdi 1 progressive scan */
	NTV2_FLD(ntv2_fld_sdiin_frame_rate_2_012,					3,	8);			/* sdi 2 frame rate 0-2 */
	NTV2_FLD(ntv2_fld_sdiin_autotiming_2,						1,	11);		/* sdi 2 auto timing */
	NTV2_FLD(ntv2_fld_sdiin_frame_geometry_2_012,				3,	12);		/* sdi 2 frame geometry 0-2 */
	NTV2_FLD(ntv2_fld_sdiin_progressive_2,						1,	15);		/* sdi 2 progressive scan */
	NTV2_FLD(ntv2_fld_sdiin_frame_rate_ref,						4,	16);		/* reference frame rate */
	NTV2_FLD(ntv2_fld_sdiin_standard_ref,						3,	20);		/* reference standard */
	NTV2_FLD(ntv2_fld_sdiin_progressive_ref,					1,	23);		/* reference progressive scan */
	NTV2_FLD(ntv2_fld_sdiin_frame_rate_1_3,						3,	28);		/* sdi 1 frame rate 3 */
	NTV2_FLD(ntv2_fld_sdiin_frame_rate_2_3,						3,	29);		/* sdi 2 frame rate 3 */
	NTV2_FLD(ntv2_fld_sdiin_frame_geometry_1_3,					3,	30);		/* sdi 1 frame geometry 3 */
	NTV2_FLD(ntv2_fld_sdiin_frame_geometry_2_3,					3,	31);		/* sdi 2 frame geometry 3 */

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

#define GENLOCK_FRAME_RATE_SIZE		11

static struct ntv2_genlock_data* s_genlock_750[GENLOCK_FRAME_RATE_SIZE] = {
	/* ntv2_frame_rate_none */					NULL,
	/* ntv2_frame_rate_6000 */					s_genlock_750_6000,
	/* ntv2_frame_rate_5994 */					s_genlock_750_5994,
	/* ntv2_frame_rate_3000 */					NULL,
	/* ntv2_frame_rate_2997 */					NULL,
	/* ntv2_frame_rate_2500 */					NULL,
	/* ntv2_frame_rate_2400 */					NULL,
	/* ntv2_frame_rate_2398 */					NULL,
	/* ntv2_frame_rate_5000 */					s_genlock_750_5000,
	/* ntv2_frame_rate_4800 */					NULL,
	/* ntv2_frame_rate_4795 */					NULL
};

static struct ntv2_genlock_data* s_genlock_1125[GENLOCK_FRAME_RATE_SIZE] = {
	/* ntv2_frame_rate_none */					NULL,
	/* ntv2_frame_rate_6000 */					s_genlock_1125_6000,
	/* ntv2_frame_rate_5994 */					s_genlock_1125_5994,
	/* ntv2_frame_rate_3000 */					s_genlock_1125_3000,
	/* ntv2_frame_rate_2997 */					s_genlock_1125_2997,
	/* ntv2_frame_rate_2500 */					s_genlock_1125_2500,
	/* ntv2_frame_rate_2400 */					s_genlock_1125_2400,
	/* ntv2_frame_rate_2398 */					s_genlock_1125_2398,
	/* ntv2_frame_rate_5000 */					s_genlock_1125_5000,
	/* ntv2_frame_rate_4800 */					s_genlock_1125_4800,
	/* ntv2_frame_rate_4795 */					s_genlock_1125_4795
};

static struct ntv2_genlock_data* s_genlock_2250[GENLOCK_FRAME_RATE_SIZE] = {
	/* ntv2_frame_rate_none */					NULL,
	/* ntv2_frame_rate_6000 */					s_genlock_2250_6000,
	/* ntv2_frame_rate_5994 */					s_genlock_2250_5994,
	/* ntv2_frame_rate_3000 */					s_genlock_2250_3000,
	/* ntv2_frame_rate_2997 */					s_genlock_2250_2997,
	/* ntv2_frame_rate_2500 */					s_genlock_2250_2500,
	/* ntv2_frame_rate_2400 */					s_genlock_2250_2400,
	/* ntv2_frame_rate_2398 */					s_genlock_2250_2398,
	/* ntv2_frame_rate_5000 */					s_genlock_2250_5000,
	/* ntv2_frame_rate_4800 */					s_genlock_2250_4800,
	/* ntv2_frame_rate_4795 */					s_genlock_2250_4795
};

static void ntv2_genlock_monitor(void* data);
static Ntv2Status ntv2_genlock_initialize(struct ntv2_genlock *ntv2_gen);
static bool has_state_changed(struct ntv2_genlock *ntv2_gen);
static struct ntv2_genlock_data* get_genlock_config(struct ntv2_genlock *ntv2_gen, uint32_t lines, uint32_t rate);
static bool configure_genlock(struct ntv2_genlock *ntv2_gen, struct ntv2_genlock_data *config, bool check);

static void spi_reset(struct ntv2_genlock *ntv2_gen);
static void spi_reset_fifos(struct ntv2_genlock *ntv2_gen);
static bool spi_wait_write_empty(struct ntv2_genlock *ntv2_gen);
static bool spi_genlock_write(struct ntv2_genlock *ntv2_gen, uint16_t addr, uint8_t value);
static bool spi_genlock_read(struct ntv2_genlock *ntv2_gen, uint16_t addr, uint8_t* value);

static uint32_t reg_read(struct ntv2_genlock *ntv2_gen, const uint32_t *reg);
static void reg_write(struct ntv2_genlock *ntv2_gen, const uint32_t *reg, uint32_t data);

struct ntv2_genlock *ntv2_genlock_open(Ntv2SystemContext* sys_con,
									   const char *name, int index)
{
	struct ntv2_genlock *ntv2_gen = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_gen = (struct ntv2_genlock *)ntv2MemoryAlloc(sizeof(struct ntv2_genlock));
	if (ntv2_gen == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_genlock instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_gen, 0, sizeof(struct ntv2_genlock));

	ntv2_gen->index = index;
#if defined(MSWindows)
	sprintf(ntv2_gen->name, "%s%d", name, index);
#else
	snprintf(ntv2_gen->name, NTV2_GENLOCK_STRING_SIZE, "%s%d", name, index);
#endif
	ntv2_gen->system_context = sys_con;

	ntv2SpinLockOpen(&ntv2_gen->state_lock, sys_con);
	ntv2ThreadOpen(&ntv2_gen->monitor_task, sys_con, "genlock monitor");
	ntv2EventOpen(&ntv2_gen->monitor_event, sys_con);

	NTV2_MSG_GENLOCK_INFO("%s: open ntv2_genlock\n", ntv2_gen->name);

	return ntv2_gen;
}

void ntv2_genlock_close(struct ntv2_genlock *ntv2_gen)
{
	if (ntv2_gen == NULL) 
		return;

	NTV2_MSG_GENLOCK_INFO("%s: close ntv2_genlock\n", ntv2_gen->name);

	ntv2_genlock_disable(ntv2_gen);

	ntv2EventClose(&ntv2_gen->monitor_event);
	ntv2ThreadClose(&ntv2_gen->monitor_task);
	ntv2SpinLockClose(&ntv2_gen->state_lock);

	memset(ntv2_gen, 0, sizeof(struct ntv2_genlock));
	ntv2MemoryFree(ntv2_gen, sizeof(struct ntv2_genlock));
}

Ntv2Status ntv2_genlock_configure(struct ntv2_genlock *ntv2_gen)
{
	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_GENLOCK_INFO("%s: configure genlock device\n", ntv2_gen->name);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_genlock_enable(struct ntv2_genlock *ntv2_gen)
{
	bool success ;

	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (ntv2_gen->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_GENLOCK_STATE("%s: enable genlock monitor\n", ntv2_gen->name);

	ntv2EventClear(&ntv2_gen->monitor_event);
	ntv2_gen->monitor_enable = true;

	success = ntv2ThreadRun(&ntv2_gen->monitor_task, ntv2_genlock_monitor, (void*)ntv2_gen);
	if (!success) {
		return NTV2_STATUS_FAIL;
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_genlock_disable(struct ntv2_genlock *ntv2_gen)
{
	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (!ntv2_gen->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_GENLOCK_STATE("%s: disable genlock monitor\n", ntv2_gen->name);

	ntv2_gen->monitor_enable = false;
	ntv2EventSignal(&ntv2_gen->monitor_event);

	ntv2ThreadStop(&ntv2_gen->monitor_task);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_genlock_program(struct ntv2_genlock *ntv2_gen,
								enum ntv2_genlock_mode mode)
{
	if (ntv2_gen == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	ntv2_gen->monitor_enable = true;
	
	switch (mode)
	{
	case ntv2_genlock_mode_zero:
		if (!configure_genlock(ntv2_gen, s_genlock_zero, true))
			return NTV2_STATUS_FAIL;
		break;
	case ntv2_genlock_mode_ntsc_27mhz:
		if (!configure_genlock(ntv2_gen, s_genlock_ntsc_27mhz, true))
			return NTV2_STATUS_FAIL;
		break;
	default:
		return NTV2_STATUS_BAD_PARAMETER;
	}

	return NTV2_STATUS_SUCCESS;
}

static void ntv2_genlock_monitor(void* data)
{
	struct ntv2_genlock *ntv2_gen = (struct ntv2_genlock *)data;
	struct ntv2_genlock_data *config = NULL;
	bool update = false;
	uint32_t lock_wait = 0;
	uint32_t unlock_wait = 0;
	Ntv2Status status;

	if (ntv2_gen == NULL)
		return;

	NTV2_MSG_GENLOCK_STATE("%s: genlock input monitor task start\n", ntv2_gen->name);

	status = ntv2_genlock_initialize(ntv2_gen);
	if (status != NTV2_STATUS_SUCCESS) {
		NTV2_MSG_ERROR("%s: genlock initialization failed\n", ntv2_gen->name);
		goto exit;
	}

	while (!ntv2ThreadShouldStop(&ntv2_gen->monitor_task) && ntv2_gen->monitor_enable) 
	{
//		if ((reg_read(ntv2_gen, ntv2_reg_control_status) & 0x1) == 0)  goto sleep;

		if (has_state_changed(ntv2_gen)) 
		{
			NTV2_MSG_GENLOCK_DETECT("%s: new genlock state   ref %s %s   gen %s %s   lines %d  rate %s\n", 
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
			config = get_genlock_config(ntv2_gen, ntv2_gen->gen_lines, ntv2_gen->gen_rate);
			if (config != NULL) {
				NTV2_MSG_GENLOCK_CONFIG("%s: configure genlock  lines %d  rate %s\n", 
										ntv2_gen->name, ntv2_gen->gen_lines, ntv2_frame_rate_name(ntv2_gen->gen_rate));
				if (!configure_genlock(ntv2_gen, config, false)) goto sleep;
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
	NTV2_MSG_GENLOCK_STATE("%s: genlock monitor task stop\n", ntv2_gen->name);
	ntv2ThreadExit(&ntv2_gen->monitor_task);
	return;
}

static Ntv2Status ntv2_genlock_initialize(struct ntv2_genlock *ntv2_gen)
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

static bool has_state_changed(struct ntv2_genlock *ntv2_gen)
{
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
}

static struct ntv2_genlock_data* get_genlock_config(struct ntv2_genlock *ntv2_gen, uint32_t lines, uint32_t rate)
{
#if defined (MSWindows)
	UNREFERENCED_PARAMETER(ntv2_gen);
#endif
	struct ntv2_genlock_data* config = NULL;

	if (rate >= GENLOCK_FRAME_RATE_SIZE) return NULL;

	switch (lines)
	{
	case 525:
		if (rate == ntv2_frame_rate_2997) config = s_genlock_525_2997;
		break;
	case 625:
		if (rate == ntv2_frame_rate_2500) config = s_genlock_625_2500;
		break;
	case 750:
		config = s_genlock_750[rate];
		break;
	case 1125:
		config = s_genlock_1125[rate];
		break;
	case 2250:
		config = s_genlock_2250[rate];
		break;
	default:
		break;
	}
		
	return config;
}

static bool configure_genlock(struct ntv2_genlock *ntv2_gen, struct ntv2_genlock_data *config, bool check)
{
	struct ntv2_genlock_data* gdat = config;
	uint8_t data;
	uint32_t count = 0;
	uint32_t errors = 0;
	uint32_t value;
	uint32_t mask;

	if (NTV2_DEBUG_ACTIVE(NTV2_DEBUG_GENLOCK_CHECK)) check = true;

	spi_reset(ntv2_gen);

	if (check) {
		NTV2_MSG_GENLOCK_CHECK("%s: genlock write registers\n", ntv2_gen->name);
	}

	while ((gdat->addr != 0) || (gdat->data != 0))
	{
		if ((gdat->addr == 0) && (gdat->data == 1))
		{
			NTV2_MSG_GENLOCK_CHECK("%s: genlock write delay\n", ntv2_gen->name);
			ntv2EventWaitForSignal(&ntv2_gen->monitor_event, c_genlock_reset_wait, true);
		}
		else
		{
			if (!spi_genlock_write(ntv2_gen, (uint16_t)gdat->addr, (uint8_t)gdat->data)) {
				NTV2_MSG_ERROR("%s: genlock spi write failed\n", ntv2_gen->name);
				return false;
			}
			count++;
		}
		gdat++;
	}

	if (check) {
		gdat = config;

		NTV2_MSG_GENLOCK_CHECK("%s: genlock verify %d registers\n", ntv2_gen->name, count);

		while ((gdat->addr != 0) || (gdat->data != 0))
		{
			if (gdat->addr != 0)
			{
				if (!spi_genlock_read(ntv2_gen, (uint16_t)gdat->addr, &data)) {
					NTV2_MSG_ERROR("%s: genlock spi read failed\n", ntv2_gen->name);
					return false;
				}
				if (data != (uint8_t)gdat->data) {
					NTV2_MSG_GENLOCK_CHECK("%s: genlock verify failed  addr %04x  wrote %02x  read %02x\n",
										   ntv2_gen->name, gdat->addr, gdat->data, data);
					errors++;
				}
			}
			gdat++;
		}

		if (errors > c_configure_error_limit) {
			NTV2_MSG_ERROR("%s: genlock %d configuration read verify errors\n", ntv2_gen->name, errors);
		}
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

static void spi_reset(struct ntv2_genlock *ntv2_gen)
{
	ntv2_gen->page_address = 0xff;

	// reset spi hardware
    reg_write(ntv2_gen, ntv2_reg_spi_reset, 0x0a);

	// configure spi & reset fifos
    reg_write(ntv2_gen, ntv2_reg_spi_slave, 0x0);
	reg_write(ntv2_gen, ntv2_reg_spi_control, 0xe6);
}

static void spi_reset_fifos(struct ntv2_genlock *ntv2_gen)
{
    reg_write(ntv2_gen, ntv2_reg_spi_control, 0xe6);
}

static bool spi_wait_write_empty(struct ntv2_genlock *ntv2_gen)
{
	uint32_t status = 0;
	uint32_t count = 0;

	status = reg_read(ntv2_gen, ntv2_reg_spi_status);
    while ((status & GENL_SPI_WRITE_FIFO_EMPTY) == 0)
	{
		if (count++ > c_spi_timeout) return false;
		if (!ntv2_gen->monitor_enable) return false;
		status = reg_read(ntv2_gen, ntv2_reg_spi_status);
	}

	return true;
}

static bool spi_genlock_write(struct ntv2_genlock *ntv2_gen, uint16_t addr, uint8_t value)
{
	uint8_t page = (addr & 0xff00) >> 8;
    uint8_t reg_8 = addr & 0xff;

    if (!spi_wait_write_empty(ntv2_gen)) return false;
	spi_reset_fifos(ntv2_gen);
	
    if (page !=  ntv2_gen->page_address)
    {
        reg_write(ntv2_gen, ntv2_reg_spi_write, GENL_SPI_SET_ADDR_CMD);
        reg_write(ntv2_gen, ntv2_reg_spi_write, 0x01);         // page register
        reg_write(ntv2_gen, ntv2_reg_spi_write, GENL_SPI_WRITE_CMD);
        reg_write(ntv2_gen, ntv2_reg_spi_write, page);         // data
        ntv2_gen->page_address = page;
    }

    reg_write(ntv2_gen, ntv2_reg_spi_write, GENL_SPI_SET_ADDR_CMD);
    reg_write(ntv2_gen, ntv2_reg_spi_write, reg_8);            // the register
    reg_write(ntv2_gen, ntv2_reg_spi_write, GENL_SPI_WRITE_CMD);
    reg_write(ntv2_gen, ntv2_reg_spi_write, value);

	return true;
}

static bool spi_genlock_read(struct ntv2_genlock *ntv2_gen, uint16_t addr, uint8_t* value)
{
    uint8_t page = (addr & 0xff00) >> 8;
    uint8_t reg_8 = addr & 0xff;
	uint32_t val = 0;
	uint32_t count = 0;
	uint32_t status;

	// reset the fifos
    if (!spi_wait_write_empty(ntv2_gen)) return false;
	spi_reset_fifos(ntv2_gen);

	// update page
    if (page !=  ntv2_gen->page_address)
    {
        reg_write(ntv2_gen, ntv2_reg_spi_write, GENL_SPI_SET_ADDR_CMD);
        reg_write(ntv2_gen, ntv2_reg_spi_write, 0x01);			// page register
        reg_write(ntv2_gen, ntv2_reg_spi_write, GENL_SPI_WRITE_CMD);
        reg_write(ntv2_gen, ntv2_reg_spi_write, page);			// data
        ntv2_gen->page_address = page;
    }

	// read command
    reg_write(ntv2_gen, ntv2_reg_spi_write, GENL_SPI_SET_ADDR_CMD);
    reg_write(ntv2_gen, ntv2_reg_spi_write, reg_8);				// the register
    reg_write(ntv2_gen, ntv2_reg_spi_write, GENL_SPI_READ_CMD);
    reg_write(ntv2_gen, ntv2_reg_spi_write, 0xff);				// dummy write clocks in the read value

	// read data
    if (!spi_wait_write_empty(ntv2_gen)) return false;
    val = reg_read(ntv2_gen, ntv2_reg_spi_read);				// read data until fifo empty
	status = reg_read(ntv2_gen, ntv2_reg_spi_status);
    while ((status & GENL_SPI_READ_FIFO_EMPTY) == 0)									
	{
		val = reg_read(ntv2_gen, ntv2_reg_spi_read);
		status = reg_read(ntv2_gen, ntv2_reg_spi_status);
		if (count++ > 50) return false;
	}

    *value = (uint8_t)(val & 0x00ff);

    return true;
}

static uint32_t reg_read(struct ntv2_genlock *ntv2_gen, const uint32_t *reg)
{
    return ntv2_reg_read(ntv2_gen->system_context, reg, ntv2_gen->index);
}

static void reg_write(struct ntv2_genlock *ntv2_gen, const uint32_t *reg, uint32_t data)
{
    ntv2_reg_write(ntv2_gen->system_context, reg, ntv2_gen->index, data);
}
