/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
#ifndef NTV2_HINREG_H
#define NTV2_HINREG_H

#include "ntv2commonreg.h"


/* video frame flags */
NTV2_CON(ntv2_kona_frame_none,						0x00000000);
NTV2_CON(ntv2_kona_frame_picture_progressive,		0x00000001);	/* picture progressive */
NTV2_CON(ntv2_kona_frame_picture_interlaced,		0x00000002);	/* picture interlaced */
NTV2_CON(ntv2_kona_frame_transport_progressive,		0x00000004);	/* transport progressive */
NTV2_CON(ntv2_kona_frame_transport_interlaced,		0x00000008);	/* transport interlaced */
NTV2_CON(ntv2_kona_frame_sd,						0x00000010);	/* clock sd */
NTV2_CON(ntv2_kona_frame_hd,						0x00000020);	/* clock hd */
NTV2_CON(ntv2_kona_frame_3g,						0x00000040);	/* clock 3g */
NTV2_CON(ntv2_kona_frame_3ga,						0x00000100);	/* sdi transport 3ga */
NTV2_CON(ntv2_kona_frame_3gb,						0x00000200);	/* sdi transport 3gb */
NTV2_CON(ntv2_kona_frame_dual_link,					0x00000400);	/* sdi transport smpte 372 4444 */
NTV2_CON(ntv2_kona_frame_line_interleaved,			0x00000800);	/* sdi transport smpte 372 >30 fps */
NTV2_CON(ntv2_kona_frame_square_division,			0x00001000);	/* transport square division */
NTV2_CON(ntv2_kona_frame_sample_interleaved,		0x00002000);	/* transport sample interleaved */
NTV2_CON(ntv2_kona_frame_4x3,						0x00100000);	/* 4x3 aspect */
NTV2_CON(ntv2_kona_frame_16x9,						0x00200000);	/* 16x9 aspect */

/* video pixel flags */
NTV2_CON(ntv2_kona_pixel_none,						0x00000000);
NTV2_CON(ntv2_kona_pixel_yuv,						0x00000001);	/* yuv color space */
NTV2_CON(ntv2_kona_pixel_rgb,						0x00000002);	/* rgb color space */
NTV2_CON(ntv2_kona_pixel_full,						0x00000004);	/* full range black - white */
NTV2_CON(ntv2_kona_pixel_smpte,						0x00000008);	/* smpte range black - white */
NTV2_CON(ntv2_kona_pixel_rec601,					0x00000010);	/* rec 601 color standard */
NTV2_CON(ntv2_kona_pixel_rec709,					0x00000020);	/* rec 709 color standard */
NTV2_CON(ntv2_kona_pixel_rec2020,					0x00000040);	/* rec 2020 color standard */
NTV2_CON(ntv2_kona_pixel_adobe,						0x00000080);	/* adobe color standard */
NTV2_CON(ntv2_kona_pixel_420,						0x00000100);	/* 420 component format */
NTV2_CON(ntv2_kona_pixel_422,						0x00000200);	/* 422 component format */
NTV2_CON(ntv2_kona_pixel_444,						0x00000400);	/* 444 component format */
NTV2_CON(ntv2_kona_pixel_4444,						0x00000800);	/* 4444 component format */
NTV2_CON(ntv2_kona_pixel_4224,						0x00001000);	/* 4224 component format */
NTV2_CON(ntv2_kona_pixel_8bit,						0x00010000);	/* 8 bit component resolution */
NTV2_CON(ntv2_kona_pixel_10bit,						0x00020000);	/* 10 bit component resolution */
NTV2_CON(ntv2_kona_pixel_12bit,						0x00040000);	/* 12 bit component resolution */
NTV2_CON(ntv2_kona_pixel_16bit,						0x00080000);	/* 16 bit component resolution */

/* hdmi input status */
NTV2_REG(ntv2_kona_reg_hdmiin_input_status,				126, 0x2c13, 0x3013);		/* hdmi input status register */
	NTV2_FLD(ntv2_kona_fld_hdmiin_locked,					1,	0);		
	NTV2_FLD(ntv2_kona_fld_hdmiin_stable,					1,	1);		
	NTV2_FLD(ntv2_kona_fld_hdmiin_rgb,						1,	2);		
	NTV2_FLD(ntv2_kona_fld_hdmiin_deep_color,				1,	3);		
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_code,				6,	4);		/* ntv2 video standard v2 */
	NTV2_FLD(ntv2_kona_fld_hdmiin_lhi_ycbcr_mode,			1,	10);		
	NTV2_FLD(ntv2_kona_fld_hdmiin_lhi_10bit_mode,			1,	11);		
	NTV2_FLD(ntv2_kona_fld_hdmiin_audio_2ch,				1,	12);	/* 2 audio channels (vs 8) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_progressive,				1,	13);	
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_sd,					1,	14);	/* video pixel clock sd (not hd or 3g) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_74_25,				1,	15);	/* not used */
	NTV2_FLD(ntv2_kona_fld_hdmiin_audio_rate,				4,	16);	
	NTV2_FLD(ntv2_kona_fld_hdmiin_audio_word_length,		4,	20);	
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_format,				3,	24);	/* really ntv2 standard */
	NTV2_FLD(ntv2_kona_fld_hdmiin_dvi,						1,	27);	/* input dvi (vs hdmi) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_rate,				4,	28);	/* ntv2 video rate */

