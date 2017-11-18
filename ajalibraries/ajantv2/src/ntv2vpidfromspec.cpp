/**
	@file		ntv2vpidfromspec.cpp
	@brief		Generates a VPID based on a specification struct. See the SMPTE 352 standard for details.
	@copyright	(C) 2012-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
	@note		This file is included in driver builds. It must not contain any c++.
**/

#include "ajatypes.h"
#include "ntv2vpidfromspec.h"

#if !defined(NTV2_BUILDING_DRIVER)
	#include "ntv2utils.h"
#elif defined(AJALinux)
	#define GetNTV2FrameRateFromVideoFormat GetNTV2ActualFrameRateFromVideoFormat
	NTV2Standard    GetNTV2StandardFromVideoFormat  (NTV2VideoFormat videoFormat);
	NTV2FrameRate   GetNTV2ActualFrameRateFromVideoFormat (NTV2VideoFormat videoFormat);
#elif defined(AJAWindows)
	#include "ntv2device.h"
	#define GetNTV2StandardFromVideoFormat	CNTV2Device::GetNTV2StandardFromVideoFormat
	#define GetNTV2FrameRateFromVideoFormat	CNTV2Device::GetNTV2ActualFrameRateFromVideoFormat
#elif defined(AJAMac)
	#include "MacDriver.h"
	#define GetNTV2StandardFromVideoFormat	MacDriver::GetNTV2StandardFromVideoFormat
	#define GetNTV2FrameRateFromVideoFormat	MacDriver::GetNTV2FrameRateFromVideoFormat
#else
	#error "Unimplemented platform"
#endif

