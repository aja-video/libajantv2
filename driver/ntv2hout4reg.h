/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2hout4reg.h
// Purpose:	 HDMI output monitor version 4
//
///////////////////////////////////////////////////////////////

#ifndef NTV2_HOUT4REG_H
#define NTV2_HOUT4REG_H

#include "ntv2commonreg.h"
#include "ntv2virtualregisters.h"
#include "ntv2enums.h"


/* control and status */
NTV2_REG(ntv2_reg_control_status,							48);			/* control status */
	NTV2_FLD(ntv2_fld_control_reference_source,					4,	24);		/* hardware reference source */
	NTV2_FLD(ntv2_fld_control_reference_present,				1,	30);		/* reference source present */
	NTV2_FLD(ntv2_fld_control_genlock_locked,					1,	31);		/* genlock locked */

/* hdmi output configuration register */
NTV2_REG(ntv2_reg_hdmiout_output_config,					125);			/* hdmi output config */
	NTV2_FLD(ntv2_fld_hdmiout_video_standard,					4,	0);			/* video standard */
	NTV2_FLD(ntv2_fld_hdmiout_audio_group_select,				1,	5);			/* audio upper group select */
	NTV2_FLD(ntv2_fld_hdmiout_tx_bypass,						1,	7);			/* v2 tx bypass? */
	NTV2_FLD(ntv2_fld_hdmiout_rgb,								1,	8);			/* rgb color space (not yuv) */
	NTV2_FLD(ntv2_fld_hdmiout_frame_rate,						4,	9);			/* frame rate */
	NTV2_FLD(ntv2_fld_hdmiout_progressive,						1,	13);		/* progressive? */
	NTV2_FLD(ntv2_fld_hdmiout_deep_color,						1,	14);		/* 10 bit deep color (not 8 bit) */
	NTV2_FLD(ntv2_fld_hdmiout_yuv_444,							1,	15);		/* yuv 444 mode */
	NTV2_FLD(ntv2_fld_hdmiout_audio_format,						2,	16);		/* hdmi output audio format */
	NTV2_FLD(ntv2_fld_hdmiout_sampling,							2,	18);		/* sampling? */
	NTV2_FLD(ntv2_fld_hdmiout_vobd,								2,	20);		/* hardware bit depth */
	NTV2_FLD(ntv2_fld_hdmiout_source_rgb,						1,	23);		/* source is rgb? */
	NTV2_FLD(ntv2_fld_hdmiout_power_down,						1,	25);		/* power down? */
	NTV2_FLD(ntv2_fld_hdmiout_tx_enable,						1,	26);		/* io4K tx enable */
	NTV2_FLD(ntv2_fld_hdmiout_rx_enable,						1,	27);		/* io4K rx enable */
	NTV2_FLD(ntv2_fld_hdmiout_full_range,						1,	28);		/* full range rgb (not smpte) */
	NTV2_FLD(ntv2_fld_hdmiout_audio_8ch,						1,	29);		/* 8 audio channels (not 2) */
	NTV2_FLD(ntv2_fld_hdmiout_dvi,								1,	30);		/* dvi mode (vs hdmi) */

/* hdmi input status */
NTV2_REG(ntv2_reg_hdmiin_input_status,						126);			/* hdmi input status register */
	NTV2_FLD(ntv2_fld_hdmiin_locked,							1,	0);		
	NTV2_FLD(ntv2_fld_hdmiin_stable,							1,	1);		
	NTV2_FLD(ntv2_fld_hdmiin_rgb,								1,	2);		
	NTV2_FLD(ntv2_fld_hdmiin_deep_color,						1,	3);		
	NTV2_FLD(ntv2_fld_hdmiin_video_code,						6,	4);			/* ntv2 video standard v2 */
	NTV2_FLD(ntv2_fld_hdmiin_lhi_ycbcr_mode,					1,	10);		
	NTV2_FLD(ntv2_fld_hdmiin_lhi_10bit_mode,					1,	11);		
	NTV2_FLD(ntv2_fld_hdmiin_audio_2ch,							1,	12);		/* 2 audio channels (vs 8) */
	NTV2_FLD(ntv2_fld_hdmiin_progressive,						1,	13);	
	NTV2_FLD(ntv2_fld_hdmiin_video_sd,							1,	14);		/* video pixel clock sd (not hd or 3g) */
	NTV2_FLD(ntv2_fld_hdmiin_video_74_25,						1,	15);		/* not used */
	NTV2_FLD(ntv2_fld_hdmiin_audio_rate,						4,	16);	
	NTV2_FLD(ntv2_fld_hdmiin_audio_word_length,					4,	20);	
	NTV2_FLD(ntv2_fld_hdmiin_video_format,						3,	24);		/* really ntv2 standard */
	NTV2_FLD(ntv2_fld_hdmiin_dvi,								1,	27);		/* input dvi (vs hdmi) */
	NTV2_FLD(ntv2_fld_hdmiin_video_rate,						4,	28);		/* ntv2 video rate */

