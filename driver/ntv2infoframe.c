/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2infoframe.c
//
//==========================================================================

#include "ntv2infoframe.h"
#include "ntv2commonreg.h"


/* info frame header */
NTV2_CON(ntv2_info_frame_header,							0);		/* header bytes 1-3 */
	NTV2_FLD(ntv2_fld_header_type,								8,	0);		/* type */
		NTV2_CON(ntv2_header_type_nodata,							0x00);
		NTV2_CON(ntv2_header_type_vendor_specific,					0x81);
		NTV2_CON(ntv2_header_type_video_info,						0x82);
		NTV2_CON(ntv2_header_type_source_product,					0x83);
		NTV2_CON(ntv2_header_type_audio_info,						0x84);
		NTV2_CON(ntv2_header_type_dynamic_range,					0x87);
	NTV2_FLD(ntv2_fld_header_version,							7,	8);		/* version */
	NTV2_FLD(ntv2_fld_header_change,							1,	15);	/* change */
	NTV2_FLD(ntv2_fld_header_length,							8,	16);	/* length */
		NTV2_CON(ntv2_video_info_length,							13);
		NTV2_CON(ntv2_vs_info_length,								6);
		NTV2_CON(ntv2_vs_dolby_length,								24);
	NTV2_FLD(ntv2_fld_header_checksum,							8,	24);	/* checksum (packet byte 0) */

