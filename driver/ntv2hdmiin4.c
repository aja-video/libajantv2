/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2hdmiin4.c
//
//==========================================================================

#include "ntv2hdmiin4.h"
#include "ntv2hin4reg.h"
#include "ntv2infoframe.h"
#include "ntv2enums.h"

// debug messages
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002
#define NTV2_DEBUG_HDMIIN4_STATE		0x00000004
#define NTV2_DEBUG_HDMIIN4_DETECT		0x00000008
#define NTV2_DEBUG_HDMIIN4_AUX			0x00000010

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	(((ntv2_debug_mask | ntv2_user_mask) & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_HDMIIN4_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_HDMIIN4_ERROR(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_HDMIIN4_STATE(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_HDMIIN4_STATE, string, __VA_ARGS__)
#define NTV2_MSG_HDMIIN4_DETECT(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_HDMIIN4_DETECT, string, __VA_ARGS__)
#define NTV2_MSG_HDMIIN4_AUX(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_HDMIIN4_AUX, string, __VA_ARGS__)

static uint32_t ntv2_debug_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR | NTV2_DEBUG_HDMIIN4_STATE | NTV2_DEBUG_HDMIIN4_DETECT;
static uint32_t ntv2_user_mask = 0;

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
	uint32_t					clock_ratio;
	uint8_t						avi_byte_4;
	uint8_t						hdmi_byte_5;
	enum ntv2_hdmi_clock_type	clock_type;
};

struct ntv2_hdmi_clock_data
{
	enum ntv2_hdmi_clock_type	clock_type;
	uint32_t					bit_depth;
	uint32_t					line_rate;
	uint32_t					tmds_rate;
};


static struct ntv2_hdmi_format_data c_hdmi_format_data[] = {
	{ ntv2_video_standard_525i,	       ntv2_frame_rate_2997,    19,   81,  138,  858,   19,  448,    4,    7,   22,  285,  266,  269,  262,  525,    2,    0,    0, ntv2_clock_type_sdn },
	{ ntv2_video_standard_625i,	       ntv2_frame_rate_2500,    12,   75,  144,  864,   12,  444,    2,    5,   24,  337,  314,  317,  312,  625,    2,    0,    0, ntv2_clock_type_sdn },
	{ ntv2_video_standard_720p,	       ntv2_frame_rate_5000,   440,  480,  700, 1980,  440,    0,    5,   10,   30,    0,    0,    0,  750,    0,    2, 0x13,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_720p,	       ntv2_frame_rate_5994,   110,  150,  370, 1650,  110,    0,    5,   10,   30,    0,    0,    0,  750,    0,    2, 0x04,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_720p,	       ntv2_frame_rate_6000,   110,  150,  370, 1650,  110,    0,    5,   10,   30,    0,    0,    0,  750,    0,    2, 0x04,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080i,       ntv2_frame_rate_2500,   528,  572,  720, 2640,  528, 1848,    2,    7,   22,  585,  564,  569,  562, 1125,    2, 0x14,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080i,       ntv2_frame_rate_2997,    88,  132,  280, 2200,   88, 1188,    2,    7,   22,  585,  564,  569,  562, 1125,    2, 0x05,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_1080i,       ntv2_frame_rate_3000,    88,  132,  280, 2200,   88, 1188,    2,    7,   22,  585,  564,  569,  562, 1125,    2, 0x05,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_2398,   638,  682,  830, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2, 0x20,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_2400,   638,  682,  830, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2, 0x20,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_2500,   528,  572,  720, 2640,  528,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2, 0x21,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_2997,    88,  132,  280, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2, 0x22,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_3000,    88,  132,  280, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2, 0x22,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_4795,   638,  682,  830, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1, 0x6f,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_4800,   638,  682,  830, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1, 0x6f,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_5000,   528,  572,  720, 2640,  528,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1, 0x1f,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_5994,    88,  132,  280, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1, 0x10,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_1080p,       ntv2_frame_rate_6000,    88,  132,  280, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1, 0x10,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_2048x1080i,  ntv2_frame_rate_2500,   528,  572,  702, 2750,  528,    0,    2,    7,   22,  585,  564,  569,  562, 1125,    2,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080i,  ntv2_frame_rate_2997,    88,  132,  152, 2200,   88, 1188,    2,    7,   22,  585,  564,  569,  562, 1125,    2,    0,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_2048x1080i,  ntv2_frame_rate_3000,    88,  132,  152, 2200,   88, 1188,    2,    7,   22,  585,  564,  569,  562, 1125,    2,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_2398,   638,  682,  702, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2,    0,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_2400,   638,  682,  702, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_2500,   528,  572,  592, 2640,  528,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_2997,    88,  132,  152, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2,    0,    0, ntv2_clock_type_hdd },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_3000,    88,  132,  152, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    2,    0,    0, ntv2_clock_type_hdn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_4795,   638,  682,  702, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1,    0,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_4800,   638,  682,  702, 2750,  638,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1,    0,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_5000,   528,  572,  592, 2640,  528,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1,    0,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_5000,   484,  528,  592, 2640,  484,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1,    0,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_5994,    88,  132,  152, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1,    0,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_2048x1080p,  ntv2_frame_rate_6000,    88,  132,  152, 2200,   88,    0,    4,    9,   45,    0,    0,    0, 1125,    0,    1,    0,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2398,   638,  682,  830, 2750,  638,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x5d,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2400,   638,  682,  830, 2750,  638,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x5d,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2500,   528,  572,  720, 2640,  528,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x60,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2997,    88,  132,  280, 2200,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x61,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_3000,    88,  132,  280, 2200,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x61,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2398,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x5d, 0x03, ntv2_clock_type_4kd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2400,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x5d, 0x03, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2500,  1056, 1144, 1440, 5280, 1056,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x5e, 0x02, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_2997,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x5f, 0x01, ntv2_clock_type_4kd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_3000,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x5f, 0x01, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_4795,   638,  682,  830, 2750,  638,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x72,    0, ntv2_clock_type_4kd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_4800,   638,  682,  830, 2750,  638,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x72,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_5000,   528,  572,  720, 2640,  528,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x60,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_5994,    88,  132,  280, 2200,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x61,    0, ntv2_clock_type_4kd },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_6000,    88,  132,  280, 2200,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x61,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_4795,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x72,    0, ntv2_clock_type_h2d },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_4800,  1276, 1364, 1660, 5500, 1276,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x72,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_5000,  1056, 1144, 1440, 5280, 1056,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x60,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_5994,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x61,    0, ntv2_clock_type_h2d },
	{ ntv2_video_standard_3840x2160p,  ntv2_frame_rate_6000,   176,  264,  560, 4400,  176,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x61,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2398,   510,  554,  702, 2750,  510,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x62,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2400,   510,  554,  702, 2750,  510,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x62,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2500,   484,  528,  592, 2640,  484,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x63,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2997,    44,   88,  152, 2200,   44,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x64,    0, ntv2_clock_type_3gd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_3000,    44,   88,  152, 2200,   44,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x64,    0, ntv2_clock_type_3gn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2398,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x62, 0x04, ntv2_clock_type_4kd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2400,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x62, 0x04, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2500,   968, 1056, 1184, 5280,  968,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x63,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_2997,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x64,    0, ntv2_clock_type_4kd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_3000,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x64,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_4795,   510,  554,  702, 2750,  510,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x73,    0, ntv2_clock_type_4kd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_4800,   510,  554,  702, 2750,  510,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x73,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_5000,   484,  528,  592, 2640,  484,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x65,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_5994,    44,   88,  152, 2200,   44,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x66,    0, ntv2_clock_type_4kd },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_6000,    44,   88,  152, 2200,   44,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x66,    0, ntv2_clock_type_4kn },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_4795,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x73,    0, ntv2_clock_type_h2d },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_4800,  1020, 1108, 1404, 5500, 1020,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x73,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_5000,   968, 1056, 1184, 5280,  968,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x65,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_5994,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x66,    0, ntv2_clock_type_h2d },
	{ ntv2_video_standard_4096x2160p,  ntv2_frame_rate_6000,    88,  176,  304, 4400,   88,    0,    8,   18,   90,    0,    0,    0, 2250,    0,    0, 0x66,    0, ntv2_clock_type_h2n },
	{ ntv2_video_standard_none,        ntv2_frame_rate_none,     0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0, ntv2_clock_type_unknown }
};


