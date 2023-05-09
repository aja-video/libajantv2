/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2devicefeatures.h
	@brief		Declares device capability functions.
	@note		Although this is a .cpp file, it must be compilable for Lin/Mac/Win kernel device drivers.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2DEVICEFEATURES_H
#define NTV2DEVICEFEATURES_H

#if defined(AJALinux) || defined(AJA_LINUX)
	// For size_t
	#ifdef __KERNEL__
		#include <linux/stddef.h>
	#else
		#include <stddef.h>
	#endif
#endif
#include "ajaexport.h"
#include "ajatypes.h"
#include "ntv2enums.h"
#if defined(__CPLUSPLUS__) || defined(__cplusplus)
#elif !defined(NTV2_BUILDING_DRIVER)
	#define false (0)
	#define true (!false)
#endif

/**
	@brief	Used with CNTV2Card::GetBoolParam to determine device capabilities.
**/
typedef enum _NTV2BoolParamID
{
	kNTV2BoolParam_FIRST		= 0,
	kDeviceCanChangeEmbeddedAudioClock = kNTV2BoolParam_FIRST,
	kDeviceCanChangeFrameBufferSize,			///< @brief True if frame buffer sizes are not fixed.
	kDeviceCanDisableUFC,						///< @brief True if there's at least one UFC, and it can be disabled.
	kDeviceCanDo2KVideo,						///< @brief True if device can handle 2Kx1556 (film) video.
	kDeviceCanDo3GLevelConversion,				///< @brief True if device can do 3G level B to 3G level A conversion.
	kDeviceCanDoRGBLevelAConversion,			///< @brief True if the device can do RGB over 3G Level A.
	kDeviceCanDo425Mux,							///< @brief True if the device supports SMPTE 425 mux control.
	kDeviceCanDo4KVideo,						///< @brief True if the device can handle 4K/UHD video.
	kDeviceCanDoAESAudioIn,						///< @note Calculate based on kDeviceGetNumAESAudioInputChannels
	kDeviceCanDoAnalogAudio,					///< @note Calculate based on kDeviceGetNumAnalogAudioInputChannels and kDeviceGetNumAnalogAudioOutputChannels
	kDeviceCanDoAnalogVideoIn,					///< @note Calculate based on kDeviceGetNumAnalogVideoInputs
	kDeviceCanDoAnalogVideoOut,					///< @note Calculate based on kDeviceGetNumAnalogVideoOutputs
	kDeviceCanDoAudio2Channels,					///< @note Calculate based on kDeviceGetMaxAudioChannels
	kDeviceCanDoAudio6Channels,					///< @note Calculate based on kDeviceGetMaxAudioChannels
	kDeviceCanDoAudio8Channels,					///< @note Calculate based on kDeviceGetMaxAudioChannels
	kDeviceCanDoAudio96K,						///< @brief True if Audio System(s) support a 96kHz sample rate.
	kDeviceCanDoAudioDelay,						///< @brief True if Audio System(s) support an adjustable delay.
	kDeviceCanDoBreakoutBox,					///< @brief True if device supports an AJA breakout box.
	kDeviceCanDoCapture,						///< @note Calculate based on kDeviceGetNumVideoInputs and kDeviceGetNumHDMIVideoInputs and kDeviceGetNumAnalogVideoInputs
	kDeviceCanDoColorCorrection,				///< @note Calculate based on kDeviceGetNumLUTs
	kDeviceCanDoCustomAnc,						///< @brief True if device has ANC inserter/extractor firmware.
	kDeviceCanDoDSKOpacity,						///< @brief True if device mixer/keyer supports adjustable opacity.
	kDeviceCanDoDualLink,						///< @brief True if device supports 10-bit RGB input/output over 2-wire SDI.
	kDeviceCanDoDVCProHD,						///< @brief True if device can squeeze/stretch between 1920x1080/1280x1080 and 1280x720/960x720.
	kDeviceCanDoEnhancedCSC,					///< @brief True if device has enhanced CSCs.
	kDeviceCanDoFrameStore1Display,				///< @brief True if device can display/output video from FrameStore 1.
	kDeviceCanDoFreezeOutput,					///< @brief True if device can freeze output video.
	kDeviceCanDoHDMIOutStereo,					///< @brief True if device supports 3D/stereo HDMI video output.
	kDeviceCanDoHDV,							///< @brief True if device can squeeze/stretch between 1920x1080 and 1440x1080.
	kDeviceCanDoHDVideo,						///< @brief True if device can handle HD (High Definition) video.
	kDeviceCanDoIsoConvert,						///< @brief True if device can do ISO conversion.
	kDeviceCanDoLTC,							///< @brief True if device can read LTC (Linear TimeCode) from one of its inputs.
	kDeviceCanDoLTCInOnRefPort,					///< @brief True if device can read LTC (Linear TimeCode) from its reference input.
	kDeviceCanDoMSI,							///< @brief True if device DMA hardware supports MSI (Message Signaled Interrupts).
	kDeviceCanDoMultiFormat,					///< @brief True if device can simultaneously handle different video formats on more than one SDI input or output.
	kDeviceCanDoPCMControl,						///< @brief True if device can mark specific audio channel pairs as not carrying PCM (Pulse Code Modulation) audio.
	kDeviceCanDoPCMDetection,					///< @brief True if device can detect which audio channel pairs are not carrying PCM (Pulse Code Modulation) audio.
	kDeviceCanDoPIO,							///< @brief True if device supports Programmed I/O.
	kDeviceCanDoPlayback,						///< @note Calculate based on kDeviceGetNumVideoOutputs and kDeviceGetNumHDMIVideoOutputs and kDeviceGetNumAnalogVideoOutputs
	kDeviceCanDoProgrammableCSC,				///< @brief True if device has at least one programmable color space converter widget.
	kDeviceCanDoProgrammableRS422,				///< @brief True if device has at least one RS-422 serial port, and it (they) can be programmed (for baud rate, parity, etc.).
	kDeviceCanDoProRes,							///< @brief True if device can can accommodate Apple ProRes-compressed video in its frame buffers.
	kDeviceCanDoQREZ,							///< @brief True if device can handle QRez.
	kDeviceCanDoQuarterExpand,					///< @brief True if device can handle quarter-sized frames (pixel-halving and line-halving during input, pixel-double and line-double during output).
	kDeviceCanDoRateConvert,					///< @brief True if device can do frame rate conversion.
	kDeviceCanDoRGBPlusAlphaOut,				///< @brief True if device has CSCs capable of splitting the key (alpha) and YCbCr (fill) from RGB frame buffers that include alpha. (Has nothing to do with RGB wire formats.)
	kDeviceCanDoRP188,							///< @brief True if device can insert and/or extract RP-188/VITC.
	kDeviceCanDoSDVideo,						///< @brief True if device can handle SD (Standard Definition) video.
	kDeviceCanDoSDIErrorChecks,					///< @brief True if device can perform SDI error checking.
	kDeviceCanDoStackedAudio,					///< @brief True if device uses a "stacked" arrangement of its audio buffers.
	kDeviceCanDoStereoIn,						///< @brief True if device supports 3D video input over dual-stream SDI.
	kDeviceCanDoStereoOut,						///< @brief True if device supports 3D video output over dual-stream SDI.
	kDeviceCanDoThunderbolt,					///< @brief True if device connects to the host using a Thunderbolt cable.
	kDeviceCanDoVideoProcessing,				///< @brief True if device can do video processing.
	kDeviceCanMeasureTemperature,				///< @brief True if device can measure its FPGA die temperature.
	kDeviceCanReportFrameSize,					///< @brief True if device can report its frame size.
	kDeviceHasBiDirectionalSDI,					///< @brief True if device SDI connectors are bi-directional.
	kDeviceHasColorSpaceConverterOnChannel2,	///< @brief Calculate based on if NTV2_WgtCSC2 is present.
	kDeviceHasNWL,								///< @brief True if device has NorthWest Logic DMA hardware.
	kDeviceHasPCIeGen2,							///< @brief True if device supports 2nd-generation PCIe.
	kDeviceHasRetailSupport,					///< @brief True if device is supported by AJA "retail" software (AJA ControlPanel & ControlRoom).
	kDeviceHasSDIRelays,						///< @brief True if device has bypass relays on its SDI connectors.
	kDeviceHasSPIFlash,							///< @brief True if device has SPI flash hardware.
	kDeviceHasSPIFlashSerial,					///< @brief True if device has serial SPI flash hardware.
	kDeviceHasSPIv2,							///< @brief Use kDeviceGetSPIVersion instead.
	kDeviceHasSPIv3,							///< @brief Use kDeviceGetSPIVersion instead.
	kDeviceHasSPIv4,							///< @brief Use kDeviceGetSPIVersion instead.
	kDeviceIs64Bit,								///< @brief True if device is 64-bit addressable.
	kDeviceIsDirectAddressable,					///< @brief True if device is direct addressable.
	kDeviceIsExternalToHost,					///< @brief True if device connects to the host with a cable.
	kDeviceIsSupported,							///< @brief True if device is supported by this SDK.
	kDeviceNeedsRoutingSetup,					///< @brief True if device widget routing can be queried or changed.
	kDeviceSoftwareCanChangeFrameBufferSize,	///< @brief True if device frame buffer size can be changed.
	kDeviceCanThermostat,						///< @brief True if device fan can be thermostatically controlled.
	kDeviceHasHEVCM31,							///< @brief True if device has an HEVC M31 encoder.
	kDeviceHasHEVCM30,							///< @brief True if device has an HEVC M30 encoder/decoder.
	kDeviceCanDoVITC2,							///< @brief True if device can insert or extract RP-188/VITC2.
	kDeviceCanDoHDMIHDROut,						///< @brief True if device supports HDMI HDR output.
	kDeviceCanDoJ2K,							///< @brief True if device supports JPEG 2000 codec.
	kDeviceCanDo12gRouting,						///< @brief True if device supports 12G routing crosspoints.
	kDeviceCanDo12GSDI,							///< @brief True if device has 12G SDI connectors.
	kDeviceCanDo2110,							///< @brief True if device supports SMPTE ST2110.
	kDeviceCanDo8KVideo,						///< @brief True if device supports 8K video formats.
	kDeviceCanDoAudio192K,						///< @brief True if Audio System(s) support a 192kHz sample rate.
	kDeviceCanDoAudioMixer,						///< @brief True if device has a firmware audio mixer.
	kDeviceCanDoCustomAux,						///< @brief True if device supports HDMI AUX data insertion/extraction.
	kDeviceCanDoFramePulseSelect,				///< @brief True if device supports frame pulse source independent of reference source.
	kDeviceCanDoHDMIMultiView,					///< @brief True if device can rasterize 4 HD signals into a single HDMI output.
	kDeviceHasMultiRasterWidget,				///< @brief True if device can rasterize 4 HD signals into a single HDMI output.
	kDeviceCanDoHFRRGB,							///< @brief True if device supports 1080p RGB at more than 50Hz frame rates.
	kDeviceCanDoIP,								///< @brief True if device has SFP connectors.
	kDeviceCanDoMultiLinkAudio,					///< @brief True if device supports grouped audio system control.
	kDeviceCanDoWarmBootFPGA,					///< @brief True if device can warm-boot to load updated firmware.
	kDeviceCanReportFailSafeLoaded,				///< @brief True if device can report if its "fail-safe" firmware is loaded/running.
	kDeviceCanReportRunningFirmwareDate,		///< @brief True if device can report its running (and not necessarily installed) firmware date.
	kDeviceHasAudioMonitorRCAJacks,				///< @brief True if device has a pair of unbalanced RCA audio monitor output connectors.
	kDeviceHasBiDirectionalAnalogAudio,			///< @brief True if device has a bi-directional analog audio connector.
	kDeviceHasGenlockv2,						///< @brief True if device has version 2 genlock hardware and/or firmware.
	kDeviceHasGenlockv3,						///< @brief True if device has version 3 genlock hardware and/or firmware.
	kDeviceHasHeadphoneJack,					///< @brief True if device has a headphone jack.
	kDeviceHasLEDAudioMeters,					///< @brief True if device has LED audio meters.
	kDeviceHasRotaryEncoder,					///< @brief True if device has a rotary encoder volume control.
	kDeviceHasSPIv5,							///< @brief Use kDeviceGetSPIVersion instead.
	kDeviceHasXilinxDMA,						///< @brief True if device has Xilinx DMA hardware.
	kDeviceHasMicrophoneInput,					///< @brief True if device has a microphone input connector.
	kNTV2BoolParam_LAST,
	kDeviceCanDo_INVALID	= kNTV2BoolParam_LAST
} NTV2BoolParamID;

