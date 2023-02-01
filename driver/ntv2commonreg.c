/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2commonreg.c
//
//==========================================================================

#include "ntv2commonreg.h"
#include "ntv2publicinterface.h"

extern uint32_t ntv2ReadRegCon32(Ntv2SystemContext* context, uint32_t regNum);
extern void ntv2WriteRegCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue);
extern bool ntv2WriteRegMSCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue,
								RegisterMask mask, RegisterShift shift);

#define NTV2_MAX_VIDEO_STANDARDS	16
#define NTV2_MAX_FRAME_RATES		16
#define NTV2_MAX_COLOR_SPACES		4
#define NTV2_MAX_COLOR_DEPTHS		4
#define NTV2_MAX_REF_SOURCES		16
#define NTV2_MAX_REF_STANDARDS		8
#define NTV2_MAX_HDR_EOTFS			8
#define NTV2_MAX_AUDIO_RATES		4
#define NTV2_MAX_AUDIO_FORMATS		4

static const char *video_standard_name[NTV2_MAX_VIDEO_STANDARDS] = {
	/* ntv2_video_standard_1080i */ 			"1920x1080i",
	/* ntv2_video_standard_720p */ 				"1280x720p",
	/* ntv2_video_standard_525i */ 				"525i",
	/* ntv2_video_standard_625i */ 				"625i",
	/* ntv2_video_standard_1080p */ 			"1920x1080p",
	/* ntv2_video_standard_2048x1556 */ 		"2048x1556",
	/* ntv2_video_standard_2048x1080p */ 		"2048x1080p",
	/* ntv2_video_standard_2048x1080i */ 		"2048x1080i",
	/* ntv2_video_standard_3840x2160p */ 		"3840x2160p",
	/* ntv2_video_standard_4096x2160p */ 		"4096x2160p",
	/* ntv2_video_standard_3840_hfr */ 			"3840_hfr",
	/* ntv2_video_standard_4096_hfr */ 			"4096_hfr",
	/* ntv2_video_standard_undefined */			"undefined",
	/* ntv2_video_standard_undefined */			"undefined",
	/* ntv2_video_standard_undefined */			"undefined",
	/* ntv2_video_standard_undefined */			"undefined"
};

static const char *frame_rate_name[NTV2_MAX_FRAME_RATES] = {
	/* ntv2_frame_rate_unknown */				"unknown",
	/* ntv2_frame_rate_6000 */					"60",
	/* ntv2_frame_rate_5994 */					"59.94",
	/* ntv2_frame_rate_3000 */					"30",
	/* ntv2_frame_rate_2997 */					"29.97",
	/* ntv2_frame_rate_2500 */					"25",
	/* ntv2_frame_rate_2400 */					"24",
	/* ntv2_frame_rate_2398 */					"23.98",
	/* ntv2_frame_rate_5000 */					"50",
	/* ntv2_frame_rate_4800 */					"48",
	/* ntv2_frame_rate_4795 */					"47.95",
	/* ntv2_frame_rate_12000 */					"120",
	/* ntv2_frame_rate_11988 */					"119.88",
	/* ntv2_frame_rate_1500 */					"15",
	/* ntv2_frame_rate_1400 */					"14",
	/* ntv2_frame_rate_unknown */				"unknown"
};

static const char *color_space_name[NTV2_MAX_COLOR_SPACES] = {
	/* ntv2_con_hdmiin4_colorspace_ycbcr422 */	"YUV 422",
	/* ntv2_con_hdmiin4_colorspace_rgb444 */	"RGB 444",
	/* ntv2_con_hdmiin4_colorspace_ycbcr444 */	"YUV 444",
	/* ntv2_con_hdmiin4_colorspace_ycbcr420 */	"YUV 420"
};

static const char *color_depth_name[NTV2_MAX_COLOR_DEPTHS] = {
	/* ntv2_con_hdmiin4_colordepth_8bit */		" 8",
	/* ntv2_con_hdmiin4_colordepth_10bit */		"10",
	/* ntv2_con_hdmiin4_colordepth_12bit */		"12",
	/*  */										"##"
};

