/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2videoraster.c
//
//==========================================================================

#include "ntv2videoraster.h"
#include "ntv2videorasterreg.h"
#include "ntv2kona.h"

/* debug messages */
#define NTV2_DEBUG_INFO					    0x00000001
#define NTV2_DEBUG_ERROR				    0x00000002
#define NTV2_DEBUG_VIDEORASTER_STATE		0x00000004
#define NTV2_DEBUG_VIDEORASTER_CONFIG		0x00000008

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	((ntv2_active_mask & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_VIDEORASTER_INFO(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_VIDEORASTER_ERROR(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_VIDEORASTER_STATE(string, ...)		NTV2_MSG_PRINT(NTV2_DEBUG_VIDEORASTER_STATE, string, __VA_ARGS__)
#define NTV2_MSG_VIDEORASTER_CONFIG(string, ...)	NTV2_MSG_PRINT(NTV2_DEBUG_VIDEORASTER_CONFIG, string, __VA_ARGS__)

//static uint32_t ntv2_debug_mask = 0xffffffff;
//static uint32_t ntv2_user_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;
static uint32_t ntv2_active_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR;
static const int64_t c_default_timeout		= 50000;

static const uint32_t c_video_standard_size = 0x100000;

static uint32_t s_standard_size = 0;
static uint32_t s_geometry_size = 0;
static uint32_t s_format_size = 0;
static uint32_t s_frame_rate_size = 0;
static uint32_t s_pixel_rate_size = 0;

// video standard data structure
struct standard_data
{
    uint32_t     video_standard;             // video standard
    uint32_t     active_width;               // number of active pixels per line
    uint32_t     line_total;                 // totals lines per frame
    uint32_t     field1_payloadid_line;      // field 1 video payload ID line number
    uint32_t     field1_active_line_start;   // field 1 active video start line number
    uint32_t     field1_active_line_end;     // field 1 active video end line number
    uint32_t     field2_payloadid_line;      // field 2 video payload ID line number
    uint32_t     field2_active_line_start;   // field 2 active start line number
    uint32_t     field2_active_line_end;     // field 2 active end line number
    uint32_t     fid_line_low;               // fid bit low transition  line number
    uint32_t     fid_line_high;              // fid bit line high transition line number
    uint32_t     alignment_line;             // RP-168 Annex A signal alignment point (1st line of analog V Sync)
    uint32_t     field1_switch_line;         // RP-168 Field 1 switching line
    uint32_t     field2_switch_line;         // RP-168 Field 2 switching line
    uint32_t     video_scan;                 // progressive/top field 1st/bottom field 1st
};

// standard data must match enum order
static const struct standard_data c_standard_data[] = 
{
    // video standard                   awidth  ltot    vpid1   astart1 aend1   vpid2   astart2 aend2   flow    fhigh   align   sw1     sw2     video scan
    { ntv2_video_standard_1080i,        1920,   1125,   10,     21,     560,    572,    584,    1123,   1,      564,    1,      7,      569,    ntv2_video_scan_top_first },
    { ntv2_video_standard_720p,         1280,   750,    10,     26,     745,    0,      0,      0,      0,      0,      1,      7,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_525i,         720,    525,    13,     18,     263,    276,    280,    525,    4,      266,    4,      10,     273,    ntv2_video_scan_bottom_first },
    { ntv2_video_standard_625i,         720,    625,    9,      23,     310,    322,    336,    623,    1,      313,    1,      6,      319,    ntv2_video_scan_top_first },
    { ntv2_video_standard_1080p,        1920,   1125,   10,     42,     1121,   0,      0,      0,      0,      0,      1,      7,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_2048x1556,    2048,   1980,   195,    211,    988,    1185,   1201,   1978,   1,      991,    1,      7,      0,      ntv2_video_scan_top_first },
    { ntv2_video_standard_2048x1080p,   2048,   1125,   10,     42,     1121,   0,      0,      0,      0,      0,      1,      7,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_2048x1080i,   2048,   1125,   10,     21,     560,    572,    584,    1123,   1,      564,    1,      7,      569,    ntv2_video_scan_top_first },
    { ntv2_video_standard_3840x2160p,   3840,   2250,   0,      84,     2242,   0,      0,      0,      0,      0,      1,      7,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_4096x2160p,   4096,   2250,   0,      84,     2242,   0,      0,      0,      0,      0,      1,      7,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_3840_hfr,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_4096_hfr,     0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_7680,         0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_8192,         0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      ntv2_video_scan_progressive },
    { ntv2_video_standard_3840i,        0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      ntv2_video_scan_top_first },
    { ntv2_video_standard_4096i,        0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      ntv2_video_scan_top_first },
    { ntv2_video_standard_none,         0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      ntv2_video_scan_unknown }
};

// frame geometry data structure
struct geometry_data
{
    uint32_t     frame_geometry;
    uint32_t     frame_width;
    uint32_t     frame_height;
};

// geometry data must match enum order
static const struct geometry_data c_geometry_data[] = 
{
    { ntv2_frame_geometry_1920x1080,     1920,  1080 },
    { ntv2_frame_geometry_1280x720,      1280,  720 },
    { ntv2_frame_geometry_720x486,       720,   486 },
    { ntv2_frame_geometry_720x576,       720,   576 },
    { ntv2_frame_geometry_1920x1114,     1920,  1114 },
    { ntv2_frame_geometry_2048x1114,     2048,  1114 },
    { ntv2_frame_geometry_720x508,       720,   508 },
    { ntv2_frame_geometry_720x598,       720,   598 },
    { ntv2_frame_geometry_1920x1112,     1920,  1112 },
    { ntv2_frame_geometry_1280x740,      1280,  740 },
    { ntv2_frame_geometry_2048x1080,     2048,  1080 },
    { ntv2_frame_geometry_2048x1556,     2048,  1556 },
    { ntv2_frame_geometry_2048x1588,     2048,  1588 },
    { ntv2_frame_geometry_2048x1112,     2048,  1112 },
    { ntv2_frame_geometry_720x514,       720,   514 },
    { ntv2_frame_geometry_720x612,       720,   612 }
};

// frame format data structure
struct format_data
{
    uint32_t     pixel_format;           // pixel format
    uint32_t     pixels_cadence_width;   // pixels per cadence of active video width
    uint32_t     bytes_cadence_width;    // bytes per cadence of active video width
    uint32_t     pixels_cadence_pitch;   // pixels per cadence of video pitch
    uint32_t     bytes_cadence_pitch;    // bytes per cadence of video pitch
    uint32_t     pixel_components;       // components per pixel
    uint32_t     color_space;            // color space
    uint32_t     bit_depth;              // bit depth
    uint32_t     frame_format;           // frame format
};

// format data must match enum order
static const struct format_data c_format_data[] = 
{
    // pixel format                              pcw bcw pcp bcp pc  color space                bit depth                 frame format
    { ntv2_pixel_format_10bit_ycbcr,             6,  16, 48, 128,2,  ntv2_color_space_yuv422,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_8bit_ycbcr,              2,  4,  2,  4,  2,  ntv2_color_space_yuv422,   ntv2_color_depth_8bit,    ntv2_frame_format_packed },
    { ntv2_pixel_format_argb,                    1,  4,  1,  4,  4,  ntv2_color_space_rgb444,   ntv2_color_depth_8bit,    ntv2_frame_format_packed },
    { ntv2_pixel_format_rgba,                    1,  4,  1,  4,  4,  ntv2_color_space_rgb444,   ntv2_color_depth_8bit,    ntv2_frame_format_packed },
    { ntv2_pixel_format_10bit_rgb,               1,  4,  1,  4,  3,  ntv2_color_space_rgb444,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_8bit_ycbcr_yuy2,         1,  4,  2,  4,  2,  ntv2_color_space_yuv422,   ntv2_color_depth_8bit,    ntv2_frame_format_packed },
    { ntv2_pixel_format_abgr,                    1,  4,  1,  4,  4,  ntv2_color_space_rgb444,   ntv2_color_depth_8bit,    ntv2_frame_format_packed },
    { ntv2_pixel_format_10bit_dpx,               1,  4,  1,  4,  3,  ntv2_color_space_rgb444,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_10bit_ycbcr_dpx,         6,  16, 48, 128,2,  ntv2_color_space_yuv422,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_8bit_dvcpro,             2,  4,  2,  4,  2,  ntv2_color_space_none,     ntv2_color_depth_none,    ntv2_frame_format_packed },
    { ntv2_pixel_format_8bit_ycbcr_420pl3,       1,  1,  1,  1,  1,  ntv2_color_space_yuv420,   ntv2_color_depth_8bit,    ntv2_frame_format_3plane },
    { ntv2_pixel_format_8bit_hdv,                2,  4,  2,  4,  2,  ntv2_color_space_yuv422,   ntv2_color_depth_8bit,    ntv2_frame_format_packed },
    { ntv2_pixel_format_24bit_rgb,               1,  3,  1,  3,  3,  ntv2_color_space_rgb444,   ntv2_color_depth_8bit,    ntv2_frame_format_packed },
    { ntv2_pixel_format_24bit_bgr,               1,  3,  1,  3,  3,  ntv2_color_space_rgb444,   ntv2_color_depth_8bit,    ntv2_frame_format_packed },
    { ntv2_pixel_format_10bit_ycbcra,            1,  1,  1,  1,  4,  ntv2_color_space_yuv422,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_10bit_dpx_le,            1,  4,  1,  4,  3,  ntv2_color_space_rgb444,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_48bit_rgb,               1,  6,  1,  6,  3,  ntv2_color_space_rgb444,   ntv2_color_depth_12bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_12bit_rgb_packed,        6,  9,  12, 18, 3,  ntv2_color_space_yuv422,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_prores_dvcpro,           1,  1,  1,  1,  1,  ntv2_color_space_none,     ntv2_color_depth_none,    ntv2_frame_format_raw },
    { ntv2_pixel_format_prores_hdv,              1,  1,  1,  1,  1,  ntv2_color_space_none,     ntv2_color_depth_none,    ntv2_frame_format_raw },
    { ntv2_pixel_format_10bit_rgb_packed,        12, 15, 12, 15, 3,  ntv2_color_space_rgb444,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_10bit_argb,              1,  1,  1,  1,  4,  ntv2_color_space_rgb444,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_16bit_argb,              1,  8,  1,  8,  4,  ntv2_color_space_rgb444,   ntv2_color_depth_16bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_8bit_ycbcr_422pl3,       1,  1,  1,  1,  1,  ntv2_color_space_yuv422,   ntv2_color_depth_12bit,   ntv2_frame_format_3plane },
    { ntv2_pixel_format_10bit_raw_rgb,           1,  1,  1,  1,  1,  ntv2_color_space_rgb444,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_10bit_raw_ycbcr,         1,  1,  1,  1,  1,  ntv2_color_space_yuv422,   ntv2_color_depth_10bit,   ntv2_frame_format_packed },
    { ntv2_pixel_format_10bit_ycbcr_420pl3_le,   1,  1,  1,  1,  1,  ntv2_color_space_yuv420,   ntv2_color_depth_10bit,   ntv2_frame_format_3plane },
    { ntv2_pixel_format_10bit_ycbcr_422pl3_le,   1,  1,  1,  1,  1,  ntv2_color_space_yuv422,   ntv2_color_depth_12bit,   ntv2_frame_format_3plane },
    { ntv2_pixel_format_10bit_ycbcr_420pl2,      1,  1,  1,  1,  1,  ntv2_color_space_yuv420,   ntv2_color_depth_10bit,   ntv2_frame_format_2plane },
    { ntv2_pixel_format_10bit_ycbcr_422pl2,      1,  1,  1,  1,  1,  ntv2_color_space_yuv422,   ntv2_color_depth_12bit,   ntv2_frame_format_2plane },
    { ntv2_pixel_format_8bit_ycbcr_420pl2,       1,  1,  1,  1,  1,  ntv2_color_space_yuv420,   ntv2_color_depth_12bit,   ntv2_frame_format_2plane },
    { ntv2_pixel_format_8bit_ycbcr_422pl2,       1,  1,  1,  1,  1,  ntv2_color_space_yuv422,   ntv2_color_depth_10bit,   ntv2_frame_format_2plane }
};

// frame rate data structure
struct frame_rate_data
{
    uint32_t     frame_rate;             // video rate
    uint32_t     scale;                  // sync rate scale (ticks/second)
    uint32_t     duration;               // sync duration (ticks/sync)
};

// video rate data must match enum order
static const struct frame_rate_data c_frame_rate_data[] = 
{
    // frame rate                scale   duration
    { ntv2_frame_rate_none,      1,      1 },
    { ntv2_frame_rate_6000,      60,     1 },
    { ntv2_frame_rate_5994,      60000,  1001 },
    { ntv2_frame_rate_3000,      30,     1 },
    { ntv2_frame_rate_2997,      30000,  1001 },
    { ntv2_frame_rate_2500,      25,     1 },
    { ntv2_frame_rate_2400,      24,     1 },
    { ntv2_frame_rate_2398,      24000,  1001 },
    { ntv2_frame_rate_5000,      50,     1 },
    { ntv2_frame_rate_4800,      48,     1 },
    { ntv2_frame_rate_4795,      48000,  1001 },
    { ntv2_frame_rate_12000,     120,    1 },
    { ntv2_frame_rate_11988,     120000, 1001 },
    { ntv2_frame_rate_1500,      15,     1 },
    { ntv2_frame_rate_1498,      15000,  1001 }
};


// pixel rate data structure
struct pixel_rate_data
{
    uint32_t     pixel_rate;             // pixel rate
    uint64_t     scale;                  // pixel rate scale (ticks/second)
    uint64_t     duration;               // pixel duration (ticks/pixel)
};

// pixel rate data must match enum order
static const struct pixel_rate_data c_pixel_rate_data[] = 
{
    // pixel rate                scale               duration
    { ntv2_pixel_rate_none,      1,                  1 },
    { ntv2_pixel_rate_1350,      13500000,           1 },
    { ntv2_pixel_rate_2700,      27000000,           1 },
    { ntv2_pixel_rate_7418,      74250000000ULL,     1001 },
    { ntv2_pixel_rate_7425,      74250000,           1 },
    { ntv2_pixel_rate_14835,     148500000000ULL,    1001 },
    { ntv2_pixel_rate_14850,     148500000,          1 }
};

// video timing offset structure
struct timing_offset_data
{
    uint32_t     video_standard;            // video standard
    int32_t      cap_roi_hoffset;           // capture roi horizontal offset
    int32_t      cap_roi_voffset;           // capture roi vertical offset
    int32_t      dsp_roi_hoffset;           // display roi horizontal offset
    int32_t      dsp_roi_voffset;           // display roi vertical offset
    int32_t      cap_fid_offset;            // capture fid offset 
    int32_t      dsp_fid_offset;            // display fid offset 
};

// timing offset data must match standard enum order
static const struct timing_offset_data c_offset_data[] =
{
    // video standard                   crh     crv     drh     drv     cf      df
    { ntv2_video_standard_1080i,        1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_720p,         1,      5,      1,      0,      0,      0 },
    { ntv2_video_standard_525i,         1,      0,      1,      3,      0,      0 },
    { ntv2_video_standard_625i,         1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_1080p,        1,      4,      1,      0,      0,      0 },
    { ntv2_video_standard_2048x1556,    1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_2048x1080p,   1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_2048x1080i,   1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_3840x2160p,   1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_4096x2160p,   1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_3840_hfr,     1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_4096_hfr,     1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_7680,         1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_8192,         1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_3840i,        1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_4096i,        1,      0,      1,      0,      0,      0 },
    { ntv2_video_standard_none,         1,      0,      1,      0,      0,      0 }
};

static Ntv2Status ntv2_videoraster_initialize(struct ntv2_videoraster *ntv2_raster, uint32_t index);
static bool has_config_changed(struct ntv2_videoraster *ntv2_raster, uint32_t index);
static bool update_format(struct ntv2_videoraster *ntv2_raster, uint32_t index);
static bool update_format_single(struct ntv2_videoraster *ntv2_raster, uint32_t index);
static uint32_t get_frame_size(struct ntv2_videoraster *ntv2_raster, uint32_t index);
//static bool is_4k_sqd_mode(struct ntv2_videoraster *ntv2_raster, uint32_t index);
//static bool is_4k_tsi_mode(struct ntv2_videoraster *ntv2_raster, uint32_t index);
//static bool is_8k_sqd_mode(struct ntv2_videoraster *ntv2_raster, uint32_t index);
//static bool is_8k_tsi_mode(struct ntv2_videoraster *ntv2_raster, uint32_t index);
static uint32_t get_sdi_pixel_rate(uint32_t video_standard, uint32_t frame_rate);

struct ntv2_videoraster *ntv2_videoraster_open(Ntv2SystemContext* sys_con,
                                               const char *name, int index)
{
	struct ntv2_videoraster *ntv2_raster = NULL;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_raster = (struct ntv2_videoraster *)ntv2MemoryAlloc(sizeof(struct ntv2_videoraster));
	if (ntv2_raster == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_videoraster instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_raster, 0, sizeof(struct ntv2_videoraster));

	ntv2_raster->index = index;
#if defined(MSWindows)
	sprintf(ntv2_raster->name, "%s%d", name, index);
#else
	snprintf(ntv2_raster->name, NTV2_VIDEORASTER_STRING_SIZE, "%s%d", name, index);
#endif
	ntv2_raster->system_context = sys_con;

	ntv2InterruptLockOpen(&ntv2_raster->state_lock, sys_con);

    s_standard_size = sizeof(c_standard_data) / sizeof(struct standard_data);
    s_geometry_size = sizeof(c_geometry_data) / sizeof(struct geometry_data);
    s_format_size = sizeof(c_format_data) / sizeof(struct format_data);
    s_frame_rate_size = sizeof(c_frame_rate_data) / sizeof(struct frame_rate_data);
    s_pixel_rate_size = sizeof(c_pixel_rate_data) / sizeof(struct pixel_rate_data);
    
	NTV2_MSG_VIDEORASTER_INFO("%s: open ntv2_videoraster index %d\n", 
        ntv2_raster->name, index);

	return ntv2_raster;
}

void ntv2_videoraster_close(struct ntv2_videoraster *ntv2_raster)
{
	if (ntv2_raster == NULL)
		return;

	NTV2_MSG_VIDEORASTER_INFO("%s: close ntv2_videoraster\n", ntv2_raster->name);

	ntv2_videoraster_disable(ntv2_raster);

    ntv2InterruptLockClose(&ntv2_raster->state_lock);
	memset(ntv2_raster, 0, sizeof(struct ntv2_videoraster));
	ntv2MemoryFree(ntv2_raster, sizeof(struct ntv2_videoraster));
}

Ntv2Status ntv2_videoraster_configure(struct ntv2_videoraster *ntv2_raster, 
    uint32_t widget_base, uint32_t widget_size, uint32_t num_widgets)
{
	if (ntv2_raster == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

    if ((num_widgets == 0) || (num_widgets > NTV2_VIDEORASTER_MAX_WIDGETS))
    {
        NTV2_MSG_VIDEORASTER_ERROR("%s: bad number of widgets %d\n", 
            ntv2_raster->name, num_widgets);
        return NTV2_STATUS_BAD_PARAMETER;
    }

	NTV2_MSG_VIDEORASTER_INFO("%s: configure video raster base %08x  num %d\n", 
        ntv2_raster->name, widget_base, num_widgets);

    ntv2_raster->widget_base = widget_base;
    ntv2_raster->widget_size = widget_size;
    ntv2_raster->num_widgets = num_widgets;

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_videoraster_enable(struct ntv2_videoraster *ntv2_raster)
{
	uint32_t i;

	if (ntv2_raster == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (ntv2_raster->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_VIDEORASTER_STATE("%s: enable video raster monitor\n", ntv2_raster->name);

    for (i = 0; i < ntv2_raster->num_widgets; i++)
    {
        ntv2_videoraster_initialize(ntv2_raster, i);
    }

	ntv2_raster->monitor_enable = true;

	return ntv2_videoraster_update_global(ntv2_raster, 0x12345678, 0x87654321);
}

Ntv2Status ntv2_videoraster_disable(struct ntv2_videoraster *ntv2_raster)
{
	if (ntv2_raster == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (!ntv2_raster->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_VIDEORASTER_STATE("%s: disable video raster monitor\n", ntv2_raster->name);

    ntv2InterruptLockAcquire(&ntv2_raster->state_lock);
	ntv2_raster->monitor_enable = false;
    ntv2InterruptLockRelease(&ntv2_raster->state_lock);

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_videoraster_update_global(struct ntv2_videoraster *ntv2_raster, uint32_t reg, uint32_t value)
{
    uint32_t i;

	if (ntv2_raster == NULL)
		return NTV2_STATUS_SUCCESS;
	if (!ntv2_raster->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_VIDEORASTER_STATE("%s: video raster update global  reg %d  value %08x", ntv2_raster->name, reg, value);

    for (i = 0; i < ntv2_raster->num_widgets; i++)
    {
        if (has_config_changed(ntv2_raster, i))
            update_format(ntv2_raster, i);
    }

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_videoraster_update_channel(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
	if (ntv2_raster == NULL)
		return NTV2_STATUS_SUCCESS;
	if (!ntv2_raster->monitor_enable)
		return NTV2_STATUS_SUCCESS;
	if (index >= ntv2_raster->num_widgets)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_VIDEORASTER_STATE("%s: video raster update channel\n", ntv2_raster->name);

	if (has_config_changed(ntv2_raster, index))
	{
		update_format(ntv2_raster, index);
	}
	
	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_videoraster_update_frame(struct ntv2_videoraster *ntv2_raster, uint32_t index, bool input, uint32_t frame_number)
{
    uint32_t base = 0;
    uint32_t global_control = 0;
    uint32_t standard = 0;
    uint32_t frame_size = 0;
    uint32_t frame_pitch = 0;
    uint32_t field1_address = 0;
    uint32_t field2_address = 0;
    uint32_t oddline_address = 0;
    bool progressive = false;
    bool top_first = false;
	bool quad = false;

    if (ntv2_raster == NULL)
    {
        return NTV2_STATUS_SUCCESS;
    }
	if (!ntv2_raster->monitor_enable)
		return NTV2_STATUS_SUCCESS;
    if (index >= ntv2_raster->num_widgets)
        return NTV2_STATUS_SUCCESS;
    if (input && (ntv2_raster->channel_mode[index] != ntv2_con_videoraster_mode_capture))
        return NTV2_STATUS_SUCCESS;
    if (!input && (ntv2_raster->channel_mode[index] != ntv2_con_videoraster_mode_display))
        return NTV2_STATUS_SUCCESS;

    ntv2InterruptLockAcquire(&ntv2_raster->state_lock);
    
    base = ntv2_raster->widget_base + index*ntv2_raster->widget_size;
    global_control = ntv2_raster->global_control[index];
    standard = NTV2_FLD_GET(ntv2_fld_global_control_standard, global_control);
    if (standard >= s_standard_size) return false;
	quad = NTV2_FLD_GET(ntv2_fld_global_control_quad_tsi_enable, global_control) != 0;
    progressive = c_standard_data[standard].video_scan == ntv2_video_scan_progressive;
    top_first = c_standard_data[standard].video_scan == ntv2_video_scan_top_first;

    frame_size = ntv2_raster->frame_size[index];
    frame_pitch = ntv2_raster->frame_pitch[index];

    if (input)
        ntv2_raster->input_frame[index] = frame_number;
    else
        ntv2_raster->output_frame[index] = frame_number;

    if (progressive)
    {
        field1_address = frame_number * frame_size;
        if (quad)
            oddline_address = frame_number * frame_size + frame_pitch;            
    }
    else if (top_first)
    {
        field1_address = frame_number * frame_size;
        field2_address = frame_number * frame_size + frame_pitch;
    }
    else
    {
        field1_address = frame_number * frame_size + frame_pitch;
        field2_address = frame_number * frame_size;
    }

    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_roifield1startaddress, field1_address);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_roifield2startaddress, field2_address);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_oddlinestartaddress, oddline_address);

    ntv2InterruptLockRelease(&ntv2_raster->state_lock);

    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f1 address        %08x\n", ntv2_raster->name, index, field1_address);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f2 address        %08x\n", ntv2_raster->name, index, field2_address);
    

   	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_videoraster_initialize(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
	NTV2_MSG_VIDEORASTER_STATE("%s: intialize channel data\n", ntv2_raster->name);

    ntv2_raster->global_control[index] = 0;
    ntv2_raster->global_control2[index] = 0;
    ntv2_raster->global_control3[index] = 0;
    ntv2_raster->channel_control[index] = 0;
    ntv2_raster->output_frame[index] = 0;
    ntv2_raster->input_frame[index] = 0;
    ntv2_raster->master_index[index] = 0;
    ntv2_raster->mode_change[index] = true;

    return NTV2_STATUS_SUCCESS;
}

static bool has_config_changed(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
    uint32_t global_control_value = 0;
    uint32_t global_control2_value = 0;
    uint32_t global_control3_value = 0;
    uint32_t channel_control_value = 0;
    uint32_t output_frame_value = 0;
    uint32_t input_frame_value = 0;
    bool ret = true;

	global_control_value = ntv2_reg_read(ntv2_raster->system_context, ntv2_reg_global_control, 0);
    global_control2_value = ntv2_reg_read(ntv2_raster->system_context, ntv2_reg_global_control2, 0);
	global_control3_value = ntv2_reg_read(ntv2_raster->system_context, ntv2_reg_global_control3, 0);

    /* find global control register value */
    if (NTV2_FLD_GET(ntv2_fld_global_control_independent_mode, global_control2_value))
    {
        global_control_value = ntv2_reg_read(ntv2_raster->system_context, ntv2_reg_global_control, index);
    }

    channel_control_value = ntv2_reg_read(ntv2_raster->system_context, ntv2_reg_channel_control, index);
    
    output_frame_value = ntv2_reg_read(ntv2_raster->system_context, ntv2_reg_channel_output_frame, index);
    input_frame_value = ntv2_reg_read(ntv2_raster->system_context, ntv2_reg_channel_input_frame, index);

    if ((global_control_value == ntv2_raster->global_control[index]) &&
        (global_control2_value == ntv2_raster->global_control2[index]) &&
        (global_control3_value == ntv2_raster->global_control3[index]) &&
        (channel_control_value == ntv2_raster->channel_control[index]) &&
        (output_frame_value == ntv2_raster->output_frame[index]) &&
        (input_frame_value == ntv2_raster->input_frame[index]))
        return false;

    ntv2_raster->mode_change[index] =
        (NTV2_FLD_GET(ntv2_fld_channel_control_capture_enable, channel_control_value) !=
            NTV2_FLD_GET(ntv2_fld_channel_control_capture_enable, ntv2_raster->channel_control[index])) ||
        (NTV2_FLD_GET(ntv2_fld_channel_control_channel_disable, channel_control_value) !=
            NTV2_FLD_GET(ntv2_fld_channel_control_channel_disable, ntv2_raster->channel_control[index]));

    ntv2_raster->global_control[index] = global_control_value;
    ntv2_raster->global_control2[index] = global_control2_value;
    ntv2_raster->global_control3[index] = global_control3_value;
    ntv2_raster->channel_control[index] = channel_control_value;
    ntv2_raster->output_frame[index] = output_frame_value;
    ntv2_raster->input_frame[index] = input_frame_value;

    if (ret)
	    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d raster data changed\n", ntv2_raster->name, index);

    return ret;
}

static bool update_format(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
    uint32_t base = ntv2_raster->widget_base + index*ntv2_raster->widget_size;
	uint32_t value = ntv2_regnum_read(ntv2_raster->system_context, base + ntv2_reg_videoraster_control);

	if (NTV2_FLD_GET(ntv2_fld_videoraster_control_disable_auto_update, value) != 0)
	{
	    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d auto update disabled\n", ntv2_raster->name, index);
		return true;
	}

    return update_format_single(ntv2_raster, index);
}

static bool update_format_single(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
    uint32_t base = ntv2_raster->widget_base + index*ntv2_raster->widget_size;
    uint32_t global_control = ntv2_raster->global_control[index];
//    uint32_t global_control2 = ntv2_raster->global_control2[index];
//    uint32_t global_control3 = ntv2_raster->global_control3[index];
    uint32_t channel_control = ntv2_raster->channel_control[index];
    uint32_t output_frame = ntv2_raster->output_frame[index];
    uint32_t input_frame = ntv2_raster->input_frame[index];
    uint32_t standard = 0;
    uint32_t geometry = 0;
    uint32_t format = 0;
    uint32_t frame_rate = 0;
    uint32_t pixel_rate = 0;
    uint32_t length = 0;
    uint32_t pitch = 0;
	uint32_t width = 0;
	uint32_t height = 0;
    uint32_t total_width = 0;
    uint32_t frame_size = 0;
    uint32_t frame_number = 0;
    uint32_t field1_address = 0;
    uint32_t field2_address = 0;
    uint32_t oddline_address = 0;
    uint32_t hoffset = 0;
    uint32_t voffset = 0;
    uint32_t fid_high = 0;
    uint32_t fid_low = 0;
    uint32_t mode = 0;
    uint32_t value = 0;
    uint32_t total_lines = 0;
    uint32_t register_sync = 0;
	uint32_t qRezMode = 0;
    uint32_t video_raster = 0;
    bool progressive = false;
    bool top_first = false;
	bool quad = false;
    char* channel_mode = "unknown";

    standard = NTV2_FLD_GET(ntv2_fld_global_control_standard, global_control);
    if (standard >= s_standard_size) return false;
    geometry = NTV2_FLD_GET(ntv2_fld_global_control_geometry, global_control);
    if (geometry >= s_geometry_size) return false;
	quad = NTV2_FLD_GET(ntv2_fld_global_control_quad_tsi_enable, global_control) != 0;
    format = NTV2_FLD_GET(ntv2_fld_channel_control_pixel_format, channel_control);
	format |= NTV2_FLD_GET(ntv2_fld_channel_control_pixel_format_high, channel_control) <<
		NTV2_FLD_SIZE(ntv2_fld_channel_control_pixel_format);;
    if (format >= s_format_size) return false;
    frame_rate = NTV2_FLD_GET(ntv2_fld_global_control_frame_rate, global_control);
    frame_rate |= NTV2_FLD_GET(ntv2_fld_global_control_frame_rate_high, global_control) <<
        NTV2_FLD_SIZE(ntv2_fld_global_control_frame_rate);
    if (frame_rate >= s_frame_rate_size) return false;
    pixel_rate = get_sdi_pixel_rate(standard, frame_rate);
    if (pixel_rate >= s_pixel_rate_size) return false;
    register_sync = NTV2_FLD_GET(ntv2_fld_global_control_reg_sync, global_control);

	qRezMode = NTV2_FLD_GET(ntv2_fld_channel_control_quarter_size_mode, channel_control);

    /* channel mode */
    mode = ntv2_con_videoraster_mode_disable;
    channel_mode = "disable";
    if (NTV2_FLD_GET(ntv2_fld_channel_control_channel_disable, channel_control) == 0)
    {
        mode = ntv2_con_videoraster_mode_capture;
        channel_mode = "capture";
        if (NTV2_FLD_GET(ntv2_fld_channel_control_capture_enable, channel_control) == 0)
        {
            mode = ntv2_con_videoraster_mode_display;
            channel_mode = "display";
        }
    }
    ntv2_raster->channel_mode[index] = mode;

    /* progressive / interlaced */
    progressive = c_standard_data[standard].video_scan == ntv2_video_scan_progressive;
    top_first = c_standard_data[standard].video_scan == ntv2_video_scan_top_first;
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  mode              %s\n", ntv2_raster->name, index, channel_mode);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  standard          %s\n", ntv2_raster->name, index, ntv2_video_standard_name(standard));
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  frame geometry    %s\n", ntv2_raster->name, index, ntv2_frame_geometry_name(geometry));
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  frame rate        %s\n", ntv2_raster->name, index, ntv2_frame_rate_name(frame_rate));
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  pixel format      %s\n", ntv2_raster->name, index, ntv2_pixel_format_name(format));

	/* frame width and height */
	width = c_geometry_data[geometry].frame_width;
	height = c_geometry_data[geometry].frame_height;

    /* frame size and number */
    frame_size = get_frame_size(ntv2_raster, index);
//	if (quad)
//		frame_size *= 4;

    if (mode == ntv2_con_videoraster_mode_capture)
        frame_number = input_frame;
    else if (mode == ntv2_con_videoraster_mode_display)
        frame_number = output_frame;
    else
        frame_number = 0;

    /* line length and pitch (bytes) */
    length = (width + c_format_data[format].pixels_cadence_width - 1) /
        c_format_data[format].pixels_cadence_width * c_format_data[format].bytes_cadence_width;
    pitch = (width + c_format_data[format].pixels_cadence_pitch - 1) /
        c_format_data[format].pixels_cadence_pitch * c_format_data[format].bytes_cadence_pitch;
	if (quad)
	{
		length *= 2;
		pitch *= 2;
	}

    ntv2InterruptLockAcquire(&ntv2_raster->state_lock);

    ntv2_raster->frame_size[index] = frame_size;
    ntv2_raster->frame_length[index] = length;
    ntv2_raster->frame_pitch[index] = pitch;

	/* frame buffer address and pitch correction */
    if (progressive)
    {
        field1_address = frame_number * frame_size;
        if (quad)
        {
            oddline_address = frame_number * frame_size + pitch;
            pitch *= 2;
        }
    }
    else if (top_first)
    {
        field1_address = frame_number * frame_size;
        field2_address = frame_number * frame_size + pitch;
        pitch *= 2;
    }
    else
    {
        field1_address = frame_number * frame_size + pitch;
        field2_address = frame_number * frame_size;
        pitch *= 2;
    }

    value = NTV2_FLD_SET(ntv2_fld_videoraster_linepitch_length, length);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_linepitch_pitch, pitch);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_linepitch, value);

    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_roifield1startaddress, field1_address);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_roifield2startaddress, field2_address);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_oddlinestartaddress, oddline_address);

    ntv2InterruptLockRelease(&ntv2_raster->state_lock);

    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  line length       %d\n", ntv2_raster->name, index, length);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  line pitch        %d\n", ntv2_raster->name, index, pitch);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f1 address        %08x\n", ntv2_raster->name, index, field1_address);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f2 address        %08x\n", ntv2_raster->name, index, field2_address);

	/* pixel dimensions (pixels) */
    value = NTV2_FLD_SET(ntv2_fld_videoraster_roisize_h, width);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_roisize_v, height);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_roisize, value);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  roi size hor      %d\n", ntv2_raster->name, index, width);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  roi size ver      %d\n", ntv2_raster->name, index, height);

    /* region of interest */
    hoffset = 0;
    voffset = 0;
    if (mode == ntv2_con_videoraster_mode_capture)
    {
        hoffset = (uint32_t)c_offset_data[standard].cap_roi_hoffset;
        voffset = (uint32_t)((int32_t)c_standard_data[standard].field1_active_line_start + c_offset_data[standard].cap_roi_voffset);
    }
    if (mode == ntv2_con_videoraster_mode_display)
    {
        hoffset = (uint32_t)c_offset_data[standard].dsp_roi_hoffset;
        voffset = (uint32_t)((int32_t)c_standard_data[standard].field1_active_line_start + c_offset_data[standard].dsp_roi_voffset);
    }
    value = NTV2_FLD_SET(ntv2_fld_videoraster_roifield1offset_h, hoffset);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_roifield1offset_v, voffset);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_roifield1offset, value);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f1 roi off hor    %d\n", ntv2_raster->name, index, hoffset);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f1 roi off ver    %d\n", ntv2_raster->name, index, voffset);

    hoffset = 0;
    voffset = 0;
    if (!progressive)
    {
        if (mode == ntv2_con_videoraster_mode_capture)
        {
            hoffset = (uint32_t)c_offset_data[standard].cap_roi_hoffset;
            voffset = (uint32_t)((int32_t)c_standard_data[standard].field2_active_line_start + c_offset_data[standard].cap_roi_voffset);
        }
        if (mode == ntv2_con_videoraster_mode_display)
        {
            hoffset = (uint32_t)c_offset_data[standard].dsp_roi_hoffset;
            voffset = (uint32_t)((int32_t)c_standard_data[standard].field2_active_line_start + c_offset_data[standard].dsp_roi_voffset);
        }
    }
    value = NTV2_FLD_SET(ntv2_fld_videoraster_roifield2offset_h, hoffset);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_roifield2offset_v, voffset);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_roifield2offset, value);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f2 roi off hor    %d\n", ntv2_raster->name, index, hoffset);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f2 roi off ver    %d\n", ntv2_raster->name, index, voffset);

    /* total lines */
    total_lines = c_standard_data[standard].line_total;

    /* calucate total width based on frame rate, pixel rate and total lines */
    total_width = (uint32_t)((c_pixel_rate_data[pixel_rate].scale * c_frame_rate_data[frame_rate].duration) /
        (c_pixel_rate_data[pixel_rate].duration * c_frame_rate_data[frame_rate].scale * total_lines));

    value = NTV2_FLD_SET(ntv2_fld_videoraster_videoh_active, width);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_videoh_total, total_width);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_videoh, value);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  h active          %d\n", ntv2_raster->name, index, width);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  h total           %d\n", ntv2_raster->name, index, total_width);

    /* fid lines with offsets */
    fid_high = 0;
    fid_low = 0;
    if (!progressive)
    {
        int32_t line;
        if (mode == ntv2_con_videoraster_mode_capture)
            line = c_standard_data[standard].fid_line_high + c_offset_data[standard].cap_fid_offset;
        else
            line = c_standard_data[standard].fid_line_high + c_offset_data[standard].dsp_fid_offset;
        if (line < 1)
            line += total_lines;
        if (line > (int32_t)total_lines)
            line -= total_lines;
        fid_high = (uint32_t)line;
        if (mode == ntv2_con_videoraster_mode_capture)
            line = c_standard_data[standard].fid_line_low + c_offset_data[standard].cap_fid_offset;
        else
            line = c_standard_data[standard].fid_line_low + c_offset_data[standard].dsp_fid_offset;
        if (line < 1)
            line += total_lines;
        if (line > (int32_t)total_lines)
            line -= total_lines;
        fid_low = (uint32_t)line;
    }
    value = NTV2_FLD_SET(ntv2_fld_videoraster_videofid_high, fid_high);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_videofid_low, fid_low);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_videofid, value);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  fid high          %d\n", ntv2_raster->name, index, fid_high);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  fid low           %d\n", ntv2_raster->name, index, fid_low);

    value = NTV2_FLD_SET(ntv2_fld_videoraster_field1active_start, c_standard_data[standard].field1_active_line_start);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_field1active_end, c_standard_data[standard].field1_active_line_end);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_field1active, value);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f1 active start   %d\n", ntv2_raster->name, index, c_standard_data[standard].field1_active_line_start);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f1 active end     %d\n", ntv2_raster->name, index, c_standard_data[standard].field1_active_line_end);

    value = 0;
    if (!progressive)
    {
        value = NTV2_FLD_SET(ntv2_fld_videoraster_field2active_start, c_standard_data[standard].field2_active_line_start);
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_field2active_end, c_standard_data[standard].field2_active_line_end);
	    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f2 active start   %d\n", ntv2_raster->name, index, c_standard_data[standard].field2_active_line_start);
		NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f2 active end     %d\n", ntv2_raster->name, index, c_standard_data[standard].field2_active_line_end);
    }
	else
	{
	    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f2 active start   0\n", ntv2_raster->name, index);
		NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  f2 active end     0\n", ntv2_raster->name, index);
	}
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_field2active, value);

    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_vtotal, total_lines);
    NTV2_MSG_VIDEORASTER_STATE("%s: chn %d  total lines       %d\n", ntv2_raster->name, index, total_lines);

    /* raster control */
    value = NTV2_FLD_SET(ntv2_fld_videoraster_control_mode, mode);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_double, qRezMode);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_fillbit, 0);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_dither, 0);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_8b10bconvert, 0);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_progressive, (progressive ? 1 : 0));
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_twosampleinterleave, (quad? 1 : 0));
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_imageformat, format);

    switch (pixel_rate)
    {
    case ntv2_pixel_rate_1350:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_pixelclock, ntv2_con_videoraster_pixelclock_2700);
        break;
    case ntv2_pixel_rate_2700:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_pixelclock, ntv2_con_videoraster_pixelclock_2700);
        break;
    case ntv2_pixel_rate_7418:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_pixelclock, ntv2_con_videoraster_pixelclock_7418);
        break;
    case ntv2_pixel_rate_7425:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_pixelclock, ntv2_con_videoraster_pixelclock_7425);
        break;
    case ntv2_pixel_rate_14835:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_pixelclock, ntv2_con_videoraster_pixelclock_14835);
        break;
    case ntv2_pixel_rate_14850:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_pixelclock, ntv2_con_videoraster_pixelclock_14850);
    default:
        break;
    }

    switch (register_sync)
    {
    case ntv2_con_reg_sync_field:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_regsync, ntv2_con_videoraster_regsync_field);
        break;
    case ntv2_con_reg_sync_frame:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_regsync, ntv2_con_videoraster_regsync_frame);
        break;
    default:
    case ntv2_con_reg_sync_immediate:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_regsync, ntv2_con_videoraster_regsync_immediate);
        break;
    case ntv2_con_reg_sync_field_10_lines:
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_regsync, ntv2_con_videoraster_regsync_external);
        break;
    }

    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_control, value);
    video_raster = value;

    /* extra parameters */
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_pixelskip, 0);

    value = NTV2_FLD_SET(ntv2_fld_videoraster_videofilla_blue, 0x8000);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_videofilla_green, 0x1000);
    if (c_format_data[format].color_space == ntv2_color_space_rgb444)
    {
        value = NTV2_FLD_SET(ntv2_fld_videoraster_videofilla_blue, 0x0000);
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_videofilla_green, 0x0000);
    }
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_videofilla, value);

    value = NTV2_FLD_SET(ntv2_fld_videoraster_videofillb_red, 0x8000);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_videofillb_alpha, 0x1000);
    if (c_format_data[format].color_space == ntv2_color_space_rgb444)
    {
        value = NTV2_FLD_SET(ntv2_fld_videoraster_videofillb_red, 0x0000);
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_videofillb_alpha, 0x0000);
    }
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_videofillb, value);

    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_roifillalpha, 0xffff);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_timingpreset, 0);

    value = NTV2_FLD_SET(ntv2_fld_videoraster_smpteframepulse_pixelnumber, 1);
    value |= NTV2_FLD_SET(ntv2_fld_videoraster_smpteframepulse_linenumber, 1);
    ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_smpteframepulse, value);

    if (ntv2_raster->mode_change[index])
    {
        /* toggle register sync immediate when changing between capture and playback*/
        value = video_raster & ~NTV2_FLD_MASK(ntv2_fld_videoraster_control_regsync);
        value |= NTV2_FLD_SET(ntv2_fld_videoraster_control_regsync, ntv2_con_videoraster_regsync_immediate);
        ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_control, value);
        ntv2_regnum_write(ntv2_raster->system_context, base + ntv2_reg_videoraster_control, video_raster);
        ntv2_raster->mode_change[index] = false;
    }

	NTV2_MSG_VIDEORASTER_STATE("%s: chn %d raster registers updated\n", ntv2_raster->name, index);

    return true;
}

