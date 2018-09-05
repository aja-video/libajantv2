//
//  ntv2deviceservices.h
//
//  Copyright (c) 2018 AJA Video, Inc. All rights reserved.
//

#ifndef _DeviceServices_
#define _DeviceServices_

#include "ntv2utils.h"
#include "ntv2vpid.h"
#include "ntv2card.h"
#include "ntv2config2022.h"
#include "virtualregistermodel.h"

#include <string.h>

#ifndef ISO_CONVERT_FMT
#define ISO_CONVERT_FMT(fmt)	(mIsoConvertEnable && (fmt == NTV2_FORMAT_525_5994 || fmt == NTV2_FORMAT_625_5000))
#endif

#define kADCStabilizeCount	10
#define kHDMIStartupPhase0	16
#define kHDMIStartupPhase1	11
#define kHDMIStartupPhase2	6
#define kHDMIStartupPhase3	1

typedef enum 
{ 
	FORMAT_RGB, 
	FORMAT_YUV, 
	FORMAT_COMPRESSED,
	FORMAT_RAW,
	FORMAT_RAW_HFR,
	FORMAT_RAW_UHFR
	
} GeneralFrameFormat;

#define AUDIO_DELAY_WRAPAROUND    8160    // for 4Mb buffer


//-------------------------------------------------------------------------------------------------------
//	class HLState
//-------------------------------------------------------------------------------------------------------
 class HLState
 {
public:
	explicit HLState() { init(); }
	HLState(const HLState& s)  { copy(s); }
	virtual ~HLState() {}
	
	void init();
	void copy(const HLState& s);
	virtual HLState& operator=(const HLState& s);
	virtual bool operator==(const HLState& s);

public:
	// misc regs
	bool					bHdmiIn;
	bool					bFbLevelA;
	bool					b4K;
	bool					b4kHfr;
	bool					bHfr;
	bool					bSdiOutRGB;
	bool					b4k6gOut;
	bool					b4k12gOut;
	NTV2FrameRate			primaryFrameRate;
};


//-------------------------------------------------------------------------------------------------------
//	class DeviceServices
//-------------------------------------------------------------------------------------------------------
class DeviceServices
{
public:
	static DeviceServices* CreateDeviceServices(NTV2DeviceID deviceID);	// factory
	
public:
	uint32_t				mHDMIStartupCountDown;
	
public:
			DeviceServices();
	virtual	~DeviceServices() {}
	virtual void ReadDriverState();
	virtual void UpdateAutoState();

	// override these
	virtual void SetDeviceEveryFrameRegs(uint32_t virtualDebug1, uint32_t everyFrameTaskFilter);
	virtual void SetDeviceEveryFrameRegs();
	virtual void SetDeviceXPointPlayback();
	virtual void SetDeviceXPointCapture();
	virtual void SetDeviceMiscRegisters();
	
	virtual NTV2VideoFormat GetLockedInputVideoFormat();
	virtual NTV2ColorSpaceMode GetSDIInputColorSpace(NTV2Channel inChannel, NTV2ColorSpaceMode inMode);
	virtual NTV2VideoFormat GetSelectedInputVideoFormat(NTV2VideoFormat referenceFormat, NTV2ColorSpaceMode* inputColorSpace=NULL);
    virtual NTV2VideoFormat GetCorrespondingAFormat(NTV2VideoFormat bVideoFormat);
	virtual void SetDeviceXPointPlaybackRaw();
	virtual void SetDeviceXPointCaptureRaw();
	virtual void SetDeviceMiscRegistersRaw(NTV2Mode mode) {(void)mode;}
	virtual void DisableStartupSequence() {mStartupDisabled = true;}

    // common IP support routines
    virtual void EveryFrameTask2022(CNTV2Config2022* config2022, NTV2Mode* modeLast, NTV2VideoFormat* videoFormatLast);
    virtual void EveryFrameTask2110(CNTV2Config2110* config2110,
                                    NTV2VideoFormat* videoFormatLast,
                                    NetworkData2110* s2110NetworkLast,
                                    TransmitVideoData2110* s2110TxVideoDataLast,
                                    TransmitAudioData2110* s2110TxAudioDataLast,
                                    ReceiveVideoData2110* s2110RxVideoDataLast,
                                    ReceiveAudioData2110* s2110RxAudioDataLast);

