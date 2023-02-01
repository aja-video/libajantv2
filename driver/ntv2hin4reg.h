/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2hin4reg.h
// Purpose:	 HDMI input monitor version 4
//
///////////////////////////////////////////////////////////////

#ifndef NTV2_HIN4REG_H
#define NTV2_HIN4REG_H

#include "ntv2commonreg.h"
#include "ntv2virtualregisters.h"

/* hdmi input status */
NTV2_REG(ntv2_reg_hdmiin_input_status,						126, 0x1d15, 0x2515);			/* hdmi input status register */
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
NTV2_REG(ntv2_reg_hdmi_control,								127, 0x1d16, 0x2516);			/* hdmi audio status register */
	NTV2_FLD(ntv2_fld_hdmiout_force_config,						1,	1);			/* force output config (ignore edid) */	
	NTV2_FLD(ntv2_fld_hdmiin_audio_pair,						2,	2);			/* hdmi input audio pair select */	
	NTV2_FLD(ntv2_fld_hdmiin_rate_convert_enable,				1,	4);			/* hdmi input audio sample rate converter enable */	
	NTV2_FLD(ntv2_fld_hdmiin_channel34_swap_disable,			1,	5);			/* hdmi input audio channel 3/4 swap disable */	
	NTV2_FLD(ntv2_fld_hdmiout_channel34_swap_disable,			1,	6);			/* hdmi output audio channel 3/4 swap disable */	
	NTV2_FLD(ntv2_fld_hdmiout_prefer_420,						1,	7);			/* hdmi output prefer 4K/UHD 420 */	
	NTV2_FLD(ntv2_fld_hdmiout_audio_format,						2,	8);			/* hdmi output audio format */
	NTV2_FLD(ntv2_fld_hdmiin_color_depth,						2,	12);		/* hdmi input bit depth */	
	NTV2_FLD(ntv2_fld_hdmiin_color_space,						2,	14);		/* hdmi input color space */
	NTV2_FLD(ntv2_fld_hdmi_polarity,							4,	16);		/* hdmi polarity? */	
	NTV2_FLD(ntv2_fld_hdmiout_source_select,					4,	20);		/* output audio source select */	
	NTV2_FLD(ntv2_fld_hdmiout_crop_enable,						1,	24);		/* crop 2k -> hd  4k -> uhd */
	NTV2_FLD(ntv2_fld_hdmiout_force_hpd,						1,	25);		/* force hpd */
	NTV2_FLD(ntv2_fld_hdmiout_deep_12bit,						1,	26);		/* deep color 12 bit */
	NTV2_FLD(ntv2_fld_hdmi_debug,								1,	27);		/* debug output enable */
	NTV2_FLD(ntv2_fld_hdmi_disable_update,						1,	28);		/* disable update loop */
	NTV2_FLD(ntv2_fld_hdmiout_channel_select,					2,	29);		/* output audio channel select */	
	NTV2_FLD(ntv2_fld_hdmi_protocol,							1,	30);		/* hdmi protocol? */	
	NTV2_FLD(ntv2_fld_hdmiin_full_range,						1,	31);		/* hdmi input quantization full range */	

NTV2_REG(ntv2_reg_hdmiin4_auxdata,							0x1c00, 0x1c00, 0x2400);		/* hdmi aux data */
		NTV2_CON(ntv2_con_auxdata_size,								8);				/* aux data register length */
		NTV2_CON(ntv2_con_auxdata_count,							32);			/* number of aux data slots */
		NTV2_CON(ntv2_con_header_type_nodata,						0x00);			/* aux header type */
		NTV2_CON(ntv2_con_header_type_vendor_specific,				0x81);
		NTV2_CON(ntv2_con_header_type_video_info,					0x82);
		NTV2_CON(ntv2_con_header_type_source_product,				0x83);
		NTV2_CON(ntv2_con_header_type_audio_info,					0x84);
		NTV2_CON(ntv2_con_header_type_drm_info,						0x87);

