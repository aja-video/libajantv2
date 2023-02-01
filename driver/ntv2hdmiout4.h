/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2hdmiout4.h
// Purpose:	 HDMI output monitor version 4
//
///////////////////////////////////////////////////////////////

#ifndef NTV2HDMIOUT4_HEADER
#define NTV2HDMIOUT4_HEADER

#include "ntv2system.h"
#include "ntv2displayid.h"

#define NTV2_HDMIOUT4_STRING_SIZE	80

struct ntv2_hdmiout4 {
	int					index;
	char				name[NTV2_HDMIOUT4_STRING_SIZE];
	Ntv2SystemContext* 	system_context;
	Ntv2SpinLock		state_lock;

	Ntv2Thread 			monitor_task;
	bool				monitor_enable;
	Ntv2Event			monitor_event;

	struct ntv2_displayid edid;

	uint32_t			hot_plug_count;
	uint32_t			hdmi_config;
	uint32_t			hdmi_source;
	uint32_t			hdmi_control;
	uint32_t			hdmi_hdr;

	uint32_t			hdr_green_primary;
	uint32_t			hdr_blue_primary;
	uint32_t			hdr_red_primary;
	uint32_t			hdr_white_point;
	uint32_t			hdr_master_luminance;
	uint32_t			hdr_light_level;
	uint32_t			hdr_control;

	bool				hdmi_mode;
	bool				scdc_mode;
	bool				scdc_active;
	bool				output_enable;
	bool				sink_present;
	bool				force_hpd;

	bool				force_config;
	bool				prefer_420;
	bool				hdr_enable;
	bool				dolby_vision;
	bool				crop_enable;
	bool				full_range;
	bool				sd_wide;
	uint32_t			video_standard;
	uint32_t			frame_rate;
	uint32_t			color_space;
	uint32_t			color_depth;

	uint32_t			audio_input;
	bool				audio_upper;
	bool				audio_swap;
	uint32_t			audio_channels;
	uint32_t			audio_select;
	uint32_t			audio_rate;
	uint32_t			audio_format;
	
	uint8_t				avi_vic;
	uint8_t				hdmi_vic;

	uint32_t			scdc_sink_scramble;
	uint32_t			scdc_sink_clock;
	uint32_t			scdc_sink_valid_ch0;
	uint32_t			scdc_sink_valid_ch1;
	uint32_t			scdc_sink_valid_ch2;
	uint32_t			scdc_sink_error_ch0;
	uint32_t			scdc_sink_error_ch1;
	uint32_t			scdc_sink_error_ch2;

	char*				vendor_name;
	char*				product_name;
};

#ifdef __cplusplus
extern "C"
{
#endif

struct ntv2_hdmiout4 *ntv2_hdmiout4_open(Ntv2SystemContext* sys_con,
										 const char *name, int index);
void ntv2_hdmiout4_close(struct ntv2_hdmiout4 *ntv2_hout);

	Ntv2Status ntv2_hdmiout4_configure(struct ntv2_hdmiout4 *ntv2_hout);

Ntv2Status ntv2_hdmiout4_enable(struct ntv2_hdmiout4 *ntv2_hout);
Ntv2Status ntv2_hdmiout4_disable(struct ntv2_hdmiout4 *ntv2_hout);

Ntv2Status ntv2_hdmiout4_write_info_frame(struct ntv2_hdmiout4 *ntv2_hout, 
										  uint32_t size,
										  uint8_t *data);
Ntv2Status ntv2_hdmiout4_clear_info_frames(struct ntv2_hdmiout4 *ntv2_hout);


#ifdef __cplusplus
}
#endif

#endif