#define NTV2_IS_VALID_BOOLPARAMID(__x__)		((__x__) >= kNTV2BoolParam_FIRST  &&  (__x__) < kNTV2BoolParam_LAST)

/**
	@brief	Used with CNTV2Card::GetNumericParam to determine device capabilities.
**/
typedef enum _NTV2NumericParamID
{
	kNTV2NumericParam_FIRST		= 2000,
	kDeviceGetActiveMemorySize	= kNTV2NumericParam_FIRST,	///< @brief The size, in bytes, of the device's active RAM available for video and audio.
	kDeviceGetDACVersion,						///< @brief The version number of the DAC on the device.
	kDeviceGetDownConverterDelay,				///< @brief The down-converter delay on the device.
	kDeviceGetHDMIVersion,						///< @brief The HDMI input(s) and/or output(s) on the device.
	kDeviceGetLUTVersion,						///< @brief The version number of the LUT(s) on the device.
	kDeviceGetMaxAudioChannels,					///< @brief The maximum number of audio channels that a single Audio System can support on the device.
	kDeviceGetMaxRegisterNumber,				///< @brief The highest register number for the device.
	kDeviceGetMaxTransferCount,					///< @brief The maximum number of 32-bit words that the DMA engine can move at a time on the device.
	kDeviceGetNumDMAEngines,					///< @brief The number of DMA engines on the device.
	kDeviceGetNumVideoChannels,					///< @brief The number of video channels supported on the device.
	kDeviceGetPingLED,							///< @brief The highest bit number of the LED bits in the Global Control Register on the device.
	kDeviceGetUFCVersion,						///< @brief The version number of the UFC on the device.
	kDeviceGetNum4kQuarterSizeConverters,		///< @brief The number of quarter-size 4K/UHD down-converters on the device.
	kDeviceGetNumAESAudioInputChannels,			///< @brief The number of AES/EBU audio input channels on the device.
	kDeviceGetNumAESAudioOutputChannels,		///< @brief The number of AES/EBU audio output channels on the device.
	kDeviceGetNumAnalogAudioInputChannels,		///< @brief The number of analog audio input channels on the device.
	kDeviceGetNumAnalogAudioOutputChannels,		///< @brief The number of analog audio output channels on the device.
	kDeviceGetNumAnalogVideoInputs,				///< @brief The number of analog video inputs on the device.
	kDeviceGetNumAnalogVideoOutputs,			///< @brief The number of analog video outputs on the device.
	kDeviceGetNumAudioSystems,					///< @brief The number of independent Audio Systems on the device.
	kDeviceGetNumCrossConverters,				///< @brief The number of cross-converters on the device.
	kDeviceGetNumCSCs,							///< @brief The number of colorspace converter widgets on the device.
	kDeviceGetNumDownConverters,				///< @brief The number of down-converters on the device.
	kDeviceGetNumEmbeddedAudioInputChannels,	///< @brief The number of SDI-embedded input audio channels supported by the device.
	kDeviceGetNumEmbeddedAudioOutputChannels,	///< @brief The number of SDI-embedded output audio channels supported by the device.
	kDeviceGetNumFrameStores,					///< @brief The number of FrameStores on the device.
	kDeviceGetNumFrameSyncs,					///< @brief The number of frame sync widgets on the device.
	kDeviceGetNumHDMIAudioInputChannels,		///< @brief The number of HDMI audio input channels on the device.
	kDeviceGetNumHDMIAudioOutputChannels,		///< @brief The number of HDMI audio output channels on the device.
	kDeviceGetNumHDMIVideoInputs,				///< @brief The number of HDMI video inputs on the device.
	kDeviceGetNumHDMIVideoOutputs,				///< @brief The number of HDMI video outputs on the device.
	kDeviceGetNumInputConverters,				///< @brief The number of input converter widgets on the device.
	kDeviceGetNumLUTs,							///< @brief The number of LUT widgets on the device.
	kDeviceGetNumMixers,						///< @brief The number of mixer/keyer widgets on the device.
	kDeviceGetNumOutputConverters,				///< @brief The number of output converter widgets on the device.
	kDeviceGetNumReferenceVideoInputs,			///< @brief The number of external reference video inputs on the device.
	kDeviceGetNumSerialPorts,					///< @brief The number of RS-422 serial ports on the device.
	kDeviceGetNumUpConverters,					///< @brief The number of up-converters on the device.
	kDeviceGetNumVideoInputs,					///< @brief The number of SDI video inputs on the device.
	kDeviceGetNumVideoOutputs,					///< @brief The number of SDI video outputs on the device.
	kDeviceGetNum2022ChannelsSFP1,				///< @brief The number of 2022 channels configured on SFP 1 on the device.
	kDeviceGetNum2022ChannelsSFP2,				///< @brief The number of 2022 channels configured on SFP 2 on the device.
	kDeviceGetNumLTCInputs,						///< @brief The number of analog LTC inputs on the device.
	kDeviceGetNumLTCOutputs,					///< @brief The number of analog LTC outputs on the device.
	kDeviceGetNumMicInputs,						///< @brief The number of microphone inputs on the device.
	kDeviceGetNumLUTBanks,						///< @brief The number of LUT banks on the device. New in SDK 17.0
	kDeviceGetTotalNumAudioSystems,				///< @brief The total number of audio systems on the device, including host audio and mixer audio systems, if present. New in SDK 17.0
	kDeviceGetNumBufferedAudioSystems,			///< @brief The total number of audio systems on the device that can read/write audio buffer memory. Includes host audio system, if present. New in SDK 17.0
	kNTV2NumericParam_LAST,
	kDeviceGetNum_INVALID	= kNTV2NumericParam_LAST
} NTV2NumericParamID;

