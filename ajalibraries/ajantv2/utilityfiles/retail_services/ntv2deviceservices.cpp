//
//  ntv2deviceservices.cpp
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#include "retailsupport.h"
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
#include "ntv2ioip2022services.h"
#include "ntv2ioip2110services.h"
#include "ntv2io4kplusservices.h"
#include "ntv2konahdmiservices.h"
#include "ntv2vpidfromspec.h"
#include "ntv2corvid88services.h"
#include "ntv2kona1services.h"
#include "ntv2kona5services.h"
#include "appsignatures.h"
#include "ajabase/system/systemtime.h"

#if defined (AJALinux) || defined (AJAMac)
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using namespace std;


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
        case DEVICE_ID_IOIP_2022:
            pDeviceServices = new IoIP2022Services();
            break;
        case DEVICE_ID_IOIP_2110:
            pDeviceServices = new IoIP2110Services();
            break;
        case DEVICE_ID_KONAIP_2110:
			pDeviceServices = new KonaIP2110Services();
			break;
        case DEVICE_ID_KONAIP_2022:
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
		case DEVICE_ID_KONALHI:
			pDeviceServices = new KonaLHiServices();
			break;
		case DEVICE_ID_KONALHEPLUS:
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
		case DEVICE_ID_KONA1:
			pDeviceServices = new Kona1Services();
			break;
		case DEVICE_ID_KONAHDMI:
			pDeviceServices = new KonaHDMIServices();
			break;
		case DEVICE_ID_KONA5:
			pDeviceServices = new Kona5Services();
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
	mInputFormatSelect				= -1;
	mInputFormatLock				= false;
	mLastInputFormatSelect			= NTV2_FORMAT_UNKNOWN;
	mDefaultVideoFormat				= NTV2_FORMAT_UNKNOWN;
}

void DeviceServices::SetCard(CNTV2Card* pCard)
{
	mCard = pCard;
	int index = pCard->GetIndexNumber();
	
	// board info
	CNTV2DeviceScanner scanner;
	bool bFound = scanner.GetDeviceInfo(index, mBoardInfo, false);
	if (bFound == false)
		printf("Fail board info scan\n");
	RetailSupport::AdjustDeviceInfoForApp(mBoardInfo);
	
	// model
	mModel.SetCard(pCard, pCard->GetDeviceID(), index);
	
	// retail object
	mRs = new RetailSupport(mDeviceState, mBoardInfo, mModel);
	
	// device state
	mRs->InitDeviceState(mDeviceState);
}

#define	AsDriverInterface(_x_)		static_cast<CNTV2DriverInterface*>(_x_)

//-------------------------------------------------------------------------------------------------------
//	ReadDriverState
//-------------------------------------------------------------------------------------------------------

#define USE_NEW_RETAIL

bool DeviceServices::ReadDriverState (void)
{
	// check the state of the hardware and see if anything has changed since last time
#ifdef USE_NEW_RETAIL
	
	DeviceState& ds = mDeviceState;

	bool bChanged = mRs->GetDeviceState(ds);
	(void) bChanged;
	
	// retrofit hack for now
	//if (bChanged)
	{
		// sdi out
		uint32_t sdiOutSize = ds.sdiOut.size();
		if (sdiOutSize > 0)
		{ 
			mVirtualDigitalOutput1Select = ds.sdiOut[0]->outSelect;
			mSDIOutput1ColorSpace = ds.sdiOut[0]->cs;
			mSDIOutput1RGBRange = ds.sdiOut[0]->rgbRange;
			m4kTransportOutSelection = ds.sdiOut[0]->transport4k;
			mSdiOutTransportType = ds.sdiOut[0]->transport3g;
		}
		if (sdiOutSize > 1) 
			mVirtualDigitalOutput2Select = ds.sdiOut[1]->outSelect;
		
		// input select
		mVirtualInputSelect = ds.inputSelect;
		mInputAudioSelect = ds.audioSelect;
		
		uint32_t sdiInSize = ds.sdiOut.size();
		if (sdiInSize > 0)
		{
			mSDIInput1ColorSpace = ds.sdiIn[0]->cs;
			mSDIInput1RGBRange = ds.sdiIn[0]->rgbRange;
		}
		
		// hdmi out
		mVirtualHDMIOutputSelect = ds.hdmiOutFormatSelect;

		// analog out
		mVirtualAnalogOutputSelect = ds.analogOutFormatSelect;
	}
				
#else
	
	// sdi output 
	AsDriverInterface(mCard)->ReadRegister(kVRegDigitalOutput1Select, mVirtualDigitalOutput1Select);
	AsDriverInterface(mCard)->ReadRegister(kVRegDigitalOutput2Select, mVirtualDigitalOutput2Select);
	AsDriverInterface(mCard)->ReadRegister(kVRegSDIOutput1ColorSpaceMode, mSDIOutput1ColorSpace);
	AsDriverInterface(mCard)->ReadRegister(kVRegSDIOutput1RGBRange, mSDIOutput1RGBRange);
	AsDriverInterface(mCard)->ReadRegister(kVReg4kOutputTransportSelection, m4kTransportOutSelection);
	AsDriverInterface(mCard)->ReadRegister(kVRegDualStreamTransportType, mSdiOutTransportType);
	
	// input select
	AsDriverInterface(mCard)->ReadRegister(kVRegInputSelect, mVirtualInputSelect);
	AsDriverInterface(mCard)->ReadRegister(kVRegAudioInputSelect, mInputAudioSelect);
	
	// sdi in
	AsDriverInterface(mCard)->ReadRegister(kVRegSDIInput1ColorSpaceMode, mSDIInput1ColorSpace);
	AsDriverInterface(mCard)->ReadRegister(kVRegSDIInput1RGBRange, mSDIInput1RGBRange);
	
	// hdmi out
	AsDriverInterface(mCard)->ReadRegister(kVRegHDMIOutputSelect, mVirtualHDMIOutputSelect);
	
	// analog out
	AsDriverInterface(mCard)->ReadRegister(kVRegAnalogOutputSelect, mVirtualAnalogOutputSelect);
	
	// auto set registers marked with "auto" enum
	UpdateAutoState();
	
#endif

	mCard->GetStreamingApplication(&mStreamingAppType, &mStreamingAppPID);
	
	AsDriverInterface(mCard)->ReadRegister(kVRegDefaultVideoFormat, mDefaultVideoFormat);
	AsDriverInterface(mCard)->ReadRegister(kVRegDefaultVideoOutMode, mDefaultVideoOutMode);
	mCard->ReadRegister(kVRegFollowInputFormat, mFollowInputFormat);
	mCard->ReadRegister(kVRegVANCMode, mVANCMode);
	mCard->ReadRegister(kVRegDefaultInput, mDefaultInput);
	AsDriverInterface(mCard)->ReadRegister(kVRegDSKMode, mDSKMode);
	AsDriverInterface(mCard)->ReadRegister(kVRegLUTType, mLUTType);
	AsDriverInterface(mCard)->ReadRegister(kVRegSecondaryFormatSelect, mVirtualSecondaryFormatSelect);
	AsDriverInterface(mCard)->ReadRegister(kVRegIsoConvertEnable, mIsoConvertEnable);
	mCard->ReadRegister(kVRegDSKAudioMode, mDSKAudioMode);
	mCard->ReadRegister(kVRegDSKForegroundMode, mDSKForegroundMode);
	mCard->ReadRegister(kVRegDSKForegroundFade, mDSKForegroundFade);
	AsDriverInterface(mCard)->ReadRegister(kVRegCaptureReferenceSelect, mCaptureReferenceSelect);
	AsDriverInterface(mCard)->ReadRegister(kVRegDisplayReferenceSelect, mDisplayReferenceSelect);
	AsDriverInterface(mCard)->ReadRegister(kVRegGammaMode, mGammaMode);
	AsDriverInterface(mCard)->ReadRegister(kVRegRGB10Range, mRGB10Range);
	AsDriverInterface(mCard)->ReadRegister(kVRegColorSpaceMode, mColorSpaceType);
	
	AsDriverInterface(mCard)->ReadRegister(kVRegFrameBuffer1RGBRange, mFrameBuffer1RGBRange);
	AsDriverInterface(mCard)->ReadRegister(kVRegAnalogOutBlackLevel, mVirtualAnalogOutBlackLevel);
	AsDriverInterface(mCard)->ReadRegister(kVRegAnalogOutputType, mVirtualAnalogOutputType);
	AsDriverInterface(mCard)->ReadRegister(kVRegAnalogInBlackLevel, mVirtualAnalogInBlackLevel);
	AsDriverInterface(mCard)->ReadRegister(kVRegAnalogInputType, mVirtualAnalogInType);
	AsDriverInterface(mCard)->ReadRegister(kVRegAnalogInStandard, mVirtualAnalogInStandard);
	
	AsDriverInterface(mCard)->ReadRegister(kVRegHDMIOutColorSpaceModeCtrl, mHDMIOutColorSpaceModeCtrl);
	AsDriverInterface(mCard)->ReadRegister(kVRegHDMIOutProtocolMode, mHDMIOutProtocolMode);
	AsDriverInterface(mCard)->ReadRegister(kVRegHDMIOutStereoSelect, mHDMIOutStereoSelect);
	AsDriverInterface(mCard)->ReadRegister(kVRegHDMIOutStereoCodecSelect, mHDMIOutStereoCodecSelect);
	AsDriverInterface(mCard)->ReadRegister(kVRegHDMIOutAudioChannels, mHDMIOutAudioChannels);
	mCard->ReadRegister(kVRegFramesPerVertical, mRegFramesPerVertical);
	
	mSDIInput2RGBRange = mSDIInput1RGBRange;
	mSDIInput2ColorSpace = mSDIInput1ColorSpace;	// for now
	
	// basic Ch1 HW registers 
	mDeviceID = mCard->GetDeviceID();
	mCard->GetVideoFormat(mFb1VideoFormat);
	mCard->GetFrameBufferFormat(NTV2_CHANNEL1, mFb1Format);
	mCard->GetMode(NTV2_CHANNEL1, mFb1Mode);
	// vpid
	if (NTV2DeviceCanDoDualLink(mDeviceID) == true)
	{
		if (NTV2DeviceGetNumVideoInputs(mDeviceID) > 0)
			mVpid1Valid = mCard->ReadSDIInVPID(NTV2_CHANNEL1, mVpid1a, mVpid1b);
		else
			mVpid1a = mVpid1b = mVpid1Valid = 0;
		
		if (NTV2DeviceGetNumVideoInputs(mDeviceID) > 1)
			mVpid2Valid = mCard->ReadSDIInVPID(NTV2_CHANNEL2, mVpid2a, mVpid2b);
		else
			mVpid2a = mVpid2b = mVpid2Valid = 0;
	}
	else
	{
		mVpid1a = mVpid1b = mVpid1Valid = mVpid2a = mVpid2b = mVpid2Valid = 0;
	}

	// basic Ch2 HW registers
	if (NTV2DeviceGetNumberFrameBuffers(mDeviceID) > 1)
		mCard->GetFrameBufferFormat(NTV2_CHANNEL2, mFb2Format);
	
    if (mCard->DeviceCanDoAudioMixer())
	{
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioMixerOverrideState,    mAudioMixerOverrideState);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioMixerSourceMainEnable, mAudioMixerSourceMainEnable);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioMixerSourceAux1Enable, mAudioMixerSourceAux1Enable);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioMixerSourceAux2Enable, mAudioMixerSourceAux2Enable);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioMixerSourceMainGain,   mAudioMixerSourceMainGain);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioMixerSourceAux1Gain,   mAudioMixerSourceAux1Gain);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioMixerSourceAux2Gain,   mAudioMixerSourceAux2Gain);
		
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioCapMixerSourceMainEnable, mAudioCapMixerSourceMainEnable);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioCapMixerSourceAux1Enable, mAudioCapMixerSourceAux1Enable);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioCapMixerSourceAux2Enable, mAudioCapMixerSourceAux2Enable);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioCapMixerSourceMainGain,   mAudioCapMixerSourceMainGain);
		AsDriverInterface(mCard)->ReadRegister(kVRegAudioCapMixerSourceAux1Gain, mAudioCapMixerSourceAux1Gain);
		//AsDriverInterface(mCard)->ReadRegister(kVRegAudioCapMixerSourceAux2Gain, mAudioCapMixerSourceAux2Gain);
	}

    if ((mDeviceID == DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
        (mDeviceID == DEVICE_ID_KONAIP_2TX_1SFP_J2K) ||
        (mDeviceID == DEVICE_ID_KONAIP_2RX_1SFP_J2K) ||
        (mDeviceID == DEVICE_ID_KONAIP_2022) ||
        (mDeviceID == DEVICE_ID_IOIP_2022))
    {
        AsDriverInterface(mCard)->ReadRegister(kVReg2022_7Enable,              m2022_7Mode);
        mCard->ReadRegister(kVReg2022_7NetworkPathDiff,     mNetworkPathDiff);

        mCard->ReadRegister(kVRegIPAddrEth0,                mEth0.ipc_ip);
        mCard->ReadRegister(kVRegSubnetEth0,                mEth0.ipc_subnet);
        mCard->ReadRegister(kVRegGatewayEth0,               mEth0.ipc_gateway);

        mCard->ReadRegister(kVRegIPAddrEth1,                mEth1.ipc_ip);
        mCard->ReadRegister(kVRegSubnetEth1,                mEth1.ipc_subnet);
        mCard->ReadRegister(kVRegGatewayEth1,               mEth1.ipc_gateway);

        mCard->ReadRegister(kVRegRxcEnable1,				mRx2022Config1.rxc_enable32);
        mCard->ReadRegister(kVRegRxcSfp1RxMatch1,           mRx2022Config1.rxc_sfp1RxMatch);
        mCard->ReadRegister(kVRegRxcSfp1SourceIp1,          mRx2022Config1.rxc_sfp1SourceIp);
        mCard->ReadRegister(kVRegRxcSfp1DestIp1,            mRx2022Config1.rxc_sfp1DestIp);
        mCard->ReadRegister(kVRegRxcSfp1SourcePort1,        mRx2022Config1.rxc_sfp1SourcePort);
        mCard->ReadRegister(kVRegRxcSfp1DestPort1,          mRx2022Config1.rxc_sfp1DestPort);
        mCard->ReadRegister(kVRegRxcSfp1Vlan1,              mRx2022Config1.rxc_sfp1Vlan);
        mCard->ReadRegister(kVRegRxcSfp2RxMatch1,           mRx2022Config1.rxc_sfp2RxMatch);
        mCard->ReadRegister(kVRegRxcSfp2SourceIp1,          mRx2022Config1.rxc_sfp2SourceIp);
        mCard->ReadRegister(kVRegRxcSfp2DestIp1,            mRx2022Config1.rxc_sfp2DestIp);
        mCard->ReadRegister(kVRegRxcSfp2SourcePort1,        mRx2022Config1.rxc_sfp2SourcePort);
        mCard->ReadRegister(kVRegRxcSfp2DestPort1,          mRx2022Config1.rxc_sfp2DestPort);
        mCard->ReadRegister(kVRegRxcSfp2Vlan1,              mRx2022Config1.rxc_sfp2Vlan);
        mCard->ReadRegister(kVRegRxcSsrc1,					mRx2022Config1.rxc_ssrc);
        mCard->ReadRegister(kVRegRxcPlayoutDelay1,			mRx2022Config1.rxc_playoutDelay);

        mCard->ReadRegister(kVRegRxcEnable2,				mRx2022Config2.rxc_enable32);
        mCard->ReadRegister(kVRegRxcSfp1RxMatch2,           mRx2022Config2.rxc_sfp1RxMatch);
        mCard->ReadRegister(kVRegRxcSfp1SourceIp2,          mRx2022Config2.rxc_sfp1SourceIp);
        mCard->ReadRegister(kVRegRxcSfp1DestIp2,            mRx2022Config2.rxc_sfp1DestIp);
        mCard->ReadRegister(kVRegRxcSfp1SourcePort2,        mRx2022Config2.rxc_sfp1SourcePort);
        mCard->ReadRegister(kVRegRxcSfp1DestPort2,          mRx2022Config2.rxc_sfp1DestPort);
        mCard->ReadRegister(kVRegRxcSfp1Vlan2,              mRx2022Config2.rxc_sfp1Vlan);
        mCard->ReadRegister(kVRegRxcSfp2RxMatch2,           mRx2022Config2.rxc_sfp2RxMatch);
        mCard->ReadRegister(kVRegRxcSfp2SourceIp2,          mRx2022Config2.rxc_sfp2SourceIp);
        mCard->ReadRegister(kVRegRxcSfp2DestIp2,            mRx2022Config2.rxc_sfp2DestIp);
        mCard->ReadRegister(kVRegRxcSfp2SourcePort2,        mRx2022Config2.rxc_sfp2SourcePort);
        mCard->ReadRegister(kVRegRxcSfp2DestPort2,          mRx2022Config2.rxc_sfp2DestPort);
        mCard->ReadRegister(kVRegRxcSfp2Vlan2,              mRx2022Config2.rxc_sfp2Vlan);
        mCard->ReadRegister(kVRegRxcSsrc2,					mRx2022Config2.rxc_ssrc);
        mCard->ReadRegister(kVRegRxcPlayoutDelay2,			mRx2022Config2.rxc_playoutDelay);

        mCard->ReadRegister(kVRegTxcEnable3,				mTx2022Config3.txc_enable32);
        mCard->ReadRegister(kVRegTxcSfp1LocalPort3,         mTx2022Config3.txc_sfp1LocalPort);
        mCard->ReadRegister(kVRegTxcSfp1RemoteIp3,          mTx2022Config3.txc_sfp1RemoteIp);
        mCard->ReadRegister(kVRegTxcSfp1RemotePort3,		mTx2022Config3.txc_sfp1RemotePort);
        mCard->ReadRegister(kVRegTxcSfp2LocalPort3,         mTx2022Config3.txc_sfp2LocalPort);
        mCard->ReadRegister(kVRegTxcSfp2RemoteIp3,          mTx2022Config3.txc_sfp2RemoteIp);
        mCard->ReadRegister(kVRegTxcSfp2RemotePort3,        mTx2022Config3.txc_sfp2RemotePort);

        mCard->ReadRegister(kVRegTxcEnable4,				mTx2022Config4.txc_enable32);
        mCard->ReadRegister(kVRegTxcSfp1LocalPort4,         mTx2022Config4.txc_sfp1LocalPort);
        mCard->ReadRegister(kVRegTxcSfp1RemoteIp4,          mTx2022Config4.txc_sfp1RemoteIp);
        mCard->ReadRegister(kVRegTxcSfp1RemotePort4,		mTx2022Config4.txc_sfp1RemotePort);
        mCard->ReadRegister(kVRegTxcSfp2LocalPort4,         mTx2022Config4.txc_sfp2LocalPort);
        mCard->ReadRegister(kVRegTxcSfp2RemoteIp4,          mTx2022Config4.txc_sfp2RemoteIp);
        mCard->ReadRegister(kVRegTxcSfp2RemotePort4,        mTx2022Config4.txc_sfp2RemotePort);
    }

    if ((mDeviceID == DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) ||
        (mDeviceID == DEVICE_ID_KONAIP_2TX_1SFP_J2K) ||
        (mDeviceID == DEVICE_ID_KONAIP_2RX_1SFP_J2K))
    {
        AsDriverInterface(mCard)->ReadRegister(kVRegRxc_2DecodeSelectionMode1,	mRx2022J2kConfig1.selectionMode);
        mCard->ReadRegister(kVRegRxc_2DecodeProgramNumber1,	mRx2022J2kConfig1.programNumber);
        mCard->ReadRegister(kVRegRxc_2DecodeProgramPID1,	mRx2022J2kConfig1.programPID);
        mCard->ReadRegister(kVRegRxc_2DecodeAudioNumber1,	mRx2022J2kConfig1.audioNumber);

        AsDriverInterface(mCard)->ReadRegister(kVRegRxc_2DecodeSelectionMode2,	mRx2022J2kConfig2.selectionMode);
        mCard->ReadRegister(kVRegRxc_2DecodeProgramNumber2,	mRx2022J2kConfig2.programNumber);
        mCard->ReadRegister(kVRegRxc_2DecodeProgramPID2,	mRx2022J2kConfig2.programPID);
        mCard->ReadRegister(kVRegRxc_2DecodeAudioNumber2,	mRx2022J2kConfig2.audioNumber);

        AsDriverInterface(mCard)->ReadRegister(kVRegTxc_2EncodeVideoFormat1,	mTx2022J2kConfig1.videoFormat);
        mCard->ReadRegister(kVRegTxc_2EncodeUllMode1,		mTx2022J2kConfig1.ullMode);
        mCard->ReadRegister(kVRegTxc_2EncodeBitDepth1,		mTx2022J2kConfig1.bitDepth);
        AsDriverInterface(mCard)->ReadRegister(kVRegTxc_2EncodeChromaSubSamp1,	mTx2022J2kConfig1.chromaSubsamp);
        mCard->ReadRegister(kVRegTxc_2EncodeMbps1,			mTx2022J2kConfig1.mbps);
        mCard->ReadRegister(kVRegTxc_2EncodeAudioChannels1, mTx2022J2kConfig1.audioChannels);
        AsDriverInterface(mCard)->ReadRegister(kVRegTxc_2EncodeStreamType1,	mTx2022J2kConfig1.streamType);
        mCard->ReadRegister(kVRegTxc_2EncodeProgramPid1,	mTx2022J2kConfig1.pmtPid);
        mCard->ReadRegister(kVRegTxc_2EncodeVideoPid1,		mTx2022J2kConfig1.videoPid);
        mCard->ReadRegister(kVRegTxc_2EncodePcrPid1,		mTx2022J2kConfig1.pcrPid);
        mCard->ReadRegister(kVRegTxc_2EncodeAudio1Pid1,		mTx2022J2kConfig1.audio1Pid);

        AsDriverInterface(mCard)->ReadRegister(kVRegTxc_2EncodeVideoFormat2,	mTx2022J2kConfig2.videoFormat);
        mCard->ReadRegister(kVRegTxc_2EncodeUllMode2,		mTx2022J2kConfig2.ullMode);
        mCard->ReadRegister(kVRegTxc_2EncodeBitDepth2,		mTx2022J2kConfig2.bitDepth);
        AsDriverInterface(mCard)->ReadRegister(kVRegTxc_2EncodeChromaSubSamp2,	mTx2022J2kConfig2.chromaSubsamp);
        mCard->ReadRegister(kVRegTxc_2EncodeMbps2,			mTx2022J2kConfig2.mbps);
        mCard->ReadRegister(kVRegTxc_2EncodeAudioChannels2, mTx2022J2kConfig2.audioChannels);
        AsDriverInterface(mCard)->ReadRegister(kVRegTxc_2EncodeStreamType2,	mTx2022J2kConfig2.streamType);
        mCard->ReadRegister(kVRegTxc_2EncodeProgramPid2,	mTx2022J2kConfig2.pmtPid);
        mCard->ReadRegister(kVRegTxc_2EncodeVideoPid2,		mTx2022J2kConfig2.videoPid);
        mCard->ReadRegister(kVRegTxc_2EncodePcrPid2,		mTx2022J2kConfig2.pcrPid);
        mCard->ReadRegister(kVRegTxc_2EncodeAudio1Pid2,		mTx2022J2kConfig2.audio1Pid);
    }

    if ((mDeviceID == DEVICE_ID_KONAIP_2110) ||
        (mDeviceID == DEVICE_ID_IOIP_2110))
    {
        bool        bOk;
        bOk = mCard->ReadVirtualData(kNetworkData2110, &m2110Network, sizeof(NetworkData2110));
        if (bOk == false)
        {
            memset(&m2110Network, 0, sizeof(NetworkData2110));
            //printf("Failed to get 2110 Network params\n");
        }

        bOk = mCard->ReadVirtualData(kTransmitVideoData2110, &m2110TxVideoData, sizeof(TransmitVideoData2110));
        if (bOk == false)
        {
            memset(&m2110TxVideoData, 0, sizeof(TransmitVideoData2110));
            //printf("Failed to get 2110 Transmit Video params\n");
        }

        bOk = mCard->ReadVirtualData(kTransmitAudioData2110, &m2110TxAudioData, sizeof(TransmitAudioData2110));
        if (bOk == false)
        {
            memset(&m2110TxAudioData, 0, sizeof(TransmitAudioData2110));
            //printf("Failed to get 2110 Transmit Audio params\n");
        }

        bOk = mCard->ReadVirtualData(kReceiveVideoData2110, &m2110RxVideoData, sizeof(ReceiveVideoData2110));
        if (bOk == false)
        {
            memset(&m2110RxVideoData, 0, sizeof(ReceiveVideoData2110));
            //printf("Failed to get 2110 Receive Video params\n");
        }

        bOk = mCard->ReadVirtualData(kReceiveAudioData2110, &m2110RxAudioData, sizeof(ReceiveAudioData2110));
        if (bOk == false)
        {
            memset(&m2110RxAudioData, 0, sizeof(ReceiveAudioData2110));
            //printf("Failed to get 2110 Receive Audio params\n");
        }

        bOk = mCard->ReadVirtualData(kChStatusData2110, &m2110IpStatusData, sizeof(IpStatus2110));
        if (bOk == false)
        {
            memset(&m2110IpStatusData, 0, sizeof(IpStatus2110));
            //printf("Failed to get 2110 Ip status params\n");
        }
    }
    
    return true;
}