NTV2_REG(ntv2_reg_hdmiin4_videocontrol,						0x1d00, 0x1d00, 0x2500);		/* hdmi control/status */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_scrambledetect,		1,	 0);		/* scdc 2.0 scramble detect */
		NTV2_CON(ntv2_con_hdmiin4_scrambledetect_false,				0x0);			/* scramble not detected */
		NTV2_CON(ntv2_con_hdmiin4_scrambledetect_true,				0x1);			/* scramble detected */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_descramblemode,		1,	 1);		/* scdc 2.0 descamble mode */
		NTV2_CON(ntv2_con_hdmiin4_descramblemode_disable,			0x0);			/* descramble disable */
		NTV2_CON(ntv2_con_hdmiin4_descramblemode_enable,			0x1);			/* descramble enable */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_scdcratedetect,		1,	 2);		/* scdc hdmi receive > 3.4 gbps */
		NTV2_CON(ntv2_con_hdmiin4_scdcratedetect_low,				0x0);			/* scdc hdmi receive rate < 3.4 gbps */
		NTV2_CON(ntv2_con_hdmiin4_scdcratedetect_high,				0x1);			/* scdc hdmi receive rate > 3.4 gbps */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_scdcratemode,		1,	 3);		/* scdc hdmi mode > 3.4 gbps */
		NTV2_CON(ntv2_con_hdmiin4_scdcratemode_low,					0x0);			/* scdc hdmi mode rate < 3.4 gbps */
		NTV2_CON(ntv2_con_hdmiin4_scdcratemode_high,				0x1);			/* scdc hdmi mode rate > 3.4 gbps */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_420mode,				1,	5);			/* 420 mode */
		NTV2_CON(ntv2_con_hdmiin4_420mode_disable,					0x0);			/* 420 disable */
		NTV2_CON(ntv2_con_hdmiin4_420mode_enable,					0x1);			/* 420 enable */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_pixelsperclock,		3,	 8);		/* pixels per clock */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_hsyncdivide,			1,	 12);		/* hsync divide mode */
		NTV2_CON(ntv2_con_hdmiin4_hsyncdivide_none,					0x0);			/* no hsync divide */
		NTV2_CON(ntv2_con_hdmiin4_hsyncdivide_2,					0x1);			/* divide hsync by 2 */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_audioswapmode,		1,	 13);		/* audio channel 34 swap */
		NTV2_CON(ntv2_con_hdmiin4_audioswapmode_enable,				0x0);			/* swap */
		NTV2_CON(ntv2_con_hdmiin4_audioswapmode_disable,			0x1);			/* no swap */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_audioresamplemode,	1,	 14);		/* audio resample mode */
		NTV2_CON(ntv2_con_hdmiin4_audioresamplemode_enable,			0x0);			/* enable */
		NTV2_CON(ntv2_con_hdmiin4_audioresamplemode_disable,		0x1);			/* disable */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_linerate,			5,	 16);		/* line rate */
		NTV2_CON(ntv2_con_hdmiin4_linerate_none,					0x0);			/* undetected */
		NTV2_CON(ntv2_con_hdmiin4_linerate_5940mhz,					0x1);			/* 5940 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_2970mhz,					0x2);			/* 2970 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_1485mhz,					0x3);			/* 1485 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_742mhz,					0x4);			/*  742 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_270mhz,					0x5);			/*  270 mhz  8 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_3712mhz,					0x6);			/* 3712 mhz 10 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_1856mhz,					0x7);			/* 1856 mhz 10 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_928mhz,					0x8);			/*  928 mhz 10 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_337mhz,					0x9);			/*  337 mhz 10 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_4455mhz,					0xa);			/* 4455 mhz 12 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_2227mhz,					0xb);			/* 2227 mhz 12 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_1113mhz,					0xc);			/* 1113 mhz 12 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_405mhz,					0xd);			/*  405 mhz 12 bit */
		NTV2_CON(ntv2_con_hdmiin4_linerate_556mhz,					0xe);			/*  556 mhz */
		NTV2_CON(ntv2_con_hdmiin4_linerate_540mhz,					0xf);			/*  540 mhz */
		NTV2_CON(ntv2_con_hdmiin4_linerate_250mhz,					0x10);			/*  250 mhz */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_deserializerlock,	3,	 24);		/* deserializers lock state */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_inputlock,			1,	 27);		/* input lock state */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_hdmi5vdetect,		1,	 28);		/* hdmi detect state */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_hotplugmode,			1,	 29);		/* hot plug mode */
		NTV2_CON(ntv2_con_hdmiin4_hotplugmode_disable,				0x0);			/* disable edid */
		NTV2_CON(ntv2_con_hdmiin4_hotplugmode_enable,				0x1);			/* enable edid */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_resetdone,			1,	 30);		/* rx reset done */
	NTV2_FLD(ntv2_fld_hdmiin4_videocontrol_reset,	   			1,	 31);		/* rx reset */

