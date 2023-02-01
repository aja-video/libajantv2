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
// Filename: ntv2hdmiin.h
// Purpose:	 Header Kona2/Xena2 specific functions.
//
///////////////////////////////////////////////////////////////

#ifndef NTV2HDMIIN_HEADER
#define NTV2HDMIIN_HEADER

#include "ntv2system.h"
#include "ntv2hdmiedid.h"

#define NTV2_STRING_SIZE			80


struct ntv2_hdmiin_format {
	uint32_t					video_standard;
	uint32_t					frame_rate;
	uint32_t					frame_flags;
	uint32_t					pixel_flags;
};

struct ntv2_hdmiin {
	int							index;
	char						name[NTV2_STRING_SIZE];
	Ntv2SystemContext* 			system_context;
	struct ntv2_hdmiedid*		edid;	
	Ntv2SpinLock				state_lock;
	Ntv2Thread 					monitor_task;
	bool						monitor_enable;
	Ntv2Event					monitor_event;

	uint8_t						i2c_device;
	uint8_t						i2c_hpa_default;
	uint8_t						i2c_color_default;
	uint32_t					i2c_reset_count;
	uint32_t					lock_error_count;

	struct ntv2_hdmiin_format	dvi_format;
	struct ntv2_hdmiin_format	hdmi_format;
	struct ntv2_hdmiin_format	video_format;

	uint32_t					relock_reports;
	bool						hdmi_mode;
	bool						hdcp_mode;
	bool						derep_mode;
	bool						uhd_mode;
	bool						interlaced_mode;
	bool						pixel_double_mode;
	bool						yuv_mode;
	bool						deep_color_10bit;
	bool						deep_color_12bit;
	bool						prefer_yuv;
	bool						prefer_rgb;
	uint32_t					horizontal_tol;
	uint32_t					vertical_tol;

	bool						cable_present;
	bool						clock_present;
	bool						input_locked;
	bool						avi_packet_present;
	bool						vsi_packet_present;

	uint32_t					h_active_pixels;
	uint32_t					h_total_pixels;
	uint32_t					h_front_porch_pixels;
	uint32_t					h_sync_pixels; 
	uint32_t					h_back_porch_pixels;
	uint32_t					v_total_lines0;
	uint32_t					v_total_lines1;
	uint32_t					v_active_lines0;
	uint32_t					v_active_lines1;
	uint32_t					v_sync_lines0;
	uint32_t					v_sync_lines1;
	uint32_t					v_front_porch_lines0;
	uint32_t					v_front_porch_lines1;
	uint32_t					v_back_porch_lines0;
	uint32_t					v_back_porch_lines1;
	uint32_t					v_frequency;
	uint32_t					tmds_frequency;

	uint32_t					color_space;
	uint32_t					color_depth;
	uint32_t					aspect_ratio;
	uint32_t					colorimetry;
	uint32_t					quantization;
};

struct ntv2_hdmiin *ntv2_hdmiin_open(Ntv2SystemContext* sys_con,
									 const char *name, int index);
void ntv2_hdmiin_close(struct ntv2_hdmiin *ntv2_hin);

Ntv2Status ntv2_hdmiin_configure(struct ntv2_hdmiin *ntv2_hin,
								 enum ntv2_edid_type edid_type, int port_index);

Ntv2Status ntv2_hdmiin_enable(struct ntv2_hdmiin *ntv2_hin);
Ntv2Status ntv2_hdmiin_disable(struct ntv2_hdmiin *ntv2_hin);

void ntv2_hdmiin_set_active(struct ntv2_hdmiin *ntv2_hin, bool active);

#endif