//-------------------------------------------------------------------------------------------------------
//	UpdateAutoState
//-------------------------------------------------------------------------------------------------------
void DeviceServices::UpdateAutoState()
{
	mSdiOutTransportType = 
		RetailSupport::AutoSelect3GTransport(mDeviceID, mSdiOutTransportType, mFb1VideoFormat);
	
	// out select sdi
	mVirtualDigitalOutput1Select = mVirtualDigitalOutput1Select == NTV2_AutoOutputSelect ?
				NTV2_PrimaryOutputSelect : mVirtualDigitalOutput1Select;
	
	// out select hdmi
	mVirtualHDMIOutputSelect = mVirtualHDMIOutputSelect == NTV2_AutoOutputSelect ?
				NTV2_PrimaryOutputSelect : mVirtualHDMIOutputSelect;
	
	// out select analog
	mVirtualAnalogOutputSelect = mVirtualAnalogOutputSelect == NTV2_AutoOutputSelect ?
				NTV2_PrimaryOutputSelect : mVirtualAnalogOutputSelect;
	
	// out cs
	mSDIOutput1ColorSpace = mSDIOutput1ColorSpace == NTV2_ColorSpaceModeAuto ?
							NTV2_ColorSpaceModeYCbCr : mSDIOutput1ColorSpace;
	
	// out range						
	mSDIOutput1RGBRange = mSDIOutput1RGBRange == NTV2_RGBRangeAuto ?
							NTV2_RGBRangeFull : mSDIOutput1RGBRange;
							
	// in range
	mSDIInput1RGBRange = mSDIInput1RGBRange == NTV2_RGBRangeAuto ?
							NTV2_RGBRangeFull : mSDIInput1RGBRange;
	mSDIInput2RGBRange = mSDIInput1RGBRange;
	
	
	// video input select
	if (mVirtualInputSelect == NTV2_InputAutoSelect)
	{
		mVirtualInputSelect = NTV2_Input1Select;
	}
	
	// audio input select
	/*
	if (ds.audioSelect_ == NTV2_AutoAudioSelect)
	{
		if (count > 0)
			ds.audioSelect = NTV2_Input1Embedded1_8Select;
		else
			ds.audioSelect = NTV2_HDMISelect;
		
		// TBD more here
	}
	else
	{
		ds.audioSelect = ds.audioSelect_;
	}
	*/
	
	// in color space - use vpid
	mSDIInput1ColorSpace = GetSDIInputColorSpace(NTV2_CHANNEL1, mSDIInput1ColorSpace);
	mSDIInput2ColorSpace = GetSDIInputColorSpace(NTV2_CHANNEL2, mSDIInput2ColorSpace);
	
	// 4k transport
	NTV24kTransportType tranport4k = NTV2_4kTransport_PixelInterleave;
	if (::NTV2DeviceCanDo425Mux(mDeviceID) == false)
		tranport4k = NTV2_4kTransport_Quadrants_4wire;
		
	m4kTransportOutSelection = m4kTransportOutSelection == NTV2_4kTransport_Auto ? 
				tranport4k : m4kTransportOutSelection;
				
	// 3G tranport
	NTV2SDITransportType transport3g = NTV2_SDITransport_DualLink_3Gb;
	if (::NTV2DeviceCanDo3GOut(mDeviceID, 0) == false)
		transport3g	= NTV2_SDITransport_DualLink_1_5;
		
	mSdiOutTransportType = mSdiOutTransportType == NTV2_SDITransport_Auto ? 
				transport3g : mSdiOutTransportType;
}



//-------------------------------------------------------------------------------------------------------
//	GetSDIInputColorSpace
//-------------------------------------------------------------------------------------------------------
NTV2ColorSpaceMode DeviceServices::GetSDIInputColorSpace(NTV2Channel inChannel, NTV2ColorSpaceMode inMode)
{
	NTV2ColorSpaceMode outMode = inMode;
	
	if (RetailSupport::CanDo3g(mDeviceID) == false)
		return NTV2_ColorSpaceModeYCbCr;
	
	if (mSDIInput1ColorSpace == NTV2_ColorSpaceModeAuto)
	{
		outMode = NTV2_ColorSpaceModeYCbCr;
		VPIDSampling sample = VPIDSampling_YUV_422;
		
		if (inChannel == NTV2_CHANNEL1 && mVpid1Valid == true)
		{
			mVpidParser.SetVPID(mVpid1a);
			sample = mVpidParser.GetSampling();
		}
		else if (inChannel == NTV2_CHANNEL2 && mVpid2Valid == true)
		{
			mVpidParser.SetVPID(mVpid2a);
			sample = mVpidParser.GetSampling();
		}
		
		outMode = 	(sample == VPIDSampling_YUV_422) ?
					NTV2_ColorSpaceModeYCbCr : NTV2_ColorSpaceModeRgb;
	}
	return outMode;
}


//-------------------------------------------------------------------------------------------------------
//	GetSelectedInputVideoFormat
//	Note:	Determine input video format based on input select and fbVideoFormat
//			which currently is videoformat of ch1-framebuffer
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat DeviceServices::GetSelectedInputVideoFormat(
											NTV2VideoFormat fbVideoFormat,
											NTV2ColorSpaceMode* inputColorSpace)
{
	NTV2VideoFormat inputFormat = NTV2_FORMAT_UNKNOWN;
	if (inputColorSpace)
		*inputColorSpace = NTV2_ColorSpaceModeYCbCr;
	
	// Figure out what our input format is based on what is selected 
	switch (mVirtualInputSelect)
	{
        case NTV2_Input1Select:
        case NTV2_Input2xDLHDSelect:
        case NTV2_Input2x4kSelect:
        case NTV2_Input4x4kSelect:
            inputFormat = GetSdiInVideoFormat(0, fbVideoFormat);
            if (inputColorSpace)
                *inputColorSpace = GetSDIInputColorSpace(NTV2_CHANNEL1, mSDIInput1ColorSpace);
            break;

        case NTV2_Input2Select:
            inputFormat = GetSdiInVideoFormat(1, fbVideoFormat);
            if (inputColorSpace)
                *inputColorSpace = GetSDIInputColorSpace(NTV2_CHANNEL2, mSDIInput2ColorSpace);
            break;

        default:
            break;
	}

	inputFormat = GetTransportCompatibleFormat(inputFormat, fbVideoFormat);
	
	return inputFormat;
}


