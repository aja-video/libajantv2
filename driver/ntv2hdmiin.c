/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA OEM boards.
//
// Filename: ntv2hdmiin.c
// Purpose:	 HDMI input monitor
//
///////////////////////////////////////////////////////////////

#include "ntv2hdmiin.h"
#include "ntv2hinreg.h"

/* debug messages */
#define NTV2_DEBUG_INFO					0x00000001
#define NTV2_DEBUG_ERROR				0x00000002
#define NTV2_DEBUG_HDMIIN_STATE			0x00000004
#define NTV2_DEBUG_KONAI2C_READ			0x00000010
#define NTV2_DEBUG_KONAI2C_WRITE		0x00000020

#define NTV2_DEBUG_ACTIVE(msg_mask) \
	(((ntv2_debug_mask | ntv2_user_mask) & msg_mask) != 0)

#define NTV2_MSG_PRINT(msg_mask, string, ...) \
	if(NTV2_DEBUG_ACTIVE(msg_mask)) ntv2Message(string, __VA_ARGS__);

#define NTV2_MSG_INFO(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_ERROR(string, ...)					NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_HDMIIN_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_HDMIIN_ERROR(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_HDMIIN_STATE(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_HDMIIN_STATE, string, __VA_ARGS__)
#define NTV2_MSG_KONAI2C_INFO(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_INFO, string, __VA_ARGS__)
#define NTV2_MSG_KONAI2C_ERROR(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_ERROR, string, __VA_ARGS__)
#define NTV2_MSG_KONAI2C_READ(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_KONAI2C_READ, string, __VA_ARGS__)
#define NTV2_MSG_KONAI2C_WRITE(string, ...)			NTV2_MSG_PRINT(NTV2_DEBUG_KONAI2C_WRITE, string, __VA_ARGS__)

static uint32_t ntv2_debug_mask = NTV2_DEBUG_INFO | NTV2_DEBUG_ERROR | NTV2_DEBUG_HDMIIN_STATE;
static uint32_t ntv2_user_mask = 0;


/* 
   Bits to flag reporting of measurements. These are all set in mRelockReports whenever
   the TMDS clock has lost lock and it is necessary to re-sync everything. They get
   cleared by the individual cogs of the state machine as details of the format 
   are determined.
*/
#define NTV2_REPORT_CABLE	0x0001
#define NTV2_REPORT_SYNC	0x0002
#define NTV2_REPORT_FREQ	0x0004
#define NTV2_REPORT_INFO	0x0008
#define NTV2_REPORT_TIMING	0x0010
#define NTV2_REPORT_FORMAT	0x0020

#define NTV2_REPORT_ANY		0xffff

#define NTV2_INIT_WAIT				100000
#define NTV2_UPDATE_WAIT			250000
#define NTV2_DEAD_COUNT				10

#define NTV2_I2C_BUSY_TIMEOUT		10000
#define NTV2_I2C_WRITE_TIMEOUT		2000
#define NTV2_I2C_WAIT_TIME			500
#define NTV2_I2C_READ_TIMEOUT		200000
#define NTV2_I2C_READ_TIME			5000
#define NTV2_I2C_RESET_TIME			1000

/*
  At or below this clock frequency, the doubler will be turned on.
  This value would be 27.5MHz if not for deep color, which maintains pixel 
  doubling up to 40.5MHz in 576p. 41MHz is used because a tolerance of .5%
  above standard is required for certification (kHz).
*/
#define TMDS_DOUBLING_FREQ			41000
/* high frequency clock phase adjustment (degrees?)*/
#define CLOCK_PHASE_HF  			18

/* vic mapping */
struct ntv2_video_code_info {
	uint32_t video_standard;
	uint32_t frame_rate;
};

#define NTV2_AVI_VIC_INFO_SIZE			120
static struct ntv2_video_code_info ntv2_avi_vic_info[NTV2_AVI_VIC_INFO_SIZE];

#define NTV2_VSI_VIC_INFO_SIZE			8
static struct ntv2_video_code_info ntv2_vsi_vic_info[NTV2_VSI_VIC_INFO_SIZE];

#define NTV2_MAX_FRAME_RATES		16
#define NTV2_MAX_VIDEO_STANDARDS	16

static uint32_t video_standard_to_hdmi[NTV2_MAX_VIDEO_STANDARDS];
static uint32_t frame_rate_to_hdmi[NTV2_MAX_FRAME_RATES];

static Ntv2Status ntv2_hdmiin_periodic_update(struct ntv2_hdmiin *ntv2_hin);
static void ntv2_hdmiin_monitor(void* data);
static Ntv2Status ntv2_hdmiin_write_multi(struct ntv2_hdmiin *ntv2_hin,
										  uint8_t device,
										  struct ntv2_reg_value *reg_value,
										  int count);
static Ntv2Status ntv2_hdmiin_read_verify(struct ntv2_hdmiin *ntv2_hin,
										  uint8_t device,
										  struct ntv2_reg_value *reg_value,
										  int count);
static Ntv2Status ntv2_hdmiin_write_edid(struct ntv2_hdmiin *ntv2_hin,
										 struct ntv2_hdmiedid *ntv2_edid,
										 uint8_t device);
static Ntv2Status ntv2_hdmiin_initialize(struct ntv2_hdmiin *ntv2_hin);
static void ntv2_hdmiin_hot_plug(struct ntv2_hdmiin *ntv2_hin);
static Ntv2Status ntv2_hdmiin_set_color_mode(struct ntv2_hdmiin *ntv2_hin, bool yuv_input, bool yuv_output);
static Ntv2Status ntv2_hdmiin_set_uhd_mode(struct ntv2_hdmiin *ntv2_hin, bool enable);
static Ntv2Status ntv2_hdmi_set_derep_mode(struct ntv2_hdmiin *ntv2_hin, bool enable);
static void ntv2_hdmiin_update_tmds_freq(struct ntv2_hdmiin *ntv2_hin);
static void ntv2_hdmiin_config_pixel_clock(struct ntv2_hdmiin *ntv2_hin);
static uint32_t ntv2_hdmiin_read_paired_value(struct ntv2_hdmiin *ntv2_hin, uint8_t reg, uint32_t bits, uint32_t shift);
static void ntv2_hdmiin_update_timing(struct ntv2_hdmiin *ntv2_hin);
static void ntv2_hdmiin_find_dvi_format(struct ntv2_hdmiin *ntv2_hin,
										struct ntv2_hdmiin_format *format);
static void ntv2_hdmiin_find_hdmi_format(struct ntv2_hdmiin *ntv2_hin,
										 struct ntv2_hdmiin_format *format);
static uint32_t ntv2_hdmiin_pixel_double(struct ntv2_hdmiin *ntv2_hin, uint32_t pixels);
static Ntv2Status ntv2_hdmiin_set_video_format(struct ntv2_hdmiin *ntv2_hin,
											   struct ntv2_hdmiin_format *format);
static void ntv2_hdmiin_set_aux_data(struct ntv2_hdmiin *ntv2_hin,
									 struct ntv2_hdmiin_format *format);
static void ntv2_hdmiin_set_no_video(struct ntv2_hdmiin *ntv2_hin);

static void ntv2_konai2c_set_device(struct ntv2_hdmiin *ntv2_hin, uint8_t device);
//static uint8_t ntv2_konai2c_get_device(struct ntv2_hdmiin *ntv2_hin);
static Ntv2Status ntv2_konai2c_write(struct ntv2_hdmiin *ntv2_hin, uint8_t address, uint8_t data);
static Ntv2Status ntv2_konai2c_cache_update(struct ntv2_hdmiin *ntv2_hin);
static uint8_t ntv2_konai2c_cache_read(struct ntv2_hdmiin *ntv2_hin, uint8_t address);
//static Ntv2Status ntv2_konai2c_rmw(struct ntv2_hdmiin *ntv2_hin, uint8_t address, uint8_t data, uint8_t mask);
static Ntv2Status ntv2_konai2c_wait_for_busy(struct ntv2_hdmiin *ntv2_hin, uint32_t timeout);
static Ntv2Status ntv2_konai2c_wait_for_write(struct ntv2_hdmiin *ntv2_hin, uint32_t timeout);
static Ntv2Status ntv2_konai2c_wait_for_read(struct ntv2_hdmiin *ntv2_hin, uint32_t timeout);
static void ntv2_konai2c_reset(struct ntv2_hdmiin *ntv2_hin);
static void ntv2_update_debug_flags(struct ntv2_hdmiin *ntv2_hin);

static uint32_t ntv2_video_standard_to_hdmiin(uint32_t video_standard);
static uint32_t ntv2_frame_rate_to_hdmiin(uint32_t frame_rate);
static void ntv2_video_format_init(struct ntv2_hdmiin_format *format);
static bool ntv2_video_format_compare(struct ntv2_hdmiin_format *format_a,
									  struct ntv2_hdmiin_format *format_b);


