//
//  ntv2deviceservices.cpp
//
//  Copyright (c) 2011 AJA Video, Inc. All rights reserved.
//

#include "ntv2ioxtservices.h"
#include "ntv2io4kservices.h"
#include "ntv2io4kufcservices.h"
#include "ntv2kona3Gquadservices.h"
#include "ntv2kona3Gservices.h"
#include "ntv2konalhiservices.h"
#include "ntv2konalheplusservices.h"
#include "ntv2ioexpressservices.h"
#include "ntv2ioxtservices.h"
#include "ntv2corvidservices.h"
#include "ntv2corvid22services.h"
#include "ntv2corvid3gservices.h"
#include "ntv2ttapservices.h"
#include "ntv2devicefeatures.h"
#include "ntv2corvid24services.h"
#include "ntv2corvid44services.h"
#include "ntv2kona4quadservices.h"
#include "ntv2kona4ufcservices.h"
#include "ntv2konaip22services.h"
#include "ntv2konaip2110services.h"
#include "ntv2konaipj2kservices.h"
#include "ntv2io4kplusservices.h"
#include "ntv2vpidfromspec.h"
#include "ntv2corvid88services.h"
#include "ajabase/system/systemtime.h"


//-------------------------------------------------------------------------------------------------------
//	static accessors
//-------------------------------------------------------------------------------------------------------

// factory method
DeviceServices* DeviceServices::CreateDeviceServices(NTV2DeviceID deviceID)
{
	DeviceServices* pDeviceServices = NULL;

	// create board servicess
	switch (deviceID)
	{
		case DEVICE_ID_KONAIP_1RX_1TX_2110:
			pDeviceServices = new KonaIP2110Services();
			break;
        case DEVICE_ID_KONAIP_4CH_1SFP:
            pDeviceServices = new KonaIP22Services();
            break;
		case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:
		case DEVICE_ID_KONAIP_2TX_1SFP_J2K:
		case DEVICE_ID_KONAIP_2RX_1SFP_J2K:
			pDeviceServices = new KonaIPJ2kServices();
			break;
        case DEVICE_ID_KONA4:
            pDeviceServices = new Kona4QuadServices();
			break;
		case DEVICE_ID_KONA4UFC:
			pDeviceServices = new Kona4UfcServices();
			break;
		case DEVICE_ID_KONA3GQUAD:
			pDeviceServices = new Kona3GQuadServices();
			break;
		case DEVICE_ID_KONA3G:
			pDeviceServices = new Kona3GServices();
			break;
		case DEVICE_ID_IOEXPRESS:
			pDeviceServices = new IoExpressServices();
			break;
		case DEVICE_ID_LHI:
			pDeviceServices = new KonaLHiServices();
			break;
		case DEVICE_ID_LHE_PLUS:
			pDeviceServices = new KonaLHePlusServices();
			break;
		case DEVICE_ID_IOXT:
			pDeviceServices = new IoXTServices();
			break;
		case DEVICE_ID_IO4K:
			pDeviceServices = new Io4KServices();
			break;
		case DEVICE_ID_IO4KUFC:
			pDeviceServices = new Io4KUfcServices();
			break;
		case DEVICE_ID_TTAP:
			pDeviceServices = new TTapServices();
			break;
		case DEVICE_ID_CORVID24:
			pDeviceServices = new Corvid24Services();
			break;
		case DEVICE_ID_CORVID3G:
			pDeviceServices = new Corvid3GServices();
			break;
		case DEVICE_ID_CORVID44:
			pDeviceServices = new Corvid44Services();
			break;
		case DEVICE_ID_CORVID88:
			pDeviceServices = new Corvid88Services();
			break;
		case DEVICE_ID_IO4KPLUS:
			pDeviceServices = new Io4KPlusServices();
			break;
		default:
		case DEVICE_ID_CORVID1:
		case DEVICE_ID_CORVID22:
			pDeviceServices = new DeviceServices();
			break;
	}
	
	return pDeviceServices;
}


//-------------------------------------------------------------------------------------------------------
//	class DeviceServices
//-------------------------------------------------------------------------------------------------------

DeviceServices::DeviceServices()
{
	mHDMIStartupCountDown			= kHDMIStartupPhase0;
	mADCStabilizeCount				= 0;
	mHDMIOutColorSpaceModeStatus	= kHDMIOutCSCAutoDetect;
	mDisplayReferenceSelect			= kFreeRun;
	mCaptureReferenceSelect			= kVideoIn;
	mVirtualAnalogInBlackLevel		= NTV2_Black75IRE;
	mVirtualAnalogInType			= NTV2_AnlgComposite;		
	mADCLockScanTestFormat			= 0;
	mStreamingAppPID				= 0;
	mDefaultInput					= 0;
	mRegResetCycleCountLast			= 0;
	mStartupDisabled				= false;
	mInputFormatSelect				= -1;
	mInputFormatLock				= false;
	mLastInputFormatSelect			= NTV2_FORMAT_UNKNOWN;
	mDefaultVideoFormat				= NTV2_FORMAT_UNKNOWN;
}

//-------------------------------------------------------------------------------------------------------
//	ReadDriverState
//-------------------------------------------------------------------------------------------------------

void DeviceServices::ReadDriverState (void)
{
	mCard->GetStreamingApplication(&mStreamingAppType, &mStreamingAppPID);
	
	mCard->ReadRegister(kVRegDefaultVideoFormat, (ULWord*) &mDefaultVideoFormat);
	mCard->ReadRegister(kVRegDefaultVideoOutMode, (ULWord*) &mDefaultVideoOutMode);
	mCard->ReadRegister(kVRegFollowInputFormat, (ULWord *) &mFollowInputFormat);
	mCard->ReadRegister(kVRegVANCMode, (ULWord *) &mVANCMode);
	mCard->ReadRegister(kVRegDefaultInput, (ULWord *) &mDefaultInput);
	mCard->ReadRegister(kVRegDualStreamTransportType, (ULWord *) &mDualStreamTransportType);
	mCard->ReadRegister(kVRegDSKMode, (ULWord *) &mDSKMode);
	mCard->ReadRegister(kVRegDigitalOutput1Select, (ULWord *) &mVirtualDigitalOutput1Select);
	mCard->ReadRegister(kVRegDigitalOutput2Select, (ULWord *) &mVirtualDigitalOutput2Select);
	mCard->ReadRegister(kVRegHDMIOutputSelect, (ULWord *) &mVirtualHDMIOutputSelect);
	mCard->ReadRegister(kVRegAnalogOutputSelect, (ULWord *) &mVirtualAnalogOutputSelect);
	mCard->ReadRegister(kVRegLUTType, (ULWord *) &mLUTType);
	mCard->ReadRegister(kVRegInputSelect, (ULWord *) &mVirtualInputSelect);
	mCard->ReadRegister(kVRegSecondaryFormatSelect, (ULWord *) &mVirtualSecondaryFormatSelect);
	mCard->ReadRegister(kVRegIsoConvertEnable, (ULWord *) &mIsoConvertEnable);
	mCard->ReadRegister(kVRegDSKAudioMode, (ULWord *) &mDSKAudioMode);
	mCard->ReadRegister(kVRegDSKForegroundMode, (ULWord *) &mDSKForegroundMode);
	mCard->ReadRegister(kVRegDSKForegroundFade, (ULWord *) &mDSKForegroundFade);
	mCard->ReadRegister(kVRegCaptureReferenceSelect, (ULWord *) &mCaptureReferenceSelect);
	mCard->ReadRegister(kVRegDisplayReferenceSelect, (ULWord *) &mDisplayReferenceSelect);
	mCard->ReadRegister(kVReg2XTransferMode, (ULWord *) &m2XTransferMode);
	mCard->ReadRegister(kVRegGammaMode, (ULWord *) &mGammaMode);
	mCard->ReadRegister(kVRegRGB10Range, (ULWord *) &mRGB10Range);
	mCard->ReadRegister(kVRegColorSpaceMode, (ULWord *) &mColorSpaceMode);
	mCard->ReadRegister(kVRegSDIOutput1RGBRange, (ULWord *) &mSDIOutput1RGBRange);
	
	mCard->ReadRegister(kVRegSDIInput1RGBRange, (ULWord *) &mSDIInput1RGBRange);
	mCard->ReadRegister(kVRegSDIInput2RGBRange, (ULWord *) &mSDIInput2RGBRange);
	mCard->ReadRegister(kVRegSDIInput1Stereo3DMode, (ULWord *) &mSDIInput1Stereo3DMode);
	mCard->ReadRegister(kVRegFrameBuffer1RGBRange, (ULWord *) &mFrameBuffer1RGBRange);
	mCard->ReadRegister(kVRegFrameBuffer1Stereo3DMode, (ULWord *) &mFrameBuffer1Stereo3DMode);
	mCard->ReadRegister(kVRegAnalogOutBlackLevel, (ULWord *) &mVirtualAnalogOutBlackLevel);
	mCard->ReadRegister(kVRegAnalogOutputType, (ULWord *) &mVirtualAnalogOutputType);
	mCard->ReadRegister(kVRegAnalogInBlackLevel, (ULWord *) &mVirtualAnalogInBlackLevel);
	mCard->ReadRegister(kVRegAnalogInputType, (ULWord *) &mVirtualAnalogInType);
	mCard->ReadRegister(kVRegAnalogInStandard, (ULWord *) &mVirtualAnalogInStandard);
	
	mCard->ReadRegister(kVRegHDMIOutColorSpaceModeCtrl, (ULWord *) &mHDMIOutColorSpaceModeCtrl);
	mCard->ReadRegister(kVRegHDMIOutProtocolMode, (ULWord *) &mHDMIOutProtocolMode);
	mCard->ReadRegister(kVRegHDMIOutStereoSelect, (ULWord *) &mHDMIOutStereoSelect);
	mCard->ReadRegister(kVRegHDMIOutStereoCodecSelect, (ULWord *) &mHDMIOutStereoCodecSelect);
	mCard->ReadRegister(kVRegHDMIOutAudioChannels, (ULWord *) &mHDMIOutAudioChannels);
	mCard->ReadRegister(kVRegResetCycleCount, (ULWord *) &mRegResetCycleCount);
	mCard->ReadRegister(kVRegFramesPerVertical, (ULWord *) &mRegFramesPerVertical);
	mCard->ReadRegister(kVReg4kOutputTransportSelection, (ULWord *)&m4kTransportOutSelection);
	
	mCard->ReadRegister(kVRegSDIInput1FormatSelect, (ULWord *) &mSDIInput1FormatSelect);
	//mCard->ReadRegister(kVRegSDIInput2FormatSelect, (ULWord *) &mSDIInput2FormatSelect);
	mSDIInput2FormatSelect = mSDIInput1FormatSelect;	// for now
	
	// read primary HW registers (primary same as Channel1)
	mCard->GetVideoFormat(mFb1VideoFormat);
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, mFb1FrameBufferFomat);
	mCard->GetMode(NTV2_CHANNEL1, mFb1Mode);
	
    if (mCard->DeviceCanDoAudioMixer())
	{
		mCard->ReadRegister(kVRegAudioMixerOverrideState, (ULWord *) &mAudioMixerOverrideState);
		mCard->ReadRegister(kVRegAudioMixerSourceMainEnable, (ULWord *) &mAudioMixerSourceMainEnable);
		mCard->ReadRegister(kVRegAudioMixerSourceAux1Enable, (ULWord *) &mAudioMixerSourceAux1Enable);
		mCard->ReadRegister(kVRegAudioMixerSourceAux2Enable, (ULWord *) &mAudioMixerSourceAux2Enable);
		mCard->ReadRegister(kVRegAudioMixerSourceMainGain, (ULWord *) &mAudioMixerSourceMainGain);
		mCard->ReadRegister(kVRegAudioMixerSourceAux1Gain, (ULWord *) &mAudioMixerSourceAux1Gain);
		mCard->ReadRegister(kVRegAudioMixerSourceAux2Gain, (ULWord *) &mAudioMixerSourceAux2Gain);
		mCard->ReadRegister(kVRegAudioMixerSourceMainSelect, (ULWord *) &mAudioMixerSourceMainSelect);
		mCard->ReadRegister(kVRegAudioMixerSourceAux1Select, (ULWord *) &mAudioMixerSourceAux1Select);
		mCard->ReadRegister(kVRegAudioMixerSourceAux2Select, (ULWord *) &mAudioMixerSourceAux2Select);
	}

    if ((NTV2DeviceGetNum2022ChannelsSFP1(mCard->GetDeviceID()) > 0) && (mCard->IsDeviceReady(true) == true))
	{
        mCard->ReadRegister(kVReg2022_7Enable, (ULWord*)&m2022_7Mode);
        
        mCard->ReadRegister(kVRegIPAddrEth0, &mEth0.ipc_ip);
        mCard->ReadRegister(kVRegSubnetEth0, &mEth0.ipc_subnet);
        mCard->ReadRegister(kVRegGatewayEth0,&mEth0.ipc_gateway);
        
        mCard->ReadRegister(kVRegIPAddrEth1,	&mEth1.ipc_ip);
        mCard->ReadRegister(kVRegSubnetEth1,	&mEth1.ipc_subnet);
        mCard->ReadRegister(kVRegGatewayEth1,&mEth1.ipc_gateway);
        
        mCard->ReadRegister(kVRegRxcEnable1,				&mRx2022Config1.rxc_enable32);
        mCard->ReadRegister(kVRegRxcPrimaryRxMatch1,		&mRx2022Config1.rxc_primaryRxMatch);
        mCard->ReadRegister(kVRegRxcPrimarySourceIp1,		&mRx2022Config1.rxc_primarySourceIp);
        mCard->ReadRegister(kVRegRxcPrimaryDestIp1,			&mRx2022Config1.rxc_primaryDestIp);
        mCard->ReadRegister(kVRegRxcPrimarySourcePort1,		&mRx2022Config1.rxc_primarySourcePort);
        mCard->ReadRegister(kVRegRxcPrimaryDestPort1,		&mRx2022Config1.rxc_primaryDestPort);
        mCard->ReadRegister(kVRegRxcPrimarySsrc1,			&mRx2022Config1.rxc_primarySsrc);
        mCard->ReadRegister(kVRegRxcPrimaryVlan1,			&mRx2022Config1.rxc_primaryVlan);
        mCard->ReadRegister(kVRegRxcSecondaryRxMatch1,		&mRx2022Config1.rxc_secondaryRxMatch);
        mCard->ReadRegister(kVRegRxcSecondarySourceIp1,		&mRx2022Config1.rxc_secondarySourceIp);
        mCard->ReadRegister(kVRegRxcSecondaryDestIp1,		&mRx2022Config1.rxc_secondaryDestIp);
        mCard->ReadRegister(kVRegRxcSecondarySourcePort1,	&mRx2022Config1.rxc_secondarySourcePort);
        mCard->ReadRegister(kVRegRxcSecondaryDestPort1,		&mRx2022Config1.rxc_secondaryDestPort);
        mCard->ReadRegister(kVRegRxcSecondarySsrc1,			&mRx2022Config1.rxc_secondarySsrc);
        mCard->ReadRegister(kVRegRxcSecondaryVlan1,			&mRx2022Config1.rxc_secondaryVlan);
        mCard->ReadRegister(kVRegRxcNetworkPathDiff1,		&mRx2022Config1.rxc_networkPathDiff);
        mCard->ReadRegister(kVRegRxcPlayoutDelay1,			&mRx2022Config1.rxc_playoutDelay);

        mCard->ReadRegister(kVRegRxcEnable2,				&mRx2022Config2.rxc_enable32);
        mCard->ReadRegister(kVRegRxcPrimaryRxMatch2,		&mRx2022Config2.rxc_primaryRxMatch);
        mCard->ReadRegister(kVRegRxcPrimarySourceIp2,		&mRx2022Config2.rxc_primarySourceIp);
        mCard->ReadRegister(kVRegRxcPrimaryDestIp2,			&mRx2022Config2.rxc_primaryDestIp);
        mCard->ReadRegister(kVRegRxcPrimarySourcePort2,		&mRx2022Config2.rxc_primarySourcePort);
        mCard->ReadRegister(kVRegRxcPrimaryDestPort2,		&mRx2022Config2.rxc_primaryDestPort);
        mCard->ReadRegister(kVRegRxcPrimarySsrc2,			&mRx2022Config2.rxc_primarySsrc);
        mCard->ReadRegister(kVRegRxcPrimaryVlan2,			&mRx2022Config2.rxc_primaryVlan);
        mCard->ReadRegister(kVRegRxcSecondaryRxMatch2,		&mRx2022Config2.rxc_secondaryRxMatch);
        mCard->ReadRegister(kVRegRxcSecondarySourceIp2,		&mRx2022Config2.rxc_secondarySourceIp);
        mCard->ReadRegister(kVRegRxcSecondaryDestIp2,		&mRx2022Config2.rxc_secondaryDestIp);
        mCard->ReadRegister(kVRegRxcSecondarySourcePort2,	&mRx2022Config2.rxc_secondarySourcePort);
        mCard->ReadRegister(kVRegRxcSecondaryDestPort2,		&mRx2022Config2.rxc_secondaryDestPort);
        mCard->ReadRegister(kVRegRxcSecondarySsrc2,			&mRx2022Config2.rxc_secondarySsrc);
        mCard->ReadRegister(kVRegRxcSecondaryVlan2,			&mRx2022Config2.rxc_secondaryVlan);
        mCard->ReadRegister(kVRegRxcNetworkPathDiff2,		&mRx2022Config2.rxc_networkPathDiff);
        mCard->ReadRegister(kVRegRxcPlayoutDelay2,			&mRx2022Config2.rxc_playoutDelay);


        mCard->ReadRegister(kVRegTxcEnable3,				&mTx2022Config3.txc_enable32);
        mCard->ReadRegister(kVRegTxcPrimaryLocalPort3,		&mTx2022Config3.txc_primaryLocalPort);
        mCard->ReadRegister(kVRegTxcPrimaryRemoteIp3,		&mTx2022Config3.txc_primaryRemoteIp);
        mCard->ReadRegister(kVRegTxcPrimaryRemotePort3,		&mTx2022Config3.txc_primaryRemotePort);
        mCard->ReadRegister(kVRegTxcSecondaryLocalPort3,	&mTx2022Config3.txc_secondaryLocalPort);
        mCard->ReadRegister(kVRegTxcSecondaryRemoteIp3,		&mTx2022Config3.txc_secondaryRemoteIp);
        mCard->ReadRegister(kVRegTxcSecondaryRemotePort3,	&mTx2022Config3.txc_secondaryRemotePort);
		
        mCard->ReadRegister(kVRegTxcEnable4,				&mTx2022Config4.txc_enable32);
        mCard->ReadRegister(kVRegTxcPrimaryLocalPort4,		&mTx2022Config4.txc_primaryLocalPort);
        mCard->ReadRegister(kVRegTxcPrimaryRemoteIp4,		&mTx2022Config4.txc_primaryRemoteIp);
        mCard->ReadRegister(kVRegTxcPrimaryRemotePort4,		&mTx2022Config4.txc_primaryRemotePort);
        mCard->ReadRegister(kVRegTxcSecondaryLocalPort4,	&mTx2022Config4.txc_secondaryLocalPort);
        mCard->ReadRegister(kVRegTxcSecondaryRemoteIp4,		&mTx2022Config4.txc_secondaryRemoteIp);
        mCard->ReadRegister(kVRegTxcSecondaryRemotePort4,	&mTx2022Config4.txc_secondaryRemotePort);
		
		mCard->ReadRegister(kVRegRxc_2DecodeSelectionMode1,	(ULWord*)&mRx2022J2kConfig1.selectionMode);
		mCard->ReadRegister(kVRegRxc_2DecodeProgramNumber1,	&mRx2022J2kConfig1.programNumber);
		mCard->ReadRegister(kVRegRxc_2DecodeProgramPID1,	&mRx2022J2kConfig1.programPID);
		mCard->ReadRegister(kVRegRxc_2DecodeAudioNumber1,	&mRx2022J2kConfig1.audioNumber);
		
		mCard->ReadRegister(kVRegRxc_2DecodeSelectionMode2,	(ULWord*)&mRx2022J2kConfig2.selectionMode);
		mCard->ReadRegister(kVRegRxc_2DecodeProgramNumber2,	&mRx2022J2kConfig2.programNumber);
		mCard->ReadRegister(kVRegRxc_2DecodeProgramPID2,	&mRx2022J2kConfig2.programPID);
		mCard->ReadRegister(kVRegRxc_2DecodeAudioNumber2,	&mRx2022J2kConfig2.audioNumber);
		
		mCard->ReadRegister(kVRegTxc_2EncodeVideoFormat1,	(ULWord*)&mTx2022J2kConfig1.videoFormat);
		mCard->ReadRegister(kVRegTxc_2EncodeUllMode1,		&mTx2022J2kConfig1.ullMode);
		mCard->ReadRegister(kVRegTxc_2EncodeBitDepth1,		&mTx2022J2kConfig1.bitDepth);
		mCard->ReadRegister(kVRegTxc_2EncodeChromaSubSamp1,	(ULWord*)&mTx2022J2kConfig1.chromaSubsamp);
		mCard->ReadRegister(kVRegTxc_2EncodeMbps1,			&mTx2022J2kConfig1.mbps);
		mCard->ReadRegister(kVRegTxc_2EncodeAudioChannels1, &mTx2022J2kConfig1.audioChannels);
		mCard->ReadRegister(kVRegTxc_2EncodeStreamType1,	(ULWord*)&mTx2022J2kConfig1.streamType);
		mCard->ReadRegister(kVRegTxc_2EncodeProgramPid1,	&mTx2022J2kConfig1.pmtPid);
		mCard->ReadRegister(kVRegTxc_2EncodeVideoPid1,		&mTx2022J2kConfig1.videoPid);
		mCard->ReadRegister(kVRegTxc_2EncodePcrPid1,		&mTx2022J2kConfig1.pcrPid);
		mCard->ReadRegister(kVRegTxc_2EncodeAudio1Pid1,		&mTx2022J2kConfig1.audio1Pid);
		
		mCard->ReadRegister(kVRegTxc_2EncodeVideoFormat2,	(ULWord*)&mTx2022J2kConfig2.videoFormat);
		mCard->ReadRegister(kVRegTxc_2EncodeUllMode2,		&mTx2022J2kConfig2.ullMode);
		mCard->ReadRegister(kVRegTxc_2EncodeBitDepth2,		&mTx2022J2kConfig2.bitDepth);
		mCard->ReadRegister(kVRegTxc_2EncodeChromaSubSamp2,	(ULWord*)&mTx2022J2kConfig2.chromaSubsamp);
		mCard->ReadRegister(kVRegTxc_2EncodeMbps2,			&mTx2022J2kConfig2.mbps);
		mCard->ReadRegister(kVRegTxc_2EncodeAudioChannels2, &mTx2022J2kConfig2.audioChannels);
		mCard->ReadRegister(kVRegTxc_2EncodeStreamType2,	(ULWord*)&mTx2022J2kConfig2.streamType);
		mCard->ReadRegister(kVRegTxc_2EncodeProgramPid2,	&mTx2022J2kConfig2.pmtPid);
		mCard->ReadRegister(kVRegTxc_2EncodeVideoPid2,		&mTx2022J2kConfig2.videoPid);
		mCard->ReadRegister(kVRegTxc_2EncodePcrPid2,		&mTx2022J2kConfig2.pcrPid);
		mCard->ReadRegister(kVRegTxc_2EncodeAudio1Pid2,		&mTx2022J2kConfig2.audio1Pid);
	}
}