static struct ntv2_hdmi_clock_data c_hdmi_clock_data[] = {
	{ ntv2_clock_type_sdd,     8,      ntv2_con_hdmiin4_linerate_270mhz,       26973027 },
	{ ntv2_clock_type_sdd,    10,      ntv2_con_hdmiin4_linerate_337mhz,       33716284 },
	{ ntv2_clock_type_sdd,    12,      ntv2_con_hdmiin4_linerate_405mhz,       40459540 },

	{ ntv2_clock_type_sdn,     8,      ntv2_con_hdmiin4_linerate_270mhz,       27000000 },
	{ ntv2_clock_type_sdn,    10,      ntv2_con_hdmiin4_linerate_337mhz,       33750000 },
	{ ntv2_clock_type_sdn,    12,      ntv2_con_hdmiin4_linerate_405mhz,       40500000 },

	{ ntv2_clock_type_hdd,     8,      ntv2_con_hdmiin4_linerate_742mhz,       74175824 },
	{ ntv2_clock_type_hdd,    10,      ntv2_con_hdmiin4_linerate_928mhz,       92719780 },
	{ ntv2_clock_type_hdd,    12,      ntv2_con_hdmiin4_linerate_1113mhz,     111263736 },

	{ ntv2_clock_type_hdn,     8,      ntv2_con_hdmiin4_linerate_742mhz,       74250000 },
	{ ntv2_clock_type_hdn,    10,      ntv2_con_hdmiin4_linerate_928mhz,       92812500 },
	{ ntv2_clock_type_hdn,    12,      ntv2_con_hdmiin4_linerate_1113mhz,     111375000 },

	{ ntv2_clock_type_3gd,     8,      ntv2_con_hdmiin4_linerate_1485mhz,     148351648 },
	{ ntv2_clock_type_3gd,    10,      ntv2_con_hdmiin4_linerate_1856mhz,     185439560 },
	{ ntv2_clock_type_3gd,    12,      ntv2_con_hdmiin4_linerate_2227mhz,     222527472 },

	{ ntv2_clock_type_3gn,     8,      ntv2_con_hdmiin4_linerate_1485mhz,     148500000 },
	{ ntv2_clock_type_3gn,    10,      ntv2_con_hdmiin4_linerate_1856mhz,     185625000 },
	{ ntv2_clock_type_3gn,    12,      ntv2_con_hdmiin4_linerate_2227mhz,     222750000 },

	{ ntv2_clock_type_4kd,     8,      ntv2_con_hdmiin4_linerate_2970mhz,     296703297 },
	{ ntv2_clock_type_4kn,     8,      ntv2_con_hdmiin4_linerate_2970mhz,     297000000 },

	{ ntv2_clock_type_4kd,    10,      ntv2_con_hdmiin4_linerate_3712mhz,      92719780 },
	{ ntv2_clock_type_4kd,    12,      ntv2_con_hdmiin4_linerate_4455mhz,     111263736 },
	{ ntv2_clock_type_4kn,    10,      ntv2_con_hdmiin4_linerate_3712mhz,      92812500 },
	{ ntv2_clock_type_4kn,    12,      ntv2_con_hdmiin4_linerate_4455mhz,     111375000 },

	{ ntv2_clock_type_h2d,     8,      ntv2_con_hdmiin4_linerate_5940mhz,     148351648 },
	{ ntv2_clock_type_h2n,     8,      ntv2_con_hdmiin4_linerate_5940mhz,     148500000 },

	{ ntv2_clock_type_unknown, 0,       0,                                    0 }
};


static const int64_t c_default_timeout		= 250000;
static const int64_t c_redriver_time		= 10000;
static const int64_t c_plug_time			= 250000;
static const int64_t c_aux_time				= 100000;
static const uint32_t c_lock_wait_max		= 2;
static const uint32_t c_unlock_wait_max		= 4;
static const uint32_t c_plug_wait_max		= 32;


static void ntv2_hdmiin4_monitor(void* data);
static Ntv2Status ntv2_hdmiin4_initialize(struct ntv2_hdmiin4 *ntv2_hin);

static bool is_input_present(struct ntv2_hdmiin4 *ntv2_hin);
static bool is_input_locked(struct ntv2_hdmiin4 *ntv2_hin);
//static bool is_deserializer_locked(struct ntv2_hdmiin4 *ntv2_hin);
static void reset_lock(struct ntv2_hdmiin4 *ntv2_hin);
static void hot_plug(struct ntv2_hdmiin4 *ntv2_hin);
static bool has_video_input_changed(struct ntv2_hdmiin4 *ntv2_hin);
static bool has_audio_input_changed(struct ntv2_hdmiin4 *ntv2_hin);
static bool update_input_state(struct ntv2_hdmiin4 *ntv2_hin);
static bool has_audio_control_changed(struct ntv2_hdmiin4 *ntv2_hin);
static bool config_audio_control(struct ntv2_hdmiin4 *ntv2_hin);
static void config_aux_data(struct ntv2_hdmiin4 *ntv2_hin);
static void set_no_video(struct ntv2_hdmiin4 *ntv2_hin);
static bool edid_write(struct ntv2_hdmiin4 *ntv2_hin, struct ntv2_hdmiedid* edid);
static bool edid_write_data(struct ntv2_hdmiin4 *ntv2_hin, uint8_t address, uint8_t data);
static bool edid_read_data(struct ntv2_hdmiin4 *ntv2_hin, uint8_t address, uint8_t* data);
static bool edid_wait_not_busy(struct ntv2_hdmiin4 *ntv2_hin);
static void aux_init(struct ntv2_hdmiin4 *ntv2_hin);
static int aux_read_ready(struct ntv2_hdmiin4 *ntv2_hin);
static void aux_read_done(struct ntv2_hdmiin4 *ntv2_hin);
static void aux_read(struct ntv2_hdmiin4 *ntv2_hin, int index, uint32_t *aux_data);
static int aux_find(struct ntv2_hdmiin4 *ntv2_hin, int count, uint8_t aux_type);
static void update_debug_flags(struct ntv2_hdmiin4 *ntv2_hin);

static struct ntv2_hdmi_format_data* find_format_data(uint32_t h_sync_start,
													 uint32_t h_sync_end,
													 uint32_t h_de_start,
													 uint32_t h_total,
													 uint32_t v_trans_f1,
													 uint32_t v_trans_f2,
													 uint32_t v_sync_start_f1,
													 uint32_t v_sync_end_f1,
													 uint32_t v_de_start_f1,
													 uint32_t v_de_start_f2,
													 uint32_t v_sync_start_f2,
													 uint32_t v_sync_end_f2,
													 uint32_t v_total_f1,
													 uint32_t v_total_f2,
													  enum ntv2_hdmi_clock_type	clockType,
													  uint32_t h_tol,
													  uint32_t v_tol);
static struct ntv2_hdmi_clock_data* find_clock_data(uint32_t lineRate, uint32_t tmdsRate);

struct ntv2_hdmiin4 *ntv2_hdmiin4_open(Ntv2SystemContext* sys_con,
									   const char *name, int index)
{
	struct ntv2_hdmiin4 *ntv2_hin = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_hin = (struct ntv2_hdmiin4 *)ntv2MemoryAlloc(sizeof(struct ntv2_hdmiin4));
	if (ntv2_hin == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_hdmiin4 instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_hin, 0, sizeof(struct ntv2_hdmiin4));

	ntv2_hin->index = index;
#if defined(MSWindows)
	sprintf(ntv2_hin->name, "%s%d", name, index);
#else
	snprintf(ntv2_hin->name, NTV2_HDMIIN4_STRING_SIZE, "%s%d", name, index);
#endif
	ntv2_hin->system_context = sys_con;

	ntv2SpinLockOpen(&ntv2_hin->state_lock, sys_con);
	ntv2ThreadOpen(&ntv2_hin->monitor_task, sys_con, "hdmi4 input monitor");
	ntv2EventOpen(&ntv2_hin->monitor_event, sys_con);

	NTV2_MSG_HDMIIN4_INFO("%s: open ntv2_hdmiin4\n", ntv2_hin->name);

	return ntv2_hin;
}

void ntv2_hdmiin4_close(struct ntv2_hdmiin4 *ntv2_hin)
{
	if (ntv2_hin == NULL) 
		return;

	NTV2_MSG_HDMIIN4_INFO("%s: close ntv2_hdmiin4\n", ntv2_hin->name);

	ntv2_hdmiin4_disable(ntv2_hin);

	ntv2EventClose(&ntv2_hin->monitor_event);
	ntv2ThreadClose(&ntv2_hin->monitor_task);
	ntv2SpinLockClose(&ntv2_hin->state_lock);
	ntv2_hdmiedid_close(ntv2_hin->edid);

	memset(ntv2_hin, 0, sizeof(struct ntv2_hdmiin4));
	ntv2MemoryFree(ntv2_hin, sizeof(struct ntv2_hdmiin4));
}

