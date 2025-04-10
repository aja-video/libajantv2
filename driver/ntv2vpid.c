/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//==========================================================================
//
//  ntv2vpid.c
//
//==========================================================================

#include "ntv2vpid.h"
#include "ntv2system.h"

extern bool SetVPIDFromSpec (ULWord * const			pOutVPID, const VPIDSpec * const	pInVPIDSpec);

//static const int FOUR_K_DC_TABLE_SIZE = 20;
#define FOUR_K_DC_TABLE_SIZE 20
#define NTV2_ENDIAN_SWAP32(_data_) (((_data_<<24)&0xff000000)|((_data_<<8)&0x00ff0000)|((_data_>>8)&0x0000ff00)|((_data_>>24)&0x000000ff))

static NTV2VideoFormat P2PSF [FOUR_K_DC_TABLE_SIZE] =
{
	NTV2_FORMAT_1080psf_2398,
	NTV2_FORMAT_1080psf_2400,
	NTV2_FORMAT_1080psf_2500_2,
	NTV2_FORMAT_1080psf_2997_2,
	NTV2_FORMAT_1080psf_3000_2,
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_4795
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_4800
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_5000
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_5994
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_6000
	NTV2_FORMAT_1080psf_2K_2398,
	NTV2_FORMAT_1080psf_2K_2400,
	NTV2_FORMAT_1080psf_2K_2500,
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_2K_2997
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_2K_3000
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_2K_4795
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_2K_4800
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_2K_5000
	NTV2_FORMAT_UNKNOWN,			//	No NTV2_FORMAT_1080psf_2K_5994
	NTV2_FORMAT_UNKNOWN				//	No NTV2_FORMAT_1080psf_2K_6000
};

static VirtualRegisterNum gShadowRegs [] = {	kVRegVideoFormatCh1, kVRegVideoFormatCh2, kVRegVideoFormatCh3, kVRegVideoFormatCh4,
												kVRegVideoFormatCh5, kVRegVideoFormatCh6, kVRegVideoFormatCh7, kVRegVideoFormatCh8};

static NTV2RegisterNumber gVPIDAInRegs[] = {	kRegSDIIn1VPIDA, kRegSDIIn2VPIDA, kRegSDIIn3VPIDA, kRegSDIIn4VPIDA,
												kRegSDIIn5VPIDA, kRegSDIIn6VPIDA, kRegSDIIn7VPIDA, kRegSDIIn8VPIDA };

static NTV2RegisterNumber gVPIDBInRegs[] = {	kRegSDIIn1VPIDB, kRegSDIIn2VPIDB, kRegSDIIn3VPIDB, kRegSDIIn4VPIDB,
												kRegSDIIn5VPIDB, kRegSDIIn6VPIDB, kRegSDIIn7VPIDB, kRegSDIIn8VPIDB };

static NTV2RegisterNumber gVPIDStatusRegs[] = { kRegSDIInput3GStatus, kRegSDIInput3GStatus, kRegSDIInput3GStatus2, kRegSDIInput3GStatus2,
												kRegSDI5678Input3GStatus, kRegSDI5678Input3GStatus, kRegSDI5678Input3GStatus, kRegSDI5678Input3GStatus };

static RegisterMask gVPIDAValidMask[] = {	kRegMaskSDIInVPIDLinkAValid, kRegMaskSDIInVPIDLinkAValid, kRegMaskSDIInVPIDLinkAValid, kRegMaskSDIInVPIDLinkAValid,
											kRegMaskSDIInVPIDLinkAValid, kRegMaskSDIInVPIDLinkAValid, kRegMaskSDIInVPIDLinkAValid, kRegMaskSDIInVPIDLinkAValid };

static RegisterMask gVPIDBValidMask[] = {	kRegMaskSDIInVPIDLinkBValid, kRegMaskSDIIn2VPIDLinkBValid, kRegMaskSDIIn3VPIDLinkBValid, kRegMaskSDIIn4VPIDLinkBValid,
											kRegMaskSDIIn5VPIDLinkBValid, kRegMaskSDIIn6VPIDLinkBValid, kRegMaskSDIIn7VPIDLinkBValid, kRegMaskSDIIn8VPIDLinkBValid };

static NTV2RegisterNumber gVPIDAOutRegs[] = {	kRegSDIOut1VPIDA, kRegSDIOut2VPIDA, kRegSDIOut3VPIDA, kRegSDIOut4VPIDA,
												kRegSDIOut5VPIDA, kRegSDIOut6VPIDA, kRegSDIOut7VPIDA, kRegSDIOut8VPIDA };

static NTV2RegisterNumber gVPIDBOutRegs[] = {	kRegSDIOut1VPIDB, kRegSDIOut2VPIDB, kRegSDIOut3VPIDB, kRegSDIOut4VPIDB,
												kRegSDIOut5VPIDB, kRegSDIOut6VPIDB, kRegSDIOut7VPIDB, kRegSDIOut8VPIDB };

static NTV2RegisterNumber gSDIOutControlRegs[] = {	kRegSDIOut1Control, kRegSDIOut2Control, kRegSDIOut3Control, kRegSDIOut4Control,
													kRegSDIOut5Control, kRegSDIOut6Control, kRegSDIOut7Control, kRegSDIOut8Control };

static const ULWord	gChannelToSDIOutVPIDTransferCharacteristics[] = {	kVRegNTV2VPIDTransferCharacteristics1, kVRegNTV2VPIDTransferCharacteristics2, kVRegNTV2VPIDTransferCharacteristics3, kVRegNTV2VPIDTransferCharacteristics4,
																		kVRegNTV2VPIDTransferCharacteristics5, kVRegNTV2VPIDTransferCharacteristics6, kVRegNTV2VPIDTransferCharacteristics7, kVRegNTV2VPIDTransferCharacteristics8, 0 };

static const ULWord	gChannelToSDIOutVPIDColorimetry[] =	{	kVRegNTV2VPIDColorimetry1, kVRegNTV2VPIDColorimetry2, kVRegNTV2VPIDColorimetry3, kVRegNTV2VPIDColorimetry4,
															kVRegNTV2VPIDColorimetry5, kVRegNTV2VPIDColorimetry6, kVRegNTV2VPIDColorimetry7, kVRegNTV2VPIDColorimetry8, 0 };