//-------------------------------------------------------------------------------------------------------
//	UpdateAutoState
//-------------------------------------------------------------------------------------------------------
void DeviceServices::UpdateAutoState ()
{
	if (mDualStreamTransportType == NTV2_SDITransport_Auto)
		mDualStreamTransportType = NTV2_SDITransport_DualLink_3Gb;
}


//-------------------------------------------------------------------------------------------------------
//	GetSelectedInputVideoFormat
//	Note:	Determine input video format based on input select and fbVideoFormat
//			which currently is videoformat of ch1-framebuffer
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat DeviceServices::GetSelectedInputVideoFormat(
											NTV2VideoFormat fbVideoFormat,
											NTV2SDIInputFormatSelect* inputFormatSelect)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	if (inputFormatSelect)
		*inputFormatSelect = NTV2_YUVSelect;
	
	// Figure out what our input format is based on what is selected 
	switch (mVirtualInputSelect)
	{
	case NTV2_Input1Select:
	case NTV2_DualLinkInputSelect:
	case NTV2_DualLink2xSdi4k:
	case NTV2_DualLink4xSdi4k:
		inputFormat = GetSdiInVideoFormat(0, fbVideoFormat);
		if (inputFormatSelect)
			*inputFormatSelect = mSDIInput1FormatSelect;
		break;

	case NTV2_Input2Select:
		inputFormat = GetSdiInVideoFormat(1, fbVideoFormat);
		if (inputFormatSelect)
			*inputFormatSelect = mSDIInput2FormatSelect;
		break;

	default:
		break;
	}

	inputFormat = GetTransportCompatibleFormat(inputFormat, fbVideoFormat);
	
	return inputFormat;
}


//-------------------------------------------------------------------------------------------------------
//	SetDeviceEveryFrameRegs
//-------------------------------------------------------------------------------------------------------

// Do everyframe task using filter variables set in virtual register  
void DeviceServices::SetDeviceEveryFrameRegs ()
{
	uint32_t virtualDebug1			= 0;
	uint32_t everyFrameTaskFilter	= 0;
	
	mCard->ReadRegister (kVRegDebug1, (ULWord *) &virtualDebug1);
	mCard->ReadRegister (kVRegEveryFrameTaskFilter, (ULWord *) &everyFrameTaskFilter);
	
	SetDeviceEveryFrameRegs (virtualDebug1, everyFrameTaskFilter);
}

