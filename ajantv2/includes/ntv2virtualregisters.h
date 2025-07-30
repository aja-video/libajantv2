/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2virtualregisters.h
	@brief		Declares enums for virtual registers used in all platform drivers and the SDK.
	@copyright	(C) 2011-2022 AJA Video Systems, Inc.All rights reserved.
**/

#ifndef NTV2VIRTUALREGISTERS_H
#define NTV2VIRTUALREGISTERS_H

#include "ajatypes.h"
#define VIRTUALREG_START			10000	//	Virtual registers start at register number 10000
#define MAX_NUM_VIRTUAL_REGISTERS	1024	//	Starting in SDK 12.6, there's room for 1024 virtual registers

/**
	@brief	Virtual registers are used to pass 32-bit values to/from the device driver, and aren't always
			associated with hardware registers.

	@note	Drivers after SDK 12.5.x all allocate a 4K page for storing an array of 1024 x 4-byte integers.
			OEM applications can store values in slots kVRegFirstOEM thru kVRegLast, inclusive.
			AJA recommends storing at kVRegLast, kVRegLast-1, kVRegLast-2, etc., being careful
			to never store anything below kVRegFirstOEM.

	@note	AJA does not reserve virtual registers for OEMs, and thus, collisions may occur with other OEM applications.
**/
typedef enum
{
	// Common to all platforms
	kVRegDriverVersion						= VIRTUALREG_START,			///< @brief			Packed driver version -- use NTV2DriverVersionEncode, NTV2DriverVersionDecode* macros to encode/decode

	// Windows platform custom section
	kVRegAudioRecordPinDelay				= VIRTUALREG_START+1,		// Audio Delay relative to video, for Windows Media capture
	kVRegRelativeVideoPlaybackDelay			= VIRTUALREG_START+2,		// Video Delay relative to audio, for Windows Media playback
	kVRegGlobalAudioPlaybackMode			= VIRTUALREG_START+3,		// Shared with Linux, but not Mac
	kVRegFlashProgramKey					= VIRTUALREG_START+4,
	kVRegStrictTiming						= VIRTUALREG_START+5,		// Drift Correction requires Strict Frame Timing for Windows Media playback;Required for BackHaul;Correlate Presentation Time Stamp with Graph Clock;Turn off (default) to allow Playback even when Graph Manager gives us a Bogus Clcok!
	kVRegCCPlayerBufSizeKB					= VIRTUALREG_START+6,		// NTV2CCPlayer: custom Anc buffer size (KiloBytes)
	kVRegCCPlayerCustomPkt					= VIRTUALREG_START+7,		// NTV2CCPlayer: if nonzero, insert custom Anc pkt: DID|SID|lineNum
	kVRegDriverType							= VIRTUALREG_START+10,		// Driver type (MacOS only:  0 == KEXT, 'DEXT' == DEXT)

	// COMMON_VIRTUAL_REGS_KONA2
	kVRegInputSelect						= VIRTUALREG_START+20,		// Input 1, Input 2, DualLink
	kVRegSecondaryFormatSelect				= VIRTUALREG_START+21,		// NTV2VideoFormats 
	kVRegDigitalOutput1Select				= VIRTUALREG_START+22,		// Primary, Secondary	
	kVRegDigitalOutput2Select				= VIRTUALREG_START+23,		// Primary, Secondary, DualLink
	kVRegAnalogOutputSelect					= VIRTUALREG_START+24,		// Primary, Secondary
	kVRegAnalogOutputType					= VIRTUALREG_START+25,		// Analog output type
	kVRegAnalogOutBlackLevel				= VIRTUALREG_START+26,		// Analog output black level
	kVRegInputSelectUser					= VIRTUALREG_START+27,		// Input 1, Input 2, DualLink, set by user

	// COMMON_VIRTUAL_REGS_MISC
	kVRegVideoOutPauseMode					= VIRTUALREG_START+40,		// whether we pause on a frame or a field
	kVRegPulldownPattern					= VIRTUALREG_START+41,		// which 3:2 pulldown pattern to use
	kVRegColorSpaceMode						= VIRTUALREG_START+42,		// which color space matrix (Rec601, Rec709, ...) to use
	kVRegGammaMode							= VIRTUALREG_START+43,		// which gamma LUT (Rec601, Rec709, ...) to use
	kVRegLUTType							= VIRTUALREG_START+44,		// the current LUT function loaded into hardware
	kVRegRGB10Range							= VIRTUALREG_START+45,		// deprecated
	kVRegRGB10Endian						= VIRTUALREG_START+46,		// the user selected 10-bit RGB endian
	kVRegFanControl							= VIRTUALREG_START+47,		// the user fan control setting

	// Windows platform custom section
	kVRegBitFileDownload					= VIRTUALREG_START+50,		// NTV2BitfileType
	kVRegSaveRegistersToRegistry			= VIRTUALREG_START+51,		// no argument				
	kVRegRecallRegistersFromRegistry		= VIRTUALREG_START+52,		// same address as above, on purpose
	kVRegClearAllSubscriptions				= VIRTUALREG_START+53,		// NTV2BitfileType
	kVRegRestoreHardwareProcampRegisters	= VIRTUALREG_START+54,
	kVRegAcquireReferenceCount				= VIRTUALREG_START+55,		// Acquire the board with a reference count on acquire
	kVRegReleaseReferenceCount				= VIRTUALREG_START+56,

	kVRegDTAudioMux0						= VIRTUALREG_START+60,		// Firmware desired
	kVRegDTAudioMux1						= VIRTUALREG_START+61,		// Firmware desired
	kVRegDTAudioMux2						= VIRTUALREG_START+62,		// Firmware desired
	kVRegDTFirmware							= VIRTUALREG_START+63,		// Firmware desired
	kVRegDTVersionAja						= VIRTUALREG_START+64,		// Driver version (Aja)
	kVRegDTVersionDurian					= VIRTUALREG_START+65,		// Driver version (Durian)
	kVRegDTAudioCapturePinConnected			= VIRTUALREG_START+66,		// Audio Capture Pin Connected ?

	kVRegTimeStampMode						= VIRTUALREG_START+70,		// 0-Scaled timestamps(100ns), 1- Raw PerformanceCounter
	kVRegTimeStampLastOutputVerticalLo		= VIRTUALREG_START+71,		// Lower 32 bits.
	kVRegTimeStampLastOutputVerticalHi		= VIRTUALREG_START+72,		// Hi 32 bits.
	kVRegTimeStampLastInput1VerticalLo		= VIRTUALREG_START+73,
	kVRegTimeStampLastInput1VerticalHi		= VIRTUALREG_START+74,
	kVRegTimeStampLastInput2VerticalLo		= VIRTUALREG_START+75,
	kVRegTimeStampLastInput2VerticalHi		= VIRTUALREG_START+76,
	kVRegNumberVideoMappingRegisters		= VIRTUALREG_START+77,
	kVRegNumberAudioMappingRegisters		= VIRTUALREG_START+78,
	kVRegAudioSyncTolerance					= VIRTUALREG_START+79,
	kVRegDmaSerialize						= VIRTUALREG_START+80,
	kVRegSyncChannel						= VIRTUALREG_START+81,		// Mac name
	kVRegSyncChannels						= VIRTUALREG_START+81,		// Windows and Linux name for the same thing
	kVRegSoftwareUartFifo					= VIRTUALREG_START+82,
	kVRegTimeCodeCh1Delay					= VIRTUALREG_START+83,
	kVRegTimeCodeCh2Delay					= VIRTUALREG_START+84,
	kVRegTimeCodeIn1Delay					= VIRTUALREG_START+85,
	kVRegTimeCodeIn2Delay					= VIRTUALREG_START+86,
	kVRegTimeCodeCh3Delay					= VIRTUALREG_START+87,
	kVRegTimeCodeCh4Delay					= VIRTUALREG_START+88,
	kVRegTimeCodeIn3Delay					= VIRTUALREG_START+89,
	kVRegTimeCodeIn4Delay					= VIRTUALREG_START+90,
	kVRegTimeCodeCh5Delay					= VIRTUALREG_START+91,
	kVRegTimeCodeIn5Delay					= VIRTUALREG_START+92,
	kVRegTimeCodeCh6Delay					= VIRTUALREG_START+93,
	kVRegTimeCodeIn6Delay					= VIRTUALREG_START+94,
	kVRegTimeCodeCh7Delay					= VIRTUALREG_START+95,
	kVRegTimeCodeIn7Delay					= VIRTUALREG_START+96,
	kVRegTimeCodeCh8Delay					= VIRTUALREG_START+97,
	kVRegTimeCodeIn8Delay					= VIRTUALREG_START+98,

	kVRegDebug1								= VIRTUALREG_START+100,		// general debug register
	kVRegDebugLastFormat					= VIRTUALREG_START+101,		// IP debug registers
	kVRegDebugIPConfigTimeMS				= VIRTUALREG_START+102,

	// Control Panel virtual registers
	kVRegDisplayReferenceSelect				= VIRTUALREG_START+120,
	kVRegVANCMode							= VIRTUALREG_START+121,
	kVRegDualStreamTransportType			= VIRTUALREG_START+122,
	kVRegSDIOut1TransportType				= kVRegDualStreamTransportType,
	kVRegDSKMode							= VIRTUALREG_START+123,
	kVRegIsoConvertEnable					= VIRTUALREG_START+124,
	kVRegDSKAudioMode						= VIRTUALREG_START+125,
	kVRegDSKForegroundMode					= VIRTUALREG_START+126,
	kVRegDSKForegroundFade					= VIRTUALREG_START+127,
	kVRegCaptureReferenceSelect				= VIRTUALREG_START+128,		// deprecated

	kVRegUnfilterAnc						= VIRTUALREG_START+130,
	kVRegSDIOutput1RGBRange					= VIRTUALREG_START+131,
	kVRegSDIInput1FormatSelect				= VIRTUALREG_START+132,
	kVRegSDIInput2FormatSelect				= VIRTUALREG_START+133,
	kVRegSDIInput1RGBRange					= VIRTUALREG_START+134,
	kVRegSDIInput2RGBRange					= VIRTUALREG_START+135,
	kVRegSDIInput1Stereo3DMode				= VIRTUALREG_START+136,		// deprecated
	kVRegSDIInput2Stereo3DMode				= VIRTUALREG_START+137,		// deprecated
	kVRegFrameBuffer1RGBRange				= VIRTUALREG_START+138,
	kVRegFrameBuffer1Stereo3DMode			= VIRTUALREG_START+139,		// deprecated

	kVRegHDMIInRgbRange						= VIRTUALREG_START+140,
	kVRegHDMIOutRgbRange					= VIRTUALREG_START+141,		//	See also kVRegHDMIOutRGBRange
	kVRegAnalogInBlackLevel					= VIRTUALREG_START+142,
	kVRegAnalogInputType					= VIRTUALREG_START+143,
	kVRegHDMIOutColorSpaceModeCtrl			= VIRTUALREG_START+144,
	kVRegHDMIOutProtocolMode				= VIRTUALREG_START+145,
	kVRegHDMIOutStereoSelect				= VIRTUALREG_START+146,		// deprecated
	kVRegHDMIOutStereoCodecSelect			= VIRTUALREG_START+147,		// deprecated
	kVRegHdmiOutSubSample					= VIRTUALREG_START+148,
	kVRegSDIInput1ColorSpaceMode			= VIRTUALREG_START+149,

	kVRegSDIInput2ColorSpaceMode			= VIRTUALREG_START+150,
	kVRegSDIOutput2RGBRange					= VIRTUALREG_START+151,
	kVRegSDIOutput1Stereo3DMode				= VIRTUALREG_START+152,		// deprecated
	kVRegSDIOutput2Stereo3DMode				= VIRTUALREG_START+153,		// deprecated
	kVRegFrameBuffer2RGBRange				= VIRTUALREG_START+154,
	kVRegFrameBuffer2Stereo3DMode			= VIRTUALREG_START+155,		// deprecated
	kVRegAudioGainDisable					= VIRTUALREG_START+156,
	kVRegLTCOnRefInSelect					= VIRTUALREG_START+157,
	kVRegActiveVideoOutFilter				= VIRTUALREG_START+158,		// deprecated
	kVRegAudioInputMapSelect				= VIRTUALREG_START+159,

	kVRegAudioInputDelay					= VIRTUALREG_START+160,
	kVRegDSKGraphicFileIndex				= VIRTUALREG_START+161,
	kVRegTimecodeBurnInMode					= VIRTUALREG_START+162,		// deprecated
	kVRegUseQTTimecode						= VIRTUALREG_START+163,		// deprecated
	kVRegAvailable164						= VIRTUALREG_START+164,
	kVRegRP188SourceSelect					= VIRTUALREG_START+165,
	kVRegQTCodecModeDebug					= VIRTUALREG_START+166,		// deprecated
	kVRegHDMIOutColorSpaceModeStatus		= VIRTUALREG_START+167,		// deprecated
	kVRegDeviceOnline						= VIRTUALREG_START+168,
	kVRegIsDefaultDevice					= VIRTUALREG_START+169,

	kVRegDesktopFrameBufferStatus			= VIRTUALREG_START+170,		// deprecated
	kVRegSDIOutput1ColorSpaceMode			= VIRTUALREG_START+171,
	kVRegSDIOutput2ColorSpaceMode			= VIRTUALREG_START+172,
	kVRegAudioOutputDelay					= VIRTUALREG_START+173,
	kVRegTimelapseEnable					= VIRTUALREG_START+174,		// deprecated
	kVRegTimelapseCaptureValue				= VIRTUALREG_START+175,		// deprecated
	kVRegTimelapseCaptureUnits				= VIRTUALREG_START+176,		// deprecated
	kVRegTimelapseIntervalValue				= VIRTUALREG_START+177,		// deprecated
	kVRegTimelapseIntervalUnits				= VIRTUALREG_START+178,		// deprecated
	kVRegSDIOutConfig						= VIRTUALREG_START+179,

	kVRegAnalogInStandard					= VIRTUALREG_START+180,		// deprecated
	kVRegOutputTimecodeOffset				= VIRTUALREG_START+181,		// deprecated
	kVRegOutputTimecodeType					= VIRTUALREG_START+182,		// deprecated
	kVRegQuicktimeUsingBoard				= VIRTUALREG_START+183,		// deprecated
	kVRegApplicationPID						= VIRTUALREG_START+184,		// The rest of this section handled by IOCTL in Mac
	kVRegApplicationCode					= VIRTUALREG_START+185,
	kVRegReleaseApplication					= VIRTUALREG_START+186,
	kVRegForceApplicationPID				= VIRTUALREG_START+187,
	kVRegForceApplicationCode				= VIRTUALREG_START+188,
	kVRegIpConfigStreamRefresh				= VIRTUALREG_START+189,
	kVRegSDIInConfig						= VIRTUALREG_START+190,
	kVRegInputChangedCount					= VIRTUALREG_START+191,
	kVReg8kOutputTransportSelection			= VIRTUALREG_START+192,
	kVRegAnalogIoSelect						= VIRTUALREG_START+193,


	// COMMON_VIRTUAL_REGS_PROCAMP_CONTROLS
	kVRegProcAmpSDRegsInitialized			= VIRTUALREG_START+200,
	kVRegProcAmpStandardDefBrightness		= VIRTUALREG_START+201,
	kVRegProcAmpStandardDefContrast			= VIRTUALREG_START+202,
	kVRegProcAmpStandardDefSaturation		= VIRTUALREG_START+203,
	kVRegProcAmpStandardDefHue				= VIRTUALREG_START+204,
	kVRegProcAmpStandardDefCbOffset			= VIRTUALREG_START+205,
	kVRegProcAmpStandardDefCrOffset			= VIRTUALREG_START+206,
	kVRegProcAmpEndStandardDefRange			= VIRTUALREG_START+207,

	kVRegProcAmpHDRegsInitialized			= VIRTUALREG_START+220,
	kVRegProcAmpHighDefBrightness			= VIRTUALREG_START+221,
	kVRegProcAmpHighDefContrast				= VIRTUALREG_START+222,
	kVRegProcAmpHighDefSaturationCb			= VIRTUALREG_START+223,
	kVRegProcAmpHighDefSaturationCr			= VIRTUALREG_START+224,
	kVRegProcAmpHighDefHue					= VIRTUALREG_START+225,
	kVRegProcAmpHighDefCbOffset				= VIRTUALREG_START+226,
	kVRegProcAmpHighDefCrOffset				= VIRTUALREG_START+227,
	kVRegProcAmpEndHighDefRange				= VIRTUALREG_START+228,

	// COMMON_VIRTUAL_REGS_USERSPACE_BUFFLEVEL
	kVRegChannel1UserBufferLevel			= VIRTUALREG_START+240,
	kVRegChannel2UserBufferLevel			= VIRTUALREG_START+241,
	kVRegInput1UserBufferLevel				= VIRTUALREG_START+242,
	kVRegInput2UserBufferLevel				= VIRTUALREG_START+243,

	// COMMON_VIRTUAL_REGS_EX
	kVRegProgressivePicture					= VIRTUALREG_START+260,
	kVRegLUT2Type							= VIRTUALREG_START+261,
	kVRegLUT3Type							= VIRTUALREG_START+262,
	kVRegLUT4Type							= VIRTUALREG_START+263,
	kVRegDigitalOutput3Select				= VIRTUALREG_START+264,
	kVRegDigitalOutput4Select				= VIRTUALREG_START+265,
	kVRegHDMIOutputSelect					= VIRTUALREG_START+266,
	kVRegRGBRangeConverterLUTType			= VIRTUALREG_START+267,
	kVRegTestPatternChoice					= VIRTUALREG_START+268,
	kVRegTestPatternFormat					= VIRTUALREG_START+269,
	kVRegEveryFrameTaskFilter				= VIRTUALREG_START+270,
	kVRegDefaultInput						= VIRTUALREG_START+271,
	kVRegDefaultVideoOutMode				= VIRTUALREG_START+272,
	kVRegDefaultVideoFormat					= VIRTUALREG_START+273,
	kVRegDigitalOutput5Select				= VIRTUALREG_START+274,
	kVRegLUT5Type							= VIRTUALREG_START+275,

	// Macintosh platform custom section
	kVRegMacUserModeDebugLevel				= VIRTUALREG_START+300,
	kVRegMacKernelModeDebugLevel			= VIRTUALREG_START+301,
	kVRegMacUserModePingLevel				= VIRTUALREG_START+302,
	kVRegMacKernelModePingLevel				= VIRTUALREG_START+303,
	kVRegLatencyTimerValue					= VIRTUALREG_START+304,

	kVRegAudioInputSelect					= VIRTUALREG_START+306,
	kVRegSerialSuspended					= VIRTUALREG_START+307,
	kVRegXilinxProgramming					= VIRTUALREG_START+308,
	kVRegETTDiagLastSerialTimestamp			= VIRTUALREG_START+309,
	kVRegETTDiagLastSerialTimecode			= VIRTUALREG_START+310,
	kVRegStartupStatusFlags					= VIRTUALREG_START+311,
	kVRegRGBRangeMode						= VIRTUALREG_START+312,
	kVRegEnableQueuedDMAs					= VIRTUALREG_START+313,		//	If non-zero, enables queued DMAs on multi-engine devices (Mac only)

	// Linux platform custom section
	kVRegBA0MemorySize						= VIRTUALREG_START+320,		// Memory-mapped register (BAR0) window size in bytes.
	kVRegBA1MemorySize						= VIRTUALREG_START+321,		// Memory-mapped framebuffer window size in bytes.
	kVRegBA4MemorySize						= VIRTUALREG_START+322,
	kVRegNumDmaDriverBuffers				= VIRTUALREG_START+323,		// Number of bigphysarea frames available (Read only).
	kVRegDMADriverBufferPhysicalAddress		= VIRTUALREG_START+324,		// Physical address of bigphysarea buffer
	kVRegBA2MemorySize						= VIRTUALREG_START+325,
	kVRegAcquireLinuxReferenceCount			= VIRTUALREG_START+326,		// Acquire the board with a reference count on acquire
	kVRegReleaseLinuxReferenceCount			= VIRTUALREG_START+327,

	// IoHD virtual registers
	kVRegAdvancedIndexing					= VIRTUALREG_START+340,		//	OBSOLETE after 12.4
	kVRegTimeStampLastInput3VerticalLo		= VIRTUALREG_START+341,
	kVRegTimeStampLastInput3VerticalHi		= VIRTUALREG_START+342,
	kVRegTimeStampLastInput4VerticalLo		= VIRTUALREG_START+343,
	kVRegTimeStampLastInput4VerticalHi		= VIRTUALREG_START+344,
	kVRegTimeStampLastInput5VerticalLo		= VIRTUALREG_START+345,
	kVRegTimeStampLastInput5VerticalHi		= VIRTUALREG_START+346,
	kVRegTimeStampLastInput6VerticalLo		= VIRTUALREG_START+347,
	kVRegTimeStampLastInput6VerticalHi		= VIRTUALREG_START+348,
	kVRegTimeStampLastInput7VerticalLo		= VIRTUALREG_START+349,
	kVRegTimeStampLastInput7VerticalHi		= VIRTUALREG_START+350,
	kVRegTimeStampLastInput8VerticalLo		= VIRTUALREG_START+351,
	kVRegTimeStampLastInput8VerticalHi		= VIRTUALREG_START+352,

	kVRegTimeStampLastOutput2VerticalLo		= VIRTUALREG_START+353,
	kVRegTimeStampLastOutput2VerticalHi		= VIRTUALREG_START+354,
	
	kVRegTimeStampLastOutput3VerticalLo		= VIRTUALREG_START+355,
	kVRegTimeStampLastOutput3VerticalHi		= VIRTUALREG_START+356,
	kVRegTimeStampLastOutput4VerticalLo		= VIRTUALREG_START+357,
	kVRegTimeStampLastOutput4VerticalHi		= VIRTUALREG_START+358,
	
	kVRegTimeStampLastOutput5VerticalLo		= VIRTUALREG_START+359,
	
	kVRegTimeStampLastOutput5VerticalHi		= VIRTUALREG_START+360,
	kVRegTimeStampLastOutput6VerticalLo		= VIRTUALREG_START+361,
	kVRegTimeStampLastOutput6VerticalHi		= VIRTUALREG_START+362,
	kVRegTimeStampLastOutput7VerticalLo		= VIRTUALREG_START+363,
	kVRegTimeStampLastOutput7VerticalHi		= VIRTUALREG_START+364,
	kVRegTimeStampLastOutput8VerticalLo		= VIRTUALREG_START+365,
	
	kVRegResetCycleCount					= VIRTUALREG_START+366,		// counts the number of device resets caused by plug-and-play or sleep, increments each time
	kVRegUseProgressive						= VIRTUALREG_START+367,		// when an option (e.g. Avid MC) choose P over PSF formats
	
	kVRegFlashSize							= VIRTUALREG_START+368,		// size of the flash partition for flash status
	kVRegFlashStatus						= VIRTUALREG_START+369,		// progress of flash patition
	kVRegFlashState							= VIRTUALREG_START+370,		// state status of flash

	kVRegPCIDeviceID						= VIRTUALREG_START+371,		// set by driver (read only)

	kVRegUartRxFifoSize						= VIRTUALREG_START+372,
	
	kVRegEFTNeedsUpdating					= VIRTUALREG_START+373,		// set when any retail virtual register has been changed
	
	kVRegSuspendSystemAudio					= VIRTUALREG_START+374,		// set when app wants to use AC audio and disable host audio (e.g., CoreAudio on MacOS)
	kVRegAcquireReferenceCounter			= VIRTUALREG_START+375,

	kVRegTimeStampLastOutput8VerticalHi		= VIRTUALREG_START+376,
	
	kVRegFramesPerVertical					= VIRTUALREG_START+377,
	kVRegServicesInitialized				= VIRTUALREG_START+378,		// set true when device is initialized by services

	kVRegFrameBufferGangCount				= VIRTUALREG_START+379,

	kVRegChannelCrosspointFirst				= VIRTUALREG_START+380,
																		//	kVRegChannelCrosspointFirst+1
																		//	kVRegChannelCrosspointFirst+2
																		//	kVRegChannelCrosspointFirst+3
																		//	kVRegChannelCrosspointFirst+4
																		//	kVRegChannelCrosspointFirst+5
																		//	kVRegChannelCrosspointFirst+6
	kVRegChannelCrosspointLast				= VIRTUALREG_START+387,		//	kVRegChannelCrosspointFirst+7

	//	Starting in SDK 13.0, kVRegDriverVersionMajor, kVRegDriverVersionMinor and kVRegDriverVersionPoint
	//	were all replaced by a single virtual register kVRegDriverVersion.
	kVRegMonAncField1Offset					= VIRTUALREG_START+389,		///< @brief Monitor Anc Field1 byte offset from end of frame buffer (IoIP only, GUMP)
	kVRegMonAncField2Offset					= VIRTUALREG_START+390,		///< @brief Monitor Anc Field2 byte offset from end of frame buffer (IoIP only, GUMP)
	kVRegFollowInputFormat					= VIRTUALREG_START+391,

	kVRegAncField1Offset					= VIRTUALREG_START+392,		///< @brief Anc Field1 byte offset from end of frame buffer (GUMP on all boards except RTP for SMPTE2022/IP)
	kVRegAncField2Offset					= VIRTUALREG_START+393,		///< @brief Anc Field2 byte offset from end of frame buffer (GUMP on all boards except RTP for SMPTE2022/IP)
	kVRegAgentCheck							= VIRTUALREG_START+394,
	kVRegUnused_2							= VIRTUALREG_START+395,
	
	kVReg4kOutputTransportSelection			= VIRTUALREG_START+396,
	kVRegCustomAncInputSelect				= VIRTUALREG_START+397,
	kVRegUseThermostat						= VIRTUALREG_START+398,
	kVRegThermalSamplingRate				= VIRTUALREG_START+399,
	kVRegFanSpeed							= VIRTUALREG_START+400,

	kVRegVideoFormatCh1						= VIRTUALREG_START+401,
	kVRegVideoFormatCh2						= VIRTUALREG_START+402,
	kVRegVideoFormatCh3						= VIRTUALREG_START+403,
	kVRegVideoFormatCh4						= VIRTUALREG_START+404,
	kVRegVideoFormatCh5						= VIRTUALREG_START+405,
	kVRegVideoFormatCh6						= VIRTUALREG_START+406,
	kVRegVideoFormatCh7						= VIRTUALREG_START+407,
	kVRegVideoFormatCh8						= VIRTUALREG_START+408,

	// Sarek VOIP section
	kVRegIPAddrEth0							= VIRTUALREG_START+409,
	kVRegSubnetEth0							= VIRTUALREG_START+410,
	kVRegGatewayEth0						= VIRTUALREG_START+411,

	kVRegIPAddrEth1							= VIRTUALREG_START+412,
	kVRegSubnetEth1							= VIRTUALREG_START+413,
	kVRegGatewayEth1						= VIRTUALREG_START+414,
	
	kVRegRxcEnable1							= VIRTUALREG_START+415,
	kVRegRxcSfp1RxMatch1					= VIRTUALREG_START+416,
	kVRegRxcSfp1SourceIp1					= VIRTUALREG_START+417,
	kVRegRxcSfp1DestIp1						= VIRTUALREG_START+418,
	kVRegRxcSfp1SourcePort1					= VIRTUALREG_START+419,
	kVRegRxcSfp1DestPort1					= VIRTUALREG_START+420,
	kVRegRxcSfp1Vlan1						= VIRTUALREG_START+421,
	kVRegRxcSfp2RxMatch1					= VIRTUALREG_START+422,
	kVRegRxcSfp2SourceIp1					= VIRTUALREG_START+423,
	kVRegRxcSfp2DestIp1						= VIRTUALREG_START+424,
	kVRegRxcSfp2SourcePort1					= VIRTUALREG_START+425,
	kVRegRxcSfp2DestPort1					= VIRTUALREG_START+426,
	kVRegRxcSfp2Vlan1						= VIRTUALREG_START+427,
	kVRegRxcSsrc1							= VIRTUALREG_START+428,
	kVRegRxcPlayoutDelay1					= VIRTUALREG_START+429,

	kVRegRxcEnable2							= VIRTUALREG_START+430,
	kVRegRxcSfp1RxMatch2					= VIRTUALREG_START+431,
	kVRegRxcSfp1SourceIp2					= VIRTUALREG_START+432,
	kVRegRxcSfp1DestIp2						= VIRTUALREG_START+433,
	kVRegRxcSfp1SourcePort2					= VIRTUALREG_START+434,
	kVRegRxcSfp1DestPort2					= VIRTUALREG_START+435,
	kVRegRxcSfp1Vlan2						= VIRTUALREG_START+436,
	kVRegRxcSfp2RxMatch2					= VIRTUALREG_START+437,
	kVRegRxcSfp2SourceIp2					= VIRTUALREG_START+438,
	kVRegRxcSfp2DestIp2						= VIRTUALREG_START+439,
	kVRegRxcSfp2SourcePort2					= VIRTUALREG_START+440,
	kVRegRxcSfp2DestPort2					= VIRTUALREG_START+441,
	kVRegRxcSfp2Vlan2						= VIRTUALREG_START+442,
	kVRegRxcSsrc2							= VIRTUALREG_START+443,
	kVRegRxcPlayoutDelay2					= VIRTUALREG_START+444,

	kVRegTxcEnable3							= VIRTUALREG_START+445,
	kVRegTxcSfp1LocalPort3					= VIRTUALREG_START+446,
	kVRegTxcSfp1RemoteIp3					= VIRTUALREG_START+447,
	kVRegTxcSfp1RemotePort3					= VIRTUALREG_START+448,
	kVRegTxcSfp2LocalPort3					= VIRTUALREG_START+449,
	kVRegTxcSfp2RemoteIp3					= VIRTUALREG_START+450,
	kVRegTxcSfp2RemotePort3					= VIRTUALREG_START+451,

	kVRegTxcEnable4							= VIRTUALREG_START+452,
	kVRegTxcSfp1LocalPort4					= VIRTUALREG_START+453,
	kVRegTxcSfp1RemoteIp4					= VIRTUALREG_START+454,
	kVRegTxcSfp1RemotePort4					= VIRTUALREG_START+455,
	kVRegTxcSfp2LocalPort4					= VIRTUALREG_START+456,
	kVRegTxcSfp2RemoteIp4					= VIRTUALREG_START+457,
	kVRegTxcSfp2RemotePort4					= VIRTUALREG_START+458,
	
	kVRegMailBoxAcquire						= VIRTUALREG_START+459,
	kVRegMailBoxRelease						= VIRTUALREG_START+460,
	kVRegMailBoxAbort						= VIRTUALREG_START+461,
	kVRegMailBoxTimeoutNS					= VIRTUALREG_START+462,		//	Units are 100 ns, not nanoseconds!

	kVRegRxc_2DecodeSelectionMode1			= VIRTUALREG_START+463,
	kVRegRxc_2DecodeProgramNumber1			= VIRTUALREG_START+464,
	kVRegRxc_2DecodeProgramPID1				= VIRTUALREG_START+465,
	kVRegRxc_2DecodeAudioNumber1			= VIRTUALREG_START+466,

	kVRegRxc_2DecodeSelectionMode2			= VIRTUALREG_START+467,
	kVRegRxc_2DecodeProgramNumber2			= VIRTUALREG_START+468,
	kVRegRxc_2DecodeProgramPID2				= VIRTUALREG_START+469,
	kVRegRxc_2DecodeAudioNumber2			= VIRTUALREG_START+470,

	kVRegTxc_2EncodeVideoFormat1			= VIRTUALREG_START+471,
	kVRegTxc_2EncodeUllMode1				= VIRTUALREG_START+472,
	kVRegTxc_2EncodeBitDepth1				= VIRTUALREG_START+473,
	kVRegTxc_2EncodeChromaSubSamp1			= VIRTUALREG_START+474,
	kVRegTxc_2EncodeMbps1					= VIRTUALREG_START+475,
	kVRegTxc_2EncodeAudioChannels1			= VIRTUALREG_START+476,
	kVRegTxc_2EncodeStreamType1				= VIRTUALREG_START+477,
	kVRegTxc_2EncodeProgramPid1				= VIRTUALREG_START+478,
	kVRegTxc_2EncodeVideoPid1				= VIRTUALREG_START+479,
	kVRegTxc_2EncodePcrPid1					= VIRTUALREG_START+480,
	kVRegTxc_2EncodeAudio1Pid1				= VIRTUALREG_START+481,
	
	kVRegTxc_2EncodeVideoFormat2			= VIRTUALREG_START+482,
	kVRegTxc_2EncodeUllMode2				= VIRTUALREG_START+483,
	kVRegTxc_2EncodeBitDepth2				= VIRTUALREG_START+484,
	kVRegTxc_2EncodeChromaSubSamp2			= VIRTUALREG_START+485,
	kVRegTxc_2EncodeMbps2					= VIRTUALREG_START+486,
	kVRegTxc_2EncodeAudioChannels2			= VIRTUALREG_START+487,
	kVRegTxc_2EncodeStreamType2				= VIRTUALREG_START+488,
	kVRegTxc_2EncodeProgramPid2				= VIRTUALREG_START+489,
	kVRegTxc_2EncodeVideoPid2				= VIRTUALREG_START+490,
	kVRegTxc_2EncodePcrPid2					= VIRTUALREG_START+491,
	kVRegTxc_2EncodeAudio1Pid2				= VIRTUALREG_START+492,

	kVReg2022_7Enable						= VIRTUALREG_START+493,
	kVReg2022_7NetworkPathDiff				= VIRTUALREG_START+494,
	
	kVRegKIPRxCfgError						= VIRTUALREG_START+495,
	kVRegKIPTxCfgError						= VIRTUALREG_START+496,
	kVRegKIPEncCfgError						= VIRTUALREG_START+497,
	kVRegKIPDecCfgError						= VIRTUALREG_START+498,
	kVRegKIPNetCfgError						= VIRTUALREG_START+499,
	kVRegUseHDMI420Mode						= VIRTUALREG_START+500,
	kVRegUnused501							= VIRTUALREG_START+501,
	
	kVRegUserDefinedDBB						= VIRTUALREG_START+502,
	
	kVRegHDMIOutAudioChannels				= VIRTUALREG_START+503,
	kVRegUnused504							= VIRTUALREG_START+504,
	kVRegZeroHostAncPostCapture				= VIRTUALREG_START+505,
	kVRegZeroDeviceAncPostCapture			= VIRTUALREG_START+506,
	kVRegAudioMonitorChannelSelect			= VIRTUALREG_START+507,
	kVRegAudioMixerOverrideState			= VIRTUALREG_START+508,
	kVRegAudioMixerSourceMainEnable			= VIRTUALREG_START+509,
	kVRegAudioMixerSourceAux1Enable			= VIRTUALREG_START+510,
	kVRegAudioMixerSourceAux2Enable			= VIRTUALREG_START+511,
	kVRegAudioMixerSourceMainGain			= VIRTUALREG_START+512,
	kVRegAudioMixerSourceAux1Gain			= VIRTUALREG_START+513,
	kVRegAudioMixerSourceAux2Gain			= VIRTUALREG_START+514,
	kVRegAudioCapMixerSourceMainEnable		= VIRTUALREG_START+515,
	kVRegAudioCapMixerSourceAux1Enable		= VIRTUALREG_START+516,
	kVRegAudioCapMixerSourceAux2Enable		= VIRTUALREG_START+517,
	kVRegAudioCapMixerSourceMainGain		= VIRTUALREG_START+518,
	kVRegAudioCapMixerSourceAux1Gain		= VIRTUALREG_START+519,
	kVRegAudioCapMixerSourceAux2Gain		= VIRTUALREG_START+520,
	
	kVRegSwizzle4kInput						= VIRTUALREG_START+521,
	kVRegSwizzle4kOutput					= VIRTUALREG_START+522,

	kVRegAnalogAudioIOConfiguration			= VIRTUALREG_START+523,
	kVRegHdmiHdrOutChanged					= VIRTUALREG_START+524,

	kVRegDisableAutoVPID					= VIRTUALREG_START+525,
	kVRegEnableBT2020						= VIRTUALREG_START+526,
	kVRegHdmiHdrOutMode						= VIRTUALREG_START+527,

	kVRegServicesForceInit					= VIRTUALREG_START+528,		// set true when power state changes
	kVRegServicesModeFinal					= VIRTUALREG_START+529,
	
	kVRegNTV2VPIDTransferCharacteristics1	= VIRTUALREG_START+530,
	kVRegNTV2VPIDColorimetry1				= VIRTUALREG_START+531,
	kVRegNTV2VPIDLuminance1					= VIRTUALREG_START+532,
	kVRegNTV2VPIDTransferCharacteristics	= kVRegNTV2VPIDTransferCharacteristics1,
	kVRegNTV2VPIDColorimetry				= kVRegNTV2VPIDColorimetry1,
	kVRegNTV2VPIDLuminance					= kVRegNTV2VPIDLuminance1,
	
	kVRegNTV2VPIDTransferCharacteristics2	= VIRTUALREG_START+533,
	kVRegNTV2VPIDColorimetry2				= VIRTUALREG_START+534,
	kVRegNTV2VPIDLuminance2					= VIRTUALREG_START+535,
	
	kVRegNTV2VPIDTransferCharacteristics3	= VIRTUALREG_START+536,
	kVRegNTV2VPIDColorimetry3				= VIRTUALREG_START+537,
	kVRegNTV2VPIDLuminance3					= VIRTUALREG_START+538,
	
	kVRegNTV2VPIDTransferCharacteristics4	= VIRTUALREG_START+539,
	kVRegNTV2VPIDColorimetry4				= VIRTUALREG_START+540,
	kVRegNTV2VPIDLuminance4					= VIRTUALREG_START+541,
	
	kVRegNTV2VPIDTransferCharacteristics5	= VIRTUALREG_START+542,
	kVRegNTV2VPIDColorimetry5				= VIRTUALREG_START+543,
	kVRegNTV2VPIDLuminance5					= VIRTUALREG_START+544,
	
	kVRegNTV2VPIDTransferCharacteristics6	= VIRTUALREG_START+545,
	kVRegNTV2VPIDColorimetry6				= VIRTUALREG_START+546,
	kVRegNTV2VPIDLuminance6					= VIRTUALREG_START+547,
	
	kVRegNTV2VPIDTransferCharacteristics7	= VIRTUALREG_START+548,
	kVRegNTV2VPIDColorimetry7				= VIRTUALREG_START+549,
	kVRegNTV2VPIDLuminance7					= VIRTUALREG_START+550,
	
	kVRegNTV2VPIDTransferCharacteristics8	= VIRTUALREG_START+551,
	kVRegNTV2VPIDColorimetry8				= VIRTUALREG_START+552,
	kVRegNTV2VPIDLuminance8					= VIRTUALREG_START+553,
	
	kVRegUserColorimetry					= VIRTUALREG_START+554,
	kVRegUserTransfer						= VIRTUALREG_START+555,
	kVRegUserLuminance						= VIRTUALREG_START+556,
	
	kVRegHdrColorimetryCh1					= VIRTUALREG_START+557,
	kVRegHdrTransferCh1						= VIRTUALREG_START+558,
	kVRegHdrLuminanceCh1					= VIRTUALREG_START+559,
	kVRegHdrGreenXCh1						= VIRTUALREG_START+560,
	kVRegHdrGreenYCh1						= VIRTUALREG_START+561,
	kVRegHdrBlueXCh1						= VIRTUALREG_START+562,
	kVRegHdrBlueYCh1						= VIRTUALREG_START+563,
	kVRegHdrRedXCh1							= VIRTUALREG_START+564,
	kVRegHdrRedYCh1							= VIRTUALREG_START+565,
	kVRegHdrWhiteXCh1						= VIRTUALREG_START+566,
	kVRegHdrWhiteYCh1						= VIRTUALREG_START+567,
	kVRegHdrMasterLumMaxCh1					= VIRTUALREG_START+568,
	kVRegHdrMasterLumMinCh1					= VIRTUALREG_START+569,
	kVRegHdrMaxCLLCh1						= VIRTUALREG_START+570,
	kVRegHdrMaxFALLCh1						= VIRTUALREG_START+571,
	kVRegHDROverrideState					= VIRTUALREG_START+572,
	
	kVRegPCIMaxReadRequestSize				= VIRTUALREG_START+573,

	kVRegUserInColorimetry					= VIRTUALREG_START+574,
	kVRegUserInTransfer						= VIRTUALREG_START+575,
	kVRegUserInLuminance					= VIRTUALREG_START+576,
	
	kVRegHdrInColorimetryCh1				= VIRTUALREG_START+577,
	kVRegHdrInTransferCh1					= VIRTUALREG_START+578,
	kVRegHdrInLuminanceCh1					= VIRTUALREG_START+579,
	kVRegHdrInGreenXCh1						= VIRTUALREG_START+580,
	kVRegHdrInGreenYCh1						= VIRTUALREG_START+581,
	kVRegHdrInBlueXCh1						= VIRTUALREG_START+582,
	kVRegHdrInBlueYCh1						= VIRTUALREG_START+583,
	kVRegHdrInRedXCh1						= VIRTUALREG_START+584,
	kVRegHdrInRedYCh1						= VIRTUALREG_START+585,
	kVRegHdrInWhiteXCh1						= VIRTUALREG_START+586,
	kVRegHdrInWhiteYCh1						= VIRTUALREG_START+587,
	kVRegHdrInMasterLumMaxCh1				= VIRTUALREG_START+588,
	kVRegHdrInMasterLumMinCh1				= VIRTUALREG_START+589,
	kVRegHdrInMaxCLLCh1						= VIRTUALREG_START+590,
	kVRegHdrInMaxFALLCh1					= VIRTUALREG_START+591,
	kVRegHDRInOverrideState					= VIRTUALREG_START+592,
	
	kVRegNTV2VPIDRGBRange1					= VIRTUALREG_START+593,
	kVRegNTV2VPIDRGBRange2					= VIRTUALREG_START+594,
	kVRegNTV2VPIDRGBRange3					= VIRTUALREG_START+595,
	kVRegNTV2VPIDRGBRange4					= VIRTUALREG_START+596,
	kVRegNTV2VPIDRGBRange5					= VIRTUALREG_START+597,
	kVRegNTV2VPIDRGBRange6					= VIRTUALREG_START+598,
	kVRegNTV2VPIDRGBRange7					= VIRTUALREG_START+599,
	kVRegNTV2VPIDRGBRange8					= VIRTUALREG_START+600,

	kVRegRotaryGainOverrideEnable			= VIRTUALREG_START+601,
	kVRegAudioMixerOutputGain				= VIRTUALREG_START+602,
	kVRegAudioHeadphoneGain					= VIRTUALREG_START+603,
	kVRegAudioMixerOutputEnable				= VIRTUALREG_START+604,
	kVRegAudioHeadphoneEnable				= VIRTUALREG_START+605,
	kVRegAudioEncoderOutputEnable			= VIRTUALREG_START+606,
	kVRegAudioEncoderHeadphoneEnable		= VIRTUALREG_START+607,

	kVRegDmaTransferRateC2H1				= VIRTUALREG_START+608,
	kVRegDmaHardwareRateC2H1				= VIRTUALREG_START+609,
	kVRegDmaTransferRateH2C1				= VIRTUALREG_START+610,
	kVRegDmaHardwareRateH2C1				= VIRTUALREG_START+611,
	kVRegDmaTransferRateC2H2				= VIRTUALREG_START+612,
	kVRegDmaHardwareRateC2H2				= VIRTUALREG_START+613,
	kVRegDmaTransferRateH2C2				= VIRTUALREG_START+614,
	kVRegDmaHardwareRateH2C2				= VIRTUALREG_START+615,
	kVRegDmaTransferRateC2H3				= VIRTUALREG_START+616,
	kVRegDmaHardwareRateC2H3				= VIRTUALREG_START+617,
	kVRegDmaTransferRateH2C3				= VIRTUALREG_START+618,
	kVRegDmaHardwareRateH2C3				= VIRTUALREG_START+619,
	kVRegDmaTransferRateC2H4				= VIRTUALREG_START+620,
	kVRegDmaHardwareRateC2H4				= VIRTUALREG_START+621,
	kVRegDmaTransferRateH2C4				= VIRTUALREG_START+622,
	kVRegDmaHardwareRateH2C4				= VIRTUALREG_START+623,
	kVRegHDMIInAviInfo1						= VIRTUALREG_START+624,
	kVRegHDMIInDrmInfo1						= VIRTUALREG_START+625,
	kVRegHDMIInDrmGreenPrimary1				= VIRTUALREG_START+626,
	kVRegHDMIInDrmBluePrimary1				= VIRTUALREG_START+627,
	kVRegHDMIInDrmRedPrimary1				= VIRTUALREG_START+628,
	kVRegHDMIInDrmWhitePoint1				= VIRTUALREG_START+629,
	kVRegHDMIInDrmMasteringLuminence1		= VIRTUALREG_START+630,
	kVRegHDMIInDrmLightLevel1				= VIRTUALREG_START+631,
	kVRegHDMIInAviInfo2						= VIRTUALREG_START+632,
	kVRegHDMIInDrmInfo2						= VIRTUALREG_START+633,
	kVRegHDMIInDrmGreenPrimary2				= VIRTUALREG_START+634,
	kVRegHDMIInDrmBluePrimary2				= VIRTUALREG_START+635,
	kVRegHDMIInDrmRedPrimary2				= VIRTUALREG_START+636,
	kVRegHDMIInDrmWhitePoint2				= VIRTUALREG_START+637,
	kVRegHDMIInDrmMasteringLuminence2		= VIRTUALREG_START+638,
	kVRegHDMIInDrmLightLevel2				= VIRTUALREG_START+639,
	kVRegBaseFirmwareDeviceID				= VIRTUALREG_START+640,
	kVRegHDMIOutStatus1						= VIRTUALREG_START+641,
	kVRegAudioOutputToneSelect				= VIRTUALREG_START+642,
	kVRegDynFirmwareUpdateCounts			= VIRTUALREG_START+643,		//	MS 16 bits: # attempts;  LS 16 bits: # successful
	kVRegPCILinkSpeed                       = VIRTUALREG_START+644,
	kVRegPCILinkWidth                       = VIRTUALREG_START+645,

    // These replace the hardware registers for hdmi output version >= 6
    kVRegHDMIOutControl1                    = VIRTUALREG_START+646,     // kRegHDMIOutControl 125
    kVRegHDMIInputStatus1                   = VIRTUALREG_START+647,     // kRegHDMIInputStatus 126
    kVRegHDMIInputControl1                  = VIRTUALREG_START+648,     // kRegHDMIInputControl 127
//	kVRegHDMIOutStatus1                     = VIRTUALREG_START+641,     //  Already defined above...
	kVRegHDMIOutHDRGreenPrimary1            = VIRTUALREG_START+649,
	kVRegHDMIOutHDRBluePrimary1             = VIRTUALREG_START+650,
	kVRegHDMIOutHDRRedPrimary1              = VIRTUALREG_START+651,
	kVRegHDMIOutHDRWhitePoint1              = VIRTUALREG_START+652,
	kVRegHDMIOutHDRMasterLuminance1         = VIRTUALREG_START+653,
	kVRegHDMIOutHDRLightLevel1              = VIRTUALREG_START+654,
	kVRegHDMIOutHDRControl1                 = VIRTUALREG_START+655,

    kVRegHDMIOutControl2                    = VIRTUALREG_START+656,
    kVRegHDMIInputStatus2                   = VIRTUALREG_START+657,
    kVRegHDMIInputControl2                  = VIRTUALREG_START+658,
	kVRegHDMIOutStatus2                     = VIRTUALREG_START+659,
	kVRegHDMIOutHDRGreenPrimary2            = VIRTUALREG_START+660,
	kVRegHDMIOutHDRBluePrimary2             = VIRTUALREG_START+661,
	kVRegHDMIOutHDRRedPrimary2              = VIRTUALREG_START+662,
	kVRegHDMIOutHDRWhitePoint2              = VIRTUALREG_START+663,
	kVRegHDMIOutHDRMasterLuminance2         = VIRTUALREG_START+664,
	kVRegHDMIOutHDRLightLevel2              = VIRTUALREG_START+665,
	kVRegHDMIOutHDRControl2                 = VIRTUALREG_START+666,

    kVRegHDMIOutControl3                    = VIRTUALREG_START+667,
    kVRegHDMIInputStatus3                   = VIRTUALREG_START+668,
    kVRegHDMIInputControl3                  = VIRTUALREG_START+669,
	kVRegHDMIOutStatus3                     = VIRTUALREG_START+670,
	kVRegHDMIOutHDRGreenPrimary3            = VIRTUALREG_START+671,
	kVRegHDMIOutHDRBluePrimary3             = VIRTUALREG_START+672,
	kVRegHDMIOutHDRRedPrimary3              = VIRTUALREG_START+673,
	kVRegHDMIOutHDRWhitePoint3              = VIRTUALREG_START+674,
	kVRegHDMIOutHDRMasterLuminance3         = VIRTUALREG_START+675,
	kVRegHDMIOutHDRLightLevel3              = VIRTUALREG_START+676,
	kVRegHDMIOutHDRControl3                 = VIRTUALREG_START+677,

    kVRegHDMIOutControl4                    = VIRTUALREG_START+678,
    kVRegHDMIInputStatus4                   = VIRTUALREG_START+679,
    kVRegHDMIInputControl4                  = VIRTUALREG_START+680,
	kVRegHDMIOutStatus4                     = VIRTUALREG_START+681,
	kVRegHDMIOutHDRGreenPrimary4            = VIRTUALREG_START+682,
	kVRegHDMIOutHDRBluePrimary4             = VIRTUALREG_START+683,
	kVRegHDMIOutHDRRedPrimary4              = VIRTUALREG_START+684,
	kVRegHDMIOutHDRWhitePoint4              = VIRTUALREG_START+685,
	kVRegHDMIOutHDRMasterLuminance4         = VIRTUALREG_START+686,
	kVRegHDMIOutHDRLightLevel4              = VIRTUALREG_START+687,
	kVRegHDMIOutHDRControl4                 = VIRTUALREG_START+688,

	kVRegSDIOutVPIDTransferCharacteristics1	= VIRTUALREG_START+689,
	kVRegSDIOutVPIDColorimetry1				= VIRTUALREG_START+690,
	kVRegSDIOutVPIDLuminance1				= VIRTUALREG_START+691,
	kVRegSDIOutVPIDRGBRange1				= VIRTUALREG_START+692,
	
	kVRegSDIOutVPIDTransferCharacteristics2	= VIRTUALREG_START+693,
	kVRegSDIOutVPIDColorimetry2				= VIRTUALREG_START+694,
	kVRegSDIOutVPIDLuminance2				= VIRTUALREG_START+695,
	kVRegSDIOutVPIDRGBRange2				= VIRTUALREG_START+696,
	
	kVRegSDIOutVPIDTransferCharacteristics3	= VIRTUALREG_START+697,
	kVRegSDIOutVPIDColorimetry3				= VIRTUALREG_START+698,
	kVRegSDIOutVPIDLuminance3				= VIRTUALREG_START+699,
	kVRegSDIOutVPIDRGBRange3				= VIRTUALREG_START+700,
	
	kVRegSDIOutVPIDTransferCharacteristics4	= VIRTUALREG_START+701,
	kVRegSDIOutVPIDColorimetry4				= VIRTUALREG_START+702,
	kVRegSDIOutVPIDLuminance4				= VIRTUALREG_START+703,
	kVRegSDIOutVPIDRGBRange4				= VIRTUALREG_START+704,
	
	kVRegSDIOutVPIDTransferCharacteristics5	= VIRTUALREG_START+705,
	kVRegSDIOutVPIDColorimetry5				= VIRTUALREG_START+706,
	kVRegSDIOutVPIDLuminance5				= VIRTUALREG_START+707,
	kVRegSDIOutVPIDRGBRange5				= VIRTUALREG_START+708,
	
	kVRegSDIOutVPIDTransferCharacteristics6	= VIRTUALREG_START+709,
	kVRegSDIOutVPIDColorimetry6				= VIRTUALREG_START+710,
	kVRegSDIOutVPIDLuminance6				= VIRTUALREG_START+711,
	kVRegSDIOutVPIDRGBRange6				= VIRTUALREG_START+712,
	
	kVRegSDIOutVPIDTransferCharacteristics7	= VIRTUALREG_START+713,
	kVRegSDIOutVPIDColorimetry7				= VIRTUALREG_START+714,
	kVRegSDIOutVPIDLuminance7				= VIRTUALREG_START+715,
	kVRegSDIOutVPIDRGBRange7				= VIRTUALREG_START+716,
	
	kVRegSDIOutVPIDTransferCharacteristics8	= VIRTUALREG_START+717,
	kVRegSDIOutVPIDColorimetry8				= VIRTUALREG_START+718,
	kVRegSDIOutVPIDLuminance8				= VIRTUALREG_START+719,
	kVRegSDIOutVPIDRGBRange8				= VIRTUALREG_START+720,

    // channel default key signal
    kVRegNTV2KeySignal1                     = VIRTUALREG_START+721,
    kVRegNTV2KeySignal2                     = VIRTUALREG_START+722,
    kVRegNTV2KeySignal3                     = VIRTUALREG_START+723,
    kVRegNTV2KeySignal4                     = VIRTUALREG_START+724,
    kVRegNTV2KeySignal5                     = VIRTUALREG_START+725,
    kVRegNTV2KeySignal6                     = VIRTUALREG_START+726,
    kVRegNTV2KeySignal7                     = VIRTUALREG_START+727,
    kVRegNTV2KeySignal8                     = VIRTUALREG_START+728,

    // sdi out key signal override
    kVRegSDIOutKeySignal1                   = VIRTUALREG_START+729,
    kVRegSDIOutKeySignal2                   = VIRTUALREG_START+730,
    kVRegSDIOutKeySignal3                   = VIRTUALREG_START+731,
    kVRegSDIOutKeySignal4                   = VIRTUALREG_START+732,
    kVRegSDIOutKeySignal5                   = VIRTUALREG_START+733,
    kVRegSDIOutKeySignal6                   = VIRTUALREG_START+734,
    kVRegSDIOutKeySignal7                   = VIRTUALREG_START+735,
    kVRegSDIOutKeySignal8                   = VIRTUALREG_START+736,
	
    kVRegLastAJA							= VIRTUALREG_START+737,		///< @brief The last AJA virtual register slot
	kVRegFirstOEM							= kVRegLastAJA + 1,			///< @brief The first virtual register slot available for general use
	kVRegLast								= VIRTUALREG_START + MAX_NUM_VIRTUAL_REGISTERS - 1	///< @brief Last virtual register slot

} VirtualRegisterNum, NTV2VirtualRegisters;

