/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA boards.
//
// Filename:	ntv2kona2.c
// Purpose: 	Support for Kona2 registers.
// Notes:
//
///////////////////////////////////////////////////////////////

#include <linux/fs.h>
#include <linux/interrupt.h>

#include "ajatypes.h"
#include "ntv2enums.h"
#include "ntv2publicinterface.h"
#include "ntv2devicefeatures.h"

#include "ntv2driver.h"
#include "ntv2linuxpublicinterface.h"
#include "registerio.h"
#include "ntv2kona2.h"
#include "driverdbg.h"
#include "../ntv2kona.h"


void SetXpt8SDIOut4InputSelect (ULWord boardNumber, NTV2OutputXptID value)
{
	WriteRegister (boardNumber,
				   kRegXptSelectGroup8,
				   value,
				   kK2RegMaskSDIOut4InputSelect,
				   kK2RegShiftSDIOut4InputSelect);
}

void GetXpt8SDIOut4InputSelect(ULWord boardNumber, NTV2OutputXptID* value)
{
	*value = ReadRegister (boardNumber,
						   kRegXptSelectGroup8,
						   kK2RegMaskSDIOut4InputSelect,
						   kK2RegShiftSDIOut4InputSelect);
}

void SetXpt8SDIOut3InputSelect (ULWord boardNumber, NTV2OutputXptID value)
{
	WriteRegister (boardNumber,
				   kRegXptSelectGroup8,
				   value,
				   kK2RegMaskSDIOut3InputSelect,
				   kK2RegShiftSDIOut3InputSelect);
}

void GetXpt8SDIOut3InputSelect(ULWord boardNumber, NTV2OutputXptID* value)
{
	*value = ReadRegister (boardNumber,
						   kRegXptSelectGroup8,
						   kK2RegMaskSDIOut3InputSelect,
						   kK2RegShiftSDIOut3InputSelect);
}

bool GetConverterOutFormat(ULWord boardNumber, NTV2VideoFormat* format)
{
	NTV2VideoFormat outFormat = NTV2_FORMAT_UNKNOWN;
	NTV2Standard standard = NTV2_NUM_STANDARDS;
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	NTV2OutputXptID xptSelect;
	bool isQuadMode = false;
	bool isQuadQuadMode = false;
	HDRDriverValues hdrRegValues;
	Ntv2SystemContext systemContext;
	systemContext.devNum = boardNumber;

	if(!FindCrosspointSource(&systemContext, &xptSelect, NTV2_XptConversionModule))
	{
		return false;
	}

	GetSourceVideoFormat(&systemContext, &videoFormat, xptSelect, &isQuadMode, &isQuadQuadMode, &hdrRegValues);
	GetConverterOutStandard(&systemContext, &standard);

	switch(videoFormat)
	{
	case NTV2_FORMAT_1080i_6000:
	case NTV2_FORMAT_1080p_6000_A:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_720p_6000:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080i_6000;
			break;
		case NTV2_STANDARD_720:
			outFormat = NTV2_FORMAT_720p_6000;
			break;
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_6000_B;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_525_5994:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080i_5994;
			break;
		case NTV2_STANDARD_720:
			outFormat = NTV2_FORMAT_720p_5994;
			break;
		case NTV2_STANDARD_525:
			outFormat = NTV2_FORMAT_525_5994;
			break;
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_5994_B;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_625_5000:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080i_5000;
			break;
		case NTV2_STANDARD_720:
			outFormat = NTV2_FORMAT_720p_5000;
			break;
		case NTV2_STANDARD_625:
			outFormat = NTV2_FORMAT_625_5000;
			break;
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_5000_B;
			break;
		default:
			return false;
		}
		break;
	// case NTV2_FORMAT_1080psf_2500:  // Same as NTV2_FORMAT_1080i_5000 in case above
	case NTV2_FORMAT_1080psf_2K_2500:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080i_5000;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080psf_2K_2500;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2K_2400:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080psf_2400;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080psf_2K_2400;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080psf_2K_2398:
		switch(standard)
		{
		case NTV2_STANDARD_1080:
			outFormat = NTV2_FORMAT_1080psf_2398;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080psf_2K_2398;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_2K_1500:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_3000;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_2K_1500;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_2K_1498:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_2997;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_2K_1498;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_1080p_2K_2500:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_2500;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080p_2K_2500;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2K_2400:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_2400;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080p_2K_2400;
			break;
		default:
			return false;
		}
		break;
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2K_2398:
		switch(standard)
		{
		case NTV2_STANDARD_1080p:
			outFormat = NTV2_FORMAT_1080p_2398;
			break;
		case NTV2_STANDARD_2K:
			outFormat = NTV2_FORMAT_1080p_2K_2398;
			break;
		default:
			return false;
		}
		break;
	default:
		return false;
	}

	if(format != NULL)
	{
		*format = outFormat;
	}

	return true;
}

NTV2VideoFormat GetDeviceVideoFormat(ULWord boardNumber, NTV2Channel channel)
{
	Ntv2SystemContext systemContext;
	NTV2Standard standard;
	NTV2FrameRate frameRate;
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	ULWord smpte372Enabled;
	systemContext.devNum = boardNumber;
	standard = GetStandard(&systemContext, channel);
	frameRate = GetFrameRate(&systemContext, channel);
	smpte372Enabled = GetSmpte372(&systemContext, channel)?1:0;

	if (NTV2DeviceGetVideoFormatFromState(&videoFormat, frameRate, GetFrameGeometry(&systemContext, channel), standard, smpte372Enabled))
		return videoFormat;
	else
		return NTV2_FORMAT_UNKNOWN;
}

uint32_t ntv2ReadRegCon32(Ntv2SystemContext* context, uint32_t regNum)
{
	if (context == NULL) return 0;
	return ReadRegister(context->devNum, regNum, NO_MASK, NO_SHIFT);
}

bool ntv2ReadRegMSCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t* regValue, RegisterMask mask, RegisterShift shift)
{
	if (context == NULL) return false;
	*regValue = ReadRegister(context->devNum, regNum, mask, shift);
	return true;
}

bool ntv2WriteRegCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue)
{
	if (context == NULL) return false;
	WriteRegister(context->devNum, regNum, regValue, NO_MASK, NO_SHIFT);
	return true;
}

bool ntv2WriteRegMSCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t regValue, RegisterMask mask, RegisterShift shift)
{
	if (context == NULL) return false;
	WriteRegister(context->devNum, regNum, regValue, mask, shift);
	return true;
}

uint32_t ntv2ReadVirtRegCon32(Ntv2SystemContext* context, uint32_t regNum)
{
	if (context == NULL) return 0;
	return ReadRegister(context->devNum, regNum, NO_MASK, NO_SHIFT);
}

bool ntv2WriteVirtRegCon32(Ntv2SystemContext* context, uint32_t regNum, uint32_t data)
{
	if (context == NULL) return 0;
	WriteRegister(context->devNum, regNum, data, NO_MASK, NO_SHIFT);
	return true;
}

