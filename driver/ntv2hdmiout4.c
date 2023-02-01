/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2hdmiout4.c
//
//==========================================================================

#include "ntv2hdmiout4.h"
#include "ntv2hout4reg.h"

/* debug messages */
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002
#define NTV2_DEBUG_HDMIOUT4_STATE		0x00000004
#define NTV2_DEBUG_HDMIOUT4_CONFIG		0x00000008
#define NTV2_DEBUG_HDMIOUT4_EDID		0x00000010
#define NTV2_DEBUG_HDMIOUT4_PARSE		0x00000020
#define NTV2_DEBUG_HDMIOUT4_SCDC		0x00000040
#define NTV2_DEBUG_HDMIOUT4_I2C			0x00000080
#define NTV2_DEBUG_HDMIOUT4_AUX			0x00000100

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	((ntv2_active_mask & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_ERROR(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_STATE(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_HDMIOUT4_STATE, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_CONFIG(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_HDMIOUT4_CONFIG, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_EDID(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_HDMIOUT4_EDID, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_PARSE(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_HDMIOUT4_PARSE, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_SCDC(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_HDMIOUT4_SCDC, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_I2C(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_HDMIOUT4_I2C, string, __VA_ARGS__)
#define NTV2_MSG_HDMIOUT4_AUX(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_HDMIOUT4_AUX, string, __VA_ARGS__)

static uint32_t ntv2_debug_mask = 0xffffffff;
static uint32_t ntv2_user_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;
static uint32_t ntv2_active_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;

#define NTV2_AUX_DATA_SIZE		0x40

enum ntv2_hdmi_clock_type
{
	ntv2_clock_type_unknown,
	ntv2_clock_type_sdd,
	ntv2_clock_type_sdn,
	ntv2_clock_type_hdd,
	ntv2_clock_type_hdn,
	ntv2_clock_type_3gd,
	ntv2_clock_type_3gn,
	ntv2_clock_type_4kd,
	ntv2_clock_type_4kn,
	ntv2_clock_type_h1d,
	ntv2_clock_type_h1n,
	ntv2_clock_type_4hd,
	ntv2_clock_type_4hn,
	ntv2_clock_type_h2d,
	ntv2_clock_type_h2n,
	ntv2_clock_type_size
};


struct ntv2_hdmi_format_data
{
	uint32_t					video_standard;
	uint32_t					frame_rate;
	uint32_t					h_sync_start;
	uint32_t					h_sync_end;
	uint32_t					h_de_start;
	uint32_t					h_total;
	uint32_t					v_trans_f1;
	uint32_t					v_trans_f2;
	uint32_t					v_sync_start_f1;
	uint32_t					v_sync_end_f1;
	uint32_t					v_de_start_f1;
	uint32_t					v_de_start_f2;
	uint32_t					v_sync_start_f2;
	uint32_t					v_sync_end_f2;
	uint32_t					v_total_f1;
	uint32_t					v_total_f2;
	uint8_t						avi_byte4;
	uint8_t						hdmi_byte5;
	enum ntv2_hdmi_clock_type	clock_type;
};

struct ntv2_hdmi_clock_data
{
	enum ntv2_hdmi_clock_type	clock_type;
	uint32_t					color_space;
	uint32_t					color_depth;
	uint32_t					line_rate;
	uint32_t					audio_n;
	uint32_t					audio_cts1;
	uint32_t					audio_cts2;
	uint32_t					audio_cts3;
	uint32_t					audio_cts4;
};


static struct ntv2_hdmi_format_data c_hdmi_format_data[] = {
	{ ntv2_video_standard_525i,	       ntv2_frame_rate_2997,    19,   81,  138,  858,   19,  448,    4,    7,   22,  285,  266,  269,  262,  525,    0,    0, ntv2_clock_type_sdd },
	{ ntv2_video_standard_625i,	       ntv2_frame_rate_2500,	12,   75,  144,  864,   12,  444,    2,    5,   24,  337,  314,  317,  312,  625,    0,    0, ntv2_clock_type_sdn },
	{ ntv2_video_standard_720p,	       ntv2_frame_rate_5000,   440,  480,  700, 1980,  440,    0,    5,   10,   30,    0,    0,    0,  750,    0, 0x13,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_720p,	       ntv2_frame_rate_5994,   110,  150,  370, 1650,  110,    0,    5,   10,   30,    0,    0,    0,  750,    0, 0x04,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_720p,	       ntv2_frame_rate_6000,   110,  150,  370, 1650,  110,    0,    5,   10,   30,    0,    0,    0,  750,    0, 0x04,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080i,       ntv2_frame_rate_2500,   528,  572,  720, 2640,  528, 1848,    2,    7,   22,  585,  564,  569,  562, 1125, 0x14,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080i,       ntv2_frame_rate_2997,    88,  132,  280, 2200,   88, 1188,    2,    7,   22,  585,  564,  569,  562, 1125, 0x05,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_1080i,       ntv2_frame_rate_3000,    88,  132,  280, 2200,   88, 1188,    2,    7,   22,  585,  564,  569,  562, 1125, 0x05,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_2398,   638,  682,  830, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x20,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_2400,   638,  682,  830, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x20,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_2500,   528,  572,  720, 2640,  528,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x21,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_2997,    88,  132,  280, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x22,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_3000,    88,  132,  280, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x22,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_4795,   638,  682,  830, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x6f,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_4800,   638,  682,  830, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x6f,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_5000,   528,  572,  720, 2640,  528,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x1f,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_5994,    88,  132,  280, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x10,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_6000,    88,  132,  280, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0, 0x10,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_2048x1080i,  ntv2_frame_rate_2500,   528,  572,  702, 2750,  528,    0,    2,    7,   22,  585,  564,  569,  562, 1125,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080i,  ntv2_frame_rate_2997,    88,  132,  152, 2200,   88, 1188,    2,    7,   22,  585,  564,  569,  562, 1125,    0,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_2048x1080i,  ntv2_frame_rate_3000,    88,  132,  152, 2200,   88, 1188,    2,    7,   22,  585,  564,  569,  562, 1125,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_2398,   638,  682,  702, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_2400,   638,  682,  702, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_2500,   528,  572,  592, 2640,  528,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_2997,    88,  132,  152, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_3000,    88,  132,  152, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_4795,   638,  682,  702, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_4800,   638,  682,  702, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_5000,   528,  572,  592, 2640,  528,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_5994,    88,  132,  152, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_6000,    88,  132,  152, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    0,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2398,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5d, 0x03, ntv2_clock_type_4kd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2400,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5d, 0x03, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2500,  1056, 1144, 1440, 5280, 1056,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5e, 0x02, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2997,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5f, 0x01, ntv2_clock_type_4kd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_3000,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5f, 0x01, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_4795,   638,  682,  830, 2750,  638,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x72,    0, ntv2_clock_type_h1d },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_4800,   638,  682,  830, 2750,  638,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x72,    0, ntv2_clock_type_h1n },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_5000,   528,  572,  720, 2640,  528,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x60,    0, ntv2_clock_type_h1n },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_5994,    88,  132,  280, 2200,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x61,    0, ntv2_clock_type_h1d },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_6000,    88,  132,  280, 2200,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x61,    0, ntv2_clock_type_h1n },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2398,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5d, 0x03, ntv2_clock_type_4hd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2400,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5d, 0x03, ntv2_clock_type_4hn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2500,  1056, 1144, 1440, 5280, 1056,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5e, 0x02, ntv2_clock_type_4hn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2997,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5f, 0x01, ntv2_clock_type_4hd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_3000,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x5f, 0x01, ntv2_clock_type_4hn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_4795,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x72,    0, ntv2_clock_type_h2d },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_4800,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x72,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_5000,  1056, 1144, 1440, 5280, 1056,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x60,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_5994,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x61,    0, ntv2_clock_type_h2d },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_6000,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x61,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2398,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x62, 0x04, ntv2_clock_type_4kd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2400,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x62, 0x04, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2500,   968, 1056, 1184, 5280,  968,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x63,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2997,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x64,    0, ntv2_clock_type_4kd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_3000,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x64,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_4795,   510,  554,  702, 2750,  510,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x73,    0, ntv2_clock_type_h1d },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_4800,   510,  554,  702, 2750,  510,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x73,    0, ntv2_clock_type_h1n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_5000,   484,  528,  592, 2640,  484,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x65,    0, ntv2_clock_type_h1n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_5994,    44,   88,  152, 2200,   44,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x66,    0, ntv2_clock_type_h1d },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_6000,    44,   88,  152, 2200,   44,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x66,    0, ntv2_clock_type_h1n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2398,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x62, 0x04, ntv2_clock_type_4hd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2400,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x62, 0x04, ntv2_clock_type_4hn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2500,   968, 1056, 1184, 5280,  968,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x63,    0, ntv2_clock_type_4hn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2997,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x64,    0, ntv2_clock_type_4hd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_3000,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x64,    0, ntv2_clock_type_4hn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_4795,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x73,    0, ntv2_clock_type_h2d },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_4800,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x73,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_5000,   968, 1056, 1184, 5280,  968,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x65,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_5994,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x66,    0, ntv2_clock_type_h2d },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_6000,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0, 0x66,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_none,        ntv2_frame_rate_none,     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, ntv2_clock_type_unknown }
};

static struct ntv2_hdmi_clock_data c_hdmi_clock_data[] = {
	{ ntv2_clock_type_sdd,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_270mhz,     6144,    27000,    27000,    27000,    27000 },
	{ ntv2_clock_type_sdd,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_270mhz,     6144,    27000,    27000,    27000,    27000 },
	{ ntv2_clock_type_sdd,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_270mhz,     6144,    27000,    27000,    27000,    27000 },
	{ ntv2_clock_type_sdd,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_270mhz,     6144,    27000,    27000,    27000,    27000 },
	{ ntv2_clock_type_sdd,      ntv2_color_space_rgb444,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_337mhz,     6144,    33750,    33750,    33750,    33750 },
	{ ntv2_clock_type_sdd,      ntv2_color_space_rgb444,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_405mhz,     6144,    40500,    40500,    40500,    40500 },

	{ ntv2_clock_type_sdn,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_270mhz,     6144,    27000,    27000,    27000,    27000 },
	{ ntv2_clock_type_sdn,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_270mhz,     6144,    27000,    27000,    27000,    27000 },
	{ ntv2_clock_type_sdn,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_270mhz,     6144,    27000,    27000,    27000,    27000 },
	{ ntv2_clock_type_sdn,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_270mhz,     6144,    27000,    27000,    27000,    27000 },
	{ ntv2_clock_type_sdn,      ntv2_color_space_rgb444,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_337mhz,     8192,    45045,    45045,    45045,    45045 },
	{ ntv2_clock_type_sdn,      ntv2_color_space_rgb444,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_405mhz,     8192,    54054,    54054,    54054,    54054 },

	{ ntv2_clock_type_hdd,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_742mhz,    11648,   140625,   140625,   140625,   140625 },
	{ ntv2_clock_type_hdd,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_742mhz,    11648,   140625,   140625,   140625,   140625 },
	{ ntv2_clock_type_hdd,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_742mhz,    11648,   140625,   140625,   140625,   140625 },
	{ ntv2_clock_type_hdd,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_742mhz,    11648,   140625,   140625,   140625,   140625 },
	{ ntv2_clock_type_hdd,      ntv2_color_space_rgb444,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_928mhz,    11648,   175781,   175781,   175781,   175782 },
	{ ntv2_clock_type_hdd,      ntv2_color_space_rgb444,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_1113mhz,   11648,   210937,   210938,   210937,   210938 },

	{ ntv2_clock_type_hdn,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_742mhz,     6144,    74250,    74250,    74250,    74250 },
	{ ntv2_clock_type_hdn,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_742mhz,     6144,    74250,    74250,    74250,    74250 },
	{ ntv2_clock_type_hdn,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_742mhz,     6144,    74250,    74250,    74250,    74250 },
	{ ntv2_clock_type_hdn,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_742mhz,     6144,    74250,    74250,    74250,    74250 },
	{ ntv2_clock_type_hdn,      ntv2_color_space_rgb444,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_928mhz,    12288,   185625,   185625,   185625,   185625 },
	{ ntv2_clock_type_hdn,      ntv2_color_space_rgb444,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_1113mhz,    6144,   111375,   111375,   111375,   111375 },

	{ ntv2_clock_type_3gd,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_1485mhz,    5824,   140625,   140625,   140625,   140625 },
	{ ntv2_clock_type_3gd,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_1485mhz,    5824,   140625,   140625,   140625,   140625 },
	{ ntv2_clock_type_3gd,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_1485mhz,    5824,   140625,   140625,   140625,   140625 },
	{ ntv2_clock_type_3gd,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_1485mhz,    5824,   140625,   140625,   140625,   140625 },
	{ ntv2_clock_type_3gd,      ntv2_color_space_rgb444,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_1856mhz,   11648,   351562,   351563,   351562,   351563 },
	{ ntv2_clock_type_3gd,      ntv2_color_space_rgb444,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_2227mhz,   11648,   421875,   421875,   421875,   421875 },