struct ntv2_hdmiin *ntv2_hdmiin_open(Ntv2SystemContext* sys_con,
									 const char *name, int index)
{
	struct ntv2_hdmiin *ntv2_hin = NULL;
	int i;

	if ((sys_con == NULL) ||
		(name == NULL))
		return NULL;

	ntv2_hin = (struct ntv2_hdmiin *)ntv2MemoryAlloc(sizeof(struct ntv2_hdmiin));
	if (ntv2_hin == NULL) {
		NTV2_MSG_ERROR("%s: ntv2_hdmiin instance memory allocation failed\n", name);
		return NULL;
	}
	memset(ntv2_hin, 0, sizeof(struct ntv2_hdmiin));

	ntv2_hin->index = index;
#if defined(MSWindows)
	sprintf(ntv2_hin->name, "%s%d", name, index);
#else
	snprintf(ntv2_hin->name, NTV2_STRING_SIZE, "%s%d", name, index);
#endif
	ntv2_hin->system_context = sys_con;

	ntv2SpinLockOpen(&ntv2_hin->state_lock, sys_con);
	ntv2ThreadOpen(&ntv2_hin->monitor_task, sys_con, "hdmi input monitor");
	ntv2EventOpen(&ntv2_hin->monitor_event, sys_con);

	/* initialize periodic update state */
	ntv2_hin->relock_reports = 0;
	ntv2_hin->hdmi_mode = false;
	ntv2_hin->hdcp_mode = false;
	ntv2_hin->derep_mode = false;;
	ntv2_hin->uhd_mode = false;
	ntv2_hin->cable_present = false;
	ntv2_hin->clock_present = false;
	ntv2_hin->input_locked = false;
	ntv2_hin->pixel_double_mode = false;
	ntv2_hin->avi_packet_present = false;
	ntv2_hin->vsi_packet_present = false;
	ntv2_hin->interlaced_mode = false;
	ntv2_hin->deep_color_10bit = false;
	ntv2_hin->deep_color_12bit = false;
	ntv2_hin->yuv_mode = false;
	ntv2_hin->prefer_yuv = false;
	ntv2_hin->prefer_rgb = false;
	ntv2_hin->relock_reports = NTV2_REPORT_ANY;
	ntv2_hin->lock_error_count = 0;
	ntv2_hin->horizontal_tol = 10;
	ntv2_hin->vertical_tol = 4;

	/* initialize hdmi avi vic to ntv2 standard and rate table */
	for (i = 0; i < NTV2_AVI_VIC_INFO_SIZE; i++) {
		ntv2_avi_vic_info[i].video_standard = ntv2_video_standard_none;
		ntv2_avi_vic_info[i].frame_rate = ntv2_frame_rate_none;
	}

	ntv2_avi_vic_info[4].video_standard = ntv2_video_standard_720p;
	ntv2_avi_vic_info[4].frame_rate = ntv2_frame_rate_6000;
	ntv2_avi_vic_info[5].video_standard = ntv2_video_standard_1080i;
	ntv2_avi_vic_info[5].frame_rate = ntv2_frame_rate_3000;
	ntv2_avi_vic_info[6].video_standard = ntv2_video_standard_525i;
	ntv2_avi_vic_info[6].frame_rate = ntv2_frame_rate_3000;
	ntv2_avi_vic_info[7].video_standard = ntv2_video_standard_525i;
	ntv2_avi_vic_info[7].frame_rate = ntv2_frame_rate_3000;
	ntv2_avi_vic_info[16].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[16].frame_rate = ntv2_frame_rate_6000;
	ntv2_avi_vic_info[19].video_standard = ntv2_video_standard_720p;
	ntv2_avi_vic_info[19].frame_rate = ntv2_frame_rate_5000;
	ntv2_avi_vic_info[20].video_standard = ntv2_video_standard_1080i;
	ntv2_avi_vic_info[20].frame_rate = ntv2_frame_rate_2500;
	ntv2_avi_vic_info[21].video_standard = ntv2_video_standard_625i;
	ntv2_avi_vic_info[21].frame_rate = ntv2_frame_rate_2500;
	ntv2_avi_vic_info[22].video_standard = ntv2_video_standard_625i;
	ntv2_avi_vic_info[22].frame_rate = ntv2_frame_rate_2500;
	ntv2_avi_vic_info[31].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[31].frame_rate = ntv2_frame_rate_5000;
	ntv2_avi_vic_info[32].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[32].frame_rate = ntv2_frame_rate_2400;
	ntv2_avi_vic_info[33].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[33].frame_rate = ntv2_frame_rate_2500;
	ntv2_avi_vic_info[34].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[34].frame_rate = ntv2_frame_rate_3000;
	ntv2_avi_vic_info[68].video_standard = ntv2_video_standard_720p;
	ntv2_avi_vic_info[68].frame_rate = ntv2_frame_rate_5000;
	ntv2_avi_vic_info[69].video_standard = ntv2_video_standard_720p;
	ntv2_avi_vic_info[69].frame_rate = ntv2_frame_rate_6000;
	ntv2_avi_vic_info[72].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[72].frame_rate = ntv2_frame_rate_2400;
	ntv2_avi_vic_info[73].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[73].frame_rate = ntv2_frame_rate_2500;
	ntv2_avi_vic_info[74].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[74].frame_rate = ntv2_frame_rate_3000;
	ntv2_avi_vic_info[75].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[75].frame_rate = ntv2_frame_rate_5000;
	ntv2_avi_vic_info[76].video_standard = ntv2_video_standard_1080p;
	ntv2_avi_vic_info[76].frame_rate = ntv2_frame_rate_6000;
	ntv2_avi_vic_info[93].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[93].frame_rate = ntv2_frame_rate_2400;
	ntv2_avi_vic_info[94].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[94].frame_rate = ntv2_frame_rate_2500;
	ntv2_avi_vic_info[95].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[95].frame_rate = ntv2_frame_rate_3000;
	ntv2_avi_vic_info[96].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[96].frame_rate = ntv2_frame_rate_5000;
	ntv2_avi_vic_info[97].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[97].frame_rate = ntv2_frame_rate_6000;
	ntv2_avi_vic_info[98].video_standard = ntv2_video_standard_4096x2160p;
	ntv2_avi_vic_info[98].frame_rate = ntv2_frame_rate_2400;
	ntv2_avi_vic_info[99].video_standard = ntv2_video_standard_4096x2160p;
	ntv2_avi_vic_info[99].frame_rate = ntv2_frame_rate_2500;
	ntv2_avi_vic_info[100].video_standard = ntv2_video_standard_4096x2160p;
	ntv2_avi_vic_info[100].frame_rate = ntv2_frame_rate_3000;
	ntv2_avi_vic_info[101].video_standard = ntv2_video_standard_4096x2160p;
	ntv2_avi_vic_info[101].frame_rate = ntv2_frame_rate_5000;
	ntv2_avi_vic_info[102].video_standard = ntv2_video_standard_4096x2160p;
	ntv2_avi_vic_info[102].frame_rate = ntv2_frame_rate_6000;
	ntv2_avi_vic_info[103].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[103].frame_rate = ntv2_frame_rate_2400;
	ntv2_avi_vic_info[104].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[104].frame_rate = ntv2_frame_rate_2500;
	ntv2_avi_vic_info[105].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[105].frame_rate = ntv2_frame_rate_3000;
	ntv2_avi_vic_info[106].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[106].frame_rate = ntv2_frame_rate_5000;
	ntv2_avi_vic_info[107].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_avi_vic_info[107].frame_rate = ntv2_frame_rate_6000;

	/* initialize hdmi vsi vic to ntv2 standard and rate table */
	for (i = 0; i < NTV2_VSI_VIC_INFO_SIZE; i++) {
		ntv2_vsi_vic_info[i].video_standard = ntv2_video_standard_none;
		ntv2_vsi_vic_info[i].frame_rate = ntv2_frame_rate_none;
	}

	ntv2_vsi_vic_info[1].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_vsi_vic_info[1].frame_rate = ntv2_frame_rate_3000;
	ntv2_vsi_vic_info[2].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_vsi_vic_info[2].frame_rate = ntv2_frame_rate_2500;
	ntv2_vsi_vic_info[3].video_standard = ntv2_video_standard_3840x2160p;
	ntv2_vsi_vic_info[3].frame_rate = ntv2_frame_rate_2400;
	ntv2_vsi_vic_info[4].video_standard = ntv2_video_standard_4096x2160p;
	ntv2_vsi_vic_info[4].frame_rate = ntv2_frame_rate_2400;

	/* ntv2 video standard to hdmi video standard */
	for (i = 0; i < NTV2_MAX_VIDEO_STANDARDS; i++) {
		video_standard_to_hdmi[i] = ntv2_kona_hdmiin_video_standard_none;
	}
	video_standard_to_hdmi[ntv2_video_standard_1080i] = ntv2_kona_hdmiin_video_standard_1080i;
	video_standard_to_hdmi[ntv2_video_standard_720p] = ntv2_kona_hdmiin_video_standard_720p;
	video_standard_to_hdmi[ntv2_video_standard_525i] = ntv2_kona_hdmiin_video_standard_525i;
	video_standard_to_hdmi[ntv2_video_standard_625i] = ntv2_kona_hdmiin_video_standard_625i;
	video_standard_to_hdmi[ntv2_video_standard_1080p] = ntv2_kona_hdmiin_video_standard_1080p;
	video_standard_to_hdmi[ntv2_video_standard_2048x1080i] = ntv2_kona_hdmiin_video_standard_1080i;
	video_standard_to_hdmi[ntv2_video_standard_2048x1080p] = ntv2_kona_hdmiin_video_standard_1080p;
	video_standard_to_hdmi[ntv2_video_standard_3840x2160p] = ntv2_kona_hdmiin_video_standard_4k;
	video_standard_to_hdmi[ntv2_video_standard_4096x2160p] = ntv2_kona_hdmiin_video_standard_4k;

	/* ntv2 frame rate to hdmi frame rate */
	for (i = 0; i < NTV2_MAX_FRAME_RATES; i++) {
		frame_rate_to_hdmi[i] = ntv2_kona_hdmiin_frame_rate_none;
	}
	frame_rate_to_hdmi[ntv2_frame_rate_6000] = ntv2_kona_hdmiin_frame_rate_6000;
	frame_rate_to_hdmi[ntv2_frame_rate_5994] = ntv2_kona_hdmiin_frame_rate_5994;
	frame_rate_to_hdmi[ntv2_frame_rate_3000] = ntv2_kona_hdmiin_frame_rate_3000;
	frame_rate_to_hdmi[ntv2_frame_rate_2997] = ntv2_kona_hdmiin_frame_rate_2997;
	frame_rate_to_hdmi[ntv2_frame_rate_2500] = ntv2_kona_hdmiin_frame_rate_2500;
	frame_rate_to_hdmi[ntv2_frame_rate_2400] = ntv2_kona_hdmiin_frame_rate_2400;
	frame_rate_to_hdmi[ntv2_frame_rate_2398] = ntv2_kona_hdmiin_frame_rate_2398;
	frame_rate_to_hdmi[ntv2_frame_rate_5000] = ntv2_kona_hdmiin_frame_rate_5000;

	NTV2_MSG_HDMIIN_INFO("%s: open ntv2_hdmiin\n", ntv2_hin->name);

	return ntv2_hin;
}

void ntv2_hdmiin_close(struct ntv2_hdmiin *ntv2_hin)
{
	if (ntv2_hin == NULL) 
		return;

	NTV2_MSG_HDMIIN_INFO("%s: close ntv2_hdmiin\n", ntv2_hin->name);

	ntv2_hdmiin_disable(ntv2_hin);

	ntv2EventClose(&ntv2_hin->monitor_event);
	ntv2ThreadClose(&ntv2_hin->monitor_task);
	ntv2SpinLockClose(&ntv2_hin->state_lock);

	ntv2_hdmiedid_close(ntv2_hin->edid);

	memset(ntv2_hin, 0, sizeof(struct ntv2_hdmiin));
	ntv2MemoryFree(ntv2_hin, sizeof(struct ntv2_hdmiin));
}

Ntv2Status ntv2_hdmiin_configure(struct ntv2_hdmiin *ntv2_hin,
								 enum ntv2_edid_type edid_type, int port_index)
{
	Ntv2Status result = NTV2_STATUS_SUCCESS;

	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_HDMIIN_INFO("%s: configure hdmi input device\n", ntv2_hin->name);