static const ULWord	gChannelToSDIOutVPIDLuminance[] = {	kVRegNTV2VPIDLuminance1, kVRegNTV2VPIDLuminance2, kVRegNTV2VPIDLuminance3, kVRegNTV2VPIDLuminance4,
														kVRegNTV2VPIDLuminance5, kVRegNTV2VPIDLuminance6, kVRegNTV2VPIDLuminance7, kVRegNTV2VPIDLuminance8, 0 };

static const ULWord	gChannelToSDIOutVPIDRGBRange[] = {	kVRegNTV2VPIDRGBRange1, kVRegNTV2VPIDRGBRange2, kVRegNTV2VPIDRGBRange3, kVRegNTV2VPIDRGBRange4,
														kVRegNTV2VPIDRGBRange5, kVRegNTV2VPIDRGBRange6, kVRegNTV2VPIDRGBRange7, kVRegNTV2VPIDRGBRange8, 0 };

static const ULWord	gChannelToLPIPOutConfig[] = { kRegLPIPOut1Config, kRegLPIPOut2Config, kRegLPIPOut3Config, kRegLPIPOut4Config, 0 };

/*********************************************/
/* Prototypes for private utility functions. */
/*********************************************/
bool SetTransferCharacteristics(uint32_t* inOutVPIDValue, NTV2VPIDXferChars iXferChars);
VPIDStandard GetVPIDStandard(uint32_t inOutVPIDValue);
bool SetColorimetry(uint32_t* inOutVPIDValue, NTV2VPIDColorimetry inColorimetry);
bool SetLuminance(uint32_t* inOutVPIDValue, NTV2VPIDLuminance inLuminance);
VPIDBitDepth GetBitDepth(uint32_t inOutVPIDValue);
bool SetBitDepth(uint32_t* inOutVPIDValue, VPIDBitDepth inBitDepth);
bool SetRGBRange(uint32_t* inOutVPIDValue, NTV2VPIDRGBRange inRGBRange);

VPIDChannel GetChannelFrom425XPT(ULWord index)
{
	switch (index)
	{
	case XPT_FB_YUV_1:
	case XPT_FB_YUV_3:
	case XPT_FB_YUV_5:
	case XPT_FB_YUV_7:
	case XPT_FB_RGB_1:
	case XPT_FB_RGB_3:
	case XPT_FB_RGB_5:
	case XPT_FB_RGB_7:
	case XPT_HDMI_IN:
		return VPIDChannel_1;

	case XPT_FB_425_YUV_1:
	case XPT_FB_425_YUV_3:
	case XPT_FB_425_YUV_5:
	case XPT_FB_425_YUV_7:
	case XPT_FB_425_RGB_1:
	case XPT_FB_425_RGB_3:
	case XPT_FB_425_RGB_5:
	case XPT_FB_425_RGB_7:
	case XPT_HDMI_IN_Q2:
		return VPIDChannel_2;

	case XPT_FB_YUV_2:
	case XPT_FB_YUV_4:
	case XPT_FB_YUV_6:
	case XPT_FB_YUV_8:
	case XPT_FB_RGB_2:
	case XPT_FB_RGB_4:
	case XPT_FB_RGB_6:
	case XPT_FB_RGB_8:
	case XPT_HDMI_IN_Q3:
		return VPIDChannel_3;

	case XPT_FB_425_YUV_2:
	case XPT_FB_425_YUV_4:
	case XPT_FB_425_YUV_6:
	case XPT_FB_425_YUV_8:
	case XPT_FB_425_RGB_2:
	case XPT_FB_425_RGB_4:
	case XPT_FB_425_RGB_6:
	case XPT_FB_425_RGB_8:
	case XPT_HDMI_IN_Q4:
		return VPIDChannel_4;
	default:
		break;
	}
	return VPIDChannel_1;
}

bool ReadSDIInVPID(Ntv2SystemContext* context, NTV2Channel channel, ULWord* valueA, ULWord* valueB)
{
	ULWord			regValue = 0;
	NTV2DeviceID	deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);
	if (valueA != NULL)
	{
		regValue = ntv2ReadRegister(context, gVPIDStatusRegs[channel]);
		if (regValue & gVPIDAValidMask[channel])
		{
			// VPID in register is byte swapped
			regValue = ntv2ReadRegister(context, gVPIDAInRegs[channel]);
			if (deviceID != DEVICE_ID_KONALHI)
			{
#if defined (AJAVirtual)
				*valueA = NTV2_ENDIAN_SWAP32(regValue);
#elif defined (MSWindows)
				*valueA = RtlUlongByteSwap(regValue);
#elif defined (AJAMac)
	#ifdef AJAMacDext
				*valueA = OSSwapBigToHostInt32(regValue);
	#else
				*valueA = OSSwapInt32(regValue);
	#endif
#elif defined (AJALinux)
				*valueA = be32_to_cpu(regValue);
#endif
			}
			else
			{
				*valueA = regValue;
			}
		}
	}

	if (valueB != NULL)
	{
		regValue = ntv2ReadRegister(context, gVPIDStatusRegs[channel]);
		if (regValue & gVPIDBValidMask[channel])
		{
			// VPID in register is byte swapped
			regValue = ntv2ReadRegister(context, gVPIDBInRegs[channel]);
			if (deviceID != DEVICE_ID_KONALHI)
			{
#if defined (AJAVirtual)
				*valueB = NTV2_ENDIAN_SWAP32(regValue);
#elif defined (MSWindows)
				*valueB = RtlUlongByteSwap(ntv2ReadRegister(context, gVPIDBInRegs[channel]));
#elif defined (AJAMac)
	#ifdef AJAMacDext
				*valueB = OSSwapBigToHostInt32(regValue);
	#else
				*valueB = OSSwapInt32(regValue);
	#endif
#elif defined (AJALinux)
				*valueB = be32_to_cpu(regValue);
#endif
			}
			else
			{
				*valueB = regValue;
			}
		}
	}

	return true;
}	//	ReadSDIInVPID