#define NTV2_IS_VALID_NUMPARAMID(__x__)		((__x__) >= kNTV2NumericParam_FIRST  &&  (__x__) < kNTV2NumericParam_LAST)

/**
	@brief	Identifies NTV2 enumerated types, used in CNTV2DriverInterface::GetSupported.
**/
typedef enum _NTV2EnumsID
{
	kNTV2EnumsID_DeviceID,					///< @brief Identifies the NTV2DeviceID enumerated type
	kNTV2EnumsID_Standard,					///< @brief Identifies the NTV2Standard enumerated type
	kNTV2EnumsID_PixelFormat,				///< @brief Identifies the NTV2PixelFormat enumerated type
	kNTV2EnumsID_FrameGeometry,				///< @brief Identifies the NTV2FrameGeometry enumerated type
	kNTV2EnumsID_FrameRate,					///< @brief Identifies the NTV2FrameRate enumerated type
	kNTV2EnumsID_ScanGeometry,				///< @brief Identifies the NTV2ScanGeometry enumerated type
	kNTV2EnumsID_VideoFormat,				///< @brief Identifies the NTV2VideoFormat enumerated type
	kNTV2EnumsID_Mode,						///< @brief Identifies the NTV2Mode enumerated type
	kNTV2EnumsID_InputSource,				///< @brief Identifies the NTV2InputSource enumerated type
	kNTV2EnumsID_OutputDest,				///< @brief Identifies the NTV2OutputDest enumerated type
	kNTV2EnumsID_Channel,					///< @brief Identifies the NTV2Channel enumerated type
	kNTV2EnumsID_RefSource,					///< @brief Identifies the NTV2RefSource enumerated type
	kNTV2EnumsID_AudioRate,					///< @brief Identifies the NTV2AudioRate enumerated type
	kNTV2EnumsID_AudioSource,				///< @brief Identifies the NTV2AudioSource enumerated type
	kNTV2EnumsID_WidgetID,					///< @brief Identifies the NTV2AudioWidgetID enumerated type
	kNTV2EnumsID_ConversionMode,			///< @brief Identifies the NTV2ConversionMode enumerated type
	kNTV2EnumsID_DSKMode,					///< @brief	Identifies the NTV2DSKMode enumerated type
	kNTV2EnumsID_INVALID
} NTV2EnumsID;

#define NTV2_IS_VALID_ENUMSID(__x__)		((__x__) >= kNTV2EnumsID_DeviceID  &&  (__x__) < kNTV2EnumsID_INVALID)


//	Most of the device features functions are generated from a Python script.
//	The script writes the implementations into 'ntv2devicefeatures.hpp', and the declarations into 'ntv2devicefeatures.hh'...
#include "ntv2devicefeatures.hh"