/* hdmi control */
NTV2_REG(ntv2_reg_hdmi_control,							127, 0x2c14, 0x3014);		/* hdmi audio status register */
	NTV2_FLD(ntv2_kona_fld_hdmiout_force_config,			1,	1);		/* force output config (ignore edid) */	
	NTV2_FLD(ntv2_kona_fld_hdmiin_audio_pair,				2,	2);		/* hdmi input audio pair select */	
	NTV2_FLD(ntv2_kona_fld_hdmiin_rate_convert_enable,		1,	4);		/* hdmi input audio sample rate converter enable */	
	NTV2_FLD(ntv2_kona_fld_hdmiin_channel34_swap_disable,	1,	5);		/* hdmi input audio channel 3/4 swap disable */	
	NTV2_FLD(ntv2_kona_fld_hdmiout_channel34_swap_disable,	1,	6);		/* hdmi output audio channel 3/4 swap disable */	
	NTV2_FLD(ntv2_kona_fld_hdmiout_prefer_420,				1,	7);		/* hdmi output prefer 4K/UHD 420 */	
	NTV2_FLD(ntv2_kona_fld_hdmiout_audio_format,			2,	8);		/* hdmi output audio format */
	NTV2_FLD(ntv2_kona_fld_hdmiin_color_depth,				2,	12);	/* hdmi input bit depth */	
	NTV2_FLD(ntv2_kona_fld_hdmiin_color_space,				2,	14);	/* hdmi input color space */
	NTV2_FLD(ntv2_kona_fld_hdmi_polarity,					4,	16);	/* hdmi polarity? */	
	NTV2_FLD(ntv2_kona_fld_hdmiout_source_select,			4,	20);	/* output audio source select */	
	NTV2_FLD(ntv2_kona_fld_hdmiout_crop_enable,				1,	24);	/* crop 2k -> hd  4k -> uhd */
	NTV2_FLD(ntv2_kona_fld_hdmiout_force_hpd,				1,	25);	/* force hpd */
	NTV2_FLD(ntv2_kona_fld_hdmiout_deep_12bit,				1,	26);	/* deep color 12 bit */
	NTV2_FLD(ntv2_kona_fld_hdmi_debug,						1,	27);	/* debug output enable */
	NTV2_FLD(ntv2_kona_fld_hdmi_disable_update,				1,	28);	/* disable update loop */
	NTV2_FLD(ntv2_kona_fld_hdmiout_channel_select,			2,	29);	/* output audio channel select */	
	NTV2_FLD(ntv2_kona_fld_hdmi_protocol,					1,	30);	/* hdmi protocol? */	
	NTV2_FLD(ntv2_kona_fld_hdmiin_full_range,				1,	31);	/* hdmi input quantization full range */	

/* hdmi input video mode */
NTV2_CON(ntv2_kona_hdmiin_video_mode_hdsdi,			0x0);		/* hd-sdi */
NTV2_CON(ntv2_kona_hdmiin_video_mode_sdsdi,			0x1);		/* sd_sdi */
NTV2_CON(ntv2_kona_hdmiin_video_mode_3gsdi,			0x2);		/* 3g-sdi */

/* hdmi input video map */
NTV2_CON(ntv2_kona_hdmiin_video_map_422_10bit,		0x0);		/* yuv 422 10 bit */
NTV2_CON(ntv2_kona_hdmiin_video_map_444_10bit,		0x1);		/* yuv/rgb 444 10 bit */

/* hdmi input video standard */
NTV2_CON(ntv2_kona_hdmiin_video_standard_1080i,		0x0);		/* 1080i */
NTV2_CON(ntv2_kona_hdmiin_video_standard_720p,		0x1);		/* 720p */
NTV2_CON(ntv2_kona_hdmiin_video_standard_525i,		0x2);		/* 525i */
NTV2_CON(ntv2_kona_hdmiin_video_standard_625i,		0x3);		/* 625i */
NTV2_CON(ntv2_kona_hdmiin_video_standard_1080p,		0x4);		/* 1080p */
NTV2_CON(ntv2_kona_hdmiin_video_standard_4k,		0x5);		/* 4K */
NTV2_CON(ntv2_kona_hdmiin_video_standard_2205p,		0x6);		/* 3D frame packed mode */
NTV2_CON(ntv2_kona_hdmiin_video_standard_none,		0x7);		/* undefined */

/* hdmi input frame rate */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_none,			0x0);		/* undefined */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_6000,			0x1);		/* 60.00 */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_5994,			0x2);		/* 59.94 */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_3000,			0x3);		/* 30.00 */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_2997,			0x4);		/* 29.97 */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_2500,			0x5);		/* 25.00 */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_2400,			0x6);		/* 24.00 */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_2398,			0x7);		/* 23.98 */
NTV2_CON(ntv2_kona_hdmiin_frame_rate_5000,			0x8);		/* 50.00 */

/* hdmi 3d structure */
NTV2_CON(ntv2_kona_hdmiin_3d_frame_packing,	    	0x0);		/* 0000 frame packing */
NTV2_CON(ntv2_kona_hdmiin_3d_field_alternative,		0x1);		/* 0001 field alternative */
NTV2_CON(ntv2_kona_hdmiin_3d_line_alternative,		0x2);		/* 0010 line alternative */
NTV2_CON(ntv2_kona_hdmiin_3d_side_by_side_full,		0x3);		/* 0011 side by side full */
NTV2_CON(ntv2_kona_hdmiin_3d_l_depth,				0x4);		/* 0100 L + depth */
NTV2_CON(ntv2_kona_hdmiin_3d_l_d_g,					0x5);		/* 0101 L + depth + graphics -depth */
NTV2_CON(ntv2_kona_hdmiin_3d_top_bottom,			0x6);		/* 0110 top bottom */
NTV2_CON(ntv2_kona_hdmiin_3d_side_by_side_half,		0x8);		/* 1000 side by side half */

/* hdmi input control */
NTV2_REG(ntv2_kona_reg_hdmiin_i2c_control,			360, 0x2c00, 0x3000);		/* hdmi input i2c control register */
	NTV2_FLD(ntv2_kona_fld_hdmiin_subaddress,			8,	0);		/* i2c subaddress (8-bit register on device) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_device_address,		7,	8);		/* i2c device address (hdmiin_addr) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_read_disable,			1,	16);	/* i2c read disable bit */
	NTV2_FLD(ntv2_kona_fld_hdmiin_write_busy,			1,	20);	/* i2c write busy bit */
	NTV2_FLD(ntv2_kona_fld_hdmiin_i2c_error,			1,	21);	/* i2c error bit */
	NTV2_FLD(ntv2_kona_fld_hdmiin_i2c_busy,				1,	22);	/* i2c busy bit */
	NTV2_FLD(ntv2_kona_fld_hdmiin_i2c_reset,			1,	24);	/* i2c reset bit */
	NTV2_FLD(ntv2_kona_fld_hdmiin_ram_data_ready,		1,	28);	/* i2c ram data ready bit */

NTV2_REG(ntv2_kona_reg_hdmiin_i2c_data,				361, 0x2c01, 0x3001);		/* hdmi input data register */
	NTV2_FLD(ntv2_kona_fld_hdmiin_data_out,				8,	0);		/* i2c data to write to selected subaddress */
	NTV2_FLD(ntv2_kona_fld_hdmiin_data_in,				8,	8);		/* i2c data read from selected subaddress */