// Do everyframe task using input filter variables  
void DeviceServices::SetDeviceEveryFrameRegs (uint32_t virtualDebug1, uint32_t everyFrameTaskFilter)
{	
	// override virtual register filter variables
	mVirtualDebug1			= virtualDebug1;
	mEveryFrameTaskFilter	= everyFrameTaskFilter;

	//	CP checks the kVRegAgentCheck virtual register to see if I'm still running...
	uint32_t	count	(0);
	mCard->ReadRegister(kVRegAgentCheck, &count);
	count++;
	mCard->WriteRegister(kVRegAgentCheck, count);

	// If the daemon is not responsible for tasks just return
	if (mVirtualDebug1 & NTV2_DRIVER_TASKS)
	{
		return;
	}
	
	// If tasks are disabled do nothing
	if (mEveryFrameTaskFilter == NTV2_DISABLE_TASKS)
	{
		return;
	}

	// OEM tasks
	if (mEveryFrameTaskFilter & NTV2_OEM_TASKS)
	{
		return;		
	}
	
	// read in virtual registers
	ReadDriverState();
	
	// auto set registers marked with "auto" enum
	UpdateAutoState();
		
	// Get the general format
	NTV2DeviceID deviceID = mCard->GetDeviceID();

	if (::NTV2DeviceCanDoMultiFormat(deviceID))
	{
		mCard->SetMultiFormatMode(false);
	}

	NTV2FrameBufferFormat fbFormat;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormat);
	
	GeneralFrameFormat format = GetGeneralFrameFormat(fbFormat);
	if (::NTV2DeviceCanDoFrameBufferFormat(deviceID, fbFormat) == false)
		format = FORMAT_YUV;
	
	// Get display/capture mode and call routines to setup XPoint
	NTV2Mode mode = GetCh1Mode();

	if (mode == NTV2_MODE_DISPLAY)
	{
		if (IS_CION_RAW(format))
			SetDeviceXPointPlaybackRaw(format);
		else
			SetDeviceXPointPlayback(format);
	}
	else
	{
		// follow input option
		if (mFollowInputFormat)
		{
			NTV2VideoFormat lockedInputFormat = GetLockedInputVideoFormat();
			if (mFb1VideoFormat != lockedInputFormat)
			{
				mCard->WriteRegister(kVRegDefaultVideoFormat, lockedInputFormat);
				mCard->SetVideoFormat(lockedInputFormat);
			}
		}
	
		if (IS_CION_RAW(format))
			SetDeviceXPointCaptureRaw(format);
		else
			SetDeviceXPointCapture(format);
	}
	
	//Setup the analog LTC stuff
	RP188SourceSelect TimecodeSource;
	mCard->ReadRegister(kVRegRP188SourceSelect, (ULWord*)&TimecodeSource);
	if (NTV2DeviceGetNumLTCInputs(mCard->GetDeviceID()) && TimecodeSource == kRP188SourceLTCPort)
	{
		mCard->SetLTCInputEnable(true);
		mCard->WriteRegister(kRegFS1ReferenceSelect, 0x1, BIT(10), 10);
		mCard->WriteRegister(kRegLTCStatusControl, 0x1, kRegMaskLTC1InBypass, kRegShiftLTC1Bypass);
		if(NTV2DeviceCanDoLTCInOnRefPort(mCard->GetDeviceID()))
			mCard->SetLTCOnReference(true);
	}
	else
	{
		mCard->SetLTCInputEnable(false);
		mCard->WriteRegister(kRegFS1ReferenceSelect, 0x0, BIT(10), 10);
		mCard->WriteRegister(kRegLTCStatusControl, 0x0, kRegMaskLTC1InBypass, kRegShiftLTC1Bypass);
		if (NTV2DeviceCanDoLTCInOnRefPort(mCard->GetDeviceID()))
			mCard->SetLTCOnReference(false);
	}
	
	
	//
	// Audio output
	//
	
	// host audio
	bool suspended = false;
	mCard->GetSuspendHostAudio(suspended);
	NTV2AudioSystem audioSystem = NTV2_AUDIOSYSTEM_1;
	NTV2AudioSystem hostAudioSystem = GetHostAudioSystem();
	
	// if host-audio not suspended - use host-audio system
	// note - historically host-audio was NTV2_AUDIOSYSTEM_1 only. Newer devices, host-audio use other systems.
	if (suspended == false)
		audioSystem = (NTV2AudioSystem) hostAudioSystem;
	
	// mixer - support
	if (mCard->DeviceCanDoAudioMixer() == true )
	{
		if (mAudioMixerOverrideState == false)
		{
			mCard->SetAudioMixerMainInputGain(mAudioMixerSourceMainGain);
			mCard->SetAudioMixerAux1InputGain(NTV2_AudioMixerChannel1, mAudioMixerSourceAux1Gain);
			mCard->SetAudioMixerAux2InputGain(NTV2_AudioMixerChannel1, mAudioMixerSourceAux2Gain);
			mCard->SetAudioMixerMainInputChannelSelect(NTV2_AudioChannel1_2);
			mCard->WriteRegister(kRegAudioMixerMutes, 0x0000, 0xffff, 0);	// unmute all output channels
		}
		
		mCard->SetAudioMixerMainInputAudioSystem(NTV2_AUDIOSYSTEM_1);
		mCard->SetAudioMixerAux1x2chInputAudioSystem(hostAudioSystem);
		mCard->SetAudioMixerAux2x2chInputAudioSystem(NTV2_AUDIOSYSTEM_2);
	
		//if (mode == NTV2_MODE_DISPLAY)
		{
			mCard->SetAudioMixerMainInputEnable(mAudioMixerSourceMainEnable);
			mCard->SetAudioMixerAux1InputEnable(mAudioMixerSourceAux1Enable);
			mCard->SetAudioMixerAux2InputEnable(mAudioMixerSourceAux2Enable);
		}
		//else
		//{
			//mCard->SetAudioMixerMainInputEnable(true);
			//mCard->SetAudioMixerAux1InputEnable(false);
			//mCard->SetAudioMixerAux2InputEnable(false);
		//}
		
		audioSystem = NTV2_AUDIOSYSTEM_6;
	}

	// Setup the SDI Outputs audio source
	switch(NTV2DeviceGetNumVideoOutputs(deviceID))
	{
	case 8:
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL8, audioSystem);
	case 7:
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL7, audioSystem);
	case 6:
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL6, audioSystem);
	case 5:
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL5, audioSystem);
	case 4:
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL4, audioSystem);
	case 3:
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL3, audioSystem);
	case 2:
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL2, audioSystem);
	default:
	case 1:
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL1, audioSystem);
		break;
	}
	if(NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1))
	{
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL5, audioSystem );
	}

	switch(NTV2DeviceGetNumVideoChannels(deviceID))
	{
	case 8:
		mCard->SetFrameBufferOrientation(NTV2_CHANNEL8, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
	case 7:
		mCard->SetFrameBufferOrientation(NTV2_CHANNEL7, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
	case 6:
		mCard->SetFrameBufferOrientation(NTV2_CHANNEL6, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
	case 5:
		mCard->SetFrameBufferOrientation(NTV2_CHANNEL5, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
	case 4:
		mCard->SetFrameBufferOrientation(NTV2_CHANNEL4, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
	case 3:
		mCard->SetFrameBufferOrientation(NTV2_CHANNEL3, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
	case 2:
		mCard->SetFrameBufferOrientation(NTV2_CHANNEL2, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
	default:
	case 1:
		mCard->SetFrameBufferOrientation(NTV2_CHANNEL1, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);
		break;
	}

	//Setup LUTs
	UpdateK2ColorSpaceMatrixSelect();
	UpdateK2LUTSelect();

	if (mStartupDisabled)
	{
		mHDMIStartupCountDown = 0;
		mRegResetCycleCountLast = mRegResetCycleCount;
	}
	else if (mRegResetCycleCount != mRegResetCycleCountLast)
	{

		// reset condition
		mRegResetCycleCountLast = mRegResetCycleCount;

		// reinitialize HDMI hardware on reset cycle
		mHDMIStartupCountDown = kHDMIStartupPhase0;

		if (::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT1))
			mCard->WriteRegister(kVRegLUTType, NTV2_LUTUnknown);
		if (::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT2))
			mCard->WriteRegister(kVRegLUT2Type, NTV2_LUTUnknown);
		if (::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT3))
			mCard->WriteRegister(kVRegLUT3Type, NTV2_LUTUnknown);
		if (::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT4))
			mCard->WriteRegister(kVRegLUT4Type, NTV2_LUTUnknown);
		if (::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT5))
			mCard->WriteRegister(kVRegLUT5Type, NTV2_LUTUnknown);
	}
	// Set misc registers
	SetDeviceMiscRegisters(mode);
}


void DeviceServices::SetDeviceMiscRegisters (NTV2Mode mode)
{
	(void) mode;
}


// MARK: -
// MARK: support


bool DeviceServices::SetVPIDData (	ULWord &				outVPID,
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
	
	bool is372						= NTV2_IS_372_DUALLINK_FORMAT(inOutputFormat) || (!inIsDualLinkRGB && inOutputIs3Gb);
	vpidSpec.videoFormat			= inOutputFormat;
	vpidSpec.isRGBOnWire			= inIsDualLinkRGB;
	vpidSpec.isOutputLevelA			= NTV2_IS_3G_FORMAT(inOutputFormat) && !inOutputIs3Gb && !is372;
	vpidSpec.isOutputLevelB			= inOutputIs3Gb;
	vpidSpec.isDualLink				= inIsDualLinkRGB ||
									  (is372 && !vpidSpec.isOutputLevelA);
	vpidSpec.isTwoSampleInterleave	= inIsSMPTE425;
	vpidSpec.useChannel				= inUseChannel;
	vpidSpec.vpidChannel			= inChannel;
	vpidSpec.isStereo				= false;
	vpidSpec.isRightEye				= false;
	vpidSpec.audioCarriage			= VPIDAudio_Unknown;
	
	bool b4k = NTV2_IS_4K_VIDEO_FORMAT(inOutputFormat);
	if (b4k == true || mFb1Mode == NTV2_MODE_CAPTURE)
	{
		vpidSpec.pixelFormat = inIsRGB48Bit ? NTV2_FBF_48BIT_RGB : NTV2_FBF_INVALID;
	}
	else
	{
		// 12 bit RGB
		if (inIsRGB48Bit)
			vpidSpec.pixelFormat = NTV2_FBF_48BIT_RGB;
		
		// Converted RGB -> YUV on wire
		else if (vpidSpec.isRGBOnWire == false && IsFrameBufferFormatRGB(mFb1FrameBufferFomat) == true)
			vpidSpec.pixelFormat = Is8BitFrameBufferFormat(mFb1FrameBufferFomat) ? NTV2_FBF_8BIT_YCBCR : NTV2_FBF_INVALID;
	
		// Converted YUV -> RGB on wire
		else if (vpidSpec.isRGBOnWire == true && IsFrameBufferFormatRGB(mFb1FrameBufferFomat) == false)
			vpidSpec.pixelFormat = NTV2_FBF_INVALID;
	
		// otherwise
		else
			vpidSpec.pixelFormat = mFb1FrameBufferFomat;
	}

	return ::SetVPIDFromSpec (&outVPID, &vpidSpec);
}


NTV2VideoFormat DeviceServices::GetLockedInputVideoFormat()
{
	const int32_t kLockAttemps		= 3;
	const int32_t kLockSleepTimeMs	= 30;
	
	NTV2VideoFormat frameBufferVideoFormat;
	mCard->GetVideoFormat(&frameBufferVideoFormat);
	
	// default output
	NTV2VideoFormat outVideoFormat = frameBufferVideoFormat;

	// following the input video format, make sure it is locked
	if (mFollowInputFormat)
	{
		NTV2VideoFormat inputVideoFormat = GetSelectedInputVideoFormat(frameBufferVideoFormat);
	
		mInputFormatLock	= mInputFormatLock &&
							  inputVideoFormat != NTV2_FORMAT_UNKNOWN &&
							  mLastInputFormatSelect == inputVideoFormat;
		
		if (mInputFormatLock)
		{
			outVideoFormat = inputVideoFormat;
		}
		else
		{
			mLastInputFormatSelect	= inputVideoFormat;
			int attempts			= kLockAttemps;
			while (attempts > 0)
			{
				AJATime::Sleep(kLockSleepTimeMs);
				inputVideoFormat = GetSelectedInputVideoFormat(frameBufferVideoFormat);
				if (inputVideoFormat != mLastInputFormatSelect)
					break;
				if (inputVideoFormat == NTV2_FORMAT_UNKNOWN)
					break;
				attempts--;
			}
			
			mInputFormatLock = (attempts == 0 && inputVideoFormat != NTV2_FORMAT_UNKNOWN);
			mLastInputFormatSelect = inputVideoFormat;
			
			if (mInputFormatLock)
				outVideoFormat = inputVideoFormat;
		}
	}
	else
		mLastInputFormatSelect = NTV2_FORMAT_UNKNOWN;
	
	
	return outVideoFormat;
}


GeneralFrameFormat DeviceServices::GetGeneralFrameFormat(NTV2FrameBufferFormat fbFormat)
{
	GeneralFrameFormat genFmt;
	
	switch (fbFormat)
	{
		case NTV2_FBF_PRORES_DVCPRO:
		case NTV2_FBF_PRORES_HDV:
		case NTV2_FBF_8BIT_DVCPRO:
		//case NTV2_FBF_8BIT_QREZ:
		case NTV2_FBF_8BIT_HDV:
			genFmt = FORMAT_COMPRESSED;
			break;

		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_DPX:
		case NTV2_FBF_24BIT_RGB:
		case NTV2_FBF_24BIT_BGR:
        case NTV2_FBF_10BIT_DPX_LITTLEENDIAN:
        case NTV2_FBF_48BIT_RGB:
			genFmt = FORMAT_RGB;
			break;
			
		case NTV2_FBF_10BIT_RAW_RGB:
			{
				if (mRegFramesPerVertical > 1)
					genFmt = FORMAT_RAW_HFR;
				else
					genFmt = FORMAT_RAW;
			}
			break;
			
		case NTV2_FBF_10BIT_RAW_YCBCR:
			genFmt = FORMAT_RAW_UHFR;
			break;

		case NTV2_FBF_PRORES:
		case NTV2_FBF_10BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR:
		case NTV2_FBF_8BIT_YCBCR_YUY2:
		case NTV2_FBF_10BIT_YCBCR_DPX:
		case NTV2_FBF_10BIT_YCBCRA:
		default:
			genFmt = FORMAT_YUV;
			break;
	}

	return genFmt;
}


NTV2Mode DeviceServices::GetCh1Mode()
{
	uint32_t regValue;

	mCard->ReadRegister(kRegCh1Control, &regValue, kRegMaskMode, kRegShiftMode);
	return (NTV2Mode)regValue;
}


bool DeviceServices::IsPulldownConverterMode(NTV2VideoFormat fmt1, NTV2VideoFormat fmt2)
{
	switch (fmt1)
	{
		case NTV2_FORMAT_1080psf_2398:
			return fmt2 == NTV2_FORMAT_525_5994;
		default:
			return false;
	}
}


bool DeviceServices::IsFrameBufferFormatRGB(NTV2FrameBufferFormat fbFormat)
{
	switch (fbFormat)
	{
		case NTV2_FBF_ARGB:
		case NTV2_FBF_RGBA:
		case NTV2_FBF_ABGR:
		case NTV2_FBF_10BIT_RGB:
		case NTV2_FBF_10BIT_DPX:
        case NTV2_FBF_48BIT_RGB:
			return true;
		default:
			return false;
	}
}


bool DeviceServices::IsCompatibleWithReference(NTV2VideoFormat fbFormat)
{
	bool bResult = false;

	// get reference frame rate
	ULWord status;
	mCard->ReadInputStatusRegister(&status);
	NTV2FrameRate rateRef = (NTV2FrameRate)((status>>16)&0xF);
	NTV2FrameRate rateFb  = GetNTV2FrameRateFromVideoFormat(fbFormat);

	switch(rateFb)
	{
		case NTV2_FRAMERATE_6000:
		case NTV2_FRAMERATE_3000:
		case NTV2_FRAMERATE_1500:
			bResult = (rateRef == NTV2_FRAMERATE_6000 || rateRef == NTV2_FRAMERATE_3000);
			break;
	
		case NTV2_FRAMERATE_5994:
		case NTV2_FRAMERATE_2997:
		case NTV2_FRAMERATE_1498:
			bResult = (rateRef == NTV2_FRAMERATE_5994 || rateRef == NTV2_FRAMERATE_2997);
			break;
			
		case NTV2_FRAMERATE_5000:
		case NTV2_FRAMERATE_2500:
			bResult = (rateRef == NTV2_FRAMERATE_5000 || rateRef == NTV2_FRAMERATE_2500);
			break;
	
		case NTV2_FRAMERATE_2398:
		case NTV2_FRAMERATE_2400:
		default:
			bResult = (rateRef == rateFb);
			break;
	}
	
	return bResult;
}


void DeviceServices::SetMacDebugOption(int item)
{
    if ( mCard->IsOpen() )
    {
		ULWord regVal = 0;
		mCard->ReadRegister( kVRegDebug1, &regVal );
		switch(item)
		{
			case 0:	// disable every frame
				regVal = regVal & (~BIT_29);
				regVal = regVal | BIT_31;
				break;
			case 2:		// oem tasks
				regVal = regVal | BIT_29;
				regVal = regVal | BIT_31;
				break;
			default:	// standard tasks - or case 1
				regVal = regVal & (~BIT_29);
				regVal = regVal & (~BIT_31);
				break;
		}
		
		mCard->WriteRegister( kVRegDebug1, regVal );
    }
}


bool DeviceServices::IsDeinterlacedMode(NTV2VideoFormat fmt1, NTV2VideoFormat fmt2)
{
	bool isDeinterlaced = false;

	switch (fmt1)
	{
		case NTV2_FORMAT_525_5994:		isDeinterlaced = fmt2 == NTV2_FORMAT_525psf_2997;		break;
		case NTV2_FORMAT_625_5000:		isDeinterlaced = fmt2 == NTV2_FORMAT_625psf_2500;		break;
		case NTV2_FORMAT_1080i_5994:	isDeinterlaced = fmt2 == NTV2_FORMAT_1080psf_2997_2;	break;
		default:						break;
	}

	return isDeinterlaced;
}



// MARK: -

NTV2RGB10Range DeviceServices::GetCSCRange()
{
	NTV2DeviceID deviceID = mCard->GetDeviceID();
	NTV2FrameBufferFormat frameBufferFormat;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &frameBufferFormat);
	
	// Get display/capture mode and call routines to setup XPoint
	NTV2Mode mode = GetCh1Mode();

	// get csc rgb range
	NTV2RGB10Range cscRange = mRGB10Range;		// default use RGB range
	
	// for case where duallink(RGB) IO is supported
	if ( ::NTV2DeviceCanDoDualLink(deviceID) )
	{
		if (mode == NTV2_MODE_DISPLAY)
		{
			// follow framebuffer RGB range
			if (NTV2_IS_FBF_RGB(frameBufferFormat))
				cscRange = mRGB10Range; 
			
			// follow output RGB range
			else									
				cscRange = (mSDIOutput1RGBRange == NTV2_RGBRangeFull) ? NTV2_RGB10RangeFull : NTV2_RGB10RangeSMPTE; 
		}
		
		else	// mode == NTV2_MODE_CAPTURE
		{
			// follow input RGB range
			if (mSDIInput1FormatSelect == NTV2_RGBSelect)
				cscRange = (mSDIInput1RGBRange == NTV2_RGBRangeFull) ? NTV2_RGB10RangeFull : NTV2_RGB10RangeSMPTE; 
			
			// follow framebuffer RGB range
			else									
				cscRange = mRGB10Range; 
		}
	}
	
	return cscRange;
}

