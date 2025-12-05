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
	@brief	Used with CNTV2DriverInterface::GetBoolParam to determine device capabilities.
	@see	vidop-features
	@note   Always add new entries at the bottom just before kNTV2BoolParam_LAST.
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
	kDeviceCanDoAESAudioIn,						///< @brief True if device has any AES audio inputs or outputs
	kDeviceCanDoAnalogAudio,					///< @brief True if device has any analog inputs or outputs
	kDeviceCanDoAnalogVideoIn,					///< @brief True if device has one or more analog video inputs
	kDeviceCanDoAnalogVideoOut,					///< @brief True if device has one or more analog video outputs
	kDeviceCanDoAudio2Channels,					///< @brief True if audio system(s) support 2 or more audio channels
	kDeviceCanDoAudio6Channels,					///< @brief True if audio system(s) support 6 or more audio channels
	kDeviceCanDoAudio8Channels,					///< @brief True if audio system(s) support 8 or more audio channels
	kDeviceCanDoAudio96K,						///< @brief True if Audio System(s) support a 96kHz sample rate.
	kDeviceCanDoAudioDelay,						///< @brief True if Audio System(s) support an adjustable delay.
	kDeviceCanDoBreakoutBox,					///< @brief True if device supports an AJA breakout box.
	kDeviceCanDoCapture,						///< @brief True if device has any SDI, HDMI or analog video inputs
	kDeviceCanDoColorCorrection,				///< @brief True if device has any LUTs
	kDeviceCanDoCustomAnc,						///< @brief True if device has SDI ANC inserter/extractor firmware.
	kDeviceCanDoDSKOpacity,						///< @brief True if device mixer/keyer supports adjustable opacity.
	kDeviceCanDoDualLink,						///< @brief True if device supports 10-bit RGB input/output over 2-wire SDI.
	kDeviceCanDoDVCProHD,						///< @brief True if device can squeeze/stretch between 1920x1080/1280x1080 and 1280x720/960x720.
	kDeviceCanDoEnhancedCSC,					///< @brief True if device has enhanced CSCs.
	kDeviceCanDoFrameStore1Display,				///< @brief True if device can display/output video from FrameStore 1.
	kDeviceCanDoFreezeOutput,					///< @brief True if device can freeze output video.
	kDeviceCanDoGPIO,							///< @brief	True if device has GPIO interface.	(New in SDK 18.0)
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
	kDeviceCanDoPlayback,						///< @brief True if device has any SDI, HDMI or analog video outputs.
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
	kDeviceHasBracketLED,						///< @brief True if device has LED(s) on the card bracket. (New in SDK 18.0)
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
	kDeviceIsLocalPhysical,						///< @brief True if device is local-host-attached, and not remote, software or virtual (new in SDK 17.5)
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
	kDeviceCanDoHDMIAuxCapture,					///< @brief True if device has HDMI AUX data extractor(s).
	kDeviceCanDoHDMIAuxPlayback,				///< @brief True if device has HDMI AUX data inserter(s).
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
	kDeviceHasGenlockv2,						///< @brief True if device has version 2 genlock hardware and/or firmware. (Deprecate -- use kDeviceGetGenlockVersion)
	kDeviceHasGenlockv3,						///< @brief True if device has version 3 genlock hardware and/or firmware. (Deprecate -- use kDeviceGetGenlockVersion)
	kDeviceHasHeadphoneJack,					///< @brief True if device has a headphone jack.
	kDeviceHasLEDAudioMeters,					///< @brief True if device has LED audio meters.
	kDeviceHasRotaryEncoder,					///< @brief True if device has a rotary encoder volume control.
	kDeviceHasSPIv5,							///< @brief Use kDeviceGetSPIVersion instead.
	kDeviceHasXilinxDMA,						///< @brief True if device has Xilinx DMA hardware.
	kDeviceHasMicrophoneInput,					///< @brief True if device has a microphone input connector.
	kDeviceCanDoBreakoutBoard,					///< @brief True if device supports an AJA breakout board. (New in SDK 17.0)
	kDeviceHasBreakoutBoard,					///< @brief True if device has attached breakout board. (New in SDK 17.0)
	kDeviceAudioCanWaitForVBI,					///< @brief True if device audio systems can wait for VBI before starting. (New in SDK 17.0)
	kDeviceHasNTV4FrameStores,					///< @brief True if device has NTV4 FrameStores. (New in SDK 17.0)
	kDeviceHasXptConnectROM,					///< @brief True if device has a crosspoint connection ROM (New in SDK 17.0)
	kDeviceCanDoAudioInput,						///< @brief True if device has any audio input capability (SDI, HDMI or analog) (New in SDK 17.1)
	kDeviceCanDoAudioOutput,					///< @brief True if device has any audio output capability (SDI, HDMI or analog) (New in SDK 17.1)
	kDeviceCanDoAESAudioOut,					///< @brief	True if device has any AES audio output channels	(New in SDK 17.1)
	kDeviceHasIDSwitch,							///< @brief	True if device has a mechanical identification switch.	(New in SDK 17.1)
	kDeviceCanDoHDMIQuadRasterConversion,		///< @brief	True if HDMI in/out supports square-division (quad) raster conversion.	(New in SDK 17.1)
	kDeviceCanDoCustomHancInsertion,			///< @brief	True if device supports custom HANC packet insertion.	(New in SDK 17.1)
	kDeviceCanDoStreamingDMA,					///< @brief	True if device supports streaming DMA.	(New in SDK 17.1)
	kDeviceHasPWMFanControl,					///< @brief	True if device has a PWM-controlled cooling fan.	(New in SDK 17.1)
	kDeviceROMHasBankSelect,					///< @brief	True if device SPIFlash ROM is bank-selected.	(New in SDK 17.1)
	kDeviceCanDoVersalSysMon,					///< @brief	True if device supports Versal Adaptive SoC System Monitor.
	kNTV2BoolParam_LAST,
	kNTV2BoolParam_COUNT	= kNTV2BoolParam_LAST-kNTV2BoolParam_FIRST,
	kDeviceCanDo_INVALID	= kNTV2BoolParam_LAST
} NTV2BoolParamID;