	/* configure edid */
	if (edid_type != ntv2_edid_type_unknown) {
		ntv2_hin->edid = ntv2_hdmiedid_open(ntv2_hin->system_context, "edid", 0); 
		if (ntv2_hin->edid != NULL) {
			result = ntv2_hdmiedid_configure(ntv2_hin->edid, edid_type, port_index);
			if (result != NTV2_STATUS_SUCCESS) {
				ntv2_hdmiedid_close(ntv2_hin->edid);
				ntv2_hin->edid = NULL;
				NTV2_MSG_HDMIIN_ERROR("%s: *error* configure edid failed\n", ntv2_hin->name);
			}
		} else {
			NTV2_MSG_HDMIIN_ERROR("%s: *error* open edid failed\n", ntv2_hin->name);
		}
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiin_enable(struct ntv2_hdmiin *ntv2_hin)
{
	bool success ;

	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (ntv2_hin->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_HDMIIN_STATE("%s: enable hdmi input monitor\n", ntv2_hin->name);

	ntv2EventClear(&ntv2_hin->monitor_event);
	ntv2_hin->monitor_enable = true;

	success = ntv2ThreadRun(&ntv2_hin->monitor_task, ntv2_hdmiin_monitor, (void*)ntv2_hin);
	if (!success) {
		return NTV2_STATUS_FAIL;
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiin_disable(struct ntv2_hdmiin *ntv2_hin)
{
	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	if (!ntv2_hin->monitor_enable)
		return NTV2_STATUS_SUCCESS;

	NTV2_MSG_HDMIIN_STATE("%s: disable hdmi input monitor\n", ntv2_hin->name);

	ntv2_hin->monitor_enable = false;
	ntv2EventSignal(&ntv2_hin->monitor_event);

	ntv2ThreadStop(&ntv2_hin->monitor_task);

	return NTV2_STATUS_SUCCESS;
}

static void ntv2_hdmiin_monitor(void* data)
{
	struct ntv2_hdmiin *ntv2_hin = (struct ntv2_hdmiin *)data;
	int init = true;
	int res = 0;
	int count = 0;
	uint32_t val;

	if (ntv2_hin == NULL)
		return;

	NTV2_MSG_HDMIIN_STATE("%s: hdmi input monitor task start\n", ntv2_hin->name);

	while (!ntv2ThreadShouldStop(&ntv2_hin->monitor_task) && ntv2_hin->monitor_enable)
	{
		if (init)
		{
			res = ntv2_hdmiin_initialize(ntv2_hin);
			if (res < 0)
			{
				count++;
				if (count > NTV2_DEAD_COUNT)
				{
					NTV2_MSG_HDMIIN_ERROR("%s: hdmi input monitor task cannot initialize hardware\n", ntv2_hin->name);
					ntv2_hin->monitor_enable = false;
					break;
				}
			}
			else
			{
				init = false;
				count = 0;
			}
			ntv2EventWaitForSignal(&ntv2_hin->monitor_event, NTV2_INIT_WAIT, true);
			continue;
		}

		
		val = ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmi_control, 0);
		if ((val & NTV2_FLD_MASK(ntv2_kona_fld_hdmi_disable_update)) == 0)
		{
			res = ntv2_hdmiin_periodic_update(ntv2_hin);
			if (res < 0)
				init = true;
		}

		ntv2EventWaitForSignal(&ntv2_hin->monitor_event, NTV2_UPDATE_WAIT, true);
	}

	NTV2_MSG_HDMIIN_STATE("%s: hdmi input monitor task stop\n", ntv2_hin->name);
	ntv2ThreadExit(&ntv2_hin->monitor_task);
	return;
}

static Ntv2Status ntv2_hdmiin_initialize(struct ntv2_hdmiin *ntv2_hin)
{
	Ntv2Status res;

	NTV2_MSG_HDMIIN_STATE("%s: hdmi input initialize\n", ntv2_hin->name);

	/* initialize periodic update state */
	ntv2_hin->relock_reports = 0;
	ntv2_hin->hdmi_mode = false;
	ntv2_hin->hdcp_mode = false;
	ntv2_hin->derep_mode = false;;
	ntv2_hin->uhd_mode = false;
	ntv2_hin->cable_present = false;
	ntv2_hin->clock_present = false;
	ntv2_hin->input_locked = false;
	ntv2_hin->pixel_double_mode = false;
	ntv2_hin->avi_packet_present = false;
	ntv2_hin->vsi_packet_present = false;
	ntv2_hin->interlaced_mode = false;
	ntv2_hin->deep_color_10bit = false;
	ntv2_hin->deep_color_12bit = false;
	ntv2_hin->yuv_mode = false;
	ntv2_hin->prefer_yuv = false;
	ntv2_hin->prefer_rgb = false;
	ntv2_hin->color_space = ntv2_color_space_none;
	ntv2_hin->color_depth = ntv2_color_depth_none;
	ntv2_hin->colorimetry = ntv2_colorimetry_unknown;
	ntv2_hin->quantization = ntv2_quantization_unknown;
	ntv2_hin->relock_reports = NTV2_REPORT_ANY;

	ntv2_video_format_init(&ntv2_hin->dvi_format);
	ntv2_video_format_init(&ntv2_hin->hdmi_format);
	ntv2_hdmiin_set_no_video(ntv2_hin);

	/* reset the hdmi input chip */
	ntv2_konai2c_set_device(ntv2_hin, device_io_bank);
	ntv2_konai2c_write(ntv2_hin, 0xff, 0x80);
	ntv2EventWaitForSignal(&ntv2_hin->monitor_event, 100000, true);

	/* configure hdmi input chip default state */
	res = ntv2_hdmiin_write_multi(ntv2_hin, device_io_bank, init_io0, init_io0_size);
	if (res < 0)
		goto bad_write;

	/* verify some written data */
	res = ntv2_hdmiin_read_verify(ntv2_hin, device_io_bank, init_io0, init_io0_size);
	if (res < 0)
		goto bad_write;

	/* continue config */
	res = ntv2_hdmiin_write_multi(ntv2_hin, device_hdmi_bank, init_hdmi1, init_hdmi1_size);
	if (res < 0)
		goto bad_write;
	res = ntv2_hdmiin_write_multi(ntv2_hin, device_io_bank, init_io2_non4k, init_io2_non4k_size);
	if (res < 0)
		goto bad_write;
	res = ntv2_hdmiin_write_multi(ntv2_hin, device_cp_bank, init_cp3, init_cp3_size);
	if (res < 0)
		goto bad_write;
	res = ntv2_hdmiin_write_multi(ntv2_hin, device_repeater_bank, init_rep4, init_rep4_size);
	if (res < 0)
		goto bad_write;
	res = ntv2_hdmiin_write_multi(ntv2_hin, device_dpll_bank, init_dpll5_non4k, init_dpll5_non4k_size);
	if (res < 0)
		goto bad_write;
	res = ntv2_hdmiin_write_multi(ntv2_hin, device_hdmi_bank, init_hdmi6, init_hdmi6_size);
	if (res < 0)
		goto bad_write;

	/* load edid */
	if (ntv2_hin->edid != NULL) {
		res = ntv2_hdmiin_write_edid(ntv2_hin, ntv2_hin->edid, device_edid_bank);
		if (res < 0)
			goto bad_write;
	}

	/* final config */
	res = ntv2_hdmiin_write_multi(ntv2_hin, device_hdmi_bank, init_hdmi8, init_hdmi8_size);
	if (res < 0)
		goto bad_write;

	/* hot plug */
	ntv2_hdmiin_hot_plug(ntv2_hin);

	return NTV2_STATUS_SUCCESS;

bad_write:
	return NTV2_STATUS_BAD_PARAMETER;
}

static Ntv2Status ntv2_hdmiin_write_multi(struct ntv2_hdmiin *ntv2_hin,
										  uint8_t device,
										  struct ntv2_reg_value *reg_value,
										  int count)
{
	int i;
	Ntv2Status res;

	ntv2_konai2c_set_device(ntv2_hin, device);
	for (i = 0; i < count; i++) {
		res = ntv2_konai2c_write(ntv2_hin, reg_value[i].address, reg_value[i].value);
		if (res < 0) {
			NTV2_MSG_HDMIIN_ERROR("%s: *error* write multi failed  device %02x  address %02x\n",
								  ntv2_hin->name, device, reg_value[i].address);
			return res;
		}
	}

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_hdmiin_read_verify(struct ntv2_hdmiin *ntv2_hin,
										  uint8_t device,
										  struct ntv2_reg_value *reg_value,
										  int count)
{
	uint8_t val;
	int i;
	Ntv2Status res;
	bool success = true;

	ntv2_konai2c_set_device(ntv2_hin, device);
	res = ntv2_konai2c_cache_update(ntv2_hin);
	if (res < 0) {
			NTV2_MSG_HDMIIN_ERROR("%s: *error* read verify cache update failed  device %02x\n",
								  ntv2_hin->name, device);
			return res;
	}

	for (i = 0; i < count; i++) {
		val = ntv2_konai2c_cache_read(ntv2_hin, reg_value[i].address);
		if (val != reg_value[i].value) {
			NTV2_MSG_HDMIIN_ERROR("%s: *error* read verify failed  device %02x  address %02x  read %02x  expected %02x\n",
								  ntv2_hin->name, device, reg_value[i].address, val, reg_value[i].value);
			success = false;
		}
	}

	return success? NTV2_STATUS_SUCCESS : NTV2_STATUS_FAIL;
}

static Ntv2Status ntv2_hdmiin_write_edid(struct ntv2_hdmiin *ntv2_hin,
										 struct ntv2_hdmiedid *ntv2_edid,
										 uint8_t device)
{
	uint8_t* data = ntv2_hdmi_get_edid_data(ntv2_edid);
	uint32_t size = ntv2_hdmi_get_edid_size(ntv2_edid);
	uint32_t address = 0;
	Ntv2Status res;

	ntv2_konai2c_set_device(ntv2_hin, device);
	for (address = 0; address < size; address++) {
		res = ntv2_konai2c_write(ntv2_hin, (uint8_t)address, data[address]);
		if (res < 0) {
			NTV2_MSG_HDMIIN_ERROR("%s: *error* write edid failed  device %02x  address %02x\n",
								  ntv2_hin->name, device, address);
			return res;
		}
	}

	return NTV2_STATUS_SUCCESS;
}

Ntv2Status ntv2_hdmiin_periodic_update(struct ntv2_hdmiin *ntv2_hin)
{
	struct ntv2_hdmiin_format dvi_format;
	struct ntv2_hdmiin_format hdmi_format;
	struct ntv2_hdmiin_format vid_format;
	bool present = false;
	bool tmds_lock_change = false;
	bool tmds_frequency_change = false;
	bool derep_on = false;
	bool yuv_input;
	bool yuv_output;
	uint8_t data = 0;
	uint32_t val = 0;
	int res = 0;

	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	/* update debug flags */
	ntv2_update_debug_flags(ntv2_hin);

	/* read io bank */
	ntv2_konai2c_set_device(ntv2_hin, device_io_bank);
	res = ntv2_konai2c_cache_update(ntv2_hin);
	if (res < 0) {
		NTV2_MSG_HDMIIN_ERROR("%s: *error* io bank read cache update failed\n",
							  ntv2_hin->name);
		return res;
	}

	/* cable detect */
	data = ntv2_konai2c_cache_read(ntv2_hin, cable_detect_reg);
	present = (data & cable_detect_mask) == cable_detect_mask;
	if (present != ntv2_hin->cable_present) {
		ntv2_hin->relock_reports |= NTV2_REPORT_CABLE;
		ntv2_hin->cable_present = present;
	}
	if ((ntv2_hin->relock_reports & NTV2_REPORT_CABLE) != 0) {
		NTV2_MSG_HDMIIN_STATE("%s: cable %s\n",
							  ntv2_hin->name, 
							  (ntv2_hin->cable_present) ? "present" : "absent")
		ntv2_hin->relock_reports &= ~NTV2_REPORT_CABLE;
	}
	if (!ntv2_hin->cable_present) {
		ntv2_hdmiin_set_no_video(ntv2_hin);		
		ntv2_hin->relock_reports = NTV2_REPORT_ANY;
		ntv2_hin->relock_reports &= ~NTV2_REPORT_CABLE;
		return res;
	}

	/* check tmds lock transition */
	data = ntv2_konai2c_cache_read(ntv2_hin, tmds_lock_detect_reg);
	tmds_lock_change = (data & tmds_lock_detect_mask) == tmds_lock_detect_mask;
	if (tmds_lock_change) {
		ntv2_konai2c_write(ntv2_hin, tmds_lock_clear_reg, tmds_lock_clear_mask);
		NTV2_MSG_HDMIIN_STATE("%s: tmds lock transition detected\n",
							  ntv2_hin->name);
		ntv2_hin->tmds_frequency = 0;
		ntv2_hin->relock_reports = NTV2_REPORT_ANY;
	}

	/* tmds clock frequency transition */
	data = ntv2_konai2c_cache_read(ntv2_hin, tmds_frequency_detect_reg);
	tmds_frequency_change = (data & tmds_frequency_detect_mask) == tmds_frequency_detect_mask;
	if (tmds_frequency_change) {
		ntv2_konai2c_write(ntv2_hin, tmds_frequency_clear_reg, tmds_frequency_clear_mask);
		NTV2_MSG_HDMIIN_STATE("%s: tmds frequency transistion detected\n",
							  ntv2_hin->name);
		/* this happens on switch to uhd mode */
		ntv2_hin->relock_reports = NTV2_REPORT_ANY;
	}

	/* check input clock */
	data = ntv2_konai2c_cache_read(ntv2_hin, clock_detect_reg);
	ntv2_hin->clock_present = (data & clock_tmdsa_lock_mask) == clock_tmdsa_lock_mask;
	if ((ntv2_hin->relock_reports & NTV2_REPORT_SYNC) != 0)	{
		NTV2_MSG_HDMIIN_STATE("%s: tmds clock %s/%s  sync %s  regen %s\n",
							  ntv2_hin->name, 
							  (data & clock_tmdsa_present_mask) ? "present" : "absent",
							  (data & clock_tmdsa_lock_mask) ? "locked" : "unlocked",
							  (data & clock_sync_lock_mask) ? "locked" : "unlocked",
							  (data & clock_regen_lock_mask) ? "locked" : "unlocked");
		if (((data & clock_sync_lock_mask) != 0) &&
			((data & clock_regen_lock_mask) != 0))
			ntv2_hin->relock_reports &= ~NTV2_REPORT_SYNC;
	}

	/* avi/vsi packet detection */
	data = ntv2_konai2c_cache_read(ntv2_hin, packet_detect_reg);
	ntv2_hin->avi_packet_present = (data & packet_detect_avi_mask) == packet_detect_avi_mask;
	ntv2_hin->vsi_packet_present = (data & packet_detect_vsi_mask) == packet_detect_vsi_mask;

	/* if pll lock was lost, recover to a state where we can take valid measurements */
	if (!ntv2_hin->clock_present || tmds_lock_change) {
		if (ntv2_hin->uhd_mode) {
			res = ntv2_hdmiin_set_uhd_mode(ntv2_hin, false);
			if (res < 0) 
				return res;
		}

		if (ntv2_hin->derep_mode) {
			res = ntv2_hdmi_set_derep_mode(ntv2_hin, false);
			if (res < 0) 
				return res;
		}

		goto bad_lock;
	}

	/* read hdmi bank */
	ntv2_konai2c_set_device(ntv2_hin, device_hdmi_bank);
	res = ntv2_konai2c_cache_update(ntv2_hin);
	if (res < 0) {
			NTV2_MSG_HDMIIN_ERROR("%s: *error* hdmi bank read cache update failed\n",
								  ntv2_hin->name);
			return res;
	}

	/* hdmi/dvi mode */
	data = ntv2_konai2c_cache_read(ntv2_hin, hdmi_mode_reg);
	ntv2_hin->hdmi_mode = (data & hdmi_mode_mask) != 0;
	ntv2_hin->hdcp_mode = (data & hdmi_encrypted_mask) != 0;

	/* deep color mode */
	data = ntv2_konai2c_cache_read(ntv2_hin, deep_color_mode_reg);
	ntv2_hin->deep_color_10bit = (data & deep_color_10bit_mask) != 0;
	ntv2_hin->deep_color_12bit = (data & deep_color_12bit_mask) != 0;

	/* dereplicator mode */
	data = ntv2_konai2c_cache_read(ntv2_hin, derep_mode_reg);
	ntv2_hin->derep_mode = (data & derep_mode_mask) != 0;

	/* input locked */
	data = ntv2_konai2c_cache_read(ntv2_hin, defilter_lock_detect_reg);
	ntv2_hin->input_locked = ((data & defilter_locked_mask) != 0) &&
		((data & vfilter_locked_mask) != 0);

	/* interlaced mode */
	data = ntv2_konai2c_cache_read(ntv2_hin, interlaced_detect_reg);
	ntv2_hin->interlaced_mode = (data & interlaced_mask) != 0;

	/* get current tmds frequency, does not work in uhd mode */
	if ((ntv2_hin->relock_reports & NTV2_REPORT_FREQ) && !ntv2_hin->uhd_mode) {
		/* read the new tmds fequency */
		ntv2_hdmiin_update_tmds_freq(ntv2_hin);
		/* update the pixel clock */
		ntv2_hdmiin_config_pixel_clock(ntv2_hin);

		NTV2_MSG_HDMIIN_STATE("%s: tmds frequency %d kHz\n",
							  ntv2_hin->name, ntv2_hin->tmds_frequency);
		ntv2_hin->relock_reports &= ~NTV2_REPORT_FREQ;
		return NTV2_STATUS_SUCCESS;
	}

	if (ntv2_hin->input_locked && !ntv2_hin->hdmi_mode)
	{
		/*
		  When in DVI mode and pixels get doubled, we must also turn on the
		  pixel dereplicator. This will halve the measurements and make
		  the video be the right size downstream. Note that in HDMI mode,
		  the dereplicator is automatically driven by the AVI info frame data.
		*/
		derep_on = (ntv2_hin->pixel_double_mode && ntv2_hin->interlaced_mode);
		/*
		  Turning on the pixel dereplicator is not reliable. Must check whether
		  the last such request really "took" and keep trying.
		*/
		if (derep_on && !ntv2_hin->derep_mode) {
			res = ntv2_hdmi_set_derep_mode(ntv2_hin, true);
			return res;
		}
		if (!derep_on && ntv2_hin->derep_mode) {
			res = ntv2_hdmi_set_derep_mode(ntv2_hin, false);
			return res;
		}
	}

	if (!ntv2_hin->input_locked)
	{
		ntv2_hin->relock_reports |= NTV2_REPORT_ANY;
	}

	if (ntv2_hin->relock_reports & NTV2_REPORT_INFO) {
		val = 8;
		if (ntv2_hin->deep_color_10bit) val = 10;
		if (ntv2_hin->deep_color_12bit) val = 12;
		NTV2_MSG_HDMIIN_STATE("%s: input %s  mode %s  hdcp %s  derep %s  interlaced %d  depth %d\n",
							  ntv2_hin->name,
							  ntv2_hin->input_locked? "locked" : "unlocked",
							  ntv2_hin->hdmi_mode? "hdmi" : "dvi",
							  ntv2_hin->hdcp_mode? "on" : "off",
							  ntv2_hin->derep_mode? "on" : "off",
							  ntv2_hin->interlaced_mode,
							  val);
		ntv2_hin->relock_reports &= ~NTV2_REPORT_INFO;
	}

	/* update timing values */
	if (ntv2_hin->relock_reports & NTV2_REPORT_TIMING) {
		ntv2_hdmiin_update_timing(ntv2_hin);
		NTV2_MSG_HDMIIN_STATE("%s: horizontal  active %d  total %d  fp %d  sync %d  bp %d\n",
							  ntv2_hin->name,
							  ntv2_hin->h_active_pixels,
							  ntv2_hin->h_total_pixels,
							  ntv2_hin->h_front_porch_pixels,
							  ntv2_hin->h_sync_pixels,
							  ntv2_hin->h_back_porch_pixels);
		NTV2_MSG_HDMIIN_STATE("%s: vertical  active %d/%d  total %d/%d  fp %d/%d  sync %d/%d  bp %d/%d  freq %d\n",
							  ntv2_hin->name,
							  ntv2_hin->v_active_lines0,
							  ntv2_hin->v_active_lines1,
							  ntv2_hin->v_total_lines0,
							  ntv2_hin->v_total_lines1,
							  ntv2_hin->v_front_porch_lines0,
							  ntv2_hin->v_front_porch_lines1,
							  ntv2_hin->v_sync_lines0,
							  ntv2_hin->v_sync_lines1,
							  ntv2_hin->v_back_porch_lines0,
							  ntv2_hin->v_back_porch_lines1,
							  ntv2_hin->v_frequency);
		ntv2_hin->relock_reports &= ~NTV2_REPORT_TIMING;
	}

	/* 
		sanity check - sometimes the hdmi chip provides bad values after a transition when 
		the pixel clock is too far off frequency (as in the certification test)
	*/
	if ((ntv2_hin->v_active_lines0 < 200) || (ntv2_hin->v_total_lines0 < 200) ||
		(ntv2_hin->h_sync_pixels < 2) || (ntv2_hin->h_active_pixels < 600) ||
		(ntv2_hin->v_sync_lines0 == 0) || (ntv2_hin->v_active_lines0 < 200) ||
		(ntv2_hin->v_total_lines0 < 200) || (ntv2_hin->v_frequency < 10) ||
		(ntv2_hin->interlaced_mode &&
		 ((ntv2_hin->v_active_lines1 < 200) || (ntv2_hin->v_total_lines1 < 200) ||
		  (ntv2_hin->v_sync_lines1 == 0)))) {
		ntv2_hin->relock_reports = NTV2_REPORT_ANY;
		goto bad_lock;
	}

	/* switch modes for uhd */
	if ((ntv2_hin->v_total_lines0 > 1200) &&
		(!ntv2_hin->uhd_mode)) {
		res = ntv2_hdmiin_set_uhd_mode(ntv2_hin, true);
		return res;
	}

	dvi_format.video_standard = ntv2_video_standard_none;
	dvi_format.frame_rate = ntv2_frame_rate_none;
	dvi_format.frame_flags = 0;
	dvi_format.pixel_flags = 0;
		
	/* determine input format from timing */
	ntv2_hdmiin_find_dvi_format(ntv2_hin, &dvi_format);
	if (!ntv2_video_format_compare(&dvi_format, &ntv2_hin->dvi_format))
	{
		ntv2_hin->relock_reports |= NTV2_REPORT_FORMAT;
	}
	ntv2_hin->dvi_format = dvi_format;

	hdmi_format.video_standard = ntv2_video_standard_none;
	hdmi_format.frame_rate = ntv2_frame_rate_none;
	hdmi_format.frame_flags = 0;
	hdmi_format.pixel_flags = 0;

	if (ntv2_hin->hdmi_mode)
	{
		/* read info bank */
		ntv2_konai2c_set_device(ntv2_hin, device_info_bank);
		res = ntv2_konai2c_cache_update(ntv2_hin);
		if (res < 0) {
			NTV2_MSG_HDMIIN_ERROR("%s: *error* info bank read cache update failed\n",
								  ntv2_hin->name);
			return res;
		}

		/* determine input format from hdmi info */
		ntv2_hdmiin_find_hdmi_format(ntv2_hin, &hdmi_format);

		if (!ntv2_video_format_compare(&hdmi_format, &ntv2_hin->hdmi_format))
		{
			ntv2_hin->relock_reports |= NTV2_REPORT_FORMAT;
		}
	}
	ntv2_hin->hdmi_format = hdmi_format;

	/* determine source format */
	if (((hdmi_format.video_standard == ntv2_video_standard_none) ||
		 (hdmi_format.video_standard == ntv2_video_standard_1080p)) &&
		(hdmi_format.frame_flags != 0) &&
		(dvi_format.video_standard == ntv2_video_standard_2048x1080p)) {
		vid_format.video_standard = dvi_format.video_standard;
		vid_format.frame_rate = dvi_format.frame_rate;
		vid_format.frame_flags = hdmi_format.frame_flags;
		vid_format.pixel_flags = hdmi_format.pixel_flags;
	}
	else if (hdmi_format.video_standard != ntv2_video_standard_none) {
		vid_format.video_standard = hdmi_format.video_standard;
		if (hdmi_format.video_standard == dvi_format.video_standard) {
			vid_format.frame_rate = dvi_format.frame_rate;
		} else {
			vid_format.frame_rate = hdmi_format.frame_rate;
		}
		vid_format.frame_flags = hdmi_format.frame_flags;
		vid_format.pixel_flags = hdmi_format.pixel_flags;
	}
	else {
		vid_format.video_standard = dvi_format.video_standard;
		vid_format.frame_rate = dvi_format.frame_rate;
		vid_format.frame_flags = dvi_format.frame_flags;
		vid_format.pixel_flags = dvi_format.pixel_flags;
	}

	if (ntv2_hin->relock_reports & NTV2_REPORT_FORMAT)
	{
		if ((vid_format.video_standard != ntv2_video_standard_none) &&
			(vid_format.frame_rate != ntv2_frame_rate_none))
		{
			/* configure output color space */
			yuv_input = (vid_format.pixel_flags & ntv2_kona_pixel_yuv) != 0;
			yuv_output = yuv_input;
			if (!ntv2_hin->uhd_mode) {
				if (ntv2_hin->prefer_yuv)
					yuv_output = true;
				if (ntv2_hin->prefer_rgb)
					yuv_output = false;
			}
			ntv2_hdmiin_set_color_mode(ntv2_hin, yuv_input, yuv_output);

			/* correct output pixel flags */
			if (yuv_output) {
				vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_rgb) | ntv2_kona_pixel_yuv;
				vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_444) | ntv2_kona_pixel_422;
				if (!yuv_input) {
					vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_full) | ntv2_kona_pixel_smpte;
					if ((vid_format.pixel_flags & ntv2_kona_pixel_10bit) != 0) {
						vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_10bit) | ntv2_kona_pixel_12bit;
					}
					if ((vid_format.pixel_flags & ntv2_kona_pixel_8bit) != 0) {
						vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_8bit) | ntv2_kona_pixel_10bit;
					}
				}
			} else {
				vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_yuv) | ntv2_kona_pixel_rgb;
				vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_422) | ntv2_kona_pixel_444;
				if (yuv_input) {
					vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_smpte) | ntv2_kona_pixel_full;
					if ((vid_format.pixel_flags & ntv2_kona_pixel_10bit) != 0) {
						vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_10bit) | ntv2_kona_pixel_8bit;
					}
					if ((vid_format.pixel_flags & ntv2_kona_pixel_12bit) != 0) {
						vid_format.pixel_flags = (vid_format.pixel_flags & ~ntv2_kona_pixel_8bit) | ntv2_kona_pixel_10bit;
					}
				}
			}

			ntv2_hdmiin_set_video_format(ntv2_hin, &vid_format);
		}
		else
		{
			ntv2_hdmiin_set_no_video(ntv2_hin);		
		}

		NTV2_MSG_HDMIIN_STATE("%s: dvi  standard %s  rate %s  frame %08x  pixel %08x\n",
							  ntv2_hin->name,
							  ntv2_video_standard_name(dvi_format.video_standard),
							  ntv2_frame_rate_name(dvi_format.frame_rate),
							  dvi_format.frame_flags,
							  dvi_format.pixel_flags);

		NTV2_MSG_HDMIIN_STATE("%s: hdmi standard %s  rate %s  frame %08x  pixel %08x\n",
							  ntv2_hin->name,
							  ntv2_video_standard_name(hdmi_format.video_standard),
							  ntv2_frame_rate_name(hdmi_format.frame_rate),
							  hdmi_format.frame_flags,
							  hdmi_format.pixel_flags);

		NTV2_MSG_HDMIIN_STATE("%s: video standard %s  rate %s  frame %08x  pixel %08x\n",
							  ntv2_hin->name,
							  ntv2_video_standard_name(vid_format.video_standard),
							  ntv2_frame_rate_name(vid_format.frame_rate),
							  vid_format.frame_flags,
							  vid_format.pixel_flags);

		ntv2_hin->relock_reports &= ~NTV2_REPORT_FORMAT;
	}

	/* configure dynamic aux data */
	if (ntv2_hin->hdmi_mode)
	{
		ntv2_hdmiin_set_aux_data(ntv2_hin, &hdmi_format);
	}

	if ((ntv2_hin->input_locked) &&
		(vid_format.video_standard != ntv2_video_standard_none) &&
		(vid_format.frame_rate != ntv2_frame_rate_none))
	{
		if (ntv2_hin->lock_error_count > 0)
			ntv2_hin->lock_error_count--;
	}
	else
	{
		if (!ntv2_hin->input_locked)
			ntv2_hin->relock_reports = NTV2_REPORT_ANY;
		goto bad_lock;
	}

	return NTV2_STATUS_SUCCESS;

