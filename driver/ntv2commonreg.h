/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2commonreg.h
// Purpose:	 HDMI version 4 common register functions
//
///////////////////////////////////////////////////////////////

#ifndef NTV2HDMI4REG_HEADER
#define NTV2HDMI4REG_HEADER

#include "ntv2system.h"
#include "ajatypes.h"


#define NTV2_EXPAND(x) x
#define NTV2_REG_ARGS(a1, a2, a3, a4, a5, a6, a7, a8, aN, ...) aN
#define NTV2_REG(reg, ...) \
	static const uint32_t reg[] = { NTV2_EXPAND(NTV2_REG_ARGS(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1)), __VA_ARGS__ }
#define NTV2_FLD(field, size, shift) \
	static const uint32_t field = (((size) << 16) | (shift))
#define NTV2_CON(con, value) enum { con = (value) }

#define NTV2_REG_COUNT(reg) (reg[0])
#define NTV2_REG_NUM(reg, index) (((index) < NTV2_REG_COUNT(reg))? reg[(index) + 1] : 0)

#define NTV2_FLD_SIZE(field) ((field) >> 16)
#define NTV2_FLD_SHIFT(field) ((field) & 0xffff)
#define NTV2_FLD_MASK(field) ((((uint32_t)0x1 << NTV2_FLD_SIZE(field)) - 1) << NTV2_FLD_SHIFT(field))
#define NTV2_FLD_GET(field, value) ((((uint32_t)value) & NTV2_FLD_MASK(field)) >> NTV2_FLD_SHIFT(field))
#define NTV2_FLD_SET(field, value) ((((uint32_t)value) << NTV2_FLD_SHIFT(field)) & NTV2_FLD_MASK(field))

/* video standard */
NTV2_CON(ntv2_video_standard_1080i,				0);
NTV2_CON(ntv2_video_standard_720p,				1);
NTV2_CON(ntv2_video_standard_525i,				2);
NTV2_CON(ntv2_video_standard_625i,				3);
NTV2_CON(ntv2_video_standard_1080p,				4);
NTV2_CON(ntv2_video_standard_2048x1556,			5);
NTV2_CON(ntv2_video_standard_2048x1080p,		6);
NTV2_CON(ntv2_video_standard_2048x1080i,		7);
NTV2_CON(ntv2_video_standard_3840x2160p,		8);
NTV2_CON(ntv2_video_standard_4096x2160p,		9);
NTV2_CON(ntv2_video_standard_3840_hfr,			10);
NTV2_CON(ntv2_video_standard_4096_hfr,			11);
NTV2_CON(ntv2_video_standard_7680,				12);
NTV2_CON(ntv2_video_standard_8192,				13);
NTV2_CON(ntv2_video_standard_3840i,				14);
NTV2_CON(ntv2_video_standard_4096i,				15);
NTV2_CON(ntv2_video_standard_none,				16);

/* video frame rate */
NTV2_CON(ntv2_frame_rate_none,					0);
NTV2_CON(ntv2_frame_rate_6000,					1);
NTV2_CON(ntv2_frame_rate_5994,					2);
NTV2_CON(ntv2_frame_rate_3000,					3);
NTV2_CON(ntv2_frame_rate_2997,					4);
NTV2_CON(ntv2_frame_rate_2500,					5);
NTV2_CON(ntv2_frame_rate_2400,					6);
NTV2_CON(ntv2_frame_rate_2398,					7);
NTV2_CON(ntv2_frame_rate_5000,					8);
NTV2_CON(ntv2_frame_rate_4800,					9);
NTV2_CON(ntv2_frame_rate_4795,					10);
NTV2_CON(ntv2_frame_rate_12000,					11);
NTV2_CON(ntv2_frame_rate_11988,					12);
NTV2_CON(ntv2_frame_rate_1500,					13);
NTV2_CON(ntv2_frame_rate_1498,					14);