Ntv2Status ntv2_hdmiin4_configure(struct ntv2_hdmiin4 *ntv2_hin,
								  enum ntv2_edid_type edid_type,
								  int port_index)
{
	Ntv2Status result = NTV2_STATUS_SUCCESS;

	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_HDMIIN4_INFO("%s: configure hdmi input device\n", ntv2_hin->name);

	// configure edid
	if (edid_type != ntv2_edid_type_unknown) {
		ntv2_hin->edid = ntv2_hdmiedid_open(ntv2_hin->system_context, "edid", 0); 
		if (ntv2_hin->edid != NULL) {
			result = ntv2_hdmiedid_configure(ntv2_hin->edid, edid_type, port_index);
			if (result != NTV2_STATUS_SUCCESS) {
				ntv2_hdmiedid_close(ntv2_hin->edid);
				ntv2_hin->edid = NULL;
				NTV2_MSG_HDMIIN4_ERROR("%s: *error* configure edid failed\n", ntv2_hin->name);
			}
		} else {
			NTV2_MSG_HDMIIN4_ERROR("%s: *error* open edid failed\n", ntv2_hin->name);
		}
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiin4_enable(struct ntv2_hdmiin4 *ntv2_hin)
{
	bool success ;

	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (ntv2_hin->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_HDMIIN4_STATE("%s: enable hdmi input monitor\n", ntv2_hin->name);

	ntv2EventClear(&ntv2_hin->monitor_event);
	ntv2_hin->monitor_enable = true;

	success = ntv2ThreadRun(&ntv2_hin->monitor_task, ntv2_hdmiin4_monitor, (void*)ntv2_hin);
	if (!success) {
		return NTV2_STATUS_FAIL;
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiin4_disable(struct ntv2_hdmiin4 *ntv2_hin)
{
	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (!ntv2_hin->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_HDMIIN4_STATE("%s: disable hdmi input monitor\n", ntv2_hin->name);

	ntv2_hin->monitor_enable = false;
	ntv2EventSignal(&ntv2_hin->monitor_event);

	ntv2ThreadStop(&ntv2_hin->monitor_task);

	return NTV2_STATUS_SUCCESS;
}

static void ntv2_hdmiin4_monitor(void* data)
{
	struct ntv2_hdmiin4 *ntv2_hin = (struct ntv2_hdmiin4 *)data;
	uint32_t lock_wait = 0;
	uint32_t unlock_wait = 0;
	uint32_t plug_wait = 0;
	uint32_t val = 0;
	bool lock = false;
	bool reset = false;
	bool input = false;
	bool new_input = true;

	if (ntv2_hin == NULL)
		return;

	NTV2_MSG_HDMIIN4_STATE("%s: hdmi input monitor task start\n", ntv2_hin->name);

	ntv2_hdmiin4_initialize(ntv2_hin);

	while (!ntv2ThreadShouldStop(&ntv2_hin->monitor_task) && ntv2_hin->monitor_enable) 
	{
		val = ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmi_control, 0);
		if ((val & NTV2_FLD_MASK(ntv2_fld_hdmi_disable_update)) != 0)
		{
			goto wait;
		}

		update_debug_flags(ntv2_hin);

		if (is_input_present(ntv2_hin)) {
			if (!input) {
				NTV2_MSG_HDMIIN4_STATE("%s: input present\n", ntv2_hin->name);
				lock_wait = 0;
				plug_wait = 0;
			}
			input = true;
		} else {
			if (input) {
				NTV2_MSG_HDMIIN4_STATE("%s: input absent\n", ntv2_hin->name);
				unlock_wait = c_lock_wait_max;
				plug_wait = 0;
			}
			input = false;
		}

		if (input && is_input_locked(ntv2_hin)) {
			reset = false;
			unlock_wait = 0;

			lock_wait++;
			if (lock_wait < c_lock_wait_max) {
				goto wait;
			}

			if (!lock) {
				NTV2_MSG_HDMIIN4_STATE("%s: input locked\n", ntv2_hin->name);
				lock = true;
				plug_wait = 0;
			}

			if (has_video_input_changed(ntv2_hin) || has_audio_input_changed(ntv2_hin)) {
				NTV2_MSG_HDMIIN4_STATE("%s: input change detected\n", ntv2_hin->name);
				new_input = true;
			}

			if (has_audio_control_changed(ntv2_hin)) {
				NTV2_MSG_HDMIIN4_STATE("%s: audio control change detected\n", ntv2_hin->name);
				config_audio_control(ntv2_hin);
			}

			if (ntv2_hin->video_standard == ntv2_video_standard_none) {
				new_input = true;
			}

			if (new_input) {
				if (!update_input_state(ntv2_hin)) {
					plug_wait++;
					if (plug_wait > c_plug_wait_max) {
						NTV2_MSG_HDMIIN4_STATE("%s: bad input state (hot plug)\n", ntv2_hin->name);
						plug_wait = 0;
						hot_plug(ntv2_hin);
					}
				}
				new_input = false;
			}

			config_aux_data(ntv2_hin);
		} 
		else {
			lock_wait = 0;

			unlock_wait++;
			if (unlock_wait < c_unlock_wait_max) {
				goto wait;
			}

			if (lock) {
				NTV2_MSG_HDMIIN4_STATE("%s: input unlocked\n", ntv2_hin->name);
				lock = false;
				plug_wait = 0;
			}

			if (!reset) {
				set_no_video(ntv2_hin);
				reset_lock(ntv2_hin);
				reset = true;
			}

			if (input) {
				plug_wait++;
				if (plug_wait > c_plug_wait_max) {
					NTV2_MSG_HDMIIN4_STATE("%s: lock timeout (hot plug)\n", ntv2_hin->name);
					plug_wait = 0;
					hot_plug(ntv2_hin);
				}
			}
		}

	wait:
		// sleep
		ntv2EventWaitForSignal(&ntv2_hin->monitor_event, c_default_timeout, true);
	}

	NTV2_MSG_HDMIIN4_STATE("%s: hdmi input monitor task stop\n", ntv2_hin->name);
	ntv2ThreadExit(&ntv2_hin->monitor_task);
	return;
}

static Ntv2Status ntv2_hdmiin4_initialize(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t value;
	uint32_t mask;

	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	ntv2_hin->horizontal_tol	= 10;
	ntv2_hin->vertical_tol		= 4;

	ntv2_hin->video_control		= 0;
	ntv2_hin->video_detect0		= 0;
	ntv2_hin->video_detect1		= 0;
	ntv2_hin->video_detect2		= 0;
	ntv2_hin->video_detect3		= 0;
	ntv2_hin->video_detect4		= 0;
	ntv2_hin->video_detect5		= 0;
	ntv2_hin->video_detect6		= 0;
	ntv2_hin->video_detect7		= 0;
	ntv2_hin->tmds_rate			= 0;

	ntv2_hin->input_locked		= false;
	ntv2_hin->hdmi_mode			= false;
	ntv2_hin->video_standard	= ntv2_video_standard_none;
	ntv2_hin->frame_rate		= ntv2_frame_rate_none;
	ntv2_hin->color_space		= ntv2_color_space_none;
	ntv2_hin->color_depth		= ntv2_color_depth_none;
	ntv2_hin->aspect_ratio		= ntv2_aspect_ratio_unknown;
	ntv2_hin->colorimetry		= ntv2_colorimetry_unknown;
	ntv2_hin->quantization		= ntv2_quantization_unknown;

	ntv2_hin->audio_swap		= true;
	ntv2_hin->audio_resample	= true;

	// write edid
	if (ntv2_hin->edid != NULL) {
		edid_write(ntv2_hin, ntv2_hin->edid);
	}

	// intialize aux reads
	aux_init(ntv2_hin);

	// configure hot plug and audio swap
	value = NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_hotplugmode, ntv2_con_hdmiin4_hotplugmode_disable);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_hotplugmode);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_audioswapmode, ntv2_con_hdmiin4_audioswapmode_enable);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_audioswapmode);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_audioresamplemode, ntv2_con_hdmiin4_audioresamplemode_enable);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_audioresamplemode);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, value, mask);

	// setup redriver
	value = NTV2_FLD_SET(ntv2_fld_hdmiin4_redrivercontrol_power, ntv2_con_hdmiin4_power_disable);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_redrivercontrol_power);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_redrivercontrol_pinmode, ntv2_con_hdmiin4_pinmode_enable);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_redrivercontrol_pinmode);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_redrivercontrol, ntv2_hin->index, value, mask);

	// wait for redriver reset
	ntv2EventWaitForSignal(&ntv2_hin->monitor_event, c_redriver_time, true);

	// enable redriver
	value = NTV2_FLD_SET(ntv2_fld_hdmiin4_redrivercontrol_power, ntv2_con_hdmiin4_power_enable);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_redrivercontrol_power);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_redrivercontrol, ntv2_hin->index, value, mask);

	// hot plug
	hot_plug(ntv2_hin);
	
	return NTV2_STATUS_SUCCESS;
}