bad_lock:
	ntv2_hin->lock_error_count += 5;
	if (ntv2_hin->lock_error_count > (12 * 5)) {
		ntv2_hin->relock_reports = NTV2_REPORT_ANY;

		if (ntv2_hin->uhd_mode)
			ntv2_hdmiin_set_uhd_mode(ntv2_hin, false);
		if (ntv2_hin->derep_mode)
			ntv2_hdmi_set_derep_mode(ntv2_hin, false);

		ntv2_hin->lock_error_count = 0;
		return NTV2_STATUS_FAIL;
	}

	return NTV2_STATUS_SUCCESS;
}

static void ntv2_hdmiin_hot_plug(struct ntv2_hdmiin *ntv2_hin)
{
	ntv2_konai2c_set_device(ntv2_hin, device_hdmi_bank);

	ntv2_konai2c_write(ntv2_hin, hdmi_hpa_reg, ntv2_hin->i2c_hpa_default | hdmi_hpa_manual_mask);
	ntv2EventWaitForSignal(&ntv2_hin->monitor_event, 250000, true);
	ntv2_konai2c_write(ntv2_hin, hdmi_hpa_reg, ntv2_hin->i2c_hpa_default & ~hdmi_hpa_manual_mask);
}

static Ntv2Status ntv2_hdmiin_set_color_mode(struct ntv2_hdmiin *ntv2_hin, bool yuv_input, bool yuv_output)
{
	ntv2_konai2c_set_device(ntv2_hin, device_io_bank);

	if (ntv2_hin->uhd_mode) {
		if (yuv_input) {
			ntv2_konai2c_write(ntv2_hin, 0x03, 0x96);
			ntv2_hin->yuv_mode = true;
		} else {
			ntv2_konai2c_write(ntv2_hin, 0x03, 0x54);
			ntv2_hin->yuv_mode = false;
		}
	} else {
		if (yuv_output) {
			ntv2_konai2c_write(ntv2_hin, 0x02, (ntv2_hin->i2c_color_default & ~0x06) | 0x04);
			ntv2_konai2c_write(ntv2_hin, 0x03, 0x82);
			ntv2_hin->yuv_mode = true;
		} else {
			ntv2_konai2c_write(ntv2_hin, 0x02, (ntv2_hin->i2c_color_default & ~0x06) | 0x06);
			ntv2_konai2c_write(ntv2_hin, 0x03, 0x42);
			ntv2_hin->yuv_mode = false;
		}
	}

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_hdmiin_set_uhd_mode(struct ntv2_hdmiin *ntv2_hin, bool enable)
{
	Ntv2Status res;

	if (enable)
	{
		NTV2_MSG_HDMIIN_STATE("%s: enable uhd mode\n", ntv2_hin->name);
		res = ntv2_hdmiin_write_multi(ntv2_hin, device_io_bank, init_io2_4k, init_io2_4k_size);
		if (res < 0)
			return res;
		res = ntv2_hdmiin_write_multi(ntv2_hin, device_dpll_bank, init_dpll5_4k, init_dpll5_4k_size);
		if (res < 0)
			return res;
	} else {
		NTV2_MSG_HDMIIN_STATE("%s: disable uhd mode\n", ntv2_hin->name);
		res = ntv2_hdmiin_write_multi(ntv2_hin, device_io_bank, init_io2_non4k, init_io2_non4k_size);
		if (res < 0)
			return res;
		res = ntv2_hdmiin_write_multi(ntv2_hin, device_dpll_bank, init_dpll5_non4k, init_dpll5_non4k_size);
		if (res < 0)
			return res;
	}

	ntv2_hin->uhd_mode = enable;

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_hdmi_set_derep_mode(struct ntv2_hdmiin *ntv2_hin, bool enable)
{
	Ntv2Status res;

	NTV2_MSG_HDMIIN_STATE("%s: %s derep mode\n", ntv2_hin->name, enable? "enable" : "disable");

	ntv2_konai2c_set_device(ntv2_hin, device_hdmi_bank);
	res = ntv2_konai2c_write(ntv2_hin, 0x41, enable? 0x11 : 0x00);
	if (res < 0)
		return res;

	ntv2_hin->derep_mode = enable;

	return NTV2_STATUS_SUCCESS;
}

static void ntv2_hdmiin_update_tmds_freq(struct ntv2_hdmiin *ntv2_hin)
{
	uint32_t hival;
	uint32_t loval;

	hival = (uint32_t)ntv2_konai2c_cache_read(ntv2_hin, 0x51);
	loval = (uint32_t)ntv2_konai2c_cache_read(ntv2_hin, 0x52);
	ntv2_hin->tmds_frequency = ((hival << 1) | (loval >> 7))*1000 +
		(loval & 0x7f)*1000/128;
}

static void ntv2_hdmiin_config_pixel_clock(struct ntv2_hdmiin *ntv2_hin)
{
	uint8_t phase = 0;
	bool invert = false;

	/* high frequency clock phase */
	if (ntv2_hin->tmds_frequency >= 279000)
		phase = CLOCK_PHASE_HF;

	ntv2_konai2c_set_device(ntv2_hin, device_hdmi_bank);
	if (ntv2_hin->tmds_frequency <= TMDS_DOUBLING_FREQ)
	{
		/* adi required for TMDS frequency 27Mhz and below */
		ntv2_konai2c_write(ntv2_hin, 0x85, 0x11);
		ntv2_konai2c_write(ntv2_hin, 0x9C, 0x80);
		ntv2_konai2c_write(ntv2_hin, 0x9C, 0xC0);
		ntv2_konai2c_write(ntv2_hin, 0x9C, 0x00);
		ntv2_konai2c_write(ntv2_hin, 0x85, 0x11);
		ntv2_konai2c_write(ntv2_hin, 0x86, 0x9B);
		ntv2_konai2c_write(ntv2_hin, 0x9B, 0x03);

		ntv2_konai2c_set_device(ntv2_hin, device_io_bank);
		ntv2_konai2c_write(ntv2_hin, 0x19, 0xC0 | phase);
		ntv2_hin->pixel_double_mode = true;
	}
	else
	{
		/* adi required for TMDS frequency above 27Mhz */
		ntv2_konai2c_write(ntv2_hin, 0x85, 0x10);
		ntv2_konai2c_write(ntv2_hin, 0x9C, 0x80);
		ntv2_konai2c_write(ntv2_hin, 0x9C, 0xC0);
		ntv2_konai2c_write(ntv2_hin, 0x9C, 0x00);
		ntv2_konai2c_write(ntv2_hin, 0x85, 0x10);
		ntv2_konai2c_write(ntv2_hin, 0x86, 0x9B);
		ntv2_konai2c_write(ntv2_hin, 0x9B, 0x03);

		ntv2_konai2c_set_device(ntv2_hin, device_io_bank);
		ntv2_konai2c_write(ntv2_hin, 0x19, 0x80 | phase);
		ntv2_hin->pixel_double_mode = false;
	}

	if (invert)
		ntv2_konai2c_write(ntv2_hin, 0x06, 0xa7);
}

static uint32_t ntv2_hdmiin_read_paired_value(struct ntv2_hdmiin *ntv2_hin, uint8_t reg, uint32_t bits, uint32_t shift)
{
	uint8_t msb;
	uint8_t lsb;
	uint32_t val;

	msb = ntv2_konai2c_cache_read(ntv2_hin, reg);
	lsb = ntv2_konai2c_cache_read(ntv2_hin, reg + 1);

	val = (msb << 8) | lsb;
	val &= ~(0xffffffff << bits);
	val >>= shift;

	return val;
}

static void ntv2_hdmiin_update_timing(struct ntv2_hdmiin *ntv2_hin)
{
	uint32_t total_lines = 0;
	uint32_t frame_rate = 0;

	ntv2_hin->h_active_pixels = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x07, 13, 0);
	ntv2_hin->h_total_pixels = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x1e, 13, 0);
	ntv2_hin->h_front_porch_pixels = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x20, 13, 0);
	ntv2_hin->h_sync_pixels = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x22, 13, 0);
	ntv2_hin->h_back_porch_pixels = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x24, 13, 0);

	ntv2_hin->v_active_lines0 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x09, 13, 0);
	ntv2_hin->v_total_lines0 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x26, 14, 1);
	ntv2_hin->v_front_porch_lines0 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x2a, 14, 1);
	ntv2_hin->v_sync_lines0 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x2e, 14, 1);
	ntv2_hin->v_back_porch_lines0 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x32, 14, 1);

	if (ntv2_hin->interlaced_mode) {
		ntv2_hin->v_active_lines1 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x0b, 13, 0);
		ntv2_hin->v_total_lines1 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x28, 14, 1);
		ntv2_hin->v_front_porch_lines1 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x2c, 14, 1);
		ntv2_hin->v_sync_lines1 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x30, 14, 1);
		ntv2_hin->v_back_porch_lines1 = ntv2_hdmiin_read_paired_value(ntv2_hin, 0x34, 14, 1);
	} else {
		ntv2_hin->v_active_lines1 = 0;
		ntv2_hin->v_total_lines1 = 0;
		ntv2_hin->v_front_porch_lines1 = 0;
		ntv2_hin->v_sync_lines1 = 0;
		ntv2_hin->v_back_porch_lines1 = 0;
	}

	/* compute frame rate (fps * 1000) */
	total_lines = ntv2_hin->v_total_lines0 + ntv2_hin->v_total_lines1;
	if ((total_lines != 0) && (ntv2_hin->h_total_pixels != 0)) {
		frame_rate = ntv2_hin->tmds_frequency * 1000 / total_lines;
		frame_rate = frame_rate * 1000 / ntv2_hin->h_total_pixels;
	}

	if (ntv2_hin->deep_color_10bit)
		frame_rate = frame_rate * 8 / 10;
	if (ntv2_hin->deep_color_12bit)
		frame_rate = frame_rate * 8 / 12;
	if (ntv2_hin->pixel_double_mode)
		frame_rate /= 2;
	ntv2_hin->v_frequency = frame_rate;
}

