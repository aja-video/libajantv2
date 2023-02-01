/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2displayid.c
//
//==========================================================================

#include "ntv2displayid.h"

/* Extension Tags are assigned by VESA: see VESA Enhanced EDID Standard, Release A, Rev 1, page 8. */
enum ntv2_edid_extension_tag
{
	ntv2_edid_extension_tag_unused				= 0x00,		/* used in Block Maps to signify "unused" block map entry */
	ntv2_edid_extension_tag_lcdtimings			= 0x01,
	ntv2_edid_extension_tag_cea861_hdmi			= 0x02,
	ntv2_edid_extension_tag_edid2				= 0x20,
	ntv2_edid_extension_tag_colorinfotype0		= 0x30,
	ntv2_edid_extension_tag_dvifeature			= 0x40,
	ntv2_edid_extension_tag_touchscreen			= 0x50,
	ntv2_edid_extension_tag_blockmap			= 0xf0,
	ntv2_edid_extension_tag_monitormfgr			= 0xff
};

/* These tags are defined in CEA-861 (Rev E: pg 64) and are used to identify specific Data Blocks in the CEA-861 Extension */
enum ntv2_cea861_datablock_tag
{
	ntv2_cea861_datablock_tag_reserved0	= 0,		/* Reserved */
	ntv2_cea861_datablock_tag_wildcard   = ntv2_cea861_datablock_tag_reserved0,	/* "Wild card" - for NTV2 use only for unconstrained searches */
	ntv2_cea861_datablock_tag_audio		= 1,		/* Audio Data Block (includes one or more Short Audio Descriptors) */
	ntv2_cea861_datablock_tag_video		= 2,		/* Video Data Block (includes one or more Short Video Descriptors) */
	ntv2_cea861_datablock_tag_vendor	= 3,		/* Vendor Specific Data Block */
	ntv2_cea861_datablock_tag_speaker	= 4,		/* Speaker Allocation Data Block */
	ntv2_cea861_datablock_tag_vesa_dtc	= 5,		/* VESA DTC Data Block */
	ntv2_cea861_datablock_tag_reserved6	= 6,		/* Reserved */
	ntv2_cea861_datablock_tag_extended	= 7			/* Use Extended Tag (see below) */
};

/* When the Data Block Tag is set to "Extended" (0x07), these Extended Tags are used in a separate */
/* "Extended Tag Format" byte to identify the type of Data Block (see CEA-861 Rev F, pg 78). */
enum ntv2_cea861_datablock_extended_tag
{
	ntv2_cea861_datablock_extended_tag_videocapability		= 0,		/* Video Capability Data Block */
	ntv2_cea861_datablock_extended_tag_vendorspecific		= 1,		/* Vendor-Specific Video Data Block */
	ntv2_cea861_datablock_extended_tag_vesadeviceinfo		= 2,		/* VESA Video Display Device Information Data Block */
	ntv2_cea861_datablock_extended_tag_vesavideo			= 3,		/* VESA Video Data Block */
	ntv2_cea861_datablock_extended_tag_hdmivideo			= 4,		/* HDMI Video Data Block */
	ntv2_cea861_datablock_extended_tag_colorimetry			= 5,		/* Colorimetry Data Block */
/*	ntv2_cea861_datablock_extended_tag_reserved06-12		= 6-12,		   Reserved for video-related blocks */

	ntv2_cea861_datablock_extended_tag_videoformatblock		= 13,		/* Video Format Preference Data Block */
	ntv2_cea861_datablock_extended_tag_ycbcr420datablock	= 14,		/* YCbCr 4:2:0 Video Data Block */
	ntv2_cea861_datablock_extended_tag_ycbcr420capmap		= 15,		/* YCbCr 4:2:0 Capability Map Data Block */
	ntv2_cea861_datablock_extended_tag_miscaudio			= 16,		/* CEA Miscellaneous Audio Fields */
	ntv2_cea861_datablock_extended_tag_vendoraudio			= 17,		/* Vendor-Specific Audio Data Block */
	ntv2_cea861_datablock_extended_tag_hdmiaudio			= 18,		/* HDMI Audio Data Block */
/*	ntv2_cea861_datablock_extended_tag_reserved19-31		= 19-31,	   Reserved for audio-related blocks */

/*	ntv2_cea861_datablock_extended_tag_reserved32-254		= 32-254,	   Reserved for general */
	ntv2_cea861_datablock_extended_tag_reserved255			= 255		/* Reserved for general */
};


/* These codes are defined in CEA-861 (Rev E: pg 48) and are used to identify */
/* Short Audio Descriptors in the CEA-861 Extension */
enum ntv2_cea861_audio_format
{
	ntv2_cea861_audio_format_reserved	= 0,	/* (in Audio InfoFrames this indicates "look in the stream data for format flags") */
	ntv2_cea861_audio_format_wildcard	= ntv2_cea861_audio_format_reserved,	/* used in NTV2 for "wildcard" searches (e.g. for generic iterators) */
	ntv2_cea861_audio_format_lpcm		= 1,	/* Linear PCM (IEC 60958-3) */
	ntv2_cea861_audio_format_ac3		= 2,	/* AC-3 (ATSC A/52B, excluding Annex E) */
	ntv2_cea861_audio_format_mpeg1		= 3,	/* MPEG-1 (ISO/IEC 11172-3 Layer 1 or Layer 2) */
	ntv2_cea861_audio_format_mp3		= 4,	/* MP3 (ISO/IEC 11172-3 Layer 3) */
	ntv2_cea861_audio_format_mpeg2		= 5,	/* MPEG2 (ISO/IEC 13818-3) */
	ntv2_cea861_audio_format_aaclc		= 6,	/* AAC LC (ISO/IEC 14496-3) */
	ntv2_cea861_audio_format_dts		= 7,	/* DTS (ETSI TS 102 114) */
	ntv2_cea861_audio_format_atrac		= 8,	/* ATRAC (IEC 61909) */
	ntv2_cea861_audio_format_dsd		= 9,	/* DSD (ISO/IEC 14496-3, subpart 10 (see also Super Audio CD) ) */
	ntv2_cea861_audio_format_eac3		= 10,	/* E-AC-3 (ATSC A/52B with Annex E) */
	ntv2_cea861_audio_format_dtshd		= 11,	/* DTS-HD (DVD Forum DTS-HD) */
	ntv2_cea861_audio_format_mlp		= 12,	/* MLP (DVD Forum MLP) */
	ntv2_cea861_audio_format_dst		= 13,	/* DST (ISO/IEC 14496-3, subpart 10) */
	ntv2_cea861_audio_format_wmapro		= 14,	/* WMA Pro (WMA Pro Decoder Specification) */
	ntv2_cea861_audio_format_extended	= 15	/* Refer to Audio Coding Extension field */
};