	{ ntv2_clock_type_3gn,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_1485mhz,    6144,   148500,   148500,   148500,   148500 },
	{ ntv2_clock_type_3gn,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_1485mhz,    6144,   148500,   148500,   148500,   148500 },
	{ ntv2_clock_type_3gn,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_1485mhz,    6144,   148500,   148500,   148500,   148500 },
	{ ntv2_clock_type_3gn,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_1485mhz,    6144,   148500,   148500,   148500,   148500 },
	{ ntv2_clock_type_3gn,      ntv2_color_space_rgb444,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_1856mhz,    6144,   185625,   185625,   185625,   185625 },
	{ ntv2_clock_type_3gn,      ntv2_color_space_rgb444,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_2227mhz,    6144,   222750,   222750,   222750,   222750 },

	{ ntv2_clock_type_4kd,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_2970mhz,    5824,   281250,   281250,   281250,   281250 },
	{ ntv2_clock_type_4kd,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_2970mhz,    5824,   281250,   281250,   281250,   281250 },
	{ ntv2_clock_type_4kd,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_2970mhz,    5824,   281250,   281250,   281250,   281250 },
	{ ntv2_clock_type_4kd,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_2970mhz,    5824,   281250,   281250,   281250,   281250 },

	{ ntv2_clock_type_4hd,      ntv2_color_space_rgb444,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_3712mhz,   11648,   703125,   703125,   703125,   703125 },
	{ ntv2_clock_type_4hd,      ntv2_color_space_rgb444,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_4455mhz,    5824,   421875,   421875,   421875,   421875 },

	{ ntv2_clock_type_4kn,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_2970mhz,    5120,   247500,   247500,   247500,   247500 },
	{ ntv2_clock_type_4kn,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_2970mhz,    5120,   247500,   247500,   247500,   247500 },
	{ ntv2_clock_type_4kn,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_2970mhz,    5120,   247500,   247500,   247500,   247500 },
	{ ntv2_clock_type_4kn,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_2970mhz,    5120,   247500,   247500,   247500,   247500 },

	{ ntv2_clock_type_4hn,      ntv2_color_space_rgb444,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_3712mhz,    5120,   309375,   309375,   309375,   309375 },
	{ ntv2_clock_type_4hn,      ntv2_color_space_rgb444,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_4455mhz,    5120,   371250,   371250,   371250,   371250 },

	{ ntv2_clock_type_h1d,      ntv2_color_space_yuv420,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_2970mhz,    5824,   281250,   281250,   281250,   281250 },

	{ ntv2_clock_type_h1n,      ntv2_color_space_yuv420,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_2970mhz,    5120,   247500,   247500,   247500,   247500 },

	{ ntv2_clock_type_h2d,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_5940mhz,    5824,   562500,   562500,   562500,   562500 },
	{ ntv2_clock_type_h2d,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_5940mhz,    5824,   562500,   562500,   562500,   562500 },
	{ ntv2_clock_type_h2d,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_5940mhz,    5824,   562500,   562500,   562500,   562500 },
	{ ntv2_clock_type_h2d,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_5940mhz,    5824,   562500,   562500,   562500,   562500 },

	{ ntv2_clock_type_h2n,      ntv2_color_space_yuv422,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_5940mhz,    6144,   594000,   594000,   594000,   594000 },
	{ ntv2_clock_type_h2n,      ntv2_color_space_yuv422,     ntv2_color_depth_10bit,     ntv2_con_hdmiout4_linerate_5940mhz,    6144,   594000,   594000,   594000,   594000 },
	{ ntv2_clock_type_h2n,      ntv2_color_space_yuv422,     ntv2_color_depth_12bit,     ntv2_con_hdmiout4_linerate_5940mhz,    6144,   594000,   594000,   594000,   594000 },
	{ ntv2_clock_type_h2n,      ntv2_color_space_rgb444,     ntv2_color_depth_8bit,      ntv2_con_hdmiout4_linerate_5940mhz,    6144,   594000,   594000,   594000,   594000 },

	{ ntv2_clock_type_unknown,  0, 0, 0, 0, 0, 0, 0, 0 }
};


static const int64_t c_default_timeout		= 50000;
static const uint32_t c_i2c_timeout			= 200000;
static const uint32_t c_lock_timeout		= 500000;
static const uint32_t c_hdr_timeout			= 2000000;
static const uint32_t c_reset_timeout		= 10000;
static const int64_t c_lock_wait_max		= 8;
static const uint32_t c_vendor_name_size	= 8;
static const uint32_t c_product_name_size	= 16;
static const uint32_t c_config_wait			= 100000;

static const uint32_t c_aux_avi_offset		= 0x40;
static const uint32_t c_aux_vs_offset		= 0x60;
static const uint32_t c_aux_audio_offset	= 0x80;
static const uint32_t c_aux_spd_offset		= 0xa0;
static const uint32_t c_aux_drm_offset		= 0xc0;
static const uint32_t c_aux_user_offset		= 0x200;
static const uint32_t c_aux_user_count		= 8;
static const uint32_t c_aux_frame_size		= 0x20;
static const uint32_t c_aux_buffer_size		= 0x800;
static uint8_t c_aux_data[NTV2_AUX_DATA_SIZE];

bool ntv2_hdmiout4_edid_read(void* context, uint8_t block_num, uint8_t reg_num, uint8_t* reg_val);

static void ntv2_hdmiout4_monitor(void* data);
static Ntv2Status ntv2_hdmiout4_initialize(struct ntv2_hdmiout4 *ntv2_hout);

static bool configure_hardware(struct ntv2_hdmiout4 *ntv2_hout);
static bool configure_hdmi_video(struct ntv2_hdmiout4 *ntv2_hout);
static bool configure_hdmi_aux(struct ntv2_hdmiout4 *ntv2_hout);
static bool configure_hdmi_audio(struct ntv2_hdmiout4 *ntv2_hout);
static bool configure_hdmi_name(struct ntv2_hdmiout4 *ntv2_hout);
static bool monitor_hardware(struct ntv2_hdmiout4 *ntv2_hout);

static void disable_output(struct ntv2_hdmiout4 *ntv2_hout);
static void enable_output(struct ntv2_hdmiout4 *ntv2_hout);
static void config_active(struct ntv2_hdmiout4 *ntv2_hout);
static void config_valid(struct ntv2_hdmiout4 *ntv2_hout);
static bool reset_transmit(struct ntv2_hdmiout4 *ntv2_hout, uint32_t timeout);

static bool check_sink_present(struct ntv2_hdmiout4 *ntv2_hout);
static bool check_force_hpd(struct ntv2_hdmiout4 *ntv2_hout);
static bool is_new_hot_plug_event(struct ntv2_hdmiout4 *ntv2_hout);
static bool is_clock_locked(struct ntv2_hdmiout4 *ntv2_hout);
static bool is_genlocked(struct ntv2_hdmiout4 *ntv2_hout);
static bool has_config_changed(struct ntv2_hdmiout4 *ntv2_hout);
static bool has_hdr_changed(struct ntv2_hdmiout4 *ntv2_hout);

static bool is_active_i2c(struct ntv2_hdmiout4 *ntv2_hout);
static bool wait_for_i2c(struct ntv2_hdmiout4 *ntv2_hout, uint32_t timeout);
static void write_i2c(struct ntv2_hdmiout4 *ntv2_hout, uint32_t device, uint32_t address, uint32_t data);
static uint32_t read_i2c(struct ntv2_hdmiout4 *ntv2_hout, uint32_t device, uint32_t address);

static bool update_edid(struct ntv2_hdmiout4 *ntv2_hout);
static uint32_t read_edid(struct ntv2_hdmiout4 *ntv2_hout, uint32_t address);
static uint32_t read_edid_register(struct ntv2_hdmiout4 *ntv2_hout, uint32_t block_num, uint32_t reg_num);
static void msg_edid_raw(struct ntv2_hdmiout4 *ntv2_hout);
static void msg_edid_format(struct ntv2_hdmiout4 *ntv2_hout);

static void clear_all_aux_data(struct ntv2_hdmiout4 *ntv2_hout);
static void clear_aux_data(struct ntv2_hdmiout4 *ntv2_hout, uint32_t offset);
static void write_aux_data(struct ntv2_hdmiout4 *ntv2_hout,
						   uint32_t offset,
						   uint32_t size,
						   uint8_t* data,
						   bool checksum);

static void update_debug_flags(struct ntv2_hdmiout4 *ntv2_hout);

static struct ntv2_hdmi_format_data* find_format_data(uint32_t video_standard, 
													  uint32_t frame_rate, 
													  int index);
static struct ntv2_hdmi_clock_data* find_clock_data(enum ntv2_hdmi_clock_type	clockType,
													uint32_t	color_space,
													uint32_t	color_depth);


struct ntv2_hdmiout4 *ntv2_hdmiout4_open(Ntv2SystemContext* sys_con,
										 const char *name, int index)
{
	struct ntv2_hdmiout4 *ntv2_hout = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_hout = (struct ntv2_hdmiout4 *)ntv2MemoryAlloc(sizeof(struct ntv2_hdmiout4));
	if (ntv2_hout == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_hdmiout4 instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_hout, 0, sizeof(struct ntv2_hdmiout4));

	ntv2_hout->index = index;
#if defined(MSWindows)
	sprintf(ntv2_hout->name, "%s%d", name, index);
#else
	snprintf(ntv2_hout->name, NTV2_HDMIOUT4_STRING_SIZE, "%s%d", name, index);
#endif
	ntv2_hout->system_context = sys_con;

	ntv2SpinLockOpen(&ntv2_hout->state_lock, sys_con);
	ntv2ThreadOpen(&ntv2_hout->monitor_task, sys_con, "hdmi4 output monitor");
	ntv2EventOpen(&ntv2_hout->monitor_event, sys_con);

	NTV2_MSG_HDMIOUT4_INFO("%s: open ntv2_hdmiout4\n", ntv2_hout->name);

	return ntv2_hout;
}

void ntv2_hdmiout4_close(struct ntv2_hdmiout4 *ntv2_hout)
{
	if (ntv2_hout == NULL) 
		return;

	NTV2_MSG_HDMIOUT4_INFO("%s: close ntv2_hdmiout4\n", ntv2_hout->name);

	ntv2_hdmiout4_disable(ntv2_hout);

	ntv2EventClose(&ntv2_hout->monitor_event);
	ntv2ThreadClose(&ntv2_hout->monitor_task);
	ntv2SpinLockClose(&ntv2_hout->state_lock);

	memset(ntv2_hout, 0, sizeof(struct ntv2_hdmiout4));
	ntv2MemoryFree(ntv2_hout, sizeof(struct ntv2_hdmiout4));
}