static void ntv2_hdmiin_find_dvi_format(struct ntv2_hdmiin *ntv2_hin,
										struct ntv2_hdmiin_format *format)
{
	uint32_t standard = ntv2_video_standard_none;
	uint32_t rate = ntv2_frame_rate_none;
	uint32_t f_flags = 0;
	uint32_t p_flags = 0;
	uint32_t width;
	uint32_t height;
	uint32_t dh;
	uint32_t dv;
	uint32_t dr;
	uint32_t dr_min = 1000000;
	bool progressive;
	uint32_t input_rate = ntv2_hin->v_frequency;
	uint32_t ntv2_rate;
	uint32_t i;

	/* find ntv2 video standard */
	for (i = 0; i < NTV2_MAX_VIDEO_STANDARDS; i++) {
		width = ntv2_video_standard_width(i);
		height = ntv2_video_standard_height(i);
		progressive = ntv2_video_standard_progressive(i);
		if (height <= 520)
			height -= 6;
		dh = ntv2_diff(ntv2_hin->h_active_pixels, width);
		dv = ntv2_diff((ntv2_hin->v_active_lines0 + ntv2_hin->v_active_lines1), height);
		if ((dh <= ntv2_hin->horizontal_tol) &&
			(dv <= ntv2_hin->vertical_tol) &&
			(progressive == !ntv2_hin->interlaced_mode)) {
			standard = i;
			break;
		}
	}