/* auxiliary video information (avi) */
NTV2_CON(ntv2_video_info_byte_1_4,							1);		/* packet bytes 1-4 */
	NTV2_FLD(ntv2_fld_video_info_scan,							2,	0);		/* s */
		NTV2_CON(ntv2_vi_scan_nodata,								0x0);
		NTV2_CON(ntv2_vi_scan_over,									0x1);
		NTV2_CON(ntv2_vi_scan_under,								0x2);
	NTV2_FLD(ntv2_fld_video_info_bar,								2,	2);	/* b */
		NTV2_CON(ntv2_vi_bar_nobar,									0x0);
		NTV2_CON(ntv2_vi_bar_verical,								0x1);
		NTV2_CON(ntv2_vi_bar_horizontal,							0x2);
		NTV2_CON(ntv2_vi_bar_both,									0x3);
	NTV2_FLD(ntv2_fld_video_info_active_format,					1,	4);		/* a */
		NTV2_CON(ntv2_vi_active_format_none,						0x0);
		NTV2_CON(ntv2_vi_active_format_present,						0x1);
	NTV2_FLD(ntv2_fld_video_info_color_space,					3,	5);		/* y */
		NTV2_CON(ntv2_vi_color_space_rgb,							0x0);
		NTV2_CON(ntv2_vi_color_space_422,							0x1);
		NTV2_CON(ntv2_vi_color_space_444,							0x2);
		NTV2_CON(ntv2_vi_color_space_420,							0x3);
	NTV2_FLD(ntv2_fld_video_info_active_aspect,					4,	8);		/* r */
		NTV2_CON(ntv2_vi_active_aspect_nodata,						0x0);
		NTV2_CON(ntv2_vi_active_aspect_box_16x9_top,				0x2);
		NTV2_CON(ntv2_vi_active_aspect_box_14x9_top,				0x3);
		NTV2_CON(ntv2_vi_active_aspect_box_16x9_cen,				0x4);
		NTV2_CON(ntv2_vi_active_aspect_picture,						0x8);
		NTV2_CON(ntv2_vi_active_aspect_4x3_cen,						0x9);
		NTV2_CON(ntv2_vi_active_aspect_16x9_cen,					0xa);
		NTV2_CON(ntv2_vi_active_aspect_14x9_cen,					0xb);
		NTV2_CON(ntv2_vi_active_aspect_4x3_cen_14x9,				0xd);
		NTV2_CON(ntv2_vi_active_aspect_16x9_cen_14x9,				0xe);
		NTV2_CON(ntv2_vi_active_aspect_16x9_cen_4x3,				0xf);
	NTV2_FLD(ntv2_fld_video_info_picture_aspect,				2,	12);	/* m */
		NTV2_CON(ntv2_vi_picture_aspect_nodata,						0x0);
		NTV2_CON(ntv2_vi_picture_aspect_4x3,						0x1);
		NTV2_CON(ntv2_vi_picture_aspect_16x9,						0x2);
	NTV2_FLD(ntv2_fld_video_info_colorimetry,					2,	14);	/* c */
		NTV2_CON(ntv2_vi_colorimetry_nodata,						0x0);
		NTV2_CON(ntv2_vi_colorimetry_170m,							0x1);
		NTV2_CON(ntv2_vi_colorimetry_bt709,							0x2);
		NTV2_CON(ntv2_vi_colorimetry_extended,						0x3);
	NTV2_FLD(ntv2_fld_video_info_scaling,						2,	16);	/* sc */
		NTV2_CON(ntv2_vi_scaling_nodata,							0x0);
		NTV2_CON(ntv2_vi_scaling_horizontal,						0x1);
		NTV2_CON(ntv2_vi_scaling_vertical,							0x2);
		NTV2_CON(ntv2_vi_scaling_both,								0x3);
	NTV2_FLD(ntv2_fld_video_info_rgb_quantization,				2,	18);	/* q */
		NTV2_CON(ntv2_vi_rgb_quantization_default,					0x0);
		NTV2_CON(ntv2_vi_rgb_quantization_limited,					0x1);
		NTV2_CON(ntv2_vi_rgb_quantization_full,						0x2);
	NTV2_FLD(ntv2_fld_video_info_ext_colorimetry,				3,	20);	/* ec */
		NTV2_CON(ntv2_vi_ext_colorimetry_xvycc_601,					0x0);
		NTV2_CON(ntv2_vi_ext_colorimetry_xvycc_709,					0x1);
		NTV2_CON(ntv2_vi_ext_colorimetry_sycc_601,					0x2);
		NTV2_CON(ntv2_vi_ext_colorimetry_adobe_601,					0x3);
		NTV2_CON(ntv2_vi_ext_colorimetry_adobe_rgb,					0x4);
		NTV2_CON(ntv2_vi_ext_colorimetry_bt2020_cl,					0x5);
		NTV2_CON(ntv2_vi_ext_colorimetry_bt2020,					0x6);
		NTV2_CON(ntv2_vi_ext_colorimetry_additional,				0x7);
	NTV2_FLD(ntv2_fld_video_info_it_content,					1,	23);	/* itc */
		NTV2_CON(ntv2_vi_it_content_nodata,							0x0);
		NTV2_CON(ntv2_vi_it_content_valid,							0x1);
	NTV2_FLD(ntv2_fld_video_info_vic,							8,	24);	/* vic */
		NTV2_CON(ntv2_vic_720x480p60,								2);		
		NTV2_CON(ntv2_vic_720x480p60_wide,							3);		
		NTV2_CON(ntv2_vic_1280x720p60,								4);		
		NTV2_CON(ntv2_vic_1920x1080i60,								5);		
		NTV2_CON(ntv2_vic_720x480i60,								6);		
		NTV2_CON(ntv2_vic_720x480i60_wide,							7);		
		NTV2_CON(ntv2_vic_1920x1080p60,								16);		
		NTV2_CON(ntv2_vic_720x576p50,								17);		
		NTV2_CON(ntv2_vic_720x576p50_wide,							18);		
		NTV2_CON(ntv2_vic_1280x720p50,								19);		
		NTV2_CON(ntv2_vic_1920x1080i50,								20);		
		NTV2_CON(ntv2_vic_720x576i50,								21);		
		NTV2_CON(ntv2_vic_720x576i50_wide,							22);		
		NTV2_CON(ntv2_vic_1920x1080p50,								31);		
		NTV2_CON(ntv2_vic_1920x1080p24,								32);		
		NTV2_CON(ntv2_vic_1920x1080p25,								33);		
		NTV2_CON(ntv2_vic_1920x1080p30,								34);		
		NTV2_CON(ntv2_vic_3840x2160p24,								93);		
		NTV2_CON(ntv2_vic_3840x2160p25,								94);		
		NTV2_CON(ntv2_vic_3840x2160p30,								95);		
		NTV2_CON(ntv2_vic_3840x2160p50,								96);		
		NTV2_CON(ntv2_vic_3840x2160p60,								97);		
		NTV2_CON(ntv2_vic_4096x2160p24,								98);		
		NTV2_CON(ntv2_vic_4096x2160p25,								99);		
		NTV2_CON(ntv2_vic_4096x2160p30,								100);		
		NTV2_CON(ntv2_vic_4096x2160p50,								101);		
		NTV2_CON(ntv2_vic_4096x2160p60,								102);		
		NTV2_CON(ntv2_vic_1920x1080p48,								111);		
		NTV2_CON(ntv2_vic_3840x2160p48,								114);		
		NTV2_CON(ntv2_vic_4096x2160p48,								115);		