    virtual NTV2VideoFormat Convert21104KFormat(NTV2VideoFormat video4KFormat);
    virtual void SetNetConfig(CNTV2Config2022* config, eSFP  port);
    virtual void SetRxConfig(CNTV2Config2022* config, NTV2Channel channel, bool is2022_7);
    virtual void SetTxConfig(CNTV2Config2022* config, NTV2Channel channel, bool is2022_7);
    virtual bool IsValidConfig(rx2022Config & virtual_config, bool is2022_7);
    virtual bool IsValidConfig(const tx2022Config & virtual_config, bool is2022_7);
    virtual bool NotEqual(const rx_2022_channel & hw_channel, const rx2022Config & virtual_config, bool is2022_7);
    virtual bool NotEqual(const tx_2022_channel & hw_channel, const tx2022Config & virtual_config, bool is2022_7);
    virtual void SetIPError(NTV2Channel channel, uint32_t configType, uint32_t val);
    virtual void GetIPError(NTV2Channel channel, uint32_t configType, uint32_t & val);
    virtual void PrintRxConfig(const rx_2022_channel chan);
    virtual void PrintTxConfig(const tx_2022_channel chan);
    virtual void PrintEncoderConfig(const j2kEncoderConfig modelConfig, j2kEncoderConfig encoderConfig);
    virtual void PrintDecoderConfig(const j2kDecoderConfig modelConfig, j2kDecoderConfig encoderConfig);
    virtual void Print2110Network(const NetworkData2110 m2110Network);
    virtual void PrintChArray(std::string title, const char* chstr);

	// overridden in some classes
	virtual NTV2LSVideoADCMode GetVideoADCMode();
	virtual bool SetVideoADCMode(NTV2LSVideoADCMode value);
	virtual NTV2VideoFormat GetSdiInVideoFormatWithVpid(int32_t index);
	virtual NTV2VideoFormat GetSdiInVideoFormat(int32_t index, NTV2VideoFormat videoFormat);
	
	// support
	bool SetVPIDData(	ULWord &		outVPID,
						NTV2VideoFormat	inOutputFormat,
						bool			inIsDualLinkRGB,
						bool			inIsRGB48Bit,
						bool			inOutputIs3Gb,
						bool			inIsSMPTE425,
						VPIDChannel		inChannel,
						bool			inUseChannel=true);
	GeneralFrameFormat GetGeneralFrameFormat(NTV2FrameBufferFormat fbFormat);
	NTV2Mode GetCh1Mode();
	
	bool IsPulldownConverterMode(NTV2VideoFormat fmt1, NTV2VideoFormat fmt2);
	bool IsCompatibleWithReference(NTV2VideoFormat videoFormat);
	bool IsCompatibleWithReference(NTV2VideoFormat videoFormat, NTV2VideoFormat inputFormat);
	bool IsCompatibleWithReference(NTV2FrameRate fbRate, NTV2FrameRate inputRate);
	bool IsFormatRaw(NTV2FrameBufferFormat fbFormat);
	bool IsFormatCompressed(NTV2FrameBufferFormat fbFormat);
	void SetMacDebugOption(int item);
	bool IsDeinterlacedMode(NTV2VideoFormat fmt1, NTV2VideoFormat fmt2);
	NTV2RGB10Range GetCSCRange();
	bool UpdateK2ColorSpaceMatrixSelect();
	bool UpdateK2LUTSelect();
	NTV2FrameRate GetInput2VideoFrameRate();
	void SetForegroundVideoCrosspoint(NTV2Crosspoint crosspoint);
	void SetForegroundKeyCrosspoint(NTV2Crosspoint crosspoint);
	void SetBackgroundVideoCrosspoint(NTV2Crosspoint crosspoint);
	void EnableRP188EtoE(NTV2WidgetID fromInputWgt, NTV2WidgetID toOutputWgt);
	void DisableRP188EtoE(NTV2WidgetID toOutputWgt);
	void WriteAudioSourceSelect(ULWord val, NTV2Channel ch=NTV2_CHANNEL1);