static uint32_t get_frame_size(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
    return GetFrameBufferSize(ntv2_raster->system_context, (NTV2Channel)index);
}

#if 0
static bool is_4k_sqd_mode(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
    uint32_t global_control2 = ntv2_raster->global_control2[index];
    bool quad_mode = false;

    switch (index)
    {
    case 0:
    case 1:
    case 2:
    case 3:
        if (NTV2_FLD_GET(ntv2_fld_global_control_quad_mode, global_control2))
            quad_mode = true;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        if (NTV2_FLD_GET(ntv2_fld_global_control_quad_mode2, global_control2))
            quad_mode = true;
    default:
        break;
    }

    return quad_mode;
}

static bool is_4k_tsi_mode(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
    uint32_t global_control2 = ntv2_raster->global_control2[index];
    bool quad_mode = false;

    switch (index)
    {
    case 0:
    case 1:
        if (NTV2_FLD_GET(ntv2_fld_global_control_425_fb12, global_control2))
           quad_mode = true;
        break;
    case 2:
    case 3:
        if (NTV2_FLD_GET(ntv2_fld_global_control_425_fb34, global_control2))
            quad_mode = true;
        break;
    case 4:
    case 5:
        if (NTV2_FLD_GET(ntv2_fld_global_control_425_fb56, global_control2))
            quad_mode = true;
        break;
    case 6:
    case 7:
        if (NTV2_FLD_GET(ntv2_fld_global_control_425_fb78, global_control2))
            quad_mode = true;
        break;
    default:
        break;
    }

    return quad_mode;
}