bool SetSDIOutVPID(Ntv2SystemContext* context, NTV2Channel channel, ULWord valueA, ULWord valueB)
{
	if (valueA != 0)
	{
		ntv2WriteRegister(context, gVPIDAOutRegs[channel], valueA);
		ntv2WriteRegister(context, gVPIDBOutRegs[channel], valueB);
		ntv2WriteRegisterMS(context, gSDIOutControlRegs[channel], 1, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite );
		ntv2WriteRegisterMS(context, gSDIOutControlRegs[channel], 1, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable);

		return true;
	}

	ntv2WriteRegisterMS(context, gSDIOutControlRegs[channel], 0, kK2RegMaskVPIDInsertionOverwrite, kK2RegShiftVPIDInsertionOverwrite);
	ntv2WriteRegisterMS(context, gSDIOutControlRegs[channel], 0, kK2RegMaskVPIDInsertionEnable, kK2RegShiftVPIDInsertionEnable);
	ntv2WriteRegister(context, gVPIDAOutRegs[channel], 0);
	ntv2WriteRegister(context, gVPIDBOutRegs[channel], 0);

	return true;
}	//	SetSDIOutVPID

bool Set2110Key(Ntv2SystemContext* context, NTV2Channel channel, bool setKey)
{
	if (channel > NTV2_CHANNEL4)
		return false;
	return ntv2WriteRegisterMS(context, gChannelToLPIPOutConfig[channel], setKey ? 1 : 0, kRegMaskIPIsKey, kRegShiftIPIsKey);
}

bool AdjustFor4KDC(Ntv2SystemContext* context, VPIDControl * pControl)
{
	
	NTV2FrameRate	frameRate	= GetNTV2FrameRateFromVideoFormat (pControl->vpidSpec.videoFormat);
	ULWord			is2Kx1080 = IsVideoFormat2Kx1080(pControl->vpidSpec.videoFormat);
	ULWord			index		= 0;

	switch (frameRate)
	{
	case NTV2_FRAMERATE_6000:
		index = 9;
		break;
	case NTV2_FRAMERATE_5994:
		index = 8;
		break;
	case NTV2_FRAMERATE_5000:
		index = 7;
		break;
	case NTV2_FRAMERATE_4800:
		index = 6;
		break;
	case NTV2_FRAMERATE_4795:
		index = 5;
		break;
	case NTV2_FRAMERATE_3000:
		index = 4;
		break;
	case NTV2_FRAMERATE_2997:
		index = 3;
		break;
	case NTV2_FRAMERATE_2500:
		index = 2;
		break;
	case NTV2_FRAMERATE_2400:
		index = 1;
		break;
	case NTV2_FRAMERATE_2398:
		break;
	default:
		pControl->vpidSpec.videoFormat = NTV2_FORMAT_UNKNOWN;	//	Rate is outside what we can handle
	}

	if (is2Kx1080)
		index += FOUR_K_DC_TABLE_SIZE / 2;		//	2K formats are in the second half of the tabel

	if (GetEnable4KDCPSFOutMode (context))
	{
		//	Conversion is P -> PSF
		pControl->vpidSpec.videoFormat = P2PSF [index];
	}

	return true;
}

/////////////////
// HDR Overrides

bool SetTransferCharacteristics(uint32_t* inOutVPIDValue, NTV2VPIDXferChars iXferChars)
{
	*inOutVPIDValue = (*inOutVPIDValue & ~kRegMaskVPIDXferChars) |
		(((ULWord)iXferChars << kRegShiftVPIDXferChars) & kRegMaskVPIDXferChars);
	return true;
}

VPIDStandard GetVPIDStandard(uint32_t inOutVPIDValue)
{
	return (VPIDStandard)((inOutVPIDValue & kRegMaskVPIDStandard) >> kRegShiftVPIDStandard);
}

bool SetColorimetry(uint32_t* inOutVPIDValue, NTV2VPIDColorimetry inColorimetry)
{
	VPIDStandard standard = GetVPIDStandard(*inOutVPIDValue);
	if (standard == VPIDStandard_1080 ||
		standard == VPIDStandard_1080_DualLink ||
		standard == VPIDStandard_1080_DualLink_3Gb ||
		standard == VPIDStandard_2160_QuadDualLink_3Gb ||
		standard == VPIDStandard_2160_DualLink)
	{
		ULWord lowBit = 0;
		ULWord highBit = 0;
		highBit = (inColorimetry & 0x2) >> 1;
		lowBit = inColorimetry & 0x1;
		*inOutVPIDValue = (*inOutVPIDValue & ~kRegMaskVPIDColorimetryAltHigh) |
			((highBit << kRegShiftVPIDColorimetryAltHigh) & kRegMaskVPIDColorimetryAltHigh);
		*inOutVPIDValue = (*inOutVPIDValue & ~kRegMaskVPIDColorimetryAltLow) |
			((lowBit << kRegShiftVPIDColorimetryAltLow) & kRegMaskVPIDColorimetryAltLow);
	}
	else
	{
		*inOutVPIDValue = (*inOutVPIDValue & ~kRegMaskVPIDColorimetry) |
			(((ULWord)inColorimetry << kRegShiftVPIDColorimetry) & kRegMaskVPIDColorimetry);
	}
	return true;
}

bool SetLuminance(uint32_t* inOutVPIDValue, NTV2VPIDLuminance inLuminance)
{
	*inOutVPIDValue = (*inOutVPIDValue & ~kRegmaskVPIDLuminance) |
		(((ULWord)inLuminance << kRegShiftVPIDLuminance) & kRegmaskVPIDLuminance);
	return true;
}

VPIDBitDepth GetBitDepth(uint32_t inOutVPIDValue)
{
	return (VPIDBitDepth)((inOutVPIDValue & kRegMaskVPIDBitDepth) >> kRegShiftVPIDBitDepth);
}

bool SetBitDepth(uint32_t* inOutVPIDValue, VPIDBitDepth inBitDepth)
{
	*inOutVPIDValue = (*inOutVPIDValue & ~kRegMaskVPIDBitDepth) |
		(((ULWord)inBitDepth << kRegShiftVPIDBitDepth) & kRegMaskVPIDBitDepth);
	return true;
}