NTV2_REG(ntv2_kona_reg_hdmiin_video_setup,			362, 0x2c02, 0x3002);		/* hdmi input video setup regiser */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_mode,			2,	0);		/* video mode (hdmiin_video_mode) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_map,			2,	2);		/* video map (hdmiin_video_map) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_420,			1,	4);		/* 420 video input */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_standard,		3,	8);		/* video standard select (hdmiin_video_standard) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_frame_rate,			4,	16);	/* frame rate select (Hz) (hdmiin_frame_rate) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_3d_structure,			4,	20);	/* 3D frame structure (hdmi_3d) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_4k,				1,	28);	/* 4K video input */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_progressive,	1,	29);	/* progressive video input */
	NTV2_FLD(ntv2_kona_fld_hdmiin_video_3d,				1,	30);	/* 3D video input */
	NTV2_FLD(ntv2_kona_fld_hdmiin_3d_frame_pack_enable,	1,	31);	/* enable special 3D frame-packed mode */

NTV2_REG(ntv2_kona_reg_hdmiin_hsync_duration,		363, 0x2c03, 0x3003);		/* hdmi input horizontal sync and back porch regiser */
NTV2_REG(ntv2_kona_reg_hdmiin_h_active,				364, 0x2c04, 0x3004);		/* hdmi input horizontal active regiser */
NTV2_REG(ntv2_kona_reg_hdmiin_vsync_duration_fld1,	365, 0x2c05, 0x3005);		/* hdmi input vertical sync and back porch regiser, field 1 */
NTV2_REG(ntv2_kona_reg_hdmiin_vsync_duration_fld2,	366, 0x2c06, 0x3006);		/* hdmi input vertical sync and back porch regiser, field 2 */
NTV2_REG(ntv2_kona_reg_hdmiin_v_active_fld1,		367, 0x2c07, 0x3007);		/* hdmi input vertical active regiser, field 1 */
NTV2_REG(ntv2_kona_reg_hdmiin_v_active_fld2,		368, 0x2c08, 0x3008);		/* hdmi input vertical active regiser, field 2 */

NTV2_REG(ntv2_kona_reg_hdmiin_video_status,			369, 0x2c09, 0x3009);		/* hdmi input video status regiser 1 */
	NTV2_FLD(ntv2_kona_fld_hdmiin_det_frame_rate,		4,	0);		/* detected frame rate (hdmiin_frame_rate) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_det_video_standard,	3,	8);		/* detected video standard (hdmiin_video_standard) */
	NTV2_FLD(ntv2_kona_fld_hdmiin_ident_valid,			1,	16);	/* identification valid */
	NTV2_FLD(ntv2_kona_fld_hdmiin_hv_locked,			1,	17);	/* HV Locked */
	NTV2_FLD(ntv2_kona_fld_hdmiin_hd_74mhz,				1,	18);	/* HD 74.xx vs 27 Mhz clock */
	NTV2_FLD(ntv2_kona_fld_hdmiin_det_progressive,		1,	19);	/* detected progressive */

NTV2_REG(ntv2_kona_reg_hdmiin_horizontal_data,		370, 0x2c0a, 0x300a);		/* hdmi input H pixel data */
	NTV2_FLD(ntv2_kona_fld_hdmiin_h_total_pixels,		16,	0);		/* H total pixels per line */
	NTV2_FLD(ntv2_kona_fld_hdmiin_h_active_pixels,		16,	16);	/* H active pixels per line */

NTV2_REG(ntv2_kona_reg_hdmiin_hblank_data0,			371, 0x2c0b, 0x300b);		/* hdmi input H blanking data */
	NTV2_FLD(ntv2_kona_fld_hdmiin_h_front_porch_pixels,	16,	0);		/* H front porch pixels */
	NTV2_FLD(ntv2_kona_fld_hdmiin_h_back_porch_pixels,	16,	16);	/* H back porch pixels */

NTV2_REG(ntv2_kona_reg_hdmiin_hblank_data1,			372, 0x2c0c, 0x300c);		/* hdmi input H Blanking data */
	NTV2_FLD(ntv2_kona_fld_hdmiin_hsync_pixels,			16,	0);		/* H sync pixels */
	NTV2_FLD(ntv2_kona_fld_hdmiin_hblank_pixels,		16,	16);	/* H blank pixels */

NTV2_REG(ntv2_kona_reg_hdmiin_vertical_data_fld1,	373, 0x2c0c, 0x300d);		/* hdmi input field 1 V data */
NTV2_REG(ntv2_kona_reg_hdmiin_vertical_data_fld2,	374, 0x2c0e, 0x300e);		/* hdmi input field 2 V data */
	NTV2_FLD(ntv2_kona_fld_hdmiin_v_total_lines,		16,	0);		/* V total lines field 1,2 */
	NTV2_FLD(ntv2_kona_fld_hdmiin_v_active_lines,		16,	16);	/* V active lines field 1,2 */

NTV2_REG(ntv2_kona_reg_hdmiin_color_depth,			375, 0x2c0f, 0x300f);		/* hdmi input color depth */
	NTV2_FLD(ntv2_kona_fld_hdmiin_deep_color_detect,	1,	6);		/* detected deep color */

/* i2c register / value data */
struct ntv2_reg_value {
	uint8_t address;
	uint8_t value;
};

/* hdmi i2c device addresses */
static const uint8_t device_io_bank						= 0x4c;		/* hdmi chip io register bank */
static const uint8_t device_hdmi_bank					= 0x34;		/* hdmi chip hdmi register bank */
static const uint8_t device_cec_bank					= 0x40;		/* hdmi chip cec register bank */
static const uint8_t device_cp_bank						= 0x22;		/* hdmi chip cp register bank */
static const uint8_t device_repeater_bank				= 0x32;		/* hdmi chip repeater register bank */
static const uint8_t device_edid_bank					= 0x36;		/* hdmi chip edid register bank */
static const uint8_t device_dpll_bank					= 0x26;		/* hdmi chip dpll register bank */
static const uint8_t device_info_bank					= 0x3e;		/* hdmi chip info frame register bank */

static const uint8_t	device_subaddress_all			= 0xff;

/* hdmi i2c data registers and bit masks */
static const uint8_t packet_detect_reg					= 0x60;
static const uint8_t packet_detect_avi_mask				= 0x01;
static const uint8_t packet_detect_vsi_mask				= 0x10;