/* These codes are defined in High Definition Multimedia Interface (Rev 1.4b: pg 143) and are used to identify */
/* supported quad/4k hdmi formats */
enum ntv2_hdmi_video_format
{
	ntv2_hdmi_video_format_quad30		= 1,
	ntv2_hdmi_video_format_quad25		= 2,
	ntv2_hdmi_video_format_quad24		= 3,
	ntv2_hdmi_video_format_four24		= 4
};

struct ntv2_displayid_audio_format
{
	enum ntv2_cea861_audio_format	audio_format;	/* audio format code as defined in CEA-861 */
	uint8_t		num_channels;	/* number of audio channels supported */

	bool		b192khz;		/* if true, sink supports 192.0 kHz sampling for this audio format */
	bool		b176khz;		/* if true, sink supports 176.4 kHz sampling for this audio format */
	bool		b96khz;			/* if true, sink supports 96.0 kHz sampling for this audio format */
	bool		b88khz;			/* if true, sink supports 88.2 kHz sampling for this audio format */
	bool		b48khz;			/* if true, sink supports 48.0 kHz sampling for this audio format */
	bool		b44khz;			/* if true, sink supports 44.1 kHz sampling for this audio format */
	bool		b32khz;			/* if true, sink supports 32.0 kHz sampling for this audio format */

	bool		lpcm_24bit;		/* if true, sink supports 24 bits per audio sample (LPCM format only) */
	bool		lpcm_20bit;		/* if true, sink supports 20 bits per audio sample (LPCM format only) */
	bool		lpcm_16bit;		/* if true, sink supports 16 bits per audio sample (LPCM format only) */

	uint8_t		byte3;			/* byte #3 of the Short Audio Descriptor (content depends on format) */
};


/* Base EDID register addresses (Block 0) */
static const uint8_t edid_extensions_reg_addr 			= 0x7E;		/* the number of (optional) extensions added to this data (0 - 254) */

/* EDID Extension register addresses (common to all extensions) */
static const uint8_t edid_extension_tag_reg_addr 				= 0x00;
static const uint8_t edid_extension_revision_reg_addr			= 0x01;
static const uint8_t edid_extension_checksum_reg_addr			= 0x7f;

static const uint8_t cea861_vendorspecific_ieeecode_hf_ms		= 0xC4;
static const uint8_t cea861_vendorspecific_ieeecode_hf_mid		= 0x5D;
static const uint8_t cea861_vendorspecific_ieeecode_hf_ls		= 0xD8;

static const uint8_t cea861_vendorspecific_ieeecode_hdmi_ms		= 0x00;
static const uint8_t cea861_vendorspecific_ieeecode_hdmi_mid	= 0x0C;
static const uint8_t cea861_vendorspecific_ieeecode_hdmi_ls		= 0x03;

static bool get_video_status(struct ntv2_displayid* ntv2_did, struct ntv2_displayid_video* video_status);

static bool get_audio_status(struct ntv2_displayid* ntv2_did, struct ntv2_displayid_audio* audio_status);

static bool get_edid_sink_protocol(struct ntv2_displayid* ntv2_did, enum ntv2_displayid_protocol* protocol);

static bool get_edid_num_extensions(struct ntv2_displayid* ntv2_did, uint8_t* num_ext);

static bool get_block_number_for_extension(struct ntv2_displayid* ntv2_did, enum ntv2_edid_extension_tag tag, uint8_t* block_num);

//static bool get_edid_extension_tag(struct ntv2_displayid* ntv2_did, uint8_t block_num, uint8_t* tag_val);

static bool get_edid_extension_revision(struct ntv2_displayid* ntv2_did, uint8_t block_num, uint8_t* rev_val);

//static bool get_edid_extension_checksum_reg(struct ntv2_displayid* ntv2_did, uint8_t block_num, uint8_t* cs_reg);

static bool get_global_video_status(struct ntv2_displayid* ntv2_did,
									uint8_t block_num,
									struct ntv2_displayid_video* video_status);

static bool get_hdmi_video_status(struct ntv2_displayid* ntv2_did,
								  uint8_t block_num,
								  struct ntv2_displayid_video* video_status);

static bool get_hf_video_status(struct ntv2_displayid* ntv2_did,
								uint8_t block_num,
								struct ntv2_displayid_video* video_status);

static bool get_global_audio_status(struct ntv2_displayid* ntv2_did, 
									uint8_t block_num,
									struct ntv2_displayid_audio* audio_status);

static bool get_audio_descriptor(struct ntv2_displayid* ntv2_did,
								 uint8_t block_num,
								 enum ntv2_cea861_audio_format format, 
								 uint8_t inst,
								 struct ntv2_displayid_audio_format* descriptor);