NTV2LHIVideoDACMode DeviceServices::GetLHIVideoDACMode(NTV2VideoFormat format, NTV2AnalogType type, NTV2AnalogBlackLevel blackLevel)
{
	NTV2LHIVideoDACMode mode = NTV2LHI_480iYPbPrSMPTE;
	
	switch (format)
	{
		case NTV2_FORMAT_525_5994:
		case NTV2_FORMAT_525psf_2997:
			if ( (type == NTV2_AnlgComposite) && (blackLevel == NTV2_Black75IRE) )
				mode = NTV2LHI_480iNTSC_US_Composite;
			else if ( (type == NTV2_AnlgComposite) && (blackLevel == NTV2_Black0IRE) )
				mode = NTV2LHI_480iNTSC_Japan_Composite;
			else if ( (type == NTV2_AnlgComponentSMPTE) && (blackLevel == NTV2_Black75IRE) )
				mode = NTV2LHI_480iYPbPrSMPTE;
			else if ( (type == NTV2_AnlgComponentBetacam) && (blackLevel == NTV2_Black75IRE) )
				mode = NTV2LHI_480iYPbPrBetacam525;
			else if ( (type == NTV2_AnlgComponentBetacam) && (blackLevel == NTV2_Black0IRE) )
				mode = NTV2LHI_480iYPbPrBetacamJapan;
			else if ( type == NTV2_AnlgComponentRGB )
				mode = NTV2LHI_480iRGB;
			else
				mode = NTV2LHI_480iYPbPrSMPTE;
			break;
		
		case NTV2_FORMAT_625_5000:
		case NTV2_FORMAT_625psf_2500:
			if (type == NTV2_AnlgComposite)
				mode = NTV2LHI_576iPAL_Composite;
			else if ( (type == NTV2_AnlgComponentSMPTE) && (blackLevel == NTV2_Black75IRE) )
				mode = NTV2LHI_576iYPbPrSMPTE;
			else if ( type == NTV2_AnlgComponentRGB )
				mode = NTV2LHI_576iRGB;
			else
				mode = NTV2LHI_576iYPbPrSMPTE;
			break;
			
		case NTV2_FORMAT_720p_5000:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
				// theoretically we'll only be asked to do a component analog mode...
			if (type == NTV2_AnlgComponentRGB)
				mode = NTV2LHI_720pRGB;
			else
				mode = NTV2LHI_720pSMPTE;
			break;
		
		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_1080i_5994:
		case NTV2_FORMAT_1080i_6000:
		case NTV2_FORMAT_1080p_2997:
		case NTV2_FORMAT_1080p_3000:
		case NTV2_FORMAT_1080p_2500:
		case NTV2_FORMAT_1080p_2398:
		case NTV2_FORMAT_1080p_2400:
		case NTV2_FORMAT_1080p_5000_B:
		case NTV2_FORMAT_1080p_5994_B:
		case NTV2_FORMAT_1080p_6000_B:
		case NTV2_FORMAT_1080p_2K_2398:
		case NTV2_FORMAT_1080p_2K_2400:
				// theoretically we'll only be asked to do a component analog mode...
			if (type == NTV2_AnlgComponentRGB)
				mode = NTV2LHI_1080iRGB;
			else
				mode = NTV2LHI_1080iSMPTE;
			break;
		
		case NTV2_FORMAT_1080psf_2500_2:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_1080psf_2398:
		case NTV2_FORMAT_1080psf_2400:
		case NTV2_FORMAT_1080psf_2K_2398:
		case NTV2_FORMAT_1080psf_2K_2400:
		case NTV2_FORMAT_1080psf_2K_2500:
				// theoretically we'll only be asked to do a component analog mode...
			if (type == NTV2_AnlgComponentRGB)
				mode = NTV2LHI_1080psfRGB;
			else
				mode = NTV2LHI_1080psfSMPTE;
			break;
		
		case NTV2_FORMAT_1080p_5000_A:
		case NTV2_FORMAT_1080p_5994_A:
		case NTV2_FORMAT_1080p_6000_A:
		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:
				break;
		
		default:	break;
	}
	
	return mode;
}


bool DeviceServices::GetExtFrameGeometry(NTV2FrameGeometry geometry, NTV2FrameGeometry* value)
{
	switch (geometry)
	{
		case NTV2_FG_720x486:	
		case NTV2_FG_720x508:
			*value = NTV2_FG_720x508;		
			break;
		case NTV2_FG_720x576:	
		case NTV2_FG_720x598:	
			*value = NTV2_FG_720x598;		
			break;
		case NTV2_FG_1280x720:	
		case NTV2_FG_1280x740:	
			*value = NTV2_FG_1280x740;	
			break;
		case NTV2_FG_1920x1080: 
		case NTV2_FG_1920x1112: 
			*value = NTV2_FG_1920x1112;	
			break;
		case NTV2_FG_2048x1080: 
		case NTV2_FG_2048x1112: 
			*value = NTV2_FG_2048x1112;	
			break;
		case NTV2_FG_2048x1556: 
		case NTV2_FG_2048x1588: 
			*value = NTV2_FG_2048x1588;	
			break;
		default: 
			break;  
	}
		
    return true;
}


void DeviceServices::SetADCRegisters(	NTV2Standard analogInStandard, NTV2AnalogType analogInType, 
										NTV2AnalogBlackLevel analogInBlackLevel, NTV2FrameRate analogInFrameRate)
{
	NTV2LSVideoADCMode adcMode = NTV2_480iADCComponentBeta;
	
	switch (analogInStandard)
	{
		case NTV2_STANDARD_525:
			switch (analogInBlackLevel)
			{
				case NTV2_Black75IRE:
					if (analogInType == NTV2_AnlgComponentBetacam)
						adcMode = NTV2_480iADCComponentBeta;
					else if (analogInType == NTV2_AnlgComponentSMPTE)
						adcMode = NTV2_480iADCComponentSMPTE;
					else if (analogInType == NTV2_AnlgSVideo)
						adcMode = NTV2_480iADCSVideoUS;
					else if (analogInType == NTV2_AnlgComposite)
						adcMode = NTV2_480iADCCompositeUS;
					break;
			
				default:
				case NTV2_Black0IRE:
					if (analogInType == NTV2_AnlgComponentBetacam)
						adcMode = NTV2_480iADCComponentBetaJapan;
					else if (analogInType == NTV2_AnlgComponentSMPTE)
						adcMode = NTV2_480iADCComponentSMPTEJapan;
					else if (analogInType == NTV2_AnlgSVideo)
						adcMode = NTV2_480iADCSVideoJapan;
					else if (analogInType == NTV2_AnlgComposite)
						adcMode = NTV2_480iADCCompositeJapan;
					break;
			}
			break;
			
		case NTV2_STANDARD_625:
			switch (analogInBlackLevel)
			{
				case NTV2_Black75IRE:
					if (analogInType == NTV2_AnlgComponentBetacam)
						adcMode = NTV2_576iADCComponentBeta;
					else if (analogInType == NTV2_AnlgComponentSMPTE)
						adcMode = NTV2_576iADCComponentSMPTE;
					else if (analogInType == NTV2_AnlgSVideo)
						adcMode = NTV2_576iADCSVideo;
					else if (analogInType == NTV2_AnlgComposite)
						adcMode = NTV2_576iADCComposite;
					break;
			
				default:
				case NTV2_Black0IRE:
					break;
			}		
			break;
			
		case NTV2_STANDARD_720:
			switch (analogInFrameRate)
			{
				case NTV2_FRAMERATE_5000:
					adcMode = NTV2_720p_50;
					break;

				case NTV2_FRAMERATE_5994:
				default:
					adcMode = NTV2_720p_60;
					break;
			}
			break;
			
		case NTV2_STANDARD_1080:
			switch (analogInFrameRate)
			{
				case NTV2_FRAMERATE_2398:
				case NTV2_FRAMERATE_2400:
					adcMode = NTV2_1080pSF24;
					break;
			
				case NTV2_FRAMERATE_2500:
					adcMode = NTV2_1080i_25;
					break;

				case NTV2_FRAMERATE_2997:
				default:
					adcMode = NTV2_1080i_30;
					break;
				
			}
			break;
		
		case NTV2_STANDARD_2K:
		default:
			adcMode = NTV2_480iADCComponentBeta;
			break;
	}
	
	SetVideoADCMode(adcMode);	
}


bool DeviceServices::GetADCRegisters(	NTV2Standard *analogInStandard, NTV2AnalogType *analogInType, 
										NTV2AnalogBlackLevel *analogInBlackLevel, NTV2FrameRate *analogInFrameRate)
{
	NTV2LSVideoADCMode adcMode = GetVideoADCMode();
	
	switch (adcMode)
	{
		case NTV2_480iADCComponentBeta:
			*analogInStandard = NTV2_STANDARD_525;
			*analogInType = NTV2_AnlgComponentBetacam;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_480iADCComponentSMPTE:
			*analogInStandard = NTV2_STANDARD_525;
			*analogInType = NTV2_AnlgComponentSMPTE;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_480iADCSVideoUS:
			*analogInStandard = NTV2_STANDARD_525;
			*analogInType = NTV2_AnlgSVideo;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_480iADCCompositeUS:
			*analogInStandard = NTV2_STANDARD_525;
			*analogInType = NTV2_AnlgComposite;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_480iADCComponentBetaJapan:
			*analogInStandard = NTV2_STANDARD_525;
			*analogInType = NTV2_AnlgComponentBetacam;
			*analogInBlackLevel = NTV2_Black0IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_480iADCComponentSMPTEJapan:
			*analogInStandard = NTV2_STANDARD_525;
			*analogInType = NTV2_AnlgComponentSMPTE;
			*analogInBlackLevel = NTV2_Black0IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_480iADCSVideoJapan:
			*analogInStandard = NTV2_STANDARD_525;
			*analogInType = NTV2_AnlgSVideo;
			*analogInBlackLevel = NTV2_Black0IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_480iADCCompositeJapan:
			*analogInStandard = NTV2_STANDARD_525;
			*analogInType = NTV2_AnlgComposite;
			*analogInBlackLevel = NTV2_Black0IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_576iADCComponentBeta:
			*analogInStandard = NTV2_STANDARD_625;
			*analogInType = NTV2_AnlgComponentBetacam;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2500;
			break;

		case NTV2_576iADCComponentSMPTE:
			*analogInStandard = NTV2_STANDARD_625;
			*analogInType = NTV2_AnlgComponentSMPTE;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2500;
			break;

		case NTV2_576iADCSVideo:
			*analogInStandard = NTV2_STANDARD_625;
			*analogInType = NTV2_AnlgSVideo;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2500;
			break;

		case NTV2_576iADCComposite:
			*analogInStandard = NTV2_STANDARD_625;
			*analogInType = NTV2_AnlgComposite;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2500;
			break;

		case NTV2_720p_60:
			*analogInStandard = NTV2_STANDARD_720;
			*analogInType = NTV2_AnlgComponentSMPTE;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_5994;
			break;

		case NTV2_1080i_30:
			*analogInStandard = NTV2_STANDARD_1080;
			*analogInType = NTV2_AnlgComponentSMPTE;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2997;
			break;

		case NTV2_720p_50:
			*analogInStandard = NTV2_STANDARD_720;
			*analogInType = NTV2_AnlgComponentSMPTE;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_5000;
			break;

		case NTV2_1080i_25:
			*analogInStandard = NTV2_STANDARD_1080;
			*analogInType = NTV2_AnlgComponentSMPTE;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2500;
			break;
			
		case NTV2_1080pSF24:
			*analogInStandard = NTV2_STANDARD_1080;
			*analogInType = NTV2_AnlgComponentSMPTE;
			*analogInBlackLevel = NTV2_Black75IRE;
			*analogInFrameRate = NTV2_FRAMERATE_2400;
			break;
		
		default:
			break;
	}
	return true;
}


NTV2Standard DeviceServices::GetAnalogInputVideoStandard()
{
	NTV2Standard standard = NTV2_NUM_STANDARDS;
	uint32_t status;
	
	if ( mCard->ReadRegister(kRegAnalogInputStatus, &status) == true )
	{
		if ( (status & kRegMaskInputStatusLock) != 0 )
			standard =  (NTV2Standard) ((status & kRegMaskInputStatusStd) >> kRegShiftInputStatusStd);
	} 
	return standard;
}

NTV2FrameRate DeviceServices::GetAnalogInputVideoFrameRate()
{
	NTV2FrameRate frameRate = NTV2_FRAMERATE_UNKNOWN;
	uint32_t status;
	
	if ( mCard->ReadRegister(kRegAnalogInputStatus, &status) == true )
	{
		if ( (status & kRegMaskInputStatusLock) != 0 )
			frameRate =  (NTV2FrameRate) ((status & kRegMaskInputStatusFPS) >> kRegShiftInputStatusFPS);
	} 
	return frameRate;
}


bool DeviceServices::SetVideoADCMode (NTV2LSVideoADCMode value)
{
	// this is handled by firmware
	return mCard->WriteRegister (kRegAnalogInputControl, value, kRegMaskAnalogInputADCMode, kRegShiftAnalogInputADCMode);
}


NTV2LSVideoADCMode DeviceServices::GetVideoADCMode ()
{
	uint32_t value = (uint32_t) NTV2_480iADCSVideoUS;
	mCard->ReadRegister (kRegAnalogInputControl, &value, kRegMaskAnalogInputADCMode, kRegShiftAnalogInputADCMode);
	return (NTV2LSVideoADCMode) value;
}


NTV2VideoFormat DeviceServices::GetNTV2VideoFormat(NTV2FrameRate frameRate, NTV2Standard standard, bool isThreeG, UByte inputGeometry, bool progressivePicture)
{
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;

	switch ( standard )
	{
	case NTV2_STANDARD_525:
		switch ( frameRate )
		{
		case NTV2_FRAMERATE_2997:
			videoFormat = progressivePicture ? NTV2_FORMAT_525psf_2997 : NTV2_FORMAT_525_5994;
			break;
		case NTV2_FRAMERATE_2400:
			videoFormat = NTV2_FORMAT_525_2400;
			break;
		case NTV2_FRAMERATE_2398:
			videoFormat = NTV2_FORMAT_525_2398;
			break;
		default:
			break;
		}
		break;
	
	case NTV2_STANDARD_625:
		if ( frameRate == NTV2_FRAMERATE_2500 )
			videoFormat = progressivePicture ? NTV2_FORMAT_625psf_2500 : NTV2_FORMAT_625_5000;
		break;

	case NTV2_STANDARD_720:
		switch ( frameRate )
		{
		case NTV2_FRAMERATE_6000:
			videoFormat = NTV2_FORMAT_720p_6000;
			break;
		case NTV2_FRAMERATE_5994:
			videoFormat = NTV2_FORMAT_720p_5994;
			break;
		case NTV2_FRAMERATE_5000:
			videoFormat = NTV2_FORMAT_720p_5000;
			break;
		case NTV2_FRAMERATE_2398:
			videoFormat = NTV2_FORMAT_720p_2398;
			break;
		default:
			break;
		}
		break;
		
	case NTV2_STANDARD_1080:
		switch ( frameRate )
		{
		case NTV2_FRAMERATE_3000:
			if(isThreeG)
				videoFormat = NTV2_FORMAT_1080p_6000_B;
			else
				videoFormat = progressivePicture ? NTV2_FORMAT_1080psf_3000_2 : NTV2_FORMAT_1080i_6000;
			break;
		case NTV2_FRAMERATE_2997:
			if(isThreeG)
				videoFormat = NTV2_FORMAT_1080p_5994_B;
			else
				videoFormat = progressivePicture ? NTV2_FORMAT_1080psf_2997_2 : NTV2_FORMAT_1080i_5994;
			break;
		case NTV2_FRAMERATE_2500:
			if(isThreeG)
				videoFormat = NTV2_FORMAT_1080p_5000_B;
			else if (inputGeometry == 8)
				videoFormat = NTV2_FORMAT_1080psf_2K_2500;
			else
				 videoFormat = progressivePicture ? NTV2_FORMAT_1080psf_2500_2 : NTV2_FORMAT_1080i_5000;
			break;
		case NTV2_FRAMERATE_2400:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080psf_2K_2400 : NTV2_FORMAT_1080psf_2400;
			break;
		case NTV2_FRAMERATE_2398:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080psf_2K_2398 : NTV2_FORMAT_1080psf_2398;
			break;
		default:
			break;	// Unsupported
		}
		break;
		
	case NTV2_STANDARD_1080p:
		switch ( frameRate )
		{
		case NTV2_FRAMERATE_6000:
				videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_6000_A : NTV2_FORMAT_1080p_6000_A;
			break;
		case NTV2_FRAMERATE_5994:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_5994_A : NTV2_FORMAT_1080p_5994_A;
			break;
		case NTV2_FRAMERATE_5000:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_5000_A : NTV2_FORMAT_1080p_5000_A;
			break;
		case NTV2_FRAMERATE_3000:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_3000 : NTV2_FORMAT_1080p_3000;
			break;
		case NTV2_FRAMERATE_2997:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_2997 : NTV2_FORMAT_1080p_2997;
			break;
		case NTV2_FRAMERATE_2500:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_2500 : NTV2_FORMAT_1080p_2500;
			break;
		case NTV2_FRAMERATE_2400:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_2400 : NTV2_FORMAT_1080p_2400;
			break;
		case NTV2_FRAMERATE_2398:
			videoFormat = inputGeometry == 8 ? NTV2_FORMAT_1080p_2K_2398 : NTV2_FORMAT_1080p_2398;
			break;
		default:
			break;	// Unsupported
		}
		break;
		
	case NTV2_STANDARD_2K:		// 2kx1556
		switch ( frameRate )
		{
		case NTV2_FRAMERATE_1500:
			videoFormat = NTV2_FORMAT_2K_1500;
			break;
		case NTV2_FRAMERATE_1498:
			videoFormat = NTV2_FORMAT_2K_1498;
			break;
		case NTV2_FRAMERATE_2400:
			videoFormat = NTV2_FORMAT_2K_2400;
			break;
		case NTV2_FRAMERATE_2398:
			videoFormat = NTV2_FORMAT_2K_2398;
			break;
		case NTV2_FRAMERATE_2500:
			videoFormat = NTV2_FORMAT_2K_2500;
		default:
			break;	// Unsupported
		}
		break;
		
	default:
		break;
	}

	return videoFormat;
}


