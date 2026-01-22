/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2devicecapabilities.h
	@brief		Declares the DeviceCapabilities class.
	@copyright	2023-2024 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef NTV2_DEVICECAPABILITIES_H
#define NTV2_DEVICECAPABILITIES_H

#include "ntv2driverinterface.h"

/**
	@brief	Convenience class/API for inquiring about device capabilities of physical and virtual devices.
			Instead of calling the old global ::NTV2DeviceCanDoXXX(mDeviceID) functions,
			call mCard.features().CanDoXXX().
			Instead of calling ::NTV2DeviceGetNumYYY(mDeviceID), call mCard.features().GetNumYYY().
	@see	\ref vidop-features
**/
class AJAExport DeviceCapabilities
{
	public:
		explicit inline	DeviceCapabilities (CNTV2DriverInterface & inDev)	: dev(inDev)	{}
		inline			operator bool() const					{return dev.IsOpen();}															///< @returns	True if I'm valid
		inline bool		CanChangeEmbeddedAudioClock (void)		{return dev.IsSupported(kDeviceCanChangeEmbeddedAudioClock);}					///< @returns	True if SDI input audio system clock can optionally use reference
		inline bool		CanChangeFrameBufferSize (void)			{return dev.IsSupported(kDeviceCanChangeFrameBufferSize);}						///< @returns	True if frame buffer sizes are not fixed
		inline bool		CanDisableUFC (void)					{return dev.IsSupported(kDeviceCanDisableUFC);}									///< @returns	True if there's at least one UFC, and it can be disabled
		inline bool		CanDo12gRouting (void)					{return dev.IsSupported(kDeviceCanDo12gRouting);}								///< @returns	True if device supports 6G/12G routing crosspoints
		inline bool		CanDo12GSDI (void)						{return dev.IsSupported(kDeviceCanDo12GSDI);}									///< @returns	True if device has 6G/12G capable SDI connector(s)
		inline bool		CanDo2110 (void)						{return dev.IsSupported(kDeviceCanDo2110);}										///< @returns	True if device supports SMPTE ST2110
		inline bool		CanDo2KVideo (void)						{return dev.IsSupported(kDeviceCanDo2KVideo);}									///< @returns	True if device can handle 2Kx1556 (film format) video
		inline bool		CanDo3GLevelConversion (void)			{return dev.IsSupported(kDeviceCanDo3GLevelConversion);}						///< @returns	True if device can do 3G level B to 3G level A conversion
		inline bool		CanDo425Mux (void)						{return dev.IsSupported(kDeviceCanDo425Mux);}									///< @returns	True if device supports SMPTE 425 mux control
		inline bool		CanDo4KVideo (void)						{return dev.IsSupported(kDeviceCanDo4KVideo);}									///< @returns	True if device supports 4K/UHD video
		inline bool		CanDo8KVideo (void)						{return dev.IsSupported(kDeviceCanDo8KVideo);}									///< @returns	True if device supports 8K/UHD2 video formats
		inline bool		CanDoAESAudioIn (void)					{return dev.IsSupported(kDeviceCanDoAESAudioIn);}								///< @returns	True if device has any AES audio input(s)
		inline bool		CanDoAESAudioOut (void)					{return dev.IsSupported(kDeviceCanDoAESAudioOut);}								///< @returns	True if device has any AES audio output(s)
		inline bool		CanDoAnalogAudio (void)					{return dev.IsSupported(kDeviceCanDoAnalogAudio);}								///< @returns	True if device has any analog audio inputs or outputs
		inline bool		CanDoAnalogVideoIn (void)				{return dev.IsSupported(kDeviceCanDoAnalogVideoIn);}							///< @returns	True if device has any analog video inputs
		inline bool		CanDoAnalogVideoOut (void)				{return dev.IsSupported(kDeviceCanDoAnalogVideoOut);}							///< @returns	True if device has any analog video outputs
		inline bool		CanDoAudio192K (void)					{return dev.IsSupported(kDeviceCanDoAudio192K);}								///< @returns	True if device audio system(s) support a 192kHz sample rate
		inline bool		CanDoAudio2Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio2Channels);}							///< @returns	True if audio system(s) support 2 or more audio channels
		inline bool		CanDoAudio6Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio6Channels);}							///< @returns	True if audio system(s) support 6 or more audio channels
		inline bool		CanDoAudio8Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio8Channels);}							///< @returns	True if audio system(s) support 8 or more audio channels
		inline bool		CanDoAudio96K (void)					{return dev.IsSupported(kDeviceCanDoAudio96K);}									///< @returns	True if device audio system(s) support a 96kHz sample rate
		inline bool		CanDoAudioDelay (void)					{return dev.IsSupported(kDeviceCanDoAudioDelay);}								///< @returns	True if audio system(s) support an adjustable delay
		inline bool		CanDoAudioInput (void)					{return dev.IsSupported(kDeviceCanDoAudioInput);}								///< @returns	True if device has any audio input capability (SDI, HDMI, AES or analog)
		inline bool		CanDoAudioMixer (void)					{return dev.IsSupported(kDeviceCanDoAudioMixer);}								///< @returns	True if device has an audio mixer
		inline bool		CanDoAudioOutput (void)					{return dev.IsSupported(kDeviceCanDoAudioOutput);}								///< @returns	True if device has any audio output capability (SDI, HDMI, AES or analog)
		inline bool		CanDoAudioWaitForVBI (void)				{return dev.IsSupported(kDeviceAudioCanWaitForVBI);}							///< @returns	True if device audio systems support delaying start until VBI
		inline bool		CanDoBreakoutBoard (void)				{return dev.IsSupported(kDeviceCanDoBreakoutBoard);}							///< @returns	True if device supports an AJA breakout board
		inline bool		CanDoBreakoutBox (void)					{return dev.IsSupported(kDeviceCanDoBreakoutBox);}								///< @returns	True if device supports an AJA breakout box
		inline bool		CanDoCapture (void)						{return dev.IsSupported(kDeviceCanDoCapture);}									///< @returns	True if device has any SDI, HDMI or analog video inputs
		inline bool		CanDoColorCorrection (void)				{return dev.IsSupported(kDeviceCanDoColorCorrection);}							///< @returns	True if device has any LUTs
		inline bool		CanDoCustomAnc (void)					{return dev.IsSupported(kDeviceCanDoCustomAnc);}								///< @returns	True if device has SDI ANC inserter/extractor firmware
		inline bool		CanDoCustomHancInsertion (void)			{return dev.IsSupported(kDeviceCanDoCustomHancInsertion);}						///< @returns	True if device supports custom HANC packet insertion
		inline bool		CanDoDSKOpacity (void)					{return dev.IsSupported(kDeviceCanDoDSKOpacity);}								///< @returns	True if device mixer/keyer supports adjustable opacity
		inline bool		CanDoDualLink (void)					{return dev.IsSupported(kDeviceCanDoDualLink);}									///< @returns	True if device supports 10-bit RGB input/output over 2-wire SDI
		inline bool		CanDoDVCProHD (void)					{return dev.IsSupported(kDeviceCanDoDVCProHD);}									///< @returns	True if device can squeeze/stretch between 1920x1080/1280x1080 and 1280x720/960x720
		inline bool		CanDoEnhancedCSC (void)					{return dev.IsSupported(kDeviceCanDoEnhancedCSC);}								///< @returns	True if device has enhanced CSCs
		inline bool		CanDoFramePulseSelect (void)			{return dev.IsSupported(kDeviceCanDoFramePulseSelect);}							///< @returns	True if device supports frame pulse source independent of reference source
		inline bool		CanDoFrameStore1Display (void)			{return dev.IsSupported(kDeviceCanDoFrameStore1Display);}						///< @returns	True if device can display/output video from FrameStore 1
		inline bool		CanDoGPIO (void)						{return dev.IsSupported(kDeviceCanDoGPIO);}										///< @returns	True if device has GPIO interface
		inline bool		CanDoHDMIAuxCapture (void)				{return dev.IsSupported(kDeviceCanDoHDMIAuxCapture);}							///< @returns	True if device has HDMI AUX data extractor(s)
		inline bool		CanDoHDMIAuxPlayback (void)				{return dev.IsSupported(kDeviceCanDoHDMIAuxPlayback);}							///< @returns	True if device has HDMI AUX data inserter(s)
		inline bool		CanDoHDMIHDROut (void)					{return dev.IsSupported(kDeviceCanDoHDMIHDROut);}								///< @returns	True if device supports HDMI HDR output
		inline bool		CanDoHDMIMultiView (void)				{return dev.IsSupported(kDeviceCanDoHDMIMultiView);}							///< @returns	True if device can rasterize 4 HD signals into a single HDMI output
		inline bool		CanDoHDMIOutStereo (void)				{return dev.IsSupported(kDeviceCanDoHDMIOutStereo);}							///< @returns	True if device supports 3D/stereo HDMI video output
		inline bool		CanDoHDMIQuadRasterConversion (void)	{return dev.IsSupported(kDeviceCanDoHDMIQuadRasterConversion);}					///< @returns	True if HDMI in/out supports square-division (quad) raster conversion
		inline bool		CanDoHDV (void)							{return dev.IsSupported(kDeviceCanDoHDV);}										///< @returns	True if device can squeeze/stretch between 1920x1080 and 1440x1080
		inline bool		CanDoHDVideo (void)						{return dev.IsSupported(kDeviceCanDoHDVideo);}									///< @returns	True if device can handle HD (High Definition) video
		inline bool		CanDoHFRRGB (void)						{return dev.IsSupported(kDeviceCanDoHFRRGB);}									///< @returns	True if device supports 1080p RGB at more than 50fps
		inline bool		CanDoIP (void)							{return dev.IsSupported(kDeviceCanDoIP);}										///< @returns	True if device has SFP network connectors
		inline bool		CanDoIsoConvert (void)					{return dev.IsSupported(kDeviceCanDoIsoConvert);}								///< @returns	True if device can do ISO conversion
		inline bool		CanDoJ2K (void)							{return dev.IsSupported(kDeviceCanDoJ2K);}										///< @returns	True if device supports JPEG 2000 codec
		inline bool		CanDoLTC (void)							{return dev.IsSupported(kDeviceCanDoLTC);}										///< @returns	True if device has any LTC (Linear TimeCode) inputs
		inline bool		CanDoLTCInOnRefPort (void)				{return dev.IsSupported(kDeviceCanDoLTCInOnRefPort);}							///< @returns	True if device can read LTC (Linear TimeCode) from its reference input
		inline bool		CanDoMSI (void)							{return dev.IsSupported(kDeviceCanDoMSI);}										///< @returns	True if device DMA hardware supports MSI (Message Signaled Interrupts)
		inline bool		CanDoMultiFormat (void)					{return dev.IsSupported(kDeviceCanDoMultiFormat);}								///< @returns	True if device FrameStores can independently accommodate different video formats
		inline bool		CanDoMultiLinkAudio (void)				{return dev.IsSupported(kDeviceCanDoMultiLinkAudio);}							///< @returns	True if device supports grouped audio system control
		inline bool		CanDoPCMControl (void)					{return dev.IsSupported(kDeviceCanDoPCMControl);}								///< @returns	True if device can mark specific audio channel pairs as not carrying LPCM
		inline bool		CanDoPCMDetection (void)				{return dev.IsSupported(kDeviceCanDoPCMDetection);}								///< @returns	True if device can detect which audio channel pairs are not carrying LPCM
		inline bool		CanDoPIO (void)							{return dev.IsSupported(kDeviceCanDoPIO);}										///< @returns	True if device supports programmed I/O
		inline bool		CanDoPlayback (void)					{return dev.IsSupported(kDeviceCanDoPlayback);}									///< @returns	True if device has any SDI, HDMI or analog video outputs
		inline bool		CanDoProgrammableRS422 (void)			{return dev.IsSupported(kDeviceCanDoProgrammableRS422);}						///< @returns	True if device has at least one RS-422 serial port that can be programmed (for baud rate, parity, etc.)
		inline bool		CanDoProRes (void)						{return dev.IsSupported(kDeviceCanDoProRes);}									///< @returns	True if device FrameStore(s) will read/write Apple ProRes-compressed video
		inline bool		CanDoQREZ (void)						{return dev.IsSupported(kDeviceCanDoQREZ);}										///< @returns	True if device can handle QRez
		inline bool		CanDoQuarterExpand (void)				{return dev.IsSupported(kDeviceCanDoQuarterExpand);}							///< @returns	True if device FrameStores will pixel-halve/line-halve on input, pixel-double/line-double on output
		inline bool		CanDoRateConvert (void)					{return dev.IsSupported(kDeviceCanDoRateConvert);}								///< @returns	True if device can do frame rate conversion
		inline bool		CanDoRGBLevelAConversion (void)			{return dev.IsSupported(kDeviceCanDoRGBLevelAConversion);}						///< @returns	True if the device can do RGB over 3G Level A
		inline bool		CanDoRGBPlusAlphaOut (void)				{return dev.IsSupported(kDeviceCanDoRGBPlusAlphaOut);}							///< @returns	True if device has CSCs capable of splitting the key & fill from RGB frame buffers (unrelated to RGB SDI wire formats)
		inline bool		CanDoRP188 (void)						{return dev.IsSupported(kDeviceCanDoRP188);}									///< @returns	True if device can insert and/or extract SMPTE 12M (RP-188/VITC)
		inline bool		CanDoSDIErrorChecks (void)				{return dev.IsSupported(kDeviceCanDoSDIErrorChecks);}							///< @returns	True if device has SDI inputs and can report errors (CRC, TRS, etc.)
		inline bool		CanDoSDVideo (void)						{return dev.IsSupported(kDeviceCanDoSDVideo);}									///< @returns	True if device can handle standard definition video
		inline bool		CanDoStackedAudio (void)				{return dev.IsSupported(kDeviceCanDoStackedAudio);}								///< @returns	True if device uses a "stacked" arrangement of its audio buffers
		inline bool		CanDoStereoIn (void)					{return dev.IsSupported(kDeviceCanDoStereoIn);}									///< @returns	True if device supports 3D/Stereo video input over dual-stream SDI
		inline bool		CanDoStereoOut (void)					{return dev.IsSupported(kDeviceCanDoStereoOut);}								///< @returns	True if device supports 3D/Stereo video output over dual-stream SDI
		inline bool		CanDoStreamingDMA (void)				{return dev.IsSupported(kDeviceCanDoStreamingDMA);}								///< @returns	True if device supports streaming DMA
		inline bool		CanDoThunderbolt (void)					{return dev.IsSupported(kDeviceCanDoThunderbolt);}								///< @returns	True if device connects to the host via a Thunderbolt cable
		inline bool		CanDoVideoProcessing (void)				{return dev.IsSupported(kDeviceCanDoVideoProcessing);}							///< @returns	True if device can do video processing
		inline bool		CanDoVITC2 (void)						{return dev.IsSupported(kDeviceCanDoVITC2);}									///< @returns	True if device can insert or extract SMPTE 12M RP-188/VITC2
		inline bool		CanDoWarmBootFPGA (void)				{return dev.IsSupported(kDeviceCanDoWarmBootFPGA);}								///< @returns	True if device can warm-boot to load updated firmware
		inline bool		CanMeasureTemperature (void)			{return dev.IsSupported(kDeviceCanMeasureTemperature);}							///< @returns	True if device can measure its FPGA die temperature
		inline bool		CanReportFailSafeLoaded (void)			{return dev.IsSupported(kDeviceCanReportFailSafeLoaded);}						///< @returns	True if device can report if its "fail-safe" firmware is loaded/running
		inline bool		CanReportFrameSize (void)				{return dev.IsSupported(kDeviceCanReportFrameSize);}							///< @returns	True if device can report its intrinsic frame size
		inline bool		CanReportRunningFirmwareDate (void)		{return dev.IsSupported(kDeviceCanReportRunningFirmwareDate);}					///< @returns	True if device can report its running (and not necessarily installed) firmware date
		inline bool		CanThermostat (void)					{return dev.IsSupported(kDeviceCanThermostat);}									///< @returns	True if device fan can be thermostatically controlled
		inline bool		HasAudioMonitorRCAJacks (void)			{return dev.IsSupported(kDeviceHasAudioMonitorRCAJacks);}						///< @returns	True if device has a pair of unbalanced RCA audio monitor output connectors
		inline bool		HasBiDirectionalAnalogAudio (void)		{return dev.IsSupported(kDeviceHasBiDirectionalAnalogAudio);}					///< @returns	True if device has a bi-directional analog audio connector
		inline bool		HasBiDirectionalSDI (void)				{return dev.IsSupported(kDeviceHasBiDirectionalSDI);}							///< @returns	True if device has bi-directional SDI connectors
		inline bool		HasBracketLED (void)					{return dev.IsSupported(kDeviceHasBracketLED);}									///< @returns	True if device has LED(s) on the card bracket
		inline bool		HasBreakoutBoard (void)					{return dev.IsSupported(kDeviceHasBreakoutBoard);}								///< @returns	True if device has attached/connected breakout board
		inline bool		HasColorSpaceConverterOnChannel2 (void)	{return dev.IsSupported(kDeviceHasColorSpaceConverterOnChannel2);}				///< @returns	True if device has a second colorspace converter widget (NTV2_WgtCSC2)
		inline bool		HasCrosspointConnectROM (void)			{return dev.IsSupported(kDeviceHasXptConnectROM);}								///< @returns	True if device has a crosspoint connection ROM
		inline bool		HasGenlockv2 (void)						{return dev.IsSupported(kDeviceHasGenlockv2);}									///< @returns	True if device has version 2 genlock hardware and/or firmware
		inline bool		HasGenlockv3 (void)						{return dev.IsSupported(kDeviceHasGenlockv3);}									///< @returns	True if device has version 3 genlock hardware and/or firmware
		inline bool		HasHeadphoneJack (void)					{return dev.IsSupported(kDeviceHasHeadphoneJack);}								///< @returns	True if device has a headphone jack
		inline bool		HasHEVCM30 (void)						{return dev.IsSupported(kDeviceHasHEVCM30);}									///< @returns	True if device has an HEVC M30 encoder/decoder
		inline bool		HasHEVCM31 (void)						{return dev.IsSupported(kDeviceHasHEVCM31);}									///< @returns	True if device has an HEVC M31 encoder/decoder
		inline bool		HasIDSwitch (void)						{return dev.IsSupported(kDeviceHasIDSwitch);}									///< @returns	True if device has a mechanical identification switch
		inline bool		HasLEDAudioMeters (void)				{return dev.IsSupported(kDeviceHasLEDAudioMeters);}								///< @returns	True if device has LED audio meters
		inline bool		HasMicInput (void)						{return dev.IsSupported(kDeviceHasMicrophoneInput);}							///< @returns	True if device has a microphone input connector
		inline bool		HasNTV4FrameStores (void)				{return dev.IsSupported(kDeviceHasNTV4FrameStores);}							///< @returns	True if device FrameStores are implemented with NTV4 firmware
		inline bool		HasNWL (void)							{return dev.IsSupported(kDeviceHasNWL);}										///< @returns	True if device has NorthWest Logic DMA hardware and/or firmware
		inline bool		HasPCIeGen2 (void)						{return dev.IsSupported(kDeviceHasPCIeGen2);}									///< @returns	True if device supports 2nd-generation PCIe
		inline bool		HasPWMFanControl (void)					{return dev.IsSupported(kDeviceHasPWMFanControl);}								///< @returns	True if device has a cooling fan that's controlled using pulse-width-modulation (PWM)
		inline bool		HasRetailSupport (void)					{return dev.IsSupported(kDeviceHasRetailSupport);}								///< @returns	True if device is supported by AJA "retail" software (AJA ControlPanel & ControlRoom)
		inline bool		HasRotaryEncoder (void)					{return dev.IsSupported(kDeviceHasRotaryEncoder);}								///< @returns	True if device has a rotary encoder control knob
		inline bool		HasSDIRelays (void)						{return dev.IsSupported(kDeviceHasSDIRelays);}									///< @returns	True if device has bypass relays on its SDI connectors
		inline bool		HasSPIFlash (void)						{return dev.IsSupported(kDeviceHasSPIFlash);}									///< @returns	True if device has SPI flash hardware
		inline bool		HasSPIFlashSerial (void)				{return dev.IsSupported(kDeviceHasSPIFlashSerial);}								///< @returns	True if device has serial SPI flash hardware
		inline bool		HasSPIv2 (void)							{return dev.IsSupported(kDeviceHasSPIv2);}										///< @deprecated	Use DeviceCapabilities::GetSPIFlashVersion instead
		inline bool		HasSPIv3 (void)							{return dev.IsSupported(kDeviceHasSPIv3);}										///< @deprecated	Use DeviceCapabilities::GetSPIFlashVersion instead
		inline bool		HasSPIv4 (void)							{return dev.IsSupported(kDeviceHasSPIv4);}										///< @deprecated	Use DeviceCapabilities::GetSPIFlashVersion instead
		inline bool		HasSPIv5 (void)							{return dev.IsSupported(kDeviceHasSPIv5);}										///< @deprecated	Use DeviceCapabilities::GetSPIFlashVersion instead
		inline bool		HasXilinxDMA (void)						{return dev.IsSupported(kDeviceHasXilinxDMA);}									///< @returns	True if device has Xilinx DMA hardware and/or firmware
		inline bool		Is64Bit (void)							{return dev.IsSupported(kDeviceIs64Bit);}										///< @returns	True if device is 64-bit addressable
		inline bool		IsDirectAddressable (void)				{return dev.IsSupported(kDeviceIsDirectAddressable);}							///< @returns	True if device is direct addressable
		inline bool		IsDNxIV (void)							{return dev.IsSupported(kDeviceHasMicrophoneInput);}							///< @deprecated	Use DeviceCapabilities::HasMicInput instead
		inline bool		IsExternalToHost (void)					{return dev.IsSupported(kDeviceIsExternalToHost);}								///< @returns	True if device connects to the host via a cable
		inline bool		IsLocalPhysical (void)					{return dev.IsSupported(kDeviceIsLocalPhysical);}								///< @returns	True if device is local-host-attached, not remote, software or virtual
		inline bool		IsSupported (void)						{return dev.IsSupported(kDeviceIsSupported);}									///< @returns	True if device is supported by this SDK
		inline bool		NeedsRoutingSetup (void)				{return dev.IsSupported(kDeviceNeedsRoutingSetup);}								///< @returns	True if device widget routing can be queried or changed
		inline bool		SoftwareCanChangeFrameBufferSize (void)	{return dev.IsSupported(kDeviceSoftwareCanChangeFrameBufferSize);}				///< @returns	True if device frame buffer size can be changed
		inline bool		ROMHasBankSelect (void)					{return dev.IsSupported(kDeviceROMHasBankSelect);}								///< @returns	True if device flash ROM has selectable banks

		inline ULWord	GetActiveMemorySize (void)				{return dev.GetNumSupported(kDeviceGetActiveMemorySize);}						///< @returns	The size, in bytes, of the device's active RAM available for video and audio
		inline UWord	GetDACVersion (void)					{return UWord(dev.GetNumSupported(kDeviceGetDACVersion));}						///< @returns	The version number of the DAC on the device
		inline UWord	GetDownConverterDelay (void)			{return UWord(dev.GetNumSupported(kDeviceGetDownConverterDelay));}				///< @returns	The down-converter delay on the device
		inline ULWord	GetGenlockVersion (void)				{return dev.GetNumSupported(kDeviceGetGenlockVersion);}							///< @returns	The version number of the device genlock hardware/firmware
		inline ULWord	GetHDMIVersion (void)					{return dev.GetNumSupported(kDeviceGetHDMIVersion);}							///< @returns	The version number of the HDMI chipset on the device
		inline ULWord	GetLUTVersion (void)					{return dev.GetNumSupported(kDeviceGetLUTVersion);}								///< @returns	The version number of the LUT(s) on the device
		inline ULWord	GetSPIFlashVersion (void)				{return dev.GetNumSupported(kDeviceGetSPIFlashVersion);}						///< @returns	The version number of the SPI-flash chipset used on the device
		inline UWord	GetMaxAudioChannels (void)				{return UWord(dev.GetNumSupported(kDeviceGetMaxAudioChannels));}				///< @returns	The maximum number of audio channels that an Audio System will accommodate on the device
		inline ULWord	GetMaxRegisterNumber (void)				{return dev.GetNumSupported(kDeviceGetMaxRegisterNumber);}						///< @returns	The highest NTV2 register number used on the device
		inline ULWord	GetMaxTransferCount (void)				{return dev.GetNumSupported(kDeviceGetMaxTransferCount);}						///< @returns	The maximum number of 32-bit words a DMA engine on the device can transfer in one operation
		inline UWord	GetNum2022ChannelsSFP1 (void)			{return UWord(dev.GetNumSupported(kDeviceGetNum2022ChannelsSFP1));}				///< @returns	The number of SMPTE 2022 channels configured on SFP 1 on the device
		inline UWord	GetNum2022ChannelsSFP2 (void)			{return UWord(dev.GetNumSupported(kDeviceGetNum2022ChannelsSFP2));}				///< @returns	The number of SMPTE 2022 channels configured on SFP 2 on the device
		inline UWord	GetNum4kQuarterSizeConverters (void)	{return UWord(dev.GetNumSupported(kDeviceGetNum4kQuarterSizeConverters));}		///< @returns	The number of quarter-size 4K/UHD down-converters on the device
		inline UWord	GetNumAESAudioInputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumAESAudioInputChannels));}		///< @returns	The number of AES/EBU audio input channels on the device
		inline UWord	GetNumAESAudioOutputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumAESAudioOutputChannels));}		///< @returns	The number of AES/EBU audio output channels on the device
		inline UWord	GetNumAnalogAudioInputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogAudioInputChannels));}		///< @returns	The number of analog audio input channels on the device
		inline UWord	GetNumAnalogAudioOutputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogAudioOutputChannels));}	///< @returns	The number of analog audio output channels on the device
		inline UWord	GetNumAnalogVideoInputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogVideoInputs));}			///< @returns	The number of analog video inputs on the device
		inline UWord	GetNumAnalogVideoOutputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogVideoOutputs));}			///< @returns	The number of analog video outputs on the device
		inline UWord	GetNumAudioSystems (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumAudioSystems));}					///< @returns	The number of independent Audio Systems on the device
		inline UWord	GetNumCrossConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumCrossConverters));}				///< @returns	The number of cross-converters on the device
		inline UWord	GetNumCSCs (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumCSCs));}							///< @returns	The number of colorspace converter widgets on the device
		inline ULWord	GetNumDMAEngines (void)					{return dev.GetNumSupported(kDeviceGetNumDMAEngines);}							///< @returns	The number of DMA engines on the device
		inline UWord	GetNumDownConverters (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumDownConverters));}				///< @returns	The number of down-converters on the device
		inline UWord	GetNumEmbeddedAudioInputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumEmbeddedAudioInputChannels));}	///< @returns	The number of SDI embedded input audio channels supported by the device
		inline UWord	GetNumEmbeddedAudioOutputChannels (void){return UWord(dev.GetNumSupported(kDeviceGetNumEmbeddedAudioOutputChannels));}	///< @returns	The number of SDI embedded output audio channels supported by the device
		inline UWord	GetNumFrameStores (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumFrameStores));}					///< @returns	The number of FrameStore widgets on the device
		inline UWord	GetNumFrameSyncs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumFrameSyncs));}					///< @returns	The number of frame sync widgets on the device
		inline UWord	GetNumHDMIAudioInputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIAudioInputChannels));}		///< @returns	The number of HDMI audio input channels on the device
		inline UWord	GetNumHDMIAudioOutputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIAudioOutputChannels));}		///< @returns	The number of HDMI audio output channels on the device
		inline UWord	GetNumHDMIVideoInputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIVideoInputs));}				///< @returns	The number of HDMI video input connectors on the device
		inline UWord	GetNumHDMIVideoOutputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIVideoOutputs));}				///< @returns	The number of HDMI video output connectors on the device
		inline UWord	GetNumInputConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumInputConverters));}				///< @returns	The number of input converter widgets on the device
		inline UWord	GetNumLTCInputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLTCInputs));}					///< @returns	The number of analog LTC input connectors on the device
		inline UWord	GetNumLTCOutputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLTCOutputs));}					///< @returns	The number of analog LTC output connectors on the device
		inline UWord	GetNumLUTBanks (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLUTBanks));}						///< @returns	The number of LUT banks on the device
		inline UWord	GetNumLUTs (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumLUTs));}							///< @returns	The number of LUT widgets on the device
		inline UWord	GetNumMicInputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumMicInputs));}					///< @returns	The number of microphone input connectors on the device
		inline UWord	GetNumMixers (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumMixers));}						///< @returns	The number of Mixer/Keyer widgets on the device
		inline UWord	GetNumOutputConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumOutputConverters));}				///< @returns	The number of output converter widgets on the device
		inline UWord	GetNumReferenceVideoInputs (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumReferenceVideoInputs));}			///< @returns	The number of external reference input connectors on the device
		inline UWord	GetNumSerialPorts (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumSerialPorts));}					///< @returns	The number of RS-422 serial port connectors on the device
		inline UWord	GetNumTSIMuxers (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumTSIMuxers));}					///< @returns	The number of TSI Mux/Demux widgets on the device
		inline UWord	GetNumUpConverters (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumUpConverters));}					///< @returns	The number of up-converters on the device
		inline ULWord	GetNumVideoChannels (void)				{return dev.GetNumSupported(kDeviceGetNumVideoChannels);}						///< @deprecated	Use DeviceCapabilities::GetNumFrameStores instead
		inline UWord	GetNumVideoInputs (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumVideoInputs));}					///< @returns	The number of SDI video inputs on the device
		inline UWord	GetNumVideoOutputs (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumVideoOutputs));}					///< @returns	The number of SDI video outputs on the device
		inline ULWord	GetPingLED (void)						{return dev.GetNumSupported(kDeviceGetPingLED);}								///< @returns	The highest bit number of the LED bits in the Global Control Register on the device
		inline UWord	GetTotalNumAudioSystems (void)			{return UWord(dev.GetNumSupported(kDeviceGetTotalNumAudioSystems));}			///< @returns	The number of independent Audio Systems on the device
		inline UWord	GetNumBufferedAudioSystems (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumBufferedAudioSystems));}			///< @returns	The total number of audio systems on the device that can read/write audio buffer memory, including the host audio system, if present
		inline ULWord	GetUFCVersion (void)					{return dev.GetNumSupported(kDeviceGetUFCVersion);}								///< @returns	The version number of the UFC on the device

		/**
			@param[in]	inChannel	Specifies the ::NTV2Channel or FrameStore of interest.
			@returns	True if the device has the given FrameStore; otherwise false.
		**/
		inline bool		CanDoChannel (const NTV2Channel inChannel)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_Channel));
							return itms.find(ULWord(inChannel)) != itms.end();
						}

		/**
			@param[in]	inMode		Specifies the ::NTV2ConversionMode of interest.
			@returns	True if the device can perform the requested conversion; otherwise false.
		**/
		inline bool		CanDoConversionMode (const NTV2ConversionMode inMode)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_ConversionMode));
							return itms.find(ULWord(inMode)) != itms.end();
						}

		/**
			@param[in]	inMode		Specifies the ::NTV2DSKMode of interest.
			@returns	True if the device's Mixer/Keyer widget(s) can accommodate the requested downstream keyer mode; otherwise false.
		**/
		inline bool		CanDoDSKMode (const NTV2DSKMode inMode)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_DSKMode));
							return itms.find(ULWord(inMode)) != itms.end();
						}

		/**
			@param[in]	inPF		Specifies the ::NTV2PixelFormat of interest.
			@returns	True if the device's FrameStore widget(s) can read or write the requested pixel format; otherwise false.
		**/
		inline bool		CanDoFrameBufferFormat (const NTV2PixelFormat inPF)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_PixelFormat));
							return itms.find(ULWord(inPF)) != itms.end();
						}
		inline bool		CanDoPixelFormat (const NTV2PixelFormat inPF)		{return CanDoFrameBufferFormat(inPF);}	///< @brief	Same as DeviceCapabilities::CanDoFrameBufferFormat.

		/**
			@param[in]	inSrc		Specifies the ::NTV2InputSource of interest.
			@returns	True if the device has the requested input source; otherwise false.
		**/
		inline bool		CanDoInputSource (const NTV2InputSource inSrc)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputSource));
							return itms.find(ULWord(inSrc)) != itms.end();
						}

		/**
			@param[in]	inDest		Specifies the ::NTV2OutputDestination of interest.
			@returns	True if the device has the requested output destination connector; otherwise false.
		**/
		inline bool		CanDoOutputDestination (const NTV2OutputDestination inDest)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputDest));
							return itms.find(ULWord(inDest)) != itms.end();
						}

		/**
			@param[in]	inVF		Specifies the ::NTV2VideoFormat of interest.
			@returns	True if the device's FrameStore(s) can handle the given video format; otherwise false.
		**/
		inline bool		CanDoVideoFormat (const NTV2VideoFormat inVF)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_VideoFormat));
							return itms.find(ULWord(inVF)) != itms.end();
						}

		/**
			@param[in]	inWgtID		Specifies the ::NTV2WidgetID of interest.
			@returns	True if the device firmware implements the given widget; otherwise false.
		**/
		inline bool		CanDoWidget (const NTV2WidgetID inWgtID)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_WidgetID));
							return itms.find(ULWord(inWgtID)) != itms.end();
						}

		/**
			@param[in]	inWgtType	Specifies the ::NTV2WidgetType.
			@param[in]	index0		Specifies the widget of interest as a zero-based index.
			@returns	True if the device firmware implements the given widget; otherwise false.
		**/
		bool			CanDoWidget (const NTV2WidgetType inWgtType, const UWord index0);

		/**
			@returns	The set of unique ::NTV2PixelFormat values supported by the device's FrameStore(s).
		**/
		inline NTV2PixelFormats	PixelFormats (void)
						{	NTV2PixelFormats result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_PixelFormat));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2PixelFormat(*it));
							return result;
						}

		/**
			@returns	The set of unique ::NTV2VideoFormat values supported by the device's FrameStore(s).
		**/
		inline NTV2VideoFormatSet	VideoFormats (void)
						{	NTV2VideoFormatSet result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_VideoFormat));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2VideoFormat(*it));
							return result;
						}

		/**
			@returns	The set of unique ::NTV2TimecodeIndex values that can be ingested and captured by the device.
		**/
		inline NTV2TCIndexes	InputTCIndexes (void)
						{	NTV2TCIndexes result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputTCIndex));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2TCIndex(*it));
							return result;
						}

		/**
			@returns	The set of unique ::NTV2TimecodeIndex values that can be output by the device.
		**/
		inline NTV2TCIndexes	OutputTCIndexes (void)
						{	NTV2TCIndexes result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputTCIndex));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2TCIndex(*it));
							return result;
						}

		/**
			@returns	The set of unique ::NTV2AudioRate values that are supported by the device.
		**/
		inline NTV2AudioRateSet	AudioSampleRates (void)								//	New in SDK 18.0
						{	NTV2AudioRateSet result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_AudioRate));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2AudioRate(*it));
							return result;
						}

		/**
			@param[in]	inTCNdx		Specifies the ::NTV2TimecodeIndex of interest.
			@returns	True if the device can read the given timecode; otherwise false.
		**/
		inline bool		CanDoInputTCIndex (const NTV2TCIndex inTCNdx)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputTCIndex));
							return itms.find(ULWord(inTCNdx)) != itms.end();
						}

		/**
			@param[in]	inTCNdx		Specifies the ::NTV2TimecodeIndex of interest.
			@returns	True if the device can write the given timecode; otherwise false.
		**/
		inline bool		CanDoOutputTCIndex (const NTV2TCIndex inTCNdx)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputTCIndex));
							return itms.find(ULWord(inTCNdx)) != itms.end();
						}

		/**
			@param[in]	inTCNdx		Specifies the ::NTV2TimecodeIndex of interest.
			@returns	True if the device can write the given timecode; otherwise false.
		**/
		inline bool		CanDoAudioSampleRate (const NTV2AudioRate inRate)			//	New in SDK 18.0
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_AudioRate));
							return itms.find(ULWord(inRate)) != itms.end();
						}
	private:
		CNTV2DriverInterface &	dev;	///< @brief	My reference to the NTV2 device
};	//	DeviceCapabilities

#endif	//	NTV2_DEVICECAPABILITIES_H