static bool get_index_for_audio_descriptor(struct ntv2_displayid* ntv2_did,
										   uint8_t block_num,
										   enum ntv2_cea861_audio_format format,
										   uint8_t inst,
										   uint8_t* index);

static bool get_index_for_hdmi_datablock(struct ntv2_displayid* ntv2_did,
										 uint8_t block_num,
										 uint8_t* index);

static bool get_index_for_hf_datablock(struct ntv2_displayid* ntv2_did,
									   uint8_t block_num,
									   uint8_t* index);

static bool get_index_for_datablock(struct ntv2_displayid* ntv2_did,
									uint8_t block_num,
									enum ntv2_cea861_datablock_tag db_tag,
									enum ntv2_cea861_datablock_extended_tag db_ext_tag,
									uint8_t inst,
									uint8_t* db_index,
									uint8_t* db_length);

static bool get_datablock_header(struct ntv2_displayid* ntv2_did,
								 uint8_t block_num,
								 uint8_t db_index,
								 enum ntv2_cea861_datablock_tag* db_tag,
								 enum ntv2_cea861_datablock_extended_tag* db_ext_tag,
								 uint8_t* num_bytes);

static bool read_edid_registers(struct ntv2_displayid* ntv2_did, 
								uint8_t block_num, 
								uint8_t reg_addr, 
								uint32_t num_regs, 
								uint8_t* data);

static bool read_edid_register(struct ntv2_displayid* ntv2_did, 
							   uint8_t block_num, 
							   uint8_t reg_num, 
							   uint8_t* reg_val);


void ntv2_displayid_config(struct ntv2_displayid* ntv2_did, ntv2_displayid_callback* callback, void* context)
{
	ntv2_displayid_clear(ntv2_did);
	ntv2_did->callback = callback;
	ntv2_did->context = context;
}

void ntv2_displayid_clear(struct ntv2_displayid* ntv2_did)
{
	if (ntv2_did == NULL) return;

	memset(&ntv2_did->video, 0, sizeof(struct ntv2_displayid_video));
	memset(&ntv2_did->audio, 0, sizeof(struct ntv2_displayid_audio));
}

bool ntv2_displayid_update(struct ntv2_displayid* ntv2_did)
{
	if (!get_video_status(ntv2_did, &ntv2_did->video)) return false;
	if (!get_audio_status(ntv2_did, &ntv2_did->audio)) return false;

	return true;
}


static bool get_video_status(struct ntv2_displayid* ntv2_did, struct ntv2_displayid_video* video_status)
{
	if (ntv2_did == NULL) return false;
	if (video_status == NULL) return false;

	memset(video_status, 0, sizeof(struct ntv2_displayid_video));

	if (!get_edid_sink_protocol(ntv2_did, &video_status->protocol)) return false;

	if (video_status->protocol == ntv2_displayid_protocol_hdmi)
	{
		uint8_t block_num = 0;

		/* get block for hdmi status */
		if (!get_block_number_for_extension(ntv2_did, ntv2_edid_extension_tag_cea861_hdmi, &block_num)) return false;
		if (block_num == 0) return true;

		/* get the CEA-861 specific flags */
		if (!get_global_video_status(ntv2_did, block_num, video_status)) return false;

		if (!get_hdmi_video_status(ntv2_did, block_num, video_status)) return false;

		if (!get_hf_video_status(ntv2_did, block_num, video_status)) return false;
	}

	return true;
}

static bool get_audio_status(struct ntv2_displayid* ntv2_did, struct ntv2_displayid_audio* audio_status)
{
	if (ntv2_did == NULL) return false;
	if (audio_status == NULL) return false;

	memset(&ntv2_did->audio, 0, sizeof(struct ntv2_displayid_audio));

	if (!get_edid_sink_protocol(ntv2_did, &audio_status->protocol)) return false;

	if (audio_status->protocol == ntv2_displayid_protocol_hdmi)
	{
		uint8_t block_num = 0;

		/* get block for hdmi status */
		if (!get_block_number_for_extension(ntv2_did, ntv2_edid_extension_tag_cea861_hdmi, &block_num)) return false;
		if (block_num == 0) return true;

		/* get the CEA-861 specific flags */
		if (!get_global_audio_status(ntv2_did, block_num, audio_status)) return false;

		/* if the Basic Audio flag is off (false), we're done */
		if (audio_status->basic_audio)
		{
			uint8_t inst;

			/* if Basic Audio is on, then we can say we have at least two PCM channels */
			audio_status->num_lpcm_channels = 2;

			/* look for more channels by scanning the Audio Data Block for LPCM Short Audio Descriptors */
			for (inst = 0; inst < 255; inst++)
			{
				struct ntv2_displayid_audio_format local_descriptor;
				memset(&local_descriptor, 0, sizeof(local_descriptor));

				// iterate through all of the LPCM Short Audio Descriptors
				if (!get_audio_descriptor(ntv2_did, block_num, ntv2_cea861_audio_format_lpcm, inst, &local_descriptor)) return false;
				if (local_descriptor.audio_format != ntv2_cea861_audio_format_lpcm) break;

				/* find the MAX number of channels from ALL of the LPCM descriptors */
				if (local_descriptor.num_channels > audio_status->num_lpcm_channels)
					audio_status->num_lpcm_channels = local_descriptor.num_channels;
			}
		}
	}

	return true;
}

