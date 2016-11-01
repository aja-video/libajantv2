/**
	@file		ntv2vpid.cpp
	@brief		Implements the CNTV2VPID class. See the SMPTE 352 standard for details.
	@copyright	(C) 2012-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2vpid.h"
#include "ntv2vpidfromspec.h"
#include "ntv2debug.h"
#include <string.h>
#include <assert.h>

#ifndef NULL
#define NULL (0)
#endif

static NTV2VideoFormat	stTable720p			[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable2048p		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable1920p		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable2048psf		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable1920psf		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable2048i		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable1920i		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable3840pSID		[VPIDPictureRate_ReservedF + 1];
static NTV2VideoFormat	stTable4096pSID		[VPIDPictureRate_ReservedF + 1];
static bool				stTablesInitialized	(false);

class VPIDTableInitializer
{
	public:
		VPIDTableInitializer ()
		{
			for (int i = 0;  i < VPIDPictureRate_ReservedF + 1;  i++)
			{
				stTable720p[i]  = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
				stTable2048p[i] = NTV2_FORMAT_UNKNOWN;
			}

			stTable720p[VPIDPictureRate_2398] = NTV2_FORMAT_720p_2398;
	//		stTable720p[VPIDPictureRate_2400] = NTV2_FORMAT_720p_2400;
			stTable720p[VPIDPictureRate_2500] = NTV2_FORMAT_720p_2500;
	//		stTable720p[VPIDPictureRate_2997] = NTV2_FORMAT_720p_2997;
	//		stTable720p[VPIDPictureRate_3000] = NTV2_FORMAT_720p_3000;
			stTable720p[VPIDPictureRate_5000] = NTV2_FORMAT_720p_5000;
			stTable720p[VPIDPictureRate_5994] = NTV2_FORMAT_720p_5994;
			stTable720p[VPIDPictureRate_6000] = NTV2_FORMAT_720p_6000;

			stTable2048p[VPIDPictureRate_2398] = NTV2_FORMAT_1080p_2K_2398;
			stTable2048p[VPIDPictureRate_2400] = NTV2_FORMAT_1080p_2K_2400;
			stTable2048p[VPIDPictureRate_2500] = NTV2_FORMAT_1080p_2K_2500;
			stTable2048p[VPIDPictureRate_2997] = NTV2_FORMAT_1080p_2K_2997;
			stTable2048p[VPIDPictureRate_3000] = NTV2_FORMAT_1080p_2K_3000;
			stTable2048p[VPIDPictureRate_4795] = NTV2_FORMAT_1080p_2K_4795_A;		// 3G-A
			stTable2048p[VPIDPictureRate_4800] = NTV2_FORMAT_1080p_2K_4800_A;		// 3G-A
			stTable2048p[VPIDPictureRate_5000] = NTV2_FORMAT_1080p_2K_5000_A;		// 3G-A
			stTable2048p[VPIDPictureRate_5994] = NTV2_FORMAT_1080p_2K_5994_A;		// 3G-A
			stTable2048p[VPIDPictureRate_6000] = NTV2_FORMAT_1080p_2K_6000_A;		// 3G-A
		
			stTable1920p[VPIDPictureRate_2398] = NTV2_FORMAT_1080p_2398;
			stTable1920p[VPIDPictureRate_2400] = NTV2_FORMAT_1080p_2400;
			stTable1920p[VPIDPictureRate_2500] = NTV2_FORMAT_1080p_2500;
			stTable1920p[VPIDPictureRate_2997] = NTV2_FORMAT_1080p_2997;
			stTable1920p[VPIDPictureRate_3000] = NTV2_FORMAT_1080p_3000;
	//		stTable1920p[VPIDPictureRate_4795] = NTV2_FORMAT_1080p_4795;			// 3G-A
	//		stTable1920p[VPIDPictureRate_4800] = NTV2_FORMAT_1080p_4800;			// 3G-A
			stTable1920p[VPIDPictureRate_5000] = NTV2_FORMAT_1080p_5000_A;			// 3G-A
			stTable1920p[VPIDPictureRate_5994] = NTV2_FORMAT_1080p_5994_A;			// 3G-A
			stTable1920p[VPIDPictureRate_6000] = NTV2_FORMAT_1080p_6000_A;			// 3G-A
		
			stTable2048psf[VPIDPictureRate_2398] = NTV2_FORMAT_1080psf_2K_2398;
			stTable2048psf[VPIDPictureRate_2400] = NTV2_FORMAT_1080psf_2K_2400;
			stTable2048psf[VPIDPictureRate_2500] = NTV2_FORMAT_1080psf_2K_2500;
	//		stTable2048psf[VPIDPictureRate_2997] = NTV2_FORMAT_1080psf_2K_2997;
	//		stTable2048psf[VPIDPictureRate_3000] = NTV2_FORMAT_1080psf_2K_3000;
			stTable2048psf[VPIDPictureRate_4795] = NTV2_FORMAT_1080p_2K_4795;		// 3G-B or Duallink 1.5G
			stTable2048psf[VPIDPictureRate_4800] = NTV2_FORMAT_1080p_2K_4800;		// 3G-B or Duallink 1.5G
			stTable2048psf[VPIDPictureRate_5000] = NTV2_FORMAT_1080p_2K_5000;		// 3G-B or Duallink 1.5G
			stTable2048psf[VPIDPictureRate_5994] = NTV2_FORMAT_1080p_2K_5994;		// 3G-B or Duallink 1.5G
			stTable2048psf[VPIDPictureRate_6000] = NTV2_FORMAT_1080p_2K_6000;		// 3G-B or Duallink 1.5G
		
			stTable1920psf[VPIDPictureRate_2398] = NTV2_FORMAT_1080psf_2398;
			stTable1920psf[VPIDPictureRate_2400] = NTV2_FORMAT_1080psf_2400;
			stTable1920psf[VPIDPictureRate_2500] = NTV2_FORMAT_1080psf_2500_2;
			stTable1920psf[VPIDPictureRate_2997] = NTV2_FORMAT_1080psf_2997_2;
			stTable1920psf[VPIDPictureRate_3000] = NTV2_FORMAT_1080psf_3000_2;
	//		stTable1920psf[VPIDPictureRate_4795] = NTV2_FORMAT_1080p_4795;			// 3G-B or Duallink 1.5G
	//		stTable1920psf[VPIDPictureRate_4800] = NTV2_FORMAT_1080p_4800;			// 3G-B or Duallink 1.5G
			stTable1920psf[VPIDPictureRate_5000] = NTV2_FORMAT_1080p_5000_B;		// 3G-B or Duallink 1.5G
			stTable1920psf[VPIDPictureRate_5994] = NTV2_FORMAT_1080p_5994_B;		// 3G-B or Duallink 1.5G
			stTable1920psf[VPIDPictureRate_6000] = NTV2_FORMAT_1080p_6000_B;		// 3G-B or Duallink 1.5G
		
	//		stTable2048i[VPIDPictureRate_2500] = NTV2_FORMAT_2048x1080i_2500;
	//		stTable2048i[VPIDPictureRate_2997] = NTV2_FORMAT_2048x1080i_2997;
	//		stTable2048i[VPIDPictureRate_3000] = NTV2_FORMAT_2048x1080i_3000;

			stTable1920i[VPIDPictureRate_2500] = NTV2_FORMAT_1080i_5000;
			stTable1920i[VPIDPictureRate_2997] = NTV2_FORMAT_1080i_5994;
			stTable1920i[VPIDPictureRate_3000] = NTV2_FORMAT_1080i_6000;

			stTable3840pSID[VPIDPictureRate_2398] = NTV2_FORMAT_4x1920x1080p_2398;
			stTable3840pSID[VPIDPictureRate_2400] = NTV2_FORMAT_4x1920x1080p_2400;	
			stTable3840pSID[VPIDPictureRate_2500] = NTV2_FORMAT_4x1920x1080p_2500;
			stTable3840pSID[VPIDPictureRate_2997] = NTV2_FORMAT_4x1920x1080p_2997;			
			stTable3840pSID[VPIDPictureRate_3000] = NTV2_FORMAT_4x1920x1080p_3000;			
			stTable3840pSID[VPIDPictureRate_5000] = NTV2_FORMAT_4x1920x1080p_5000;
			stTable3840pSID[VPIDPictureRate_5994] = NTV2_FORMAT_4x1920x1080p_5994;
			stTable3840pSID[VPIDPictureRate_6000] = NTV2_FORMAT_4x1920x1080p_6000;

			stTable4096pSID[VPIDPictureRate_2398] = NTV2_FORMAT_4x2048x1080p_2398;
			stTable4096pSID[VPIDPictureRate_2400] = NTV2_FORMAT_4x2048x1080p_2400;
			stTable4096pSID[VPIDPictureRate_2500] = NTV2_FORMAT_4x2048x1080p_2500;
			stTable4096pSID[VPIDPictureRate_2997] = NTV2_FORMAT_4x2048x1080p_2997;
			stTable4096pSID[VPIDPictureRate_3000] = NTV2_FORMAT_4x2048x1080p_3000;
			stTable4096pSID[VPIDPictureRate_5000] = NTV2_FORMAT_4x2048x1080p_5000;
			stTable4096pSID[VPIDPictureRate_5994] = NTV2_FORMAT_4x2048x1080p_5994;
			stTable4096pSID[VPIDPictureRate_6000] = NTV2_FORMAT_4x2048x1080p_6000;
			stTablesInitialized = true;
		}	//	constructor

};	//	VPIDTableInitializer


static VPIDTableInitializer	gVPIDTableInitializer;


CNTV2VPID::CNTV2VPID()
{
	assert (stTablesInitialized);
	m_uVPID = 0;
}


CNTV2VPID::CNTV2VPID (const CNTV2VPID & inVPID)
{
	assert (stTablesInitialized);
	m_uVPID = inVPID.m_uVPID;
}


CNTV2VPID & CNTV2VPID::operator = (const CNTV2VPID & inRHS)
{
	assert (stTablesInitialized);
	if (&inRHS != this)
		m_uVPID = inRHS.m_uVPID;
	return *this;
}


bool CNTV2VPID::SetVPID (NTV2VideoFormat videoFormat,
						NTV2FrameBufferFormat frameBufferFormat,
						bool progressive,
						bool aspect16x9,
						VPIDChannel channel)
{
	return SetVPIDData (m_uVPID, videoFormat, frameBufferFormat, progressive, aspect16x9, channel);
}


bool CNTV2VPID::SetVPID (NTV2VideoFormat videoFormat,
						bool dualLink,
						bool format48Bit,
						bool output3Gb,
						bool st425,
						VPIDChannel channel)
{
	return SetVPIDData (m_uVPID,  videoFormat, dualLink, format48Bit, output3Gb, st425, channel);
}


bool CNTV2VPID::IsStandard3Ga (void) const
{
	switch(GetStandard())
	{
	case VPIDStandard_720_3Ga:				//	Not to be used for aspect ratio. 720p is always zero.
	case VPIDStandard_1080_3Ga:
	case VPIDStandard_1080_Dual_3Ga:
	case VPIDStandard_2160_QuadLink_3Ga:
		return true;
	default:
		break;
	}

	return false;
}


bool CNTV2VPID::IsStandardTwoSampleInterleave (void) const
{
	switch(GetStandard())
	{
	case VPIDStandard_2160_DualLink:
	case VPIDStandard_2160_QuadLink_3Ga:
	case VPIDStandard_2160_QuadDualLink_3Gb:
		return true;
	default:
		break;
	}

	return false;
}

void CNTV2VPID::SetVersion (const VPIDVersion inVPIDVersion)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDVersionID) |
		(((ULWord)inVPIDVersion << kRegShiftVPIDVersionID) & kRegMaskVPIDVersionID);
}


VPIDVersion CNTV2VPID::GetVersion (void) const
{
	return (VPIDVersion)((m_uVPID & kRegMaskVPIDVersionID) >> kRegShiftVPIDVersionID); 
}


void CNTV2VPID::SetStandard (const VPIDStandard inStandard)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDStandard) |
		(((ULWord)inStandard << kRegShiftVPIDStandard) & kRegMaskVPIDStandard);
}


VPIDStandard CNTV2VPID::GetStandard (void) const
{
	return (VPIDStandard)((m_uVPID & kRegMaskVPIDStandard) >> kRegShiftVPIDStandard); 
}


void CNTV2VPID::SetProgressiveTransport (const bool inIsProgressiveTransport)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDProgressiveTransport) |
		(((inIsProgressiveTransport ? 1 : 0) << kRegShiftVPIDProgressiveTransport) & kRegMaskVPIDProgressiveTransport);
}


bool CNTV2VPID::GetProgressiveTransport (void) const
{
	 return (m_uVPID & kRegMaskVPIDProgressiveTransport) != 0; 
}


void CNTV2VPID::SetProgressivePicture (const bool inIsProgressivePicture)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDProgressivePicture) |
		(((inIsProgressivePicture ? 1 : 0) << kRegShiftVPIDProgressivePicture) & kRegMaskVPIDProgressivePicture);
}


bool CNTV2VPID::GetProgressivePicture (void) const
{
	return (m_uVPID & kRegMaskVPIDProgressivePicture) != 0; 
}


void CNTV2VPID::SetPictureRate (const VPIDPictureRate inPictureRate)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDPictureRate) |
		(((ULWord)inPictureRate << kRegShiftVPIDPictureRate) & kRegMaskVPIDPictureRate);
}

VPIDPictureRate CNTV2VPID::GetPictureRate (void) const
{
	return (VPIDPictureRate)((m_uVPID & kRegMaskVPIDPictureRate) >> kRegShiftVPIDPictureRate); 
}


void CNTV2VPID::SetImageAspect16x9 (const bool inIs16x9Aspect)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDImageAspect16x9) |
		(((inIs16x9Aspect ? 1 : 0) << kRegShiftVPIDImageAspect16x9) & kRegMaskVPIDImageAspect16x9);
}


bool CNTV2VPID::GetImageAspect16x9 (void) const
{
	return (m_uVPID & kRegMaskVPIDImageAspect16x9) != 0; 
}


void CNTV2VPID::SetSampling (const VPIDSampling inSampling)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDSampling) |
		(((ULWord)inSampling << kRegShiftVPIDSampling) & kRegMaskVPIDSampling);
}


VPIDSampling CNTV2VPID::GetSampling (void) const
{
	return (VPIDSampling)((m_uVPID & kRegMaskVPIDSampling) >> kRegShiftVPIDSampling); 
}


void CNTV2VPID::SetChannel (const VPIDChannel inChannel)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDChannel) |
		(((ULWord)inChannel << kRegShiftVPIDChannel) & kRegMaskVPIDChannel);
}


VPIDChannel CNTV2VPID::GetChannel (void) const
{
	return (VPIDChannel)((m_uVPID & kRegMaskVPIDChannel) >> kRegShiftVPIDChannel); 
}


void CNTV2VPID::SetDualLinkChannel (const VPIDChannel inChannel)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDDualLinkChannel) |
		(((ULWord)inChannel << kRegShiftVPIDDualLinkChannel) & kRegMaskVPIDDualLinkChannel);
}


VPIDChannel CNTV2VPID::GetDualLinkChannel (void) const
{
	return (VPIDChannel)((m_uVPID & kRegMaskVPIDDualLinkChannel) >> kRegShiftVPIDDualLinkChannel);
}


void CNTV2VPID::SetDynamicRange (const VPIDDynamicRange inDynamicRange)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDDynamicRange) |
		(((ULWord)inDynamicRange << kRegShiftVPIDDynamicRange) & kRegMaskVPIDDynamicRange);
}


VPIDDynamicRange CNTV2VPID::GetDynamicRange (void) const
{
	return (VPIDDynamicRange)((m_uVPID & kRegMaskVPIDDynamicRange) >> kRegShiftVPIDDynamicRange); 
}


void CNTV2VPID::SetBitDepth (const VPIDBitDepth inBitDepth)
{
	m_uVPID = (m_uVPID & ~kRegMaskVPIDBitDepth) |
		(((ULWord)inBitDepth << kRegShiftVPIDBitDepth) & kRegMaskVPIDBitDepth);
}


VPIDBitDepth CNTV2VPID::GetBitDepth (void) const
{
	return (VPIDBitDepth)((m_uVPID & kRegMaskVPIDBitDepth) >> kRegShiftVPIDBitDepth); 
}


//	static - this one doesn't support 3Gb
bool CNTV2VPID::SetVPIDData (ULWord &					outVPID,
							const NTV2VideoFormat		inOutputFormat,
							const NTV2FrameBufferFormat	inFrameBufferFormat,
							const bool					inIsProgressive,
							const bool					inIs16x9Aspect,
							const VPIDChannel			inChannel,
							const bool					inUseChannel)
{
	(void)inIsProgressive;
	(void)inIs16x9Aspect;
	
	bool		bRGB48Bit		(false);
	bool		bDualLinkRGB	(false);
	const bool	kOutput3Gb		(false);
	
	switch(inFrameBufferFormat)
	{
	case NTV2_FBF_48BIT_RGB:
		bRGB48Bit = true;
		bDualLinkRGB = true;			// RGB 12 bit
		break;
	case NTV2_FBF_ARGB:
	case NTV2_FBF_RGBA:
	case NTV2_FBF_ABGR:
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_10BIT_DPX:
	case NTV2_FBF_24BIT_RGB:
	case NTV2_FBF_24BIT_BGR:
	case NTV2_FBF_10BIT_DPX_LITTLEENDIAN:
	case NTV2_FBF_10BIT_RGB_PACKED:
	case NTV2_FBF_10BIT_ARGB:
	case NTV2_FBF_16BIT_ARGB:
		bDualLinkRGB = true;			// RGB 8+10 bit;
		bRGB48Bit = false;
		break;
	default:
		bDualLinkRGB = false;			// All other
		bRGB48Bit = false;
		break;
	}
	
	// should use this call directly
	return SetVPIDData (outVPID,
						inOutputFormat,
						bDualLinkRGB,
						bRGB48Bit,
						kOutput3Gb,
						false,			// not ST525
						inChannel,
						inUseChannel);
}


bool CNTV2VPID::SetVPIDData (ULWord &				outVPID,
							const NTV2VideoFormat	inOutputFormat,
							const bool				inIsDualLinkRGB,
							const bool				inIsRGB48Bit,
							const bool				inOutputIs3Gb,
							const bool				inIsSMPTE425,
							const VPIDChannel		inChannel,
							const bool				inUseChannel)
{
	VPIDSpec vpidSpec;


	::memset ((void *)&vpidSpec, 0, sizeof (vpidSpec));

	vpidSpec.videoFormat			= inOutputFormat;
	vpidSpec.pixelFormat			= inIsRGB48Bit ? NTV2_FBF_48BIT_RGB : NTV2_FBF_INVALID;
	vpidSpec.isRGBOnWire			= inIsDualLinkRGB;
	vpidSpec.isOutputLevelA			= (NTV2_IS_3G_FORMAT (inOutputFormat) && ! inOutputIs3Gb) ? true : false;
	vpidSpec.isOutputLevelB			= inOutputIs3Gb;
	vpidSpec.isDualLink				= inIsDualLinkRGB || (NTV2_IS_372_DUALLINK_FORMAT(inOutputFormat) && !vpidSpec.isOutputLevelA);
	vpidSpec.isTwoSampleInterleave	= inIsSMPTE425;
	vpidSpec.useChannel				= inUseChannel;
	vpidSpec.vpidChannel			= inChannel;
	vpidSpec.isStereo				= false;
	vpidSpec.isRightEye				= false;
	vpidSpec.audioCarriage			= VPIDAudio_Unknown;

	return ::SetVPIDFromSpec (&outVPID, &vpidSpec);
}


NTV2VideoFormat CNTV2VPID::GetVideoFormat (void) const
{
	NTV2VideoFormat  videoFormat = NTV2_FORMAT_UNKNOWN;
	VPIDStandard vpidStandard	 = GetStandard();
	bool vpidProgPicture         = GetProgressivePicture();
	bool vpidProgTransport       = GetProgressiveTransport();
	bool vpidHorizontal2048      = ((m_uVPID & kRegMaskVPIDHorizontalSampling) != 0);

	VPIDPictureRate vpidFrameRate = GetPictureRate();

	switch (vpidStandard)
	{
	case VPIDStandard_483_576:							
		switch (vpidFrameRate)
		{
		case VPIDPictureRate_2500:
			videoFormat = NTV2_FORMAT_625_5000;
			break;
		case VPIDPictureRate_2997:
			videoFormat = NTV2_FORMAT_525_5994;
			break;
		default:
			break;
		}
		break;
	case VPIDStandard_720:
		switch (vpidFrameRate)
		{
		case VPIDPictureRate_5000:
			videoFormat = NTV2_FORMAT_720p_5000;
			break;
		case VPIDPictureRate_5994:
			videoFormat = NTV2_FORMAT_720p_5994;
			break;
		case VPIDPictureRate_6000:
			videoFormat = NTV2_FORMAT_720p_6000;
			break;
		default:
			break;;
		}
		break;
	case VPIDStandard_720_3Gb:
		videoFormat = stTable720p[vpidFrameRate];
		break;
	case VPIDStandard_1080:
	case VPIDStandard_1080_DualLink:
	case VPIDStandard_1080_3Ga:
	case VPIDStandard_1080_3Gb:
	case VPIDStandard_1080_DualLink_3Gb:
		if (vpidProgPicture)
		{
			if (vpidProgTransport)
			{
				if (vpidHorizontal2048)
				{
					videoFormat = stTable2048p[vpidFrameRate];
				}
				else
				{
					videoFormat = stTable1920p[vpidFrameRate];
				}
			}
			else
			{
				if (vpidHorizontal2048)
				{
					videoFormat = stTable2048psf[vpidFrameRate];
				}
				else
				{
					videoFormat = stTable1920psf[vpidFrameRate];
				}
			}
		}
		else
		{
			if (vpidHorizontal2048)
			{
				videoFormat = stTable2048i[vpidFrameRate];
			}
			else
			{
				videoFormat = stTable1920i[vpidFrameRate];
			}
		}
		break;
	case VPIDStandard_2160_DualLink:
	case VPIDStandard_2160_QuadLink_3Ga:
	case VPIDStandard_2160_QuadDualLink_3Gb:
		if (vpidHorizontal2048)
		{
			videoFormat = stTable4096pSID[vpidFrameRate];
		}
		else
		{
			videoFormat = stTable3840pSID[vpidFrameRate];
		}
		break;
	default:
		break;
	}

	return videoFormat;
}