//-------------------------------------------------------------------------------------------------------
//	GetCorrespondingAFormat
//	Note:	Returns corresponding A level format for any B level format.  If the input format is not level B it
//          will just return what you passed in.
//-------------------------------------------------------------------------------------------------------
NTV2VideoFormat DeviceServices::GetCorrespondingAFormat(NTV2VideoFormat inputFormat)
{
    if (inputFormat == NTV2_FORMAT_1080p_5000_B)
        return NTV2_FORMAT_1080p_5000_A;
    else if (inputFormat == NTV2_FORMAT_1080p_5994_B)
        return NTV2_FORMAT_1080p_5994_A;
    else if (inputFormat == NTV2_FORMAT_1080p_6000_B)
        return NTV2_FORMAT_1080p_6000_A;
    else if (inputFormat == NTV2_FORMAT_1080p_2K_6000_B)
        return NTV2_FORMAT_1080p_2K_6000_A;
    else if (inputFormat == NTV2_FORMAT_1080p_2K_5994_B)
        return NTV2_FORMAT_1080p_2K_5994_A;
    else if (inputFormat == NTV2_FORMAT_1080p_2K_5000_B)
        return NTV2_FORMAT_1080p_2K_5000_A;
    else if (inputFormat == NTV2_FORMAT_1080p_2K_4800_B)
        return NTV2_FORMAT_1080p_2K_4800_A;
    else if (inputFormat == NTV2_FORMAT_1080p_2K_4795_B)
        return NTV2_FORMAT_1080p_2K_4795_A;
    else
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
	
	mCard->ReadRegister (kVRegDebug1, virtualDebug1);
	mCard->ReadRegister (kVRegEveryFrameTaskFilter, everyFrameTaskFilter);
	
	SetDeviceEveryFrameRegs (virtualDebug1, everyFrameTaskFilter);
}


// Do everyframe task using input filter variables  
void DeviceServices::SetDeviceEveryFrameRegs (uint32_t virtualDebug1, uint32_t everyFrameTaskFilter)
{	
	// override virtual register filter variables
	mVirtualDebug1			= virtualDebug1;
	mEveryFrameTaskFilter	= everyFrameTaskFilter;

    //	CP checks the kVRegAgentCheck virtual register to see if I'm still running...
    AgentIsAlive();

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
	bool bChanged = ReadDriverState();
	if (bChanged == false)
		return;
		
	// Get the general format
	if (::NTV2DeviceCanDoMultiFormat(mDeviceID))
	{
		mCard->SetMultiFormatMode(false);
	}

	if (mFb1Mode == NTV2_MODE_DISPLAY)
	{
		if (IsFormatRaw(mFb1Format))
			SetDeviceXPointPlaybackRaw();
		else
			SetDeviceXPointPlayback();
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
	
		if (IsFormatRaw(mFb1Format))
			SetDeviceXPointCaptureRaw();
		else
			SetDeviceXPointCapture();
	}
	
	//Setup the analog LTC stuff
	RP188SourceSelect TimecodeSource;
	AsDriverInterface(mCard)->ReadRegister(kVRegRP188SourceSelect, TimecodeSource);
	if (NTV2DeviceGetNumLTCInputs(mDeviceID) && TimecodeSource == kRP188SourceLTCPort)
	{
		mCard->SetLTCInputEnable(true);
		mCard->WriteRegister(kRegFS1ReferenceSelect, 0x1, BIT(10), 10);
		mCard->WriteRegister(kRegLTCStatusControl, 0x0, kRegMaskLTC1InBypass, kRegShiftLTC1Bypass);
		if(NTV2DeviceCanDoLTCInOnRefPort(mDeviceID))
			mCard->SetLTCOnReference(true);
	}
	else
	{
		mCard->SetLTCInputEnable(false);
		mCard->WriteRegister(kRegFS1ReferenceSelect, 0x0, BIT(10), 10);
		mCard->WriteRegister(kRegLTCStatusControl, 0x0, kRegMaskLTC1InBypass, kRegShiftLTC1Bypass);
		if (NTV2DeviceCanDoLTCInOnRefPort(mDeviceID))
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
		ULWord appType=0; int32_t pid=0;
		mCard->GetStreamingApplication(&appType, &pid);
		bool bHostAudioApp = AppUsesHostAudio(appType);
	
		if (mAudioMixerOverrideState == false)
		{
			mCard->SetAudioMixerMainInputChannelSelect(NTV2_AudioChannel1_2);
			mCard->WriteRegister(kRegAudioMixerChannelSelect, 0x06, 0xff00, 8);		// 64 audio samples (2^6) avg'd on meters
		}
		
		// shared source
		mCard->SetAudioMixerAux1x2chInputAudioSystem(NTV2_AUDIOSYSTEM_2);
		mCard->SetAudioMixerAux2x2chInputAudioSystem(hostAudioSystem);
	
		if (mFb1Mode == NTV2_MODE_DISPLAY)
		{
			mCard->SetAudioMixerMainInputAudioSystem(bHostAudioApp ? hostAudioSystem : NTV2_AUDIOSYSTEM_1);
		
			if (mAudioMixerOverrideState == false)
			{
				mCard->WriteRegister(kRegAudioMixerMutes, mAudioMixerSourceMainEnable ? 0x0000 : 0xfffc, 0xffff, 0);
				mCard->SetAudioMixerMainInputEnable(mAudioMixerSourceMainEnable);
				mCard->SetAudioMixerAux1InputEnable(mAudioMixerSourceAux1Enable);
				mCard->SetAudioMixerMainInputGain(mAudioMixerSourceMainGain);
				mCard->SetAudioMixerAux1InputGain(NTV2_AudioMixerChannel1, mAudioMixerSourceAux1Gain);
			}
			
			mCard->SetAudioMixerAux2InputGain(NTV2_AudioMixerChannel1, mAudioMixerSourceAux2Gain);
			mCard->SetAudioMixerAux2InputEnable(bHostAudioApp ? false : mAudioMixerSourceAux2Enable);
		}
		else
		{
			mCard->SetAudioMixerMainInputAudioSystem(NTV2_AUDIOSYSTEM_1);
		
			if (mAudioMixerOverrideState == false)
			{
				mCard->WriteRegister(kRegAudioMixerMutes, mAudioCapMixerSourceMainEnable ? 0x0000 : 0xfffc, 0xffff, 0);
				mCard->SetAudioMixerMainInputEnable(mAudioCapMixerSourceMainEnable);
				mCard->SetAudioMixerAux1InputEnable(false);
				mCard->SetAudioMixerMainInputGain(mAudioCapMixerSourceMainGain);
				//mCard->SetAudioMixerAux1InputGain(NTV2_AudioMixerChannel1, mAudioCapMixerSourceAux1Gain);
			}
			
			mCard->SetAudioMixerAux2InputGain(NTV2_AudioMixerChannel1, mAudioMixerSourceAux2Gain);
			mCard->SetAudioMixerAux2InputEnable(mAudioCapMixerSourceAux2Enable);
		}
		
		mCard->WriteRegister(kRegHDMIInputControl, 1, BIT(1), 1);
		
		audioSystem = NTV2DeviceGetAudioMixerSystem(mDeviceID);
	}

	// Setup the SDI Outputs audio source
	switch(NTV2DeviceGetNumVideoOutputs(mDeviceID))
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
	if(NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtSDIMonOut1))
	{
		mCard->SetSDIOutputAudioSystem(NTV2_CHANNEL5, audioSystem );
	}

	switch(NTV2DeviceGetNumVideoChannels(mDeviceID))
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
	
	// audio monitor
	ULWord chSelect = NTV2_AudioMonitor1_2;
	mCard->ReadRegister(kVRegAudioMonitorChannelSelect, chSelect);
	if (mDeviceID == DEVICE_ID_IO4KPLUS || mDeviceID == DEVICE_ID_IO4K || 
		mDeviceID == DEVICE_ID_KONA4 || mCard->DeviceCanDoAudioMixer() == true)
	{
		mCard->SetAudioOutputMonitorSource((NTV2AudioMonitorSelect)chSelect, NTV2_CHANNEL4);
		mCard->SetAESOutputSource(NTV2_AudioChannel1_4, NTV2_AUDIOSYSTEM_4, chSelect <= NTV2_AudioMonitor7_8 ? NTV2_AudioChannel1_4 : NTV2_AudioChannel9_12);
		mCard->SetAESOutputSource(NTV2_AudioChannel5_8, NTV2_AUDIOSYSTEM_4,  chSelect <= NTV2_AudioMonitor7_8 ? NTV2_AudioChannel5_8 : NTV2_AudioChannel13_16);
	}
	else if (	mDeviceID == DEVICE_ID_KONA3G	|| mDeviceID == DEVICE_ID_KONA3GQUAD ||
				mDeviceID == DEVICE_ID_IO4KUFC	|| mDeviceID == DEVICE_ID_KONA4UFC	||
				mDeviceID == DEVICE_ID_IOXT )
	{
		mCard->SetAudioOutputMonitorSource((NTV2AudioMonitorSelect)chSelect,  NTV2_CHANNEL1);
	}
	else
	{
		mCard->WriteRegister(kRegAud1Control, chSelect, kK2RegMaskKBoxAnalogMonitor, kK2RegShiftKBoxAnalogMonitor);
	}

	//Setup LUTs
	UpdateK2ColorSpaceMatrixSelect();
	UpdateK2LUTSelect();

	// Set misc registers
	SetDeviceMiscRegisters();
	
	// mark completion on cycle - used in media composer
	mCard->WriteRegister(kVRegServicesModeFinal, mFb1Mode);
}


void DeviceServices::SetDeviceMiscRegisters ()
{
}


// MARK: support -


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
		else if (vpidSpec.isRGBOnWire == false && IsRGBFormat(mFb1Format) == true)
			vpidSpec.pixelFormat = Is8BitFrameBufferFormat(mFb1Format) ? NTV2_FBF_8BIT_YCBCR : NTV2_FBF_INVALID;
	
		// Converted YUV -> RGB on wire
		else if (vpidSpec.isRGBOnWire == true && IsRGBFormat(mFb1Format) == false)
			vpidSpec.pixelFormat = NTV2_FBF_INVALID;
	
		// otherwise
		else
			vpidSpec.pixelFormat = mFb1Format;
	}

	return ::SetVPIDFromSpec (&outVPID, &vpidSpec);
}