	bool GetExtFrameGeometry(NTV2FrameGeometry geometry, NTV2FrameGeometry* value);
	NTV2LHIVideoDACMode GetLHIVideoDACMode(NTV2VideoFormat format, NTV2AnalogType type, NTV2AnalogBlackLevel blackLevel);
	bool UpdateK2ColorSpaceMatrixSelect (NTV2VideoFormat currFormat, bool ajamac=true);
	bool UpdateK2LUTSelect (NTV2VideoFormat currFormat, bool ajamac=true);
	void SetADCRegisters(	NTV2Standard analogInStandard, NTV2AnalogType analogInType,
							NTV2AnalogBlackLevel analogInBlackLevel, NTV2FrameRate analogInFrameRate);
	bool GetADCRegisters(	NTV2Standard *analogInStandard, NTV2AnalogType *analogInType,
							NTV2AnalogBlackLevel *analogInBlackLevel, NTV2FrameRate *analogInFrameRate);
	NTV2Standard GetAnalogInputVideoStandard();
	NTV2FrameRate GetAnalogInputVideoFrameRate();
	NTV2VideoFormat GetNTV2VideoFormat(NTV2FrameRate frameRate, NTV2Standard standard, bool isThreeG, UByte inputGeometry, bool progressivePicture);
	NTV2VideoDACMode GetVideoDACModeForVideoFormat(NTV2VideoFormat format, NTV2AnalogType type, NTV2AnalogBlackLevel blackLevel);
	
	NTV2VideoFormat GetPairedInterlaceTransportFormat(NTV2VideoFormat format);
	bool CanConvertFormat(NTV2VideoFormat inFormat, NTV2VideoFormat outFormat);
	NTV2VideoFormat GetConversionCompatibleFormat(NTV2VideoFormat sourceFmt, NTV2VideoFormat secondaryFmt);
	NTV2FrameRate HalfFrameRate(NTV2FrameRate rate);
	bool InputRequiresBToAConvertsion(NTV2Channel ch);
	
	uint32_t GetAudioDelayOffset(double frames);
	NTV2AudioSystem GetHostAudioSystem();

	void SetAudioInputSelect(NTV2InputAudioSelect input);
    void AgentIsAlive();

public:
	CNTV2Card*				mCard;
	
	// set by every frame, not user
	NTV2VideoFormat			mDefaultVideoFormat;
	uint32_t				mADCStabilizeCount;	
	HDMIOutColorSpaceMode	mHDMIOutColorSpaceModeStatus;	
	uint32_t				mADCLockScanTestFormat;
	
	// virtual register
	DefaultVideoOutMode		mDefaultVideoOutMode;
	uint32_t				mFollowInputFormat;
	uint32_t				mVANCMode;
	uint32_t				mVirtualDebug1;
	uint32_t				mEveryFrameTaskFilter;
	uint32_t				mDefaultInput;
	NTV2SDITransportType	mDualStreamTransportType;
	NTV24kTransportType		m4kTransportOutSelection;
	NTV2DSKMode				mDSKMode;
	int32_t					mStreamingAppPID;
	uint32_t				mStreamingAppType;
	NTV2OutputVideoSelect	mVirtualDigitalOutput1Select;
	NTV2OutputVideoSelect	mVirtualDigitalOutput2Select;
	NTV2OutputVideoSelect	mVirtualHDMIOutputSelect;
	NTV2OutputVideoSelect	mVirtualAnalogOutputSelect;
	NTV2LutType				mLUTType;
	NTV2LutType				mLUT2Type;
	NTV2InputVideoSelect	mVirtualInputSelect;
	NTV2VideoFormat			mVirtualSecondaryFormatSelect;
	bool					mIsoConvertEnable;
	uint32_t				mDSKAudioMode;
	uint32_t				mDSKForegroundMode;
	uint32_t				mDSKForegroundFade;
	ReferenceSelect			mCaptureReferenceSelect;
	ReferenceSelect			mDisplayReferenceSelect;
	NTV2GammaType			mGammaMode;
	NTV2RGB10Range			mRGB10Range;
	NTV2ColorSpaceType		mColorSpaceType;
	NTV2ColorSpaceMode		mSDIOutput1ColorSpace;
	NTV2RGBRangeMode		mSDIOutput1RGBRange;
	