static bool is_8k_sqd_mode(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
    uint32_t global_control3 = ntv2_raster->global_control3[index];
    bool quad_quad_mode = false;

    if (NTV2_FLD_GET(ntv2_fld_global_control_quad_quad_squares_mode, global_control3))
        quad_quad_mode = true;

    return quad_quad_mode;
}

static bool is_8k_tsi_mode(struct ntv2_videoraster *ntv2_raster, uint32_t index)
{
    uint32_t global_control3 = ntv2_raster->global_control3[index];
    bool quad_quad_mode = false;

    switch (index)
    {
    case 0:
    case 1:
        if (NTV2_FLD_GET(ntv2_fld_global_control_quad_quad_mode, global_control3))
            quad_quad_mode = true;
        break;
    case 2:
    case 3:
        if (NTV2_FLD_GET(ntv2_fld_global_control_quad_quad_mode2, global_control3))
            quad_quad_mode = true;
        break;
    default:
        break;
    }

    return quad_quad_mode;
}
#endif
static uint32_t get_sdi_pixel_rate(uint32_t video_standard, uint32_t frame_rate)
{
    uint32_t pixel_rate = ntv2_pixel_rate_none;
    
    switch(video_standard)
    {
    case ntv2_video_standard_525i:
    case ntv2_video_standard_625i:
        pixel_rate = ntv2_pixel_rate_1350;
        break;
    case ntv2_video_standard_720p:
    case ntv2_video_standard_1080i:
    case ntv2_video_standard_2048x1080i:
    case ntv2_video_standard_2048x1556:
    case ntv2_video_standard_3840i:
    case ntv2_video_standard_4096i:
        switch(frame_rate)
        {
        case ntv2_frame_rate_2398:
        case ntv2_frame_rate_2997:
        case ntv2_frame_rate_4795:
        case ntv2_frame_rate_5994:
            pixel_rate = ntv2_pixel_rate_7418;
            break;
        default:
            pixel_rate = ntv2_pixel_rate_7425;
            break;
        }
        break;
    case ntv2_video_standard_1080p:
    case ntv2_video_standard_2048x1080p:
    case ntv2_video_standard_3840x2160p:
    case ntv2_video_standard_4096x2160p:
    case ntv2_video_standard_3840_hfr:
    case ntv2_video_standard_4096_hfr:
    case ntv2_video_standard_7680:
    case ntv2_video_standard_8192:
        switch(frame_rate)
        {
        case ntv2_frame_rate_2398:
        case ntv2_frame_rate_2997:
            pixel_rate = ntv2_pixel_rate_7418;
            break;
        case ntv2_frame_rate_2400:
        case ntv2_frame_rate_2500:
        case ntv2_frame_rate_3000:
            pixel_rate = ntv2_pixel_rate_7425;
            break;
        case ntv2_frame_rate_4795:
        case ntv2_frame_rate_5994:
            pixel_rate = ntv2_pixel_rate_14835;
            break;
        default:
            pixel_rate = ntv2_pixel_rate_14850;
            break;
        }
        break;
    }

    return pixel_rate;
}