NTV2VideoFormat DeviceServices::GetLockedInputVideoFormat()
{
	const int32_t kLockAttemps		= 3;
	const int32_t kLockSleepTimeMs	= 30;	

	NTV2VideoFormat frameBufferVideoFormat;
	mCard->GetVideoFormat(frameBufferVideoFormat);
	
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
        case NTV2_FBF_10BIT_DPX_LE:
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

	mCard->ReadRegister(kRegCh1Control, regValue, kRegMaskMode, kRegShiftMode);
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

bool DeviceServices::IsFormatRaw(NTV2FrameBufferFormat fbFormat)
{
	GeneralFrameFormat gFormat = GetGeneralFrameFormat(fbFormat);
	switch (gFormat)
	{
		case FORMAT_RAW:
		case FORMAT_RAW_HFR:
		case FORMAT_RAW_UHFR:
			return true;
		default:
			return false;
	}
}

bool DeviceServices::IsFormatCompressed(NTV2FrameBufferFormat fbFormat)
{
	switch (fbFormat)
	{
		
		case NTV2_FBF_PRORES_DVCPRO:
		case NTV2_FBF_PRORES_HDV:
		case NTV2_FBF_8BIT_DVCPRO:
		//case NTV2_FBF_8BIT_QREZ:
		case NTV2_FBF_8BIT_HDV:
			return true;
		default:
			return false;
	}
}


bool DeviceServices::IsCompatibleWithReference(NTV2VideoFormat fbFormat)
{
	// get reference frame rate
	ULWord status;
	mCard->ReadInputStatusRegister(&status);
	NTV2FrameRate refRate = (NTV2FrameRate)((status>>16)&0xF);
	NTV2FrameRate fbRate  = GetNTV2FrameRateFromVideoFormat(fbFormat);
	return IsCompatibleWithReference(fbRate, refRate);
}


bool DeviceServices::IsCompatibleWithReference(NTV2VideoFormat fbFormat, NTV2VideoFormat inputFormat)
{
	NTV2FrameRate fbRate  = GetNTV2FrameRateFromVideoFormat(fbFormat);
	NTV2FrameRate inputRate  = GetNTV2FrameRateFromVideoFormat(inputFormat);
	return IsCompatibleWithReference(fbRate, inputRate);
}


bool DeviceServices::IsCompatibleWithReference(NTV2FrameRate fbRate, NTV2FrameRate inputRate)
{
	bool bResult = false;

	switch(fbRate)
	{
		case NTV2_FRAMERATE_6000:
		case NTV2_FRAMERATE_3000:
		case NTV2_FRAMERATE_1500:
			bResult = (inputRate == NTV2_FRAMERATE_6000 || inputRate == NTV2_FRAMERATE_3000);
			break;
	
		case NTV2_FRAMERATE_5994:
		case NTV2_FRAMERATE_2997:
		case NTV2_FRAMERATE_1498:
			bResult = (inputRate == NTV2_FRAMERATE_5994 || inputRate == NTV2_FRAMERATE_2997);
			break;
			
		case NTV2_FRAMERATE_5000:
		case NTV2_FRAMERATE_2500:
			bResult = (inputRate == NTV2_FRAMERATE_5000 || inputRate == NTV2_FRAMERATE_2500);
			break;
	
		case NTV2_FRAMERATE_2398:
		case NTV2_FRAMERATE_2400:
		default:
			bResult = (inputRate == fbRate);
			break;
	}
	
	return bResult;
}


void DeviceServices::SetMacDebugOption(int item)
{
    if ( mCard->IsOpen() )
    {
		ULWord regVal = 0;
		mCard->ReadRegister( kVRegDebug1, regVal );
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
	// get csc rgb range
	NTV2RGB10Range cscRange = mRGB10Range;		// default use RGB range
	
	// for case where duallink(RGB) IO is supported
	if ( ::NTV2DeviceCanDoDualLink(mDeviceID) )
	{
		if (mFb1Mode == NTV2_MODE_DISPLAY)
		{
			// follow framebuffer RGB range
			if (NTV2_IS_FBF_RGB(mFb1Format))
				cscRange = mRGB10Range; 
			
			// follow output RGB range
			else									
				cscRange = (mSDIOutput1RGBRange == NTV2_RGBRangeFull) ? NTV2_RGB10RangeFull : NTV2_RGB10RangeSMPTE; 
		}
		
		else	// mFb1Mode == NTV2_MODE_CAPTURE
		{
			// follow input RGB range
			if (mSDIInput1ColorSpace == NTV2_ColorSpaceModeRgb)
			{
				cscRange = (mSDIInput1RGBRange == NTV2_RGBRangeFull) ? NTV2_RGB10RangeFull : NTV2_RGB10RangeSMPTE;
			}
			
			// follow framebuffer RGB range
			else									
				cscRange = mRGB10Range; 
		}
	}
	else if (mDeviceID == DEVICE_ID_KONAHDMI)
	{
		NTV2InputVideoType inType = RetailSupport::GetInputVideoTypeForIndex(mDeviceID, mVirtualInputSelect);
		if (inType >= NTV2_InputVideoHdmi1 && inType <= NTV2_InputVideoHdmi4)
		{
			NTV2HDMIRange rgbRange = NTV2_HDMIRangeFull;
			mCard->GetHDMIInputRange(rgbRange, NTV2_CHANNEL1);
			cscRange = (rgbRange == NTV2_HDMIRangeFull) ? NTV2_RGB10RangeFull : NTV2_RGB10RangeSMPTE;
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
	
	if ( mCard->ReadRegister(kRegAnalogInputStatus, status) == true )
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
	
	if ( mCard->ReadRegister(kRegAnalogInputStatus, status) == true )
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
	mCard->ReadRegister (kRegAnalogInputControl, value, kRegMaskAnalogInputADCMode, kRegShiftAnalogInputADCMode);
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
		bResult = ::NTV2DeviceCanDoConversionMode(mDeviceID, cMode);
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

	if (index == 0 && mVpid1Valid == true && mVpid1a != 0)
	{
		mVpidParser.SetVPID(mVpid1a);
		inputFormat = mVpidParser.GetVideoFormat();
		if (mVirtualInputSelect == NTV2_Input4x4kSelect || mVirtualInputSelect == NTV2_Input2x4kSelect)
			inputFormat = GetQuadSizedVideoFormat(inputFormat);
	}
	else if (index == 1 && mVpid2Valid == true && mVpid2a != 0)
	{
		mVpidParser.SetVPID(mVpid2a);
		inputFormat = mVpidParser.GetVideoFormat();
		if (mVirtualInputSelect == NTV2_Input4x4kSelect || mVirtualInputSelect == NTV2_Input2x4kSelect)
			inputFormat = GetQuadSizedVideoFormat(inputFormat);
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
	if (sdiInFormat != videoFormat && ::NTV2DeviceGetNumInputConverters(mDeviceID) > 0)
		sdiInFormat = GetConversionCompatibleFormat(sdiInFormat, mVirtualSecondaryFormatSelect);
	
	// HACK NOTICE 3
	// note: there is no enum for 2Kp60b et al
	// we special case define 2Kp60b as 2Kp60a with 3Gb flag set
	bool b1080pHfr = IsVideoFormatB(sdiInFormat);		// i.e. 1080p60b hfrs
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
			sdiInFormat = geometry == 8 ? NTV2_FORMAT_1080p_2K_5000_A : NTV2_FORMAT_1080p_5000_A;
		else if (sdiInFormat == NTV2_FORMAT_1080p_5994_B)
			sdiInFormat = geometry == 8 ? NTV2_FORMAT_1080p_2K_5994_A : NTV2_FORMAT_1080p_5994_A;
		else if (sdiInFormat == NTV2_FORMAT_1080p_6000_B)
			sdiInFormat = geometry == 8 ? NTV2_FORMAT_1080p_2K_6000_A : NTV2_FORMAT_1080p_6000_A;
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


bool DeviceServices::InputRequiresBToAConvertsion(NTV2Channel ch)
{
	bool b3GbInEnabled = false;
	mCard->GetSDIInput3GbPresent(b3GbInEnabled, ch);
	bool bConvert = b3GbInEnabled && IsVideoFormatA(mFb1VideoFormat);
	return bConvert;
}


// MARK: -

// IP common support routines

//-------------------------------------------------------------------------------------------------------
//	Support Routines
//-------------------------------------------------------------------------------------------------------
void DeviceServices::EveryFrameTask2110(CNTV2Config2110* config2110,
                                        NTV2VideoFormat* videoFormatLast,
                                        NetworkData2110* s2110NetworkLast,
                                        TransmitVideoData2110* s2110TxVideoDataLast,
                                        TransmitAudioData2110* s2110TxAudioDataLast,
                                        ReceiveVideoData2110* s2110RxVideoDataLast,
                                        ReceiveAudioData2110* s2110RxAudioDataLast)
{
    bool ipServiceEnable, ipServiceForceConfig;

    config2110->GetIPServicesControl(ipServiceEnable, ipServiceForceConfig);
    if (ipServiceEnable)
    {
        tx_2110Config txConfig;

        // Handle reset case
        if ((m2110TxVideoData.numTxVideoChannels == 0) &&
            (m2110TxAudioData.numTxAudioChannels == 0) &&
            (m2110RxVideoData.numRxVideoChannels == 0) &&
            (m2110RxAudioData.numRxAudioChannels == 0))
        {
            for (uint32_t i=0; i<4; i++)
            {
                if (memcmp(&m2110TxVideoData.txVideoCh[i], &s2110TxVideoDataLast->txVideoCh[i], sizeof(TxVideoChData2110)) != 0)
                {
                    printf("TX Video Reset disable %d\n", s2110TxVideoDataLast->txVideoCh[i].stream);
                    config2110->SetTxStreamEnable(s2110TxVideoDataLast->txVideoCh[i].stream, false, false);
                    s2110TxVideoDataLast->txVideoCh[i] = m2110TxVideoData.txVideoCh[i];
                }
                if (memcmp(&m2110TxAudioData.txAudioCh[i], &s2110TxAudioDataLast->txAudioCh[i], sizeof(TxAudioChData2110)) != 0)
                {
                    printf("TX Audio Reset disable %d\n", s2110TxAudioDataLast->txAudioCh[i].stream);
                    config2110->SetTxStreamEnable(s2110TxAudioDataLast->txAudioCh[i].stream, false, false);
                    s2110TxAudioDataLast->txAudioCh[i] = m2110TxAudioData.txAudioCh[i];
                }
                if (memcmp(&m2110RxVideoData.rxVideoCh[i], &s2110RxVideoDataLast->rxVideoCh[i], sizeof(RxVideoChData2110)) != 0)
                {
                    printf("RX Video Reset disable %d\n", s2110RxVideoDataLast->rxVideoCh[i].stream);
                    config2110->SetRxStreamEnable(SFP_1, s2110RxVideoDataLast->rxVideoCh[i].stream, false);
                    config2110->SetRxStreamEnable(SFP_2, s2110RxVideoDataLast->rxVideoCh[i].stream, false);
                    s2110RxVideoDataLast->rxVideoCh[i] = m2110RxVideoData.rxVideoCh[i];
                }
                if (memcmp(&m2110RxAudioData.rxAudioCh[i], &s2110RxAudioDataLast->rxAudioCh[i], sizeof(RxAudioChData2110)) != 0)
                {
                    printf("RX Audio Reset disable %d\n", s2110RxAudioDataLast->rxAudioCh[i].stream);
                    config2110->SetRxStreamEnable(SFP_1, s2110RxAudioDataLast->rxAudioCh[i].stream, false);
                    config2110->SetRxStreamEnable(SFP_2, s2110RxAudioDataLast->rxAudioCh[i].stream, false);
                    s2110RxAudioDataLast->rxAudioCh[i] = m2110RxAudioData.rxAudioCh[i];
                }
                m2110IpStatusData.txChStatus[i] = kIpStatusStopped;
                m2110IpStatusData.rxChStatus[i] = kIpStatusStopped;
            }
        }
        else
        {
            // See if any transmit video channels need configuring/enabling
            for (uint32_t i=0; i<m2110TxVideoData.numTxVideoChannels; i++)
            {
                if (memcmp(&m2110TxVideoData.txVideoCh[i], &s2110TxVideoDataLast->txVideoCh[i], sizeof(TxVideoChData2110)) != 0 ||
                    *videoFormatLast != mFb1VideoFormat ||
                    ipServiceForceConfig)
                {
                    // Process the configuration
                    txConfig.init();
                    txConfig.remoteIP[0] = m2110TxVideoData.txVideoCh[i].remoteIP[0];
                    txConfig.remoteIP[1] = m2110TxVideoData.txVideoCh[i].remoteIP[1];
                    txConfig.remotePort[0] = m2110TxVideoData.txVideoCh[i].remotePort[0];
                    txConfig.remotePort[1] = m2110TxVideoData.txVideoCh[i].remotePort[1];
                    txConfig.localPort[0] = m2110TxVideoData.txVideoCh[i].localPort[0];
                    txConfig.localPort[1] = m2110TxVideoData.txVideoCh[i].localPort[1];
                    txConfig.localPort[0] = m2110TxVideoData.txVideoCh[i].localPort[0];
                    txConfig.localPort[1] = m2110TxVideoData.txVideoCh[i].localPort[1];
                    txConfig.payloadType = m2110TxVideoData.txVideoCh[i].payloadType;
                    txConfig.ttl = 0x40;
                    txConfig.tos = 0x64;

                    // Video specific
                    txConfig.videoFormat = Convert21104KFormat(mFb1VideoFormat);
                    txConfig.videoSamples = VPIDSampling_YUV_422;

                    // Start by turning off the video stream
                    printf("SetTxVideoStream off %d\n", m2110TxVideoData.txVideoCh[i].stream);
                    config2110->SetTxStreamEnable(m2110TxVideoData.txVideoCh[i].stream, false, false);
                    m2110IpStatusData.txChStatus[i] = kIpStatusStopped;

                    if (config2110->SetTxStreamConfiguration(m2110TxVideoData.txVideoCh[i].stream, txConfig) == true)
                    {
                        printf("SetTxStreamConfiguration Video OK\n");
                        s2110TxVideoDataLast->txVideoCh[i] = m2110TxVideoData.txVideoCh[i];
                        SetIPError((NTV2Channel)m2110TxVideoData.txVideoCh[i].stream, kErrNetworkConfig, NTV2IpErrNone);

                        // Process the enable
                        if (m2110TxVideoData.txVideoCh[i].enable)
                        {
                            printf("SetTxVideoStream on %d\n", m2110TxVideoData.txVideoCh[i].stream);
                            config2110->SetTxStreamEnable(m2110TxVideoData.txVideoCh[i].stream,
                                                          (bool)m2110TxVideoData.txVideoCh[i].sfpEnable[0],
                                                          (bool)m2110TxVideoData.txVideoCh[i].sfpEnable[1]);
                            m2110IpStatusData.txChStatus[i] = kIpStatusRunning;
                        }
                    }
                    else
                    {
                        printf("SetTxStreamConfiguration Video ERROR %s\n", config2110->getLastError().c_str());
                        SetIPError((NTV2Channel)m2110TxVideoData.txVideoCh[i].stream, kErrNetworkConfig, config2110->getLastErrorCode());
                        m2110IpStatusData.txChStatus[i] = kIpStatusFail;
                    }
                    AgentIsAlive();
                }
            }

            // See if any transmit audio channels need configuring/enabling
            for (uint32_t i=0; i<m2110TxAudioData.numTxAudioChannels; i++)
            {
                if (memcmp(&m2110TxAudioData.txAudioCh[i], &s2110TxAudioDataLast->txAudioCh[i], sizeof(TxAudioChData2110)) != 0 ||
                    *videoFormatLast != mFb1VideoFormat ||
                    ipServiceForceConfig)
                {
                    // Process the configuration
                    txConfig.init();
                    txConfig.remoteIP[0] = m2110TxAudioData.txAudioCh[i].remoteIP[0];
                    txConfig.remoteIP[1] = m2110TxAudioData.txAudioCh[i].remoteIP[1];
                    txConfig.remotePort[0] = m2110TxAudioData.txAudioCh[i].remotePort[0];
                    txConfig.remotePort[1] = m2110TxAudioData.txAudioCh[i].remotePort[1];
                    txConfig.localPort[0] = m2110TxAudioData.txAudioCh[i].localPort[0];
                    txConfig.localPort[1] = m2110TxAudioData.txAudioCh[i].localPort[1];
                    txConfig.localPort[0] = m2110TxAudioData.txAudioCh[i].localPort[0];
                    txConfig.localPort[1] = m2110TxAudioData.txAudioCh[i].localPort[1];
                    txConfig.payloadType = m2110TxAudioData.txAudioCh[i].payloadType;
                    txConfig.ttl = 0x40;
                    txConfig.tos = 0x64;

                    // Audio specific
                    txConfig.channel = m2110TxAudioData.txAudioCh[i].channel;
                    txConfig.numAudioChannels = m2110TxAudioData.txAudioCh[i].numAudioChannels;
                    txConfig.firstAudioChannel = m2110TxAudioData.txAudioCh[i].firstAudioChannel;
                    txConfig.audioPktInterval = m2110TxAudioData.txAudioCh[i].audioPktInterval;

                    // Start by turning off the audio stream
                    printf("SetTxAudioStream off %d\n", m2110TxAudioData.txAudioCh[i].stream);
                    config2110->SetTxStreamEnable(m2110TxAudioData.txAudioCh[i].stream, false, false);

                    if (config2110->SetTxStreamConfiguration(m2110TxAudioData.txAudioCh[i].stream, txConfig) == true)
                    {
                        printf("SetTxStreamConfiguration Audio OK\n");
                        s2110TxAudioDataLast->txAudioCh[i] = m2110TxAudioData.txAudioCh[i];
                        SetIPError((NTV2Channel)m2110TxVideoData.txVideoCh[i].stream, kErrNetworkConfig, NTV2IpErrNone);

                        // Process the enable
                        if (m2110TxAudioData.txAudioCh[i].enable)
                        {
                            printf("SetTxAudioStream on %d\n", m2110TxAudioData.txAudioCh[i].stream);
                            config2110->SetTxStreamEnable(m2110TxAudioData.txAudioCh[i].stream,
                                                          (bool)m2110TxAudioData.txAudioCh[i].sfpEnable[0],
                                                          (bool)m2110TxAudioData.txAudioCh[i].sfpEnable[1]);
                        }
                    }
                    else
                    {
                        printf("SetTxStreamConfiguration Audio ERROR %s\n", config2110->getLastError().c_str());
                        SetIPError((NTV2Channel)m2110TxAudioData.txAudioCh[i].stream, kErrNetworkConfig, config2110->getLastErrorCode());
                    }
                    AgentIsAlive();
                }
            }

            rx_2110Config rxConfig;
            eSFP sfp = SFP_1;

            // See if any receive video channels need configuring/enabling
            for (uint32_t i=0; i<m2110RxVideoData.numRxVideoChannels; i++)
            {
                if (memcmp(&m2110RxVideoData.rxVideoCh[i], &s2110RxVideoDataLast->rxVideoCh[i], sizeof(RxVideoChData2110)) != 0 ||
                    *videoFormatLast != mFb1VideoFormat ||
                    ipServiceForceConfig)
                {
                    rxConfig.init();
                    if (m2110RxVideoData.rxVideoCh[i].sfpEnable[1])
                    {
                        // Use SFP 2 params
                        sfp = SFP_2;
                        rxConfig.rxMatch = 0x14;    // PSM hard code temporarily until we get params sent down properly
                        rxConfig.sourceIP = m2110RxVideoData.rxVideoCh[i].sourceIP[1];
                        rxConfig.destIP = m2110RxVideoData.rxVideoCh[i].destIP[1];
                        rxConfig.sourcePort = m2110RxVideoData.rxVideoCh[i].sourcePort[1];
                        rxConfig.destPort = m2110RxVideoData.rxVideoCh[i].destPort[1];
                        sfp = SFP_2;
                    }
                    else if (m2110RxVideoData.rxVideoCh[i].sfpEnable[0])
                    {
                        // Use SFP 1 params
                        sfp = SFP_1;
                        rxConfig.rxMatch = 0x14;    // PSM hard code temporarily until we get params sent down properly
                        rxConfig.sourceIP = m2110RxVideoData.rxVideoCh[i].sourceIP[0];
                        rxConfig.destIP = m2110RxVideoData.rxVideoCh[i].destIP[0];
                        rxConfig.sourcePort = m2110RxVideoData.rxVideoCh[i].sourcePort[0];
                        rxConfig.destPort = m2110RxVideoData.rxVideoCh[i].destPort[0];
                    }
                    rxConfig.payloadType = m2110RxVideoData.rxVideoCh[i].payloadType;

                    // Video specific
                    if (mFollowInputFormat && (m2110RxVideoData.rxVideoCh[i].videoFormat != NTV2_FORMAT_UNKNOWN))
                    {
                        rxConfig.videoFormat = Convert21104KFormat(m2110RxVideoData.rxVideoCh[i].videoFormat);

                        // if format was not converted assume it was not a 4k format and disable 4k mode, otherwise enable it
                        if (rxConfig.videoFormat == m2110RxVideoData.rxVideoCh[i].videoFormat)
                            config2110->Set4KModeEnable(false);
                        else
                            config2110->Set4KModeEnable(true);
                    }
                    else
                    {
                        rxConfig.videoFormat = Convert21104KFormat(mFb1VideoFormat);

                        // if format was not converted assume it was not a 4k format and disable 4k mode, otherwise enable it
                        if (rxConfig.videoFormat == mFb1VideoFormat)
                            config2110->Set4KModeEnable(false);
                        else
                            config2110->Set4KModeEnable(true);
                    }
                    rxConfig.videoSamples = VPIDSampling_YUV_422;
                    printf("Format (%d, %d, %d)\n", i, mFollowInputFormat, rxConfig.videoFormat);

                    // Start by turning off the video receiver
                    printf("SetRxVideoStream off %d\n", m2110RxVideoData.rxVideoCh[i].stream);
                    config2110->SetRxStreamEnable(sfp, m2110RxVideoData.rxVideoCh[i].stream, false);
                    m2110IpStatusData.rxChStatus[i] = kIpStatusStopped;

                    if (config2110->SetRxStreamConfiguration(sfp, m2110RxVideoData.rxVideoCh[i].stream, rxConfig) == true)
                    {
                        printf("SetRxStreamConfiguration Video OK\n");
                        s2110RxVideoDataLast->rxVideoCh[i] = m2110RxVideoData.rxVideoCh[i];
                        SetIPError((NTV2Channel)m2110RxVideoData.rxVideoCh[i].stream, kErrNetworkConfig, NTV2IpErrNone);

                        // Process the enable
                        if (m2110RxVideoData.rxVideoCh[i].enable)
                        {
                            printf("SetRxVideoStream on %d\n", m2110RxVideoData.rxVideoCh[i].stream);
                            config2110->SetRxStreamEnable(sfp, m2110RxVideoData.rxVideoCh[i].stream, true);
                            m2110IpStatusData.rxChStatus[i] = kIpStatusRunning;
                        }
                    }
                    else
                    {
                        printf("SetRxStreamConfiguration Video ERROR %s\n", config2110->getLastError().c_str());
                        SetIPError((NTV2Channel)m2110RxVideoData.rxVideoCh[i].stream, kErrNetworkConfig, config2110->getLastErrorCode());
                        m2110IpStatusData.rxChStatus[i] = kIpStatusFail;
                    }
                    AgentIsAlive();
                }
            }
            *videoFormatLast = mFb1VideoFormat;

            // See if any receive audio channels need configuring/enabling
            for (uint32_t i=0; i<m2110RxAudioData.numRxAudioChannels; i++)
            {
                if (memcmp(&m2110RxAudioData.rxAudioCh[i], &s2110RxAudioDataLast->rxAudioCh[i], sizeof(RxAudioChData2110)) != 0 || ipServiceForceConfig)
                {
                    rxConfig.init();
                    if (m2110RxAudioData.rxAudioCh[i].sfpEnable[1])
                    {
                        // Use SFP 2 params
                        sfp = SFP_2;
                        rxConfig.rxMatch = 0x14;    // PSM hard code temporarily until we get params sent down properly
                        rxConfig.sourceIP = m2110RxAudioData.rxAudioCh[i].sourceIP[1];
                        rxConfig.destIP = m2110RxAudioData.rxAudioCh[i].destIP[1];
                        rxConfig.sourcePort = m2110RxAudioData.rxAudioCh[i].sourcePort[1];
                        rxConfig.destPort = m2110RxAudioData.rxAudioCh[i].destPort[1];
                        sfp = SFP_2;
                    }
                    else if (m2110RxAudioData.rxAudioCh[i].sfpEnable[0])
                    {
                        // Use SFP 1 params
                        sfp = SFP_1;
                        rxConfig.rxMatch = 0x14;    // PSM hard code temporarily until we get params sent down properly
                        rxConfig.sourceIP = m2110RxAudioData.rxAudioCh[i].sourceIP[0];
                        rxConfig.destIP = m2110RxAudioData.rxAudioCh[i].destIP[0];
                        rxConfig.sourcePort = m2110RxAudioData.rxAudioCh[i].sourcePort[0];
                        rxConfig.destPort = m2110RxAudioData.rxAudioCh[i].destPort[0];
                    }
                    rxConfig.payloadType = m2110RxAudioData.rxAudioCh[i].payloadType;

                    // Audio specific
                    rxConfig.numAudioChannels = m2110RxAudioData.rxAudioCh[i].numAudioChannels;
                    rxConfig.audioPktInterval = m2110RxAudioData.rxAudioCh[i].audioPktInterval;

                    // Start by turning off the audio receiver
                    printf("SetRxAudioStream off %d\n", m2110RxAudioData.rxAudioCh[i].stream);
                    config2110->SetRxStreamEnable(sfp, m2110RxAudioData.rxAudioCh[i].stream, false);

                    if (config2110->SetRxStreamConfiguration(sfp, m2110RxAudioData.rxAudioCh[i].stream, rxConfig) == true)
                    {
                        printf("SetRxStreamConfiguration Audio OK\n");
                        s2110RxAudioDataLast->rxAudioCh[i] = m2110RxAudioData.rxAudioCh[i];
                        SetIPError(m2110RxAudioData.rxAudioCh[i].channel, kErrNetworkConfig, NTV2IpErrNone);

                        // Process the enable
                        if (m2110RxAudioData.rxAudioCh[i].enable)
                        {
                            printf("SetRxAudioStream on %d\n", m2110RxAudioData.rxAudioCh[i].stream);
                            config2110->SetRxStreamEnable(sfp, m2110RxAudioData.rxAudioCh[i].stream, true);
                        }
                    }
                    else
                    {
                        printf("SetRxStreamConfiguration Audio ERROR %s\n", config2110->getLastError().c_str());
                        SetIPError(m2110RxAudioData.rxAudioCh[i].channel, kErrNetworkConfig, config2110->getLastErrorCode());
                    }
                    AgentIsAlive();
                }
            }
        }
        
        // See if network needs configuring
        if (memcmp(&m2110Network, s2110NetworkLast, sizeof(NetworkData2110)) != 0 || ipServiceForceConfig)
        {
            *s2110NetworkLast = m2110Network;

            mCard->SetReference(NTV2_REFERENCE_SFP1_PTP);
            config2110->SetPTPDomain(m2110Network.ptpDomain);
            config2110->SetPTPPreferredGrandMasterId(m2110Network.ptpPreferredGMID);

            for (uint32_t i = 0; i < SFP_MAX_NUM_SFPS; i++)
            {
                eSFP sfp = SFP_1;
                if (i > 0)
                    sfp = SFP_2;

                bool rv;
                if (m2110Network.sfp[i].enable)
                {
                    rv =  config2110->SetNetworkConfiguration(sfp,
                                                              m2110Network.sfp[i].ipAddress,
                                                              m2110Network.sfp[i].subnetMask,
                                                              m2110Network.sfp[i].gateWay);
                    if (rv)
                    {
                        printf("SetNetworkConfiguration OK\n");
                        SetIPError(NTV2_CHANNEL1, kErrNetworkConfig, NTV2IpErrNone);
                    }
                    else
                    {
                        printf("SetNetworkConfiguration ERROR %s\n", config2110->getLastError().c_str());
                        SetIPError(NTV2_CHANNEL1, kErrNetworkConfig, config2110->getLastErrorCode());
                    }
                }
                else
                {
                    printf("DisableNetworkInterface\n");
                    config2110->DisableNetworkInterface(sfp);
                }
                AgentIsAlive();
            }
        }

        // Write status
        mCard->WriteVirtualData(kChStatusData2110, &m2110IpStatusData, sizeof(IpStatus2110));
        
        // Turn off force config
        config2110->SetIPServicesControl(ipServiceEnable, false);
    }
}

void DeviceServices::EveryFrameTask2022(CNTV2Config2022* config2022, NTV2Mode* modeLast, NTV2VideoFormat* videoFormatLast)
{
    bool					rv, rv2, enableChCard, enable2022_7Card;
    uint32_t				enableChServices;
    uint32_t				networkPathDiffCard;
    uint32_t                configErr;

    rx_2022_channel         rxHwConfig;
    tx_2022_channel         txHwConfig, txHwConfig2;
    bool                    ipServiceEnable, ipServiceForceConfig;

    config2022->GetIPServicesControl(ipServiceEnable, ipServiceForceConfig);
    if (ipServiceEnable)
    {
        // KonaIP network configuration
        string hwIp,hwNet,hwGate;       // current hardware config

        rv = config2022->GetNetworkConfiguration(SFP_1,hwIp,hwNet,hwGate);
        if (rv)
        {
            uint32_t ip, net, gate;
            ip   = inet_addr(hwIp.c_str());
            net  = inet_addr(hwNet.c_str());
            gate = inet_addr(hwGate.c_str());

            if ((ip != mEth0.ipc_ip) ||
                (net != mEth0.ipc_subnet) ||
                (gate != mEth0.ipc_gateway) ||
                ipServiceForceConfig)
            {
                SetNetConfig(config2022, SFP_1);
            }
        }
        else
            printf("GetNetworkConfiguration SFP_TOP - FAILED\n");

        rv = config2022->GetNetworkConfiguration(SFP_2,hwIp,hwNet,hwGate);
        if (rv)
        {
            uint32_t ip, net, gate;
            ip   = inet_addr(hwIp.c_str());
            net  = inet_addr(hwNet.c_str());
            gate = inet_addr(hwGate.c_str());

            if ((ip != mEth1.ipc_ip) ||
                (net != mEth1.ipc_subnet) ||
                (gate != mEth1.ipc_gateway) ||
                ipServiceForceConfig)
            {
                SetNetConfig(config2022, SFP_2);
            }
        }
        else
            printf("GetNetworkConfiguration SFP_BOTTOM - FAILED\n");

        // KonaIP look for changes in 2022-7 mode and NPD if enabled
        rv  = config2022->Get2022_7_Mode(enable2022_7Card, networkPathDiffCard);

        if (rv && ((enable2022_7Card != m2022_7Mode) || (enable2022_7Card && (networkPathDiffCard != mNetworkPathDiff))))
        {
            printf("NPD ser/card (%d %d)\n", mNetworkPathDiff, networkPathDiffCard);
            if (config2022->Set2022_7_Mode(m2022_7Mode, mNetworkPathDiff) == true)
            {
                printf("Set 2022_7Mode OK\n");
                SetIPError(NTV2_CHANNEL1, kErrRxConfig, NTV2IpErrNone);
            }
            else
            {
                printf("Set 2022_7Mode ERROR %s\n", config2022->getLastError().c_str());
                SetIPError(NTV2_CHANNEL1, kErrRxConfig, config2022->getLastErrorCode());
            }
        }

        // KonaIP Input configurations
        if (IsValidConfig(mRx2022Config1, m2022_7Mode))
        {
            // clear any previous error
            SetIPError(NTV2_CHANNEL1,kErrRxConfig,NTV2IpErrNone);
            rv  = config2022->GetRxChannelConfiguration(NTV2_CHANNEL1,rxHwConfig);
            rv2 = config2022->GetRxChannelEnable(NTV2_CHANNEL1,enableChCard);
            mCard->ReadRegister(kVRegRxcEnable1, enableChServices);
            if (rv && rv2)
            {
                // if the channel enable toggled
                if (enableChCard != (enableChServices ? true : false))
                {
                    config2022->SetRxChannelEnable(NTV2_CHANNEL1, false);

                    // if the channel is enabled
                    if (enableChServices)
                    {
                        SetRxConfig(config2022, NTV2_CHANNEL1, m2022_7Mode);
                        GetIPError(NTV2_CHANNEL1,kErrRxConfig,configErr);
                        if (!configErr)
                        {
                            config2022->SetRxChannelEnable(NTV2_CHANNEL1, true);
                        }
                    }
                }
                // if the channel is already enabled then check to see if a configuration has changed
                else if (enableChServices)
                {
                    if (NotEqual(rxHwConfig, mRx2022Config1, m2022_7Mode) ||
                        enable2022_7Card != m2022_7Mode ||
                        *modeLast != mFb1Mode ||
                        *videoFormatLast != mFb1VideoFormat)
                    {
                        config2022->SetRxChannelEnable(NTV2_CHANNEL1, false);
                        SetRxConfig(config2022, NTV2_CHANNEL1, m2022_7Mode);
                        GetIPError(NTV2_CHANNEL1,kErrRxConfig,configErr);
                        if (!configErr)
                        {
                            config2022->SetRxChannelEnable(NTV2_CHANNEL1, true);
                        }
                    }
                }
            }
            else printf("rxConfig ch 1 read failed\n");
        }
        else SetIPError(NTV2_CHANNEL1,kErrRxConfig,NTV2IpErrInvalidConfig);

        if (IsValidConfig(mRx2022Config2, m2022_7Mode))
        {
            // clear any previous error
            SetIPError(NTV2_CHANNEL2,kErrRxConfig,NTV2IpErrNone);
            rv  = config2022->GetRxChannelConfiguration(NTV2_CHANNEL2, rxHwConfig);
            rv2 = config2022->GetRxChannelEnable(NTV2_CHANNEL2, enableChCard);
            mCard->ReadRegister(kVRegRxcEnable2, enableChServices);
            if (rv && rv2)
            {
                // if the channel enable toggled
                if (enableChCard != (enableChServices ? true : false))
                {
                    config2022->SetRxChannelEnable(NTV2_CHANNEL2, false);

                    // if the channel is enabled
                    if (enableChServices)
                    {
                        SetRxConfig(config2022, NTV2_CHANNEL2, m2022_7Mode);
                        GetIPError(NTV2_CHANNEL2,kErrRxConfig,configErr);
                        if (!configErr)
                        {
                            config2022->SetRxChannelEnable(NTV2_CHANNEL2, true);
                        }
                    }
                }
                // if the channel is already enabled then check to see if a configuration has changed
                else if (enableChServices)
                {
                    if (NotEqual(rxHwConfig, mRx2022Config2, m2022_7Mode) ||
                        enable2022_7Card != m2022_7Mode ||
                        *modeLast != mFb1Mode ||
                        *videoFormatLast != mFb1VideoFormat)
                    {
                        config2022->SetRxChannelEnable(NTV2_CHANNEL2, false);
                        SetRxConfig(config2022, NTV2_CHANNEL2, m2022_7Mode);
                        GetIPError(NTV2_CHANNEL2,kErrRxConfig,configErr);
                        if (!configErr)
                        {
                            config2022->SetRxChannelEnable(NTV2_CHANNEL2, true);
                        }
                    }
                }
            }
            else printf("rxConfig ch 2 config read failed\n");
        }
        else SetIPError(NTV2_CHANNEL2,kErrRxConfig,NTV2IpErrInvalidConfig);

        // KonaIP output configurations
        if (IsValidConfig(mTx2022Config3, m2022_7Mode))
        {
            // clear any previous error
            SetIPError(NTV2_CHANNEL3,kErrTxConfig,NTV2IpErrNone);
            rv  = config2022->GetTxChannelConfiguration(NTV2_CHANNEL3, txHwConfig);
            rv2 = config2022->GetTxChannelEnable(NTV2_CHANNEL3, enableChCard);
            GetIPError(NTV2_CHANNEL3,kErrTxConfig,configErr);
            mCard->ReadRegister(kVRegTxcEnable3, enableChServices);
            if (rv && rv2)
            {
                // if the channel enable toggled
                if (enableChCard != (enableChServices ? true : false))
                {
                    config2022->SetTxChannelEnable(NTV2_CHANNEL3, false);

                    // if the channel is enabled
                    if (enableChServices)
                    {
                        SetTxConfig(config2022, NTV2_CHANNEL3, m2022_7Mode);
                        GetIPError(NTV2_CHANNEL3,kErrTxConfig,configErr);
                        if (!configErr)
                        {
                            config2022->SetTxChannelEnable(NTV2_CHANNEL3, true);
                        }
                    }
                }
                // if the channel is already enabled then check to see if a configuration has changed
                else if (enableChServices)
                {
                    if (NotEqual(txHwConfig, mTx2022Config3, m2022_7Mode) ||
                        configErr ||
                        enable2022_7Card != m2022_7Mode ||
                        *modeLast != mFb1Mode ||
                        *videoFormatLast != mFb1VideoFormat)
                    {
                        config2022->SetTxChannelEnable(NTV2_CHANNEL3, false);
                        SetTxConfig(config2022, NTV2_CHANNEL3, m2022_7Mode);
                        GetIPError(NTV2_CHANNEL3,kErrTxConfig,configErr);
                        if (!configErr)
                        {
                            config2022->SetTxChannelEnable(NTV2_CHANNEL3, true);
                        }
                    }
                }
            }
            else printf("txConfig ch 3 read failed\n");
        }
        else SetIPError(NTV2_CHANNEL3,kErrTxConfig,NTV2IpErrInvalidConfig);

        if (IsValidConfig(mTx2022Config4, m2022_7Mode))
        {
            // clear any previous error
            SetIPError(NTV2_CHANNEL4,kErrTxConfig,NTV2IpErrNone);
            rv  = config2022->GetTxChannelConfiguration(NTV2_CHANNEL4, txHwConfig2);
            rv2 = config2022->GetTxChannelEnable(NTV2_CHANNEL4, enableChCard);
            GetIPError(NTV2_CHANNEL4,kErrTxConfig,configErr);
            mCard->ReadRegister(kVRegTxcEnable4, enableChServices);
            if (rv && rv2)
            {
                // if the channel enable toggled
                if (enableChCard != (enableChServices ? true : false))
                {
                    config2022->SetTxChannelEnable(NTV2_CHANNEL4, false);

                    // if the channel is enabled
                    if (enableChServices)
                    {
                        SetTxConfig(config2022, NTV2_CHANNEL4, m2022_7Mode);
                        GetIPError(NTV2_CHANNEL4,kErrTxConfig,configErr);
                        if (!configErr)
                        {
                            config2022->SetTxChannelEnable(NTV2_CHANNEL4, true);
                        }
                    }
                }
                // if the channel is already enabled then check to see if a configuration has changed
                else if (enableChServices)
                {
                    if (NotEqual(txHwConfig2, mTx2022Config4, m2022_7Mode) ||
                        configErr ||
                        enable2022_7Card != m2022_7Mode ||
                        *modeLast != mFb1Mode ||
                        *videoFormatLast != mFb1VideoFormat)
                    {
                        config2022->SetTxChannelEnable(NTV2_CHANNEL4, false);
                        SetTxConfig(config2022, NTV2_CHANNEL4, m2022_7Mode);
                        GetIPError(NTV2_CHANNEL4,kErrTxConfig,configErr);
                        if (!configErr)
                        {
                            config2022->SetTxChannelEnable(NTV2_CHANNEL4, true);
                        }
                    }
                }
            }
            else printf("txConfig ch 4 read failed\n");
        }
        else
            SetIPError(NTV2_CHANNEL4,kErrTxConfig,NTV2IpErrInvalidConfig);
        
        *modeLast = mFb1Mode;
        *videoFormatLast = mFb1VideoFormat;
        
        config2022->SetIPServicesControl(true, false);
    }
}

NTV2VideoFormat DeviceServices::Convert21104KFormat(NTV2VideoFormat videoFormat)
{
    NTV2VideoFormat format;

    switch (videoFormat)
    {
        case NTV2_FORMAT_4x2048x1080p_2398:
        case NTV2_FORMAT_4x2048x1080psf_2398:
            format = NTV2_FORMAT_1080p_2K_2398;
            break;
        case NTV2_FORMAT_4x2048x1080p_2400:
        case NTV2_FORMAT_4x2048x1080psf_2400:
            format = NTV2_FORMAT_1080p_2K_2400;
            break;
        case NTV2_FORMAT_4x2048x1080p_2500:
        case NTV2_FORMAT_4x2048x1080psf_2500:
            format = NTV2_FORMAT_1080p_2K_2500;
            break;
        case NTV2_FORMAT_4x2048x1080p_2997:
        case NTV2_FORMAT_4x2048x1080psf_2997:
            format = NTV2_FORMAT_1080p_2K_2997;
            break;
        case NTV2_FORMAT_4x2048x1080p_3000:
        case NTV2_FORMAT_4x2048x1080psf_3000:
            format = NTV2_FORMAT_1080p_2K_3000;
            break;
        case NTV2_FORMAT_4x2048x1080p_4795:
            format = NTV2_FORMAT_1080p_2K_4795_A;
            break;
        case NTV2_FORMAT_4x2048x1080p_4800:
            format = NTV2_FORMAT_1080p_2K_4800_A;
            break;
        case NTV2_FORMAT_4x2048x1080p_5000:
            format = NTV2_FORMAT_1080p_2K_5000_A;
            break;
        case NTV2_FORMAT_4x2048x1080p_5994:
            format = NTV2_FORMAT_1080p_2K_5994_A;
            break;
        case NTV2_FORMAT_4x2048x1080p_6000:
            format = NTV2_FORMAT_1080p_2K_6000_A;
            break;

        case NTV2_FORMAT_4x1920x1080p_2398:
        case NTV2_FORMAT_4x1920x1080psf_2398:
            format = NTV2_FORMAT_1080p_2398;
            break;
        case NTV2_FORMAT_4x1920x1080p_2400:
        case NTV2_FORMAT_4x1920x1080psf_2400:
            format = NTV2_FORMAT_1080p_2400;
            break;
        case NTV2_FORMAT_4x1920x1080p_2500:
        case NTV2_FORMAT_4x1920x1080psf_2500:
            format = NTV2_FORMAT_1080p_2500;
            break;
        case NTV2_FORMAT_4x1920x1080p_2997:
        case NTV2_FORMAT_4x1920x1080psf_2997:
            format = NTV2_FORMAT_1080p_2997;
            break;
        case NTV2_FORMAT_4x1920x1080p_3000:
        case NTV2_FORMAT_4x1920x1080psf_3000:
            format = NTV2_FORMAT_1080p_3000;
            break;
        case NTV2_FORMAT_4x1920x1080p_5000:
            format = NTV2_FORMAT_1080p_5000_A;
            break;
        case NTV2_FORMAT_4x1920x1080p_5994:
            format = NTV2_FORMAT_1080p_5994_A;
            break;
        case NTV2_FORMAT_4x1920x1080p_6000:
            format = NTV2_FORMAT_1080p_6000_A;
            break;

        default:
            format = videoFormat;
            break;
    }
    
    return format;
}

void  DeviceServices::SetNetConfig(CNTV2Config2022* config, eSFP  port)
{
    string  ip,sub,gate;
    struct  in_addr addr;
    
    switch (port)
    {
        case SFP_2:
            addr.s_addr = mEth1.ipc_ip;
            ip = inet_ntoa(addr);
            addr.s_addr = mEth1.ipc_subnet;
            sub = inet_ntoa(addr);
            addr.s_addr = mEth1.ipc_gateway;
            gate = inet_ntoa(addr);
            break;
        case SFP_1:
        default:
            addr.s_addr = mEth0.ipc_ip;
            ip = inet_ntoa(addr);
            addr.s_addr = mEth0.ipc_subnet;
            sub = inet_ntoa(addr);
            addr.s_addr = mEth0.ipc_gateway;
            gate = inet_ntoa(addr);
            break;
    }
    
    if (config->SetNetworkConfiguration(port,ip,sub,gate) == true)
    {
        printf("SetNetworkConfiguration port=%d OK\n",(int)port);
        SetIPError(NTV2_CHANNEL1, kErrNetworkConfig, NTV2IpErrNone);
    }
    else
    {
        printf("SetNetworkConfiguration port=%d ERROR %s\n",(int)port, config->getLastError().c_str());
        SetIPError(NTV2_CHANNEL1, kErrNetworkConfig, config->getLastErrorCode());
    }
}

void DeviceServices::SetRxConfig(CNTV2Config2022* config, NTV2Channel channel, bool is2022_7)
{
    rx_2022_channel chan;
    struct in_addr addr;
    
    // Always enable sfp1 only enable sfp2 if 2022_7 enabled
    chan.sfp1Enable	= true;
    chan.sfp2Enable	= is2022_7;
    
    switch ((int)channel)
    {
        case NTV2_CHANNEL2:
            addr.s_addr             = mRx2022Config2.rxc_sfp1SourceIp;
            chan.sfp1SourceIP       = inet_ntoa(addr);
            addr.s_addr             = mRx2022Config2.rxc_sfp1DestIp;
            chan.sfp1DestIP         = inet_ntoa(addr);
            chan.sfp1RxMatch        = mRx2022Config2.rxc_sfp1RxMatch & 0x7fffffff;
            chan.sfp1SourcePort     = mRx2022Config2.rxc_sfp1SourcePort;
            chan.sfp1DestPort       = mRx2022Config2.rxc_sfp1DestPort;
            chan.sfp1Vlan           = mRx2022Config2.rxc_sfp1Vlan;
            
            addr.s_addr             = mRx2022Config2.rxc_sfp2SourceIp;
            chan.sfp2SourceIP       = inet_ntoa(addr);
            addr.s_addr             = mRx2022Config2.rxc_sfp2DestIp;
            chan.sfp2DestIP         = inet_ntoa(addr);
            chan.sfp2RxMatch        = mRx2022Config2.rxc_sfp2RxMatch & 0x7fffffff;
            chan.sfp2SourcePort     = mRx2022Config2.rxc_sfp2SourcePort;
            chan.sfp2DestPort       = mRx2022Config2.rxc_sfp2DestPort;
            chan.sfp2Vlan           = mRx2022Config2.rxc_sfp2Vlan;
            
            chan.ssrc               = mRx2022Config2.rxc_ssrc;
            chan.playoutDelay       = mRx2022Config2.rxc_playoutDelay;
            break;

        default:
        case NTV2_CHANNEL1:
            addr.s_addr             = mRx2022Config1.rxc_sfp1SourceIp;
            chan.sfp1SourceIP       = inet_ntoa(addr);
            addr.s_addr             = mRx2022Config1.rxc_sfp1DestIp;
            chan.sfp1DestIP         = inet_ntoa(addr);
            chan.sfp1RxMatch        = mRx2022Config1.rxc_sfp1RxMatch  & 0x7fffffff;
            chan.sfp1SourcePort     = mRx2022Config1.rxc_sfp1SourcePort;
            chan.sfp1DestPort       = mRx2022Config1.rxc_sfp1DestPort;
            chan.sfp1Vlan           = mRx2022Config1.rxc_sfp1Vlan;
            
            addr.s_addr             = mRx2022Config1.rxc_sfp2SourceIp;
            chan.sfp2SourceIP       = inet_ntoa(addr);
            addr.s_addr             = mRx2022Config1.rxc_sfp2DestIp;
            chan.sfp2DestIP         = inet_ntoa(addr);
            chan.sfp2RxMatch        = mRx2022Config1.rxc_sfp2RxMatch & 0x7fffffff;
            chan.sfp2SourcePort     = mRx2022Config1.rxc_sfp2SourcePort;
            chan.sfp2DestPort       = mRx2022Config1.rxc_sfp2DestPort;
            chan.sfp2Vlan           = mRx2022Config1.rxc_sfp2Vlan;
            
            chan.ssrc               = mRx2022Config1.rxc_ssrc;
            chan.playoutDelay       = mRx2022Config1.rxc_playoutDelay;
            break;
    }
    
    if (config->SetRxChannelConfiguration(channel,chan) == true)
    {
        printf("setRxConfig chn=%d OK\n",(int)channel);
        SetIPError(channel, kErrRxConfig, NTV2IpErrNone);
    }
    else
    {
        printf("setRxConfig chn=%d ERROR %s\n",(int)channel, config->getLastError().c_str());
        SetIPError(channel, kErrRxConfig, config->getLastErrorCode());
    }
}

void DeviceServices::SetTxConfig(CNTV2Config2022* config, NTV2Channel channel, bool is2022_7)
{
    tx_2022_channel chan;
    struct in_addr addr;
    
    // Always enable link A only enable link B if 2022_7 enabled
    chan.sfp1Enable	= true;
    chan.sfp2Enable	= is2022_7;
    
    switch((int)channel)
    {
        case NTV2_CHANNEL4:
            addr.s_addr             = mTx2022Config4.txc_sfp1RemoteIp;
            chan.sfp1RemoteIP      = inet_ntoa(addr);
            chan.sfp1LocalPort     = mTx2022Config4.txc_sfp1LocalPort;
            chan.sfp1RemotePort    = mTx2022Config4.txc_sfp1RemotePort;
            
            addr.s_addr             = mTx2022Config4.txc_sfp2RemoteIp;
            chan.sfp2RemoteIP      = inet_ntoa(addr);
            chan.sfp2LocalPort     = mTx2022Config4.txc_sfp2LocalPort;
            chan.sfp2RemotePort    = mTx2022Config4.txc_sfp2RemotePort;
            break;
        default:
            
        case NTV2_CHANNEL3:
            addr.s_addr             = mTx2022Config3.txc_sfp1RemoteIp;
            chan.sfp1RemoteIP      = inet_ntoa(addr);
            chan.sfp1LocalPort     = mTx2022Config3.txc_sfp1LocalPort;
            chan.sfp1RemotePort    = mTx2022Config3.txc_sfp1RemotePort;
            
            addr.s_addr             = mTx2022Config3.txc_sfp2RemoteIp;
            chan.sfp2RemoteIP      = inet_ntoa(addr);
            chan.sfp2LocalPort     = mTx2022Config3.txc_sfp2LocalPort;
            chan.sfp2RemotePort    = mTx2022Config3.txc_sfp2RemotePort;
            break;
    }
    
    if (config->SetTxChannelConfiguration(channel,chan) == true)
    {
        printf("setTxConfig chn=%d OK\n",(int)channel);
        SetIPError(channel, kErrTxConfig, NTV2IpErrNone);
    }
    else
    {
        printf("setTxConfig chn=%d ERROR %s\n",(int)channel, config->getLastError().c_str());
        SetIPError(channel, kErrTxConfig, config->getLastErrorCode());
    }
}

bool DeviceServices::IsValidConfig(rx2022Config & virtual_config, bool is2022_7)
{
    if (virtual_config.rxc_sfp1DestIp == 0) return false;

    // Insure the match is set to something.  At the very least have it match on dest IP if it is 0
    if (virtual_config.rxc_sfp1RxMatch == 0)
        virtual_config.rxc_sfp1RxMatch = 4;

    // We only care about looking at sfp2 settings if we are doing 2022_7
    if (is2022_7)
    {
        if (virtual_config.rxc_sfp2DestIp == 0) return false;

        if (virtual_config.rxc_sfp2RxMatch == 0)
            virtual_config.rxc_sfp2RxMatch = 4;
    }
    return true;
}

bool DeviceServices::IsValidConfig(const tx2022Config & virtual_config, bool is2022_7)
{
    if (virtual_config.txc_sfp1RemoteIp == 0) return false;
    if (virtual_config.txc_sfp1RemotePort == 0) return false;
    
    // We only care about looking at sfp2 settings if we are doing 2022_7
    if (is2022_7)
    {
        if (virtual_config.txc_sfp2RemoteIp == 0) return false;
        if (virtual_config.txc_sfp2RemotePort == 0) return false;
    }
    return true;
}

bool DeviceServices::NotEqual(const rx_2022_channel & hw_channel, const rx2022Config & virtual_config, bool is2022_7)
{
    uint32_t addr;
    
    if (virtual_config.rxc_sfp1SourcePort != hw_channel.sfp1SourcePort)return true;
    if (virtual_config.rxc_sfp1DestPort != hw_channel.sfp1DestPort) return true;
    if ((virtual_config.rxc_sfp1RxMatch & 0x7fffffff) != (hw_channel.sfp1RxMatch & 0x7fffffff)) return true;
    
    addr = inet_addr(hw_channel.sfp1DestIP.c_str());
    if (virtual_config.rxc_sfp1DestIp != addr) return true;
    
    addr = inet_addr(hw_channel.sfp1SourceIP.c_str());
    if (virtual_config.rxc_sfp1SourceIp != addr) return true;
    
    if (virtual_config.rxc_playoutDelay != hw_channel.playoutDelay) return true;
    
    // We only care about looking at sfp2 settings if we are doing 2022_7
    if (is2022_7)
    {
        if (virtual_config.rxc_sfp2SourcePort != hw_channel.sfp2SourcePort)return true;
        if (virtual_config.rxc_sfp2DestPort != hw_channel.sfp2DestPort) return true;
        if ((virtual_config.rxc_sfp2RxMatch & 0x7fffffff) != (hw_channel.sfp2RxMatch & 0x7fffffff)) return true;
        
        addr = inet_addr(hw_channel.sfp2DestIP.c_str());
        if (virtual_config.rxc_sfp2DestIp != addr) return true;
        
        addr = inet_addr(hw_channel.sfp2SourceIP.c_str());
        if (virtual_config.rxc_sfp2SourceIp != addr) return true;
    }
    
    return false;
}

bool DeviceServices::NotEqual(const tx_2022_channel & hw_channel, const tx2022Config & virtual_config, bool is2022_7)
{
    uint32_t addr;
    
    if (virtual_config.txc_sfp1LocalPort	!= hw_channel.sfp1LocalPort)  return true;
    if (virtual_config.txc_sfp1RemotePort != hw_channel.sfp1RemotePort) return true;
    
    addr = inet_addr(hw_channel.sfp1RemoteIP.c_str());
    if (virtual_config.txc_sfp1RemoteIp != addr) return true;
    
    // We only care about looking at sfp2 settings if we are doing 2022_7
    if (is2022_7)
    {
        if (virtual_config.txc_sfp2LocalPort != hw_channel.sfp2LocalPort)  return true;
        if (virtual_config.txc_sfp2RemotePort != hw_channel.sfp2RemotePort) return true;
        
        addr = inet_addr(hw_channel.sfp2RemoteIP.c_str());
        if (virtual_config.txc_sfp2RemoteIp != addr) return true;
    }
    
    return false;
}

void DeviceServices::SetIPError(NTV2Channel channel, uint32_t configType, uint32_t val)
{
    uint32_t errCode;
    uint32_t value = val & 0xff;
    uint32_t reg;
    
    switch( configType )
    {
        default:
        case kErrNetworkConfig:
            reg = kVRegKIPNetCfgError;
            break;
        case kErrTxConfig:
            reg = kVRegKIPTxCfgError;
            break;
        case kErrRxConfig:
            reg = kVRegKIPRxCfgError;
            break;
        case kErrJ2kEncoderConfig:
            reg = kVRegKIPEncCfgError;
            break;
        case kErrJ2kDecoderConfig:
            reg = kVRegKIPDecCfgError;
            break;
    }
    
    mCard->ReadRegister(reg, errCode);
    
    switch( channel )
    {
        default:
        case NTV2_CHANNEL1:
            errCode = (errCode & 0xffffff00) | value;
            break;
            
        case NTV2_CHANNEL2:
            errCode = (errCode & 0xffff00ff) | (value << 8);
            break;
            
        case NTV2_CHANNEL3:
            errCode = (errCode & 0xff00ffff) | (value << 16);
            break;
            
        case NTV2_CHANNEL4:
            errCode = (errCode & 0x00ffffff) | (value << 24);
            break;
    }
    
    mCard->WriteRegister(reg, errCode);
}

void DeviceServices::GetIPError(NTV2Channel channel, uint32_t configType, uint32_t & val)
{
    uint32_t errCode;
    uint32_t reg;
    
    switch( configType )
    {
        default:
        case kErrNetworkConfig:
            reg = kVRegKIPNetCfgError;
            break;
        case kErrTxConfig:
            reg = kVRegKIPTxCfgError;
            break;
        case kErrRxConfig:
            reg = kVRegKIPRxCfgError;
            break;
        case kErrJ2kEncoderConfig:
            reg = kVRegKIPEncCfgError;
            break;
        case kErrJ2kDecoderConfig:
            reg = kVRegKIPDecCfgError;
            break;
    }
    
    mCard->ReadRegister(reg, errCode);
    
    switch( channel )
    {
        default:
        case NTV2_CHANNEL1:
            errCode = errCode & 0xff;
            break;
            
        case NTV2_CHANNEL2:
            errCode = (errCode >> 8) & 0xff;
            break;
            
        case NTV2_CHANNEL3:
            errCode = (errCode >> 16) & 0xff;
            break;
            
        case NTV2_CHANNEL4:
            errCode = (errCode >> 24) & 0xff;
            break;
    }
    
    val = errCode;
}

void DeviceServices::PrintRxConfig(const rx_2022_channel chan)
{
    printf("sfp1Enable          %s\n", chan.sfp1Enable == true? "true":"false");
    printf("sfp1SourceIP        %s\n", chan.sfp1SourceIP.c_str());
    printf("sfp1DestIP			%s\n", chan.sfp1DestIP.c_str());
    printf("sfp1SourcePort		%d\n", chan.sfp1SourcePort);
    printf("sfp1DestPort        %d\n", chan.sfp1DestPort);
    printf("sfp1Vlan            %d\n", chan.sfp1Vlan);
    printf("sfp1RxMatch         %d\n", chan.sfp1RxMatch);

    printf("sfp2Enable          %s\n", chan.sfp2Enable == true? "true":"false");
    printf("sfp2SourceIP		%s\n", chan.sfp2SourceIP.c_str());
    printf("sfp2DestIP			%s\n", chan.sfp2DestIP.c_str());
    printf("sfp2SourcePort		%d\n", chan.sfp2SourcePort);
    printf("sfp2DestPort		%d\n", chan.sfp2DestPort);
    printf("sfp2Vlan			%d\n", chan.sfp2Vlan);
    printf("sfp2RxMatch         %d\n\n", chan.sfp2RxMatch);
}

void DeviceServices::PrintTxConfig(const tx_2022_channel chan)
{
    printf("sfp1Enable          %s\n", chan.sfp1Enable == true? "true":"false");
    printf("sfp1RemoteIP        %s\n", chan.sfp1RemoteIP.c_str());
    printf("sfp1LocalPort		%d\n", chan.sfp1LocalPort);
    printf("sfp1RemotePort		%d\n", chan.sfp1RemotePort);
    
    printf("sfp2Enable          %s\n", chan.sfp2Enable == true? "true":"false");
    printf("sfp2RemoteIP		%s\n", chan.sfp2RemoteIP.c_str());
    printf("sfp2LocalPort		%d\n", chan.sfp2LocalPort);
    printf("sfp2RemotePort		%d\n", chan.sfp2RemotePort);
}

void DeviceServices::PrintEncoderConfig(const j2kEncoderConfig modelConfig, j2kEncoderConfig encoderConfig)
{
    printf("videoFormat	   %6d%6d\n", modelConfig.videoFormat, encoderConfig.videoFormat);
    printf("ullMode		   %6d%6d\n", modelConfig.ullMode, encoderConfig.ullMode);
    printf("bitDepth	   %6d%6d\n", modelConfig.bitDepth, encoderConfig.bitDepth);
    printf("chromaSubsamp  %6d%6d\n", modelConfig.chromaSubsamp, encoderConfig.chromaSubsamp);
    printf("mbps		   %6d%6d\n", modelConfig.mbps, encoderConfig.mbps);
    printf("audioChannels  %6d%6d\n", modelConfig.audioChannels, encoderConfig.audioChannels);
    printf("streamType	   %6d%6d\n", modelConfig.streamType, encoderConfig.streamType);
    printf("pmtPid		   %6d%6d\n", modelConfig.pmtPid, encoderConfig.pmtPid);
    printf("videoPid	   %6d%6d\n", modelConfig.videoPid, encoderConfig.videoPid);
    printf("pcrPid		   %6d%6d\n", modelConfig.pcrPid, encoderConfig.pcrPid);
    printf("audio1Pid	   %6d%6d\n\n", modelConfig.audio1Pid, encoderConfig.audio1Pid);
}

void DeviceServices::PrintDecoderConfig(const j2kDecoderConfig modelConfig, j2kDecoderConfig encoderConfig)
{
    printf("selectionMode  %6d%6d\n", modelConfig.selectionMode, encoderConfig.selectionMode);
    printf("programNumber  %6d%6d\n", modelConfig.programNumber, encoderConfig.programNumber);
    printf("programPID	   %6d%6d\n", modelConfig.programPID, encoderConfig.programPID);
    printf("audioNumber    %6d%6d\n\n", modelConfig.audioNumber, encoderConfig.audioNumber);
}


void DeviceServices::Print2110Network(const NetworkData2110 m2110Network)
{
    PrintChArray("ptpMaster", &m2110Network.sfp[0].ipAddress[0]);
    //PrintChArray("ptpMaster", &m2110Network.sfp[0].subnetMask[0]);
    //PrintChArray("ptpMaster", &m2110Network.sfp[0].gateWay[0]);
    PrintChArray("ptpMaster", &m2110Network.sfp[1].ipAddress[0]);
    //PrintChArray("ptpMaster", &m2110Network.sfp[1].subnetMask[0]);
    //PrintChArray("ptpMaster", &m2110Network.sfp[1].gateWay[0]);
    printf("\n");

}

void DeviceServices::PrintChArray(const std::string title, const char* chstr)
{
    printf("%4s          ", title.c_str());
    for (uint32_t i=0; i< IP_STRSIZE; i++)
    {
        printf("%c", chstr[i]);
    }
    printf("\n");


}



// MARK: -

// based on the user ColorSpace mode and the current video format,
// set the ColorSpaceMatrixSelect
bool DeviceServices::UpdateK2ColorSpaceMatrixSelect()
{
	bool bResult = true;
	
	// if the board doesn't have LUTs, bail
	if ( !::NTV2DeviceCanDoProgrammableCSC(mDeviceID) )
		return bResult;

	// figure out what ColorSpace we want to be using
	NTV2ColorSpaceMatrixType matrix = NTV2_Rec709Matrix;
	switch (mColorSpaceType)
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
	int numberCSCs = ::NTV2DeviceGetNumCSCs(mDeviceID);
	
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
	
	bool bFb1RGB = IsRGBFormat(mFb1Format);

	// if the board doesn't have LUTs, bail
	if ( !::NTV2DeviceCanDoColorCorrection(mDeviceID) )
		return bResult;
	
	NTV2RGB10Range cscRange = GetCSCRange();

	// figure out which gamma LUTs we WANT to be using
	NTV2LutType wantedLUT = NTV2_LUTUnknown;
	switch (mGammaMode)
	{
		// custom LUT in use - do not change
		case NTV2_GammaNone:		
			wantedLUT = NTV2_LUTCustom;
			break;
		
		// old QuickTime 1.8 gamma - used for legacy QT apps (VTRX, FCP7) old time mac monitors
		case NTV2_GammaMac:	
			if (NTV2_IS_SD_VIDEO_FORMAT(mFb1VideoFormat) )
				wantedLUT = (cscRange == NTV2_RGB10RangeFull) ? NTV2_LUTGamma18_Rec601 : NTV2_LUTGamma18_Rec601_SMPTE;
			else
				wantedLUT = (cscRange == NTV2_RGB10RangeFull) ? NTV2_LUTGamma18_Rec709 : NTV2_LUTGamma18_Rec709_SMPTE;
			break;
		
		case NTV2_GammaAuto:	
			wantedLUT = NTV2_LUTLinear;
			break;
		
		// when in doubt use linear
		default:
		case NTV2_GammaRec601:	
		case NTV2_GammaRec709:		
			wantedLUT = NTV2_LUTLinear;	
			break;
	}
	
	// special case for RGB-to-RGB LUT conversion
	if (::NTV2DeviceCanDoDualLink(mDeviceID) && wantedLUT != NTV2_LUTCustom)
	{
		// convert to NTV2RGB10Range to NTV2RGBRangeMode to do the comparison
		NTV2RGBRangeMode fbRange = (mRGB10Range == NTV2_RGB10RangeFull) ? NTV2_RGBRangeFull : NTV2_RGBRangeSMPTE;
	
		if (mFb1Mode == NTV2_MODE_DISPLAY && bFb1RGB == true && mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)
		{
			wantedLUT = (fbRange == mSDIOutput1RGBRange) ? NTV2_LUTLinear : NTV2_LUTRGBRangeFull_SMPTE;
		}
		
		else if (mFb1Mode == NTV2_MODE_CAPTURE && bFb1RGB == true && mSDIInput1ColorSpace == NTV2_ColorSpaceModeRgb)
		{
			wantedLUT = NTV2_LUTRGBRangeFull_SMPTE;
		}
	}

	// Note some devices can to LUT5 but not LUT4 (e.g. Io4K UFC)
	bool bLut2 = ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtLUT2);
	bool bLut3 = ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtLUT3);
	bool bLut4 = ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtLUT4);
	bool bLut5 = ::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtLUT5);
	
	ULWord lut2Type = mLUTType, lut3Type = mLUTType, lut4Type = mLUTType, lut5Type = mLUTType;
	if (bLut2)
		mCard->ReadRegister(kVRegLUT2Type, lut2Type);
	if (bLut3)
		mCard->ReadRegister(kVRegLUT3Type, lut3Type);
	if (bLut4)
		mCard->ReadRegister(kVRegLUT4Type, lut4Type);
	if (bLut5)
		mCard->ReadRegister(kVRegLUT5Type, lut5Type);
	
	// test for special use of LUT2 for E-to-E rgb range conversion
	bool bE2ERangeConversion = (bLut2 == true) &&  (NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat) == false) && (mFb1Mode == NTV2_MODE_CAPTURE);
	
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
	
	mCard->ReadRegister(kRegInputStatus, status);

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
		mCard->ReadRegister(kRegRP188InOut1DBB, passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister (kRegRP188InOut1DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut1DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_WgtSDIOut2:
	case NTV2_Wgt3GSDIOut2:
		mCard->ReadRegister(kRegRP188InOut2DBB, passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister (kRegRP188InOut2DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut2DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_WgtSDIOut3:
	case NTV2_Wgt3GSDIOut3:
		mCard->ReadRegister(kRegRP188InOut3DBB, passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut3DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut3DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_WgtSDIOut4:
	case NTV2_Wgt3GSDIOut4:
		mCard->ReadRegister(kRegRP188InOut4DBB, passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut4DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut4DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_Wgt3GSDIOut5:
	case NTV2_WgtSDIMonOut1:
		mCard->ReadRegister(kRegRP188InOut5DBB, passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut5DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut5DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_Wgt3GSDIOut6:
		mCard->ReadRegister(kRegRP188InOut6DBB, passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut6DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut6DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_Wgt3GSDIOut7:
		mCard->ReadRegister(kRegRP188InOut7DBB, passthrough, BIT(23), 23);
		if (passthrough == 1)
		{
			mCard->WriteRegister(kRegRP188InOut7DBB, 0x0, BIT(23), 23);
			mCard->WriteRegister(kRegRP188InOut7DBB, 0xFF, kRegMaskRP188DBB, kRegShiftRP188DBB);
		}
		break;
	case NTV2_Wgt3GSDIOut8:
		mCard->ReadRegister(kRegRP188InOut8DBB, passthrough, BIT(23), 23);
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

void DeviceServices::WriteAudioSourceSelect(ULWord inValue, NTV2Channel inChannel)
{
	// read modify write with mask that contains holes
	ULWord mask = 0x0091ffff;
	ULWord curValue = 0;
	mCard->ReadAudioSource(curValue, inChannel);
	ULWord regValue = (curValue & ~mask) | (inValue & mask);
	mCard->WriteAudioSource(regValue, inChannel);
}


uint32_t DeviceServices::GetAudioDelayOffset(double frames)
{
	const uint32_t kBytesPerUnit = 512;		// each hardware click is 64 bytes
    
	NTV2FrameRate rate =  NTV2_FRAMERATE_UNKNOWN;
	mCard->GetFrameRate(rate);
    
	double frate		   = GetFramesPerSecond(rate);
	double samplesPerFrame = 48000.0 / frate;
	ULWord channels        = 16;
	mCard->GetNumberAudioChannels(channels);

	double  bytes          = samplesPerFrame * 4 * frames * channels;
	uint32_t offset        = uint32_t(bytes / kBytesPerUnit);
	
	return offset;
}


NTV2AudioSystem	DeviceServices::GetHostAudioSystem()
{
	NTV2AudioSystem hostAudioSystem = NTV2_AUDIOSYSTEM_1;
	if (mCard->DeviceCanDoAudioMixer())
	{
		uint32_t audioSystem = NTV2DeviceGetNumAudioSystems(mDeviceID);
		hostAudioSystem = (NTV2AudioSystem)audioSystem;
	}
	return hostAudioSystem;
}



// select square division or 2 pixel interleave in frame buffer
void DeviceServices::AdjustFor4kQuadOrTpiOut()
{
    if (NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat))
    {
   		bool bTpi = (m4kTransportOutSelection != NTV2_4kTransport_Quadrants_2wire &&
					 m4kTransportOutSelection != NTV2_4kTransport_Quadrants_4wire);
   		Set4kTpiState(bTpi);
    }
}


// select square division or 2 pixel interleave in frame buffer
void DeviceServices::AdjustFor4kQuadOrTpiIn(NTV2VideoFormat inputFormat, bool b2pi)
{
    if (NTV2_IS_4K_VIDEO_FORMAT(inputFormat))
    {
   		Set4kTpiState(b2pi);
    }
}

// true to tpi, false for quads
void DeviceServices::Set4kTpiState(bool bTpi)
{
	bool bEnabled = false;
	if (bTpi)
	{
		mCard->GetTsiFrameEnable(bEnabled, NTV2_CHANNEL1);
		if (bEnabled == false)
			mCard->SetTsiFrameEnable(true, NTV2_CHANNEL1);
	}
	else // quad
	{
		mCard->Get4kSquaresEnable(bEnabled, NTV2_CHANNEL1);
		if (bEnabled == false)
			mCard->Set4kSquaresEnable(true, NTV2_CHANNEL1);
	}
}


//
// MARK: Base Service -
//

void DeviceServices::SetDeviceXPointCapture()
{
	//mCard->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_ON, NTV2_AUDIOSYSTEM_1);

	//bool b4K = NTV2_IS_4K_VIDEO_FORMAT(mFb1VideoFormat);
	bool hasBiDirectionalSDI = NTV2DeviceHasBiDirectionalSDI(mDeviceID);
	
	NTV2WidgetID inputSelectID = NTV2_Wgt3GSDIIn1;
	if(mVirtualInputSelect == NTV2_Input2Select)
		inputSelectID = NTV2_Wgt3GSDIIn2;

	if ((mDeviceID != DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) &&
        (mDeviceID != DEVICE_ID_KONAIP_2TX_1SFP_J2K) &&
        (mDeviceID != DEVICE_ID_KONAIP_2RX_1SFP_J2K) &&
        (mDeviceID != DEVICE_ID_KONAIP_2110) &&
		(mDeviceID != DEVICE_ID_IOIP_2110))
	{
		SetAudioInputSelect(mInputAudioSelect);

		// The reference (genlock) source: if it's a video input, make sure it matches our current selection
		ReferenceSelect refSelect = NTV2DeviceHasGenlockv2(mDeviceID) ? kVideoIn : mCaptureReferenceSelect;
		switch (refSelect)
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
				switch(mDeviceID)
				{
				case DEVICE_ID_KONAHDMI:
					mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
					break;
				default:
					mCard->SetReference(NTV2_REFERENCE_INPUT1);
					break;
				}
				break;
			case NTV2_Input2Select:
				switch(mDeviceID)
				{
				case DEVICE_ID_KONALHI:
				case DEVICE_ID_IOEXPRESS:
					mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
					break;
				case DEVICE_ID_KONALHEPLUS:
					mCard->SetReference(NTV2_REFERENCE_ANALOG_INPUT);
					break;
				case DEVICE_ID_KONAHDMI:
					mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT2);
				default:
					mCard->SetReference(NTV2_REFERENCE_INPUT2);
					break;
				}
				break;
			case NTV2_Input3Select:
				{
					switch(mDeviceID)
					{
					case DEVICE_ID_KONALHI:
						mCard->SetReference(NTV2_REFERENCE_ANALOG_INPUT);
						break;
					case DEVICE_ID_KONAHDMI:
						mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT3);
						break;
					case DEVICE_ID_IO4KUFC:
					case DEVICE_ID_IOXT:
					default:
						mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
						break;
					}
				}
				break;
			case NTV2_Input4Select:
				{
					switch(mDeviceID)
					{
					case DEVICE_ID_KONAHDMI:
						mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT3);
						break;
					default: 
						break;
					}
				}
				break;
			case NTV2_Input5Select:
				{
					switch(mDeviceID)
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
    // For 2110 need to set PTP as reference
    else if ((mDeviceID == DEVICE_ID_KONAIP_2110) ||
             (mDeviceID == DEVICE_ID_IOIP_2110))
    {
        SetAudioInputSelect(mInputAudioSelect);
        mCard->SetReference(NTV2_REFERENCE_SFP1_PTP);
    }
	// For J2K devices we don't set the audio input select reg, audio input
	// has to come from AES and the configuration code will set this properly
	// we also force the reference on input to NTV2_REFERENCE_SFP1_PCR
	else
	{
		mCard->SetReference(NTV2_REFERENCE_SFP1_PCR);
	}

	if(mDeviceID != DEVICE_ID_KONAHDMI)
	{
		//Following the logic from each individual file
		//this should cover almost all cases

		//Fall through so every output is setup
		//1 and 2 get set if they are not bi-directional
		//otherwise 1 and 2 are direction input
		switch(NTV2DeviceGetNumVideoInputs(mDeviceID))
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
	if(NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtSDIMonOut1))
		EnableRP188EtoE(inputSelectID, NTV2_Wgt3GSDIOut5);

	// set custom anc input select
	if (NTV2DeviceCanDoCustomAnc(mDeviceID) == true)
	{
		uint32_t numSdiInputs = NTV2DeviceGetNumVideoInputs(mDeviceID);
		uint32_t selectedAncInput = mVirtualInputSelect;

		if (selectedAncInput >= numSdiInputs)
			selectedAncInput = 0;

		mCard->WriteRegister(kVRegCustomAncInputSelect, selectedAncInput);
	}

}

void DeviceServices::SetDeviceXPointPlayback()
{
	bool bDSKNeedsInputRef 	= false;
	bool bFb2RGB 			= IsRGBFormat(mFb2Format);
	bool bDSKGraphicMode 	= mDSKMode == NTV2_DSKModeGraphicOverMatte || 
							  mDSKMode == NTV2_DSKModeGraphicOverVideoIn || 
							  mDSKMode == NTV2_DSKModeGraphicOverFB;
	bool bDSKOn 			= mDSKMode == NTV2_DSKModeFBOverMatte || 
							  mDSKMode == NTV2_DSKModeFBOverVideoIn || 
							  (bFb2RGB && bDSKGraphicMode);
	
	SetAudioInputSelect(mInputAudioSelect);

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
	bool lockV2 = NTV2DeviceHasGenlockv2(mDeviceID);
	ReferenceSelect refSelect = bDSKNeedsInputRef ? mCaptureReferenceSelect : mDisplayReferenceSelect;
	
    if ((mDeviceID != DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K) &&
        (mDeviceID != DEVICE_ID_KONAIP_2TX_1SFP_J2K) &&
        (mDeviceID != DEVICE_ID_KONAIP_2RX_1SFP_J2K) &&
        (mDeviceID != DEVICE_ID_KONAIP_2110) &&
        (mDeviceID != DEVICE_ID_IOIP_2110))
    {

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
                NTV2VideoFormat inputFormat = GetSelectedInputVideoFormat(mFb1VideoFormat, NULL);
                if (IsCompatibleWithReference(mFb1VideoFormat, inputFormat) == false)
                {
                    mCard->SetReference(NTV2_REFERENCE_FREERUN);
                }
                else
                {
                    //Some boards have HDMI some have analog LHi has both
                    switch (mVirtualInputSelect)
                    {
                    default:
                    case NTV2_Input1Select:
                        mCard->SetReference(NTV2_REFERENCE_INPUT1);
                        break;
                    case NTV2_Input2Select:
                        switch(mDeviceID)
                        {
                        case DEVICE_ID_KONALHI:
                        case DEVICE_ID_IOEXPRESS:
                            mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
                            break;
                        case DEVICE_ID_KONALHEPLUS:
                            mCard->SetReference(NTV2_REFERENCE_ANALOG_INPUT);
                            break;
                        default:
                            mCard->SetReference(NTV2_REFERENCE_INPUT2);
                            break;
                        }
                        break;
                    case NTV2_Input3Select:
                        switch(mDeviceID)
                        {
                        default:
                        case DEVICE_ID_IOXT:
                        case DEVICE_ID_IO4KUFC:
                            mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
                            break;
                        case DEVICE_ID_KONALHI:
                            mCard->SetReference(NTV2_REFERENCE_ANALOG_INPUT);
                            break;
                        }
                        break;
                    case NTV2_Input5Select://Only used by io4k quad id
                        if (lockV2)
                            mCard->SetReference(NTV2_REFERENCE_FREERUN);
                        else
                            mCard->SetReference(NTV2_REFERENCE_HDMI_INPUT);
                        break;
                    } // end switch
                } // end else
            
            } // end switch-case
            break;
        
        }// end switch (refSelect)
    }
    // For 2110 need to set PTP as reference
    else if ((mDeviceID == DEVICE_ID_KONAIP_2110) ||
             (mDeviceID == DEVICE_ID_IOIP_2110))
    {
        mCard->SetReference(NTV2_REFERENCE_SFP1_PTP);
    }
    // For J2K devices we don't set the audio input select reg, audio input
    // has to come from AES and the configuration code will set this properly
    // we also force the reference on input to NTV2_REFERENCE_SFP1_PCR
    else
    {
        mCard->SetReference(NTV2_REFERENCE_SFP1_PCR);
    }        

	if (mAudioMixerOverrideState == false)
		mCard->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_OFF, NTV2_AUDIOSYSTEM_1);

	switch(NTV2DeviceGetNumVideoInputs(mDeviceID))
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
		if(!NTV2DeviceHasBiDirectionalSDI(mDeviceID))
		{
			DisableRP188EtoE(NTV2_Wgt3GSDIOut2);
		}
	default:
	case 1:
		if(!NTV2DeviceHasBiDirectionalSDI(mDeviceID))
		{
			DisableRP188EtoE(NTV2_Wgt3GSDIOut1);
		}
		break;
	}
	if(NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtSDIMonOut1))
	{
		DisableRP188EtoE(NTV2_Wgt3GSDIOut5);
	}
}


void DeviceServices::SetDeviceXPointPlaybackRaw()
{


	// CSC 1
	mCard->Connect (NTV2_XptCSC1VidInput, NTV2_XptBlack);
	// CSC 2
	mCard->Connect (NTV2_XptCSC2VidInput, NTV2_XptBlack);
	// CSC 3
	mCard->Connect (NTV2_XptCSC3VidInput, NTV2_XptBlack);
	// CSC 4
	mCard->Connect (NTV2_XptCSC4VidInput, NTV2_XptBlack);
	// CSC 5
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtCSC5))
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
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtLUT5))
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
	mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_DISPLAY);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
	mCard->WriteRegister(kRegCh2Control, 0, kRegMaskChannelDisable, kRegShiftChannelDisable);
	
	GeneralFrameFormat format = GetGeneralFrameFormat(mFb1Format);
	if (format == FORMAT_RAW_HFR || format == FORMAT_RAW_UHFR)
	{
		mCard->SetMode(NTV2_CHANNEL3, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL3, mFb1Format);
		mCard->SetMode(NTV2_CHANNEL4, NTV2_MODE_DISPLAY);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL4, mFb1Format);
		
		mCard->WriteRegister(kRegCh3Control, 0, kRegMaskChannelDisable, kRegShiftChannelDisable);
		mCard->WriteRegister(kRegCh4Control, 0, kRegMaskChannelDisable, kRegShiftChannelDisable);
	}
	else
	{
		mCard->WriteRegister(kRegCh3Control, 1, kRegMaskChannelDisable, kRegShiftChannelDisable);
		mCard->WriteRegister(kRegCh4Control, 1, kRegMaskChannelDisable, kRegShiftChannelDisable);
	}

	// 4K Down Converter
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_Wgt4KDownConverter))
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
	
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtDualLinkV2Out5))
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
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_Wgt3GSDIOut5))
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