static bool get_edid_sink_protocol(struct ntv2_displayid* ntv2_did, enum ntv2_displayid_protocol* protocol)
{
	uint8_t block_num = 0;
	uint8_t rev_val = 0;

	if (ntv2_did == NULL) return false;
	if (protocol == NULL) return false;

	/* is there a CEA-861 extension? if not, then we're DVI only */
	if (!get_block_number_for_extension(ntv2_did, ntv2_edid_extension_tag_cea861_hdmi, &block_num)) return false;
	if (block_num == 0 || block_num == 255)
	{
		*protocol = ntv2_displayid_protocol_dvi;
	}
	else
	{
		if (!get_edid_extension_revision(ntv2_did, block_num, &rev_val)) return false;
		if (rev_val == 0)
		{
			*protocol = ntv2_displayid_protocol_dvi;
		}
		else if (rev_val == 1)
		{
			*protocol = ntv2_displayid_protocol_hdmiv1;
		}
		else
		{
			*protocol = ntv2_displayid_protocol_hdmi;
		}
	}

	return true;
}

static bool get_edid_num_extensions(struct ntv2_displayid* ntv2_did, uint8_t* num_ext)
{
	uint8_t reg_val;

	if (ntv2_did == NULL) return false;
	if (num_ext == NULL) return false;

	if (!read_edid_register(ntv2_did, 0, edid_extensions_reg_addr, &reg_val))
	{
		*num_ext = 0;
		return false;
	}
	*num_ext = reg_val;

	return true;
}

static bool get_block_number_for_extension(struct ntv2_displayid* ntv2_did, enum ntv2_edid_extension_tag tag, uint8_t* block_num)
{
	uint8_t num_ext = 0;
	uint8_t block_tag = 0;

	if (ntv2_did == NULL) return false;
	if (block_num == NULL) return false;

	/* are there ANY extensions on the EDID? */
	if (!get_edid_num_extensions(ntv2_did, &num_ext)) return false;
	if (num_ext == 0)
	{
		*block_num = 0;
		return true;
	}

	/* note: we currently only allow for one extension - so that extension had better be the CEA-861 extension */
	if (!read_edid_register(ntv2_did, 1, edid_extension_tag_reg_addr, &block_tag))
	{
		*block_num = 0;
		return false;
	}

	/* does Block 1 have the tag we're looking for? */
	if (block_tag == tag)
		*block_num = 1;
	else
		*block_num = 0;

	return true;
}

#if 0
static bool get_edid_extension_tag(struct ntv2_displayid* ntv2_did, uint8_t block_num, uint8_t* tag_val)
{
	uint8_t reg_val;

	if (ntv2_did == NULL) return false;
	if (block_num < 1 || block_num > 254) return false;
	if (tag_val == NULL) return false;

	if (!read_edid_register(ntv2_did, block_num, edid_extension_tag_reg_addr, &reg_val))
	{
		*tag_val = 0;
		return false;
	}
	*tag_val = reg_val;

	return true;
}
#endif
static bool get_edid_extension_revision(struct ntv2_displayid* ntv2_did, uint8_t block_num, uint8_t* rev_val)
{
	uint8_t reg_val;

	if (ntv2_did == NULL) return false;
	if (block_num < 1 || block_num > 254) return false;
	if (rev_val == NULL) return false;

	if (!read_edid_register(ntv2_did, block_num, edid_extension_revision_reg_addr, &reg_val))
	{
		*rev_val = 0;
		return false;
	}
	*rev_val = reg_val;

	return true;
}

#if 0
static bool get_edid_extension_checksum_reg(struct ntv2_displayid* ntv2_did, uint8_t block_num, uint8_t* cs_reg)
{
	uint8_t reg_val;

	if (ntv2_did == NULL) return false;
	if (block_num < 1 || block_num > 254) return false;
	if (cs_reg == NULL) return false;

	if (!read_edid_register(ntv2_did, block_num, edid_extension_checksum_reg_addr, &reg_val))
	{
		*cs_reg = 0;
		return false;
	}
	*cs_reg = reg_val;

	return true;
}
#endif
static bool get_global_video_status(struct ntv2_displayid* ntv2_did,
									uint8_t block_num,
									struct ntv2_displayid_video* video_status)
{
	uint8_t reg_val = 0;
	uint8_t tag_length;
	uint8_t ndx;

	if (ntv2_did == NULL) return false;
	if (block_num == 0) return false;

	/* read byte #3 (the 4th byte) to get the global support bits */
	if (!read_edid_register(ntv2_did, block_num, 3, &reg_val)) return false;
	video_status->underscan = ((reg_val & 0x80) == 0) ? false : true;
	video_status->ycbcr_444  = ((reg_val & 0x20) == 0) ? false : true;
	video_status->ycbcr_422  = ((reg_val & 0x10) == 0) ? false : true;
	video_status->ycbcr_420 = false;

	/* The "Data Block Collection" starts at byte 4 of the extension */
	ndx = 4;
	while (ndx < 128)
	{
		if (!read_edid_register(ntv2_did, block_num, ndx, &reg_val)) return false;
		tag_length = (0x1F & reg_val);

		/* Does this Data Block Tag indicate that an Extended Tag follows? */
		if ((ntv2_cea861_datablock_tag_extended << 5) == (0xe0 & reg_val))
		{
			if (!read_edid_register(ntv2_did, block_num, ndx + 1, &reg_val)) return false;

			/* Do we have an Extended Tag Code that interests us? */
			switch (reg_val)
			{
				case ntv2_cea861_datablock_extended_tag_videocapability:
					break;
				case ntv2_cea861_datablock_extended_tag_vendorspecific:
					break;
				case ntv2_cea861_datablock_extended_tag_vesadeviceinfo:
					break;
				case ntv2_cea861_datablock_extended_tag_vesavideo:
					break;
				case ntv2_cea861_datablock_extended_tag_hdmivideo:
					break;
				case ntv2_cea861_datablock_extended_tag_colorimetry:
					break;
				/* NTV2_CEA861_datablock_extended_tag_Reserved06-12  Reserved for video-related blocks */
				case ntv2_cea861_datablock_extended_tag_videoformatblock:
					break;
				case ntv2_cea861_datablock_extended_tag_ycbcr420datablock:
					video_status->ycbcr_420 = true;
					break;
				case ntv2_cea861_datablock_extended_tag_ycbcr420capmap:
					video_status->ycbcr_420 = true;
					break;
				case ntv2_cea861_datablock_extended_tag_miscaudio:
					break;
				case ntv2_cea861_datablock_extended_tag_vendoraudio:
					break;
				case ntv2_cea861_datablock_extended_tag_hdmiaudio:
					break;
				/* NTV2_CEA861_datablock_extended_tag_Reserved19-31   Reserved for audio-related blocks */
				/* NTV2_CEA861_datablock_extended_tag_Reserved32-254  Reserved for general */
				case ntv2_cea861_datablock_extended_tag_reserved255:
					break;
				default:
					break;
			}
		}

		if (tag_length == 0)
		{
			break;
		}

		/* Account for the Data Block Tag Code byte */
		ndx += (tag_length + 1);
	}