static bool is_input_present(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t mask;
	uint32_t value;
	bool present;

	// read 5v detect
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_hdmi5vdetect);
	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index);
	present = (value & mask) == mask;

	// write hot plug
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_hotplugmode);
	value = present? ntv2_con_hdmiin4_hotplugmode_enable : ntv2_con_hdmiin4_hotplugmode_disable;
	value = NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_hotplugmode, value);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, value, mask);

	return present;
}

static bool is_input_locked(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_inputlock);
	uint32_t value;

	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index);
	if ((value & mask) == mask) return true;

	return false;
}
#if 0
static bool is_deserializer_locked(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_deserializerlock);
	uint32_t value;

	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index);
	if ((value & mask) == mask) return true;

	return false;
}
#endif
static void reset_lock(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t reset = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_reset);

	// reset lock
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, reset, reset);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, 0, reset);
}

static void hot_plug(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t value = 0;
	uint32_t mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_hotplugmode);

	// disable hot plug
	value = NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_hotplugmode, ntv2_con_hdmiin4_hotplugmode_disable);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, value, mask);

	// wait for input
	ntv2EventWaitForSignal(&ntv2_hin->monitor_event, c_plug_time, true);

	// configure hot plug
	is_input_present(ntv2_hin);
}

static bool has_video_input_changed(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t value;
	bool changed = false;
	uint32_t control_mask = 
		NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_scrambledetect) |
		NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_descramblemode) |
		NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_scdcratedetect) |
		NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_scdcratemode) |
		NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_linerate) |
		NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_inputlock) |
		NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_hdmi5vdetect);

	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index) & control_mask;
	if (value != ntv2_hin->video_control) {
		ntv2_hin->video_control = value;
		changed = true;
	}

	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videodetect0, ntv2_hin->index);
	if (value != ntv2_hin->video_detect0) {
		ntv2_hin->video_detect0 = value;
		changed = true;
	}
	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videodetect1, ntv2_hin->index);
	if (value != ntv2_hin->video_detect1) {
		ntv2_hin->video_detect1 = value;
		changed = true;
	}
	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videodetect2, ntv2_hin->index);
	if (value != ntv2_hin->video_detect2) {
		ntv2_hin->video_detect2 = value;
		changed = true;
	}
	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videodetect3, ntv2_hin->index);
	if (value != ntv2_hin->video_detect3) {
		ntv2_hin->video_detect3 = value;
		changed = true;
	}
	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videodetect4, ntv2_hin->index);
	if (value != ntv2_hin->video_detect4) {
		ntv2_hin->video_detect4 = value;
		changed = true;
	}
	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videodetect5, ntv2_hin->index);
	if (value != ntv2_hin->video_detect5) {
		ntv2_hin->video_detect5 = value;
		changed = true;
	}
	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videodetect6, ntv2_hin->index);
	if (value != ntv2_hin->video_detect6) {
		ntv2_hin->video_detect6 = value;
		changed = true;
	}
	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_videodetect7, ntv2_hin->index);
	if (value != ntv2_hin->video_detect7) {
		ntv2_hin->video_detect7 = value;
		changed = true;
	}

	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_tmdsclockfrequency, ntv2_hin->index);
	value = (value < 50000000)? (value & 0xfffff000) : (value & 0xffffc000);
	if (value != ntv2_hin->tmds_rate) {
		ntv2_hin->tmds_rate = value;
		changed = true;
	}

	return changed;
}

static bool has_audio_input_changed(struct ntv2_hdmiin4 *ntv2_hin)
{
#if defined (MSWindows)
	UNREFERENCED_PARAMETER(ntv2_hin);
#endif

	return false;
}