bool SetRGBRange(uint32_t* inOutVPIDValue, NTV2VPIDRGBRange inRGBRange)
{
	switch (GetBitDepth(*inOutVPIDValue))
	{
	case VPIDBitDepth_10_Full:
	case VPIDBitDepth_10:
		if (inRGBRange == NTV2_VPID_Range_Narrow)
			SetBitDepth(inOutVPIDValue, VPIDBitDepth_10);
		else
			SetBitDepth(inOutVPIDValue, VPIDBitDepth_10_Full);
		break;
	case VPIDBitDepth_12_Full:
	case VPIDBitDepth_12:
		if (inRGBRange == NTV2_VPID_Range_Narrow)
			SetBitDepth(inOutVPIDValue, VPIDBitDepth_12);
		else
			SetBitDepth(inOutVPIDValue, VPIDBitDepth_12_Full);
	}

	return true;
}

///////////

bool FindVPID(Ntv2SystemContext* context, NTV2OutputXptID startingXpt, VPIDControl * pControl)
{
	NTV2FrameBufferFormat	frameBufferFormat = NTV2_FBF_NUMFRAMEBUFFERFORMATS;
	NTV2OutputXptID			currentXpt = startingXpt;
	const int				kMaxLoopCount = 10;		// no endless loops
	int loopCount = 0;
	NTV2VideoFormat newVideoFormat = NTV2_FORMAT_UNKNOWN;
	
	for (loopCount = 0; currentXpt != NTV2_XptBlack && loopCount < kMaxLoopCount; loopCount++)
	{
		NTV2XptLookupEntry source = GetCrosspointIDInput(currentXpt);
		//	If non-existent source, there's a routing error, so no VPID
		if (!source.registerNumber)
			break;

		//	If an SDI input, get the VPID from the input widget
		if ((source.registerNumber >= XPT_SDI_IN_1) && (source.registerNumber <= XPT_SDI_IN_8_DS2))
		{
			if ((source.registerNumber >= XPT_SDI_IN_1) && (source.registerNumber <= XPT_SDI_IN_8))
			{
				NTV2Channel inputChannel = (NTV2Channel)(source.registerNumber - XPT_SDI_IN_1);
				ReadSDIInVPID(context, inputChannel, &pControl->value, NULL);
				pControl->isComplete = true;
				break;
			}

			if ((source.registerNumber >= XPT_SDI_IN_1_DS2) && (source.registerNumber <= XPT_SDI_IN_8_DS2))
			{
				ReadSDIInVPID(context, (NTV2Channel)(source.registerNumber - XPT_SDI_IN_1_DS2), NULL, &pControl->value);
				pControl->isComplete = true;
				break;
			}
		}

		//	If a frame store
		if ((source.registerNumber >= XPT_FB_YUV_1) && (source.registerNumber <= XPT_FB_425_RGB_8))
		{
			NTV2VideoFormat shadowFormat = NTV2_FORMAT_UNKNOWN;
			//	Map FB range onto 0 to 7
			pControl->frameStoreIndex = source.registerNumber;
			pControl->frameStoreIndex -= (pControl->frameStoreIndex >= XPT_FB_425_RGB_1) ? XPT_FB_425_RGB_1 : 0;
			pControl->frameStoreIndex -= (pControl->frameStoreIndex >= XPT_FB_425_YUV_1) ? XPT_FB_425_YUV_1 : 0;
			pControl->frameStoreIndex -= (pControl->frameStoreIndex >= XPT_FB_RGB_1) ? XPT_FB_RGB_1 : 0;
			pControl->frameStoreIndex -= (pControl->frameStoreIndex >= XPT_FB_YUV_1) ? XPT_FB_YUV_1 : 0;

			if (IsMultiFormatActive(context))
			{
				pControl->vpidSpec.videoFormat = GetBoardVideoFormat(context, (NTV2Channel)pControl->frameStoreIndex);
				shadowFormat = (NTV2VideoFormat)ntv2ReadVirtualRegister(context, gShadowRegs[pControl->frameStoreIndex]);
			}

			//Grab the HDR settings
			pControl->vpidSpec.transferCharacteristics = (NTV2VPIDTransferCharacteristics)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDTransferCharacteristics[pControl->frameStoreIndex]);
			pControl->vpidSpec.colorimetry = (NTV2VPIDColorimetry)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDColorimetry[pControl->frameStoreIndex]);
			pControl->vpidSpec.luminance = (NTV2VPIDLuminance)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDLuminance[pControl->frameStoreIndex]);
			pControl->vpidSpec.rgbRange = (NTV2VPIDRGBRange)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDRGBRange[pControl->frameStoreIndex]);

			//	Allows the formst to be psf, even though the hardware is i
			if (!NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE(pControl->vpidSpec.videoFormat) && NTV2_IS_PSF_VIDEO_FORMAT(shadowFormat))
			{
				pControl->vpidSpec.videoFormat = shadowFormat;
			}

			frameBufferFormat = GetFrameBufferFormat(context, (NTV2Channel)pControl->frameStoreIndex);
			if (pControl->flags & CSCInPath)
			{
				//	Output of CSC is always 10 bit YCbCr
				pControl->vpidSpec.pixelFormat = NTV2_FBF_10BIT_YCBCR;
			}
			else
			{
				pControl->vpidSpec.pixelFormat = frameBufferFormat;
			}

			//	Check for 4K formats
			if (GetQuadFrameEnable(context, (NTV2Channel)pControl->frameStoreIndex))
			{
				pControl->vpidSpec.videoFormat = GetQuadSizedVideoFormat(pControl->vpidSpec.videoFormat);
				shadowFormat = GetQuadSizedVideoFormat(shadowFormat);
				if(Get12GTSIFrameEnable(context, (NTV2Channel)pControl->frameStoreIndex))
				{
					pControl->vpidSpec.videoFormat = Get12GVideoFormat(pControl->vpidSpec.videoFormat);
					shadowFormat = Get12GVideoFormat(shadowFormat);
				}
				if (GetQuadQuadFrameEnable(context, (NTV2Channel)pControl->frameStoreIndex))
				{
					if (!GetQuadQuadSquaresEnable(context, (NTV2Channel)pControl->frameStoreIndex))
					{
						pControl->vpidSpec.videoFormat = GetQuadQuadSizedVideoFormat(pControl->vpidSpec.videoFormat);
						shadowFormat = GetQuadQuadSizedVideoFormat(shadowFormat);
					}
				}
				//	Check for TSI
				if (Get425FrameEnable(context, (NTV2Channel)pControl->frameStoreIndex))
				{
					pControl->vpidSpec.isTwoSampleInterleave = true;
					pControl->vpidSpec.vpidChannel = GetChannelFrom425XPT(source.registerNumber);

					if(pControl->vpidSpec.isMultiLink)
					{
						if(pControl->isML1)
							pControl->vpidSpec.vpidChannel = VPIDChannel_1;
						else if(pControl->isML2)
							pControl->vpidSpec.vpidChannel = VPIDChannel_2;
						else if(pControl->isML3)
							pControl->vpidSpec.vpidChannel = VPIDChannel_3;
						else if(pControl->isML4)
							pControl->vpidSpec.vpidChannel = VPIDChannel_4;
						else
							pControl->vpidSpec.vpidChannel = pControl->vpidSpec.vpidChannel;
					}

					if(pControl->vpidSpec.isDualLink)
					{
						switch(pControl->vpidSpec.vpidChannel)
						{
						case VPIDChannel_1:
							pControl->vpidSpec.vpidChannel = pControl->isDS2 ? VPIDChannel_2 : VPIDChannel_1;
							break;
						case VPIDChannel_2:
							pControl->vpidSpec.vpidChannel = pControl->isDS2 ? VPIDChannel_4 : VPIDChannel_3;
							break;
						case VPIDChannel_3:
							pControl->vpidSpec.vpidChannel = pControl->isDS2 ? VPIDChannel_6 : VPIDChannel_5;
							break;
						case VPIDChannel_4:
							pControl->vpidSpec.vpidChannel = pControl->isDS2 ? VPIDChannel_8 : VPIDChannel_7;
							break;
						default:
							break;
						}
					}
				}
				else if (Get4kSquaresEnable(context, (NTV2Channel)pControl->frameStoreIndex))
				{
					NTV2FrameRate	frameRate = GetNTV2FrameRateFromVideoFormat(pControl->vpidSpec.videoFormat);

					if ((pControl->vpidSpec.isOutputLevelB && (NTV2_IS_HIGH_NTV2FrameRate(frameRate) || NTV2_IS_FBF_RGB(pControl->vpidSpec.pixelFormat))) || pControl->vpidSpec.isDualLink)
					{
						//	Treat the streams as 372 dual link
						pControl->vpidSpec.vpidChannel = pControl->isDS2 ? VPIDChannel_2 : VPIDChannel_1;
						pControl->vpidSpec.isDualLink = true;
						pControl->vpidSpec.useChannel = true;
					}
					else
					{
						pControl->vpidSpec.vpidChannel = VPIDChannel_1;
					}
				}
			}
			else
			{
				// check 372
				NTV2FrameRate	frameRate = GetNTV2FrameRateFromVideoFormat(pControl->vpidSpec.videoFormat);
				if ((GetSmpte372(context, (NTV2Channel)((pControl->frameStoreIndex & 0x1) ? pControl->frameStoreIndex - 1 : pControl->frameStoreIndex))))
				{
					pControl->vpidSpec.vpidChannel = (pControl->frameStoreIndex & 0x1) ? VPIDChannel_2 : VPIDChannel_1;
					pControl->vpidSpec.isDualLink = true;
					pControl->vpidSpec.useChannel = true;
				}
				else if ((pControl->vpidSpec.isOutputLevelB && (NTV2_IS_HIGH_NTV2FrameRate(frameRate) || NTV2_IS_FBF_RGB(pControl->vpidSpec.pixelFormat))) || pControl->vpidSpec.isDualLink)
				{
					pControl->vpidSpec.vpidChannel = pControl->isDS2 ? VPIDChannel_2 : VPIDChannel_1;
					pControl->vpidSpec.isDualLink = true;
					pControl->vpidSpec.useChannel = true;
				}
				else
				{
					pControl->vpidSpec.vpidChannel = VPIDChannel_1;
				}
			}

			pControl->isComplete = true;
			break;
		}

		//	If HDMI, get the parameters from the HDMI widget
		if (source.registerNumber >= XPT_HDMI_IN && source.registerNumber <= XPT_HDMI_IN_Q4)
		{
			pControl->vpidSpec.videoFormat = GetHDMIInputVideoFormat(context);
			if(NTV2_IS_QUAD_FRAME_FORMAT(pControl->vpidSpec.videoFormat))
			{
				pControl->vpidSpec.isTwoSampleInterleave = true;
				pControl->vpidSpec.vpidChannel = GetChannelFrom425XPT(source.registerNumber);
			}
			//Grab the HDR settings
			pControl->vpidSpec.transferCharacteristics = (NTV2VPIDTransferCharacteristics)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDTransferCharacteristics[NTV2CROSSPOINT_CHANNEL1]);
			pControl->vpidSpec.colorimetry = (NTV2VPIDColorimetry)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDColorimetry[NTV2CROSSPOINT_CHANNEL1]);
			pControl->vpidSpec.luminance = (NTV2VPIDLuminance)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDLuminance[NTV2CROSSPOINT_CHANNEL1]);
			pControl->vpidSpec.rgbRange = (NTV2VPIDRGBRange)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDRGBRange[NTV2CROSSPOINT_CHANNEL1]);
			pControl->isComplete = true;
			break;
		}

		//	If an analog source
		if (source.registerNumber == XPT_ANALOG_IN)
		{
			pControl->vpidSpec.videoFormat = GetAnalogInputVideoFormat(context);
			pControl->isComplete = true;
			break;
		}

		//	If a Color Space Converter
		switch (currentXpt)
		{
		case NTV2_XptCSC1VidYUV:
		case NTV2_XptCSC2VidYUV:
		case NTV2_XptCSC3VidYUV:
		case NTV2_XptCSC4VidYUV:
		case NTV2_XptCSC5VidYUV:
		case NTV2_XptCSC6VidYUV:
		case NTV2_XptCSC7VidYUV:
		case NTV2_XptCSC8VidYUV:
			pControl->flags = (VPIDFlags)(pControl->flags | CSCInPath);
			break;
		case NTV2_XptCSC1KeyYUV:
		case NTV2_XptCSC2KeyYUV:
		case NTV2_XptCSC3KeyYUV:
		case NTV2_XptCSC4KeyYUV:
		case NTV2_XptCSC5KeyYUV:
		case NTV2_XptCSC6KeyYUV:
		case NTV2_XptCSC7KeyYUV:
		case NTV2_XptCSC8KeyYUV:
			pControl->flags = (VPIDFlags)(pControl->flags | KeySignal);
			pControl->flags = (VPIDFlags)(pControl->flags | CSCInPath);
			break;
		case NTV2_XptMixer1KeyYUV:
		case NTV2_XptMixer2KeyYUV:
		case NTV2_XptMixer3KeyYUV:
		case NTV2_XptMixer4KeyYUV:
			pControl->flags = (VPIDFlags)(pControl->flags | KeySignal);
			break;
		//	If a dual link widget DS 1
		case NTV2_XptDuallinkOut1:
		case NTV2_XptDuallinkOut2:
		case NTV2_XptDuallinkOut3:
		case NTV2_XptDuallinkOut4:
		case NTV2_XptDuallinkOut5:
		case NTV2_XptDuallinkOut6:
		case NTV2_XptDuallinkOut7:
		case NTV2_XptDuallinkOut8:
			pControl->vpidSpec.isRGBOnWire = true;
			pControl->vpidSpec.isDualLink = true;
			pControl->isDS1 = true;
			break;
		//	If a dual link widget DS 2
		case NTV2_XptDuallinkOut1DS2:
		case NTV2_XptDuallinkOut2DS2:
		case NTV2_XptDuallinkOut3DS2:
		case NTV2_XptDuallinkOut4DS2:
		case NTV2_XptDuallinkOut5DS2:
		case NTV2_XptDuallinkOut6DS2:
		case NTV2_XptDuallinkOut7DS2:
		case NTV2_XptDuallinkOut8DS2:
			pControl->vpidSpec.isRGBOnWire = true;
			pControl->vpidSpec.isDualLink = true;
			pControl->isDS2 = true;
			break;
		//	If a multi link widget DS 1
		case NTV2_XptMultiLinkOut1DS1:
			pControl->vpidSpec.isMultiLink = true;
			pControl->isML1 = true;
			break;
		//	If a multi link widget DS 2
		case NTV2_XptMultiLinkOut1DS2:
			pControl->vpidSpec.isMultiLink = true;
			pControl->isML2 = true;
			break;
		//	If a multi link widget DS 3
		case NTV2_XptMultiLinkOut1DS3:
			pControl->vpidSpec.isMultiLink = true;
			pControl->isML3 = true;
			break;
		//	If a multi link widget DS 4
		case NTV2_XptMultiLinkOut1DS4:
			pControl->vpidSpec.isMultiLink = true;
			pControl->isML4 = true;
			break;
			//	If a dual link widget 
		case NTV2_XptDuallinkIn1:
			if (pControl->vpidSpec.isRGBOnWire && pControl->vpidSpec.isDualLink && pControl->isDS2)
				source = GetCrosspointIDInput(NTV2_XptDuallinkIn1DS2);
			break;
		case NTV2_XptDuallinkIn2:
			if (pControl->vpidSpec.isRGBOnWire && pControl->vpidSpec.isDualLink && pControl->isDS2)
				source = GetCrosspointIDInput(NTV2_XptDuallinkIn2DS2);
			break;
		case NTV2_XptDuallinkIn3:
			if (pControl->vpidSpec.isRGBOnWire && pControl->vpidSpec.isDualLink && pControl->isDS2)
				source = GetCrosspointIDInput(NTV2_XptDuallinkIn3DS2);
			break;
		case NTV2_XptDuallinkIn4:
			if (pControl->vpidSpec.isRGBOnWire && pControl->vpidSpec.isDualLink && pControl->isDS2)
				source = GetCrosspointIDInput(NTV2_XptDuallinkIn4DS2);
			break;
		case NTV2_XptDuallinkIn5:
			if (pControl->vpidSpec.isRGBOnWire && pControl->vpidSpec.isDualLink && pControl->isDS2)
				source = GetCrosspointIDInput(NTV2_XptDuallinkIn5DS2);
			break;
		case NTV2_XptDuallinkIn6:
			if (pControl->vpidSpec.isRGBOnWire && pControl->vpidSpec.isDualLink && pControl->isDS2)
				source = GetCrosspointIDInput(NTV2_XptDuallinkIn6DS2);
			break;
		case NTV2_XptDuallinkIn7:
			if (pControl->vpidSpec.isRGBOnWire && pControl->vpidSpec.isDualLink && pControl->isDS2)
				source = GetCrosspointIDInput(NTV2_XptDuallinkIn7DS2);
			break;
		case NTV2_XptDuallinkIn8:
			if (pControl->vpidSpec.isRGBOnWire && pControl->vpidSpec.isDualLink && pControl->isDS2)
				source = GetCrosspointIDInput(NTV2_XptDuallinkIn8DS2);
			break;
		default:
			break;
		}

		//	If the 4K down converter
		newVideoFormat = NTV2_FORMAT_UNKNOWN;
		switch (currentXpt)
		{
		case NTV2_Xpt4KDownConverterOut:
			pControl->flags = (VPIDFlags)(pControl->flags | DC4KInPath);
			break;
		case NTV2_XptConversionModule:
			GetK2ConverterOutFormat(context, &newVideoFormat);
			pControl->vpidSpec.videoFormat = newVideoFormat;
			break;
		default:
			break;
		}

		if (pControl->isComplete)
			return true;

		//	Else back up one more widget and try again
		ntv2ReadRegisterMS(
			context,
			source.registerNumber,
			(ULWord*)&currentXpt,
			source.registerMask,
			source.registerShift);
	}
	return true;
}	//	FindVPID