static const uint8_t clock_detect_reg					= 0x6a;
static const uint8_t clock_tmdsa_present_mask			= 0x10;
static const uint8_t clock_tmdsa_lock_mask				= 0x40;
static const uint8_t clock_sync_lock_mask				= 0x02;
static const uint8_t clock_regen_lock_mask				= 0x01;

static const uint8_t tmds_lock_detect_reg				= 0x6b;
static const uint8_t tmds_lock_detect_mask				= 0x40;

static const uint8_t tmds_lock_clear_reg				= 0x6c;
static const uint8_t tmds_lock_clear_mask				= 0x40;

static const uint8_t cable_detect_reg					= 0x6f;
static const uint8_t cable_detect_mask					= 0x01;

static const uint8_t tmds_frequency_detect_reg			= 0x83;
static const uint8_t tmds_frequency_detect_mask			= 0x02;

static const uint8_t tmds_frequency_clear_reg			= 0x85;
static const uint8_t tmds_frequency_clear_mask			= 0x02;

static const uint8_t io_color_reg						= 0x02;
static const uint8_t io_color_space_mask				= 0x06;

static const uint8_t hdmi_hpa_reg						= 0x6c;
static const uint8_t hdmi_hpa_manual_mask				= 0x01;

static const uint8_t hdmi_mode_reg						= 0x05;
static const uint8_t hdmi_mode_mask						= 0x80;
static const uint8_t hdmi_encrypted_mask				= 0x40;

static const uint8_t deep_color_mode_reg				= 0x0b;
static const uint8_t deep_color_10bit_mask				= 0x40;
static const uint8_t deep_color_12bit_mask				= 0x80;

static const uint8_t derep_mode_reg						= 0x41;
static const uint8_t derep_mode_mask					= 0x1f;

static const uint8_t defilter_lock_detect_reg			= 0x07;
static const uint8_t defilter_locked_mask				= 0x20;         
static const uint8_t vfilter_locked_mask				= 0x80;         

static const uint8_t interlaced_detect_reg				= 0x0b;
static const uint8_t interlaced_mask					= 0x20;

static const uint8_t tristate_reg						= 0x15;
static const uint8_t tristate_disable_outputs			= 0x9e;
static const uint8_t tristate_enable_outputs			= 0x80;         

static const uint8_t vsi_infoframe_packet_id			= 0xec;
static const uint8_t vsi_infoframe_version				= 0xed;
static const uint8_t vsi_infoframe_length				= 0xee;
static const uint8_t vsi_infoframe_checksum				= 0x54;
static const uint8_t vsi_infoframe_byte1				= 0x55;

static const uint8_t vsi_video_format_mask4				= 0xe0;
static const uint8_t vsi_video_format_shift4			= 0x05;
static const uint8_t vsi_3d_structure_mask5				= 0xf0;
static const uint8_t vsi_3d_structure_shift5			= 0x04;

static const uint8_t avi_infoframe_packet_id			= 0xe0;
static const uint8_t avi_infoframe_version				= 0xe1;
static const uint8_t avi_infoframe_length				= 0xe2;
static const uint8_t avi_infoframe_checksum				= 0x00;
static const uint8_t avi_infoframe_byte1				= 0x01;

static const uint8_t avi_scan_data_mask1				= 0x03;
static const uint8_t avi_scan_data_shift1				= 0x00;
static const uint8_t avi_bar_data_mask1					= 0x0c;
static const uint8_t avi_bar_data_shift1				= 0x02;
static const uint8_t avi_active_format_mask1			= 0x10;
static const uint8_t avi_active_format_shift1			= 0x04;
static const uint8_t avi_color_component_mask1			= 0x60;
static const uint8_t avi_color_component_shift1			= 0x05;
static const uint8_t avi_active_aspect_mask2			= 0x0f;
static const uint8_t avi_active_aspect_shift2			= 0x00;
static const uint8_t avi_frame_aspect_ratio_mask2		= 0x30;
static const uint8_t avi_frame_aspect_ratio_shift2		= 0x04;
static const uint8_t avi_colorimetry_mask2				= 0xc0;
static const uint8_t avi_colorimetry_shift2				= 0x06;
static const uint8_t avi_nonuniform_scaling_mask3		= 0x03;
static const uint8_t avi_nonuniform_scaling_shift3		= 0x00;
static const uint8_t avi_quantization_range_mask3		= 0x0c;
static const uint8_t avi_quantization_range_shift3		= 0x02;
static const uint8_t avi_extended_colorimetry_mask3		= 0x70;
static const uint8_t avi_extended_colorimetry_shift3	= 0x04;
static const uint8_t avi_it_content_mask3				= 0x80;
static const uint8_t avi_it_content_shift3				= 0x07;
static const uint8_t avi_vic_mask4						= 0x7f;
static const uint8_t avi_vic_shift4						= 0x00;
static const uint8_t avi_pixel_repetition_mask5			= 0x0f;
static const uint8_t avi_pixel_repetition_shift5		= 0x00;
static const uint8_t avi_it_content_type_mask5			= 0x30;
static const uint8_t avi_it_content_type_shift5			= 0x04;
static const uint8_t avi_ycc_quant_range_mask5			= 0xc0;
static const uint8_t avi_ycc_quant_range_shift5			= 0x06;

/* info frame data values */
static const uint8_t vsi_packet_id						= 0x81;
static const uint8_t vsi_version						= 0x01;

static const uint8_t vsi_format_none					= 0x00;
static const uint8_t vsi_format_extended				= 0x01;
static const uint8_t vsi_format_3d						= 0x02;

static const uint8_t vsi_vic_reserved					= 0x00;
static const uint8_t vsi_vic_3840x2160_30				= 0x01;
static const uint8_t vsi_vic_3840x2160_25				= 0x02;
static const uint8_t vsi_vic_3840x2160_24				= 0x03;
static const uint8_t vsi_vic_4096x2160_24				= 0x04;

static const uint8_t avi_packet_id						= 0x82;
static const uint8_t avi_version						= 0x02;

static const uint8_t avi_scan_nodata					= 0x00;
static const uint8_t avi_scan_ovderscanned				= 0x01;
static const uint8_t avi_scan_underscanned				= 0x02;
static const uint8_t avi_scan_future					= 0x03;

static const uint8_t avi_bar_nodata						= 0x00;
static const uint8_t avi_bar_vertical					= 0x01;
static const uint8_t avi_bar_horizontal					= 0x02;
static const uint8_t avi_bar_both						= 0x03;