NTV2_REG(ntv2_reg_hdmiin4_videodetect0,						0x1d01, 0x1d01, 0x2501);		/* video detect 0 register */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect0_colordepth,			2,	 0);		/* color depth */
   	NTV2_FLD(ntv2_fld_hdmiin4_videodetect0_colorspace,			2,	 2);		/* color space */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect0_scanmode,			1,	 4);		/* video scan mode */
		NTV2_CON(ntv2_con_hdmiin4_scanmode_interlaced,				0x0);			/* interlaced */
		NTV2_CON(ntv2_con_hdmiin4_scanmode_progressive,				0x1);			/* progressive */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect0_interfacemode,		1,	 5);		/* interface mode */
		NTV2_CON(ntv2_con_hdmiin4_interfacemode_hdmi,				0x0);			/* hdmi */
		NTV2_CON(ntv2_con_hdmiin4_interfacemode_dvi,				0x1);			/* dvi */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect0_syncpolarity,		1,	 6);		/* sync polarity */
		NTV2_CON(ntv2_con_hdmiin4_syncpolarity_activelow,			0x0);			/* active low */
		NTV2_CON(ntv2_con_hdmiin4_syncpolarity_activehigh,			0x1);			/* active high */

NTV2_REG(ntv2_reg_hdmiin4_videodetect1,						0x1d02, 0x1d02, 0x2502);		/* video detect 1 register */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect1_hsyncstart,			16,	 0);		/* horizontal sync start */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect1_hsyncend,			16,	 16);		/* horizontal sync end */

NTV2_REG(ntv2_reg_hdmiin4_videodetect2,						0x1d03, 0x1d03, 0x2503);		/* video detect 2 register */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect2_hdestart,			16,	 0);		/* horizontal de start */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect2_htotal,				16,	 16);		/* horizontal total */

NTV2_REG(ntv2_reg_hdmiin4_videodetect3,						0x1d04, 0x1d04, 0x2504);		/* video detect 3 register */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect3_vtransf1,			16,	 0);		/* vertical transistion field 1 */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect3_vtransf2,			16,	 16);		/* vertical transistion field 2 */

NTV2_REG(ntv2_reg_hdmiin4_videodetect4,						0x1d05, 0x1d05, 0x2505);		/* video detect 4 register */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect4_vsyncstartf1,		16,	 0);		/* vertical sync start field 1 */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect4_vsyncendf1,			16,	 16);		/* virtical sync end field 1 */

NTV2_REG(ntv2_reg_hdmiin4_videodetect5,						0x1d06, 0x1d06, 0x2506);		/* video detect 5 register */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect5_vdestartf1,			16,	 0);		/* vertical de start field 1 */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect5_vdestartf2,			16,	 16);		/* vertical de start field 2 */

NTV2_REG(ntv2_reg_hdmiin4_videodetect6,						0x1d07, 0x1d07, 0x2507);		/* video detect 6 register */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect6_vsyncstartf2,		16,	 0);		/* vertical sync start field 2 */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect6_vsyncendf2,			16,	 16);		/* virtical sync end field 2 */

NTV2_REG(ntv2_reg_hdmiin4_videodetect7,						0x1d08, 0x1d08, 0x2508);		/* video detect 7 register */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect7_vtotalf1,			16,	 0);		/* vertical total field 1 */
	NTV2_FLD(ntv2_fld_hdmiin4_videodetect7_vtotalf2,			16,	 16);		/* vertical total field 2 */