#define NTV2_IS_VALID_BOOLPARAMID(__x__)		((__x__) >= kNTV2BoolParam_FIRST  &&  (__x__) < kNTV2BoolParam_LAST)

/**
	@brief	Used with CNTV2DriverInterface::GetNumericParam to determine device capabilities.
	@note   Always add new entries at the bottom just before kNTV2NumericParam_LAST.
	@see	vidop-features
**/
typedef enum _NTV2NumericParamID
{
	kNTV2NumericParam_FIRST		= 2000,
	kDeviceGetActiveMemorySize	= kNTV2NumericParam_FIRST,	///< @brief The size, in bytes, of the device's active RAM available for video and audio.
	kDeviceGetDACVersion,						///< @brief The version number of the DAC on the device.
	kDeviceGetDownConverterDelay,				///< @brief The down-converter delay on the device.
	kDeviceGetHDMIVersion,						///< @brief The version number of the HDMI chipset on the device.
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
	kDeviceGetNumLUTBanks,						///< @brief The number of LUT banks on the device. (New in SDK 17.0)
	kDeviceGetTotalNumAudioSystems,				///< @brief The total number of audio systems on the device, including host audio and mixer audio systems, if present. (New in SDK 17.0)
	kDeviceGetNumBufferedAudioSystems,			///< @brief The total number of audio systems on the device that can read/write audio buffer memory. Includes host audio system, if present. (New in SDK 17.0)
	kDeviceGetNumTSIMuxers,						///< @brief	The number of TSI muxers on the device. (New in SDK 17.0)
	kDeviceGetSPIFlashVersion,					///< @brief	The SPI-flash version on the device. (New in SDK 17.1)
	kDeviceGetGenlockVersion,					///< @brief	The version number of the device's genlock hardware/firmware. (New in SDK 17.6)
	kNTV2NumericParam_LAST,
	kNTV2NumericParam_COUNT	= kNTV2NumericParam_LAST-kNTV2NumericParam_FIRST,
	kDeviceGetNum_INVALID	= kNTV2NumericParam_LAST
} NTV2NumericParamID;

#define NTV2_IS_VALID_NUMPARAMID(__x__)		((__x__) >= kNTV2NumericParam_FIRST  &&  (__x__) < kNTV2NumericParam_LAST)

