//
//  ntv2deviceservices.h
//
//  Copyright (c) 2011 AJA Video, Inc. All rights reserved.
//

#ifndef _DeviceServices_
#define _DeviceServices_

#include "ntv2utils.h"
#include "ntv2vpid.h"
#include "ntv2vidproc.h"
#include "ntv2vidprocmasks.h"
#include "ntv2config2022.h"

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

#define IS_CION_RAW(genFmt)	(genFmt >= FORMAT_RAW && genFmt <= FORMAT_RAW_UHFR)

#define AUDIO_DELAY_WRAPAROUND    8160    // for 4Mb buffer

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
	virtual void SetDeviceXPointPlayback(GeneralFrameFormat format);
	virtual void SetDeviceXPointCapture(GeneralFrameFormat format);
	virtual void SetDeviceMiscRegisters(NTV2Mode mode);
	
	virtual NTV2VideoFormat GetLockedInputVideoFormat();
	virtual NTV2VideoFormat GetSelectedInputVideoFormat(NTV2VideoFormat referenceFormat, NTV2SDIInputFormatSelect* inputFormatSelect=NULL);
	virtual void SetDeviceXPointPlaybackRaw(GeneralFrameFormat format);
	virtual void SetDeviceXPointCaptureRaw(GeneralFrameFormat format);
	virtual void SetDeviceMiscRegistersRaw(NTV2Mode mode) {(void)mode;}
	virtual void DisableStartupSequence() {mStartupDisabled = true;}

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
	NTV2FrameGeometry GetNTV2FrameGeometryFromVideoFormat(NTV2VideoFormat videoFormat);
	NTV2Mode GetCh1Mode();
	
	bool IsPulldownConverterMode(NTV2VideoFormat fmt1, NTV2VideoFormat fmt2);
	bool IsCompatibleWithReference(NTV2VideoFormat videoFormat);
	bool IsFrameBufferFormatRGB(NTV2FrameBufferFormat fbFormat);
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
    void SetVideoOutputStandard(NTV2Channel channel);
	
	uint32_t GetAudioDelayOffset(double frames);

	void SetAudioInputSelect(NTV2InputAudioSelect input);

public:
	CNTV2VidProc*			mCard;
	
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
	bool					m2XTransferMode;
	NTV2GammaType			mGammaMode;
	NTV2RGB10Range			mRGB10Range;
	NTV2ColorSpaceType		mColorSpaceMode;
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

	// real register state - common
	NTV2VideoFormat			mFb1VideoFormat;
	NTV2FrameBufferFormat	mFb1FrameBufferFomat;
	NTV2Mode				mFb1Mode;

	// calculated valule, selected by user
	NTV2VideoFormat			mSelectedInputVideoFormat;
	NTV2SDIInputFormatSelect mSDIInputFormatSelect;
	
	NTV2SDIInputFormatSelect mSDIInput1FormatSelect;
	NTV2SDIInputFormatSelect mSDIInput2FormatSelect;
	NTV2RGBRangeMode		mSDIInput1RGBRange;
	NTV2RGBRangeMode		mSDIInput2RGBRange;
	NTV2Stereo3DMode		mSDIInput1Stereo3DMode;
	NTV2RGBRangeMode		mFrameBuffer1RGBRange;
	NTV2Stereo3DMode		mFrameBuffer1Stereo3DMode;
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
	
	uint32_t				mRegFramesPerVertical;		// frames per vertical interrupt (e.g. CION RAW)
	uint32_t				mRegResetCycleCount;		// reset cycle count (power-cycle, or sleep)
	uint32_t				mRegResetCycleCountLast;	// prev reset cycle count used to detect changes
	bool					mStartupDisabled;			// sometime we don't want to do hw reset
	int32_t					mInputFormatSelect;			// set and read by device services only
	bool					mInputFormatLock;
	NTV2VideoFormat			mLastInputFormatSelect;

	void SetCard (CNTV2VidProc* card)
		{ mCard = card; }
	
};

#endif