	/* find ntv2 frame rate */
	for (i = 0; i < NTV2_MAX_FRAME_RATES; i++) {
		ntv2_rate = ntv2_frame_rate_scale(i) * 1000 / ntv2_frame_rate_duration(i);
		dr = ntv2_diff(ntv2_rate, input_rate);
		if (dr < dr_min) {
			dr_min = dr;
			rate = i;
		}
	}

	/* set ntv2 frame progressive/interlaced flags */
	if (ntv2_hin->interlaced_mode) {
		f_flags = ntv2_kona_frame_picture_interlaced | ntv2_kona_frame_transport_interlaced;
	} else {
		f_flags = ntv2_kona_frame_picture_progressive | ntv2_kona_frame_transport_progressive;
	}

	/* use tmds frequency to set frame rate class */
	if (ntv2_hin->tmds_frequency > 145000) {
		f_flags |= ntv2_kona_frame_3g;
	} else if (ntv2_hin->tmds_frequency > 74000) {
		f_flags |= ntv2_kona_frame_hd;
	} else {
		f_flags |= ntv2_kona_frame_sd;
	}

	/* set ntv2 pixel flags */
	p_flags = ntv2_kona_pixel_rgb |
		ntv2_kona_pixel_full |
		ntv2_kona_pixel_444 |
		ntv2_kona_pixel_8bit;
	if ((standard == ntv2_video_standard_525i) ||
		(standard == ntv2_video_standard_625i)) {
		f_flags |= ntv2_kona_frame_4x3;
		p_flags |= ntv2_kona_pixel_rec601;
	} else {
		f_flags |= ntv2_kona_frame_16x9;
		p_flags |= ntv2_kona_pixel_rec709;
	}

	ntv2_hin->color_space = ntv2_color_space_rgb444;
	ntv2_hin->color_depth = ntv2_color_depth_8bit;
	ntv2_hin->aspect_ratio = ntv2_aspect_ratio_unknown;
	ntv2_hin->colorimetry = ntv2_colorimetry_unknown;
	ntv2_hin->quantization = ntv2_quantization_unknown;

	format->video_standard = standard;
	format->frame_rate = rate;
	format->frame_flags = f_flags;
	format->pixel_flags = p_flags;
}		

static void ntv2_hdmiin_find_hdmi_format(struct ntv2_hdmiin *ntv2_hin,
										 struct ntv2_hdmiin_format *format)
{
	uint32_t standard = ntv2_video_standard_none;
	uint32_t rate = ntv2_frame_rate_none;
	uint32_t f_flags = 0;
	uint32_t p_flags = 0;
	uint8_t a_packet;
	uint8_t a_version;
	uint8_t a_byte[6];
	uint8_t v_packet;
	uint8_t v_version;
	uint8_t v_byte[6];
	uint8_t val;
	bool a_good = false;
	bool v_good = false;
	uint8_t i;
	
	/* read avi info data */
	if (ntv2_hin->avi_packet_present) {
		a_packet = ntv2_konai2c_cache_read(ntv2_hin, avi_infoframe_packet_id);
		a_version = ntv2_konai2c_cache_read(ntv2_hin, avi_infoframe_version);
		a_good = (a_packet == avi_packet_id); // && (a_version <= avi_version);
	}
	if (a_good) {
		for (i = 0; i < 5; i++) {
			a_byte[i + 1] = ntv2_konai2c_cache_read(ntv2_hin, avi_infoframe_byte1 + i);
		}
	}
	else {
		goto done;
	}

	/* read vsi info data */
	if (ntv2_hin->vsi_packet_present) {
		v_packet = ntv2_konai2c_cache_read(ntv2_hin, vsi_infoframe_packet_id);
		v_version = ntv2_konai2c_cache_read(ntv2_hin, vsi_infoframe_version);
		v_good = (v_packet == vsi_packet_id); // && (v_version <= vsi_version);
	}
	if (v_good) {
		for (i = 0; i < 5; i++) {
			v_byte[i + 1] = ntv2_konai2c_cache_read(ntv2_hin, vsi_infoframe_byte1 + i);
		}
	}

	/* scan for standard and rate */
	val = (a_byte[4] & avi_vic_mask4) >> avi_vic_shift4;
	if (val != 0) {
		if (val < NTV2_AVI_VIC_INFO_SIZE) {
			standard = ntv2_avi_vic_info[val].video_standard;
			rate = ntv2_avi_vic_info[val].frame_rate;
		}
	} else {
		standard = ntv2_video_standard_none;
		if (v_good) {
			val = (v_byte[4] & vsi_video_format_mask4) >> vsi_video_format_shift4;
			if (val == vsi_format_extended) {
				val = v_byte[5];
				if (val < NTV2_VSI_VIC_INFO_SIZE) {
					standard = ntv2_vsi_vic_info[val].video_standard;
					rate = ntv2_vsi_vic_info[val].frame_rate;
				}
			}
		}
	}

	/* set frame progressive/interlace flags */
	if (ntv2_video_standard_progressive(standard)) {
		f_flags = ntv2_kona_frame_picture_progressive | ntv2_kona_frame_transport_progressive;
	} else {
		f_flags = ntv2_kona_frame_picture_interlaced | ntv2_kona_frame_transport_interlaced;
	}

	/* use tmds frequency to set frame rate class */
	if (ntv2_hin->tmds_frequency > 145000) {
		f_flags |= ntv2_kona_frame_3g;
	} else if (ntv2_hin->tmds_frequency > 74000) {
		f_flags |= ntv2_kona_frame_hd;
	} else {
		f_flags |= ntv2_kona_frame_sd;
	}

	/* scan for color component */
	val = (a_byte[1] & avi_color_component_mask1) >> avi_color_component_shift1;
	if (val == avi_color_comp_422) {
		p_flags |= ntv2_kona_pixel_yuv | ntv2_kona_pixel_422;
		ntv2_hin->color_space = ntv2_color_space_yuv422;
	} else if (val == avi_color_comp_444) {
		p_flags |= ntv2_kona_pixel_yuv | ntv2_kona_pixel_444;
		ntv2_hin->color_space = ntv2_color_space_yuv444;
	} else if (val == avi_color_comp_420) {
		p_flags |= ntv2_kona_pixel_yuv | ntv2_kona_pixel_420;
		ntv2_hin->color_space = ntv2_color_space_yuv420;
	} else {
		p_flags |= ntv2_kona_pixel_rgb | ntv2_kona_pixel_444;
		ntv2_hin->color_space = ntv2_color_space_rgb444;
	}

	/* scan for colorimetry */
	val = (a_byte[2] & avi_colorimetry_mask2) >> avi_colorimetry_shift2;
	if (val == avi_colorimetry_extended) {
		val = (a_byte[3] & avi_extended_colorimetry_mask3) >> avi_extended_colorimetry_shift3;
		if (val == avi_ext_colorimetry_adobe_601) {
			p_flags |= ntv2_kona_pixel_adobe;
			ntv2_hin->colorimetry = ntv2_colorimetry_adobe_601;
		} else if (val == avi_ext_colorimetry_adobe_rgb) {
			p_flags |= ntv2_kona_pixel_adobe;
			ntv2_hin->colorimetry = ntv2_colorimetry_adobe_rgb;
		} else if (val == avi_ext_colorimetry_xv_ycc601) {
			p_flags |= ntv2_kona_pixel_rec601;
			ntv2_hin->colorimetry = ntv2_colorimetry_xvycc_601;
		} else if (val == avi_ext_colorimetry_s_ycc601) {
			p_flags |= ntv2_kona_pixel_rec601;
			ntv2_hin->colorimetry = ntv2_colorimetry_170m;
		} else if (val == avi_ext_colorimetry_ycc2020) {
			p_flags |= ntv2_kona_pixel_rec2020;
			ntv2_hin->colorimetry = ntv2_colorimetry_bt2020_cl;
		} else if (val == avi_ext_colorimetry_rgb2020) {
			p_flags |= ntv2_kona_pixel_rec2020;
			ntv2_hin->colorimetry = ntv2_colorimetry_bt2020;
		} else {
			p_flags |= ntv2_kona_pixel_rec709;
			ntv2_hin->colorimetry = ntv2_colorimetry_bt709;
		}
	} else if (val == avi_colorimetry_smpte170m) {
		p_flags |= ntv2_kona_pixel_rec601;
		ntv2_hin->colorimetry = ntv2_colorimetry_170m;
	} else {
		p_flags |= ntv2_kona_pixel_rec709;
		ntv2_hin->colorimetry = ntv2_colorimetry_bt709;
	}

	/* scan for black/white range */
	if ((p_flags & ntv2_kona_pixel_rgb) != 0) {
		val = (a_byte[3] & avi_quantization_range_mask3) >> avi_quantization_range_shift3;
		if (val == avi_rgb_quant_range_default) {
			p_flags |= ntv2_kona_pixel_full;
			ntv2_hin->quantization = ntv2_quantization_default;
		} else if (val == avi_rgb_quant_range_limited) {
			p_flags |= ntv2_kona_pixel_smpte;
			ntv2_hin->quantization = ntv2_quantization_limited;
		} else {
			p_flags |= ntv2_kona_pixel_full;
			ntv2_hin->quantization = ntv2_quantization_full;
		}
	} else {
		p_flags |= ntv2_kona_pixel_smpte;
		ntv2_hin->quantization = ntv2_quantization_limited;
	}

	/* scan for aspect ratio */
	val = (a_byte[2] & avi_frame_aspect_ratio_mask2) >> avi_frame_aspect_ratio_shift2;
	if (val == avi_frame_aspect_nodata) {
		f_flags |= ntv2_kona_frame_16x9;
		ntv2_hin->aspect_ratio = ntv2_aspect_ratio_nodata;
	} else if (val == avi_frame_aspect_4x3) {
		f_flags |= ntv2_kona_frame_4x3;
		ntv2_hin->aspect_ratio = ntv2_aspect_ratio_4x3;
	} else {
		f_flags |= ntv2_kona_frame_16x9;
		ntv2_hin->aspect_ratio = ntv2_aspect_ratio_16x9;
	}

	/* use detected deep color for bit depth */
	if ((p_flags & ntv2_kona_pixel_rgb) != 0) {
		if (ntv2_hin->deep_color_12bit) {
			p_flags |= ntv2_kona_pixel_12bit;
			ntv2_hin->color_depth = ntv2_color_depth_12bit;
		} else if (ntv2_hin->deep_color_10bit) {
			p_flags |= ntv2_kona_pixel_10bit;
			ntv2_hin->color_depth = ntv2_color_depth_10bit;
		} else {
			p_flags |= ntv2_kona_pixel_8bit;
			ntv2_hin->color_depth = ntv2_color_depth_8bit;
		}
	} else {
		p_flags |= ntv2_kona_pixel_10bit;
		ntv2_hin->color_depth = ntv2_color_depth_10bit;
	}

done:
	format->video_standard = standard;
	format->frame_rate = rate;
	format->frame_flags = f_flags;
	format->pixel_flags = p_flags;
}