NTV2_CON(ntv2_video_info_byte_5_8,							2);		/* packet bytes 5-8 */
	NTV2_FLD(ntv2_fld_video_info_pixel_repetition,				4,	0);		/* pr */
		NTV2_CON(ntv2_vi_pixel_repetition_none,						0x0);
		NTV2_CON(ntv2_vi_pixel_repetition_2,						0x1);
		NTV2_CON(ntv2_vi_pixel_repetition_3,						0x2);
		NTV2_CON(ntv2_vi_pixel_repetition_4,						0x3);
		NTV2_CON(ntv2_vi_pixel_repetition_5,						0x4);
		NTV2_CON(ntv2_vi_pixel_repetition_6,						0x5);
		NTV2_CON(ntv2_vi_pixel_repetition_7,						0x6);
		NTV2_CON(ntv2_vi_pixel_repetition_8,						0x7);
		NTV2_CON(ntv2_vi_pixel_repetition_9,						0x8);
		NTV2_CON(ntv2_vi_pixel_repetition_10,						0x9);
	NTV2_FLD(ntv2_fld_video_info_content_type,					2,	4);		/* cn */
		NTV2_CON(ntv2_vi_content_type_graphics,						0x0);
		NTV2_CON(ntv2_vi_content_type_photo,						0x1);
		NTV2_CON(ntv2_vi_content_type_cinema,						0x2);
		NTV2_CON(ntv2_vi_content_type_game,							0x3);
	NTV2_FLD(ntv2_fld_video_info_ycc_quantization,				2,	6);		/* yq */
		NTV2_CON(ntv2_vi_ycc_quantization_limited,					0x0);
		NTV2_CON(ntv2_vi_ycc_quantization_full,						0x1);
	NTV2_FLD(ntv2_fld_video_info_etb_lsb,						8,	8);		/* packet byte 6 */
	NTV2_FLD(ntv2_fld_video_info_etb_msb,						8,	16);	/* packet byte 7 */
	NTV2_FLD(ntv2_fld_video_info_sbb_lsb,						8,	24);	/* packet byte 8 */
NTV2_CON(ntv2_video_info_byte_9_12,							3);		/* packet bytes 9-12 */
	NTV2_FLD(ntv2_fld_video_info_sbb_msb,						8,	0);		/* packet byte 9 */
	NTV2_FLD(ntv2_fld_video_info_elb_lsb,						8,	8);		/* packet byte 10 */
	NTV2_FLD(ntv2_fld_video_info_elb_msb,						8,	16);	/* packet byte 11 */
	NTV2_FLD(ntv2_fld_video_info_srb_lsb,						8,	24);	/* packet byte 12 */
NTV2_CON(ntv2_video_info_byte_13_16,						4);		/* packet bytes 13-16 */
	NTV2_FLD(ntv2_fld_video_info_srb_msb,						8,	0);		/* packet byte 13 */

/* dynamic range and mastering information (drm) */
NTV2_CON(ntv2_drm_info_byte_1_4,							1);		/* packet bytes 1-4 */
	NTV2_FLD(ntv2_fld_drm_info_eotf,							3,	0);		/* packet byte 1 */
		NTV2_CON(ntv2_eotf_sdr,										0x0);
		NTV2_CON(ntv2_eotf_hdr,										0x1);
		NTV2_CON(ntv2_eotf_st2084,									0x2);
		NTV2_CON(ntv2_eotf_hlg,										0x3);
	NTV2_FLD(ntv2_fld_drm_info_metadata_id,						3,	8);		/* packet byte 2 */
		NTV2_CON(ntv2_smd_id_type1,									0x0);
	NTV2_FLD(ntv2_fld_drm_info_primary_x0_lsb,					8,	16);	/* packet byte 3 */
	NTV2_FLD(ntv2_fld_drm_info_primary_x0_msb,					8,	24);	/* packet byte 4 */
NTV2_CON(ntv2_drm_info_byte_5_8,							2);		/* packet bytes 5-8 */
	NTV2_FLD(ntv2_fld_drm_info_primary_y0_lsb,					8,	0);		/* packet byte 5 */
	NTV2_FLD(ntv2_fld_drm_info_primary_y0_msb,					8,	8);		/* packet byte 6 */
	NTV2_FLD(ntv2_fld_drm_info_primary_x1_lsb,					8,	16);	/* packet byte 7 */
	NTV2_FLD(ntv2_fld_drm_info_primary_x1_msb,					8,	24);	/* packet byte 8 */