static const uint8_t avi_color_comp_rgb					= 0x00;
static const uint8_t avi_color_comp_422					= 0x01;
static const uint8_t avi_color_comp_444					= 0x02;
static const uint8_t avi_color_comp_420					= 0x03;

static const uint8_t avi_frame_aspect_nodata			= 0x00;
static const uint8_t avi_frame_aspect_4x3				= 0x01;
static const uint8_t avi_frame_aspect_16x9				= 0x02;
static const uint8_t avi_frame_aspect_future			= 0x03;

static const uint8_t avi_colorimetry_nodata				= 0x00;
static const uint8_t avi_colorimetry_smpte170m			= 0x01;
static const uint8_t avi_colorimetry_itu_r709			= 0x02;
static const uint8_t avi_colorimetry_extended			= 0x03;

static const uint8_t avi_active_aspect_nodata			= 0x00;
static const uint8_t avi_active_aspect_reserved			= 0x01;
static const uint8_t avi_active_aspect_box_16x9_top		= 0x02;
static const uint8_t avi_active_aspect_box_14x9_top		= 0x03;
static const uint8_t avi_active_aspect_box_16x9_cen		= 0x04;
static const uint8_t avi_active_aspect_coded_frame		= 0x08;
static const uint8_t avi_active_aspect_4x3_cen			= 0x09;
static const uint8_t avi_active_aspect_16x9_cen			= 0x0a;
static const uint8_t avi_active_aspect_14x9_cen			= 0x0b;
static const uint8_t avi_active_aspect_4x3_cen_14x9		= 0x0d;
static const uint8_t avi_active_aspect_16x9_cen_14x9	= 0x0e;
static const uint8_t avi_active_aspect_16x9_cen_4x3		= 0x0f;

static const uint8_t avi_nonuniform_scaling_nodata		= 0x00;
static const uint8_t avi_nonuniform_scaling_horiz		= 0x01;
static const uint8_t avi_nonuniform_scaling_vert		= 0x02;
static const uint8_t avi_nonuniform_scaling_both		= 0x03;

static const uint8_t avi_rgb_quant_range_default		= 0x00;
static const uint8_t avi_rgb_quant_range_limited		= 0x01;
static const uint8_t avi_rgb_quant_range_full			= 0x02;
static const uint8_t avi_rgb_quant_range_reserved		= 0x03;

static const uint8_t avi_ext_colorimetry_xv_ycc601		= 0x00;
static const uint8_t avi_ext_colorimetry_xv_ycc709		= 0x01;
static const uint8_t avi_ext_colorimetry_s_ycc601		= 0x02;
static const uint8_t avi_ext_colorimetry_adobe_601		= 0x03;
static const uint8_t avi_ext_colorimetry_adobe_rgb		= 0x04;
static const uint8_t avi_ext_colorimetry_ycc2020		= 0x05;
static const uint8_t avi_ext_colorimetry_rgb2020		= 0x06;
static const uint8_t avi_ext_colorimetry_reserved		= 0x07;

static const uint8_t avi_it_type_graphics				= 0x00;
static const uint8_t avi_it_type_photo					= 0x01;
static const uint8_t avi_it_type_cinema					= 0x02;
static const uint8_t avi_it_type_game					= 0x03;

static const uint8_t avi_ycc_quant_range_limited		= 0x00;
static const uint8_t avi_ycc_quant_range_full			= 0x01;
static const uint8_t avi_ycc_quant_range_reserved		= 0x02;
static const uint8_t avi_ycc_quant_range_reserved1		= 0x03;