static uint32_t ntv2_hdmiin_pixel_double(struct ntv2_hdmiin *ntv2_hin, uint32_t pixels)
{
	return ntv2_hin->pixel_double_mode ? pixels * 2 : pixels;
}

static Ntv2Status ntv2_hdmiin_set_video_format(struct ntv2_hdmiin *ntv2_hin,
											   struct ntv2_hdmiin_format *format)
{
	uint32_t hdmiin_standard = ntv2_video_standard_to_hdmiin(format->video_standard);
	uint32_t hdmiin_rate = ntv2_frame_rate_to_hdmiin(format->frame_rate);
	uint32_t video_deep;
	uint32_t video_sd;
	uint32_t video_mode;
	uint32_t video_rgb;
	uint32_t video_map;
	uint32_t video_420;
	uint32_t video_uhd;
	uint32_t video_prog;
	uint32_t video_status;
	uint32_t h_sync_bp;
	uint32_t h_active;
	uint32_t h_blank;
	uint32_t v_sync_bp_fld1;
	uint32_t v_sync_bp_fld2;
	uint32_t v_active_fld1;
	uint32_t v_active_fld2;
	uint32_t video_setup;
	uint32_t horizontal_data;
	uint32_t hblank_data0;
	uint32_t hblank_data1;
	uint32_t vertical_data_fld1;
	uint32_t vertical_data_fld2;
	uint32_t input_status;
	uint32_t input_mask;
	uint32_t v_sync_offset_lines0 = 0;
	uint32_t v_sync_offset_lines1 = 0;
	uint32_t v_active_offset_lines0 = 0;
	uint32_t v_active_offset_lines1 = 0;

	/* good format ??? */
	if ((hdmiin_standard == ntv2_kona_hdmiin_video_standard_none) ||
		(hdmiin_rate == ntv2_kona_hdmiin_frame_rate_none))
		return NTV2_STATUS_FAIL;

	/* gather register field data */
	video_deep = (((format->pixel_flags & ntv2_kona_pixel_422) != 0) || 
		(((format->pixel_flags & ntv2_kona_pixel_444) != 0) &&
		((format->pixel_flags & ntv2_kona_pixel_8bit) == 0))) ? 1 : 0;

	video_sd = (format->video_standard == ntv2_video_standard_525i) ||
		(format->video_standard == ntv2_video_standard_625i)? 1 : 0;

	if ((format->frame_flags & ntv2_kona_frame_3g) != 0) {
		video_mode = ntv2_kona_hdmiin_video_mode_3gsdi;
	} else if ((format->frame_flags & ntv2_kona_frame_hd) != 0) {
		video_mode = ntv2_kona_hdmiin_video_mode_hdsdi;
	} else {
		video_mode = ntv2_kona_hdmiin_video_mode_sdsdi;
	}

	video_rgb = ((format->pixel_flags & ntv2_kona_pixel_rgb) != 0)? 1 : 0;

	video_map = ntv2_kona_hdmiin_video_map_444_10bit;
	if ((format->pixel_flags & ntv2_kona_pixel_444) == 0)
		video_map = ntv2_kona_hdmiin_video_map_422_10bit;

	video_420 = ((format->pixel_flags & ntv2_kona_pixel_420) != 0)? 1 : 0;

	video_uhd = (hdmiin_standard == ntv2_kona_hdmiin_video_standard_4k)? 1 : 0;

	video_prog = ((format->frame_flags & ntv2_kona_frame_transport_progressive) != 0)? 1 : 0;

	h_blank = ntv2_hin->h_front_porch_pixels +
		ntv2_hin->h_sync_pixels +
		ntv2_hin->h_back_porch_pixels;

	h_sync_bp = 0;
	h_active = 0;
	v_sync_bp_fld1 = 0;
	v_sync_bp_fld2 = 0;
	v_active_fld1 = 0;
	v_active_fld2 = 0;

	if (!ntv2_hin->uhd_mode) {
		h_sync_bp = ntv2_hdmiin_pixel_double(ntv2_hin,
											 ntv2_hin->h_sync_pixels +
											 ntv2_hin->h_back_porch_pixels);
										 
		h_active = ntv2_hdmiin_pixel_double(ntv2_hin, ntv2_hin->h_active_pixels);

		if (format->video_standard == ntv2_video_standard_525i)
		{
			v_sync_offset_lines0 = 1;
			v_sync_offset_lines1 = 2;
			v_active_offset_lines0 = 3;
			v_active_offset_lines1 = 3;
		}

		v_sync_bp_fld1 = ntv2_hdmiin_pixel_double(ntv2_hin,
												  ntv2_hin->h_total_pixels *
												  (ntv2_hin->v_sync_lines0 - 
												   v_sync_offset_lines0 +
												   ntv2_hin->v_back_porch_lines0) -
												  ntv2_hin->h_front_porch_pixels);

		v_sync_bp_fld2 = ntv2_hdmiin_pixel_double(ntv2_hin,
												  ntv2_hin->h_total_pixels *
												  (ntv2_hin->v_sync_lines1 -
												   v_sync_offset_lines1 +
												   ntv2_hin->v_back_porch_lines1) -
												  ntv2_hin->h_front_porch_pixels +
												  ntv2_hin->h_total_pixels/2);

		v_active_fld1 = ntv2_hdmiin_pixel_double(ntv2_hin,
												 (ntv2_hin->v_active_lines0 +
												  v_active_offset_lines0) *
												 ntv2_hin->h_total_pixels);

		v_active_fld2 = ntv2_hdmiin_pixel_double(ntv2_hin,
												 (ntv2_hin->v_active_lines1 +
												  v_active_offset_lines1) *
												 ntv2_hin->h_total_pixels);
	}										

	/* enable hdmi to fpga link */
	ntv2_konai2c_set_device(ntv2_hin, device_io_bank);
	ntv2_konai2c_write(ntv2_hin, tristate_reg, tristate_enable_outputs);

	/* setup fpga hdmi input data */
	video_setup = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_mode, video_mode);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_map, video_map);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_420, video_420);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_standard, hdmiin_standard);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_frame_rate, hdmiin_rate);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_3d_structure, ntv2_kona_hdmiin_3d_frame_packing);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_4k, video_uhd);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_progressive, video_prog);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_3d, 0);
	video_setup |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_3d_frame_pack_enable, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_video_setup, ntv2_hin->index, video_setup);

	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_hsync_duration, ntv2_hin->index, h_sync_bp);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_h_active, ntv2_hin->index, h_active);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_vsync_duration_fld1, ntv2_hin->index, v_sync_bp_fld1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_vsync_duration_fld2, ntv2_hin->index, v_sync_bp_fld2);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_v_active_fld1, ntv2_hin->index, v_active_fld1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_v_active_fld2, ntv2_hin->index, v_active_fld2);

	video_status = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_video_status, ntv2_hin->index);

	horizontal_data = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_h_total_pixels, ntv2_hin->h_total_pixels);
	horizontal_data |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_h_active_pixels, ntv2_hin->h_active_pixels);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_horizontal_data, ntv2_hin->index, horizontal_data);

	hblank_data0 = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_h_front_porch_pixels, ntv2_hin->h_front_porch_pixels);
	hblank_data0 |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_h_back_porch_pixels, ntv2_hin->h_back_porch_pixels);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_hblank_data0, ntv2_hin->index, hblank_data0);

	hblank_data1 = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_hsync_pixels, ntv2_hin->h_sync_pixels);
	hblank_data1 |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_hblank_pixels, h_blank);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_hblank_data1, ntv2_hin->index, hblank_data1);

	vertical_data_fld1 = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_v_total_lines, ntv2_hin->v_total_lines0);
	vertical_data_fld1 |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_v_active_lines, ntv2_hin->v_active_lines0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_vertical_data_fld1, ntv2_hin->index, vertical_data_fld1);

	vertical_data_fld2 = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_v_total_lines, ntv2_hin->v_total_lines1);
	vertical_data_fld2 |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_v_active_lines, ntv2_hin->v_active_lines1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_vertical_data_fld2, ntv2_hin->index, vertical_data_fld2);

	/* set fpga hdmi status */
	input_status = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_locked, (ntv2_hin->input_locked? 1 : 0));
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_stable, (ntv2_hin->input_locked? 1 : 0));
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_rgb, video_rgb);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_deep_color, video_deep);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_code, format->video_standard);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_audio_2ch, 0);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_progressive, video_prog);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_sd, video_sd);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_74_25, 0);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_audio_rate, 0);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_audio_word_length, 0);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_format, format->video_standard);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_dvi, (ntv2_hin->hdmi_mode? 0 : 1));
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_rate, format->frame_rate);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_input_status, ntv2_hin->index, input_status);

	input_status = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_color_space, ntv2_hin->color_space);
	input_mask = NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_color_space);
	input_status |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_color_depth, ntv2_hin->color_depth);
	input_mask |= NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_color_depth);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmi_control, ntv2_hin->index, input_status, input_mask);

	ntv2SpinLockAcquire(&ntv2_hin->state_lock);
	ntv2_hin->video_format = *format;
	ntv2SpinLockRelease(&ntv2_hin->state_lock);
#if 0
	NTV2_MSG_HDMIIN_STATE("%s: video setup            %08x\n", ntv2_hin->name, video_setup);
	NTV2_MSG_HDMIIN_STATE("%s: h sync                 %08x\n", ntv2_hin->name, h_sync_bp);
	NTV2_MSG_HDMIIN_STATE("%s: h active               %08x\n", ntv2_hin->name, h_active);
	NTV2_MSG_HDMIIN_STATE("%s: v sync fld 1           %08x\n", ntv2_hin->name, v_sync_bp_fld1);
	NTV2_MSG_HDMIIN_STATE("%s: v sync fld 2           %08x\n", ntv2_hin->name, v_sync_bp_fld2);
	NTV2_MSG_HDMIIN_STATE("%s: v active fld 1         %08x\n", ntv2_hin->name, v_active_fld1);
	NTV2_MSG_HDMIIN_STATE("%s: v active fld 2         %08x\n", ntv2_hin->name, v_active_fld2);
	NTV2_MSG_HDMIIN_STATE("%s: video status           %08x\n", ntv2_hin->name, video_status);
	NTV2_MSG_HDMIIN_STATE("%s: h active:total         %08x\n", ntv2_hin->name, horizontal_data);
	NTV2_MSG_HDMIIN_STATE("%s: h back:front           %08x\n", ntv2_hin->name, hblank_data0);
	NTV2_MSG_HDMIIN_STATE("%s: h blank:sync           %08x\n", ntv2_hin->name, hblank_data1);
	NTV2_MSG_HDMIIN_STATE("%s: v active:total fld 1   %08x\n", ntv2_hin->name, vertical_data_fld1);
	NTV2_MSG_HDMIIN_STATE("%s: v active:total fld 2   %08x\n", ntv2_hin->name, vertical_data_fld2);
#endif
//	NTV2_MSG_HDMIIN_STATE("%s: input status           %08x\n", ntv2_hin->name, input_status);

	return NTV2_STATUS_SUCCESS;
}

static void ntv2_hdmiin_set_aux_data(struct ntv2_hdmiin *ntv2_hin,
									 struct ntv2_hdmiin_format *format)
{
	uint32_t full_range = 0;
	uint32_t value = 0;
	uint32_t mask = 0;

	full_range = ((format->pixel_flags & ntv2_kona_pixel_full) != 0)? 1 : 0;

	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_full_range, full_range);
	mask |= NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_full_range);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmi_control, ntv2_hin->index, value, mask);
}