NTV2_CON(ntv2_drm_info_byte_9_12,							3);		/* packet bytes 9-12 */
	NTV2_FLD(ntv2_fld_drm_info_primary_y1_lsb,					8,	0);		/* packet byte 9 */
	NTV2_FLD(ntv2_fld_drm_info_primary_y1_msb,					8,	8);		/* packet byte 10 */
	NTV2_FLD(ntv2_fld_drm_info_primary_x2_lsb,					8,	16);	/* packet byte 11 */
	NTV2_FLD(ntv2_fld_drm_info_primary_x2_msb,					8,	24);	/* packet byte 12 */
NTV2_CON(ntv2_drm_info_byte_13_16,							4);		/* packet bytes 13-16 */
	NTV2_FLD(ntv2_fld_drm_info_primary_y2_lsb,					8,	0);		/* packet byte 13 */
	NTV2_FLD(ntv2_fld_drm_info_primary_y2_msb,					8,	8);		/* packet byte 14 */
	NTV2_FLD(ntv2_fld_drm_info_white_point_x_lsb,				8,	16);	/* packet byte 15 */
	NTV2_FLD(ntv2_fld_drm_info_white_point_x_msb,				8,	24);	/* packet byte 16 */
NTV2_CON(ntv2_drm_info_byte_17_20,							5);		/* packet bytes 17-20 */
	NTV2_FLD(ntv2_fld_drm_info_white_point_y_lsb,				8,	0);		/* packet byte 17 */
	NTV2_FLD(ntv2_fld_drm_info_white_point_y_msb,				8,	8);		/* packet byte 18 */
	NTV2_FLD(ntv2_fld_drm_info_luminance_max_lsb,				8,	16);	/* packet byte 19 */
	NTV2_FLD(ntv2_fld_drm_info_luminance_max_msb,				8,	24);	/* packet byte 20 */
NTV2_CON(ntv2_drm_info_byte_21_24,							6);		/* packet bytes 21-24 */
	NTV2_FLD(ntv2_fld_drm_info_luminance_min_lsb,				8,	0);		/* packet byte 21 */
	NTV2_FLD(ntv2_fld_drm_info_luminance_min_msb,				8,	8);		/* packet byte 22 */
	NTV2_FLD(ntv2_fld_drm_info_content_level_max_lsb,			8,	16);	/* packet byte 23 */
	NTV2_FLD(ntv2_fld_drm_info_content_level_max_msb,			8,	24);	/* packet byte 24 */
NTV2_CON(ntv2_drm_info_byte_25_26,							7);		/* packet bytes 25-26 */
	NTV2_FLD(ntv2_fld_drm_info_frameavr_level_max_lsb,			8,	0);		/* packet byte 25 */
	NTV2_FLD(ntv2_fld_drm_info_frameavr_level_max_msb,			8,	8);		/* packet byte 26 */

/* vendor specific information (vsp) */
NTV2_CON(ntv2_vsp_info_byte_1_4,							1);		/* packet bytes 1-4 */
	NTV2_FLD(ntv2_fld_vsp_info_ieee_id,							24,	0);		/* packet byte 1 - 3 */
		NTV2_CON(ntv2_ieee_id,										0x000c03);
	NTV2_FLD(ntv2_fld_vsp_info_hdmi_format,						3,	29);	/* packet byte 4 */
NTV2_CON(ntv2_vsp_info_byte_5_8,							2);		/* packet bytes 5-8 */
	NTV2_FLD(ntv2_fld_vsp_info_hdmi_vic,						8,	0);		/* packet byte 5 */