Ntv2Status ntv2_hdmiout4_configure(struct ntv2_hdmiout4 *ntv2_hout)
{
	if (ntv2_hout == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_HDMIOUT4_INFO("%s: configure hdmi output device\n", ntv2_hout->name);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiout4_enable(struct ntv2_hdmiout4 *ntv2_hout)
{
	bool success ;

	if (ntv2_hout == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (ntv2_hout->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_HDMIOUT4_STATE("%s: enable hdmi output monitor\n", ntv2_hout->name);

	ntv2EventClear(&ntv2_hout->monitor_event);
	ntv2_hout->monitor_enable = true;

	success = ntv2ThreadRun(&ntv2_hout->monitor_task, ntv2_hdmiout4_monitor, (void*)ntv2_hout);
	if (!success) {
		return NTV2_STATUS_FAIL;
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiout4_disable(struct ntv2_hdmiout4 *ntv2_hout)
{
	if (ntv2_hout == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (!ntv2_hout->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_HDMIOUT4_STATE("%s: disable hdmi output monitor\n", ntv2_hout->name);

	ntv2_hout->monitor_enable = false;
	ntv2EventSignal(&ntv2_hout->monitor_event);

	ntv2ThreadStop(&ntv2_hout->monitor_task);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiout4_write_info_frame(struct ntv2_hdmiout4 *ntv2_hout,
										  uint32_t size,
										  uint8_t *data)
{
	uint32_t offset = 0;
	uint32_t index = 0;
    bool enable = false;

	if (ntv2_hout == NULL) 
		return NTV2_STATUS_BAD_PARAMETER;
	if ((data == NULL) || (size == 0) || (size > c_aux_frame_size))
		return NTV2_STATUS_BAD_PARAMETER;

	index = (*data) & 0x07;
	if (index >= c_aux_user_count)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_HDMIOUT4_AUX("%s: aux data\n"
						  "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"
						  "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
						  ntv2_hout->name,
						  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
						  data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15],
						  data[16], data[17], data[18], data[19], data[20], data[21], data[22], data[23],
						  data[24], data[25], data[26], data[27], data[28], data[29], data[30], data[31]);
						  
	enable = ((*data) & 0x80) != 0;
	offset = c_aux_user_offset + (index * c_aux_frame_size);
	if (enable)
	{
		NTV2_MSG_HDMIOUT4_AUX("%s: write aux data\n", ntv2_hout->name);
		write_aux_data(ntv2_hout, offset, (size - 1), (data + 1), false);
	}
	else
	{
		NTV2_MSG_HDMIOUT4_AUX("%s: clear aux data\n", ntv2_hout->name);
			clear_aux_data(ntv2_hout, offset);
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiout4_clear_info_frames(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t i;

	for (i = 0; i < c_aux_user_count; i++) {
		clear_aux_data(ntv2_hout, c_aux_user_offset + (i * c_aux_frame_size));
	}

	return NTV2_STATUS_SUCCESS;
}

bool ntv2_hdmiout4_edid_read(void* context, uint8_t block_num, uint8_t reg_num, uint8_t* reg_val)
{
	struct ntv2_hdmiout4 *ntv2_hout = (struct ntv2_hdmiout4 *)context;

	if (ntv2_hout == NULL) return false;
	if ((block_num > 1) || (reg_num > 127)) return false;
	if (reg_val == NULL) return false;

	// if no sink return 0
	if (!ntv2_hout->sink_present) {
		*reg_val = 0;
		return true;
	}

	// read edid data
	*reg_val = (uint8_t)read_edid(ntv2_hout, (block_num*128)+reg_num);

	return true;
}

static void ntv2_hdmiout4_monitor(void* data)
{
	struct ntv2_hdmiout4 *ntv2_hout = (struct ntv2_hdmiout4 *)data;
	bool present = false;
	bool locked = false;
	bool hot_plug = false;
	bool sink_present = false;
	bool force_hpd = false;
	bool config_hardware = true;
	bool genlocked = false;
	bool genlocked_last = false;
	uint32_t count;
	uint32_t present_wait = 0;
	uint32_t absent_wait = 0;
	uint32_t lock_retry = 0;
	uint32_t edid_retry = 0;
	uint32_t i;

	if (ntv2_hout == NULL)
		return;

	NTV2_MSG_HDMIOUT4_STATE("%s: hdmi output monitor task start\n", ntv2_hout->name);

	ntv2_hdmiout4_initialize(ntv2_hout);

	while (!ntv2ThreadShouldStop(&ntv2_hout->monitor_task) && ntv2_hout->monitor_enable) 
	{
		update_debug_flags(ntv2_hout);

		sink_present = check_sink_present(ntv2_hout);
		force_hpd = check_force_hpd(ntv2_hout);

		if (sink_present || force_hpd) 
		{
			absent_wait = 0;
			present_wait++;
			if (present_wait > 2) 
			{
				if (!present) 
				{
					NTV2_MSG_HDMIOUT4_CONFIG("%s: sink present\n", ntv2_hout->name);
					present = true;
				}
			} 
			else 
			{
				goto wait;
			}
		} 
		else 
		{
			present_wait = 0;
			absent_wait++;
			if (absent_wait > 1) 
			{
				if (present) 
				{
					NTV2_MSG_HDMIOUT4_CONFIG("%s: sink not present\n", ntv2_hout->name);
					ntv2_hdmiout4_initialize(ntv2_hout);
					present = false;
					locked = false;
					lock_retry = 0;
					edid_retry = 0;
				}
			}
			goto wait;
		}

		// if the clock does not lock reconfig
		if (is_clock_locked(ntv2_hout)) 
		{
			if (!locked) 
			{
				NTV2_MSG_HDMIOUT4_CONFIG("%s: clock locked\n", ntv2_hout->name);
				locked = true;
			}
			enable_output(ntv2_hout);
			lock_retry = 0;
		}
		else 
		{
			if (locked) 
			{
				NTV2_MSG_HDMIOUT4_CONFIG("%s: clock unlocked\n", ntv2_hout->name);
				disable_output(ntv2_hout);
				locked = false;
			}
			if (lock_retry < 10) 
			{
				reset_transmit(ntv2_hout, c_reset_timeout);
				config_hardware = true;
				lock_retry++;
			}
		}

		// check for changes in congifuration registers
		if (has_config_changed(ntv2_hout)) 
		{
			NTV2_MSG_HDMIOUT4_CONFIG("%s: config has changed\n", ntv2_hout->name);
			config_hardware = true;
			lock_retry = 0;
		}
		else
		{
			if (has_hdr_changed(ntv2_hout))
			{
				configure_hdmi_aux(ntv2_hout);
			}
		}

		// check for hot plug
		hot_plug = is_new_hot_plug_event(ntv2_hout);
		if (hot_plug || ((ntv2_hout->edid.video.protocol != ntv2_displayid_protocol_hdmi) && (edid_retry < 2)))
		{
			NTV2_MSG_HDMIOUT4_CONFIG("%s: hot plug detected\n", ntv2_hout->name);
			edid_retry++;

			// clear display id
			ntv2_displayid_clear(&ntv2_hout->edid);

			// update edid block ram
			NTV2_MSG_HDMIOUT4_CONFIG("%s: read edid\n", ntv2_hout->name);
			if (update_edid(ntv2_hout))
			{
				msg_edid_raw(ntv2_hout);

				// parse edid
				if (!ntv2_displayid_update(&ntv2_hout->edid)) 
				{
					NTV2_MSG_HDMIOUT4_CONFIG("%s: parse edid failed\n", ntv2_hout->name);
					ntv2_displayid_clear(&ntv2_hout->edid);
				}
				msg_edid_format(ntv2_hout);
			}
			else
			{
				NTV2_MSG_HDMIOUT4_CONFIG("%s: read edid failed\n", ntv2_hout->name);
				ntv2_displayid_clear(&ntv2_hout->edid);
			}

			lock_retry = 0;
			config_hardware = true;
		}

		// check genlock
		genlocked_last = genlocked;
		genlocked = is_genlocked(ntv2_hout);
		if (!genlocked_last && genlocked)
		{
			ntv2_hout->video_standard = ntv2_video_standard_none;
			config_hardware = true;
		}

		// configure the hardware
		if (config_hardware && is_genlocked(ntv2_hout)) 
		{
			if (configure_hardware(ntv2_hout)) 
			{
				// wait for clock to stabilize
				count = (uint32_t)(c_lock_timeout / c_default_timeout);
				for (i = 0; (i < count) && ntv2_hout->monitor_enable; i++) 
				{
					ntv2EventWaitForSignal(&ntv2_hout->monitor_event, c_default_timeout, true);
					monitor_hardware(ntv2_hout);
				}
			}
			config_hardware = false;
		}
		else 
		{
			if (locked) 
			{
				monitor_hardware(ntv2_hout);
			}
		}

wait:
		// sleep
		if (ntv2_hout->monitor_enable)
			ntv2EventWaitForSignal(&ntv2_hout->monitor_event, c_default_timeout, true);
	}

	NTV2_MSG_HDMIOUT4_STATE("%s: hdmi output monitor task stop\n", ntv2_hout->name);
	ntv2ThreadExit(&ntv2_hout->monitor_task);
	return;
}

static Ntv2Status ntv2_hdmiout4_initialize(struct ntv2_hdmiout4 *ntv2_hout)
{
	ntv2_displayid_config(&ntv2_hout->edid, ntv2_hdmiout4_edid_read, ntv2_hout);
	
	ntv2_hout->hdmi_config = 0;
	ntv2_hout->hdmi_source = 0;
	ntv2_hout->hdmi_control = 0;
	ntv2_hout->hdmi_hdr = 0;

	ntv2_hout->force_config = false;
	ntv2_hout->prefer_420 = false;
	ntv2_hout->hdmi_mode = false;
	ntv2_hout->scdc_mode = false;
	ntv2_hout->sink_present = false;
	ntv2_hout->force_hpd = false;
	ntv2_hout->crop_enable = false;
	ntv2_hout->full_range = false;
	ntv2_hout->sd_wide = false;
	ntv2_hout->video_standard = ntv2_video_standard_none;
	ntv2_hout->frame_rate = ntv2_frame_rate_none;
	ntv2_hout->color_space = ntv2_color_space_yuv422;
	ntv2_hout->color_depth = ntv2_color_depth_8bit;
	ntv2_hout->audio_input = 0;
	ntv2_hout->audio_upper = false;
	ntv2_hout->audio_swap = true;
	ntv2_hout->audio_channels = 0;
	ntv2_hout->audio_select = 0;
	ntv2_hout->audio_rate = ntv2_audio_rate_48;
	ntv2_hout->audio_format = ntv2_audio_format_lpcm;
	ntv2_hout->avi_vic = 0;
	ntv2_hout->hdmi_vic = 0;
	ntv2_hout->scdc_active = false;

	ntv2_hout->scdc_sink_scramble = 0;
	ntv2_hout->scdc_sink_clock = 0;
	ntv2_hout->scdc_sink_valid_ch0 = 0;
	ntv2_hout->scdc_sink_valid_ch1 = 0;
	ntv2_hout->scdc_sink_valid_ch2 = 0;
	ntv2_hout->scdc_sink_error_ch0 = 0;
	ntv2_hout->scdc_sink_error_ch1 = 0;
	ntv2_hout->scdc_sink_error_ch2 = 0;

	ntv2_hout->vendor_name = "AJAVideo";
	ntv2_hout->product_name = "Kona IO";

	ntv2_hout->output_enable = true;
	ntv2_hout->hdr_enable = false;
	ntv2_hout->dolby_vision = false;

	disable_output(ntv2_hout);
	clear_all_aux_data(ntv2_hout);

	return NTV2_STATUS_SUCCESS;
}

static bool configure_hardware(struct ntv2_hdmiout4 *ntv2_hout)
{
	struct ntv2_displayid_video* vid = &ntv2_hout->edid.video;
	struct ntv2_displayid_audio* aud = &ntv2_hout->edid.audio;

	bool hdmi_mode = (vid->protocol == ntv2_displayid_protocol_hdmi) || (!ntv2_hout->sink_present && ntv2_hout->force_hpd);
	bool scdc_mode = vid->scdc_present || (!ntv2_hout->sink_present && ntv2_hout->force_hpd);

	// get configuration hints
	uint32_t video_standard = NTV2_FLD_GET(ntv2_fld_hdmiout_video_standard, ntv2_hout->hdmi_config);
	bool audio_upper = NTV2_FLD_GET(ntv2_fld_hdmiout_audio_group_select, ntv2_hout->hdmi_config) != 0;
	uint32_t frame_rate = NTV2_FLD_GET(ntv2_fld_hdmiout_frame_rate, ntv2_hout->hdmi_config);
	uint32_t deep_color = (NTV2_FLD_GET(ntv2_fld_hdmiout_deep_color, ntv2_hout->hdmi_config) != 0)?
		ntv2_color_depth_10bit : ntv2_color_depth_8bit;
	bool full_range = NTV2_FLD_GET(ntv2_fld_hdmiout_full_range, ntv2_hout->hdmi_config) != 0;
	bool sd_wide = false;
	uint32_t audio_channels = (NTV2_FLD_GET(ntv2_fld_hdmiout_audio_8ch, ntv2_hout->hdmi_config) != 0)? 8 : 2;
	bool dvi_mode = NTV2_FLD_GET(ntv2_fld_hdmiout_dvi, ntv2_hout->hdmi_config) != 0;
	uint32_t vobd = NTV2_FLD_GET(ntv2_fld_hdmiout_vobd, ntv2_hout->hdmi_config);

	// get crosspoint hints
	uint32_t color_space = (NTV2_FLD_GET(ntv2_fld_hdmiout_hdmi_rgb, ntv2_hout->hdmi_source) != 0)?
		ntv2_color_space_rgb444 : ntv2_color_space_yuv422;

	// get control hints
	uint32_t color_depth = (NTV2_FLD_GET(ntv2_fld_hdmiout_deep_12bit, ntv2_hout->hdmi_control) != 0)?
		ntv2_color_depth_12bit : deep_color;
	bool force_config = NTV2_FLD_GET(ntv2_fld_hdmiout_force_config, ntv2_hout->hdmi_control) != 0;
	bool prefer_420 = NTV2_FLD_GET(ntv2_fld_hdmiout_prefer_420, ntv2_hout->hdmi_control) != 0;
	uint32_t audio_format = NTV2_FLD_GET(ntv2_fld_hdmiout_audio_format, ntv2_hout->hdmi_control);
	uint32_t audio_input = NTV2_FLD_GET(ntv2_fld_hdmiout_source_select, ntv2_hout->hdmi_control);
	bool crop_enable = NTV2_FLD_GET(ntv2_fld_hdmiout_crop_enable, ntv2_hout->hdmi_control) != 0;
	bool audio_swap = NTV2_FLD_GET(ntv2_fld_hdmiout_channel34_swap_disable, ntv2_hout->hdmi_control) == 0;
	uint32_t audio_select = NTV2_FLD_GET(ntv2_fld_hdmiout_channel_select, ntv2_hout->hdmi_control);
	uint32_t audio_rate = NTV2_FLD_GET(ntv2_fld_hdmiout_audio_rate, ntv2_hout->hdmi_control);

	bool config_video = false;
	bool config_aux = false;
	bool config_audio = false;
    bool is24 = false;
    bool is25 = false;
    bool is30 = false;
    bool is48 = false;
    bool is50 = false;
    bool is60 = false;
    bool is_444 = false;

	uint32_t max_freq = vid->max_clock_freq;

	// support old audio rate bits move
	if ((vobd == 0x2) && (color_depth != ntv2_color_depth_12bit))
		audio_rate = ntv2_audio_rate_192;

	if (vid->max_tmds_csc > max_freq)
		max_freq = vid->max_tmds_csc;

	// support 420 extended hdmi standards
	if (video_standard == ntv2_video_standard_3840_hfr)
	{
		video_standard = ntv2_video_standard_3840x2160p;
		scdc_mode = false;
		if (force_config)
		{
			color_space = ntv2_color_space_yuv420;
			color_depth = ntv2_color_depth_8bit;
		}
	}

	if (video_standard == ntv2_video_standard_4096_hfr)
	{
		video_standard = ntv2_video_standard_4096x2160p;
		scdc_mode = false;
		if (force_config)
		{
			color_space = ntv2_color_space_yuv420;
			color_depth = ntv2_color_depth_8bit;
		}
	}

    is24 = (frame_rate == ntv2_frame_rate_2398) || (frame_rate == ntv2_frame_rate_2400);
    is25 = (frame_rate == ntv2_frame_rate_2500);
    is30 = (frame_rate == ntv2_frame_rate_2997) || (frame_rate == ntv2_frame_rate_3000);
    is48 = (frame_rate == ntv2_frame_rate_4795) || (frame_rate == ntv2_frame_rate_4800);
    is50 = (frame_rate == ntv2_frame_rate_5000);
    is60 = (frame_rate == ntv2_frame_rate_5994) || (frame_rate == ntv2_frame_rate_6000);
    is_444 = (color_space == ntv2_color_space_rgb444) || (color_space == ntv2_color_space_yuv444);

	if (force_config)
	{
		hdmi_mode = false;
		scdc_mode = false;
	}

	// configure video params
	if ((!force_config && !hdmi_mode) || (force_config && dvi_mode))
	{
		// dvi mode
		hdmi_mode = false;
		scdc_mode = false;
		crop_enable = false;
		full_range = true;
		sd_wide = true;
		color_space = ntv2_color_space_rgb444;
		color_depth = ntv2_color_depth_8bit;
		if (video_standard == ntv2_video_standard_2048x1080i)
		{
			video_standard = ntv2_video_standard_1080i;
			crop_enable = true;
		}
		if (video_standard == ntv2_video_standard_3840x2160p)
		{
			video_standard = ntv2_video_standard_1080p;
		}
		if ((video_standard == ntv2_video_standard_2048x1080p) ||
			(video_standard == ntv2_video_standard_4096x2160p))
		{
			video_standard = ntv2_video_standard_1080p;
			crop_enable = true;
		}
	}
	else if (!force_config)
	{
		// config crop
		if ((video_standard == ntv2_video_standard_2048x1080p) && crop_enable)
			video_standard = ntv2_video_standard_1080p;
		else if ((video_standard == ntv2_video_standard_2048x1080i) && crop_enable)
			video_standard = ntv2_video_standard_1080i;
		else if ((video_standard == ntv2_video_standard_4096x2160p) && crop_enable)
			video_standard = ntv2_video_standard_3840x2160p;
		else
			crop_enable = false;

		// limit 4k
		if (video_standard == ntv2_video_standard_4096x2160p)
		{
			if (scdc_mode)
			{
				// limit clock rate
				if (is_444)
				{
					if (is48 || is50 || is60) 
					{
						color_depth = ntv2_color_depth_8bit;
					}
					else
					{
						if ((color_depth == ntv2_color_depth_12bit) && (max_freq < 445))
							color_depth = ntv2_color_depth_10bit;
						if ((color_depth == ntv2_color_depth_10bit) && (max_freq < 371))
							color_depth = ntv2_color_depth_8bit;
					}
				}
				else
				{
					if (prefer_420 && vid->ycbcr_420 && (is48 || is50 || is60))
					{
						scdc_mode = false;
						color_space = ntv2_color_space_yuv420;
						color_depth = ntv2_color_depth_8bit;
					}
				}
			}
			else
			{
				if (vid->four_24)
				{
					// 1.4b high rate mode
					if ((is48 || is50 || is60) && !is_444 && vid->ycbcr_420)
					{
						color_space = ntv2_color_space_yuv420;
						color_depth = ntv2_color_depth_8bit;
					}
					else 	
					{
						// limit clock rate
						if (is_444)
							color_depth = ntv2_color_depth_8bit;
					}
				}
				else
				{
					// 4k not supported
					video_standard = ntv2_video_standard_2048x1080p;
				}
			}
		}

		// limit uhd
		else if (video_standard == ntv2_video_standard_3840x2160p)
		{
			if (scdc_mode)
			{
				// limit clock rate
				if (is_444)
				{
					if (is50 || is60) 
					{
						color_depth = ntv2_color_depth_8bit;
					}
					else
					{
						if ((color_depth == ntv2_color_depth_12bit) && (max_freq < 445))
							color_depth = ntv2_color_depth_10bit;
						if ((color_depth == ntv2_color_depth_10bit) && (max_freq < 371))
							color_depth = ntv2_color_depth_8bit;
					}
				}
				else
				{
					if (prefer_420 && vid->ycbcr_420 && (is48 || is50 || is60))
					{
						scdc_mode = false;
						color_space = ntv2_color_space_yuv420;
						color_depth = ntv2_color_depth_8bit;
					}
				}
			}
			else
			{
				if (vid->quad_24 || vid->quad_25 || vid->quad_30)
				{
					// 1.4b high rate mode
					if ((is48 || is50 || is60) && !is_444 && vid->ycbcr_420)
					{
						color_space = ntv2_color_space_yuv420;
					}
					else if ((is24 && vid->quad_24) ||
							 (is25 && vid->quad_25) ||
							 (is30 && vid->quad_30))
					{
						// limit clock rate
						if (is_444)
							color_depth = ntv2_color_depth_8bit;
					}
					else
					{
						// uhd not supported
						video_standard = ntv2_video_standard_1080p;
					}
				}
				else
				{
					// uhd not supported
					video_standard = ntv2_video_standard_1080p;
				}
			}
		}
		// limit hd/sd
		else
		{
			if (is_444)
			{
				// limit rgb deep color
				if ((color_depth == ntv2_color_depth_12bit) && !vid->dc_36bit)
					color_depth = ntv2_color_depth_10bit;
				if ((color_depth == ntv2_color_depth_10bit) && !vid->dc_30bit)
					color_depth = ntv2_color_depth_8bit;
			}
		}
	}
	else // force hdmi
	{
		hdmi_mode = true;

		if ((video_standard == ntv2_video_standard_2048x1080p) && crop_enable)
			video_standard = ntv2_video_standard_1080p;
		else if ((video_standard == ntv2_video_standard_2048x1080i) && crop_enable)
			video_standard = ntv2_video_standard_1080i;
		else if ((video_standard == ntv2_video_standard_4096x2160p) && crop_enable)
			video_standard = ntv2_video_standard_3840x2160p;
		else
			crop_enable = false;

		if ((video_standard == ntv2_video_standard_3840x2160p) ||
			(video_standard == ntv2_video_standard_4096x2160p))
		{
			if (is48 || is50 || is60)
			{
				if ((color_space == ntv2_color_space_yuv422) ||
					(color_space == ntv2_color_space_yuv444) ||
					(color_space == ntv2_color_space_rgb444))
				{
					scdc_mode = true;
					if (is_444)
					{
						color_depth = ntv2_color_depth_8bit;
					}
				}
			}
			else
			{
				if (is_444 &&
					((color_depth == ntv2_color_depth_10bit) ||
					 (color_depth == ntv2_color_depth_12bit)))
				{
					scdc_mode = true;
				}
			}
		}
	}

	// configure audio params
	if ((!force_config && !hdmi_mode) || (force_config && dvi_mode))
	{
		// dvi mode
		audio_channels = 0;
	}
	else if (!force_config)
	{
		// limit audio channels
		if (audio_channels > aud->num_lpcm_channels)
			audio_channels = aud->num_lpcm_channels;
		if (aud->basic_audio && (audio_channels < 2))
			audio_channels = 2;
		if (!aud->basic_audio)
			audio_channels = 0;
	}

	// look for changes
	if ((video_standard != ntv2_hout->video_standard) ||
		(frame_rate != ntv2_hout->frame_rate) ||
		(hdmi_mode != ntv2_hout->hdmi_mode) ||
		(scdc_mode != ntv2_hout->scdc_mode) ||
		(crop_enable != ntv2_hout->crop_enable) ||
		(color_space != ntv2_hout->color_space) ||
		(color_depth != ntv2_hout->color_depth) ||
		(audio_channels != ntv2_hout->audio_channels) ||
		(audio_rate != ntv2_hout->audio_rate) ||
		(audio_format != ntv2_hout->audio_format))
	{
		config_video = true;
		config_aux = true;
		config_audio = true;
	}

	if ((full_range != ntv2_hout->full_range) ||
		(sd_wide != ntv2_hout->sd_wide))
	{
		config_aux = true;
	}

	if ((audio_input != ntv2_hout->audio_input) ||
		(audio_upper != ntv2_hout->audio_upper) ||
		(audio_swap != ntv2_hout->audio_swap) ||
		(audio_select != ntv2_hout->audio_select))
	{
		config_audio = true;
	}

	// update state
	ntv2_hout->force_config = force_config;
	ntv2_hout->prefer_420 = prefer_420;
	ntv2_hout->video_standard = video_standard;
	ntv2_hout->frame_rate = frame_rate;
	ntv2_hout->hdmi_mode = hdmi_mode;
	ntv2_hout->scdc_mode = scdc_mode;
	ntv2_hout->crop_enable = crop_enable;
	ntv2_hout->full_range = full_range;
	ntv2_hout->sd_wide = sd_wide;
	ntv2_hout->color_space = color_space; 
	ntv2_hout->color_depth = color_depth;
	ntv2_hout->audio_input = audio_input;
	ntv2_hout->audio_upper = audio_upper;
	ntv2_hout->audio_swap = audio_swap;
	ntv2_hout->audio_channels = audio_channels;
	ntv2_hout->audio_select = audio_select;
	ntv2_hout->audio_rate = audio_rate;
	ntv2_hout->audio_format = audio_format;

	// configure hardware
	if (config_video) 
	{
		disable_output(ntv2_hout);
		config_active(ntv2_hout);
	}

	if (config_video) 
	{
		if (!configure_hdmi_video(ntv2_hout)) {
			NTV2_MSG_HDMIOUT4_ERROR("%s: error configuring hdmi video\n", ntv2_hout->name);
			return false;
		}
		if (!configure_hdmi_name(ntv2_hout)) {
			NTV2_MSG_HDMIOUT4_ERROR("%s: error configuring hdmi name\n", ntv2_hout->name);
			return false;
		}
	}

	if (config_aux) 
	{
		if (!configure_hdmi_aux(ntv2_hout)) {
			NTV2_MSG_HDMIOUT4_ERROR("%s: error configuring hdmi aux\n", ntv2_hout->name);
			return false;
		}
	}

	if (config_audio) 
	{
		if (!configure_hdmi_audio(ntv2_hout)) {
			NTV2_MSG_HDMIOUT4_ERROR("%s: error configuring hdmi audio\n", ntv2_hout->name);
			return false;
		}
	}

	if (config_video) 
	{
		config_valid(ntv2_hout);
	}

	return true;
}

static bool configure_hdmi_video(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	//uint32_t mask;
	uint32_t vid_420;
	uint32_t pix_420;
	uint32_t pix_rep;
	uint32_t rep_fac;
	uint32_t sync_pol;
	uint32_t pix_clock;
	uint32_t lin_int;
	uint32_t pix_int;
	uint32_t scram_mode;
	uint32_t tran_mode;
	uint32_t crop_mode;
	uint32_t aud_mult;

	int i;

	bool hdmi_mode = ntv2_hout->hdmi_mode;
	bool scdc_mode = ntv2_hout->scdc_mode;
	bool crop_enable = ntv2_hout->crop_enable;
	uint32_t video_standard = ntv2_hout->video_standard;
	uint32_t frame_rate = ntv2_hout->frame_rate;
	uint32_t color_space = ntv2_hout->color_space;
	uint32_t color_depth = ntv2_hout->color_depth;
	uint32_t audio_rate = ntv2_hout->audio_rate;

    struct ntv2_hdmi_format_data* format_data = NULL;
    struct ntv2_hdmi_clock_data* clock_data = NULL;
	uint32_t scdc_version;

	NTV2_MSG_HDMIOUT4_CONFIG("%s: config video  mode %s  std %s(%d)  rate %s(%d)  clr %s(%d)  dpth %s(%d)\n", 
							 ntv2_hout->name,
							 (scdc_mode? "scdc" : (hdmi_mode? "hdmi" : "dvi")),
							 ntv2_video_standard_name(video_standard), video_standard,
							 ntv2_frame_rate_name(frame_rate), frame_rate,
							 ntv2_color_space_name(color_space), color_space,
							 ntv2_color_depth_name(color_depth), color_depth);
							 
	// 420 always 8 bit
	if (color_space == ntv2_color_space_yuv420)
	{
		color_depth = ntv2_color_depth_8bit;
	}
	// 422 always 10 bit
	if (color_space == ntv2_color_space_yuv422)
	{
		color_depth = ntv2_color_depth_10bit;
	}

	// clear aux data
	clear_aux_data(ntv2_hout, c_aux_avi_offset);
	clear_aux_data(ntv2_hout, c_aux_vs_offset);

	// reset sink status
	ntv2_hout->scdc_sink_scramble = false;
	ntv2_hout->scdc_sink_clock = 0;
	ntv2_hout->scdc_sink_valid_ch0 = 0;
	ntv2_hout->scdc_sink_valid_ch1 = 0;
	ntv2_hout->scdc_sink_valid_ch2 = 0;
	ntv2_hout->scdc_sink_error_ch0 = 0;
	ntv2_hout->scdc_sink_error_ch1 = 0;
	ntv2_hout->scdc_sink_error_ch2 = 0;

	if (video_standard == ntv2_video_standard_none)  {
		NTV2_MSG_HDMIOUT4_CONFIG("%s: no video standard\n", ntv2_hout->name);
		return false;
	}

	// find format and clock data
	for (i = 0; i < 10; i++)
	{
		// get hdmi format info
		format_data = find_format_data(video_standard, frame_rate, i);
		if (format_data == NULL) {
			NTV2_MSG_HDMIOUT4_CONFIG("%s: unsupported video format\n", ntv2_hout->name);
			return false;
		}

		// get hdmi clock info
		clock_data = find_clock_data(format_data->clock_type, color_space, color_depth);
		if (clock_data != NULL)
		{
			break;
		}
	}
	if (clock_data == NULL)	{
		NTV2_MSG_HDMIOUT4_CONFIG("%s: unsupported video clock\n", ntv2_hout->name);
		return false;
	}

	// setup color space conversion???

	// setup crop
	crop_mode = crop_enable? ntv2_con_hdmiout4_cropmode_enable : ntv2_con_hdmiout4_cropmode_disable;
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_croplocation_start, 0x040);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_croplocation_end, 0x7bf);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_croplocation, ntv2_hout->index, value);

	// setup sampling conversion
	vid_420 = ntv2_con_hdmiout4_420mode_disable;
	pix_420 = ntv2_con_hdmiout4_420convert_disable;
	if (color_space == ntv2_color_space_yuv420) 
	{
		vid_420 = ntv2_con_hdmiout4_420mode_enable;
		pix_420 = ntv2_con_hdmiout4_420convert_enable;
	}

	// configure transmitter
	pix_rep = ntv2_con_hdmiout4_pixelreplicate_disable;
	rep_fac = 0;
	sync_pol = ntv2_con_hdmiout4_syncpolarity_activehigh;
	if ((clock_data->clock_type == ntv2_clock_type_sdd) ||
		(clock_data->clock_type == ntv2_clock_type_sdn)) 
	{
		pix_rep = ntv2_con_hdmiout4_pixelreplicate_enable;
		rep_fac = 10;
		sync_pol = ntv2_con_hdmiout4_syncpolarity_activelow;
	}

	pix_clock = 1;
	lin_int = ntv2_con_hdmiout4_lineinterleave_disable;
	pix_int = ntv2_con_hdmiout4_pixelinterleave_disable;
	if ((clock_data->clock_type == ntv2_clock_type_4kd) ||
		(clock_data->clock_type == ntv2_clock_type_4kn) ||
		(clock_data->clock_type == ntv2_clock_type_h1d) ||
		(clock_data->clock_type == ntv2_clock_type_h1n) ||
		(clock_data->clock_type == ntv2_clock_type_4hd) ||
		(clock_data->clock_type == ntv2_clock_type_4hn) ||
		(clock_data->clock_type == ntv2_clock_type_h2d) ||
		(clock_data->clock_type == ntv2_clock_type_h2n)) 
	{
		pix_clock = 4;
		lin_int = ntv2_con_hdmiout4_lineinterleave_enable;
		pix_int = ntv2_con_hdmiout4_pixelinterleave_enable;
	}

	scram_mode = ntv2_con_hdmiout4_scramblemode_disable;
	tran_mode = ntv2_con_hdmiout4_tranceivermode_disable;
	if ((clock_data->clock_type == ntv2_clock_type_4hd) ||
		(clock_data->clock_type == ntv2_clock_type_4hn) ||
		(clock_data->clock_type == ntv2_clock_type_h2d) ||
		(clock_data->clock_type == ntv2_clock_type_h2n)) 
	{
		scram_mode = ntv2_con_hdmiout4_scramblemode_enable;
		tran_mode = ntv2_con_hdmiout4_tranceivermode_enable;
		if (scdc_mode) 
		{
			value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_tmdsconfig);
			if (value != 0x03)
			{
				scdc_version = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_sinkversion);
				NTV2_MSG_HDMIOUT4_SCDC("%s: scdc sink version 0x%02x enable\n", ntv2_hout->name, scdc_version);
				ntv2_hout->scdc_active = true;

				// update sink tmds multiplier
				write_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_tmdsconfig, 0x03);
				value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_tmdsconfig);
				if (value != 0x03)
				{
					NTV2_MSG_HDMIOUT4_ERROR("%s: scdc write verify failed device 0x54 address 0x20 got 0x%02x expected 0x03\n",
											ntv2_hout->name, value);
				}

				write_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_scdcconfig, 0x00);
				value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_scdcconfig);
				if (value != 0x00)
				{
					NTV2_MSG_HDMIOUT4_ERROR("%s: scdc write verify failed device 0x54 address 0x30 got 0x%02x expected 0x00\n",
											ntv2_hout->name, value);
				}
			}
		}
	}
	else
	{
		if (ntv2_hout->scdc_active)
		{
			value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_tmdsconfig);
			if (value != 0x00)
			{
				scdc_version = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_sinkversion);
				NTV2_MSG_HDMIOUT4_SCDC("%s: scdc sink version 0x%02x disable\n", ntv2_hout->name, scdc_version);
				ntv2_hout->scdc_active = false;

				// update sink tmds multiplier
				write_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_tmdsconfig, 0x00);
				value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_tmdsconfig);
				if (value != 0x00)
				{
					NTV2_MSG_HDMIOUT4_ERROR("%s: scdc write verify failed device 0x54 address 0x20 got 0x%02x expected 0x00\n",
											ntv2_hout->name, value);
				}

				write_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_scdcconfig, 0x00);
				value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_scdcconfig);
				if (value != 0x00)
				{
					NTV2_MSG_HDMIOUT4_ERROR("%s: scdc write verify failed device 0x54 address 0x30 got 0x%02x expected 0x00\n",
											ntv2_hout->name, value);
				}
			}
		}
	}

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_scrambleMode, scram_mode);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_tranceivermode, tran_mode);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_420mode, vid_420);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_pixelsperclock, pix_clock);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_pixelreplicate, pix_rep);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_replicatefactor, rep_fac);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_linerate, clock_data->line_rate);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_audiomode, ntv2_con_hdmiout4_audiomode_disable);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_txconfigmode, ntv2_con_hdmiout4_txconfigmode_active);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_pixelcontrol_lineinterleave, lin_int);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_pixelcontrol_pixelinterleave, pix_int);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_pixelcontrol_420convert, pix_420);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_pixelcontrol_cropmode, crop_mode);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_pixelcontrol, ntv2_hout->index, value);

	// configure video
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup0_colorspace, color_space);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup0_colordepth, color_depth);

	if (ntv2_video_standard_progressive(video_standard))
	{
		value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup0_scanmode, ntv2_con_hdmiout4_scanmode_progressive);
	}
	else
	{
		value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup0_scanmode, ntv2_con_hdmiout4_scanmode_interlaced);
	}

	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup0_interfacemode,
						  hdmi_mode? ntv2_con_hdmiout4_interfacemode_hdmi : ntv2_con_hdmiout4_interfacemode_dvi);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup0_syncpolarity, sync_pol);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videosetup0, ntv2_hout->index, value);

	// configure raster
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup1_hsyncstart, format_data->h_sync_start);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup1_hsyncend, format_data->h_sync_end);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videosetup1, ntv2_hout->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup2_hdestart, format_data->h_de_start);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup2_htotal, format_data->h_total);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videosetup2, ntv2_hout->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup3_vtransf1, format_data->v_trans_f1);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup3_vtransf2, format_data->v_trans_f2);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videosetup3, ntv2_hout->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup4_vsyncstartf1, format_data->v_sync_start_f1);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup4_vsyncendf1, format_data->v_sync_end_f1);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videosetup4, ntv2_hout->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup5_vdestartf1, format_data->v_de_start_f1);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup5_vdestartf2, format_data->v_de_start_f2);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videosetup5, ntv2_hout->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup6_vsyncstartf2, format_data->v_sync_start_f2);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup6_vsyncendf2, format_data->v_sync_end_f2);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videosetup6, ntv2_hout->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup7_vtotalf1, format_data->v_total_f1);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_videosetup7_vtotalf2, format_data->v_total_f2);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_videosetup7, ntv2_hout->index, value);

	// configure audio clock
	aud_mult = 1;
	if (audio_rate == ntv2_audio_rate_96)
		aud_mult = 2;
	if (audio_rate == ntv2_audio_rate_192)
		aud_mult = 4;

	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_audio_cts1, ntv2_hout->index, clock_data->audio_cts1);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_audio_cts2, ntv2_hout->index, clock_data->audio_cts2);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_audio_cts3, ntv2_hout->index, clock_data->audio_cts3);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_audio_cts4, ntv2_hout->index, clock_data->audio_cts4);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_audio_n, ntv2_hout->index, clock_data->audio_n * aud_mult);

	// set the vic
	ntv2_hout->avi_vic = 0;
	ntv2_hout->hdmi_vic = 0;
	if (video_standard == ntv2_video_standard_525i)
	{
		ntv2_hout->avi_vic = 0x06;
	}
	else if (video_standard == ntv2_video_standard_625i)
	{
		ntv2_hout->avi_vic = 0x15;
	}
	else
	{
		ntv2_hout->avi_vic = format_data->avi_byte4;
		ntv2_hout->hdmi_vic = format_data->hdmi_byte5;
	}

	return true;
}