bool SetVPIDFromSpec (ULWord * const			pOutVPID,
					  const VPIDSpec * const	pInVPIDSpec)
{
	NTV2VideoFormat			outputFormat	= NTV2_FORMAT_UNKNOWN;
	NTV2FrameBufferFormat	pixelFormat		= NTV2_FBF_INVALID;
	NTV2FrameRate			frameRate		= NTV2_FRAMERATE_UNKNOWN;

	bool	isProgressivePicture	= false;
	bool	isProgressiveTransport	= false;
	bool	isDualLink				= false;
	bool	isLevelA				= false;
	bool	isLevelB				= false;
	bool	is3G					= false;
	bool	isRGB					= false;
	bool	isTSI					= false;
	bool	isStereo				= false;
	bool	is6G					= false;
	bool	is12G					= false;
	VPIDChannel vpidChannel			= VPIDChannel_1;

	uint8_t	byte1 = 0;
	uint8_t	byte2 = 0;
	uint8_t	byte3 = 0;
	uint8_t	byte4 = 0;

	if (! pOutVPID || ! pInVPIDSpec)
		return false;

	outputFormat			= pInVPIDSpec->videoFormat;
	pixelFormat				= pInVPIDSpec->pixelFormat;
	isLevelA				= pInVPIDSpec->isOutputLevelA;
	isLevelB				= pInVPIDSpec->isOutputLevelB;
	is3G					= isLevelA || isLevelB;
	isDualLink				= pInVPIDSpec->isDualLink;
	isRGB					= pInVPIDSpec->isRGBOnWire;
	isTSI					= pInVPIDSpec->isTwoSampleInterleave;
	isStereo				= pInVPIDSpec->isStereo;
	is6G					= pInVPIDSpec->isOutput6G;
	is12G					= pInVPIDSpec->isOutput12G;
	vpidChannel				= pInVPIDSpec->vpidChannel;

	if (! NTV2_IS_WIRE_FORMAT (outputFormat))
	{
		*pOutVPID = 0;
		return true;
	}

	if (is6G || is12G)
		vpidChannel = VPIDChannel_1;

	frameRate				= GetNTV2FrameRateFromVideoFormat			(outputFormat);
	isProgressivePicture	= NTV2_VIDEO_FORMAT_HAS_PROGRESSIVE_PICTURE (outputFormat);
	isProgressiveTransport	= isProgressivePicture;							//	Must be a progressive format to start

	if (NTV2_IS_720P_VIDEO_FORMAT (outputFormat))
		isProgressiveTransport = false;										//	720p does not use progressive transport

	if (NTV2_IS_PSF_VIDEO_FORMAT (outputFormat))
		isProgressiveTransport = false;										//	PSF is never a progressive transport

	if ( ! isRGB && isDualLink &&  ! isTSI)
		isProgressiveTransport = false;										//	Dual link YCbCr is not a progressive transport

	if (isTSI && NTV2_IS_4K_HFR_VIDEO_FORMAT (outputFormat) && isLevelB)
		isProgressiveTransport = false;										//	Only TSI Quad Link 3.0 HFR Level B is not progressive

	//
	//	Byte 1
	//

	switch (outputFormat)
	{
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_525psf_2997:
	case NTV2_FORMAT_625psf_2500:
		byte1 = is3G ? (uint8_t) VPIDStandard_483_576_3Gb : (uint8_t) VPIDStandard_483_576;	//	0x8D : 0x81
		break;

	case NTV2_FORMAT_720p_2398:
	case NTV2_FORMAT_720p_2500:
	case NTV2_FORMAT_720p_5000:
	case NTV2_FORMAT_720p_5994:
	case NTV2_FORMAT_720p_6000:
		if (is3G)
		{
			if (isLevelB)
				byte1 = isStereo ? (uint8_t) VPIDStandard_720_Stereo_3Gb : (uint8_t) VPIDStandard_720_3Gb;	//	0x8E : 0x8B
			else
				byte1 = isStereo ? (uint8_t) VPIDStandard_720_Stereo_3Ga : (uint8_t) VPIDStandard_720_3Ga;	//	0x91 : 0x88
		}
		else
			byte1 = (isStereo && isDualLink) ? (uint8_t) VPIDStandard_720_1080_Stereo : (uint8_t) VPIDStandard_720;		//	0xB1 : 0x84
		break;

	case NTV2_FORMAT_1080i_5000:		//	Same as NTV2_FORMAT_1080psf_2500
	case NTV2_FORMAT_1080i_5994:		//	Same as NTV2_FORMAT_1080psf_2997
	case NTV2_FORMAT_1080i_6000:		//	Same as NTV2_FORMAT_1080psf_3000
	case NTV2_FORMAT_1080psf_2398:
	case NTV2_FORMAT_1080psf_2400:
	case NTV2_FORMAT_1080psf_2500_2:
	case NTV2_FORMAT_1080psf_2997_2:
	case NTV2_FORMAT_1080psf_3000_2:
	case NTV2_FORMAT_1080p_2398:
	case NTV2_FORMAT_1080p_2400:
	case NTV2_FORMAT_1080p_2500:
	case NTV2_FORMAT_1080p_2997:
	case NTV2_FORMAT_1080p_3000:
	case NTV2_FORMAT_1080psf_2K_2398:
	case NTV2_FORMAT_1080psf_2K_2400:
	case NTV2_FORMAT_1080psf_2K_2500:
	case NTV2_FORMAT_1080p_2K_2398:
	case NTV2_FORMAT_1080p_2K_2400:
	case NTV2_FORMAT_1080p_2K_2500:
	case NTV2_FORMAT_1080p_2K_2997:
	case NTV2_FORMAT_1080p_2K_3000:
		if (is3G)
		{
			if (isLevelB)
			{
				if (isDualLink)
					byte1 = isStereo ? (uint8_t) VPIDStandard_1080_Stereo_3Gb : (uint8_t) VPIDStandard_1080_DualLink_3Gb;	//	0x8F : 0x8A
				else
					byte1 = isStereo ? (uint8_t) VPIDStandard_1080_Stereo_3Gb : (uint8_t) VPIDStandard_1080_3Gb;	//	0x8F : 0x8C
			}
			else
			{
				byte1 = isStereo ? (uint8_t) VPIDStandard_1080_Stereo_3Ga : (uint8_t) VPIDStandard_1080_3Ga;	//	0x92 : 0x89
			}
		}
		else
		{
			if (isDualLink)
				byte1 = isStereo ? (uint8_t) VPIDStandard_720_1080_Stereo : (uint8_t) VPIDStandard_1080_DualLink;		//	0xB1 : 0x87
			else
				byte1 = isStereo ? (uint8_t) VPIDStandard_720_1080_Stereo : (uint8_t) VPIDStandard_1080;		//	0xB1 : 0x85
		}
		break;

	case NTV2_FORMAT_1080p_5000_A:
	case NTV2_FORMAT_1080p_5000_B:
	case NTV2_FORMAT_1080p_5994_A:
	case NTV2_FORMAT_1080p_5994_B:
	case NTV2_FORMAT_1080p_6000_A:
	case NTV2_FORMAT_1080p_6000_B:
	case NTV2_FORMAT_1080p_2K_4795_A:
	case NTV2_FORMAT_1080p_2K_4800_A:
	case NTV2_FORMAT_1080p_2K_5000_A:
	case NTV2_FORMAT_1080p_2K_5000_B:
	case NTV2_FORMAT_1080p_2K_5994_A:
	case NTV2_FORMAT_1080p_2K_5994_B:
	case NTV2_FORMAT_1080p_2K_6000_A:
	case NTV2_FORMAT_1080p_2K_6000_B:
		if (isDualLink)
			byte1 = isLevelB ? (uint8_t) VPIDStandard_1080_DualLink_3Gb : (uint8_t) VPIDStandard_1080_DualLink;		//	0x8A : 0x87
		else
			byte1 = isLevelB ? (uint8_t) VPIDStandard_1080_DualLink_3Gb : (uint8_t) VPIDStandard_1080_3Ga;			//	0x8A : 0x89
		break;

	case NTV2_FORMAT_4x1920x1080psf_2398:
	case NTV2_FORMAT_4x1920x1080psf_2400:
	case NTV2_FORMAT_4x1920x1080psf_2500:
	case NTV2_FORMAT_4x2048x1080psf_2398:
	case NTV2_FORMAT_4x2048x1080psf_2400:
	case NTV2_FORMAT_4x2048x1080psf_2500:
	case NTV2_FORMAT_4x1920x1080p_2398:
	case NTV2_FORMAT_4x1920x1080p_2400:
	case NTV2_FORMAT_4x1920x1080p_2500:
	case NTV2_FORMAT_4x1920x1080p_2997:
	case NTV2_FORMAT_4x1920x1080p_3000:
	case NTV2_FORMAT_4x2048x1080p_2398:
	case NTV2_FORMAT_4x2048x1080p_2400:
	case NTV2_FORMAT_4x2048x1080p_2500:
	case NTV2_FORMAT_4x2048x1080p_2997:
	case NTV2_FORMAT_4x2048x1080p_3000:
		if (isTSI)
		{
			if(is12G)
				byte1 = VPIDStandard_2160_Single_12Gb; //0xCE
			else if(is6G)
				byte1 = VPIDStandard_2160_Single_6Gb; //0xC0
			else if (is3G)
			{
				if (isLevelB)
					byte1 = isDualLink? (uint8_t) VPIDStandard_2160_QuadDualLink_3Gb : (uint8_t) VPIDStandard_2160_DualLink;  //  0x98 : 0x96
				else
					byte1 = (uint8_t) VPIDStandard_2160_QuadLink_3Ga;  //  0x97
			}
			else
				byte1 = (uint8_t) VPIDStandard_1080;  //  0x85 (bogus if not 3G)
		}
		else
		{
			if (is3G)
			{
				if (isLevelB)
					byte1 = isDualLink? (uint8_t) VPIDStandard_1080_DualLink_3Gb : (uint8_t) VPIDStandard_1080_3Gb;  //  8A : 8C
				else
					byte1 = (uint8_t) VPIDStandard_1080_3Ga;   // 89
			}
			else
				byte1 = isDualLink? (uint8_t) VPIDStandard_1080_DualLink : (uint8_t) VPIDStandard_1080;  //  0x87 : 0x85
		}
		break;

	case NTV2_FORMAT_4x1920x1080p_5000:
	case NTV2_FORMAT_4x1920x1080p_5994:
	case NTV2_FORMAT_4x1920x1080p_6000:
	case NTV2_FORMAT_4x2048x1080p_4795:
	case NTV2_FORMAT_4x2048x1080p_4800:
	case NTV2_FORMAT_4x2048x1080p_5000:
	case NTV2_FORMAT_4x2048x1080p_5994:
	case NTV2_FORMAT_4x2048x1080p_6000:
		if (isTSI)
		{
			if(is12G)
				byte1 = VPIDStandard_2160_Single_12Gb;
			else if(is6G)
				byte1 = VPIDStandard_2160_Single_6Gb;
			else
				byte1 = isLevelB ? (uint8_t) VPIDStandard_2160_QuadDualLink_3Gb : (uint8_t) VPIDStandard_2160_QuadLink_3Ga;	//	0x98 : 0x97
		}
		else
		{
			byte1 = isLevelB ? (uint8_t) VPIDStandard_1080_DualLink_3Gb : (uint8_t) VPIDStandard_1080_3Ga;		//	0x8A : 0x89
		}
		break;

	default:
		*pOutVPID = 0;
		return true;
	}

	//
	//	Byte 2
	//

	//	Picture rate
	switch (frameRate)
	{
	case NTV2_FRAMERATE_2398:
		byte2 = VPIDPictureRate_2398;
		break;
	case NTV2_FRAMERATE_2400:
		byte2 = VPIDPictureRate_2400;
		break;
	case NTV2_FRAMERATE_2500:
		byte2 = VPIDPictureRate_2500;
		break;
	case NTV2_FRAMERATE_2997:
		byte2 = VPIDPictureRate_2997;
		break;
	case NTV2_FRAMERATE_3000:
		byte2 = VPIDPictureRate_3000;
		break;
	case NTV2_FRAMERATE_4795:
		byte2 = VPIDPictureRate_4795;
		break;
	case NTV2_FRAMERATE_4800:
		byte2 = VPIDPictureRate_4800;
		break;
	case NTV2_FRAMERATE_5000:
		byte2 = VPIDPictureRate_5000;
		break;
	case NTV2_FRAMERATE_5994:
		byte2 = VPIDPictureRate_5994;
		break;
	case NTV2_FRAMERATE_6000:
		byte2 = VPIDPictureRate_6000;
		break;
	default:
		*pOutVPID = 0;
		return true;
	}

	//	Progressive picture
	byte2 |= isProgressivePicture ? (1UL << 6) : 0;	//	0x40

	//	Progressive transport
	byte2 |= isProgressiveTransport ? (1UL << 7) : 0;	//	0x80

	//
	//	Byte 3
	//

	//	Horizontal pixel count
	if (isStereo)
	{
	}
	else
	{
		byte3 |= NTV2_IS_2K_1080_VIDEO_FORMAT (outputFormat) ? (1UL << 6) : 0;	//	0x40
		byte3 |= NTV2_IS_4K_4096_VIDEO_FORMAT (outputFormat) ? (1UL << 6) : 0;	//	0x40
	}

	//	Aspect ratio
	if ( NTV2_IS_HD_VIDEO_FORMAT		(outputFormat) &&
		 ! NTV2_IS_720P_VIDEO_FORMAT	(outputFormat) &&
		 ! NTV2_IS_2K_1080_VIDEO_FORMAT	(outputFormat))
	{
		if (is3G && !isLevelB && !isDualLink)
			byte3 |= (1UL << 7);			//	0x80
		else
			byte3 |= (1UL << 5);			//	0x20
	}

	if ( NTV2_IS_4K_VIDEO_FORMAT (outputFormat) &&
		 ! NTV2_IS_4K_4096_VIDEO_FORMAT (outputFormat))
	{
		if (is3G && !isLevelB && !isDualLink)
			byte3 |= (1UL << 7);			//	0x80
		else
			byte3 |= (1UL << 5);			//	0x20
	}

	//	Sampling structure
	if (pixelFormat == NTV2_FBF_INVALID)
	{
		//	Pixel format not specified, make a guess
		if (pInVPIDSpec->isRGBOnWire)
			pixelFormat = NTV2_FBF_10BIT_DPX;	//	Most people use this if not 48 bit
		else
			pixelFormat = NTV2_FBF_10BIT_YCBCR;
	}

	if (isRGB)
	{
		switch (pixelFormat)
		{
		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_10BIT_ARGB:
		case NTV2_FBF_ABGR:
			byte3 |= VPIDSampling_GBRA_4444;
			break;

		case NTV2_FBF_10BIT_DPX:
		case NTV2_FBF_10BIT_DPX_LITTLEENDIAN:
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:
		case NTV2_FBF_48BIT_RGB:
		case NTV2_FBF_10BIT_RGB_PACKED:
			byte3 |= VPIDSampling_GBR_444;
			break;

		//	Although RGB is on the wire, the pixel format can be YCbCr if
		//	the signal is routed through a CSC before going to a Dual Link.
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:
		case NTV2_FBF_10BIT_YCBCR_DPX:
			byte3 |= VPIDSampling_GBR_444;
			break;

		default:
			*pOutVPID = 0;
			return true;
		}
	}
	else
	{
		switch (pixelFormat)
		{
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:
		case NTV2_FBF_10BIT_YCBCR_DPX:
			byte3 |= VPIDSampling_YUV_422;
			break;

		default:
			*pOutVPID = 0;
			return true;
		}
	}

	//
	//	Byte 4
	//

	//	VPID channel
	if (pInVPIDSpec->isTwoSampleInterleave)
	{
		if (isLevelB && NTV2_IS_4K_HFR_VIDEO_FORMAT (outputFormat))
			byte4 |= vpidChannel << 5;
		else
			byte4 |= vpidChannel << 6;
	}
	else
	{
		if (pInVPIDSpec->useChannel)
			byte4 |= vpidChannel << 6;	
	}

	//	Audio
	if (isStereo)
	{
		byte4 |= pInVPIDSpec->isRightEye	<< 6;		//	0x40
		byte4 |= pInVPIDSpec->audioCarriage	<< 2;		//	0x0C
	}

	//	Bit depth
	switch (pixelFormat)
	{
	case NTV2_FBF_ARGB:
	case NTV2_FBF_RGBA:
	case NTV2_FBF_ABGR:
	case NTV2_FBF_8BIT_YCBCR:
	case NTV2_FBF_8BIT_YCBCR_YUY2:
	case NTV2_FBF_24BIT_BGR:
	case NTV2_FBF_24BIT_RGB:
		byte4 |= VPIDBitDepth_8;
		break;

	case NTV2_FBF_10BIT_YCBCR:
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_10BIT_DPX:
	case NTV2_FBF_10BIT_DPX_LITTLEENDIAN:
	case NTV2_FBF_10BIT_RGB_PACKED:
	case NTV2_FBF_10BIT_YCBCR_DPX:
	case NTV2_FBF_10BIT_ARGB:
		byte4 |= VPIDBitDepth_10;
		break;

	case NTV2_FBF_48BIT_RGB:
		byte4 |= VPIDBitDepth_12;
		break;

	default:
		*pOutVPID = 0;
		return true;
	}

	//	Return VPID value to caller
	*pOutVPID = ((ULWord)byte1 << 24) | ((ULWord)byte2 << 16) | ((ULWord)byte3 << 8) | byte4;

	return true;
}

