/**
	@file		ntv2vidproc.cpp
	@brief		Declares the CNTV2VidProc class.
	@copyright	(C) 2004-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2vidproc.h"
#include "ntv2vidprocmasks.h"
#include "ntv2transcode.h"
#include "fixed.h"
#include <assert.h>
#if defined (NTV2_DEPRECATE)
	#define	CNTV2VIDPROCCLASS	CNTV2Card
#else
	#define	CNTV2VIDPROCCLASS	CNTV2VidProc


	CNTV2VidProc::CNTV2VidProc(UWord boardNumber,bool displayErrorMessage,UWord ulBoardType )
	:   CNTV2TestPattern(boardNumber, displayErrorMessage,  ulBoardType)
	{
	}

	CNTV2VidProc::CNTV2VidProc (UWord		inDeviceIndex,
								bool		inDisplayErrors,
								UWord		inDeviceTypes,
								bool		inAutoRoute,
								const char	pInHostName[])
		:	CNTV2TestPattern (inDeviceIndex, inDisplayErrors, inDeviceTypes, inAutoRoute, pInHostName)
	{
	}


	CNTV2VidProc::~CNTV2VidProc()
	{
		// board closed in CNTV2Card destructor
	}

#endif	//	!defined (NTV2_DEPRECATE)


void CNTV2VIDPROCCLASS::SetupDefaultVidProc()
{
	// Setup for Horizontal Split between input 1 and channel 1
	SetForegroundVideoCrosspoint(NTV2CROSSPOINT_CHANNEL1);
	SetForegroundKeyCrosspoint(NTV2CROSSPOINT_CHANNEL1);
	SetBackgroundVideoCrosspoint(NTV2CROSSPOINT_INPUT1);
	SetBackgroundKeyCrosspoint(NTV2CROSSPOINT_INPUT1);
	SetCh1VidProcMode(NTV2VIDPROCMODE_SPLIT);
	SetSplitParameters(FIXED_ONE/2,0);
//	SetCh2OutputMode(NTV2Ch2OUTPUTMODE_BGV);  // leave ch2Output Mode
	SetMixCoefficient(FIXED_ONE);
}

void CNTV2VIDPROCCLASS::DisableVidProc()
{
	// Channel 1 to go out Output 1 and 
	// Channel 2 to go out Output 2.
	SetForegroundVideoCrosspoint(NTV2CROSSPOINT_CHANNEL1);
	SetForegroundKeyCrosspoint(NTV2CROSSPOINT_CHANNEL1);
	SetBackgroundVideoCrosspoint(NTV2CROSSPOINT_CHANNEL2);
	SetBackgroundKeyCrosspoint(NTV2CROSSPOINT_CHANNEL2);
	SetCh1VidProcMode(NTV2VIDPROCMODE_MIX);
	SetMixCoefficient(FIXED_ONE);
}

void CNTV2VIDPROCCLASS::SetForegroundVideoCrosspoint (NTV2Crosspoint crosspoint)
{
	ULWord regValue;

	ReadVideoProcessingControlCrosspoint (&regValue);
	regValue &= ~(FGVCROSSPOINTMASK);
	regValue |= (crosspoint<<FGVCROSSPOINTSHIFT);
	WriteVideoProcessingControlCrosspoint(regValue);
}

void CNTV2VIDPROCCLASS::SetForegroundKeyCrosspoint(NTV2Crosspoint crosspoint)
{
	ULWord regValue;

	ReadVideoProcessingControlCrosspoint(&regValue);
	regValue &= ~(FGKCROSSPOINTMASK);
	regValue |= (crosspoint<<FGKCROSSPOINTSHIFT);
	WriteVideoProcessingControlCrosspoint(regValue);

}

void CNTV2VIDPROCCLASS::SetBackgroundVideoCrosspoint(NTV2Crosspoint crosspoint)
{
	ULWord regValue;

	ReadVideoProcessingControlCrosspoint(&regValue);
	regValue &= ~(BGVCROSSPOINTMASK);
	regValue |= (crosspoint<<BGVCROSSPOINTSHIFT);
	WriteVideoProcessingControlCrosspoint(regValue);

}

void CNTV2VIDPROCCLASS::SetBackgroundKeyCrosspoint(NTV2Crosspoint crosspoint)
{
	ULWord regValue;

	ReadVideoProcessingControlCrosspoint(&regValue);
	regValue &= ~(BGKCROSSPOINTMASK);
	regValue |= (crosspoint<<BGKCROSSPOINTSHIFT);
	WriteVideoProcessingControlCrosspoint(regValue);

}
//
NTV2Crosspoint CNTV2VIDPROCCLASS::GetForegroundVideoCrosspoint()
{
	ULWord regValue;
	
	ReadVideoProcessingControlCrosspoint(&regValue);
	regValue &= (FGVCROSSPOINTMASK);
	regValue >>= (FGVCROSSPOINTSHIFT);
	
	return static_cast<NTV2Crosspoint>(regValue);
}

NTV2Crosspoint CNTV2VIDPROCCLASS::GetForegroundKeyCrosspoint()
{
	ULWord regValue;

	ReadVideoProcessingControlCrosspoint(&regValue);
	regValue &= (FGKCROSSPOINTMASK);
	regValue >>= (FGKCROSSPOINTSHIFT);
	return static_cast<NTV2Crosspoint>(regValue);

}

NTV2Crosspoint CNTV2VIDPROCCLASS::GetBackgroundVideoCrosspoint()
{
	ULWord regValue;

	ReadVideoProcessingControlCrosspoint(&regValue);
	regValue &= (BGVCROSSPOINTMASK);
	regValue >>= (BGVCROSSPOINTSHIFT);
	return static_cast<NTV2Crosspoint>(regValue);

}

NTV2Crosspoint CNTV2VIDPROCCLASS::GetBackgroundKeyCrosspoint()
{
	ULWord regValue;

	ReadVideoProcessingControlCrosspoint(&regValue);
	regValue &= (BGKCROSSPOINTMASK);
	regValue >>= (BGKCROSSPOINTSHIFT);
	return static_cast<NTV2Crosspoint>(regValue);

}
//
void CNTV2VIDPROCCLASS::SetCh1VidProcMode(NTV2Ch1VidProcMode vidProcMode)
{
	ULWord regValue;
	
	ReadVideoProcessingControl(&regValue);
	regValue &= ~(VIDPROCMUX1MASK + VIDPROCMUX2MASK + VIDPROCMUX3MASK);
	
	switch (vidProcMode)
	{
		case NTV2VIDPROCMODE_MIX:		regValue |= (BIT_0+BIT_2);		break;
		case NTV2VIDPROCMODE_SPLIT:		regValue |= (BIT_0+BIT_3);		break;
		case NTV2VIDPROCMODE_KEY:		regValue |= (BIT_0);			break;
		case NTV2VIDPROCMODE_INVALID:									return;
	}

	WriteVideoProcessingControl(regValue);
}

NTV2Ch1VidProcMode CNTV2VIDPROCCLASS::GetCh1VidProcMode()
{
	ULWord				regValue		(0);
	NTV2Ch1VidProcMode	ch1VidProcMode	(NTV2VIDPROCMODE_INVALID);

	ReadVideoProcessingControl (&regValue);
	regValue &= ( VIDPROCMUX2MASK );
	regValue >>= (VIDPROCMUX2SHIFT);
	
	if(regValue == 0)
		ch1VidProcMode = NTV2VIDPROCMODE_KEY;
	else if(regValue == 1)
		ch1VidProcMode = NTV2VIDPROCMODE_MIX;
	else if(regValue == 2)
		ch1VidProcMode = NTV2VIDPROCMODE_SPLIT;
	else
		NTV2_ASSERT(false);

	return ch1VidProcMode;
}

void CNTV2VIDPROCCLASS::SetCh2OutputMode(NTV2Ch2OutputMode outputMode)
{
	ULWord	regValue;
	ReadVideoProcessingControl(&regValue);
	regValue &= ~(VIDPROCMUX5MASK);

	switch (outputMode)
	{
		case NTV2Ch2OUTPUTMODE_BGV:									break;
		case NTV2Ch2OUTPUTMODE_FGV:			regValue |= (BIT_8);	break;
		case NTV2Ch2OUTPUTMODE_MIXEDKEY:	regValue |= (BIT_9);	break;
		case NTV2Ch2OUTPUTMODE_INVALID:								return;
	}
	
	WriteVideoProcessingControl(regValue);
}

NTV2Ch2OutputMode CNTV2VIDPROCCLASS::GetCh2OutputMode()
{
	ULWord regValue;

	ReadVideoProcessingControl(&regValue);
	regValue &= (VIDPROCMUX5MASK);
	regValue >>= (VIDPROCMUX5SHIFT);

	return static_cast<NTV2Ch2OutputMode>(regValue);
	
}

void CNTV2VIDPROCCLASS::SetSplitMode(NTV2SplitMode splitMode)
{
	ULWord regValue;

	ReadSplitControl(&regValue);
	regValue &= ~(SPLITMODEMASK);
	regValue |= (splitMode<<SPLITMODESHIFT);
	WriteSplitControl(regValue);
}

NTV2SplitMode CNTV2VIDPROCCLASS::GetSplitMode()
{
	ULWord regValue;

	ReadSplitControl(&regValue);
	regValue &= (SPLITMODEMASK);

	return static_cast<NTV2SplitMode>((regValue>>SPLITMODESHIFT)&0x3);

}


void CNTV2VIDPROCCLASS::SetSplitParameters(Fixed_ position, Fixed_ softness)
{
	ULWord max = 0, offset = 0;
	NTV2FrameDimensions	frameBufferSize	(GetActiveFrameDimensions ());

	switch ( GetSplitMode() )
	{
	case NTV2SPLITMODE_HORZSPLIT:
        {
            switch ( frameBufferSize.Width())
            {
            case HD_NUMCOMPONENTPIXELS_1080:
            case HD_NUMCOMPONENTPIXELS_720:
		        max = frameBufferSize.Width();
    		    offset = 8;
                break;
            case NUMCOMPONENTPIXELS:
 		        max = frameBufferSize.Width()*2;
		        offset = 8;
                break;
            }
        }
        break;
	case NTV2SPLITMODE_VERTSPLIT:
        {
            switch ( frameBufferSize.Height())
            {
            case HD_NUMACTIVELINES_1080:
			    offset = 19;
		        max = frameBufferSize.Height()+1;
            case HD_NUMACTIVELINES_720:
			    offset = 7;
		        max = frameBufferSize.Height()+1;
                break;
            case NUMACTIVELINES_525:
		        max = (frameBufferSize.Height()/2)+1;
		        offset = 8;
                break;
            case NUMACTIVELINES_625:
		        max = (frameBufferSize.Height()/2)+1;
		        offset = 8;
                break;
            }
        }
        break;
	default:
		return;
	}

	ULWord regValue;

	ReadSplitControl(&regValue);
	regValue &= (SPLITMODEMASK);

	ULWord positionValue = FixedMix(0,max,position);
	ULWord softnessPixels = 0x1FFF;
	ULWord softnessSlope = 0x1FFF;
	if ( softness == 0 )
	{
		softnessSlope = 0x1FFF;
		softnessPixels = 1;
	}
	else
	{
		// need to tame softness to based on position.
		// 1st find out what the maximum softness is
		ULWord maxSoftness;
		if ( positionValue > max/2 )
			maxSoftness = max - positionValue;
		else
			maxSoftness = positionValue;

		// softness is limited to 1/4 of max
		if ( maxSoftness > max/4 )
		{
			maxSoftness = max/4;
		}

		if ( maxSoftness == 0 )
		{
			softnessPixels = 1;
		}
		else
		{
			softnessPixels = FixedMix(1,maxSoftness,softness);

		}


	}
	if ( softnessPixels == 0 )
		softnessPixels = 1; // shouldn't happen but....
	softnessSlope = 0x1FFF/softnessPixels;
	positionValue -= (softnessPixels/2);

	WriteSplitControl(regValue | (softnessSlope<<16) | ((positionValue+offset)<<2));

}

void CNTV2VIDPROCCLASS::SetSlitParameters(Fixed_ start, Fixed_ width)
{
	LWord max = 0, maxWidth = 0, offset = 0;
	NTV2FrameDimensions	frameBufferSize	(GetActiveFrameDimensions ());
	if (!frameBufferSize.IsValid ())
		return;

	switch ( GetSplitMode() )
	{
	case NTV2SPLITMODE_HORZSLIT:
        {
            switch ( frameBufferSize.Width())
            {
            case HD_NUMCOMPONENTPIXELS_1080:
            case HD_NUMCOMPONENTPIXELS_720:
		        max = frameBufferSize.Width();
    		    offset = 8;
                break;
            case NUMCOMPONENTPIXELS:
 		        max = frameBufferSize.Width()*2;
		        offset = 8;
                break;
            }
        }
		maxWidth = max;
		break;
	case NTV2SPLITMODE_VERTSLIT:
        {
            switch ( frameBufferSize.Height())
            {
            case HD_NUMACTIVELINES_1080:
			    offset = 19;
		        max = frameBufferSize.Height()+1;
			    maxWidth = max/2;
            case HD_NUMACTIVELINES_720:
			    offset = 7;
		        max = frameBufferSize.Height()+1;
			    maxWidth = max;
                break;
            case NUMACTIVELINES_525:
		        offset = 8;
		        max = (frameBufferSize.Height()/2)+1;
			    maxWidth = max;
                break;
            case NUMACTIVELINES_625:
		        offset = 8;
		        max = (frameBufferSize.Height()/2)+1;
			    maxWidth = max;
                break;
            }
        }
		break;
	default:
		return;
	}

	ULWord regValue;

	ReadSplitControl(&regValue);
	regValue &= (SPLITMODEMASK);

	ULWord positionValue = FixedMix(0,max,start);
	ULWord widthPixels = FixedMix(1,maxWidth,width);
	ULWord widthSlope;

	if ( widthPixels == 0 )
		widthPixels = 1; // shouldn't happen but....

	if ( width == 0 )
		widthSlope = 0x1FFF;
	else
		widthSlope = 0x1FFF/widthPixels;

	WriteSplitControl(regValue | (widthSlope<<16) | ((positionValue+offset)<<2));

}

void CNTV2VIDPROCCLASS::SetMixCoefficient(Fixed_ coefficient)
{
	WriteMixerCoefficient(coefficient);
}

Fixed_ CNTV2VIDPROCCLASS::GetMixCoefficient()
{
	ULWord coefficient;
	ReadMixerCoefficient(&coefficient);

	return static_cast<Fixed_>(coefficient);
}

void CNTV2VIDPROCCLASS::SetMatteColor(YCbCr10BitPixel ycbcrPixel)
{
	ULWord packedValue;

	// clip y
	if ( ycbcrPixel.y < 0x40 ) 
		ycbcrPixel.y = 0x0;
	else
		ycbcrPixel.y -= 0x40;


	// pack it
	packedValue = ycbcrPixel.cb | (ycbcrPixel.y<<10) | (ycbcrPixel.cr<<20);

	WriteFlatMatteValue(packedValue);
}

#ifdef MSWindows
	void CNTV2VIDPROCCLASS::SetMatteColor(COLORREF rgbColor)
	{

		RGBAlphaPixel RGBMatte;
		RGBMatte.Red = GetRValue(rgbColor);
		RGBMatte.Green = GetGValue(rgbColor);
		RGBMatte.Blue = GetBValue(rgbColor);
		YCbCr10BitPixel YCbCrMatte;
		// BUGBUG: This is wrong for XenaHS and Kona2, Xena2 in SD Mode.
		// BOARDTYPE_KHD, BOARDTYPE_HDNTV
		HDConvertRGBAlphatoYCbCr(&RGBMatte, &YCbCrMatte);
		SetMatteColor(YCbCrMatte);
	}
#endif	//	MSWindows

#ifdef AJALinux
	void CNTV2VIDPROCCLASS::SetMatteColor(AJARgb rgbColor)
	{

		RGBAlphaPixel RGBMatte;
		RGBMatte.Red = ajaRed(rgbColor);
		RGBMatte.Green = ajaGreen(rgbColor);
		RGBMatte.Blue = ajaBlue(rgbColor);
		YCbCr10BitPixel YCbCrMatte;
		// BUGBUG: This is wrong for XenaHS and Kona2, Xena2 in SD Mode.
		// BOARDTYPE_KHD, BOARDTYPE_HDNTV, BOARDTYPE_XENA2
		HDConvertRGBAlphatoYCbCr(&RGBMatte, &YCbCrMatte);
		SetMatteColor(YCbCrMatte);
	}
#endif	//	AJALinux