static void ntv2_hdmiin_set_no_video(struct ntv2_hdmiin *ntv2_hin)
{
	uint32_t value = 0;
	uint32_t mask = 0;

	/* disable hdmi to fpga link */
	ntv2_konai2c_set_device(ntv2_hin, device_io_bank);
	ntv2_konai2c_write(ntv2_hin, tristate_reg, tristate_disable_outputs);

	/* clear fpga hdmi input data */
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_video_setup, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_hsync_duration, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_h_active, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_vsync_duration_fld1, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_vsync_duration_fld2, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_v_active_fld1, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_v_active_fld2, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_horizontal_data, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_hblank_data0, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_hblank_data1, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_vertical_data_fld1, ntv2_hin->index, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_vertical_data_fld2, ntv2_hin->index, 0);

	/* clear fpga status */
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_locked, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_stable, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_rgb, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_deep_color, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_code, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_audio_2ch,	0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_progressive, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_sd, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_74_25, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_audio_rate, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_audio_word_length,	0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_format, 0);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_dvi, 1);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_video_rate, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_input_status, ntv2_hin->index, value);

	value = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_color_space, 0);
	mask = NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_color_space);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_color_depth, 0);
	mask |= NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_color_depth);
	value |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_full_range, 0);
	mask |= NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_full_range);
	ntv2_reg_rmw(ntv2_hin->system_context, ntv2_reg_hdmi_control, ntv2_hin->index, value, mask);

	ntv2SpinLockAcquire(&ntv2_hin->state_lock);
	ntv2_video_format_init(&ntv2_hin->video_format);
	ntv2SpinLockRelease(&ntv2_hin->state_lock);
}

static void ntv2_konai2c_set_device(struct ntv2_hdmiin *ntv2_hin, uint8_t device)
{
	uint32_t val;

	if (ntv2_hin == NULL)
		return;

	ntv2_hin->i2c_device = device;

	val = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_device_address, ntv2_hin->i2c_device);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_read_disable, 1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index, val);
}
#if 0
static uint8_t ntv2_konai2c_get_device(struct ntv2_hdmiin *ntv2_hin)
{
	if (ntv2_hin == NULL)
		return 0;

	return ntv2_hin->i2c_device;
}
#endif
static Ntv2Status ntv2_konai2c_write(struct ntv2_hdmiin *ntv2_hin, uint8_t address, uint8_t data)
{
	uint32_t val;
	Ntv2Status res;

	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_KONAI2C_WRITE("%s: write dev %02x  add %02x  data %02x\n",
						   ntv2_hin->name, ntv2_hin->i2c_device, address, data);

	res = ntv2_konai2c_wait_for_busy(ntv2_hin, NTV2_I2C_BUSY_TIMEOUT);
	if (res != NTV2_STATUS_SUCCESS) {
		return res;
	}

	val = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_device_address, ntv2_hin->i2c_device);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_subaddress, address);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_read_disable, 1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index, val);

	val = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_data_out, data);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_data, ntv2_hin->index, val);

	res = ntv2_konai2c_wait_for_write(ntv2_hin, NTV2_I2C_WRITE_TIMEOUT);
	if (res != NTV2_STATUS_SUCCESS) {
		return res;
	}

	if ((ntv2_hin->i2c_device == device_hdmi_bank) && (address == hdmi_hpa_reg)) {
		ntv2_hin->i2c_hpa_default = data;
	}
	if ((ntv2_hin->i2c_device == device_io_bank) && (address == io_color_reg)) {
		ntv2_hin->i2c_color_default = data;
	}

	return NTV2_STATUS_SUCCESS;
}

static Ntv2Status ntv2_konai2c_cache_update(struct ntv2_hdmiin *ntv2_hin)
{
	uint32_t val;
	Ntv2Status res;

	if (ntv2_hin == NULL)
		return NTV2_STATUS_BAD_PARAMETER;

	NTV2_MSG_KONAI2C_READ("%s: update device %02x read cache\n",
						  ntv2_hin->name, ntv2_hin->i2c_device);

	res = ntv2_konai2c_wait_for_busy(ntv2_hin, NTV2_I2C_BUSY_TIMEOUT);
	if (res != NTV2_STATUS_SUCCESS)
		return res;

	/* enable i2c reads */
	val = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_device_address, ntv2_hin->i2c_device);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_subaddress, device_subaddress_all);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_read_disable, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index, val);

	res = ntv2_konai2c_wait_for_read(ntv2_hin, NTV2_I2C_READ_TIMEOUT);
	if (res != NTV2_STATUS_SUCCESS)
		return res;

	/* disable i2c reads */
	val = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_device_address, ntv2_hin->i2c_device);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_subaddress, device_subaddress_all);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_read_disable, 1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index, val);

	res = ntv2_konai2c_wait_for_busy(ntv2_hin, NTV2_I2C_BUSY_TIMEOUT);
	if (res != NTV2_STATUS_SUCCESS)
		return res;

	return NTV2_STATUS_SUCCESS;
}

static uint8_t ntv2_konai2c_cache_read(struct ntv2_hdmiin *ntv2_hin, uint8_t address)
{
	uint32_t val;
	uint32_t data;
	Ntv2Status res;

	if (ntv2_hin == NULL)
		return 0;

	res = ntv2_konai2c_wait_for_busy(ntv2_hin, NTV2_I2C_BUSY_TIMEOUT);
	if (res != NTV2_STATUS_SUCCESS) {
		NTV2_MSG_KONAI2C_ERROR("%s: *error* read dev %02x  address %02x  failed\n",
							   ntv2_hin->name, ntv2_hin->i2c_device, address);
		return 0;
	}

	val = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_device_address, ntv2_hin->i2c_device);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_subaddress, address);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_read_disable, 1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index, val);

	val = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_data, ntv2_hin->index);
	data = NTV2_FLD_GET(ntv2_kona_fld_hdmiin_data_in, val);

	NTV2_MSG_KONAI2C_READ("%s: read  dev %02x  add %02x  data %02x\n",
						  ntv2_hin->name, ntv2_hin->i2c_device, address, data);
	return (uint8_t)data;
}
#if 0
static Ntv2Status ntv2_konai2c_rmw(struct ntv2_hdmiin *ntv2_hin, uint8_t address, uint8_t data, uint8_t mask)
{
	uint8_t val;

	val = ntv2_konai2c_cache_read(ntv2_hin, address);
	val = (val & (~mask)) | (data & mask);
	return ntv2_konai2c_write(ntv2_hin, address, val);
}
#endif
static Ntv2Status ntv2_konai2c_wait_for_busy(struct ntv2_hdmiin *ntv2_hin, uint32_t timeout)
{
	uint32_t val;
	uint32_t mask = NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_i2c_busy);
	int count = timeout / NTV2_I2C_WAIT_TIME;
	int i;

	if (timeout == 0) {
		val = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index);
		return ((val & mask) == 0)? 0 : NTV2_STATUS_BUSY;
	}

	for (i = 0; i < count; i++) {
		val = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index);
		if ((val & mask) == 0)
			return NTV2_STATUS_SUCCESS;
		ntv2EventWaitForSignal(&ntv2_hin->monitor_event, NTV2_I2C_WAIT_TIME, true);
	}
	ntv2_konai2c_reset(ntv2_hin);
	NTV2_MSG_KONAI2C_ERROR("%s: *error* wait for i2c busy failed - reset count %d\n",
						   ntv2_hin->name, ntv2_hin->i2c_reset_count);
	return NTV2_STATUS_TIMEOUT;
}

static Ntv2Status ntv2_konai2c_wait_for_write(struct ntv2_hdmiin *ntv2_hin, uint32_t timeout)
{
	uint32_t val;
	uint32_t mask = NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_write_busy);
	int count = timeout / NTV2_I2C_WAIT_TIME;
	int i;

	if (timeout == 0) {
		val = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index);
		return ((val & mask) == 0)? 0 : NTV2_STATUS_BUSY;
	}

	for (i = 0; i < count; i++) {
		val = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index);
		if ((val & mask) == 0)
			return NTV2_STATUS_SUCCESS;
		ntv2EventWaitForSignal(&ntv2_hin->monitor_event, NTV2_I2C_WAIT_TIME, true);
	}
	ntv2_konai2c_reset(ntv2_hin);
	NTV2_MSG_KONAI2C_ERROR("%s: *error* wait for i2c write failed - reset count %d\n",
						   ntv2_hin->name, ntv2_hin->i2c_reset_count);
	return NTV2_STATUS_TIMEOUT;
}

static Ntv2Status ntv2_konai2c_wait_for_read(struct ntv2_hdmiin *ntv2_hin, uint32_t timeout)
{
	uint32_t val;
	uint32_t mask = NTV2_FLD_MASK(ntv2_kona_fld_hdmiin_ram_data_ready);
	int count = timeout / NTV2_I2C_WAIT_TIME;
	int i;

	if (timeout == 0) {
		val = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index);
		return ((val & mask) != 0)? 0 : NTV2_STATUS_BUSY;
	}

	for (i = 0; i < count; i++) {
		val = ntv2_reg_read(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index);
		if ((val & mask) != 0)
			return NTV2_STATUS_SUCCESS;
		ntv2EventWaitForSignal(&ntv2_hin->monitor_event, NTV2_I2C_READ_TIME, true);
	}
	ntv2_konai2c_reset(ntv2_hin);
	NTV2_MSG_KONAI2C_ERROR("%s: *error* wait for i2c read failed - reset count %d\n",
						   ntv2_hin->name, ntv2_hin->i2c_reset_count);
	return NTV2_STATUS_TIMEOUT;
}

static void ntv2_konai2c_reset(struct ntv2_hdmiin *ntv2_hin)
{
	uint32_t val;

	ntv2_hin->i2c_reset_count++;

	/* set reset */
	val = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_device_address, 0);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_subaddress, 0);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_read_disable, 1);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_i2c_reset, 1);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index, val);

	ntv2EventWaitForSignal(&ntv2_hin->monitor_event, NTV2_I2C_RESET_TIME, true);

	/* clear reset */
	val = NTV2_FLD_SET(ntv2_kona_fld_hdmiin_device_address, 0);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_subaddress, 0);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_read_disable, 1);
	val |= NTV2_FLD_SET(ntv2_kona_fld_hdmiin_i2c_reset, 0);
	ntv2_reg_write(ntv2_hin->system_context, ntv2_kona_reg_hdmiin_i2c_control, ntv2_hin->index, val);

	ntv2EventWaitForSignal(&ntv2_hin->monitor_event, NTV2_I2C_RESET_TIME, true);
}

static void ntv2_update_debug_flags(struct ntv2_hdmiin *ntv2_hin)
{
	uint32_t val;

	val = ntv2_reg_read(ntv2_hin->system_context, ntv2_reg_hdmi_control, 0);
	val = NTV2_FLD_GET(ntv2_kona_fld_hdmi_debug, val);
	if (val != 0)
	{
		ntv2_user_mask = NTV2_DEBUG_HDMIIN_STATE | NTV2_DEBUG_ERROR;
	}
	else
	{
		ntv2_user_mask = 0;
	}
}

static uint32_t ntv2_video_standard_to_hdmiin(uint32_t video_standard)
{
	if (video_standard >= NTV2_MAX_VIDEO_STANDARDS)
		return 0;

	return video_standard_to_hdmi[video_standard];
}

static uint32_t ntv2_frame_rate_to_hdmiin(uint32_t frame_rate)
{
	if (frame_rate >= NTV2_MAX_FRAME_RATES)
		return 0;

	return frame_rate_to_hdmi[frame_rate];
}

static void ntv2_video_format_init(struct ntv2_hdmiin_format *format)
{
	if (format == NULL)
		return;
	
	format->video_standard = ntv2_video_standard_none;
	format->frame_rate = ntv2_frame_rate_none;
	format->frame_flags = 0;
	format->pixel_flags = 0;
}

static bool ntv2_video_format_compare(struct ntv2_hdmiin_format *format_a,
									  struct ntv2_hdmiin_format *format_b)
{
	if ((format_a == NULL) || (format_b == NULL))
		return false;

	if ((format_a->video_standard == format_b->video_standard) &&
		(format_a->frame_rate == format_b->frame_rate) &&
		(format_a->frame_flags == format_b->frame_flags) &&
		(format_a->pixel_flags == format_b->pixel_flags))
		return true;

	return false;
}