static bool configure_hdmi_aux(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t index;
	uint8_t avi_byte1 = 0x00;
	uint8_t avi_byte2 = 0x00;
	uint8_t avi_byte3 = 0x00;
	uint8_t avi_byte4 = 0x00;
	uint8_t avi_byte5 = 0x00;
	uint8_t hdmi_byte5 = 0x00;
	bool is_sd = false;
	bool is_rgb = false;
	int i;

	uint32_t video_standard = ntv2_hout->video_standard;
	uint32_t color_space = ntv2_hout->color_space;
	bool full_range = ntv2_hout->full_range;
	bool sd_wide = ntv2_hout->sd_wide;
	bool hdr_enable = NTV2_FLD_GET(ntv2_fld_hdr_enable, ntv2_hout->hdr_control);
	bool dolby_vision = NTV2_FLD_GET(ntv2_fld_hdr_dolby_vision_enable, ntv2_hout->hdr_control);
	bool constant_luma = NTV2_FLD_GET(ntv2_fld_hdr_constant_luminance, ntv2_hout->hdr_control);
	bool dci_colorimetry = NTV2_FLD_GET(ntv2_fld_hdr_dci_colorimetry, ntv2_hout->hdr_control);

	// force for dolby vision
	if (dolby_vision)
	{
		color_space = ntv2_color_space_rgb444;
		full_range = true;
		hdr_enable = false;
	}

	// configure vics
	avi_byte4 = ntv2_hout->avi_vic;
	if (sd_wide && (avi_byte4 == 0x06))
	{
		avi_byte4 = 0x07;
	}
	if (sd_wide && (avi_byte4 == 0x15))
	{
		avi_byte4 = 0x16;
	}
	hdmi_byte5 = ntv2_hout->hdmi_vic;

	// color space
	switch (color_space)
	{
	case ntv2_color_space_yuv422:
		avi_byte1 = 0x20;
		is_rgb = false;
		break;
	case ntv2_color_space_yuv444:
		avi_byte1 = 0x40;
		is_rgb = false;
		break;
	case ntv2_color_space_yuv420:
		avi_byte1 = 0x60;
		is_rgb = false;
		break;
	default:
		avi_byte1 = 0x00;
		is_rgb = true;
		break;
	}

	// aspect and repetition
	avi_byte2 = 0x28;
	if ((video_standard == ntv2_video_standard_525i) ||
		(video_standard == ntv2_video_standard_625i))
	{
		avi_byte5 = 0x01;
		if (!sd_wide)
		{
			avi_byte2 = 0x18;
		}
		is_sd = true;
	}

	// colorimetry
	if (is_rgb)
	{
		avi_byte3 = 0x04;			// smpte range
		if (full_range)
		{
			avi_byte3 = 0x08;		// full range
		}
		if (hdr_enable)
		{
			if (dci_colorimetry)
			{
				avi_byte2 |= 0xc0;	// extended
				avi_byte3 |= 0x70;	// dci
			}
			else
			{
				avi_byte2 |= 0xc0;	// extended
				avi_byte3 |= 0x60;	// 2020
			}
		}
	}
	else
	{
		if (hdr_enable)
		{
			if (constant_luma)
			{
				avi_byte2 |= 0xc0;	// extended
				avi_byte3 = 0x50;	// 2020 CL
			}
			else
			{
				avi_byte2 |= 0xc0;	// extended
				avi_byte3 = 0x60;	// 2020
			}
		}
		else if (is_sd)
		{
			avi_byte2 |= 0x40;		// 601
			avi_byte3 = 0x00;
		}
		else
		{
			avi_byte2 |= 0x80;		// 709
			avi_byte3 = 0x00;
		}
	}

	// avi info
	index = 0;
	c_aux_data[index++] = 0x82; // header
	c_aux_data[index++] = 0x02;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = 0x00; // checksum

	c_aux_data[index++] = 0x00 + avi_byte1;
	c_aux_data[index++] = 0x00 + avi_byte2;
	c_aux_data[index++] = 0x00 + avi_byte3;
	c_aux_data[index++] = 0x00 + avi_byte4; // CEA_VIC
	c_aux_data[index++] = 0x00 + avi_byte5;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = 0x00;

	write_aux_data(ntv2_hout, c_aux_avi_offset, index, c_aux_data, true);

	// vs info
	index = 0;
	c_aux_data[index++] = 0x81; // header
	c_aux_data[index++] = 0x01;
	c_aux_data[index++] = 0x00;	// length
	c_aux_data[index++] = 0x00; // checksum

	c_aux_data[index++] = 0x03;
	c_aux_data[index++] = 0x0c;
	c_aux_data[index++] = 0x00;
	c_aux_data[index++] = (hdmi_byte5 != 0) ? 0x20 : 0x00;
	c_aux_data[index++] = hdmi_byte5; // HDMI_VIC
	c_aux_data[index++] = 0x00;

	if (dolby_vision)
	{
		if (!ntv2_hout->dolby_vision)
		{
			NTV2_MSG_HDMIOUT4_CONFIG("%s: dolby vision enable\n", ntv2_hout->name);
			ntv2_hout->dolby_vision = true;
		}

		for (i = 0; i < 18; i++)
		{
			c_aux_data[index++] = 0x00;
		}
	}
	else
	{
		if (ntv2_hout->dolby_vision)
		{
			NTV2_MSG_HDMIOUT4_CONFIG("%s: dolby vision disable\n", ntv2_hout->name);
			ntv2_hout->dolby_vision = false;
		}
	}

	write_aux_data(ntv2_hout, c_aux_vs_offset, index, c_aux_data, true);

	// dynamic mastering info
	if (hdr_enable || ntv2_hout->hdr_enable)
	{
		uint32_t eotf = NTV2_FLD_GET(ntv2_fld_hdr_transfer_function, ntv2_hout->hdr_control);
		uint32_t green_x = NTV2_FLD_GET(ntv2_fld_hdr_primary_x, ntv2_hout->hdr_green_primary);
		uint32_t green_y = NTV2_FLD_GET(ntv2_fld_hdr_primary_y, ntv2_hout->hdr_green_primary);
		uint32_t blue_x = NTV2_FLD_GET(ntv2_fld_hdr_primary_x, ntv2_hout->hdr_blue_primary);
		uint32_t blue_y = NTV2_FLD_GET(ntv2_fld_hdr_primary_y, ntv2_hout->hdr_blue_primary);
		uint32_t red_x = NTV2_FLD_GET(ntv2_fld_hdr_primary_x, ntv2_hout->hdr_red_primary);
		uint32_t red_y = NTV2_FLD_GET(ntv2_fld_hdr_primary_y, ntv2_hout->hdr_red_primary);
		uint32_t white_x = NTV2_FLD_GET(ntv2_fld_hdr_white_point_x, ntv2_hout->hdr_white_point);
		uint32_t white_y = NTV2_FLD_GET(ntv2_fld_hdr_white_point_y, ntv2_hout->hdr_white_point);
		uint32_t luma_max = NTV2_FLD_GET(ntv2_fld_hdr_luminance_max, ntv2_hout->hdr_master_luminance);
		uint32_t luma_min = NTV2_FLD_GET(ntv2_fld_hdr_luminance_min, ntv2_hout->hdr_master_luminance);
		uint32_t content_max = NTV2_FLD_GET(ntv2_fld_hdr_content_light_max, ntv2_hout->hdr_light_level);
		uint32_t frame_max = NTV2_FLD_GET(ntv2_fld_hdr_frame_average_max, ntv2_hout->hdr_light_level);

		if (!ntv2_hout->hdr_enable)
		{
			NTV2_MSG_HDMIOUT4_CONFIG("%s: hdr enable  eotf %s\n", ntv2_hout->name, ntv2_hdr_eotf_name(eotf));
			ntv2_hout->hdr_enable = true;
		}

		index = 0;
		c_aux_data[index++] = 0x87; // header
		c_aux_data[index++] = 0x01;
		c_aux_data[index++] = 0x00;	// length
		c_aux_data[index++] = 0x00; // checksum

		c_aux_data[index++] = hdr_enable? (eotf & 0x7) : 0;
		c_aux_data[index++] = 0x00;
		c_aux_data[index++] = (uint8_t)(green_x & 0xff);
		c_aux_data[index++] = (uint8_t)((green_x >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(green_y & 0xff);
		c_aux_data[index++] = (uint8_t)((green_y >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(blue_x & 0xff);
		c_aux_data[index++] = (uint8_t)((blue_x >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(blue_y & 0xff);
		c_aux_data[index++] = (uint8_t)((blue_y >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(red_x & 0xff);
		c_aux_data[index++] = (uint8_t)((red_x >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(red_y & 0xff);
		c_aux_data[index++] = (uint8_t)((red_y >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(white_x & 0xff);
		c_aux_data[index++] = (uint8_t)((white_x >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(white_y & 0xff);
		c_aux_data[index++] = (uint8_t)((white_y >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(luma_max & 0xff);
		c_aux_data[index++] = (uint8_t)((luma_max >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(luma_min & 0xff);
		c_aux_data[index++] = (uint8_t)((luma_min >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(content_max & 0xff);
		c_aux_data[index++] = (uint8_t)((content_max >> 8) & 0xff);
		c_aux_data[index++] = (uint8_t)(frame_max & 0xff);
		c_aux_data[index++] = (uint8_t)((frame_max >> 8) & 0xff);

		write_aux_data(ntv2_hout, c_aux_drm_offset, index, c_aux_data, true);

		if (!hdr_enable && ntv2_hout->hdr_enable)
		{
			NTV2_MSG_HDMIOUT4_CONFIG("%s: hdr disable start\n", ntv2_hout->name);

			// wait 2 seconds for montior to disable
			ntv2EventWaitForSignal(&ntv2_hout->monitor_event, c_hdr_timeout, true);

			clear_aux_data(ntv2_hout, c_aux_drm_offset);

			NTV2_MSG_HDMIOUT4_CONFIG("%s: hdr disable done\n", ntv2_hout->name);
			ntv2_hout->hdr_enable = false;
		}
	}
	else
	{
		clear_aux_data(ntv2_hout, c_aux_drm_offset);
	}

	return true;
}

static bool configure_hdmi_audio(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t mask;
	uint32_t index;
	uint8_t aud_byte1 = 0;
	uint8_t aud_byte4 = 0;

	uint32_t audio_input = ntv2_hout->audio_input;
	bool audio_upper = ntv2_hout->audio_upper;
	bool audio_swap = ntv2_hout->audio_swap;
	uint32_t audio_channels = ntv2_hout->audio_channels;
	uint32_t audio_select = ntv2_hout->audio_select;
	uint32_t audio_rate = ntv2_hout->audio_rate;
	uint32_t audio_format = ntv2_hout->audio_format;

	NTV2_MSG_HDMIOUT4_CONFIG("%s: config audio  format %s  rate %s  source %d  group %s  channels %d  swap %s  select %d\n", ntv2_hout->name,
							 ntv2_audio_format_name(audio_format),
							 ntv2_audio_rate_name(audio_rate),
							 audio_input,
							 audio_upper? "upper" : "lower",
							 audio_channels,
							 audio_swap? "enable" : "disable",
							 audio_select);

	// clear aux data
	clear_aux_data(ntv2_hout, c_aux_audio_offset);

	// configure audio source
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_audiocontrol_source, audio_input);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_audiocontrol_source);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_audiocontrol_group_select, audio_upper?
					  ntv2_con_hdmiout4_group_select_upper : ntv2_con_hdmiout4_group_select_lower);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiout4_audiocontrol_group_select);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_audiocontrol_audioswapmode, audio_swap?
					  ntv2_con_hdmiout4_audioswapmode_enable : ntv2_con_hdmiout4_audioswapmode_disable);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiout4_audiocontrol_audioswapmode);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_audiocontrol_channel_select, audio_select);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiout4_audiocontrol_channel_select);

	// configure audio output channels
	if (audio_channels == 8)
	{
		value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_audiocontrol_num_channels, ntv2_con_hdmiout4_num_channels_8);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiout4_audiocontrol_num_channels);
		if (audio_format == ntv2_audio_format_lpcm)
		{
			aud_byte1 = 0x07;
			aud_byte4 = 0x13;
		}
	}
	else
	{
		value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_audiocontrol_num_channels, ntv2_con_hdmiout4_num_channels_2);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiout4_audiocontrol_num_channels);
		if (audio_format == ntv2_audio_format_lpcm)
		{
			aud_byte1 = 0x01;
			aud_byte4 = 0x00;
		}
	}

	// configure audio rate
	value |=  NTV2_FLD_SET(ntv2_fld_hdmiout4_audiocontrol_audio_rate, audio_rate);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiout4_audiocontrol_audio_rate);

	// configure audio format
	value |=  NTV2_FLD_SET(ntv2_fld_hdmiout4_audiocontrol_audio_format, audio_format);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiout4_audiocontrol_audio_format);

	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_audiocontrol, ntv2_hout->index, value, mask);

	// configure audio info
	if (audio_channels != 0)
	{
		// enable audio
		value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_audiomode, ntv2_con_hdmiout4_audiomode_enable);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_videocontrol_audiomode);

		// audio info frame
		index = 0;
		c_aux_data[index++] = 0x84; // header
		c_aux_data[index++] = 0x01;
		c_aux_data[index++] = 0x00;	// length
		c_aux_data[index++] = 0x00; // checksum

		c_aux_data[index++] = 0x00 + aud_byte1;
		c_aux_data[index++] = 0x00;
		c_aux_data[index++] = 0x00;
		c_aux_data[index++] = 0x00 + aud_byte4;
		c_aux_data[index++] = 0x00;
		c_aux_data[index++] = 0x00;
		c_aux_data[index++] = 0x00;
		c_aux_data[index++] = 0x00;
		c_aux_data[index++] = 0x00;
		c_aux_data[index++] = 0x00;

		write_aux_data(ntv2_hout, c_aux_audio_offset, index, c_aux_data, true);
	}
	else
	{
		// disable audio
		value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_audiomode, ntv2_con_hdmiout4_audiomode_disable);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_videocontrol_audiomode);
	}

	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index, value, mask);

	return true;
}