NTV2VideoDACMode DeviceServices::GetVideoDACModeForVideoFormat(NTV2VideoFormat format, NTV2AnalogType type, NTV2AnalogBlackLevel blackLevel)
{
	NTV2VideoDACMode mode = NTV2_480iYPbPrSMPTE;
	
	switch (format)
	{
		case NTV2_FORMAT_525_5994:
		case NTV2_FORMAT_525psf_2997:
			if ( (type == NTV2_AnlgComposite) && (blackLevel == NTV2_Black75IRE) )
				mode = NTV2_480iNTSC_US_Composite;
			else if ( (type == NTV2_AnlgComposite) && (blackLevel == NTV2_Black0IRE) )
				mode = NTV2_480iNTSC_Japan_Composite;
			else if ( (type == NTV2_AnlgComponentBetacam) && (blackLevel == NTV2_Black75IRE) )
				mode = NTV2_480iYPbPrBetacam525;
			else if ( (type == NTV2_AnlgComponentBetacam) && (blackLevel == NTV2_Black0IRE) )
				mode = NTV2_480iYPbPrBetacamJapan;
			else if (type == NTV2_AnlgComponentRGB)
				mode = NTV2_480iRGB;
			else
				mode = NTV2_480iYPbPrSMPTE;
			break;
		
		case NTV2_FORMAT_625_5000:
		case NTV2_FORMAT_625psf_2500:
			if (type == NTV2_AnlgComposite)
				mode = NTV2_576iPAL_Composite;
			else if (type == NTV2_AnlgComponentRGB)
				mode = NTV2_576iRGB;
			else
				mode = NTV2_576iYPbPrSMPTE;
			break;
				
		case NTV2_FORMAT_720p_5000:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
				// theoretically we'll only be asked to do a component analog mode...
			if (type == NTV2_AnlgComponentRGB)
				mode = NTV2_720pRGB;
			else if (type == NTV2_AnlgXVGA)
				mode = NTV2_720pXVGA;
			else
				mode = NTV2_720pSMPTE;
			break;
		
		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_1080i_5994:
		case NTV2_FORMAT_1080i_6000:
		case NTV2_FORMAT_1080psf_2500_2:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
				// theoretically we'll only be asked to do a component analog mode...
			if (type == NTV2_AnlgComponentRGB)
				mode = NTV2_1080iRGB;
			else if (type == NTV2_AnlgXVGA)
				mode = NTV2_1080iXVGA;
			else
				mode = NTV2_1080iSMPTE;
			break;
		
		case NTV2_FORMAT_1080p_2997:
		case NTV2_FORMAT_1080p_3000:
		case NTV2_FORMAT_1080p_2500:
		case NTV2_FORMAT_1080p_2398:
		case NTV2_FORMAT_1080p_2400:
		case NTV2_FORMAT_1080p_5000_B:
		case NTV2_FORMAT_1080p_5994_B:
		case NTV2_FORMAT_1080p_6000_B:
			// NOTE: p formats display garbage on analog - but we set them to something close
		case NTV2_FORMAT_1080psf_2398:
		case NTV2_FORMAT_1080psf_2400:
				// theoretically we'll only be asked to do a component analog mode...
			if (type == NTV2_AnlgComponentRGB)
				mode = NTV2_1080psfRGB;
			else if (type == NTV2_AnlgXVGA)
				mode = NTV2_1080psfXVGA;
			else
				mode = NTV2_1080psfSMPTE;
			break;
			
		case NTV2_FORMAT_1080p_2K_2398:
		case NTV2_FORMAT_1080p_2K_2400:		
		case NTV2_FORMAT_1080p_2K_2500:		
			// NOTE: p formats display garbage on analog - but we set them to something close
		case NTV2_FORMAT_1080psf_2K_2398:
		case NTV2_FORMAT_1080psf_2K_2400:
		case NTV2_FORMAT_1080psf_2K_2500:
				// theoretically we'll only be asked to do a component analog mode...
			if (type == NTV2_AnlgComponentRGB)
				mode = NTV2_2Kx1080RGB;
			else if (type == NTV2_AnlgXVGA)
				mode = NTV2_2Kx1080XVGA;
			else
				mode = NTV2_2Kx1080SMPTE;
			break;
		
		case NTV2_FORMAT_1080p_5000_A:
		case NTV2_FORMAT_1080p_5994_A:
		case NTV2_FORMAT_1080p_6000_A:
		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:
			break; // not supported
		
		default:	break;
	}
	
	return mode;
}


NTV2VideoFormat DeviceServices::GetPairedInterlaceTransportFormat(NTV2VideoFormat format)
{
	NTV2VideoFormat pairedFmt = format;

	switch (format)
	{
		case NTV2_FORMAT_525_5994:
			pairedFmt = NTV2_FORMAT_525psf_2997;
			break;
		case NTV2_FORMAT_525psf_2997:
			pairedFmt = NTV2_FORMAT_525_5994;
			break;
		case NTV2_FORMAT_625_5000:
			pairedFmt = NTV2_FORMAT_625psf_2500;
			break;
		case NTV2_FORMAT_625psf_2500:
			pairedFmt = NTV2_FORMAT_625_5000;
			break;
		case NTV2_FORMAT_1080i_5000:
			pairedFmt = NTV2_FORMAT_1080psf_2500_2;
			break;
		case NTV2_FORMAT_1080psf_2500_2:
			pairedFmt = NTV2_FORMAT_1080i_5000;
			break;
		case NTV2_FORMAT_1080i_5994:
			pairedFmt = NTV2_FORMAT_1080psf_2997_2;
			break;
		case NTV2_FORMAT_1080psf_2997_2:
			pairedFmt = NTV2_FORMAT_1080i_5994;
			break;
		case NTV2_FORMAT_1080i_6000:
			pairedFmt = NTV2_FORMAT_1080psf_3000_2;
			break;
		case NTV2_FORMAT_1080psf_3000_2:
			pairedFmt = NTV2_FORMAT_1080i_6000;
			break;
		default:
			break;
	}
	
	return pairedFmt;
}


bool DeviceServices::CanConvertFormat(NTV2VideoFormat inFormat, NTV2VideoFormat outFormat)
{
	// can always convert to the same format
	if (inFormat == outFormat)
		return true;

	bool bResult = false;

	NTV2ConversionMode cMode = GetConversionMode(inFormat, outFormat);
	if (cMode != NTV2_CONVERSIONMODE_UNKNOWN)
	{
		NTV2DeviceID	deviceID = mCard->GetDeviceID();
		bResult = ::NTV2DeviceCanDoConversionMode(deviceID, cMode);
	}
	
	return bResult;
}



// Return convertible format if UFC is available, otherwise return same fmt
// Specifically this will choose between 1080i vs 1080psf for rates 2500, 2997, 3000.
NTV2VideoFormat DeviceServices::GetConversionCompatibleFormat(NTV2VideoFormat sourceFmt,
														      NTV2VideoFormat secondaryFmt)
{
	NTV2VideoFormat convertedFormat = sourceFmt;
	if (CanConvertFormat(sourceFmt, secondaryFmt) == false)
	{
		sourceFmt = GetPairedInterlaceTransportFormat(sourceFmt);
		if (CanConvertFormat(sourceFmt, secondaryFmt))
			convertedFormat = sourceFmt;
	}
		
	return convertedFormat;
}


// use vpid to determine sdi input video format.
// If no VPID for SDI input index, return NTV2_FORMAT_UNKNOWN
NTV2VideoFormat DeviceServices::GetSdiInVideoFormatWithVpid(int32_t index)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	ULWord vpida = 0;
	ULWord vpidb = 0;

	if (mCard->ReadSDIInVPID((NTV2Channel)index, vpida, vpidb))
	{
		// if there is a vpid - use it to determine format
		if (vpida != 0)
		{
			CNTV2VPID parser;
			parser.SetVPID(vpida);
			inputFormat = parser.GetVideoFormat();

			if (mVirtualInputSelect == NTV2_DualLink4xSdi4k || mVirtualInputSelect == NTV2_DualLink2xSdi4k)
			{
				inputFormat = GetQuadSizedVideoFormat(inputFormat);
			}
		}
	}
	return inputFormat;
}


// NOTES:
// videoFormat is a reference format, typically the CH1 frame buffer video format
// special/alternate support for LevelB HFR input types
// switch to LevelA transport format, use 3Gb flag as LevelB indicator
NTV2VideoFormat DeviceServices::GetSdiInVideoFormat(int32_t index, NTV2VideoFormat videoFormat)
{
	// start by trying to use VPID
	NTV2VideoFormat sdiInFormat = GetSdiInVideoFormatWithVpid(index);
	if (sdiInFormat != NTV2_FORMAT_UNKNOWN)
		return sdiInFormat;

	//
	// no valid VPID found, now guess using context
	//

	// if follow input, preference non-progressive picture option
	bool progressivePicture = mFollowInputFormat ? false : IsProgressiveTransport(videoFormat);
	sdiInFormat = mCard->GetSDIInputVideoFormat((NTV2Channel)index, progressivePicture);
	
	// HACK NOTICE 1:
	bool b4kHfr = NTV2_IS_4K_HFR_VIDEO_FORMAT(videoFormat);
	if (sdiInFormat == NTV2_FORMAT_1080psf_2K_2500 && b4kHfr == true)
		sdiInFormat = NTV2_FORMAT_1080p_5000_B;
	
	// HACK NOTICE 2
	sdiInFormat = GetTransportCompatibleFormat(sdiInFormat, videoFormat);
	NTV2DeviceID deviceID = mCard->GetDeviceID();
	if (sdiInFormat != videoFormat && ::NTV2DeviceGetNumInputConverters(deviceID) > 0)
		sdiInFormat = GetConversionCompatibleFormat(sdiInFormat, mVirtualSecondaryFormatSelect);
	
	// HACK NOTICE 3
	// note: there is no enum for 2Kp60b et al
	// we special case define 2Kp60b as 2Kp60a with 3Gb flag set
	bool b1080pHfr = NTV2_IS_3Gb_FORMAT(sdiInFormat);		// i.e. 1080p60b hfrs
	if (b1080pHfr && b4kHfr)
	{
		ULWord status;
		uint32_t geometry = 0;
		if (index == 0)
		{
			mCard->ReadInputStatusRegister(&status);
			geometry = ((status>>27)&BIT_3)|((status>>4)&0x7);
		}
		else if (index == 1)
		{
			mCard->ReadInputStatusRegister(&status);
			geometry = ((status>>28)&BIT_3)|((status>>12)&0x7);
		}
		else if (index == 2)
		{
			mCard->ReadInputStatus2Register(&status);
			geometry = ((status>>27)&BIT_3)|((status>>4)&0x7);
		}
		else if (index == 3)
		{
			mCard->ReadInputStatus2Register(&status);
			geometry = ((status>>28)&BIT_3)|((status>>12)&0x7);
		}
		
		// switch to LevelA transport format, use 3Gb flag as LevelB indicator
		if (sdiInFormat == NTV2_FORMAT_1080p_5000_B)
			sdiInFormat = geometry == 8 ? NTV2_FORMAT_1080p_2K_5000 : NTV2_FORMAT_1080p_5000_A;
		else if (sdiInFormat == NTV2_FORMAT_1080p_5994_B)
			sdiInFormat = geometry == 8 ? NTV2_FORMAT_1080p_2K_5994 : NTV2_FORMAT_1080p_5994_A;
		else if (sdiInFormat == NTV2_FORMAT_1080p_6000_B)
			sdiInFormat = geometry == 8 ? NTV2_FORMAT_1080p_2K_6000 : NTV2_FORMAT_1080p_6000_A;
	}
	
	return sdiInFormat;
}


NTV2FrameRate DeviceServices::HalfFrameRate(NTV2FrameRate rate)
{
	NTV2FrameRate halfRate;
	
	switch (rate)
	{
	case NTV2_FRAMERATE_6000:
		halfRate = NTV2_FRAMERATE_3000; break;
	case NTV2_FRAMERATE_5994:
		halfRate = NTV2_FRAMERATE_2997; break;
	case NTV2_FRAMERATE_5000:
		halfRate = NTV2_FRAMERATE_2500; break;	
	case NTV2_FRAMERATE_4800:
		halfRate = NTV2_FRAMERATE_2400; break;	
	case NTV2_FRAMERATE_4795:
		halfRate = NTV2_FRAMERATE_2398; break;	
	default:
		halfRate = rate; break;
	}

	return halfRate;
}


// MARK: -

// based on the user ColorSpace mode and the current video format,
// set the ColorSpaceMatrixSelect
bool DeviceServices::UpdateK2ColorSpaceMatrixSelect()
{
	bool bResult = true;
	NTV2DeviceID	deviceID = mCard->GetDeviceID();
	
	// if the board doesn't have LUTs, bail
	if ( !::NTV2DeviceCanDoProgrammableCSC(deviceID) )
		return bResult;

	// figure out what ColorSpace we want to be using
	NTV2ColorSpaceMatrixType matrix = NTV2_Rec709Matrix;
	switch (mColorSpaceMode)
	{
		// force to Rec 601
		default:	
		case NTV2_ColorSpaceTypeRec601:
			matrix = NTV2_Rec601Matrix;
			break;
		
		// force to Rec 709
		case NTV2_ColorSpaceTypeRec709:
			matrix = NTV2_Rec709Matrix;
			break;
		
		// Auto-switch between SD (Rec 601) and HD (Rec 709)
		case NTV2_ColorSpaceTypeAuto:
			if (NTV2_IS_SD_VIDEO_FORMAT(mFb1VideoFormat) )
				matrix = NTV2_Rec601Matrix;
			else
				matrix = NTV2_Rec709Matrix;
			break;
	}
	
	// set matrix
	int numberCSCs = ::NTV2DeviceGetNumCSCs(deviceID);
	
	bResult = mCard->SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL1);
	if (numberCSCs >= 2)
		bResult = mCard->SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL2);
	if (numberCSCs >= 3)
		bResult = mCard->SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL3);
	if (numberCSCs >= 4)
		bResult = mCard->SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL4);
	if (numberCSCs >= 5)
		bResult = mCard->SetColorSpaceMatrixSelect(matrix, NTV2_CHANNEL5);
	
	
	NTV2RGB10Range cscRange = GetCSCRange();
	
	// set csc rgb range
	bResult = mCard->SetColorSpaceRGBBlackRange((NTV2RGBBlackRange)cscRange, NTV2_CHANNEL1); 
	if (numberCSCs >= 2)
		bResult = mCard->SetColorSpaceRGBBlackRange((NTV2RGBBlackRange)cscRange, NTV2_CHANNEL2);
	if (numberCSCs >= 3)
		bResult = mCard->SetColorSpaceRGBBlackRange((NTV2RGBBlackRange)cscRange, NTV2_CHANNEL3);
	if (numberCSCs >= 4)
		bResult = mCard->SetColorSpaceRGBBlackRange((NTV2RGBBlackRange)cscRange, NTV2_CHANNEL4);
	if (numberCSCs >= 5)
		bResult = mCard->SetColorSpaceRGBBlackRange((NTV2RGBBlackRange)cscRange, NTV2_CHANNEL5);
	
	return bResult;
}