void DeviceServices::SetDeviceXPointCaptureRaw()
{
	// call superclass first
	DeviceServices::SetDeviceXPointCapture();
	GeneralFrameFormat format = GetGeneralFrameFormat(mFb1Format);
	
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
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtCSC5))
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
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtLUT5))
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
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtDualLinkV2Out5))
	{
		mCard->Connect (NTV2_XptDualLinkOut5Input, NTV2_XptBlack);
	}
	
	
	// Frame Buffer 1
	int bFb1Disable=false, bFb2Disable=false, bFb3Disable=false, bFb4Disable=false;
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
			bFb3Disable = true;
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
			bFb4Disable = true;
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
	mCard->SetMode(NTV2_CHANNEL2, NTV2_MODE_CAPTURE);
	mCard->SetFrameBufferFormat(NTV2_CHANNEL2, mFb1Format);
	if (format == FORMAT_RAW_HFR || format == FORMAT_RAW_UHFR)
	{
		mCard->SetMode(NTV2_CHANNEL3, NTV2_MODE_CAPTURE);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL3, mFb1Format);
		mCard->SetMode(NTV2_CHANNEL4, NTV2_MODE_CAPTURE);
		mCard->SetFrameBufferFormat(NTV2_CHANNEL4, mFb1Format);
	}

	// Frame Buffer (1 2 3 4) disable
	mCard->WriteRegister(kRegCh1Control, bFb1Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh2Control, bFb2Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh3Control, bFb3Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	mCard->WriteRegister(kRegCh4Control, bFb4Disable, kRegMaskChannelDisable, kRegShiftChannelDisable);
	
	
	// 4K Down Converter
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_Wgt4KDownConverter))
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
			if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)
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
			if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)
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
			if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)
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
	if (::NTV2DeviceCanDoWidget(mDeviceID, NTV2_WgtSDIMonOut1))
	{
		switch (format)
		{
			default:
			case FORMAT_RAW:
			case FORMAT_RAW_HFR:
				if (mSDIOutput1ColorSpace == NTV2_ColorSpaceModeRgb)
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
	ULWord regValue = 0;
	// convert from enum to actual register bits
	
    // pick reasonable selection for Auto 
    if (input == NTV2_AutoAudioSelect)
		input = RetailSupport::AutoSelectAudioInput(mDeviceID, mVirtualInputSelect);
	
	switch (input)
	{
		case NTV2_Input1Embedded1_8Select:		regValue = 0x00004321;  break;
		case NTV2_Input1Embedded9_16Select:		regValue = 0x00008765;  break;
		case NTV2_Input2Embedded1_8Select:		regValue = 0x00014321;  break;
		case NTV2_Input2Embedded9_16Select:		regValue = 0x00018765;  break;
		case NTV2_HDMISelect:					regValue = 0x0010000A;  break;
		case NTV2_HDMI2Select:					regValue = 0x0011000A;  break;
		case NTV2_HDMI3Select:					regValue = 0x0090000A;  break;
		case NTV2_HDMI4Select:					regValue = 0x0091000A;  break;
		case NTV2_MicInSelect:					regValue = 0x0010000B;	break;
		case NTV2_AnalogSelect:					regValue = 0x00009999;  break;
		case NTV2_AES_EBU_XLRSelect:
		case NTV2_AES_EBU_BNCSelect:
		case NTV2_AutoAudioSelect:
		default:								regValue = 0x00000000;  break;
	}

	// write the reg value to hardware
	WriteAudioSourceSelect(regValue);
	if(mCard->DeviceCanDoAudioMixer())
	{
		WriteAudioSourceSelect(regValue, NTV2_CHANNEL2);
		if (mAudioMixerOverrideState == false)
			mCard->SetAudioLoopBack(NTV2_AUDIO_LOOPBACK_ON, NTV2_AUDIOSYSTEM_2);
		
		// Host System Audio Input
		NTV2AudioSource audioSource;
		NTV2EmbeddedAudioInput embedVideo = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_1;
		switch (input)
		{
			default:
			case NTV2_Input1Embedded1_8Select:		audioSource = NTV2_AUDIO_EMBEDDED;  break;
			case NTV2_Input1Embedded9_16Select:		audioSource = NTV2_AUDIO_EMBEDDED;  break;
			case NTV2_Input2Embedded1_8Select:		audioSource = NTV2_AUDIO_EMBEDDED;  embedVideo = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2; break;
			case NTV2_Input2Embedded9_16Select:		audioSource = NTV2_AUDIO_EMBEDDED;  embedVideo = NTV2_EMBEDDED_AUDIO_INPUT_VIDEO_2; break;
			case NTV2_HDMISelect:					audioSource = NTV2_AUDIO_HDMI;		break;
			case NTV2_MicInSelect:					audioSource = NTV2_AUDIO_MIC;		break;
			case NTV2_AnalogSelect:					audioSource = NTV2_AUDIO_ANALOG;	break;
			case NTV2_AES_EBU_XLRSelect:			audioSource = NTV2_AUDIO_AES;		break;
			case NTV2_AES_EBU_BNCSelect:			audioSource = NTV2_AUDIO_AES;		break;
		}
		NTV2AudioSystem hostAudioSystem = GetHostAudioSystem();
		mCard->SetAudioSystemInputSource(hostAudioSystem, audioSource, embedVideo);
	}

	// in addition, if this is an AES input select the correct physical layer
	if (input == NTV2_AES_EBU_BNCSelect)
		mCard->WriteRegister(kRegAud1Control, 0, kK2RegMaskKBoxAudioInputSelect, kK2RegShiftKBoxAudioInputSelect);
	else if (input == NTV2_AES_EBU_XLRSelect)
		mCard->WriteRegister(kRegAud1Control, 1, kK2RegMaskKBoxAudioInputSelect, kK2RegShiftKBoxAudioInputSelect);
}


//-------------------------------------------------------------------------------------------------------
//	AgentIsAlive - CP checks the kVRegAgentCheck virtual register to see if I'm still running...
//-------------------------------------------------------------------------------------------------------
void DeviceServices::AgentIsAlive()
{
    uint32_t count(0);
    mCard->ReadRegister(kVRegAgentCheck, count);
    count++;
    mCard->WriteRegister(kVRegAgentCheck, count);
}