/* Establish register bank mappings. Note that the actual I2C bus addresses end up */
/* being right shifted by 1 from the addresses used here and in the chip docs. */
static struct ntv2_reg_value init_io0[] = 
{
	{ 0xf4, 0x80 },		/* CEC Map Registers, I2C Address = 80 */
	{ 0xf5, 0x7C },		/* Info Frame Map Registers, I2C Address = 7C */
	{ 0xf8, 0x4c },		/* DPLL Map Registers, I2C Address = 4C */
	{ 0xf9, 0x64 },		/* Repeater Map Registers, I2C Address = 64 */
	{ 0xfa, 0x6c },		/* EDID Map Registers, I2C Address = 6C */
	{ 0xfb, 0x68 },		/* HDMI Map Registers, I2C Address = 68 */
	{ 0xfd, 0x44 }		/* CP Map Registers, I2C Address = 44 */
};
static int init_io0_size = sizeof(init_io0) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_hdmi1[] = 
/* HDMI Register - I2C address = 0x68 */
/* ADI Recommended write */
{
	{ 0xC0, 0x03 },		/* Recommended ADI write, documentation from script */
	{ 0x4C, 0x44 },		/* %%%%% Set NEW_VS_PARAM (improves vertical filter locking) */

	/* %%%%% "Recommended writes" added 7/14/14 */
	{ 0x03, 0x98 },
	{ 0x10, 0xA5 },
	{ 0x45, 0x04 },
	{ 0x3D, 0x10 },
	{ 0x3e, 0x69 },
	{ 0x3F, 0x46 },
	{ 0x4E, 0xFE },
	{ 0x4f, 0x08 },
	{ 0x50, 0x00 },
	{ 0x57, 0xa3 },
	{ 0x58, 0x07 },
	{ 0x93, 0x03 },
	{ 0x5A, 0x80 },

//>	{ 0x6C, 0x14 },		/* Auto-assert HPD 100ms after (EDID active & cable detect) */
	{ 0x6C, 0x54 },		/* Auto-assert HPD 100ms after (EDID active & cable detect) */
	{ 0x0d, 0x02 }		/* Set TMDS frequency change tolerance to 2MHz */
};
static int init_hdmi1_size = sizeof(init_hdmi1) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_io2_non4k[] =
/* IO registers - I2C address = 0x98 */
{
	{ 0x00, 0x02 },		/* ADI Recommended Write */
	{ 0x01, 0x06 },		/* ADI Recommended Write  */
	{ 0x02, 0xf2 },		/* %%%%% INP_COLOR_SPACE[3:0], Address 0x02[7:4] = 1111 */
						/* 1111: Input color space depends on color space reported by HDMI block */
						/* ALT_GAMMA, Address 0x02[3] */
						/* 0 (default) No conversion */
						/* 1 YUV601 to YUV709 conversion if input is YUV601, YUV709 to YUV601 conversion if input is YUV709 */
						/* OP_656_RANGE, IO, Address 0x02[2] */
						/* 0 (default) Enables full output range (0 to 255) */
						/* 1 Enables limited output range (16 to 235)    */
						/* RGB_OUT, IO, Address 0x02[1]  */
						/* 0 (default) YPbPr color space output */
						/* 1 RGB color space output */
						/* ALT_DATA_SAT, IO, Address 0x02[0] */
						/* 0 (default) Data saturator enabled or disabled according to OP_656_RANGE setting */
						/* 1 Reverses OP_656_RANGE decision to enable or disable the data saturator */
	{ 0x03, 0x42 },		/* 36 Bit SDR Mode, RGB, Non-4K mode */
						/* Register changes to 0x54 for 4K mode */
	{ 0x04, 0x00 },		/* OP_CH_SEL[2:0], Address 0x04[7:5] = 000 P[35:24] Y/G, P[23:12] U/CrCb/B, P[11:0] V/R */
						/* XTAL_FREQ_SEL[1:0], Address 0x04[2:1] = 00, 27 Mhz */
						/* 4K mode requires 0x62 */
	{ 0x05, 0x38 },		/* F_OUT_SEL, IO, Address 0x05[4], Select DE or FIELD signal to be output on the DE pin */
						/* 0 (default) Selects DE output on DE pin */
						/* 1 Selects FIELD output on DE pin */
						/* DATA_BLANK_EN, IO, Address 0x05[3], A control to blank data during video blanking sections */
						/* 0 Do not blank data during horizontal and vertical blanking periods */
						/* 1 (default) Blank data during horizontal and vertical blanking periods */
						/* AVCODE_INSERT_EN, IO, Address 0x05[2], Select AV code insertion into the data stream */
						/* 0 Does not insert AV codes into data stream */
						/* 1 (default) Inserts AV codes into data stream */
						/* REPL_AV_CODE, IO, Address 0x05[1], duplicate AV codes and insertion on all output stream data channels */
						/* 0 (default) Outputs complete SAV/EAV codes on all channels, Channel A, Channel B, and Channel C */
						/* 1 Spreads AV code across three channels, Channel B and C contain the first two ten bit words, 0x3FF and 0x000 */
						/* Channel A contains the final two 10-bit words 0x00 and 0xXYZ */
						/* OP_SWAP_CB_CR, IO, Address 0x05[0], Controls the swapping of Cr and Cb data on the pixel buses */
						/* 0 (default) Outputs Cr and Cb as per OP_FORMAT_SEL */
						/* 1 Inverts the order of Cb and Cr in the interleaved data stream */
	{ 0x06, 0xa6 },		/* VS_OUT_SEL, Address 0x06[7], Select the VSync or FIELD signal to be output on the VS/FIELD/ALSB pin */
						/* 0 Selects FIELD output on VS/FIELD/ALSB pin */
						/* 1 (default) Selects VSync output on VS/FIELD/ALSB pin */
						/* INV_F_POL, Address 0x06[3], controls polarity of the DE signal */
						/* 0 (default) Negative FIELD/DE polarity */
						/* 1 Positive FIELD/DE polarity */
						/* INV_VS_POL, IO, Address 0x06[2] Controls polarity of the VS/FIELD/ALSB signal */
						/* 0 (default) Negative polarity VS/FIELD/ALSB */
						/* 1 Positive polarity VS/FIELD/ALSB */
						/* INV_HS_POL, Address 0x06[1], Controls polarity of the HS signal */
						/* 0 (default) Negative polarity HS */
						/* 1 Positive polarity HS */
						/* INV_LLC_POL, Address 0x06[0], Controls the polarity of the LLC */
						/* 0 (default) Does not invert LLC */
						/* 1 Inverts LLC */
	{ 0x0c, 0x42 },		/* Power up part */
	{ 0x14, 0x3F },		/* DR_STR[1:0], IO, Address 0x14[5:4] */
						/* 00 Reserved */
						/* 01 Medium low (2 */
						/* 10 (default) Medium high (3 */
						/* 11 High (4 */
						/* DR_STR_CLK[1:0], IO, Address 0x14[3:2] */
						/* 00 Reserved */
						/* 01 Medium low (2 for LLC up to 60 MHz */
						/* 10 (default) Medium high (3 for LLC from 44 MHz to 105 MHz */
						/* 11 High (4 for LLC greater than 100 MHz */
						/* DR_STR_SYNC[1:0], IO, Address 0x14[1:0] */
						/* 00 Reserved */
						/* 01 Medium low (2 */
						/* 10 (default) Medium high (3 */
						/* 11 High (4 */
	{ 0x15, 0x80 },		/* Disable Tristate of Pins */
/*!!        { 0x19, 0xC0 },	%%%%%	LLC DLL phase */
	{ 0x20, 0x04 },		/* HPA_MAN_VALUE_A, IO, Address 0x20[7] */
						/* A manual control for the value of HPA on Port A, Valid only if HPA_MANUAL is set to 1 */
						/* 0 - 0 V applied to HPA_A pin */
						/* 1 (default) High level applied to HPA_A pin */
						/* HPA_MAN_VALUE_B, IO, Address 0x20[6] */
						/* A manual control for the value of HPB on Port A, Valid only if HPA_MANUAL is set to 1 */
						/* 0 - 0 V applied to HPA_B pin */
						/* 1 (default) High level applied to HPA_B pin */
						/* HPA_TRISTATE_A, IO, Address 0x20[3] Tristates HPA output pin for Port A */
						/* 0 (default) HPA_A pin active */
						/* 1 Tristates HPA_A pin */
						/* HPA_TRISTATE_B, IO, Address 0x20[2] Tristates HPA output pin for Port B */
						/* 0 (default) HPA_B pin active */
						/* 1 Tristates HPA_B pin */
	{ 0x33, 0x40 },		/* LLC DLL MUX enable */
	{ 0xdd, 0x00 },		/* Normal LLC frequency = 0x00 for non-4K modes */
						/* LLC Half frequence = 0xA0 for 4K modes */
	{ 0xE7, 0x00 },		/* default: ADI Recommended Write per PCN 15_0178 */
	{ 0x6e, 0x40 },		/* %%%%% TMDSPLL_LCK_A_MB1 enable to catch PLL loss of lock (enables INT1) */
	{ 0x86, 0x02 } 		/* %%%%% NEW_TMDS_FREQ_MB1 enable to catch frequency changes */
};
static int init_io2_non4k_size = sizeof(init_io2_non4k) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_io2_4k[] = 
/* IO registers - I2C address = 0x98 */
{
//	{ 0x00, 0x02 },		/* ADI Recommended Write */
//	{ 0x01, 0x06 },		/* ADI Recommended Write  */

	{ 0x00, 0x19 },		/* ADI Recommended Write per PCN 15_0178 */
	{ 0x01, 0x05 },		/* ADI Recommended Write per PCN 15_0178 */

	{ 0x02, 0xf2 },		/* INP_COLOR_SPACE[3:0], Address 0x02[7:4] = 1111 */
						/* 1111: Input color space depends on color space reported by HDMI block */
						/* ALT_GAMMA, Address 0x02[3] */
						/* 0 (default) No conversion */
						/* 1 YUV601 to YUV709 conversion if input is YUV601, YUV709 to YUV601 conversion if input is YUV709 */
						/* OP_656_RANGE, IO, Address 0x02[2] */
						/* 0 (default) Enables full output range (0 to 255) */
						/* 1 Enables limited output range (16 to 235)    */
						/* RGB_OUT, IO, Address 0x02[1]  */
						/* 0 (default) YPbPr color space output */
						/* 1 RGB color space output */
						/* ALT_DATA_SAT, IO, Address 0x02[0] */
						/* 0 (default) Data saturator enabled or disabled according to OP_656_RANGE setting */
						/* 1 Reverses OP_656_RANGE decision to enable or disable the data saturator */
	{ 0x03, 0x54 },		/* 36 Bit SDR Mode, RGB, Non-4K mode */
						/* Register changes to 0x54 for 4K mode */
	{ 0x04, 0x62 },		/* OP_CH_SEL[2:0], Address 0x04[7:5] = 000 P[35:24] Y/G, P[23:12] U/CrCb/B, P[11:0] V/R */
						/* XTAL_FREQ_SEL[1:0], Address 0x04[2:1] = 00, 27 Mhz */
						/* 4K mode requires 0x62 */
	{ 0x05, 0x38 },		/* F_OUT_SEL, IO, Address 0x05[4], Select DE or FIELD signal to be output on the DE pin */
						/* 0 (default) Selects DE output on DE pin */
						/* 1 Selects FIELD output on DE pin */
						/* DATA_BLANK_EN, IO, Address 0x05[3], A control to blank data during video blanking sections */
						/* 0 Do not blank data during horizontal and vertical blanking periods */
						/* 1 (default) Blank data during horizontal and vertical blanking periods */
						/* AVCODE_INSERT_EN, IO, Address 0x05[2], Select AV code insertion into the data stream */
						/* 0 Does not insert AV codes into data stream */
						/* 1 (default) Inserts AV codes into data stream */
						/* REPL_AV_CODE, IO, Address 0x05[1], duplicate AV codes and insertion on all output stream data channels */
						/* 0 (default) Outputs complete SAV/EAV codes on all channels, Channel A, Channel B, and Channel C */
						/* 1 Spreads AV code across three channels, Channel B and C contain the first two ten bit words, 0x3FF and 0x000 */
						/* Channel A contains the final two 10-bit words 0x00 and 0xXYZ */
						/* OP_SWAP_CB_CR, IO, Address 0x05[0], Controls the swapping of Cr and Cb data on the pixel buses */
						/* 0 (default) Outputs Cr and Cb as per OP_FORMAT_SEL */
						/* 1 Inverts the order of Cb and Cr in the interleaved data stream */
	{ 0x06, 0xa6 },		/* VS_OUT_SEL, Address 0x06[7], Select the VSync or FIELD signal to be output on the VS/FIELD/ALSB pin */
						/* 0 Selects FIELD output on VS/FIELD/ALSB pin */
						/* 1 (default) Selects VSync output on VS/FIELD/ALSB pin */
						/* INV_F_POL, Address 0x06[3], controls polarity of the DE signal */
						/* 0 (default) Negative FIELD/DE polarity */
						/* 1 Positive FIELD/DE polarity */
						/* INV_VS_POL, IO, Address 0x06[2] Controls polarity of the VS/FIELD/ALSB signal */
						/* 0 (default) Negative polarity VS/FIELD/ALSB */
						/* 1 Positive polarity VS/FIELD/ALSB */
						/* INV_HS_POL, Address 0x06[1], Controls polarity of the HS signal */
						/* 0 (default) Negative polarity HS */
						/* 1 Positive polarity HS */
						/* INV_LLC_POL, Address 0x06[0], Controls the polarity of the LLC */
						/* 0 (default) Does not invert LLC */
						/* 1 Inverts LLC */
	{ 0x0c, 0x42 },		/* Power up part */
	{ 0x14, 0x3F },		/* DR_STR[1:0], IO, Address 0x14[5:4] */
						/* 00 Reserved */
						/* 01 Medium low (2 */
						/* 10 (default) Medium high (3 */
						/* 11 High (4 */
						/* DR_STR_CLK[1:0], IO, Address 0x14[3:2] */
						/* 00 Reserved */
						/* 01 Medium low (2 for LLC up to 60 MHz */
						/* 10 (default) Medium high (3 for LLC from 44 MHz to 105 MHz */
						/* 11 High (4 for LLC greater than 100 MHz */
						/* DR_STR_SYNC[1:0], IO, Address 0x14[1:0] */
						/* 00 Reserved */
						/* 01 Medium low (2 */
						/* 10 (default) Medium high (3 */
						/* 11 High (4 */
	{ 0x15, 0x80 },		/* Disable Tristate of Pins */
/*!!        { 0x19, 0x80 },	%%%%%	LLC DLL phase */
	{ 0x33, 0x40 },		/* LLC DLL MUX enable */
//	{ 0xdd, 0xA0 } 		/* Normal LLC frequency = 0x00 for non-4K modes */
						/* LLC Half frequence = 0xA0 for 4K modes */

	{ 0xdd, 0x00 },		/* ADI Recommended Write per PCN 15_0178 */
	{ 0xE7, 0x04 }		/* ADI Recommended Write per PCN 15_0178 */
};
static int init_io2_4k_size = sizeof(init_io2_4k) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_cp3[] = 
/* %%%%% CP Register - I2C address = 0x44 */
{
	{ 0xba, 0x00 },		/* No HDMI FreeRun */
	{ 0x6c, 0x00 }, 	/* CP clamp disable */
	{ 0x69, 0x10 },
	{ 0x68, 0x00 }
};
static int init_cp3_size = sizeof(init_cp3) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_rep4[] = 
/* Repeater Map Registers - I2C address = 0x64 */
{
	{ 0x40, 0x81 },		/* BCAPS  */
	{ 0x74, 0x03 } 		/* Enable EDID */
};
static int init_rep4_size = sizeof(init_rep4) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_dpll5_non4k[] = 
/* DPLL Registers - I2C address = 0x4C */
{
	{ 0xb5, 0x01 },		/* Setting MCLK to 256Fs */
	{ 0xc3, 0x00 },		/* ADI Recommended Settings (NormFreq) */
	{ 0xcf, 0x00 }, 		/* ADI Recommended Settings (NormFreq) */
	{ 0xdb, 0x00 }		/* default: ADI Recommended Write per PCN 15_0178 */
};
static int init_dpll5_non4k_size = sizeof(init_dpll5_non4k) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_dpll5_4k[] = 
/* DPLL Registers - I2C address = 0x4C */
{
	{ 0xb5, 0x01 },		/* Setting MCLK to 256Fs */
	{ 0xc3, 0x80 },		/* ADI Recommended Settings (NormFreq) */
	{ 0xcf, 0x03 },		/* ADI Recommended Settings (NormFreq) */
	{ 0xdb, 0x80 }		/* ADI Recommended Write per PCN 15_0178 */
};
static int init_dpll5_4k_size = sizeof(init_dpll5_4k) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_hdmi6[] = 
/* HDMI Registers - I2C address = 0x68 */
{
	{ 0x00, 0x00 },		/* BG_MEAS_PORT_SEL[2:0], Addr 68 (HDMI), Address 0x00[5:3] */
						/* 000 (default) Port A */
						/* 001 Port B */
	{ 0x01, 0x01 },		/* TERM_AUTO, Address 0x01[0] */
						/* This bit allows the user to select automatic or manual control of clock termination */
						/* If automatic mode termination is enabled, then termination on the port HDMI_PORT_SELECT[1:0] is enabled  */
						/* 0 (default) Disable termination automatic control */
						/* 1 Enable termination automatic control */
	{ 0x02, 0x01 },		/* EN_BG_PORT_A, Address 0x02[0] */
						/* 0 (default) Port disabled, unless selected with HDMI_PORT_SELECT[2:0] */
						/* 1 Port enabled in background mode */
						/* EN_BG_PORT_B, Address 0x02[1] */
						/* 0 (default) Port disabled, unless selected with HDMI_PORT_SELECT[2:0] */
						/* 1 Port enabled in background mode */
	{ 0x03, 0x58 },		/* I2SOUTMODE[1:0],  Address 0x03[6:5] */
						/* 00 (default) I2S mode */
						/* 01 Right justified */
						/* 10 Left justified */
						/* 11 Raw SPDIF (IEC60958) mode */
						/* I2SBITWIDTH[4:0], Address 0x03[4:0] */
						/* 11000 24 bits */
    { 0x14, 0x31 },		/* Audio mute triggers: turn off MT_MSK_PARITY_ERROR and also */
    { 0x15, 0xff },		/* turn off bits 7,6,3,2 in r14 which are undocumented */
    { 0x16, 0xff },		/* but must be 1 per defaults; this fixes iOS audio input. */
//>	{ 0x6c, 0x01 },		/* HPA_MANUAL, Address 0x6C[0] */
	{ 0x6c, 0x54 },		/* HPA_MANUAL, Address 0x6C[0] */
						/* Manual control enable for the HPA output pins */
						/* Manual control is determined by the HPA_MAN_VALUE_A */
						/* 1 HPA takes its value from HPA_MAN_VALUE_A */
	{ 0x3e, 0x69 },
	{ 0x3f, 0x46 },
	{ 0x4e, 0x7e },
	{ 0x4f, 0x42 },
	{ 0x57, 0xa3 },
	{ 0x58, 0x07 },
	{ 0x83, 0xfc },		/* CLOCK_TERMB_DISABLE, Address 0x83[1] */
						/* Disable clock termination on Port B, Can be used when TERM_AUTO set to 0 */
						/* 0 Enable Termination Port B */
						/* 1 (default) Disable Termination Port B */
						/* CLOCK_TERMA_DISABLE, Address 0x83[0] */
						/* Disable clock termination on Port A, Can be used when TERM_AUTO set to 0 */
						/* 0 Enable Termination Port A */
						/* 1 (default) Disable Termination Port A */
						/* Note - TERM_AUTO, Address 0x01[0] set to 1 which overrides this bit */

						/* Required for TMDS frequency 27Mhz and below */

	{ 0x89, 0x03 },
	{ 0x84, 0x03 },

	{ 0x85, 0x11 },		/* ADI Recommended Write */
	{ 0x9C, 0x80 },		/* ADI Recommended Write */
	{ 0x9C, 0xC0 },		/* ADI Recommended Write */
	{ 0x9C, 0x00 },		/* ADI Recommended Write */
	{ 0x85, 0x11 },		/* ADI Recommended Write */
	{ 0x86, 0x9B },		/* ADI Recommended Write */
	{ 0x9b, 0x03 }
};
static int init_hdmi6_size = sizeof(init_hdmi6) / sizeof(struct ntv2_reg_value);

static struct ntv2_reg_value init_hdmi8[] = 
/* HDMI Registers - I2C address = 0x68 */
{
	{ 0x6c, 0x54 }		/* HPA_MANUAL, Address 0x6C[0] */
						/* 0 (default)HPA takes its value based on HPA_AUTO_INT_EDID */
						/* HPA_AUTO_INT_EDID[1:0],Address 0x6C[2:1] */
						/* HPA_AUTO_INT_EDID[1:0] */
						/* 10 */
						/* HPA of an HDMI port asserted high after two conditions met */
						/* 1. Internal EDID is active for that port */
						/* 2. Delayed version of cable detect signal CABLE_DET_X_RAW for that port is high */
						/* HPA of an HDMI port immediately deasserted after either of these two conditions are met: */
						/* 1. Internal EDID is de-activated for that port */
						/* 2. Cable detect signal CABLE_DET_X_RAW for that port is low  */
						/* HPA of a specific HDMI port deasserted low immediately after internal E-EDID is de-activated */
};
static int init_hdmi8_size = sizeof(init_hdmi8) / sizeof(struct ntv2_reg_value);


#endif