// based on the user Gamma mode, the current video format,
// load the LUTs with an appropriate gamma translation
bool DeviceServices::UpdateK2LUTSelect()
{
	bool bResult = true;
	NTV2DeviceID	deviceID = mCard->GetDeviceID();
	
	NTV2FrameBufferFormat frameBufferFormat;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &frameBufferFormat);
	GeneralFrameFormat genFrameFormat = GetGeneralFrameFormat(frameBufferFormat);
	NTV2Mode mode = GetCh1Mode();

	// if the board doesn't have LUTs, bail
	if ( !::NTV2DeviceCanDoColorCorrection(deviceID) )
		return bResult;
	
	NTV2RGB10Range cscRange = GetCSCRange();

	// figure out which gamma LUTs we WANT to be using
	NTV2LutType wantedLUT = NTV2_LUTUnknown;
	switch (mGammaMode)
	{
		// force to Rec 601
		case NTV2_GammaRec601:		
			wantedLUT = (cscRange == NTV2_RGB10RangeFull) ? NTV2_LUTGamma18_Rec601 : NTV2_LUTGamma18_Rec601_SMPTE;	
			break;
	
		// force to Rec 709
		case NTV2_GammaRec709:		
			wantedLUT = (cscRange == NTV2_RGB10RangeFull) ? NTV2_LUTGamma18_Rec709 : NTV2_LUTGamma18_Rec709_SMPTE;	
			break;
	
		// Auto-switch between SD (Rec 601) and HD (Rec 709)
		case NTV2_GammaAuto:		
			if (NTV2_IS_SD_VIDEO_FORMAT(mFb1VideoFormat) )
				wantedLUT = (cscRange == NTV2_RGB10RangeFull) ? NTV2_LUTGamma18_Rec601 : NTV2_LUTGamma18_Rec601_SMPTE;
			else
				wantedLUT = (cscRange == NTV2_RGB10RangeFull) ? NTV2_LUTGamma18_Rec709 : NTV2_LUTGamma18_Rec709_SMPTE;
			break;
				
		// custom LUT in use - do not change
		case NTV2_GammaNone:		
			wantedLUT = NTV2_LUTCustom;
			break;
								
		default:
		case NTV2_GammaMac:			
			wantedLUT = NTV2_LUTLinear;
			break;
	}
	
	// special case for RGB-to-RGB LUT conversion
	if (::NTV2DeviceCanDoDualLink(deviceID) && wantedLUT != NTV2_LUTCustom)
	{
		// convert to NTV2RGB10Range to NTV2RGBRangeMode to do the comparison
		NTV2RGBRangeMode fbRange = (mRGB10Range == NTV2_RGB10RangeFull) ? NTV2_RGBRangeFull : NTV2_RGBRangeSMPTE;
	
		if (mode == NTV2_MODE_DISPLAY && genFrameFormat == FORMAT_RGB && mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
		{
			wantedLUT = (fbRange == mSDIOutput1RGBRange) ? NTV2_LUTLinear : NTV2_LUTRGBRangeFull_SMPTE;
		}
		
		else if (mode == NTV2_MODE_CAPTURE && genFrameFormat == FORMAT_RGB && mSDIInput1FormatSelect == NTV2_RGBSelect)
		{
			wantedLUT = NTV2_LUTRGBRangeFull_SMPTE;
		}
	}

	// Note some devices can to LUT5 but not LUT4 (e.g. Io4K UFC)
	bool bLut2 = ::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT2);
	bool bLut3 = ::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT3);
	bool bLut4 = ::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT4);
	bool bLut5 = ::NTV2DeviceCanDoWidget(deviceID, NTV2_WgtLUT5);
	
	ULWord lut2Type = mLUTType, lut3Type = mLUTType, lut4Type = mLUTType, lut5Type = mLUTType;
	if (bLut2)
		mCard->ReadRegister(kVRegLUT2Type, &lut2Type);
	if (bLut3)
		mCard->ReadRegister(kVRegLUT3Type, &lut3Type);
	if (bLut4)
		mCard->ReadRegister(kVRegLUT4Type, &lut4Type);
	if (bLut5)
		mCard->ReadRegister(kVRegLUT5Type, &lut5Type);
	
	// test for special use of LUT2 for E-to-E rgb range conversion
	bool bE2ERangeConversion = (bLut2 == true) &&  (NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat) == false) && (mode == NTV2_MODE_CAPTURE);
	
	// what LUT function is CURRENTLY loaded into hardware?
	if ((wantedLUT != NTV2_LUTCustom) &&
		((wantedLUT != mLUTType) || (wantedLUT != lut2Type && bE2ERangeConversion == false) ||
		 (wantedLUT != lut3Type) || (wantedLUT != lut4Type) || (wantedLUT != lut4Type)))
	{
		// generate and download new LUTs
		//printf (" Changing from LUT %d to LUT %d  \n", mLUTType, wantedLUT);
		double table[1024];
	
		// generate RGB=>YUV LUT function 
		mCard->GenerateGammaTable(wantedLUT, kLUTBank_RGB2YUV, table);
	
		// download it to HW
		mCard->DownloadLUTToHW(table, NTV2_CHANNEL1, kLUTBank_RGB2YUV);
		if (bLut2 == true && wantedLUT != lut2Type && bE2ERangeConversion == false)
			mCard->DownloadLUTToHW(table, NTV2_CHANNEL2, kLUTBank_RGB2YUV);
		if (bLut3 == true && wantedLUT != lut3Type)
			mCard->DownloadLUTToHW(table, NTV2_CHANNEL3, kLUTBank_RGB2YUV);
		if (bLut4 == true && wantedLUT != lut4Type)
			mCard->DownloadLUTToHW(table, NTV2_CHANNEL4, kLUTBank_RGB2YUV);
		if (bLut5 == true && wantedLUT != lut5Type)
			mCard->DownloadLUTToHW(table, NTV2_CHANNEL5, kLUTBank_RGB2YUV);

		// generate YUV=>RGB LUT function
		mCard->GenerateGammaTable(wantedLUT, kLUTBank_YUV2RGB, table);
		
		// download it to HW
		mCard->DownloadLUTToHW(table, NTV2_CHANNEL1, kLUTBank_YUV2RGB);
		if (bLut2 == true && wantedLUT != lut2Type && bE2ERangeConversion == false)
			mCard->DownloadLUTToHW(table, NTV2_CHANNEL2, kLUTBank_YUV2RGB);
		if (bLut3 == true && wantedLUT != lut3Type)
			mCard->DownloadLUTToHW(table, NTV2_CHANNEL3, kLUTBank_YUV2RGB);
		if (bLut4 == true && wantedLUT != lut4Type)
			mCard->DownloadLUTToHW(table, NTV2_CHANNEL4, kLUTBank_YUV2RGB);
		if (bLut5 == true && wantedLUT != lut5Type)
			mCard->DownloadLUTToHW(table, NTV2_CHANNEL5, kLUTBank_YUV2RGB);
	}
	
	// special use of LUT2 for E-to-E rgb range conversion
	if (bLut2 == true && bE2ERangeConversion == true && lut2Type != NTV2_LUTRGBRangeFull_SMPTE)
	{
		double table[1024];
	
		mCard->GenerateGammaTable(NTV2_LUTRGBRangeFull_SMPTE, kLUTBank_SMPTE2FULL, table);
		mCard->DownloadLUTToHW(table, NTV2_CHANNEL2, kLUTBank_SMPTE2FULL);
		mCard->GenerateGammaTable(NTV2_LUTRGBRangeFull_SMPTE, kLUTBank_FULL2SMPTE, table);
		mCard->DownloadLUTToHW(table, NTV2_CHANNEL2, kLUTBank_FULL2SMPTE);
		mCard->WriteRegister(kVRegLUT2Type, NTV2_LUTRGBRangeFull_SMPTE);
	}
	
	// write LUT type reg
	mCard->WriteRegister(kVRegLUTType, wantedLUT);
	if (bLut2 && bE2ERangeConversion == false)
		mCard->WriteRegister(kVRegLUT2Type, wantedLUT);
	if (bLut3)
		mCard->WriteRegister(kVRegLUT3Type, wantedLUT);
	if (bLut4)
		mCard->WriteRegister(kVRegLUT4Type, wantedLUT);
	if (bLut5)
		mCard->WriteRegister(kVRegLUT5Type, wantedLUT);
	
	return bResult;
}

NTV2FrameRate DeviceServices::GetInput2VideoFrameRate()
{
	ULWord status;
	NTV2FrameRate result;
	
	mCard->ReadRegister(kRegInputStatus, &status);

	result = (NTV2FrameRate)( ((status >> 26) & BIT_3) | ((status >> 8) & 0x7) );
	
	// Kludge because KLH doesn't always distinguish between 59.94 and 60 or 29.97 and 30 correctly
	if (result == NTV2_FRAMERATE_6000)
		result = NTV2_FRAMERATE_5994;
	else if (result == NTV2_FRAMERATE_3000)
		result = NTV2_FRAMERATE_2997;
		
	return result;
}


void DeviceServices::SetForegroundVideoCrosspoint(NTV2Crosspoint crosspoint)
{
	ULWord regValue = crosspoint;
	mCard->WriteRegister (kRegVidProcXptControl, regValue, FGVCROSSPOINTMASK, FGVCROSSPOINTSHIFT);
}


void DeviceServices::SetForegroundKeyCrosspoint(NTV2Crosspoint crosspoint)
{
	ULWord regValue = crosspoint;
	mCard->WriteRegister (kRegVidProcXptControl, regValue, FGKCROSSPOINTMASK, FGKCROSSPOINTSHIFT);
}

void DeviceServices::SetBackgroundVideoCrosspoint(NTV2Crosspoint crosspoint)
{
	ULWord regValue = crosspoint;
	mCard->WriteRegister (kRegVidProcXptControl, regValue, BGVCROSSPOINTMASK, BGVCROSSPOINTSHIFT);
}