/* video frame geometry */
NTV2_CON(ntv2_frame_geometry_1920x1080,         0);
NTV2_CON(ntv2_frame_geometry_1280x720,          1);
NTV2_CON(ntv2_frame_geometry_720x486,           2);
NTV2_CON(ntv2_frame_geometry_720x576,           3);
NTV2_CON(ntv2_frame_geometry_1920x1114,         4);
NTV2_CON(ntv2_frame_geometry_2048x1114,         5);
NTV2_CON(ntv2_frame_geometry_720x508,           6);
NTV2_CON(ntv2_frame_geometry_720x598,           7);
NTV2_CON(ntv2_frame_geometry_1920x1112,         8);
NTV2_CON(ntv2_frame_geometry_1280x740,          9);
NTV2_CON(ntv2_frame_geometry_2048x1080,         10);
NTV2_CON(ntv2_frame_geometry_2048x1556,         11);
NTV2_CON(ntv2_frame_geometry_2048x1588,         12);
NTV2_CON(ntv2_frame_geometry_2048x1112,         13);
NTV2_CON(ntv2_frame_geometry_720x514,           14);
NTV2_CON(ntv2_frame_geometry_720x612,           15);

/* video scan */
NTV2_CON(ntv2_video_scan_unknown,               0);
NTV2_CON(ntv2_video_scan_progressive,           1);
NTV2_CON(ntv2_video_scan_top_first,             2);
NTV2_CON(ntv2_video_scan_bottom_first,          3);

/* pixel format */
NTV2_CON(ntv2_pixel_format_10bit_ycbcr,         0);
NTV2_CON(ntv2_pixel_format_8bit_ycbcr,          1);
NTV2_CON(ntv2_pixel_format_argb,                2);
NTV2_CON(ntv2_pixel_format_rgba,                3);
NTV2_CON(ntv2_pixel_format_10bit_rgb,           4);
NTV2_CON(ntv2_pixel_format_8bit_ycbcr_yuy2,     5);
NTV2_CON(ntv2_pixel_format_abgr,                6);
NTV2_CON(ntv2_pixel_format_10bit_dpx,           7);
NTV2_CON(ntv2_pixel_format_10bit_ycbcr_dpx,     8);
NTV2_CON(ntv2_pixel_format_8bit_dvcpro,         9);
NTV2_CON(ntv2_pixel_format_8bit_ycbcr_420pl3,   10);
NTV2_CON(ntv2_pixel_format_8bit_hdv,            11);
NTV2_CON(ntv2_pixel_format_24bit_rgb,           12);
NTV2_CON(ntv2_pixel_format_24bit_bgr,           13);
NTV2_CON(ntv2_pixel_format_10bit_ycbcra,        14);
NTV2_CON(ntv2_pixel_format_10bit_dpx_le,        15);
NTV2_CON(ntv2_pixel_format_48bit_rgb,           16);
NTV2_CON(ntv2_pixel_format_12bit_rgb_packed,    17);
NTV2_CON(ntv2_pixel_format_prores_dvcpro,       18);
NTV2_CON(ntv2_pixel_format_prores_hdv,          19);
NTV2_CON(ntv2_pixel_format_10bit_rgb_packed,    20);
NTV2_CON(ntv2_pixel_format_10bit_argb,          21);
NTV2_CON(ntv2_pixel_format_16bit_argb,          22);
NTV2_CON(ntv2_pixel_format_8bit_ycbcr_422pl3,   23);
NTV2_CON(ntv2_pixel_format_10bit_raw_rgb,       24);
NTV2_CON(ntv2_pixel_format_10bit_raw_ycbcr,     25);
NTV2_CON(ntv2_pixel_format_10bit_ycbcr_420pl3_le,  26);
NTV2_CON(ntv2_pixel_format_10bit_ycbcr_422pl3_le,  27);
NTV2_CON(ntv2_pixel_format_10bit_ycbcr_420pl2,  28);
NTV2_CON(ntv2_pixel_format_10bit_ycbcr_422pl2,  29);
NTV2_CON(ntv2_pixel_format_8bit_ycbcr_420pl2,   30);
NTV2_CON(ntv2_pixel_format_8bit_ycbcr_422pl2,   31);