bool update_input_state(struct ntv2_hdmiin4 *ntv2_hin)
{
	struct ntv2_hdmi_clock_data* clock_data;
	struct ntv2_hdmi_format_data* format_data;
	uint32_t value;
	uint32_t mask;
	uint32_t line_rate;
	uint32_t color_depth;
	uint32_t color_space;
	uint32_t interface;
	uint32_t h_sync_start;
	uint32_t h_sync_end;
	uint32_t h_de_start;
	uint32_t h_total;
	uint32_t v_trans_f1;
	uint32_t v_trans_f2;
	uint32_t v_sync_start_f1;
	uint32_t v_sync_end_f1;
	uint32_t v_de_start_f1;
	uint32_t v_de_start_f2;
	uint32_t v_sync_start_f2;
	uint32_t v_sync_end_f2;
	uint32_t v_total_f1;
	uint32_t v_total_f2;

	bool input_locked = false;
	bool hdmi_mode = false;
	uint32_t video_rgb = 0;
	uint32_t video_deep = 0;
	uint32_t video_standard = 0;
	uint32_t video_prog = 0;
	uint32_t video_sd = 0;
	uint32_t frame_rate = 0;

	// read hardware input state
	line_rate = NTV2_FLD_GET(ntv2_fld_hdmiin4_videocontrol_linerate, ntv2_hin->video_control);

	NTV2_MSG_HDMIIN4_DETECT("%s: clock  line %d  tmds %d\n", 
							ntv2_hin->name, line_rate, ntv2_hin->tmds_rate);

	// find clock rate type base on hardware data
	clock_data = find_clock_data(line_rate, ntv2_hin->tmds_rate);
	if (clock_data == NULL)	{
		if (ntv2_hin->format_clock_count < 1) {
			NTV2_MSG_HDMIIN4_STATE("%s: unrecognized hardware clock data\n", ntv2_hin->name);
		}
		ntv2_hin->format_clock_count++;
		set_no_video(ntv2_hin);
		return false;
	}
	ntv2_hin->format_clock_count = 0;

	color_space = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect0_colorspace, ntv2_hin->video_detect0);
	color_depth = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect0_colordepth, ntv2_hin->video_detect0);
	interface = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect0_interfacemode, ntv2_hin->video_detect0);

	h_sync_start = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect1_hsyncstart, ntv2_hin->video_detect1);
	h_sync_end = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect1_hsyncend, ntv2_hin->video_detect1);
	
	h_de_start = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect2_hdestart, ntv2_hin->video_detect2);
	h_total = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect2_htotal, ntv2_hin->video_detect2);

	v_trans_f1 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect3_vtransf1, ntv2_hin->video_detect3);
	v_trans_f2 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect3_vtransf2, ntv2_hin->video_detect3);

	v_sync_start_f1 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect4_vsyncstartf1, ntv2_hin->video_detect4);
	v_sync_end_f1 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect4_vsyncendf1, ntv2_hin->video_detect4);

	v_de_start_f1 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect5_vdestartf1, ntv2_hin->video_detect5);
	v_de_start_f2 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect5_vdestartf2, ntv2_hin->video_detect5);

	v_sync_start_f2 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect6_vsyncstartf2, ntv2_hin->video_detect6);
	v_sync_end_f2 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect6_vsyncendf2, ntv2_hin->video_detect6);

	v_total_f1 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect7_vtotalf1, ntv2_hin->video_detect7);
	v_total_f2 = NTV2_FLD_GET(ntv2_fld_hdmiin4_videodetect7_vtotalf2, ntv2_hin->video_detect7);

	NTV2_MSG_HDMIIN4_DETECT("%s: detect  cs %d  cd %d  dvi %d\n", 
							ntv2_hin->name, color_space, color_depth, interface);
	NTV2_MSG_HDMIIN4_DETECT("%s: detect  hss %d  hse %d  hds %d  ht %d\n", 
							ntv2_hin->name, h_sync_start, h_sync_end, h_de_start, h_total);
	NTV2_MSG_HDMIIN4_DETECT("%s: detect  vtr1 %d  vtr2 %d  vss1 %d  vse1 %d\n", 
							ntv2_hin->name, v_trans_f1, v_trans_f2, v_sync_start_f1, v_sync_end_f1);
	NTV2_MSG_HDMIIN4_DETECT("%s: detect  vds1 %d  vds2 %d  vss2 %d  vse2 %d\n", 
							ntv2_hin->name, v_de_start_f1, v_de_start_f2, v_sync_start_f2, v_sync_end_f2);
	NTV2_MSG_HDMIIN4_DETECT("%s: detect  vtot1 %d  vtot2 %d\n", 
							ntv2_hin->name, v_total_f1, v_total_f2);

	// find the format based on the hardware registers
	format_data = find_format_data(h_sync_start,
								   h_sync_end,
								   h_de_start,
								   h_total,
								   v_trans_f1,
								   v_trans_f2,
								   v_sync_start_f1,
								   v_sync_end_f1,
								   v_de_start_f1,
								   v_de_start_f2,
								   v_sync_start_f2,
								   v_sync_end_f2,
								   v_total_f1,
								   v_total_f2,
								   clock_data->clock_type,
								   ntv2_hin->horizontal_tol,
								   ntv2_hin->vertical_tol);
	if (format_data == NULL) {
		if (ntv2_hin->format_raster_count < 1) {
			NTV2_MSG_HDMIIN4_STATE("%s: unrecognized hardware raster data\n", ntv2_hin->name);
		}
		ntv2_hin->format_raster_count++;
		set_no_video(ntv2_hin);
		return false;
	}
	ntv2_hin->format_raster_count = 0;

	// get video data
	input_locked = true;
	hdmi_mode = (interface == ntv2_con_hdmiin4_interfacemode_hdmi);
	video_standard = format_data->video_standard;
	frame_rate = format_data->frame_rate;
	video_prog = ntv2_video_standard_progressive(video_standard);
	video_rgb = (color_space == ntv2_color_space_rgb444)? 1 : 0;
	video_sd = ((video_standard == ntv2_video_standard_525i) || (video_standard == ntv2_video_standard_625i))? 1 : 0;
	video_deep = ((color_space == ntv2_color_space_yuv422) || 
				  ((color_space == ntv2_color_space_rgb444) && (color_depth != ntv2_color_depth_8bit)))? 1 : 0;

	// check to do 420 conversions
	if (color_space == ntv2_color_space_yuv420)
	{
		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_420mode, ntv2_con_hdmiin4_420mode_enable);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_420mode);
		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, value, mask);

		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_420convert, ntv2_con_hdmiin4_420convert_enable);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_420convert);
		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_pixelcontrol, ntv2_hin->index, value, mask);
	}
	else
	{
		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_420mode, ntv2_con_hdmiin4_420mode_disable);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_420mode);
		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, value, mask);

		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_420convert, ntv2_con_hdmiin4_420convert_disable);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_420convert);
		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_pixelcontrol, ntv2_hin->index, value, mask);
	}

	// check to do 4K conversions
	if (ntv2_video_standard_width(video_standard) > 2048)
	{
		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_hsyncdivide, ntv2_con_hdmiin4_hsyncdivide_none);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_hsyncdivide);

		value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_pixelsperclock, 4);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_pixelsperclock);

		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, value, mask);

		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_hlinefilter, ntv2_con_hdmiin4_hlinefilter_disable);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_hlinefilter);

		value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_clockratio, format_data->clock_ratio);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_clockratio);

		value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_lineinterleave, ntv2_con_hdmiin4_lineinterleave_enable);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_lineinterleave);

		value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_pixelinterleave, ntv2_con_hdmiin4_pixelinterleave_enable);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_pixelinterleave);

		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_pixelcontrol, ntv2_hin->index, value, mask);
	}
	else
	{
		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_hsyncdivide, ntv2_con_hdmiin4_hsyncdivide_none);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_hsyncdivide);

		value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_pixelsperclock, 1);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_pixelsperclock);

		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, value, mask);

		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_hlinefilter, ntv2_con_hdmiin4_hlinefilter_enable);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_hlinefilter);

		value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_clockratio, format_data->clock_ratio);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_clockratio);

		value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_lineinterleave, ntv2_con_hdmiin4_lineinterleave_disable);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_lineinterleave);

		value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_pixelinterleave, ntv2_con_hdmiin4_pixelinterleave_disable);
		mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_pixelinterleave);

		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_pixelcontrol, ntv2_hin->index, value, mask);
	}

	// disable crop
	value = NTV2_FLD_SET(ntv2_fld_hdmiin4_croplocation_start, 0x040);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_croplocation_end, 0x7bf);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_croplocation, ntv2_hin->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiin4_pixelcontrol_cropmode, ntv2_con_hdmiin4_cropmode_disable);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_pixelcontrol_cropmode);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_pixelcontrol, ntv2_hin->index, value, mask);

	// write input format
	value = NTV2_FLD_SET(ntv2_fld_hdmiin_locked, input_locked? 1 : 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_stable, input_locked? 1 : 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_rgb, video_rgb);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_deep_color, video_deep);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_code, video_standard);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_audio_2ch, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_progressive, video_prog);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_sd, video_sd);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_74_25, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_audio_rate, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_audio_word_length, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_format, video_standard);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_dvi, (hdmi_mode? 0 : 1));
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_rate, frame_rate);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin_input_status, ntv2_hin->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiin_color_space, color_space);
	if (color_space == ntv2_color_space_yuv422)
	{
		value |= NTV2_FLD_SET(ntv2_fld_hdmiin_color_depth, ntv2_color_depth_10bit);
	}
	else if (color_space == ntv2_color_space_yuv420)
	{
		value |= NTV2_FLD_SET(ntv2_fld_hdmiin_color_depth, ntv2_color_depth_8bit);
	}
	else
	{
		value |= NTV2_FLD_SET(ntv2_fld_hdmiin_color_depth, color_depth);
	}
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiin_color_space);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin_color_depth);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmi_control, ntv2_hin->index, value, mask);

//	if ((ntv2_hin->input_locked != input_locked) ||
//		(ntv2_hin->hdmi_mode != hdmi_mode) ||
//		(ntv2_hin->video_standard != video_standard) ||
//		(ntv2_hin->frame_rate != frame_rate) ||
//		(ntv2_hin->color_space != color_space) ||
//		(ntv2_hin->color_depth != color_depth)) 
	{
		NTV2_MSG_HDMIIN4_STATE("%s: new format  mode %s  std %s  rate %s  clr %s  dpth %s\n",
							   ntv2_hin->name,
							   hdmi_mode? "hdmi" : "dvi",
							   ntv2_video_standard_name(video_standard),
							   ntv2_frame_rate_name(frame_rate),
							   ntv2_color_space_name(color_space),
							   ntv2_color_depth_name(color_depth));

		ntv2_hin->input_locked = input_locked;
		ntv2_hin->hdmi_mode = hdmi_mode;
		ntv2_hin->video_standard = video_standard;
		ntv2_hin->frame_rate = frame_rate;
		ntv2_hin->color_space = color_space;
		ntv2_hin->color_depth = color_depth;
	}

	return true;
}

static bool has_audio_control_changed(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t value = 0;
	bool changed = false;
	bool audio_swap;
	bool audio_resample;

	value =  ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmi_control, ntv2_hin->index);
	audio_swap = NTV2_FLD_GET(ntv2_fld_hdmiin_channel34_swap_disable, value) == 0;
	audio_resample = NTV2_FLD_GET(ntv2_fld_hdmiin_rate_convert_enable, value) == 0;

	if (audio_swap != ntv2_hin->audio_swap) {
		ntv2_hin->audio_swap = audio_swap;
		changed = true;
	}


	if (audio_resample != ntv2_hin->audio_resample) {
		ntv2_hin->audio_resample = audio_resample;
		changed = true;
	}

	return changed;
}

static bool config_audio_control(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t data = ntv2_hin->audio_swap? ntv2_con_hdmiin4_audioswapmode_enable : ntv2_con_hdmiin4_audioswapmode_disable;
	uint32_t value = NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_audioswapmode, data);
	uint32_t mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_audioswapmode);

	data = ntv2_hin->audio_resample? ntv2_con_hdmiin4_audioresamplemode_enable : ntv2_con_hdmiin4_audioresamplemode_disable;
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_videocontrol_audioresamplemode, data);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin4_videocontrol_audioresamplemode);

	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmiin4_videocontrol, ntv2_hin->index, value, mask);

	NTV2_MSG_HDMIIN4_STATE("%s: new control  audio swap %s  resample %s\n", ntv2_hin->name, 
						   ntv2_hin->audio_swap? "enable":"disable",
						   ntv2_hin->audio_resample? "enable":"disable");

	return true;
}
	