// enable RP188 E-E passthrough based on route
void DeviceServices::EnableRP188EtoE(NTV2WidgetID fromInputWgt, NTV2WidgetID toOutputWgt)
{
	ULWord passthroughInput = 0x0;
	// enable RP188 output
	// enable RP188 bypass (E-E mode)
	// disabled DBB filtering
	// select source
	if(fromInputWgt == NTV2_WgtSDIIn1 || fromInputWgt == NTV2_Wgt3GSDIIn1)
	{
		passthroughInput = 0x0;
		switch(toOutputWgt)
		{
		case NTV2_WgtSDIOut1:
		case NTV2_Wgt3GSDIOut1:
			mCard->WriteRegister (kRegGlobalControl, 0x1, kRegMaskRP188ModeCh1, kRegShiftRP188ModeCh1);
			mCard->WriteRegister (kRegRP188InOut1DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut1DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut1DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut1DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_WgtSDIOut2:
		case NTV2_Wgt3GSDIOut2:
			mCard->WriteRegister (kRegGlobalControl, 0x1, kRegMaskRP188ModeCh2, kRegShiftRP188ModeCh2);
			mCard->WriteRegister (kRegRP188InOut2DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut2DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut2DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut2DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_WgtSDIOut3:
		case NTV2_Wgt3GSDIOut3:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh3, kRegShiftRP188ModeCh3);
			mCard->WriteRegister (kRegRP188InOut3DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut3DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut3DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut3DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_WgtSDIOut4:
		case NTV2_Wgt3GSDIOut4:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh4, kRegShiftRP188ModeCh4);
			mCard->WriteRegister (kRegRP188InOut4DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut4DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut4DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut4DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_Wgt3GSDIOut5:
		case NTV2_WgtSDIMonOut1:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh5, kRegShiftRP188ModeCh5);
			mCard->WriteRegister (kRegRP188InOut5DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut5DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut5DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut5DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_Wgt3GSDIOut6:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh6, kRegShiftRP188ModeCh6);
			mCard->WriteRegister (kRegRP188InOut6DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut6DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut6DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut6DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_Wgt3GSDIOut7:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh7, kRegShiftRP188ModeCh7);
			mCard->WriteRegister (kRegRP188InOut7DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut7DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut7DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut7DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_Wgt3GSDIOut8:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh8, kRegShiftRP188ModeCh8);
			mCard->WriteRegister (kRegRP188InOut8DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut8DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut8DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut8DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		default:
			break;
		}
	}
	else if(fromInputWgt == NTV2_WgtSDIIn2 || fromInputWgt == NTV2_Wgt3GSDIIn2)
	{
		passthroughInput = 0x2;
		switch(toOutputWgt)
		{
		case NTV2_WgtSDIOut1:
		case NTV2_Wgt3GSDIOut1:
			mCard->WriteRegister (kRegGlobalControl, 0x1, kRegMaskRP188ModeCh1, kRegShiftRP188ModeCh1);
			mCard->WriteRegister (kRegRP188InOut1DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut1DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut1DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut1DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_WgtSDIOut2:
		case NTV2_Wgt3GSDIOut2:
			mCard->WriteRegister (kRegGlobalControl, 0x1, kRegMaskRP188ModeCh2, kRegShiftRP188ModeCh2);
			mCard->WriteRegister (kRegRP188InOut2DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut2DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut2DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut2DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_WgtSDIOut3:
		case NTV2_Wgt3GSDIOut3:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh3, kRegShiftRP188ModeCh3);
			mCard->WriteRegister (kRegRP188InOut3DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut3DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut3DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut3DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_WgtSDIOut4:
		case NTV2_Wgt3GSDIOut4:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh4, kRegShiftRP188ModeCh4);
			mCard->WriteRegister (kRegRP188InOut4DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut4DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut4DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut4DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_Wgt3GSDIOut5:
		case NTV2_WgtSDIMonOut1:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh5, kRegShiftRP188ModeCh5);
			mCard->WriteRegister (kRegRP188InOut5DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut5DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut5DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut5DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_Wgt3GSDIOut6:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh6, kRegShiftRP188ModeCh6);
			mCard->WriteRegister (kRegRP188InOut6DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut6DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut6DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut6DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_Wgt3GSDIOut7:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh7, kRegShiftRP188ModeCh7);
			mCard->WriteRegister (kRegRP188InOut7DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut7DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut7DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut7DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		case NTV2_Wgt3GSDIOut8:
			mCard->WriteRegister (kRegGlobalControl2, 0x1, kRegMaskRP188ModeCh8, kRegShiftRP188ModeCh8);
			mCard->WriteRegister (kRegRP188InOut8DBB, 0x1, BIT(23), 23);
			mCard->WriteRegister (kRegRP188InOut8DBB, 0xFF, kRegMaskRP188SourceSelect, kRegShiftRP188Source);
			mCard->WriteRegister (kRegRP188InOut8DBB, passthroughInput, BIT(21)+BIT(22), 21);
			mCard->WriteRegister(kRegRP188InOut8DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
			break;
		default:
			break;
		}
	}

	//Setup Analog LTC for passthrough
	bool ltcEnabled = false, ltcPresent = false;
	mCard->GetLTCInputEnable(ltcEnabled);
	mCard->GetLTCInputPresent(ltcPresent);
	if (ltcEnabled && ltcPresent)
		mCard->WriteRegister(kRegLTCStatusControl, 0x1, kRegMaskLTC1InBypass, kRegShiftLTC1Bypass);
	else
		mCard->WriteRegister(kRegLTCStatusControl, 0x0, kRegMaskLTC1InBypass, kRegShiftLTC1Bypass);
}

// disable RP188 E-E passthrough to specified output
void DeviceServices::DisableRP188EtoE(NTV2WidgetID toOutputWgt)
{
	ULWord passthrough = 0;
	switch(toOutputWgt)
	{
	case NTV2_WgtSDIOut1:
	case NTV2_Wgt3GSDIOut1:
		mCard->ReadRegister(kRegRP188InOut1DBB, &passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister (kRegRP188InOut1DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut1DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_WgtSDIOut2:
	case NTV2_Wgt3GSDIOut2:
		mCard->ReadRegister(kRegRP188InOut2DBB, &passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister (kRegRP188InOut2DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut2DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_WgtSDIOut3:
	case NTV2_Wgt3GSDIOut3:
		mCard->ReadRegister(kRegRP188InOut3DBB, &passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut3DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut3DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_WgtSDIOut4:
	case NTV2_Wgt3GSDIOut4:
		mCard->ReadRegister(kRegRP188InOut4DBB, &passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut4DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut4DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_Wgt3GSDIOut5:
	case NTV2_WgtSDIMonOut1:
		mCard->ReadRegister(kRegRP188InOut5DBB, &passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut5DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut5DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_Wgt3GSDIOut6:
		mCard->ReadRegister(kRegRP188InOut6DBB, &passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut6DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut6DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_Wgt3GSDIOut7:
		mCard->ReadRegister(kRegRP188InOut7DBB, &passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut7DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut7DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_Wgt3GSDIOut8:
		mCard->ReadRegister(kRegRP188InOut8DBB, &passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut8DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut8DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	default:
		break;
	}

	//Disable Analog LTC passthrough
	mCard->WriteRegister(kRegLTCStatusControl, 0x0, kRegMaskLTC1InBypass, kRegShiftLTC1Bypass);
}



#define XENA2_SEARCHTIMEOUT 5

void DeviceServices::SetVideoOutputStandard(NTV2Channel channel)
{
	NTV2Standard standard = NTV2_NUM_STANDARDS;
	NTV2VideoFormat videoFormat;
	UWord searchTimeout = 0;
    
	NTV2OutputCrosspointID xptSelect;
	mCard->GetConnectedOutput (channel == NTV2_CHANNEL1 ? NTV2_XptSDIOut1Input : NTV2_XptSDIOut2Input, xptSelect);

	do {
		switch ( xptSelect)
		{
            case NTV2_XptSDIIn1:
                videoFormat = mCard->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1);
                standard = GetNTV2StandardFromVideoFormat(videoFormat);
                if ( standard == NTV2_NUM_STANDARDS) 
                    searchTimeout = XENA2_SEARCHTIMEOUT;
                break;
                
            case NTV2_XptSDIIn2:
                videoFormat = mCard->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI2);
                standard = GetNTV2StandardFromVideoFormat(videoFormat);
                if ( standard == NTV2_NUM_STANDARDS) 
                    searchTimeout = XENA2_SEARCHTIMEOUT;
                break;
                
            case NTV2_XptFrameBuffer1YUV:
            case NTV2_XptFrameBuffer1RGB:
            case NTV2_XptFrameBuffer2YUV:
            case NTV2_XptFrameBuffer2RGB:
            case NTV2_XptBlack:
                mCard->GetVideoFormat(&videoFormat);
                standard = GetNTV2StandardFromVideoFormat(videoFormat);
                break;
                
            case NTV2_XptDuallinkIn1:
                videoFormat = mCard->GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1);
                standard = GetNTV2StandardFromVideoFormat(videoFormat);
                if ( standard == NTV2_NUM_STANDARDS) 
                    searchTimeout = XENA2_SEARCHTIMEOUT;
                break;
                
            case NTV2_XptConversionModule:
                mCard->GetConverterOutStandard(&standard);
                if ( standard >= NTV2_NUM_STANDARDS)
                {
                    standard = NTV2_NUM_STANDARDS;
                    searchTimeout = XENA2_SEARCHTIMEOUT;
                }
                break;
                
            case NTV2_XptCSC1VidRGB:
            case NTV2_XptCSC1VidYUV:
            case NTV2_XptCSC1KeyYUV:
                mCard->GetConnectedOutput (NTV2_XptCSC1VidInput, xptSelect);
                break;
                
            case NTV2_XptCompressionModule:
                mCard->GetConnectedOutput (NTV2_XptCompressionModInput, xptSelect);
                break;
                
            case NTV2_XptDuallinkOut1:     
                mCard->GetConnectedOutput (NTV2_XptDualLinkOut1Input, xptSelect);
                break;
                
            case NTV2_XptLUT1RGB:
                mCard->GetConnectedOutput (NTV2_XptLUT1Input, xptSelect);
                break;
                
            case NTV2_XptLUT2RGB:
                mCard->GetConnectedOutput (NTV2_XptLUT2Input, xptSelect);
                break;
                
            case NTV2_XptCSC2VidYUV:
            case NTV2_XptCSC2VidRGB:
            case NTV2_XptCSC2KeyYUV:
                mCard->GetConnectedOutput (NTV2_XptCSC2VidInput, xptSelect);
                break;
                
            case NTV2_XptMixer1VidYUV:
            case NTV2_XptMixer1KeyYUV:
                mCard->GetConnectedOutput (NTV2_XptMixer1FGVidInput, xptSelect);
                break;
                
            case NTV2_XptAlphaOut:
            default:
                searchTimeout = XENA2_SEARCHTIMEOUT;
                break;
                
		}
        
	} while ( (standard == NTV2_NUM_STANDARDS) && (searchTimeout++ < XENA2_SEARCHTIMEOUT));
    
	if (standard != NTV2_NUM_STANDARDS)
	{
		if (channel == NTV2_CHANNEL1)
			mCard->SetSDIOutputStandard(NTV2_CHANNEL1, standard);
		else
			mCard->SetSDIOutputStandard(NTV2_CHANNEL2, standard);
	}
} 


uint32_t DeviceServices::GetAudioDelayOffset(double frames)
{
#define BYTES_PER_UNIT	512		// each hardware click is 64 bytes
    
	NTV2FrameRate rate =  NTV2_FRAMERATE_UNKNOWN;
	mCard->GetFrameRate(&rate);
    
	double frate		   = GetFramesPerSecond(rate);
	double samplesPerFrame = 48000.0 / frate;
	ULWord channels        = 16;
	mCard->GetNumberAudioChannels(channels);

	double  bytes          = samplesPerFrame * 4 * frames * channels;
	uint32_t offset        = uint32_t(bytes / BYTES_PER_UNIT);
	
	return offset;
}


NTV2AudioSystem	DeviceServices::GetHostAudioSystem()
{
	NTV2AudioSystem hostAudioSystem = NTV2_AUDIOSYSTEM_1;
	if (mCard->DeviceCanDoAudioMixer())
	{
		NTV2DeviceID deviceID = mCard->GetDeviceID();
		uint32_t audioSystem = NTV2DeviceGetNumAudioSystems(deviceID);
		hostAudioSystem = (NTV2AudioSystem)audioSystem;
	}
	return hostAudioSystem;
}


//
// MARK: Base Service -
//

void DeviceServices::SetDeviceXPointCapture( GeneralFrameFormat format )
{
	(void) format;
	NTV2DeviceID deviceID = mCard->GetDeviceID();

	mCard->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_ON, NTV2_AUDIOSYSTEM_1);

	bool b4K = NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	bool hasBiDirectionalSDI = NTV2DeviceHasBiDirectionalSDI(deviceID);
	
	NTV2WidgetID inputSelectID = NTV2_Wgt3GSDIIn1;
	if(mVirtualInputSelect == NTV2_Input2Select)
		inputSelectID = NTV2_Wgt3GSDIIn2;

	if ((deviceID != DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) &&
		(deviceID != DEVICE_ID_KONAIP_2TX_1SFP_J2K) &&
		(deviceID != DEVICE_ID_KONAIP_2RX_1SFP_J2K))
	{
		uint32_t audioInputSelect;
		mCard->ReadRegister(kVRegAudioInputSelect, &audioInputSelect);
		SetAudioInputSelect((NTV2InputAudioSelect)audioInputSelect);

		// The reference (genlock) source: if it's a video input, make sure it matches our current selection
		switch (mCaptureReferenceSelect)
		{
		default:
		case kFreeRun:
			mCard->SetReference(NTV2_REFERENCE_FREERUN);
			break;
		case kReferenceIn:
			mCard->SetReference(NTV2_REFERENCE_EXTERNAL);
			break;
		case kVideoIn:
			switch (mVirtualInputSelect)
			{
			default:
			case NTV2_Input1Select:
				mCard->SetReference(NTV2_REFERENCE_INPUT1);
				break;
			case NTV2_Input2Select:
				switch(deviceID)
				{
				case DEVICE_ID_LHI:
				case DEVICE_ID_IOEXPRESS:
					mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
					break;
				case DEVICE_ID_LHE_PLUS:
					mCard->SetReference(NTV2_REFERENCE_ANALOG_INPUT);
					break;
				default:
					mCard->SetReference(NTV2_REFERENCE_INPUT2);
					break;
				}
				break;
			case NTV2_Input3Select:
				{
					switch(deviceID)
					{
					case DEVICE_ID_LHI:
						mCard->SetReference(NTV2_REFERENCE_ANALOG_INPUT);
						break;
					case DEVICE_ID_IO4KUFC:
					case DEVICE_ID_IOXT:
					default:
						mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
						break;
					}
				}
				break;
			case NTV2_Input5Select:
				{
					switch(deviceID)
					{
					default:
					case DEVICE_ID_IO4KPLUS:
					case DEVICE_ID_IO4K:
						mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
						break;
					}
				}
				break;
			}
		}
	}
	// For J2K devices we don't set the audio input select reg, audio input
	// has to come from AES and the configuration code will set this properly
	// we also force the reference on input to NTV2_REFERENCE_SFP1_PCR
	else
	{
		mCard->SetReference(NTV2_REFERENCE_SFP1_PCR);
	}

	if(!b4K)//if we are 4k all connections are inputs
	{
		//Following the logic from each individual file
		//this should cover almost all cases

		//Fall through so every output is setup
		//1 and 2 get set if they are not bi-directional
		//otherwise 1 and 2 are direction input
		switch(NTV2DeviceGetNumVideoInputs(deviceID))
		{
		case 8:
			EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut8);
		case 7:
			EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut7);
		case 6:
			EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut6);
		case 5:
			EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut5);
		case 4:
			EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut4);
		case 3:
			EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut3);
		case 2:
			if(!hasBiDirectionalSDI)
				EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut2);
		default:
		case 1:
			if(!hasBiDirectionalSDI)
				EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut1);
			break;
		}
	}
	if(NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1))
		EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut5);

	// set custom anc input select
	if (NTV2DeviceCanDoCustomAnc(deviceID) == true)
	{
		uint32_t numSdiInputs = NTV2DeviceGetNumVideoInputs(deviceID);
		uint32_t selectedAncInput = mVirtualInputSelect;

		if (selectedAncInput >= numSdiInputs)
			selectedAncInput = 0;

		mCard->WriteRegister(kVRegCustomAncInputSelect, selectedAncInput);
	}

}

void DeviceServices::SetDeviceXPointPlayback( GeneralFrameFormat format )
{
	(void) format;
	NTV2DeviceID deviceID = mCard->GetDeviceID();

	NTV2FrameBufferFormat fbFormatCh1;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormatCh1);

	NTV2FrameBufferFormat fbFormatCh2;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL2, &fbFormatCh2);
	bool bCh2RGB = IsFrameBufferFormatRGB(fbFormatCh2);

	bool bDSKGraphicMode = (mDSKMode == NTV2_DSKModeGraphicOverMatte || mDSKMode == NTV2_DSKModeGraphicOverVideoIn || mDSKMode == NTV2_DSKModeGraphicOverFB);
	bool bDSKOn = (mDSKMode == NTV2_DSKModeFBOverMatte || mDSKMode == NTV2_DSKModeFBOverVideoIn || (bCh2RGB && bDSKGraphicMode));
	bool bDSKNeedsInputRef = false;

	// don't let the DSK be ON if we're in Mac Desktop mode
	if ((!mStreamingAppPID && mDefaultVideoOutMode == kDefaultModeDesktop) || !NTV2DeviceCanDoWidget(deviceID, NTV2_WgtMixer1))
		bDSKOn = false;
	
	if (mCard->DeviceCanDoAudioMixer())
	{
		uint32_t audioInputSelect;
		mCard->ReadRegister(kVRegAudioInputSelect, &audioInputSelect);
		SetAudioInputSelect((NTV2InputAudioSelect)audioInputSelect);
	}

	if(bDSKOn)
	{
		switch(mDSKMode)
		{
		case NTV2_DSKModeFBOverVideoIn:
		case NTV2_DSKModeGraphicOverVideoIn:
			bDSKNeedsInputRef = true;
			break;
		default:
			break;

		}
	}

	// The reference (genlock) source: if it's a video input, make sure it matches our current selection
	ReferenceSelect refSelect = bDSKNeedsInputRef ? mCaptureReferenceSelect : mDisplayReferenceSelect;
	switch (refSelect)
	{
	default:
	case kFreeRun:
		mCard->SetReference(NTV2_REFERENCE_FREERUN);
		break;
	case kReferenceIn:
		if (IsCompatibleWithReference(mFb1VideoFormat))
			mCard->SetReference(NTV2_REFERENCE_EXTERNAL);
		else
			mCard->SetReference(NTV2_REFERENCE_FREERUN);
		break;
	case kVideoIn:
		{
			//Some boards have HDMI some have analog LHi has both
			switch (mVirtualInputSelect)
			{
			default:
			case NTV2_Input1Select:
				mCard->SetReference(NTV2_REFERENCE_INPUT1);
				break;
			case NTV2_Input2Select:
				switch(deviceID)
				{
				case DEVICE_ID_LHI:
				case DEVICE_ID_IOEXPRESS:
					mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
					break;
				case DEVICE_ID_LHE_PLUS:
					mCard->SetReference(NTV2_REFERENCE_ANALOG_INPUT);
					break;
				default:
					mCard->SetReference(NTV2_REFERENCE_INPUT2);
					break;
				}
				break;
			case NTV2_Input3Select:
				switch(deviceID)
				{
				default:
				case DEVICE_ID_IOXT:
				case DEVICE_ID_IO4KUFC:
					mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
					break;
				case DEVICE_ID_LHI:
					mCard->SetReference(NTV2_REFERENCE_ANALOG_INPUT);
					break;
				}
				break;
			case NTV2_Input5Select://Only used by io4k quad id
				mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
				break;
			}
		}
		break;
	}

	mCard->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, NTV2_AUDIOSYSTEM_1);

	switch(NTV2DeviceGetNumVideoInputs(deviceID))
	{
	case 8:
		DisableRP188EtoE(NTV2_Wgt3GSDIOut8);
	case 7:
		DisableRP188EtoE(NTV2_Wgt3GSDIOut7);
	case 6:
		DisableRP188EtoE(NTV2_Wgt3GSDIOut6);
	case 5:
		DisableRP188EtoE(NTV2_Wgt3GSDIOut5);
	case 4:
		DisableRP188EtoE(NTV2_Wgt3GSDIOut4);
	case 3:
		DisableRP188EtoE(NTV2_Wgt3GSDIOut3);
	case 2:
		if(!NTV2DeviceHasBiDirectionalSDI(deviceID))
		{
			DisableRP188EtoE(NTV2_Wgt3GSDIOut2);
		}
	default:
	case 1:
		if(!NTV2DeviceHasBiDirectionalSDI(deviceID))
		{
			DisableRP188EtoE(NTV2_Wgt3GSDIOut1);
		}
		break;
	}
	if(NTV2DeviceCanDoWidget(deviceID, NTV2_WgtSDIMonOut1))
	{
		DisableRP188EtoE(NTV2_Wgt3GSDIOut5);
	}
}