/* hdmi control */
NTV2_REG(ntv2_reg_hdmi_control,								127);			/* hdmi audio status register */
	NTV2_FLD(ntv2_fld_hdmiout_force_config,						1,	1);			/* force output config (ignore edid) */	
	NTV2_FLD(ntv2_fld_hdmiin_audio_pair,						2,	2);			/* hdmi input audio pair select */	
	NTV2_FLD(ntv2_fld_hdmiin_rate_convert_enable,				1,	4);			/* hdmi input audio sample rate converter enable */	
	NTV2_FLD(ntv2_fld_hdmiin_channel34_swap_disable,			1,	5);			/* hdmi input audio channel 3/4 swap disable */	
	NTV2_FLD(ntv2_fld_hdmiout_channel34_swap_disable,			1,	6);			/* hdmi output audio channel 3/4 swap disable */	
	NTV2_FLD(ntv2_fld_hdmiout_prefer_420,						1,	7);			/* hdmi output prefer 4K/UHD 420 */	
	NTV2_FLD(ntv2_fld_hdmiin_color_depth,						2,	12);		/* hdmi input bit depth */	
	NTV2_FLD(ntv2_fld_hdmiin_color_space,						2,	14);		/* hdmi input color space */
	NTV2_FLD(ntv2_fld_hdmiout_audio_rate,						2,	16);		/* audio rate */
	NTV2_FLD(ntv2_fld_hdmiout_source_select,					4,	20);		/* output audio source select */	
	NTV2_FLD(ntv2_fld_hdmiout_crop_enable,						1,	24);		/* crop 2k -> hd  4k -> uhd */
	NTV2_FLD(ntv2_fld_hdmiout_force_hpd,						1,	25);		/* force hpd */
	NTV2_FLD(ntv2_fld_hdmiout_deep_12bit,						1,	26);		/* deep color 12 bit */
	NTV2_FLD(ntv2_fld_hdmi_debug,								1,	27);		/* debug output enable */
	NTV2_FLD(ntv2_fld_hdmi_disable_update,						1,	28);		/* disable update loop */
	NTV2_FLD(ntv2_fld_hdmiout_channel_select,					2,	29);		/* output audio channel select */	
	NTV2_FLD(ntv2_fld_hdmi_protocol,							1,	30);		/* hdmi protocol? */	
	NTV2_FLD(ntv2_fld_hdmiin_full_range,						1,	31);		/* hdmi input quantization full range */	

	NTV2_REG(ntv2_reg_hdmi_output_status1,					kVRegHDMIOutStatus1);	/* hdmi otuput status */
	NTV2_FLD(ntv2_fld_hdmiout_status_video_standard,			4,	0);			/* video standard */
	NTV2_FLD(ntv2_fld_hdmiout_status_frame_rate,				4,	4);			/* video frame rate */
	NTV2_FLD(ntv2_fld_hdmiout_status_bit_depth,					4,	8);			/* video bit depth */	
	NTV2_FLD(ntv2_fld_hdmiout_status_color_rgb,					1,	12);		/* video color rgb */
	NTV2_FLD(ntv2_fld_hdmiout_status_range_full,				1,	13);		/* video range full */
	NTV2_FLD(ntv2_fld_hdmiout_status_pixel_420,					1,	14);		/* video pixel 420 */
	NTV2_FLD(ntv2_fld_hdmiout_status_protocol,					1,	15);		/* dvi mode (vs hdmi) */
	NTV2_FLD(ntv2_fld_hdmiout_status_audio_format,				4,	16);		/* audio format */
	NTV2_FLD(ntv2_fld_hdmiout_status_audio_rate,				4,	20);		/* audio rate */
	NTV2_FLD(ntv2_fld_hdmiout_status_audio_channels,			4,	24);		/* audio channels */