NTV2_REG(ntv2_reg_hdmiin4_auxcontrol,						0x1d09, 0x1d09, 0x2509);		/* video detect 9 register */
	NTV2_FLD(ntv2_fld_hdmiin4_auxcontrol_auxactive,				1,	 0);		/* aux data active bank */
		NTV2_CON(ntv2_con_hdmiin4_auxactive_bank0,					0x0);			/* bank 0 */
		NTV2_CON(ntv2_con_hdmiin4_auxactive_bank1,					0x1);			/* bank 1 */
	NTV2_FLD(ntv2_fld_hdmiin4_auxcontrol_auxread,				1,	 1);		/* aux data read bank */
		NTV2_CON(ntv2_con_hdmiin4_auxread_bank0,					0x0);			/* bank 0 */
		NTV2_CON(ntv2_con_hdmiin4_auxread_bank1,					0x1);			/* bank 1 */
	NTV2_FLD(ntv2_fld_hdmiin4_auxcontrol_auxwrite,				1,	 2);		/* aux data write bank */
		NTV2_CON(ntv2_con_hdmiin4_auxwrite_bank0,					0x0);			/* bank 0 */
		NTV2_CON(ntv2_con_hdmiin4_auxwrite_bank1,					0x1);			/* bank 1 */
	NTV2_FLD(ntv2_fld_hdmiin4_auxcontrol_bank0count,			8,	 8);		/* aux bank 0 packet count */
	NTV2_FLD(ntv2_fld_hdmiin4_auxcontrol_bank1count,			8,	 16);		/* aux bank 1 packet count */

NTV2_REG(ntv2_reg_hdmiin4_receiverstatus,					0x1d0a, 0x1d0a, 0x250a);		/* rx status */
	NTV2_FLD(ntv2_fld_hdmiin4_receiverstatus_errorcount,		24,	 0);			/* rx error count */

NTV2_REG(ntv2_reg_hdmiin4_auxpacketignore0,					0x1d0b, 0x1d0b, 0x250b);	/* aux packet ignore 0 */
NTV2_REG(ntv2_reg_hdmiin4_auxpacketignore1,					0x1d0c, 0x1d0c, 0x250c);	/* aux packet ignore 1 */
NTV2_REG(ntv2_reg_hdmiin4_auxpacketignore2,					0x1d0d, 0x1d0d, 0x250d);	/* aux packet ignore 2 */
NTV2_REG(ntv2_reg_hdmiin4_auxpacketignore3,					0x1d0e, 0x1d0e, 0x250e);	/* aux packet ignore 3 */

NTV2_REG(ntv2_reg_hdmiin4_redrivercontrol,					0x1d0f, 0x1d0f, 0x250f);	/* hdmi redriver control */
	NTV2_FLD(ntv2_fld_hdmiin4_redrivercontrol_power,			1,	 0);		/* power */
		NTV2_CON(ntv2_con_hdmiin4_power_disable,					0x0);			/* power disable */
		NTV2_CON(ntv2_con_hdmiin4_power_enable,						0x1);			/* power enable */
	NTV2_FLD(ntv2_fld_hdmiin4_redrivercontrol_pinmode,			1,	 1);		/* pin mode */
		NTV2_CON(ntv2_con_hdmiin4_pinmode_disable,					0x0);			/* pin disable */
		NTV2_CON(ntv2_con_hdmiin4_pinmode_enable,					0x1);			/* pin enable */
	NTV2_FLD(ntv2_fld_hdmiin4_redrivercontrol_vodrange,			1,	 2);		/* differential voltage range */
		NTV2_CON(ntv2_con_hdmiin4_vodrange_low,						0x0);			/* voltage swing low */
		NTV2_CON(ntv2_con_hdmiin4_vodrange_high,					0x1);			/* voltage swing high */
	NTV2_FLD(ntv2_fld_hdmiin4_redrivercontrol_deemphasis,		2,	 4);		/* deemphasis */
		NTV2_CON(ntv2_con_hdmiin4_deemphasis_0d0db,					0x0);			/* 0 db */
		NTV2_CON(ntv2_con_hdmiin4_deemphasis_3d5db,					0x1);			/* 3.5 db */
		NTV2_CON(ntv2_con_hdmiin4_deemphasis_6d0db,					0x2);			/* 6 db */
		NTV2_CON(ntv2_con_hdmiin4_deemphasis_9d5db,					0x3);			/* 9.5 db */
	NTV2_FLD(ntv2_fld_hdmiin4_redrivercontrol_preemphasis,		2,	 8);		/* preemphasis */
		NTV2_CON(ntv2_con_hdmiin4_preemphasis_0d0db,				0x0);			/* 0 db */
		NTV2_CON(ntv2_con_hdmiin4_preemphasis_1d6db,				0x1);			/* 1.6 db */
		NTV2_CON(ntv2_con_hdmiin4_preemphasis_3d5db,				0x2);			/* 3.5 db */
		NTV2_CON(ntv2_con_hdmiin4_preemphasis_6d0db,				0x3);			/* 6 db */
	NTV2_FLD(ntv2_fld_hdmiin4_redrivercontrol_boost,			4,	 12);		/* boost */
		NTV2_CON(ntv2_con_hdmiin4_boost_00d25db,					0x0);			/* 0.25 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_00d80db,					0x1);			/* 0.80 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_01d10db,					0x2);			/* 1.1 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_02d20db,					0x3);			/* 2.2 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_04d10db,					0x4);			/* 4.1 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_07d10db,					0x5);			/* 7.1 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_09d00db,					0x6);			/* 9.0 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_10d30db,					0x7);			/* 10.3 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_11d80db,					0x8);			/* 11.8 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_13d90db,					0x9);			/* 13.9 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_15d30db,					0xa);			/* 15.3 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_16d90db,					0xb);			/* 16.9 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_17d90db,					0xc);			/* 17.9 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_19d20db,					0xd);			/* 19.2 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_20d50db,					0xe);			/* 20.5 db */
		NTV2_CON(ntv2_con_hdmiin4_boost_22d20db,					0xf);			/* 22.2 db */