#if defined(__cplusplus) && defined(NTV2_BUILDING_DRIVER)
extern "C"
{
#endif
/*
	@return True if the device having the given ID can do audio output;	 otherwise false.
	@param[in]	inDeviceID	Specifies the NTV2DeviceID of the device of interest.
*/
AJAExport bool NTV2DeviceCanDoAudioOut(const NTV2DeviceID inDeviceID);
/*
	@return True if the device having the given ID can do audio input;	otherwise false.
	@param[in]	inDeviceID	Specifies the NTV2DeviceID of the device of interest.
*/
AJAExport bool NTV2DeviceCanDoAudioIn(const NTV2DeviceID inDeviceID);

AJAExport bool NTV2DeviceCanDo292Out(NTV2DeviceID boardID, UWord index0);
AJAExport bool NTV2DeviceCanDo3GOut (NTV2DeviceID boardID, UWord index0);
AJAExport bool NTV2DeviceCanDo12GOut(NTV2DeviceID boardID, UWord index0);
AJAExport bool NTV2DeviceCanDo292In(NTV2DeviceID boardID, UWord index0);
AJAExport bool NTV2DeviceCanDo3GIn(NTV2DeviceID boardID, UWord index0);
AJAExport bool NTV2DeviceCanDo12GIn(NTV2DeviceID boardID, UWord index0);
AJAExport bool NTV2DeviceCanDoLTCEmbeddedN (NTV2DeviceID boardID, UWord index0);

/**
	@return True if the device having the given ID supports the given output destination connector;	 otherwise false.
	@param[in]	inDeviceID		Specifies the NTV2DeviceID of the device of interest.
	@param[in]	inOutputDest	Specifies the NTV2OutputDestination of interest.
**/
AJAExport bool NTV2DeviceCanDoOutputDestination (const NTV2DeviceID inDeviceID, const NTV2OutputDestination inOutputDest);

/**
	@return The minimum number of 8MB chunks that will accommodate a raster having the given frame geometry and pixel format.
	@param[in]	inFG	A valid frame geometry.
	@param[in]	inFBF	A valid frame buffer format.
**/
AJAExport UWord Get8MBFrameSizeFactor (const NTV2FrameGeometry inFG, const NTV2FrameBufferFormat inFBF);

/**
	@return		True if the device having the given ID has at least one programmable LUT.
	@param[in]	inDeviceID		Specifies the NTV2DeviceID of interest.
**/
AJAExport bool NTV2DeviceCanDoColorCorrection (const NTV2DeviceID inDeviceID);

/**
	@return		True if the device having the given ID has at least one color space converter widget - all CSCs are programable.
	@param[in]	inDeviceID		Specifies the NTV2DeviceID of interest.
**/
AJAExport bool NTV2DeviceCanDoProgrammableCSC (const NTV2DeviceID inDeviceID);


// Overloading not supported by the ANSI C compiler used for Linux drivers.
// 
// TODO: Audit all platforms and switch all the original calls to the _Ex
// versions.
#if (defined(__CPLUSPLUS__) || defined(__cplusplus)) && !defined(NTV2_BUILDING_DRIVER)
	AJAExport ULWord NTV2DeviceGetFrameBufferSize(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetNumberFrameBuffers(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetAudioFrameBuffer(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetAudioFrameBuffer2(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
#else
	AJAExport ULWord NTV2DeviceGetFrameBufferSize_Ex(NTV2DeviceID boardID);		//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetNumberFrameBuffers_Ex(NTV2DeviceID boardID);	//	Revisit for 2MB granularity
	AJAExport ULWord NTV2DeviceGetAudioFrameBuffer_Ex(NTV2DeviceID boardID);	//	Revisit for 2MB granularity
#endif

AJAExport ULWord NTV2DeviceGetFrameBufferSize(NTV2DeviceID boardID, NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat);	//	Revisit for 2MB granularity
AJAExport ULWord NTV2DeviceGetNumberFrameBuffers(NTV2DeviceID boardID, NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat); //	Revisit for 2MB granularity
AJAExport ULWord NTV2DeviceGetAudioFrameBuffer(NTV2DeviceID boardID, NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat);	//	Revisit for 2MB granularity
AJAExport ULWord NTV2DeviceGetAudioFrameBuffer2(NTV2DeviceID boardID, NTV2FrameGeometry frameGeometry, NTV2FrameBufferFormat frameFormat);	//	Revisit for 2MB granularity

AJAExport bool NTV2DeviceGetVideoFormatFromState (	NTV2VideoFormat *		pOutValue,
													const NTV2FrameRate		inFrameRate,
													const NTV2FrameGeometry inFrameGeometry,
													const NTV2Standard		inStandard,
													const ULWord			inIsSMPTE372Enabled);

AJAExport bool NTV2DeviceGetVideoFormatFromState_Ex (	NTV2VideoFormat *		pOutValue,
														const NTV2FrameRate		inFrameRate,
														const NTV2FrameGeometry inFrameGeometry,
														const NTV2Standard		inStandard,
														const ULWord			inIsSMPTE372Enabled,
														const bool				inIsProgressivePicture);

AJAExport bool NTV2DeviceGetVideoFormatFromState_Ex2 (	NTV2VideoFormat *		pOutValue,
														const NTV2FrameRate		inFrameRate,
														const NTV2FrameGeometry inFrameGeometry,
														const NTV2Standard		inStandard,
														const ULWord			inIsSMPTE372Enabled,
														const bool				inIsProgressivePicture,
														const bool				inIsSquareDivision);
#if !defined(NTV2_DEPRECATE_17_0)
	//	In SDK 17.0, these were all replaced by NTV2DeviceGetSPIFlashVersion:
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasSPIv2(const NTV2DeviceID inDeviceID);) ///< @deprecated	Call NTV2DeviceGetSPIFlashVersion instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasSPIv3(const NTV2DeviceID inDeviceID);) ///< @deprecated	Call NTV2DeviceGetSPIFlashVersion instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasSPIv4(const NTV2DeviceID inDeviceID);) ///< @deprecated	Call NTV2DeviceGetSPIFlashVersion instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasSPIv5(const NTV2DeviceID inDeviceID);) ///< @deprecated	Call NTV2DeviceGetSPIFlashVersion instead.

	//	In SDK 17.0, these were replaced by NTV2DeviceGetGenlockVersion...
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasGenlockv2(const NTV2DeviceID devID);)	///< @deprecated	Use NTV2DeviceGetGenlockVersion instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasGenlockv3(const NTV2DeviceID devID);)	///< @deprecated	Use NTV2DeviceGetGenlockVersion instead.

	//	In SDK 17.0, this was replaced by NTV2DeviceCanDoWidget(NTV2_WgtCSC2)...
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasColorSpaceConverterOnChannel2(const NTV2DeviceID devID);)	///< @deprecated	Call NTV2DeviceCanDoWidget with NTV2_WgtCSC2 instead.

	//	In SDK 17.0, these were replaced by NTV2GetMaxNumAudioChannels:
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudio2Channels(const NTV2DeviceID devID);)	///< @deprecated	Use NTV2DeviceGetMaxAudioChannels instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudio6Channels(const NTV2DeviceID devID);)	///< @deprecated	Use NTV2DeviceGetMaxAudioChannels instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudio8Channels(const NTV2DeviceID devID);)	///< @deprecated	Use NTV2DeviceGetMaxAudioChannels instead.

	//	These have been marked deprecated for some time. In SDK 17.0, it's official.
	AJAExport NTV2_DEPRECATED_f(UWord NTV2DeviceGetNumAudioStreams(const NTV2DeviceID devID);)	///< @deprecated	Use NTV2DeviceGetNumAudioSystems instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudioN(const NTV2DeviceID devID, UWord index0);)	///< @deprecated	Use NTV2DeviceGetNumAudioSystems instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoLTCOutN(const NTV2DeviceID devID, UWord index0);)	///< @deprecated	Use NTV2DeviceGetNumLTCOutputs instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoLTCInN(const NTV2DeviceID devID, UWord index0);)	///< @deprecated	Use NTV2DeviceGetNumLTCInputs instead.
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoRS422N(const NTV2DeviceID devID, const NTV2Channel ch);)	///< @deprecated	Use NTV2DeviceGetNumSerialPorts instead.
#endif	//	NTV2_DEPRECATE_17_0

#if !defined(NTV2_DEPRECATE_15_6)
	AJAExport bool NTV2DeviceCanConnect (const NTV2DeviceID inDeviceID, const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt);	///< @deprecated	Obsoleted by CNTV2Card::HasCanConnectROM and CNTV2Card::CanConnect
