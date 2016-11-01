/**
	@file		xena2vidproc.cpp
	@brief		Implements the CXena2VidProc class.
	@copyright	(C) 2004-2016 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "xena2vidproc.h"
#include "ntv2vidprocmasks.h"
#include "ntv2transcode.h"
#include "fixed.h"


#if	!defined (NTV2_DEPRECATE)

CXena2VidProc::CXena2VidProc(UWord boardNumber,bool displayErrorMessage,UWord ulBoardType )
:   CNTV2TestPattern(boardNumber, displayErrorMessage,  ulBoardType) 
{
}

CXena2VidProc::~CXena2VidProc()
{
	// board closed in CNTV2Card desctructor
}




/// Use the follow base class memebers to set inputs to xena2 video processing widget
//	bool SetK2Xpt4MixerBGKeyInputSelect(NTV2K2CrosspointSelections value);
//	bool GetK2Xpt4MixerBGKeyInputSelect(NTV2K2CrosspointSelections* value);
//	bool SetK2Xpt4MixerBGVidInputSelect(NTV2K2CrosspointSelections value);
//	bool GetK2Xpt4MixerBGVidInputSelect(NTV2K2CrosspointSelections* value);
//	bool SetK2Xpt4MixerFGKeyInputSelect(NTV2K2CrosspointSelections value);
//	bool GetK2Xpt4MixerFGKeyInputSelect(NTV2K2CrosspointSelections* value);
//	bool SetK2Xpt4MixerFGVidInputSelect(NTV2K2CrosspointSelections value);
//	bool GetK2Xpt4MixerFGVidInputSelect(NTV2K2CrosspointSelections* value);


bool CXena2VidProc::SetXena2VidProcInputControl(NTV2Channel channel,NTV2MixerKeyerInputControl inputControl)
{
	if (channel == NTV2_CHANNEL1)
		return WriteRegister (kRegVidProc1Control,	inputControl,	kK2RegMaskXena2FgVidProcInputControl,	kK2RegShiftXena2FgVidProcInputControl);
	else
		return WriteRegister (kRegVidProc1Control,	inputControl,	kK2RegMaskXena2BgVidProcInputControl,	kK2RegShiftXena2BgVidProcInputControl);
}

bool CXena2VidProc::SetXena2VidProc2InputControl(NTV2Channel channel,NTV2MixerKeyerInputControl inputControl)
{
	if (channel == NTV2_CHANNEL1)
		return WriteRegister (kRegVidProc2Control,	inputControl,	kK2RegMaskXena2FgVidProcInputControl,	kK2RegShiftXena2FgVidProcInputControl);
	else
		return WriteRegister (kRegVidProc2Control,	inputControl,	kK2RegMaskXena2BgVidProcInputControl,	kK2RegShiftXena2BgVidProcInputControl);
}

bool CXena2VidProc::GetXena2VidProcInputControl(NTV2Channel channel, NTV2MixerKeyerInputControl* inputControl)
{
	if (channel == NTV2_CHANNEL1)
		return ReadRegister (kRegVidProc1Control,	(ULWord*)inputControl,	kK2RegMaskXena2FgVidProcInputControl,	kK2RegShiftXena2FgVidProcInputControl);
	else
		return ReadRegister (kRegVidProc1Control,	(ULWord*)inputControl,	kK2RegMaskXena2BgVidProcInputControl,	kK2RegShiftXena2BgVidProcInputControl);
}

bool CXena2VidProc::GetXena2VidProc2InputControl(NTV2Channel channel, NTV2MixerKeyerInputControl* inputControl)
{
	if (channel == NTV2_CHANNEL1)
		return ReadRegister (kRegVidProc2Control,	(ULWord*)inputControl,	kK2RegMaskXena2FgVidProcInputControl,	kK2RegShiftXena2FgVidProcInputControl);
	else
		return ReadRegister (kRegVidProc2Control,	(ULWord*)inputControl,	kK2RegMaskXena2BgVidProcInputControl,	kK2RegShiftXena2BgVidProcInputControl);
}

bool CXena2VidProc::SetXena2VidProc3InputControl(NTV2Channel channel,NTV2MixerKeyerInputControl inputControl)
{
	if (channel == NTV2_CHANNEL1)
		return WriteRegister (kRegVidProc3Control,	inputControl,	kK2RegMaskXena2FgVidProcInputControl,	kK2RegShiftXena2FgVidProcInputControl);
	else
		return WriteRegister (kRegVidProc3Control,	inputControl,	kK2RegMaskXena2BgVidProcInputControl,	kK2RegShiftXena2BgVidProcInputControl);
}

bool CXena2VidProc::GetXena2VidProc3InputControl(NTV2Channel channel, NTV2MixerKeyerInputControl* inputControl)
{
	if (channel == NTV2_CHANNEL1)
		return ReadRegister (kRegVidProc3Control,	(ULWord*)inputControl,	kK2RegMaskXena2FgVidProcInputControl,	kK2RegShiftXena2FgVidProcInputControl);
	else
		return ReadRegister (kRegVidProc3Control,	(ULWord*)inputControl,	kK2RegMaskXena2BgVidProcInputControl,	kK2RegShiftXena2BgVidProcInputControl);
}

bool CXena2VidProc::SetXena2VidProc4InputControl(NTV2Channel channel,NTV2MixerKeyerInputControl inputControl)
{
	if (channel == NTV2_CHANNEL1)
		return WriteRegister (kRegVidProc4Control,	inputControl,	kK2RegMaskXena2FgVidProcInputControl,	kK2RegShiftXena2FgVidProcInputControl);
	else
		return WriteRegister (kRegVidProc4Control,	inputControl,	kK2RegMaskXena2BgVidProcInputControl,	kK2RegShiftXena2BgVidProcInputControl);
}

bool CXena2VidProc::GetXena2VidProc4InputControl(NTV2Channel channel, NTV2MixerKeyerInputControl* inputControl)
{
	if (channel == NTV2_CHANNEL1)
		return ReadRegister (kRegVidProc4Control,	(ULWord*)inputControl,	kK2RegMaskXena2FgVidProcInputControl,	kK2RegShiftXena2FgVidProcInputControl);
	else
		return ReadRegister (kRegVidProc4Control,	(ULWord*)inputControl,	kK2RegMaskXena2BgVidProcInputControl,	kK2RegShiftXena2BgVidProcInputControl);
}

bool CXena2VidProc::SetXena2VidProcMode(NTV2MixerKeyerMode mode)	{return WriteRegister (kRegVidProc1Control,	mode,	kK2RegMaskXena2VidProcMode,	kK2RegShiftXena2VidProcMode);}
bool CXena2VidProc::SetXena2VidProc2Mode(NTV2MixerKeyerMode mode)	{return WriteRegister (kRegVidProc2Control,	mode,	kK2RegMaskXena2VidProcMode,	kK2RegShiftXena2VidProcMode);}
bool CXena2VidProc::SetXena2VidProc3Mode(NTV2MixerKeyerMode mode)	{return WriteRegister (kRegVidProc3Control,	mode,	kK2RegMaskXena2VidProcMode,	kK2RegShiftXena2VidProcMode);}
bool CXena2VidProc::SetXena2VidProc4Mode(NTV2MixerKeyerMode mode)	{return WriteRegister (kRegVidProc4Control,	mode,	kK2RegMaskXena2VidProcMode,	kK2RegShiftXena2VidProcMode);}

bool CXena2VidProc::GetXena2VidProcMode(NTV2MixerKeyerMode *mode)	{return ReadRegister (kRegVidProc1Control,	(ULWord*)mode,	kK2RegMaskXena2VidProcMode,	kK2RegShiftXena2VidProcMode);}
bool CXena2VidProc::GetXena2VidProc2Mode(NTV2MixerKeyerMode *mode){return ReadRegister (kRegVidProc2Control,	(ULWord*)mode,	kK2RegMaskXena2VidProcMode,	kK2RegShiftXena2VidProcMode);}
bool CXena2VidProc::GetXena2VidProc3Mode(NTV2MixerKeyerMode *mode){return ReadRegister (kRegVidProc3Control,	(ULWord*)mode,	kK2RegMaskXena2VidProcMode,	kK2RegShiftXena2VidProcMode);}
bool CXena2VidProc::GetXena2VidProc4Mode(NTV2MixerKeyerMode *mode){return ReadRegister (kRegVidProc4Control,	(ULWord*)mode,	kK2RegMaskXena2VidProcMode,	kK2RegShiftXena2VidProcMode);}

void CXena2VidProc::SetSplitMode(NTV2SplitMode splitMode)
{
	ULWord regValue;

	ReadSplitControl(&regValue);
	regValue &= ~(SPLITMODEMASK);
	regValue |= (splitMode<<SPLITMODESHIFT);
	WriteSplitControl(regValue);
}

NTV2SplitMode CXena2VidProc::GetSplitMode()
{
	ULWord regValue;

	ReadSplitControl(&regValue);
	regValue &= (SPLITMODEMASK);

	return static_cast<NTV2SplitMode>((regValue>>SPLITMODESHIFT)&0x3);

}

void CXena2VidProc::SetSplitParameters(Fixed_ position, Fixed_ softness)
{
	ULWord max = 0, offset = 0;
	SIZE frameBufferSize;
	GetActiveFramebufferSize(&frameBufferSize);

	switch ( GetSplitMode() )
	{
	case NTV2SPLITMODE_HORZSPLIT:
        {
            switch ( frameBufferSize.cx)
            {
            case HD_NUMCOMPONENTPIXELS_1080:
            case HD_NUMCOMPONENTPIXELS_720:
		        max = frameBufferSize.cx;
    		    offset = 8;
                break;
            case NUMCOMPONENTPIXELS:
 		        max = frameBufferSize.cx*2;
		        offset = 8;
                break;
            }
        }
        break;
	case NTV2SPLITMODE_VERTSPLIT:
        {
            switch ( frameBufferSize.cy)
            {
            case HD_NUMACTIVELINES_1080:
			    offset = 19;
		        max = frameBufferSize.cy+1;
            case HD_NUMACTIVELINES_720:
			    offset = 7;
		        max = frameBufferSize.cy+1;
                break;
            case NUMACTIVELINES_525:
		        max = (frameBufferSize.cy/2)+1;
		        offset = 8;
                break;
            case NUMACTIVELINES_625:
		        max = (frameBufferSize.cy/2)+1;
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

void CXena2VidProc::SetSlitParameters(Fixed_ start, Fixed_ width)
{
	LWord max = 0, maxWidth = 0, offset = 0;
	SIZE frameBufferSize;
	GetActiveFramebufferSize(&frameBufferSize);

	switch ( GetSplitMode() )
	{
	case NTV2SPLITMODE_HORZSLIT:
        {
            switch ( frameBufferSize.cx)
            {
            case HD_NUMCOMPONENTPIXELS_1080:
            case HD_NUMCOMPONENTPIXELS_720:
		        max = frameBufferSize.cx;
    		    offset = 8;
                break;
            case NUMCOMPONENTPIXELS:
 		        max = frameBufferSize.cx*2;
		        offset = 8;
                break;
            }
        }
		maxWidth = max;
		break;
	case NTV2SPLITMODE_VERTSLIT:
        {
            switch ( frameBufferSize.cy)
            {
            case HD_NUMACTIVELINES_1080:
			    offset = 19;
		        max = frameBufferSize.cy+1;
			    maxWidth = max/2;
            case HD_NUMACTIVELINES_720:
			    offset = 7;
		        max = frameBufferSize.cy+1;
			    maxWidth = max;
                break;
            case NUMACTIVELINES_525:
		        offset = 8;
		        max = (frameBufferSize.cy/2)+1;
			    maxWidth = max;
                break;
            case NUMACTIVELINES_625:
		        offset = 8;
		        max = (frameBufferSize.cy/2)+1;
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

void CXena2VidProc::SetMixCoefficient(Fixed_ coefficient)
{
	WriteMixerCoefficient(coefficient);
}

void CXena2VidProc::SetMix2Coefficient(Fixed_ coefficient)
{
	WriteMixer2Coefficient(coefficient);
}

Fixed_ CXena2VidProc::GetMixCoefficient()
{
	ULWord coefficient;
	ReadMixerCoefficient(&coefficient);

	return static_cast<Fixed_>(coefficient);
}

Fixed_ CXena2VidProc::GetMix2Coefficient()
{
	ULWord coefficient;
	ReadMixer2Coefficient(&coefficient);

	return static_cast<Fixed_>(coefficient);
}

void CXena2VidProc::SetMix3Coefficient(Fixed_ coefficient)
{
	WriteMixer3Coefficient(coefficient);
}

Fixed_ CXena2VidProc::GetMix3Coefficient()
{
	ULWord coefficient;
	ReadMixer3Coefficient(&coefficient);

	return static_cast<Fixed_>(coefficient);
}

void CXena2VidProc::SetMix4Coefficient(Fixed_ coefficient)
{
	WriteMixer4Coefficient(coefficient);
}

Fixed_ CXena2VidProc::GetMix4Coefficient()
{
	ULWord coefficient;
	ReadMixer4Coefficient(&coefficient);

	return static_cast<Fixed_>(coefficient);
}

#endif	//	!defined (NTV2_DEPRECATE)