bool ntv2_aux_to_avi_info(uint32_t *aux_data, uint32_t aux_size, struct ntv2_avi_info_data *avi_data)
{
	int i;
	uint32_t sum = 0;
	
	if ((aux_data == NULL) ||
		(aux_size < 5) ||
		(avi_data == NULL))
		return false;

	/* clear avi data */
	memset(avi_data, 0, sizeof(struct ntv2_avi_info_data));

	/* check for avi info present */
	if (NTV2_FLD_GET(ntv2_fld_header_type, aux_data[ntv2_info_frame_header]) != ntv2_header_type_video_info)
		return false;

	/* check checksum */
	for (i = 0; i < (int)aux_size; i++) {
		sum += (aux_data[i] & 0x000000ff);
		sum += (aux_data[i] & 0x0000ff00) >> 8;
		sum += (aux_data[i] & 0x00ff0000) >> 16;
		sum += (aux_data[i] & 0xff000000) >> 24;
	}
	if ((sum & 0xff) != 0)
		return false;

	/* pixel format */
	switch (NTV2_FLD_GET(ntv2_fld_video_info_color_space, aux_data[ntv2_video_info_byte_1_4])) {
	case ntv2_vi_color_space_rgb: avi_data->color_space = ntv2_color_space_rgb444; break;
	case ntv2_vi_color_space_422: avi_data->color_space = ntv2_color_space_yuv422; break;
	case ntv2_vi_color_space_444: avi_data->color_space = ntv2_color_space_yuv444; break;
	case ntv2_vi_color_space_420: avi_data->color_space = ntv2_color_space_yuv420; break;
	default: avi_data->color_space = ntv2_color_space_none; break;
	}

	/* picture aspect ratio */
	switch (NTV2_FLD_GET(ntv2_fld_video_info_picture_aspect, aux_data[ntv2_video_info_byte_1_4])) {
	case ntv2_vi_picture_aspect_4x3: avi_data->aspect_ratio = ntv2_aspect_ratio_4x3; break;
	case ntv2_vi_picture_aspect_16x9: avi_data->aspect_ratio = ntv2_aspect_ratio_16x9; break;
	default: avi_data->aspect_ratio = ntv2_aspect_ratio_unknown; break;
	}

	/* colorimetry */
	switch (NTV2_FLD_GET(ntv2_fld_video_info_colorimetry, aux_data[ntv2_video_info_byte_1_4])) {
	case ntv2_vi_colorimetry_nodata: avi_data->colorimetry = ntv2_colorimetry_unknown; break;
	case ntv2_vi_colorimetry_170m: avi_data->colorimetry = ntv2_colorimetry_170m; break;
	case ntv2_vi_colorimetry_bt709: avi_data->colorimetry = ntv2_colorimetry_bt709; break;
	case ntv2_vi_colorimetry_extended:
		switch (NTV2_FLD_GET(ntv2_fld_video_info_ext_colorimetry, aux_data[ntv2_video_info_byte_1_4])) {
		case ntv2_vi_ext_colorimetry_xvycc_601: avi_data->colorimetry = ntv2_colorimetry_xvycc_601; break;
		case ntv2_vi_ext_colorimetry_xvycc_709: avi_data->colorimetry = ntv2_colorimetry_xvycc_709; break;
		case ntv2_vi_ext_colorimetry_adobe_601: avi_data->colorimetry = ntv2_colorimetry_adobe_601; break;
		case ntv2_vi_ext_colorimetry_adobe_rgb: avi_data->colorimetry = ntv2_colorimetry_adobe_rgb; break;
		case ntv2_vi_ext_colorimetry_bt2020_cl: avi_data->colorimetry = ntv2_colorimetry_bt2020_cl; break;
		case ntv2_vi_ext_colorimetry_bt2020: avi_data->colorimetry = ntv2_colorimetry_bt2020; break;
		case ntv2_vi_ext_colorimetry_additional: avi_data->colorimetry = ntv2_colorimetry_dcip3_d65; break;
		default: avi_data->colorimetry = ntv2_colorimetry_unknown; break;
		}
		break;
	default: avi_data->colorimetry = ntv2_colorimetry_unknown; break;
	}

	/* quantization */
	if ((avi_data->colorimetry == ntv2_colorimetry_xvycc_601) ||
		(avi_data->colorimetry == ntv2_colorimetry_xvycc_709)) {
		switch (NTV2_FLD_GET(ntv2_fld_video_info_ycc_quantization, aux_data[ntv2_video_info_byte_5_8])) {
		case ntv2_vi_ycc_quantization_limited: avi_data->quantization = ntv2_quantization_limited; break;
		case ntv2_vi_ycc_quantization_full: avi_data->quantization = ntv2_quantization_full; break;
		default: avi_data->quantization = ntv2_quantization_unknown; break;
		}
	} else {
		switch (NTV2_FLD_GET(ntv2_fld_video_info_rgb_quantization, aux_data[ntv2_video_info_byte_1_4])) {
		case ntv2_vi_rgb_quantization_default: avi_data->quantization = ntv2_quantization_default; break;
		case ntv2_vi_rgb_quantization_limited: avi_data->quantization = ntv2_quantization_limited; break;
		case ntv2_vi_rgb_quantization_full: avi_data->quantization = ntv2_quantization_full; break;
		default: avi_data->quantization = ntv2_quantization_unknown; break;
		}
	}

	/* vic */
	switch (NTV2_FLD_GET(ntv2_fld_video_info_vic, aux_data[ntv2_video_info_byte_1_4])) {
	case ntv2_vic_1280x720p60:
		avi_data->video_standard = ntv2_video_standard_720p;
		avi_data->frame_rate = ntv2_frame_rate_6000;
		break;
	case ntv2_vic_1920x1080i60:
		avi_data->video_standard = ntv2_video_standard_1080i;
		avi_data->frame_rate = ntv2_frame_rate_3000;
		break;
	case ntv2_vic_720x480i60:
		avi_data->video_standard = ntv2_video_standard_525i;
		avi_data->frame_rate = ntv2_frame_rate_2997;
		break;
	case ntv2_vic_720x480i60_wide:
		avi_data->video_standard = ntv2_video_standard_525i;
		avi_data->frame_rate = ntv2_frame_rate_2997;
		break;
	case ntv2_vic_1920x1080p60:
		avi_data->video_standard = ntv2_video_standard_1080p;
		avi_data->frame_rate = ntv2_frame_rate_6000;
		break;
	case ntv2_vic_1280x720p50:
		avi_data->video_standard = ntv2_video_standard_720p;
		avi_data->frame_rate = ntv2_frame_rate_5000;
		break;
	case ntv2_vic_1920x1080i50:
		avi_data->video_standard = ntv2_video_standard_1080i;
		avi_data->frame_rate = ntv2_frame_rate_2500;
		break;
	case ntv2_vic_720x576i50:
		avi_data->video_standard = ntv2_video_standard_625i;
		avi_data->frame_rate = ntv2_frame_rate_2500;
		break;
	case ntv2_vic_720x576i50_wide:
		avi_data->video_standard = ntv2_video_standard_625i;
		avi_data->frame_rate = ntv2_frame_rate_2500;
		break;
	case ntv2_vic_1920x1080p50:
		avi_data->video_standard = ntv2_video_standard_1080p;
		avi_data->frame_rate = ntv2_frame_rate_5000;
		break;
	case ntv2_vic_1920x1080p24:
		avi_data->video_standard = ntv2_video_standard_1080p;
		avi_data->frame_rate = ntv2_frame_rate_2400;
		break;
	case ntv2_vic_1920x1080p25:
		avi_data->video_standard = ntv2_video_standard_1080p;
		avi_data->frame_rate = ntv2_frame_rate_2500;
		break;
	case ntv2_vic_1920x1080p30:
		avi_data->video_standard = ntv2_video_standard_1080p;
		avi_data->frame_rate = ntv2_frame_rate_3000;
		break;
	case ntv2_vic_3840x2160p24:
		avi_data->video_standard = ntv2_video_standard_3840x2160p;
		avi_data->frame_rate = ntv2_frame_rate_2400;
		break;
	case ntv2_vic_3840x2160p25:
		avi_data->video_standard = ntv2_video_standard_3840x2160p;
		avi_data->frame_rate = ntv2_frame_rate_2500;
		break;
	case ntv2_vic_3840x2160p30:
		avi_data->video_standard = ntv2_video_standard_3840x2160p;
		avi_data->frame_rate = ntv2_frame_rate_3000;
		break;
	case ntv2_vic_3840x2160p50:
		avi_data->video_standard = ntv2_video_standard_3840x2160p;
		avi_data->frame_rate = ntv2_frame_rate_5000;
		break;
	case ntv2_vic_3840x2160p60:
		avi_data->video_standard = ntv2_video_standard_3840x2160p;
		avi_data->frame_rate = ntv2_frame_rate_6000;
		break;
	case ntv2_vic_4096x2160p24:
		avi_data->video_standard = ntv2_video_standard_4096x2160p;
		avi_data->frame_rate = ntv2_frame_rate_2400;
		break;
	case ntv2_vic_4096x2160p25:
		avi_data->video_standard = ntv2_video_standard_4096x2160p;
		avi_data->frame_rate = ntv2_frame_rate_2500;
		break;
	case ntv2_vic_4096x2160p30:
		avi_data->video_standard = ntv2_video_standard_4096x2160p;
		avi_data->frame_rate = ntv2_frame_rate_3000;
		break;
	case ntv2_vic_4096x2160p50:
		avi_data->video_standard = ntv2_video_standard_4096x2160p;
		avi_data->frame_rate = ntv2_frame_rate_5000;
		break;
	case ntv2_vic_4096x2160p60:
		avi_data->video_standard = ntv2_video_standard_4096x2160p;
		avi_data->frame_rate = ntv2_frame_rate_6000;
		break;
	case ntv2_vic_1920x1080p48:
		avi_data->video_standard = ntv2_video_standard_1080p;
		avi_data->frame_rate = ntv2_frame_rate_4800;
		break;
	case ntv2_vic_3840x2160p48:
		avi_data->video_standard = ntv2_video_standard_3840x2160p;
		avi_data->frame_rate = ntv2_frame_rate_4800;
		break;
	case ntv2_vic_4096x2160p48:
		avi_data->video_standard = ntv2_video_standard_4096x2160p;
		avi_data->frame_rate = ntv2_frame_rate_4800;
		break;
	default:
		avi_data->video_standard = ntv2_video_standard_none;
		avi_data->frame_rate = ntv2_frame_rate_none;
		break;
	}

	return true;
}