/**
	@brief	Identifies NTV2 enumerated types, used in CNTV2DriverInterface::GetSupportedItems.
	@note   Always add new entries at the bottom just before kNTV2EnumsID_LAST.
	@see	vidop-features
**/
typedef enum _NTV2EnumsID
{
	kNTV2EnumsID_FIRST,
	kNTV2EnumsID_DeviceID = kNTV2EnumsID_FIRST,	///< @brief Identifies the NTV2DeviceID enumerated type
	kNTV2EnumsID_Standard,						///< @brief Identifies the NTV2Standard enumerated type
	kNTV2EnumsID_PixelFormat,					///< @brief Identifies the NTV2PixelFormat enumerated type
	kNTV2EnumsID_FrameGeometry,					///< @brief Identifies the NTV2FrameGeometry enumerated type
	kNTV2EnumsID_FrameRate,						///< @brief Identifies the NTV2FrameRate enumerated type
	kNTV2EnumsID_ScanGeometry,					///< @brief Identifies the NTV2ScanGeometry enumerated type
	kNTV2EnumsID_VideoFormat,					///< @brief Identifies the NTV2VideoFormat enumerated type
	kNTV2EnumsID_Mode,							///< @brief Identifies the NTV2Mode enumerated type
	kNTV2EnumsID_InputSource,					///< @brief Identifies the NTV2InputSource enumerated type
	kNTV2EnumsID_OutputDest,					///< @brief Identifies the NTV2OutputDest enumerated type
	kNTV2EnumsID_Channel,						///< @brief Identifies the NTV2Channel enumerated type
	kNTV2EnumsID_RefSource,						///< @brief Identifies the NTV2RefSource enumerated type
	kNTV2EnumsID_AudioRate,						///< @brief Identifies the NTV2AudioRate enumerated type
	kNTV2EnumsID_AudioSource,					///< @brief Identifies the NTV2AudioSource enumerated type
	kNTV2EnumsID_WidgetID,						///< @brief Identifies the NTV2AudioWidgetID enumerated type
	kNTV2EnumsID_ConversionMode,				///< @brief Identifies the NTV2ConversionMode enumerated type
	kNTV2EnumsID_DSKMode,						///< @brief	Identifies the NTV2DSKMode enumerated type
	kNTV2EnumsID_InputTCIndex,					///< @brief	Identifies the NTV2TCIndex enumerated type for input
	kNTV2EnumsID_OutputTCIndex,					///< @brief	Identifies the NTV2TCIndex enumerated type for output
	kNTV2EnumsID_LAST,
	kNTV2EnumsID_COUNT		= kNTV2EnumsID_LAST-kNTV2EnumsID_FIRST,
	kNTV2EnumsID_INVALID	= kNTV2EnumsID_LAST,
} NTV2EnumsID;

#define NTV2_IS_VALID_ENUMSID(__x__)		((__x__) >= kNTV2EnumsID_FIRST  &&  (__x__) < kNTV2EnumsID_LAST)


//	Most of the device features functions are generated from a Python script.
//	The script writes the implementations into 'ntv2devicefeatures.hpp', and the declarations into 'ntv2devicefeatures.hh'...
#include "ntv2devicefeatures.hh"