bool SetVPIDOutput(Ntv2SystemContext* context, NTV2Channel channel)
{
	NTV2VideoFormat		videoFormat = GetBoardVideoFormat(context, NTV2_CHANNEL1);
	NTV2VideoFormat		shadowFormat = (NTV2VideoFormat)ntv2ReadVirtualRegister(context, kVRegVideoFormatCh1);
	NTV2OutputXptID		startingXpt = NTV2_XptBlack;
	bool 				is3G = 0;
	bool 				is3Gb = 0;
	bool 				isRGBLevelA = 0;
	bool 				isLevelA2B = 0;
	VPIDControl			vpidControlDS1;
	VPIDControl			vpidControlDS2;
	NTV2DeviceID		deviceID = (NTV2DeviceID)ntv2ReadRegister(context, kRegBoardID);

	if (IsMultiFormatActive(context))
	{
		videoFormat = GetBoardVideoFormat(context, channel);
		shadowFormat = (NTV2VideoFormat)ntv2ReadVirtualRegister(context, gShadowRegs[channel]);
	}

	//	Allows the formst to be psf, even though the hardware is i
	if ( ! NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE (videoFormat) && NTV2_IS_PSF_VIDEO_FORMAT (shadowFormat))
	{
		videoFormat = shadowFormat;
	}

	//	"Construct" the objects to track the backtrace
	memset((void*)&vpidControlDS1, 0, sizeof(vpidControlDS1));

	vpidControlDS1.vpidSpec.videoFormat = videoFormat;
	vpidControlDS1.vpidSpec.vpidChannel = VPIDChannel_1;
	vpidControlDS1.deviceNumber = 0;
	vpidControlDS1.videoChannel = channel;
	vpidControlDS1.frameStoreIndex = 0xFF;			//	Illegal index
	vpidControlDS1.flags = (VPIDFlags)0;
	vpidControlDS1.isDS2 = false;
	vpidControlDS1.isComplete = false;
	vpidControlDS1.value = 0;
	vpidControlDS1.vpidSpec.enableBT2020 = false;

	//	Get level information from the SDI output
	GetXptSDIOutInputSelect(context, channel, &startingXpt);

	GetSDIOut3GEnable(context, channel, &is3G);
	vpidControlDS1.is3G = is3G;
	GetSDIOut3GbEnable(context, channel, &is3Gb);
	GetSDIOutRGBLevelAConversion(context, channel, &isRGBLevelA);
	GetSDIOutLevelAtoLevelBConversion(context, channel, &isLevelA2B);
	GetSDIOut6GEnable(context, channel, &vpidControlDS1.vpidSpec.isOutput6G);
	GetSDIOut12GEnable(context, channel, &vpidControlDS1.vpidSpec.isOutput12G);
	vpidControlDS2.vpidSpec.isOutput6G = vpidControlDS1.vpidSpec.isOutput6G;
	vpidControlDS2.vpidSpec.isOutput12G = vpidControlDS1.vpidSpec.isOutput12G;

	if (is3G)
	{
		if (isRGBLevelA)
			vpidControlDS1.vpidSpec.isOutputLevelA = vpidControlDS2.vpidSpec.isOutputLevelA = true;
		else
		{
			if (is3Gb)
				vpidControlDS1.vpidSpec.isOutputLevelB = vpidControlDS2.vpidSpec.isOutputLevelB = true;
			else
				vpidControlDS1.vpidSpec.isOutputLevelA = vpidControlDS2.vpidSpec.isOutputLevelA = true;
		}
	}

	memcpy(&vpidControlDS2, &vpidControlDS1, sizeof(VPIDControl));
	vpidControlDS2.isDS2 = vpidControlDS1.vpidSpec.isOutputLevelA ? false : true;

	//	Find VPID for DS1
	FindVPID(context, startingXpt, &vpidControlDS1);

	//	Find VPID for DS2
	GetXptSDIOutDS2InputSelect(context, channel, &startingXpt);
	FindVPID(context, startingXpt, &vpidControlDS2);

	if (vpidControlDS1.isComplete)
	{
		if (vpidControlDS1.flags & DC4KInPath)
		{
			AdjustFor4KDC(context, &vpidControlDS1);
		}
		else if (NTV2_IS_4K_VIDEO_FORMAT(vpidControlDS1.vpidSpec.videoFormat))
		{
			if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1) && channel == NTV2_CHANNEL5)
			{
				vpidControlDS1.vpidSpec.videoFormat = GetHDSizedVideoFormat(vpidControlDS1.vpidSpec.videoFormat);
			}
			else if (NTV2DeviceCanDo12GSDI(deviceID) && !NTV2DeviceCanDo12gRouting(deviceID) && channel != NTV2_CHANNEL3)
			{
				bool is6g = false;
				bool is12g = false;
				GetSDIOut6GEnable(context, NTV2_CHANNEL3, &is6g);
				GetSDIOut12GEnable(context, NTV2_CHANNEL3, &is12g);
				if(is6g || is12g)
				{
					vpidControlDS1.vpidSpec.videoFormat = GetHDSizedVideoFormat(vpidControlDS1.vpidSpec.videoFormat);
					vpidControlDS1.vpidSpec.isOutput6G = false;
					vpidControlDS1.vpidSpec.isOutput12G = false;
					vpidControlDS1.vpidSpec.isTwoSampleInterleave = false;
					if(vpidControlDS1.vpidSpec.isDualLink)
					{
						vpidControlDS1.vpidSpec.vpidChannel = VPIDChannel_1;
						vpidControlDS1.vpidSpec.useChannel = true;
					}
				}
			}
		}

		if (vpidControlDS1.vpidSpec.isOutput6G || vpidControlDS1.vpidSpec.isOutput12G)
		{
			if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1) && vpidControlDS1.videoChannel == NTV2_CHANNEL5)
			{
				vpidControlDS1.vpidSpec.isOutput6G = false;
				vpidControlDS1.vpidSpec.isOutput12G = false;
				vpidControlDS1.vpidSpec.useChannel = false;
			}
			else if (vpidControlDS1.vpidSpec.isTwoSampleInterleave)
			{
				vpidControlDS1.vpidSpec.videoFormat = GetQuadSizedVideoFormat(vpidControlDS1.vpidSpec.videoFormat);
				//vpidControlDS1.vpidSpec.isTwoSampleInterleave = true;
				vpidControlDS1.vpidSpec.useChannel = true;
			}
		}

		if (vpidControlDS1.value == 0)
		{
			if (!SetVPIDFromSpec(&vpidControlDS1.value,
				&vpidControlDS1.vpidSpec))
			{
				vpidControlDS1.value = 0;
			}
		}
		else
		{
			if (deviceID == DEVICE_ID_KONA5_3DLUT)
			{
				//override the E2E HDR outputs with user defined
				SetTransferCharacteristics(&vpidControlDS1.value, (NTV2VPIDTransferCharacteristics)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDTransferCharacteristics[NTV2_CHANNEL2]));
				SetColorimetry(&vpidControlDS1.value, (NTV2VPIDColorimetry)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDColorimetry[NTV2_CHANNEL2]));
				SetLuminance(&vpidControlDS1.value, (NTV2VPIDLuminance)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDLuminance[NTV2_CHANNEL2]));
				SetRGBRange(&vpidControlDS1.value, (NTV2VPIDRGBRange)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDRGBRange[NTV2_CHANNEL2]));
			}

			if (((deviceID == DEVICE_ID_KONA5 || deviceID == DEVICE_ID_IO4KPLUS) && channel != NTV2_CHANNEL3))
			{
				if ((vpidControlDS1.value & 0xFF000000) == 0xCE000000)
				{
					vpidControlDS1.value -= 0x45000000;
				}
				else if ((vpidControlDS1.value & 0xFF000000) == 0xC0000000)
				{
					vpidControlDS1.value -= 0x3b000000;
				}
			}
		}
	}

	if (vpidControlDS2.isComplete)
	{
		if (vpidControlDS2.flags & DC4KInPath)
		{
			AdjustFor4KDC(context, &vpidControlDS2);
		}
		else if (NTV2_IS_4K_VIDEO_FORMAT(vpidControlDS2.vpidSpec.videoFormat))
		{
			if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1) && channel == NTV2_CHANNEL5)
			{
				vpidControlDS2.vpidSpec.videoFormat = GetHDSizedVideoFormat(vpidControlDS2.vpidSpec.videoFormat);
			}
			else if (NTV2DeviceCanDo12GSDI(deviceID) && !NTV2DeviceCanDo12gRouting(deviceID) && channel != NTV2_CHANNEL3)
			{
				bool is6g = false;
				bool is12g = false;
				GetSDIOut6GEnable(context, NTV2_CHANNEL3, &is6g);
				GetSDIOut12GEnable(context, NTV2_CHANNEL3, &is12g);
				if(is6g || is12g)
				{
					vpidControlDS2.vpidSpec.videoFormat = GetHDSizedVideoFormat(vpidControlDS2.vpidSpec.videoFormat);
					vpidControlDS2.vpidSpec.isOutput6G = false;
					vpidControlDS2.vpidSpec.isOutput12G = false;
					vpidControlDS2.vpidSpec.isTwoSampleInterleave = false;
					if(vpidControlDS2.vpidSpec.isDualLink)
					{
						vpidControlDS2.vpidSpec.vpidChannel = VPIDChannel_2;
						vpidControlDS2.vpidSpec.useChannel = true;
					}
				}
			}
		}

		if (vpidControlDS2.vpidSpec.isOutput6G || vpidControlDS2.vpidSpec.isOutput12G)
		{
			if (NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1) && vpidControlDS2.videoChannel == NTV2_CHANNEL5)
			{
				vpidControlDS2.vpidSpec.isOutput6G = false;
				vpidControlDS2.vpidSpec.isOutput12G = false;
				vpidControlDS2.vpidSpec.useChannel = false;
			}
			else if (vpidControlDS2.vpidSpec.isTwoSampleInterleave)
			{
				vpidControlDS2.vpidSpec.videoFormat = GetQuadSizedVideoFormat(vpidControlDS2.vpidSpec.videoFormat);
				//vpidControlDS2.vpidSpec.isTwoSampleInterleave = true;
				vpidControlDS2.vpidSpec.useChannel = false;
			}
		}

		if(vpidControlDS2.value == 0)
		{
			if (!SetVPIDFromSpec(&vpidControlDS2.value,
				&vpidControlDS2.vpidSpec))
			{
				vpidControlDS2.value = 0;
			}
		}
		else
		{
			if (deviceID == DEVICE_ID_KONA5_3DLUT)
			{
				//override the E2E HDR outputs with user defined
				SetTransferCharacteristics(&vpidControlDS2.value, (NTV2VPIDTransferCharacteristics)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDTransferCharacteristics[NTV2_CHANNEL2]));
				SetColorimetry(&vpidControlDS2.value, (NTV2VPIDColorimetry)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDColorimetry[NTV2_CHANNEL2]));
				SetLuminance(&vpidControlDS2.value, (NTV2VPIDLuminance)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDLuminance[NTV2_CHANNEL2]));
				SetRGBRange(&vpidControlDS2.value, (NTV2VPIDRGBRange)ntv2ReadVirtualRegister(context, gChannelToSDIOutVPIDRGBRange[NTV2_CHANNEL2]));
			}

			if (((deviceID == DEVICE_ID_KONA5 || deviceID == DEVICE_ID_IO4KPLUS) && channel != NTV2_CHANNEL3))
			{
				if ((vpidControlDS2.value & 0xFF000000) == 0xCE000000)
				{
					vpidControlDS2.value -= 0x45000000;
				}
				else if ((vpidControlDS2.value & 0xFF000000) == 0xC0000000)
				{
					vpidControlDS2.value -= 0x3b000000;
				}
			}
		}
	}
	else
	{
		if (vpidControlDS1.vpidSpec.isOutputLevelA)
			memcpy(&vpidControlDS2, &vpidControlDS1, sizeof(VPIDControl));
	}

	if (!vpidControlDS1.vpidSpec.isTwoSampleInterleave)
	{
		//	For level B routings converted from level A, DS2 gets the same VPID as DS1 + channel 2
		if (is3Gb && isLevelA2B)
			vpidControlDS2.value = vpidControlDS1.value | 0x40;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtautological-compare"
	if (((vpidControlDS1.flags & KeySignal) == 1) && NTV2DeviceCanDo25GIP(deviceID))
		Set2110Key(context, channel, vpidControlDS1.flags & KeySignal);
#pragma GCC diagnostic pop

    return SetSDIOutVPID(context, channel, vpidControlDS1.value, vpidControlDS2.value);
}	//	SetVPIDOutput