#endif	//	NTV2_DEPRECATE_15_6
AJAExport bool NTV2DeviceCanDoTCIndex (const NTV2DeviceID inDeviceID, const NTV2TCIndex inTCIndex); ///< @return	True if the device having the given ID supports the specified NTV2TCIndex.
AJAExport bool NTV2DeviceCanDoInputTCIndex (const NTV2DeviceID inDeviceID, const NTV2TCIndex inTCIndex);	///< @return	True if the device having the given ID supports the specified NTV2TCIndex for input.
AJAExport NTV2AudioSystem NTV2DeviceGetAudioMixerSystem(const NTV2DeviceID inDeviceID);	///< @return	The NTV2AudioSystem used by the audio mixer for the given device (or NTV2_AUDIOSYSTEM_INVALID if there is no mixer).
AJAExport NTV2AudioSystem NTV2DeviceGetHostAudioSystem(const NTV2DeviceID inDeviceID);	///< @return	The NTV2AudioSystem used for host audio support for the given device (or NTV2_AUDIOSYSTEM_INVALID if there is no host audio system).
AJAExport bool NTV2DeviceROMHasBankSelect (const NTV2DeviceID inDeviceID);	///< @return	True if the device has SPI flash that incorporates bank selection.

#if !defined (NTV2_DEPRECATE_14_3)
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoFreezeOutput (const NTV2DeviceID inDeviceID));	///< @deprecated	This function is obsolete.
#endif	//	!defined (NTV2_DEPRECATE_14_3)

#if !defined (NTV2_DEPRECATE)
	#define NTV2GetDACVersion						NTV2DeviceGetDACVersion					///< @deprecated	Use NTV2DeviceGetDACVersion instead.
	#define NTV2GetNumDMAEngines					NTV2DeviceGetNumDMAEngines				///< @deprecated	Use NTV2DeviceGetNumDMAEngines instead.
	#define NTV2DeviceGetNumAnlgVideoInputs			NTV2DeviceGetNumAnalogVideoInputs		///< @deprecated	Use NTV2DeviceGetNumAnalogVideoInputs instead.
	#define NTV2DeviceGetNumAnlgVideoOutputs		NTV2DeviceGetNumAnalogVideoOutputs		///< @deprecated	Use NTV2DeviceGetNumAnalogVideoOutputs instead.
	#define NTV2GetHDMIVersion						NTV2DeviceGetHDMIVersion				///< @deprecated	Use NTV2DeviceGetHDMIVersion instead.
	#define NTV2DeviceGetNumAudioEngines			NTV2DeviceGetNumAudioSystems			///< @deprecated	Use NTV2DeviceGetNumAudioSystems instead.

	#define NTV2DeviceCanDoProAudio(_boardID_)								false
	#define NTV2BoardCanDoProcAmp(_boardID_)								false
	#define NTV2BoardCanDoBrightnessAdjustment(_boardID_,_videoADCMode_)	false
	#define NTV2BoardCanDoContrastAdjustment(_boardID_,_videoADCMode_)		false
	#define NTV2BoardCanDoSaturationAdjustment(_boardID_,_videoADCMode_)	false
	#define NTV2BoardCanDoHueAdjustment(_boardID_,_videoADCMode_)			false

	#define NTV2BoardCanDoAudio(_boardID_)			NTV2DeviceCanDoAudioN((_boardID_), 0)
	#define NTV2BoardCanDoAudio2(_boardID_)			NTV2DeviceCanDoAudioN((_boardID_), 1)
	#define NTV2BoardCanDoAudio3(_boardID_)			NTV2DeviceCanDoAudioN((_boardID_), 2)
	#define NTV2BoardCanDoAudio4(_boardID_)			NTV2DeviceCanDoAudioN((_boardID_), 3)
	#define NTV2BoardCanDoAudio5(_boardID_)			NTV2DeviceCanDoAudioN((_boardID_), 4)
	#define NTV2BoardCanDoAudio6(_boardID_)			NTV2DeviceCanDoAudioN((_boardID_), 5)
	#define NTV2BoardCanDoAudio7(_boardID_)			NTV2DeviceCanDoAudioN((_boardID_), 6)
	#define NTV2BoardCanDoAudio8(_boardID_)			NTV2DeviceCanDoAudioN((_boardID_), 7)

	#define NTV2BoardCanDo3G(_boardID_)				NTV2DeviceCanDo3GOut((_boardID_), 0)
	#define NTV2BoardCanDo3GOut2(_boardID_)			NTV2DeviceCanDo3GOut((_boardID_), 1)
	#define NTV2BoardCanDo3GOut3(_boardID_)			NTV2DeviceCanDo3GOut((_boardID_), 2)
	#define NTV2BoardCanDo3GOut4(_boardID_)			NTV2DeviceCanDo3GOut((_boardID_), 3)
	#define NTV2BoardCanDo3GOut5(_boardID_)			NTV2DeviceCanDo3GOut((_boardID_), 4)
	#define NTV2BoardCanDo3GOut6(_boardID_)			NTV2DeviceCanDo3GOut((_boardID_), 5)
	#define NTV2BoardCanDo3GOut7(_boardID_)			NTV2DeviceCanDo3GOut((_boardID_), 6)
	#define NTV2BoardCanDo3GOut8(_boardID_)			NTV2DeviceCanDo3GOut((_boardID_), 7)

	#define NTV2BoardCanDoMixer2(_boardID_)			(NTV2DeviceGetNumMixers(_boardID_) >= 2)
	#define NTV2BoardCanDoMixer3(_boardID_)			(NTV2DeviceGetNumMixers(_boardID_) >= 3)
	#define NTV2BoardCanDoMixer4(_boardID_)			(NTV2DeviceGetNumMixers(_boardID_) >= 4)

	#define NTV2BoardCanDoLTCIn(_boardID_)			NTV2DeviceCanDoLTCInN((_boardID_), 0)
	#define NTV2BoardCanDoLTCIn2(_boardID_)			NTV2DeviceCanDoLTCInN((_boardID_), 1)

	#define NTV2BoardCanDoLTCOut(_boardID_)			NTV2DeviceCanDoLTCOutN((_boardID_), 0)
	#define NTV2BoardCanDoLTCOut2(_boardID_)		NTV2DeviceCanDoLTCOutN((_boardID_), 1)

	#define NTV2BoardCanDoLTCEmbedded(_boardID_)	NTV2DeviceCanDoLTCEmbeddedN((_boardID_), 0)
	#define NTV2BoardCanDoLTCEmbedded2(_boardID_)	NTV2DeviceCanDoLTCEmbeddedN((_boardID_), 1)
	#define NTV2BoardCanDoLTCEmbedded3(_boardID_)	NTV2DeviceCanDoLTCEmbeddedN((_boardID_), 2)
	#define NTV2BoardCanDoLTCEmbedded4(_boardID_)	NTV2DeviceCanDoLTCEmbeddedN((_boardID_), 3)
	#define NTV2BoardCanDoLTCEmbedded5(_boardID_)	NTV2DeviceCanDoLTCEmbeddedN((_boardID_), 4)
	#define NTV2BoardCanDoLTCEmbedded6(_boardID_)	NTV2DeviceCanDoLTCEmbeddedN((_boardID_), 5)
	#define NTV2BoardCanDoLTCEmbedded7(_boardID_)	NTV2DeviceCanDoLTCEmbeddedN((_boardID_), 6)
	#define NTV2BoardCanDoLTCEmbedded8(_boardID_)	NTV2DeviceCanDoLTCEmbeddedN((_boardID_), 7)

	#define NTV2BoardCanDoUART(_boardID_)			NTV2DeviceCanDoUARTN((_boardID_), 0)
	#define NTV2BoardCanDoUART2(_boardID_)			NTV2DeviceCanDoUARTN((_boardID_), 1)

	#define NTV2BoardCanDoUARTN(_boardID_,_ndx0_)	NTV2DeviceCanDoRS422N((_boardID_),(_ndx0_))
	#define NTV2DeviceCanDoUARTN(_boardID_,_ndx0_)	NTV2DeviceCanDoRS422N((_boardID_),(_ndx0_))