#if defined(__cplusplus) && defined(NTV2_BUILDING_DRIVER)
extern "C"
{
#endif

#if !defined(NTV2_DEPRECATE_17_2)
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudioOut(const NTV2DeviceID id));		///< @deprecated	Starting in SDK 17.1. Call DeviceCapabilities::CanDoAudioOutput instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudioIn(const NTV2DeviceID id));		///< @deprecated	Starting in SDK 17.1. Call DeviceCapabilities::CanDoAudioInput instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDo292Out(NTV2DeviceID id, UWord ndx));	///< @deprecated	Starting in SDK 17.1. Call DeviceCapabilities::CanDoWidget(NTV2WidgetType_SDIOut, ndx) instead
	AJAExport NTV2_SHOULD_DEPRECATE(bool NTV2DeviceCanDo3GOut (NTV2DeviceID id, UWord ndx));	///< @deprecated	Starting in SDK 17.1. Call DeviceCapabilities::CanDoWidget(NTV2WidgetType_SDIOut3G, ndx) instead
	AJAExport NTV2_SHOULD_DEPRECATE(bool NTV2DeviceCanDo12GOut(NTV2DeviceID id, UWord ndx));	///< @deprecated	Starting in SDK 17.1. Call DeviceCapabilities::CanDoWidget(NTV2WidgetType_SDIOut12G, ndx) instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDo292In(NTV2DeviceID id, UWord ndx));		///< @deprecated	Starting in SDK 17.1. Call DeviceCapabilities::CanDoWidget(NTV2WidgetType_SDIIn, ndx) instead
	AJAExport NTV2_SHOULD_DEPRECATE(bool NTV2DeviceCanDo3GIn(NTV2DeviceID id, UWord ndx));	///< @deprecated	Starting in SDK 17.1. Call DeviceCapabilities::CanDoWidget(NTV2WidgetType_SDIIn3G, ndx) instead
	AJAExport NTV2_SHOULD_DEPRECATE(bool NTV2DeviceCanDo12GIn(NTV2DeviceID id, UWord ndx));	///< @deprecated	Starting in SDK 17.1. Call DeviceCapabilities::CanDoWidget(NTV2WidgetType_SDIIn12G, ndx) instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoLTCEmbeddedN (NTV2DeviceID id, UWord ndx));		///< @deprecated	Starting in SDK 17.1
	AJAExport NTV2_SHOULD_DEPRECATE(bool NTV2DeviceCanDoOutputDestination (const NTV2DeviceID id, const NTV2OutputDest od));	///< @deprecated	Starting in SDK 17.1
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoColorCorrection (const NTV2DeviceID id));///< @deprecated	Starting in SDK 17.1
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoProgrammableCSC (const NTV2DeviceID id));///< @deprecated	Starting in SDK 17.1
	AJAExport NTV2_SHOULD_DEPRECATE(UWord Get8MBFrameSizeFactor (const NTV2FrameGeometry inFG, const NTV2FrameBufferFormat inFBF));	///< @deprecated	Starting in SDK 17.1

	#if (defined(__CPLUSPLUS__) || defined(__cplusplus)) && !defined(NTV2_BUILDING_DRIVER)
		AJAExport NTV2_SHOULD_DEPRECATE(ULWord NTV2DeviceGetFrameBufferSize(NTV2DeviceID id));		///< @deprecated	Starting in SDK 17.1
		AJAExport NTV2_SHOULD_DEPRECATE(ULWord NTV2DeviceGetNumberFrameBuffers(NTV2DeviceID id));	///< @deprecated	Starting in SDK 17.1
		AJAExport NTV2_SHOULD_DEPRECATE(ULWord NTV2DeviceGetAudioFrameBuffer(NTV2DeviceID id));		///< @deprecated	Starting in SDK 17.1
		AJAExport NTV2_SHOULD_DEPRECATE(ULWord NTV2DeviceGetAudioFrameBuffer2(NTV2DeviceID id));	///< @deprecated	Starting in SDK 17.1
	#else
		AJAExport NTV2_DEPRECATED_f(ULWord NTV2DeviceGetFrameBufferSize_Ex(NTV2DeviceID id));	///< @deprecated	Starting in SDK 17.1
		AJAExport NTV2_DEPRECATED_f(ULWord NTV2DeviceGetNumberFrameBuffers_Ex(NTV2DeviceID id));///< @deprecated	Starting in SDK 17.1
		AJAExport NTV2_DEPRECATED_f(ULWord NTV2DeviceGetAudioFrameBuffer_Ex(NTV2DeviceID id));	///< @deprecated	Starting in SDK 17.1
		AJAExport NTV2_DEPRECATED_f(ULWord NTV2DeviceGetAudioFrameBuffer2_Ex(NTV2DeviceID id));	///< @deprecated	Starting in SDK 17.1
	#endif
	AJAExport NTV2_SHOULD_DEPRECATE(ULWord NTV2DeviceGetFrameBufferSize(NTV2DeviceID id, NTV2FrameGeometry fg, NTV2FrameBufferFormat fbf));		///< @deprecated	Starting in SDK 17.1
	AJAExport NTV2_SHOULD_DEPRECATE(ULWord NTV2DeviceGetNumberFrameBuffers(NTV2DeviceID id, NTV2FrameGeometry fg, NTV2FrameBufferFormat fbf));	///< @deprecated	Starting in SDK 17.1
#endif	//	defined(NTV2_DEPRECATE_17_2)
	