bool ntv2_aux_to_drm_info(uint32_t *aux_data, uint32_t aux_size, struct ntv2_drm_info_data *drm_data)
{
	int i;
	uint32_t sum = 0;
	
	if ((aux_data == NULL) ||
		(aux_size < 5) ||
		(drm_data == NULL))
		return false;

	/* clear drm data */
	memset(drm_data, 0, sizeof(struct ntv2_drm_info_data));

	/* check for drm info present */
	if (NTV2_FLD_GET(ntv2_fld_header_type, aux_data[ntv2_info_frame_header]) != ntv2_header_type_dynamic_range)
		return false;

	/* check checksum */
	for (i = 0; i < (int)aux_size; i++) {
		sum += (aux_data[i] & 0x000000ff);
		sum += (aux_data[i] & 0x0000ff00) >> 8;
		sum += (aux_data[i] & 0x00ff0000) >> 16;
		sum += (aux_data[i] & 0xff000000) >> 24;
	}
	if ((sum & 0xff) != 0)
		return false;

	switch(NTV2_FLD_GET(ntv2_fld_drm_info_eotf, aux_data[ntv2_drm_info_byte_1_4]))
	{
	case ntv2_eotf_sdr: drm_data->eotf = ntv2_hdr_eotf_sdr; break;
	case ntv2_eotf_hdr: drm_data->eotf = ntv2_hdr_eotf_hdr; break;
	case ntv2_eotf_st2084: drm_data->eotf = ntv2_hdr_eotf_st2084; break;
	case ntv2_eotf_hlg: drm_data->eotf = ntv2_hdr_eotf_hlg; break;
	default:
		return false;
	}
	
	drm_data->metadata_id = NTV2_FLD_GET(ntv2_fld_drm_info_metadata_id, aux_data[ntv2_drm_info_byte_1_4]);
	if (drm_data->metadata_id != ntv2_smd_id_type1)
		return false;

	drm_data->primary_x0 = NTV2_FLD_GET(ntv2_fld_drm_info_primary_x0_lsb, aux_data[ntv2_drm_info_byte_1_4]);
	drm_data->primary_x0 |= NTV2_FLD_GET(ntv2_fld_drm_info_primary_x0_msb, aux_data[ntv2_drm_info_byte_1_4]) << 8;

	drm_data->primary_y0 = NTV2_FLD_GET(ntv2_fld_drm_info_primary_y0_lsb, aux_data[ntv2_drm_info_byte_5_8]);
	drm_data->primary_y0 |= NTV2_FLD_GET(ntv2_fld_drm_info_primary_y0_msb, aux_data[ntv2_drm_info_byte_5_8]) << 8;
	drm_data->primary_x1 = NTV2_FLD_GET(ntv2_fld_drm_info_primary_x1_lsb, aux_data[ntv2_drm_info_byte_5_8]);
	drm_data->primary_x1 |= NTV2_FLD_GET(ntv2_fld_drm_info_primary_x1_msb, aux_data[ntv2_drm_info_byte_5_8]) << 8;

	drm_data->primary_y1 = NTV2_FLD_GET(ntv2_fld_drm_info_primary_y1_lsb, aux_data[ntv2_drm_info_byte_9_12]);
	drm_data->primary_y1 |= NTV2_FLD_GET(ntv2_fld_drm_info_primary_y1_msb, aux_data[ntv2_drm_info_byte_9_12]) << 8;
	drm_data->primary_x2 = NTV2_FLD_GET(ntv2_fld_drm_info_primary_x2_lsb, aux_data[ntv2_drm_info_byte_9_12]);
	drm_data->primary_x2 |= NTV2_FLD_GET(ntv2_fld_drm_info_primary_x2_msb, aux_data[ntv2_drm_info_byte_9_12]) << 8;

	drm_data->primary_y2 = NTV2_FLD_GET(ntv2_fld_drm_info_primary_y2_lsb, aux_data[ntv2_drm_info_byte_13_16]);
	drm_data->primary_y2 |= NTV2_FLD_GET(ntv2_fld_drm_info_primary_y2_msb, aux_data[ntv2_drm_info_byte_13_16]) << 8;
	drm_data->white_point_x = NTV2_FLD_GET(ntv2_fld_drm_info_white_point_x_lsb, aux_data[ntv2_drm_info_byte_13_16]);
	drm_data->white_point_x |= NTV2_FLD_GET(ntv2_fld_drm_info_white_point_x_msb, aux_data[ntv2_drm_info_byte_13_16]) << 8;

	drm_data->white_point_y = NTV2_FLD_GET(ntv2_fld_drm_info_white_point_y_lsb, aux_data[ntv2_drm_info_byte_17_20]);
	drm_data->white_point_y |= NTV2_FLD_GET(ntv2_fld_drm_info_white_point_y_msb, aux_data[ntv2_drm_info_byte_17_20]) << 8;
	drm_data->luminance_max = NTV2_FLD_GET(ntv2_fld_drm_info_luminance_max_lsb, aux_data[ntv2_drm_info_byte_17_20]);
	drm_data->luminance_max |= NTV2_FLD_GET(ntv2_fld_drm_info_luminance_max_msb, aux_data[ntv2_drm_info_byte_17_20]) << 8;

	drm_data->luminance_min = NTV2_FLD_GET(ntv2_fld_drm_info_luminance_min_lsb, aux_data[ntv2_drm_info_byte_21_24]);
	drm_data->luminance_min |= NTV2_FLD_GET(ntv2_fld_drm_info_luminance_min_msb, aux_data[ntv2_drm_info_byte_21_24]) << 8;
	drm_data->content_level_max = NTV2_FLD_GET(ntv2_fld_drm_info_content_level_max_lsb, aux_data[ntv2_drm_info_byte_21_24]);
	drm_data->content_level_max |= NTV2_FLD_GET(ntv2_fld_drm_info_content_level_max_msb, aux_data[ntv2_drm_info_byte_21_24]) << 8;

	drm_data->frameavr_level_max = NTV2_FLD_GET(ntv2_fld_drm_info_frameavr_level_max_lsb, aux_data[ntv2_drm_info_byte_25_26]);
	drm_data->frameavr_level_max |= NTV2_FLD_GET(ntv2_fld_drm_info_frameavr_level_max_msb, aux_data[ntv2_drm_info_byte_25_26]) << 8;

	return true;
}

