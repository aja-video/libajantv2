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
**/
class AJAExport DeviceCapabilities
{
	public:
		explicit inline	DeviceCapabilities (CNTV2DriverInterface & inDev)	: dev(inDev)	{}
		/**
			@returns	True if valid/open.
		**/
		inline			operator bool() const					{return dev.IsOpen();}	//	New in SDK 17.1
		inline bool		CanChangeEmbeddedAudioClock (void)		{return dev.IsSupported(kDeviceCanChangeEmbeddedAudioClock);}
		inline bool		CanChangeFrameBufferSize (void)			{return dev.IsSupported(kDeviceCanChangeFrameBufferSize);}
		inline bool		CanDisableUFC (void)					{return dev.IsSupported(kDeviceCanDisableUFC);}
		inline bool		CanDo12gRouting (void)					{return dev.IsSupported(kDeviceCanDo12gRouting);}
		inline bool		CanDo12GSDI (void)						{return dev.IsSupported(kDeviceCanDo12GSDI);}
		inline bool		CanDo2110 (void)						{return dev.IsSupported(kDeviceCanDo2110);}
		inline bool		CanDo2KVideo (void)						{return dev.IsSupported(kDeviceCanDo2KVideo);}
		inline bool		CanDo3GLevelConversion (void)			{return dev.IsSupported(kDeviceCanDo3GLevelConversion);}
		inline bool		CanDo425Mux (void)						{return dev.IsSupported(kDeviceCanDo425Mux);}
		inline bool		CanDo4KVideo (void)						{return dev.IsSupported(kDeviceCanDo4KVideo);}
		inline bool		CanDo8KVideo (void)						{return dev.IsSupported(kDeviceCanDo8KVideo);}
		inline bool		CanDoAESAudioIn (void)					{return dev.IsSupported(kDeviceCanDoAESAudioIn);}
		inline bool		CanDoAESAudioOut (void)					{return dev.IsSupported(kDeviceCanDoAESAudioOut);}
		inline bool		CanDoAnalogAudio (void)					{return dev.IsSupported(kDeviceCanDoAnalogAudio);}
		inline bool		CanDoAnalogVideoIn (void)				{return dev.IsSupported(kDeviceCanDoAnalogVideoIn);}
		inline bool		CanDoAnalogVideoOut (void)				{return dev.IsSupported(kDeviceCanDoAnalogVideoOut);}
		inline bool		CanDoAudio192K (void)					{return dev.IsSupported(kDeviceCanDoAudio192K);}
		inline bool		CanDoAudio2Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio2Channels);}
		inline bool		CanDoAudio6Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio6Channels);}
		inline bool		CanDoAudio8Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio8Channels);}
		inline bool		CanDoAudio96K (void)					{return dev.IsSupported(kDeviceCanDoAudio96K);}
		inline bool		CanDoAudioDelay (void)					{return dev.IsSupported(kDeviceCanDoAudioDelay);}
		inline bool		CanDoAudioInput (void)					{return dev.IsSupported(kDeviceCanDoAudioInput);}
		inline bool		CanDoAudioMixer (void)					{return dev.IsSupported(kDeviceCanDoAudioMixer);}
		inline bool		CanDoAudioOutput (void)					{return dev.IsSupported(kDeviceCanDoAudioOutput);}
		inline bool		CanDoBreakoutBoard (void)				{return dev.IsSupported(kDeviceCanDoBreakoutBoard);}
		inline bool		CanDoBreakoutBox (void)					{return dev.IsSupported(kDeviceCanDoBreakoutBox);}
		inline bool		CanDoCapture (void)						{return dev.IsSupported(kDeviceCanDoCapture);}
		inline bool		CanDoColorCorrection (void)				{return dev.IsSupported(kDeviceCanDoColorCorrection);}
		inline bool		CanDoCustomAnc (void)					{return dev.IsSupported(kDeviceCanDoCustomAnc);}
		inline bool		CanDoCustomHancInsertion (void)			{return dev.IsSupported(kDeviceCanDoCustomHancInsertion);}
		inline bool		CanDoDSKOpacity (void)					{return dev.IsSupported(kDeviceCanDoDSKOpacity);}
		inline bool		CanDoDualLink (void)					{return dev.IsSupported(kDeviceCanDoDualLink);}
		inline bool		CanDoDVCProHD (void)					{return dev.IsSupported(kDeviceCanDoDVCProHD);}
		inline bool		CanDoEnhancedCSC (void)					{return dev.IsSupported(kDeviceCanDoEnhancedCSC);}
		inline bool		CanDoFramePulseSelect (void)			{return dev.IsSupported(kDeviceCanDoFramePulseSelect);}
		inline bool		CanDoFrameStore1Display (void)			{return dev.IsSupported(kDeviceCanDoFrameStore1Display);}
		inline bool		CanDoHDMIAuxCapture (void)				{return dev.IsSupported(kDeviceCanDoHDMIAuxCapture);}
		inline bool		CanDoHDMIAuxPlayback (void)				{return dev.IsSupported(kDeviceCanDoHDMIAuxPlayback);}
		inline bool		CanDoHDMIHDROut (void)					{return dev.IsSupported(kDeviceCanDoHDMIHDROut);}
		inline bool		CanDoHDMIMultiView (void)				{return dev.IsSupported(kDeviceCanDoHDMIMultiView);}
		inline bool		CanDoHDMIOutStereo (void)				{return dev.IsSupported(kDeviceCanDoHDMIOutStereo);}
		inline bool		CanDoHDMIQuadRasterConversion (void)	{return dev.IsSupported(kDeviceCanDoHDMIQuadRasterConversion);}
		inline bool		CanDoHDV (void)							{return dev.IsSupported(kDeviceCanDoHDV);}
		inline bool		CanDoHDVideo (void)						{return dev.IsSupported(kDeviceCanDoHDVideo);}
		inline bool		CanDoHFRRGB (void)						{return dev.IsSupported(kDeviceCanDoHFRRGB);}
		inline bool		CanDoIP (void)							{return dev.IsSupported(kDeviceCanDoIP);}
		inline bool		CanDoIsoConvert (void)					{return dev.IsSupported(kDeviceCanDoIsoConvert);}
		inline bool		CanDoJ2K (void)							{return dev.IsSupported(kDeviceCanDoJ2K);}
		inline bool		CanDoLTC (void)							{return dev.IsSupported(kDeviceCanDoLTC);}
		inline bool		CanDoLTCInOnRefPort (void)				{return dev.IsSupported(kDeviceCanDoLTCInOnRefPort);}
		inline bool		CanDoMSI (void)							{return dev.IsSupported(kDeviceCanDoMSI);}
		inline bool		CanDoMultiFormat (void)					{return dev.IsSupported(kDeviceCanDoMultiFormat);}
		inline bool		CanDoMultiLinkAudio (void)				{return dev.IsSupported(kDeviceCanDoMultiLinkAudio);}
		inline bool		CanDoPCMControl (void)					{return dev.IsSupported(kDeviceCanDoPCMControl);}
		inline bool		CanDoPCMDetection (void)				{return dev.IsSupported(kDeviceCanDoPCMDetection);}
		inline bool		CanDoPIO (void)							{return dev.IsSupported(kDeviceCanDoPIO);}
		inline bool		CanDoPlayback (void)					{return dev.IsSupported(kDeviceCanDoPlayback);}
		inline bool		CanDoProgrammableRS422 (void)			{return dev.IsSupported(kDeviceCanDoProgrammableRS422);}
		inline bool		CanDoProRes (void)						{return dev.IsSupported(kDeviceCanDoProRes);}
		inline bool		CanDoQREZ (void)						{return dev.IsSupported(kDeviceCanDoQREZ);}
		inline bool		CanDoQuarterExpand (void)				{return dev.IsSupported(kDeviceCanDoQuarterExpand);}
		inline bool		CanDoRateConvert (void)					{return dev.IsSupported(kDeviceCanDoRateConvert);}
		inline bool		CanDoRGBLevelAConversion (void)			{return dev.IsSupported(kDeviceCanDoRGBLevelAConversion);}
		inline bool		CanDoRGBPlusAlphaOut (void)				{return dev.IsSupported(kDeviceCanDoRGBPlusAlphaOut);}
		inline bool		CanDoRP188 (void)						{return dev.IsSupported(kDeviceCanDoRP188);}
		inline bool		CanDoSDIErrorChecks (void)				{return dev.IsSupported(kDeviceCanDoSDIErrorChecks);}
		inline bool		CanDoSDVideo (void)						{return dev.IsSupported(kDeviceCanDoSDVideo);}
		inline bool		CanDoStackedAudio (void)				{return dev.IsSupported(kDeviceCanDoStackedAudio);}
		inline bool		CanDoStereoIn (void)					{return dev.IsSupported(kDeviceCanDoStereoIn);}
		inline bool		CanDoStereoOut (void)					{return dev.IsSupported(kDeviceCanDoStereoOut);}
		inline bool		CanDoStreamingDMA (void)				{return dev.IsSupported(kDeviceCanDoStreamingDMA);}
		inline bool		CanDoThunderbolt (void)					{return dev.IsSupported(kDeviceCanDoThunderbolt);}
		inline bool		CanDoVideoProcessing (void)				{return dev.IsSupported(kDeviceCanDoVideoProcessing);}
		inline bool		CanDoVITC2 (void)						{return dev.IsSupported(kDeviceCanDoVITC2);}
		inline bool		CanDoWarmBootFPGA (void)				{return dev.IsSupported(kDeviceCanDoWarmBootFPGA);}
		inline bool		CanMeasureTemperature (void)			{return dev.IsSupported(kDeviceCanMeasureTemperature);}
		inline bool		CanReportFailSafeLoaded (void)			{return dev.IsSupported(kDeviceCanReportFailSafeLoaded);}
		inline bool		CanReportFrameSize (void)				{return dev.IsSupported(kDeviceCanReportFrameSize);}
		inline bool		CanReportRunningFirmwareDate (void)		{return dev.IsSupported(kDeviceCanReportRunningFirmwareDate);}
		inline bool		CanThermostat (void)					{return dev.IsSupported(kDeviceCanThermostat);}
		inline bool		HasAudioMonitorRCAJacks (void)			{return dev.IsSupported(kDeviceHasAudioMonitorRCAJacks);}
		inline bool		HasBiDirectionalAnalogAudio (void)		{return dev.IsSupported(kDeviceHasBiDirectionalAnalogAudio);}
		inline bool		HasBiDirectionalSDI (void)				{return dev.IsSupported(kDeviceHasBiDirectionalSDI);}
		inline bool		HasBreakoutBoard (void)					{return dev.IsSupported(kDeviceHasBreakoutBoard);}
		inline bool		HasColorSpaceConverterOnChannel2 (void)	{return dev.IsSupported(kDeviceHasColorSpaceConverterOnChannel2);}
		inline bool		HasCrosspointConnectROM (void)			{return dev.IsSupported(kDeviceHasXptConnectROM);}
		inline bool		HasGenlockv2 (void)						{return dev.IsSupported(kDeviceHasGenlockv2);}
		inline bool		HasGenlockv3 (void)						{return dev.IsSupported(kDeviceHasGenlockv3);}
		inline bool		HasHeadphoneJack (void)					{return dev.IsSupported(kDeviceHasHeadphoneJack);}
		inline bool		HasHEVCM30 (void)						{return dev.IsSupported(kDeviceHasHEVCM30);}
		inline bool		HasHEVCM31 (void)						{return dev.IsSupported(kDeviceHasHEVCM31);}
		inline bool		HasIDSwitch (void)						{return dev.IsSupported(kDeviceHasIDSwitch);}
		inline bool		HasLEDAudioMeters (void)				{return dev.IsSupported(kDeviceHasLEDAudioMeters);}
		inline bool		HasMicInput (void)						{return dev.IsSupported(kDeviceHasMicrophoneInput);}
		inline bool		HasNTV4FrameStores (void)				{return dev.IsSupported(kDeviceHasNTV4FrameStores);}
		inline bool		HasNWL (void)							{return dev.IsSupported(kDeviceHasNWL);}
		inline bool		HasPCIeGen2 (void)						{return dev.IsSupported(kDeviceHasPCIeGen2);}
		inline bool		HasRetailSupport (void)					{return dev.IsSupported(kDeviceHasRetailSupport);}
		inline bool		HasRotaryEncoder (void)					{return dev.IsSupported(kDeviceHasRotaryEncoder);}
		inline bool		HasSDIRelays (void)						{return dev.IsSupported(kDeviceHasSDIRelays);}
		inline bool		HasSPIFlash (void)						{return dev.IsSupported(kDeviceHasSPIFlash);}
		inline bool		HasSPIFlashSerial (void)				{return dev.IsSupported(kDeviceHasSPIFlashSerial);}
		inline bool		HasSPIv2 (void)							{return dev.IsSupported(kDeviceHasSPIv2);}
		inline bool		HasSPIv3 (void)							{return dev.IsSupported(kDeviceHasSPIv3);}
		inline bool		HasSPIv4 (void)							{return dev.IsSupported(kDeviceHasSPIv4);}
		inline bool		HasSPIv5 (void)							{return dev.IsSupported(kDeviceHasSPIv5);}
		inline bool		HasXilinxDMA (void)						{return dev.IsSupported(kDeviceHasXilinxDMA);}
		inline bool		Is64Bit (void)							{return dev.IsSupported(kDeviceIs64Bit);}
		inline bool		IsDirectAddressable (void)				{return dev.IsSupported(kDeviceIsDirectAddressable);}
		inline bool		IsDNxIV (void)							{return dev.IsSupported(kDeviceHasMicrophoneInput);}
		inline bool		IsExternalToHost (void)					{return dev.IsSupported(kDeviceIsExternalToHost);}
		inline bool		IsSupported (void)						{return dev.IsSupported(kDeviceIsSupported);}
		inline bool		NeedsRoutingSetup (void)				{return dev.IsSupported(kDeviceNeedsRoutingSetup);}
		inline bool		SoftwareCanChangeFrameBufferSize (void)	{return dev.IsSupported(kDeviceSoftwareCanChangeFrameBufferSize);}
		inline ULWord	GetActiveMemorySize (void)				{return dev.GetNumSupported(kDeviceGetActiveMemorySize);}
		inline UWord	GetDACVersion (void)					{return UWord(dev.GetNumSupported(kDeviceGetDACVersion));}
		inline UWord	GetDownConverterDelay (void)			{return UWord(dev.GetNumSupported(kDeviceGetDownConverterDelay));}
		inline ULWord	GetHDMIVersion (void)					{return dev.GetNumSupported(kDeviceGetHDMIVersion);}
		inline ULWord	GetLUTVersion (void)					{return dev.GetNumSupported(kDeviceGetLUTVersion);}
		inline ULWord	GetSPIFlashVersion (void)				{return dev.GetNumSupported(kDeviceGetSPIFlashVersion);}
		inline UWord	GetMaxAudioChannels (void)				{return UWord(dev.GetNumSupported(kDeviceGetMaxAudioChannels));}
		inline ULWord	GetMaxRegisterNumber (void)				{return dev.GetNumSupported(kDeviceGetMaxRegisterNumber);}
		inline ULWord	GetMaxTransferCount (void)				{return dev.GetNumSupported(kDeviceGetMaxTransferCount);}
		inline UWord	GetNum2022ChannelsSFP1 (void)			{return UWord(dev.GetNumSupported(kDeviceGetNum2022ChannelsSFP1));}
		inline UWord	GetNum2022ChannelsSFP2 (void)			{return UWord(dev.GetNumSupported(kDeviceGetNum2022ChannelsSFP2));}
		inline UWord	GetNum4kQuarterSizeConverters (void)	{return UWord(dev.GetNumSupported(kDeviceGetNum4kQuarterSizeConverters));}
		inline UWord	GetNumAESAudioInputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumAESAudioInputChannels));}
		inline UWord	GetNumAESAudioOutputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumAESAudioOutputChannels));}
		inline UWord	GetNumAnalogAudioInputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogAudioInputChannels));}
		inline UWord	GetNumAnalogAudioOutputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogAudioOutputChannels));}
		inline UWord	GetNumAnalogVideoInputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogVideoInputs));}
		inline UWord	GetNumAnalogVideoOutputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumAnalogVideoOutputs));}
		inline UWord	GetNumAudioSystems (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumAudioSystems));}
		inline UWord	GetNumCrossConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumCrossConverters));}
		inline UWord	GetNumCSCs (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumCSCs));}
		inline ULWord	GetNumDMAEngines (void)					{return dev.GetNumSupported(kDeviceGetNumDMAEngines);}
		inline UWord	GetNumDownConverters (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumDownConverters));}
		inline UWord	GetNumEmbeddedAudioInputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumEmbeddedAudioInputChannels));}
		inline UWord	GetNumEmbeddedAudioOutputChannels (void){return UWord(dev.GetNumSupported(kDeviceGetNumEmbeddedAudioOutputChannels));}
		inline UWord	GetNumFrameStores (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumFrameStores));}
		inline UWord	GetNumFrameSyncs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumFrameSyncs));}
		inline UWord	GetNumHDMIAudioInputChannels (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIAudioInputChannels));}
		inline UWord	GetNumHDMIAudioOutputChannels (void)	{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIAudioOutputChannels));}
		inline UWord	GetNumHDMIVideoInputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIVideoInputs));}
		inline UWord	GetNumHDMIVideoOutputs (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumHDMIVideoOutputs));}
		inline UWord	GetNumInputConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumInputConverters));}
		inline UWord	GetNumLTCInputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLTCInputs));}
		inline UWord	GetNumLTCOutputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLTCOutputs));}
		inline UWord	GetNumLUTBanks (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumLUTBanks));}
		inline UWord	GetNumLUTs (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumLUTs));}
		inline UWord	GetNumMicInputs (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumMicInputs));}
		inline UWord	GetNumMixers (void)						{return UWord(dev.GetNumSupported(kDeviceGetNumMixers));}
		inline UWord	GetNumOutputConverters (void)			{return UWord(dev.GetNumSupported(kDeviceGetNumOutputConverters));}
		inline UWord	GetNumReferenceVideoInputs (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumReferenceVideoInputs));}
		inline UWord	GetNumSerialPorts (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumSerialPorts));}
		inline UWord	GetNumTSIMuxers (void)					{return UWord(dev.GetNumSupported(kDeviceGetNumTSIMuxers));}
		inline UWord	GetNumUpConverters (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumUpConverters));}
		inline ULWord	GetNumVideoChannels (void)				{return dev.GetNumSupported(kDeviceGetNumVideoChannels);}
		inline UWord	GetNumVideoInputs (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumVideoInputs));}
		inline UWord	GetNumVideoOutputs (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumVideoOutputs));}
		inline ULWord	GetPingLED (void)						{return dev.GetNumSupported(kDeviceGetPingLED);}
		inline UWord	GetTotalNumAudioSystems (void)			{return UWord(dev.GetNumSupported(kDeviceGetTotalNumAudioSystems));}
		inline UWord	GetNumBufferedAudioSystems (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumBufferedAudioSystems));}
		inline ULWord	GetUFCVersion (void)					{return dev.GetNumSupported(kDeviceGetUFCVersion);}
		inline bool		CanDoChannel (const NTV2Channel inChannel)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_Channel));
							return itms.find(ULWord(inChannel)) != itms.end();
						}
		inline bool		CanDoConversionMode (const NTV2ConversionMode inMode)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_ConversionMode));
							return itms.find(ULWord(inMode)) != itms.end();
						}
		inline bool		CanDoDSKMode (const NTV2DSKMode inMode)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_DSKMode));
							return itms.find(ULWord(inMode)) != itms.end();
						}
		inline bool		CanDoFrameBufferFormat (const NTV2PixelFormat inPF)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_PixelFormat));
							return itms.find(ULWord(inPF)) != itms.end();
						}
		inline bool		CanDoPixelFormat (const NTV2PixelFormat inPF)		{return CanDoFrameBufferFormat(inPF);}
		inline bool		CanDoInputSource (const NTV2InputSource inSrc)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputSource));
							return itms.find(ULWord(inSrc)) != itms.end();
						}
		inline bool		CanDoOutputDestination (const NTV2OutputDestination inDest)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputDest));
							return itms.find(ULWord(inDest)) != itms.end();
						}
		inline bool		CanDoVideoFormat (const NTV2VideoFormat inVF)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_VideoFormat));
							return itms.find(ULWord(inVF)) != itms.end();
						}
		inline bool		CanDoWidget (const NTV2WidgetID inWgtID)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_WidgetID));
							return itms.find(ULWord(inWgtID)) != itms.end();
						}
		bool			CanDoWidget (const NTV2WidgetType inWgtType, const UWord index0);
		inline NTV2PixelFormats	PixelFormats (void)
						{	NTV2PixelFormats result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_PixelFormat));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2PixelFormat(*it));
							return result;
						}
		inline NTV2VideoFormatSet	VideoFormats (void)
						{	NTV2VideoFormatSet result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_VideoFormat));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2VideoFormat(*it));
							return result;
						}
		inline NTV2TCIndexes	InputTCIndexes (void)
						{	NTV2TCIndexes result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputTCIndex));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2TCIndex(*it));
							return result;
						}
		inline NTV2TCIndexes	OutputTCIndexes (void)
						{	NTV2TCIndexes result;
							const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputTCIndex));
							for (ULWordSetConstIter it(itms.begin());  it != itms.end();  ++it)
								result.insert(NTV2TCIndex(*it));
							return result;
						}
		inline bool		CanDoInputTCIndex (const NTV2TCIndex inTCNdx)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputTCIndex));
							return itms.find(ULWord(inTCNdx)) != itms.end();
						}
		inline bool		CanDoOutputTCIndex (const NTV2TCIndex inTCNdx)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputTCIndex));
							return itms.find(ULWord(inTCNdx)) != itms.end();
						}
	private:
		CNTV2DriverInterface &	dev;	//	My reference to the physical or virtual NTV2 device
};	//	DeviceCapabilities

#endif	//	NTV2_DEVICECAPABILITIES_H