/* hdmi source register */
NTV2_REG(ntv2_reg_hdmiout_cross_group6,						141);			/* crosspoint group 6 */
	NTV2_FLD(ntv2_fld_hdmiout_hdmi_source,						7,	16);		/* hdmi source */
	NTV2_FLD(ntv2_fld_hdmiout_hdmi_rgb,							1,	23);		/* rgb color space (not yuv) */

// hdr parameters
NTV2_REG(ntv2_reg_hdr_green_primary,						330);			/* hdr green primary register */
	NTV2_FLD(ntv2_fld_hdr_primary_x,							16,	0);			/* rgb primary x value */	
	NTV2_FLD(ntv2_fld_hdr_primary_y,							16,	16);		/* rgb primary y value */	
NTV2_REG(ntv2_reg_hdr_blue_primary,							331);			/* hdr blue primary register */
NTV2_REG(ntv2_reg_hdr_red_primary,							332);			/* hdr red primary register */

NTV2_REG(ntv2_reg_hdr_white_point,							333);			/* hdr white point register */
	NTV2_FLD(ntv2_fld_hdr_white_point_x,						16,	0);			/* white point x value */	
	NTV2_FLD(ntv2_fld_hdr_white_point_y,						16,	16);		/* white point y value */	
NTV2_REG(ntv2_reg_hdr_master_luminance,						334);			/* hdr mastering luminance register */
	NTV2_FLD(ntv2_fld_hdr_luminance_max,						16,	0);			/* luminance maximun value */	
	NTV2_FLD(ntv2_fld_hdr_luminance_min,						16,	16);		/* luminance minimum value */	
NTV2_REG(ntv2_reg_hdr_light_level,							335);			/* hdr light level register */
	NTV2_FLD(ntv2_fld_hdr_content_light_max,					16,	0);			/* content light level maximun value */	
	NTV2_FLD(ntv2_fld_hdr_frame_average_max,					16,	16);		/* franme average level maximum value */	

/* hdr control */
NTV2_REG(ntv2_reg_hdr_control,								336);			/* hdr control register */
	NTV2_FLD(ntv2_fld_hdr_constant_luminance,					1,	0);			/* constant luminance */	
	NTV2_FLD(ntv2_fld_hdr_dci_colorimetry,						1,	5);			/* dci colorimetry */	
	NTV2_FLD(ntv2_fld_hdr_dolby_vision_enable,					1,	6);			/* dolby vision enable */	
	NTV2_FLD(ntv2_fld_hdr_enable,								1,	7);			/* hdr enable */	
	NTV2_FLD(ntv2_fld_hdr_transfer_function,					8,	16);		/* electro optical transfer function */	
	NTV2_FLD(ntv2_fld_hdr_metadata_id,							8,	24);		/* metadata descriptor id */	