NTV2_REG(ntv2_reg_hdmiin4_refclockfrequency,				0x1d10, 0x1d10, 0x2510);	/* reference clock frequency */
NTV2_REG(ntv2_reg_hdmiin4_tmdsclockfrequency,				0x1d11, 0x1d11, 0x2511);	/* tmds clock frequency */
NTV2_REG(ntv2_reg_hdmiin4_rxclockfrequency,					0x1d12, 0x1d12, 0x2512);	/* rx clock frequency */

NTV2_REG(ntv2_reg_hdmiin4_rxoversampling,					0x1d13, 0x1d13, 0x2513);	/* rx oversampling */
	NTV2_FLD(ntv2_fld_hdmiin4_rxoversampling_ratiofraction,		10,	 0);		/* oversampling ratio fraction */
	NTV2_FLD(ntv2_fld_hdmiin4_rxoversampling_ratiointeger,		4,	 10);		/* oversampling ratio integer */
	NTV2_FLD(ntv2_fld_hdmiin4_rxoversampling_mode,				2,	 16);		/* oversampling mode */
		NTV2_CON(ntv2_con_hdmiin4_mode_none,						0x0);			/* no oversampling */
		NTV2_CON(ntv2_con_hdmiin4_mode_asynchronous,				0x1);			/* asynchronous oversampling */
		NTV2_CON(ntv2_con_hdmiin4_mode_synchronous,					0x2);			/* synchronous oversampling */

NTV2_REG(ntv2_kona_reg_hdmiin4_edid,						0x1d1d, 0x1d1d, 0x251d);
	NTV2_FLD(ntv2_kona_fld_hdmiin4_edid_write_data,				8,	 0);
	NTV2_FLD(ntv2_kona_fld_hdmiin4_edid_read_data,				8,	 8);
	NTV2_FLD(ntv2_kona_fld_hdmiin4_edid_address,				8,	 16);
	NTV2_FLD(ntv2_kona_fld_hdmiin4_edid_write_enable,			1,	 24);
	NTV2_FLD(ntv2_kona_fld_hdmiin4_edid_busy,					1,	 25);

NTV2_REG(ntv2_reg_hdmiin4_croplocation,						0x1d1e, 0x1d1e, 0x251e);	/* crop location */
	NTV2_FLD(ntv2_fld_hdmiin4_croplocation_start,				16,	 0);		/* crop start location */
	NTV2_FLD(ntv2_fld_hdmiin4_croplocation_end,					16,	 16);		/* crop end location */