	return true;
}

static bool get_hdmi_video_status(struct ntv2_displayid* ntv2_did,
								  uint8_t block_num,
								  struct ntv2_displayid_video* video_status)
{
	uint8_t db_index = 0;

	if (ntv2_did == NULL) return false;
	if (block_num == 0) return false;

	// find the HDMI Vendor Specific Data Block
	if (!get_index_for_hdmi_datablock(ntv2_did, block_num, &db_index)) return false;
	if (db_index != 0)
	{
		uint8_t index = db_index;
		uint8_t dc_reg = 0;
		uint8_t block_length = 0;

		/*  extract the Deep Color status bits from byte #6 */
		if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
		block_length = dc_reg & 0x1f;

		index += 6;
		if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
		if (block_length >= (index - db_index))
		{
			video_status->dc_48bit = ((dc_reg & 0x40) == 0) ? false : true;
			video_status->dc_36bit = ((dc_reg & 0x20) == 0) ? false : true;
			video_status->dc_30bit = ((dc_reg & 0x10) == 0) ? false : true;
			video_status->dc_y444 = ((dc_reg & 0x08) == 0) ? false : true;
		}

		index++;
		if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
		if (block_length >= (index - db_index))
		{
			video_status->max_clock_freq = dc_reg * 5;
		}

		index++;
		if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
		if (block_length >= (index - db_index))
		{
			bool lat_present = ((dc_reg & 0x80) == 0) ? false : true;
			bool ilat_present = ((dc_reg & 0x40) == 0) ? false : true;
			bool hdmi_present = ((dc_reg & 0x20) == 0) ? false : true;

			video_status->game = ((dc_reg & 0x08) == 0) ? false : true;
			video_status->cinema = ((dc_reg & 0x04) == 0) ? false : true;
			video_status->photo = ((dc_reg & 0x02) == 0) ? false : true;
			video_status->graphics = ((dc_reg & 0x01) == 0) ? false : true;

			if (lat_present)
			{
				index++;
				if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
				if ((dc_reg > 0) && (block_length >= (index - db_index)))
				{
					video_status->video_latency = (dc_reg - 1) * 2;
				}
				index++;
				if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
				if ((dc_reg > 0) && (block_length >= (index - db_index)))
				{
					video_status->audio_latency = (dc_reg - 1) * 2;
				}
				if (ilat_present)
				{
					index++;
					if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
					if ((dc_reg > 0) && (block_length >= (index - db_index)))
					{
						video_status->int_video_latency = (dc_reg - 1) * 2;
					}
					index++;
					if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
					if ((dc_reg > 0) && (block_length >= (index - db_index)))
					{
						video_status->int_audio_latency = (dc_reg - 1) * 2;
					}
				}
				else
				{
					video_status->int_video_latency = video_status->video_latency;
					video_status->int_audio_latency = video_status->audio_latency;
				}
			}

			if (hdmi_present)
			{
				index++;  /* skip 3D stuff for now */

				index++;
				if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
				if (block_length >= (index - db_index))
				{
					uint8_t vic_len = (dc_reg >> 5) & 0x7;
					uint8_t i;

					for (i = 0; i < vic_len; i++)
					{
						index++;
						if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
						if (block_length >= (index - db_index))
						{
							if (dc_reg == ntv2_hdmi_video_format_quad30)
							{
								video_status->quad_30 = true;
							}
							else if (dc_reg == ntv2_hdmi_video_format_quad25)
							{
								video_status->quad_25 = true;
							}
							else if (dc_reg == ntv2_hdmi_video_format_quad24)
							{
								video_status->quad_24 = true;
							}
							else if (dc_reg == ntv2_hdmi_video_format_four24)
							{
								video_status->four_24 = true;
							}
						}
					}
				}
			}
		}
	}

	return true;
}

static bool get_hf_video_status(struct ntv2_displayid* ntv2_did,
								uint8_t block_num,
								struct ntv2_displayid_video* video_status)
{
	uint8_t db_index = 0;

	if (ntv2_did == NULL) return false;
	if (block_num == 0) return false;