/* hdmi output control registers */
NTV2_REG(ntv2_reg_hdmiout4_videocontrol,					0x1d40);	/* hdmi control/status */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_scrambleMode,		1,	 1);	/* scdc 2.0 scramble mode */
		NTV2_CON(ntv2_con_hdmiout4_scramblemode_disable,			0x0);		/* scramble disable */
		NTV2_CON(ntv2_con_hdmiout4_scramblemode_enable,				0x1);		/* scramble enable */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_tranceivermode,		1,	 3);	/* transceiver mode */
		NTV2_CON(ntv2_con_hdmiout4_tranceivermode_disable,			0x0);		/* tranceiver disable */
		NTV2_CON(ntv2_con_hdmiout4_tranceivermode_enable,			0x1);		/* tranceiver enable */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_420mode,			1,	5);		/* 420 mode */
		NTV2_CON(ntv2_con_hdmiout4_420mode_disable,					0x0);		/* 420 disable */
		NTV2_CON(ntv2_con_hdmiout4_420mode_enable,					0x1);		/* 420 enable */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_pixelsperclock,		3,	 8);	/* pixels per clock */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_pixelreplicate,		1,	 11);	/* pixel replicate */
		NTV2_CON(ntv2_con_hdmiout4_pixelreplicate_disable,			0x0);		/* replicate disable */
		NTV2_CON(ntv2_con_hdmiout4_pixelreplicate_enable,			0x1);		/* replicate enable */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_replicatefactor,	4,	 12);	/* pixels replicate factor */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_linerate,			5,	 16);	/* line rate */
		NTV2_CON(ntv2_con_hdmiout4_linerate_none,					0x0);		/* undetected */
		NTV2_CON(ntv2_con_hdmiout4_linerate_5940mhz,				0x1);		/* 5940 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_2970mhz,				0x2);		/* 2970 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_1485mhz,				0x3);		/* 1485 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_742mhz,					0x4);		/*  742 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_270mhz,					0x5);		/*  270 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_3712mhz,				0x6);		/* 3712 mhz 10 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_1856mhz,				0x7);		/* 1856 mhz 10 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_928mhz,					0x8);		/*  928 mhz 10 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_337mhz,					0x9);		/*  337 mhz 10 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_4455mhz,				0xa);		/* 4455 mhz 12 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_2227mhz,				0xb);		/* 2227 mhz 12 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_1113mhz,				0xc);		/* 1113 mhz 12 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_405mhz,					0xd);		/*  405 mhz 12 bit */
		NTV2_CON(ntv2_con_hdmiout4_linerate_556mhz,					0xe);		/*  556 mhz */
		NTV2_CON(ntv2_con_hdmiout4_linerate_540mhz,					0xf);		/*  540 mhz */
		NTV2_CON(ntv2_con_hdmiout4_linerate_250mhz,					0x10);		/*  250 mhz */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_audiomode,			1,	 26);	/* audio mode */
		NTV2_CON(ntv2_con_hdmiout4_audiomode_disable,				0x0);		/* audio disable */
		NTV2_CON(ntv2_con_hdmiout4_audiomode_enable,				0x1);		/* audio enable */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_txlockstate,		1,	 27);	/* tx lock state */
		NTV2_CON(ntv2_con_hdmiout4_txlockstate_unlocked,			0x0);		/* tx unlocked */
		NTV2_CON(ntv2_con_hdmiout4_txlockstate_locked,				0x1);		/* tx locked */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_txconfigmode,		1,	 28);	/* tx configuration mode */
		NTV2_CON(ntv2_con_hdmiout4_txconfigmode_active,				0x0);		/* tx config active */
		NTV2_CON(ntv2_con_hdmiout4_txconfigmode_valid,				0x1);		/* tx config valid */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_sinkpresent,		1,	 29);	/* sink present */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_resetdone,			1,	 30);	/* rx reset done */
	NTV2_FLD(ntv2_fld_hdmiout4_videocontrol_reset,	   			1,	 31);	/* rx reset */

NTV2_REG(ntv2_reg_hdmiout4_videosetup0,						0x1d41);	/* video setup 0 register */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup0_colordepth,			2,	 0);	/* color depth */
   	NTV2_FLD(ntv2_fld_hdmiout4_videosetup0_colorspace,			2,	 2);	/* color space */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup0_scanmode,			1,	 4);	/* video scan mode */
		NTV2_CON(ntv2_con_hdmiout4_scanmode_interlaced,				0x0);		/* interlaced */
		NTV2_CON(ntv2_con_hdmiout4_scanmode_progressive,			0x1);		/* progressive */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup0_interfacemode,		1,	 5);	/* interface mode */
		NTV2_CON(ntv2_con_hdmiout4_interfacemode_hdmi,				0x0);		/* hdmi */
		NTV2_CON(ntv2_con_hdmiout4_interfacemode_dvi,				0x1);		/* dvi */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup0_syncpolarity,		1,	 6);	/* sync polarity */
		NTV2_CON(ntv2_con_hdmiout4_syncpolarity_activelow,			0x0);		/* active low */
		NTV2_CON(ntv2_con_hdmiout4_syncpolarity_activehigh,			0x1);		/* active high */

NTV2_REG(ntv2_reg_hdmiout4_videosetup1,						0x1d42);	/* video setup 1 register */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup1_hsyncstart,			16,	 0);	/* horizontal sync start */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup1_hsyncend,			16,	 16);	/* horizontal sync end */

NTV2_REG(ntv2_reg_hdmiout4_videosetup2,						0x1d43);	/* video setup 2 register */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup2_hdestart,			16,	 0);	/* horizontal de start */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup2_htotal,				16,	 16);	/* horizontal total */