void DeviceServices::SetDeviceXPointPlaybackRaw( GeneralFrameFormat format )
{
	(void) format;
	NTV2DeviceID deviceId = mCard->GetDeviceID();
	

	// CSC 1
	mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptBlack);
	// CSC 2
	mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptBlack);
	// CSC 3
	mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	// CSC 4
	mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);
	// CSC 5
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_WgtCSC5))
	{
		mCard->Connect (NTV2_XptCSC5VidInput, NTV2_XptBlack);
	}
	

	// LUT 1
	mCard->Connect (NTV2_XptLUT1Input, NTV2_XptBlack);
	// LUT 2
	mCard->Connect (NTV2_XptLUT2Input, NTV2_XptBlack);
	// LUT 3
	mCard->Connect (NTV2_XptLUT3Input, NTV2_XptBlack);
	// LUT 4
	mCard->Connect (NTV2_XptLUT4Input, NTV2_XptBlack);
	// LUT 5
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_WgtLUT5))
	{
		mCard->Connect (NTV2_XptLUT5Input, NTV2_XptBlack);
	}

	
	// Frame Buffer 1
	mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptBlack);
	// Frame Buffer 2
	mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptBlack);
	// Frame Buffer 3
	mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
	// Frame Buffer 4
	mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
	
	// Frame Buffer (2,3,4) disabling, format, direction
	NTV2FrameBufferFormat fbFormatCh1;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormatCh1);
	mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL2, fbFormatCh1);
	mCard->WriteRegister(kRegCh2Control, 0, kRegMaskChannelDisable, kRegShiftChannelDisable);
	if (format == FORMAT_RAW_HFR || format == FORMAT_RAW_UHFR)
	{
		mCard->SetMode(NTV2_CHANNEL3, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL3, fbFormatCh1);
		mCard->SetMode(NTV2_CHANNEL4, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL4, fbFormatCh1);
		
		mCard->WriteRegister(kRegCh3Control, 0, kRegMaskChannelDisable, kRegShiftChannelDisable);
		mCard->WriteRegister(kRegCh4Control, 0, kRegMaskChannelDisable, kRegShiftChannelDisable);
	}
	else
	{
		mCard->WriteRegister(kRegCh3Control, 1, kRegMaskChannelDisable, kRegShiftChannelDisable);
		mCard->WriteRegister(kRegCh4Control, 1, kRegMaskChannelDisable, kRegShiftChannelDisable);
	}

	// 4K Down Converter
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_Wgt4KDownConverter))
	{
		mCard->Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptBlack);
		mCard->Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptBlack);
		mCard->Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptBlack);
		mCard->Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptBlack);
	}
	

	// Duallink Out 1
	mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptBlack);
	// Duallink Out 2
	mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptBlack);
	// Duallink Out 3
	mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptBlack);
	// Duallink Out 4
	mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptBlack);
	
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_WgtDualLinkV2Out5))
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, NTV2_XptBlack);
	}
	
	// SDI Out 1
	mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	
	// SDI Out 2
	mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
	
	// SDI Out 3 - acts like SDI 1 in non-4K mode
	mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
	
	// SDI Out 4 - acts like SDI 2 in non-4K mode
	mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);

	// SDI Out 5
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_Wgt3GSDIOut5))
	{
		mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptBlack);
		mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
	}
	

	// HDMI Out
	mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ4Input, NTV2_XptBlack);
	// 4K Hdmi-to-Hdmi Bypass always disabled for playback
	mCard->WriteRegister(kRegHDMIOutControl, false, kRegMaskHDMIV2TxBypass, kRegShiftHDMIV2TxBypass);

	
	// Mixer/Keyer
	mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptBlack);
	// default video proc mode - FG = Full, BG = Full, VidProc = FG On
	mCard->WriteRegister (kRegVidProc1Control, 0, ~kRegMaskVidProcLimiting);
	
	
	// audio loopback: 0 = frame buffer playback, 1 = input loopback (E-to-E) mode
	int audioLoopbackMode = 0;
	mCard->WriteRegister (kRegAud1Control, audioLoopbackMode, kRegMaskLoopBack, kRegShiftLoopBack);
	
	
	// Reference
	// If it's a video input, make sure it matches our current selection
	ReferenceSelect refSelect = mDisplayReferenceSelect;
	switch (refSelect)
	{
		default:
		case kFreeRun:
			mCard->SetReference(NTV2_REFERENCE_FREERUN);
			break;
		case kReferenceIn:
			if (IsCompatibleWithReference(mFb1VideoFormat))
				mCard->SetReference(NTV2_REFERENCE_EXTERNAL);
			else
				mCard->SetReference(NTV2_REFERENCE_FREERUN);
			break;
		case kVideoIn:
			switch (mVirtualInputSelect)
		{
			default:
			case NTV2_Input1Select:	mCard->SetReference(NTV2_REFERENCE_INPUT1);		break;
			case NTV2_Input2Select:	mCard->SetReference(NTV2_REFERENCE_INPUT2);		break;
			case NTV2_Input5Select:	mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);	break;
		}
	}

}


void DeviceServices::SetDeviceXPointCaptureRaw( GeneralFrameFormat format )
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture(format);
	NTV2DeviceID deviceId = mCard->GetDeviceID();
	
	bool b3GbInEnabled;
	mCard->GetSDIInput3GPresent(b3GbInEnabled, NTV2_CHANNEL1);
	

	// SDI In (1 2 3 4)
	switch (format)
	{
		default:
		case FORMAT_RAW:
			// Level B-to-A conversion
			mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL1, false);
			break;
		
		case FORMAT_RAW_HFR:
			// Level B-to-A conversion
			mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL1, false);
			mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL2, false);
			break;
			
		case FORMAT_RAW_UHFR:
			// Level B-to-A conversion
			mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL1, b3GbInEnabled);
			mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL2, b3GbInEnabled);
			mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL3, b3GbInEnabled);
			mCard->SetSDIInLevelBtoLevelAConversion(NTV2_CHANNEL4, b3GbInEnabled);
			break;
	}

	
	// Duallink In 1
	switch (format)
	{
		default:
		case FORMAT_RAW:
		case FORMAT_RAW_HFR:
			mCard->Connect (NTV2_XptDualLinkIn1Input, NTV2_XptSDIIn1);
			mCard->Connect (NTV2_XptDualLinkIn1DSInput, NTV2_XptSDIIn1DS2);
			break;
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptDualLinkIn1Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptDualLinkIn1DSInput, NTV2_XptBlack);
			break;
	}
	
	// Duallink In 2
	switch (format)
	{
		case FORMAT_RAW_HFR:
			mCard->Connect (NTV2_XptDualLinkIn2Input, NTV2_XptSDIIn2);
			mCard->Connect (NTV2_XptDualLinkIn2DSInput, NTV2_XptSDIIn2DS2);
			break;
		default:
		case FORMAT_RAW:
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptDualLinkIn2Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptDualLinkIn2DSInput, NTV2_XptBlack);
			break;
	}
	
	// Duallink In 3
	mCard->Connect (NTV2_XptDualLinkIn3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptDualLinkIn3DSInput, NTV2_XptBlack);
	
	// Duallink In 4
	mCard->Connect (NTV2_XptDualLinkIn4Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptDualLinkIn4DSInput, NTV2_XptBlack);
	
	
	
	// CSC 1
	mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptDuallinkIn1);
	// CSC 2
	mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptBlack);
	// CSC 3
	mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	// CSC 4
	mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);
	// CSC 5
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_WgtCSC5))
	{
		mCard->Connect (NTV2_XptCSC5VidInput, NTV2_XptBlack);
	}
	
	
	// LUT 1
	mCard->Connect (NTV2_XptLUT1Input, NTV2_XptBlack);
	// LUT 2
	mCard->Connect (NTV2_XptLUT2Input, NTV2_XptBlack);
	// LUT 3
	mCard->Connect (NTV2_XptLUT3Input, NTV2_XptBlack);
	// LUT 4
	mCard->Connect (NTV2_XptLUT4Input, NTV2_XptBlack);
	// LUT 5
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_WgtLUT5))
	{
		mCard->Connect (NTV2_XptLUT5Input, NTV2_XptBlack);
	}
	
	
	// Duallink Out 1
	mCard->Connect (NTV2_XptDualLinkOut1Input, NTV2_XptBlack);
	// Duallink Out 2
	mCard->Connect (NTV2_XptDualLinkOut2Input, NTV2_XptBlack);
	// Duallink Out 3
	mCard->Connect (NTV2_XptDualLinkOut3Input, NTV2_XptBlack);
	// Duallink Out 4
	mCard->Connect (NTV2_XptDualLinkOut4Input, NTV2_XptBlack);
	// Duallink Out 5
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_WgtDualLinkV2Out5))
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, NTV2_XptBlack);
	}
	
	
	// Frame Buffer 1
	int bCh1Disable=false, bCh2Disable=false, bCh3Disable=false, bCh4Disable=false;
	switch (format)
	{
		default:
		case FORMAT_RAW:
		case FORMAT_RAW_HFR:
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptDuallinkIn1);
			break;
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptFrameBuffer1Input, NTV2_XptSDIIn1);
			break;
	}
	
	// Frame Buffer 2
	switch (format)
	{
		default:
		case FORMAT_RAW:
		case FORMAT_RAW_HFR:
			mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptDuallinkIn1);
			break;
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptFrameBuffer2Input, NTV2_XptSDIIn2);
			break;
	}

	// Frame Buffer 3
	switch (format)
	{
		default:
		case FORMAT_RAW:
			bCh3Disable = true;
			mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptBlack);
			break;
		case FORMAT_RAW_HFR:
			mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptDuallinkIn2);
			break;
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptFrameBuffer3Input, NTV2_XptSDIIn3);
			break;
	}
	
	// Frame Buffer 4
	switch (format)
	{
		default:
		case FORMAT_RAW:
			bCh4Disable = true;
			mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptBlack);
			break;
		case FORMAT_RAW_HFR:
			mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptDuallinkIn2);
			break;
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptFrameBuffer4Input, NTV2_XptSDIIn4);
			break;
	}
	
	// Frame Buffer (1 2 3 4) format, direction
	NTV2FrameBufferFormat fbFormatCh1;
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, &fbFormatCh1);
	mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL2, fbFormatCh1);
	if (format == FORMAT_RAW_HFR || format == FORMAT_RAW_UHFR)
	{
		mCard->SetMode(NTV2_CHANNEL3, NTV2_MODE_CAPTURE);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL3, fbFormatCh1);
		mCard->SetMode(NTV2_CHANNEL4, NTV2_MODE_CAPTURE);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL4, fbFormatCh1);
	}

	// Frame Buffer (1 2 3 4) disable
	mCard->WriteRegister(kRegCh1Control, bCh1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bCh2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh3Control, bCh3Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh4Control, bCh4Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	
	
	// 4K Down Converter
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_Wgt4KDownConverter))
	{
		mCard->Connect (NTV2_Xpt4KDCQ1Input, NTV2_XptBlack);
		mCard->Connect (NTV2_Xpt4KDCQ2Input, NTV2_XptBlack);
		mCard->Connect (NTV2_Xpt4KDCQ3Input, NTV2_XptBlack);
		mCard->Connect (NTV2_Xpt4KDCQ4Input, NTV2_XptBlack);
	}
	

	// SDI Out 1
	mCard->Connect (NTV2_XptSDIOut1Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut1InputDS2, NTV2_XptBlack);
	// SDI Out 2
	mCard->Connect (NTV2_XptSDIOut2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptSDIOut2InputDS2, NTV2_XptBlack);
	// SDI Out 3
	switch (format)
	{
		default:
		case FORMAT_RAW:
		case FORMAT_RAW_HFR:
			if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
			{
				mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptSDIIn1);
				mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptSDIIn1DS2);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC1VidYUV);
				mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
			}
			break;
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptSDIOut3Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptSDIOut3InputDS2, NTV2_XptBlack);
			break;
	}
	// SDI Out 4
	switch (format)
	{
		default:
		case FORMAT_RAW:
			if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
			{
				mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptSDIIn1);
				mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptSDIIn1DS2);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC1VidYUV);
				mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
			}
			break;
		case FORMAT_RAW_HFR:
			if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
			{
				mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptSDIIn2);
				mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptSDIIn2DS2);
			}
			else
			{
				mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptCSC1VidYUV);
				mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
			}
			break;
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptSDIOut4Input, NTV2_XptBlack);
			mCard->Connect (NTV2_XptSDIOut4InputDS2, NTV2_XptBlack);
			break;
	}
	// SDI Out 5
	if (::NTV2DeviceCanDoWidget(deviceId, NTV2_WgtSDIMonOut1))
	{
		switch (format)
		{
			default:
			case FORMAT_RAW:
			case FORMAT_RAW_HFR:
				if (mVirtualDigitalOutput1Select == NTV2_DualLinkOutputSelect)
				{
					mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptSDIIn1);
					mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptSDIIn1DS2);
				}
				else
				{
					mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC1VidYUV);
					mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
				}
				break;
			case FORMAT_RAW_UHFR:
				mCard->Connect (NTV2_XptSDIOut5Input, NTV2_XptSDIIn1);
				mCard->Connect (NTV2_XptSDIOut5InputDS2, NTV2_XptBlack);
				break;
		}
	}


	// HDMI Out
	switch (format)
	{
		default:
		case FORMAT_RAW:
		case FORMAT_RAW_HFR:
			mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptDuallinkIn1);
			break;
		
		case FORMAT_RAW_UHFR:
			mCard->Connect (NTV2_XptHDMIOutInput, NTV2_XptSDIIn1);
			break;
	}
	mCard->Connect (NTV2_XptHDMIOutQ2Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ3Input, NTV2_XptBlack);
	mCard->Connect (NTV2_XptHDMIOutQ4Input, NTV2_XptBlack);
	
	
	// Mixer/Keyer
	mCard->Connect (NTV2_XptMixer1FGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1FGKeyInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1BGVidInput, NTV2_XptBlack);
	mCard->Connect (NTV2_XptMixer1BGKeyInput, NTV2_XptBlack);
	// default video proc mode - FG = Full, BG = Full, VidProc = FG On
	mCard->WriteRegister (kRegVidProc1Control, 0, ~kRegMaskVidProcLimiting);
	
	
	// Reference
	// If it's a video input, make sure it matches our current selection
	switch (mCaptureReferenceSelect)
	{
		default:
		case kFreeRun:		mCard->SetReference(NTV2_REFERENCE_FREERUN);		break;
		case kReferenceIn:	mCard->SetReference(NTV2_REFERENCE_EXTERNAL);		break;
		case kVideoIn:
			switch (mVirtualInputSelect)
			{
				default:
				case NTV2_Input1Select:	mCard->SetReference(NTV2_REFERENCE_INPUT1);		break;
				case NTV2_Input2Select:	mCard->SetReference(NTV2_REFERENCE_INPUT2);		break;
				case NTV2_Input5Select:	mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);	break;
			}
			break;
	}
}

//-------------------------------------------------------------------------------------------------------
//	SetAudioInputSelect
//-------------------------------------------------------------------------------------------------------
void DeviceServices::SetAudioInputSelect(NTV2InputAudioSelect input)
{
	ULWord regValue;
	// convert from enum to actual register bits
	switch (input)
	{
		case NTV2_Input1Embedded1_8Select:		regValue = 0x00004321;  break;
		case NTV2_Input1Embedded9_16Select:		regValue = 0x00008765;  break;
		case NTV2_Input2Embedded1_8Select:		regValue = 0x00014321;  break;
		case NTV2_Input2Embedded9_16Select:		regValue = 0x00018765;  break;
		case NTV2_HDMISelect:					regValue = 0x0010000A;  break;
		case NTV2_MicInSelect:					regValue = 0x0010000B;	break;
		case NTV2_AnalogSelect:					regValue = 0x00009999;  break;
		case NTV2_AES_EBU_XLRSelect:
		case NTV2_AES_EBU_BNCSelect:
		default:								regValue = 0x00000000;  break;
	}

	// write the reg value to hardware
	mCard->WriteAudioSource(regValue);
	if(mCard->DeviceCanDoAudioMixer())
	{
		mCard->WriteAudioSource(regValue, NTV2_CHANNEL2);
		mCard->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_ON, NTV2_AUDIOSYSTEM_2);
	}

	// in addition, if this is an AES input select the correct physical layer
	if (input == NTV2_AES_EBU_BNCSelect)
		mCard->WriteRegister(kRegAud1Control, 0, kK2RegMaskKBoxAudioInputSelect, kK2RegShiftKBoxAudioInputSelect);
	else if (input == NTV2_AES_EBU_XLRSelect)
		mCard->WriteRegister(kRegAud1Control, 1, kK2RegMaskKBoxAudioInputSelect, kK2RegShiftKBoxAudioInputSelect);

}