static bool configure_hdmi_name(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t index = 0;
	uint32_t i;
	char* name;

	clear_aux_data(ntv2_hout, c_aux_spd_offset);

	if ((ntv2_hout->vendor_name == NULL) || (ntv2_hout->product_name == NULL)) return false;

	index = 0;
	c_aux_data[index++] = 0x83; // header
	c_aux_data[index++] = 0x01;
	c_aux_data[index++] = 0x00;	// length
	c_aux_data[index++] = 0x00; // checksum

	name = ntv2_hout->vendor_name;
	for(i = 0; i < c_vendor_name_size; i++)
	{
		if (*name != '\0') {
			c_aux_data[index++] = (uint8_t)*name;
			name++;
		}
		else {
			c_aux_data[index++] = ' ';
		}
	}

	name = ntv2_hout->product_name;
	for(i = 0; i < c_product_name_size; i++)
	{
		if (*name != '\0') {
			c_aux_data[index++] = (uint8_t)*name;
			name++;
		}
		else {
			c_aux_data[index++] = ' ';
		}
	}

	c_aux_data[index++] = 0x00; // Unknown source

	write_aux_data(ntv2_hout, c_aux_spd_offset, index, c_aux_data, true);

	return true;
}

static bool monitor_hardware(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t low;
	uint32_t high;
	uint32_t err0;
	uint32_t err1;
	uint32_t err2;
	uint32_t checksum;
	bool scramble;
	bool clock;
	bool ch0;
	bool ch1;
	bool ch2;

	if (ntv2_hout->scdc_active) {

		// reset update flags
		value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_updateflags0);
		write_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_updateflags0, value);
		value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_updateflags1);
		write_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_updateflags1, value);

		// scrambler status
		value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_scamblerstatus);
		if (value != ntv2_hout->scdc_sink_scramble) {
			scramble = (NTV2_FLD_GET(ntv2_fld_hdmiout4_scamblerstatus_scrambledetect, value) != 0);
			NTV2_MSG_HDMIOUT4_SCDC("%s: scdc sink scramble detect: %s\n", ntv2_hout->name, scramble ? "yes" : "no");
			ntv2_hout->scdc_sink_scramble = value;
		}

		// clock status
		value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_scdcstatus0);
		if (value != ntv2_hout->scdc_sink_clock) {
			clock = (NTV2_FLD_GET(ntv2_fld_hdmiout4_scdcstatus0_clockdetect, value) != 0);
			ch0 = (NTV2_FLD_GET(ntv2_fld_hdmiout4_scdcstatus0_ch0lock, value) != 0);
			ch1 = (NTV2_FLD_GET(ntv2_fld_hdmiout4_scdcstatus0_ch1lock, value) != 0);
			ch2 = (NTV2_FLD_GET(ntv2_fld_hdmiout4_scdcstatus0_ch2lock, value) != 0);
			NTV2_MSG_HDMIOUT4_SCDC("%s: scdc sink clock detect: %s   ch0 lock: %s   ch1 lock: %s   ch2 lock: %s\n",
								   ntv2_hout->name, clock ? "yes" : "no ",
								   ch0 ? "yes" : "no ", ch1 ? "yes" : "no ", ch2 ? "yes" : "no ");
			ntv2_hout->scdc_sink_clock = value;
		}
		value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_scdcstatus1);

		// channel status
		checksum = 0;
		low = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_ch0errorlow);
		high = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_ch0errorhigh);
		err0 = low | (high << 8);
		ch0 = (NTV2_FLD_GET(ntv2_fld_hdmiout4_ch0errorhigh_valid, high) != 0);
		checksum += (uint8_t)low;
		checksum += (uint8_t)high;

		low = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_ch1errorlow);
		high = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_ch1errorhigh);
		err1 = low | (high << 8);
		ch1 = (NTV2_FLD_GET(ntv2_fld_hdmiout4_ch1errorhigh_valid, high) != 0);
		checksum += (uint8_t)low;
		checksum += (uint8_t)high;

		low = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_ch2errorlow);
		high = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_ch2errorhigh);
		err2 = low | (high << 8);
		ch2 = (NTV2_FLD_GET(ntv2_fld_hdmiout4_ch2errorhigh_valid, high) != 0);
		checksum += (uint8_t)low;
		checksum += (uint8_t)high;

		value = read_i2c(ntv2_hout, ntv2_dev_hdmiout4_sink, ntv2_reg_hdmiout4_errorchecksum);
		checksum += (uint8_t)value;

		checksum &= 0xff;

		if ((ch0 != ntv2_hout->scdc_sink_valid_ch0) || 
			(ch1 != ntv2_hout->scdc_sink_valid_ch1) || 
			(ch2 != ntv2_hout->scdc_sink_valid_ch2) || 
			(err0 != ntv2_hout->scdc_sink_error_ch0) ||
			(err1 != ntv2_hout->scdc_sink_error_ch1) ||
			(err2 != ntv2_hout->scdc_sink_error_ch2)) {
			NTV2_MSG_HDMIOUT4_SCDC("%s: scdc sink   ch0 valid: %s  errs: %5d   ch1 valid: %s  errs: %5d   ch2 valid: %s  errs: %5d   checksum %02x\n",
								   ntv2_hout->name,
								   ch0 ? "yes" : "no ", (err0 & 0x7fff),
								   ch1 ? "yes" : "no ", (err1 & 0x7fff),
								   ch2 ? "yes" : "no ", (err2 & 0x7fff),
								   checksum);
			ntv2_hout->scdc_sink_valid_ch0 = ch0;
			ntv2_hout->scdc_sink_valid_ch1 = ch1;
			ntv2_hout->scdc_sink_valid_ch2 = ch2;
			ntv2_hout->scdc_sink_error_ch0 = err0;
			ntv2_hout->scdc_sink_error_ch1 = err1;
			ntv2_hout->scdc_sink_error_ch2 = err2;
		}
	}

	return true;
}