NTV2_REG(ntv2_reg_hdmiout4_videosetup3,						0x1d44);	/* video setup 3 register */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup3_vtransf1,			16,	 0);	/* vertical transistion field 1 */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup3_vtransf2,			16,	 16);	/* vertical transistion field 2 */

NTV2_REG(ntv2_reg_hdmiout4_videosetup4,						0x1d45);	/* video setup 4 register */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup4_vsyncstartf1,		16,	 0);	/* vertical sync start field 1 */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup4_vsyncendf1,			16,	 16);	/* virtical sync end field 1 */

NTV2_REG(ntv2_reg_hdmiout4_videosetup5,						0x1d46);	/* video setup 5 register */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup5_vdestartf1,			16,	 0);	/* vertical de start field 1 */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup5_vdestartf2,			16,	 16);	/* vertical de start field 2 */

NTV2_REG(ntv2_reg_hdmiout4_videosetup6,						0x1d47);	/* video setup 6 register */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup6_vsyncstartf2,		16,	 0);	/* vertical sync start field 2 */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup6_vsyncendf2,			16,	 16);	/* virtical sync end field 2 */

NTV2_REG(ntv2_reg_hdmiout4_videosetup7,						0x1d48);	/* video setup 7 register */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup7_vtotalf1,			16,	 0);	/* vertical total field 1 */
	NTV2_FLD(ntv2_fld_hdmiout4_videosetup7_vtotalf2,			16,	 16);	/* vertical total field 2 */

NTV2_REG(ntv2_reg_hdmiout4_auxcontrol,						0x1d49);	/* aux data control */
	NTV2_FLD(ntv2_fld_hdmiout4_auxcontrol_auxdata,				8,	 0);	/* aux data */
	NTV2_FLD(ntv2_fld_hdmiout4_auxcontrol_auxaddress,			11,	 8);	/* aux address */
	NTV2_FLD(ntv2_fld_hdmiout4_auxcontrol_auxwrite,				1,	 20);	/* aux write */

NTV2_REG(ntv2_reg_hdmiout4_audiocontrol,					0x1d4b);	/* audio data control */
	NTV2_FLD(ntv2_fld_hdmiout4_audiocontrol_source,				4,	 0);	/* source */
	NTV2_FLD(ntv2_fld_hdmiout4_audiocontrol_group_select,		1,	 4);	/* upper/lower 8 source channels */
		NTV2_CON(ntv2_con_hdmiout4_group_select_lower,				0x0);		/* lower 8 channels */
		NTV2_CON(ntv2_con_hdmiout4_group_select_upper,				0x1);		/* upper 8 channels */
	NTV2_FLD(ntv2_fld_hdmiout4_audiocontrol_num_channels,		1,	 5);	/* 8/2 channel output */
		NTV2_CON(ntv2_con_hdmiout4_num_channels_2,					0x0);		/* 2 channel audio */
		NTV2_CON(ntv2_con_hdmiout4_num_channels_8,					0x1);		/* 8 channel audio */
	NTV2_FLD(ntv2_fld_hdmiout4_audiocontrol_audioswapmode,		1,	 6);	/* audio channel 3/4 swap */
		NTV2_CON(ntv2_con_hdmiout4_audioswapmode_enable,			0x0);		/* swap */
		NTV2_CON(ntv2_con_hdmiout4_audioswapmode_disable,			0x1);		/* no swap */
	NTV2_FLD(ntv2_fld_hdmiout4_audiocontrol_channel_select,		2,	 8);	/* 2 channel select */
	NTV2_FLD(ntv2_fld_hdmiout4_audiocontrol_audio_format,		2,	 12);	/* encode format */
		NTV2_CON(ntv2_con_hdmiout4_audio_format_lpcm,				0x0);		/* lpcm data */
		NTV2_CON(ntv2_con_hdmiout4_audio_format_dolby,				0x1);		/* dolby encoded data */
	NTV2_FLD(ntv2_fld_hdmiout4_audiocontrol_audio_rate,			2,	 14);	/* sample rate */
		NTV2_CON(ntv2_con_hdmiout4_audio_rate_48,					0x0);		/* 48 khz */
		NTV2_CON(ntv2_con_hdmiout4_audio_rate_96,					0x1);		/* 96 khz */
		NTV2_CON(ntv2_con_hdmiout4_audio_rate_192,					0x2);		/* 192 khz */