static const char *ref_source_name[NTV2_MAX_REF_SOURCES] = {
	/* ntv2_ref_source_external */				"external",
	/* ntv2_ref_source_input_1 */				"input 1",
	/* ntv2_ref_source_input_2 */				"input 2",
	/* ntv2_ref_source_freerun */				"free run",
	/* ntv2_ref_source_analog */				"analog",
	/* ntv2_ref_source_hdmi */					"hdmi",
	/* ntv2_ref_source_input_3 */				"input 3",
	/* ntv2_ref_source_input_4 */				"input 4",
	/* ntv2_ref_source_input_5 */				"input 5",
	/* ntv2_ref_source_input_6 */				"input 6",
	/* ntv2_ref_source_input_7 */				"input 7",
	/* ntv2_ref_source_input_8 */				"input 8",
	/* ntv2_ref_source_sfp1_ptp */				"sfp1 ptp",
	/* ntv2_ref_source_sfp1_pcr */				"sfp1 pcr",
	/* ntv2_ref_source_sfp2_ptp */				"sfp2 ptp",
	/* ntv2_ref_source_sfp2_pcr */				"sfp2 pcr"
};

static const char *ref_standard_name[NTV2_MAX_REF_STANDARDS] = {
	/* ntv2_ref_standard_unknown */				"unknown",
	/* ntv2_ref_standard_525 */					"525",
	/* ntv2_ref_standard_625 */					"625",
	/* ntv2_ref_standard_750 */					"750",
	/* ntv2_ref_standard_1125 */				"1125",
	/* ntv2_ref_standard_unknown */				"unknown", 
	/* ntv2_ref_standard_unknown */				"unknown", 
	/* ntv2_ref_standard_unknown */				"unknown"
};

static const char *hdr_eotf_name[NTV2_MAX_HDR_EOTFS] = {
	/* ntv2_hdr_eotf_sdr */						"traditional sdr",
	/* ntv2_hdr_eotf_hdr */						"traditional hdr",
	/* ntv2_hdr_eotf_st2084 */					"smpte st2084",
	/* ntv2_hdr_eotf_hlg */						"hybrid log gamma",
	/* unknown */								"unknown",
	/* unknown */								"unknown",
	/* unknown */								"unknown",
	/* unknown */								"unknown"
};

static const char *audio_rate_name[NTV2_MAX_AUDIO_RATES] = {
	/* ntv2_audio_rate_48 */					"48",
	/* ntv2_audio_rate_96 */					"96",
	/* ntv2_audio_rate_192 */					"192",
	/*  */										"##"
};

static const char *audio_format_name[NTV2_MAX_AUDIO_FORMATS] = {
	/* ntv2_audio_format_lpcm */				"lpcm",
	/* ntv2_audio_format_dolby */				"dolby",
	/*  */										"unknown",
	/*  */										"unknown"
};

static const uint32_t video_standard_info[NTV2_MAX_VIDEO_STANDARDS][4] = {
	/* ntv2_video_standard_1080i */				{ 1920, 1080, 0, 1125 },
	/* ntv2_video_standard_720p */				{ 1280,  720, 1,  750 },
	/* ntv2_video_standard_525i */				{ 720,   486, 0,  525 },
	/* ntv2_video_standard_625i */				{ 720,   576, 0,  625 },
	/* ntv2_video_standard_1080p */				{ 1920, 1080, 1, 1125 },
	/* ntv2_video_standard_2048x1556 */			{ 2048, 1556, 0, 1556 },
	/* ntv2_video_standard_2048x1080p */		{ 2048, 1080, 1, 1125 },
	/* ntv2_video_standard_2048x1080i */		{ 2048, 1080, 0, 1125 },
	/* ntv2_video_standard_3840x2160p */		{ 3840, 2160, 1, 2250 },
	/* ntv2_video_standard_4096x2160p */		{ 4096, 2160, 1, 2250 },
	/* ntv2_video_standard_3840_hfr */			{ 3840, 2160, 1, 2250 },
	/* ntv2_video_standard_4096_hfr */			{ 4096, 2160, 1, 2250 },
	/* ntv2_video_standard_undefined */			{    0,    0, 0,    0 },
	/* ntv2_video_standard_undefined */			{    0,    0, 0,    0 },
	/* ntv2_video_standard_undefined */			{    0,    0, 0,    0 },
	/* ntv2_video_standard_undefined */			{    0,    0, 0,    0 }
};