	/* find the HF Vendor Specific Data Block */
	if (!get_index_for_hf_datablock(ntv2_did, block_num, &db_index)) return false;
	if (db_index != 0)
	{
		uint8_t index = db_index;
		uint8_t dc_reg = 0;
		uint8_t block_length = 0;

		/* parse HF Vendor-Specific Data Block! */
		if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
		block_length = dc_reg & 0x1f;

		index += 5;
		if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
		if (block_length >= (index - db_index))
		{
			video_status->max_tmds_csc = dc_reg * 5;
		}

		index++;
		if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
		if (block_length >= (index - db_index))
		{
			video_status->osd_disparity = ((dc_reg & 0x01) == 0) ? false : true;
			video_status->dual_view = ((dc_reg & 0x02) == 0) ? false : true;
			video_status->indep_view = ((dc_reg & 0x04) == 0) ? false : true;
			video_status->lte_scramble = ((dc_reg & 0x08) == 0) ? false : true;
			video_status->rr_capable = ((dc_reg & 0x40) == 0) ? false : true;
			video_status->scdc_present = ((dc_reg & 0x80) == 0) ? false : true;
		}

		index++;
		if (!read_edid_register(ntv2_did, block_num, index, &dc_reg)) return false;
		if (block_length >= (index - db_index))
		{
			video_status->dc_30bit_420 = ((dc_reg & 0x01) == 0) ? false : true;
			video_status->dc_36bit_420 = ((dc_reg & 0x02) == 0) ? false : true;
			video_status->dc_48bit_420 = ((dc_reg & 0x04) == 0) ? false : true;
		}
	}

	return true;
}

static bool get_global_audio_status(struct ntv2_displayid* ntv2_did, 
									uint8_t block_num,
									struct ntv2_displayid_audio* audio_status)
{
	uint8_t reg_val = 0;

	if (ntv2_did == NULL) return false;
	if (block_num == 0) return false;

	/* read byte #3 (the 4th byte) to get the global support bits */
	if (!read_edid_register(ntv2_did, block_num, 3, &reg_val)) return false;

	/* parse the bits for support status */
	audio_status->basic_audio = ((reg_val & 0x40) == 0) ? false : true;

	return true;
}

static bool get_audio_descriptor(struct ntv2_displayid* ntv2_did,
								 uint8_t block_num,
								 enum ntv2_cea861_audio_format format, 
								 uint8_t inst,
								 struct ntv2_displayid_audio_format* descriptor)
{
	uint8_t ds_index = 0;
	uint8_t bytes[3];

	if (ntv2_did == NULL) return false;
	if (block_num == 0) return false;

	if (!get_index_for_audio_descriptor(ntv2_did, block_num, format, inst, &ds_index)) return false;
	if (ds_index == 0)
	{
		/* set returned format to "Reserved" to flag failure */
		descriptor->audio_format = ntv2_cea861_audio_format_reserved;
		return true;
	}

	/* parse the Short Audio Descriptor into our struct */
	if (!read_edid_registers(ntv2_did, block_num, (ds_index+0), 3, bytes)) return false;

	descriptor->audio_format = (enum ntv2_cea861_audio_format)((bytes[0] >> 3) & 0x0F);	// byte 1, bits 6:3 = Audio Format Code
	descriptor->num_channels = (bytes[0] & 0x07) + 1;								// byte 1, bits 2:0 = "Max Number of channels - 1"

	descriptor->b192khz = ((bytes[1] & 0x40) == 0) ? false : true;	// byte2, bit 6
	descriptor->b176khz = ((bytes[1] & 0x20) == 0) ? false : true;	// byte2, bit 5
	descriptor->b96khz  = ((bytes[1] & 0x10) == 0) ? false : true;	// byte2, bit 4
	descriptor->b88khz  = ((bytes[1] & 0x08) == 0) ? false : true;	// byte2, bit 3
	descriptor->b48khz  = ((bytes[1] & 0x04) == 0) ? false : true;	// byte2, bit 2
	descriptor->b44khz  = ((bytes[1] & 0x02) == 0) ? false : true;	// byte2, bit 1
	descriptor->b32khz  = ((bytes[1] & 0x01) == 0) ? false : true;	// byte2, bit 0

	if (descriptor->audio_format == ntv2_cea861_audio_format_lpcm)
	{
		descriptor->lpcm_24bit = ((bytes[2] & 0x04) == 0) ? false : true;	// byte3, bit 2
		descriptor->lpcm_20bit = ((bytes[2] & 0x02) == 0) ? false : true;	// byte3, bit 1
		descriptor->lpcm_16bit = ((bytes[2] & 0x01) == 0) ? false : true;	// byte3, bit 0
	}
	else
	{
		descriptor->lpcm_24bit = false;
		descriptor->lpcm_20bit = false;
		descriptor->lpcm_16bit = false;
	}
	descriptor->byte3 = bytes[2];

	return true;
}

static bool get_index_for_audio_descriptor(struct ntv2_displayid* ntv2_did,
										   uint8_t block_num, 
										   enum ntv2_cea861_audio_format format, 
										   uint8_t inst,
										   uint8_t* index)
{
	uint8_t db_inst = 0;
	uint8_t db_index = 0;
	uint8_t db_length = 0;
	uint8_t ds_index = 0;
	uint8_t num_descriptors = 0;
	uint8_t match_count = 0;
	uint8_t byte1 = 0;
	uint8_t i;
	uint8_t j;
	enum ntv2_cea861_audio_format ds_format;

	if (ntv2_did == NULL) return false;
	if (index == NULL) return false;

	if (block_num == 0 || block_num == 255)
	{
		*index = 0;
		return false;
	}

	/* iterate on the number of Audio Data Blocks */
	for (i = 0; i < 254; i++)
	{
		if (!get_index_for_datablock(ntv2_did,
									 block_num,
									 ntv2_cea861_datablock_tag_audio,
									 ntv2_cea861_datablock_extended_tag_reserved255,
									 db_inst, 
									 &db_index, 
									 &db_length))
		{
			*index = 0;
			return false;
		}
		if (db_index == 0)
		{
			/* not found */
			*index = 0;
			return true;
		}

		/* got an Audio Data Block, now paw through the Short Audio Descriptors to try and find the one we want */
		ds_index = db_index + 1;			/* start at the next byte (past the header) */
		num_descriptors = db_length / 3;	/* three bytes per descriptor */
		match_count = 0;					/* counts the number of matches we find */

		/* iterate on the number of Short Audio Descriptors */
		for (j = 0; j < num_descriptors; j++)
		{
			if (format == ntv2_cea861_audio_format_wildcard)
			{
				/* by definition, this is a match - but is it the right instance? */
				if (match_count == inst)
				{
					*index = ds_index;
					return true;
				}
				else
				{
					match_count++;
				}
			}
			else	/* not a wildcard format - we need to parse the Short Audio Descriptor to get its format */
			{
				/* get the audio format code from the descriptor */
				if (!read_edid_register(ntv2_did, block_num, ds_index, &byte1))
				{
					*index = 0;
					return false;
				}

				/* the Audio Format Code is in bits[6:3] of the first byte (the CEA-861 spec labels this "Byte 1") */
				ds_format = (enum ntv2_cea861_audio_format)((byte1 >> 3) & 0x0F);
				if (ds_format == format)
				{
					if (match_count == inst)
					{
						*index = ds_index;
						return true;
					}
					else
					{
						match_count++;
					}
				}
			}
			ds_index += 3;
		}
		db_inst += 1;
	}

	/* not found */
	*index = 0;
	return true;
}

