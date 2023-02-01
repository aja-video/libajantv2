/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2infoframe.h
// Purpose:	 HDMI info frame parser
//
///////////////////////////////////////////////////////////////

#ifndef NTV2_INFOFRAME_H
#define NTV2_INFOFRAME_H

#include "ntv2system.h"

struct ntv2_avi_info_data {
	uint32_t		video_standard;
	uint32_t		frame_rate;
	uint32_t		color_space;
	uint32_t		aspect_ratio;
	uint32_t		colorimetry;
	uint32_t		quantization;
};

struct ntv2_drm_info_data {
	uint32_t		eotf;
	uint32_t		metadata_id;
	uint32_t		primary_x0;
	uint32_t		primary_y0;
	uint32_t		primary_x1;
	uint32_t		primary_y1;
	uint32_t		primary_x2;
	uint32_t		primary_y2;
	uint32_t		white_point_x;
	uint32_t		white_point_y;
	uint32_t		luminance_max;
	uint32_t		luminance_min;
	uint32_t		content_level_max;
	uint32_t		frameavr_level_max;
};

struct ntv2_vsp_info_data {
	uint32_t		hdmi_video_format;
	uint32_t		hdmi_vic;
	uint32_t		dolby_vision;
};


bool ntv2_aux_to_avi_info(uint32_t *aux_data, uint32_t aux_size, struct ntv2_avi_info_data *avi_data);
bool ntv2_aux_to_drm_info(uint32_t *aux_data, uint32_t aux_size, struct ntv2_drm_info_data *drm_data);
bool ntv2_aux_to_vsp_info(uint32_t *aux_data, uint32_t aux_size, struct ntv2_vsp_info_data *vsp_data);

#endif
