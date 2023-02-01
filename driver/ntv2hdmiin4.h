/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2hdmiin4.h
// Purpose:	 HDMI input monitor version 4
//
///////////////////////////////////////////////////////////////

#ifndef NTV2HDMIIN4_HEADER
#define NTV2HDMIIN4_HEADER

#include "ntv2system.h"
#include "ntv2hdmiedid.h"

#define NTV2_HDMIIN4_STRING_SIZE	80


struct ntv2_hdmiin4 {
	int						index;
	char					name[NTV2_HDMIIN4_STRING_SIZE];
	Ntv2SystemContext* 		system_context;
	struct ntv2_hdmiedid*	edid;
	Ntv2SpinLock			state_lock;

	Ntv2Thread 				monitor_task;
	bool					monitor_enable;
	Ntv2Event				monitor_event;

	uint32_t				horizontal_tol;
	uint32_t				vertical_tol;
	
	uint32_t				video_control;
	uint32_t				video_detect0;
	uint32_t				video_detect1;
	uint32_t				video_detect2;
	uint32_t				video_detect3;
	uint32_t				video_detect4;
	uint32_t				video_detect5;
	uint32_t				video_detect6;
	uint32_t				video_detect7;
	uint32_t				tmds_rate;

	bool					input_locked;
	bool					hdmi_mode;
	uint32_t				video_standard;
	uint32_t				frame_rate;
	uint32_t				color_space;
	uint32_t				color_depth;
	uint32_t				aspect_ratio;
	uint32_t				colorimetry;
	uint32_t				quantization;

	bool					audio_swap;
	bool					audio_resample;

	uint32_t				format_clock_count;
	uint32_t				format_raster_count;
};

struct ntv2_hdmiin4 *ntv2_hdmiin4_open(Ntv2SystemContext* sys_con,
									   const char *name, int index);
void ntv2_hdmiin4_close(struct ntv2_hdmiin4 *ntv2_hin);

Ntv2Status ntv2_hdmiin4_configure(struct ntv2_hdmiin4 *ntv2_hin,
								  enum ntv2_edid_type edid_type,
								  int port_index);

Ntv2Status ntv2_hdmiin4_enable(struct ntv2_hdmiin4 *ntv2_hin);
Ntv2Status ntv2_hdmiin4_disable(struct ntv2_hdmiin4 *ntv2_hin);

#endif

