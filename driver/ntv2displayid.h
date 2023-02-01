/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
////////////////////////////////////////////////////////////
//
// Filename: ntv2dispalyid.h
// Purpose:	 HDMI edid parser
//
///////////////////////////////////////////////////////////////

#ifndef NTV2_DISPLAYID_H
#define NTV2_DISPLAYID_H

#include "ntv2system.h"


enum ntv2_displayid_protocol
{
	ntv2_displayid_protocol_unknown,
	ntv2_displayid_protocol_dvi,		/* no CEA-861 extension (therefore, no audio, RGB only) */
	ntv2_displayid_protocol_hdmiv1,		/* adds CEA-861 extension rev 1, timing only */
	ntv2_displayid_protocol_hdmi,		/* adds CEA-861 extension rev >1, audio, YCbCr and DeepColor options */
	ntv2_displayid_protocol_size
};

struct ntv2_displayid_video
{
	enum ntv2_displayid_protocol	protocol;	/* DVI vs HDMI */

	bool	underscan;			/* true if sink device underscans IT video formats by default */
	bool	ycbcr_444;			/* true if sink device supports YCbCr 4:4:4 in addition to RGB */
	bool	ycbcr_422;			/* true if sink device supports YCbCr 4:2:2 in addition to RGB */
	bool	ycbcr_420;			/* true if sink device supports YCbCr 4:2:0 in addition to RGB */
	bool	dc_48bit;			/* true if sink device supports 48-bit (16 bits per component) RGB */
	bool	dc_36bit;			/* true if sink device supports 36-bit (12 bits per component) RGB */
	bool	dc_30bit;			/* true if sink device supports 30-bit (10 bits per component) RGB */
	bool	dc_y444;			/* true if the 48-bit, 36-bit, and 30-bit flags ALSO apply to YCbCr inputs */

	uint32_t max_clock_freq;	/* maximum TMDS clock frequency supported by sink device (MHz / 0 = unknown) */

	bool	graphics;			/* true if sink device has special graphics processing */
	bool	photo;				/* true if sink device has special photo processing */
	bool	cinema;				/* true if sink device has special cinema processing */
	bool	game;				/* true if sink device has special game processing */

	uint32_t video_latency;		/* video latency (mSec / 0 = unknown) */
	uint32_t audio_latency;		/* audio latency (mSec / 0 = unknown) */
	uint32_t int_video_latency;	/* interlaced video latency (mSec / 0 = unknown) */
	uint32_t int_audio_latency;	/* interlaced audio latency (mSec / 0 = unknown) */

	bool	quad_30;			/* true if sink device supports 1920x2160 29.97/30 */
	bool	quad_25;			/* true if sink device supports 1920x2160 25 */
	bool	quad_24;			/* true if sink device supports 1920x2160 23.98/24 */
	bool	four_24;			/* true if sink device supports 2048x2160 24 */

	uint32_t max_tmds_csc;		/* maximum TMDS character rate supported above 340 Mcsc (0 = not supported) */

	bool	osd_disparity;		/* true if sink supports receiving 3d osd displarity in the hf-vsif */
	bool	dual_view;			/* true if sink supports 3d dual view */
	bool	indep_view;			/* true if sink supports 3d independent view */
	bool	lte_scramble;		/* true if sink supports scrambling below 340 Mcsc */
	bool	rr_capable;			/* true if sink supports scdc read request */
	bool	scdc_present;		/* true if sink supports scdc */
	bool	dc_30bit_420;		/* true if sink supports 30 bit 420 */
	bool	dc_36bit_420;		/* true if sink supports 36 bit 420 */
	bool	dc_48bit_420;		/* true if sink supports 48 bit 420 */
};

struct ntv2_displayid_audio
{
	enum ntv2_displayid_protocol	protocol;	/* DVI vs HDMI */

	bool	basic_audio;			/* true if sink device supports basic audio */

	uint32_t num_lpcm_channels;		/* max number of channels from LPCM Short Audio Descriptors (if present) */
};


/**
 *	HDMI EDID register callback function
 *
 *	Callback to obtain EDID register data.
 *
 *	@param[in]	pContext			Callback context
 *	@param[in]	blockNum			Register block number
 *	@param[in]	regNum				Register number within the block
 *	@param[out]	regVal				Register value
 *	@return							AJA_STATUS_SUCCESS if register read works
 */
typedef bool ntv2_displayid_callback(void* context, uint8_t block_num, uint8_t reg_num, uint8_t* reg_val);


struct ntv2_displayid
{
	struct ntv2_displayid_video		video;
	struct ntv2_displayid_audio		audio;
	ntv2_displayid_callback*		callback;
	void*							context;
};


#ifdef __cplusplus
extern "C"
{
#endif

void ntv2_displayid_config(struct ntv2_displayid* ntv2_did, ntv2_displayid_callback* callback, void* context);
void ntv2_displayid_clear(struct ntv2_displayid* ntv2_did);
bool ntv2_displayid_update(struct ntv2_displayid* ntv2_did);

#ifdef __cplusplus
}
#endif

#endif