AJAExport ULWord NTV2DeviceGetNumberVideoFrameBuffers(NTV2DeviceID inDeviceID, NTV2FrameGeometry inFrameGeometry, NTV2Framesize inFramesize);
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
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasSPIv2(const NTV2DeviceID inDeviceID);) ///< @deprecated	Use CNTV2Card::features().GetSPIFlashVersion() or NTV2DeviceGetSPIFlashVersion instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasSPIv3(const NTV2DeviceID inDeviceID);) ///< @deprecated	Use CNTV2Card::features().GetSPIFlashVersion() or NTV2DeviceGetSPIFlashVersion instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasSPIv4(const NTV2DeviceID inDeviceID);) ///< @deprecated	Use CNTV2Card::features().GetSPIFlashVersion() or NTV2DeviceGetSPIFlashVersion instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasSPIv5(const NTV2DeviceID inDeviceID);) ///< @deprecated	Use CNTV2Card::features().GetSPIFlashVersion() or NTV2DeviceGetSPIFlashVersion instead

	//	In SDK 17.0, these were replaced by NTV2DeviceGetGenlockVersion...
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasGenlockv2(const NTV2DeviceID devID);)	///< @deprecated	Use CNTV2Card::features().GetGenlockVersion() or NTV2DeviceGetGenlockVersion instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasGenlockv3(const NTV2DeviceID devID);)	///< @deprecated	Use CNTV2Card::features().GetGenlockVersion() or NTV2DeviceGetGenlockVersion instead

	//	In SDK 17.0, this was replaced by NTV2DeviceCanDoWidget(NTV2_WgtCSC2)...
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceHasColorSpaceConverterOnChannel2(const NTV2DeviceID devID);)	///< @deprecated	Call CNTV2Card::features().CanDoWidget(NTV2_WgtCSC2) or NTV2DeviceCanDoWidget instead

	//	In SDK 17.0, these were replaced by NTV2GetMaxNumAudioChannels:
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudio2Channels(const NTV2DeviceID devID);)	///< @deprecated	Use CNTV2Card::features().GetMaxAudioChannels() or NTV2DeviceGetMaxAudioChannels instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudio6Channels(const NTV2DeviceID devID);)	///< @deprecated	Use CNTV2Card::features().GetMaxAudioChannels() or NTV2DeviceGetMaxAudioChannels instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudio8Channels(const NTV2DeviceID devID);)	///< @deprecated	Use CNTV2Card::features().GetMaxAudioChannels() or NTV2DeviceGetMaxAudioChannels instead

	//	These have been marked deprecated for some time. In SDK 17.0, it's official.
	AJAExport NTV2_DEPRECATED_f(UWord NTV2DeviceGetNumAudioStreams(const NTV2DeviceID devID);)	///< @deprecated	Use CNTV2Card::features().GetNumAudioSystems() or NTV2DeviceGetNumAudioSystems instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoAudioN(const NTV2DeviceID devID, UWord index0);)	///< @deprecated	Use CNTV2Card::features().GetNumAudioSystems() or NTV2DeviceGetNumAudioSystems instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoLTCOutN(const NTV2DeviceID devID, UWord index0);)	///< @deprecated	Use CNTV2Card::features().GetNumLTCOutputs() or NTV2DeviceGetNumLTCOutputs instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoLTCInN(const NTV2DeviceID devID, UWord index0);)	///< @deprecated	Use CNTV2Card::features().GetNumLTCInputs() or NTV2DeviceGetNumLTCInputs instead
	AJAExport NTV2_DEPRECATED_f(bool NTV2DeviceCanDoRS422N(const NTV2DeviceID devID, const NTV2Channel ch);)	///< @deprecated	Use CNTV2Card::features().GetNumSerialPorts() or NTV2DeviceGetNumSerialPorts instead
#endif	//	NTV2_DEPRECATE_17_0

bool work_around_erroneous_compiler_warning (void);	//	This declaration stops erroneous deprecation warnings for NTV2DeviceCanDoTCIndex (immediately below)
AJAExport bool NTV2DeviceCanDoTCIndex (const NTV2DeviceID inDeviceID, const NTV2TCIndex inTCIndex); ///< @return	True if the device having the given ID supports the specified NTV2TCIndex.
AJAExport bool NTV2DeviceCanDoInputTCIndex (const NTV2DeviceID inDeviceID, const NTV2TCIndex inTCIndex);	///< @return	True if the device having the given ID supports the specified NTV2TCIndex for input.
AJAExport bool NTV2DeviceCanDoOutputTCIndex (const NTV2DeviceID inDeviceID, const NTV2TCIndex inTCIndex);	///< @return	True if the device having the given ID supports the specified NTV2TCIndex for output.	(New in SDK 17.1)
AJAExport NTV2AudioSystem NTV2DeviceGetAudioMixerSystem(const NTV2DeviceID inDeviceID);	///< @return	The NTV2AudioSystem used by the audio mixer for the given device (or NTV2_AUDIOSYSTEM_INVALID if there is no mixer).
AJAExport NTV2AudioSystem NTV2DeviceGetHostAudioSystem(const NTV2DeviceID inDeviceID);	///< @return	The NTV2AudioSystem used for host audio support for the given device (or NTV2_AUDIOSYSTEM_INVALID if there is no host audio system).
AJAExport bool NTV2DeviceROMHasBankSelect (const NTV2DeviceID inDeviceID);	///< @return	True if the device has SPI flash that incorporates bank selection.

#if defined(__cplusplus) && defined(NTV2_BUILDING_DRIVER)
}
#endif

#endif	//	NTV2DEVICEFEATURES_H