NTV2_REG(ntv2_reg_hdmiout4_redrivercontrol,					0x1d4f);	/* hdmi redriver control */
	NTV2_FLD(ntv2_fld_hdmiout4_redrivercontrol_power,			1,	 0);	/* power */
		NTV2_CON(ntv2_con_hdmiout4_power_disable,					0x0);		/* power disable */
		NTV2_CON(ntv2_con_hdmiout4_power_enable,					0x1);		/* power enable */
	NTV2_FLD(ntv2_fld_hdmiout4_redrivercontrol_pinmode,			1,	 1);	/* pin mode */
		NTV2_CON(ntv2_con_hdmiout4_pinmode_disable,					0x0);		/* pin disable */
		NTV2_CON(ntv2_con_hdmiout4_pinmode_enable,					0x1);		/* pin enable */
	NTV2_FLD(ntv2_fld_hdmiout4_redrivercontrol_vodrange,		1,	 2);	/* differential voltage range */
		NTV2_CON(ntv2_con_hdmiout4_vodrange_low,					0x0);		/* voltage swing low */
		NTV2_CON(ntv2_con_hdmiout4_vodrange_high,					0x1);		/* voltage swing high */
	NTV2_FLD(ntv2_fld_hdmiout4_redrivercontrol_deemphasis,		2,	 4);	/* deemphasis */
		NTV2_CON(ntv2_con_hdmiout4_deemphasis_0d0db,				0x0);		/* 0 db */
		NTV2_CON(ntv2_con_hdmiout4_deemphasis_3d5db,				0x1);		/* 3.5 db */
		NTV2_CON(ntv2_con_hdmiout4_deemphasis_6d0db,				0x2);		/* 6 db */
		NTV2_CON(ntv2_con_hdmiout4_deemphasis_9d5db,				0x3);		/* 9.5 db */
	NTV2_FLD(ntv2_fld_hdmiout4_redrivercontrol_preemphasis,		2,	 8);	/* preemphasis */
		NTV2_CON(ntv2_con_hdmiout4_preemphasis_0d0db,				0x0);		/* 0 db */
		NTV2_CON(ntv2_con_hdmiout4_preemphasis_1d6db,				0x1);		/* 1.6 db */
		NTV2_CON(ntv2_con_hdmiout4_preemphasis_3d5db,				0x2);		/* 3.5 db */
		NTV2_CON(ntv2_con_hdmiout4_preemphasis_6d0db,				0x3);		/* 6 db */
	NTV2_FLD(ntv2_fld_hdmiout4_redrivercontrol_boost,			4,	 12);	/* boost */
		NTV2_CON(ntv2_con_hdmiout4_boost_00d25db,					0x0);		/* 0.25 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_00d80db,					0x1);		/* 0.80 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_01d10db,					0x2);		/* 1.1 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_02d20db,					0x3);		/* 2.2 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_04d10db,					0x4);		/* 4.1 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_07d10db,					0x5);		/* 7.1 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_09d00db,					0x6);		/* 9.0 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_10d30db,					0x7);		/* 10.3 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_11d80db,					0x8);		/* 11.8 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_13d90db,					0x9);		/* 13.9 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_15d30db,					0xa);		/* 15.3 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_16d90db,					0xb);		/* 16.9 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_17d90db,					0xc);		/* 17.9 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_19d20db,					0xd);		/* 19.2 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_20d50db,					0xe);		/* 20.5 db */
		NTV2_CON(ntv2_con_hdmiout4_boost_22d20db,					0xf);		/* 22.2 db */

NTV2_REG(ntv2_reg_hdmiout4_refclockfrequency,				0x1d50);	/* reference clock frequency */
NTV2_REG(ntv2_reg_hdmiout4_tmdsclockfrequency,				0x1d51);	/* tmds clock frequency */
NTV2_REG(ntv2_reg_hdmiout4_txclockfrequency,				0x1d52);	/* tx clock frequency */
NTV2_REG(ntv2_reg_hdmiout4_fpllclockfrequency,				0x1d53);	/* fpll clock frequency */