NTV2_REG(ntv2_reg_hdmiin4_pixelcontrol,						0x1d1f, 0x1d1f, 0x251f);	/* pixel control */
	NTV2_FLD(ntv2_fld_hdmiin4_pixelcontrol_lineinterleave,		1,	0);		/* line interleave */
		NTV2_CON(ntv2_con_hdmiin4_lineinterleave_disable,			0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiin4_lineinterleave_enable,			0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiin4_pixelcontrol_pixelinterleave,		1,	1);		/* pixel interleave */
		NTV2_CON(ntv2_con_hdmiin4_pixelinterleave_disable,			0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiin4_pixelinterleave_enable,			0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiin4_pixelcontrol_420convert,			1,	2);		/* 420 to 422 conversion */
		NTV2_CON(ntv2_con_hdmiin4_420convert_disable,				0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiin4_420convert_enable,				0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiin4_pixelcontrol_cropmode,			1,	 3);	/* crop mode */
		NTV2_CON(ntv2_con_hdmiin4_cropmode_disable,					0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiin4_cropmode_enable,					0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiin4_pixelcontrol_hlinefilter,			1,	 4);	/* horizontal line filter mode */
		NTV2_CON(ntv2_con_hdmiin4_hlinefilter_disable,				0x0);		/* disable */
		NTV2_CON(ntv2_con_hdmiin4_hlinefilter_enable,				0x1);		/* enable */
	NTV2_FLD(ntv2_fld_hdmiin4_pixelcontrol_clockratio,			4,	 8);	/* core clock to data clock ratio */

NTV2_REG(ntv2_vreg_hdmiin4_avi_info,						kVRegHDMIInAviInfo1,
		 													kVRegHDMIInAviInfo1,
		 													kVRegHDMIInAviInfo2);	/* avi info data */

	NTV2_FLD(ntv2_fld_hdmiin4_colorimetry,						4,	0);			/* colorimetry */
	NTV2_FLD(ntv2_fld_hdmiin4_dolby_vision,						1,	4);			/* dolby vision detected */	

NTV2_REG(ntv2_vreg_hdmiin4_drm_info,						kVRegHDMIInDrmInfo1,
															kVRegHDMIInDrmInfo1,
															kVRegHDMIInDrmInfo2);	/* drm info data */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_present,						1,	0);		/* drm info frame present */	
	NTV2_FLD(ntv2_fld_hdmiin4_drm_eotf,							4,	8);		/* electro optical transfer function */	
	NTV2_FLD(ntv2_fld_hdmiin4_drm_metadata_id,					4,	12);	/* metadata descriptor id */	

NTV2_REG(ntv2_vreg_hdmiin4_drm_green,						kVRegHDMIInDrmGreenPrimary1,
															kVRegHDMIInDrmGreenPrimary1,
															kVRegHDMIInDrmGreenPrimary2);	/* drm green primary */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_green_x,						16,	 0);	/* green primary x */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_green_y,						16,	 16);	/* green primary y */

NTV2_REG(ntv2_vreg_hdmiin4_drm_blue,						kVRegHDMIInDrmBluePrimary1,
															kVRegHDMIInDrmBluePrimary1,
															kVRegHDMIInDrmBluePrimary2);	/* drm blue primary */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_blue_x,						16,	 0);	/* blue primary x */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_blue_y,						16,	 16);	/* blue primary y */

NTV2_REG(ntv2_vreg_hdmiin4_drm_red,							kVRegHDMIInDrmRedPrimary1,
															kVRegHDMIInDrmRedPrimary1,
															kVRegHDMIInDrmRedPrimary2);		/* drm red primary */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_red_x,						16,	 0);	/* red primary x */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_red_y,						16,	 16);	/* red primary y */

NTV2_REG(ntv2_vreg_hdmiin4_drm_white,						kVRegHDMIInDrmWhitePoint1,
															kVRegHDMIInDrmWhitePoint1,
															kVRegHDMIInDrmWhitePoint2);		/* drm white point */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_white_x,						16,	 0);	/* white point x */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_white_y,						16,	 16);	/* white point y */

NTV2_REG(ntv2_vreg_hdmiin4_drm_luma,						kVRegHDMIInDrmMasteringLuminence1,
															kVRegHDMIInDrmMasteringLuminence1,
															kVRegHDMIInDrmMasteringLuminence2);	/* drm luminence level */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_luma_max,						16,	 0);	/* luminence max */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_luma_min,						16,	 16);	/* luminence min */

NTV2_REG(ntv2_vreg_hdmiin4_drm_light,						kVRegHDMIInDrmLightLevel1,
															kVRegHDMIInDrmLightLevel1,
															kVRegHDMIInDrmLightLevel2);		/* drm light level */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_light_content_max,			16,	 0);	/* light level content max */
	NTV2_FLD(ntv2_fld_hdmiin4_drm_light_average_max,			16,	 16);	/* light level average max */

#endif