/* frame format */
NTV2_CON(ntv2_frame_format_unknown,             0);
NTV2_CON(ntv2_frame_format_packed,              1);
NTV2_CON(ntv2_frame_format_2plane,              2);
NTV2_CON(ntv2_frame_format_3plane,              3);
NTV2_CON(ntv2_frame_format_raw,                 4);

/* pixel rate */
NTV2_CON(ntv2_pixel_rate_none,                  0);
NTV2_CON(ntv2_pixel_rate_1350,                  1);
NTV2_CON(ntv2_pixel_rate_2700,                  2);
NTV2_CON(ntv2_pixel_rate_7418,                  3);
NTV2_CON(ntv2_pixel_rate_7425,                  4);
NTV2_CON(ntv2_pixel_rate_14835,                 5);
NTV2_CON(ntv2_pixel_rate_14850,                 6);

/* register syncronization */
NTV2_CON(ntv2_con_reg_sync_field,               0);
NTV2_CON(ntv2_con_reg_sync_frame,               1);
NTV2_CON(ntv2_con_reg_sync_immediate,           2);
NTV2_CON(ntv2_con_reg_sync_field_10_lines,      3);

/* color space */
NTV2_CON(ntv2_color_space_yuv422,				0);
NTV2_CON(ntv2_color_space_rgb444,				1);
NTV2_CON(ntv2_color_space_yuv444,				2);
NTV2_CON(ntv2_color_space_yuv420,				3);
NTV2_CON(ntv2_color_space_none,					4);

/* color depth */
NTV2_CON(ntv2_color_depth_8bit,					0);
NTV2_CON(ntv2_color_depth_10bit,				1);
NTV2_CON(ntv2_color_depth_12bit,				2);
NTV2_CON(ntv2_color_depth_16bit,				3);
NTV2_CON(ntv2_color_depth_none,					4);

/* reference source */
NTV2_CON(ntv2_ref_source_external,				0);
NTV2_CON(ntv2_ref_source_input_1,				1);
NTV2_CON(ntv2_ref_source_input_2,				2);
NTV2_CON(ntv2_ref_source_freerun,				3);
NTV2_CON(ntv2_ref_source_analog,				4);
NTV2_CON(ntv2_ref_source_hdmi,					5);
NTV2_CON(ntv2_ref_source_input_3,				6);
NTV2_CON(ntv2_ref_source_input_4,				7);
NTV2_CON(ntv2_ref_source_input_5,				8);
NTV2_CON(ntv2_ref_source_input_6,				9);
NTV2_CON(ntv2_ref_source_input_7,				10);
NTV2_CON(ntv2_ref_source_input_8,				11);
NTV2_CON(ntv2_ref_source_sfp1_ptp,				12);
NTV2_CON(ntv2_ref_source_sfp1_pcr,				13);
NTV2_CON(ntv2_ref_source_sfp2_ptp,				14);
NTV2_CON(ntv2_ref_source_sfp2_pcr,				15);

/* reference standard */
NTV2_CON(ntv2_ref_standard_unknown,				0);
NTV2_CON(ntv2_ref_standard_525,					1);
NTV2_CON(ntv2_ref_standard_625,					2);
NTV2_CON(ntv2_ref_standard_750,					3);
NTV2_CON(ntv2_ref_standard_1125,				4);

/* aspect ratio */
NTV2_CON(ntv2_aspect_ratio_unknown,				0);
NTV2_CON(ntv2_aspect_ratio_nodata,				1);
NTV2_CON(ntv2_aspect_ratio_4x3,					2);
NTV2_CON(ntv2_aspect_ratio_16x9,				3);