NTV2_REG(ntv2_reg_hdmiout4_audio_cts1,						0x1d54);	/* audio clock cts 1 */
NTV2_REG(ntv2_reg_hdmiout4_audio_cts2,						0x1d55);	/* audio clock cts 2 */
NTV2_REG(ntv2_reg_hdmiout4_audio_cts3,						0x1d56);	/* audio clock cts 3 */
NTV2_REG(ntv2_reg_hdmiout4_audio_cts4,						0x1d57);	/* audio clock cts 4 */
NTV2_REG(ntv2_reg_hdmiout4_audio_n,							0x1d58);	/* audio clock n */

NTV2_REG(ntv2_reg_hdmiout4_croplocation,					0x1d5e);	/* crop location */
	NTV2_FLD(ntv2_fld_hdmiout4_croplocation_start,				16,	 0);	/* crop start location */
	NTV2_FLD(ntv2_fld_hdmiout4_croplocation_end,				16,	 16);	/* crop end location */

NTV2_REG(ntv2_reg_hdmiout4_pixelcontrol,					0x1d5f);	/* pixel control */
	NTV2_FLD(ntv2_fld_hdmiout4_pixelcontrol_lineinterleave,		1,	0);		/* line interleave */
		NTV2_CON(ntv2_con_hdmiout4_lineinterleave_disable,			0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiout4_lineinterleave_enable,			0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiout4_pixelcontrol_pixelinterleave,	1,	1);		/* pixel interleave */
		NTV2_CON(ntv2_con_hdmiout4_pixelinterleave_disable,			0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiout4_pixelinterleave_enable,			0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiout4_pixelcontrol_420convert,			1,	2);		/* 420 to 422 conversion */
		NTV2_CON(ntv2_con_hdmiout4_420convert_disable,				0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiout4_420convert_enable,				0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiout4_pixelcontrol_cropmode,			1,	 3);	/* crop mode */
		NTV2_CON(ntv2_con_hdmiout4_cropmode_disable,				0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiout4_cropmode_enable,					0x1);		/* enable */

NTV2_REG(ntv2_reg_hdmiout4_i2ccontrol,						0x1d60);	/* i2c control */
	NTV2_FLD(ntv2_fld_hdmiout4_i2ccontrol_writedata,			8,	 0);	/* write data */
	NTV2_FLD(ntv2_fld_hdmiout4_i2ccontrol_subaddress,			8,	 8);	/* i2c sub-address */
	NTV2_FLD(ntv2_fld_hdmiout4_i2ccontrol_devaddress,			7,	 16);	/* i2c device address */
	NTV2_FLD(ntv2_fld_hdmiout4_i2ccontrol_read,					1,	 23);	/* read (not write) */
	NTV2_FLD(ntv2_fld_hdmiout4_i2ccontrol_readdata,				8,	 24);	/* read data */

NTV2_REG(ntv2_reg_hdmiout4_i2cedid,							0x1d61);	/* edid read control */
	NTV2_FLD(ntv2_fld_hdmiout4_i2cedid_subaddress,				8,	 0);	/* edid sub-address */
	NTV2_FLD(ntv2_fld_hdmiout4_i2cedid_readdata,				8,	 8);	/* read data */
	NTV2_FLD(ntv2_fld_hdmiout4_i2cedid_update,					1,	 16);	/* trigger edid update */
	NTV2_FLD(ntv2_fld_hdmiout4_i2cedid_done,					1,	 17);	/* i2c engine done */
	NTV2_FLD(ntv2_fld_hdmiout4_i2cedid_present,					1,	 27);	/* sink present */
	NTV2_FLD(ntv2_fld_hdmiout4_i2cedid_hotplugcount,			4,	 28);	/* hot plug count */

/* hdmi output scdc i2c registers */
NTV2_CON(ntv2_dev_hdmiout4_sink,							0x54);		/* sink device address */

NTV2_CON(ntv2_reg_hdmiout4_sinkversion,						0x01);		/* sink version */
NTV2_CON(ntv2_reg_hdmiout4_sourceversion,					0x02);		/* source version */

NTV2_CON(ntv2_reg_hdmiout4_updateflags0,					0x10);		/* update flags */
	NTV2_FLD(ntv2_fld_hdmiout4_updateflags0_statusupdate,		1,	 0);	/* status flags register has changed */
	NTV2_FLD(ntv2_fld_hdmiout4_updateflags0_cedupdate,			1,	 1);	/* character error detection update */
	NTV2_FLD(ntv2_fld_hdmiout4_updateflags0_rrtest,				1,	 2);	/* read request test ack */