static const uint32_t frame_fraction[NTV2_MAX_FRAME_RATES][2] = {
	/* ntv2_frame_rate_unknown */				{    1,      1 },
	/* ntv2_frame_rate_6000 */					{    1,     60 },
	/* ntv2_frame_rate_5994 */					{ 1001,  60000 },
	/* ntv2_frame_rate_3000 */					{    1,     30 },
	/* ntv2_frame_rate_2997 */					{ 1001,  30000 },
	/* ntv2_frame_rate_2500 */					{    1,     25 },
	/* ntv2_frame_rate_2400 */					{    1,     24 },
	/* ntv2_frame_rate_2398 */					{ 1001,  24000 },
	/* ntv2_frame_rate_5000 */					{    1,     50 },
	/* ntv2_frame_rate_4800 */					{    1,     48 },
	/* ntv2_frame_rate_4795 */					{ 1001,  48000 },
	/* ntv2_frame_rate_12000 */					{    1,    120 },
	/* ntv2_frame_rate_11988 */					{ 1001, 120000 },
	/* ntv2_frame_rate_1500 */					{    1,     15 },
	/* ntv2_frame_rate_1400 */					{    1,     14 },
	/* ntv2_frame_rate_unknown */				{    1,      1 }
};

static const uint32_t ref_standard_info[NTV2_MAX_REF_STANDARDS][1] = {
	/* ntv2_ref_standard_unknown */				{ 0 },
	/* ntv2_ref_standard_525 */					{ 525 },
	/* ntv2_ref_standard_625 */					{ 625 },
	/* ntv2_ref_standard_750 */					{ 750 },
	/* ntv2_ref_standard_1125 */				{ 1125 },
	/* ntv2_ref_standard_unknown */				{ 0 }, 
	/* ntv2_ref_standard_unknown */				{ 0 }, 
	/* ntv2_ref_standard_unknown */				{ 0 }
};


uint32_t ntv2_reg_read(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index)
{
	if ((reg == NULL) ||
		(index >= NTV2_REG_COUNT(reg)))
		return 0;

	return ntv2_regnum_read(context, NTV2_REG_NUM(reg, index));
}

void ntv2_reg_write(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index, uint32_t data)
{
	if ((reg == NULL) ||
		(index >= NTV2_REG_COUNT(reg)))
		return;

	ntv2_regnum_write(context, NTV2_REG_NUM(reg, index), data);
}

void ntv2_reg_rmw(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index, uint32_t data, uint32_t mask)
{
	if ((reg == NULL) ||
		(index >= NTV2_REG_COUNT(reg)))
		return;

	ntv2_regnum_rmw(context, NTV2_REG_NUM(reg, index), data, mask);
}

uint32_t ntv2_regnum_read(Ntv2SystemContext* context, uint32_t regnum)
{
	return ntv2ReadRegCon32(context, regnum);
}

void ntv2_regnum_write(Ntv2SystemContext* context, uint32_t regnum, uint32_t data)
{
	ntv2WriteRegCon32(context, regnum, data);
}

void ntv2_regnum_rmw(Ntv2SystemContext* context, uint32_t regnum, uint32_t data, uint32_t mask)
{
#if defined (AJAMac)
	uint32_t val = ntv2ReadRegCon32(context, regnum);
	val = (val & (~mask)) | (data & mask);
	ntv2WriteRegCon32(context, regnum, val);
#else
	ntv2WriteRegMSCon32(context, regnum, data, mask, 0);
#endif
}

uint32_t ntv2_vreg_read(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index)
{
	if ((reg == NULL) ||
		(index >= NTV2_REG_COUNT(reg)))
		return 0;

	return ntv2_vregnum_read(context, NTV2_REG_NUM(reg, index));
}

void ntv2_vreg_write(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index, uint32_t data)
{
	if ((reg == NULL) ||
		(index >= NTV2_REG_COUNT(reg)))
		return;

	ntv2_vregnum_write(context, NTV2_REG_NUM(reg, index), data);
}

void ntv2_vreg_rmw(Ntv2SystemContext* context, const uint32_t *reg, uint32_t index, uint32_t data, uint32_t mask)
{
	if ((reg == NULL) ||
		(index >= NTV2_REG_COUNT(reg)))
		return;

	ntv2_vregnum_rmw(context, NTV2_REG_NUM(reg, index), data, mask);
}

