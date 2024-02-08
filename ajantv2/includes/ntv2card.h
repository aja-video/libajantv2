/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2card.h
	@brief		Declares the CNTV2Card class.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2CARD_H
#define NTV2CARD_H

#if defined (MSWindows)
	#include "ntv2windriverinterface.h"
#elif defined (AJAMac)
	#include "ntv2macdriverinterface.h"
#elif defined (AJALinux)
	#include "ntv2linuxdriverinterface.h"
#elif defined (AJABareMetal)
  #include "ntv2baremetaldriverinterface.h"
#endif
#include "ntv2signalrouter.h"
#include "ntv2utils.h"
#include <set>
#include <string>
#include <iostream>
#include <vector>
#include <bitset>


typedef std::set <NTV2AudioChannelPair>			NTV2AudioChannelPairs;			///< @brief A set of distinct NTV2AudioChannelPair values.
typedef NTV2AudioChannelPairs::const_iterator	NTV2AudioChannelPairsConstIter; ///< @brief Handy const iterator to iterate over a set of distinct NTV2AudioChannelPair values.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioChannelPairs & inSet); ///<	@brief	Handy ostream writer for NTV2AudioChannelPairs.

typedef std::set <NTV2AudioChannelQuad>			NTV2AudioChannelQuads;			///< @brief A set of distinct NTV2AudioChannelQuad values.
typedef NTV2AudioChannelQuads::const_iterator	NTV2AudioChannelQuadsConstIter; ///< @brief Handy const iterator to iterate over a set of distinct NTV2AudioChannelQuad values.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioChannelQuads & inSet); ///<	@brief	Handy ostream writer for NTV2AudioChannelQuads.

typedef std::set <NTV2AudioChannelOctet>		NTV2AudioChannelOctets;			///< @brief A set of distinct NTV2AudioChannelOctet values.
typedef NTV2AudioChannelOctets::const_iterator	NTV2AudioChannelOctetsConstIter;///< @brief Handy const iterator to iterate over a set of distinct NTV2AudioChannelOctet values.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioChannelOctets & inSet); ///<	@brief	Handy ostream writer for NTV2AudioChannelOctets.

typedef std::vector <double>					NTV2DoubleArray;				///< @brief An array of double-precision floating-point values.
typedef NTV2DoubleArray::iterator				NTV2DoubleArrayIter;			///< @brief Handy non-const iterator to iterate over an NTV2DoubleArray.
typedef NTV2DoubleArray::const_iterator			NTV2DoubleArrayConstIter;		///< @brief Handy const iterator to iterate over an NTV2DoubleArray.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2DoubleArray & inVector);	///<	@brief	Handy ostream writer for NTV2DoubleArray.

typedef UByte						NTV2DID;				///< @brief An ancillary Data IDentifier.
typedef std::set <UByte>			NTV2DIDSet;				///< @brief A set of distinct NTV2DID values.
typedef NTV2DIDSet::iterator		NTV2DIDSetIter;			///< @brief Handy non-const iterator to iterate over an NTV2DIDSet.
typedef NTV2DIDSet::const_iterator	NTV2DIDSetConstIter;	///< @brief Handy const iterator to iterate over an NTV2DIDSet.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2DIDSet & inDIDs);	///<	@brief	Handy ostream writer for NTV2DIDSet.


typedef std::bitset<16>		NTV2AudioChannelsMuted16;		///< @brief Per-audio-channel mute state for up to 16 audio channels.
const NTV2AudioChannelsMuted16	NTV2AudioChannelsMuteAll = NTV2AudioChannelsMuted16(0xFFFF);	///< @brief All 16 audio channels muted/disabled.
const NTV2AudioChannelsMuted16	NTV2AudioChannelsEnableAll = NTV2AudioChannelsMuted16(0x0000);	///< @brief All 16 audio channels unmuted/enabled.
const ULWord LUTTablePartitionSize = ULWord(0x40000);

#if defined(NTV2_INCLUDE_DEVICE_CAPABILITIES_API)
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
		inline bool	CanChangeEmbeddedAudioClock (void)		{return dev.IsSupported(kDeviceCanChangeEmbeddedAudioClock);}
		inline bool	CanChangeFrameBufferSize (void)			{return dev.IsSupported(kDeviceCanChangeFrameBufferSize);}
		inline bool	CanDisableUFC (void)					{return dev.IsSupported(kDeviceCanDisableUFC);}
		inline bool	CanDo12gRouting (void)					{return dev.IsSupported(kDeviceCanDo12gRouting);}
		inline bool	CanDo12GSDI (void)						{return dev.IsSupported(kDeviceCanDo12GSDI);}
		inline bool	CanDo2110 (void)						{return dev.IsSupported(kDeviceCanDo2110);}
		inline bool	CanDo2KVideo (void)						{return dev.IsSupported(kDeviceCanDo2KVideo);}
		inline bool	CanDo3GLevelConversion (void)			{return dev.IsSupported(kDeviceCanDo3GLevelConversion);}
		inline bool	CanDo425Mux (void)						{return dev.IsSupported(kDeviceCanDo425Mux);}
		inline bool	CanDo4KVideo (void)						{return dev.IsSupported(kDeviceCanDo4KVideo);}
		inline bool	CanDo8KVideo (void)						{return dev.IsSupported(kDeviceCanDo8KVideo);}
		inline bool	CanDoAESAudioIn (void)					{return dev.IsSupported(kDeviceCanDoAESAudioIn);}
		inline bool	CanDoAnalogAudio (void)					{return dev.IsSupported(kDeviceCanDoAnalogAudio);}
		inline bool	CanDoAnalogVideoIn (void)				{return dev.IsSupported(kDeviceCanDoAnalogVideoIn);}
		inline bool	CanDoAnalogVideoOut (void)				{return dev.IsSupported(kDeviceCanDoAnalogVideoOut);}
		inline bool	CanDoAudio192K (void)					{return dev.IsSupported(kDeviceCanDoAudio192K);}
		inline bool	CanDoAudio2Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio2Channels);}
		inline bool	CanDoAudio6Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio6Channels);}
		inline bool	CanDoAudio8Channels (void)				{return dev.IsSupported(kDeviceCanDoAudio8Channels);}
		inline bool	CanDoAudio96K (void)					{return dev.IsSupported(kDeviceCanDoAudio96K);}
		inline bool	CanDoAudioDelay (void)					{return dev.IsSupported(kDeviceCanDoAudioDelay);}
		inline bool	CanDoAudioMixer (void)					{return dev.IsSupported(kDeviceCanDoAudioMixer);}
		inline bool	CanDoBreakoutBox (void)					{return dev.IsSupported(kDeviceCanDoBreakoutBox);}
		inline bool	CanDoCapture (void)						{return dev.IsSupported(kDeviceCanDoCapture);}
		inline bool	CanDoCustomAnc (void)					{return dev.IsSupported(kDeviceCanDoCustomAnc);}
		inline bool	CanDoCustomAux (void)					{return dev.IsSupported(kDeviceCanDoCustomAux);}
		inline bool	CanDoDSKOpacity (void)					{return dev.IsSupported(kDeviceCanDoDSKOpacity);}
		inline bool	CanDoDualLink (void)					{return dev.IsSupported(kDeviceCanDoDualLink);}
		inline bool	CanDoDVCProHD (void)					{return dev.IsSupported(kDeviceCanDoDVCProHD);}
		inline bool	CanDoEnhancedCSC (void)					{return dev.IsSupported(kDeviceCanDoEnhancedCSC);}
		inline bool	CanDoFramePulseSelect (void)			{return dev.IsSupported(kDeviceCanDoFramePulseSelect);}
		inline bool	CanDoFrameStore1Display (void)			{return dev.IsSupported(kDeviceCanDoFrameStore1Display);}
		inline bool	CanDoHDMIHDROut (void)					{return dev.IsSupported(kDeviceCanDoHDMIHDROut);}
		inline bool	CanDoHDMIMultiView (void)				{return dev.IsSupported(kDeviceCanDoHDMIMultiView);}
		inline bool	CanDoHDMIOutStereo (void)				{return dev.IsSupported(kDeviceCanDoHDMIOutStereo);}
		inline bool	CanDoHDV (void)							{return dev.IsSupported(kDeviceCanDoHDV);}
		inline bool	CanDoHDVideo (void)						{return dev.IsSupported(kDeviceCanDoHDVideo);}
		inline bool	CanDoHFRRGB (void)						{return dev.IsSupported(kDeviceCanDoHFRRGB);}
		inline bool	CanDoIP (void)							{return dev.IsSupported(kDeviceCanDoIP);}
		inline bool	CanDoIsoConvert (void)					{return dev.IsSupported(kDeviceCanDoIsoConvert);}
		inline bool	CanDoJ2K (void)							{return dev.IsSupported(kDeviceCanDoJ2K);}
		inline bool	CanDoLTC (void)							{return dev.IsSupported(kDeviceCanDoLTC);}
		inline bool	CanDoLTCInOnRefPort (void)				{return dev.IsSupported(kDeviceCanDoLTCInOnRefPort);}
		inline bool	CanDoMSI (void)							{return dev.IsSupported(kDeviceCanDoMSI);}
		inline bool	CanDoMultiFormat (void)					{return dev.IsSupported(kDeviceCanDoMultiFormat);}
		inline bool	CanDoMultiLinkAudio (void)				{return dev.IsSupported(kDeviceCanDoMultiLinkAudio);}
		inline bool	CanDoPCMControl (void)					{return dev.IsSupported(kDeviceCanDoPCMControl);}
		inline bool	CanDoPCMDetection (void)				{return dev.IsSupported(kDeviceCanDoPCMDetection);}
		inline bool	CanDoPIO (void)							{return dev.IsSupported(kDeviceCanDoPIO);}
		inline bool	CanDoPlayback (void)					{return dev.IsSupported(kDeviceCanDoPlayback);}
		inline bool	CanDoProgrammableRS422 (void)			{return dev.IsSupported(kDeviceCanDoProgrammableRS422);}
		inline bool	CanDoProRes (void)						{return dev.IsSupported(kDeviceCanDoProRes);}
		inline bool	CanDoQREZ (void)						{return dev.IsSupported(kDeviceCanDoQREZ);}
		inline bool	CanDoQuarterExpand (void)				{return dev.IsSupported(kDeviceCanDoQuarterExpand);}
		inline bool	CanDoRateConvert (void)					{return dev.IsSupported(kDeviceCanDoRateConvert);}
		inline bool	CanDoRGBLevelAConversion (void)			{return dev.IsSupported(kDeviceCanDoRGBLevelAConversion);}
		inline bool	CanDoRGBPlusAlphaOut (void)				{return dev.IsSupported(kDeviceCanDoRGBPlusAlphaOut);}
		inline bool	CanDoRP188 (void)						{return dev.IsSupported(kDeviceCanDoRP188);}
		inline bool	CanDoSDIErrorChecks (void)				{return dev.IsSupported(kDeviceCanDoSDIErrorChecks);}
		inline bool	CanDoSDVideo (void)						{return dev.IsSupported(kDeviceCanDoSDVideo);}
		inline bool	CanDoStackedAudio (void)				{return dev.IsSupported(kDeviceCanDoStackedAudio);}
		inline bool	CanDoStereoIn (void)					{return dev.IsSupported(kDeviceCanDoStereoIn);}
		inline bool	CanDoStereoOut (void)					{return dev.IsSupported(kDeviceCanDoStereoOut);}
		inline bool	CanDoThunderbolt (void)					{return dev.IsSupported(kDeviceCanDoThunderbolt);}
		inline bool	CanDoVideoProcessing (void)				{return dev.IsSupported(kDeviceCanDoVideoProcessing);}
		inline bool	CanDoVITC2 (void)						{return dev.IsSupported(kDeviceCanDoVITC2);}
		inline bool	CanDoWarmBootFPGA (void)				{return dev.IsSupported(kDeviceCanDoWarmBootFPGA);}
		inline bool	CanMeasureTemperature (void)			{return dev.IsSupported(kDeviceCanMeasureTemperature);}
		inline bool	CanReportFailSafeLoaded (void)			{return dev.IsSupported(kDeviceCanReportFailSafeLoaded);}
		inline bool	CanReportFrameSize (void)				{return dev.IsSupported(kDeviceCanReportFrameSize);}
		inline bool	CanReportRunningFirmwareDate (void)		{return dev.IsSupported(kDeviceCanReportRunningFirmwareDate);}
		inline bool	CanThermostat (void)					{return dev.IsSupported(kDeviceCanThermostat);}
		inline bool	HasAudioMonitorRCAJacks (void)			{return dev.IsSupported(kDeviceHasAudioMonitorRCAJacks);}
		inline bool	HasBiDirectionalAnalogAudio (void)		{return dev.IsSupported(kDeviceHasBiDirectionalAnalogAudio);}
		inline bool	HasBiDirectionalSDI (void)				{return dev.IsSupported(kDeviceHasBiDirectionalSDI);}
		inline bool	HasColorSpaceConverterOnChannel2 (void)	{return dev.IsSupported(kDeviceHasColorSpaceConverterOnChannel2);}
		inline bool	HasGenlockv2 (void)						{return dev.IsSupported(kDeviceHasGenlockv2);}
		inline bool	HasGenlockv3 (void)						{return dev.IsSupported(kDeviceHasGenlockv3);}
		inline bool	HasHeadphoneJack (void)					{return dev.IsSupported(kDeviceHasHeadphoneJack);}
		inline bool	HasHEVCM30 (void)						{return dev.IsSupported(kDeviceHasHEVCM30);}
		inline bool	HasHEVCM31 (void)						{return dev.IsSupported(kDeviceHasHEVCM31);}
		inline bool	HasLEDAudioMeters (void)				{return dev.IsSupported(kDeviceHasLEDAudioMeters);}
		inline bool	HasMicInput (void)						{return dev.IsSupported(kDeviceHasMicrophoneInput);}
		inline bool	HasNWL (void)							{return dev.IsSupported(kDeviceHasNWL);}
		inline bool	HasPCIeGen2 (void)						{return dev.IsSupported(kDeviceHasPCIeGen2);}
		inline bool	HasRetailSupport (void)					{return dev.IsSupported(kDeviceHasRetailSupport);}
		inline bool	HasRotaryEncoder (void)					{return dev.IsSupported(kDeviceHasRotaryEncoder);}
		inline bool	HasSDIRelays (void)						{return dev.IsSupported(kDeviceHasSDIRelays);}
		inline bool	HasSPIFlash (void)						{return dev.IsSupported(kDeviceHasSPIFlash);}
		inline bool	HasSPIFlashSerial (void)				{return dev.IsSupported(kDeviceHasSPIFlashSerial);}
		inline bool	HasSPIv2 (void)							{return dev.IsSupported(kDeviceHasSPIv2);}
		inline bool	HasSPIv3 (void)							{return dev.IsSupported(kDeviceHasSPIv3);}
		inline bool	HasSPIv4 (void)							{return dev.IsSupported(kDeviceHasSPIv4);}
		inline bool	HasSPIv5 (void)							{return dev.IsSupported(kDeviceHasSPIv5);}
		inline bool	HasXilinxDMA (void)						{return dev.IsSupported(kDeviceHasXilinxDMA);}
		inline bool	Is64Bit (void)							{return dev.IsSupported(kDeviceIs64Bit);}
		inline bool	IsDirectAddressable (void)				{return dev.IsSupported(kDeviceIsDirectAddressable);}
		inline bool	IsDNxIV (void)							{return dev.IsSupported(kDeviceHasMicrophoneInput);}
		inline bool	IsExternalToHost (void)					{return dev.IsSupported(kDeviceIsExternalToHost);}
		inline bool	IsSupported (void)						{return dev.IsSupported(kDeviceIsSupported);}
		inline bool	NeedsRoutingSetup (void)				{return dev.IsSupported(kDeviceNeedsRoutingSetup);}
		inline bool	SoftwareCanChangeFrameBufferSize (void)	{return dev.IsSupported(kDeviceSoftwareCanChangeFrameBufferSize);}
		inline ULWord	GetActiveMemorySize (void)				{return dev.GetNumSupported(kDeviceGetActiveMemorySize);}
		inline UWord	GetDACVersion (void)					{return UWord(dev.GetNumSupported(kDeviceGetDACVersion));}
		inline UWord	GetDownConverterDelay (void)			{return UWord(dev.GetNumSupported(kDeviceGetDownConverterDelay));}
		inline ULWord	GetHDMIVersion (void)					{return dev.GetNumSupported(kDeviceGetHDMIVersion);}
		inline ULWord	GetLUTVersion (void)					{return dev.GetNumSupported(kDeviceGetLUTVersion);}
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
		inline UWord	GetNumUpConverters (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumUpConverters));}
		inline ULWord	GetNumVideoChannels (void)				{return dev.GetNumSupported(kDeviceGetNumVideoChannels);}
		inline UWord	GetNumVideoInputs (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumVideoInputs));}
		inline UWord	GetNumVideoOutputs (void)				{return UWord(dev.GetNumSupported(kDeviceGetNumVideoOutputs));}
		inline ULWord	GetPingLED (void)						{return dev.GetNumSupported(kDeviceGetPingLED);}
		inline UWord	GetTotalNumAudioSystems (void)			{return UWord(dev.GetNumSupported(kDeviceGetTotalNumAudioSystems));}
		inline UWord	GetNumBufferedAudioSystems (void)		{return UWord(dev.GetNumSupported(kDeviceGetNumBufferedAudioSystems));}
		inline ULWord	GetUFCVersion (void)					{return dev.GetNumSupported(kDeviceGetUFCVersion);}
		bool			CanDoChannel (const NTV2Channel inChannel)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_Channel));
							return itms.find(ULWord(inChannel)) != itms.end();
						}
		bool			CanDoConversionMode (const NTV2ConversionMode inMode)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_ConversionMode));
							return itms.find(ULWord(inMode)) != itms.end();
						}
		bool			CanDoDSKMode (const NTV2DSKMode inMode)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_DSKMode));
							return itms.find(ULWord(inMode)) != itms.end();
						}
		bool			CanDoFrameBufferFormat (const NTV2PixelFormat inPF)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_PixelFormat));
							return itms.find(ULWord(inPF)) != itms.end();
						}
		bool			CanDoInputSource (const NTV2InputSource inSrc)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_InputSource));
							return itms.find(ULWord(inSrc)) != itms.end();
						}
		bool			CanDoOutputDestination (const NTV2OutputDestination inDest)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_OutputDest));
							return itms.find(ULWord(inDest)) != itms.end();
						}
		bool			CanDoVideoFormat (const NTV2VideoFormat inVF)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_VideoFormat));
							return itms.find(ULWord(inVF)) != itms.end();
						}
		bool			CanDoWidget (const NTV2WidgetID inWgtID)
						{	const ULWordSet itms (dev.GetSupportedItems(kNTV2EnumsID_WidgetID));
							return itms.find(ULWord(inWgtID)) != itms.end();
						}
	private:
		CNTV2DriverInterface &	dev;	//	My reference to the physical or virtual NTV2 device
};	//	DeviceCapabilities
#endif	//	defined(NTV2_INCLUDE_DEVICE_CAPABILITIES_API)

/**
	@brief	I interrogate and control an AJA video/audio capture/playout device.
**/
#if defined (MSWindows)
	class AJAExport CNTV2Card	: public CNTV2WinDriverInterface
#elif defined (AJAMac)
	class CNTV2Card				: public CNTV2MacDriverInterface
#elif defined (AJALinux)
	class CNTV2Card				: public CNTV2LinuxDriverInterface
#elif defined (AJABareMetal)
	class CNTV2Card				: public CNTV2BareMetalDriverInterface
#endif
{
public:
	/**
		@name	Construction & Destruction
	**/
	///@{
	/**
		@brief	My default constructor.
	**/
										CNTV2Card ();

	/**
		@brief	Constructor that opens the device.
		@param[in]	inDeviceIndex	A zero-based index number that identifies which device to open,
									which should be the number received from the ::CNTV2DeviceScanner.
		@param[in]	inHostName		If non-empty, must contain the name of a host that has one or more
									AJA devices. Defaults to empty string (the local host).
		@nosubgrouping
	**/
	explicit							CNTV2Card ( const UWord		inDeviceIndex,
													const std::string & inHostName		= std::string());

	/**
		@brief	My destructor.
	**/
	virtual								~CNTV2Card();
	///@}


	/**
		@name	Inquiry
	**/
	///@{
	/**
		@brief	Answers with this device's display name.
		@return A string containing this device's display name.
	**/
	AJA_VIRTUAL std::string			GetDisplayName (void);

	/**
		@brief	Answers with this device's model name.
		@return A string containing this device's model name.
	**/
	AJA_VIRTUAL std::string			GetModelName (void);

	/**
		@brief	Answers with this device's version number.
		@return This device's version number.
	**/
	AJA_VIRTUAL Word				GetDeviceVersion (void);

	/**
		@brief	Answers with this device's version number as a human-readable string.
		@return A string containing this device's version number as a human-readable string.
	**/
	AJA_VIRTUAL std::string			GetDeviceVersionString (void);

	/**
		@brief	Answers with this device's driver's version as a human-readable string.
		@return A string containing this device's driver's version as a human-readable string.
	**/
	AJA_VIRTUAL std::string			GetDriverVersionString (void);

	/**
		@brief	Answers with the individual version components of this device's driver.
		@param[out] outMajor	Receives the driver's major version number.
		@param[out] outMinor	Receives the driver's minor version number.
		@param[out] outPoint	Receives the driver's point release number.
		@param[out] outBuild	Receives the driver's build number.
		@return True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool				GetDriverVersionComponents (UWord & outMajor, UWord & outMinor, UWord & outPoint, UWord & outBuild);

	/**
		@brief	Answers with my serial number.
		@return My 64-bit serial number.
		@note	To decode this into a human-readable form, use my SerialNum64ToString class method.
	**/
	AJA_VIRTUAL uint64_t			GetSerialNumber (void);											//	From CNTV2Status

	/**
		@brief	Answers with a string that contains my human-readable serial number.
		@return True if successful (and valid);	 otherwise false.
	**/
	AJA_VIRTUAL bool				GetSerialNumberString (std::string & outSerialNumberString);	//	From CNTV2Status

	/**
		@brief	Answers with my PCI device ID.
		@param[out]		outPCIDeviceID		Receives my PCI device ID.
		@return True if successful (and valid);	 otherwise false.
	**/
	AJA_VIRTUAL bool				GetPCIDeviceID (ULWord & outPCIDeviceID);


	/**
		@return My current breakout box hardware type, if any is attached.
	**/
	AJA_VIRTUAL NTV2BreakoutType	GetBreakoutHardware (void);
	///@}

	/**
		@name	Device Features
	**/
	///@{
#if defined(NTV2_INCLUDE_DEVICE_CAPABILITIES_API)
	/**
		@return A reference to my DeviceCapabilities API, for querying my capabilities,
				even if I'm a virtual device.
	**/
	AJA_VIRTUAL inline class DeviceCapabilities & features (void)	{return mDevCap;}	//	New in SDK 17.0
#endif	//	defined(NTV2_INCLUDE_DEVICE_CAPABILITIES_API)
#if !defined(NTV2_DEPRECATE_16_3)
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoFormat (const NTV2FrameRate inFR,
														const NTV2FrameGeometry	inFG, 
														const NTV2Standard inStd));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDo3GOut (const UWord index0));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoLTCEmbeddedN (const UWord index0));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord DeviceGetFrameBufferSize(void));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord DeviceGetNumberFrameBuffers(void));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord DeviceGetAudioFrameBuffer(void));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord DeviceGetAudioFrameBuffer2(void));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord DeviceGetFrameBufferSize (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord DeviceGetNumberFrameBuffers (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord DeviceGetAudioFrameBuffer (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(ULWord DeviceGetAudioFrameBuffer2 (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoVideoFormat (const NTV2VideoFormat inVF));	///< @deprecated	Use DeviceCapabilities::CanDoVideoFormat instead.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoFrameBufferFormat (const NTV2PixelFormat inPF));	///< @deprecated	Use DeviceCapabilities::CanDoFrameBufferFormat instead.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoWidget (const NTV2WidgetID inWgtID));	///< @deprecated	Use DeviceCapabilities::CanDoWidget instead.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoConversionMode (const NTV2ConversionMode inCM));	///< @deprecated	Use DeviceCapabilities::CanDoConversionMode instead.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoDSKMode (const NTV2DSKMode inDSKM));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoInputSource (const NTV2InputSource inSrc));	///< @deprecated	Use DeviceCapabilities::CanDoInputSource instead.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoAudioMixer(void));	///< @deprecated	Use CNTV2DriverInterface::IsSupported with kDeviceCanDoAudioMixer instead.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceCanDoHDMIQuadRasterConversion(void));	///< @deprecated	This function is obsolete. Do not use it.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceIsDNxIV(void));	///< @deprecated	Use DeviceCapabilities::IsDNxIV instead.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool DeviceHasMicInput(void));	///< @deprecated	Use CNTV2DriverInterface::IsSupported with kDeviceHasMicrophoneInput instead.
#endif	//	defined(NTV2_DEPRECATE_16_3)
	///@}


	/**
		@name	DMA Transfer
	**/
	///@{
	/**
		@brief		Transfers data from the AJA device to the host.
		@param[in]	inFrameNumber	Specifies the zero-based frame index number of the starting frame to be read from the device.
									For \ref vidop-fbindexing purposes, this function assumes the intrinsic frame size of the device.
		@param[in]	pFrameBuffer	Specifies the non-NULL address of the host buffer that is to receive the frame data.
									The memory it points to must be writeable.
		@param[in]	inOffsetBytes	Specifies the byte offset into the device frame buffer.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@return		True if successful; otherwise false.
		@note		The host buffer should be at least inByteCount + inOffsetBytes in size, or host memory will be corrupted.
		@note		This function will block and not return until the transfer has finished or failed.
		@see		CNTV2Card::DMAWrite, CNTV2Card::DMAReadFrame, CNTV2Card::DMAReadSegments, \ref vidop-fbaccess
	**/
	AJA_VIRTUAL bool	DMARead (const ULWord inFrameNumber, ULWord * pFrameBuffer, const ULWord inOffsetBytes, const ULWord inByteCount);

	/**
		@brief		Transfers data from the host to the AJA device.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame to be written on the device.
									For \ref vidop-fbindexing purposes, this function assumes the intrinsic frame size of the device.
		@param[in]	pFrameBuffer	Specifies the non-NULL address of the host buffer that is to supply the frame data.
									The memory it points to must be readable.
		@param[in]	inOffsetBytes	Specifies the byte offset into the device frame buffer.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@return		True if successful; otherwise false.
		@note		The host buffer should be at least inByteCount + inOffsetBytes in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
		@see		CNTV2Card::DMARead, CNTV2Card::DMAWriteFrame, CNTV2Card::DMAWriteSegments, \ref vidop-fbaccess
	**/
	AJA_VIRTUAL bool	DMAWrite (const ULWord inFrameNumber, const ULWord * pFrameBuffer, const ULWord inOffsetBytes, const ULWord inByteCount);


	/**
		@brief		Transfers a single frame from the AJA device to the host.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame to be read from the device.
									For \ref vidop-fbindexing purposes, this function assumes the intrinsic frame size of the device.
		@param[in]	pOutFrameBuffer Specifies the non-NULL address of the host buffer that is to receive the frame data.
									The memory it points to must be writeable.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@return		True if successful; otherwise false.
		@note		The host buffer must be at least inByteCount in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
		@see		CNTV2Card::DMAWriteFrame, CNTV2Card::DMARead, CNTV2Card::DMAReadSegments, \ref vidop-fbaccess
	**/
	AJA_VIRTUAL bool	DMAReadFrame (const ULWord inFrameNumber, ULWord * pOutFrameBuffer, const ULWord inByteCount);

	/**
		@brief		Transfers a single frame from the AJA device to the host. This call is multi-format compatible.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame to be read from the device.
									For \ref vidop-fbindexing purposes, this function assumes frame offsets/sizes
									based on the FrameStore identified by the \c inChannel parameter (below).
		@param[in]	pHostBuffer		Specifies the non-NULL address of the host buffer that is to receive the frame data.
									The memory it points to must be writeable.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@param[in]	inChannel		Specifies the FrameStore to supply the \ref vidop-fbindexing context to use with the
									\c inFrameNumber parameter. If the FrameStore, for example, is configured for UHD/4K,
									the intrinsic frame size (8MB or 16MB) will be quadrupled when calculating the starting
									source address in device SDRAM.
		@return		True if successful; otherwise false.
		@note		The host buffer must be at least inByteCount in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
		@see		CNTV2Card::DMAWriteFrame, CNTV2Card::DMARead, CNTV2Card::DMAReadSegments, \ref vidop-fbaccess
	**/
	AJA_VIRTUAL bool	DMAReadFrame (const ULWord inFrameNumber, ULWord * pHostBuffer, const ULWord inByteCount, const NTV2Channel inChannel);

	/**
		@brief		Transfers a single frame from the host to the AJA device.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame to be written to the device.
									For \ref vidop-fbindexing purposes, this function assumes the intrinsic frame size of the device.
		@param[in]	pInFrameBuffer	Specifies the non-NULL address of the host buffer that is to supply the frame data.
									The memory it points to must be readable.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@return		True if successful; otherwise false.
		@note		The host buffer must be at least inByteCount in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
		@see		CNTV2Card::DMAReadFrame, CNTV2Card::DMAWrite, CNTV2Card::DMAWriteSegments, \ref vidop-fbaccess
	**/
	AJA_VIRTUAL bool	DMAWriteFrame (const ULWord inFrameNumber, const ULWord * pInFrameBuffer, const ULWord inByteCount);

	/**
		@brief		Transfers a single frame from the host to the AJA device. This function is multi-format compatible.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame to be written to the device.
									For \ref vidop-fbindexing purposes, this function assumes frame offsets/sizes
									based on the FrameStore identified by the \c inChannel parameter (below).
		@param[in]	pInFrameBuffer	Specifies the non-NULL address of the host buffer that is to supply the frame data.
									The memory it points to must be readable.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@param[in]	inChannel		Specifies the FrameStore to supply the \ref vidop-fbindexing context to use with the
									\c inFrameNumber parameter. If the FrameStore, for example, is configured for UHD/4K,
									the intrinsic frame size (8MB or 16MB) will be quadrupled when calculating the starting
									destination address in device SDRAM.
		@return		True if successful; otherwise false.
		@note		The host buffer must be at least inByteCount in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
		@see		CNTV2Card::DMAReadFrame, CNTV2Card::DMAWrite, CNTV2Card::DMAWriteSegments, \ref vidop-fbaccess
	**/
	AJA_VIRTUAL bool	DMAWriteFrame (const ULWord inFrameNumber, const ULWord * pInFrameBuffer, const ULWord inByteCount, const NTV2Channel inChannel);

	/**
		@brief		Performs a segmented data transfer from the AJA device to the host.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame to be read from the device.
		@param[in]	pFrameBuffer		Specifies the non-NULL address of the host buffer that is to supply the frame data.
										The memory it points to must be writeable.
		@param[in]	inCardOffsetBytes	Specifies the initial on-device memory byte offset for the first bytes transferred.
		@param[in]	inSegmentByteCount	Specifies the number of bytes to transfer per segment.
		@param[in]	inNumSegments		Specifies the number of segments to transfer.
		@param[in]	inSegmentHostPitch	Specifies the number of bytes to increment the host memory pointer after each segment is transferred.
		@param[in]	inSegmentCardPitch	Specifies the number of bytes to increment the on-device memory pointer after each segment is transferred.
		@return		True if successful; otherwise false.
		@note		The host buffer should be at least inSegmentByteCount*inNumSegments+inOffsetBytes in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
		@see		CNTV2Card::DMAWriteSegments, CNTV2Card::DMARead, CNTV2Card::DMAReadFrame, \ref vidop-fbaccess
	**/
	AJA_VIRTUAL bool	DMAReadSegments (	const ULWord	inFrameNumber,
											ULWord *		pFrameBuffer,
											const ULWord	inCardOffsetBytes,
											const ULWord	inSegmentByteCount,
											const ULWord	inNumSegments,
											const ULWord	inSegmentHostPitch,
											const ULWord	inSegmentCardPitch);

	/**
		@brief		Performs a segmented data transfer from the host to the AJA device.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame to be written on the device.
		@param[in]	pFrameBuffer		Specifies the non-NULL address of the host buffer that is to supply the frame data.
										The memory it points to must be readable.
		@param[in]	inOffsetBytes		Specifies the initial device memory byte offset for the first bytes transferred.
		@param[in]	inSegmentByteCount	Specifies the number of bytes to transfer per segment.
		@param[in]	inNumSegments		Specifies the number of segments to transfer.
		@param[in]	inSegmentHostPitch	Specifies the number of bytes to increment the host memory pointer after each segment is transferred.
		@param[in]	inSegmentCardPitch	Specifies the number of bytes to increment the on-device memory pointer after each segment is transferred.
		@return		True if successful; otherwise false.
		@note		The host buffer should be at least inSegmentByteCount*inNumSegments+inOffsetBytes in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
		@see		CNTV2Card::DMAReadSegments, CNTV2Card::DMAWrite, CNTV2Card::DMAWriteFrame, \ref vidop-fbaccess
	**/
	AJA_VIRTUAL bool	DMAWriteSegments (	const ULWord	inFrameNumber,
											const ULWord *	pFrameBuffer,
											const ULWord	inOffsetBytes,
											const ULWord	inSegmentByteCount,
											const ULWord	inNumSegments,
											const ULWord	inSegmentHostPitch,
											const ULWord	inSegmentCardPitch);

	/**
		@brief		DirectGMA p2p transfers (not GPUDirect: see DMABufferLock)
	**/
	AJA_VIRTUAL bool	DmaP2PTargetFrame (NTV2Channel channel,					// frame buffer channel output frame to update on completion
											ULWord frameNumber,					// frame number to target
											ULWord frameOffset,					// frame buffer offset (bytes)
											PCHANNEL_P2P_STRUCT pP2PData);		// p2p target data (output)

	AJA_VIRTUAL bool	DmaP2PTransferFrame (NTV2DMAEngine DMAEngine,			// dma engine for transfer
											 ULWord frameNumber,				// source frame number
											 ULWord frameOffset,				// source frame buffer offset (bytes)
											 ULWord transferSize,				// transfer size (bytes)
											 ULWord numSegments,				// number of segments (0 if not a segmented transfer)
											 ULWord segmentTargetPitch,			// target frame pitch (0 if not a segmented transfer)
											 ULWord segmentCardPitch,			// source frame pitch (0 if not a segmented transfer)
											 PCHANNEL_P2P_STRUCT pP2PData);		// p2p target data

	/**
		@brief		Streaming transfers
	**/
	AJA_VIRTUAL bool	DMAStreamStart  (ULWord * inBuffer,
										 const ULWord inByteCount,
										 const NTV2Channel inChannel,
										 const bool inToHost);

	AJA_VIRTUAL bool	DMAStreamStop  (const NTV2Channel inChannel,
										const bool inToHost);

	/**
		@brief		Synchronously transfers audio data from a given Audio System's buffer memory on the AJA device to the specified host
					buffer, blocking until the transfer has completed.
		@param[in]	inAudioSystem		Specifies the Audio System on the device that is to supply the audio data.
		@param		pOutAudioBuffer		Specifies a valid, non-NULL pointer to the host buffer that is to receive the audio data.
										This buffer must be large enough to accommodate "inByteCount" bytes of data specified (below).
		@param[in]	inOffsetBytes		Specifies the offset into the Audio System's buffer memory on the device from which to transfer
										audio data. Specify zero to start reading from the device audio output (playout) buffer.
										Specify the value returned from CNTV2Card::GetAudioReadOffset (0x00400000) to start reading from
										the audio input (capture) portion of the Audio System's audio buffer.
		@param[in]	inByteCount			Specifies the number of audio bytes to transfer.
		@return		True if successful; otherwise false.
		@note		This function can also be used to read audio samples from the output (playout) portion of the Audio System's buffer
					memory.
	**/
	AJA_VIRTUAL bool	DMAReadAudio (	const NTV2AudioSystem	inAudioSystem,
										ULWord *				pOutAudioBuffer,
										const ULWord			inOffsetBytes,
										const ULWord			inByteCount);

	/**
		@brief		Synchronously transfers audio data from the specified host buffer to the given Audio System's buffer memory
					on the AJA device, blocking until the transfer has completed.
		@param[in]	inAudioSystem		Specifies the Audio System on the device that is to receive the audio data.
		@param[in]	pInAudioBuffer		Specifies a valid, non-NULL pointer to the host buffer that is to supply the audio data.
		@param[in]	inOffsetBytes		Specifies the offset into the Audio System's buffer memory on the device to which audio data
										will be transferred. Use zero for the start of the playout portion of the Audio System's buffer
										memory. Specifying 0 will start writing at the start of the device audio output (playout) buffer;
										specifying 0x00400000 will start writing at the start of the audio input (capture) buffer.
		@param[in]	inByteCount			Specifies the number of audio bytes to transfer. Note that this value must not overrun the host
										buffer, nor the device's audio buffer.
		@return		True if successful; otherwise false.
		@note		This function can also be used to write audio samples into the capture portion of the Audio System's buffer
					memory (which will quickly be overwritten if the capture audio system has been started).
	**/
	AJA_VIRTUAL bool	DMAWriteAudio ( const NTV2AudioSystem	inAudioSystem,
										const ULWord *			pInAudioBuffer,
										const ULWord			inOffsetBytes,
										const ULWord			inByteCount);

	/**
		@brief		Transfers the contents of the ancillary data buffer(s) from a given frame on the AJA device to the host.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame buffer to be read on the device.
										The actual starting device memory address is predicated on the device's current
										frame size, which doesn't take into account "quad" or "quad-quad 8K" frames.
										Callers must carefully calculate this number, taking into account other FrameStores
										or Channels that may be active, and if they're using "Quad" or "QuadQuad" geometries.
		@param[out] outAncF1Buffer		Specifies the host buffer that is to receive the device F1 ancillary data buffer contents.
		@param[out] outAncF2Buffer		Optionally specifies the host buffer that is to receive the device F2 ancillary data
										buffer contents.
		@param[in]	inChannel			The FrameStore/Channel being used for ingest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@note		This function assumes the device Anc extractor has been properly configured by a prior calls to
					CNTV2Card::AncExtractInit, CNTV2Card::AncExtractSetWriteParams (and also possibly CNTV2Card::AncExtractSetField2WriteParams).
		@note		This function will block and not return until the transfer has finished or failed.
		@note		This function uses the values stored in the ::kVRegAncField1Offset and ::kVRegAncField2Offset virtual registers
					to determine the Anc data boundary locations within each frame buffer in device memory.
		@note		For <b>capture</b> from IP devices running S2110 firmware, this method will automatically extract <b>VPID</b> and
					<b>RP188</b> timecode packets from the incoming RTP Anc streams, even if not ::AUTOCIRCULATE_WITH_ANC, or without
					Anc buffers in the "transferInfo".
					-	To disable this behavior completely, use an invalid ::NTV2Channel value (e.g. ::NTV2_CHANNEL_INVALID).
	**/
	AJA_VIRTUAL bool	DMAReadAnc (	const ULWord	inFrameNumber,
										NTV2Buffer &	outAncF1Buffer,
										NTV2Buffer &	outAncF2Buffer	= NULL_POINTER,
										const NTV2Channel	inChannel	= NTV2_CHANNEL1);

	/**
		@brief		Transfers the contents of the ancillary data buffer(s) from the host to a given frame on the AJA device.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame to be read from the device.
										The actual starting device memory address is predicated on the device's current
										frame size, which doesn't take into account "quad" or "quad-quad 8K" frames.
										Callers must carefully calculate this number, taking into account other FrameStores
										or Channels that may be active, and if they're using "Quad" or "QuadQuad" geometries.
		@param[in]	inAncF1Buffer		Specifies the host buffer that is to supply the F1 ancillary data buffer content.
		@param[in]	inAncF2Buffer		Optionally specifies the host buffer that is to supply the F2 ancillary data
										buffer content.
		@param[in]	inChannel			The FrameStore/Channel being used for playout. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@note		This function assumes the device Anc inserter has been properly configured by prior calls to
					CNTV2Card::AncInsertInit, CNTV2Card::AncInsertSetReadParams (and also possibly CNTV2Card::AncInsertSetField2ReadParams).
		@note		This function will block and not return until the transfer has finished or failed.
		@note		This function uses the values stored in the ::kVRegAncField1Offset and ::kVRegAncField2Offset virtual registers
					to determine the Anc data boundary locations within each frame buffer in device memory.
		@note		For <b>playout</b> to IP devices running S2110 firmware, this method will automatically add <b>VPID</b> and
					<b>RP188</b> timecode packets into the outgoing RTP Anc streams, even if such packets weren't placed into the
					given buffers. This default behavior can be overridden or disabled:
					-	To disable this behavior completely, use an invalid ::NTV2Channel value (e.g. ::NTV2_CHANNEL_INVALID).
					-	To disable the default <b>VPID</b> insertion, call CNTV2Card::SetSDIOutVPID, passing zeroes for
						the VPID values.
					-	To override the default <b>VPID</b> values, insert your own <b>VPID</b> packet(s) into the F1
						and/or F2 Anc buffers.
					-	To override the default <b>RP188</b> value(s), insert your own <b>RP188</b> packets into the
						F1 and/or F2 Anc buffers.
	**/
	AJA_VIRTUAL bool	DMAWriteAnc (	const ULWord		inFrameNumber,
										NTV2Buffer &		inAncF1Buffer,
										NTV2Buffer &		inAncF2Buffer	= NULL_POINTER,
										const NTV2Channel	inChannel		= NTV2_CHANNEL1);
	

	/**
		@brief		Synchronously transfers LUT data from the specified host buffer to the given buffer memory
					on the AJA device, blocking until the transfer has completed.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame buffer to be read on the device.
		@param[in]	pInLUTBuffer		Specifies a valid, non-NULL pointer to the host buffer that is to supply the LUT data.
		@param[in]	inLUTIndex			Specifies the index of the LUT
		@param[in]	inByteCount			Specifies the amount to DMA
		@return		True if successful; otherwise false.
		@note		This function can also be used to write LUT Tables into memory
	**/
	AJA_VIRTUAL bool DMAWriteLUTTable ( const ULWord inFrameNumber, const ULWord * pInLUTBuffer, const ULWord inLUTIndex,const ULWord inByteCount = LUTTablePartitionSize);

	/**
		@brief		Page-locks the given host buffer to reduce transfer time and CPU usage of DMA transfers.
		@param[in]	inBuffer	Specifies the host buffer to lock.
		@param[in]	inMap		Also lock the segment map.
		@param[in]	inRDMA		Lock a GPUDirect buffer for p2p DMA.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::DMABufferUnlock, CNTV2Card::DMABufferAutoLock, CNTV2Card::DMABufferUnlockAll, \ref vidop-fblocking
	**/
	AJA_VIRTUAL bool	DMABufferLock (const NTV2Buffer & inBuffer, bool inMap = false, bool inRDMA = false); //	New in SDK 15.5

	/**
		@brief		Page-locks the given host buffer to reduce transfer time and CPU usage of DMA transfers.
		@param[in]	pInBuffer		Specifies the starting address of the host buffer to lock.
		@param[in]	inByteCount		Specifies the total length of the host buffer.
		@param[in]	inMap			Also lock the segment map.
		@param[in]	inRDMA			Lock a GPUDirect buffer for p2p DMA.
		@return		True if successful; otherwise false.
		@note		On the Windows platform, buffers allocated using GlobalAlloc or VirtualAlloc often won't map properly because
					these functions allocate the virtual address space but not the memory pages, which only get allocated and committed
					when the memory gets written (touched). Instead, we recommend using _aligned_malloc for mapped host memory.
		@see		CNTV2Card::DMABufferUnlock, CNTV2Card::DMABufferAutoLock, CNTV2Card::DMABufferUnlockAll, \ref vidop-fblocking
	**/
	AJA_VIRTUAL inline bool DMABufferLock (const ULWord * pInBuffer, const ULWord inByteCount, bool inMap = false, bool inRDMA = false)
	{
		return DMABufferLock(NTV2Buffer(pInBuffer, inByteCount), inMap, inRDMA);
	}


	/**
		@brief		Unlocks the given host buffer that was previously locked using CNTV2Card::DMABufferLock.
		@param[in]	inBuffer	Specifies the host buffer to unlock.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::DMABufferLock, CNTV2Card::DMABufferUnlockAll, \ref vidop-fblocking
	**/
	AJA_VIRTUAL bool	DMABufferUnlock (const NTV2Buffer & inBuffer);

	/**
		@brief		Unlocks the given host buffer that was previously locked using CNTV2Card::DMABufferLock.
		@param[in]	pInBuffer		Specifies the starting address of the previously locked host buffer.
		@param[in]	inByteCount		Specifies the total length of the previously locked host buffer.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::DMABufferLock, CNTV2Card::DMABufferUnlockAll, \ref vidop-fblocking
	**/
	AJA_VIRTUAL inline bool DMABufferUnlock (const ULWord * pInBuffer, const ULWord inByteCount)
	{
		return DMABufferUnlock(NTV2Buffer(pInBuffer, inByteCount));
	}

	/**
		@brief		Unlocks all previously-locked buffers used for DMA transfers.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::DMABufferLock, CNTV2Card::DMABufferUnlock, \ref vidop-fblocking
	**/
	AJA_VIRTUAL bool	DMABufferUnlockAll ();

	/**
		@brief		Enables or disables automatic buffer locking.
		@param[in]	inEnable		Specify true to enable automatic buffer locking;  otherwise false to disable it.
		@param[in]	inMap			If enabling automatic locking, also try to lock the segment map.
		@param[in]	inMaxLockSize	Specify the maximum number of locked bytes.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::DMABufferLock, CNTV2Card::DMABufferUnlock, CNTV2Card::DMABufferUnlockAll, \ref vidop-fblocking
	**/
	AJA_VIRTUAL bool	DMABufferAutoLock (const bool inEnable, const bool inMap = false, const ULWord64 inMaxLockSize = 0);


	/**
		@brief		Clears the ancillary data region in the device frame buffer for the specified frames.
		@param[in]	inStartFrameNumber		Specifies the starting device frame number.
		@param[in]	inEndFrameNumber		Specifies the ending device frame number.
		@param[in]	inAncRegion				Optionally specifies the ancillary data region to clear (e.g.
											NTV2_AncRgn_Field1, NTV2_AncRgn_Field2, etc.).	Defaults to all regions.
		@param[in]	inChannel				Optionally specifies the channel associated with the frame numbers.
											Defaults to NTV2_CHANNEL1. (New in SDK 16.1)
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	DMAClearAncRegion ( const UWord inStartFrameNumber,
											const UWord inEndFrameNumber,
											const NTV2AncillaryDataRegion inAncRegion = NTV2_AncRgn_All,
											const NTV2Channel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the address and size of the given frame.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame of interest.
		@param[in]	inChannel		Specifies the ::NTV2Channel of interest (for multi-format mode).
									Ignored and assumes ::NTV2_CHANNEL1 if device is not in multi-format mode.
		@param[out] outAddress		Receives the device memory address of the first byte of the given frame.
		@param[out] outLength		Receives the frame size, in bytes.
		@note		The ::NTV2Channel parameter must not refer to a FrameStore that is slaved to a quad or quad-quad master.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetDeviceFrameInfo (const UWord inFrameNumber,	const NTV2Channel inChannel,
											uint64_t & outAddress,	uint64_t & outLength);

	AJA_VIRTUAL bool	GetDeviceFrameInfo (const UWord inFrameNumber,	const NTV2Channel inChannel, ULWord & outIntrinsicSize,
											bool & outMultiFmt, bool & outQuad, bool & outQuadQuad, bool & outSquares, bool & outTSI,
											uint64_t & outAddress,	uint64_t & outLength);	//	New in SDK 16.2

	/**
		@brief		Answers with the frame number that contains the given address.
		@param[in]	inAddress		Specifies the device memory address of the first byte of the given frame.
		@param[out] outFrameNumber	Receives the zero-based frame number.
		@param[in]	inChannel		Optionally specifies the channel of interest (for multi-format).
									Defaults to NTV2_CHANNEL1 for uni-format.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	DeviceAddressToFrameNumber (const uint64_t inAddress,  UWord & outFrameNumber,	const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the offset and size of an ancillary data region within a device frame buffer.
		@param[out] outByteOffset	Receives the byte offset where the ancillary data region starts in the frame buffer,
									(measured from the start of the frame buffer).
									This is guaranteed to be non-zero if the function succeeds, and zero if it fails.
		@param[out] outByteCount	Receives the size of the ancillary data region, in bytes.
									This is guaranteed to be non-zero if the function succeeds, and zero if it fails.
		@param[in]	inAncRegion		Optionally specifies the ancillary data region of interest (e.g. NTV2_AncRgn_Field1,
									NTV2_AncRgn_Field2, etc.).	Defaults to all regions, for the maximum offset and size
									among all of them.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::AncSetFrameBufferSize, \ref anccapture-dataspace
	**/
	AJA_VIRTUAL bool	GetAncRegionOffsetAndSize (ULWord & outByteOffset, ULWord & outByteCount,
													const NTV2AncillaryDataRegion inAncRegion = NTV2_AncRgn_All);

	/**
		@brief		Answers with the byte offset to the start of an ancillary data region within a device frame buffer,
					as measured from the bottom of the frame buffer.
		@param[out] outByteOffsetFromBottom		Receives the byte offset to the start of the ancillary data region,
												as measured from the bottom of the frame buffer.
		@param[in]	inAncRegion		Optionally specifies the ancillary data region of interest (e.g. NTV2_AncRgn_Field1,
									NTV2_AncRgn_Field2, etc.).	Defaults to all regions, for the largest offset among
									them all.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::GetAncRegionOffsetAndSize, \ref anccapture-dataspace
	**/
	AJA_VIRTUAL bool	GetAncRegionOffsetFromBottom (ULWord & outByteOffsetFromBottom,
														const NTV2AncillaryDataRegion inAncRegion = NTV2_AncRgn_All);
	///@}

//
//	 Set/Get Parameter routines
//
	#if defined (AJAMac)
		#define AJA_RETAIL_DEFAULT	true
	#else	//	else !defined (AJAMac)
		#define AJA_RETAIL_DEFAULT	false
	#endif	//	!defined (AJAMac)

	/**
		@brief		Configures the AJA device to handle a specific video format.
		@param[in]	inVideoFormat			Specifies the desired video format for the given channel on the device.
											It must be a valid ::NTV2VideoFormat constant.
		@param[in]	inIsAJARetail			Specify 'true' to preserve the current horizontal and vertical timing settings.
											Defaults to true on MacOS, false on other platforms.
		@param[in]	inKeepVancSettings		If true, specifies that the device's current VANC settings are to be preserved;
											otherwise, they will not be preserved. Defaults to false.
		@param[in]	inChannel				Specifies the NTV2Channel of interest. Defaults to ::NTV2_CHANNEL1.
											For UHD/4K video formats, specify NTV2_CHANNEL1 to configure quadrant channels 1-4,
											or ::NTV2_CHANNEL5 to configure quadrant channels 5-8.
		@return		True if successful; otherwise false.
		@details	This function changes the device configuration to a specific video standard (e.g., 525, 1080, etc.),
					frame geometry (e.g., 1920x1080, 720x486, etc.) and frame rate (e.g., 59.94 fps, 29.97 fps, etc.),
					plus a few other settings (e.g., progressive/interlaced, etc.), all based on the given video format.
	**/
	AJA_VIRTUAL bool	SetVideoFormat (const NTV2VideoFormat inVideoFormat, const bool inIsAJARetail = AJA_RETAIL_DEFAULT, const bool inKeepVancSettings = false, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Sets the video format for one or more FrameStores.
		@param[in]	inFrameStores			Specifies the FrameStore(s) of interest as a channel-set (a set of zero-based index numbers).
		@param[in]	inVideoFormat			Specifies the desired video format. It must be a valid ::NTV2VideoFormat constant.
		@param[in]	inIsAJARetail			Specify 'true' to preserve the current horizontal and vertical timing settings.
											Defaults to true on MacOS, false on other platforms.
		@return		True if successful; otherwise false.
		@details	This function changes the device configuration to a specific video standard (e.g., 525, 1080, etc.),
					frame geometry (e.g., 1920x1080, 720x486, etc.) and frame rate (e.g., 59.94 fps, 29.97 fps, etc.),
					plus a few other settings (e.g., progressive/interlaced, etc.), all based on the given video format.
	**/
	AJA_VIRTUAL bool	SetVideoFormat (const NTV2ChannelSet & inFrameStores, const NTV2VideoFormat inVideoFormat, const bool inIsAJARetail = AJA_RETAIL_DEFAULT);

	/**
		@brief		Sets the frame geometry of the given channel.
		@param[in]	inGeometry		Specifies the desired frame geometry. It must be a valid ::NTV2FrameGeometry value.
		@param[in]	inIsRetail		This parameter is ignored.
		@param[in]	inChannel		Specifies the ::NTV2Channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	SetFrameGeometry (NTV2FrameGeometry inGeometry, bool inIsRetail = AJA_RETAIL_DEFAULT, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Sets the frame buffer format for the given FrameStore on the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
		@param[in]	inNewFormat		Specifies the desired frame buffer format.
									This must be a valid ::NTV2FrameBufferFormat value.
		@param[in]	inIsAJARetail	Specifies if the AJA retail configuration settings are to be respected or not.
									Defaults to false on all platforms other than MacOS, which defaults to true.
		@param[in]	inXferChars		Specifies the HDR tranfer characteristc description (affects output VPID).
		@param[in]	inColorimetry	Specifies the HDR colorimetry description (affects output VPID).
		@param[in]	inLuminance		Specifies the HDR luminance description (affects output VPID).
		@details	This function allows client applications to control the format of frame data stored
					in the FrameStores on an AJA device. This is important, because when frames are transferred
					between the host and the AJA device, the frame data format is presumed to be identical.
	**/
	AJA_VIRTUAL bool	SetFrameBufferFormat (NTV2Channel inChannel,
											  NTV2FrameBufferFormat inNewFormat,
											  bool inIsAJARetail = AJA_RETAIL_DEFAULT,
											  NTV2HDRXferChars inXferChars = NTV2_VPID_TC_SDR_TV,
											  NTV2HDRColorimetry inColorimetry = NTV2_VPID_Color_Rec709,
											  NTV2HDRLuminance inLuminance = NTV2_VPID_Luminance_YCbCr);

	/**
		@brief		Sets the frame buffer format for the given FrameStore(s) on the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inFrameStores	Specifies the FrameStore(s) of interest as a channel-set (a set of zero-based index numbers).
		@param[in]	inNewFormat		Specifies the desired frame buffer format.
									This must be a valid ::NTV2FrameBufferFormat value.
		@param[in]	inIsAJARetail	Specifies if the AJA retail configuration settings are to be respected or not.
									Defaults to false on all platforms other than MacOS, which defaults to true.
		@param[in]	inXferChars		Specifies the HDR tranfer characteristc description (affects output VPID).
		@param[in]	inColorimetry	Specifies the HDR colorimetry description (affects output VPID).
		@param[in]	inLuminance		Specifies the HDR luminance description (affects output VPID).
		@details	This function allows client applications to control the format of frame data read or written
					by the FrameStore(s) on an AJA device. This is important, because when frames are transferred
					between the host and the AJA device, the frame data format is presumed to be identical.
	**/
	AJA_VIRTUAL bool	SetFrameBufferFormat (const NTV2ChannelSet & inFrameStores,
											  const NTV2FrameBufferFormat inNewFormat,
											  const bool inIsAJARetail = AJA_RETAIL_DEFAULT,
											  const NTV2HDRXferChars inXferChars = NTV2_VPID_TC_SDR_TV,
											  const NTV2HDRColorimetry inColorimetry = NTV2_VPID_Color_Rec709,
											  const NTV2HDRLuminance inLuminance = NTV2_VPID_Luminance_YCbCr);

	/**
		@brief		Sets the device's clock reference source. See \ref deviceclockingandsync for more information.
		@param[in]	inRefSource				Specifies the ::NTV2ReferenceSource to use.
		@param[in]	inKeepFramePulseSelect	For devices that support a frame pulse source that's independent of the
											reference source, specify true to prevent resetting the frame pulse source.
		@return		True if successful; otherwise false.
		@see		GetReference, GetReferenceVideoFormat, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool		SetReference (const NTV2ReferenceSource inRefSource, const bool inKeepFramePulseSelect = false);

	/**
		@brief		Answers with the device's current clock reference source.
		@param[out]	outRefSource	Receives the current ::NTV2ReferenceSource value.
		@return		True if successful; otherwise false.
		@see		SetReference, GetReferenceVideoFormat, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool		GetReference (NTV2ReferenceSource & outRefSource);
	
	/**
		@brief		Enables the device's frame pulse reference select.
		@param[in]	inEnable	Specify true to enable the frame pulse reference; otherwise specify false.
		@return		True if successful; otherwise false.
		@see		GetEnableFramePulseReference, GetFramePulseReference, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool		EnableFramePulseReference (const bool inEnable);	//	New in SDK 15.5
	
	/**
		@brief		Answers whether or not the device's current frame pulse reference source is enabled.
					See \ref deviceclockingandsync for more information.
		@param[out]	outEnabled		Receives true if the frame pulse reference is enabled; otherwise false.
		@return		True if successful; otherwise false.
		@see		EnableFramePulseReference, GetFramePulseReference, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool		GetEnableFramePulseReference (bool & outEnabled);	//	New in SDK 15.5
	
	/**
		@brief		Sets the device's frame pulse reference source. See \ref deviceclockingandsync for more information.
		@param[in]	inRefSource		Specifies the ::NTV2ReferenceSource to use for the device's frame pulse reference.
		@return		True if successful; otherwise false.
		@see		GetFramePulseReference, GetEnableFramePulseReference, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool		SetFramePulseReference (const NTV2ReferenceSource inRefSource); //	New in SDK 15.5
	
	/**
		@brief		Answers with the device's current frame pulse reference source.
		@param[out]	outRefSource	Receives the ::NTV2ReferenceSource value.
		@return		True if successful; otherwise false.
		@see		SetFramePulseReference, GetEnableFramePulseReference, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool		GetFramePulseReference (NTV2ReferenceSource & outRefSource);	//	New in SDK 15.5

	/**
		@brief		Retrieves the device's current "retail service" task mode.
		@param[out] outMode		Receives the device's current "every frame task mode" setting. If successful, the
								variable will contain ::NTV2_DISABLE_TASKS, ::NTV2_STANDARD_TASKS, or ::NTV2_OEM_TASKS.
		@return		True if successful; otherwise false.
		@see		CNTV2DriverInterface::GetStreamingApplication, \ref devicesharing
	**/
	AJA_VIRTUAL bool		GetEveryFrameServices (NTV2EveryFrameTaskMode & outMode);

	/**
		@brief		Sets the device's task mode.
		@return		True if successful; otherwise false.
		@param[in]	inMode		Specifies the task mode the device is to assume, and must be one of the following values:
								::NTV2_DISABLE_TASKS, ::NTV2_STANDARD_TASKS, or ::NTV2_OEM_TASKS.
		@warning	Do not use task mode ::NTV2_STANDARD_TASKS for OEM applications.
		@see		CNTV2DriverInterface::GetStreamingApplication, \ref devicesharing
	**/
	AJA_VIRTUAL bool		SetEveryFrameServices (const NTV2EveryFrameTaskMode inMode);

	/**
		@brief		Determines if a given FrameStore on the AJA device will be used to capture or playout video.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
		@param[in]	inNewValue		Specifies the desired mode (::NTV2_MODE_DISPLAY or ::NTV2_MODE_CAPTURE).
		@param[in]	inIsRetail		This parameter is obsolete and ignored.
		@return		True if successful; otherwise false.
		@details	In ::NTV2_MODE_CAPTURE mode, device frame memory is written;  in ::NTV2_MODE_DISPLAY mode, it's read from.
		@note		\ref aboutautocirculate automatically sets the ::NTV2Mode (but doesn't automatically un-set it after use).
		@see		CNTV2Card::GetMode, \ref vidop-fs
	**/
	AJA_VIRTUAL bool		SetMode (const NTV2Channel inChannel, const NTV2Mode inNewValue, const bool inIsRetail = AJA_RETAIL_DEFAULT);

	/**
		@brief		Sets the capture or playout mode of a set of FrameStores on the AJA device.
		@param[in]	inChannels		Specifies the FrameStore(s) of interest.
		@param[in]	inNewValue		Specifies the desired mode (::NTV2_MODE_DISPLAY or ::NTV2_MODE_CAPTURE).
		@return		True if all FrameStore(s) were successfully configured; otherwise false.
		@details	In ::NTV2_MODE_CAPTURE mode, device frame memory is written;  in ::NTV2_MODE_DISPLAY mode, it's read from.
		@note		\ref aboutautocirculate automatically sets the ::NTV2Mode (but doesn't automatically un-set it after use).
		@see		CNTV2Card::GetMode, \ref vidop-fs
	**/
	AJA_VIRTUAL bool		SetMode (const NTV2ChannelSet & inChannels, const NTV2Mode inMode);

	/**
		@brief		Answers with the current ::NTV2Mode of the given FrameStore on the AJA device.
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel value (a zero-based index number).
		@param[out] outValue	Receives the FrameStore's current ::NTV2Mode (::NTV2_MODE_DISPLAY or ::NTV2_MODE_CAPTURE).
		@return		True if successful; otherwise false.
		@details	In ::NTV2_MODE_CAPTURE mode, device frame memory is written;  in ::NTV2_MODE_DISPLAY mode, it's read from.
		@see		CNTV2Card::SetMode, \ref vidop-fs
	**/
	AJA_VIRTUAL bool		GetMode (const NTV2Channel inChannel, NTV2Mode & outValue);

	AJA_VIRTUAL bool		GetFrameGeometry (NTV2FrameGeometry & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Returns the current frame buffer format for the given FrameStore on the AJA device.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
		@param[out] outValue		Receives the FrameStore's current pixel format. If the function result is true,
									the variable will contain a valid ::NTV2FrameBufferFormat value.
		@return		True if successful; otherwise false.
		@details	This function allows client applications to inquire about the current format of frame data
					stored in an AJA device's FrameStore. This is important because when frames are transferred
					between the host and the AJA device, the frame data format is presumed to be identical.
	**/
	AJA_VIRTUAL bool		GetFrameBufferFormat (NTV2Channel inChannel, NTV2FrameBufferFormat & outValue);


	/**
		@brief		Returns a std::set of ::NTV2VideoFormat values that I support.
		@param[out] outFormats	Receives the set of ::NTV2VideoFormat values, or empty upon failure.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetSupportedVideoFormats (NTV2VideoFormatSet & outFormats);


	// The rest of the routines
	AJA_VIRTUAL bool		GetVideoFormat (NTV2VideoFormat & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool		SetStandard (NTV2Standard inValue, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool		GetStandard (NTV2Standard & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool		IsProgressiveStandard (bool & outIsProgressive, NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool		IsSDStandard (bool & outIsStandardDef, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Sets the AJA device's frame rate.
		@return		True if successful; otherwise false.
		@param[in]	inNewValue		Specifies the new ::NTV2FrameRate value the AJA device is to be configured with.
		@param[in]	inChannel		Specifies the ::NTV2Channel of interest.
		@note		The frame rate setting for ::NTV2_CHANNEL1 dictates the device reference clock for both single
					and multi-format mode (see \ref deviceclockingandsync).
	**/
	AJA_VIRTUAL bool		SetFrameRate (NTV2FrameRate inNewValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Returns the AJA device's currently configured frame rate via its "value" parameter.
		@return		True if successful; otherwise false.
		@param[out] outValue	Receives the device's current ::NTV2FrameRate value.
		@param[in]	inChannel	Specifies the ::NTV2Channel of interest.
		@note		The frame rate setting for ::NTV2_CHANNEL1 dictates the device reference clock for both single
					and multi-format mode (see \ref deviceclockingandsync).
	**/
	AJA_VIRTUAL bool		GetFrameRate (NTV2FrameRate & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Enables or disables the device's SMPTE-372 (dual-link) mode (used for older 3G-levelB-capable devices).
		@note		This allows older devices to handle 1080p60/1080p5994/1080p50 signals by "ganging" two 30Hz FrameStores. See \ref duallinkoverview for more information.
		@note		The enable bits work on channel pairs, thus a parameter of ::NTV2_CHANNEL1 or ::NTV2_CHANNEL2 refers to the same control bit.
		@return		True if successful; otherwise false.
		@param[in]	inValue		Specify a non-zero value (true) to put the device into SMPTE 372 dual-link mode.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1.
		@todo		Should use bool parameter instead of a ULWord.
		@todo		Should be named SetSMPTE372Enable.
	**/
	AJA_VIRTUAL bool		SetSmpte372 (ULWord inValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Returns the device's current SMPTE-372 (dual-link) mode, whether it's enabled or not.
		@note		The enable bits work on channel pairs, thus a parameter of ::NTV2_CHANNEL1 or ::NTV2_CHANNEL2 refers to the same control bit.
		@return		True if successful; otherwise false.
		@param[out] outValue	Receives 1 if the device is currently in dual-link mode;  otherwise receives 0.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1.
		@todo		Should use bool& parameter instead of a ULWord&.
		@todo		Should be named GetSMPTE372Enable.
	**/
	AJA_VIRTUAL bool		GetSmpte372 (ULWord & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool		SetProgressivePicture (ULWord value);
	AJA_VIRTUAL bool		GetProgressivePicture (ULWord & outValue);

	/**
		@brief		Enables or disables quad-frame mode on the device.
		@return		True if successful; otherwise false.
		@param[in]	inValue		Specify 'true' to put the device into quad frame mode.
								Specify 'false' to put the device into normal (non-quad) frame mode.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1. Ignored if the device
								is incapable of 4K.
		@note		Most clients won't need to call this function, as calling CNTV2Card::SetVideoFormat to one of
					the 4K/UHD formats will automatically do so.
	**/
	AJA_VIRTUAL bool		SetQuadFrameEnable (const bool inValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Returns the device's current quad-frame mode, whether it's enabled or not.
		@return		True if successful; otherwise false.
		@param[out] outValue	Receives 'true' if the device is currently in quad frame mode; otherwise 'false'.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1. Ignored if the device
								is incapable of 4K.
	**/
	AJA_VIRTUAL bool		GetQuadFrameEnable (bool & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	
	/**
		@brief		Enables or disables "quad-quad" 8K frame mode on the device.
		@return		True if successful; otherwise false.
		@param[in]	inValue		Specify 'true' to put the device into "quad quad" frame mode.
								Specify 'false' to put the device into normal (non-quad-quad) frame mode.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1. Ignored if the device
								is incapable of 8K/UHD2.
		@note		Most clients won't need to call this function, as CNTV2Card::SetVideoFormat using one of
					the 8K/UHD2 formats will automatically do so.
	**/
	AJA_VIRTUAL bool		SetQuadQuadFrameEnable (const bool inValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Returns the device's current "quad-quad" frame mode, whether it's enabled or not.
		@return		True if successful; otherwise false.
		@param[out] outValue	Receives 'true' if the device is currently in quad quad frame mode; otherwise 'false'.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1. Ignored if the device
								is incapable of 8K/UHD2.
	**/
	AJA_VIRTUAL bool		GetQuadQuadFrameEnable (bool & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Enables or disables quad-quad-frame (8K) squares mode on the device.
		@return		True if successful; otherwise false.
		@param[in]	inValue		Specify a non-zero value (true) to put the device into quad quad frame mode.
								Specify zero (false) to put the device into normal (non-quad-quad) frame mode.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1. Ignored if the device
								is incapable of multi-format mode, or is not currently in multi-format mode.
	**/
	AJA_VIRTUAL bool		SetQuadQuadSquaresEnable (const bool inValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Returns the device's current "quad-quad-squares" frame mode, whether it's enabled or not.
		@return		True if successful; otherwise false.
		@param[out] outValue	Receives 'true' if the device is currently in "quad quad squares" frame mode;
								otherwise 'false'.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1. Ignored if the device
								is incapable of multi-format mode, or is not currently in multi-format mode.
	**/
	AJA_VIRTUAL bool		GetQuadQuadSquaresEnable (bool & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Enables or disables SMPTE 425 "2K quadrants" mode for the given FrameStore bank on the device.
					Client applications should call this function when "4K Squares" mode (not two-sample-interleave) is needed.
		@return		True if successful; otherwise false.
		@param[in]	inIsEnabled		Specify true to put the device's FrameStores into "4K squares" (i.e., "2K quadrants") mode.
									Specify false to put the device's FrameStores into normal mode (if not currently running in quad frame mode), or the non-2K quadrants quad mode.
		@param[in]	inChannel		Specifies the FrameStore bank of interest. Using anything ordinally less than ::NTV2_CHANNEL5
									will affect FrameStores 1/2/3/4, while anything ordinally greater than ::NTV2_CHANNEL4 will
									affect FrameStores 5/6/7/8.
		@note		Disabling 4K squares will implicitly set two-sample-interleave mode for the FrameStores.
	**/
	AJA_VIRTUAL bool		Set4kSquaresEnable (const bool inIsEnabled, const NTV2Channel inChannel);

	/**
		@brief		Answers whether the FrameStore bank's current SMPTE 425 "4K squares" (i.e., "2K quadrants") mode is enabled or not.
		@return		True if successful; otherwise false.
		@param[out] outIsEnabled	Receives true if the device's FrameStores are currently in "4K squares"
									(i.e., "2K quadrants") mode;  otherwise false.
		@param[in]	inChannel		Specifies the FrameStore bank of interest. Using anything ordinally less than ::NTV2_CHANNEL5
									will report on FrameStores 1/2/3/4, while anything ordinally greater than ::NTV2_CHANNEL4 will
									report on FrameStores 5/6/7/8.
	**/
	AJA_VIRTUAL bool		Get4kSquaresEnable (bool & outIsEnabled, const NTV2Channel inChannel);

	/**
		@brief		Enables or disables SMPTE 425 two-sample interleave (Tsi) frame mode on the device.
		@return		True if successful; otherwise false.
		@param[in]	inIsEnabled		Specify true to put the device's FrameStores into two-sample interleave (Tsi) mode.
									Specify false to put the device's FrameStores into non-Tsi mode.
		@param[in]	inChannel		Specifies the FrameStore bank of interest. Using anything ordinally less than ::NTV2_CHANNEL5
									will affect FrameStores 1/2/3/4, while anything ordinally greater than ::NTV2_CHANNEL4 will
									affect FrameStores 5/6/7/8.
		@note		Since Tsi is the default 4K mode, there's no need to call this function if Set4kSquaresEnable(false) was called.
	**/
	AJA_VIRTUAL bool		SetTsiFrameEnable (const bool inIsEnabled, const NTV2Channel inChannel);

	/**
		@brief		Returns the current SMPTE 425 two-sample-interleave frame mode on the device, whether it's enabled or not.
		@return		True if successful; otherwise false.
		@param[out] outIsEnabled	Receives true if the device's FrameStores are currently in two-sample interleave (Tsi) mode; otherwise false.
		@param[in]	inChannel		Specifies the FrameStore bank of interest. Using anything ordinally less than ::NTV2_CHANNEL5
									will report on FrameStores 1/2/3/4, while anything ordinally greater than ::NTV2_CHANNEL4 will
									report on FrameStores 5/6/7/8.
	**/
	AJA_VIRTUAL bool		GetTsiFrameEnable (bool & outIsEnabled, const NTV2Channel inChannel);

	/**
		@brief		Answers if the SMPTE 425 two-sample-interleave mux/demux input sync has failed or not.
		@return		True if successful; otherwise false.
		@param[out] outSyncFailed	Receives true if the device's Tsi Mux input sync detect is indicating failure.
		@param[in]	inWhichTsiMux	Specifies the Tsi Mux of interest. Use NTV2_CHANNEL1 for Tsi Mux 1, etc.
	**/
	AJA_VIRTUAL bool		GetTsiMuxSyncFail (bool & outSyncFailed, const NTV2Channel inWhichTsiMux);

	AJA_VIRTUAL bool		SetFrameBufferQuarterSizeMode (NTV2Channel inChannel, NTV2QuarterSizeExpandMode inValue);
	AJA_VIRTUAL bool		GetFrameBufferQuarterSizeMode (NTV2Channel inChannel, NTV2QuarterSizeExpandMode & outValue);

	AJA_VIRTUAL bool		SetFrameBufferQuality (NTV2Channel inChannel, NTV2FrameBufferQuality inValue);
	AJA_VIRTUAL bool		GetFrameBufferQuality (NTV2Channel inChannel, NTV2FrameBufferQuality & outValue);

	AJA_VIRTUAL bool		SetEncodeAsPSF (NTV2Channel inChannel, NTV2EncodeAsPSF inValue);
	AJA_VIRTUAL bool		GetEncodeAsPSF (NTV2Channel inChannel, NTV2EncodeAsPSF & outValue);

	/**
		@brief		Sets the frame buffer orientation for the given NTV2Channel.
		@param[in]	inChannel	Specifies the channel (aka FrameStore) of interest.
		@param[in]	inValue		Specifies the new frame buffer orientation.
		@return		True if successful;	 otherwise false.
		@note		For capture, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN or NTV2_FRAMEBUFFER_ORIENTATION_NORMAL specifies that the input de-embedder writes
					incoming pixel data in top-to-bottom order in the frame buffer, whereas NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP writes incoming pixel
					data in bottom-to-top order. For playout, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN or NTV2_FRAMEBUFFER_ORIENTATION_NORMAL specifies that
					the output embedder reads outgoing pixel data in top-to-bottom order from frame buffer memory, whereas NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP
					reads it in bottom-to-top order.
	**/
	AJA_VIRTUAL bool		SetFrameBufferOrientation (const NTV2Channel inChannel, const NTV2FBOrientation inValue);

	/**
		@brief		Answers with the current frame buffer orientation for the given NTV2Channel.
		@param[in]	inChannel	Specifies the channel (aka FrameStore) of interest.
		@param[out] outValue	Receives the NTV2VideoFrameBufferOrientation value.
		@return		True if successful;	 otherwise false.
		@note		Normal operation is NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN. For capture, the input de-embedder writes incoming pixel data in top-to-bottom
					order in the frame buffer, and the output embedder also reads outgoing pixel data in top-to-bottom order from frame buffer memory.
					In NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP operation, this is reversed. The input de-embedder writes incoming pixel data in bottom-to-top
					order during capture, while for playout, the output embedder reads pixel data in bottom-to-top order.
	**/
	AJA_VIRTUAL bool		GetFrameBufferOrientation (const NTV2Channel inChannel, NTV2FBOrientation & outValue);

	AJA_VIRTUAL bool		SetAlphaFromInput2Bit (ULWord inValue);
	AJA_VIRTUAL bool		GetAlphaFromInput2Bit (ULWord & outValue);

	/**
		@brief		Sets the output frame index number for the given FrameStore. This identifies which frame in device
					SDRAM will be used for playout after the next VBI.
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
		@param[in]	inValue		Specifies the desired output frame index number in device memory. See \ref vidop-fbindexing
								for more information.
		@return		True if successful;	 otherwise false.
		@note		For the effect to be noticeable, the FrameStore should be enabled (see CNTV2Card::EnableChannel)
					and in ::NTV2_MODE_DISPLAY mode.
		@note		Normally, if the device ::NTV2RegisterWriteMode is ::NTV2_REGWRITE_SYNCTOFRAME, the new value takes
					effect at the next output <b>frame</b> interrupt. For example, if line 300 of frame 5 is currently
					going "out the jack" at the instant this function is called with frame 6, frame 6 won't go "out the jack"
					until the output VBI fires after the last line of frame 5 has gone out the spigot.
		@note		If the FrameStore's ::NTV2RegisterWriteMode is ::NTV2_REGWRITE_SYNCTOFIELD, the new value takes effect
					at the next output <b>field</b> interrupt, which makes it possible to playout lines for ::NTV2_FIELD0
					and ::NTV2_FIELD1 from separate frame buffers, if desired. See CNTV2Card::SetRegisterWriteMode and/or
					\ref fieldframeinterrupts for more information.
		@warning	If the designated FrameStore is enabled and in ::NTV2_MODE_DISPLAY mode, and the given frame is within
					the frame range being used by another FrameStore/channel, this will likely result in wrong/torn/bad
					output video. See \ref vidop-fbconflict
		@warning	If the designated FrameStore is enabled and in ::NTV2_MODE_DISPLAY mode, and the given frame is in
					Audio Buffer memory that's in use by a running Audio System, this will likely result in wrong/torn/bad
					output video. See \ref audioclobber
		@see		CNTV2Card::GetOutputFrame, \ref vidop-fs
	**/
	AJA_VIRTUAL bool		SetOutputFrame (const NTV2Channel inChannel, const ULWord inValue);

	/**
		@brief		Answers with the current output frame number for the given FrameStore (expressed as an ::NTV2Channel).
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
		@param[out] outValue	Receives the current output frame number, a zero-based index into each 8/16/32 MB
								block of SDRAM on the device.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetOutputFrame, \ref vidop-fs
	**/
	AJA_VIRTUAL bool		GetOutputFrame (const NTV2Channel inChannel, ULWord & outValue);

	/**
		@brief		Sets the input frame index number for the given FrameStore. This identifies which frame in device
					SDRAM will be written after the next VBI.
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
		@param[in]	inValue		Specifies the desired frame index number in device memory to be written. See \ref vidop-fbindexing
								for more information.
		@return		True if successful;	 otherwise false.
		@note		For the effect to be noticeable, the FrameStore should be enabled (see CNTV2Card::EnableChannel)
					and in ::NTV2_MODE_CAPTURE mode.
		@note		Normally, if the device ::NTV2RegisterWriteMode is ::NTV2_REGWRITE_SYNCTOFRAME, the new value takes
					effect at the next input <b>frame</b> interrupt. For example, if line 300 of frame 5 is currently
					being written in device memory at the instant this function is called with frame 6, video won't be
					written into frame 6 in device memory until the input VBI fires after the last line of frame 5 has
					been written.
		@note		If the FrameStore's ::NTV2RegisterWriteMode is ::NTV2_REGWRITE_SYNCTOFIELD, the new value takes effect
					at the next input <b>field</b> interrupt. This makes it possible to record lines from ::NTV2_FIELD0
					and ::NTV2_FIELD1 in separate frame buffers, if desired. See CNTV2Card::SetRegisterWriteMode and/or
					\ref fieldframeinterrupts for more information.
		@warning	If the designated FrameStore/channel is enabled and in ::NTV2_MODE_CAPTURE mode, and the given frame
					is within the frame range being used by another FrameStore/channel, this will likely result in torn/bad
					video in either or both channels. See \ref vidop-fbconflict
		@warning	If the designated FrameStore/channel is enabled and in ::NTV2_MODE_CAPTURE mode, and the given frame is
					in Audio Buffer memory that's in use by a running Audio System, this will likely result in torn/bad video
					and/or bad audio. See \ref audioclobber
		@see		CNTV2Card::GetInputFrame, \ref vidop-fs
	**/
	AJA_VIRTUAL bool		SetInputFrame (const NTV2Channel inChannel, const ULWord inValue);

	/**
		@brief		Answers with the current input frame index number for the given FrameStore.
					This identifies which particular frame in device SDRAM will be written after the next frame interrupt.
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
								(The FrameStore should be enabled and set for capture mode.)
		@param[out] outValue	Receives the current input frame index number of the frame in device memory being written.
								See \ref vidop-fbindexing for more information.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetInputFrame, \ref vidop-fs
	**/
	AJA_VIRTUAL bool		GetInputFrame (const NTV2Channel inChannel, ULWord & outValue);

	AJA_VIRTUAL bool		SetDualLinkOutputEnable (const bool inIsEnabled);
	AJA_VIRTUAL bool		GetDualLinkOutputEnable (bool & outIsEnabled);

	AJA_VIRTUAL bool		SetDualLinkInputEnable (const bool inIsEnabled);
	AJA_VIRTUAL bool		GetDualLinkInputEnable (bool & outIsEnabled);

	AJA_VIRTUAL bool		SetVideoLimiting (const NTV2VideoLimiting inValue);
	AJA_VIRTUAL bool		GetVideoLimiting (NTV2VideoLimiting & outValue);

	AJA_VIRTUAL bool		SetEnableVANCData (const bool inVANCenabled, const bool inTallerVANC, const NTV2Standard inStandard, const NTV2FrameGeometry inGeometry, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool		SetEnableVANCData (const bool inVANCenabled, const bool inTallerVANC = false, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Sets the VANC mode for the given FrameStore.
		@param[in]	inVancMode		Specifies the new ::NTV2VANCMode setting.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
									Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::GetVANCMode, \ref vancframegeometries
	**/
	AJA_VIRTUAL bool		SetVANCMode (const NTV2VANCMode inVancMode, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Sets the VANC mode for the given FrameStores.
		@param[in]	inChannels		Specifies the FrameStores of interest as any number of ::NTV2Channel values, each a zero-based index number.
		@param[in]	inVancMode		Specifies the new ::NTV2VANCMode setting.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::GetVANCMode, \ref vancframegeometries
	**/
	AJA_VIRTUAL bool		SetVANCMode (const NTV2ChannelSet & inChannels, const NTV2VANCMode inVancMode);

	/**
		@brief		Retrieves the current VANC mode for the given FrameStore.
		@param[out] outVancMode		Receives the current ::NTV2VANCMode setting.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
									Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::SetVANCMode, \ref vancframegeometries
	**/
	AJA_VIRTUAL bool		GetVANCMode (NTV2VANCMode & outVancMode, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Enables or disables the "VANC Shift Mode" feature for the given channel.
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
		@param[in]	inMode		Specifies the new data shift mode.
								Use ::NTV2_VANCDATA_NORMAL to disable;	use ::NTV2_VANCDATA_8BITSHIFT_ENABLE to enable.
		@return		True if successful;	 otherwise false.
		@note		The bit shift feature only affects VANC lines (not visible raster lines) and only when the device FrameStore is configured as follows:
					-	video format is set for an HD format (see ::NTV2_IS_HD_VIDEO_FORMAT macro) -- not SD or 4K/UHD;
					-	pixel format is set for ::NTV2_FBF_8BIT_YCBCR;
					-	VANC mode is set to ::NTV2_VANCMODE_TALL or ::NTV2_VANCMODE_TALLER (see CNTV2Card::SetVANCMode).
		@see		CNTV2Card::GetVANCShiftMode, CNTV2Card::GetVANCMode, CNTV2Card::SetVANCMode, \ref vancframegeometries
	**/
	AJA_VIRTUAL bool		SetVANCShiftMode (NTV2Channel inChannel, NTV2VANCDataShiftMode inMode);

	/**
		@brief		Sets the "VANC Shift Mode" for the given channel(s).
		@param[in]	inChannels	Specifies the FrameStore(s) of interest as ::NTV2Channel values (zero-based index numbers).
		@param[in]	inMode		Specifies the new data shift mode.
								Use ::NTV2_VANCDATA_NORMAL to disable;	use ::NTV2_VANCDATA_8BITSHIFT_ENABLE to enable.
		@return		True if successful;	 otherwise false.
		@note		The bit shift feature only affects VANC lines (not visible raster lines) and only when the device FrameStore is configured as follows:
					-	video format is set for an HD format (see ::NTV2_IS_HD_VIDEO_FORMAT macro) -- not SD or 4K/UHD;
					-	pixel format is set for ::NTV2_FBF_8BIT_YCBCR;
					-	VANC mode is set to ::NTV2_VANCMODE_TALL or ::NTV2_VANCMODE_TALLER (see CNTV2Card::SetVANCMode).
		@see		CNTV2Card::GetVANCShiftMode, CNTV2Card::GetVANCMode, CNTV2Card::SetVANCMode, \ref vancframegeometries
	**/
	AJA_VIRTUAL bool		SetVANCShiftMode (NTV2ChannelSet & inChannels, const NTV2VANCDataShiftMode inMode);	//	New in SDK 16.2

	/**
		@brief		Retrieves the current "VANC Shift Mode" feature for the given channel.
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
		@param[out] outValue	Receives the current ::NTV2VANCDataShiftMode setting.
								If ::NTV2_VANCDATA_NORMAL, then bit shifting is disabled.
								If ::NTV2_VANCDATA_8BITSHIFT_ENABLE, then it's enabled.
		@return		True if successful;	 otherwise false.
		@note		The bit shift feature only affects VANC lines (not visible raster lines) and only when the device FrameStore is configured as follows:
					-	video format is set for an HD format (see ::NTV2_IS_HD_VIDEO_FORMAT macro) -- not SD or 4K/UHD;
					-	pixel format is set for ::NTV2_FBF_8BIT_YCBCR;
					-	VANC mode is set to ::NTV2_VANCMODE_TALL or ::NTV2_VANCMODE_TALLER (see CNTV2Card::SetVANCMode).
		@see		CNTV2Card::SetVANCShiftMode, CNTV2Card::GetVANCMode, CNTV2Card::SetVANCMode, \ref vancframegeometries
	**/
	AJA_VIRTUAL bool		GetVANCShiftMode (NTV2Channel inChannel, NTV2VANCDataShiftMode & outValue);

	AJA_VIRTUAL bool		SetPulldownMode (NTV2Channel inChannel, bool inValue);
	AJA_VIRTUAL bool		GetPulldownMode (NTV2Channel inChannel, bool & outValue);

	/**
		@brief		Answers with the line offset into the frame currently being read (::NTV2_MODE_DISPLAY) or written
					(::NTV2_MODE_CAPTURE) for FrameStore 1.
		@param[out] outValue	Receives the line number currently being read or written.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		ReadLineCount (ULWord & outValue);

#if !defined(NTV2_DEPRECATE_16_3)
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetDefaultVideoOutMode(ULWord mode)); ///< @deprecated	Obsolete starting in SDK 16.3.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetDefaultVideoOutMode(ULWord & outMode)); ///< @deprecated	Obsolete starting in SDK 16.3.
#endif	//	defined(NTV2_DEPRECATE_16_3)
#if !defined(NTV2_DEPRECATE_16_2)
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetActiveFrameDimensions (NTV2FrameDimensions & outFrameDimensions, const NTV2Channel inChannel = NTV2_CHANNEL1)); ///< @deprecated	Obsolete starting in SDK 16.2.
	AJA_VIRTUAL NTV2_DEPRECATED_f(NTV2FrameDimensions GetActiveFrameDimensions (const NTV2Channel inChannel = NTV2_CHANNEL1)); ///< @deprecated	Obsolete starting in SDK 16.2.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetNumberActiveLines (ULWord & outNumActiveLines)); ///< @deprecated	Obsolete starting in SDK 16.2.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool FlipFlopPage (const NTV2Channel inChannel)); ///< @deprecated	Declared obsolete starting in SDK 16.2. Swapped the PCI access frame and output frame registers at the next output VBI.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetPCIAccessFrame (const NTV2Channel inChannel, const ULWord inValue, const bool inWaitForVBI = true)); ///< @deprecated	Declared obsolete starting in SDK 16.2. The "PCI Access Frame" register had no effect on the hardware, but was used to store the next/pending output frame.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetPCIAccessFrame (const NTV2Channel inChannel, ULWord & outValue)); ///< @deprecated	Declared obsolete starting in SDK 16.2. The "PCI Access Frame" register had no effect on the hardware, but was used to store the next/pending output frame.
#endif	//	!defined(NTV2_DEPRECATE_16_2)
#if !defined(NTV2_DEPRECATE_16_0)
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool SetEnableVANCData (const NTV2ChannelSet & inChannels, const bool inVANCenable, const bool inTallerVANC = false))	{return SetVANCMode(inChannels, NTV2VANCModeFromBools(inVANCenable, inTallerVANC));}
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool SetVANCMode (const NTV2VANCMode inVancMode, const NTV2Standard st, const NTV2FrameGeometry fg,
																	const NTV2Channel inChannel = NTV2_CHANNEL1))	{(void) st; (void) fg; return SetVANCMode(inVancMode, inChannel);}
	#define Set425FrameEnable	SetTsiFrameEnable	//	Replace calls to Set425FrameEnable with calls to SetTsiFrameEnable instead
	#define Get425FrameEnable	GetTsiFrameEnable	//	Replace calls to Get425FrameEnable with calls to GetTsiFrameEnable instead
#endif	//	NTV2_DEPRECATE_16_0


	/**
		@name	Mixer/Keyer & Video Processing
	**/
	///@{

	/**
		@brief		Sets the VANC source for the given mixer/keyer to the foreground video (or not).
					See the \ref ancillarydata discussion for more information.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer			Specifies the mixer/keyer to be affected as a zero-based index number.
		@param[in]	inFromForegroundSource	If true, sets the mixer/keyer's VANC source to its foreground video input;
											otherwise, sets it to its background video input.
		@see		CNTV2Card::GetMixerVancOutputFromForeground, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	SetMixerVancOutputFromForeground (const UWord inWhichMixer, const bool inFromForegroundSource = true);

	/**
		@brief		Answers whether or not the VANC source for the given mixer/keyer is currently the foreground video.
					See the \ref ancillarydata discussion for more information.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer				Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outIsFromForegroundSource	Receives True if the mixer/keyer's VANC source is its foreground video input;
												otherwise False if it's its background video input.
		@see		CNTV2Card::SetMixerVancOutputFromForeground, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	GetMixerVancOutputFromForeground (const UWord inWhichMixer, bool & outIsFromForegroundSource);


	/**
		@brief		Sets the foreground input control value for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inInputControl		Specifies the mixer/keyer's foreground input control value.
		@see		CNTV2Card::GetMixerFGInputControl, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	SetMixerFGInputControl (const UWord inWhichMixer, const NTV2MixerKeyerInputControl inInputControl);

	/**
		@brief		Returns the current foreground input control value for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outInputControl		Receives the mixer/keyer's foreground input control value; otherwise NTV2MIXERINPUTCONTROL_INVALID upon failure.
		@see		CNTV2Card::SetMixerFGInputControl, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	GetMixerFGInputControl (const UWord inWhichMixer, NTV2MixerKeyerInputControl & outInputControl);

	/**
		@brief		Sets the background input control value for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inInputControl		Specifies the mixer/keyer's background input control value.
		@see		CNTV2Card::GetMixerBGInputControl, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	SetMixerBGInputControl (const UWord inWhichMixer, const NTV2MixerKeyerInputControl inInputControl);

	/**
		@brief		Returns the current background input control value for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outInputControl		Receives the mixer/keyer's background input control value; otherwise NTV2MIXERINPUTCONTROL_INVALID upon failure.
		@see		CNTV2Card::SetMixerBGInputControl, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	GetMixerBGInputControl (const UWord inWhichMixer, NTV2MixerKeyerInputControl & outInputControl);

	/**
		@brief		Sets the mode for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inMode				Specifies the mode value.
		@see		CNTV2Card::GetMixerMode, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	SetMixerMode (const UWord inWhichMixer, const NTV2MixerKeyerMode inMode);

	/**
		@brief		Returns the current operating mode of the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outMode				Receives the mode value.
		@see		CNTV2Card::SetMixerMode, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	GetMixerMode (const UWord inWhichMixer, NTV2MixerKeyerMode & outMode);

	/**
		@brief		Sets the current mix coefficient of the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inMixCoefficient	Specifies the new mix coefficient value, where \c 0x10000 is the maximum.
		@see		CNTV2Card::GetMixerCoefficient, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	SetMixerCoefficient (const UWord inWhichMixer, const ULWord inMixCoefficient);

	/**
		@brief		Returns the current mix coefficient the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outMixCoefficient	Receives the current mix coefficient value, where \c 0x10000 is the maximum.
		@see		CNTV2Card::SetMixerCoefficient, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	GetMixerCoefficient (const UWord inWhichMixer, ULWord & outMixCoefficient);

	/**
		@brief		Returns the current sync state of the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outIsSyncOK			Receives the mixer's current sync state. If true, the mixer is synchronized to its inputs.
										If false, the mixer is not synchronized to its inputs.
		@see		See \ref vidop-mixerkeyer for more information
	**/
	AJA_VIRTUAL bool	GetMixerSyncStatus (const UWord inWhichMixer, bool & outIsSyncOK);

	/**
		@brief		Answers if the given mixer/keyer's foreground matte is enabled or not.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outIsEnabled		Receives true if the mixer's foreground matte is currently enabled;	 otherwise false.
		@see		CNTV2Card::SetMixerFGMatteEnabled, CNTV2Card::GetMixerBGMatteEnabled, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	GetMixerFGMatteEnabled (const UWord inWhichMixer, bool & outIsEnabled);

	/**
		@brief		Answers if the given mixer/keyer's foreground matte is enabled or not.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inIsEnabled			Specify true to enable the mixer's foreground matte;  otherwise false to disable it.
		@see		CNTV2Card::GetMixerFGMatteEnabled, CNTV2Card::SetMixerBGMatteEnabled, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	SetMixerFGMatteEnabled (const UWord inWhichMixer, const bool inIsEnabled);

	/**
		@brief		Answers if the given mixer/keyer's background matte is enabled or not.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outIsEnabled		Receives true if the mixer's background matte is currently enabled;	 otherwise false.
		@see		CNTV2Card::SetMixerBGMatteEnabled, CNTV2Card::GetMixerFGMatteEnabled, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	GetMixerBGMatteEnabled (const UWord inWhichMixer, bool & outIsEnabled);

	/**
		@brief		Answers if the given mixer/keyer's background matte is enabled or not.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inIsEnabled			Specify true to enable the mixer's background matte;  otherwise false to disable it.
		@see		CNTV2Card::GetMixerBGMatteEnabled, CNTV2Card::SetMixerFGMatteEnabled, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	SetMixerBGMatteEnabled (const UWord inWhichMixer, const bool inIsEnabled);

	/**
		@brief		Answers with the given mixer/keyer's current matte color value being used.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outYCbCrValue		Receives the mixer's current matte color value.
		@see		CNTV2Card::SetMixerMatteColor, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	GetMixerMatteColor (const UWord inWhichMixer, YCbCr10BitPixel & outYCbCrValue);

	/**
		@brief		Sets the matte color to use for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inYCbCrValue		Specifies the new matte color value to use.
		@see		CNTV2Card::GetMixerMatteColor, CNTV2Card::GetMixerFGMatteEnabled, CNTV2Card::SetMixerFGMatteEnabled, CNTV2Card::GetMixerBGMatteEnabled, CNTV2Card::SetMixerBGMatteEnabled, \ref vidop-mixerkeyer, \ref widget_mixkey
	**/
	AJA_VIRTUAL bool	SetMixerMatteColor (const UWord inWhichMixer, const YCbCr10BitPixel inYCbCrValue);
	
	/**
		@brief		Answers if the given mixer/keyer's has RGB mode support.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outIsSupported		Receives true if the mixer has RGB mode support.
	**/
	AJA_VIRTUAL bool	MixerHasRGBModeSupport (const UWord inWhichMixer, bool & outIsSupported);
	
	/**
		@brief		Sets the RGB range for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inRGBRange			Specifies the new RGB range value to use.
	**/
	AJA_VIRTUAL bool	SetMixerRGBRange (const UWord inWhichMixer, const NTV2MixerRGBRange inRGBRange);
	
	/**
		@brief		Answers with the given mixer/keyer's current RGB Range.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[out] outRGBRange		Receives the mixer's current matte color value.
	**/
	AJA_VIRTUAL bool	GetMixerRGBRange (const UWord inWhichMixer, NTV2MixerRGBRange & outRGBRange);
	///@}


	/**
		@name	Audio
	**/
	///@{

	/**
		@brief		Sets the number of audio channels to be concurrently captured or played for a given Audio System on the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inNumChannels	Specifies the number of audio channels the device will record or play to/from a
									given Audio System. For most applications, this should always be set to the maximum
									number of audio channels the device is capable of capturing or playing, which can
									be obtained by calling the ::NTV2DeviceGetMaxAudioChannels function.
		@param[in]	inAudioSystem	Optionally specifies the Audio System of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
	**/
	AJA_VIRTUAL bool		SetNumberAudioChannels (const ULWord inNumChannels, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Sets the number of audio channels to be concurrently captured or played for the given Audio System(s).
		@return		True if successful; otherwise false.
		@param[in]	inNumChannels	Specifies the number of audio channels the device will record or play.
									For most applications, this should always be set to the maximum number of audio channels
									the device is capable of capturing or playing (see ::NTV2DeviceGetMaxAudioChannels ).
		@param[in]	inAudioSystems	Specifies the Audio System(s) of interest.
	**/
	AJA_VIRTUAL bool		SetNumberAudioChannels (const ULWord inNumChannels, const NTV2AudioSystemSet & inAudioSystems);	//	New in SDK 16.2

	/**
		@brief		Returns the current number of audio channels being captured or played by a given Audio System on the AJA device.
		@return		True if successful; otherwise false.
		@param[out] outNumChannels		Receives the number of audio channels that the AJA device hardware is currently capturing
										or playing for the given Audio System. If the function result is true, the variable's
										contents will be valid, and for most AJA devices will be 6, 8, or 16.
		@param[in]	inAudioSystem		Optionally specifies the Audio System of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@details	This function allows client applications to determine how many audio channels the AJA hardware is
					currently capturing/playing into/from the given Audio System on the device.
	**/
	AJA_VIRTUAL bool		GetNumberAudioChannels (ULWord & outNumChannels, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Sets the NTV2AudioRate for the given Audio System.
		@return		True if successful; otherwise false.
		@param[in]	inRate			Specifies the new NTV2AudioRate.
		@param[in]	inAudioSystem	Optionally specifies the Audio System of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@note		Current generation NTV2 devices only support a fixed 48 kHz sample rate.
	**/
	AJA_VIRTUAL bool		SetAudioRate (const NTV2AudioRate inRate, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Returns the current NTV2AudioRate for the given Audio System.
		@return		True if successful; otherwise false.
		@param[out] outRate			Receives the current NTV2AudioRate.
		@param[in]	inAudioSystem	Optionally specifies the Audio System of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@note		Current generation NTV2 devices only support a fixed 48 kHz sample rate.
	**/
	AJA_VIRTUAL bool		GetAudioRate (NTV2AudioRate & outRate, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Changes the size of the audio buffer that is used for a given Audio System in the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inValue			Specifies the desired size of the capture/playout audio buffer to be used on the AJA device.
									All modern AJA devices use ::NTV2_AUDIO_BUFFER_BIG (4 MB).
		@param[in]	inAudioSystem	Optionally specifies the Audio System of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@see		CNTV2Card::GetAudioBufferSize
	**/
	AJA_VIRTUAL bool		SetAudioBufferSize (const NTV2AudioBufferSize inValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Changes the size of the audio buffer used for the given Audio System(s).
		@return		True if successful; otherwise false.
		@param[in]	inValue			Specifies the desired size of the capture/playout audio buffer to be used on the AJA device.
									All modern AJA devices use ::NTV2_AUDIO_BUFFER_BIG (4 MB).
		@param[in]	inAudioSystems	Specifies the Audio System(s) of interest.
		@see		CNTV2Card::GetAudioBufferSize
	**/
	AJA_VIRTUAL bool		SetAudioBufferSize (const NTV2AudioBufferSize inMode, const NTV2AudioSystemSet & inAudioSystems);	//	New in SDK 16.2

	/**
		@brief		Retrieves the size of the input or output audio buffer being used for a given Audio System on the AJA device.
		@return		True if successful; otherwise false.
		@param[out] outSize			Receives the size of the capture/playout audio buffer for the given Audio System on the AJA device.
		@param[in]	inAudioSystem	Optionally specifies the Audio System of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@see		CNTV2Card::SetAudioBufferSize
	**/
	AJA_VIRTUAL bool		GetAudioBufferSize (NTV2AudioBufferSize & outSize, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Enables or disables 20-bit mode for the ::NTV2AudioSystem.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem on the device to be affected.
		@param[in]	inEnable		Specify \c true to use 20-bit mode;	 specify \c false for normal (24-bit) mode.
		@note		This function is relevant only for the \ref konaip or \ref ioip.
	**/
	AJA_VIRTUAL bool		SetAudio20BitMode (const NTV2AudioSystem inAudioSystem, const bool inEnable);	//	New in SDK 13.0

	/**
		@brief		Answers whether or not the device's ::NTV2AudioSystem is currently operating in 20-bit mode.
					Normally the ::NTV2AudioSystem operates in 24-bit mode.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest.
		@param[out] outEnable		Receives 'true' if the Audio System is in 20 bit mode, or 'false'
									if audio is in 24 bit mode.
		@note		This function is relevant only for the \ref konaip or \ref ioip.
	**/
	AJA_VIRTUAL bool		GetAudio20BitMode (const NTV2AudioSystem inAudioSystem, bool & outEnable);	//	New in SDK 13.0

	/**
		@brief		Enables or disables ::NTV2AudioLoopBack mode for the given ::NTV2AudioSystem.
		@return		True if successful; otherwise false.
		@param[in]	inMode			Specify ::NTV2_AUDIO_LOOPBACK_ON to force the Audio System's output embedder, to pull audio samples
									from the Audio System's input de-embedder.
									Specify ::NTV2_AUDIO_LOOPBACK_OFF to have the output embedder emit silence (zeroes).
		@param[in]	inAudioSystem	Optionally specifies the Audio System on the device to be affected. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@see		CNTV2Card::GetAudioLoopBack, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioLoopBack (const NTV2AudioLoopBack inMode, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Enables or disables ::NTV2AudioLoopBack mode for the given Audio Systems.
		@return		True if successful; otherwise false.
		@param[in]	inMode			Specify ::NTV2_AUDIO_LOOPBACK_ON to force each Audio System's output embedder to pull audio samples
									from its corresponding SDI input de-embedder;  otherwise specify ::NTV2_AUDIO_LOOPBACK_OFF.
		@param[in]	inAudioSystems	Specifies the Audio System(s) on the device to be affected.
		@see		CNTV2Card::GetAudioLoopBack, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioLoopBack (const NTV2AudioLoopBack inMode, const NTV2AudioSystemSet & inAudioSystems);	//	New in SDK 16.2

	/**
		@brief		Answers if ::NTV2AudioLoopBack mode is currently on or off for the given ::NTV2AudioSystem.
		@return		True if successful; otherwise false.
		@param[out] outMode			Receives ::NTV2_AUDIO_LOOPBACK_ON if the Audio System's output embedder will pull audio samples from
									the Audio System's input de-embedder;  otherwise receives ::NTV2_AUDIO_LOOPBACK_OFF if the output
									embedder emits silence (zeroes).
		@param[in]	inAudioSystem	Optionally specifies the Audio System on the device to be affected. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@see		CNTV2Card::SetAudioLoopBack, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioLoopBack (NTV2AudioLoopBack & outMode, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);


	AJA_VIRTUAL bool		SetAudioAnalogLevel (const NTV2AudioLevel value, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool		GetAudioAnalogLevel (NTV2AudioLevel & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool		SetEncodedAudioMode (const NTV2EncodedAudioMode value, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool		GetEncodedAudioMode (NTV2EncodedAudioMode & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Sets the ::NTV2EmbeddedAudioClock setting for the given ::NTV2AudioSystem.
		@return		True if successful; otherwise false.
		@param[in]	inValue			Specifies the ::NTV2EmbeddedAudioClock setting to use.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@see		CNTV2Card::SetEmbeddedAudioClock, ::NTV2DeviceCanChangeEmbeddedAudioClock, \ref audiocapture
	**/
	AJA_VIRTUAL bool		SetEmbeddedAudioClock (const NTV2EmbeddedAudioClock inValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		For the given ::NTV2AudioSystem, answers with the current ::NTV2EmbeddedAudioClock setting.
		@return		True if successful; otherwise false.
		@param[out] outValue		Receives the current ::NTV2EmbeddedAudioClock setting.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
		@see		CNTV2Card::SetEmbeddedAudioClock, ::NTV2DeviceCanChangeEmbeddedAudioClock, \ref audiocapture
	**/
	AJA_VIRTUAL bool		GetEmbeddedAudioClock (NTV2EmbeddedAudioClock & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		For the given Audio System, answers with the wrap address, the threshold at which input/record or output/play transfers
					will likely need to account for "wrapping" around the end of the buffer back to the start of the buffer to continue
					recording/playing.
		@note		Since input/record wrap detection is simple -- i.e., the value returned from CNTV2Card::ReadAudioLastIn has decreased --
					this function is provided to detect when an output wrap will occur.
		@return		True if successful; otherwise false.
		@param[out] outWrapAddress	Receives the byte offset in the audio output buffer at which a "wrap" will occur.
									This is typically 16KB from the end of the buffer. If the current offset plus the audio bytes to be
									written will go past this position, the caller should split the DMA transfer into two separate ones:
									one to fill to the end of the buffer, and the remainder from the start of the buffer.
		@param[in]	inAudioSystem	Optionally specifies the Audio System of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
	**/
	AJA_VIRTUAL bool		GetAudioWrapAddress (ULWord & outWrapAddress, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		For the given Audio System, answers with the byte offset from the start of the audio buffer to the first byte
					of the input/capture buffer. (The capture buffer immediately follows the playout buffer in increasing memory
					address order.)
		@return		True if successful; otherwise false.
		@param[out] outReadOffset	Receives the offset to the audio capture buffer from the start of the audio buffer.
									This will typically be 4MB.
		@param[in]	inAudioSystem	Optionally specifies the Audio System of interest. Defaults to ::NTV2_AUDIOSYSTEM_1.
	**/
	AJA_VIRTUAL bool		GetAudioReadOffset (ULWord & outReadOffset, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Answers with the byte offset in device SDRAM into the specified Audio System's audio buffer.
		@return		True if successful; otherwise false.
		@param[in]	inOffsetBytes		Specifies a byte offset as measured from the top of the Audio System's audio buffer memory.
										If "inCaptureBuffer" is set to 'true', this value represents the offset as measured from
										the top of the Audio System's capture buffer.
		@param[out] outAbsByteOffset	Receives the equivalent absolute byte offset in device SDRAM.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[in]	inCaptureBuffer		If true, "inOffsetBytes" is to be interpreted as relative to the start of the audio
										capture buffer. If false, the default, "inOffsetBytes" is relative to the start of
										the audio playout buffer.
	**/
	AJA_VIRTUAL bool		GetAudioMemoryOffset (const ULWord inOffsetBytes,  ULWord & outAbsByteOffset,
												const NTV2AudioSystem inAudioSystem, const bool inCaptureBuffer = false);	//	New in SDK 13.0

	/**
		@brief		For the given Audio System, answers with the byte offset of the tail end of the last chunk of
					audio samples read by the device's output audio embedder. This is essentially the position of
					the "Play Head" during audio output.
		@param[out] outValue		Receives the byte offset of the tail end of the last chunk of audio samples read
									by the device's output audio embedder in its output audio buffer. This offset is
									measured from the start of the device playback buffer.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::WriteAudioLastOut, \ref audioplayout
	**/
	AJA_VIRTUAL bool		ReadAudioLastOut (ULWord & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		For the given Audio System, answers with the byte offset to the last byte of the latest chunk of
					4-byte audio samples written into the device's input audio buffer by its input audio de-embedder.
					This is essentially the position of the "Write Head" during audio capture.
		@param[out] outValue		Receives the byte offset of the last byte of audio data written into the input
									audio buffer. This offset is measured from the start of the device capture buffer.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest.
		@return		True if successful;	 otherwise false.
		@see		\ref audiocapture
	**/
	AJA_VIRTUAL bool		ReadAudioLastIn (ULWord & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Starts the playout side of the given ::NTV2AudioSystem, reading outgoing audio samples
					from the Audio System's playout buffer.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[in]	inWaitForVBI		If 'false', the default, immediately starts reading audio samples from the
										playout buffer and inserting into the HANC stream.	If 'true', checks if
										the required firmware feature is present, and if so, waits until the next
										output VBI before starting to read samples from the playout buffer.
		@note		It is not an error to call this function when the Audio System's playout side is already running.
		@note		Applications using \ref aboutautocirculate won't need to call this function, since AutoCirculate
					configures the Audio System automatically.
		@see		CNTV2Card::StopAudioOutput, CNTV2Card::IsAudioOutputRunning, CNTV2Card::CanDoAudioWaitForVBI, \ref audioplayout
	**/
	AJA_VIRTUAL bool		StartAudioOutput (const NTV2AudioSystem inAudioSystem, const bool inWaitForVBI = false);	//	New in SDK 14.3

	/**
		@brief		Stops the playout side of the given ::NTV2AudioSystem, parking the "Read Head" at the start
					of the playout buffer.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@note		It is not an error to call this function when the Audio System's playout side is already stopped.
		@note		Applications using \ref aboutautocirculate won't need to call this function, since AutoCirculate
					configures the Audio System automatically.
		@see		CNTV2Card::StartAudioOutput, CNTV2Card::IsAudioOutputRunning, \ref audioplayout
	**/
	AJA_VIRTUAL bool		StopAudioOutput (const NTV2AudioSystem inAudioSystem);	//	New in SDK 14.3

	/**
		@brief		Answers whether or not the playout side of the given ::NTV2AudioSystem is currently running.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[out] outIsRunning		Receives 'true' if the Audio System's playout side is currently running;
										otherwise receives 'false'.
		@see		CNTV2Card::StartAudioOutput, CNTV2Card::StopAudioOutput, CNTV2Card::SetAudioOutputPause,
					CNTV2Card::GetAudioOutputPause, \ref audioplayout
	**/
	AJA_VIRTUAL bool		IsAudioOutputRunning (const NTV2AudioSystem inAudioSystem, bool & outIsRunning);	//	New in SDK 14.3

	/**
		@brief		Pauses or resumes output of audio samples and advancement of the audio buffer pointer
					("play head") of the given Audio System.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[in]	inPausePlayout		Specify 'true' to pause producing audio samples ("paused").
										Specify 'false' to resume producing audio samples ("running").
		@note		This function doesn't alter the Audio Output Buffer Pointer ("play head") position.
					To reset the "play head" back to the beginning of the buffer, use CNTV2Card::SetAudioOutputReset instead.
		@see		CNTV2Card::GetAudioOutputPause, CNTV2Card::StartAudioOutput, CNTV2Card::StopAudioOutput, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioOutputPause (const NTV2AudioSystem inAudioSystem, const bool inPausePlayout);	//	New in SDK 12.3

	/**
		@brief		Answers if the device's Audio System is currently paused or not.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[out] outIsPaused			Receives 'true' if audio output is paused, or 'false' if audio
										playout is running normally.
		@see		CNTV2Card::SetAudioOutputPause, CNTV2Card::IsAudioOutputRunning, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioOutputPause (const NTV2AudioSystem inAudioSystem, bool & outIsPaused);	//	New in SDK 12.3

	/**
		@brief		Starts the capture side of the given ::NTV2AudioSystem, writing incoming audio samples
					into the Audio System's capture buffer.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[in]	inWaitForVBI		If 'false', the default, immediately starts writing captured audio samples
										into the capture buffer.  If 'true', checks if the required firmware feature
										is present, and if so, waits until the next input VBI before starting to
										write captured samples into the capture buffer.
		@note		It is not an error to call this function when the Audio System's capture side is already running.
		@note		Applications using \ref aboutautocirculate won't need to call this function, since AutoCirculate
					configures the Audio System automatically.
		@see		CNTV2Card::StopAudioInput, CNTV2Card::IsAudioInputRunning, \ref audiocapture
	**/
	AJA_VIRTUAL bool		StartAudioInput (const NTV2AudioSystem inAudioSystem, const bool inWaitForVBI = false);	//	New in SDK 14.3

	/**
		@brief		Stops the capture side of the given ::NTV2AudioSystem, and resets the capture position
					(i.e. "Write Head") back to the start of the Audio System's capture buffer (offset zero).
					This can be useful for resynchronizing audio and video.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@note		It is not an error to call this function when the Audio System's capture side is already stopped.
		@note		Applications using \ref aboutautocirculate won't need to call this function, since AutoCirculate
					configures the Audio System automatically.
		@see		CNTV2Card::StartAudioInput, CNTV2Card::IsAudioInputRunning, \ref audiocapture
	**/
	AJA_VIRTUAL bool		StopAudioInput (const NTV2AudioSystem inAudioSystem);	//	New in SDK 14.3

	/**
		@brief		Answers whether or not the capture side of the given ::NTV2AudioSystem is currently running.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[out] outIsRunning		Receives 'true' if the Audio System's capture side is currently running;
										otherwise receives 'false'.
		@see		CNTV2Card::StartAudioInput, CNTV2Card::StopAudioInput, \ref audiocapture
	**/
	AJA_VIRTUAL bool		IsAudioInputRunning (const NTV2AudioSystem inAudioSystem, bool & outIsRunning);	//	New in SDK 14.3

	/**
		@brief		Enables or disables the writing of incoming audio into the given Audio System's capture buffer.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the Audio System of interest.
		@param[in]	inEnable		If true, the Audio System will write captured samples into device audio buffer
									memory (if currently running). If false, the Audio System will not write captured
									samples into device audio buffer memory.
		@note		Applications using \ref aboutautocirculate won't need to call this function, since AutoCirculate
					configures the Audio System automatically.
		@see		CNTV2Card::GetAudioCaptureEnable, \ref audiocapture
	**/
	AJA_VIRTUAL bool		SetAudioCaptureEnable (const NTV2AudioSystem inAudioSystem, const bool inEnable);	//	New in SDK 12.3

	/**
		@brief		Answers whether or not the Audio System is configured to write captured audio samples into
					device audio buffer memory.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[out] outEnable			Receives 'true' if the Audio System will write captured samples into device
										audio buffer memory when running;  otherwise 'false' if the Audio System is
										prohibited from writing captured samples into device audio buffer memory.
		@see		CNTV2Card::SetAudioCaptureEnable, \ref audiocapture
	**/
	AJA_VIRTUAL bool		GetAudioCaptureEnable (const NTV2AudioSystem inAudioSystem, bool & outEnable);	//	New in SDK 12.3

	/**
		@brief		Enables or disables a special mode for the given Audio System whereby its embedder and
					deembedder both start from the audio base address, instead of operating independently.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System on the device to be affected.
		@param[in]	inEnable			If true, de-embedder and embedder both start from the audio base address.
										If false, the Audio System operates normally.
	**/
	AJA_VIRTUAL bool		SetAudioPlayCaptureModeEnable (const NTV2AudioSystem inAudioSystem, const bool inEnable);

	/**
		@brief		Answers whether or not the device's Audio System is currently operating in a special mode
					in which its embedder and deembedder both start from the audio base address, instead of
					operating independently.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the Audio System of interest.
		@param[out] outEnable			A boolean variable that is to receive 'true' if the Audio System's
										de-embedder and embedder both start from the audio base address,
										or 'false' if the Audio System is operating normally.
	**/
	AJA_VIRTUAL bool		GetAudioPlayCaptureModeEnable (const NTV2AudioSystem inAudioSystem, bool & outEnable);

	/**
		@brief		Sets the audio input delay for the given Audio System on the device.
		@param[in]	inAudioSystem	Specifies the Audio System whose input delay is to be set.
		@param[in]	inDelay			Specifies the new audio input delay value for the Audio System.
									Each increment of this value increases the delay by exactly 512 bytes
									in the Audio System's audio buffer, or about 166.7 microseconds.
									Values from 0 thru 8159 are valid, which gives a maximum delay of about
									1.36 seconds.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioInputDelay, \ref audiocapture
	**/
	AJA_VIRTUAL bool		SetAudioInputDelay (const NTV2AudioSystem inAudioSystem, const ULWord inDelay);

	/**
		@brief		Answers with the audio input delay for the given Audio System on the device.
		@param[in]	inAudioSystem	Specifies the Audio System whose input delay is to be retrieved.
		@param[out] outDelay		A ULWord variable that is to receive the Audio System's current input delay
									value, expressed as an integral number of 512 byte chunks in the Audio System's
									audio buffer on the device. This can be translated into microseconds by multiplying
									the received value by 166.7.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioInputDelay, \ref audiocapture
	**/
	AJA_VIRTUAL bool		GetAudioInputDelay (const NTV2AudioSystem inAudioSystem, ULWord & outDelay);


	/**
		@brief		Sets the audio output delay for the given Audio System on the device.
		@param[in]	inAudioSystem	Specifies the Audio System whose output delay is to be set.
		@param[in]	inDelay			Specifies the new audio output delay value for the Audio System. Each increment of this
									value increases the delay by exactly 512 bytes in the Audio System's audio buffer, or
									about 166.7 microseconds (when configured for 16 audio channels). Values from 0 to 8159
									are valid, which gives a maximum delay of about 1.36 seconds.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioOutputDelay, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioOutputDelay (const NTV2AudioSystem inAudioSystem, const ULWord inDelay);

	/**
		@brief		Answers with the audio output delay for the given Audio System on the device.
		@param[in]	inAudioSystem	Specifies the Audio System whose output delay is to be retrieved.
		@param[out] outDelay		A ULWord variable that is to receive the Audio System's current output delay
									value, expressed as an integral number of 512-byte chunks in the Audio System's
									audio buffer on the device. This can be translated into microseconds by multiplying
									the received value by 166.7.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioOutputDelay, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioOutputDelay (const NTV2AudioSystem inAudioSystem, ULWord & outDelay);


	/**
		@brief		Determines whether or not all outgoing audio channel pairs are to be flagged as non-PCM for the given Audio
					System on the device.
		@param[in]	inAudioSystem	Specifies the Audio System of interest.
		@param[in]	inIsNonPCM		If true, the "non-PCM" indicator in the AES header of all outgoing audio channel pairs is set "On";
									otherwise, the indicator is set "Off" (to indicate normal PCM audio).
		@return		True if successful;	 otherwise false.
		@note		This setting, if non-PCM, overrides per-audio-channel-pair PCM control on those devices that support it
					(see ::NTV2DeviceCanDoPCMControl).
		@see		CNTV2Card::GetAudioPCMControl, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const bool inIsNonPCM);	//	New in SDK 12.1


	/**
		@brief		Answers whether or not all outgoing audio channel pairs are currently being flagged as non-PCM for the
					given Audio System on the device.
		@param[in]	inAudioSystem	Specifies the Audio System of interest.
		@param[out] outIsNonPCM		Receives true if all outgoing audio channel pairs are currently flagged as non-PCM;
									otherwise receives false if the non-PCM indicator is set "Off".
		@return		True if successful; otherwise false.
		@see		CNTV2Card::SetAudioPCMControl, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, bool & outIsNonPCM);	//	New in SDK 12.1


	/**
		@brief		Determines whether the given audio channel pair being transmitted by the given Audio System is to be
					flagged as non-PCM or not.
		@param[in]	inAudioSystem	Specifies the Audio System of interest.
		@param[in]	inChannelPair	Specifies the audio channel pair of interest.
		@param[in]	inIsNonPCM		If true, the "non-PCM" indicator in the AES header of the channel pair is set "On";
									otherwise, the indicator is set "Off" (to indicate normal PCM audio).
		@return		True if successful;	 otherwise false.
		@note		Call ::NTV2DeviceCanDoPCMControl to determine if per-audio-channel-pair PCM control capability is available on this device.
		@note		This function has no effect if the Audio-System-wide non-PCM control setting is set to non-PCM.
					(See the two-parameter overloaded version of this function.)
		@see		CNTV2Card::GetAudioPCMControl, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelPair, const bool inIsNonPCM);	//	New in SDK 12.2


	/**
		@brief		Sets the non-PCM indicator in the AES header of the given channel pairs for the given Audio System on the device.
		@param[in]	inAudioSystem			Specifies the Audio System of interest.
		@param[in]	inNonPCMChannelPairs	Specifies the audio channel pairs whose non-PCM indicators will be set "On".
		@return		True if successful; otherwise false.
		@note		Call ::NTV2DeviceCanDoPCMControl to determine if this device supports per-audio-channel-pair PCM control.
		@see		CNTV2Card::GetAudioPCMControl, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPairs & inNonPCMChannelPairs);	//	New in SDK 12.4


	/**
		@brief		Answers whether or not the given audio channel pair in the given Audio System on the device is being treated
					as normal PCM audio.
		@param[in]	inAudioSystem	Specifies the Audio System of interest.
		@param[in]	inChannelPair	Specifies the channel pair of interest.
		@param[out] outIsNonPCM		Receives true if the audio channel pair is currently being flagged as non-PCM;
									otherwise false.
		@return		True if successful; otherwise false.
		@note		Call ::NTV2DeviceCanDoPCMControl to determine if this device supports per-audio-channel-pair PCM control.
		@note		This function's answer is irrelevant if the Audio-System-wide non-PCM control setting is set to non-PCM.
					(See the two-parameter overloaded version of this function.)
		@see		CNTV2Card::SetAudioPCMControl, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelPair, bool & outIsNonPCM);	//	New in SDK 12.2


	/**
		@brief		Answers which ::NTV2AudioChannelPairs being transmitted by the given ::NTV2AudioSystem are currently
					being flagged as non-PCM.
		@param[in]	inAudioSystem			Specifies the ::NTV2AudioSystem of interest.
		@param[out] outNonPCMChannelPairs	Receives the ::NTV2AudioChannelPairs that are currently being flagged as non-PCM.
		@return		True if successful; otherwise false.
		@note		Call ::NTV2DeviceCanDoPCMControl to determine if this device supports per-audio-channel-pair PCM control.
		@see		CNTV2Card::SetAudioPCMControl, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, NTV2AudioChannelPairs & outNonPCMChannelPairs);	//	New in SDK 12.4


	/**
		@brief		Answers whether or not the given ::NTV2AudioChannelPair in the given ::NTV2AudioSystem on the device is present in the input signal.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest.
		@param[in]	inChannelPair	Specifies the ::NTV2AudioChannelPair of interest.
		@param[out] outIsPresent	Receives true if the ::NTV2AudioChannelPair is present;	 otherwise false if it's not present.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::GetDetectedAudioChannelPairs, \ref audiocapture
	**/
	AJA_VIRTUAL bool		IsAudioChannelPairPresent (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelPair, bool & outIsPresent);	//	New in SDK 12.5


	/**
		@brief		Answers which audio channel pairs are present in the given Audio System's input stream.
		@param[in]	inAudioSystem				Specifies the ::NTV2AudioSystem of interest.
		@param[out] outDetectedChannelPairs		Receives the ::NTV2AudioChannelPairs that are present in the Audio System's input stream.
		@return		True if successful; otherwise false.
		@note		NTV2 device firmware performs this detection using a simple method of detecting the presence of the Audio Group's data packet.
					It does not perform detailed inspection of the packet -- i.e., checking bits b1/b2 of the AES sub-frame per SMPTE 272, nor
					checking the V/U/C/P bits per SMPTE 299.
		@see		CNTV2Card::IsAudioChannelPairPresent, \ref audiocapture
	**/
	AJA_VIRTUAL bool		GetDetectedAudioChannelPairs (const NTV2AudioSystem inAudioSystem, NTV2AudioChannelPairs & outDetectedChannelPairs);	//	New in SDK 12.5


	/**
		@brief		Answers which AES/EBU audio channel pairs are present on the device.
		@param[out] outDetectedChannelPairs		Receives the set of unique audio channel pairs that are present in any of the device's AES/EBU inputs.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::IsAudioChannelPairPresent, \ref audiocapture
	**/
	AJA_VIRTUAL bool		GetDetectedAESChannelPairs (NTV2AudioChannelPairs & outDetectedChannelPairs);	//	New in SDK 13.0


	/**
		@brief		Sets the audio source for the given ::NTV2AudioSystem on the device.
		@param[in]	inAudioSystem		Specifies the ::NTV2AudioSystem of interest.
		@param[in]	inAudioSource		Specifies the ::NTV2AudioSource to use for the given ::NTV2AudioSystem
										e.g., ::NTV2_AUDIO_EMBEDDED, ::NTV2_AUDIO_AES, ::NTV2_AUDIO_ANALOG, etc.).
		@param[in]	inEmbeddedInput		If the audio source is set to ::NTV2_AUDIO_EMBEDDED, and the device has multiple SDI inputs, use \c inEmbeddedInput
										to specify which ::NTV2EmbeddedAudioInput to use. This parameter is ignored if \c inAudioSource is not ::NTV2_AUDIO_EMBEDDED.
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@note		The \ref corvid88 has a firmware limitation where audio systems 5/6/7/8 cannot record or playback embedded audio on SDIs 1/2/3/4,
					nor can audio systems 1/2/3/4 record or playback embedded audio on SDIs 5/6/7/8.
		@see		CNTV2Card::GetAudioSystemInputSource, \ref audiocapture
	**/
	AJA_VIRTUAL bool		SetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, const NTV2AudioSource inAudioSource, const NTV2EmbeddedAudioInput inEmbeddedInput);

	/**
		@brief		Answers with the device's current ::NTV2AudioSource (and also possibly its ::NTV2EmbeddedAudioInput) for the given ::NTV2AudioSystem.
		@param[in]	inAudioSystem		Specifies the ::NTV2AudioSystem of interest.
		@param[out] outAudioSource		Receives the ::NTV2AudioSource that's currently being used for the given ::NTV2AudioSystem
										(e.g., ::NTV2_AUDIO_EMBEDDED, ::NTV2_AUDIO_AES, ::NTV2_AUDIO_ANALOG, etc.).
		@param[out] outEmbeddedSource	Receives the ::NTV2EmbeddedAudioInput.
										Ignore this result if the audio source is not ::NTV2_AUDIO_EMBEDDED.
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@see		CNTV2Card::SetAudioSystemInputSource, \ref audiocapture
	**/
	AJA_VIRTUAL bool		GetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, NTV2AudioSource & outAudioSource, NTV2EmbeddedAudioInput & outEmbeddedSource);

	/**
		@brief		Sets the embedded (SDI) audio source for the given ::NTV2AudioSystem on the device.
		@param[in]	inEmbeddedSource	If the audio source is set to ::NTV2_AUDIO_EMBEDDED, and the device has multiple SDI inputs, use \c inEmbeddedInput
										to specify which ::NTV2EmbeddedAudioInput to use. This parameter is ignored if \c inAudioSource is not ::NTV2_AUDIO_EMBEDDED.
		@param[in]	inAudioSystem		Specifies the ::NTV2AudioSystem of interest.
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@note		This function will have no effect if the device's current ::NTV2AudioSource is something other than ::NTV2_AUDIO_EMBEDDED.
					Usually it's best to call CNTV2Card::SetAudioSystemInputSource instead of this function.
		@note		The \ref corvid88 has a firmware limitation where audio systems 5/6/7/8 cannot record or playback embedded audio on SDIs 1/2/3/4,
					nor can audio systems 1/2/3/4 record or playback embedded audio on SDIs 5/6/7/8.
		@see		CNTV2Card::GetEmbeddedAudioInput, CNTV2Card::SetAudioSystemInputSource, \ref audiocapture
	**/
	AJA_VIRTUAL bool		SetEmbeddedAudioInput (const NTV2EmbeddedAudioInput inEmbeddedSource, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Answers with the device's current embedded (SDI) audio source for the given ::NTV2AudioSystem.
		@param[out] outEmbeddedSource	Receives the ::NTV2EmbeddedAudioInput (SDI audio source).
		@param[in]	inAudioSystem		Specifies the ::NTV2AudioSystem of interest.
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@note		This function assumes the device's current ::NTV2AudioSource is ::NTV2_AUDIO_EMBEDDED.
					Usually it's best to call CNTV2Card::GetAudioSystemInputSource instead of this function.
		@see		CNTV2Card::SetEmbeddedAudioInput, CNTV2Card::GetAudioSystemInputSource, \ref audiocapture
	**/
	AJA_VIRTUAL bool		GetEmbeddedAudioInput (NTV2EmbeddedAudioInput & outEmbeddedSource, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Sets the device's ::NTV2AudioSystem that will provide audio for the given SDI output's audio embedder.
					For 3G-capable SDI outputs, this affects Data Stream 1 (or Link A).
		@param[in]	inSDIOutputConnector	Specifies the SDI output connector of interest as an ::NTV2Channel (a zero-based index number).
		@param[in]	inAudioSystem			Specifies the Audio System to be used (e.g., ::NTV2_AUDIOSYSTEM_1).
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@note		Use the ::NTV2DeviceGetNumVideoOutputs function to determine the number of SDI output jacks the device has.
		@note		The \ref corvid88 has a firmware limitation where audio systems 5/6/7/8 cannot playback embedded audio on SDIs 1/2/3/4,
					nor can audio systems 1/2/3/4 playback embedded audio on SDIs 5/6/7/8.
		@see		CNTV2Card::GetSDIOutputAudioSystem, CNTV2Card::SetSDIOutputDS2AudioSystem, CNTV2Card::GetSDIOutputDS2AudioSystem, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetSDIOutputAudioSystem (const NTV2Channel inSDIOutputConnector, const NTV2AudioSystem inAudioSystem);

	/**
		@brief		Sets the device's ::NTV2AudioSystem that will provide audio for the given SDI outputs' audio embedders.
					For 3G-capable SDI outputs, this affects Data Stream 1 (or Link A).
		@param[in]	inSDIOutputs	Specifies the SDI output connectors of interest as an ::NTV2ChannelSet (a set of zero-based index numbers).
		@param[in]	inAudioSystem	Specifies the Audio System to be used (e.g., ::NTV2_AUDIOSYSTEM_1).
		@param[in]	inDS2			Optionally specifies if Data Stream 2 should be configured. Defaults to false (DS1).
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@note		Use the ::NTV2DeviceGetNumVideoOutputs function to determine the number of SDI output jacks the device has.
		@note		The \ref corvid88 has a firmware limitation where audio systems 5/6/7/8 cannot playback embedded audio on SDIs 1/2/3/4,
					nor can audio systems 1/2/3/4 playback embedded audio on SDIs 5/6/7/8.
		@see		CNTV2Card::GetSDIOutputAudioSystem, CNTV2Card::GetSDIOutputDS2AudioSystem, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetSDIOutputAudioSystem (const NTV2ChannelSet & inSDIOutputs, const NTV2AudioSystem inAudioSystem, const bool inDS2 = false);	//	New in SDK 16.2

	/**
		@brief		Answers with the device's ::NTV2AudioSystem that is currently providing audio for the given SDI output's audio embedder.
					(For 3G-capable SDI outputs, this is for Data Stream 1, or Link A.)
		@param[in]	inSDIOutputConnector	Specifies the SDI output connector of interest as an ::NTV2Channel (a zero-based index number).
		@param[out] outAudioSystem			Receives the Audio System that's currently being used (e.g., ::NTV2_AUDIOSYSTEM_1).
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@note		Use the ::NTV2DeviceGetNumVideoOutputs function to determine the number of SDI output jacks the device has.
		@see		CNTV2Card::SetSDIOutputAudioSystem, CNTV2Card::GetSDIOutputDS2AudioSystem, CNTV2Card::SetSDIOutputDS2AudioSystem, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetSDIOutputAudioSystem (const NTV2Channel inSDIOutputConnector, NTV2AudioSystem & outAudioSystem);

	/**
		@brief		Sets the Audio System that will supply audio for the given SDI output's audio embedder for Data Stream 2
					(Link B) for dual-link playout.
		@param[in]	inSDIOutputConnector	Specifies the SDI output connector of interest as an ::NTV2Channel (a zero-based index number).
		@param[in]	inAudioSystem			Specifies the Audio System that is to be used by the SDI output's embedder (e.g., ::NTV2_AUDIOSYSTEM_1).
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@note		The \ref corvid88 has a firmware limitation where audio systems 5/6/7/8 cannot playback embedded audio on SDIs 1/2/3/4,
					nor can audio systems 1/2/3/4 playback embedded audio on SDIs 5/6/7/8.
		@see		CNTV2Card::GetSDIOutputAudioSystem, CNTV2Card::SetSDIOutputAudioSystem, CNTV2Card::GetSDIOutputDS2AudioSystem, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetSDIOutputDS2AudioSystem (const NTV2Channel inSDIOutputConnector, const NTV2AudioSystem inAudioSystem);

	/**
		@brief		Answers with the device's Audio System that is currently providing audio for the given SDI output's audio
					embedder for Data Stream 2 (Link B) for dual-link output.
		@param[in]	inSDIOutputConnector	Specifies the SDI output connector of interest as an ::NTV2Channel (a zero-based index number).
		@param[out] outAudioSystem			Receives the Audio System that's currently being used (e.g., ::NTV2_AUDIOSYSTEM_1).
		@return		True if successful; otherwise false.
		@note		Use the ::NTV2DeviceGetNumAudioSystems function to determine how many independent Audio Systems are available on the device.
		@note		The \ref corvid88 has a firmware limitation where audio systems 5/6/7/8 cannot playback embedded audio on SDIs 1/2/3/4,
					nor can audio systems 1/2/3/4 playback embedded audio on SDIs 5/6/7/8.
		@see		CNTV2Card::SetSDIOutputAudioSystem, CNTV2Card::GetSDIOutputAudioSystem, CNTV2Card::SetSDIOutputDS2AudioSystem, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetSDIOutputDS2AudioSystem (const NTV2Channel inSDIOutputConnector, NTV2AudioSystem & outAudioSystem);

	/**
		@brief		For the given SDI input (specified as a channel number), answers if the specified audio channel pair is currently PCM-encoded or not.
		@param[in]	inSDIInputConnector Specifies the SDI input connector of interest as an ::NTV2Channel (a zero-based index number).
		@param[in]	inAudioChannelPair	Specifies the audio channel pair of interest.
		@param[out] outIsPCM			Receives true if the channel pair is currently PCM-encoded;	 otherwise false.
		@return		True if successful;	 otherwise false.
	**/
	virtual bool			InputAudioChannelPairHasPCM (const NTV2Channel inSDIInputConnector, const NTV2AudioChannelPair inAudioChannelPair, bool & outIsPCM);	//	New in SDK 12.3

	/**
		@brief		For the given SDI input (specified as a channel number), returns the set of audio channel pairs that are currently PCM-encoded.
		@param[in]	inSDIInputConnector		Specifies the SDI input connector of interest as an ::NTV2Channel (a zero-based index number).
		@param[out] outChannelPairs			Receives the channel pairs that are currently PCM-encoded.
		@return		True if successful;	 otherwise false.
		@note		The audio de-embedder firmware sets non-PCM-detect bits in registers independently of its channel-pair-detection registers.
					Non-PCM-detect bits representing missing channel pairs are always clear. Therefore, callers of this function may wish to also
					call CNTV2Card::GetDetectedAudioChannelPairs (or CNTV2Card::GetDetectedAESChannelPairs), and then use std::set_intersection to
					produce a more realistic set of PCM channel pairs.
	**/
	virtual bool			GetInputAudioChannelPairsWithPCM (const NTV2Channel inSDIInputConnector, NTV2AudioChannelPairs & outChannelPairs);	//	New in SDK 12.3

	/**
		@brief		For the given SDI input (specified as a channel number), returns the set of audio channel pairs that are currently not PCM-encoded.
		@param[in]	inSDIInputConnector		Specifies the SDI input connector of interest as an ::NTV2Channel (a zero-based index number).
		@param[out] outChannelPairs			Receives the channel pairs that are not currently PCM-encoded.
		@return		True if successful;	 otherwise false.
	**/
	virtual bool			GetInputAudioChannelPairsWithoutPCM (const NTV2Channel inSDIInputConnector, NTV2AudioChannelPairs & outChannelPairs);	//	New in SDK 12.3

	/**
		@brief		Answers as to whether or not the host OS audio services for the AJA device (e.g. CoreAudio on MacOS)
					are currently suspended or not.
		@param[out] outIsSuspended	Receives 'true' if the host OS audio service is currently suspended for the AJA
									device;	 otherwise, receives 'false'.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool		GetSuspendHostAudio (bool & outIsSuspended);	//	New in SDK 12.1

	/**
		@brief		Suspends or resumes host OS audio (e.g. CoreAudio on MacOS) for the AJA device.
		@param[in]	inSuspend	If true, suspends host OS audio for the AJA device;	 otherwise, resumes it.
		@return		True if successful; otherwise false.
		@note		This function is currently only implemented on MacOS, and is used to suspend or resume CoreAudio
					when an application uses AutoCirculate to capture or play audio, to keep the two Audio Systems
					from conflicting with each other.
	**/
	AJA_VIRTUAL bool		SetSuspendHostAudio (const bool inSuspend);	//	New in SDK 12.1

	/**
		@brief		Answers with the current audio source for a given quad of AES audio output channels.
					By default, at power-up, for AJA devices that support AES audio output, the content of AES audio output channels 1/2/3/4
					reflect what's being output from audio channels 1/2/3/4 from NTV2_AUDIOSYSTEM_1, likewise with audio channels 5/6/7/8, etc.
		@param[in]	inAESAudioChannels		Specifies the AES audio output channel quad of interest.
		@param[out] outSrcAudioSystem		Receives the NTV2AudioSystem that is currently driving the given AES audio output channel quad.
		@param[out] outSrcAudioChannels		Receives the audio channel quad from the Audio System that's sourcing the given AES audio output channel quad.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::SetAESOutputSource, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAESOutputSource (const NTV2Audio4ChannelSelect inAESAudioChannels, NTV2AudioSystem & outSrcAudioSystem, NTV2Audio4ChannelSelect & outSrcAudioChannels);	//	New in SDK 12.5

	/**
		@brief		Changes the audio source for the given quad of AES audio output channels.
					By default, at power-up, for AJA devices that support AES audio output, the content of AES audio output channels 1/2/3/4
					reflect what's being output from audio channels 1/2/3/4 from NTV2_AUDIOSYSTEM_1, likewise with audio channels 5/6/7/8, etc.
		@param[in]	inAESAudioChannels		Specifies the AES audio output channel quad of interest.
		@param[in]	inSrcAudioSystem		Specifies the NTV2AudioSystem that should drive the given AES audio output channel quad.
		@param[in]	inSrcAudioChannels		Specifies the audio channel quad from the given Audio System that should drive the given AES audio output channel quad.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::GetAESOutputSource, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAESOutputSource (const NTV2Audio4ChannelSelect inAESAudioChannels, const NTV2AudioSystem inSrcAudioSystem, const NTV2Audio4ChannelSelect inSrcAudioChannels);	//	New in SDK 12.5

	/**
		@brief		Sets the audio monitor output source to a specified audio system and channel pair. The audio output monitor
					is typically a pair of RCA jacks (white + red) and/or a headphone jack.
		@param[in]	inAudioSystem	Specifies the audio system to use.
		@param[in]	inChannelPair	Specifies the audio channel pair to use.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::GetAudioOutputMonitorSource, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioOutputMonitorSource (const NTV2AudioChannelPair inChannelPair, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Answers with the current audio monitor source. The audio output monitor
					is typically a pair of RCA jacks (white + red) and/or a headphone jack.
		@param[out] outAudioSystem		Receives the current audio system being used.
		@param[out] outChannelPair		Receives the current audio channel pair being used.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::SetAudioOutputMonitorSource, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioOutputMonitorSource (NTV2AudioChannelPair & outChannelPair, NTV2AudioSystem & outAudioSystem);

	/**
		@brief		Answers with the current state of the audio output embedder for the given SDI output connector (specified as a channel number).
					When the embedder is disabled, the device will not embed any SMPTE 299M (HD) or SMPTE 272M (SD) packets in the HANC in the SDI output stream.
		@param[in]	inSDIOutputConnector	Specifies the SDI output of interest.
		@param[out] outIsEnabled			Receives 'true' if the audio output embedder is enabled;  otherwise 'false' if disabled.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioOutputEmbedderState, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioOutputEmbedderState (const NTV2Channel inSDIOutputConnector, bool & outIsEnabled);	//	New in SDK 13.0

	/**
		@brief		Enables or disables the audio output embedder for the given SDI output connector (specified as a channel number).
					When the embedder is disabled, the device will not embed any SMPTE 299M (HD) or SMPTE 272M (SD) packets in the HANC in the SDI output stream.
		@param[in]	inSDIOutputConnector	Specifies the SDI output of interest.
		@param[in]	inEnable				Specify 'true' to enable the audio output embedder (normal operation).
											Specify 'false' to disable the embedder.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioOutputEmbedderState, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioOutputEmbedderState (const NTV2Channel inSDIOutputConnector, const bool & inEnable);	//	New in SDK 13.0

	/**
		@brief		Answers with the current state of the audio output erase mode for the given Audio System.
					If enabled, the Audio System automatically writes zeroes into the audio output buffer behind the output read head during playout.
		@param[in]	inAudioSystem			Specifies the audio system of interest.
		@param[out] outEraseModeEnabled		Receives 'true' if enabled;	 otherwise 'false' if disabled (normal operation).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioOutputEraseMode, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioOutputEraseMode (const NTV2AudioSystem inAudioSystem, bool & outEraseModeEnabled);	//	New in SDK 13.0

	/**
		@brief		Enables or disables output erase mode for the given Audio System, which, when enabled, automatically writes zeroes into the audio output buffer
					behind the output read head.
		@param[in]	inAudioSystem			Specifies the audio system of interest.
		@param[in]	inEraseModeEnabled		Specify 'true' to enable output erase mode;	 otherwise 'false' for normal operation.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioOutputEraseMode, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioOutputEraseMode (const NTV2AudioSystem inAudioSystem, const bool & inEraseModeEnabled);	//	New in SDK 13.0

	/**
		@brief		Answers with the current state of the AES Sync Mode bit for the given Audio System's output.
		@param[in]	inAudioSystem			Specifies the audio system of interest.
		@param[out] outAESSyncModeBitSet	Receives 'true' if the bit is set;	otherwise 'false' (normal operation).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioOutputAESSyncModeBit, \ref audioplayout
	**/
	AJA_VIRTUAL bool		GetAudioOutputAESSyncModeBit (const NTV2AudioSystem inAudioSystem, bool & outAESSyncModeBitSet);	//	New in SDK 15.2

	/**
		@brief		Sets or clears the AES Sync Mode bit for the given Audio System's output.
		@param[in]	inAudioSystem			Specifies the audio system of interest.
		@param[in]	inAESSyncModeBitSet		Specify 'true' to set the AES Sync Mode bit;  otherwise 'false' for normal operation.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioOutputAESSyncModeBit, \ref audioplayout
	**/
	AJA_VIRTUAL bool		SetAudioOutputAESSyncModeBit (const NTV2AudioSystem inAudioSystem, const bool & inAESSyncModeBitSet);	//	New in SDK 15.2

	/**
		@brief		Sets the specified bidirectional XLR audio connectors to collectively act as inputs or outputs.
		@param[in]	inChannelQuad	Specifies the XLR audio connectors of interest.
		@param[in]	inEnable		If true, specifies that the connectors are to be used as outputs.
									If false, specifies they're to be used as inputs.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAnalogAudioTransmitEnable
	**/
	AJA_VIRTUAL bool		SetAnalogAudioTransmitEnable (const NTV2Audio4ChannelSelect inChannelQuad, const bool inEnable);	//	New in SDK 16.1

	/**
		@brief		Answers whether or not the specified bidirectional XLR audio connectors are collectively acting as inputs or outputs.
		@param[in]	inChannelQuad	Specifies the XLR audio connectors of interest.
		@param[out] outEnabled		Receives true if the XLR connectors are currently transmitting (output),
									or false if they're receiving (input).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAnalogAudioTransmitEnable
	**/
	AJA_VIRTUAL bool		GetAnalogAudioTransmitEnable (const NTV2Audio4ChannelSelect inChannelQuad, bool & outEnabled);	//	New in SDK 16.1

	/**
		@brief		Answers with the current value of the 48kHz audio clock counter.
		@return		True if successful; otherwise false.
		@param[out] outValue		Receives the number of 48kHz "ticks" that have transpired since the
									device was powered up.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest. Currently ignored, but
									may be used if a future NTV2 device has more than one audio clock.
		@note		This counter will overflow and wrap back to zero in 24:51:00 [hh:mm:ss].
	**/
	AJA_VIRTUAL bool		GetRawAudioTimer (ULWord & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);	//	New in SDK 15.5
	
	/**
		@brief		Enables breakout board analog audio XLR inputs.
		@param[in]	inEnable		If true, specifies that the XLR connectors are enabled.
									If false, breakout board audio reverts to digital AES inputs.
		@return		True if successful;	 otherwise false.
		@see		kDeviceCanDoBreakoutBoard, kDeviceHasBreakoutBoard
	**/
	AJA_VIRTUAL bool		EnableBOBAnalogAudioIn (bool inEnable);	//	New in SDK 17.0
	
	/**
		@brief		Sets the multi-link audio mode for the given audio system.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest.
		@param[in]	inEnable		Specify true to enable multi-link audio mode;  otherwise specify false.
		@see		CNTV2Card::GetMultiLinkAudioMode, ::NTV2DeviceCanDoMultiLinkAudio, \ref audiomultilink
	**/
	AJA_VIRTUAL bool		SetMultiLinkAudioMode (const NTV2AudioSystem inAudioSystem, const bool inEnable);	//	New in SDK 16.2

	/**
		@brief		Answers with the current multi-link audio mode for the given audio system.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the ::NTV2AudioSystem of interest.
		@param[out]	outEnable		Receives true if multi-link audio mode is currently enabled;  otherwise false if disabled.
		@see		CNTV2Card::SetMultiLinkAudioMode, ::NTV2DeviceCanDoMultiLinkAudio, \ref audiomultilink
	**/
	AJA_VIRTUAL bool		GetMultiLinkAudioMode (const NTV2AudioSystem inAudioSystem, bool & outEnabled);	//	New in SDK 16.2

#if !defined(NTV2_DEPRECATE_16_0)
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool WriteAudioSource (const ULWord inValue, const NTV2Channel inChannel = NTV2_CHANNEL1));	///< @deprecated	This function is obsolete.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool ReadAudioSource (ULWord & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1));	///< @deprecated	This function is obsolete.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetAudioOutputReset (const NTV2AudioSystem inAudioSystem, const bool inIsReset))	{return inIsReset ? StopAudioOutput(inAudioSystem) : StartAudioOutput(inAudioSystem);}	///< @deprecated	Call CNTV2Card::StartAudioOutput or CNTV2Card::StopAudioOutput instead.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetAudioOutputReset (const NTV2AudioSystem inAudioSystem, bool & outIsReset)) {if(!IsAudioOutputRunning(inAudioSystem, outIsReset)) return false; outIsReset = !outIsReset; return true; }	///< @deprecated	Call CNTV2Card::IsAudioOutputRunning instead.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetAudioInputReset (const NTV2AudioSystem inAudioSystem, const bool inIsReset))	{return inIsReset ? StopAudioInput(inAudioSystem) : StartAudioInput(inAudioSystem);}	///< @deprecated	Call CNTV2Card::StartAudioInput or CNTV2Card::StopAudioInput instead.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetAudioInputReset (const NTV2AudioSystem inAudioSystem, bool & outIsReset))	{if(!IsAudioInputRunning(inAudioSystem, outIsReset)) return false; outIsReset = !outIsReset; return true; } ///< @deprecated	Call CNTV2Card::IsAudioInputRunning instead.
#endif	//	!defined(NTV2_DEPRECATE_16_0)
#if !defined(NTV2_DEPRECATE_16_1)
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetAnalogAudioIOConfiguration (const NTV2AnalogAudioIO inConfig));	///< @deprecated	Use CNTV2Card::SetAnalogAudioTransmitEnable instead.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetAnalogAudioIOConfiguration (NTV2AnalogAudioIO & outConfig));	///< @deprecated	Use CNTV2Card::GetAnalogAudioTransmitEnable instead.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadAudioLastOut (ULWord & outValue, const NTV2Channel inChannel))	{return ReadAudioLastOut(outValue, NTV2AudioSystem(inChannel));}	///< @deprecated	Use CNTV2Card::ReadAudioLastOut(ULWord &, const NTV2AudioSystem) instead.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadAudioLastIn (ULWord & outValue, const NTV2Channel inChannel)) {return ReadAudioLastIn(outValue, NTV2AudioSystem(inChannel));} ///< @deprecated	Use CNTV2Card::ReadAudioLastIn(ULWord &, const NTV2AudioSystem) instead.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool WriteAudioLastOut (ULWord & outValue, const NTV2Channel inChannel)) {(void)outValue;(void)inChannel; return false;}	///< @deprecated	This function is obsolete.
#endif	//	!defined(NTV2_DEPRECATE_16_1)
#if !defined(NTV2_DEPRECATE_16_3)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool WriteAudioLastOut (const ULWord inValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1)) {(void)inValue;(void)inAudioSystem; return false;}	///< @deprecated	This function is obsolete.
#endif	//	!defined(NTV2_DEPRECATE_16_1)
#if !defined(NTV2_DEPRECATE_17_0)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool CanDoAudioWaitForVBI(void)) {return IsSupported(kDeviceAudioCanWaitForVBI);}	///< @deprecated	Use CNTV2DriverInterface::IsSupported instead. (Was new in SDK 16.0)
#endif	//	!defined(NTV2_DEPRECATE_17_0)
	///@}

	/**
		@name	Audio Mixer
	**/
	///@{

	/**
		@brief		Answers with the Audio System that's currently driving the given input of the Audio Mixer.
		@param[in]	inMixerInput	Specifies the Audio Mixer's input of interest.
		@param[out] outAudioSystem	Receives the ::NTV2AudioSystem that's currently driving the Audio Mixer's input.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioMixerInputAudioSystem, \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerInputAudioSystem (const NTV2AudioMixerInput inMixerInput, NTV2AudioSystem & outAudioSystem);	//	New in SDK 15.5

	/**
		@brief		Sets the Audio System that will drive the given input of the Audio Mixer.
		@param[in]	inMixerInput	Specifies the Audio Mixer's input of interest.
		@param[in]	inAudioSystem	Specifies the new ::NTV2AudioSystem that is to drive the Audio Mixer's input.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioMixerInputAudioSystem, \ref audiomixer
	**/
	AJA_VIRTUAL bool		SetAudioMixerInputAudioSystem (const NTV2AudioMixerInput inMixerInput, const NTV2AudioSystem inAudioSystem);	//	New in SDK 15.5

	/**
		@brief		Answers with the Audio Channel Pair that's currently driving the given input of the Audio Mixer.
		@param[in]	inMixerInput	Specifies the Audio Mixer's input of interest.
		@param[out] outChannelPair	Receives the ::NTV2AudioChannelPair that's currently driving the audio mixer's input.
		@return		True if successful;	 otherwise false.
		@note		Audio mixer inputs ::NTV2_AudioMixerInputAux1 and ::NTV2_AudioMixerInputAux2 currently return ::NTV2_AudioChannel1_2.
		@see		CNTV2Card::SetAudioMixerInputChannelSelect, \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerInputChannelSelect (const NTV2AudioMixerInput inMixerInput, NTV2AudioChannelPair & outChannelPair);	//	New in SDK 15.5

	/**
		@brief		Specifies the Audio Channel Pair that will drive the given input of the Audio Mixer.
		@param[in]	inMixerInput	Specifies the Audio Mixer's input of interest.
		@param[in]	inChannelPair	Specifies the new ::NTV2AudioChannelPair that is to drive the audio mixer's input.
		@return		True if successful;	 otherwise false.
		@note		Audio mixer inputs ::NTV2_AudioMixerInputAux1 and ::NTV2_AudioMixerInputAux2 are currently fixed to ::NTV2_AudioChannel1_2 and cannot be changed.
		@see		CNTV2Card::SetAudioMixerInputChannelSelect, \ref audiomixer
	**/
	AJA_VIRTUAL bool		SetAudioMixerInputChannelSelect (const NTV2AudioMixerInput inMixerInput, const NTV2AudioChannelPair inChannelPair); //	New in SDK 15.5

	/**
		@brief		Answers with the current gain setting for the Audio Mixer's given input.
		@param[in]	inMixerInput	Specifies the Audio Mixer's input of interest.
		@param[in]	inChannel		Specifies the audio channel of interest.
		@param[out] outGainValue	Receives the current main input gain level.
									This is a signed 18-bit value, where unity gain is 0x10000.
		@return		True if successful;	 otherwise false.
		@note		Currently, the Audio Mixer's Main input gain control affects both audio channels 1 & 2 (L & R),
					while the Aux 1 & 2 inputs have separate gain settings for audio channels 1 & 2 (L & R).
		@see		CNTV2Card::GetAudioMixerInputGain, \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerInputGain (const NTV2AudioMixerInput inMixerInput, const NTV2AudioMixerChannel inChannel, ULWord & outGainValue);	//	New in SDK 15.5

	/**
		@brief		Sets the gain for the given input of the Audio Mixer.
		@param[in]	inMixerInput	Specifies the Audio Mixer's input of interest.
		@param[in]	inChannel		Specifies the audio channel of interest.
		@param[in]	inGainValue		Specifies the new input gain level.
									This is a signed 18-bit value, where unity gain is 0x10000.
		@return		True if successful;	 otherwise false.
		@note		Currently, the Audio Mixer's Main input gain control affects both audio channels 1 & 2 (L & R),
					while the Aux 1 & 2 inputs have separate gain settings for audio channels 1 & 2 (L & R).
		@see		CNTV2Card::SetAudioMixerInputGain, \ref audiomixer
	**/
	AJA_VIRTUAL bool		SetAudioMixerInputGain (const NTV2AudioMixerInput inMixerInput, const NTV2AudioMixerChannel inChannel, const ULWord inGainValue);	//	New in SDK 15.5
	
	/**
		@brief		Answers with the current gain setting for the Audio Mixer's output.
		@param[out] outGainValue	Receives the current main input gain level.
									This is a signed 18-bit value, where unity gain is 0x10000.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioMixerOutputGain, \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerOutputGain (ULWord & outGainValue);	//	New in SDK 15.5

	/**
		@brief		Sets the gain for the output of the Audio Mixer.
		@param[in]	inGainValue		Specifies the new input gain level.
									This is a signed 18-bit value, where unity gain is 0x10000.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioMixerOutputGain, \ref audiomixer
	**/
	AJA_VIRTUAL bool		SetAudioMixerOutputGain (const ULWord inGainValue); //	New in SDK 15.5
	
	/**
		@brief		Answers with the Audio Mixer's current audio output levels.
		@param[in]	inChannelPairs		Specifies the audio channel pair(s) of interest.
										Use an empty list to retrieve all available audio channels.
		@param[out] outLevels			A std::vector of ULWord values, one per audio channel, in ascending audio
										channel order (per the ::NTV2AudioChannelPairs that were specified).
		@return		True if successful;	 otherwise false.
		@see		See \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerOutputLevels (const NTV2AudioChannelPairs & inChannelPairs, std::vector<uint32_t> & outLevels);	//	New in SDK 15.5
	
	/**
		@brief		Answers with the current gain setting for the headphone out.
		@param[out] outGainValue	Receives the current headphone gain level.
									This is a signed 18-bit value, where unity gain is 0x10000.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetHeadphoneOutputGain, \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetHeadphoneOutputGain (ULWord & outGainValue);

	/**
		@brief		Sets the gain for the headphone out.
		@param[in]	inGainValue		Specifies the new headphone gain level.
									This is a signed 18-bit value, where unity gain is 0x10000.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetHeadphoneOutputGain, \ref audiomixer
	**/
	AJA_VIRTUAL bool		SetHeadphoneOutputGain (const ULWord inGainValue);

	/**
		@brief		Answers with a std::bitset that indicates which input audio channels of the given Audio Mixer input are currently muted.
		@param[in]	inMixerInput	Specifies the Audio Mixer's input of interest.
		@param[out] outMutes		Receives the bitset. Call its "test" method, passing it a valid ::NTV2AudioMixerChannel
									to determine if that channel is muted (true) or not (false).
									Note that only audio channels ::NTV2_AudioMixerChannel1 and ::NTV2_AudioMixerChannel2 are relevant.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioMixerInputChannelsMute, \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerInputChannelsMute (const NTV2AudioMixerInput inMixerInput, NTV2AudioChannelsMuted16 & outMutes);	//	New in SDK 15.5

	/**
		@brief		Mutes (or enables) the given output audio channel of the Audio Mixer.
		@param[in]	inMixerInput	Specifies the Audio Mixer's input of interest.
		@param[in]	inMutes			Specifies the mute state for each audio channel as a std::bitset.
									The index in the bitset directly correlates with the ::NTV2AudioMixerChannel.
									Set the bit to mute the channel;  clear/reset the bit to unmute/enable the channel.
									Note that only audio channels ::NTV2_AudioMixerChannel1 and ::NTV2_AudioMixerChannel2 are relevant.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioMixerInputChannelsMute, \ref audiomixer
	**/
	AJA_VIRTUAL bool		SetAudioMixerInputChannelsMute (const NTV2AudioMixerInput inMixerInput, const NTV2AudioChannelsMuted16 inMutes);	//	New in SDK 15.5

	/**
		@brief		Answers with the Audio Mixer's current audio input levels.
		@param[in]	inMixerInput		Specifies the Audio Mixer's input of interest.
		@param[in]	inChannelPairs		Specifies the audio channel pair(s) of interest.
										Use an empty list to retrieve all available audio channels.
		@param[out] outLevels			A std::vector of ULWord values, one per audio channel, in ascending audio
										channel order (per the ::NTV2AudioChannelPairs that were specified).
		@return		True if successful;	 otherwise false.
		@see		See \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerInputLevels (const NTV2AudioMixerInput inMixerInput, const NTV2AudioChannelPairs & inChannelPairs, std::vector<uint32_t> & outLevels); //	New in SDK 15.5

	/**
		@brief		Answers with the Audio Mixer's current sample count used for measuring audio levels.
		@param[out] outSampleCount		Receives the current sample count.
		@return		True if successful;	 otherwise false.
		@see		See \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerLevelsSampleCount (ULWord & outSampleCount);	//	New in SDK 15.5

	/**
		@brief		Sets the Audio Mixer's sample count it uses for measuring audio levels.
		@param[in]	inSampleCount	Specifies the new sample count. Must be a power of two
									(e.g. 1, 2, 4, 8 ...) up to 0x8000 maximum.
		@return		True if successful;	 otherwise false.
		@see		See \ref audiomixer
	**/
	AJA_VIRTUAL bool		SetAudioMixerLevelsSampleCount (const ULWord inSampleCount);	//	New in SDK 15.5

	/**
		@brief		Answers with a std::bitset that indicates which output audio channels of the Audio Mixer are currently muted.
		@param[out] outMutes	Receives the bitset. Call its "test" method, passing it a valid ::NTV2AudioMixerChannel
								to determine if that channel is muted (true) or not (false).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetAudioMixerOutputChannelsMute, \ref audiomixer
	**/
	AJA_VIRTUAL bool		GetAudioMixerOutputChannelsMute (NTV2AudioChannelsMuted16 & outMutes);	//	New in SDK 15.5

	/**
		@brief		Mutes or enables the individual output audio channels of the Audio Mixer.
		@param[in]	inMutes		Specifies the mute state for each audio channel as a std::bitset.
								The index in the bitset directly correlates with the ::NTV2AudioMixerChannel.
								Set the bit to mute the channel;  clear/reset the bit to unmute/enable the channel.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetAudioMixerOutputChannelsMute, \ref audiomixer
	**/
	AJA_VIRTUAL bool		SetAudioMixerOutputChannelsMute (const NTV2AudioChannelsMuted16 inMutes);	//	New in SDK 15.5
	///@}

	#if !defined(NTV2_DEPRECATE_16_3)
		AJA_VIRTUAL NTV2_DEPRECATED_f(bool WriteGlobalControl(const ULWord inVal))		{return WriteRegister(kRegGlobalControl, inVal);}	///< @deprecated	This function is obsolete. Do not use it.
		AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadGlobalControl(ULWord * pOutVal))	{return pOutVal ? ReadRegister(kRegGlobalControl, *pOutVal) : false;}	///< @deprecated	This function is obsolete. Do not use it.
	#endif	//	!defined(NTV2_DEPRECATE_16_3)


	/**
		@name	Programming
	**/
	///@{
	AJA_VIRTUAL bool	ReadFlashProgramControl(ULWord & outValue);
	AJA_VIRTUAL bool	IsXilinxProgrammed();
	AJA_VIRTUAL bool	ProgramMainFlash(const std::string & inFileName, const bool bInForceUpdate = false, const bool bInQuiet = false);	//	inFileName became const std::string& in SDK 16.2
	AJA_VIRTUAL bool	GetProgramStatus(SSC_GET_FIRMWARE_PROGRESS_STRUCT *statusStruct);

	/**
		@brief		Reports the revision number of the currently-running firmware package.
					KonaIP style boards have a package.
		@param[out] outRevision		Receives the revision number.
		@return		True if successful;	 otherwise false.
		@note		This may differ from the revision number of the installed firmware package if, after
					erasing or reflashing, the device was not power-cycled to force its FPGA to reload.
	**/
	AJA_VIRTUAL bool	GetRunningFirmwarePackageRevision (ULWord & outRevision);


	/**
		@brief		Reports the revision number of the currently-running firmware.
		@param[out] outRevision		Receives the revision number.
		@return		True if successful;	 otherwise false.
		@note		This may differ from the revision number of the installed firmware if, after
					erasing or reflashing, the device was not power-cycled to force its FPGA to reload.
		@see		CNTV2Card::GetRunningFirmwareDate, CNTV2Card::GetRunningFirmwareTime, \ref devicefirmware.
	**/
	AJA_VIRTUAL bool	GetRunningFirmwareRevision (UWord & outRevision);

	/**
		@brief		Reports the (local Pacific) build date of the currently-running firmware.
		@param[out] outYear		Receives the year portion of the build date, an unsigned integer
								representing a standard Gregorian calendar year (e.g., 2017).
		@param[out] outMonth	Receives the month portion of the build date, an unsigned integer
								representing a standard 1-based Gregorian calendar month (e.g., 1 == January).
		@param[out] outDay		Receives the day portion of the build date, an unsigned integer
								representing a standard 1-based Gregorian calendar day (i.e., 1 thru 31).
		@return		True if successful;	 otherwise false.
		@note		This date may differ from the build date of the installed firmware if, after erasing
					or reflashing, the device was never power-cycled to force its FPGA to reload.
		@see		CNTV2Card::GetRunningFirmwareTime, CNTV2Card::GetRunningFirmwareRevision, \ref devicefirmware.
	**/
	AJA_VIRTUAL bool	GetRunningFirmwareDate (UWord & outYear, UWord & outMonth, UWord & outDay);

	/**
		@brief		Reports the (local Pacific) build time of the currently-running firmware.
		@param[out] outHours	Receives the hours portion of the build time, an unsigned integer
								representing the number of hours past the start of day (0 thru 23).
		@param[out] outMinutes	Receives the minutes portion of the build time, an unsigned integer
								representing the number of minutes past the hour (0 thru 59).
		@param[out] outSeconds	Receives the seconds portion of the build time, an unsigned integer
								representing the number of seconds past the minute (0 thru 59).
		@return		True if successful;	 otherwise false.
		@note		This date may differ from the build date of the installed firmware if, after erasing
					or reflashing, the device was never power-cycled to force its FPGA to reload.
		@see		CNTV2Card::GetRunningFirmwareDate, CNTV2Card::GetRunningFirmwareRevision, \ref devicefirmware.
	**/
	AJA_VIRTUAL bool	GetRunningFirmwareTime (UWord & outHours, UWord & outMinutes, UWord & outSeconds);

	/**
		@brief		Reports the (local Pacific) build date and time of the currently-running firmware.
		@param[out] outDate		Receives a string containing the human-readable running firmware build date,
								in the form 'YYYY/MM/DD', where YYYY, MM and DD are the numeric Gregorian year,
								month and day values, expressed as unsigned decimal values (with leading zeroes).
		@param[out] outTime		Receives a string containing the human-readable running firmware build time,
								in the form 'HH:MM:SS', where HH, MM and SS are the numeric hour, minute and second
								values, expressed as unsigned decimal values (with leading zeroes), and a 24-hour
								clock format.
		@return		True if successful;	 otherwise false.
		@note		This date/time may differ from the build date/time of the installed firmware if, after erasing
					or reflashing, the device was never power-cycled to force its FPGA to reload.
		@see		CNTV2Card::GetRunningFirmwareTime, CNTV2Card::GetRunningFirmwareRevision, \ref devicefirmware.
	**/
	AJA_VIRTUAL bool	GetRunningFirmwareDate (std::string & outDate, std::string & outTime);

	/**
		@brief		Reports the UserID number of the currently-running firmware.
		@param[out] outUserID		Receives the UserID.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetRunningFirmwareDate, CNTV2Card::GetRunningFirmwareTime, \ref devicefirmware.
	**/
	AJA_VIRTUAL bool	GetRunningFirmwareUserID (ULWord & outUserID);	//	 New in SDK 16.1
	///@}

#if !defined(NTV2_DEPRECATE_16_0)
	//	OEM Mapping to Userspace Functions
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetBaseAddress(NTV2Channel channel, ULWord **pBaseAddress));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetBaseAddress(ULWord **pBaseAddress));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetRegisterBaseAddress(ULWord regNumber, ULWord ** pRegAddress));	///< @deprecated	Obsolete starting in SDK 16.0.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetXena2FlashBaseAddress(ULWord ** pXena2FlashAddress));	///< @deprecated	Obsolete starting in SDK 16.0.
#endif	//	!defined(NTV2_DEPRECATE_16_0)

#if !defined(NTV2_DEPRECATE_17_0)
	//	Read-Only Status Registers
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadStatusRegister (ULWord *pVal))			{return pVal ? ReadRegister(kRegStatus, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadStatus2Register (ULWord *pVal))			{return pVal ? ReadRegister(kRegStatus2, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadInputStatusRegister (ULWord *pVal))		{return pVal ? ReadRegister(kRegInputStatus, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadInputStatus2Register (ULWord *pVal))		{return pVal ? ReadRegister(kRegInputStatus2, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadInput56StatusRegister (ULWord *pVal))		{return pVal ? ReadRegister(kRegInput56Status, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool ReadInput78StatusRegister (ULWord *pVal))		{return pVal ? ReadRegister(kRegInput78Status, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool Read3GInputStatusRegister(ULWord *pVal))		{return pVal ? ReadRegister(kRegSDIInput3GStatus, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool Read3GInputStatus2Register(ULWord *pVal))		{return pVal ? ReadRegister(kRegSDIInput3GStatus2, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool Read3GInput5678StatusRegister(ULWord *pVal))	{return pVal ? ReadRegister(kRegSDI5678Input3GStatus, *pVal) : false;}	///< @deprecated	Obsolete starting in SDK 17.0.
#endif	//	!defined(NTV2_DEPRECATE_17_0)

	AJA_VIRTUAL bool	SupportsP2PTransfer (void); ///< @return	True if this device can directly transmit data to another PCIe device via DMA;	otherwise false.
	AJA_VIRTUAL bool	SupportsP2PTarget (void);	///< @return	True if this device can directly receive data from another PCIe device via DMA;	 otherwise false.


	/**
		@name	On-Device LEDs
	**/
	///@{
	/**
		@brief	The four on-board LEDs can be set by writing 0-15
		@param[in]	inValue		Sets the state of the four on-board LEDs using the least significant
								four bits of the given ULWord value.
		@return True if successful;	 otherwise, false.
	**/
	AJA_VIRTUAL bool	SetLEDState (ULWord inValue);

	/**
		@brief	Answers with the current state of the four on-board LEDs.
		@param[out] outValue	Receives the current state of the four on-board LEDs.
								Only the least significant four bits of the ULWord have any meaning.
		@return True if successful;	 otherwise, false.
	**/
	AJA_VIRTUAL bool			GetLEDState (ULWord & outValue);
	///@}


	/**
		@name	RP-188
	**/
	///@{
	/**
		@brief		Sets the current RP188 mode -- ::NTV2_RP188_INPUT or ::NTV2_RP188_OUTPUT -- for the given channel.
		@param[in]	inChannel		Specifies the channel of interest.
		@param[in]	inMode			Specifies the new RP-188 mode for the given channel.
									Must be one of ::NTV2_RP188_INPUT or ::NTV2_RP188_OUTPUT. All other values are illegal.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetRP188Mode, \ref anctimecode
	**/
	AJA_VIRTUAL bool	SetRP188Mode			(const NTV2Channel inChannel,	const NTV2_RP188Mode inMode);

	/**
		@brief		Returns the current RP188 mode -- ::NTV2_RP188_INPUT or ::NTV2_RP188_OUTPUT -- for the given channel.
		@param[in]	inChannel		Specifies the channel of interest.
		@param[out] outMode			Receives the RP-188 mode for the given channel.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetRP188Mode, \ref anctimecode
	**/
	AJA_VIRTUAL bool	GetRP188Mode			(const NTV2Channel inChannel,	NTV2_RP188Mode & outMode);

	/**
		@brief		Writes the raw RP188 data into the DBB/Low/Hi registers for the given SDI output.
					These values are latched and sent at the next VBI.
		@param[in]	inSDIOutput		Specifies the SDI output of interest as an ::NTV2Channel value.
		@param[in]	inRP188Data		Specifies the raw RP188 data values to be written.
		@note		This call will have no effect if the SDI output is in "bypass mode".
		@see		CNTV2Card::GetRP188Data, CNTV2Card::IsRP188BypassEnabled, CNTV2Card::DisableRP188Bypass, \ref anctimecode
	**/
	AJA_VIRTUAL bool	SetRP188Data			(const NTV2Channel inSDIOutput, const NTV2_RP188 & inRP188Data);

	/**
		@brief		Reads the raw RP188 data from the DBB/Low/Hi registers for the given SDI input.
					On newer devices with bi-directional SDI connectors -- see \ref anctimecode for details --
					if the device is configured for...
					-	<b>input</b>: answers with the last timecode received at the SDI input (subject to
						the SDI input's RP188 source filter -- see CNTV2Card::GetRP188SourceFilter);
					-	<b>output</b>: answers with the timecode that's to be embedded into the SDI output
						(usually the last timecode written via CNTV2Card::SetRP188Data).
		@param[in]	inSDIInput		Specifies the SDI input of interest, expressed as an ::NTV2Channel.
									For bi-directional SDI devices, specifies the SDI connector of interest,
									which can specify an SDI output.
		@param[out] outRP188Data	Receives the raw RP188 data values.
		@see		CNTV2Card::SetRP188Data, CNTV2Card::GetRP188SourceFilter, \ref anctimecode
	**/
	AJA_VIRTUAL bool	GetRP188Data			(const NTV2Channel inSDIInput,	NTV2_RP188 & outRP188Data);

	/**
		@brief		Sets the RP188 DBB filter for the given SDI input.
		@param[in]	inSDIInput		Specifies the SDI input of interest, expressed as an NTV2Channel.
		@param[in]	inFilterValue	Specifies the new filter value to use. Only the lower 8 bits are used.
									Use 0x00 for LTC;  0x01 for VITC1;	0x02 for VITC2; 0xFF for unfiltered.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetRP188SourceFilter, CNTV2Card::SetRP188BypassSource, \ref anctimecode
	**/
	AJA_VIRTUAL bool	SetRP188SourceFilter	(const NTV2Channel inSDIInput,	const UWord inFilterValue);

	/**
		@brief		Returns the current RP188 filter setting for the given SDI input.
		@param[in]	inSDIInput		Specifies the SDI input of interest, expressed as an NTV2Channel.
		@param[out] outFilterValue	Receives the given SDI input's current RP188 SDI input filter, an 8-bit value.
									0x00 is LTC;  0x01 is VITC1;  0x02 is VITC2;  0xFF is unfiltered, which results
									in the timecode registers containing the last received timecode packet for the
									input frame.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetRP188SourceFilter, CNTV2Card::IsRP188BypassEnabled, CNTV2Card::GetRP188BypassSource, \ref anctimecode
	**/
	AJA_VIRTUAL bool	GetRP188SourceFilter	(const NTV2Channel inSDIInput,	UWord & outFilterValue);

	/**
		@brief		Answers if the SDI output's RP-188 bypass mode is enabled or not.
		@param[in]	inSDIOutput			Specifies the SDI output of interest, expressed as an NTV2Channel.
		@param[out] outIsBypassEnabled	Receives true if the SDI output's RP188 timecode is currently sourced from its
										RP188 registers (see CNTV2Card::SetRP188Data).
										Receives false if its output timecode is currently sourced from an SDI input
										(see CNTV2Card::GetRP188BypassSource and CNTV2Card::GetRP188SourceFilter).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::EnableRP188Bypass, CNTV2Card::DisableRP188Bypass, CNTV2Card::GetRP188BypassSource, CNTV2Card::GetRP188SourceFilter
	**/
	AJA_VIRTUAL bool	IsRP188BypassEnabled	(const NTV2Channel inSDIOutput, bool & outIsBypassEnabled);

	/**
		@brief		Configures the SDI output's embedder to embed SMPTE 12M timecode specified in calls to CNTV2Card::SetRP188Data.
		@param[in]	inSDIOutput			Specifies the SDI output of interest, expressed as an NTV2Channel.
		@return		True if successful; otherwise false.
		@note		The SDI output's ::NTV2_RP188Mode must be set to ::NTV2_RP188_OUTPUT.
		@see		CNTV2Card::EnableRP188Bypass, CNTV2Card::SetRP188Data, CNTV2Card::SetRP188Mode, \ref anctimecode
	**/
	AJA_VIRTUAL bool	DisableRP188Bypass		(const NTV2Channel inSDIOutput);

	/**
		@brief		Configures the SDI output's embedder to embed SMPTE 12M timecode obtained from an SDI input,
					which is often useful in E-E mode.
		@param[in]	inSDIOutput			Specifies the SDI output of interest, expressed as an NTV2Channel.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::DisableRP188Bypass, CNTV2Card::SetRP188SourceFilter, CNTV2Card::SetRP188BypassSource, \ref anctimecode
	**/
	AJA_VIRTUAL bool	EnableRP188Bypass		(const NTV2Channel inSDIOutput);

	/**
		@brief		For the given SDI output that's in RP188 bypass mode (E-E), specifies the SDI input
					to be used as a timecode source.
		@param[in]	inSDIOutput		Specifies the SDI output of interest, expressed as an NTV2Channel.
		@param[in]	inSDIInput		Specifies the SDI input to be used as a timecode source,
									expressed as a zero-based index number.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	SetRP188BypassSource (const NTV2Channel inSDIOutput, const UWord inSDIInput);

	/**
		@brief		For the given SDI output that's in RP188 bypass mode (E-E), answers with the SDI input
					that's currently being used as a timecode source.
		@param[in]	inSDIOutput		Specifies the SDI output of interest, expressed as an NTV2Channel.
		@param[out] outSDIInput		Receives the SDI input being used as a timecode source, expressed
									as a zero-based index number.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetRP188BypassSource (const NTV2Channel inSDIOutput, UWord & outSDIInput);
	///@}


	/**
		@name	Interrupts & Events
	**/
	///@{
	//
	//	Enable Interrupt/Event
	//
	AJA_VIRTUAL bool	EnableInterrupt (const INTERRUPT_ENUMS inEventCode);							//	GENERIC!

	/**
		@brief		Allows the CNTV2Card instance to wait for and respond to output vertical blanking interrupts
					originating from the given frameStore on the receiver's AJA device.
		@param[in]	channel		Specifies the frameStore of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	EnableOutputInterrupt (const NTV2Channel channel = NTV2_CHANNEL1);

	/**
		@brief		Allows the CNTV2Card instance to wait for and respond to input vertical blanking interrupts
					originating from the given input channel on the receiver's AJA device.
		@param[in]	channel		Specifies the input channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	EnableInputInterrupt (const NTV2Channel channel = NTV2_CHANNEL1);

	/**
		@brief		Allows the CNTV2Card instance to wait for and respond to input vertical blanking interrupts
					originating from the given FrameStore(s).
		@param[in]	inFrameStores	The input frameStore(s) of interest.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	EnableInputInterrupt (const NTV2ChannelSet & inFrameStores);


	//
	//	Disable Interrupt/Event
	//
	AJA_VIRTUAL bool	DisableInterrupt (const INTERRUPT_ENUMS inEventCode);						//	GENERIC!

	/**
		@brief		Prevents the CNTV2Card instance from waiting for and responding to vertical blanking
					interrupts originating from the given output channel on the device.
		@param[in]	channel		Specifies the output channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	DisableOutputInterrupt (const NTV2Channel channel = NTV2_CHANNEL1);

	/**
		@brief		Prevents the CNTV2Card instance from waiting for and responding to vertical blanking
					interrupts originating from the given input channel on the device.
		@param[in]	channel		Specifies the input channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	DisableInputInterrupt (const NTV2Channel channel = NTV2_CHANNEL1);

	/**
		@brief		Prevents the CNTV2Card instance from waiting for and responding to input vertical blanking
					interrupts originating from the given frameStore(s) on the device.
		@param[in]	inFrameStores		Specifies the frameStore(s) of interest.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	DisableInputInterrupt (const NTV2ChannelSet & inFrameStores);

	AJA_VIRTUAL bool	GetCurrentInterruptMasks (NTV2InterruptMask & outIntMask1, NTV2Interrupt2Mask & outIntMask2);


	//
	//	Subscribe to events
	//
	/**
		@brief		Causes me to be notified when the given event/interrupt is triggered for the AJA device.
		@param[in]	inEventCode		Specifies the INTERRUPT_ENUMS of interest.
		@return		True if successful; otherwise false, which can indicate communication with the device has been lost,
					or on the Windows platform, there are no more event subscription handles available.
		@see		CNTV2Card::UnsubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	SubscribeEvent (const INTERRUPT_ENUMS inEventCode);						//	GENERIC!

	/**
		@brief		Causes me to be notified when an output vertical blanking interrupt is generated for the given output channel.
		@param[in]	inChannel	Specifies the output channel of interest.
		@return		True if successful; otherwise false, which can indicate communication with the device has been lost,
					or on the Windows platform, there are no more event subscription handles available.
		@note		<b>Windows Users:</b> AJA recommends calling this function on the same thread that will call
					CNTV2Card::WaitForOutputVerticalInterrupt or CNTV2Card::WaitForOutputFieldID.
		@see		CNTV2Card::UnsubscribeOutputVerticalEvent, CNTV2Card::SubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	SubscribeOutputVerticalEvent (const NTV2Channel inChannel);

	/**
		@brief		Causes me to be notified when an output vertical blanking interrupt is generated for the given output channel(s).
		@param[in]	inChannels	Specifies the output channel(s) of interest.
		@return		True if successful; otherwise false, which can indicate communication with the device has been lost,
					or on the Windows platform, there are no more event subscription handles available.
		@note		<b>Windows Users:</b> AJA recommends calling this function on the same thread that will call
					CNTV2Card::WaitForOutputVerticalInterrupt or CNTV2Card::WaitForOutputFieldID.
		@see		CNTV2Card::UnsubscribeOutputVerticalEvents, CNTV2Card::SubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	SubscribeOutputVerticalEvent (const NTV2ChannelSet & inChannels);


	/**
		@brief		Causes me to be notified when an input vertical blanking interrupt occurs on the given input channel.
		@param[in]	inChannel	Specifies the input channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false, which can indicate communication with the device has been lost,
					or on the Windows platform, there are no more event subscription handles available.
		@note		<b>Windows Users:</b> AJA recommends calling this function on the same thread that will call
					CNTV2Card::WaitForInputVerticalInterrupt or CNTV2Card::WaitForInputFieldID.
		@see		CNTV2Card::UnsubscribeInputVerticalEvent, CNTV2Card::SubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	SubscribeInputVerticalEvent (const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Causes me to be notified when an input vertical blanking interrupt occurs on the given input channel(s).
		@param[in]	inChannels	Specifies the input channel(s) of interest.
		@return		True if successful; otherwise false, which can indicate communication with the device has been lost,
					or on the Windows platform, there are no more event subscription handles available.
		@note		<b>Windows Users:</b> AJA recommends calling this function on the same thread that will call
					CNTV2Card::WaitForInputVerticalInterrupt or CNTV2Card::WaitForInputFieldID.
		@see		CNTV2Card::UnsubscribeInputVerticalEvent, CNTV2Card::SubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	SubscribeInputVerticalEvent (const NTV2ChannelSet & inChannels);


	//
	//	Unsubscribe from events
	//
	/**
		@brief		Unregisters me so I'm no longer notified when the given event/interrupt is triggered on the AJA device.
		@param[in]	inEventCode		Specifies the INTERRUPT_ENUMS of interest.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::SubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	UnsubscribeEvent (const INTERRUPT_ENUMS inEventCode);						//	GENERIC!

	/**
		@brief		Unregisters me so I'm no longer notified when an output VBI is signaled on the given output channel.
		@param[in]	inChannel	Specifies the output channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@details	This function undoes the effect of a prior call to SubscribeOutputVerticalEvent.
		@see		CNTV2Card::SubscribeOutputVerticalEvent, CNTV2Card::UnsubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	UnsubscribeOutputVerticalEvent (const NTV2Channel inChannel);

	/**
		@brief		Unregisters me so I'm no longer notified when an output VBI is signaled on the given output channel(s).
		@param[in]	inChannels	Specifies the output channel(s) of interest.
		@return		True if successful; otherwise false.
		@details	This function undoes the effect of a prior call to SubscribeOutputVerticalEvents.
		@see		CNTV2Card::SubscribeOutputVerticalEvents, CNTV2Card::UnsubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	UnsubscribeOutputVerticalEvent (const NTV2ChannelSet & inChannels);

	/**
		@brief		Unregisters me so I'm no longer notified when an input VBI is signaled on the given input channel.
		@param[in]	inChannel	Specifies the input channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@details	This function undoes the effects of a prior call to SubscribeInputVerticalEvent.
		@see		CNTV2Card::SubscribeInputVerticalEvent, CNTV2Card::UnsubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	UnsubscribeInputVerticalEvent (const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Unregisters me so I'm no longer notified when an input VBI is signaled on the given input channel(s).
		@param[in]	inChannels	Specifies the input channel(s) of interest.
		@return		True if successful; otherwise false.
		@details	This function undoes the effects of a prior call to SubscribeInputVerticalEvents.
		@see		CNTV2Card::SubscribeInputVerticalEvents, CNTV2Card::UnsubscribeEvent, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	UnsubscribeInputVerticalEvent (const NTV2ChannelSet & inChannels);


	//
	//	Get interrupt counts
	//
	/**
		@brief		Answers with the number of output vertical interrupts handled by the driver for the given output channel.
		@param[out] outCount	Receives the number of output VBIs handled by the driver since it was loaded.
		@param[in]	inChannel	Specifies the output channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::SetOutputVerticalEventCount, CNTV2DriverInterface::GetInterruptEventCount, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	GetOutputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the number of input vertical interrupts handled by the driver for the given input channel.
		@param[out] outCount	Receives the number of input VBIs handled by the driver since it was loaded.
		@param[in]	inChannel	Specifies the input channel of interest. Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@see		CNTV2Card::SetInputVerticalEventCount, CNTV2DriverInterface::GetInterruptEventCount, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	GetInputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the number of output interrupt events that I successfully waited for on the given channel.
		@param[out] outCount		Receives the number of output interrupt events that were successfully waited for.
		@param[in]	inChannel		Specifies the NTV2Channel of interest.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetOutputVerticalEventCount, CNTV2DriverInterface::GetInterruptEventCount, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	GetOutputVerticalEventCount (ULWord & outCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the number of input interrupt events that I successfully waited for on the given channel.
		@param[out] outCount		Receives the number of input interrupt events that were successfully waited for.
		@param[in]	inChannel		Specifies the NTV2Channel of interest.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetInputVerticalEventCount, CNTV2Card::GetInterruptEventCount, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	GetInputVerticalEventCount (ULWord & outCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Resets my output interrupt event tally for the given channel.
		@param[in]	inCount			Specifies the new count value. Use zero to reset the tally.
		@param[in]	inChannel		Specifies the [output] channel.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetOutputVerticalEventCount, CNTV2Card::SetInterruptEventCount, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	SetOutputVerticalEventCount (const ULWord inCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Resets my input interrupt event tally for the given channel.
		@param[in]	inCount			Specifies the new count value. Use zero to reset the tally.
		@param[in]	inChannel		Specifies the [input] channel.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetInputVerticalEventCount, CNTV2Card::SetInterruptEventCount, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	SetInputVerticalEventCount (const ULWord inCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	//
	//	Current field ID
	//

	/**
		@brief		Returns the current field ID of the specified output channel.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
		@param[out] outFieldID		The current field ID of the specified output channel.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetOutputFieldID (const NTV2Channel channel, NTV2FieldID & outFieldID);

	/**
		@brief		Returns the current field ID of the specified input channel.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
		@param[out] outFieldID		The current field ID of the specified input channel.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetInputFieldID (const NTV2Channel channel, NTV2FieldID & outFieldID);

	//
	//	Wait for event
	//
	/**
		@brief		Efficiently sleeps the calling thread/process until the next one or more field (interlaced video)
					or frame (progressive or interlaced video) VBIs occur for the specified output channel.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
									Defaults to ::NTV2_CHANNEL1. Note that this parameter is irrelevant for all currently
									supported NTV2 devices, which use one common hardware clock that drives all SDI outputs.
		@param[in]	inRepeatCount	Specifies the number of output VBIs to wait for until returning. Defaults to 1.
		@return		True if successful; otherwise false. A <b>false</b> result usually indicates the wait timed out.
		@note		The device's timing reference source affects how often -- or even if -- the VBI occurs.
					See \ref deviceclockingandsync for more information.
		@note		For interlaced video, callers that need to know whether the field or frame interrupt occurred should
					call CNTV2Card::WaitForOutputFieldID instead.
		@bug		On the Windows platform, the SDK uses an event to wait on, which only gets cleared by a prior call to
					WaitForOutputVerticalInterrupt. This is historically different from Linux and MacOS, where the event
					is always cleared before the Wait. Each method has advantages and disadvantages. To work around this:
					-	Call CNTV2Card::GetOutputVerticalInterruptCount before and after calling this function to verify
						that an interrupt really occurred;
					-	Call CNTV2WinDriverInterface::GetInterruptEvent to obtain the Windows event \c HANDLE, and
						manually clear it before calling this function.
		@see		CNTV2Card::WaitForOutputFieldID, \ref fieldframeinterrupts, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool	WaitForOutputVerticalInterrupt (const NTV2Channel inChannel = NTV2_CHANNEL1, UWord inRepeatCount = 1);

	/**
		@brief		Efficiently sleeps the calling thread/process until the next output VBI for the given field and output
					channel.
		@param[in]	inFieldID	Specifies the field identifier of interest. Use ::NTV2_FIELD0 to wait for the frame
								interrupt of progressive or interlaced video. Use ::NTV2_FIELD1 to wait for the field
								interrupt of interlaced video.
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
								Defaults to ::NTV2_CHANNEL1. Note that this parameter is irrelevant for all currently
								supported NTV2 devices, which use one common hardware clock that drives all SDI outputs.
		@return		True if successful; otherwise false. A <b>false</b> result usually indicates the wait timed out.
		@note		The device's timing reference source affects how often -- or even if -- the VBI occurs.
					See \ref deviceclockingandsync for more information.
		@bug		On the Windows platform, the SDK uses an event to wait on, which only gets cleared by a prior call to
					CNTV2Card::WaitForOutputFieldID. This is historically different from Linux and MacOS, where the event
					is always cleared before the Wait. Each method has advantages and disadvantages. To work around this:
					-	Call CNTV2Card::GetOutputVerticalInterruptCount before and after calling this function to verify
						that an interrupt really occurred;
					-	Call CNTV2WinDriverInterface::GetInterruptEvent to get the event handle, and manually clear the
						event before calling this function.
		@see		CNTV2Card::WaitForOutputVerticalInterrupt, \ref fieldframeinterrupts, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool	WaitForOutputFieldID (const NTV2FieldID inFieldID, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Efficiently sleeps the calling thread/process until the next one or more field (interlaced video)
					or frame (progressive or interlaced video) VBIs occur for the specified input channel.
		@param[in]	inChannel		Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
									Defaults to ::NTV2_CHANNEL1.
		@param[in]	inRepeatCount	Specifies the number of input VBIs to wait for until returning. Defaults to 1.
		@return		True if successful; otherwise false. A <b>false</b> result usually indicates the wait timed out,
					which is often due to the FrameStore being configured for playout, or if its input is not receiving
					a valid signal.
		@bug		On the Windows platform, the SDK uses an event to wait on, which only gets cleared by a previous call to
					Wait. Unfortunately, this is historically different from Linux and MacOS, where the event is always
					cleared before the Wait. Each method has advantages and disadvantages. To work around this:
					-	Call GetInputVerticalInterruptCount before and after calling this function to verify that an
						interrupt really occurred;
					-	Call CNTV2WinDriverInterface::GetInterruptEvent to get the event handle, and manually clear the
						event before calling this function.
		@see		CNTV2Card::WaitForInputFieldID, \ref fieldframeinterrupts, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool	WaitForInputVerticalInterrupt (const NTV2Channel inChannel = NTV2_CHANNEL1, UWord inRepeatCount = 1);

	/**
		@brief		Efficiently sleeps the calling thread/process until the next input VBI for the given field and input
					channel.
		@param[in]	inFieldID	Specifies the field identifier of interest. Use ::NTV2_FIELD0 to wait for the frame
								interrupt of progressive or interlaced video. Use ::NTV2_FIELD1 to wait for the field
								interrupt of interlaced or Psf video.
		@param[in]	inChannel	Specifies the FrameStore of interest as an ::NTV2Channel (a zero-based index number).
								Defaults to ::NTV2_CHANNEL1.
		@return		True if successful; otherwise false. A <b>false</b> result usually indicates the wait timed out,
					which is often due to the FrameStore being configured for playout, or if its input is not receiving
					a valid signal.
		@bug		On the Windows platform, the SDK uses an event to wait on, which only gets cleared by a previous call to
					Wait. Unfortunately, this is historically different from Linux and MacOS, where the event is always
					cleared before the Wait. Each method has advantages and disadvantages. To work around this:
					-	Call GetInputVerticalInterruptCount before and after calling this function to verify that an
						interrupt really occurred;
					-	Call CNTV2WinDriverInterface::GetInterruptEvent to get the event handle, and manually clear the
						event before calling this function.
		@see		CNTV2Card::WaitForInputVerticalInterrupt, \ref fieldframeinterrupts, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool	WaitForInputFieldID (const NTV2FieldID inFieldID, const NTV2Channel inChannel = NTV2_CHANNEL1);

	//
	//	RegisterAccess Control
	//
	/**
		@brief		Sets the FrameStore's ::NTV2RegisterWriteMode, which determines when CNTV2Card::SetInputFrame or
					CNTV2Card::SetOutputFrame calls (and others) actually take effect.
		@param[in]	inValue			Specifies the ::NTV2RegisterWriteMode to set for the FrameStore.
		@param[in]	inFrameStore	Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
									If omitted, defaults to ::NTV2_CHANNEL1.
		@see		CNTV2Card::GetRegisterWriteMode, CNTV2Card::SetInputFrame, CNTV2Card::SetOutputFrame, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	SetRegisterWriteMode (const NTV2RegisterWriteMode inValue, const NTV2Channel inFrameStore = NTV2_CHANNEL1);

	/**
		@brief		Answers with the FrameStore's current ::NTV2RegisterWriteMode setting, which determines when
					CNTV2Card::SetInputFrame or CNTV2Card::SetOutputFrame calls (and others) actually take effect.
		@param[out] outValue		Receives the ::NTV2RegisterWriteMode that's currently being used by the FrameStore.
		@param[in]	inFrameStore	Specifies the FrameStore of interest as an ::NTV2Channel, a zero-based index number.
									If omitted, defaults to ::NTV2_CHANNEL1.
		@see		CNTV2Card::SetRegisterWriteMode, CNTV2Card::SetInputFrame, CNTV2Card::SetOutputFrame, \ref fieldframeinterrupts
	**/
	AJA_VIRTUAL bool	GetRegisterWriteMode (NTV2RegisterWriteMode & outValue, const NTV2Channel inFrameStore = NTV2_CHANNEL1);

	#if !defined (NTV2_DEPRECATE_16_0)
		AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool	SetRegisterWritemode(NTV2RegisterWriteMode inVal, const NTV2Channel inChan=NTV2_CHANNEL1))	{return SetRegisterWriteMode(inVal,inChan);}		///< @deprecated	Use CNTV2Card::SetRegisterWriteMode instead.
		AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool	GetRegisterWritemode(NTV2RegisterWriteMode & outVal, const NTV2Channel inChan=NTV2_CHANNEL1))	{return SetRegisterWriteMode(outVal,inChan);}	///< @deprecated	Use CNTV2Card::GetRegisterWriteMode instead.
	#endif	//	NTV2_DEPRECATE_16_0
	///@}

	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	SetForce64(ULWord force64));
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	GetForce64(ULWord* force64));
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	Get64BitAutodetect(ULWord* autodetect64));

	/**
		@name	AutoCirculate
	**/
	///@{
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	GetFrameStamp (NTV2Crosspoint channelSpec, ULWord frameNum, FRAME_STAMP_STRUCT* pFrameStamp));		///< @deprecated	Use CNTV2Card::AutoCirculateGetFrame instead.
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	GetAutoCirculate (NTV2Crosspoint channelSpec, AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus));	///< @deprecated	Use CNTV2Card::AutoCirculateGetStatus instead.


	/**
		@brief		Prepares for subsequent AutoCirculate ingest, designating a contiguous block of frame buffers on the device for use
					by the FrameStore/channel, and specifies other optional behaviors.
					Upon successful return, the channel's ::NTV2AutoCirculateState is set to ::NTV2_AUTOCIRCULATE_INIT where
					it will remain until a subsequent call is made to CNTV2Card::AutoCirculateStart or CNTV2Card::AutoCirculateStop.
		@return		\c true if successful; otherwise \c false.
		@param[in]	inChannel			Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
										FrameStores (and therefore channels) are available on the device.
		@param[in]	inFrameCount		Optionally specifies the number of contiguous device frame buffers to be continuously cycled
										through if zero is specified for both \c inStartFrameNumber and \c inEndFrameNumber.
										Defaults to 7. This value is ignored if a legitimate frame range is specified in the
										\c inStartFrameNumber and \c inEndFrameNumber parameters (see below).
		@param[in]	inAudioSystem		Specifies the Audio System to use, if any. Defaults to ::NTV2_AUDIOSYSTEM_INVALID (no audio).
		@param[in]	inOptionFlags		A bit mask that specifies additional AutoCirculate options (e.g., ::AUTOCIRCULATE_WITH_RP188,
										::AUTOCIRCULATE_WITH_LTC, ::AUTOCIRCULATE_WITH_ANC, etc.). Defaults to zero (no options).
		@param[in]	inNumChannels		Optionally specifies the number of channels to operate on when CNTV2Card::AutoCirculateStart or
										CNTV2Card::AutoCirculateStop are called. Defaults to 1. Must be greater than zero.
		@param[in]	inStartFrameNumber	Specifies the starting frame index number as a zero-based unsigned decimal integer. Defaults to zero.
										This parameter always overrides \c inFrameCount if, when specified with \c inEndFrameNumber,
										are both non-zero. If specified, must be less than \c inEndFrameNumber -- see \ref vidop-fbindexing
										for more information.
		@param[in]	inEndFrameNumber	Specifies the ending frame number as a zero-based unsigned decimal integer. Defaults to zero.
										This parameter always overrides \c inFrameCount if, when specified with \c inEndFrameNumber,
										are both non-zero. If specified, must be larger than \c inStartFrameNumber -- see \ref vidop-fbindexing
										for more information.
		@note		For Multi-Channel or 4K/8K applications (i.e. where more than one channel is used for streaming video), AJA
					recommends specifying zero for \c inFrameCount, and explicitly specifying a frame range using \c inStartFrameNumber
					and \c inEndFrameNumber parameters.
		@note		Fewer frames reduces latency, but increases the likelihood of frame drops.
					See \ref autocircfrmcnt.
		@note		All \ref ntv2signalrouting should be completed prior to calling this function.
		@note		This function logs \ref autocirculatemsgs to \ref usingajalogger or the \ref usinglogreader.
					Be sure the <tt>AutoCirculate_39</tt> message group is enabled, and the client application has called AJADebug::Open.
		@details	If this function succeeds, the driver will have designated a contiguous set of device frame buffers to be written by
					the FrameStore, and placed the channel into the ::NTV2_AUTOCIRCULATE_INIT state. The channel will then be ready for
					a subsequent call to CNTV2Card::AutoCirculateStart or CNTV2Card::AutoCirculateTransfer.
					If the device's ::NTV2EveryFrameTaskMode (see CNTV2Card::GetEveryFrameServices ) is ::NTV2_OEM_TASKS, the driver
					will perform most of the device setup, including configuring the FrameStore, etc.;
					otherwise (if ::NTV2_DISABLE_TASKS ), the caller must manage <i>all</i> aspects of the FrameStore ( ::NTV2Mode,
					::NTV2VideoFormat, etc.) before calling this function.
		@see		CNTV2Card::AutoCirculateStop, CNTV2Card::AutoCirculateInitForOutput, \ref autocirculatecapture
	**/

	AJA_VIRTUAL bool	AutoCirculateInitForInput ( const NTV2Channel		inChannel,
													const UWord				inFrameCount		= 7,
													const NTV2AudioSystem	inAudioSystem		= NTV2_AUDIOSYSTEM_INVALID,
													const ULWord			inOptionFlags		= 0,
													const UByte				inNumChannels		= 1,
													const UWord				inStartFrameNumber	= 0,
													const UWord				inEndFrameNumber	= 0);

	/**
		@brief		Prepares for subsequent AutoCirculate playout, designating a contiguous block of frame buffers on the device for use
					by the FrameStore/channel, and specifies other optional behaviors.
					Upon successful return, the channel's ::NTV2AutoCirculateState is set to ::NTV2_AUTOCIRCULATE_INIT where
					it will remain until a subsequent call is made to CNTV2Card::AutoCirculateStart or CNTV2Card::AutoCirculateStop.
		@return		\c true if successful; otherwise \c false.
		@param[in]	inChannel			Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
										FrameStores (and therefore channels) are available on the device.
		@param[in]	inFrameCount		Optionally specifies the number of contiguous device frame buffers to be continuously cycled
										through if zero is specified for both \c inStartFrameNumber and \c inEndFrameNumber.
										Defaults to 7. This value is ignored if a legitimate frame range is specified in the
										\c inStartFrameNumber and \c inEndFrameNumber parameters (see below).
		@param[in]	inAudioSystem		Specifies the Audio System to use, if any. Defaults to ::NTV2_AUDIOSYSTEM_INVALID (no audio).
		@param[in]	inOptionFlags		A bit mask that specifies additional AutoCirculate options (e.g., ::AUTOCIRCULATE_WITH_RP188,
										::AUTOCIRCULATE_WITH_LTC, ::AUTOCIRCULATE_WITH_ANC, etc.). Defaults to zero (no options).
		@param[in]	inNumChannels		Optionally specifies the number of channels to operate on when CNTV2Card::AutoCirculateStart or
										CNTV2Card::AutoCirculateStop are called. Defaults to 7. Must be greater than zero.
		@param[in]	inStartFrameNumber	Specifies the starting frame index number as a zero-based unsigned decimal integer. Defaults to zero.
										This parameter always overrides \c inFrameCount if, when specified with \c inEndFrameNumber,
										are both non-zero. If specified, must be less than \c inEndFrameNumber -- see \ref vidop-fbindexing
										for more information.
		@param[in]	inEndFrameNumber	Specifies the ending frame index number as a zero-based unsigned decimal integer. Defaults to zero.
										This parameter always overrides \c inFrameCount if, when specified with \c inEndFrameNumber,
										are both non-zero. If specified, must be larger than \c inStartFrameNumber -- see \ref vidop-fbindexing
										for more information.
		@note		For Multi-Channel or 4K/8K applications (i.e. where more than one channel is used for streaming video), AJA
					recommends specifying zero for \c inFrameCount, and explicitly specifying a frame range using \c inStartFrameNumber
					and \c inEndFrameNumber parameters.
		@note		Fewer frames reduces latency, but increases the likelihood of frame drops. See \ref autocirculatelowlatency.
		@note		All \ref ntv2signalrouting should be completed prior to calling this function.
		@note		This function logs \ref autocirculatemsgs to \ref usingajalogger or the \ref usinglogreader.
					Be sure the <tt>AutoCirculate_39</tt> message group is enabled, and the client application has called AJADebug::Open.
		@details	If this function succeeds, the driver will have designated a contiguous set of device frame buffers to be read by
					the FrameStore, and placed the channel into the ::NTV2_AUTOCIRCULATE_INIT state. The channel will then be ready for
					a subsequent call to CNTV2Card::AutoCirculateStart or CNTV2Card::AutoCirculateTransfer.
					If the device's ::NTV2EveryFrameTaskMode (see CNTV2Card::GetEveryFrameServices ) is ::NTV2_OEM_TASKS, the driver
					will perform most of the device setup, including configuring the FrameStore, setting the output standard, etc.;
					otherwise (if ::NTV2_DISABLE_TASKS ), the caller must manage <i>all</i> aspects of the FrameStore ( ::NTV2Mode,
					::NTV2VideoFormat, etc.) before calling this function.
		@see		CNTV2Card::AutoCirculateStop, CNTV2Card::AutoCirculateInitForInput, \ref autocirculateplayout
	**/

	AJA_VIRTUAL bool	AutoCirculateInitForOutput (const NTV2Channel		inChannel,
													const UWord				inFrameCount		= 7,
													const NTV2AudioSystem	inAudioSystem		= NTV2_AUDIOSYSTEM_INVALID,
													const ULWord			inOptionFlags		= 0,
													const UByte				inNumChannels		= 1,
													const UWord				inStartFrameNumber	= 0,
													const UWord				inEndFrameNumber	= 0);

	/**
		@brief		Starts AutoCirculating the specified channel that was previously initialized by CNTV2Card::AutoCirculateInitForInput or
					CNTV2Card::AutoCirculateInitForOutput.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
									FrameStores (and therefore channels) are available on the device.
		@param[in]	inStartTime		Optionally specifies a future start time as an unsigned 64-bit host-OS-dependent time value.
									If zero, the default, AutoCirculate will switch to the ::NTV2_AUTOCIRCULATE_RUNNING state at
									the next VBI received by the given channel. If non-zero, AutoCirculate will remain in the
									::NTV2_AUTOCIRCULATE_STARTING_AT_TIME state until the system tick clock exceeds this value, at
									which point it will switch to the ::NTV2_AUTOCIRCULATE_RUNNING state. This value is denominated
									in the same time units as the finest-grained time counter available on the host's operating system.
		@details	This function performs the following tasks:
					-	It sets the state of the channel in the driver from ::NTV2_AUTOCIRCULATE_INIT to ::NTV2_AUTOCIRCULATE_STARTING.
					-	At the next VBI, the driver's interrupt service routine (ISR) will check the OS tick clock, and if it exceeds
						\c inStartTime value, it will continue starting AutoCirculate -- otherwise the channel will remain in the
						::NTV2_AUTOCIRCULATE_STARTING phase, with the driver re-checking the clock at every subsequent VBI.
					-	The driver will change the channel's state to ::NTV2_AUTOCIRCULATE_RUNNING.
					-	The driver will start tracking which memory frames are available and which are empty.
					<b>Capture/Input Mode</b> -- The next frame to be recorded is determined, and the current last input audio sample is
					written into the next frame's FRAME_STAMP's acAudioInStartAddress field. Finally, the channel's active frame
					is set to the next frame number.
					<b>Playout/Output Mode</b> -- The next frame to go out the jack is determined, and the current last output audio
					sample is written into the next frame's FRAME_STAMP::acAudioOutStartAddress field. Finally, the channel's active
					frame is set to the next frame number.
					-	Finally, the driver will AutoCirculate frames at every VBI on a per-channel basis.
		@note		This method will fail if the specified channel's AutoCirculate state is not ::NTV2_AUTOCIRCULATE_INIT.
		@note		Calling CNTV2Card::AutoCirculateStart while in the ::NTV2_AUTOCIRCULATE_PAUSED state won't un-pause AutoCirculate.
					(Instead, it will restart it.)
		@see		CNTV2Card::AutoCirculateStop, CNTV2Card::AutoCirculatePause, \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	AutoCirculateStart (const NTV2Channel inChannel, const ULWord64 inStartTime = 0);

	/**
		@brief		Stops AutoCirculate for the given channel, and releases the on-device frame buffers that were allocated to it.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the ::NTV2Channel to stop. Call ::NTV2DeviceGetNumFrameStores to discover how many
									FrameStores (and therefore channels) are available on the device.
		@param[in]	inAbort			Specifies if AutoCirculate is to be immediately stopped, not gracefully.
									Defaults to false (graceful stop).
		@details	If asked to stop gracefully (the default), the channel's AutoCirculate state is set to ::NTV2_AUTOCIRCULATE_STOPPING,
					and at the next VBI, AutoCirculate is explicitly stopped, after which the channel's state is set to ::NTV2_AUTOCIRCULATE_DISABLED.
					Once this method has been called, the channel cannot be used until it gets reinitialized by a subsequent call
					to CNTV2Card::AutoCirculateInitForInput or CNTV2Card::AutoCirculateInitForOutput.
					When called with \c inAbort set to \c true, audio capture or playback is immediately stopped (if a valid ::NTV2AudioSystem
					was specified in the AutoCirculateInit call), and the AutoCirculate channel status is changed to ::NTV2_AUTOCIRCULATE_DISABLED.
		@see		CNTV2Card::AutoCirculatePause, CNTV2Card::AutoCirculateResume, CNTV2Card::AutoCirculateStart, \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	AutoCirculateStop (const NTV2Channel inChannel, const bool inAbort = false);

	/**
		@brief		Stops any number of AutoCirculate channels.
					See the other AutoCirculateStop function overload for full details on what happens to each channel.
		@return		True if all specified channels were successfully stopped; otherwise false.
		@param[in]	inChannels		Specifies the channel(s) to stop.
		@param[in]	inAbort			Specifies if each channel is to be stopped immediately (i.e. not gracefully).
									Defaults to false (graceful stop).
	**/
	AJA_VIRTUAL bool	AutoCirculateStop (const NTV2ChannelSet & inChannels, const bool inAbort = false);	//	New in SDK 17.0

	/**
		@brief		Pauses AutoCirculate for the given channel. Once paused, AutoCirculate can be resumed later by calling
					CNTV2Card::AutoCirculateResume, picking up at the next frame without any loss of audio synchronization.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
									FrameStores (and therefore channels) are available on the device.
		@param[in]	inAtFrameNum	Reserved for future use.  (Added to API in SDK 16.2)
		@details	When pausing, if the channel is in the ::NTV2_AUTOCIRCULATE_RUNNING state, it will be set to ::NTV2_AUTOCIRCULATE_PAUSED,
					and at the next VBI, the driver will explicitly stop audio circulating.
		@see		CNTV2Card::AutoCirculateResume, \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	AutoCirculatePause (const NTV2Channel inChannel,  const UWord inAtFrameNum = 0xFFFF);	//	Added inAtFrameNum in SDK 16.2

	/**
		@brief		Resumes AutoCirculate for the given channel, picking up at the next frame without loss of audio synchronization.
		@return		True if successful; otherwise false.
		@param[in]	inChannel			Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
										FrameStores (and therefore channels) are available on the device.
		@param[in]	inClearDropCount	Specify \c true to clear the AUTOCIRCULATE_STATUS::acFramesDropped counter; otherwise
										leaves it unchanged. Defaults to \c false (don't clear it).
		@details	When resuming, if the channel is in the ::NTV2_AUTOCIRCULATE_PAUSED state, it will be changed to ::NTV2_AUTOCIRCULATE_RUNNING, and at the next VBI, the
					driver will restart audio AutoCirculate.
		@see		CNTV2Card::AutoCirculatePause, \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	AutoCirculateResume (const NTV2Channel inChannel, const bool inClearDropCount = false);

	/**
		@brief		Flushes AutoCirculate for the given channel.
		@return		True if successful; otherwise false.
		@param[in]	inChannel			Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
										FrameStores (and therefore channels) are available on the device.
		@param[in]	inClearDropCount	Specify \c true to clear the AUTOCIRCULATE_STATUS::acFramesDropped counter; otherwise
										leaves it unchanged. Defaults to \c false (don't clear it).
		@details	On capture, flushes all recorded frames that haven't yet been transferred to the host.
					On playout, all queued frames that have already been transferred to the device (that haven't yet played)
					are discarded.
					In either mode, this function has no effect on the <b>Active Frame</b> (the frame currently being captured
					or played by the device hardware at the moment the function was called).
					The ::NTV2AutoCirculateState (::NTV2_AUTOCIRCULATE_RUNNING, etc.) for the given channel will remain unchanged.
		@see		See \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	AutoCirculateFlush (const NTV2Channel inChannel, const bool inClearDropCount = false);

	/**
		@brief		Tells AutoCirculate how many frames to skip before playout starts for the given channel.
		@return		True if successful; otherwise false.
		@param[in]	inChannel			Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
										FrameStores (and therefore channels) are available on the device.
		@param[in]	inPreRollFrames		Specifies the number of frames to skip (ignore) before starting AutoCirculate.
		@details	Normally used for playout, this method instructs the driver to mark the given number of frames as valid.
					It's useful only in the rare case when, after CNTV2Card::AutoCirculateInitForOutput was called, several
					frames have already been transferred to the device (perhaps using CNTV2Card::DMAWrite), and calling
					CNTV2Card::AutoCirculateStart will ignore those pre-rolled frames without an intervening
					CNTV2Card::AutoCirculateTransfer call.
		@note		This method does nothing if the channel's state is not currently ::NTV2_AUTOCIRCULATE_STARTING,
					::NTV2_AUTOCIRCULATE_RUNNING or ::NTV2_AUTOCIRCULATE_PAUSED, or if the channel was initialized by
					CNTV2Card::AutoCirculateInitForInput.
		@see		See \ref autocirculateplayout
	**/
	AJA_VIRTUAL bool	AutoCirculatePreRoll (const NTV2Channel inChannel, const ULWord inPreRollFrames);

	/**
		@brief		Returns the current AutoCirculate status for the given channel.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
									FrameStores (and therefore channels) are available on the device.
		@param[out] outStatus		Receives the ::AUTOCIRCULATE_STATUS information for the channel.
		@details	Clients can use the ::AUTOCIRCULATE_STATUS information to determine if there are sufficient readable frames
					in the driver to safely support a DMA transfer to host memory (for capture);  or to determine if any frames
					have been dropped.
		@see		See \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	AutoCirculateGetStatus (const NTV2Channel inChannel, AUTOCIRCULATE_STATUS & outStatus);


	/**
		@brief		Returns precise timing information for the given frame and channel that's currently AutoCirculating.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
									FrameStores (and therefore channels) are available on the device.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of interest. This value must be no less than
									AUTOCIRCULATE_STATUS::acStartFrame and no more than AUTOCIRCULATE_STATUS::acEndFrame
									for the given channel. For capture/ingest, it should be "behind the record head".
									For playout, it should be "behind the play head."
		@param[out] outFrameInfo	Receives the ::FRAME_STAMP information for the given frame number and channel.
		@details	When the given channel is AutoCirculating, the driver will continuously fill in a ::FRAME_STAMP record for the frame
					it's currently working on, which is intended to give enough information to determine if frames have been dropped
					either on input or output. Moreover, it allows for synchronization of audio and video by time-stamping the audio
					input address at the start and end of a video frame.
		@see		See \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	AutoCirculateGetFrameStamp (const NTV2Channel inChannel, const ULWord inFrameNumber, FRAME_STAMP & outFrameInfo);

	/**
		@brief		Immediately changes the <b>Active Frame</b> for the given channel.
		@return		True if successful; otherwise false.
		@param[in]	inChannel			Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
										FrameStores (and therefore channels) are available on the device.
		@param[in]	inNewActiveFrame	Specifies the zero-based frame number to use. This value must be no less than <i>acStartFrame</i>
										and no more than <i>acEndFrame</i> for the given channel (see AUTOCIRCULATE_STATUS).
		@details	This method, assuming it succeeds, changes the <b>Active Frame</b> for the given channel by having the driver
					change the FrameStore's <b>InputFrame</b> register (input) or <b>OutputFrame</b> register (output). When one
					of these registers change on the device, it won't take effect until the next VBI, which ensures, for example,
					that an outgoing frame won't suddenly change mid-frame.
		@note		This method does nothing if the channel's AutoCirculate state is not currently ::NTV2_AUTOCIRCULATE_STARTING,
					::NTV2_AUTOCIRCULATE_RUNNING or ::NTV2_AUTOCIRCULATE_PAUSED.
		@see		See \ref aboutautocirculate, \ref videooperation
	**/
	AJA_VIRTUAL bool	AutoCirculateSetActiveFrame (const NTV2Channel inChannel, const ULWord inNewActiveFrame);

	/**
		@brief		Transfers all or part of a frame as specified in the given ::AUTOCIRCULATE_TRANSFER object to/from the host.
		@param[in]	inChannel		Specifies the ::NTV2Channel to use. Call ::NTV2DeviceGetNumFrameStores to discover how many
									FrameStores (and therefore channels) are available on the device.
		@param		transferInfo	On entry, specifies the ::AUTOCIRCULATE_TRANSFER details.
									Upon return, contains information about the (successful) transfer.
		@details	It is recommended that this method be called from inside a loop in a separate execution thread, with a way to gracefully
					exit the loop. Once outside of the loop, CNTV2Card::AutoCirculateStop can then be called.
		@warning	It is the caller's responsibility to provide valid video, audio and ancillary data pointers (and byte counts)
					via the AUTOCIRCULATE_TRANSFER::SetBuffers function(s). Bad addresses and/or sizes can cause crashes.
		@note		Do not call this method with an ::NTV2Channel that's in the ::NTV2_AUTOCIRCULATE_DISABLED state.
		@note		The calling thread will block until the transfer completes (or fails).
		@note		For <b>capture</b>, there should be an available frame buffer on the device waiting to be transferred to the host.
					Call CNTV2Card::AutoCirculateGetStatus, and check AUTOCIRCULATE_STATUS::HasAvailableInputFrame.
		@note		For <b>playout</b>, there should be a free frame buffer on the device to accommodate the new frame being transferred.
					Call CNTV2Card::AutoCirculateGetStatus, and check AUTOCIRCULATE_STATUS::CanAcceptMoreOutputFrames.
		@note		For <b>playout</b> to IP devices running S2110 firmware, this method will automatically insert <b>VPID</b> and <b>RP188</b>
					timecode packets into the outgoing RTP Anc streams, even if CNTV2Card::AutoCirculateInitForOutput was called
					without ::AUTOCIRCULATE_WITH_ANC, or if Anc buffers weren't specified in the ::AUTOCIRCULATE_TRANSFER object.
					This default behavior can be overridden or disabled:
					-	To disable the default insertion of VPID, call CNTV2Card::SetSDIOutVPID, passing zeroes for the VPID values.
					-	To override the default <b>VPID</b> packets, call CNTV2Card::AutoCirculateInitForOutput with ::AUTOCIRCULATE_WITH_ANC,
						and insert your own <b>VPID</b> packet(s) into the ::AUTOCIRCULATE_TRANSFER object's Anc buffers.
					-	To disable the default insertion of <b>RP188</b>, nullify (clear) the AUTOCIRCULATE_TRANSFER::acOutputTimeCodes
						array storage.
					-	To override the default <b>RP188</b> packet(s), be sure CNTV2Card::AutoCirculateInitForOutput was called with
						::AUTOCIRCULATE_WITH_ANC, and insert your own <b>RP188</b> packets into the ::AUTOCIRCULATE_TRANSFER object's
						Anc buffers.
		@note		For <b>capture</b> from IP devices running S2110 firmware, this method will automatically extract <b>VPID</b> and
					<b>RP188</b> timecode packets from the incoming RTP Anc streams, even if not ::AUTOCIRCULATE_WITH_ANC, or without
					Anc buffers in the "transferInfo".
		@bug		For IP devices running S2110 firmware, this method performs many heap allocations in order to transparently
					support normal (<b>VPID</b> and <b>RP188</b>) and custom ancillary data. This feature should be re-implemented
					to use separate, per-channel, private, pre-allocated heaps for the STL objects and other buffers it uses.
		@see		CNTV2Card::DMAReadFrame, CNTV2Card::DMAWriteFrame, \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	AutoCirculateTransfer (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & transferInfo);

	/**
		@brief		Returns the device frame buffer numbers of the first unallocated contiguous band of frame buffers having the given
					size that are available for use. This function is called by CNTV2Card::AutoCirculateInitForInput and
					CNTV2Card::AutoCirculateInitForOutput whenever a frame count is specified in lieu of explicit frame ranges.
		@param[in]	inFrameCount		Specifies the desired number of contiguous device frame buffers. Must exceed zero.
		@param[out]	outStartFrame		Receives the starting device frame buffer number (or -1 upon failure).
		@param[out]	outEndFrame			Receives the ending device frame buffer number (or -1 upon failure).
		@param[in]	inFrameStore		Optionally specifies the FrameStore of interest for the desired frame count. Any memory
										being read or written by this FrameStore will be excluded when collecting unallocated
										regions of device SDRAM, and also its ::NTV2VideoFormat will determine if the frames are
										quad-sized or quad-quad-sized. Defaults to ::NTV2_CHANNEL_INVALID for backward compatibility.
		@return		True if successful; otherwise false.
		@note		Prior to SDK 16.1, this function returned invalid frame numbers if any AutoCirculate channels were actively
					processing UHD/4K or UHD2/8K. In SDK 16.2, this function failed (returned false) if any AutoCirculate channels
					were actively processing UHD/4K or UHD2/8K. In SDK 16.3, the function was corrected to work with UHD/4K and UHD2/8K,
					but requires specifying a FrameStore of interest.
		@see		See \ref aboutautocirculate
	**/
	AJA_VIRTUAL bool	FindUnallocatedFrames (const UWord inFrameCount, LWord & outStartFrame, LWord & outEndFrame,
												const NTV2Channel inFrameStore = NTV2_CHANNEL_INVALID);
	///@}


#define NTV2_STREAM_SUCCESS(__status__)  (__status__ == NTV2_STREAM_SUCCESS)
#define NTV2_STREAM_FAIL(__status__)  (__status__ != NTV2_STREAM_SUCCESS)

	/**
		@brief		Initialize a stream.  Put the stream queue and hardware in a known good state ready for use.  Releases
                    all buffers remaining in the queue.  At least one new buffer must be queued before starting the stream.
    **/
	AJA_VIRTUAL ULWord	StreamChannelInitialize (const NTV2Channel inChannel);

   	/**
		@brief		Release a stream.  Releases all buffers remaining in the queue.
    **/
	AJA_VIRTUAL ULWord	StreamChannelRelease (const NTV2Channel inChannel);

	/**
		@brief		Start a stream.  Put the stream is the active state to start processing queued buffers.  For frame
                    based video, the stream will start with the current buffer on the next frame sync.
		@return		The current stream status.
    **/
	AJA_VIRTUAL ULWord	StreamChannelStart (const NTV2Channel inChannel,
                                            NTV2StreamChannel& status);

	/**
		@brief		Stop a stream.  Put the stream is the idle state.  For frame based video, the stream will idle on 
                    the buffer on air after the next frame sync.
		@return		The current stream status.
    **/
	AJA_VIRTUAL ULWord	StreamChannelStop (const NTV2Channel inChannel,
                                           NTV2StreamChannel& status);

	/**
		@brief		Flush a stream.  Remove all the buffers from the stream except a currently active idle buffer.
                    The stream must be in the initialize or idle state.
		@return		The current stream status.
    **/
    AJA_VIRTUAL ULWord	StreamChannelFlush (const NTV2Channel inChannel,
                                            NTV2StreamChannel& status);

	/**
		@brief		Get the current stream status.
		@return		The current stream status.
    **/
	AJA_VIRTUAL ULWord	StreamChannelStatus (const NTV2Channel inChannel,
                                             NTV2StreamChannel& status);

	/**
		@brief		Wait for any stream event.  Returns for any state or buffer change.
		@return		The current stream status.
    **/
	AJA_VIRTUAL ULWord	StreamChannelWait (const NTV2Channel inChannel,
                                           NTV2StreamChannel& status);

	/**
		@brief		Queue a buffer to the stream.  The bufferCookie is a user defined identifier of the buffer
                    used by the stream methods.
		@return		The queued buffer status.
    **/
	AJA_VIRTUAL ULWord	StreamBufferQueue (const NTV2Channel inChannel,
                                           NTV2_POINTER inBuffer,
                                           ULWord64 bufferCookie,
                                           NTV2StreamBuffer& status);

	/**
		@brief		Remove the oldest buffer released by the streaming engine from the buffer queue.
		@return		The removed buffer status.
	**/
	AJA_VIRTUAL ULWord	StreamBufferRelease (const NTV2Channel inChannel,
											NTV2StreamBuffer& status);

	/**
		@brief		Get the status of the buffer identified by the bufferCookie.
		@return		The buffer status.
    **/
	AJA_VIRTUAL ULWord	StreamBufferStatus (const NTV2Channel inChannel,
											ULWord64 bufferCookie,
											NTV2StreamBuffer& status);





#if defined(READREGMULTICHANGE)
	/**
		@brief			Reads the given set of registers.
		@param[in]		inRegNums		Specifies the set of registers to be read.
		@param[out]		outValues		Receives the resulting register/value map. Any registers
										in the "inRegNums" set that don't appear in this map were
										not able to be read successfully.
		@return			True if all requested registers were successfully read; otherwise false.
		@note			This operation is not guaranteed to be performed atomically.
						A VBI may occur while the requested registers are being read.
	**/
	AJA_VIRTUAL bool	ReadRegisters (const NTV2RegNumSet & inRegNums,	 NTV2RegisterValueMap & outValues);
#endif	//	defined(READREGMULTICHANGE)

	/**
		@brief			Reads the given set of registers from the bank specified in position 0
		@param[in]		inBankSelect	Specifies the bank select register.
		@param[in]		inOutRegInfo	Specifies the register to be read (plus mask/shift/value).
		@return			True if all requested registers were successfully read; otherwise false.
		@note			This operation is not guaranteed to be performed atomically.
	**/
	AJA_VIRTUAL bool	BankSelectReadRegister (const NTV2RegInfo & inBankSelect, NTV2RegInfo & inOutRegInfo);

	/**
		@brief			Writes the given sequence of NTV2RegInfo's.
		@param[in]		inRegWrites		Specifies the sequence of NTV2RegInfo's to be written.
		@return			True if all registers were written successfully; otherwise false.
		@note			This operation is not guaranteed to be performed atomically.
	**/
	AJA_VIRTUAL bool	WriteRegisters (const NTV2RegisterWrites & inRegWrites);

	/**
		@brief			Writes the given set of registers to the bank specified at position 0.
		@param[in]		inBankSelect	Specifies the bank select register.
		@param[in]		inRegInfo		Specifies the register to be written, and its mask, shift and value.
		@return			True if all requested registers were successfully written; otherwise false.
		@note			This operation is not guaranteed to be performed atomically.
	**/
	AJA_VIRTUAL bool	BankSelectWriteRegister (const NTV2RegInfo & inBankSelect, const NTV2RegInfo & inRegInfo);

	/**
		@brief			Writes the block of virtual data.
		@param[in]		inTag				Tag for the virtual data.
		@param[in]		inVirtualData		Virtual data to be written
		@param[in]		inVirtualDataSize	Virtual data size to write
		@return			True if all requested data was successfully written; otherwise false.
		@note			This operation is not guaranteed to be performed atomically.
	**/
	AJA_VIRTUAL bool	WriteVirtualData (const ULWord inTag, const void* inVirtualData, const ULWord inVirtualDataSize);

	/**
		@brief			Reads the block of virtual data for a specific tag
		@param[in]		inTag				Tag for the virtual data.
		@param[out]		outVirtualData		Virtual data buffer to be written
		@param[in]		inVirtualDataSize	Virtual data size to read
		@return			True if all requested data was successfully read; otherwise false.
		@note			This operation is not guaranteed to be performed atomically.
	**/
	AJA_VIRTUAL bool	ReadVirtualData (const ULWord inTag, void* outVirtualData, const ULWord inVirtualDataSize);

	/**
		@brief			For devices that support it (see the ::NTV2DeviceCanDoSDIErrorChecks function in "ntv2devicefeatures.h"),
						this function fetches the SDI statistics for all SDI input spigots.
		@param[out]		outStats	Receives the SDI statistics for all SDI input spigots.
		@return			True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	ReadSDIStatistics (NTV2SDIInStatistics & outStats);

	/**
		@brief		Sets the device's intrinsic frame buffer size.
		@param[in]	inSize		Specifies the intrinsic frame size the hardware should use.
		@return		True if successful; otherwise false. Failure usually indicates the intrinsic frame size is not
					software-changeable.
		@see		\ref vidop-fbindexing, CNTV2Card::GetFrameBufferSize
	**/
	AJA_VIRTUAL bool	SetFrameBufferSize (const NTV2Framesize inSize);

	/**
		@brief		Sets the device's intrinsic frame buffer size.
		@param[in]	inChannel	Specifies the FrameStore to be affected. (Currently ignored -- see note below.)
		@param[in]	inValue		Specifies the new frame size. Must be NTV2_FRAMESIZE_8MB or NTV2_FRAMESIZE_16MB.
		@return		True if successful;	 otherwise false.
		@see		\ref vidop-fbindexing, CNTV2Card::GetFrameBufferSize
	**/
	AJA_VIRTUAL bool	SetFrameBufferSize (const NTV2Channel inChannel, const NTV2Framesize inValue);

	/**
		@brief		Answers with the frame size currently being used on the device.
		@param[in]	inChannel	Currently ignored. Use ::NTV2_CHANNEL1.
		@param[out] outValue	Receives the device's current frame size.
		@return		True if successful;	 otherwise false.
		@see		\ref vidop-fbindexing, CNTV2Card::SetFrameBufferSize
	**/
	AJA_VIRTUAL bool	GetFrameBufferSize (const NTV2Channel inChannel, NTV2Framesize & outValue);
	using CNTV2DriverInterface::GetFrameBufferSize;		//	Keep CNTV2DriverInterface::GetFrameBufferSize visible after being shadowed by CNTV2Card::GetFrameBufferSize

	/**
		@return		True if the device intrinsic frame buffer size is currently settable and set by software
					(overriding the firmware);  otherwise false.
		@see		\ref vidop-fbindexing
	**/
	AJA_VIRTUAL bool			IsBufferSizeSetBySW (void);

	/**
		@brief		Disables the given FrameStore.
		@param[in]	inChannel	Specifies the FrameStore as an NTV2Channel value.
		@return		True if successful;	 otherwise false.
		@note		It is not an error to disable a FrameStore that is already disabled.
		@see		CNTV2Card::EnableChannel, CNTV2Card::DisableChannels, \ref vidop-fs
	**/
	AJA_VIRTUAL bool	DisableChannel (const NTV2Channel inChannel);

	/**
		@brief		Disables the given FrameStore(s).
		@param[in]	inChannels	Specifies the FrameStore(s) to be disabled.
		@return		True if successful;	 otherwise false.
		@note		It is not an error to disable a FrameStore that is already disabled.
		@see		CNTV2Card::EnableChannels, CNTV2Card::DisableChannel, \ref vidop-fs
	**/
	AJA_VIRTUAL bool	DisableChannels (const NTV2ChannelSet & inChannels);

	/**
		@brief		Enables the given FrameStore.
		@param[in]	inChannel	Specifies the FrameStore as an NTV2Channel value.
		@return		True if successful;	 otherwise false.
		@note		It is not an error to enable a FrameStore that is already enabled.
		@see		CNTV2Card::DisableChannel, CNTV2Card::EnableChannels, \ref vidop-fs
	**/
	AJA_VIRTUAL bool	EnableChannel (const NTV2Channel inChannel);

	/**
		@brief		Enables the given FrameStore(s).
		@param[in]	inChannels			Specifies the FrameStore(s) to be enabled.
		@param[in]	inDisableOthers		If true, disables all other FrameStores on the device.
										Otherwise, leaves other FrameStores untouched.
										Defaults to false.
		@return		True if successful;	 otherwise false.
		@note		It is not an error to enable a FrameStore that is already enabled.
		@see		CNTV2Card::EnableChannel, CNTV2Card::DisableChannels, \ref vidop-fs
	**/
	AJA_VIRTUAL bool	EnableChannels (const NTV2ChannelSet & inChannels, const bool inDisableOthers = false);

	/**
		@brief		Answers whether or not the given FrameStore is enabled.
		@param[in]	inChannel	Specifies the FrameStore, as identified by an NTV2Channel value.
		@param[out] outEnabled	Receives "true" if the FrameStore is enabled, or "false" if disabled.
		@return		True if successful;	 otherwise false.
		@see		See \ref vidop-fs
	**/
	AJA_VIRTUAL bool	IsChannelEnabled (const NTV2Channel inChannel, bool & outEnabled);

	/**
		@brief		Answers with the set of channels that are currently enabled.
		@param[out] outChannels		Receives the set of enabled channels.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetDisabledChannels, CNTV2Card::EnableChannels, CNTV2Card::DisableChannels, \ref vidop-fs
	**/
	AJA_VIRTUAL bool	GetEnabledChannels (NTV2ChannelSet & outChannels);	//	New in SDK 16.3

	/**
		@brief		Answers with the set of channels that are currently disabled.
		@param[out] outChannels		Receives the set of enabled channels.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetEnabledChannels, CNTV2Card::EnableChannels, CNTV2Card::DisableChannels, \ref vidop-fs
	**/
	AJA_VIRTUAL bool	GetDisabledChannels (NTV2ChannelSet & outChannels);	//	New in SDK 16.3

	AJA_VIRTUAL bool	SetVideoDACMode (NTV2VideoDACMode inValue);
	AJA_VIRTUAL bool	GetVideoDACMode (NTV2VideoDACMode & outValue);


	/**
		@name	Timing/Offset Control
	**/
	///@{
	AJA_VIRTUAL bool	GetNominalMinMaxHV (int & outNominalH, int & outMinH, int & outMaxH, int & outNominalV, int & outMinV, int & outMaxV);

	/**
		@brief		Adjusts the horizontal timing offset, in pixels, for the given SDI output connector.
		@param[in]	inHOffset		Specifies the horizontal output timing offset, a signed value, in pixels after or before the nominal value.
		@param[in]	inOutputSpigot	(Added in SDK v16.1) Optionally specifies the SDI output connector of interest. Defaults to 0 (SDI Out 1).
		@note		The output timing can only be adjusted when the device's reference source is set for external reference.
		@note		The "inOutputSpigot" parameter is respected only if the device is multi-format-capable (see ::NTV2DeviceCanDoMultiFormat)
					and the device is currently in multi-format mode (see CNTV2Card::GetMultiFormatMode and CNTV2Card::SetMultiFormatMode).
					Otherwise, this function sets the horizontal timing offset for SDI Output 1 (i.e., the "global" output timing).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetVideoHOffset, CNTV2Card::SetVideoVOffset
	**/
	AJA_VIRTUAL bool	SetVideoHOffset (const int inHOffset, const UWord inOutputSpigot = 0);

	/**
		@brief		Answers with the current horizontal timing offset, in pixels, for the given SDI output connector.
		@param[out] outHOffset		Receives the current horizontal output timing offset, a signed value, in pixels after or before the nominal value.
		@param[in]	inOutputSpigot	(Added in SDK v16.1) Optionally specifies the SDI output connector of interest. Defaults to 0 (SDI Out 1).
		@note		The "inOutputSpigot" parameter is respected only if the device is multi-format-capable (see ::NTV2DeviceCanDoMultiFormat)
					and the device is currently in multi-format mode (see CNTV2Card::GetMultiFormatMode and CNTV2Card::SetMultiFormatMode).
					Otherwise, this function only reports the horizontal timing offset for SDI Output 1 (i.e., the "global" output timing).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetVideoHOffset, CNTV2Card::GetVideoVOffset
	**/
	AJA_VIRTUAL bool	GetVideoHOffset (int & outHOffset, const UWord inOutputSpigot = 0);

	/**
		@brief		Adjusts the vertical timing offset, in lines, for the given SDI output connector.
		@param[in]	inVOffset		Specifies the vertical output timing offset, a signed value, in lines after or before the nominal value.
		@param[in]	inOutputSpigot	(Added in SDK v16.1) Optionally specifies the SDI output connector of interest. Defaults to 0 (SDI Out 1).
		@note		The output timing can only be adjusted when the device's reference source is set for external reference.
		@note		The "inOutputSpigot" parameter is respected only if the device is multi-format-capable (see ::NTV2DeviceCanDoMultiFormat)
					and the device is currently in multi-format mode (see CNTV2Card::GetMultiFormatMode and CNTV2Card::SetMultiFormatMode).
					Otherwise, this function sets the vertical timing offset for SDI Output 1 (i.e., the "global" output timing).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetVideoVOffset, CNTV2Card::SetVideoHOffset
	**/
	AJA_VIRTUAL bool	SetVideoVOffset (const int inVOffset, const UWord inOutputSpigot = 0);

	/**
		@brief		Answers with the current vertical timing offset, in lines, for the given SDI output connector.
		@param[out] outVOffset		Receives the current vertical output timing offset, a signed value, in lines after or before the nominal value.
		@param[in]	inOutputSpigot	(Added in SDK v16.1) Optionally specifies the SDI output spigot of interest. Defaults to 0 (SDI Out 1).
		@note		The "inOutputSpigot" parameter is respected only if the device is multi-format-capable (see ::NTV2DeviceCanDoMultiFormat)
					and the device is currently in multi-format mode (see CNTV2Card::GetMultiFormatMode and CNTV2Card::SetMultiFormatMode).
					Otherwise, this function only reports the vertical timing offset for SDI Output 1 (i.e., the "global" output timing).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetVideoVOffset, CNTV2Card::GetVideoHOffset
	**/
	AJA_VIRTUAL bool	GetVideoVOffset (int & outVOffset, const UWord inOutputSpigot = 0);

	AJA_VIRTUAL bool	SetAnalogOutHTiming (ULWord inValue);
	AJA_VIRTUAL bool	GetAnalogOutHTiming (ULWord & outValue);

	/**
		@brief		Adjusts the output timing for the given SDI output connector.
		@param[in]	inValue			Specifies the output timing control value to use. The lower 16 bits of this 32-bit value
									control the horizontal timing, while the upper 16 bits control the vertical.
									Each horizontal increment/decrement moves the output relative to the reference by one pixel.
									Each vertical increment/decrement moves the output relative to the reference by one line.
		@param[in]	inOutputSpigot	Optionally specifies the SDI output connector of interest. Defaults to zero (SDI Out 1).
		@note		The output timing can only be adjusted when the device's reference source is set for external reference.
		@note		The "inOutputSpigot" parameter is respected only if the device is multi-format-capable (see ::NTV2DeviceCanDoMultiFormat)
					and the device is currently in multi-format mode (see CNTV2Card::GetMultiFormatMode and CNTV2Card::SetMultiFormatMode).
					Otherwise, the timing is changed for all SDI outputs.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::ReadOutputTimingControl, CNTV2Card::SetVideoVOffset, CNTV2Card::SetVideoHOffset
	**/
	AJA_VIRTUAL bool	WriteOutputTimingControl (const ULWord inValue, const UWord inOutputSpigot = 0);

	/**
		@brief		Returns the current output timing control value for the given SDI output connector.
		@param[out] outValue		Receives the current output timing control value.
		@param[in]	inOutputSpigot	Optionally specifies the SDI output connector of interest. Defaults to 0 (SDI Out 1).
		@note		The "inOutputSpigot" parameter is respected only if the device is multi-format-capable (see ::NTV2DeviceCanDoMultiFormat)
					and the device is currently in multi-format mode (see CNTV2Card::GetMultiFormatMode and CNTV2Card::SetMultiFormatMode).
					Otherwise, this function only reports the timing for SDI Output 1 (i.e., the "global" output timing).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::WriteOutputTimingControl, CNTV2Card::GetVideoVOffset, CNTV2Card::GetVideoHOffset
	**/
	AJA_VIRTUAL bool	ReadOutputTimingControl (ULWord & outValue, const UWord inOutputSpigot = 0);

	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	SetSDI1OutHTiming (ULWord value));
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	GetSDI1OutHTiming(ULWord* value));
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	SetSDI2OutHTiming (ULWord value));
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	GetSDI2OutHTiming(ULWord* value));
	///@}

	/**
		@name	VPID
	**/
	///@{
	/*
		@brief		Sets the VPID payload value(s) for each link of the given 3G SDI output.
		@param[in]	inValueA		Specifies the "Link A" VPID value to be embedded in the "A" link output signal.
		@param[in]	inValueB		Specifies the "Link B" VPID value to be embedded in the "B" link output signal.
		@param[in]	inOutputSpigot	Specifies the SDI output connector of interest as a zero-based index value.
									Defaults to zero, the first SDI output connector.
		@return		True if successful; otherwise false.
	*/
	AJA_VIRTUAL bool		SetSDIOutVPID (const ULWord inValueA, const ULWord inValueB, const UWord inOutputSpigot = NTV2_CHANNEL1);

	/*
		@brief		Answers with the VPID payload value(s) that are currently being embedded for each link of the given 3G SDI output.
		@param[out] outValueA		Receives the VPID payload value currently being embedded in the "A" link output signal.
		@param[out] outValueB		Receives the VPID payload value currently being embedded in the "B" link output signal.
		@param[in]	inOutputSpigot	Specifies the SDI output connector of interest as a zero-based index value.
									Defaults to zero, the first SDI output connector.
		@return		True if successful; otherwise false.
	*/
	AJA_VIRTUAL bool		GetSDIOutVPID (ULWord & outValueA, ULWord & outValueB, const UWord inOutputSpigot = NTV2_CHANNEL1);

	/*
		@brief		Reads the latest received VPID payload value(s) for each link of the given SDI input.
		@param[in]	inSDIInput	Specifies the 3G SDI input connector as an ::NTV2Channel, a zero-based index value.
		@param[out] outValueA	Receives the "Link A" VPID payload value -- or zero if VPID was not present.
		@param[out] outValueB	Receives the "Link B" VPID payload value -- or zero if VPID was not present.
		@return		True if successful; otherwise false.
	*/
	AJA_VIRTUAL bool		ReadSDIInVPID (const NTV2Channel inSDIInput, ULWord & outValueA, ULWord & outValueB);

	/**
		@return		True if the SDI VPID link A input status is valid;	otherwise false.
		@param[in]	inChannel		Specifies the SDI input connector of interest, specified as an ::NTV2Channel, a zero-based index value.
	**/
	AJA_VIRTUAL bool		GetVPIDValidA (const NTV2Channel inChannel);

	/**
		@return		True if the SDI VPID link B input status is valid;	otherwise false.
		@param[in]	inChannel		Specifies the SDI input connector of interest, specified as an ::NTV2Channel, a zero-based index value.
	**/
	AJA_VIRTUAL bool		GetVPIDValidB (const NTV2Channel inChannel);
	///@}

	/**
		@brief	Sets the SDI output spigot's video standard.
		@param[in]	inOutputSpigot	Specifies the SDI output spigot of interest as a zero-based index number, where zero is "SDI Out 1".
		@param[in]	inValue			Specifies the video standard.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	SetSDIOutputStandard (const UWord inOutputSpigot, const NTV2Standard inValue);

	/**
		@brief		Sets the video standard for the given SDI output(s).
		@param[in]	inSDIOutputs	Specifies the SDI output connector(s) of interest.
									Each is a zero-based index number, where zero is "SDIOut1".
		@param[in]	inValue			Specifies the video standard.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	SetSDIOutputStandard (const NTV2ChannelSet & inSDIOutputs, const NTV2Standard inValue); //	New in SDK v16.0

	/**
		@brief	Answers with the current video standard of the given SDI output spigot.
		@param[in]	inOutputSpigot	Specifies the SDI output spigot of interest as a zero-based index number, where zero is "SDI Out 1".
		@param[out] outValue		Receives the video standard.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	GetSDIOutputStandard (const UWord inOutputSpigot, NTV2Standard & outValue);


	/**
		@name	Up/Down Conversion
	**/
	///@{
	AJA_VIRTUAL bool	SetUpConvertMode (const NTV2UpConvertMode inValue);
	AJA_VIRTUAL bool	GetUpConvertMode (NTV2UpConvertMode & outValue);
	AJA_VIRTUAL bool	SetConverterOutStandard (const NTV2Standard inValue);
	AJA_VIRTUAL bool	GetConverterOutStandard (NTV2Standard & outValue);
	AJA_VIRTUAL bool	SetConverterOutRate (const NTV2FrameRate inValue);
	AJA_VIRTUAL bool	GetConverterOutRate (NTV2FrameRate & outValue);
	AJA_VIRTUAL bool	SetConverterInStandard (const NTV2Standard inValue);
	AJA_VIRTUAL bool	GetConverterInStandard (NTV2Standard & outValue);
	AJA_VIRTUAL bool	SetConverterInRate (const NTV2FrameRate inValue);
	AJA_VIRTUAL bool	GetConverterInRate (NTV2FrameRate & outValue);
	AJA_VIRTUAL bool	SetConverterPulldown (const ULWord inValue);
	AJA_VIRTUAL bool	GetConverterPulldown (ULWord & outValue);
	AJA_VIRTUAL bool	SetUCPassLine21 (const ULWord inValue);
	AJA_VIRTUAL bool	GetUCPassLine21 (ULWord & outValue);
	AJA_VIRTUAL bool	SetUCAutoLine21 (const ULWord inValue);
	AJA_VIRTUAL bool	GetUCAutoLine21 (ULWord & outValue);

	AJA_VIRTUAL bool	SetDownConvertMode (const NTV2DownConvertMode inValue);
	AJA_VIRTUAL bool	GetDownConvertMode (NTV2DownConvertMode & outValue);
	AJA_VIRTUAL bool	SetIsoConvertMode (const NTV2IsoConvertMode inValue);
	AJA_VIRTUAL bool	GetIsoConvertMode (NTV2IsoConvertMode & outValue);
	AJA_VIRTUAL bool	SetEnableConverter (const bool inValue);
	AJA_VIRTUAL bool	GetEnableConverter (bool & outValue);
	AJA_VIRTUAL bool	SetDeinterlaceMode (const ULWord inValue);
	AJA_VIRTUAL bool	GetDeinterlaceMode (ULWord & outValue);

	AJA_VIRTUAL bool	SetConversionMode (const NTV2ConversionMode inConversionMode);
	AJA_VIRTUAL bool	GetConversionMode (NTV2ConversionMode & outConversionMode);
	///@}

	/**
		@name	CSCs, LUTs & Color Correction
	**/
	///@{
	/**
		@brief		Selects the color space converter operation method.
		@param[in]	inCSCMethod		Specifies the method by which the color space converter will transform its input into its output.
		@param[in]	inChannel		Specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
		@return		True if the call was successful; otherwise false. 
		@note		When selecting ::NTV2_CSC_Method_Enhanced_4K as the method, the channel must be ::NTV2_CHANNEL1 or ::NTV2_CHANNEL5.
					This will group four CSCs together to process the 4K image. To leave 4K, take CSC 1 (or CSC 5) out of 4K mode. 
		@see		CNTV2Card::GetColorSpaceMethod, \ref vidop-csc and \ref widget_csc
	**/
	AJA_VIRTUAL bool		SetColorSpaceMethod (const NTV2ColorSpaceMethod inCSCMethod, const NTV2Channel inChannel);

	/**
		@brief		Answers with the current operating mode of the given color space converter.
		@param[out] outMethod	Receives the CSC's current operating method.
		@param[in]	inChannel	Optionally specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
								Defaults to ::NTV2_CHANNEL1 (CSC1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetColorSpaceMethod, \ref vidop-csc and \ref widget_csc
	**/
	AJA_VIRTUAL bool	GetColorSpaceMethod (NTV2ColorSpaceMethod & outMethod, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Sets the matrix type to be used for the given CSC, typically ::NTV2_Rec601Matrix or ::NTV2_Rec709Matrix.
		@param[in]	inType		Specifies the matrix type to be used.
		@param[in]	inChannel	Optionally specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
								Defaults to ::NTV2_CHANNEL1 (CSC1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetColorSpaceMatrixSelect, \ref vidop-csc and \ref widget_csc
	**/
	AJA_VIRTUAL bool	SetColorSpaceMatrixSelect (const NTV2ColorSpaceMatrixType inType, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the current matrix type being used for the given CSC.
		@param[out] outType		Receives the matrix type being used, typically ::NTV2_Rec601Matrix or ::NTV2_Rec709Matrix.
		@param[in]	inChannel	Optionally specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
								Defaults to ::NTV2_CHANNEL1 (CSC1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetColorSpaceMatrixSelect, \ref vidop-csc and \ref widget_csc
	**/
	AJA_VIRTUAL bool	GetColorSpaceMatrixSelect (NTV2ColorSpaceMatrixType & outType, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Sends the given color lookup tables (LUTs) to the given LUT and bank.
		@param[in]	inRedLUT	The Red LUT, a std::vector of double-precision floating-point values.
		@param[in]	inGreenLUT	The Green LUT, a std::vector of double-precision floating-point values.
		@param[in]	inBlueLUT	The Blue LUT, a std::vector of double-precision floating-point values.
		@param[in]	inLUT		Specifies the LUT of interest, expressed as an ::NTV2Channel (a zero-based index number).
		@param[in]	inBank		Specifies the LUT bank of interest (0 or 1).
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	DownloadLUTToHW (const NTV2DoubleArray & inRedLUT, const NTV2DoubleArray & inGreenLUT, const NTV2DoubleArray & inBlueLUT,
										const NTV2Channel inLUT, const int inBank);
	AJA_VIRTUAL bool	Download12BitLUTToHW (const NTV2DoubleArray & inRedLUT, const NTV2DoubleArray & inGreenLUT, const NTV2DoubleArray & inBlueLUT,
										const NTV2Channel inLUT, const int inBank);

	/**
		@brief		Writes the LUT tables to the given LUT and bank.
		@param[in]	inRedLUT	The Red LUT, a std::vector of UWord values.
		@param[in]	inGreenLUT	The Green LUT, a std::vector of UWord values.
		@param[in]	inBlueLUT	The Blue LUT, a std::vector of UWord values.
		@param[in]	inLUT		Specifies the LUT of interest, expressed as an ::NTV2Channel (a zero-based index number).
		@param[in]	inBank		Specifies the LUT bank of interest (0 or 1).
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	DownloadLUTToHW (const UWordSequence & inRedLUT, const UWordSequence & inGreenLUT, const UWordSequence & inBlueLUT,
										const NTV2Channel inLUT, const int inBank);
	AJA_VIRTUAL bool	Download12BitLUTToHW (const UWordSequence & inRedLUT, const UWordSequence & inGreenLUT, const UWordSequence & inBlueLUT,
										const NTV2Channel inLUT, const int inBank);

	/**
		@brief		Enables or disables the given LUT.
		@param[in]	inEnable	Specify true to enable, or false to disable.
		@param[in]	inLUT		Specifies the LUT of interest, expressed as an ::NTV2Channel (a zero-based index number).
		@return		True if successful;	 otherwise false.
		@note		This function only affects devices having version 2 LUTs (see ::NTV2DeviceGetLUTVersion).
	**/
	AJA_VIRTUAL bool	SetLUTEnable (const bool inEnable, const NTV2Channel inLUT);

	static bool			GenerateGammaTable (const NTV2LutType inLUTType, const int inBank, NTV2DoubleArray & outTable, const NTV2LutBitDepth inBitDepth = NTV2_LUT10Bit);
	static bool			GenerateGammaTable (const NTV2LutType inLUTType, const int inBank, UWordSequence & outTable, const NTV2LutBitDepth inBitDepth = NTV2_LUT10Bit);

	/**
		@brief		Writes the LUT tables.
		@param[in]	inRedLUT	The Red LUT, a std::vector of double-precision floating-point values.
		@param[in]	inGreenLUT	The Green LUT, a std::vector of double-precision floating-point values.
		@param[in]	inBlueLUT	The Blue LUT, a std::vector of double-precision floating-point values.
		@return		True if successful;	 otherwise false.
		@note		Version 2 LUTs (see ::NTV2DeviceGetLUTVersion) require setup of ::kRegLUTV2Control (register 376)
					for this function to work properly.
	**/
	AJA_VIRTUAL bool		LoadLUTTables (const NTV2DoubleArray & inRedLUT, const NTV2DoubleArray & inGreenLUT, const NTV2DoubleArray & inBlueLUT);
	AJA_VIRTUAL bool		Load12BitLUTTables (const NTV2DoubleArray & inRedLUT, const NTV2DoubleArray & inGreenLUT, const NTV2DoubleArray & inBlueLUT);

	/**
		@brief		Writes the LUT tables.
		@param[in]	inRedLUT		The Red LUT, a std::vector of unsigned 10-bit integer values.
		@param[in]	inGreenLUT		The Green LUT, a std::vector of unsigned 10-bit integer values.
		@param[in]	inBlueLUT		The Blue LUT, a std::vector of unsigned 10-bit integer values.
		@return		True if successful;	 otherwise false.
		@note		Version 2 LUTs (see ::NTV2DeviceGetLUTVersion) require setup of ::kRegLUTV2Control (register 376)
					for this function to work properly.
	**/
	AJA_VIRTUAL bool		WriteLUTTables (const UWordSequence & inRedLUT, const UWordSequence & inGreenLUT, const UWordSequence & inBlueLUT);
	AJA_VIRTUAL bool		Write12BitLUTTables (const UWordSequence & inRedLUT, const UWordSequence & inGreenLUT, const UWordSequence & inBlueLUT);

	/**
		@brief		Reads the LUT tables (as double-precision floating point values).
		@param[out] outRedLUT		Receives the Red LUT, a std::vector of double-precision floating-point values.
		@param[out] outGreenLUT		Receives the Green LUT, a std::vector of double-precision floating-point values.
		@param[out] outBlueLUT		Receives the Blue LUT, a std::vector of double-precision floating-point values.
		@return		True if successful;	 otherwise false.
		@note		Version 2 LUTs (see ::NTV2DeviceGetLUTVersion) require setup of ::kRegLUTV2Control (register 376)
					for this function to work properly.
	**/
	AJA_VIRTUAL bool		GetLUTTables (NTV2DoubleArray & outRedLUT, NTV2DoubleArray & outGreenLUT, NTV2DoubleArray & outBlueLUT);
	AJA_VIRTUAL bool		Get12BitLUTTables (NTV2DoubleArray & outRedLUT, NTV2DoubleArray & outGreenLUT, NTV2DoubleArray & outBlueLUT);

	/**
		@brief		Reads the LUT tables (as raw, unsigned 10-bit integers).
		@param[out] outRedLUT		Receives the Red LUT as a vector of unsigned 10-bit values (0-1023).
		@param[out] outGreenLUT		Receives the Green LUT as a vector of unsigned 10-bit values (0-1023).
		@param[out] outBlueLUT		Receives the Blue LUT as a vector of unsigned 10-bit values (0-1023).
		@return		True if successful;	 otherwise false.
		@note		Version 2 LUTs (see ::NTV2DeviceGetLUTVersion) require setup of ::kRegLUTV2Control (register 376)
					for this function to work properly.
	**/
	AJA_VIRTUAL bool		ReadLUTTables (UWordSequence & outRedLUT, UWordSequence & outGreenLUT, UWordSequence & outBlueLUT);
	AJA_VIRTUAL bool		Read12BitLUTTables (UWordSequence & outRedLUT, UWordSequence & outGreenLUT, UWordSequence & outBlueLUT);

	AJA_VIRTUAL bool		SetLUTV2HostAccessBank (const NTV2ColorCorrectionHostAccessBank inValue);
	AJA_VIRTUAL bool		GetLUTV2HostAccessBank (NTV2ColorCorrectionHostAccessBank & outValue, const NTV2Channel inChannel);

	AJA_VIRTUAL bool		SetLUTV2OutputBank (const NTV2Channel inLUTWidget, const ULWord inBank);
	AJA_VIRTUAL bool		GetLUTV2OutputBank (const NTV2Channel inLUTWidget, ULWord & outBank);

	AJA_VIRTUAL bool		Has12BitLUTSupport (void);
	
	/**
		@brief		Sets the LUT plane.
		@param[in]	inLUTPlane	Specifies the LUT plane of interest.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		Set12BitLUTPlaneSelect (const NTV2LUTPlaneSelect inLUTPlane);

	/**
		@brief		Answers with the current LUT plane.
		@param[out] outLUTPlane Receives the current LUT plane.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		Get12BitLUTPlaneSelect (NTV2LUTPlaneSelect & outLUTPlane);

	/**
		@brief		Sets the RGB range for the given CSC.
		@param[in]	inRange		Specifies the new RGB range (::NTV2_CSC_RGB_RANGE_FULL or ::NTV2_CSC_RGB_RANGE_SMPTE).
		@param[in]	inChannel	Optionally specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
								Call ::NTV2DeviceGetNumCSCs to determine the number of available CSCs on the device.
								Defaults to ::NTV2_CHANNEL1 (CSC1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetColorSpaceRGBBlackRange, \ref vidop-csc and \ref widget_csc
	**/
	AJA_VIRTUAL bool	SetColorSpaceRGBBlackRange (const NTV2_CSC_RGB_Range inRange,  const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the current RGB range being used by a given CSC.
		@param[out] outRange	Receives the RGB range (::NTV2_CSC_RGB_RANGE_FULL, ::NTV2_CSC_RGB_RANGE_SMPTE, or ::NTV2_CSC_RGB_RANGE_INVALID upon failure).
		@param[in]	inChannel	Optionally specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
								Call ::NTV2DeviceGetNumCSCs to determine the number of available CSCs on the device.
								Defaults to ::NTV2_CHANNEL1 (CSC1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetColorSpaceRGBBlackRange, \ref vidop-csc and \ref widget_csc
	**/
	AJA_VIRTUAL bool	GetColorSpaceRGBBlackRange (NTV2_CSC_RGB_Range & outRange,	const NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetColorSpaceUseCustomCoefficient (const ULWord inUseCustomCoefficient, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceUseCustomCoefficient (ULWord & outUseCustomCoefficient, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Specifies whether or not the given CSC will produce alpha channel data from its key input.
		@param[in]	inMakeAlphaFromKey		Specify true to have the CSC generate alpha channel data from a YCbCr video signal
											applied to its Key Input;  otherwise specify false to have it generate an "opaque" value.
		@param[in]	inChannel	Optionally specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
								Call ::NTV2DeviceGetNumCSCs to determine the number of available CSCs on the device.
								Defaults to ::NTV2_CHANNEL1 (CSC1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetColorSpaceMakeAlphaFromKey, \ref vidop-csc and \ref widget_csc
	**/
	AJA_VIRTUAL bool	SetColorSpaceMakeAlphaFromKey (const bool inMakeAlphaFromKey, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers whether or not the given CSC is set to produce alpha channel data from its key input.
		@param[out] outMakeAlphaFromKey		Receives true if the CSC is generating alpha channel data from the YCbCr video signal
											being applied to its Key Input;	 otherwise receives false.
		@param[in]	inChannel	Optionally specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
								Call ::NTV2DeviceGetNumCSCs to determine the number of available CSCs on the device.
								Defaults to ::NTV2_CHANNEL1 (CSC1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetColorSpaceMakeAlphaFromKey, \ref vidop-csc and \ref widget_csc
	**/
	AJA_VIRTUAL bool	GetColorSpaceMakeAlphaFromKey (ULWord & outMakeAlphaFromKey, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers whether or not the video signal present at the CSC’s Key Input is in sync with the video signal present
					at its Video Input.
		@param[out] outVideoKeySyncFail		Receives true if the video signal present at the CSC’s Key Input is NOT sync'd to the
											video signal present at its Video Input;  otherwise receives false.
		@param[in]	inChannel	Optionally specifies the CSC of interest, a zero-based index value expressed as an ::NTV2Channel.
								Call ::NTV2DeviceGetNumCSCs to determine the number of available CSCs on the device.
								Defaults to ::NTV2_CHANNEL1 (CSC1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetColorSpaceMakeAlphaFromKey, CNTV2Card::SetColorSpaceMakeAlphaFromKey, \ref vidop-csc and \ref widget_csc
		@note		The "outVideoKeySyncFail" result is valid and trustworthy when all of the following are true:
					-	the Video Input is connected to a YCbCr signal source crosspoint;
					-	the CSC's "Make Alpha From Key" setting is enabled;
					-	the Key Input is connected to a YCbCr signal source crosspoint.
	**/
	AJA_VIRTUAL bool	GetColorSpaceVideoKeySyncFail (bool & outVideoKeySyncFail, const NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetColorSpaceCustomCoefficients (const NTV2CSCCustomCoeffs & inCustomCoefficients, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceCustomCoefficients (NTV2CSCCustomCoeffs & outCustomCoefficients, const NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetColorSpaceCustomCoefficients12Bit (const NTV2CSCCustomCoeffs & inCustomCoefficients, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceCustomCoefficients12Bit (NTV2CSCCustomCoeffs & outCustomCoefficients, const NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetLUTControlSelect (const NTV2LUTControlSelect inLUTSelect);
	AJA_VIRTUAL bool	GetLUTControlSelect (NTV2LUTControlSelect & outLUTSelect);

	//
	// Color Correction Functions (KHD only )
	//
	AJA_VIRTUAL bool		SetColorCorrectionMode(const NTV2Channel inChannel, const NTV2ColorCorrectionMode inMode);
	AJA_VIRTUAL bool		GetColorCorrectionMode(const NTV2Channel inChannel, NTV2ColorCorrectionMode & outMode);

	/**
		@brief		Sets the LUT bank to be used for the given LUT.
		@param[in]	inLUTWidget		Specifies the LUT widget of interest as an ::NTV2Channel, a zero-based index number.
		@param[in]	inBank			Specifies the bank number to be used. Must be 0 or 1.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::GetColorCorrectionOutputBank, \ref vidop-lut
	**/
	AJA_VIRTUAL bool		SetColorCorrectionOutputBank (const NTV2Channel inLUTWidget, const ULWord inBank);

	/**
		@brief		Answers with the current LUT bank in use for the given LUT.
		@param[in]	inLUTWidget		Specifies the LUT widget of interest as an ::NTV2Channel, a zero-based index number.
		@param[out] outBank			Receives the bank number that's currently in use (0 or 1).
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::SetColorCorrectionOutputBank, \ref vidop-lut
	**/
	AJA_VIRTUAL bool		GetColorCorrectionOutputBank (const NTV2Channel inLUTWidget, ULWord & outBank);

	AJA_VIRTUAL bool		SetColorCorrectionHostAccessBank (const NTV2ColorCorrectionHostAccessBank inValue);
	AJA_VIRTUAL bool		GetColorCorrectionHostAccessBank (NTV2ColorCorrectionHostAccessBank & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool		SetColorCorrectionSaturation (const NTV2Channel inChannel, const ULWord inValue);
	AJA_VIRTUAL bool		GetColorCorrectionSaturation (const NTV2Channel inChannel, ULWord & outValue);

	AJA_VIRTUAL bool		SetDitherFor8BitInputs (const NTV2Channel inChannel, const ULWord inDither);
	AJA_VIRTUAL bool		GetDitherFor8BitInputs (const NTV2Channel inChannel, ULWord & outDither);

	//	Old APIs
	static NTV2_SHOULD_BE_DEPRECATED(bool	GenerateGammaTable (const NTV2LutType inLUTType, const int inBank, double * pOutTable));
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(NTV2ColorSpaceMethod	GetColorSpaceMethod (const NTV2Channel inChannel));
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool	DownloadLUTToHW(const double * pInTable, const NTV2Channel inChannel, const int inBank));
	AJA_VIRTUAL NTV2_SHOULD_BE_DEPRECATED(bool		LoadLUTTable (const double * pInTable));
	///@}


	AJA_VIRTUAL bool	SetSecondaryVideoFormat(NTV2VideoFormat inFormat);			//	RETAIL USE ONLY
	AJA_VIRTUAL bool	GetSecondaryVideoFormat(NTV2VideoFormat & outFormat);		//	RETAIL USE ONLY

	#if !defined(R2_DEPRECATE)
	AJA_VIRTUAL bool	SetInputVideoSelect (NTV2InputVideoSelect inInputSelect);	//	RETAIL USE ONLY
	AJA_VIRTUAL bool	GetInputVideoSelect(NTV2InputVideoSelect & outInputSelect); //	RETAIL USE ONLY
	#endif

	//	--------------------------------------------
	//	GetNTV2VideoFormat functions
	//		@deprecated		These static functions don't work correctly, and will be deprecated.
	//		For a given frame rate, geometry and transport, there may be 2 (or more!) possible matching video formats.
	//		The improved GetNTV2VideoFormat function may return a new CNTV2SDIVideoInfo object that can be interrogated about many things.
	//		@note			This function originated in CNTV2Status.
	static NTV2VideoFormat		GetNTV2VideoFormat (NTV2FrameRate frameRate, UByte inputGeometry, bool progressiveTransport, bool isThreeG, bool progressivePicture=false);
	static NTV2VideoFormat		GetNTV2VideoFormat (NTV2FrameRate frameRate, NTV2Standard standard, bool isThreeG, UByte inputGeometry=0, bool progressivePicture=false, bool isSquareDivision = true);
	//	--------------------------------------------

public:
	/**
		@brief		Returns the video format of the signal that is present on the given input source.
		@param[in]	inVideoSource		Specifies the video input source as an ::NTV2InputSource.
		@param[in]	inIsProgressive		Optionally specifies if the video format is expected to be progressive or not.
										Defaults to false (presumed to be interlaced).
		@return		A valid ::NTV2VideoFormat if successful; otherwise returns ::NTV2_FORMAT_UNKNOWN.
		@details	This function allows client applications to determine the kind of video signal, if any, is being presented
					to a given input source of the device. Because the hardware has no way of knowing if the incoming signal
					is progressive or interlaced (e.g., 525/29.97fps progressive versus 525/59.94fps interlaced),
					the function assumes interlaced, but the caller can override the function's "interlace" assumption.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetInputVideoFormat (const NTV2InputSource inVideoSource = NTV2_INPUTSOURCE_SDI1, const bool inIsProgressive = false);

	/**
		@brief		Returns the video format of the signal that is present on the given SDI input source.
		@param[in]	inChannel			Specifies the SDI input connector as an ::NTV2Channel value, a zero-based index number.
		@param[in]	inIsProgressive		Optionally specifies if the video format is expected to be progressive or not.
										Defaults to false (presumed to be interlaced).
		@return		A valid ::NTV2VideoFormat if successful; otherwise returns ::NTV2_FORMAT_UNKNOWN.
		@details	This function allows client applications to determine the kind of video signal, if any, is being presented
					to a given input source of the device. Because the hardware has no way of knowing if the incoming signal
					is progressive or interlaced (e.g., 525/29.97fps progressive versus 525/59.94fps interlaced),
					the function assumes interlaced, but the caller can override this assumption.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetSDIInputVideoFormat (NTV2Channel inChannel, bool inIsProgressive = false);

	/**
		@return		A valid ::NTV2VideoFormat if successful; otherwise returns ::NTV2_FORMAT_UNKNOWN.
		@param[in]	inHDMIInput		Specifies the HDMI input of interest as an ::NTV2Channel, a zero-based index value.
									Defaults to ::NTV2_CHANNEL1 (the first HDMI input).
	**/
	AJA_VIRTUAL NTV2VideoFormat GetHDMIInputVideoFormat (NTV2Channel inHDMIInput = NTV2_CHANNEL1);

	/**
		@brief		Returns the video format of the signal that is present on the device's analog video input.
		@return		A valid ::NTV2VideoFormat if successful; otherwise returns ::NTV2_FORMAT_UNKNOWN.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetAnalogInputVideoFormat (void);

	/**
		@brief		Returns the video format of the signal that is present on the device's composite video input.
		@return		A valid ::NTV2VideoFormat if successful; otherwise returns ::NTV2_FORMAT_UNKNOWN.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetAnalogCompositeInputVideoFormat (void);

	/**
		@brief		Returns the video format of the signal that is present on the device's reference input.
		@return		A valid ::NTV2VideoFormat if successful; otherwise returns ::NTV2_FORMAT_UNKNOWN.
		@note		The returned video format, if valid, will be an SD format for black burst and HD for tri-level.
		@note		If the device is capable of reading analog LTC from its reference input connector
					(i.e. if ::NTV2DeviceCanDoLTCInOnRefPort returns true), then CNTV2Card::GetLTCInputEnable
					should be called to confirm that the reference port is reading reference, and not LTC.
		@see		GetReference, SetReference, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL NTV2VideoFormat GetReferenceVideoFormat (void);
	
	AJA_VIRTUAL NTV2FrameRate GetSDIInputRate (const NTV2Channel channel);
	AJA_VIRTUAL NTV2FrameGeometry GetSDIInputGeometry (const NTV2Channel channel);
	AJA_VIRTUAL bool GetSDIInputIsProgressive (const NTV2Channel channel);

	AJA_VIRTUAL bool	GetSDIInput3GPresent (bool & outValue, const NTV2Channel channel);
	AJA_VIRTUAL bool	GetSDIInput3GbPresent (bool & outValue, const NTV2Channel channel);
	AJA_VIRTUAL bool	GetSDIInput6GPresent (bool & outValue, const NTV2Channel channel);
	AJA_VIRTUAL bool	GetSDIInput12GPresent (bool & outValue, const NTV2Channel channel);

	/**
		@name	Signal Routing
	**/
	///@{

	/**
		@brief		Answers with the currently connected NTV2OutputCrosspointID for the given NTV2InputCrosspointID.
		@param[in]	inInputXpt		Specifies the input (signal sink) of interest.
		@param[out] outOutputXpt	Receives the output (signal source) the given input is connected to (if connected),
									or NTV2_XptBlack if not connected.
		@return		True if successful;	 otherwise false.
		@see		\ref ntv2signalrouting, CNTV2Card::GetConnectedInput, CNTV2Card::IsConnected
	**/
	AJA_VIRTUAL bool	GetConnectedOutput (const NTV2InputCrosspointID inInputXpt, NTV2OutputCrosspointID & outOutputXpt);

	/**
		@brief		Answers with a connected NTV2InputCrosspointID for the given NTV2OutputCrosspointID.
		@param[in]	inOutputXpt		Specifies the output (signal source) of interest.
		@param[out] outInputXpt		Receives one of the input (signal sink) the given output is connected to (if connected),
									or NTV2_XptBlack if not connected.
		@return		True if successful;	 otherwise false.
		@see		\ref ntv2signalrouting, CNTV2Card::GetConnectedOutput, CNTV2Card::IsConnected
		@bug		If the output is connected to more than one widget input, this function only returns the first one found.
	**/
	AJA_VIRTUAL bool	GetConnectedInput (const NTV2OutputCrosspointID inOutputXpt, NTV2InputCrosspointID & outInputXpt);

	/**
		@brief		Answers with all of the NTV2InputCrosspointIDs that are connected to the given NTV2OutputCrosspointID.
		@param[in]	inOutputXpt		Specifies the output (signal source) of interest.
		@param[out] outInputXpts	Receives the ::NTV2InputCrosspointIDSet of the inputs (signal sinks) the given output
									is connected to (if connected). If none are connected, the set will be empty.
		@return		True if successful;	 otherwise false.
		@see		\ref ntv2signalrouting, CNTV2Card::GetConnectedOutput, CNTV2Card::IsConnected
	**/
	AJA_VIRTUAL bool	GetConnectedInputs (const NTV2OutputCrosspointID inOutputXpt, NTV2InputCrosspointIDSet & outInputXpts); //	New in SDK 15.5

	/**
		@brief		Connects the given widget signal input (sink) to the given widget signal output (source).
		@param[in]	inInputXpt		Specifies the input (signal sink) to be connected to the given output.
		@param[in]	inOutputXpt		Specifies the output (signal source) to be connected to the given input.
									Specifying NTV2_XptBlack effects a disconnect.
		@param[in]	inValidate		If true, calls NTV2Card::CanConnect to verify that the connection exists in firmware
									before writing the crosspoint register;	 otherwise writes the crosspoint register
									regardless. Defaults to false.
		@return		True if successful;	 otherwise false.
		@see		\ref ntv2signalrouting, CNTV2Card::Disconnect, CNTV2Card::IsConnected
	**/
	AJA_VIRTUAL bool	Connect (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, const bool inValidate = false);

	/**
		@brief		Disconnects the given widget signal input (sink) from whatever output (source) it may be connected.
		@param[in]	inInputXpt		Specifies the input (signal sink) to be disconnected.
		@return		True if successful;	 otherwise false.
		@see		\ref ntv2signalrouting, CNTV2Card::Connect
	**/
	AJA_VIRTUAL bool	Disconnect (const NTV2InputCrosspointID inInputXpt);

	/**
		@brief		Answers whether or not the given widget signal input (sink) is connected to another output (source).
		@param[in]	inInputXpt		Specifies the input (signal sink) of interest.
		@param[out] outIsConnected	Receives true if the input is connected to any other output (other than NTV2_XptBlack).
		@return		True if successful;	 otherwise false.
		@note		If the input is connected to NTV2_XptBlack, "outIsConnected" will be "false".
		@see		\ref ntv2signalrouting, CNTV2Card::IsConnectedTo
	**/
	AJA_VIRTUAL bool	IsConnected (const NTV2InputCrosspointID inInputXpt, bool & outIsConnected);

	/**
		@brief		Answers whether or not the given widget signal input (sink) is connected to another output (source).
		@param[in]	inInputXpt		Specifies the input (signal sink) of interest.
		@param[in]	inOutputXpt		Specifies the output (signal source) of interest. It's okay to specify NTV2_XptBlack.
		@param[out] outIsConnected	Receives true if the input is connected to the specified output.
		@return		True if successful;	 otherwise false.
		@see		CNTV2Card::IsConnected, \ref ntv2signalrouting
	**/
	AJA_VIRTUAL bool	IsConnectedTo (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, bool & outIsConnected);


	/**
		@brief		Answers whether or not the given widget signal input (sink) can legally be connected to the given signal output (source).
		@param[in]	inInputXpt		Specifies the input (signal sink) of interest.
		@param[in]	inOutputXpt		Specifies the output (signal source) of interest.
		@param[out] outCanConnect	Receives true if the input can be connected to the specified output;  otherwise false.
		@return		True if successful;	 otherwise false.
		@note		This function will return false (failure) if the device firmware doesn't support route validation.
		@see		\ref ntv2signalrouting, CNTV2Card::Connect
	**/
	AJA_VIRTUAL bool	CanConnect (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, bool & outCanConnect);


	/**
		@brief		Applies the given routing table to the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inRouter		Specifies the CNTV2SignalRouter that contains the routing to be applied to the device.
		@param[in]	inReplace		If true, replaces the device's existing widget routing with the given one.
									If false, augments the device's existing widget routing.
									Defaults to false.
		@details	Most modern AJA devices do not have fixed interconnections between inputs, outputs, FrameStores
					and the various video processing widgets (e.g., colorspace converters, mixer/keyers, etc.).
					Instead, these routing configurations are designated by a set of registers, one for each input
					of each widget. The register's value determines which widget output node (crosspoint) the input
					is connected to. A zero value in the register means that the input is not connected to anything.
					To simplify this process of routing widgets on the device, a set of signal paths (i.e., interconnects)
					are built and then applied to the device in this function call.
					This function iterates over each connection that's specified in the given routing table and updates
					the appropriate register in the device.
		@see		\ref ntv2signalrouting, CNTV2SignalRouter
	**/
	AJA_VIRTUAL bool	ApplySignalRoute (const CNTV2SignalRouter & inRouter, const bool inReplace = false);


	/**
		@brief		Applies the given widget routing connections to the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inConnections	Specifies the routing connections to be applied to the device.
		@param[in]	inReplace		If true, replaces the device's existing widget routing with the given one.
									If false, augments the device's existing widget routing.
									Defaults to false.
		@details	Most modern AJA devices do not have fixed interconnections between inputs, outputs, FrameStores
					and the various video processing widgets (e.g., colorspace converters, mixer/keyers, etc.).
					Instead, these routing configurations are designated by a set of registers, one for each input
					of each widget. The register's value determines which widget output node (crosspoint) the input
					is connected to. A zero value in the register means that the input is not connected to anything.
					To simplify this process of routing widgets on the device, a set of signal paths (i.e., interconnects)
					are built and then applied to the device in this function call.
					This function iterates over each connection that's specified in the given routing table and updates
					the appropriate register in the device.
		@see		\ref ntv2signalrouting
	**/
	AJA_VIRTUAL bool	ApplySignalRoute (const NTV2XptConnections & inConnections, const bool inReplace = false);

	/**
		@brief		Removes the given widget routing connections from the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inConnections	Specifies the routing connections to be removed from the device.
		@see		\ref ntv2signalrouting
	**/
	AJA_VIRTUAL bool	RemoveConnections (const NTV2XptConnections & inConnections);

	/**
		@brief		Removes all existing signal path connections between any and all widgets on the AJA device.
		@return		True if successful; otherwise false.
		@details	This function writes zeroes into all crosspoint selection registers, effectively
					clearing any existing routing configuration on the device.
		@see		\ref ntv2signalrouting
	**/
	AJA_VIRTUAL bool	ClearRouting (void);

	/**
		@brief		Answers with the current signal routing between any and all widgets on the AJA device.
		@param[out] outRouting	Receives the current signal routing.
		@return		True if successful; otherwise false.
		@see		\ref ntv2signalrouting, CNTV2SignalRouter, CNTV2Card::GetRoutingForChannel, CNTV2Card::ApplySignalRoute
	**/
	AJA_VIRTUAL bool	GetRouting (CNTV2SignalRouter & outRouting);

	/**
		@brief		Answers with the device's current widget routing connections.
		@return		True if successful; otherwise false.
		@param[out] outConnections	Receives the current routing connections.
		@see		\ref ntv2signalrouting
	**/
	AJA_VIRTUAL bool	GetConnections (NTV2XptConnections & outConnections);	//	New in SDK 16.0

	/**
		@brief		Answers with the current signal routing for the given channel.
		@param[in]	inChannel	Specifies the NTV2Channel of interest.
		@param[out] outRouting	Receives the current signal routing for the given channel.
		@return		True if successful; otherwise false.
		@see		\ref ntv2signalrouting, CNTV2SignalRouter, CNTV2Card::GetRouting, CNTV2Card::ApplySignalRoute
	**/
	AJA_VIRTUAL bool	GetRoutingForChannel (const NTV2Channel inChannel, CNTV2SignalRouter & outRouting);

#if !defined(NTV2_DEPRECATE_17_0)
	AJA_VIRTUAL inline NTV2_SHOULD_BE_DEPRECATED(bool HasCanConnectROM(void)) {return IsSupported(kDeviceHasXptConnectROM);}	///< @deprecated	Call IsSupported(kDeviceHasXptConnectROM) or features().HasCrosspointConnectROM() instead
#endif	//	!defined(NTV2_DEPRECATE_17_0)
	/**
		@brief		Answers with the implemented crosspoint connections (if known).
		@param[out] outConnections	Receives the device's ::NTV2PossibleConnections.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	GetPossibleConnections (NTV2PossibleConnections & outConnections);
	///@}


	/**
		@name	Analog
		@brief	These functions only work on devices with analog inputs.
	**/
	///@{
	AJA_VIRTUAL bool		WriteSDProcAmpControlsInitialized (const ULWord inNewValue = 1);
	AJA_VIRTUAL bool		WriteSDBrightnessAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteSDContrastAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteSDSaturationAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteSDHueAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteSDCbOffsetAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteSDCrOffsetAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteHDProcAmpControlsInitialized (const ULWord inNewValue = 1);
	AJA_VIRTUAL bool		WriteHDBrightnessAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteHDContrastAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteHDSaturationAdjustmentCb (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteHDSaturationAdjustmentCr (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteHDCbOffsetAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteHDCrOffsetAdjustment (const ULWord inNewValue);

	AJA_VIRTUAL bool		ReadSDProcAmpControlsInitialized (ULWord & outValue);
	AJA_VIRTUAL bool		ReadSDBrightnessAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadSDContrastAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadSDSaturationAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadSDHueAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadSDCbOffsetAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadSDCrOffsetAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadHDProcAmpControlsInitialized (ULWord & outValue);
	AJA_VIRTUAL bool		ReadHDBrightnessAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadHDContrastAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadHDSaturationAdjustmentCb (ULWord & outValue);
	AJA_VIRTUAL bool		ReadHDSaturationAdjustmentCr (ULWord & outValue);
	AJA_VIRTUAL bool		ReadHDCbOffsetAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadHDCrOffsetAdjustment (ULWord & outValue);

	// FS1 (and other?) ProcAmp methods
	AJA_VIRTUAL bool		WriteProcAmpC1YAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteProcAmpC1CBAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteProcAmpC1CRAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteProcAmpC2CBAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteProcAmpC2CRAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		WriteProcAmpOffsetYAdjustment (const ULWord inNewValue);
	AJA_VIRTUAL bool		ReadProcAmpC1YAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadProcAmpC1CBAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadProcAmpC1CRAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadProcAmpC2CBAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadProcAmpC2CRAdjustment (ULWord & outValue);
	AJA_VIRTUAL bool		ReadProcAmpOffsetYAdjustment (ULWord & outValue);

	#if !defined(R2_DEPRECATE)
	AJA_VIRTUAL bool		SetAnalogInputADCMode (const NTV2LSVideoADCMode inValue);
	AJA_VIRTUAL bool		GetAnalogInputADCMode (NTV2LSVideoADCMode & outValue);
	#endif
	///@}


	/**
		@name	HDMI
	**/
	///@{
	/**
		@brief						Answers with the current colorspace for the given HDMI input.
		@param[out] outValue		Receives the HDMI input's current ::NTV2LHIHDMIColorSpace value.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetHDMIInputColor (NTV2LHIHDMIColorSpace & outValue,  const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Sets the given HDMI input's input range.
		@param[in]	inNewValue		Specifies the new ::NTV2HDMIRange value to be used.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		SetHDMIInputRange (const NTV2HDMIRange inNewValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the given HDMI input's current input range setting.
		@param[out] outValue		Receives the HDMI input's current ::NTV2HDMIRange value.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetHDMIInputRange (NTV2HDMIRange & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the current number of audio channels being received on the given HDMI input.
		@param[out] outValue		Receives the current ::NTV2HDMIAudioChannels value.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetHDMIInputAudioChannels (NTV2HDMIAudioChannels & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Sets the given HDMI input's color space.
		@param[in]	inNewValue		Specifies the new ::NTV2HDMIColorSpace value to be used.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		SetHDMIInColorSpace (const NTV2HDMIColorSpace inNewValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the given HDMI input's current color space setting.
		@param[out] outValue		Receives the HDMI input's current ::NTV2HDMIColorSpace value.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetHDMIInColorSpace (NTV2HDMIColorSpace & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the given HDMI input's protocol.
		@param[out] outValue		Receives the HDMI input's current ::NTV2HDMIProtocol value.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetHDMIInProtocol (NTV2HDMIProtocol & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers if the given HDMI input is genlocked or not.
		@param[out] outIsLocked		Receives \c true if the HDMI input is locked;  otherwise \c false.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetHDMIInIsLocked (bool & outIsLocked, const NTV2Channel inChannel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool		SetHDMIInAudioSampleRateConverterEnable (const bool inNewValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool		GetHDMIInAudioSampleRateConverterEnable (bool & outIsEnabled, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Sets the given HDMI input's bit depth.
		@param[in]	inNewValue		Specifies the new ::NTV2HDMIBitDepth value to be used.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		SetHDMIInBitDepth (const NTV2HDMIBitDepth inNewValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the given HDMI input's current bit depth setting.
		@param[out] outValue		Receives the HDMI input's current ::NTV2HDMIBitDepth value.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetHDMIInBitDepth (NTV2HDMIBitDepth & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the given HDMI input's current audio channel 3/4 swap setting.
		@param[out] outIsSwapped	Receives true if channels 3 & 4 are currently being swapped;  otherwise false.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
		@see						CNTV2Card::SetHDMIInAudioChannel34Swap
	**/
	AJA_VIRTUAL bool		GetHDMIInAudioChannel34Swap (bool & outIsSwapped, const NTV2Channel inChannel = NTV2_CHANNEL1); //	New in SDK v16.0

	/**
		@brief						Sets the given HDMI input's audio channel 3/4 swap state.
		@param[in]	inIsSwapped		Specify true to swap channels 3 & 4;  otherwise false.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
		@see						CNTV2Card::GetHDMIInAudioChannel34Swap
	**/
	AJA_VIRTUAL bool		SetHDMIInAudioChannel34Swap (const bool inIsSwapped, const NTV2Channel inChannel = NTV2_CHANNEL1);	//	New in SDK v16.0


	/**
		@brief						Answers with the given HDMI input's video black/white range.
		@param[out] outValue		Receives the HDMI input's current ::NTV2HDMIRange value.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetHDMIInVideoRange (NTV2HDMIRange & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the given HDMI input's video dynamic range and mastering information.
		@param[out] outRegValue		Receives the HDMI input's current HDRRegValues data.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 false if no information present.
	**/
	AJA_VIRTUAL bool		GetHDMIInDynamicRange (HDRRegValues & outRegValues, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the given HDMI input's video dynamic range and mastering information.
		@param[out] outFloatValue	Receives the HDMI input's current HDRFloatValues data.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 false if no information present.
	**/
	AJA_VIRTUAL bool		GetHDMIInDynamicRange (HDRFloatValues & outFloatValues, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief						Answers with the given HDMI input's current colorimetry.
		@param[out] outColorimetry	Receives the input channels colorimetry.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
		@see						CNTV2Card::SetHDMIInAudioChannel34Swap
	**/
	AJA_VIRTUAL bool		GetHDMIInColorimetry (NTV2HDMIColorimetry & outColorimetry, const NTV2Channel inChannel = NTV2_CHANNEL1);	//	New in SDK v16.0

	/**
		@brief						Answers with the given HDMI input's Dolby Vision flag is set.
		@param[out] outIsDolbyVision	Receives true if Dolby Vision input detected;  otherwise false.
		@param[in]	inChannel		Specifies the HDMI input of interest as an ::NTV2Channel (a zero-based index number). Defaults to NTV2_CHANNEL1.
		@return						True if successful;	 otherwise false.
		@see						CNTV2Card::SetHDMIInAudioChannel34Swap
	**/
	AJA_VIRTUAL bool		GetHDMIInDolbyVision (bool & outIsDolbyVision, const NTV2Channel inChannel = NTV2_CHANNEL1);	//	New in SDK v16.0

	AJA_VIRTUAL bool		SetHDMIOut3DPresent (const bool inIs3DPresent);
	AJA_VIRTUAL bool		GetHDMIOut3DPresent (bool & outIs3DPresent);

	AJA_VIRTUAL bool		SetHDMIOut3DMode (const NTV2HDMIOut3DMode inValue);
	AJA_VIRTUAL bool		GetHDMIOut3DMode (NTV2HDMIOut3DMode & outValue);

	AJA_VIRTUAL bool		SetHDMIV2TxBypass (const bool inBypass);

	AJA_VIRTUAL bool		SetHDMIOutVideoStandard (const NTV2Standard inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutVideoStandard (NTV2Standard & outValue);

	AJA_VIRTUAL bool		SetHDMIOutSampleStructure (const NTV2HDMISampleStructure inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutSampleStructure (NTV2HDMISampleStructure & outValue);

	AJA_VIRTUAL bool		SetHDMIOutVideoFPS (const NTV2FrameRate inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutVideoFPS (NTV2FrameRate & outValue);

	AJA_VIRTUAL bool		SetHDMIOutRange (const NTV2HDMIRange inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutRange (NTV2HDMIRange & outValue);

	AJA_VIRTUAL bool		SetHDMIOutAudioChannels (const NTV2HDMIAudioChannels inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutAudioChannels (NTV2HDMIAudioChannels & outValue);

	AJA_VIRTUAL bool		SetHDMIOutColorSpace (const NTV2HDMIColorSpace inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutColorSpace (NTV2HDMIColorSpace & outValue);
	AJA_VIRTUAL bool		SetLHIHDMIOutColorSpace (const NTV2LHIHDMIColorSpace inNewValue);
	AJA_VIRTUAL bool		GetLHIHDMIOutColorSpace (NTV2LHIHDMIColorSpace & outValue);

	AJA_VIRTUAL bool		SetHDMIOutBitDepth (const NTV2HDMIBitDepth inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutBitDepth (NTV2HDMIBitDepth & outValue);

	AJA_VIRTUAL bool		SetHDMIOutProtocol (const NTV2HDMIProtocol inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutProtocol (NTV2HDMIProtocol & outValue);

	AJA_VIRTUAL bool		SetHDMIOutForceConfig (const bool inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutForceConfig (bool & outValue);

	AJA_VIRTUAL bool		SetHDMIOutPrefer420 (const bool inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutPrefer420 (bool & outValue);

	AJA_VIRTUAL bool		GetHDMIOutDownstreamBitDepth (NTV2HDMIBitDepth & outValue);

	AJA_VIRTUAL bool		GetHDMIOutDownstreamColorSpace (NTV2LHIHDMIColorSpace & outValue);

	/**
		@brief						Sets the HDMI output's 2-channel audio source.
		@param[in]	inNewValue		Specifies the audio channels from the given Audio System to be used.
		@param[in]	inAudioSystem	Specifies the Audio System that will supply audio samples to the HDMI output. Defaults to NTV2_AUDIOSYSTEM_1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	SetHDMIOutAudioSource2Channel (const NTV2AudioChannelPair inNewValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief						Answers with the HDMI output's current 2-channel audio source.
		@param[out] outValue		Receives the audio channels that are currently being used.
		@param[out] outAudioSystem	Receives the Audio System that is currently supplying audio samples to the HDMI output.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	GetHDMIOutAudioSource2Channel (NTV2AudioChannelPair & outValue, NTV2AudioSystem & outAudioSystem);

	/**
		@brief						Changes the HDMI output's 8-channel audio source.
		@param[in]	inNewValue		Specifies the audio channels from the given Audio System to be used.
		@param[in]	inAudioSystem	Specifies the Audio System that will supply audio samples to the HDMI output. Defaults to NTV2_AUDIOSYSTEM_1.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	SetHDMIOutAudioSource8Channel (const NTV2Audio8ChannelSelect inNewValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief						Answers with the HDMI output's current 8-channel audio source.
		@param[out] outValue		Receives the audio channels that are currently being used.
		@param[out] outAudioSystem	Receives the Audio System that is currently supplying audio samples to the HDMI output.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	GetHDMIOutAudioSource8Channel (NTV2Audio8ChannelSelect & outValue, NTV2AudioSystem & outAudioSystem);

	/**
		@brief						Answers with the HDMI output's current audio channel 3/4 swap setting.
		@param[out] outIsSwapped	Receives true if channels 3 & 4 are currently being swapped;  otherwise false.
		@param[in]	inWhichHDMIOut	Optionally specifies the HDMI output of interest as an ::NTV2Channel, a zero-based index value.
									Defaults to the first HDMI output.
		@return						True if successful;	 otherwise false.
		@see						CNTV2Card::SetHDMIOutAudioChannel34Swap
	**/
	AJA_VIRTUAL bool	GetHDMIOutAudioChannel34Swap (bool & outIsSwapped, const NTV2Channel inWhichHDMIOut = NTV2_CHANNEL1);	//	New in SDK v16.0

	/**
		@brief						Sets the HDMI output's audio channel 3/4 swap state.
		@param[in]	inIsSwapped		Specify true to swap channels 3 & 4;  otherwise false.
		@param[in]	inWhichHDMIOut	Optionally specifies the HDMI output of interest as an ::NTV2Channel, a zero-based index value.
									Defaults to the first HDMI output.
		@return						True if successful;	 otherwise false.
		@see						CNTV2Card::GetHDMIOutAudioChannel34Swap
	**/
	AJA_VIRTUAL bool	SetHDMIOutAudioChannel34Swap (const bool inIsSwapped, const NTV2Channel inWhichHDMIOut = NTV2_CHANNEL1);	//	New in SDK v16.0

	/**
		@brief						Sets the HDMI output's audio rate
		@param[in]	inNewValue		Specifies the audio rate
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	SetHDMIOutAudioRate (const NTV2AudioRate inNewValue);

	/**
		@brief						Answers with the HDMI output's current audio rate.
		@param[out] outValue		Receives the HDMI output's current audio rate.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	GetHDMIOutAudioRate (NTV2AudioRate & outValue);

	/**
		@brief						Sets the HDMI output's audio format
		@param[in]	inNewValue		Specifies the audio format
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	SetHDMIOutAudioFormat (const NTV2AudioFormat inNewValue);

	/**
		@brief						Answers with the HDMI output's current audio format.
		@param[out] outValue		Receives the HDMI output's current audio format.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	GetHDMIOutAudioFormat (NTV2AudioFormat & outValue);

	/**
		@brief						Enables or disables override of HDMI parameters.
		@param[in]	inEnable		Specify true to enable HDMI user-override;	otherwise false to disable it.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	EnableHDMIOutUserOverride (const bool inEnable);

	/**
		@brief						Answers if override of HDMI parameters is enabled or not.
		@param[out] outIsEnabled	Receives true if enabled;  otherwise false.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	GetEnableHDMIOutUserOverride (bool & outIsEnabled);

	/**
		@brief						Controls the 4k/2k -> UHD/HD HDMI center cropping feature.
		@param[in]	inEnable		Specify true to enable center cropping;	 otherwise false to disable it.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	EnableHDMIOutCenterCrop (const bool inEnable);

	/**
		@brief						Answers if the HDMI 4k/2k -> UHD/HD center cropping is enabled or not.
		@param[out] outIsEnabled	Receives true if center cropping is enabled;  otherwise false.
		@return						True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool	GetEnableHDMIOutCenterCrop (bool & outIsEnabled);

	/**
		@brief		Enables or disables decimate mode on the device's HDMI rasterizer, which halves the
					output frame rate when enabled. This allows a 60 Hz video stream to be displayed on
					a 30 Hz HDMI monitor.
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, enables decimation mode; otherwise disables decimation mode.
	**/
	AJA_VIRTUAL bool		SetHDMIOutDecimateMode (const bool inEnable);

	AJA_VIRTUAL bool		GetHDMIOutDecimateMode (bool & outIsEnabled);

	/**
		@brief		Enables or disables two sample interleave I/O mode on the device's HDMI rasterizer.
		@return		True if successful; otherwise false.
		@param[in]	inTsiEnable		If true, enables two sample interleave I/O; otherwise disables two sample interleave I/O.
	**/
	AJA_VIRTUAL bool		SetHDMIOutTsiIO (const bool inTsiEnable);

	AJA_VIRTUAL bool		GetHDMIOutTsiIO (bool & tsiEnabled);

	/**
		@brief		Enables or disables level-B mode on the device's HDMI rasterizer.
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, enables level-B mode; otherwise disables level-B mode.
	**/
	AJA_VIRTUAL bool		SetHDMIOutLevelBMode (const bool inEnable);

	AJA_VIRTUAL bool		GetHDMIOutLevelBMode (bool & outIsEnabled);

	/**
		@brief		Sets HDMI V2 mode for the device.
		@return		True if successful; otherwise false.
		@param[in]	inMode	Specifies the HDMI V2 operation mode for the device.
							Use ::NTV2_HDMI_V2_HDSD_BIDIRECTIONAL for HD or SD capture and playback.
							Use ::NTV2_HDMI_V2_4K_CAPTURE for 4K capture.
							Use ::NTV2_HDMI_V2_4K_PLAYBACK for 4K playback.
		@note		4K modes are exclusive.
	**/
	AJA_VIRTUAL bool		SetHDMIV2Mode (const NTV2HDMIV2Mode inMode);

	/**
		@brief		Answers with the current HDMI V2 mode of the device.
		@return		True if successful; otherwise false.
		@param[out] outMode Receives the current HDMI V2 operation mode for the device.
	**/
	AJA_VIRTUAL bool		GetHDMIV2Mode (NTV2HDMIV2Mode & outMode);


	/**
		@brief		Answers with the current HDMI output status.
		@return		True if successful; otherwise false.
		@param[out] outStatus	Receives the current status of the HDMI output.
	**/
	AJA_VIRTUAL bool		GetHDMIOutStatus (NTV2HDMIOutputStatus & outStatus);	//	New in SDK 16.1

	//protected:	SHOULD BE PROTECTED/PRIVATE:
		AJA_VIRTUAL bool	GetHDMIInputStatus (ULWord & outValue,	const NTV2Channel inChannel = NTV2_CHANNEL1, const bool in12BitDetection = false);	///< @brief Answers with the contents of the HDMI Input status register for the given HDMI input.
	protected:
		AJA_VIRTUAL bool	GetHDMIInputStatusRegNum (ULWord & outRegNum,  const NTV2Channel inChannel = NTV2_CHANNEL1, const bool in12BitDetection = false);	///< @brief Answers with the HDMI Input status register number for the given HDMI input.
	///@}

	public:
	AJA_VIRTUAL bool		SetLHIVideoDACStandard (const NTV2Standard inValue);
	AJA_VIRTUAL bool		GetLHIVideoDACStandard (NTV2Standard & outValue);
	AJA_VIRTUAL bool		SetLHIVideoDACMode (NTV2LHIVideoDACMode value);
	AJA_VIRTUAL bool		GetLHIVideoDACMode (NTV2LHIVideoDACMode & outValue);
	AJA_VIRTUAL bool		SetLHIVideoDACMode (const NTV2VideoDACMode inValue);	// overloaded
	AJA_VIRTUAL bool		GetLHIVideoDACMode (NTV2VideoDACMode & outValue);	// overloaded

	/**
		@name	Analog LTC
	**/
	///@{
	/**
		@brief		Enables or disables the ability for the device to read analog LTC on the reference input connector.
		@param[in]	inEnable	If true, the device will read analog LTC from the reference input connector,
								and won't be able to genlock to external reference. If false, the device will
								expect a reference signal at the reference input connector, not analog LTC.
		@return		True if successful; otherwise false.
		@note		Not all devices are able to read analog LTC from their reference input.
					Call ::NTV2DeviceCanDoLTCInOnRefPort to find out.
		@see		GetLTCInputEnable, ::NTV2DeviceCanDoLTCInOnRefPort, \ref ancanalogltcinput
	**/
	AJA_VIRTUAL bool		SetLTCInputEnable (const bool inEnable);

	/**
		@brief		Answers true if the device is currently configured to read analog LTC from the reference
					input connector (instead of reference).
		@param[out]	outEnabled	Receives true if the device is set to read analog LTC from its reference connector,
								or false if it's configured to read reference.
		@return		True if successful; otherwise false.
		@note		Not all devices are able to read analog LTC from their reference input.
					Call ::NTV2DeviceCanDoLTCInOnRefPort to find out.
		@see		SetLTCInputEnable, ::NTV2DeviceCanDoLTCInOnRefPort, \ref ancanalogltcinput
	**/
	AJA_VIRTUAL bool		GetLTCInputEnable (bool & outIsEnabled);

	/**
		@brief		Answers whether or not a valid analog LTC signal is being applied to the device's analog LTC input connector.
		@param[out] outIsPresent	Receives 'true' if a valid analog LTC signal is present at the analog LTC input connector;
									otherwise 'false'.
		@param[in]	inLTCInputNdx	Optionally specifies the LTC input connector. Defaults to 0 (LTCIn1).
		@return		True if successful; otherwise false.
		@note		Some devices share analog LTC input and reference input on one connector.
					For these devices, this call should be preceded by a call to NTV2Card::SetLTCInputEnable(true).
		@see		ReadAnalogLTCInput, ::NTV2DeviceGetNumLTCInputs, \ref ancanalogltcinput
	**/
	AJA_VIRTUAL bool		GetLTCInputPresent (bool & outIsPresent, const UWord inLTCInputNdx = 0);

	/**
		@brief	Reads the current contents of the device's analog LTC input registers.
		@param[in]	inLTCInput		Specifies the device's analog LTC input to use. Use 0 for LTC In 1, or 1 for LTC In 2.
									(Call ::NTV2DeviceGetNumLTCInputs to determine the number of analog LTC inputs.)
		@param[out] outRP188Data	Receives the timecode read from the device registers.
		@return		True if successful; otherwise false.
		@note		The registers are read immediately, and should contain stable data if called soon after the VBI.
		@see		GetLTCInputPresent, ::NTV2DeviceGetNumLTCInputs, \ref ancanalogltcinput
	**/
	AJA_VIRTUAL bool		ReadAnalogLTCInput (const UWord inLTCInput, RP188_STRUCT & outRP188Data);

	/**
		@brief	Reads the current contents of the device's analog LTC input registers.
		@param[in]	inLTCInput		Specifies the device's analog LTC input to use. Use 0 for LTC In 1, or 1 for LTC In 2.
									(Call ::NTV2DeviceGetNumLTCInputs to determine the number of analog LTC inputs.)
		@param[out] outRP188Data	Receives the timecode read from the device registers.
		@return		True if successful; otherwise false.
		@note		The registers are read immediately, and should contain stable data if called soon after the VBI.
		@see		GetLTCInputPresent, ::NTV2DeviceGetNumLTCInputs, \ref ancanalogltcinput
	**/
	AJA_VIRTUAL bool		ReadAnalogLTCInput (const UWord inLTCInput, NTV2_RP188 & outRP188Data);

	/**
		@brief	Answers with the (SDI) input channel that's providing the clock reference being used by the given device's analog LTC input.
		@param[in]	inLTCInput		Specifies the device's analog LTC input. Use 0 for LTC In 1, or 1 for LTC In 2.
									(Call ::NTV2DeviceGetNumLTCInputs to determine the number of analog LTC inputs.)
		@param[out] outChannel		Receives the NTV2Channel that is currently providing the clock reference for reading the given analog LTC input.
		@return		True if successful; otherwise false.
		@note		This function is provided for devices that are capable of handling multiple, disparate video formats (see ::NTV2DeviceCanDoMultiFormat
					and CNTV2Card::GetMultiFormatMode functions). It doesn't make sense to call this function on a device that is running in "UniFormat" mode.
		@see		SetAnalogLTCInClockChannel, ::NTV2DeviceGetNumLTCInputs, \ref ancanalogltcinput
	**/
	AJA_VIRTUAL bool		GetAnalogLTCInClockChannel (const UWord inLTCInput, NTV2Channel & outChannel);

	/**
		@brief	Sets the (SDI) input channel that is to provide the clock reference to be used by the given analog LTC input.
		@param[in]	inLTCInput		Specifies the device's analog LTC input. Use 0 for LTC In 1, or 1 for LTC In 2.
									(Call ::NTV2DeviceGetNumLTCInputs to determine the number of analog LTC inputs.)
		@param[in]	inChannel		Specifies the NTV2Channel that should provide the clock reference for reading the given analog LTC input.
		@return		True if successful; otherwise false.
		@note		This function is provided for devices that are capable of handling multiple, disparate video formats (see ::NTV2DeviceCanDoMultiFormat
					and CNTV2Card::GetMultiFormatMode functions). It doesn't make sense to call this function on a device that is running in "UniFormat" mode.
		@see		GetAnalogLTCInClockChannel, ::NTV2DeviceGetNumLTCInputs, \ref ancanalogltcinput
	**/
	AJA_VIRTUAL bool		SetAnalogLTCInClockChannel (const UWord inLTCInput, const NTV2Channel inChannel);

	/**
		@brief	Writes the given timecode to the specified analog LTC output register.
		@param[in]	inLTCOutput		Specifies the device's analog LTC output to use. Use 0 for LTC Out 1, 1 for LTC Out 2, etc.
									(Call ::NTV2DeviceGetNumLTCOutputs to determine the number of analog LTC outputs.)
		@param[in]	inRP188Data		Specifies the timecode to write into the device registers.
									Only the "Low" and "High" fields are used -- the "DBB" field is ignored.
		@return		True if successful; otherwise false.
		@see		::NTV2DeviceGetNumLTCOutputs, \ref ancanalogltcoutput
	**/
	AJA_VIRTUAL bool		WriteAnalogLTCOutput (const UWord inLTCOutput, const RP188_STRUCT & inRP188Data);

	/**
		@brief	Writes the given timecode to the specified analog LTC output register.
		@param[in]	inLTCOutput		Specifies the device's analog LTC output to use. Use 0 for LTC Out 1, 1 for LTC Out 2, etc.
									(Call ::NTV2DeviceGetNumLTCOutputs to determine the number of analog LTC outputs.)
		@param[in]	inRP188Data		Specifies the timecode to write into the device registers.
									Only the "Low" and "High" fields are used -- the "DBB" field is ignored.
		@return		True if successful; otherwise false.
		@see		::NTV2DeviceGetNumLTCOutputs, \ref ancanalogltcoutput
	**/
	AJA_VIRTUAL bool		WriteAnalogLTCOutput (const UWord inLTCOutput, const NTV2_RP188 & inRP188Data);

	/**
		@brief	Answers with the (SDI) output channel that's providing the clock reference being used by the given device's analog LTC output.
		@param[in]	inLTCOutput		Specifies the device's analog LTC output. Use 0 for LTC Out 1, or 1 for LTC Out 2.
									(Call ::NTV2DeviceGetNumLTCOutputs to determine the number of analog LTC outputs.)
		@param[out] outChannel		Receives the NTV2Channel that is currently providing the clock reference for writing the given analog LTC output.
		@return		True if successful; otherwise false.
		@note		This function is provided for devices that are capable of handling multiple, disparate video formats (see ::NTV2DeviceCanDoMultiFormat
					and CNTV2Card::GetMultiFormatMode functions). It doesn't make sense to call this function on a device that is running in "UniFormat" mode.
		@see		SetAnalogLTCOutClockChannel, ::NTV2DeviceGetNumLTCOutputs, \ref ancanalogltcoutput
	**/
	AJA_VIRTUAL bool		GetAnalogLTCOutClockChannel (const UWord inLTCOutput, NTV2Channel & outChannel);

	/**
		@brief	Sets the (SDI) output channel that is to provide the clock reference to be used by the given analog LTC output.
		@param[in]	inLTCOutput		Specifies the device's analog LTC output. Use 0 for LTC Out 1, 1 for LTC Out 2, etc.
									(Call ::NTV2DeviceGetNumLTCOutputs to determine the number of analog LTC outputs.)
		@param[in]	inChannel		Specifies the NTV2Channel that should provide the clock reference for writing the given analog LTC output.
		@return		True if successful; otherwise false.
		@note		This function is provided for devices that are capable of handling multiple, disparate video formats (see ::NTV2DeviceCanDoMultiFormat
					and CNTV2Card::GetMultiFormatMode functions). It doesn't make sense to call this function on a device that is running in "UniFormat" mode.
		@see		GetAnalogLTCOutClockChannel, ::NTV2DeviceGetNumLTCOutputs, \ref ancanalogltcoutput
	**/
	AJA_VIRTUAL bool		SetAnalogLTCOutClockChannel (const UWord inLTCOutput, const NTV2Channel inChannel);

#if !defined(NTV2_DEPRECATE_16_3)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetLTCOnReference(bool val))		{return SetLTCInputEnable(val);}	///< @deprecated	Use SetLTCInputEnable instead. First deprecated in SDK 16.3.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetLTCOnReference(bool & outVal))	{return GetLTCInputEnable(outVal);}	///< @deprecated	Use GetLTCInputEnable instead. First deprecated in SDK 16.3.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool SetLTCEmbeddedOutEnable (const bool inNewValue));	///< @deprecated	Use GetLTCInputEnable instead. First deprecated in SDK 16.3.
	AJA_VIRTUAL NTV2_DEPRECATED_f(bool GetLTCEmbeddedOutEnable (bool & outValue));			///< @deprecated	Obsolete, do not use. First deprecated in SDK 16.3.
#endif	//	!defined(NTV2_DEPRECATE_16_3)
	///@}

	/**
		@name	Stereo Compression
	**/
	///@{
	AJA_VIRTUAL bool		SetStereoCompressorOutputMode (const NTV2StereoCompressorOutputMode inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorOutputMode (NTV2StereoCompressorOutputMode & outValue);
	AJA_VIRTUAL bool		SetStereoCompressorFlipMode (const ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipMode (ULWord & outValue);
	AJA_VIRTUAL bool		SetStereoCompressorFlipLeftHorz (const ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipLeftHorz (ULWord & outValue);
	AJA_VIRTUAL bool		SetStereoCompressorFlipLeftVert (const ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipLeftVert (ULWord & outValue);
	AJA_VIRTUAL bool		SetStereoCompressorFlipRightHorz (const ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipRightHorz (ULWord & outValue);
	AJA_VIRTUAL bool		SetStereoCompressorFlipRightVert (const ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipRightVert (ULWord & outValue);
	AJA_VIRTUAL bool		SetStereoCompressorStandard (const NTV2Standard inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorStandard (NTV2Standard & outValue);
	AJA_VIRTUAL bool		SetStereoCompressorLeftSource (const NTV2OutputCrosspointID inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorLeftSource (NTV2OutputCrosspointID & outValue);
	AJA_VIRTUAL bool		SetStereoCompressorRightSource (const NTV2OutputCrosspointID inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorRightSource (NTV2OutputCrosspointID & outValue);
	///@}

	/**
		@name	Bi-directional SDI
	**/
	///@{
	/**
		@brief		Sets the specified bidirectional SDI connector to act as an input or an output.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the SDI connector as an ::NTV2Channel (a zero-based index number).
		@param[in]	inEnable		If true, specifies that the channel connector is to be used as an output.
									If false, specifies it's to be used as an input.
		@note		After switching a bidirectional SDI connector from output to input (i.e., inEnable = false),
					it may take the hardware on the device up to approximately 10 frames to synchronize with
					the input signal such that the device can accurately report the video format seen at the
					input.
		@see		::NTV2DeviceHasBiDirectionalSDI, \ref devicesignalinputsoutputs
	**/
	AJA_VIRTUAL bool		SetSDITransmitEnable (const NTV2Channel inChannel, const bool inEnable);

	/**
		@brief		Sets the specified SDI connector(s) to act as an input or an output.
		@return		True if successful; otherwise false.
		@param[in]	inSDIConnectors Specifies the SDI connector(s) to be affected.
		@param[in]	inEnable		If true, specifies that the channel connector is to be used as an output.
									If false, specifies it's to be used as an input.
		@note		After switching a bidirectional SDI connector from output to input (i.e., inEnable = false),
					it may take the hardware on the device up to approximately 10 frames to synchronize with
					the input signal such that the device can accurately report the video format seen at the
					input.
		@see		::NTV2DeviceHasBiDirectionalSDI, \ref devicesignalinputsoutputs
	**/
	AJA_VIRTUAL bool		SetSDITransmitEnable (const NTV2ChannelSet & inSDIConnectors, const bool inEnable);

	/**
		@brief		Answers whether or not the specified SDI connector is currently acting as a transmitter
					(i.e. an output).
		@param[in]	inChannel		Specifies the SDI connector as an ::NTV2Channel (a zero-based index number).
		@param[out] outEnabled		Receives true if the SDI channel connector is currently a transmitter (output),
									or false if it's acting as an input.
		@return		True if successful; otherwise false.
		@see		::NTV2DeviceHasBiDirectionalSDI, \ref devicesignalinputsoutputs
	**/
	AJA_VIRTUAL bool		GetSDITransmitEnable (const NTV2Channel inChannel, bool & outEnabled);

	/**
		@brief		Answers with the transmitting/output SDI connectors.
		@param[out] outXmitSDIs		Receives the set of transmitting SDI connectors.
		@return		True if successful; otherwise false.
		@note		This function returns valid results for devices with or without bi-directional SDIs.
		@note		This function does not take into account an SDI output that's designated as a "monitor"
					(i.e. ::NTV2WidgetType_SDIMonOut or ::NTV2_WgtSDIMonOut1).
		@see		::NTV2DeviceHasBiDirectionalSDI, \ref devicesignalinputsoutputs
	**/
	AJA_VIRTUAL bool		GetTransmitSDIs (NTV2ChannelSet & outXmitSDIs);	//	New in SDK 17.0
	///@}

	AJA_VIRTUAL bool		SetSDIOut2Kx1080Enable (const NTV2Channel inChannel, const bool inIsEnabled);
	AJA_VIRTUAL bool		GetSDIOut2Kx1080Enable (const NTV2Channel inChannel, bool & outIsEnabled);

	AJA_VIRTUAL bool		SetSDIOut3GEnable (const NTV2Channel inChannel,const bool inEnable);
	AJA_VIRTUAL bool		GetSDIOut3GEnable (const NTV2Channel inChannel, bool & outIsEnabled);

	AJA_VIRTUAL bool		SetSDIOut3GbEnable (const NTV2Channel inChannel, const bool inEnable);
	AJA_VIRTUAL bool		GetSDIOut3GbEnable (const NTV2Channel inChannel, bool & outIsEnabled);

	AJA_VIRTUAL bool		SetSDIOut6GEnable(const NTV2Channel inChannel, const bool inEnable);
	AJA_VIRTUAL bool		GetSDIOut6GEnable(const NTV2Channel inChannel, bool & outIsEnabled);

	AJA_VIRTUAL bool		SetSDIOut12GEnable(const NTV2Channel inChannel, const bool inEnable);
	AJA_VIRTUAL bool		GetSDIOut12GEnable(const NTV2Channel inChannel, bool & outIsEnabled);


	/**
		@name	SDI Bypass Relays
	**/
	///@{
	/**
		@brief		Answers if the bypass relays between connectors 1/2 or 3/4 are currently
					in bypass or routing the signals through the device.
		@return		True if successful; otherwise false.
		@param[out] outValue	Receives the current state of the relays (::NTV2_DEVICE_BYPASSED
								or ::NTV2_THROUGH_DEVICE).
		@param[in]	inIndex0	Specifies the relay/connector pair of interest.
								Use 0 for SDI 1&2, or 1 for SDI 3&4.
	**/
	AJA_VIRTUAL bool	GetSDIRelayPosition (NTV2RelayState & outValue, const UWord inIndex0);

	/**
		@brief		Answers if the bypass relays between connectors 1 and 2 would be in
					bypass or would route signals through the device, if under manual control.
		@param[out] outValue	Receives the relay state (::NTV2_DEVICE_BYPASSED or ::NTV2_THROUGH_DEVICE).
		@param[in]	inIndex0	Specifies the relay/connector pair of interest.
								Use 0 for SDI 1&2, or 1 for SDI 3&4.
		@return		True if successful; otherwise false.
		@note		Manual control will not change the state of the relays if
					the watchdog timer for the relays is enabled.
	**/
	AJA_VIRTUAL bool	GetSDIRelayManualControl (NTV2RelayState & outValue, const UWord inIndex0);

	/**
		@brief		Sets the state of the given connector pair relays to ::NTV2_DEVICE_BYPASSED
					(or ::NTV2_THROUGH_DEVICE if under manual control).
		@param[in]	inValue		Specifies the desired relay state.
		@param[in]	inIndex0	Specifies the relay/connector pair of interest.
								Use 0 for SDI 1&2, or 1 for SDI 3&4.
		@return		True if successful; otherwise false.
		@note		Manual control won't change the state of the relays if the watchdog timer for the relays
					is enabled. Because this call modifies the control register, it sends a "kick" sequence,
					which has the side effect of restarting the timeout counter.
	**/
	AJA_VIRTUAL bool	SetSDIRelayManualControl (const NTV2RelayState inValue, const UWord inIndex0);

	/**
		@brief		Answers true if the given connector pair relays are under watchdog timer control,
					or false if they're under manual control.
		@param[out] outIsEnabled	Receives 'true' if the watchdog timer is controlling the relays;
									receives 'false' if the relays are under manual control.
		@param[in]	inIndex0	Specifies the relay/connector pair of interest.
								Use 0 for SDI 1&2, or 1 for SDI 3&4.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetSDIWatchdogEnable (bool & outIsEnabled, const UWord inIndex0);

	/**
		@brief		Sets the connector pair relays to be under watchdog timer control or manual control.
		@param[in]	inEnable	Specify true to have the watchdog timer control the relays.
								Specify false to manually control the relays.
		@param[in]	inIndex0	Specifies the relay/connector pair of interest.
								Use 0 for SDI 1&2, or 1 for SDI 3&4.
		@return		True if successful; otherwise false.
		@note		Because this call modifies the control register, it sends a kick sequence,
					which restarts the timeout counter.
		@note		If the board's jumpers aren't set in the position to enable the watchdog timer,
					this call will have no effect. See the <b>Hardware Jumpers</b> section in the
					\ref corvid24 documentation for more information.
	**/
	AJA_VIRTUAL bool	SetSDIWatchdogEnable (const bool inEnable, const UWord inIndex0);

	/**
		@brief		Answers if the watchdog timer would put the SDI relays into ::NTV2_DEVICE_BYPASSED
					or ::NTV2_THROUGH_DEVICE.
		@param[out] outValue	Receives the current state of the watchdog timer.
		@return		True if successful; otherwise false.
		@note		The watchdog timer won't change the state of the relays if they're under manual control.
	**/
	AJA_VIRTUAL bool	GetSDIWatchdogStatus (NTV2RelayState & outValue);

	/**
		@brief		Answers with the amount of time that must elapse before the watchdog timer times out.
		@param[out] outValue	Receives the time value in units of 8 nanoseconds.
		@return		True if successful; otherwise false.
		@note		The timeout interval begins or is reset when a kick sequence is received.
	**/
	AJA_VIRTUAL bool	GetSDIWatchdogTimeout (ULWord & outValue);

	/**
		@brief		Specifies the amount of time that must elapse before the watchdog timer times out.
		@param[in]	inValue		Specifies the timeout interval in units of 8 nanoseconds.
		@return		True if successful; otherwise false.
		@note		The timeout interval begins or is reset when a kick sequence is received.
					This call resets the timeout counter to zero, which will then start counting
					until this value is reached, triggering the watchdog timer (if it's enabled).
	**/
	AJA_VIRTUAL bool	SetSDIWatchdogTimeout (const ULWord inValue);

	/**
		@brief		Restarts the countdown timer to prevent the watchdog timer from timing out.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	KickSDIWatchdog (void);
	///@}

	/**
		@name	4K Conversion
	**/
	///@{
	/**
		@brief		Sets 4K Down Convert RGB mode
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, specifies RGB mode
									If false, specifies YCC mode
	**/
	AJA_VIRTUAL bool		Enable4KDCRGBMode(bool inEnable);

	AJA_VIRTUAL bool		GetEnable4KDCRGBMode(bool & outIsEnabled);

	/**
		@brief		Sets 4K Down Convert YCC 444 mode
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, specifies YCC 444
									If false, specifies YCC 422
	**/
	AJA_VIRTUAL bool		Enable4KDCYCC444Mode(bool inEnable);

	AJA_VIRTUAL bool		GetEnable4KDCYCC444Mode(bool & outIsEnabled);

	/**
		@brief		Sets 4K Down Convert PSF in mode
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, specifies PSF in
									If false, specifies P in
	**/
	AJA_VIRTUAL bool		Enable4KDCPSFInMode(bool inEnable);

	AJA_VIRTUAL bool		GetEnable4KDCPSFInMode(bool & outIsEnabled);

	/**
		@brief		Sets 4K Down Convert PSF out Mode
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, specifies PSF out
									If false, specifies P out
	**/
	AJA_VIRTUAL bool		Enable4KPSFOutMode(bool inEnable);

	AJA_VIRTUAL bool		GetEnable4KPSFOutMode(bool & outIsEnabled);
	///@}


	/**
		@brief		Enables or disables 3G level B to 3G level A conversion at the SDI input(s).
		@return		True if successful; otherwise false.
		@param[in]	inSDIInputs		Specifies the SDI input(s) to be affected, each specified by an ::NTV2Channel,
									a zero-based index (where 0 == SDIIn1, 1 == SDIIn2, etc.).
		@param[in]	inEnable		Specify true to automatically convert incoming 3g level B signals to 3g level A.
									Specify false for normal operation.
	**/
	AJA_VIRTUAL bool		SetSDIInLevelBtoLevelAConversion (const NTV2ChannelSet & inSDIInputs, const bool inEnable);

	/**
		@brief		Enables or disables 3G level B to 3G level A conversion at the SDI input widget (assuming the AJA device can do so).
		@return		True if successful; otherwise false.
		@param[in]	inInputSpigot	Specifies the SDI input spigot to be affected (where 0 == SDIIn1, 1 == SDIIn2, etc.).
		@param[in]	inEnable		Specify true to automatically convert incoming 3g level B signals to 3g level A.
									Specify false for normal operation.
	**/
	AJA_VIRTUAL bool		SetSDIInLevelBtoLevelAConversion (const UWord inInputSpigot, const bool inEnable);

	/**
		@brief		Answers with the device's current 3G level B to 3G level A conversion setting for the given SDI input spigot (assuming the device can do such a conversion).
		@return		True if successful; otherwise false.
		@param[in]	inInputSpigot	Specifies the SDI input widget of interest (0 == SDIIn1, 1 == SDIIn2, etc.).
		@param[out] outIsEnabled	Receives true if enabled, or false if disabled (normal operation).
	**/
	AJA_VIRTUAL bool		GetSDIInLevelBtoLevelAConversion (const UWord inInputSpigot, bool & outIsEnabled);

	/**
		@brief		Enables or disables 3G level A to 3G level B conversion at the SDI output widget (assuming the AJA device can do so).
		@return		True if successful; otherwise false.
		@param[in]	inOutputSpigot	Specifies the SDI output widget to be affected (where 0 == SDIOut1, 1 == SDIOut2, etc.).
		@param[in]	inEnable		If true, outgoing 3g level a signal converted to 3g level b signal at SDI output widget.
									If false, specifies normal operation.
	**/
	AJA_VIRTUAL bool		SetSDIOutLevelAtoLevelBConversion (const UWord inOutputSpigot, const bool inEnable);

	/**
		@brief		Enables or disables 3G level A to 3G level B conversion at the given SDI output(s).
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutputs	Specifies the SDI output(s) to be affected, each specified by an ::NTV2Channel,
									a zero-based index (where 0 == SDIOut1, 1 == SDIOut2, etc.).
		@param[in]	inEnable		If true, outgoing 3G level A signal converted to 3G level B signal at SDI output widget.
									If false, specifies normal operation.
	**/
	AJA_VIRTUAL bool		SetSDIOutLevelAtoLevelBConversion (const NTV2ChannelSet & inSDIOutputs, const bool inEnable);	//	New in SDK v16.0

	/**
		@brief		Answers with the device's current 3G level A to 3G level B conversion setting for the given SDI output spigot (assuming the device can do such a conversion).
		@return		True if successful; otherwise false.
		@param[in]	inOutputSpigot	Specifies the SDI output widget of interest (0 == SDIOut1, 1 == SDIOut2, etc.).
		@param[out] outIsEnabled	Receives true if enabled, or false if disabled (normal operation).
	**/
	AJA_VIRTUAL bool		GetSDIOutLevelAtoLevelBConversion (const UWord inOutputSpigot, bool & outIsEnabled);

	/**
		@brief		Enables or disables an RGB-over-3G-level-A conversion at the SDI output widget (assuming the AJA device can do so).
		@return		True if successful; otherwise false.
		@param[in]	inOutputSpigot	Specifies the SDI output to be affected (where 0 == SDIOut1, 1 == SDIOut2, etc.).
		@param[in]	inEnable		If true, perform the conversion at the output SDI spigot;  otherwise have the SDI output spigot operate normally (no conversion).
	**/
	AJA_VIRTUAL bool		SetSDIOutRGBLevelAConversion (const UWord inOutputSpigot, const bool inEnable);

	/**
		@brief		Enables or disables an RGB-over-3G-level-A conversion at the given SDI output(s).
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutputs	Specifies the SDI output(s) to be affected, each specified by an ::NTV2Channel,
									a zero-based index (where 0 == SDIOut1, 1 == SDIOut2, etc.).
		@param[in]	inEnable		If true, perform the conversion at the output SDI spigot;
									otherwise have the SDI output spigot operate normally (no conversion).
	**/
	AJA_VIRTUAL bool		SetSDIOutRGBLevelAConversion (const NTV2ChannelSet & inSDIOutputs, const bool inEnable);	//	New in SDK v16.0

	/**
		@brief		Answers with the device's current RGB-over-3G-level-A conversion at the given SDI output spigot (assuming the device can do such a conversion).
		@return		True if successful; otherwise false.
		@param[in]	inOutputSpigot	Specifies the SDI output spigot of interest (where 0 is "SDI Out 1").
		@param[out] outIsEnabled	Receives true if the device's output spigot is currently performing the conversion;	 otherwise false (not converting).
	**/
	AJA_VIRTUAL bool		GetSDIOutRGBLevelAConversion (const UWord inOutputSpigot, bool & outIsEnabled);


	/**
		@name	SDI Input Error Detection
	**/
	///@{
	/**
		@return		True if the given SDI input is currently reporting TRS errors;	otherwise false.
		@param[in]	inChannel		Specifies the channel (SDI input) of interest.
	**/
	AJA_VIRTUAL bool		GetSDITRSError (const NTV2Channel inChannel);

	/**
		@return		True if the given SDI input is currently locked to a valid signal;	otherwise false.
		@param[in]	inChannel		Specifies the channel (SDI input) of interest.
		@note		This function returns valid information only for devices for which ::NTV2DeviceCanDoSDIErrorChecks returns 'true'.
	**/
	AJA_VIRTUAL bool		GetSDILock (const NTV2Channel inChannel);

	/**
		@return		The number of transitions to the "unlocked" state for the given SDI input.
		@param[in]	inChannel		Specifies the channel (SDI input) of interest.
		@note		This function returns valid information only for devices for which ::NTV2DeviceCanDoSDIErrorChecks returns 'true'.
	**/
	AJA_VIRTUAL ULWord		GetSDIUnlockCount (const NTV2Channel inChannel);

	/**
		@return		The current Link A error count for the given SDI input.
		@param[in]	inChannel		Specifies the channel (SDI input) of interest.
		@note		This function returns valid information only for devices for which ::NTV2DeviceCanDoSDIErrorChecks returns 'true'.
	**/
	AJA_VIRTUAL ULWord		GetCRCErrorCountA (const NTV2Channel inChannel);

	/**
		@return		The current Link B error count for the given SDI input.
		@param[in]	inChannel		Specifies the channel (SDI input) of interest.
		@note		This function returns valid information only for devices for which ::NTV2DeviceCanDoSDIErrorChecks returns 'true'.
	**/
	AJA_VIRTUAL ULWord		GetCRCErrorCountB (const NTV2Channel inChannel);
	///@}

	/**
		@brief		Enables or disables multi-format (per channel) device operation.
					If enabled, each device channel can handle a different video format (subject to certain limitations).
					If disabled, all device channels have the same video format.
		@return		True if successful; otherwise false.
		@param[in]	inEnable	If true, sets the device in multi-format mode.
								If false, sets the device in uni-format mode.
		@deprecated	Uniformat mode is deprecated starting in SDK 17.0.
					Passing false for inEnable will cause this function to return false (fail).
					Eventually this function will be removed from the class.
		@see		::NTV2DeviceCanDoMultiFormat, CNTV2Card::GetMultiFormatMode, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool	   SetMultiFormatMode (const bool inEnable);

	/**
		@brief		Answers if the device is operating in multiple-format per channel (independent channel) mode or not.
					If enabled, each device channel can accommodate a different video format (subject to certain limitations).
					If disabled, all device channels have the same video format.
		@return		True if successful; otherwise false.
		@param[out] outIsEnabled	Receives true if the device is currently in multi-format mode,
									or false if it's in uni-format mode.
		@deprecated	Uniformat mode is deprecated starting in SDK 17.0.
					Eventually this function will be removed from the class.
		@see		::NTV2DeviceCanDoMultiFormat, CNTV2Card::SetMultiFormatMode, \ref deviceclockingandsync
	**/
	AJA_VIRTUAL bool		GetMultiFormatMode (bool & outIsEnabled);


public:
	/**
		@name	RS-422
	**/
	///@{
	/**
		@brief		Sets the parity control on the specified RS422 serial port.
		@return		True if successful; otherwise false.
		@param[in]	inSerialPort	Specifies the RS422 serial port to be affected, expressed as an ::NTV2Channel
									(a zero-based index number). Call ::NTV2DeviceGetNumSerialPorts to determine
									the number of available serial ports.
		@param[in]	inParity		Specifies if parity should be used, and if so, whether it should be odd or even.
		@note		This function only works with programmable RS422 devices.
		@see		CNTV2Card::GetRS422Parity, CNTV2Card::SetRS422BaudRate, ::NTV2DeviceGetNumSerialPorts,
					::NTV2DeviceCanDoProgrammableRS422
	**/
	AJA_VIRTUAL bool		SetRS422Parity	(const NTV2Channel inSerialPort, const NTV2_RS422_PARITY inParity);

	/**
		@brief		Answers with the current parity control for the specified RS422 serial port.
		@return		True if successful; otherwise false.
		@param[in]	inSerialPort	Specifies the RS422 serial port of interest, expressed as an ::NTV2Channel
									(a zero-based index number). Call ::NTV2DeviceGetNumSerialPorts to determine
									the number of available serial ports.
		@param[out] outParity		Receives the serial port's current ::NTV2_RS422_PARITY setting.
		@see		CNTV2Card::SetRS422Parity, CNTV2Card::GetRS422BaudRate, ::NTV2DeviceGetNumSerialPorts,
					::NTV2DeviceCanDoProgrammableRS422
	**/
	AJA_VIRTUAL bool		GetRS422Parity (const NTV2Channel inSerialPort, NTV2_RS422_PARITY & outParity);

	/**
		@brief		Sets the baud rate of the specified RS422 serial port.
		@return		True if successful; otherwise false.
		@param[in]	inSerialPort	Specifies the RS422 serial port to be affected, expressed as an ::NTV2Channel
									(a zero-based index number). Call ::NTV2DeviceGetNumSerialPorts to determine
									the number of available serial ports.
		@param[in]	inBaudRate		Specifies the ::NTV2_RS422_BAUD_RATE to be used. (The default baud rate upon
									power-up or reset is ::NTV2_RS422_BAUD_RATE_38400.)
		@note		This function only works with programmable RS422 devices.
		@see		CNTV2Card::SetRS422Parity, CNTV2Card::GetRS422BaudRate, ::NTV2DeviceGetNumSerialPorts,
					::NTV2DeviceCanDoProgrammableRS422
	**/
	AJA_VIRTUAL bool		SetRS422BaudRate  (const NTV2Channel inSerialPort, const NTV2_RS422_BAUD_RATE inBaudRate);

	/**
		@brief		Answers with the current baud rate of the specified RS422 serial port.
		@return		True if successful; otherwise false.
		@param[in]	inSerialPort	Specifies the RS422 serial port of interest, expressed as an ::NTV2Channel
									(a zero-based index number). Call ::NTV2DeviceGetNumSerialPorts to determine
									the number of available serial ports.
		@param[out] outBaudRate		Receives the ::NTV2_RS422_BAUD_RATE being used.
		@see		CNTV2Card::GetRS422Parity, CNTV2Card::SetRS422BaudRate, ::NTV2DeviceGetNumSerialPorts,
					::NTV2DeviceCanDoProgrammableRS422
	**/
	AJA_VIRTUAL bool		GetRS422BaudRate (const NTV2Channel inSerialPort, NTV2_RS422_BAUD_RATE & outBaudRate);
	///@}

	/**
		@name	Ancillary Data
	**/
	///@{

	/**
		@brief		Sets the capacity of the ANC buffers in device frame memory.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inF1Size		Specifies the capacity of the Field 1 anc buffer, in bytes.
		@param[in]	inF2Size		Specifies the capacity of the Field 2 anc buffer, in bytes.
		@note		This function should be used before configuring the anc extractors/inserters.
		@note		Size changes apply to all anc extractors/inserters.
		@warning	Setting these values too large will result in anc data occupying the bottom of the video raster.
		@see		CNTV2Card::GetAncRegionOffsetAndSize, \ref anccapture-dataspace
	**/
	AJA_VIRTUAL bool	AncSetFrameBufferSize (const ULWord inF1Size, const ULWord inF2Size);	//	New in SDK 16.0


	/**
		@brief		Initializes the given SDI output's Anc inserter for custom Anc packet insertion.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutput		Specifies the SDI output of interest (e.g., 0=SDIOut1, 1=SDIOut2, etc.).
		@param[in]	inChannel		Optionally specifies the ::NTV2Channel (FrameStore) that's driving the SDI output,
									if different from the SDI output. The default is to use the same ::NTV2Channel
									that corresponds to the given SDI output (e.g., ::NTV2_CHANNEL1 == 0 == SDIOut1).
		@param[in]	inStandard		Optionally overrides the ::NTV2Standard used to initialize the Anc inserter.
									Defaults to using the ::NTV2Standard of the ::NTV2Channel being used.
		@note		This function is provided for playback methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
	**/
	AJA_VIRTUAL bool	AncInsertInit (const UWord inSDIOutput, const NTV2Channel inChannel = NTV2_CHANNEL_INVALID,
										const NTV2Standard inStandard = NTV2_STANDARD_INVALID);	//	New in SDK 15.0

	/**
		@brief		Enables or disables individual Anc insertion components for the given SDI output.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutput		Specifies the SDI output of interest (e.g., 0=SDIOut1, 1=SDIOut2, etc.).
		@param[in]	inVancY			Specify true to enable Vanc Y component insertion;	otherwise false to disable it.
		@param[in]	inVancC			Specify true to enable Vanc C component insertion;	otherwise false to disable it.
		@param[in]	inHancY			Specify true to enable Hanc Y component insertion;	otherwise false to disable it.
		@param[in]	inHancC			Specify true to enable Hanc C component insertion;	otherwise false to disable it.
		@note		This function is provided for playback methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
	**/
	AJA_VIRTUAL bool	AncInsertSetComponents (const UWord inSDIOutput,
												const bool inVancY, const bool inVancC,
												const bool inHancY, const bool inHancC);	//	New in SDK 16.0

	/**
		@brief		Enables or disables the given SDI output's Anc inserter frame buffer reads.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutput		Specifies the SDI output of interest (e.g., 0=SDIOut1, 1=SDIOut2, etc.).
		@param[in]	inIsEnabled		Specify true to enable the Anc inserter;  otherwise false to disable it.
		@note		This function is provided for playback methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
	**/
	AJA_VIRTUAL bool	AncInsertSetEnable (const UWord inSDIOutput, const bool inIsEnabled);	//	New in SDK 15.0

	/**
		@brief		Answers with the run state of the given Anc inserter -- i.e. if its "memory reader" is enabled or not.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutput		Specifies the SDI output of interest as a zero-based index value (e.g., 0 == SDIOut1).
		@param[out] outIsEnabled	Receives 'true' if the Anc inserter is enabled (running);  otherwise false.
	**/
	AJA_VIRTUAL bool	AncInsertIsEnabled (const UWord inSDIOutput, bool & outIsEnabled);	//	New in SDK 15.0

	/**
		@brief		Configures the Anc inserter for the next frame's F1 Anc data to embed/transmit.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutput		Specifies the SDI output of interest (e.g., 0=SDIOut1, 1=SDIOut2, etc.).
		@param[in]	inFrameNumber	Tells the Anc inserter where to find the Anc data to transmit, specified as a
									zero-based frame number.
		@param[in]	inF1Size		Specifies the maximum number of F1 bytes to process in the Anc data buffer in the frame.
		@param[in]	inChannel		Optionally specifies the ::NTV2Channel (FrameStore) that's driving the SDI output,
									if different from the SDI output. The default is to use the same ::NTV2Channel
									that corresponds to the given SDI output (e.g., ::NTV2_CHANNEL1 == 0 == SDIOut1).
		@param[in]	inFrameSize		Optionally overrides the ::NTV2Framesize used to calculate the Anc buffer location
									in device SDRAM. Defaults to using the ::NTV2Framesize of the ::NTV2Channel being used.
		@note		This function is provided for playback methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
	**/
	AJA_VIRTUAL bool	AncInsertSetReadParams (const UWord inSDIOutput, const ULWord inFrameNumber, const ULWord inF1Size,
												const NTV2Channel inChannel = NTV2_CHANNEL_INVALID,
												const NTV2Framesize inFrameSize = NTV2_FRAMESIZE_INVALID);	//	New in SDK 15.0

	/**
		@brief		Configures the Anc inserter for the next frame's F2 Anc data to embed/transmit.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutput		Specifies the SDI output of interest (e.g., 0=SDIOut1, 1=SDIOut2, etc.).
		@param[in]	inFrameNumber	Tells the Anc inserter where to find the Anc data to transmit, specified as a
									zero-based frame number.
		@param[in]	inF2Size		Specifies the maximum number of F2 bytes to process in the Anc data buffer in the frame.
		@param[in]	inChannel		Optionally specifies the ::NTV2Channel (FrameStore) that's driving the SDI output,
									if different from the SDI output. The default is to use the same ::NTV2Channel
									that corresponds to the given SDI output (e.g., ::NTV2_CHANNEL1 == 0 == SDIOut1).
		@param[in]	inFrameSize		Optionally overrides the ::NTV2Framesize used to calculate the Anc buffer location
									in device SDRAM. Defaults to using the ::NTV2Framesize of the ::NTV2Channel being used.
		@note		This function is provided for playback methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
	**/
	AJA_VIRTUAL bool	AncInsertSetField2ReadParams (const UWord inSDIOutput, const ULWord inFrameNumber, const ULWord inF2Size,
														const NTV2Channel inChannel = NTV2_CHANNEL_INVALID,
														const NTV2Framesize inFrameSize = NTV2_FRAMESIZE_INVALID);	//	New in SDK 15.0

	/**
		@brief		Configures the Anc inserter IP specific params.
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutput		Specifies the SDI output of interest (e.g., 0=SDIOut1, 1=SDIOut2, etc.).
		@param[in]	ancChannel		Tells the IP packetizer which Anc inserter to use (4-7).
		@param[in]	payloadID		Tells the IP packetizer what the RTP Payload Id is.
		@param[in]	ssrc			Tells the IP packetizer what the RTP SSRC is.
	**/
	AJA_VIRTUAL bool	AncInsertSetIPParams (const UWord inSDIOutput, const UWord ancChannel, const ULWord payloadID, const ULWord ssrc);	//	New in SDK 15.2

	/**
		@brief		Answers where, in device SDRAM, the given SDI connector's Anc inserter is currently reading Anc data for playout.
		@return		True if successful; otherwise false.
		@param[in]	inSDIOutput		Specifies the SDI output of interest (e.g., 0=SDIOut1, 1=SDIOut2, etc.).
		@param[out]	outF1StartAddr	Receives the Anc inserter's current F1 starting address.
		@param[out]	outF2StartAddr	Receives the Anc inserter's current F2 starting address.
	**/
	AJA_VIRTUAL bool	AncInsertGetReadInfo (const UWord inSDIOutput, uint64_t & outF1StartAddr, uint64_t & outF2StartAddr);	//	New in SDK v16.2


	/**
		@brief		Initializes the given SDI input's Anc extractor for custom Anc packet detection and de-embedding.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[in]	inChannel		Optionally specifies the ::NTV2Channel (FrameStore) that's fed from the SDI input,
									if different from the SDI input. The default is to use the same ::NTV2Channel
									that corresponds to the given SDI input (e.g., ::NTV2_CHANNEL1 == 0 == SDIIn1).
		@param[in]	inStandard		Optionally overrides the ::NTV2Standard used to initialize the Anc extractor.
									Defaults to using the ::NTV2Standard of the ::NTV2Channel being used.
		@note		This function is provided for capture methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
		@see		CNTV2Card::AncExtractSetEnable, CNTV2Card::AncExtractSetWriteParams,
					CNTV2Card::AncExtractSetFilterDIDs, \ref anccapture
	**/
	AJA_VIRTUAL bool	AncExtractInit (const UWord inSDIInput, const NTV2Channel inChannel = NTV2_CHANNEL_INVALID,
										const NTV2Standard inStandard = NTV2_STANDARD_INVALID);	//	New in SDK 15.0

	/**
		@brief		Enables or disables the Anc extraction components for the given SDI input.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput	Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[in]	inVancY		Specify true to enable Vanc Y component extraction;	 otherwise false to disable it.
		@param[in]	inVancC		Specify true to enable Vanc C component extraction;	 otherwise false to disable it.
		@param[in]	inHancY		Specify true to enable Hanc Y component extraction;	 otherwise false to disable it.
		@param[in]	inHancC		Specify true to enable Hanc C component extraction;	 otherwise false to disable it.
		@note		This function is provided for capture methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
	**/
	AJA_VIRTUAL bool	AncExtractSetComponents (const UWord inSDIInput,
													const bool inVancY, const bool inVancC,
													const bool inHancY, const bool inHancC);	//	New in SDK 16.0

	/**
		@brief		Enables or disables the given SDI input's Anc extractor.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[in]	inIsEnabled		Specify true to enable the Anc extractor;  otherwise false to disable it.
		@note		This function is provided for capture methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
		@see		CNTV2Card::AncExtractIsEnabled, \ref anccapture
	**/
	AJA_VIRTUAL bool	AncExtractSetEnable (const UWord inSDIInput, const bool inIsEnabled);	//	New in SDK 15.0

	/**
		@brief		Answers whether the given SDI input's Anc extractor is enabled/active or not.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest as a zero-based index value (e.g., 0 == SDIIn1).
		@param[out] outIsEnabled	Receives 'true' if the Anc extractor is enabled (running);	otherwise false.
		@see		CNTV2Card::AncExtractSetEnable, \ref anccapture
	**/
	AJA_VIRTUAL bool	AncExtractIsEnabled (const UWord inSDIInput, bool & outIsEnabled);	//	New in SDK 15.0

	/**
		@brief		Configures the given SDI input's Anc extractor to receive the next frame's F1 Anc data.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[in]	inFrameNumber	Tells the Anc extractor where to write the received Anc data, specified as a
									frame number.
		@param[in]	inChannel		Optionally specifies the ::NTV2Channel (FrameStore) that's driving the SDI input,
									if different from the SDI input. The default is to use the same ::NTV2Channel
									that corresponds to the SDI input (e.g., ::NTV2_CHANNEL1 == 0 == SDIIn1).
		@param[in]	inFrameSize		Optionally overrides the ::NTV2Framesize used to calculate the Anc buffer location
									in device SDRAM. Defaults to using the ::NTV2Framesize of the ::NTV2Channel being used.
		@note		This function is provided for capture methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
		@see		CNTV2Card::AncExtractSetField2WriteParams, \ref anccapture
	**/
	AJA_VIRTUAL bool	AncExtractSetWriteParams (const UWord inSDIInput, const ULWord inFrameNumber,
													const NTV2Channel inChannel = NTV2_CHANNEL_INVALID,
													const NTV2Framesize inFrameSize = NTV2_FRAMESIZE_INVALID);	//	New in SDK 15.0

	/**
		@brief		Configures the given SDI input's Anc extractor to receive the next frame's F2 Anc data.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[in]	inFrameNumber	Tells the Anc extractor where to write the received Anc data, specified as a
									frame number.
		@param[in]	inChannel		Optionally specifies the ::NTV2Channel (FrameStore) that's driving the SDI input,
									if different from the SDI input. The default is to use the same ::NTV2Channel
									that corresponds to the SDI input (e.g., ::NTV2_CHANNEL1 == 0 == SDIIn1).
		@param[in]	inFrameSize		Optionally overrides the ::NTV2Framesize used to calculate the Anc buffer location
									in device SDRAM. Defaults to using the ::NTV2Framesize of the ::NTV2Channel being used.
		@note		This function is provided for capture methods that don't use \ref aboutautocirculate.
					\ref aboutautocirculate based applications should not call this function.
		@see		CNTV2Card::AncExtractSetWriteParams, \ref anccapture
	**/
	AJA_VIRTUAL bool	AncExtractSetField2WriteParams (const UWord inSDIInput, const ULWord inFrameNumber,
														const NTV2Channel inChannel = NTV2_CHANNEL_INVALID,
														const NTV2Framesize inFrameSize = NTV2_FRAMESIZE_INVALID);	//	New in SDK 15.0

	/**
		@brief		Answers with the given SDI input's current Anc extractor info.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports custom Anc inserter firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[out]	outF1StartAddr	Receives the device SDRAM offset where the extractor starts writing F1 anc data.
		@param[out]	outF1EndAddr	Receives the device SDRAM offset where the extractor will stop writing F1 anc data.
		@param[out]	outF2StartAddr	Receives the device SDRAM offset where the extractor starts writing F2 anc data.
		@param[out]	outF2EndAddr	Receives the device SDRAM offset where the extractor will stop writing F2 anc data.
		@see		\ref anccapture
	**/
	AJA_VIRTUAL bool	AncExtractGetWriteInfo	(const UWord inSDIInput,
												uint64_t & outF1StartAddr, uint64_t & outF1EndAddr,
												uint64_t & outF2StartAddr, uint64_t & outF2EndAddr);	//	New in SDK v16.2

	/**
		@brief		Answers with the DIDs currently being excluded (filtered) by the SDI input's Anc extractor.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[out] outDIDs			Receives the ::NTV2DIDSet that contain the DIDs that are currently being
									filtered (excluded).
		@see		CNTV2Card::AncExtractSetFilterDIDs, \ref anccapture-filter
	**/
	AJA_VIRTUAL bool	AncExtractGetFilterDIDs (const UWord inSDIInput, NTV2DIDSet & outDIDs);	//	New in SDK 15.0

	/**
		@brief		Replaces the DIDs to be excluded (filtered) by the given SDI input's Anc extractor.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[in]	inDIDs			Specifies the DIDs to be filtered (excluded). Specify an empty set to
									disable all packet filtering.
		@note		DIDs having the value 0 (zero) are ignored.
		@see		CNTV2Card::AncExtractGetFilterDIDs, \ref anccapture-filter
	**/
	AJA_VIRTUAL bool	AncExtractSetFilterDIDs (const UWord inSDIInput, const NTV2DIDSet & inDIDs);	//	New in SDK 15.0

	/**
		@brief		Answers with the number of bytes of field 1 ANC extracted.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[out] outF1Size		Receives the number of bytes of field 1 ANC extracted;
	**/
	AJA_VIRTUAL bool	AncExtractGetField1Size (const UWord inSDIInput, ULWord & outF1Size);	//	New in SDK 16.0

	/**
		@brief		Answers with the number of bytes of field 2 ANC extracted.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[out] outF2Size		Receives the number of bytes of field 2 ANC extracted;
	**/
	AJA_VIRTUAL bool	AncExtractGetField2Size (const UWord inSDIInput, ULWord & outF2Size);	//	New in SDK 16.0

	/**
		@brief		Answers whether or not the given SDI input's Anc extractor reached its buffer limits.
					(Call ::NTV2DeviceCanDoCustomAnc to determine if the device supports Anc extractor firmware.)
		@return		True if successful; otherwise false.
		@param[in]	inSDIInput		Specifies the SDI input of interest (e.g., 0=SDIIn1, 1=SDIIn2, etc.).
		@param[out] outIsOverrun	Receives true if the extractor is reporting that it overran its buffer limits;
									otherwise false if it didn't.
		@param[in]	inField			Optionally specifies the field of interest. Specify 0 for the "total" buffer
									overflow between the F1 and F2 buffers;	 specify 1 for Field 1;	 specify 2 for Field 2.
									Defaults to zero (the "total"). (Added in SDK 16.1)
		@note		The extractor will not actually write any Anc bytes past its "stop" address, but it will
					report that it was about to via this "overrun" flag.
	**/
	AJA_VIRTUAL bool	AncExtractGetBufferOverrun (const UWord inSDIInput, bool & outIsOverrun, const UWord inField = 0);	//	New in SDK 15.0

	/**
		@return		The maximum number of distinct DIDs that the device Anc extractor filter can accommodate.
		@see		CNTV2Card::AncExtractSetFilterDIDs, CNTV2Card::AncExtractGetDefaultDIDs, \ref anccapture-filter
	**/
	static UWord		AncExtractGetMaxNumFilterDIDs (void);	//	New in SDK 15.0

	/**
		@return		The default DIDs that the device Anc extractor filter is started with.
		@param[in]	inHDAudio	Optionally specifies the desired audio packet filtering.
								Specify true (the default) for the default HD audio packet DIDs;
								otherwise false for the default SD audio packet DIDs.
		@see		CNTV2Card::AncExtractSetFilterDIDs, CNTV2Card::AncExtractGetMaxNumFilterDIDs, \ref anccapture-filter
	**/
	static NTV2DIDSet	AncExtractGetDefaultDIDs (const bool inHDAudio = true);	//	New in SDK 13.0
	///@}

	/**
		@name	TCP/IP
	**/
	///@{
	AJA_VIRTUAL bool		AcquireMailBoxLock (void);
	AJA_VIRTUAL bool		ReleaseMailBoxLock (void);
	AJA_VIRTUAL bool		AbortMailBoxLock (void);
	///@}

	/**
		@name	Misc
	**/
	///@{
	/**
		@brief		Reads the current die temperature of the device.
		@param[out] outTemp			Receives the temperature value that was read from the device.
		@param[in]	inTempScale		Specifies the temperature unit scale to use. Defaults to Celsius.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetDieTemperature (double & outTemp, const NTV2DieTempScale inTempScale = NTV2DieTempScale_Celsius);

	/**
		@brief		Reads the current "Vcc" voltage of the device.
		@param[out] outVoltage		Receives the "Vcc" voltage that was read from the device.
		@return		True if successful;	 otherwise false.
	**/
	AJA_VIRTUAL bool		GetDieVoltage (double & outVoltage);
	///@}

public:
	AJA_VIRTUAL std::string		GetFPGAVersionString (const NTV2XilinxFPGA inFPGA = eFPGAVideoProc);

	AJA_VIRTUAL Word			GetPCIFPGAVersion (void);		//	From CNTV2Status
	AJA_VIRTUAL std::string		GetPCIFPGAVersionString (void);

	/**
		@brief		Returns the size and time/date stamp of the device's currently-installed firmware.
		@param[out] outNumBytes		Receives the size of the installed firmware image, in bytes.
		@param[out] outDateStr		Receives a human-readable string containing the date the currently-installed firmware was built.
									The string has the format "YYYY/MM/DD", where "YYYY" is the year, "MM" is the month ("00" thru "12"),
									and "DD" is the day of the month ("00" thru "31").
		@param[out] outTimeStr		Receives a human-readable string containing the time the currently-installed firmware was built
									(in local Pacific time). The string has the format "HH:MM:SS", where HH is "00" thru "23",
									and both MM and SS are "00" thru "59".
		@return		True if successful;	 otherwise false.
		@note		This function has nothing to do with the firmware bitfiles that are currently installed on the local host's file system.
	**/
	AJA_VIRTUAL bool			GetInstalledBitfileInfo (ULWord & outNumBytes, std::string & outDateStr, std::string & outTimeStr);

	/**
		@brief		Generates and returns an info string with date, time and name for the given inBifFileInfo.
		@param[in]	inBitFileInfo	BitFile Info structure to get information from, no validity checks are made on the structure before use.
		@return		A string containing the bitfile information.
	**/
	AJA_VIRTUAL std::string		GetBitfileInfoString (const BITFILE_INFO_STRUCT & inBitFileInfo);

	/**
		@brief		Answers whether or not the "fail-safe" (aka "safe-boot") bitfile is currently loaded and running in the FPGA.
		@param[out] outIsFailSafe	Receives true if the "fail-safe" bitfile is currently loaded and running in the FPGA;
									otherwise receives false. This return value cannot be trusted if the function result
									is false.
		@return		True if successful;	 otherwise false.
		@note		The "outIsFailSafe" answer can only be trusted if the function result is true.
		@see		::NTV2DeviceCanReportFailSafeLoaded
	**/
	AJA_VIRTUAL bool			IsFailSafeBitfileLoaded (bool & outIsFailSafe);

	/**
		@brief		Answers whether or not the FPGA can be reloaded without powering off.
		@param[out] outCanWarmBoot	Receives true if the FPGA can be reloaded without powering off the device.
									Receives false if it can only be reloaded after power-cycling.
		@return		True if successful;	 otherwise false.
		@note		The "outCanWarmBoot" answer can only be trusted if the function result is true.
	**/
	AJA_VIRTUAL bool			CanWarmBootFPGA (bool & outCanWarmBoot);

	AJA_VIRTUAL bool				IsDynamicDevice (void);			///< @return	True if this device can quickly change bitfiles;  otherwise false.
	AJA_VIRTUAL bool				IsDynamicFirmwareLoaded(void);	///< @return	True if the device has been dynamically reconfigured;  otherwise false.
	AJA_VIRTUAL NTV2DeviceID		GetBaseDeviceID();				///< @return	Return base device id for IsDynamicDevice, otherwise DEVICE_ID_INVALID
	AJA_VIRTUAL NTV2DeviceIDList	GetDynamicDeviceList (void);	///< @return	A list of supported/available dynamic device IDs.
	AJA_VIRTUAL NTV2DeviceIDSet		GetDynamicDeviceIDs (void);		///< @return	A set of supported/available dynamic device IDs.

	/**
		@param[in]	inDeviceID	Specifies the device ID of interest.
		@return		True if the given ::NTV2DeviceID can be dynamically loaded; otherwise false.
	**/
	AJA_VIRTUAL bool			CanLoadDynamicDevice (const NTV2DeviceID inDeviceID);

	/**
		@brief		Quickly, dynamically loads the given device ID firmware.
		@param[in]	inDeviceID		Specifies the device ID of interest.
		@return		True if loaded successfully; otherwise false.
		@note		If successful, calling CNTV2Card::GetDeviceID will return the same ::NTV2DeviceID
					as "inDeviceID". This CNTV2Card instance will be talking to the same hardware
					device, but it will have a different personality with different capabilities.
	**/
	AJA_VIRTUAL bool			LoadDynamicDevice (const NTV2DeviceID inDeviceID);

	/**
		@brief		Adds the given bitfile to the list of available dynamic bitfiles.
		@param[in]	inBitfilePath		A string containing the path to the bitfile.
		@return		True if added successfully; otherwise false.
	**/
	AJA_VIRTUAL bool			AddDynamicBitfile (const std::string & inBitfilePath);

	/**
		@brief		Adds all bitfiles found in the given host file directory to the list
					of available dynamic bitfiles.
		@param[in]	inDirectory		A string containing the path to the directory.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool			AddDynamicDirectory (const std::string & inDirectory);

	/**
		@brief		Returns a string containing the decoded, human-readable device serial number.
		@param[in]	inSerialNumber		Specifies the 64-bit device serial number.
		@return		A string containing the decoded, human-readable device serial number. If invalid, returns the string "INVALID?".
	**/
	static std::string			SerialNum64ToString (const uint64_t inSerialNumber);

	typedef enum
	{
		RED,
		GREEN,
		BLUE,
		NUM_COLORS
	} ColorCorrectionColor; //	From CNTV2ColorCorrection

protected:
	static NTV2Buffer NULL_POINTER;	///< @brief Used for default empty NTV2Buffer parameters -- do not modify.

public:
	/**
		@name	HEVC-Specific Functions
	**/
	///@{
	/**
		@brief		Returns the driver version and time/date stamp of the hevc device's currently-installed firmware.
		@param[out] pInfo			HevcDeviceInfo structure to receive the information.
		@return		True if successful;	 otherwise false.
	**/ 
	AJA_VIRTUAL bool HevcGetDeviceInfo (HevcDeviceInfo* pInfo);

	/**
		@brief		Write an hevc register.
		@param[in]	address			Hevc register byte address
		@param[in]	value			Hevc register data
		@param[in]	mask			Read bit mask
		@param[in]	shift			Read bit shift
		@return		True if successful;	 otherwise false.
	**/ 
	AJA_VIRTUAL bool HevcWriteRegister (ULWord address, ULWord value, ULWord mask = 0xffffffff, ULWord shift = 0);

	/**
		@brief		Read an hevc register.
		@param[in]	address			Hevc register byte address
		@param[out] pValue			Hevc register data
		@param[in]	mask			Read bit mask
		@param[in]	shift			Read bit shift
		@return		True if successful;	 otherwise false.
	**/ 
	AJA_VIRTUAL bool HevcReadRegister (ULWord address, ULWord* pValue, ULWord mask = 0xffffffff, ULWord shift = 0);

	/**
		@brief		Send a command to the hevc device.	See the hevc codec documentation for details on commands.
		@param[in]	pCommand		HevcDeviceCommand structure with the command parameters.
		@return		True if successful;	 otherwise false.
	**/ 
	AJA_VIRTUAL bool HevcSendCommand (HevcDeviceCommand* pCommand);

	/**
		@brief		Transfer video to/from the hevc device.
		@param[in]	pTransfer		HevcDeviceTransfer structure with the transfer parameters.
		@return		True if successful;	 otherwise false.
	**/ 
	AJA_VIRTUAL bool HevcVideoTransfer (HevcDeviceTransfer* pTransfer);

	/**
		@brief		Get the status of the hevc device.
		@param[in]	pStatus			HevcDeviceDebug structure to receive the information.
		@return		True if successful;	 otherwise false.
	**/ 
	AJA_VIRTUAL bool HevcGetStatus (HevcDeviceStatus* pStatus);

	/**
		@brief		Get debug data from the hevc device.
		@param[in]	pDebug			HevcDeviceStatus structure to receive the information.	This is an expanded version
									of the device status that contains performance information.	 This structure may change
									more often.
		@return		True if successful;	 otherwise false.
	**/ 
	AJA_VIRTUAL bool HevcDebugInfo (HevcDeviceDebug* pDebug);
	///@}

	/**
		@name	HDMI HDR Support
	**/
	///@{
	/**
		@brief		Enables or disables HDMI HDR.
		@param[in]	inEnableHDMIHDR		If true, sets the device to output HDMI HDR Metadata; otherwise sets the device to not output HDMI HDR Metadata.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool EnableHDMIHDR (const bool inEnableHDMIHDR);	//	New in SDK 12.5
	AJA_VIRTUAL bool GetHDMIHDREnabled (void);	///< @return	True if HDMI HDR metadata output is enabled for the device;	 otherwise false.

	/**
		@brief		Enables or disables HDMI HDR Dolby Vision.
		@param[in]	inEnable		If true, sets the device to output HDMI HDR Dolby Vision; otherwise sets the device to not output HDMI HDR Dolby Vision.
		@note		This function only affects Dolby HDR signaling. The client application is responsible for transferring Dolby-encoded pixel data from the
					host to the device frame buffer(s) for HDMI transmission.
		@see		CNTV2Card::GetHDMIHDRDolbyVisionEnabled
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool EnableHDMIHDRDolbyVision (const bool inEnable);	//	New in SDK 13.0
	AJA_VIRTUAL bool GetHDMIHDRDolbyVisionEnabled (void);	///< @return	True if HDMI HDR Dolby Vision output is enabled for the device;	 otherwise false.


	/**
		@brief		Enables or disables BT.2020 Y'cC'bcC'rc versus BT.2020 Y'C'bC'r or R'G'B'.
		@param[in]	inEnableConstantLuminance	If true, sets the device to BT.2020 Y'cC'bcC'rc; otherwise sets the device to BT.2020 Y'C'bC'r or R'G'B'.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRConstantLuminance (const bool inEnableConstantLuminance);	//	New in SDK 12.5
	AJA_VIRTUAL bool GetHDMIHDRConstantLuminance (void);		///< @return	True if BT.2020 Y'cC'bcC'rc is enabled; otherwise false for BT.2020 Y'C'bC'r or R'G'B'.

	/**
		@brief		Sets the Display Mastering data for Green Primary X as defined in SMPTE ST 2086. This is Byte 3 and 4 of SMDT Type 1.
		@param[in]	inGreenPrimaryX		Specifies the Green Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRGreenPrimaryX (const uint16_t inGreenPrimaryX);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for Green Primary X as defined in SMPTE ST 2086. This is Byte 3 and 4 of SMDT Type 1.
		@param[out] outGreenPrimaryX		Receives the Green Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRGreenPrimaryX (uint16_t & outGreenPrimaryX);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for Green Primary Y as defined in SMPTE ST 2086. This is Byte 5 and 6 of SMDT Type 1.
		@param[in]	inGreenPrimaryY		Specifies the Green Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRGreenPrimaryY (const uint16_t inGreenPrimaryY);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for Green Primary Y as defined in SMPTE ST 2086. This is Byte 5 and 6 of SMDT Type 1.
		@param[out] outGreenPrimaryY		Receives the Green Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRGreenPrimaryY (uint16_t & outGreenPrimaryY);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for Blue Primary X as defined in SMPTE ST 2086. This is Byte 7 and 8 of SMDT Type 1.
		@param[in]	inBluePrimaryX		Specifies the Blue Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRBluePrimaryX (const uint16_t inBluePrimaryX);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for Blue Primary X as defined in SMPTE ST 2086. This is Byte 7 and 8 of SMDT Type 1.
		@param[out] outBluePrimaryX		Receives the Blue Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRBluePrimaryX (uint16_t & outBluePrimaryX);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for Blue Primary Y as defined in SMPTE ST 2086. This is Byte 9 and 10 of SMDT Type 1.
		@param[in]	inBluePrimaryY		Specifies the Blue Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRBluePrimaryY (const uint16_t inBluePrimaryY);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for Blue Primary Y as defined in SMPTE ST 2086. This is Byte 9 and 10 of SMDT Type 1.
		@param[out] outBluePrimaryY		Receives the Blue Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRBluePrimaryY (uint16_t & outBluePrimaryY);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for Red Primary X as defined in SMPTE ST 2086. This is Byte 11 and 12 of SMDT Type 1.
		@param[in]	inRedPrimaryX		Specifies the Red Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRRedPrimaryX (const uint16_t inRedPrimaryX);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for Red Primary X as defined in SMPTE ST 2086. This is Byte 11 and 12 of SMDT Type 1.
		@param[out] outRedPrimaryX		Receives the Red Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRRedPrimaryX (uint16_t & outRedPrimaryX);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for Red Primary Y as defined in SMPTE ST 2086. This is Byte 13 and 14 of SMDT Type 1.
		@param[in]	inRedPrimaryY		Specifies the Red Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRRedPrimaryY (const uint16_t inRedPrimaryY);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for Red Primary Y as defined in SMPTE ST 2086. This is Byte 13 and 14 of SMDT Type 1.
		@param[out] outRedPrimaryY		Receives the Red Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRRedPrimaryY (uint16_t & outRedPrimaryY);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for White Point X as defined in SMPTE ST 2086. This is Byte 15 and 16 of SMDT Type 1.
		@param[in]	inWhitePointX		Specifies the White Point X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRWhitePointX (const uint16_t inWhitePointX);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for White Point X as defined in SMPTE ST 2086. This is Byte 15 and 16 of SMDT Type 1.
		@param[out] outWhitePointX		Receives the White Point X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRWhitePointX (uint16_t & outWhitePointX);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for White Point Y as defined in SMPTE ST 2086. This is Byte 17 and 18 of SMDT Type 1.
		@param[in]	inWhitePointY		Specifies the White Point Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRWhitePointY (const uint16_t inWhitePointY);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for White Point Y as defined in SMPTE ST 2086. This is Byte 17 and 18 of SMDT Type 1.
		@param[out] outWhitePointY		Receives the White Point Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRWhitePointY (uint16_t & outWhitePointY);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for the Max Mastering Luminance value as defined in SMPTE ST 2086. This is Byte 19 and 20 of SMDT Type 1.
		@param[in]	inMaxMasteringLuminance		Specifies the Max Mastering Luminance value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRMaxMasteringLuminance (const uint16_t inMaxMasteringLuminance);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for the Max Mastering Luminance value as defined in SMPTE ST 2086. This is Byte 19 and 20 of SMDT Type 1.
		@param[out] outMaxMasteringLuminance		Receives the Max Mastering Luminance value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRMaxMasteringLuminance (uint16_t & outMaxMasteringLuminance);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for the Min Mastering Luminance value as defined in SMPTE ST 2086. This is Byte 21 and 22 of SMDT Type 1.
		@param[in]	inMinMasteringLuminance		Specifies the Min Mastering Luminance value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRMinMasteringLuminance (const uint16_t inMinMasteringLuminance);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for the Min Mastering Luminance value as defined in SMPTE ST 2086. This is Byte 21 and 22 of SMDT Type 1.
		@param[out] outMinMasteringLuminance		Receives the Min Mastering Luminance value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRMinMasteringLuminance (uint16_t & outMinMasteringLuminance);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for the Max Content Light Level(Max CLL) value. This is Byte 23 and 24 of SMDT Type 1.
		@param[in]	inMaxContentLightLevel		Specifies the Max Content Light Level value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRMaxContentLightLevel (const uint16_t inMaxContentLightLevel);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for the Max Content Light Level(Max CLL) value. This is Byte 23 and 24 of SMDT Type 1.
		@param[out] outMaxContentLightLevel		Receives the Max Content Light Level value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRMaxContentLightLevel (uint16_t & outMaxContentLightLevel);	//	New in SDK 12.5

	/**
		@brief		Sets the Display Mastering data for the Max Frame Average Light Level(Max FALL) value. This is Byte 25 and 26 of SMDT Type 1.
		@param[in]	inMaxFrameAverageLightLevel		Specifies the Max Frame Average Light Level value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRMaxFrameAverageLightLevel (const uint16_t inMaxFrameAverageLightLevel);	//	New in SDK 12.5
	/**
		@brief		Answers with the Display Mastering data for the Max Frame Average Light Level(Max FALL) value. This is Byte 25 and 26 of SMDT Type 1.
		@param[out] outMaxFrameAverageLightLevel		Receives the Max Frame Average Light Level value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRMaxFrameAverageLightLevel (uint16_t & outMaxFrameAverageLightLevel);	//	New in SDK 12.5


	AJA_VIRTUAL bool SetHDMIHDRElectroOpticalTransferFunction (const uint8_t inEOTFByte);	//	New in SDK 12.5
	AJA_VIRTUAL bool GetHDMIHDRElectroOpticalTransferFunction (uint8_t & outEOTFByte);	//	New in SDK 12.5
	AJA_VIRTUAL bool SetHDMIHDRStaticMetadataDescriptorID (const uint8_t inSMDId);	//	New in SDK 12.5
	AJA_VIRTUAL bool GetHDMIHDRStaticMetadataDescriptorID (uint8_t & outSMDId);	//	New in SDK 12.5

	AJA_VIRTUAL bool SetHDRData (const HDRFloatValues & inFloatValues);	//	New in SDK 12.5
	AJA_VIRTUAL bool SetHDRData (const HDRRegValues & inRegisterValues);	//	New in SDK 12.5
	AJA_VIRTUAL bool GetHDRData (HDRFloatValues & outFloatValues);	//	New in SDK 12.5
	AJA_VIRTUAL bool GetHDRData (HDRRegValues & outRegisterValues);	//	New in SDK 12.5
	AJA_VIRTUAL bool SetHDMIHDRBT2020 (void);	//	New in SDK 12.5
	AJA_VIRTUAL bool SetHDMIHDRDCIP3 (void);	//	New in SDK 12.5
	
	AJA_VIRTUAL bool SetVPIDTransferCharacteristics (const NTV2VPIDTransferCharacteristics inValue, const NTV2Channel inChannel);	//	New in SDK 15.2
	AJA_VIRTUAL bool GetVPIDTransferCharacteristics (NTV2VPIDTransferCharacteristics & outValue, const NTV2Channel inChannel);	//	New in SDK 15.2
	AJA_VIRTUAL bool SetVPIDColorimetry (const NTV2VPIDColorimetry inValue, const NTV2Channel inChannel);	//	New in SDK 15.2
	AJA_VIRTUAL bool GetVPIDColorimetry (NTV2VPIDColorimetry & outValue, const NTV2Channel inChannel);	//	New in SDK 15.2
	AJA_VIRTUAL bool SetVPIDLuminance (const NTV2VPIDLuminance inValue, const NTV2Channel inChannel);	//	New in SDK 15.2
	AJA_VIRTUAL bool GetVPIDLuminance (NTV2VPIDLuminance & outValue, const NTV2Channel inChannel);	//	New in SDK 15.2
	AJA_VIRTUAL bool SetVPIDRGBRange (const NTV2VPIDRGBRange inValue, const NTV2Channel inChannel);	//	New in SDK 16.0
	AJA_VIRTUAL bool GetVPIDRGBRange (NTV2VPIDRGBRange & outValue, const NTV2Channel inChannel);	//	New in SDK 16.0
	
	AJA_VIRTUAL bool Set3DLUTTableLocation (const ULWord inFrameNumber, ULWord inLUTIndex = 0);	//	New in SDK 16.0
	AJA_VIRTUAL bool Load3DLUTTable (void);	//	New in SDK 16.0
	AJA_VIRTUAL bool Set1DLUTTableLocation (const NTV2Channel inChannel, const ULWord inFrameNumber, ULWord inLUTIndex = 0);	//	New in SDK 16.0
	AJA_VIRTUAL bool Load1DLUTTable (const NTV2Channel inChannel);	//	New in SDK 16.0

	//	MultiViewer/MultiRasterizer
	AJA_VIRTUAL bool HasMultiRasterWidget (void);						//	New in SDK 16.1
	AJA_VIRTUAL bool SetMultiRasterBypassEnable (const bool inEnable);	//	New in SDK 16.1
	AJA_VIRTUAL bool GetMultiRasterBypassEnable (bool & outEnabled);	//	New in SDK 16.1
	AJA_VIRTUAL bool IsMultiRasterWidgetChannel (const NTV2Channel inChannel);	//	New in SDK 16.2
	///@}

	AJA_VIRTUAL bool	IsBreakoutBoardConnected (void);	//	New in SDK 17.0

#if !defined(NTV2_DEPRECATE_16_1)
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool SetAudioOutputMonitorSource (const NTV2AudioMonitorSelect inChannelPair, const NTV2Channel inAudioSystem = NTV2_CHANNEL1))	{return SetAudioOutputMonitorSource(inChannelPair, NTV2AudioSystem(inAudioSystem));}	///< @deprecated	Use the function that uses NTV2AudioChannelPair and NTV2AudioSystem params.
	AJA_VIRTUAL inline NTV2_DEPRECATED_f(bool GetAudioOutputMonitorSource (NTV2AudioMonitorSelect & chp, NTV2Channel & ch)) {NTV2AudioSystem sys;  if (GetAudioOutputMonitorSource(chp, sys)) {ch = NTV2Channel(sys); return true;}	 return false;} ///< @deprecated	Use the function that uses NTV2AudioChannelPair and NTV2AudioSystem params.
#endif	//	NTV2_DEPRECATE_16_1

protected:
	AJA_VIRTUAL ULWord			GetSerialNumberLow (void);			//	From CNTV2Status
	AJA_VIRTUAL ULWord			GetSerialNumberHigh (void);			//	From CNTV2Status
	AJA_VIRTUAL inline bool		IS_CHANNEL_VALID (const NTV2Channel inChannel) const	{return !IS_CHANNEL_INVALID(inChannel);}	//	New in SDK 16.2
	AJA_VIRTUAL bool			IS_CHANNEL_INVALID (const NTV2Channel inChannel) const;
	AJA_VIRTUAL bool			IS_OUTPUT_SPIGOT_INVALID (const UWord inOutputSpigot) const;
	AJA_VIRTUAL bool			IS_INPUT_SPIGOT_INVALID (const UWord inInputSpigot) const;
	AJA_VIRTUAL bool			SetWarmBootFirmwareReload(bool enable);

	//	Seamless Anc Playout & Capture
	//		For AutoCirculate Playout
	AJA_VIRTUAL bool			S2110DeviceAncToXferBuffers (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo);
	//		For Non-AutoCirculate Playout
	AJA_VIRTUAL bool			S2110DeviceAncToBuffers (const NTV2Channel inChannel, NTV2Buffer & ancF1, NTV2Buffer & ancF2);
	//		For AutoCirculate Capture
	AJA_VIRTUAL bool			S2110DeviceAncFromXferBuffers (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo);
	//		For Non-AutoCirculate Capture
	AJA_VIRTUAL bool			S2110DeviceAncFromBuffers (const NTV2Channel inChannel, NTV2Buffer & ancF1, NTV2Buffer & ancF2);
	AJA_VIRTUAL bool			WriteSDIInVPID (const NTV2Channel inChannel, const ULWord inValA, const ULWord inValB);

private:
	// frame buffer sizing helpers
	AJA_VIRTUAL bool	GetLargestFrameBufferFormatInUse(NTV2FrameBufferFormat & outFBF);
	AJA_VIRTUAL bool	GetFrameInfo(const NTV2Channel inChannel, NTV2FrameGeometry & outGeometry, NTV2FrameBufferFormat & outFBF);
	AJA_VIRTUAL bool	IsBufferSizeChangeRequired(NTV2Channel channel, NTV2FrameGeometry currentGeometry, NTV2FrameGeometry newGeometry,
													NTV2FrameBufferFormat format);
	AJA_VIRTUAL bool	IsBufferSizeChangeRequired(NTV2Channel channel, NTV2FrameGeometry geometry,
									NTV2FrameBufferFormat currentFormat, NTV2FrameBufferFormat newFormat);
	AJA_VIRTUAL bool	GetFBSizeAndCountFromHW(ULWord* size, ULWord* count);

	AJA_VIRTUAL bool	IsMultiFormatActive (void); ///< @return	True if the device supports the multi format feature and it's enabled; otherwise false.
	AJA_VIRTUAL bool	CopyVideoFormat(const NTV2Channel inSrc, const NTV2Channel inFirst, const NTV2Channel inLast);
	class DeviceCapabilities	mDevCap;
};	//	CNTV2Card


typedef CNTV2Card	CNTV2Device;	///< @brief Instances of this class are able to interrogate and control an NTV2 AJA video/audio capture/playout device.
#if !defined(NTV2_DEPRECATE_16_2)
	typedef CNTV2Card	CNTV2Status;			///< @deprecated	Use CNTV2Card instead.
	typedef CNTV2Card	CNTV2TestPattern;		///< @deprecated	Use CNTV2Card instead.
	typedef CNTV2Card	CNTV2VidProc;			///< @deprecated	Use CNTV2Card instead
	typedef CNTV2Card	CNTV2ColorCorrection;	///< @deprecated	Use CNTV2Card instead.
	typedef CNTV2Card	CNTV2ProcAmp;			///< @deprecated	Use CNTV2Card instead.
	typedef CNTV2Card	CXena2VidProc;			///< @deprecated	Use CNTV2Card instead.
#endif	//	!defined(NTV2_DEPRECATE_16_2)

#define SetTablesToHardware						LoadLUTTables
#define GetTablesFromHardware					GetLUTTables

/////////////////////////////////////////////////////////////////////////////


/**
	@brief		Audits an NTV2 device's SDRAM utilization, and can report contiguous regions of SDRAM, whether unused/free,
				those being read/written by AutoCirculate, those being read/written by non-AutoCirculating FrameStores,
				those that are in conflict (AutoCirculate, FrameStore and/or Audio collisions), plus invalid/out-of-bounds
				regions being accessed.
	@details	This implementation currently employs a least-common-denominator 8MB frame size. This was chosen because
				the SDRAM complement across all currently-supported devices is evenly divisible by (at most) 8MB. Any 8MB
				block can be translated into larger denominations as needed (e.g. 16MB, 32MB, 64MB, etc.).
				A memory region is described by an offset and length, both unsigned 16-bit values encoded into a 32-bit ULWord.
				Region lists are specified or provided by the ::ULWordSequence data type (a std::vector of ULWord values).
				Memory regions can be tagged with one or more std::string tags in an ::NTV2StringList (a std::vector of
				std::string values). A region with no tags (an empty list) is considered unallocated/free/unused. A tag string
				describes a region that is considered in-use (or potentially in-use).
				-	<b>Audio System:</b> "Aud1 Write" indicates Audio System 1 is actively capturing;  "Aud2 Read" indicates
					Audio System 2 is actively playing;  "Aud3" indicates the memory region that would be used by Audio System 3
					if it were actively capturing or playing.
				-	<b>FrameStore:</b>  "Ch2 Read" indicates FrameStore 2 is enabled in playback mode;  "Ch3 Write" indicates
					FrameStore 3 is enabled and configured for capture/ingest.
				-	<b>AutoCirculate:</b>  "AC1 Write" indicates AutoCirculate channel 1 is initialized for capture.
				-	A region with only one tag is considered "in-use", or, in the case of an "Aud" tag without "Read" or "Write"
					is <i>potentially</i> in use.
				-	A region having more than one tag may be "in-conflict" and considered "bad" or potentially "problematic".
**/
class SDRAMAuditor
{
	public:
		/**
			@brief	My default constructor.  I am not ready for use until my SDRAMAuditor::Assess method has been called.
		**/
		explicit SDRAMAuditor ()
			:	mDeviceID		(DEVICE_ID_INVALID),
				mFrameTags		(),
				m8MB			(0x00800000),
				mNumFrames		(0),
				mIntrinsicSize	(0)
		{
		}

		/**
			@brief	Constructs me and automatically assesses the given device.
			@param[in]	inDevice	The device of interest, which must be open and ready.
		**/
		explicit SDRAMAuditor (CNTV2Card & inDevice)
			:	mDeviceID		(DEVICE_ID_INVALID),
				mFrameTags		(),
				m8MB			(0x00800000),
				mNumFrames		(0),
				mIntrinsicSize	(0)
		{
			AssessDevice(inDevice);
		}

		/**
			@brief	Assesses the given device.
			@param[in]	inDevice	The device of interest.
			@param[in]	inIgnoreStoppedAudioBuffers		Optionally specifies how to treat stopped audio system buffer regions.
														False means "do not ignore non-running audio engine buffers", which will
														tag their buffer regions, making them potentially or likely to be in-use.
														True means non-running audio buffers will not be tagged.
														Defaults to false (erring on the safe side, to avoid potential conflicts).
		**/
		bool AssessDevice (CNTV2Card & inDevice, const bool inIgnoreStoppedAudioBuffers = false);

		/**
			@brief	Answers with the lists of free, in-use and conflicting 8MB memory blocks.
					Each ULWord represents a region -- an 8MB block offset in the most-significant 16 bits,
					and the number of 8MB blocks in the least-significant 16 bits.
			@param[out]	outFree		Receives the list of free (unused) memory regions.
			@param[out]	outUsed		Receives the list of used memory regions.
			@param[out]	outBad		Receives the list of colliding and illegal memory regions.
			@return	True if successful;  otherwise false.
		**/
		bool GetRegions (ULWordSequence & outFree, ULWordSequence & outUsed, ULWordSequence & outBad) const;

		/**
			@brief	Answers with the list of free memory regions.
			@param[out]	outBlks		Receives the region list.
		**/
		inline bool GetFreeRegions (ULWordSequence & outBlks) const
		{
			ULWordSequence	used, bad;
			return GetRegions (outBlks, used, bad);
		}

		/**
			@brief	Answers with the list of colliding and illegal memory regions.
			@param[out]	outBlks		Receives the region list.
		**/
		inline bool GetBadRegions (ULWordSequence & outBlks) const
		{
			ULWordSequence	used, free;
			return GetRegions (free, used, outBlks);
		}

		/**
			@brief	Answers with the list of used memory regions.
			@param[out]	outBlks		Receives the region list.
		**/
		inline bool GetUsedRegions (ULWordSequence & outBlks) const
		{
			ULWordSequence	bad, free;
			return GetRegions (free, outBlks, bad);
		}

		/**
			@brief	Answers with the list of tags for the given frame number.
			@param[in]	inIndex		Specifies the zero-based frame index number of the frame of interest.
			@param[out]	outTags		Receives the list of tag strings (if any).
			@return		True if successful;  otherwise false.
		**/
		bool GetTagsForFrameIndex (const UWord inIndex, NTV2StringSet & outTags) const;

		bool HasFrameIndex (const UWord inIndex) const	{return mFrameTags.find(inIndex) != mFrameTags.end();}	///< @return	True if the given frame number is valid.

		size_t GetTagCount (const UWord inIndex) const;		///< @return	The number of tags associated with the given frame number.

		inline bool HasTag (const UWord inIndex) const				{return GetTagCount(inIndex) > 0;}		///< @return	True if the given frame number has a tag associated with it.
		inline bool HasConflicts (const UWord inIndex) const		{return GetTagCount(inIndex) > 1;}		///< @return	True if the given frame number has problems (i.e. more than one tag associated with it).
		inline ULWord GetIntrinsicFrameByteCount (void) const		{return mIntrinsicSize;}				///< @return	The size, in bytes, of the intrinsic frame size;  or zero if not known.

		/**
			@brief		Translates an 8MB-chunked list of regions into another list of regions with frame indexes and sizes
						expressed in frame units determined by the device's intrinsic frame size (measured when AssessDevice
						was called) and also based on if the given quad or quad-quad mode is active.
			@param[out]	outRgns			Receives the translated region list.
			@param[in]	inRgns			Specifies the 8MB-based region list to be translated.
			@param[in]	inIsQuad		Specify true if translated regions should be Quad-sized.
			@param[in]	inIsQuadQuad	Specify true if translated regions should be Quad-Quad-sized.
			@return		True if successful;  otherwise false.
		**/
		bool TranslateRegions (ULWordSequence & outRgns, const ULWordSequence & inRgns, const bool inIsQuad, const bool inIsQuadQuad) const;

		/**
			@brief	Dumps a human-readable list of regions into the given stream.
			@param	oss		Specifies the output stream to receives the report.
			@return	A reference to the given output stream.
		**/
		std::ostream & RawDump (std::ostream & oss) const;

		/**
			@brief	Dumps all 8MB blocks/frames and their tags, if any, into the given stream.
			@param	oss		Specifies the output stream to receives the report.
			@return	A reference to the given output stream.
		**/
		std::ostream & DumpBlocks (std::ostream & oss) const;

	//	Static/Class Methods
	public:
		/**
			@return	The unique set of regions resulting from the union of the given three lists of regions.
		**/
		static ULWordSet CoalesceRegions (const ULWordSequence & inRgn1, const ULWordSequence & inRgn2, const ULWordSequence & inRgn3);

	protected:
		bool TagAudioBuffers (CNTV2Card & inDevice, const bool inMarkStoppedAudioBuffersFree);
		bool TagVideoFrames (CNTV2Card & inDevice);
		bool TagMemoryBlock (const ULWord inStartAddr, const ULWord inByteLength, const std::string & inTag);
		bool TagMemoryBlock (const uint64_t inStartAddr, const uint64_t inByteLength, const std::string & inTag)
		{
			return TagMemoryBlock (ULWord(inStartAddr), ULWord(inByteLength), inTag);
		}

	private:
		typedef	std::pair<UWord, NTV2StringSet>	FrameTag;
		typedef	std::map<UWord, NTV2StringSet>	FrameTags;
		typedef FrameTags::const_iterator		FrameTagsConstIter;

		NTV2DeviceID	mDeviceID;		///< @brief	The device ID of the device being assessed.
		FrameTags		mFrameTags;		///< @brief	Stores tag(s) for each 8MB frame.
		const ULWord	m8MB;			///< @brief	1024 x 1024 x 8
		UWord			mNumFrames;		///< @brief	Total maximum number of 8MB frames on this device.
		ULWord			mIntrinsicSize;	///< @brief	Intrinsic frame size of device at time of assessment, in bytes (8MB or 16MB).
};	//	SDRAMAuditor

#endif	//	NTV2CARD_H