#endif	//	!defined (NTV2_DEPRECATE)

#if !defined (NTV2_DEPRECATE_14_2)	//	UNDEFINED AS OF SDK 14.2
	#define NTV2BoardCanChangeEmbeddedAudioClock		NTV2DeviceCanChangeEmbeddedAudioClock			///< @deprecated	Use NTV2DeviceCanChangeEmbeddedAudioClock instead.
	#define NTV2BoardCanChangeFrameBufferSize			NTV2DeviceCanChangeFrameBufferSize				///< @deprecated	Use NTV2DeviceCanChangeFrameBufferSize instead.
	#define NTV2BoardCanDisableUFC						NTV2DeviceCanDisableUFC							///< @deprecated	Use NTV2DeviceCanDisableUFC instead.
	#define NTV2BoardCanDo2KVideo						NTV2DeviceCanDo2KVideo							///< @deprecated	Use NTV2DeviceCanDo2KVideo instead.
	#define NTV2BoardCanDo3GLevelConversion				NTV2DeviceCanDo3GLevelConversion				///< @deprecated	Use NTV2DeviceCanDo3GLevelConversion instead.
	#define NTV2BoardCanDo3GOut							NTV2DeviceCanDo3GOut							///< @deprecated	Use NTV2DeviceCanDo3GOut instead.
	#define NTV2BoardCanDo4KVideo						NTV2DeviceCanDo4KVideo							///< @deprecated	Use NTV2DeviceCanDo4KVideo instead.
	#define NTV2BoardCanDoAESAudioIn					NTV2DeviceCanDoAESAudioIn						///< @deprecated	Use NTV2DeviceCanDoAESAudioIn instead.
	#define NTV2BoardCanDoAnalogAudio					NTV2DeviceCanDoAnalogAudio						///< @deprecated	Use NTV2DeviceCanDoAnalogAudio instead.
	#define NTV2BoardCanDoAnalogVideoIn					NTV2DeviceCanDoAnalogVideoIn					///< @deprecated	Use NTV2DeviceCanDoAnalogVideoIn instead.
	#define NTV2BoardCanDoAnalogVideoOut				NTV2DeviceCanDoAnalogVideoOut					///< @deprecated	Use NTV2DeviceCanDoAnalogVideoOut instead.
	#define NTV2BoardCanDoAudio2Channels				NTV2DeviceCanDoAudio2Channels					///< @deprecated	Use NTV2DeviceCanDoAudio2Channels instead.
	#define NTV2BoardCanDoAudio6Channels				NTV2DeviceCanDoAudio6Channels					///< @deprecated	Use NTV2DeviceCanDoAudio6Channels instead.
	#define NTV2BoardCanDoAudio8Channels				NTV2DeviceCanDoAudio8Channels					///< @deprecated	Use NTV2DeviceCanDoAudio8Channels instead.
	#define NTV2BoardCanDoAudio96K						NTV2DeviceCanDoAudio96K							///< @deprecated	Use NTV2DeviceCanDoAudio96K instead.
	#define NTV2BoardCanDoAudioDelay					NTV2DeviceCanDoAudioDelay						///< @deprecated	Use NTV2DeviceCanDoAudioDelay instead.
	#define NTV2BoardCanDoAudioN						NTV2DeviceCanDoAudioN							///< @deprecated	Use NTV2DeviceCanDoAudioN instead.
	#define NTV2BoardCanDoBreakoutBox					NTV2DeviceCanDoBreakoutBox						///< @deprecated	Use NTV2DeviceCanDoBreakoutBox instead.
	#define NTV2BoardCanDoCapture						NTV2DeviceCanDoCapture							///< @deprecated	Use NTV2DeviceCanDoCapture instead.
	#define NTV2BoardCanDoColorCorrection				NTV2DeviceCanDoColorCorrection					///< @deprecated	Use NTV2DeviceCanDoColorCorrection instead.
	#define NTV2BoardCanDoConversionMode				NTV2DeviceCanDoConversionMode					///< @deprecated	Use NTV2DeviceCanDoConversionMode instead.
	#define NTV2BoardCanDoDSKMode						NTV2DeviceCanDoDSKMode							///< @deprecated	Use NTV2DeviceCanDoDSKMode instead.
	#define NTV2BoardCanDoDSKOpacity					NTV2DeviceCanDoDSKOpacity						///< @deprecated	Use NTV2DeviceCanDoDSKOpacity instead.
	#define NTV2BoardCanDoDualLink						NTV2DeviceCanDoDualLink							///< @deprecated	Use NTV2DeviceCanDoDualLink instead.
	#define NTV2BoardCanDoDVCProHD						NTV2DeviceCanDoDVCProHD							///< @deprecated	Use NTV2DeviceCanDoDVCProHD instead.
	#define NTV2BoardCanDoFormat						NTV2DeviceCanDoFormat							///< @deprecated	Use NTV2DeviceCanDoVideoFormat instead.
	#define NTV2BoardCanDoFrameBufferFormat				NTV2DeviceCanDoFrameBufferFormat				///< @deprecated	Use NTV2DeviceCanDoFrameBufferFormat instead.
	#define NTV2BoardCanDoFreezeOutput					NTV2DeviceCanDoFreezeOutput						///< @deprecated	Use NTV2DeviceCanDoFreezeOutput instead.
	#define NTV2BoardCanDoHDMIOutStereo					NTV2DeviceCanDoHDMIOutStereo					///< @deprecated	Use NTV2DeviceCanDoHDMIOutStereo instead.
	#define NTV2BoardCanDoHDV							NTV2DeviceCanDoHDV								///< @deprecated	Use NTV2DeviceCanDoHDV instead.
	#define NTV2BoardCanDoHDVideo						NTV2DeviceCanDoHDVideo							///< @deprecated	Use NTV2DeviceCanDoHDVideo instead.
	#define NTV2BoardCanDoInputSource					NTV2DeviceCanDoInputSource						///< @deprecated	Use NTV2DeviceCanDoInputSource instead.
	#define NTV2BoardCanDoIsoConvert					NTV2DeviceCanDoIsoConvert						///< @deprecated	Use NTV2DeviceCanDoIsoConvert instead.
	#define NTV2BoardCanDoLTC							NTV2DeviceCanDoLTC								///< @deprecated	Use NTV2DeviceCanDoLTC instead.
	#define NTV2BoardCanDoLTCEmbeddedN					NTV2DeviceCanDoLTCEmbeddedN						///< @deprecated	Use NTV2DeviceCanDoLTCEmbeddedN instead.
	#define NTV2BoardCanDoLTCInN						NTV2DeviceCanDoLTCInN							///< @deprecated	Use NTV2DeviceCanDoLTCInN instead.
	#define NTV2BoardCanDoLTCInOnRefPort				NTV2DeviceCanDoLTCInOnRefPort					///< @deprecated	Use NTV2DeviceCanDoLTCInOnRefPort instead.
	#define NTV2BoardCanDoLTCOutN						NTV2DeviceCanDoLTCOutN							///< @deprecated	Use NTV2DeviceCanDoLTCOutN instead.
	#define NTV2BoardCanDoMSI							NTV2DeviceCanDoMSI								///< @deprecated	Use NTV2DeviceCanDoMSI instead.
	#define NTV2BoardCanDoMultiFormat					NTV2DeviceCanDoMultiFormat						///< @deprecated	Use NTV2DeviceCanDoMultiFormat instead.
	#define NTV2BoardCanDoPCMControl					NTV2DeviceCanDoPCMControl						///< @deprecated	Use NTV2DeviceCanDoPCMControl instead.
	#define NTV2BoardCanDoPIO							NTV2DeviceCanDoPIO								///< @deprecated	Use NTV2DeviceCanDoPIO instead.
	#define NTV2BoardCanDoPlayback						NTV2DeviceCanDoPlayback							///< @deprecated	Use NTV2DeviceCanDoPlayback instead.
	#define NTV2BoardCanDoProAudio						NTV2DeviceCanDoProAudio							///< @deprecated	Use NTV2DeviceCanDoProAudio instead.
	#define NTV2BoardCanDoProgrammableCSC				NTV2DeviceCanDoProgrammableCSC					///< @deprecated	Use NTV2DeviceCanDoProgrammableCSC instead.
	#define NTV2BoardCanDoProgrammableRS422				NTV2DeviceCanDoProgrammableRS422				///< @deprecated	Use NTV2DeviceCanDoProgrammableRS422 instead.
	#define NTV2BoardCanDoProRes						NTV2DeviceCanDoProRes							///< @deprecated	Use NTV2DeviceCanDoProRes instead.
	#define NTV2BoardCanDoQREZ							NTV2DeviceCanDoQREZ								///< @deprecated	Use NTV2DeviceCanDoQREZ instead.
	#define NTV2BoardCanDoQuarterExpand					NTV2DeviceCanDoQuarterExpand					///< @deprecated	Use NTV2DeviceCanDoQuarterExpand instead.
	#define NTV2BoardCanDoRateConvert					NTV2DeviceCanDoRateConvert						///< @deprecated	Use NTV2DeviceCanDoRateConvert instead.
	#define NTV2BoardCanDoRGBPlusAlphaOut				NTV2DeviceCanDoRGBPlusAlphaOut					///< @deprecated	Use NTV2DeviceCanDoRGBPlusAlphaOut instead.
	#define NTV2BoardCanDoRP188							NTV2DeviceCanDoRP188							///< @deprecated	Use NTV2DeviceCanDoRP188 instead.
	#define NTV2BoardCanDoRS422N						NTV2DeviceCanDoRS422N							///< @deprecated	Use NTV2DeviceCanDoRS422N instead.
	#define NTV2BoardCanDoSDVideo						NTV2DeviceCanDoSDVideo							///< @deprecated	Use NTV2DeviceCanDoSDVideo instead.
	#define NTV2BoardCanDoStackedAudio					NTV2DeviceCanDoStackedAudio						///< @deprecated	Use NTV2DeviceCanDoStackedAudio instead.
	#define NTV2BoardCanDoStereoIn						NTV2DeviceCanDoStereoIn							///< @deprecated	Use NTV2DeviceCanDoStereoIn instead.
	#define NTV2BoardCanDoStereoOut						NTV2DeviceCanDoStereoOut						///< @deprecated	Use NTV2DeviceCanDoStereoOut instead.
	#define NTV2BoardCanDoThunderbolt					NTV2DeviceCanDoThunderbolt						///< @deprecated	Use NTV2DeviceCanDoThunderbolt instead.
	#define NTV2BoardCanDoVideoFormat					NTV2DeviceCanDoVideoFormat						///< @deprecated	Use NTV2DeviceCanDoVideoFormat instead.
	#define NTV2BoardCanDoVideoProcessing				NTV2DeviceCanDoVideoProcessing					///< @deprecated	Use NTV2DeviceCanDoVideoProcessing instead.
	#define NTV2BoardCanDoWidget						NTV2DeviceCanDoWidget							///< @deprecated	Use NTV2DeviceCanDoWidget instead.
	#define NTV2BoardGetActiveMemorySize				NTV2DeviceGetActiveMemorySize					///< @deprecated	Use NTV2DeviceGetActiveMemorySize instead.
	#define NTV2BoardGetAudioFrameBuffer				NTV2DeviceGetAudioFrameBuffer					///< @deprecated	Use NTV2DeviceGetAudioFrameBuffer instead.
	#define NTV2BoardGetAudioFrameBuffer2				NTV2DeviceGetAudioFrameBuffer2					///< @deprecated	Use NTV2DeviceGetAudioFrameBuffer2 instead.
	#define NTV2BoardGetDownConverterDelay				NTV2DeviceGetDownConverterDelay					///< @deprecated	Use NTV2DeviceGetDownConverterDelay instead.
	#define NTV2BoardGetFrameBufferSize					NTV2DeviceGetFrameBufferSize					///< @deprecated	Use NTV2DeviceGetFrameBufferSize instead.
	#define NTV2BoardGetFrameBufferSize_Ex				NTV2DeviceGetFrameBufferSize_Ex					///< @deprecated	Use NTV2DeviceGetFrameBufferSize_Ex instead.
	#define NTV2BoardGetHDMIVersion						NTV2DeviceGetHDMIVersion						///< @deprecated	Use NTV2DeviceGetHDMIVersion instead.
	#define NTV2BoardGetLUTVersion						NTV2DeviceGetLUTVersion							///< @deprecated	Use NTV2DeviceGetLUTVersion instead.
	#define NTV2BoardGetMaxAudioChannels				NTV2DeviceGetMaxAudioChannels					///< @deprecated	Use NTV2DeviceGetMaxAudioChannels instead.
	#define NTV2BoardGetMaxTransferCount				NTV2DeviceGetMaxTransferCount					///< @deprecated	Use NTV2DeviceGetMaxTransferCount instead.
	#define NTV2BoardGetNum4kQuarterSizeConverters		NTV2DeviceGetNum4kQuarterSizeConverters			///< @deprecated	Use NTV2DeviceGetNum4kQuarterSizeConverters instead.
	#define NTV2BoardGetNumAESAudioInputChannels		NTV2DeviceGetNumAESAudioInputChannels			///< @deprecated	Use NTV2DeviceGetNumAESAudioInputChannels instead.
	#define NTV2BoardGetNumAESAudioOutputChannels		NTV2DeviceGetNumAESAudioOutputChannels			///< @deprecated	Use NTV2DeviceGetNumAESAudioOutputChannels instead.
	#define NTV2BoardGetNumAnalogAudioInputChannels		NTV2DeviceGetNumAnalogAudioInputChannels		///< @deprecated	Use NTV2DeviceGetNumAnalogAudioInputChannels instead.
	#define NTV2BoardGetNumAnalogAudioOutputChannels	NTV2DeviceGetNumAnalogAudioOutputChannels		///< @deprecated	Use NTV2DeviceGetNumAnalogAudioOutputChannels instead.
	#define NTV2BoardGetNumAnlgVideoInputs				NTV2DeviceGetNumAnalogVideoInputs				///< @deprecated	Use NTV2DeviceGetNumAnalogVideoInputs instead.
	#define NTV2BoardGetNumAnlgVideoOutputs				NTV2DeviceGetNumAnalogVideoOutputs				///< @deprecated	Use NTV2DeviceGetNumAnalogVideoOutputs instead.
	#define NTV2BoardGetNumAudioStreams					NTV2DeviceGetNumAudioStreams					///< @deprecated	Use NTV2DeviceGetNumAudioStreams instead.
	#define NTV2BoardGetNumberFrameBuffers				NTV2DeviceGetNumberFrameBuffers					///< @deprecated	Use NTV2DeviceGetNumberFrameBuffers instead.
	#define NTV2DeviceGetNumberRegisters				NTV2DeviceGetMaxRegisterNumber					///< @deprecated	Use NTV2DeviceGetMaxRegisterNumber instead.
	#define NTV2BoardGetNumberRegisters					NTV2DeviceGetMaxRegisterNumber					///< @deprecated	Use NTV2DeviceGetMaxRegisterNumber instead.
	#define NTV2BoardGetNumCrossConverters				NTV2DeviceGetNumCrossConverters					///< @deprecated	Use NTV2DeviceGetNumCrossConverters instead.
	#define NTV2BoardGetNumCSCs							NTV2DeviceGetNumCSCs							///< @deprecated	Use NTV2DeviceGetNumCSCs instead.
	#define NTV2BoardGetNumDownConverters				NTV2DeviceGetNumDownConverters					///< @deprecated	Use NTV2DeviceGetNumDownConverters instead.
	#define NTV2BoardGetNumEmbeddedAudioInputChannels	NTV2DeviceGetNumEmbeddedAudioInputChannels		///< @deprecated	Use NTV2DeviceGetNumEmbeddedAudioInputChannels instead.
	#define NTV2BoardGetNumEmbeddedAudioOutputChannels	NTV2DeviceGetNumEmbeddedAudioOutputChannels		///< @deprecated	Use NTV2DeviceGetNumEmbeddedAudioOutputChannels instead.
	#define NTV2BoardGetNumFrameStores					NTV2DeviceGetNumFrameStores						///< @deprecated	Use NTV2DeviceGetNumFrameStores instead.
	#define NTV2BoardGetNumFrameSyncs					NTV2DeviceGetNumFrameSyncs						///< @deprecated	Use NTV2DeviceGetNumFrameSyncs instead.
	#define NTV2BoardGetNumHDMIAudioInputChannels		NTV2DeviceGetNumHDMIAudioInputChannels			///< @deprecated	Use NTV2DeviceGetNumHDMIAudioInputChannels instead.
	#define NTV2BoardGetNumHDMIAudioOutputChannels		NTV2DeviceGetNumHDMIAudioOutputChannels			///< @deprecated	Use NTV2DeviceGetNumHDMIAudioOutputChannels instead.
	#define NTV2BoardGetNumHDMIVideoInputs				NTV2DeviceGetNumHDMIVideoInputs					///< @deprecated	Use NTV2DeviceGetNumHDMIVideoInputs instead.
	#define NTV2BoardGetNumHDMIVideoOutputs				NTV2DeviceGetNumHDMIVideoOutputs				///< @deprecated	Use NTV2DeviceGetNumHDMIVideoOutputs instead.
	#define NTV2BoardGetNumInputConverters				NTV2DeviceGetNumInputConverters					///< @deprecated	Use NTV2DeviceGetNumInputConverters instead.
	#define NTV2BoardGetNumLUTs							NTV2DeviceGetNumLUTs							///< @deprecated	Use NTV2DeviceGetNumLUTs instead.
	#define NTV2BoardGetNumMixers						NTV2DeviceGetNumMixers							///< @deprecated	Use NTV2DeviceGetNumMixers instead.
	#define NTV2BoardGetNumOutputConverters				NTV2DeviceGetNumOutputConverters				///< @deprecated	Use NTV2DeviceGetNumOutputConverters instead.
	#define NTV2BoardGetNumReferenceVideoInputs			NTV2DeviceGetNumReferenceVideoInputs			///< @deprecated	Use NTV2DeviceGetNumReferenceVideoInputs instead.
	#define NTV2BoardGetNumSerialPorts					NTV2DeviceGetNumSerialPorts						///< @deprecated	Use NTV2DeviceGetNumSerialPorts instead.
	#define NTV2BoardGetNumUpConverters					NTV2DeviceGetNumUpConverters					///< @deprecated	Use NTV2DeviceGetNumUpConverters instead.
	#define NTV2BoardGetNumVideoChannels				NTV2DeviceGetNumVideoChannels					///< @deprecated	Use NTV2DeviceGetNumVideoChannels instead.
	#define NTV2BoardGetNumVideoInputs					NTV2DeviceGetNumVideoInputs						///< @deprecated	Use NTV2DeviceGetNumVideoInputs instead.
	#define NTV2BoardGetNumVideoOutputs					NTV2DeviceGetNumVideoOutputs					///< @deprecated	Use NTV2DeviceGetNumVideoOutputs instead.
	#define NTV2BoardGetPingLED							NTV2DeviceGetPingLED							///< @deprecated	Use NTV2DeviceGetPingLED instead.
	#define NTV2BoardGetUFCVersion						NTV2DeviceGetUFCVersion							///< @deprecated	Use NTV2DeviceGetUFCVersion instead.
	#define NTV2BoardGetVideoFormatFromState			NTV2DeviceGetVideoFormatFromState				///< @deprecated	Use NTV2DeviceGetVideoFormatFromState instead.
	#define NTV2BoardGetVideoFormatFromState_Ex			NTV2DeviceGetVideoFormatFromState_Ex			///< @deprecated	Use NTV2DeviceGetVideoFormatFromState_Ex instead.
	#define NTV2BoardHasBiDirectionalSDI				NTV2DeviceHasBiDirectionalSDI					///< @deprecated	Use NTV2DeviceHasBiDirectionalSDI instead.
	#define NTV2BoardHasColorSpaceConverterOnChannel2	NTV2DeviceHasColorSpaceConverterOnChannel2		///< @deprecated	Use NTV2DeviceHasColorSpaceConverterOnChannel2 instead.
	#define NTV2BoardHasNWL								NTV2DeviceHasNWL								///< @deprecated	Use NTV2DeviceHasNWL instead.
	#define NTV2BoardHasPCIeGen2						NTV2DeviceHasPCIeGen2							///< @deprecated	Use NTV2DeviceHasPCIeGen2 instead.
	#define NTV2BoardHasSDIRelays						NTV2DeviceHasSDIRelays							///< @deprecated	Use NTV2DeviceHasSDIRelays instead.
	#define NTV2BoardHasSPIFlash						NTV2DeviceHasSPIFlash							///< @deprecated	Use NTV2DeviceHasSPIFlash instead.
	#define NTV2BoardHasSPIFlashSerial					NTV2DeviceHasSPIFlashSerial						///< @deprecated	Use NTV2DeviceHasSPIFlashSerial instead.
	#define NTV2BoardHasSPIv2							NTV2DeviceHasSPIv2								///< @deprecated	Use NTV2DeviceHasSPIv2 instead.
	#define NTV2BoardHasSPIv3							NTV2DeviceHasSPIv3								///< @deprecated	Use NTV2DeviceHasSPIv3 instead.
	#define NTV2BoardIs64Bit							NTV2DeviceIs64Bit								///< @deprecated	Use NTV2DeviceIs64Bit instead.
	#define NTV2BoardIsDirectAddressable				NTV2DeviceIsDirectAddressable					///< @deprecated	Use NTV2DeviceIsDirectAddressable instead.
	#define NTV2BoardIsExternalToHost					NTV2DeviceIsExternalToHost						///< @deprecated	Use NTV2DeviceIsExternalToHost instead.
	#define NTV2BoardNeedsRoutingSetup					NTV2DeviceNeedsRoutingSetup						///< @deprecated	Use NTV2DeviceNeedsRoutingSetup instead.
	#define NTV2BoardSoftwareCanChangeFrameBufferSize	NTV2DeviceSoftwareCanChangeFrameBufferSize		///< @deprecated	Use NTV2DeviceSoftwareCanChangeFrameBufferSize instead.
#endif	//	!defined (NTV2_DEPRECATE)

#if defined(__cplusplus) && defined(NTV2_BUILDING_DRIVER)
}
#endif

#endif	//	NTV2DEVICEFEATURES_H