static void config_aux_data(struct ntv2_hdmiin4 *ntv2_hin)
{
	struct ntv2_avi_info_data avi_data;
	struct ntv2_drm_info_data drm_data;
	struct ntv2_vsp_info_data vsp_data;
	uint32_t aux_data[ntv2_con_auxdata_size];
	uint32_t full_range = 0;
	uint32_t value = 0;
	uint32_t mask = 0;
	uint32_t red_x = 0;
	uint32_t red_y = 0;
	uint32_t green_x = 0;
	uint32_t green_y = 0;
	uint32_t blue_x = 0;
	uint32_t blue_y = 0;
	int aux_count = 0;
	int aux_index = 0;
	bool found = false;

	if (!ntv2_hin->hdmi_mode)
		return;
	
	aux_count = aux_read_ready(ntv2_hin);
	if (aux_count == 0) {
		aux_init(ntv2_hin);
		return;
	}

	NTV2_MSG_HDMIIN4_AUX("%s: detect %d info frames\n", ntv2_hin->name, aux_count);

	// look for video infoframe
	found = false;
	aux_index = aux_find(ntv2_hin, aux_count, (uint8_t)ntv2_con_header_type_video_info);
	if (aux_index >= 0) {
		NTV2_MSG_HDMIIN4_AUX("%s: found avi info frame %d\n", ntv2_hin->name, aux_index);

		aux_read(ntv2_hin, aux_index, aux_data);

		NTV2_MSG_HDMIIN4_AUX("%s: read avi info: %08x %08x %08x %08x\n",
							 ntv2_hin->name, aux_data[0], aux_data[1], aux_data[2], aux_data[3]);
		NTV2_MSG_HDMIIN4_AUX("%s: read avi info: %08x %08x %08x %08x\n",
							 ntv2_hin->name, aux_data[4], aux_data[5], aux_data[6], aux_data[7]);
	
		found = ntv2_aux_to_avi_info(aux_data, ntv2_con_auxdata_size, &avi_data);
		if (found) {
			if (avi_data.quantization != ntv2_hin->quantization) {
				full_range = (avi_data.quantization == ntv2_quantization_full)? 1 : 0;
				if ((avi_data.quantization == ntv2_quantization_default) &&
					(avi_data.color_space == ntv2_color_space_rgb444)) {
					full_range = 1;
				}
				value = NTV2_FLD_SET(ntv2_fld_hdmiin_full_range, full_range);
				mask = NTV2_FLD_MASK(ntv2_fld_hdmiin_full_range);
				ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmi_control, ntv2_hin->index, value, mask);
				NTV2_MSG_HDMIIN4_DETECT("%s: detect  quant %s  range %s \n", 
										ntv2_hin->name,
										((avi_data.quantization == ntv2_quantization_default)? "default" :
										 ((avi_data.quantization == ntv2_quantization_full)? "full" : "limited")),
										((full_range == 1)? "full" : "smpte"));
				ntv2_hin->quantization = avi_data.quantization;
			}

			if (avi_data.colorimetry != ntv2_hin->colorimetry) {
				switch (avi_data.colorimetry)
				{
				case ntv2_colorimetry_170m:
				case ntv2_colorimetry_xvycc_601:
				case ntv2_colorimetry_adobe_601:
					value = NTV2_FLD_SET(ntv2_fld_hdmiin4_colorimetry, NTV2_HDMIColorimetry601);
					break;
				case ntv2_colorimetry_bt709:
				case ntv2_colorimetry_xvycc_709:
				case ntv2_colorimetry_adobe_rgb:
					value = NTV2_FLD_SET(ntv2_fld_hdmiin4_colorimetry, NTV2_HDMIColorimetry709);
					break;
				case ntv2_colorimetry_bt2020_cl:
					value = NTV2_FLD_SET(ntv2_fld_hdmiin4_colorimetry, NTV2_HDMIColorimetry2020CL);
					break;
				case ntv2_colorimetry_bt2020:
					value = NTV2_FLD_SET(ntv2_fld_hdmiin4_colorimetry, NTV2_HDMIColorimetry2020);
					break;
				case ntv2_colorimetry_dcip3_d65:
					value = NTV2_FLD_SET(ntv2_fld_hdmiin4_colorimetry, NTV2_HDMIColorimetryDCI);
					break;
				default:
					value = NTV2_FLD_SET(ntv2_fld_hdmiin4_colorimetry, NTV2_HDMIColorimetryNoData);
					break;
				}
				mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_colorimetry);
				ntv2_vreg_rmw(ntv2_hin->system_context, ntv2_vreg_hdmiin4_avi_info, ntv2_hin->index, value, mask);
				ntv2_hin->colorimetry = avi_data.colorimetry;
			}

			ntv2_hin->aspect_ratio = avi_data.aspect_ratio;
		}
	}

	if (!found) {
		value = NTV2_FLD_SET(ntv2_fld_hdmiin_full_range, 0);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin_full_range);
		ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmi_control, ntv2_hin->index, value, mask);

		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_colorimetry, NTV2_HDMIColorimetryNoData);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_colorimetry);
		ntv2_vreg_rmw(ntv2_hin->system_context, ntv2_vreg_hdmiin4_avi_info, ntv2_hin->index, value, mask);

		ntv2_hin->quantization = ntv2_quantization_unknown;
		ntv2_hin->colorimetry = ntv2_colorimetry_unknown;
		ntv2_hin->aspect_ratio = ntv2_aspect_ratio_unknown;
	}

	// look for video infoframe
	found = false;
	aux_index = aux_find(ntv2_hin, aux_count, (uint8_t)ntv2_con_header_type_vendor_specific);
	if (aux_index >= 0) {
		NTV2_MSG_HDMIIN4_AUX("%s: found vendor specific info frame %d\n", ntv2_hin->name, aux_index);

		aux_read(ntv2_hin, aux_index, aux_data);

		NTV2_MSG_HDMIIN4_AUX("%s: read vendor specific info: %08x %08x %08x %08x\n",
							 ntv2_hin->name, aux_data[0], aux_data[1], aux_data[2], aux_data[3]);
		NTV2_MSG_HDMIIN4_AUX("%s: read vendor specific info: %08x %08x %08x %08x\n",
							 ntv2_hin->name, aux_data[4], aux_data[5], aux_data[6], aux_data[7]);

		found = ntv2_aux_to_vsp_info(aux_data, ntv2_con_auxdata_size, &vsp_data);
		if (found) {
			value = NTV2_FLD_SET(ntv2_fld_hdmiin4_dolby_vision, vsp_data.dolby_vision);
			mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_dolby_vision);
			ntv2_vreg_rmw(ntv2_hin->system_context, ntv2_vreg_hdmiin4_avi_info, ntv2_hin->index, value, mask);
		}
	}

	if (!found) {
		value = NTV2_FLD_SET(ntv2_fld_hdmiin4_dolby_vision, 0);
		mask = NTV2_FLD_MASK(ntv2_fld_hdmiin4_dolby_vision);
		ntv2_vreg_rmw(ntv2_hin->system_context, ntv2_vreg_hdmiin4_avi_info, ntv2_hin->index, value, mask);
	}

	// look for dynamic range mastering infoframe
	found = false;
	aux_index = aux_find(ntv2_hin, aux_count, (uint8_t)ntv2_con_header_type_drm_info);
	if (aux_index >= 0) {
		NTV2_MSG_HDMIIN4_AUX("%s: found drm info frame %d\n", ntv2_hin->name, aux_index);

		aux_read(ntv2_hin, aux_index, aux_data);

		NTV2_MSG_HDMIIN4_AUX("%s: read drm info: %08x %08x %08x %08x\n",
							 ntv2_hin->name, aux_data[0], aux_data[1], aux_data[2], aux_data[3]);
		NTV2_MSG_HDMIIN4_AUX("%s: read drm info: %08x %08x %08x %08x\n",
							 ntv2_hin->name, aux_data[4], aux_data[5], aux_data[6], aux_data[7]);

		found = ntv2_aux_to_drm_info(aux_data, ntv2_con_auxdata_size, &drm_data);
		if (found) {
			// drm info found
			if (drm_data.primary_x0 >= drm_data.primary_x1) {
				if (drm_data.primary_x0 >= drm_data.primary_x2) {
					red_x = drm_data.primary_x0;
					red_y = drm_data.primary_y0;
					if (drm_data.primary_y1 >= drm_data.primary_y2) {
						green_x = drm_data.primary_x1;
						green_y = drm_data.primary_y1;
						blue_x = drm_data.primary_x2;
						blue_y = drm_data.primary_y2;
					} else {
						green_x = drm_data.primary_x2;
						green_y = drm_data.primary_y2;
						blue_x = drm_data.primary_x1;
						blue_y = drm_data.primary_y1;
					}
				} else {
					red_x = drm_data.primary_x2;
					red_y = drm_data.primary_y2;
					if (drm_data.primary_y0 >= drm_data.primary_y1) {
						green_x = drm_data.primary_x0;
						green_y = drm_data.primary_y0;
						blue_x = drm_data.primary_x1;
						blue_y = drm_data.primary_y1;
					} else {
						green_x = drm_data.primary_x1;
						green_y = drm_data.primary_y1;
						blue_x = drm_data.primary_x0;
						blue_y = drm_data.primary_y0;
					}
				}
			} else {
				if (drm_data.primary_x1 >= drm_data.primary_x2) {
					red_x = drm_data.primary_x1;
					red_y = drm_data.primary_y1;
					if (drm_data.primary_y0 >= drm_data.primary_y2) {
						green_x = drm_data.primary_x0;
						green_y = drm_data.primary_y0;
						blue_x = drm_data.primary_x2;
						blue_y = drm_data.primary_y2;
					} else {
						green_x = drm_data.primary_x2;
						green_y = drm_data.primary_y2;
						blue_x = drm_data.primary_x0;
						blue_y = drm_data.primary_y0;
					}
				} else {
					red_x = drm_data.primary_x2;
					red_y = drm_data.primary_y2;
					if (drm_data.primary_y0 >= drm_data.primary_y1) {
						green_x = drm_data.primary_x0;
						green_y = drm_data.primary_y0;
						blue_x = drm_data.primary_x1;
						blue_y = drm_data.primary_y1;
					} else {
						green_x = drm_data.primary_x1;
						green_y = drm_data.primary_y1;
						blue_x = drm_data.primary_x0;
						blue_y = drm_data.primary_y0;
					}
				}
			}

			value = NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_present, 1);
			value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_eotf, drm_data.eotf);
			value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_metadata_id, drm_data.metadata_id);						
			ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_info, ntv2_hin->index, value);
			
			value = NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_red_x, red_x);
			value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_red_y, red_y);
			ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_red, ntv2_hin->index, value);

			value = NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_green_x, green_x);
			value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_green_y, green_y);
			ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_green, ntv2_hin->index, value);

			value = NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_blue_x, blue_x);
			value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_blue_y, blue_y);
			ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_blue, ntv2_hin->index, value);

			value = NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_white_x, drm_data.white_point_x);
			value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_white_y, drm_data.white_point_y);
			ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_white, ntv2_hin->index, value);

			value = NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_luma_max, drm_data.luminance_max);
			value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_luma_min, drm_data.luminance_min);
			ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_luma, ntv2_hin->index, value);

			value = NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_light_content_max, drm_data.content_level_max);
			value |= NTV2_FLD_SET(ntv2_fld_hdmiin4_drm_light_average_max, drm_data.frameavr_level_max);
			ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_light, ntv2_hin->index, value);
		}
	}

	if (!found) {
		ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_info, ntv2_hin->index, 0);
		ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_red, ntv2_hin->index, 0);
		ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_green, ntv2_hin->index, 0);
		ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_blue, ntv2_hin->index, 0);
		ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_white, ntv2_hin->index, 0);
		ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_luma, ntv2_hin->index, 0);
		ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_light, ntv2_hin->index, 0);
	}

	aux_read_done(ntv2_hin);
}