uint32_t ntv2_vregnum_read(Ntv2SystemContext* context, uint32_t regnum)
{
	return ntv2ReadVirtualRegister(context, regnum);
}

void ntv2_vregnum_write(Ntv2SystemContext* context, uint32_t regnum, uint32_t data)
{
	ntv2WriteVirtualRegister(context, regnum, data);
}

void ntv2_vregnum_rmw(Ntv2SystemContext* context, uint32_t regnum, uint32_t data, uint32_t mask)
{
	uint32_t val = ntv2ReadVirtualRegister(context, regnum);
	val = (val & (~mask)) | (data & mask);
	ntv2WriteVirtualRegister(context, regnum, val);
}

const char* ntv2_video_standard_name(uint32_t standard)
{
	if (standard >= NTV2_MAX_VIDEO_STANDARDS)
		return "*bad video standard*";
	return video_standard_name[standard];
}

const char* ntv2_frame_rate_name(uint32_t rate)
{
	if (rate >= NTV2_MAX_FRAME_RATES)
		return "*bad frame rate*";
	return frame_rate_name[rate];
}

const char* ntv2_color_space_name(uint32_t color_space)
{
	if (color_space >= NTV2_MAX_COLOR_SPACES)
		return "*bad color space*";
	return color_space_name[color_space];
}

const char* ntv2_color_depth_name(uint32_t color_depth)
{
	if (color_depth >= NTV2_MAX_COLOR_DEPTHS)
		return "*bad color depth*";
	return color_depth_name[color_depth];
}

const char* ntv2_ref_source_name(uint32_t ref_source)
{
	if (ref_source >= NTV2_MAX_REF_SOURCES)
		return "*bad ref source*";
	return ref_source_name[ref_source];
}

const char* ntv2_ref_standard_name(uint32_t ref_standard)
{
	if (ref_standard >= NTV2_MAX_REF_STANDARDS)
		return "*bad ref standard*";
	return ref_standard_name[ref_standard];
}

const char* ntv2_hdr_eotf_name(uint32_t eotf)
{
	if (eotf >= NTV2_MAX_HDR_EOTFS)
		return "*bad hdr eotf*";
	return hdr_eotf_name[eotf];
}

const char* ntv2_audio_rate_name(uint32_t rate)
{
	if (rate >= NTV2_MAX_AUDIO_RATES)
		return "*bad audio rate*";
	return audio_rate_name[rate];
}

const char* ntv2_audio_format_name(uint32_t format)
{
	if (format >= NTV2_MAX_AUDIO_FORMATS)
		return "*bad audio format*";
	return audio_format_name[format];
}

uint32_t ntv2_video_standard_width(uint32_t video_standard)
{
	if (video_standard >= NTV2_MAX_VIDEO_STANDARDS)
		return 0;

	return video_standard_info[video_standard][0];
}

uint32_t ntv2_video_standard_height(uint32_t video_standard)
{
	if (video_standard >= NTV2_MAX_VIDEO_STANDARDS)
		return 0;

	return video_standard_info[video_standard][1];
}

bool ntv2_video_standard_progressive(uint32_t video_standard)
{
	if (video_standard >= NTV2_MAX_VIDEO_STANDARDS)
		return 0;

	return (video_standard_info[video_standard][2] == 1);
}

uint32_t ntv2_video_standard_lines(uint32_t video_standard)
{
	if (video_standard >= NTV2_MAX_VIDEO_STANDARDS)
		return 0;

	return video_standard_info[video_standard][3];
}

uint32_t ntv2_frame_rate_duration(uint32_t frame_rate)
{
	if (frame_rate >= NTV2_MAX_FRAME_RATES)
		return 1;

	return frame_fraction[frame_rate][0];
}

uint32_t ntv2_frame_rate_scale(uint32_t frame_rate)
{
	if (frame_rate >= NTV2_MAX_FRAME_RATES)
		return 1;

	return frame_fraction[frame_rate][1];
}

uint32_t ntv2_ref_standard_lines(uint32_t ref_standard)
{
	if (ref_standard >= NTV2_MAX_REF_STANDARDS)
		return 0;

	return ref_standard_info[ref_standard][0];
}

uint32_t ntv2_diff(uint32_t opa, uint32_t opb)
{
	if (opa < opb)
		return opb - opa;

	return opa - opb;
}