static void disable_output(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t mask;

	if (!ntv2_hout->output_enable) return;

	NTV2_MSG_HDMIOUT4_CONFIG("%s: disable output\n", ntv2_hout->name);
	ntv2_hout->output_enable = false;

	// disable output
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_redrivercontrol_power, ntv2_con_hdmiout4_power_disable);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_redrivercontrol_power);
	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_redrivercontrol, ntv2_hout->index, value, mask);

	// set hdmi output status
	ntv2_vreg_write(ntv2_hout->system_context, ntv2_reg_hdmi_output_status1, ntv2_hout->index, 0);
}

static void enable_output(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t mask;

	if (ntv2_hout->output_enable) return;

	NTV2_MSG_HDMIOUT4_CONFIG("%s: enable output\n", ntv2_hout->name);
	ntv2_hout->output_enable = true;

	// enable output
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_redrivercontrol_power, ntv2_con_hdmiout4_power_enable);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_redrivercontrol_power);
	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_redrivercontrol, ntv2_hout->index, value, mask);

	// set hdmi output status
	value = NTV2_FLD_SET(ntv2_fld_hdmiout_status_video_standard, ntv2_hout->video_standard);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_frame_rate, ntv2_hout->frame_rate);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_bit_depth, ntv2_hout->color_depth);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_color_rgb, (ntv2_hout->color_space == ntv2_color_space_rgb444));
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_range_full, ntv2_hout->full_range);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_pixel_420, (ntv2_hout->color_space == ntv2_color_space_yuv420));
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_protocol, (ntv2_hout->hdmi_mode? NTV2_HDMIProtocolHDMI : NTV2_HDMIProtocolDVI));
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_audio_format, ntv2_hout->audio_format);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_audio_rate, ntv2_hout->audio_rate);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout_status_audio_channels,
						 ((ntv2_hout->audio_channels == 2)? NTV2_HDMIAudio2Channels :
						  ((ntv2_hout->audio_channels == 8)? NTV2_HDMIAudio8Channels :
						   NTV2_INVALID_HDMI_AUDIO_CHANNELS)));
	ntv2_vreg_write(ntv2_hout->system_context, ntv2_reg_hdmi_output_status1, ntv2_hout->index, value);
}