static void set_no_video(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t value;
	uint32_t mask;

	// clear fpga hdmi status
	value = NTV2_FLD_SET(ntv2_fld_hdmiin_locked, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_stable, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_rgb, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_deep_color, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_code, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_audio_2ch, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_progressive, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_sd, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_74_25, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_audio_rate, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_audio_word_length, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_format, 0);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_dvi, 1);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_video_rate, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin_input_status, ntv2_hin->index, value);

	value = NTV2_FLD_SET(ntv2_fld_hdmiin_color_space, 0);
	mask = NTV2_FLD_MASK(ntv2_fld_hdmiin_color_space);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_color_depth, 0);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin_color_depth);
	value |= NTV2_FLD_SET(ntv2_fld_hdmiin_full_range, 0);
	mask |= NTV2_FLD_MASK(ntv2_fld_hdmiin_full_range);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmi_control, ntv2_hin->index, value, mask);

	ntv2_hin->input_locked		= false;
	ntv2_hin->hdmi_mode			= false;
	ntv2_hin->video_standard	= ntv2_video_standard_none;
	ntv2_hin->frame_rate		= ntv2_frame_rate_none;
	ntv2_hin->color_space		= ntv2_color_space_none;
	ntv2_hin->color_depth		= ntv2_color_depth_none;
	ntv2_hin->aspect_ratio		= ntv2_aspect_ratio_unknown;
	ntv2_hin->colorimetry		= ntv2_colorimetry_unknown;
	ntv2_hin->quantization		= ntv2_quantization_unknown;

	// no info
	ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_avi_info, ntv2_hin->index, 0);
	ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_info, ntv2_hin->index, 0);
	ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_red, ntv2_hin->index, 0);
	ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_green, ntv2_hin->index, 0);
	ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_blue, ntv2_hin->index, 0);
	ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_white, ntv2_hin->index, 0);
	ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_luma, ntv2_hin->index, 0);
	ntv2_vreg_write(ntv2_hin->system_context, ntv2_vreg_hdmiin4_drm_light, ntv2_hin->index, 0);
}

static struct ntv2_hdmi_format_data* find_format_data(uint32_t h_sync_start,
													  uint32_t h_sync_end,
													  uint32_t h_de_start,
													  uint32_t h_total,
													  uint32_t v_trans_f1,
													  uint32_t v_trans_f2,
													  uint32_t v_sync_start_f1,
													  uint32_t v_sync_end_f1,
													  uint32_t v_de_start_f1,
													  uint32_t v_de_start_f2,
													  uint32_t v_sync_start_f2,
													  uint32_t v_sync_end_f2,
													  uint32_t v_total_f1,
													  uint32_t v_total_f2,
													  enum ntv2_hdmi_clock_type clock_type,
													  uint32_t h_tol,
													  uint32_t v_tol)
{
	uint32_t delta = 1000000;
	int min = (-1);
	int i = 0;
	uint32_t dhd = ntv2_diff(h_total, h_de_start);
	uint32_t df1d = ntv2_diff(v_total_f1, v_de_start_f1);
	uint32_t df2d = ntv2_diff(v_total_f2, v_de_start_f2);

	(void)h_sync_start;
	(void)h_sync_end;
	(void)v_trans_f1;
	(void)v_trans_f2;
	(void)v_sync_start_f1;
	(void)v_sync_end_f1;
	(void)v_de_start_f1;
	(void)v_de_start_f2;
	(void)v_sync_start_f2;
	(void)v_sync_end_f2;
	
	while (c_hdmi_format_data[i].video_standard != ntv2_video_standard_none)
	{
		uint32_t dht = ntv2_diff(c_hdmi_format_data[i].h_total, c_hdmi_format_data[i].h_de_start);
		uint32_t df1t = ntv2_diff(c_hdmi_format_data[i].v_total_f1, c_hdmi_format_data[i].v_de_start_f1);
		uint32_t df2t = ntv2_diff(c_hdmi_format_data[i].v_total_f2, c_hdmi_format_data[i].v_de_start_f2);
		uint32_t dh = ntv2_diff(dhd, dht);
		uint32_t df1 = ntv2_diff(df1d, df1t);
		uint32_t df2 = ntv2_diff(df2d, df2t);
		uint32_t dl;

		if ((clock_type == c_hdmi_format_data[i].clock_type) &&
			(dh <= h_tol) && (df1 <= v_tol) && (df2 <= v_tol))
		{
			if ((h_total == c_hdmi_format_data[i].h_total) &&
				(v_total_f1 == c_hdmi_format_data[i].v_total_f1) &&
				(v_total_f2 == c_hdmi_format_data[i].v_total_f2))
			{
				return &c_hdmi_format_data[i];
			}

			if (c_hdmi_format_data[i].v_total_f2 != 0)
			{
				dl = ntv2_diff((h_total * v_total_f2), (c_hdmi_format_data[i].h_total * c_hdmi_format_data[i].v_total_f2));
			}
			else
			{
				dl = ntv2_diff((h_total * v_total_f1), (c_hdmi_format_data[i].h_total * c_hdmi_format_data[i].v_total_f1));
			}

			if (dl == 0)
				return &c_hdmi_format_data[i];

			if (dl < delta) {
				delta = dl;
			    min = i;
			}
		}
		i++;
	}

	if (min < 0)
		return NULL;

	return &c_hdmi_format_data[min];
}