NTV2_CON(ntv2_reg_hdmiout4_updateflags1,					0x11);		/* update flags */

NTV2_CON(ntv2_reg_hdmiout4_tmdsconfig,						0x20);		/* tmds configuration */
	NTV2_FLD(ntv2_fld_hdmiout4_tmdsconfig_scamblemode,			1,	 0);	/* sink scamble mode */
		NTV2_CON(ntv2_con_hdmiout4_scamblemode_disable,				0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiout4_scamblemode_enable,				0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiout4_tmdsconfig_clockratio,			1,	 1);	/* tmds bit clock ratio */
		NTV2_CON(ntv2_con_hdmiout4_clockratio_10,					0x0);		/* 1/10 */
		NTV2_CON(ntv2_con_hdmiout4_clockratio_40,					0x1);		/* 1/40 */
NTV2_CON(ntv2_reg_hdmiout4_scamblerstatus,					0x21);		/* scrambler status */
	NTV2_FLD(ntv2_fld_hdmiout4_scamblerstatus_scrambledetect,	1,	 0);	/* sink detects scrambling */

NTV2_CON(ntv2_reg_hdmiout4_scdcconfig,						0x30);		/* scdc config */
	NTV2_FLD(ntv2_fld_hdmiout4_scdcconfig_readmode,				1,	 0);	/* read request mode */
		NTV2_CON(ntv2_con_hdmiout4_readmode_poll,					0x0);		/* source polls */
		NTV2_CON(ntv2_con_hdmiout4_readmode_request,				0x1);		/* source uses read requests */

NTV2_CON(ntv2_reg_hdmiout4_scdcstatus0,						0x40);		/* scdc status 0 */
	NTV2_FLD(ntv2_fld_hdmiout4_scdcstatus0_clockdetect,			1,	 0);	/* clock detected */
	NTV2_FLD(ntv2_fld_hdmiout4_scdcstatus0_ch0lock,				1,	 1);	/* channel 0 locked */
	NTV2_FLD(ntv2_fld_hdmiout4_scdcstatus0_ch1lock,				1,	 2);	/* channel 1 locked */
	NTV2_FLD(ntv2_fld_hdmiout4_scdcstatus0_ch2lock,				1,	 3);	/* channel 2 locked */
NTV2_CON(ntv2_reg_hdmiout4_scdcstatus1,						0x41);		/* scdc status 1 */

NTV2_CON(ntv2_reg_hdmiout4_ch0errorlow,						0x50);		/* channel 0 error count low */
	NTV2_FLD(ntv2_fld_hdmiout4_ch0errorlow_count,				8,	 0);	/* count */
NTV2_CON(ntv2_reg_hdmiout4_ch0errorhigh,					0x51);		/* channel 0 error count high */
	NTV2_FLD(ntv2_fld_hdmiout4_ch0errorhigh_count,				7,	 0);	/* count */
	NTV2_FLD(ntv2_fld_hdmiout4_ch0errorhigh_valid,				1,	 7);	/* valid */
NTV2_CON(ntv2_reg_hdmiout4_ch1errorlow,						0x52);		/* channel 1 error count low */
	NTV2_FLD(ntv2_fld_hdmiout4_ch1errorlow_count,				8,	 0);	/* count */
NTV2_CON(ntv2_reg_hdmiout4_ch1errorhigh,					0x53);		/* channel 1 error count high */
	NTV2_FLD(ntv2_fld_hdmiout4_ch1errorhigh_count,				7,	 0);	/* count */
	NTV2_FLD(ntv2_fld_hdmiout4_ch1errorhigh_valid,				1,	 7);	/* valid */
NTV2_CON(ntv2_reg_hdmiout4_ch2errorlow,						0x54);		/* channel 2 error count low */
	NTV2_FLD(ntv2_fld_hdmiout4_ch2errorlow_count,				8,	 0);	/* count */
NTV2_CON(ntv2_reg_hdmiout4_ch2errorhigh,					0x55);		/* channel 3 error count high */
	NTV2_FLD(ntv2_fld_hdmiout4_ch2errorhigh_count,				7,	 0);	/* count */
	NTV2_FLD(ntv2_fld_hdmiout4_ch2errorhigh_valid,				1,	 7);	/* valid */
NTV2_CON(ntv2_reg_hdmiout4_errorchecksum,					0x55);		/* checksum of all channel errors */

#endif