static bool get_index_for_hdmi_datablock(struct ntv2_displayid* ntv2_did,
										 uint8_t block_num,
										 uint8_t* index)
{
	uint8_t db_inst = 0;
	uint8_t db_index = 0;
	uint8_t db_length = 0;
	uint8_t reg_id[3];
	int i;

	if (ntv2_did == NULL) return false;
	if (index == NULL) return false;

	if ((block_num == 0) || (block_num == 255))
	{
		*index = 0;
		return false;
	}

	/* iterate through any Vendor-Specific Data Blocks until we find the first one that is "HDMI" */
	for (i = 0; i < 254; i++)
	{
		if (!get_index_for_datablock(ntv2_did,
									 block_num,
									 ntv2_cea861_datablock_tag_vendor,
									 ntv2_cea861_datablock_extended_tag_reserved255,
									 db_inst,
									 &db_index,
									 &db_length))
		{
			*index = 0;
			return false;
		}
		if (db_index == 0)
		{
			/* not found */
			*index = 0;
			return true;
		}

		/* found a Vendor-Specific Data Block, but is it an "HDMI" flavor? */
		/* (Note: HDMI-LLC Vendor-Specific Data Blocks are defined in the HDMI Spec (i.e. NOT CEA-861). */
		/*        In the HDMI v1.4 spec, the description is on pp 146-150). */
		if (!read_edid_registers(ntv2_did, block_num, (db_index + 1), 3, reg_id))
		{
			*index = 0;
			return false;
		}

		/* do we have a match? (note: LS byte is first) */
		if ((reg_id[0] == cea861_vendorspecific_ieeecode_hdmi_ls) &&
			(reg_id[1] == cea861_vendorspecific_ieeecode_hdmi_mid) &&
			(reg_id[2] == cea861_vendorspecific_ieeecode_hdmi_ms))
		{
			*index = db_index;
			return true;
		}

		db_inst++;
	}

	*index = 0;
	return true;
}

static bool get_index_for_hf_datablock(struct ntv2_displayid* ntv2_did,
									   uint8_t block_num,
									   uint8_t* index)
{
	uint8_t inst = 0;
	uint8_t db_index = 0;
	uint8_t db_length = 0;
	uint8_t reg_id[3];
	int i;

	if (ntv2_did == NULL) return false;
	if (index == NULL) return false;

	if (block_num == 0 || block_num == 255)
	{
		*index = 0;
		return false;
	}

	/* iterate through any Vendor-Specific Data Blocks until we find the first one that is "HDMI" */
	for (i = 0; i < 254; i++)
	{
		if (!get_index_for_datablock(ntv2_did,
									 block_num,
									 ntv2_cea861_datablock_tag_vendor,
									 ntv2_cea861_datablock_extended_tag_reserved255,
									 inst, 
									 &db_index, 
									 &db_length))
		{
			*index = 0;
			return false;
		}
		if (db_index == 0)
		{
			/* not found */
			*index = 0;
			return true;
		}

		/* found a Vendor-Specific Data Block, but is it an "HDMI" flavor? */
		/* (Note: HDMI-LLC Vendor-Specific Data Blocks are defined in the HDMI Spec (i.e. NOT CEA-861). */
		/*        In the HDMI v1.4 spec, the description is on pp 146-150). */
		if (!read_edid_registers(ntv2_did, block_num, (db_index + 1), 3, reg_id))
		{
			*index = 0;
			return false;
		}

		if ((reg_id[0] == cea861_vendorspecific_ieeecode_hf_ls) &&
			(reg_id[1] == cea861_vendorspecific_ieeecode_hf_mid) &&
			(reg_id[2] == cea861_vendorspecific_ieeecode_hf_ms))
		{
			*index = db_index;
			return true;
		}

		inst++;
	}

	/* not found */
	*index = 0;
	return true;
}