/* colorimetry */
NTV2_CON(ntv2_colorimetry_unknown,				0);
NTV2_CON(ntv2_colorimetry_nodata,				1);	
NTV2_CON(ntv2_colorimetry_170m,					2);
NTV2_CON(ntv2_colorimetry_bt709,				3);
NTV2_CON(ntv2_colorimetry_xvycc_601,			4);
NTV2_CON(ntv2_colorimetry_xvycc_709,			5);
NTV2_CON(ntv2_colorimetry_adobe_601,			6);
NTV2_CON(ntv2_colorimetry_adobe_rgb,			7);
NTV2_CON(ntv2_colorimetry_bt2020_cl,			8);
NTV2_CON(ntv2_colorimetry_bt2020,				9);
NTV2_CON(ntv2_colorimetry_dcip3_d65,			10);

/* quantization */
NTV2_CON(ntv2_quantization_unknown,				0);
NTV2_CON(ntv2_quantization_default,				1);
NTV2_CON(ntv2_quantization_limited,				2);
NTV2_CON(ntv2_quantization_full,				3);

/* hdr eotf */
NTV2_CON(ntv2_hdr_eotf_sdr,						0);
NTV2_CON(ntv2_hdr_eotf_hdr,						1);
NTV2_CON(ntv2_hdr_eotf_st2084,					2);
NTV2_CON(ntv2_hdr_eotf_hlg,						3);

/* audio rate */
NTV2_CON(ntv2_audio_rate_48,					0);
NTV2_CON(ntv2_audio_rate_96,					1);
NTV2_CON(ntv2_audio_rate_192,					2);

/* audio format */
NTV2_CON(ntv2_audio_format_lpcm,				0);
NTV2_CON(ntv2_audio_format_dolby,				1);


uint32_t ntv2_reg_read(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index);
void ntv2_reg_write(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index, uint32_t data);
void ntv2_reg_rmw(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index, uint32_t data, uint32_t mask);

uint32_t ntv2_regnum_read(Ntv2SystemContext* context, uint32_t regnum);
void ntv2_regnum_write(Ntv2SystemContext* context, uint32_t regnum, uint32_t data);
void ntv2_regnum_rmw(Ntv2SystemContext* context, uint32_t regnum, uint32_t data, uint32_t mask);
	
uint32_t ntv2_vreg_read(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index);
void ntv2_vreg_write(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index, uint32_t data);
void ntv2_vreg_rmw(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index, uint32_t data, uint32_t mask);

uint32_t ntv2_vregnum_read(Ntv2SystemContext* context, uint32_t regnum);
void ntv2_vregnum_write(Ntv2SystemContext* context, uint32_t regnum, uint32_t data);
void ntv2_vregnum_rmw(Ntv2SystemContext* context, uint32_t regnum, uint32_t data, uint32_t mask);

const char* ntv2_video_standard_name(uint32_t standard);
const char* ntv2_frame_rate_name(uint32_t rate);
const char* ntv2_frame_geometry_name(uint32_t geometry);
const char* ntv2_video_scan_name(uint32_t scan);
const char* ntv2_pixel_format_name(uint32_t format);
const char* ntv2_frame_format_name(uint32_t format);
const char* ntv2_pixel_rate_name(uint32_t rate);
const char* ntv2_reg_sync_name(uint32_t sync);
const char* ntv2_color_space_name(uint32_t color_space);
const char* ntv2_color_depth_name(uint32_t color_depth);
const char* ntv2_ref_source_name(uint32_t ref_source);
const char* ntv2_ref_standard_name(uint32_t ref_source);
const char* ntv2_hdr_eotf_name(uint32_t eotf);
const char* ntv2_audio_rate_name(uint32_t rate);
const char* ntv2_audio_format_name(uint32_t format);

uint32_t ntv2_video_standard_width(uint32_t video_standard);
uint32_t ntv2_video_standard_height(uint32_t video_standard);
uint32_t ntv2_video_standard_lines(uint32_t video_standard);
bool ntv2_video_standard_progressive(uint32_t video_standard);

uint32_t ntv2_frame_rate_duration(uint32_t frame_rate);
uint32_t ntv2_frame_rate_scale(uint32_t frame_rate);

uint32_t ntv2_ref_standard_lines(uint32_t ref_lines);

uint32_t ntv2_diff(uint32_t opa, uint32_t opb);	

#endif