typedef enum
{
	kVRegMaskHDMIInColorimetry				= BIT(3)+BIT(2)+BIT(1)+BIT(0),
	kVRegMaskHDMIInDolbyVision				= BIT(4),
	kVRegMaskHDMIInPresent					= BIT(0),
	kVRegMaskHDMIInEOTF						= BIT(11)+BIT(10)+BIT(9)+BIT(8),	
	kVRegMaskHDMIInMetadataID				= BIT(15)+BIT(14)+BIT(13)+BIT(12),
	kVRegMaskHDMOutVideoStandard			= BIT(3)+BIT(2)+BIT(1)+BIT(0),
	kVRegMaskHDMOutVideoFrameRate			= BIT(7)+BIT(6)+BIT(5)+BIT(4),
	kVRegMaskHDMOutBitDepth					= BIT(11)+BIT(10)+BIT(9)+BIT(8),
	kVRegMaskHDMOutColorRGB					= BIT(12),
	kVRegMaskHDMOutRangeFull				= BIT(13),
	kVRegMaskHDMOutPixel420					= BIT(14),
	kVRegMaskHDMOutProtocol					= BIT(15),
	kVRegMaskHDMOutAudioFormat				= BIT(19)+BIT(18)+BIT(17)+BIT(16),
	kVRegMaskHDMOutAudioRate				= BIT(23)+BIT(22)+BIT(21)+BIT(20),
	kVRegMaskHDMOutAudioChannels			= BIT(27)+BIT(26)+BIT(25)+BIT(24),

    kVRegMaskSDIOutVPIDValue                = BIT(0)+BIT(1)+BIT(2)+BIT(3)+BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11)+BIT(12)+BIT(13)+BIT(14)+BIT(15),
    kVRegMaskSDIOutVPIDOverride             = BIT(16)
} NTV2VirtualRegisterMasks;

typedef enum
{
	kVRegShiftHDMIInColorimetry				= 0,
	kVRegShiftHDMIInDolbyVision				= 4,
	kVRegShiftHDMIInPresent					= 0,
	kVRegShiftHDMIInEOTF					= 8,
	kVRegShiftHDMIInMetadataID				= 12,
	kVRegShiftHDMOutVideoStandard			= 0,
	kVRegShiftHDMOutVideoFrameRate			= 4,
	kVRegShiftHDMOutBitDepth				= 8,
	kVRegShiftHDMOutColorRGB				= 12,
	kVRegShiftHDMOutRangeFull				= 13,
	kVRegShiftHDMOutPixel420				= 14,
	kVRegShiftHDMOutProtocol				= 15,
	kVRegShiftHDMOutAudioFormat				= 16,
	kVRegShiftHDMOutAudioRate				= 20,
	kVRegShiftHDMOutAudioChannels			= 24,

    kVRegShiftSDIOutVPIDValue               = 0,
    kVRegShiftSDIOutVPIDOverride            = 16
} NTV2VirtualRegisterShifts;

#endif// NTV2VIRTUALREGISTERS_H
