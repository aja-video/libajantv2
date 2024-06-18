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
	@see	vidop-features
**/
class AJAExport DeviceCapabilities
{
	public:
		explicit inline	DeviceCapabilities (CNTV2DriverInterface & inDev)	: dev(inDev)	{}
		inline			operator bool() const					{return dev.IsOpen();}	///< @returns true if I'm valid
		inline bool		CanChangeEmbeddedAudioClock (void)		{return dev.IsSupported(kDeviceCanChangeEmbeddedAudioClock);}
		inline bool		CanChangeFrameBufferSize (void)			{return dev.IsSupported(kDeviceCanChangeFrameBufferSize);}	///< @returns true if frame buffer sizes are not fixed
		inline bool		CanDisableUFC (void)					{return dev.IsSupported(kDeviceCanDisableUFC);}	///< @returns true if there's at least one UFC, and it can be disabled
		inline bool		CanDo12gRouting (void)					{return dev.IsSupported(kDeviceCanDo12gRouting);}	///< @returns true if device supports 6G/12G routing crosspoints
		inline bool		CanDo12GSDI (void)						{return dev.IsSupported(kDeviceCanDo12GSDI);}	///< @returns true if device has 6G/12G capable SDI connector(s)
		inline bool		CanDo2110 (void)						{return dev.IsSupported(kDeviceCanDo2110);}	///< @returns true if device supports SMPTE ST2110
		inline bool		CanDo2KVideo (void)						{return dev.IsSupported(kDeviceCanDo2KVideo);}	///< @returns true if device can handle 2Kx1556 (film format) video
		inline bool		CanDo3GLevelConversion (void)			{return dev.IsSupported(kDeviceCanDo3GLevelConversion);}	///< @returns true if device can do 3G level B to 3G level A conversion
		inline bool		CanDo425Mux (void)						{return dev.IsSupported(kDeviceCanDo425Mux);}	///< @returns true if device supports SMPTE 425 mux control
		inline bool		CanDo4KVideo (void)						{return dev.IsSupported(kDeviceCanDo4KVideo);}	///< @returns true if device supports 4K/UHD video
		inline bool		CanDo8KVideo (void)						{return dev.IsSupported(kDeviceCanDo8KVideo);}	///< @returns true if device supports 8K/UHD2 video formats
		inline bool		CanDoAESAudioIn (void)					{return dev.IsSupported(kDeviceCanDoAESAudioIn);}	///< @returns true if device has any AES audio input(s)
		inline bool		CanDoAESAudioOut (void)					{return dev.IsSupported(kDeviceCanDoAESAudioOut);}	///< @returns true if device has any AES audio output(s)
		inline bool		CanDoAnalogAudio (void)					{return dev.IsSupported(kDeviceCanDoAnalogAudio);}	///< @returns true if device has any analog audio inputs or outputs
		inline bool		CanDoAnalogVideoIn (void)				{return dev.IsSupported(kDeviceCanDoAnalogVideoIn);}	///< @returns true if device has any analog video inputs
		inline bool		CanDoAnalogVideoOut (void)				{return dev.IsSupported(kDeviceCanDoAnalogVideoOut);}	///< @returns true if device has any analog video outputs
		inline bool		CanDoAudio192K (void)					{return dev.IsSupported(kDeviceCanDoAudio192K);}	///< @returns true if device audio system(s) support a 192kHz sample rate
		inline bool		CanDoAudio2Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio2Channels);}	///< @returns true if audio system(s) support 2 or more audio channels
		inline bool		CanDoAudio6Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio6Channels);}	///< @returns true if audio system(s) support 6 or more audio channels
		inline bool		CanDoAudio8Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio8Channels);}	///< @returns true if audio system(s) support 8 or more audio channels
		inline bool		CanDoAudio96K (void)					{return dev.IsSupported(kDeviceCanDoAudio96K);}	///< @returns true if device audio system(s) support a 96kHz sample rate
		inline bool		CanDoAudioDelay (void)					{return dev.IsSupported(kDeviceCanDoAudioDelay);}	///< @returns true if audio system(s) support an adjustable delay
		inline bool		CanDoAudioInput (void)					{return dev.IsSupported(kDeviceCanDoAudioInput);}	///< @returns true if device has any audio input capability (SDI, HDMI, AES or analog)
		inline bool		CanDoAudioMixer (void)					{return dev.IsSupported(kDeviceCanDoAudioMixer);}	///< @returns true if device has an audio mixer
		inline bool		CanDoAudioOutput (void)					{return dev.IsSupported(kDeviceCanDoAudioOutput);}	///< @returns true if device has any audio output capability (SDI, HDMI, AES or analog)
		inline bool		CanDoBreakoutBoard (void)				{return dev.IsSupported(kDeviceCanDoBreakoutBoard);}	///< @returns true if device supports an AJA breakout board
		inline bool		CanDoBreakoutBox (void)					{return dev.IsSupported(kDeviceCanDoBreakoutBox);}	///< @returns true if device supports an AJA breakout box
		inline bool		CanDoCapture (void)						{return dev.IsSupported(kDeviceCanDoCapture);}	///< @returns true if device has any SDI, HDMI or analog video inputs
		inline bool		CanDoColorCorrection (void)				{return dev.IsSupported(kDeviceCanDoColorCorrection);}	///< @returns true if device has any LUTs
		inline bool		CanDoCustomAnc (void)					{return dev.IsSupported(kDeviceCanDoCustomAnc);}	///< @returns true if device has SDI ANC inserter/extractor firmware
		inline bool		CanDoCustomHancInsertion (void)			{return dev.IsSupported(kDeviceCanDoCustomHancInsertion);}	///< @returns	true if device supports custom HANC packet insertion
		inline bool		CanDoDSKOpacity (void)					{return dev.IsSupported(kDeviceCanDoDSKOpacity);}	///< @returns true if device mixer/keyer supports adjustable opacity
		inline bool		CanDoDualLink (void)					{return dev.IsSupported(kDeviceCanDoDualLink);}	///< @returns true if device supports 10-bit RGB input/output over 2-wire SDI
		inline bool		CanDoDVCProHD (void)					{return dev.IsSupported(kDeviceCanDoDVCProHD);}	///< @returns true if device can squeeze/stretch between 1920x1080/1280x1080 and 1280x720/960x720
		inline bool		CanDoEnhancedCSC (void)					{return dev.IsSupported(kDeviceCanDoEnhancedCSC);}	///< @returns true if device has enhanced CSCs
		inline bool		CanDoFramePulseSelect (void)			{return dev.IsSupported(kDeviceCanDoFramePulseSelect);}	///< @returns true if device supports frame pulse source independent of reference source
		inline bool		CanDoFrameStore1Display (void)			{return dev.IsSupported(kDeviceCanDoFrameStore1Display);}	///< @returns true if device can display/output video from FrameStore 1
		inline bool		CanDoHDMIAuxCapture (void)				{return dev.IsSupported(kDeviceCanDoHDMIAuxCapture);}	///< @returns true if device has HDMI AUX data extractor(s)
		inline bool		CanDoHDMIAuxPlayback (void)				{return dev.IsSupported(kDeviceCanDoHDMIAuxPlayback);}	///< @returns true if device has HDMI AUX data inserter(s)
		inline bool		CanDoHDMIHDROut (void)					{return dev.IsSupported(kDeviceCanDoHDMIHDROut);}	///< @returns true if device supports HDMI HDR output
		inline bool		CanDoHDMIMultiView (void)				{return dev.IsSupported(kDeviceCanDoHDMIMultiView);}	///< @returns true if device can rasterize 4 HD signals into a single HDMI output
		inline bool		CanDoHDMIOutStereo (void)				{return dev.IsSupported(kDeviceCanDoHDMIOutStereo);}	///< @returns true if device supports 3D/stereo HDMI video output
		inline bool		CanDoHDMIQuadRasterConversion (void)	{return dev.IsSupported(kDeviceCanDoHDMIQuadRasterConversion);}	///< @returns	true if HDMI in/out supports square-division (quad) raster conversion
		inline bool		CanDoHDV (void)							{return dev.IsSupported(kDeviceCanDoHDV);}	///< @returns true if device can squeeze/stretch between 1920x1080 and 1440x1080
		inline bool		CanDoHDVideo (void)						{return dev.IsSupported(kDeviceCanDoHDVideo);}	///< @returns true if device can handle HD (High Definition) video
		inline bool		CanDoHFRRGB (void)						{return dev.IsSupported(kDeviceCanDoHFRRGB);}	///< @returns true if device supports 1080p RGB at more than 50fps
		inline bool		CanDoIP (void)							{return dev.IsSupported(kDeviceCanDoIP);}	///< @returns true if device has SFP network connectors
		inline bool		CanDoIsoConvert (void)					{return dev.IsSupported(kDeviceCanDoIsoConvert);}	///< @returns true if device can do ISO conversion
		inline bool		CanDoJ2K (void)							{return dev.IsSupported(kDeviceCanDoJ2K);}	///< @returns true if device supports JPEG 2000 codec
		inline bool		CanDoLTC (void)							{return dev.IsSupported(kDeviceCanDoLTC);}	///< @returns true if device has any LTC (Linear TimeCode) inputs
		inline bool		CanDoLTCInOnRefPort (void)				{return dev.IsSupported(kDeviceCanDoLTCInOnRefPort);}	///< @returns true if device can read LTC (Linear TimeCode) from its reference input
		inline bool		CanDoMSI (void)							{return dev.IsSupported(kDeviceCanDoMSI);}	///< @returns true if device DMA hardware supports MSI (Message Signaled Interrupts)
		inline bool		CanDoMultiFormat (void)					{return dev.IsSupported(kDeviceCanDoMultiFormat);}	///< @returns true if device FrameStores can independently accommodate different video formats
		inline bool		CanDoMultiLinkAudio (void)				{return dev.IsSupported(kDeviceCanDoMultiLinkAudio);}	///< @returns true if device supports grouped audio system control
		inline bool		CanDoPCMControl (void)					{return dev.IsSupported(kDeviceCanDoPCMControl);}	///< @returns true if device can mark specific audio channel pairs as not carrying LPCM
		inline bool		CanDoPCMDetection (void)				{return dev.IsSupported(kDeviceCanDoPCMDetection);}	///< @returns true if device can detect which audio channel pairs are not carrying LPCM
		inline bool		CanDoPIO (void)							{return dev.IsSupported(kDeviceCanDoPIO);}	///< @returns true if device supports programmed I/O
		inline bool		CanDoPlayback (void)					{return dev.IsSupported(kDeviceCanDoPlayback);}	///< @returns true if device has any SDI, HDMI or analog video outputs
		inline bool		CanDoProgrammableRS422 (void)			{return dev.IsSupported(kDeviceCanDoProgrammableRS422);}	///< @returns true if device has at least one RS-422 serial port that can be programmed (for baud rate, parity, etc.)
		inline bool		CanDoProRes (void)						{return dev.IsSupported(kDeviceCanDoProRes);}	///< @returns true if device FrameStore(s) will read/write Apple ProRes-compressed video
		inline bool		CanDoQREZ (void)						{return dev.IsSupported(kDeviceCanDoQREZ);}	///< @returns true if device can handle QRez
		inline bool		CanDoQuarterExpand (void)				{return dev.IsSupported(kDeviceCanDoQuarterExpand);}	///< @returns true if device FrameStores will pixel-halve/line-halve on input, pixel-double/line-double on output
		inline bool		CanDoRateConvert (void)					{return dev.IsSupported(kDeviceCanDoRateConvert);}	///< @returns true if device can do frame rate conversion
		inline bool		CanDoRGBLevelAConversion (void)			{return dev.IsSupported(kDeviceCanDoRGBLevelAConversion);}	///< @returns true if the device can do RGB over 3G Level A
		inline bool		CanDoRGBPlusAlphaOut (void)				{return dev.IsSupported(kDeviceCanDoRGBPlusAlphaOut);}	///< @returns true if device has CSCs capable of splitting the key & fill from RGB frame buffers (unrelated to RGB SDI wire formats)
		inline bool		CanDoRP188 (void)						{return dev.IsSupported(kDeviceCanDoRP188);}	///< @returns true if device can insert and/or extract SMPTE 12M (RP-188/VITC)
		inline bool		CanDoSDIErrorChecks (void)				{return dev.IsSupported(kDeviceCanDoSDIErrorChecks);}	///< @returns true if device has SDI inputs and can report errors (CRC, TRS, etc.)
		inline bool		CanDoSDVideo (void)						{return dev.IsSupported(kDeviceCanDoSDVideo);}	///< @returns true if device can handle standard definition video
		inline bool		CanDoStackedAudio (void)				{return dev.IsSupported(kDeviceCanDoStackedAudio);}	///< @returns true if device uses a "stacked" arrangement of its audio buffers
		inline bool		CanDoStereoIn (void)					{return dev.IsSupported(kDeviceCanDoStereoIn);}	///< @returns true if device supports 3D/Stereo video input over dual-stream SDI
		inline bool		CanDoStereoOut (void)					{return dev.IsSupported(kDeviceCanDoStereoOut);}	///< @returns true if device supports 3D/Stereo video output over dual-stream SDI
		inline bool		CanDoStreamingDMA (void)				{return dev.IsSupported(kDeviceCanDoStreamingDMA);}	///< @returns	true if device supports streaming DMA
		inline bool		CanDoThunderbolt (void)					{return dev.IsSupported(kDeviceCanDoThunderbolt);}	///< @returns true if device connects to the host via a Thunderbolt cable
		inline bool		CanDoVideoProcessing (void)				{return dev.IsSupported(kDeviceCanDoVideoProcessing);}	///< @returns true if device can do video processing
		inline bool		CanDoVITC2 (void)						{return dev.IsSupported(kDeviceCanDoVITC2);}	///< @returns true if device can insert or extract SMPTE 12M RP-188/VITC2
		inline bool		CanDoWarmBootFPGA (void)				{return dev.IsSupported(kDeviceCanDoWarmBootFPGA);}	///< @returns true if device can warm-boot to load updated firmware
		inline bool		CanMeasureTemperature (void)			{return dev.IsSupported(kDeviceCanMeasureTemperature);}	///< @returns true if device can measure its FPGA die temperature
		inline bool		CanReportFailSafeLoaded (void)			{return dev.IsSupported(kDeviceCanReportFailSafeLoaded);}	///< @returns true if device can report if its "fail-safe" firmware is loaded/running
		inline bool		CanReportFrameSize (void)				{return dev.IsSupported(kDeviceCanReportFrameSize);}	///< @returns true if device can report its intrinsic frame size
		inline bool		CanReportRunningFirmwareDate (void)		{return dev.IsSupported(kDeviceCanReportRunningFirmwareDate);}	///< @returns true if device can report its running (and not necessarily installed) firmware date
		inline bool		CanThermostat (void)					{return dev.IsSupported(kDeviceCanThermostat);}	///< @returns true if device fan can be thermostatically controlled
		inline bool		HasAudioMonitorRCAJacks (void)			{return dev.IsSupported(kDeviceHasAudioMonitorRCAJacks);}	///< @returns true if device has a pair of unbalanced RCA audio monitor output connectors
		inline bool		HasBiDirectionalAnalogAudio (void)		{return dev.IsSupported(kDeviceHasBiDirectionalAnalogAudio);}	///< @returns true if device has a bi-directional analog audio connector
		inline bool		HasBiDirectionalSDI (void)				{return dev.IsSupported(kDeviceHasBiDirectionalSDI);}	///< @returns true if device has bi-directional SDI connectors
		inline bool		HasBreakoutBoard (void)					{return dev.IsSupported(kDeviceHasBreakoutBoard);}	///< @returns true if device has attached/connected breakout board
		inline bool		HasColorSpaceConverterOnChannel2 (void)	{return dev.IsSupported(kDeviceHasColorSpaceConverterOnChannel2);}	///< @returns true if device has a second colorspace converter widget (NTV2_WgtCSC2)
		inline bool		HasCrosspointConnectROM (void)			{return dev.IsSupported(kDeviceHasXptConnectROM);}	///< @returns true if device has a crosspoint connection ROM
		inline bool		HasGenlockv2 (void)						{return dev.IsSupported(kDeviceHasGenlockv2);}	///< @returns true if device has version 2 genlock hardware and/or firmware
		inline bool		HasGenlockv3 (void)						{return dev.IsSupported(kDeviceHasGenlockv3);}	///< @returns true if device has version 3 genlock hardware and/or firmware
		inline bool		HasHeadphoneJack (void)					{return dev.IsSupported(kDeviceHasHeadphoneJack);}	///< @returns true if device has a headphone jack
		inline bool		HasHEVCM30 (void)						{return dev.IsSupported(kDeviceHasHEVCM30);}	///< @returns true if device has an HEVC M30 encoder/decoder
		inline bool		HasHEVCM31 (void)						{return dev.IsSupported(kDeviceHasHEVCM31);}	///< @returns true if device has an HEVC M31 encoder/decoder
		inline bool		HasIDSwitch (void)						{return dev.IsSupported(kDeviceHasIDSwitch);}	///< @returns	true if device has a mechanical identification switch
		inline bool		HasLEDAudioMeters (void)				{return dev.IsSupported(kDeviceHasLEDAudioMeters);}	///< @returns true if device has LED audio meters
		inline bool		HasMicInput (void)						{return dev.IsSupported(kDeviceHasMicrophoneInput);}	///< @returns true if device has a microphone input connector
		inline bool		HasNTV4FrameStores (void)				{return dev.IsSupported(kDeviceHasNTV4FrameStores);}	///< @returns true if device FrameStores are implemented with NTV4 firmware
		inline bool		HasNWL (void)							{return dev.IsSupported(kDeviceHasNWL);}	///< @returns true if device has NorthWest Logic DMA hardware and/or firmware
		inline bool		HasPCIeGen2 (void)						{return dev.IsSupported(kDeviceHasPCIeGen2);}	///< @returns true if device supports 2nd-generation PCIe
		inline bool		HasPWMFanControl (void)					{return dev.IsSupported(kDeviceHasPWMFanControl);}	///< @returns true if device has a cooling fan that's controlled using pulse-width-modulation (PWM)
		inline bool		HasRetailSupport (void)					{return dev.IsSupported(kDeviceHasRetailSupport);}	///< @returns true if device is supported by AJA "retail" software (AJA ControlPanel & ControlRoom)
		inline bool		HasRotaryEncoder (void)					{return dev.IsSupported(kDeviceHasRotaryEncoder);}	///< @returns true if device has a rotary encoder control knob
		inline bool		HasSDIRelays (void)						{return dev.IsSupported(kDeviceHasSDIRelays);}	///< @returns true if device has bypass relays on its SDI connectors
		inline bool		HasSPIFlash (void)						{return dev.IsSupported(kDeviceHasSPIFlash);}	///< @returns true if device has SPI flash hardware
		inline bool		HasSPIFlashSerial (void)				{return dev.IsSupported(kDeviceHasSPIFlashSerial);}	///< @returns true if device has serial SPI flash hardware
		inline bool		HasSPIv2 (void)							{return dev.IsSupported(kDeviceHasSPIv2);}	///< @deprecated Use DeviceCapabilities::GetSPIFlashVersion instead
		inline bool		HasSPIv3 (void)							{return dev.IsSupported(kDeviceHasSPIv3);}	///< @deprecated Use DeviceCapabilities::GetSPIFlashVersion instead
		inline bool		HasSPIv4 (void)							{return dev.IsSupported(kDeviceHasSPIv4);}	///< @deprecated Use DeviceCapabilities::GetSPIFlashVersion instead
		inline bool		HasSPIv5 (void)							{return dev.IsSupported(kDeviceHasSPIv5);}	///< @deprecated Use DeviceCapabilities::GetSPIFlashVersion instead
		inline bool		HasXilinxDMA (void)						{return dev.IsSupported(kDeviceHasXilinxDMA);}	///< @returns true if device has Xilinx DMA hardware and/or firmware
		inline bool		Is64Bit (void)							{return dev.IsSupported(kDeviceIs64Bit);}	///< @returns true if device is 64-bit addressable
		inline bool		IsDirectAddressable (void)				{return dev.IsSupported(kDeviceIsDirectAddressable);}	///< @returns true if device is direct addressable
		inline bool		IsDNxIV (void)							{return dev.IsSupported(kDeviceHasMicrophoneInput);}	///< @deprecated Use DeviceCapabilities::HasMicInput instead
		inline bool		IsExternalToHost (void)					{return dev.IsSupported(kDeviceIsExternalToHost);}	///< @returns true if device connects to the host via a cable
		inline bool		IsSupported (void)						{return dev.IsSupported(kDeviceIsSupported);}	///< @returns true if device is supported by this SDK
		inline bool		NeedsRoutingSetup (void)				{return dev.IsSupported(kDeviceNeedsRoutingSetup);}	///< @returns true if device widget routing can be queried or changed
		inline bool		SoftwareCanChangeFrameBufferSize (void)	{return dev.IsSupported(kDeviceSoftwareCanChangeFrameBufferSize);}	///< @returns true if device frame buffer size can be changed
		inline ULWord	GetActiveMemorySize (void)				{return dev.GetNumSupported(kDeviceGetActiveMemorySize);}	///< @returns the size, in bytes, of the device's active RAM available for video and audio
		inline UWord	GetDACVersion (void)					{return UWord(dev.GetNumSupported(kDeviceGetDACVersion));}	///< @returns the version number of the DAC on the device
		inline UWord	GetDownConverterDelay (void)			{return UWord(dev.GetNumSupported(kDeviceGetDownConverterDelay));}	///< @returns the down-converter delay on the device
		inline ULWord	GetHDMIVersion (void)					{return dev.GetNumSupported(kDeviceGetHDMIVersion);}	///< @returns the version number of the HDMI chipset on the device
		inline ULWord	GetLUTVersion (void)					{return dev.GetNumSupported(kDeviceGetLUTVersion);}	///< @returns the version number of the LUT(s) on the device
		inline ULWord	GetSPIFlashVersion (void)				{return dev.GetNumSupported(kDeviceGetSPIFlashVersion);}	///< @returns	the version number of the SPI-flash chipset used on the device
		inline UWord	GetMaxAudioChannels (void)				{return UWord(dev.GetNumSupported(kDeviceGetMaxAudioChannels));}	///< @returns the maximum number of audio channels that an Audio System will accommodate on the device
		inline ULWord	GetMaxRegisterNumber (void)				{return dev.GetNumSupported(kDeviceGetMaxRegisterNumber);}	///< @returns the highest NTV2 register number used on the device
		inline ULWord	GetMaxTransferCount (void)				{return dev.GetNumSupported(kDeviceGetMaxTransferCount);}	///< @returns the maximum number of 32-bit words a DMA engine on the device can transfer in one operation
		inline UWord	GetNum2022ChannelsSFP1 (void)			{return UWord(dev.GetNumSupported(kDeviceGetNum2022ChannelsSFP1));}	///< @returns the number of SMPTE 2022 channels configured on SFP 1 on the device
		inline UWord	GetNum2022ChannelsSFP2 (void)			{return UWord(dev.GetNumSupported(kDeviceGetNum2022ChannelsSFP2));}	///< @returns the number of SMPTE 2022 channels configured on SFP 2 on the device
		inline UWord	GetNum4kQuarterSizeConverters (void)	{return UWord(dev.GetNumSupported(kDeviceGetNum4kQuarterSizeConverters));}	///< @returns the number of quarter-size 4K/UHD down-converters on the device
		inline UWord	GetNumAESAudioInputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumAESAudioInputChannels));}	///< @returns the number of AES/EBU audio input channels on the device
		inline UWord	GetNumAESAudioOutputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumAESAudioOutputChannels));}	///< @returns the number of AES/EBU audio output channels on the device
		inline UWord	GetNumAnalogAudioInputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogAudioInputChannels));}	///< @returns the number of analog audio input channels on the device
		inline UWord	GetNumAnalogAudioOutputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogAudioOutputChannels));}	///< @returns the number of analog audio output channels on the device
		inline UWord	GetNumAnalogVideoInputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogVideoInputs));}	///< @returns the number of analog video inputs on the device
		inline UWord	GetNumAnalogVideoOutputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogVideoOutputs));}	///< @returns the number of analog video outputs on the device
		inline UWord	GetNumAudioSystems (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumAudioSystems));}	///< @returns the number of independent Audio Systems on the device
		inline UWord	GetNumCrossConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumCrossConverters));}	///< @returns the number of cross-converters on the device
		inline UWord	GetNumCSCs (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumCSCs));}	///< @returns the number of colorspace converter widgets on the device
		inline ULWord	GetNumDMAEngines (void)					{return dev.GetNumSupported(kDeviceGetNumDMAEngines);}	///< @returns the number of DMA engines on the device
		inline UWord	GetNumDownConverters (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumDownConverters));}	///< @returns the number of down-converters on the device
		inline UWord	GetNumEmbeddedAudioInputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumEmbeddedAudioInputChannels));}	///< @returns the number of SDI embedded input audio channels supported by the device
		inline UWord	GetNumEmbeddedAudioOutputChannels (void){return UWord(dev.GetNumSupported(kDeviceGetNumEmbeddedAudioOutputChannels));}	///< @returns the number of SDI embedded output audio channels supported by the device
		inline UWord	GetNumFrameStores (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumFrameStores));}	///< @returns the number of FrameStore widgets on the device
		inline UWord	GetNumFrameSyncs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumFrameSyncs));}	///< @returns the number of frame sync widgets on the device
		inline UWord	GetNumHDMIAudioInputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIAudioInputChannels));}	///< @returns the number of HDMI audio input channels on the device
		inline UWord	GetNumHDMIAudioOutputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIAudioOutputChannels));}	///< @returns the number of HDMI audio output channels on the device
		inline UWord	GetNumHDMIVideoInputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIVideoInputs));}	///< @returns the number of HDMI video input connectors on the device
		inline UWord	GetNumHDMIVideoOutputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIVideoOutputs));}	///< @returns the number of HDMI video output connectors on the device
		inline UWord	GetNumInputConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumInputConverters));}	///< @returns the number of input converter widgets on the device
		inline UWord	GetNumLTCInputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLTCInputs));}	///< @returns the number of analog LTC input connectors on the device
		inline UWord	GetNumLTCOutputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLTCOutputs));}	///< @returns the number of analog LTC output connectors on the device
		inline UWord	GetNumLUTBanks (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLUTBanks));}	///< @returns the number of LUT banks on the device
		inline UWord	GetNumLUTs (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumLUTs));}	///< @returns the number of LUT widgets on the device
		inline UWord	GetNumMicInputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumMicInputs));}	///< @returns the number of microphone input connectors on the device
		inline UWord	GetNumMixers (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumMixers));}	///< @returns the number of Mixer/Keyer widgets on the device
		inline UWord	GetNumOutputConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumOutputConverters));}	///< @returns the number of output converter widgets on the device
		inline UWord	GetNumReferenceVideoInputs (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumReferenceVideoInputs));}	///< @returns the number of external reference input connectors on the device
		inline UWord	GetNumSerialPorts (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumSerialPorts));}	///< @returns The number of RS-422 serial port connectors on the device
		inline UWord	GetNumTSIMuxers (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumTSIMuxers));}	///< @returns	the number of TSI Mux/Demux widgets on the device
		inline UWord	GetNumUpConverters (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumUpConverters));}	///< @returns the number of up-converters on the device
		inline ULWord	GetNumVideoChannels (void)				{return dev.GetNumSupported(kDeviceGetNumVideoChannels);}	///< @deprecated Use DeviceCapabilities::GetNumFrameStores instead
		inline UWord	GetNumVideoInputs (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumVideoInputs));}	///< @returns the number of SDI video inputs on the device
		inline UWord	GetNumVideoOutputs (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumVideoOutputs));}	///< @returns the number of SDI video outputs on the device
		inline ULWord	GetPingLED (void)						{return dev.GetNumSupported(kDeviceGetPingLED);}	///< @returns the highest bit number of the LED bits in the Global Control Register on the device
		inline UWord	GetTotalNumAudioSystems (void)			{return UWord(dev.GetNumSupported(kDeviceGetTotalNumAudioSystems));}	///< @returns the number of independent Audio Systems on the device
		inline UWord	GetNumBufferedAudioSystems (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumBufferedAudioSystems));}	///< @returns the total number of audio systems on the device that can read/write audio buffer memory, including the host audio system, if present
		inline ULWord	GetUFCVersion (void)					{return dev.GetNumSupported(kDeviceGetUFCVersion);}	///< @returns the version number of the UFC on the device
        
		/**
			@param[in]	inChannel	Specifies the channel or FrameStore of interest.
			@returns	true if the device has the given FrameStore; otherwise false.
		**/
		inline bool		CanDoChannel (const NTV2Channel inChannel)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_Channel));
							return itms.find(ULWord(inChannel)) != itms.end();
						}

		/**
			@param[in]	inMode		Specifies the NTV2ConversionMode of interest.
			@returns	true if the device can perform the requested conversion; otherwise false.
		**/
		inline bool		CanDoConversionMode (const NTV2ConversionMode inMode)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_ConversionMode));
							return itms.find(ULWord(inMode)) != itms.end();
						}

		/**
			@param[in]	inMode		Specifies the NTV2DSKMode of interest.
			@returns	true if the device's Mixer/Keyer widget(s) can accommodate the requested downstream keyer mode; otherwise false.
		**/
		inline bool		CanDoDSKMode (const NTV2DSKMode inMode)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_DSKMode));
							return itms.find(ULWord(inMode)) != itms.end();
						}

		/**
			@param[in]	inPF		Specifies the NTV2PixelFormat of interest.
			@returns	true if the device's FrameStore widget(s) can read or write the requested pixel format; otherwise false.
		**/
		inline bool		CanDoFrameBufferFormat (const NTV2PixelFormat inPF)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_PixelFormat));
							return itms.find(ULWord(inPF)) != itms.end();
						}
		inline bool		CanDoPixelFormat (const NTV2PixelFormat inPF)		{return CanDoFrameBufferFormat(inPF);}

		/**
			@param[in]	inSrc		Specifies the NTV2InputSource of interest.
			@returns	true if the device has the requested input source; otherwise false.
		**/
		inline bool		CanDoInputSource (const NTV2InputSource inSrc)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputSource));
							return itms.find(ULWord(inSrc)) != itms.end();
						}

		/**
			@param[in]	inDest		Specifies the NTV2OutputDestination of interest.
			@returns	true if the device has the requested output destination connector; otherwise false.
		**/
		inline bool		CanDoOutputDestination (const NTV2OutputDestination inDest)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputDest));
							return itms.find(ULWord(inDest)) != itms.end();
						}

		/**
			@param[in]	inVF		Specifies the NTV2VideoFormat of interest.
			@returns	true if the device's FrameStore(s) can handle the given video format; otherwise false.
		**/
		inline bool		CanDoVideoFormat (const NTV2VideoFormat inVF)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_VideoFormat));
							return itms.find(ULWord(inVF)) != itms.end();
						}

		/**
			@param[in]	inWgtID		Specifies the NTV2WidgetID of interest.
			@returns	true if the device firmware implements the given widget; otherwise false.
		**/
		inline bool		CanDoWidget (const NTV2WidgetID inWgtID)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_WidgetID));
							return itms.find(ULWord(inWgtID)) != itms.end();
						}

		/**
			@param[in]	inWgtType	Specifies the NTV2WidgetType.
			@param[in]	index0		Specifies the widget of interest as a zero-based index.
			@returns	true if the device firmware implements the given widget; otherwise false.
		**/
		bool			CanDoWidget (const NTV2WidgetType inWgtType, const UWord index0);

		/**
			@returns	the set of unique NTV2PixelFormat values supported by the device's FrameStore(s).
		**/
		inline NTV2PixelFormats	PixelFormats (void)
						{	NTV2PixelFormats result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_PixelFormat));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2PixelFormat(*it));
							return result;
						}

		/**
			@returns	the set of unique NTV2VideoFormat values supported by the device's FrameStore(s).
		**/
		inline NTV2VideoFormatSet	VideoFormats (void)
						{	NTV2VideoFormatSet result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_VideoFormat));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2VideoFormat(*it));
							return result;
						}

		/**
			@returns	the set of unique NTV2TimecodeIndex values that can be ingested and captured by the device.
		**/
		inline NTV2TCIndexes	InputTCIndexes (void)
						{	NTV2TCIndexes result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputTCIndex));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2TCIndex(*it));
							return result;
						}

		/**
			@returns	the set of unique NTV2TimecodeIndex values that can be output by the device.
		**/
		inline NTV2TCIndexes	OutputTCIndexes (void)
						{	NTV2TCIndexes result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputTCIndex));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2TCIndex(*it));
							return result;
						}

		/**
			@param[in]	inTCNdx		Specifies the NTV2TimecodeIndex of interest.
			@returns	true if the device can read the given timecode; otherwise false.
		**/
		inline bool		CanDoInputTCIndex (const NTV2TCIndex inTCNdx)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputTCIndex));
							return itms.find(ULWord(inTCNdx)) != itms.end();
						}

		/**
			@param[in]	inTCNdx		Specifies the NTV2TimecodeIndex of interest.
			@returns	true if the device can write the given timecode; otherwise false.
		**/
		inline bool		CanDoOutputTCIndex (const NTV2TCIndex inTCNdx)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputTCIndex));
							return itms.find(ULWord(inTCNdx)) != itms.end();
						}
	private:
		CNTV2DriverInterface &	dev;	///< @brief	My reference to the NTV2 device
};	//	DeviceCapabilities

#endif	//	NTV2_DEVICECAPABILITIES_H