static bool get_index_for_datablock(struct ntv2_displayid* ntv2_did,
									uint8_t block_num,
									enum ntv2_cea861_datablock_tag datablock_tag,
									enum ntv2_cea861_datablock_extended_tag datablock_ext_tag,
									uint8_t db_inst,
									uint8_t* db_index,
									uint8_t* db_length)
{
	/* the "Data Block Collection" starts at byte 4 of the extension */
	uint8_t index = 4;
	uint8_t dtd_start = 0;
	uint8_t db_end = 0;
	uint8_t inst_count = 0;
    bool got_match = false;

	/* (note: see CEA-861-E pg 62 for schematic and description of Data Block layout within a CEA-861 extension) */
	if (ntv2_did == NULL) return false;
	if (db_index == NULL) return false;
	if (db_length == NULL) return false;

	/* the "Data Block Collection" ends one byte before the detailed timing descriptors start, which is defined in byte 2 */
	/* the offset to the start of the detailed timing descriptors */
	if (!read_edid_register(ntv2_did, block_num, 2, &dtd_start))
	{
		*db_index = 0;
		*db_length = 0;
		return false;
	}

	/* if there are no detailed timing descriptors, dtdStart will be zero which means */
	/* the entire extension could be filled with Data Blocks */
	if (dtd_start == 0)
		db_end = 126;
	else
		db_end = dtd_start;


	while (index < db_end)
	{
		/* index is the index of the first byte of a data block. Get the Data Block's tag and length. */
		enum ntv2_cea861_datablock_tag db_tag = ntv2_cea861_datablock_tag_reserved0;
		enum ntv2_cea861_datablock_extended_tag db_ext_tag = ntv2_cea861_datablock_extended_tag_reserved255;
		uint8_t db_num_bytes = 0;

		if(!get_datablock_header(ntv2_did, block_num, index, &db_tag, &db_ext_tag, &db_num_bytes))
		{
			*db_index = 0;
			*db_length = 0;
			return false;
		}

		/* special case: if the Tag is zero and the length is zero, assume we've run past the Data Blocks and into the padding */
		if ((db_tag == ntv2_cea861_datablock_tag_reserved0) && (db_num_bytes == 0))
		{
			*db_index = 0;
			*db_length = 0;
			return true;
		}

		/* is this the type of data block we're looking for? if not, jump over it and keep searching */
		if (datablock_tag == ntv2_cea861_datablock_tag_wildcard)
		{
			got_match = true;
		}
		else if (db_tag == datablock_tag)
		{
			/* one more hoop to jump through: see if this is an "extended" tag - if so we also need to compare those too */
			if (db_tag != ntv2_cea861_datablock_tag_extended)
			{
				got_match = true;
			}
			else
			{
				if (db_ext_tag == datablock_ext_tag)
					got_match = true;
			}
		}

		/* if we have a match, is it the instance number we're looking for? */
		if (got_match)
		{
			if (inst_count == db_inst)
			{
				*db_index  = index;
				*db_length = db_num_bytes;
				return true;
			}
			else
			{
				inst_count++;
				got_match = false;
			}
		}

		/* no match - skip over this Data Block and keep looking */
		index += (db_num_bytes + 1);
	}

	/* not found */
	*db_index = 0;
	*db_length = 0;
	return true;
}

static bool get_datablock_header(struct ntv2_displayid* ntv2_did,
								 uint8_t block_num,
								 uint8_t db_index,
								 enum ntv2_cea861_datablock_tag* db_tag,
								 enum ntv2_cea861_datablock_extended_tag* db_ext_tag,
								 uint8_t* num_bytes)
{
	uint8_t reg_val = 0;

	if (ntv2_did == NULL) return false;
	if (db_tag == NULL) return false;
	if (db_ext_tag == NULL) return false;
	if (num_bytes == NULL) return false;

	/* dbIndex points to the first byte in a Data Block, which contains the tag (bits 7:5) and payload length (bits 4:0) */
	if (!read_edid_register(ntv2_did, block_num, db_index, &reg_val))
	{
		*db_tag = ntv2_cea861_datablock_tag_reserved0;
		*db_ext_tag = ntv2_cea861_datablock_extended_tag_reserved255;
		*num_bytes = 128;
		return false;
	}

	*db_tag = (enum ntv2_cea861_datablock_tag)((reg_val >> 5) & 0x07);	/* bits 7:5 */
	*num_bytes = reg_val & 0x1F;										/* bits 4:0 */

	/* if the primary tag says "Extended", we also need to get the Extended Tag from the second byte */
	if ((*db_tag == ntv2_cea861_datablock_tag_extended) && (*num_bytes > 0))
	{
		if(!read_edid_register(ntv2_did, block_num, (db_index + 1), &reg_val))
		{
			*db_tag = ntv2_cea861_datablock_tag_reserved0;
			*db_ext_tag = ntv2_cea861_datablock_extended_tag_reserved255;
			*num_bytes = 128;
			return false;
		}

		*db_ext_tag = (enum ntv2_cea861_datablock_extended_tag)reg_val;
	}
	else
	{
		*db_ext_tag = ntv2_cea861_datablock_extended_tag_reserved255;
	}

	return true;
}

static bool read_edid_registers(struct ntv2_displayid* ntv2_did, 
								uint8_t block_num, 
								uint8_t reg_addr, 
								uint32_t num_regs, 
								uint8_t* data)
{
    uint32_t last_reg;
	uint32_t i;
	uint8_t reg_val;

	if (ntv2_did == NULL) return false;
	if (data == NULL) return false;
	if (num_regs == 0) return false;

	if (reg_addr > 127)	reg_addr = 127;

	last_reg = reg_addr + num_regs - 1;

	if (last_reg > 127)
		last_reg = 127;

    for (i = reg_addr; i <= last_reg; i++)
	{
		if (!read_edid_register(ntv2_did, block_num, (uint8_t)i, &reg_val)) break;
		data[i - reg_addr] = reg_val;
	}

	return true;
}

static bool read_edid_register(struct ntv2_displayid* ntv2_did, 
							   uint8_t block_num, 
							   uint8_t reg_num, 
							   uint8_t* reg_val)
{
	if (ntv2_did == NULL) return false;
	if (block_num > 1) return false;
	if (reg_num >= 128) return false;
	if (ntv2_did->callback == NULL) return false;

	return (*ntv2_did->callback)(ntv2_did->context, block_num, reg_num, reg_val);
}