static void config_active(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t mask;

	// configuration mode active
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_txconfigmode, ntv2_con_hdmiout4_txconfigmode_active);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_videocontrol_txconfigmode);
	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index, value, mask);
}

static void config_valid(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t mask;

	// configuration mode valid
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_txconfigmode, ntv2_con_hdmiout4_txconfigmode_valid);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_videocontrol_txconfigmode);
	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index, value, mask);
}

static bool reset_transmit(struct ntv2_hdmiout4 *ntv2_hout, uint32_t timeout)
{
	uint32_t time = 0; 
	uint32_t value;
	uint32_t mask;

	NTV2_MSG_HDMIOUT4_CONFIG("%s: reset transmit\n", ntv2_hout->name);

	// toggle reset
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_videocontrol_reset);
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_reset, 1);
	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index, value, mask);
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_videocontrol_reset, 0);
	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index, value, mask);

	// wait for reset done to clear
	ntv2TimeSleep(100);

	// wait for reset done to set
	while (time < timeout)
	{
		value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index);
		value = NTV2_FLD_GET(ntv2_fld_hdmiout4_videocontrol_resetdone, value);
		if (value == 0) break;
		ntv2TimeSleep(100);
		if (!ntv2_hout->monitor_enable) return false;
		time += 100;
	}

	if (time >= timeout) return false;
	return true;
}


static bool check_sink_present(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t present;

	value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index);
	present = NTV2_FLD_GET(ntv2_fld_hdmiout4_videocontrol_sinkpresent, value);

	ntv2_hout->sink_present = (present == 1);

	return ntv2_hout->sink_present;
}

static bool check_force_hpd(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t force;

	value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmi_control, ntv2_hout->index);
	force = NTV2_FLD_GET(ntv2_fld_hdmiout_force_hpd, value);

	ntv2_hout->force_hpd = (force == 1);

	return ntv2_hout->force_hpd;
}

static bool is_new_hot_plug_event(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t count;

	value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout4_i2cedid, ntv2_hout->index);
	count = NTV2_FLD_GET(ntv2_fld_hdmiout4_i2cedid_hotplugcount, value);

	if (count == ntv2_hout->hot_plug_count)	{
		return false;
	}

	ntv2_hout->hot_plug_count = count;

	return true;
}

static bool is_clock_locked(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t lock;

	value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout4_videocontrol, ntv2_hout->index);
	lock = NTV2_FLD_GET(ntv2_fld_hdmiout4_videocontrol_txlockstate, value);

	return (lock == ntv2_con_hdmiout4_txlockstate_locked);
}

static bool is_genlocked(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t lock;

	value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_control_status, ntv2_hout->index);
	lock = NTV2_FLD_GET(ntv2_fld_control_genlock_locked, value);

	return (lock != 0);
}

static bool has_config_changed(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value_config;
	uint32_t value_source;
	uint32_t value_control;

	uint32_t mask_config = NTV2_FLD_MASK(ntv2_fld_hdmiout_video_standard) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_audio_group_select) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_frame_rate) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_deep_color) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_yuv_444) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_audio_format) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_full_range) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_audio_8ch) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_dvi);
	uint32_t mask_source = NTV2_FLD_MASK(ntv2_fld_hdmiout_hdmi_rgb);
	uint32_t mask_control = 
		NTV2_FLD_MASK(ntv2_fld_hdmiout_deep_12bit) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_force_hpd) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_audio_rate) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_source_select) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_crop_enable) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_channel34_swap_disable) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_channel_select) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_prefer_420) |
		NTV2_FLD_MASK(ntv2_fld_hdmiout_force_config);

	// read hdmi configuration
	value_config = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout_output_config, ntv2_hout->index);
	value_config &= mask_config;

	// read hdmi source rgb vs yuv crosspoint
	value_source = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout_cross_group6, ntv2_hout->index);
	value_source &= mask_source;

	// read audio configuration
	value_control = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmi_control, ntv2_hout->index);
	value_control &= mask_control;

	// look for changes
	if ((value_config == ntv2_hout->hdmi_config) &&
		(value_source == ntv2_hout->hdmi_source) &&
		(value_control == ntv2_hout->hdmi_control)) return false;

	// update state
	ntv2_hout->hdmi_config = value_config;
	ntv2_hout->hdmi_source = value_source;
	ntv2_hout->hdmi_control = value_control;

	return true;
}

static bool has_hdr_changed(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value_green_primary;
	uint32_t value_blue_primary;
	uint32_t value_red_primary;
	uint32_t value_white_point;
	uint32_t value_master_luminance;
	uint32_t value_light_level;
	uint32_t value_hdr_control;

	// read hdr cofiguration
	value_green_primary = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdr_green_primary, ntv2_hout->index);
	value_blue_primary = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdr_blue_primary, ntv2_hout->index);
	value_red_primary = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdr_red_primary, ntv2_hout->index);
	value_white_point = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdr_white_point, ntv2_hout->index);
	value_master_luminance = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdr_master_luminance, ntv2_hout->index);
	value_light_level = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdr_light_level, ntv2_hout->index);
	value_hdr_control = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdr_control, ntv2_hout->index);

	if ((value_green_primary == ntv2_hout->hdr_green_primary) &&
		(value_blue_primary == ntv2_hout->hdr_blue_primary) &&
		(value_red_primary == ntv2_hout->hdr_red_primary) &&
		(value_white_point == ntv2_hout->hdr_white_point) &&
		(value_master_luminance == ntv2_hout->hdr_master_luminance) &&
		(value_light_level == ntv2_hout->hdr_light_level) &&
		(value_hdr_control == ntv2_hout->hdr_control)) return false;

	// update state
	ntv2_hout->hdr_green_primary = value_green_primary;
	ntv2_hout->hdr_blue_primary = value_blue_primary;
	ntv2_hout->hdr_red_primary = value_red_primary;
	ntv2_hout->hdr_white_point = value_white_point;
	ntv2_hout->hdr_light_level = value_light_level;
	ntv2_hout->hdr_master_luminance = value_master_luminance;
	ntv2_hout->hdr_control = value_hdr_control;

	return true;
}