static struct ntv2_hdmi_clock_data* find_clock_data(uint32_t lineRate, uint32_t tmdsRate)
{
	uint32_t diff;
//	uint32_t tol;
	uint32_t minDiff = 1000000000;
	int min = (-1);
	int i = 0;

	// find the clock 
	while (c_hdmi_clock_data[i].clock_type != ntv2_clock_type_unknown)
	{
		if (lineRate == c_hdmi_clock_data[i].line_rate) {
			diff = ntv2_diff(tmdsRate, c_hdmi_clock_data[i].tmds_rate);
			if (diff < minDiff)	{
				minDiff = diff;
				min = i;
			}
		}
		i++;
	}
	
	if (min < 0)
		return NULL;

	// check clock rate (+|- 0.5%)
#if 0	
	tol = c_hdmi_clock_data[min].tmds_rate / 200;
	if ((tmdsRate < (c_hdmi_clock_data[min].tmds_rate - tol)) ||
		(tmdsRate > (c_hdmi_clock_data[min].tmds_rate + tol)))
	{
	}
#endif
	return &c_hdmi_clock_data[min];
}

static bool edid_write(struct ntv2_hdmiin4 *ntv2_hin, struct ntv2_hdmiedid* ntv2_edid)
{
	uint8_t* data = ntv2_hdmi_get_edid_data(ntv2_edid);
	uint32_t count = ntv2_hdmi_get_edid_size(ntv2_edid);
	uint32_t address = 0;
	uint8_t value;
	
	for (address = 0; address < count; address++) {
		if (!edid_write_data(ntv2_hin, (uint8_t)address, data[address])) {
			NTV2_MSG_HDMIIN4_ERROR("%s: *error* write edid failed  address %02x\n",
								   ntv2_hin->name, address);
			return false;
		}
	}

	for (address = 0; address < count; address++) {
		if (!edid_read_data(ntv2_hin, (uint8_t)address, &value)) {
			NTV2_MSG_HDMIIN4_ERROR("%s: *error* read edid failed  address %02x\n",
								   ntv2_hin->name, address);
			return false;
		}
		if (value != data[address]) {
			NTV2_MSG_HDMIIN4_ERROR("%s: *error* verify edid failed  address %02x  exp %02x  act %02x\n",
								   ntv2_hin->name, address, data[address], value);
			return false;
		}
	}

	return true;
}

static bool edid_write_data(struct ntv2_hdmiin4 *ntv2_hin, uint8_t address, uint8_t data)
{
	uint32_t value;

	// wait for not busy
	if (!edid_wait_not_busy(ntv2_hin)) return false;

	// write edid data
	value = NTV2_FLD_SET(ntv2_kona_fld_hdmiin4_edid_address, address);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin4_edid_write_data, data);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin4_edid_write_enable, 1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin4_edid, ntv2_hin->index, value);

	return true;
}

static bool edid_read_data(struct ntv2_hdmiin4 *ntv2_hin, uint8_t address, uint8_t* data)
{
	uint32_t value;

	// wait for not busy
	if (!edid_wait_not_busy(ntv2_hin)) return false;

	// request edid read
	value = NTV2_FLD_SET(ntv2_kona_fld_hdmiin4_edid_address, address);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin4_edid, ntv2_hin->index, value);

	// wait for read
	if (!edid_wait_not_busy(ntv2_hin)) return false;

	// read data
	value = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin4_edid, ntv2_hin->index);
	*data = (uint8_t)NTV2_FLD_GET(ntv2_kona_fld_hdmiin4_edid_read_data, value);

	return true;
}

static bool edid_wait_not_busy(struct ntv2_hdmiin4 *ntv2_hin)
{
	int i;
	uint32_t value;

	// spin until not busy
	for (i = 0; i < 1000; i++) {
		value = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin4_edid, ntv2_hin->index);
		value = NTV2_FLD_GET(ntv2_kona_fld_hdmiin4_edid_busy, value);
		if (value == 0) return true;
	}

	return false;
}

static void aux_init(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t aux;

	ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxpacketignore0, ntv2_hin->index, 0x07060504);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxpacketignore1, ntv2_hin->index, 0x0b0a0908);
		
	aux = NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxread, 1);
	aux |= NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxwrite, 1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxcontrol, ntv2_hin->index, aux);

	ntv2EventWaitForSignal(&ntv2_hin->monitor_event, c_aux_time, true);

	aux = NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxread, 1);
	aux |= NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxwrite, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxcontrol, ntv2_hin->index, aux);
}

static int aux_read_ready(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t aux;
	uint32_t active;
	uint32_t read;
	uint32_t write;
	uint32_t bank0;
	uint32_t bank1;
	
	aux = ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxcontrol, ntv2_hin->index);
	active = NTV2_FLD_GET(ntv2_fld_hdmiin4_auxcontrol_auxactive, aux);
	read = NTV2_FLD_GET(ntv2_fld_hdmiin4_auxcontrol_auxread, aux);
	write = NTV2_FLD_GET(ntv2_fld_hdmiin4_auxcontrol_auxwrite, aux);
	bank0 = NTV2_FLD_GET(ntv2_fld_hdmiin4_auxcontrol_bank0count, aux);
	bank1 = NTV2_FLD_GET(ntv2_fld_hdmiin4_auxcontrol_bank1count, aux);

	if (active != write)
		return false;

	if ((active == 0) && (write == 0)) {
		aux = NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxread, 1);
		aux |= NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxwrite, write);
		ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxcontrol, ntv2_hin->index, aux);
		if (bank1 > ntv2_con_auxdata_count)
			bank1 = ntv2_con_auxdata_count;
		return (int)bank1;
	}
	if ((active == 1) && (write == 1)) {
		aux = NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxread, 0);
		aux |= NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxwrite, write);
		ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxcontrol, ntv2_hin->index, aux);
		if (bank0 > ntv2_con_auxdata_count)
			bank0 = ntv2_con_auxdata_count;
		return (int)bank0;
	}

	return 0;
}

static void aux_read_done(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t aux;
	uint32_t active;
	uint32_t read;
	uint32_t write;
	
	aux = ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxcontrol, ntv2_hin->index);
	active = NTV2_FLD_GET(ntv2_fld_hdmiin4_auxcontrol_auxactive, aux);
	read = NTV2_FLD_GET(ntv2_fld_hdmiin4_auxcontrol_auxread, aux);
	write = NTV2_FLD_GET(ntv2_fld_hdmiin4_auxcontrol_auxwrite, aux);

	if ((active == 0) && (write == 0)) {
		aux = NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxread, 1);
		aux |= NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxwrite, 1);
		ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxcontrol, ntv2_hin->index, aux);
	}
	if ((active == 1) && (write == 1)) {
		aux = NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxread, 0);
		aux |= NTV2_FLD_SET(ntv2_fld_hdmiin4_auxcontrol_auxwrite, 0);
		ntv2_reg_write(ntv2_hin->system_context, ntv2_reg_hdmiin4_auxcontrol, ntv2_hin->index, aux);
	}
}

static void aux_read(struct ntv2_hdmiin4 *ntv2_hin, int index, uint32_t *aux_data)
{
	uint32_t regnum = NTV2_REG_NUM(ntv2_reg_hdmiin4_auxdata, (uint32_t)ntv2_hin->index) + (index*ntv2_con_auxdata_size);
	int i;
	
	for (i = 0; i < ntv2_con_auxdata_size; i++) {
		aux_data[i] = ntv2_regnum_read(ntv2_hin->system_context, (regnum + i));
	}
}

static int aux_find(struct ntv2_hdmiin4 *ntv2_hin, int count, uint8_t aux_type)
{
	uint32_t regnum = NTV2_REG_NUM(ntv2_reg_hdmiin4_auxdata, (uint32_t)ntv2_hin->index);
	uint8_t head;
	int i;

	for (i = 0; i < count; i++) {
		head = (uint8_t)ntv2_regnum_read(ntv2_hin->system_context, (regnum + (i*ntv2_con_auxdata_size)));
		if (head == aux_type)
			return i;
	}

	return (-1);
}

static void update_debug_flags(struct ntv2_hdmiin4 *ntv2_hin)
{
	uint32_t val;

	val = ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmi_control, 0);
	val = NTV2_FLD_GET(ntv2_fld_hdmi_debug, val);
	if (val != 0)
	{
		ntv2_user_mask = NTV2_DEBUG_HDMIIN4_STATE | NTV2_DEBUG_HDMIIN4_DETECT | NTV2_DEBUG_ERROR;
	}
	else
	{
		ntv2_user_mask = 0;
	}
}