bool ntv2_aux_to_vsp_info(uint32_t *aux_data, uint32_t aux_size, struct ntv2_vsp_info_data *vsp_data)
{
	int i;
	uint32_t sum = 0;
	
	if ((aux_data == NULL) ||
		(aux_size < 5) ||
		(vsp_data == NULL))
		return false;

	/* clear drm data */
	memset(vsp_data, 0, sizeof(struct ntv2_vsp_info_data));

	/* check for drm info present */
	if (NTV2_FLD_GET(ntv2_fld_header_type, aux_data[ntv2_info_frame_header]) != ntv2_header_type_vendor_specific)
		return false;

	/* check checksum */
	for (i = 0; i < (int)aux_size; i++) {
		sum += (aux_data[i] & 0x000000ff);
		sum += (aux_data[i] & 0x0000ff00) >> 8;
		sum += (aux_data[i] & 0x00ff0000) >> 16;
		sum += (aux_data[i] & 0xff000000) >> 24;
	}
	if ((sum & 0xff) != 0)
		return false;

	if (NTV2_FLD_GET(ntv2_fld_vsp_info_ieee_id, aux_data[ntv2_vsp_info_byte_1_4]) != ntv2_ieee_id)
		return false;

	vsp_data->hdmi_video_format = NTV2_FLD_GET(ntv2_fld_vsp_info_hdmi_format, aux_data[ntv2_vsp_info_byte_1_4]);
	vsp_data->hdmi_vic = NTV2_FLD_GET(ntv2_fld_vsp_info_hdmi_vic, aux_data[ntv2_vsp_info_byte_5_8]);

	if (NTV2_FLD_GET(ntv2_fld_header_length, aux_data[ntv2_info_frame_header]) == ntv2_vs_dolby_length)
		vsp_data->dolby_vision = 1;

	return true;
}