static bool is_active_i2c(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t done;

	value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout4_i2cedid, ntv2_hout->index);
	done = NTV2_FLD_GET(ntv2_fld_hdmiout4_i2cedid_done, value);

	return (done == 0);
}

static bool wait_for_i2c(struct ntv2_hdmiout4 *ntv2_hout, uint32_t timeout)
{
	uint32_t time = 0; 

	// wait for done to clear
	ntv2TimeSleep(100);

	while (time < timeout)
	{
		if (!is_active_i2c(ntv2_hout)) break;
		ntv2TimeSleep(100);
		if (!ntv2_hout->monitor_enable) return false;
		time += 100;
	}
	if (time >= timeout) return false;

	return true;
}

static void write_i2c(struct ntv2_hdmiout4 *ntv2_hout, uint32_t device, uint32_t address, uint32_t data)
{
	uint32_t value;

	// wait for engine
	if (!wait_for_i2c(ntv2_hout, c_i2c_timeout)) {
		NTV2_MSG_HDMIOUT4_I2C("%s: wait for i2c before i2c write failed\n", ntv2_hout->name);
	}

	// write i2c data
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_i2ccontrol_devaddress, device);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_i2ccontrol_subaddress, address);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_i2ccontrol_writedata, data);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_i2ccontrol, ntv2_hout->index, value);
}

static uint32_t read_i2c(struct ntv2_hdmiout4 *ntv2_hout, uint32_t device, uint32_t address)
{
	uint32_t value;
	uint32_t data;

	// wait for engine
	if (!wait_for_i2c(ntv2_hout, c_i2c_timeout)) {
		NTV2_MSG_HDMIOUT4_I2C("%s: wait for i2c before i2c read address failed\n", ntv2_hout->name);
	}

	// write i2c read command
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_i2ccontrol_devaddress, device);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_i2ccontrol_subaddress, address);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_i2ccontrol_read, 0x1);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_i2ccontrol, ntv2_hout->index, value);

	// wait for engine
	if (!wait_for_i2c(ntv2_hout, c_i2c_timeout)) {
		NTV2_MSG_HDMIOUT4_I2C("%s: wait for i2c before i2c read data failed\n", ntv2_hout->name);
	}

	// read i2c data
	value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout4_i2ccontrol, ntv2_hout->index);
	data = NTV2_FLD_GET(ntv2_fld_hdmiout4_i2ccontrol_readdata, value);

	return data;
}

static bool update_edid(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t value;
	uint32_t mask;

	// wait for engine
	if (!wait_for_i2c(ntv2_hout, c_i2c_timeout)) {
		NTV2_MSG_HDMIOUT4_I2C("%s: wait for i2c before update edid failed\n", ntv2_hout->name);
		return false;
	}

	// issue edid update
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_i2cedid_update, 0x1);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiout4_i2cedid_update);
	ntv2_reg_rmw(ntv2_hout->system_context, ntv2_reg_hdmiout4_i2cedid, ntv2_hout->index, value, mask);

	// wait for edid read
	if (!wait_for_i2c(ntv2_hout, c_i2c_timeout)) {
		NTV2_MSG_HDMIOUT4_I2C("%s: wait for i2c after update edid failed\n", ntv2_hout->name);
		return false;
	}

	return true;
}

static uint32_t read_edid(struct ntv2_hdmiout4 *ntv2_hout, uint32_t address)
{
	uint32_t value;
	uint32_t data;

	// wait for engine
	if (!wait_for_i2c(ntv2_hout, c_i2c_timeout)) {
		NTV2_MSG_HDMIOUT4_I2C("%s: wait for i2c before read edid address failed\n", ntv2_hout->name);
	}

	// write edid read address
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_i2cedid_subaddress, address);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_i2cedid, ntv2_hout->index, value);

	// wait for engine
	if (!wait_for_i2c(ntv2_hout, c_i2c_timeout)) {
		NTV2_MSG_HDMIOUT4_I2C("%s: wait for i2c before read edid data failed\n", ntv2_hout->name);
	}

	// read edid data
	value = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmiout4_i2cedid, ntv2_hout->index);
	data = NTV2_FLD_GET(ntv2_fld_hdmiout4_i2cedid_readdata, value);

//	NTV2_MSG_HDMIOUT4_EDID("%s: read edid addr %02x  data %02x\n", ntv2_hout->name, address, data);

	return data;
}

static uint32_t read_edid_register(struct ntv2_hdmiout4 *ntv2_hout, uint32_t block_num, uint32_t reg_num)
{
	uint32_t data;

	if (ntv2_hout == NULL) return 0;
	if ((block_num > 1) || (reg_num > 127)) return 0;

	// if no sink return 0
	if (!ntv2_hout->sink_present) {
		return 0;
	}

	// read edid data
	data = read_edid(ntv2_hout, (block_num*128)+reg_num);

	return data;
}

static void msg_edid_raw(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint8_t data[256];
	uint32_t index = 0;
	int i;
	int j;

	if (!NTV2_DEBUG_ACTIVE(NTV2_DEBUG_HDMIOUT4_EDID)) return;

	for (i = 0; i < 2; i++)
	{
		for (j = 0; j < 128; j++)
		{
			data[index++] = (uint8_t)read_edid_register(ntv2_hout, i, j);
		}
	}

	NTV2_MSG_HDMIOUT4_EDID("%s: hdmi edid data\n", ntv2_hout->name);
	for (i = 0; i < 2; i++)
	{
		NTV2_MSG_HDMIOUT4_EDID("%s\n", "");
		for (j = 0; j < 8; j++)
		{
			NTV2_MSG_HDMIOUT4_EDID("    %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
								   data[i * 128 + j * 16 + 0], data[i * 128 + j * 16 + 1], data[i * 128 + j * 16 + 2], data[i * 128 + j * 16 + 3],
								   data[i * 128 + j * 16 + 4], data[i * 128 + j * 16 + 5], data[i * 128 + j * 16 + 6], data[i * 128 + j * 16 + 7],
								   data[i * 128 + j * 16 + 8], data[i * 128 + j * 16 + 9], data[i * 128 + j * 16 + 10], data[i * 128 + j * 16 + 11],
								   data[i * 128 + j * 16 + 12], data[i * 128 + j * 16 + 13], data[i * 128 + j * 16 + 14], data[i * 128 + j * 16 + 15]);
		}
	}
	NTV2_MSG_HDMIOUT4_EDID("%s\n", "");
}

static void msg_edid_format(struct ntv2_hdmiout4 *ntv2_hout)
{
	struct ntv2_displayid_video* video = &ntv2_hout->edid.video;
	struct ntv2_displayid_audio* audio = &ntv2_hout->edid.audio;

	if (!NTV2_DEBUG_ACTIVE(NTV2_DEBUG_HDMIOUT4_PARSE)) return;

	if (ntv2_hout->edid.video.protocol != ntv2_displayid_protocol_hdmi) {
		NTV2_MSG_HDMIOUT4_PARSE("%s: dvi (no edid info)\n", ntv2_hout->name);
		return;
	}

	NTV2_MSG_HDMIOUT4_PARSE("%s: hdmi edid video info  max clock %d MHz  tmds %d Mcsc\n", ntv2_hout->name, 
						    video->max_clock_freq, video->max_tmds_csc);
	NTV2_MSG_HDMIOUT4_PARSE("%s:    RGB   24[%s]  30[%s]  36[%s]  48[%s]  bits/pixel\n", ntv2_hout->name, 
						    "y", video->dc_30bit?"y":"n", video->dc_36bit?"y":"n", video->dc_48bit?"y":"n");
	NTV2_MSG_HDMIOUT4_PARSE("%s:    SCDC  24[%s]  30[%s]  36[%s]  48[%s]  bits/pixel\n", ntv2_hout->name, 
						    video->scdc_present?"y":"n", video->dc_30bit_420?"y":"n", video->dc_36bit_420?"y":"n", video->dc_48bit_420?"y":"n");
	NTV2_MSG_HDMIOUT4_PARSE("%s:    YCbCr 422[%s]  420[%s]  444[%s]  444dc[%s]  sampling\n", ntv2_hout->name,
						    video->ycbcr_422?"y":"n", video->ycbcr_420?"y":"n", video->ycbcr_444?"y":"n",  video->dc_y444?"y":"n");
	NTV2_MSG_HDMIOUT4_PARSE("%s:    UHD   24[%s]  25[%s]  30[%s]  4K24[%s]  fps\n", ntv2_hout->name,
						    video->quad_24?"y":"n", video->quad_25?"y":"n", video->quad_30?"y":"n", video->four_24?"y":"n");
	NTV2_MSG_HDMIOUT4_PARSE("%s: hdmi edid audio info  basic[%s]  num channels %d\n", ntv2_hout->name, 
						    audio->basic_audio?"y":"n", audio->num_lpcm_channels);
}

static void clear_all_aux_data(struct ntv2_hdmiout4 *ntv2_hout)
{
    uint32_t value, i;

    for (i = 0; i < c_aux_buffer_size; i += 32)
	{
		value = NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxwrite, 1);
		value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxaddress, i);
		ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_auxcontrol, ntv2_hout->index, value);
	}
}

static void clear_aux_data(struct ntv2_hdmiout4 *ntv2_hout, uint32_t offset)
{
	uint32_t value;

	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxwrite, 1);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxaddress, offset);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_auxcontrol, ntv2_hout->index, value);
}
 
static void write_aux_data(struct ntv2_hdmiout4 *ntv2_hout, 
						   uint32_t offset, 
						   uint32_t size, 
						   uint8_t* data, 
						   bool checksum)
{
	uint32_t value;
	uint32_t sum = 0;
	uint32_t i;

	// compute length & checksum
	if (checksum) {
		data[2] = (uint8_t)(size - 4);
		for (i = 0; i < size; i++) {
			sum += data[i];
		}
		data[3] = ((~sum) + 1) & 0xff;
	}

	// write aux data
	for (i = 0; i < size; i++)
	{
		value = NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxwrite, 1);
		value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxaddress, (offset + i + 1));
		value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxdata, data[i]);
		ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_auxcontrol, ntv2_hout->index, value);
	}

	// enable aux packet
	value = NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxwrite, 1);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxaddress, offset);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiout4_auxcontrol_auxdata, 0x01);
	ntv2_reg_write(ntv2_hout->system_context, ntv2_reg_hdmiout4_auxcontrol, ntv2_hout->index, value);
}

static void update_debug_flags(struct ntv2_hdmiout4 *ntv2_hout)
{
	uint32_t val;

	val = ntv2_reg_read(ntv2_hout->system_context, ntv2_reg_hdmi_control, 0);
	val = NTV2_FLD_GET(ntv2_fld_hdmi_debug, val);
	if (val != 0)
	{
		ntv2_active_mask = ntv2_debug_mask;
	}
	else
	{
		ntv2_active_mask = ntv2_user_mask;
	}
}

static struct ntv2_hdmi_format_data* find_format_data(uint32_t video_standard, 
													  uint32_t frame_rate, 
													  int index)
{
	int i = 0;
	int ind = 0;

	while (c_hdmi_format_data[i].video_standard != ntv2_video_standard_none)
	{
		if ((video_standard == c_hdmi_format_data[i].video_standard) &&
			(frame_rate == c_hdmi_format_data[i].frame_rate))
		{
			if (ind == index)
			{
				return &c_hdmi_format_data[i];
			}
			ind++;
		}
		i++;
	}

	return NULL;
}

static struct ntv2_hdmi_clock_data* find_clock_data(enum ntv2_hdmi_clock_type	clockType,
													uint32_t	color_space,
													uint32_t	color_depth)
{
	int i = 0;
	while (c_hdmi_clock_data[i].clock_type != ntv2_clock_type_unknown)
	{
		if ((clockType == c_hdmi_clock_data[i].clock_type) &&
			(color_space == c_hdmi_clock_data[i].color_space) &&
			(color_depth == c_hdmi_clock_data[i].color_depth))
		{
			return &c_hdmi_clock_data[i];
		}
		i++;
	}

	return NULL;
}