    rx2022Config            mRx2022Config1;
    rx2022Config            mRx2022Config2;
    tx2022Config            mTx2022Config3;
    tx2022Config            mTx2022Config4;
	j2kDecoderConfig		mRx2022J2kConfig1;
	j2kDecoderConfig		mRx2022J2kConfig2;
	j2kEncoderConfig		mTx2022J2kConfig1;
	j2kEncoderConfig		mTx2022J2kConfig2;

    IPVNetConfig            mEth0;
    IPVNetConfig            mEth1;
    bool                    m2022_7Mode;
    uint32_t				mNetworkPathDiff;

    NetworkData2110			m2110Network;
    TransmitVideoData2110   m2110TxVideoData;
    TransmitAudioData2110   m2110TxAudioData;
    ReceiveVideoData2110    m2110RxVideoData;
    ReceiveAudioData2110    m2110RxAudioData;
    IpStatus2110            m2110IpStatusData;

	// real register state - common
	NTV2DeviceID			mDeviceID;
    NTV2VideoFormat			mFb1VideoFormat;
	NTV2FrameBufferFormat	mFb1Format;
	NTV2FrameBufferFormat	mFb2Format;
	NTV2Mode				mFb1Mode;
	bool					mVpid1Valid;
	ULWord					mVpid1a;
	ULWord					mVpid1b;
	bool					mVpid2Valid;
	ULWord					mVpid2a;
	ULWord					mVpid2b;

	// calculated valule, selected by user
	NTV2VideoFormat			mSelectedInputVideoFormat;
	//NTV2SDIInputFormatSelect mSDIInputFormatSelect;
	NTV2ColorSpaceMode 		mSDIInput1ColorSpace;
	NTV2ColorSpaceMode 		mSDIInput2ColorSpace;
	NTV2RGBRangeMode		mSDIInput1RGBRange;
	NTV2RGBRangeMode		mSDIInput2RGBRange;
	NTV2RGBRangeMode		mFrameBuffer1RGBRange;
	NTV2AnalogBlackLevel	mVirtualAnalogOutBlackLevel;
	NTV2AnalogType			mVirtualAnalogOutputType;
	NTV2AnalogBlackLevel	mVirtualAnalogInBlackLevel;
	NTV2AnalogType			mVirtualAnalogInType;
	NTV2Standard			mVirtualAnalogInStandard;
	
	HDMIOutColorSpaceMode	mHDMIOutColorSpaceModeCtrl;	
	HDMIOutProtocolMode		mHDMIOutProtocolMode;
	HDMIOutStereoSelect		mHDMIOutStereoSelect;		// selection driven by user choice in CP
	HDMIOutStereoSelect		mHDMIOutStereoCodecSelect;	// selection driver by codec settings
	NTV2HDMIAudioChannels	mHDMIOutAudioChannels;
	NTV2HDMIRange			mHDMIInRGBRange;
	
	uint32_t				mRegFramesPerVertical;		// frames per vertical interrupt (e.g. CION RAW)
	uint32_t				mRegResetCycleCount;		// reset cycle count (power-cycle, or sleep)
	uint32_t				mRegResetCycleCountLast;	// prev reset cycle count used to detect changes
	bool					mStartupDisabled;			// sometime we don't want to do hw reset
	int32_t					mInputFormatSelect;			// set and read by device services only
	bool					mInputFormatLock;
	NTV2VideoFormat			mLastInputFormatSelect;
	
	// audio mixer
	bool					mAudioMixerOverrideState;
	bool					mAudioMixerSourceMainEnable;
	bool					mAudioMixerSourceAux1Enable;
	bool					mAudioMixerSourceAux2Enable;
	int32_t					mAudioMixerSourceMainGain;
	int32_t					mAudioMixerSourceAux1Gain;
	int32_t					mAudioMixerSourceAux2Gain;
	
	bool					mAudioCapMixerSourceMainEnable;
	bool					mAudioCapMixerSourceAux1Enable;
	bool					mAudioCapMixerSourceAux2Enable;
	int32_t					mAudioCapMixerSourceMainGain;
	int32_t					mAudioCapMixerSourceAux1Gain;
	int32_t					mAudioCapMixerSourceAux2Gain;

	void SetCard (CNTV2Card* card)
		{ mCard = card; }
};

#endif


