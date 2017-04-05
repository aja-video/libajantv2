/**
	@file		ntv2card.h
	@brief		Declares the CNTV2Card class and the NTV2VideoFormatSet.
	@copyright	(C) 2004-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#ifndef NTV2CARD_H
#define NTV2CARD_H

#include "ajaexport.h"

#if defined (MSWindows)
	#include "ntv2windriverinterface.h"
#elif defined (AJAMac)
	#include "ntv2macdriverinterface.h"
#elif defined (AJALinux)
	#include "ntv2linuxdriverinterface.h"
#endif
#include "ntv2signalrouter.h"

#include <set>
#include <string>
#include <iostream>
#include <vector>

/**
	@brief	Used in calls to CNTV2Card::GetBoolParam to determine device features.
**/
typedef enum _NTV2BoolParamID
{
//	kDeviceCanChangeEmbeddedAudioClock,			///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has NorthWest Logic DMA hardware.
	kDeviceCanChangeFrameBufferSize,			///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device does not have fixed frame buffer sizes.
	kDeviceCanDisableUFC,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has at least one UFC, and it can be disabled.
	kDeviceCanDo2KVideo,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device is capable of handling 2Kx1556 video.
	kDeviceCanDo3GLevelConversion,				///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can perform 3G level B to 3G level A conversion.
	kDeviceCanDoRGBLevelAConversion,			///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can do RGB over 3G Level A.
	kDeviceCanDo425Mux,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device supports SMPTE 425 mux control.
	kDeviceCanDo4KVideo,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can handle 4K/UHD video.
	kDeviceCanDoAESAudioIn,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has at least one AES/EBU audio input.
	kDeviceCanDoAnalogAudio,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has at least one analog audio input or output.
	kDeviceCanDoAnalogVideoIn,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has at least one analog video input.
	kDeviceCanDoAnalogVideoOut,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has at least one analog video output.
//	kDeviceCanDoAudio2Channels,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the audio system(s) on the device can be configured to embed/de-embed only 2 audio channels.
//	kDeviceCanDoAudio6Channels,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the audio system(s) on the device can be configured to embed/de-embed only 6 audio channels.
//	kDeviceCanDoAudio8Channels,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the audio system(s) on the device can be configured to embed/de-embed only 8 audio channels.
//	kDeviceCanDoAudio96K,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if audio system(s) on the device can be set to a 96kHz sample rate.
//	kDeviceCanDoAudioDelay,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if audio system(s) on the device have an adjustable delay.
	kDeviceCanDoBreakoutBox,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can be connected to an AJA breakout box.
	kDeviceCanDoCapture,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can capture (ingest) video.
//	kDeviceCanDoColorCorrection,				///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has at least one programmable LUT.
//	kDeviceCanDoCustomAnc,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device supports ANC insertion/extraction.
//	kDeviceCanDoDSKOpacity,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has a mixer/keyer whose opacity is adjustable.
//	kDeviceCanDoDualLink,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can input/output 10-bit RGB over 2-wire SDI.
//	kDeviceCanDoDVCProHD,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can squeeze/stretch between 1920x1080/1280x1080 and 1280x720/960x720.
//	kDeviceCanDoEnhancedCSC,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has enhanced color space converter capability.
//	kDeviceCanDoFrameStore1Display,				///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can display video from frame store 1.
//	kDeviceCanDoFreezeOutput,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can freeze output video.
//	kDeviceCanDoHDMIOutStereo,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can handle 3D-stereo video output over HDMI.
//	kDeviceCanDoHDV,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can squeeze/stretch between 1920x1080 and 1440x1080.
//	kDeviceCanDoHDVideo,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can handle HD (High Definition) video.
	kDeviceCanDoIsoConvert,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can do ISO conversion.
	kDeviceCanDoLTC,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can take in LTC (Linear TimeCode) from one of its inputs.
	kDeviceCanDoLTCInOnRefPort,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can read LTC (Linear TimeCode) from its reference input.
	kDeviceCanDoMSI,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device DMA hardware supports MSI (Message Signaled Interrupts).
	kDeviceCanDoMultiFormat,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can simultaneously handle different video formats on more than one SDI input or output.
	kDeviceCanDoPCMControl,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device audio system(s) can disable PCM (Pulse Code Modulation) normalization on a per-channel-pair basis.
	kDeviceCanDoPCMDetection,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has per-audio-channel-pair PCM detection capabilities.
//	kDeviceCanDoPIO,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device supports Programmed I/O.
	kDeviceCanDoPlayback,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can output (play) video.
	kDeviceCanDoProgrammableCSC,				///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has at least one programmable color space converter widget.
	kDeviceCanDoProgrammableRS422,				///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has at least one RS-422 serial port, and it (they) can be programmed (for baud rate, parity, etc.).
	kDeviceCanDoProRes,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can can accommodate Apple ProRes-compressed video in its frame buffers.
	kDeviceCanDoQREZ,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can handle QRez.
	kDeviceCanDoQuarterExpand,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can handle quarter-sized frames (pixel-halving and line-halving during input, pixel-double and line-double during output).
//	kDeviceCanDoRateConvert,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can do frame rate conversion.
//	kDeviceCanDoRGBPlusAlphaOut,				///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has CSCs capable of splitting the key (alpha) and YCbCr (fill) from RGB frame buffers that include alpha. (Has nothing to do with RGB wire formats.)
//	kDeviceCanDoRP188,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can insert and/or extract RP-188/VITC.
//	kDeviceCanDoSDVideo,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can handle SD (Standard Definition) video.
	kDeviceCanDoSDIErrorChecks,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can perform SDI error checking.
//	kDeviceCanDoStackedAudio,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device uses a "stacked" arrangement of its audio buffers.
//	kDeviceCanDoStereoIn,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device supports 3D video input over dual-stream SDI.
//	kDeviceCanDoStereoOut,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device supports 3D video output over dual-stream SDI.
	kDeviceCanDoThunderbolt,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device connects to the host using a Thunderbolt cable.
	kDeviceCanDoVideoProcessing,				///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can do video processing.
	kDeviceCanMeasureTemperature,				///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can measure its temperature.
	kDeviceCanReportFrameSize,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can report its frame size.
	kDeviceHasBiDirectionalSDI,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device SDI connectors are bi-directional.
//	kDeviceHasColorSpaceConverterOnChannel2,	///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has a CSC on channel 2.
	kDeviceHasNWL,								///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has NorthWest Logic DMA hardware.
	kDeviceHasPCIeGen2,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device supports 2nd-generation PCIe.
	kDeviceHasRetailSupport,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can be configured and controlled by the retail services and AJA ControlPanel.
	kDeviceHasSDIRelays,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has bypass relays on its SDI connectors.
//	kDeviceHasSPIFlash,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has SPI flash hardware.
//	kDeviceHasSPIFlashSerial,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has serial SPI flash hardware.
	kDeviceHasSPIv2,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device uses version 2 SPI hardware.
	kDeviceHasSPIv3,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device uses version 3 SPI hardware.
	kDeviceHasSPIv4,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device uses version 4 SPI hardware.
//	kDeviceIs64Bit,								///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device is 64-bit addressable.
//	kDeviceIsDirectAddressable,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device is direct addressable.
	kDeviceIsExternalToHost,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device connects to the host with a cable.
	kDeviceIsSupported,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device is supported by this SDK.
//	kDeviceNeedsRoutingSetup,					///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device widget routing can be queried and/or changed.
	kDeviceSoftwareCanChangeFrameBufferSize,	///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device frame buffer size can be changed.
	kDeviceCanThermostat,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the fan on the device can be thermostatically controlled.
	kDeviceHasHEVCM31,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has an HEVC M31 encoder.
	kDeviceHasHEVCM30,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device has an HEVC M30 encoder/decoder.
	kDeviceCanDoVITC2,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device can insert and/or extract RP-188/VITC2.
	kDeviceCanDoHDMIHDROut,						///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device supports HDMI HDR output.
	kDeviceCanDoJ2K,							///< @brief	Use with CNTV2Card::GetBoolParam to determine if the device supports the JPEG 2000 codec.
	kDeviceCanDo_INVALID
} NTV2BoolParamID;

/**
	@brief	Used in calls to CNTV2Card::GetNumericParam to determine numeric device features.
**/
typedef enum _NTV2NumericParamID
{
	kDeviceGetActiveMemorySize,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the size, in bytes, of the device's active RAM available for video and audio.
	kDeviceGetDACVersion,						///< @brief	Use with CNTV2Card::GetNumericParam to obtain the version number of the DAC on the device.
	kDeviceGetDownConverterDelay,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the down-converter delay on the device.
	kDeviceGetHDMIVersion,						///< @brief	Use with CNTV2Card::GetNumericParam to obtain the version number of the HDMI input(s) and/or output(s) on the device.
	kDeviceGetLUTVersion,						///< @brief	Use with CNTV2Card::GetNumericParam to obtain the version number of the LUT(s) on the device.
	kDeviceGetMaxAudioChannels,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the maximum number of audio channels that a single audio system can support on the device.
	kDeviceGetMaxRegisterNumber,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the highest register number for the device.
	kDeviceGetMaxTransferCount,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the maximum number of 32-bit words that the DMA engine can move at a time on the device.
	kDeviceGetNumDMAEngines,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of DMA engines on the device.
	kDeviceGetNumVideoChannels,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of video channels supported on the device.
	kDeviceGetPingLED,							///< @brief	Use with CNTV2Card::GetNumericParam to obtain the highest bit number of the LED bits in the Global Control Register on the device.
	kDeviceGetUFCVersion,						///< @brief	Use with CNTV2Card::GetNumericParam to obtain the version number of the UFC on the device.
	kDeviceGetNum4kQuarterSizeConverters,		///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of quarter-size 4K/UHD down-converters on the device.
	kDeviceGetNumAESAudioInputChannels,			///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of AES/EBU audio input channels on the device.
	kDeviceGetNumAESAudioOutputChannels,		///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of AES/EBU audio output channels on the device.
	kDeviceGetNumAnalogAudioInputChannels,		///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of analog audio input channels on the device.
	kDeviceGetNumAnalogAudioOutputChannels,		///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of analog audio output channels on the device.
	kDeviceGetNumAnalogVideoInputs,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of analog video inputs on the device.
	kDeviceGetNumAnalogVideoOutputs,			///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of analog video outputs on the device.
	kDeviceGetNumAudioSystems,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of independent audio systems on the device.
	kDeviceGetNumCrossConverters,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of cross-converters on the device.
	kDeviceGetNumCSCs,							///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of colorspace converter widgets on the device.
	kDeviceGetNumDownConverters,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of down-converters on the device.
	kDeviceGetNumEmbeddedAudioInputChannels,	///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of SDI-embedded input audio channels supported by the device.
	kDeviceGetNumEmbeddedAudioOutputChannels,	///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of SDI-embedded output audio channels supported by the device.
	kDeviceGetNumFrameStores,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of frame stores on the device.
	kDeviceGetNumFrameSyncs,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of frame sync widgets on the device.
	kDeviceGetNumHDMIAudioInputChannels,		///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of HDMI audio input channels on the device.
	kDeviceGetNumHDMIAudioOutputChannels,		///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of HDMI audio output channels on the device.
	kDeviceGetNumHDMIVideoInputs,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of HDMI video inputs on the device.
	kDeviceGetNumHDMIVideoOutputs,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of HDMI video outputs on the device.
	kDeviceGetNumInputConverters,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of input converter widgets on the device.
	kDeviceGetNumLUTs,							///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of LUT widgets on the device.
	kDeviceGetNumMixers,						///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of mixer/keyer widgets on the device.
	kDeviceGetNumOutputConverters,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of output converter widgets on the device.
	kDeviceGetNumReferenceVideoInputs,			///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of reference video inputs on the device.
	kDeviceGetNumSerialPorts,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of RS-422 serial ports on the device.
	kDeviceGetNumUpConverters,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of up-converters on the device.
	kDeviceGetNumVideoInputs,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of SDI video inputs on the device.
	kDeviceGetNumVideoOutputs,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of SDI video outputs on the device.
	kDeviceGetNum2022ChannelsSFP1,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of 2022 channels configured on SFP 1 on the device.
	kDeviceGetNum2022ChannelsSFP2,				///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of 2022 channels configured on SFP 2 on the device.
	kDeviceGetNumLTCInputs,						///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of analog LTC inputs on the device.
	kDeviceGetNumLTCOutputs,					///< @brief	Use with CNTV2Card::GetNumericParam to obtain the number of analog LTC outputs on the device.
	kDeviceGetNum_INVALID
} NTV2NumericParamID;


typedef std::set <NTV2AudioChannelPair>			NTV2AudioChannelPairs;			/// @brief	A set of distinct NTV2AudioChannelPair values.
typedef NTV2AudioChannelPairs::const_iterator	NTV2AudioChannelPairsConstIter;	/// @brief	Handy const iterator to iterate over a set of distinct NTV2AudioChannelPair values.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioChannelPairs & inSet);	///<	@brief	Handy ostream writer for NTV2AudioChannelPairs.

typedef std::set <NTV2AudioChannelQuad>			NTV2AudioChannelQuads;			/// @brief	A set of distinct NTV2AudioChannelQuad values.
typedef NTV2AudioChannelQuads::const_iterator	NTV2AudioChannelQuadsConstIter;	/// @brief	Handy const iterator to iterate over a set of distinct NTV2AudioChannelQuad values.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioChannelQuads & inSet);	///<	@brief	Handy ostream writer for NTV2AudioChannelQuads.

typedef std::set <NTV2AudioChannelOctet>		NTV2AudioChannelOctets;			/// @brief	A set of distinct NTV2AudioChannelOctet values.
typedef NTV2AudioChannelOctets::const_iterator	NTV2AudioChannelOctetsConstIter;/// @brief	Handy const iterator to iterate over a set of distinct NTV2AudioChannelOctet values.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioChannelOctets & inSet);	///<	@brief	Handy ostream writer for NTV2AudioChannelOctets.

typedef std::vector <double>					NTV2DoubleArray;				/// @brief	An array of double-precision floating-point values.
typedef NTV2DoubleArray::iterator				NTV2DoubleArrayIter;			/// @brief	Handy non-const iterator to iterate over an NTV2DoubleArray.
typedef NTV2DoubleArray::const_iterator			NTV2DoubleArrayConstIter;		/// @brief	Handy const iterator to iterate over an NTV2DoubleArray.
AJAExport std::ostream &	operator << (std::ostream & inOutStr, const NTV2DoubleArray & inVector);	///<	@brief	Handy ostream writer for NTV2DoubleArray.


//////////////////////////////////////////////////////////
//////////  From ntv2vidproc.h              //////////////
//////////////////////////////////////////////////////////
#ifdef AJALinux
	typedef unsigned int AJARgb;
	const AJARgb  AJA_RGB_MASK    = 0x00ffffff;		// masks RGB values

	inline int ajaRed( AJARgb rgb )		// get red part of RGB
	{ return (int)((rgb >> 16) & 0xff); }

	inline int ajaGreen( AJARgb rgb )		// get green part of RGB
	{ return (int)((rgb >> 8) & 0xff); }

	inline int ajaBlue( AJARgb rgb )		// get blue part of RGB
	{ return (int)(rgb & 0xff); }

	inline int ajaAlpha( AJARgb rgb )		// get alpha part of RGBA
	{ return (int)((rgb >> 24) & 0xff); }

	inline AJARgb ajaRgb( int r, int g, int b )// set RGB value
	{ return (0xff << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff); }

	inline AJARgb ajaRgba( int r, int g, int b, int a )// set RGBA value
	{ return ((a & 0xff) << 24) | ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff); }

	inline int ajaGray( int r, int g, int b )// convert R,G,B to gray 0..255
	{ return (r*11+g*16+b*5)/32; }

	inline int ajaGray( AJARgb rgb )		// convert RGB to gray 0..255
	{ return ajaGray( ajaRed(rgb), ajaGreen(rgb), ajaBlue(rgb) ); }
#endif	//	AJALinux
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////
//////////	From CNTV2TestPattern			//////////////
//////////////////////////////////////////////////////////
	typedef std::vector <const char *>			TestPatternList;

	typedef struct
	{
		int				startLine;
		int				endLine;
		const ULWord *	data;
	} SegmentDescriptor;

	const UWord NumTestPatternSegments = 8;

	typedef struct
	{
		const char *		name;
		SegmentDescriptor	segmentDescriptor [NTV2_NUM_STANDARDS] [NumTestPatternSegments];
	} SegmentTestPatternData;
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////



/**
	@brief	I interrogate and control an AJA video/audio capture/playout device.
**/
#if defined (MSWindows)
	class AJAExport CNTV2Card	: public CNTV2WinDriverInterface
#elif defined (AJAMac)
	class CNTV2Card				: public CNTV2MacDriverInterface
#elif defined (AJALinux)
	class CNTV2Card				: public CNTV2LinuxDriverInterface
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
		@brief	Constructs me from the given parameters.
		@param[in]	inDeviceIndex	A zero-based index number that identifies which device to open,
									which should be the number received from the NTV2DeviceScanner.
		@param[in]	inDisplayError	If true, displays a message box if there's a failure while opening.
									This parameter is obsolete and won't be available in the future.
		@param[in]	inDeviceType	Specifies the NTV2DeviceType of the device to open.
									This parameter is obsolete and won't be available in the future.
		@param[in]	pInHostName		If non-NULL, must be a valid pointer to a character buffer that
									contains the name of a host that has one or more AJA devices.
									Defaults to NULL (the local host).
		@nosubgrouping
	**/
	explicit							CNTV2Card ( const UWord		inDeviceIndex,
													const bool		inDisplayError	= false,
													const UWord		inDeviceType	= DEVICETYPE_NTV2,
													const char *	pInHostName		= 0);
	/**
		@brief	My destructor.
	**/
	virtual								~CNTV2Card();
	///@}


	/**
		@name	Opening & Closing
	**/
	///@{
	#if !defined (NTV2_DEPRECATE)
		virtual NTV2_DEPRECATED	bool	SetBoard (UWord inDeviceIndex);		///< @deprecated	Use CNTV2DeviceScanner or Open(deviceIndex) instead.
	#endif	//	!defined (NTV2_DEPRECATE)
	///@}

	/**
		@name	Inquiry
	**/
	///@{

	/**
		@brief	Answers with a 4-byte value that uniquely identifies the kind of AJA device I'm talking to.
		@return	The 4-byte value that identifies the kind of AJA device this is.
	**/
	AJA_VIRTUAL NTV2DeviceID		GetDeviceID (void);

	/**
		@brief	Answers with this device's zero-based index number (relative to other known devices).
		@return	This device's zero-based index number.
	**/
	AJA_VIRTUAL inline UWord		GetIndexNumber (void) const		{return _boardNumber;}

	/**
		@brief	Answers with this device's display name.
		@return	A string containing this device's display name.
	**/
	AJA_VIRTUAL std::string			GetDisplayName (void);

	/**
		@brief	Answers with this device's version number.
		@return	This device's version number.
	**/
	AJA_VIRTUAL Word				GetDeviceVersion (void);

	/**
		@brief	Answers with this device's version number as a human-readable string.
		@return	A string containing this device's version number as a human-readable string.
	**/
	AJA_VIRTUAL std::string			GetDeviceVersionString (void);

	/**
		@brief	Answers with this device's driver's version as a human-readable string.
		@return	A string containing this device's driver's version as a human-readable string.
	**/
	AJA_VIRTUAL std::string			GetDriverVersionString (void);

	/**
		@brief	Answers with the individual version components of this device's driver.
		@param[out]	outMajor	Receives the driver's major version number.
		@param[out]	outMinor	Receives the driver's minor version number.
		@param[out]	outPoint	Receives the driver's point release number.
		@param[out]	outBuild	Receives the driver's build number.
		@return	True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool				GetDriverVersionComponents (UWord & outMajor, UWord & outMinor, UWord & outPoint, UWord & outBuild);

	/**
		@brief	Answers with my serial number.
		@return	My 64-bit serial number.
		@note	To decode this into a human-readable form, use my SerialNum64ToString class method.
	**/
	AJA_VIRTUAL uint64_t			GetSerialNumber (void);											//	From CNTV2Status

	/**
		@brief	Answers with a string that contains my human-readable serial number.
		@return	True if successful (and valid);  otherwise false.
	**/
	AJA_VIRTUAL bool				GetSerialNumberString (std::string & outSerialNumberString);	//	From CNTV2Status

	/**
		@brief	Answers with my PCI device ID.
		@param[out]		outPCIDeviceID		Receives my PCI device ID.
		@return	True if successful (and valid);  otherwise false.
	**/
	AJA_VIRTUAL bool				GetPCIDeviceID (ULWord & outPCIDeviceID);


	/**
		@return	My current breakout box hardware type, if any is attached.
	**/
	AJA_VIRTUAL NTV2BreakoutType	GetBreakoutHardware (void);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED	inline NTV2BoardID	GetBoardID (void)				{return GetDeviceID ();}		///< @deprecated	Use GetDeviceID instead.
		AJA_VIRTUAL NTV2_DEPRECATED	inline UWord		GetBoardNumber (void) const		{return GetIndexNumber ();}		///< @deprecated	Use GetIndexNumber instead.
		AJA_VIRTUAL NTV2_DEPRECATED	NTV2BoardType		GetBoardType (void) const;										///< @deprecated	NTV2BoardType is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED	NTV2BoardSubType	GetBoardSubType (void);											///< @deprecated	NTV2BoardSubType is obsolete.
		static NTV2_DEPRECATED		UWord				GetNumNTV2Boards (void);										///< @deprecated	Use CNTV2DeviceScanner instead.
	#endif	//	!defined (NTV2_DEPRECATE)
	///@}


	/**
		@name	Device Features
	**/
	///@{
	AJA_VIRTUAL bool	DeviceCanDoFormat (NTV2FrameRate		inFrameRate,
											NTV2FrameGeometry	inFrameGeometry, 
											NTV2Standard		inStandard);
	AJA_VIRTUAL bool	DeviceCanDo3GOut (UWord index0);
	AJA_VIRTUAL bool	DeviceCanDoLTCEmbeddedN (UWord index0);
	AJA_VIRTUAL ULWord	DeviceGetFrameBufferSize (void);		//	Revisit for 2MB granularity
	AJA_VIRTUAL ULWord	DeviceGetNumberFrameBuffers (void);		//	Revisit for 2MB granularity
	AJA_VIRTUAL ULWord	DeviceGetAudioFrameBuffer (void);		//	Revisit for 2MB granularity
	AJA_VIRTUAL ULWord	DeviceGetAudioFrameBuffer2 (void);		//	Revisit for 2MB granularity
	AJA_VIRTUAL ULWord	DeviceGetFrameBufferSize (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat);	//	Revisit for 2MB granularity
	AJA_VIRTUAL ULWord	DeviceGetNumberFrameBuffers (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat);	//	Revisit for 2MB granularity
	AJA_VIRTUAL ULWord	DeviceGetAudioFrameBuffer (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat);	//	Revisit for 2MB granularity
	AJA_VIRTUAL ULWord	DeviceGetAudioFrameBuffer2 (const NTV2FrameGeometry inFrameGeometry, const NTV2FrameBufferFormat inFBFormat);	//	Revisit for 2MB granularity

	/**
		@brief		Returns true if the device having the given ID supports the given NTV2VideoFormat.
		@param[in]	inVideoFormat	Specifies the NTV2VideoFormat.
		@return		True if the device supports the given video format.
	**/
	AJA_VIRTUAL bool	DeviceCanDoVideoFormat (const NTV2VideoFormat inVideoFormat);

	/**
		@brief		Returns true if the device having the given ID supports the given NTV2FrameBufferFormat.
		@param[in]	inFBFormat		Specifies the NTV2FrameBufferFormat.
		@return		True if the device supports the given frame buffer (pixel) format.
	**/
	AJA_VIRTUAL bool	DeviceCanDoFrameBufferFormat (const NTV2FrameBufferFormat inFBFormat);

	/**
		@brief		Returns true if the device having the given ID supports the given NTV2WidgetID.
		@param[in]	inWidgetID		Specifies the NTV2WidgetID.
		@return		True if the device supports the given widget.
	**/
	AJA_VIRTUAL bool	DeviceCanDoWidget (const NTV2WidgetID inWidgetID);

	/**
		@brief		Returns true if the device having the given ID supports the given NTV2ConversionMode.
		@param[in]	inConversionMode	Specifies the NTV2ConversionMode.
		@return		True if the device supports the given conversion mode.
	**/
	AJA_VIRTUAL bool	DeviceCanDoConversionMode (const NTV2ConversionMode inConversionMode);

	/**
		@brief		Returns true if the device having the given ID supports the given NTV2DSKMode.
		@param[in]	inDSKMode		Specifies the NTV2DSKMode.
		@return		True if the device supports the given DSK mode.
	**/
	AJA_VIRTUAL bool	DeviceCanDoDSKMode (const NTV2DSKMode inDSKMode);

	/**
		@brief		Returns true if the device having the given ID supports the given NTV2InputSource.
		@param[in]	inInputSource	Specifies the NTV2InputSource.
		@return		True if the device supports the given input source.
	**/
	AJA_VIRTUAL bool	DeviceCanDoInputSource (const NTV2InputSource inInputSource);

	/**
		@brief		Fetches the requested boolean value. Typically called to determine device features.
		@param[in]	inParamID	Specifies the NTV2BoolParamID of interest.
		@param[out]	outValue	Receives the requested boolean value.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetBoolParam (const NTV2BoolParamID inParamID, bool & outValue);

	/**
		@brief		Fetches the requested numeric value. Typically called to determine device features.
		@param[in]	inParamID	Specifies the NTV2NumericParamID of interest.
		@param[out]	outValue	Receives the requested numeric value.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetNumericParam (const NTV2NumericParamID inParamID, uint32_t & outValue);
	///@}


	/**
		@name	DMA Transfer
	**/
	///@{
	/**
		@brief		Transfers data from the AJA device to the host.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the starting frame to be read from the device.
		@param[in]	pFrameBuffer	Specifies the non-NULL address of the host buffer that is to receive the frame data.
									The memory it points to must be writeable.
		@param[in]	inOffsetBytes	Specifies the byte offset into the device frame buffer.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@return		True if successful; otherwise false.
		@note		The host buffer should be at least inByteCount + inOffsetBytes in size, or host memory will be corrupted.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMARead (const ULWord inFrameNumber, ULWord * pFrameBuffer, const ULWord inOffsetBytes, const ULWord inByteCount);

	/**
		@brief		Transfers data from the host to the AJA device.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame to be written on the device.
		@param[in]	pFrameBuffer	Specifies the non-NULL address of the host buffer that is to supply the frame data.
									The memory it points to must be readable.
		@param[in]	inOffsetBytes	Specifies the byte offset into the device frame buffer.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@return		True if successful; otherwise false.
		@note		The host buffer should be at least inByteCount + inOffsetBytes in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAWrite (const ULWord inFrameNumber, const ULWord * pFrameBuffer, const ULWord inOffsetBytes, const ULWord inByteCount);


	/**
		@brief		Transfers a single frame from the AJA device to the host.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame to be read from the device.
		@param[in]	pOutFrameBuffer	Specifies the non-NULL address of the host buffer that is to receive the frame data.
									The memory it points to must be writeable.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@return		True if successful; otherwise false.
		@note		The host buffer must be at least inByteCount in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAReadFrame (const ULWord inFrameNumber, ULWord * pOutFrameBuffer, const ULWord inByteCount);

	/**
		@brief		Transfers a single frame from the host to the AJA device.
		@param[in]	inFrameNumber	Specifies the zero-based frame number of the frame to be written to the device.
		@param[in]	pInFrameBuffer	Specifies the non-NULL address of the host buffer that is to supply the frame data.
									The memory it points to must be readable.
		@param[in]	inByteCount		Specifies the total number of bytes to transfer.
		@return		True if successful; otherwise false.
		@note		The host buffer must be at least inByteCount in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAWriteFrame (const ULWord inFrameNumber, const ULWord * pInFrameBuffer, const ULWord inByteCount);

	/**
		@brief		Performs a segmented data transfer from the AJA device to the host.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame to be read from the device.
		@param[in]	pFrameBuffer		Specifies the non-NULL address of the host buffer that is to supply the frame data.
										The memory it points to must be writeable.
		@param[in]	inOffsetBytes		Specifies the initial device memory byte offset for the first bytes transferred.
		@param[in]	inByteCount			Specifies the total number of bytes to transfer.
		@param[in]	inNumSegments		Specifies the number of segments to transfer.
		@param[in]	inSegmentHostPitch	Specifies the number of bytes to increment the host memory pointer after each segment is transferred.
		@param[in]	inSegmentCardPitch	Specifies the number of bytes to increment the on-device memory pointer after each segment is transferred.
		@return		True if successful; otherwise false.
		@note		The host buffer should be at least inByteCount + inOffsetBytes in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAReadSegments (	const ULWord		inFrameNumber,
											ULWord *			pFrameBuffer,
											const ULWord		inOffsetBytes,
											const ULWord		inByteCount,
											const ULWord		inNumSegments,
											const ULWord		inSegmentHostPitch,
											const ULWord		inSegmentCardPitch);

	/**
		@brief		Performs a segmented data transfer from the host to the AJA device.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame to be written on the device.
		@param[in]	pFrameBuffer		Specifies the non-NULL address of the host buffer that is to supply the frame data.
										The memory it points to must be readable.
		@param[in]	inOffsetBytes		Specifies the initial device memory byte offset for the first bytes transferred.
		@param[in]	inByteCount			Specifies the total number of bytes to transfer.
		@param[in]	inNumSegments		Specifies the number of segments to transfer.
		@param[in]	inSegmentHostPitch	Specifies the number of bytes to increment the host memory pointer after each segment is transferred.
		@param[in]	inSegmentCardPitch	Specifies the number of bytes to increment the on-device memory pointer after each segment is transferred.
		@return		True if successful; otherwise false.
		@note		The host buffer should be at least inByteCount + inOffsetBytes in size, or a host memory access violation may occur.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAWriteSegments (	const ULWord		inFrameNumber,
											const ULWord *		pFrameBuffer,
											const ULWord		inOffsetBytes,
											const ULWord		inByteCount,
											const ULWord		inNumSegments,
											const ULWord		inSegmentHostPitch,
											const ULWord		inSegmentCardPitch);

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
		@brief		Transfers audio data from a given audio system on the AJA device to the host.
		@param[in]	inAudioEngine		Specifies the audio engine on the device that is to supply the audio data.
		@param		pOutAudioBuffer		Specifies a valid, non-NULL pointer to the host buffer that is to receive the audio data.
										This buffer must be large enough to accommodate "inByteCount" bytes of data specified (below).
		@param[in]	inOffsetBytes		Specifies the offset into the audio engine's capture buffer on the device from which to transfer audio data.
		@param[in]	inByteCount			Specifies the number of audio bytes to transfer.
		@return		True if successful; otherwise false.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAReadAudio (	const NTV2AudioSystem	inAudioEngine,
										ULWord *				pOutAudioBuffer,
										const ULWord			inOffsetBytes,
										const ULWord			inByteCount);

	/**
		@brief		Transfers audio data from a given host buffer to a specific audio system's playout buffer on the AJA device.
		@param[in]	inAudioEngine		Specifies the audio engine on the device that is to receive the audio data.
		@param[in]	pInAudioBuffer		Specifies a valid, non-NULL pointer to the host buffer that is to supply the audio data.
		@param[in]	inOffsetBytes		Specifies the offset into the audio engine's playout buffer on the device to which audio data will be transferred.
		@param[in]	inByteCount			Specifies the number of audio bytes to transfer. Note that this value must not overrun the host
										buffer, nor the device's audio playout buffer.
		@return		True if successful; otherwise false.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAWriteAudio (	const NTV2AudioSystem	inAudioEngine,
										const ULWord *			pInAudioBuffer,
										const ULWord			inOffsetBytes,
										const ULWord			inByteCount);

	/**
		@brief		Transfers ancillary data from a given field/frame on the AJA device to the host.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame to be read from the device.
		@param		pOutAncBuffer		Specifies a valid, non-NULL pointer to the host buffer that is to receive the ancillary data.
										This buffer must be large enough to accommodate "inByteCount" bytes of data specified (below).
		@param[in]	inFieldID			Specifies the field of interest. Use NTV2_FIELD0 for progressive formats. Defaults to NTV2_FIELD0.
		@param[in]	inByteCount			Specifies the number of bytes to transfer. Note that this value must not overrun the host
										buffer, nor the device's frame buffer. Defaults to 2K.
		@return		True if successful; otherwise false.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAReadAnc (	const ULWord			inFrameNumber,
										UByte *					pOutAncBuffer,
										const NTV2FieldID		inFieldID		= NTV2_FIELD0,
										const ULWord			inByteCount		= 2048);

	/**
		@brief		Transfers ancillary data from a given host buffer to a specific field/frame buffer on the AJA device.
		@param[in]	inFrameNumber		Specifies the zero-based frame number of the frame to be written on the device.
		@param[in]	pInAncBuffer		Specifies a valid, non-NULL pointer to the host buffer that is to supply the ancillary data to
										be written.
		@param[in]	inFieldID			Specifies the field of interest. Use NTV2_FIELD0 for progressive formats. Defaults to NTV2_FIELD0.
		@param[in]	inByteCount			Specifies the number of bytes to transfer. Note that this value must not overrun the host
										buffer, nor the device's frame buffer. Defaults to 2K.
		@return		True if successful; otherwise false.
		@note		This function will block and not return until the transfer has finished or failed.
	**/
	AJA_VIRTUAL bool	DMAWriteAnc (	const ULWord			inFrameNumber,
										const UByte *			pInAncBuffer,
										const NTV2FieldID		inFieldID		= NTV2_FIELD0,
										const ULWord			inByteCount		= 2048);


	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	DmaRead (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
														const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous = true);	///< @deprecated	Use DMARead instead.

		AJA_VIRTUAL NTV2_DEPRECATED bool	DmaWrite (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
														const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous = true);	///< @deprecated	Use DMAWrite instead.

		AJA_VIRTUAL NTV2_DEPRECATED bool	DmaReadFrame (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
															const ULWord inByteCount, const bool inSynchronous = true);	///< @deprecated	Use DMAReadFrame instead.

		AJA_VIRTUAL NTV2_DEPRECATED bool	DmaWriteFrame (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
															const ULWord inByteCount, const bool inSynchronous = true);	///< @deprecated	Use DMAWriteFrame instead.

		AJA_VIRTUAL NTV2_DEPRECATED bool	DmaReadSegment (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
															const ULWord inOffsetBytes, const ULWord inByteCount,
															const ULWord inNumSegments, const ULWord inSegmentHostPitch, const ULWord inSegmentCardPitch,
															const bool inSynchronous = true);	///< @deprecated	Use DMAReadSegments instead.

		AJA_VIRTUAL NTV2_DEPRECATED bool	DmaWriteSegment (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
															const ULWord inOffsetBytes, const ULWord inByteCount,
															const ULWord inNumSegments, const ULWord inSegmentHostPitch, const ULWord inSegmentCardPitch,
															const bool inSynchronous = true);	///< @deprecated	Use DMAWriteSegments instead.

		AJA_VIRTUAL NTV2_DEPRECATED bool	DmaAudioRead (	const NTV2DMAEngine		inDMAEngine,
															const NTV2AudioSystem	inAudioEngine,
															ULWord *				pOutAudioBuffer,
															const ULWord			inOffsetBytes,
															const ULWord			inByteCount,
															const bool				inSynchronous = true);	///< @deprecated	Use DMAReadAudio instead.

		AJA_VIRTUAL NTV2_DEPRECATED bool	DmaAudioWrite (	const NTV2DMAEngine		inDMAEngine,
															const NTV2AudioSystem	inAudioEngine,
															const ULWord *			pInAudioBuffer,
															const ULWord			inOffsetBytes,
															const ULWord			inByteCount,
															const bool				inSynchronous = true);	///< @deprecated	Use DMAWriteAudio instead.

		AJA_VIRTUAL NTV2_DEPRECATED	bool	DmaReadField (NTV2DMAEngine DMAEngine, ULWord frameNumber, NTV2FieldID fieldID, ULWord *pFrameBuffer,
											ULWord bytes, bool bSync = true);	///< @deprecated	This function is obsolete, as no current AJA devices use non-interleaved fields.
		AJA_VIRTUAL NTV2_DEPRECATED	bool	DmaWriteField (NTV2DMAEngine DMAEngine, ULWord frameNumber, NTV2FieldID fieldID, ULWord *pFrameBuffer,
											ULWord bytes, bool bSync = true);	///< @deprecated	This function is obsolete, as no current AJA devices use non-interleaved fields.
	#endif	//	!defined (NTV2_DEPRECATE)
	///@}

//
//	 Set/Get Parameter routines
//
	#if defined (AJAMac)
		#define	AJA_RETAIL_DEFAULT	true
	#else	//	else !defined (AJAMac)
		#define	AJA_RETAIL_DEFAULT	false
	#endif	//	!defined (AJAMac)

	/**
		@brief		Configures the AJA device to handle a specific video format.
		@param[in]	inVideoFormat			Specifies the desired video format for the given channel on the device.
											It must be a valid NTV2VideoFormat constant.
		@param[in]	inIsAJARetail			Specify 'true' to preserve the current horizontal and vertical timing settings.
											Defaults to true on MacOS, false on other platforms.
		@param[in]	inKeepVancSettings		If true, specifies that the device's current VANC settings are to be preserved;
											otherwise, they will not be preserved. Defaults to false.
		@param[in]	inChannel				Specifies the NTV2Channel of interest. Defaults to NTV2_CHANNEL1.
											For UHD/4K video formats, specify NTV2_CHANNEL1 to configure quadrant channels 1-4,
											or NTV2_CHANNEL5 to configure quadrant channels 5-8.
		@return		True if successful; otherwise false.
		@details	This function changes the device configuration to a specific video standard (e.g., 525, 1080, etc.),
					frame geometry (e.g., 1920x1080, 720x486, etc.) and frame rate (e.g., 59.94 fps, 29.97 fps, etc.),
					plus a few other settings (e.g., progressive/interlaced, etc.), all based on the given video format.
	**/
	AJA_VIRTUAL bool	SetVideoFormat (NTV2VideoFormat inVideoFormat, bool inIsAJARetail = AJA_RETAIL_DEFAULT, bool inKeepVancSettings = false, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Sets the frame geometry of the given channel.
		@param[in]	inGeometry		Specifies the desired frame geometry. It must be a valid NTV2FrameGeometry value.
		@param[in]	inIsRetail		This parameter is ignored.
		@param[in]	inChannel		Specifies the NTV2Channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	SetFrameGeometry (NTV2FrameGeometry inGeometry, bool inIsRetail = AJA_RETAIL_DEFAULT, NTV2Channel inChannel = NTV2_CHANNEL1);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED	bool	SetReferenceSource (NTV2ReferenceSource value, bool ajaRetail = AJA_RETAIL_DEFAULT);	///< @deprecated	Use SetReference instead.
		AJA_VIRTUAL NTV2_DEPRECATED	bool	GetReferenceSource (NTV2ReferenceSource* value, bool ajaRetail = AJA_RETAIL_DEFAULT);	///< @deprecated	Use GetReference instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		Sets the frame buffer format for the given frame store on the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inChannel			Specifies the frame store to be affected, which must be one of NTV2_CHANNEL1,
									NTV2_CHANNEL2, NTV2_CHANNEL3, or NTV2_CHANNEL4.
		@param[in]	inNewFormat		Specifies the desired frame buffer format.
									This must be a valid NTV2FrameBufferFormat value.
		@param[in]	inIsAJARetail	Specifies if the AJA retail configuration settings are to be respected or not.
									Defaults to false on all platforms other than MacOS, which defaults to true.
		@details	This function allows client applications to control the format of frame data stored
					in the frame stores on an AJA device. This is important, because when frames are transferred
					between the host and the AJA device, the frame data format is presumed to be identical.
	**/
	AJA_VIRTUAL bool	SetFrameBufferFormat (NTV2Channel inChannel, NTV2FrameBufferFormat inNewFormat, bool inIsAJARetail = AJA_RETAIL_DEFAULT);


	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool			UpdateK2ColorSpaceMatrixSelect (NTV2VideoFormat currFormat = NTV2_FORMAT_UNKNOWN, bool ajaRetail = AJA_RETAIL_DEFAULT);	///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool			UpdateK2LUTSelect (NTV2VideoFormat currFormat = NTV2_FORMAT_UNKNOWN, bool ajaRetail = AJA_RETAIL_DEFAULT);	///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2BitfileType	BitfileSwitchNeeded (NTV2DeviceID deviceID, NTV2VideoFormat value, bool ajaRetail = AJA_RETAIL_DEFAULT);	///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)

	AJA_VIRTUAL bool		SetReference (NTV2ReferenceSource value);
	AJA_VIRTUAL bool		GetReference (NTV2ReferenceSource & outValue);
	AJA_VIRTUAL inline bool	GetReference (NTV2ReferenceSource * pOutValue)									{return pOutValue ? GetReference (*pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief		Retrieves the device's current retail service task mode.
		@return		True if successful; otherwise false.
		@param[out]	outMode		Receives the device's current "every frame task mode" setting. If successful, the
								variable will contain NTV2_DISABLE_TASKS, NTV2_STANDARD_TASKS, or NTV2_OEM_TASKS.
		@details	AJA's retail drivers come with a program that automatically and continuously configures the
					device once-per-frame using settings that are dictated by the AJA Control Panel application.
					The task runs as a service on Windows and as an agent on MacOS. It starts when a host user logs in,
					restores the device configuration to its last known state (as set by that user via the AJA Control Panel),
					then holds that setting while running in the background, until the user logs off the host.
					Some OEM applications cannot assume that the user's Control Panel settings will be valid for their
					proper operation, and thus may want to know if the retail service has control of the device.
	**/
	AJA_VIRTUAL bool		GetEveryFrameServices (NTV2EveryFrameTaskMode & outMode);
	AJA_VIRTUAL inline bool	GetEveryFrameServices (NTV2EveryFrameTaskMode * pOutMode)						{return pOutMode ? GetEveryFrameServices (*pOutMode) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief		Enables or disables all or part of the retail mode service task that continuously controls
					the "retail mode" device configuration.
		@return		True if successful; otherwise false.
		@param[in]	mode		Specifies the "every frame task mode" the device is to assume,
								and must be one of the following values: NTV2_DISABLE_TASKS, NTV2_STANDARD_TASKS, or NTV2_OEM_TASKS.
		@details	AJA's retail software provides a program that automatically and continuously configures the device once per frame
					using settings that are dictated by the AJA Control Panel application. This task runs as a service on Windows,
					and as an agent on MacOS X. It starts when a host user logs in, restores the device configuration to its last
					known state (as set by the user via the AJA Control Panel), then holds that setting while running in the background,
					until the user logs off the host. Some OEM applications cannot assume that the user's Control Panel settings
					will be valid for their proper operation, and thus will need to disable the service task as long as their
					application is running.
	**/
	AJA_VIRTUAL bool	SetEveryFrameServices (NTV2EveryFrameTaskMode mode);

	AJA_VIRTUAL bool		SetDefaultVideoOutMode (ULWord mode);
	AJA_VIRTUAL bool		GetDefaultVideoOutMode (ULWord & outMode);
	AJA_VIRTUAL inline bool	GetDefaultVideoOutMode (ULWord * pOutMode)								{return pOutMode ? GetDefaultVideoOutMode (*pOutMode) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief		Determines if a given frame store on the AJA device will be used to capture or playout video.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the NTV2Channel of interest (which corresponds to the Frame Store of interest).
		@param[in]	inNewValue		Specifies the desired mode for the frame store, which must be either NTV2_MODE_DISPLAY
									or NTV2_MODE_CAPTURE.
		@param[in]	inIsAJARetail	Specifies if the AJA retail configuration should be respected or not.
									Defaults to false on all platforms other than MacOS, which defaults to true.
		@note		Applications that acquire exclusive use of the AJA device, set its "every frame services" mode
					to NTV2_OEM_TASKS, and use AutoCirculate won't need to call this function, since AutoCirculate
					sets the frame store's mode automatically.
	**/
	AJA_VIRTUAL bool	SetMode (NTV2Channel inChannel, NTV2Mode inNewValue, bool inIsAJARetail = AJA_RETAIL_DEFAULT);

	/**
		@brief		Returns the current mode (capture or playout) of the given frame store on the AJA device.
		@param[in]	inChannel	Specifies the frame store of interest (NTV2_CHANNEL1 - NTV2_CHANNEL4).
		@param[out]	outValue	Receives the current mode for the channel. If the function result is true,
								it will contain either NTV2_MODE_DISPLAY or NTV2_MODE_CAPTURE.
		@details	A frame store can either be set to record/capture or display/playout.
					This function allows client applications to determine a frame store's mode.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool		GetMode (const NTV2Channel inChannel, NTV2Mode & outValue);
	AJA_VIRTUAL inline bool	GetMode (const NTV2Channel inChannel, NTV2Mode * pOutValue)				{return pOutValue ? GetMode (inChannel, *pOutValue) : false;}

	AJA_VIRTUAL bool	GetFrameGeometry (NTV2FrameGeometry & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetFrameGeometry (NTV2FrameGeometry * pOutValue, NTV2Channel inChannel = NTV2_CHANNEL1)		{return pOutValue ? GetFrameGeometry (*pOutValue, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief		Returns the current frame buffer format for the given frame store on the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the frame store (channel) of interest.
		@param[out]	outValue		Receives the frame store's current pixel format. If the function result is true,
									the variable will contain a valid NTV2FrameBufferFormat value.
		@details	This function allows client applications to inquire about the current format of frame data
					stored in an AJA device's frame store. This is important because when frames are transferred
					between the host and the AJA device, the frame data format is presumed to be identical.
	**/
	AJA_VIRTUAL bool		GetFrameBufferFormat (NTV2Channel inChannel, NTV2FrameBufferFormat & outValue);
	AJA_VIRTUAL inline bool	GetFrameBufferFormat (NTV2Channel inChannel, NTV2FrameBufferFormat * pOutValue)		{return pOutValue ? GetFrameBufferFormat (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.


	/**
		@brief		Returns a std::set of NTV2VideoFormat values that I support.
		@param[out]	outFormats	Receives the set of NTV2VideoFormat values.
								This will be empty if the function fails.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetSupportedVideoFormats (NTV2VideoFormatSet & outFormats);


	// The rest of the routines
	AJA_VIRTUAL bool		GetVideoFormat (NTV2VideoFormat & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL inline bool	GetVideoFormat (NTV2VideoFormat * pOutValue, NTV2Channel inChannel = NTV2_CHANNEL1)		{return pOutValue ? GetVideoFormat (*pOutValue, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool				GetActiveFrameDimensions (NTV2FrameDimensions & outFrameDimensions, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL NTV2FrameDimensions	GetActiveFrameDimensions (const NTV2Channel inChannel = NTV2_CHANNEL1);
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool GetActiveFramebufferSize (SIZE * pOutFrameDimensions, const NTV2Channel inChannel = NTV2_CHANNEL1);	///< @deprecated	Use GetActiveFrameDimensions instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		Sets the frame buffer size on those boards that allow software to select a video buffer size.
		@return		True if successful; otherwise false.
		@param[in]	size			Specifies the size of frame buffer the hardware should use.
		@details	The firmware will use a frame buffer size big enough to accommodate the largest possible frame
					for the frame buffer format and frame buffer geometry in use.  This can be wasteful, for example,
					when using an 8 bit YCbCr format with a "tall" frame geometry so that VANC can be processed.
					These frames will fit in 8MB, but the firmware will use a size of 16MB just in case the pixel
					format is changed to 48 bit RGB.  This function provides a way to force a given frame buffer size.
					Selecting a smaller size than that actually needed by the hardware will compromise video integrity.
	**/
	AJA_VIRTUAL bool		SetFrameBufferSize (NTV2Framesize size);

	AJA_VIRTUAL bool		GetNumberActiveLines (ULWord & outNumActiveLines);
	AJA_VIRTUAL inline bool	GetNumberActiveLines (ULWord * pOutNumActiveLines)			{return pOutNumActiveLines ? GetNumberActiveLines (*pOutNumActiveLines) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetStandard (NTV2Standard inValue, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool		GetStandard (NTV2Standard & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL inline bool	GetStandard (NTV2Standard * pOutValue, NTV2Channel inChannel = NTV2_CHANNEL1)	{return pOutValue ? GetStandard (*pOutValue, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		IsProgressiveStandard (bool & outIsProgressive, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL inline bool	IsProgressiveStandard (bool * pOutIsProgressive, NTV2Channel inChannel = NTV2_CHANNEL1)	{return pOutIsProgressive ? IsProgressiveStandard (*pOutIsProgressive, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		IsSDStandard (bool & outIsStandardDef, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL inline bool	IsSDStandard (bool * pOutIsStandardDef, NTV2Channel inChannel = NTV2_CHANNEL1)	{return pOutIsStandardDef ? IsSDStandard (*pOutIsStandardDef, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.
	#if !defined (NTV2_DEPRECATE)
		static NTV2_DEPRECATED bool	IsSDVideoADCMode (NTV2LSVideoADCMode mode);			///< @deprecated	This function is obsolete.
		static NTV2_DEPRECATED bool	IsHDVideoADCMode (NTV2LSVideoADCMode mode);			///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)
	AJA_VIRTUAL bool	IsBufferSizeSetBySW();

	/**
		@brief		Sets the AJA device's frame rate.
		@return		True if successful; otherwise false.
		@param[in]	inNewValue		Specifies the new NTV2FrameRate value the AJA device is to be configured with.
		@param[in]	inChannel		Specifies the NTV2Channel of interest.
		@details	This function changes bits 0, 1, 2 and 22 of the AJA device's Global Control Register (register 0),
					to change the device's frame rate configuration.
	**/
	AJA_VIRTUAL bool	SetFrameRate (NTV2FrameRate inNewValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Returns the AJA device's currently configured frame rate via its "value" parameter.
		@return		True if successful; otherwise false.
		@param[out]	outValue	Receives the device's current NTV2FrameRate value.
		@param[in]	inChannel	Specifies the NTV2Channel of interest.
		@details	This function queries the AJA device's Global Control Register (register 0), and inspects bits 0, 1, 2 and 22,
					to determine the device's current frame rate configuration.
	**/
	AJA_VIRTUAL bool		GetFrameRate (NTV2FrameRate & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL inline bool	GetFrameRate (NTV2FrameRate * pOutValue, NTV2Channel inChannel = NTV2_CHANNEL1)		{return pOutValue ? GetFrameRate (*pOutValue, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief		Enables or disables the device's SMPTE-372 (dual-link) mode (used for older 3G-levelB-capable devices).
		@note		This allows older devices to handle 1080p60/1080p5994/1080p50 signals by "ganging" two 30Hz frame stores. See \ref duallinkoverview for more information.
		@note		The enable bits work on channel pairs, thus a parameter of NTV2_CHANNEL1 or NTV2_CHANNEL2 refers to the same control bit.
		@return		True if successful; otherwise false.
		@param[in]	inValue		Specify a non-zero value (true) to put the device into SMPTE 372 dual-link mode.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1.
		@todo		Should use bool parameter instead of a ULWord.
		@todo		Should be named SetSMPTE372Enable.
	**/
	AJA_VIRTUAL bool		SetSmpte372 (ULWord inValue, NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Returns the device's current SMPTE-372 (dual-link) mode, whether it's enabled or not.
		@note		The enable bits work on channel pairs, thus a parameter of NTV2_CHANNEL1 or NTV2_CHANNEL2 refers to the same control bit.
		@return		True if successful; otherwise false.
		@param[in]	outValue	Receives 1 if the device is currently in dual-link mode;  otherwise receives 0.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1.
		@todo		Should use bool& parameter instead of a ULWord&.
		@todo		Should be named GetSMPTE372Enable.
	**/
	AJA_VIRTUAL bool		GetSmpte372 (ULWord & outValue, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL inline bool	GetSmpte372 (ULWord * pOutValue, NTV2Channel inChannel = NTV2_CHANNEL1)		{return pOutValue ? GetSmpte372 (*pOutValue, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetProgressivePicture (ULWord value);
	AJA_VIRTUAL bool		GetProgressivePicture (ULWord & outValue);
	AJA_VIRTUAL inline bool	GetProgressivePicture (ULWord * pOutValue)									{return pOutValue ? GetProgressivePicture (*pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief	Enables or disables quad-frame mode on the device.
		@return		True if successful; otherwise false.
		@param[in]	inValue		Specify a non-zero value (true) to put the device into quad frame mode.
								Specify zero (false) to put the device into normal (non-quad) frame mode.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1. Ignored if the device
								is incapable of multi-format mode, or is not currently in multi-format mode.
		@todo		Should use bool parameter.
		@note		Most clients need not call this function, as SetVideoFormat to one of the 4K/UHD formats will automatically call this function.
	**/
	AJA_VIRTUAL bool		SetQuadFrameEnable (const ULWord inValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief	Returns the device's current quad-frame mode, whether it's enabled or not.
		@return		True if successful; otherwise false.
		@param[in]	outValue	Receives 1 if the device is currently in quad frame mode; otherwise receives 0.
		@param[in]	inChannel	Specifies the channel of interest. Defaults to channel 1. Ignored if the device
								is incapable of multi-format mode, or is not currently in multi-format mode.
		@todo		Should use bool & parameter.
	**/
	AJA_VIRTUAL bool		GetQuadFrameEnable (ULWord & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL inline bool	GetQuadFrameEnable (ULWord * pOutValue, const NTV2Channel inChannel = NTV2_CHANNEL1)	{return pOutValue ? GetQuadFrameEnable (*pOutValue, inChannel) : false;}		///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief	Enables or disables SMPTE 425 "2K quadrants" mode for the given frame store on the device.
				Client applications should call this function only when Tsi is needed.
		@return		True if successful; otherwise false.
		@param[in]	inIsEnabled		Specify true to put the device's frame stores into "4K squares" (i.e., "2K quadrants") mode.
									Specify false to put the device's frame stores into normal mode (if not currently running in quad frame mode), or the non-2K quadrants quad mode.
		@param[in]	inChannel		Specifies the frame store bank of interest. Using anything ordinally less than NTV2_CHANNEL5
									will affect Frame Stores 1/2/3/4, while anything ordinally greater than NTV2_CHANNEL4 will
									affect Frame Stores 5/6/7/8.
		@note	Disabling 4K squares will implicitly set two-sample-interleave mode for the frame stores.
	**/
	AJA_VIRTUAL bool		Set4kSquaresEnable (const bool inIsEnabled, const NTV2Channel inChannel);

	/**
		@brief	Returns the device frame store's current SMPTE 425 "2K quadrants" mode, whether it's enabled or not.
		@return		True if successful; otherwise false.
		@param[in]	outIsEnabled	Receives true if the device's frame stores are currently in "2K quadrants" mode; otherwise false.
		@param[in]	inChannel		Specifies the frame store bank of interest. Using anything ordinally less than NTV2_CHANNEL5
									will report on Frame Stores 1/2/3/4, while anything ordinally greater than NTV2_CHANNEL4 will
									report on Frame Stores 5/6/7/8.
	**/
	AJA_VIRTUAL bool		Get4kSquaresEnable (bool & outIsEnabled, const NTV2Channel inChannel);
	AJA_VIRTUAL inline bool	Get4kSquaresEnable (bool * pOutIsEnabled, const NTV2Channel inChannel)		{return pOutIsEnabled ? Get4kSquaresEnable (*pOutIsEnabled, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief	Enables or disables SMPTE 425 two-sample interleave (Tsi) frame mode on the device.
		@return		True if successful; otherwise false.
		@param[in]	inIsEnabled		Specify true to put the device's frame stores into two-sample interleave (Tsi) mode.
									Specify false to put the device's frame stores into non-Tsi mode.
		@param[in]	inChannel		Specifies the frame store bank of interest. Using anything ordinally less than NTV2_CHANNEL5
									will affect Frame Stores 1/2/3/4, while anything ordinally greater than NTV2_CHANNEL4 will
									affect Frame Stores 5/6/7/8.
		@note	There is no need to call this function if Set4kSquaresEnable(false) was called.
	**/
	AJA_VIRTUAL bool		SetTsiFrameEnable (const bool inIsEnabled, const NTV2Channel inChannel);
#define Set425FrameEnable	SetTsiFrameEnable

	/**
		@brief	Returns the current SMPTE 425 two-sample-interleave frame mode on the device, whether it's enabled or not.
		@return		True if successful; otherwise false.
		@param[in]	outIsEnabled	Receives true if the device's frame stores are currently in two-sample interleave (Tsi) mode; otherwise false.
		@param[in]	inChannel		Specifies the frame store bank of interest. Using anything ordinally less than NTV2_CHANNEL5
									will report on Frame Stores 1/2/3/4, while anything ordinally greater than NTV2_CHANNEL4 will
									report on Frame Stores 5/6/7/8.
	**/
	AJA_VIRTUAL bool		GetTsiFrameEnable (bool & outIsEnabled, const NTV2Channel inChannel);
	AJA_VIRTUAL inline bool	GetTsiFrameEnable (bool * pOutIsEnabled, const NTV2Channel inChannel)		{return pOutIsEnabled ? GetTsiFrameEnable (*pOutIsEnabled, inChannel) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.
#define Get425FrameEnable	GetTsiFrameEnable

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool		SetReferenceVoltage (NTV2RefVoltage value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool		GetReferenceVoltage (NTV2RefVoltage* value);		///< @deprecated	This function is obsolete.

		AJA_VIRTUAL NTV2_DEPRECATED bool		SetFrameBufferMode (NTV2Channel inChannel, NTV2FrameBufferMode inValue);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool		GetFrameBufferMode (NTV2Channel inChannel, NTV2FrameBufferMode & outValue);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetFrameBufferMode (NTV2Channel inChannel, NTV2FrameBufferMode * pOutValue)		{return pOutValue ? GetFrameBufferMode (inChannel, *pOutValue) : false;}	///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)

	AJA_VIRTUAL bool		SetFrameBufferQuarterSizeMode (NTV2Channel inChannel, NTV2QuarterSizeExpandMode inValue);
	AJA_VIRTUAL bool		GetFrameBufferQuarterSizeMode (NTV2Channel inChannel, NTV2QuarterSizeExpandMode & outValue);
	AJA_VIRTUAL inline bool	GetFrameBufferQuarterSizeMode (NTV2Channel inChannel, NTV2QuarterSizeExpandMode * pOutValue)	{return pOutValue ? GetFrameBufferQuarterSizeMode (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetFrameBufferQuality (NTV2Channel inChannel, NTV2FrameBufferQuality inValue);
	AJA_VIRTUAL bool		GetFrameBufferQuality (NTV2Channel inChannel, NTV2FrameBufferQuality & outValue);
	AJA_VIRTUAL inline bool	GetFrameBufferQuality (NTV2Channel inChannel, NTV2FrameBufferQuality * pOutValue)	{return pOutValue ? GetFrameBufferQuality (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetEncodeAsPSF (NTV2Channel inChannel, NTV2EncodeAsPSF inValue);
	AJA_VIRTUAL bool		GetEncodeAsPSF (NTV2Channel inChannel, NTV2EncodeAsPSF & outValue);
	AJA_VIRTUAL inline bool	GetEncodeAsPSF (NTV2Channel inChannel, NTV2EncodeAsPSF * pOutValue)					{return pOutValue ? GetEncodeAsPSF (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief		Sets the frame buffer orientation for the given NTV2Channel.
		@param[in]	inChannel	Specifies the channel (aka Frame Store) of interest.
		@param[in]	inValue		Specifies the new frame buffer orientation.
		@return		True if successful;  otherwise false.
		@note		For capture, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN or NTV2_FRAMEBUFFER_ORIENTATION_NORMAL specifies that the input de-embedder writes
					incoming pixel data in top-to-bottom order in the frame buffer, whereas NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP writes incoming pixel
					data in bottom-to-top order. For playout, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN or NTV2_FRAMEBUFFER_ORIENTATION_NORMAL specifies that
					the output embedder reads outgoing pixel data in top-to-bottom order from frame buffer memory, whereas NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP
					reads it in bottom-to-top order.
	**/
	AJA_VIRTUAL bool		SetFrameBufferOrientation (const NTV2Channel inChannel, const NTV2FBOrientation inValue);

	/**
		@brief		Answers with the current frame buffer orientation for the given NTV2Channel.
		@param[in]	inChannel	Specifies the channel (aka Frame Store) of interest.
		@param[out]	outValue	Receives the NTV2VideoFrameBufferOrientation value.
		@return		True if successful;  otherwise false.
		@note		Normal operation is NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN. For capture, the input de-embedder writes incoming pixel data in top-to-bottom
					order in the frame buffer, and the output embedder also reads outgoing pixel data in top-to-bottom order from frame buffer memory.
					In NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP operation, this is reversed. The input de-embedder writes incoming pixel data in bottom-to-top
					order during capture, while for playout, the output embedder reads pixel data in bottom-to-top order.
	**/
	AJA_VIRTUAL bool		GetFrameBufferOrientation (const NTV2Channel inChannel, NTV2FBOrientation & outValue);
	AJA_VIRTUAL inline bool	GetFrameBufferOrientation (const NTV2Channel inChannel, NTV2FBOrientation * pOutValue)	{return pOutValue ? GetFrameBufferOrientation (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetAlphaFromInput2Bit (ULWord inValue);
	AJA_VIRTUAL bool		GetAlphaFromInput2Bit (ULWord & outValue);
	AJA_VIRTUAL inline bool	GetAlphaFromInput2Bit (ULWord * pOutValue)										{return pOutValue ? GetAlphaFromInput2Bit (*pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetPCIAccessFrame (NTV2Channel inChannel, ULWord inValue, bool inWaitForVertical = true);
	AJA_VIRTUAL bool		GetPCIAccessFrame (NTV2Channel inChannel, ULWord & outValue);
	AJA_VIRTUAL inline bool	GetPCIAccessFrame (NTV2Channel inChannel, ULWord * pOutValue)					{return pOutValue ? GetPCIAccessFrame (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetOutputFrame (NTV2Channel inChannel, ULWord value);
	AJA_VIRTUAL bool		GetOutputFrame (NTV2Channel inChannel, ULWord & outValue);
	AJA_VIRTUAL inline bool	GetOutputFrame (NTV2Channel inChannel, ULWord * pOutValue)						{return pOutValue ? GetOutputFrame (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetInputFrame (NTV2Channel inChannel, ULWord value);
	AJA_VIRTUAL bool		GetInputFrame (NTV2Channel inChannel, ULWord & outValue);
	AJA_VIRTUAL inline bool	GetInputFrame (NTV2Channel inChannel, ULWord * pOutValue)						{return pOutValue ? GetInputFrame (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetDualLinkOutputEnable (bool inIsEnabled);
	AJA_VIRTUAL bool		GetDualLinkOutputEnable (bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetDualLinkOutputEnable (bool * pOutIsEnabled)									{return pOutIsEnabled ? GetDualLinkOutputEnable (*pOutIsEnabled) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetDualLinkInputEnable (bool enable);
	AJA_VIRTUAL bool		GetDualLinkInputEnable (bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetDualLinkInputEnable (bool * pOutIsEnabled)									{return pOutIsEnabled ? GetDualLinkInputEnable (*pOutIsEnabled) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetVideoLimiting (NTV2VideoLimiting inValue);
	AJA_VIRTUAL bool		GetVideoLimiting (NTV2VideoLimiting & outValue);
	AJA_VIRTUAL inline bool	GetVideoLimiting (NTV2VideoLimiting * pOutValue)								{return pOutValue ? GetVideoLimiting (*pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetEnableVANCData (const bool inVANCenabled, const bool inTallerVANC, const NTV2Standard inStandard, const NTV2FrameGeometry inGeometry, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool		SetEnableVANCData (const bool inVANCenabled, const bool inTallerVANC = false, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool		SetVANCMode (const NTV2VANCMode inVancMode, const NTV2Standard inStandard, const NTV2FrameGeometry inFrameGeometry, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Retrieves the current VANC mode for the AJA device.
		@return		True if successful; otherwise false.
		@param[out]	outVancMode		Receives true if VANC is currently enabled for the given channel on the device.
		@param[in]	inChannel		Specifies the NTV2Channel of interest.
	**/
	AJA_VIRTUAL bool		GetVANCMode (NTV2VANCMode & outVancMode, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool		GetEnableVANCData (bool & outIsEnabled, bool & outIsWideVANCEnabled, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool		GetEnableVANCData (bool * pOutIsEnabled, bool * pOutIsWideVANCEnabled = NULL, NTV2Channel inChannel = NTV2_CHANNEL1);	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetVANCShiftMode (NTV2Channel inChannel, NTV2VANCDataShiftMode value);
	AJA_VIRTUAL bool		GetVANCShiftMode (NTV2Channel inChannel, NTV2VANCDataShiftMode & outValue);
	AJA_VIRTUAL inline bool	GetVANCShiftMode (NTV2Channel inChannel, NTV2VANCDataShiftMode * pOutValue)		{return pOutValue ? GetVANCShiftMode (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	AJA_VIRTUAL bool		SetPulldownMode (NTV2Channel inChannel, bool inValue);
	AJA_VIRTUAL bool		GetPulldownMode (NTV2Channel inChannel, bool & outValue);
	AJA_VIRTUAL inline bool	GetPulldownMode (NTV2Channel inChannel, bool * pOutValue)	{return pOutValue ? GetPulldownMode (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief	Swaps the values stored in the PCI access frame and output frame registers for the given frame store (channel).
		@param[in]	inChannel	Specifies the channel (frame store) of interest.
		@return	True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool		FlipFlopPage (NTV2Channel inChannel);


	/**
		@name	Mixer/Keyer & Video Processing
	**/
	///@{

	/**
		@brief		Sets the VANC source for the given mixer/keyer to the foreground video (or not).
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer			Specifies the mixer/keyer to be affected as a zero-based index number.
		@param[in]	inFromForegroundSource	If true, sets the mixer/keyer's VANC source to its foreground video input;
											otherwise, sets it to its background video input.
	**/
	AJA_VIRTUAL bool	SetMixerVancOutputFromForeground (const UWord inWhichMixer, const bool inFromForegroundSource = true);

	/**
		@brief		Answers whether or not the VANC source for the given mixer/keyer is currently the foreground video.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer				Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	outIsFromForegroundSource	Receives True if the mixer/keyer's VANC source is its foreground video input;
												otherwise False if it's its background video input.
	**/
	AJA_VIRTUAL bool	GetMixerVancOutputFromForeground (const UWord inWhichMixer, bool & outIsFromForegroundSource);


	/**
		@brief		Sets the foreground input control value for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inInputControl		Specifies the mixer/keyer's foreground input control value.
	**/
	AJA_VIRTUAL bool	SetMixerFGInputControl (const UWord inWhichMixer, const NTV2MixerKeyerInputControl inInputControl);

	/**
		@brief		Returns the current foreground input control value for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	outInputControl		Receives the mixer/keyer's foreground input control value; otherwise NTV2MIXERINPUTCONTROL_INVALID upon failure.
	**/
	AJA_VIRTUAL bool	GetMixerFGInputControl (const UWord inWhichMixer, NTV2MixerKeyerInputControl & outInputControl);

	/**
		@brief		Sets the background input control value for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inInputControl		Specifies the mixer/keyer's background input control value.
	**/
	AJA_VIRTUAL bool	SetMixerBGInputControl (const UWord inWhichMixer, const NTV2MixerKeyerInputControl inInputControl);

	/**
		@brief		Returns the current background input control value for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	outInputControl		Receives the mixer/keyer's background input control value; otherwise NTV2MIXERINPUTCONTROL_INVALID upon failure.
	**/
	AJA_VIRTUAL bool	GetMixerBGInputControl (const UWord inWhichMixer, NTV2MixerKeyerInputControl & outInputControl);

	/**
		@brief		Sets the mode for the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inMode				Specifies the mode value.
	**/
	AJA_VIRTUAL bool	SetMixerMode (const UWord inWhichMixer, const NTV2MixerKeyerMode inMode);

	/**
		@brief		Returns the current operating mode of the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	outMode				Receives the mode value.
	**/
	AJA_VIRTUAL bool	GetMixerMode (const UWord inWhichMixer, NTV2MixerKeyerMode & outMode);

	/**
		@brief		Sets the current mix coefficient of the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	inMixCoefficient	Specifies the new mix coefficient value.
	**/
	AJA_VIRTUAL bool	SetMixerCoefficient (const UWord inWhichMixer, const ULWord inMixCoefficient);

	/**
		@brief		Returns the current mix coefficient the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	outMixCoefficient	Receives the current mix coefficient value.
	**/
	AJA_VIRTUAL bool	GetMixerCoefficient (const UWord inWhichMixer, ULWord & outMixCoefficient);

	/**
		@brief		Returns the current sync state of the given mixer/keyer.
		@return		True if successful; otherwise false.
		@param[in]	inWhichMixer		Specifies the mixer/keyer of interest as a zero-based index number.
		@param[in]	outIsSyncOK			Receives the mixer's current sync state. If true, the mixer is synchronized to its inputs.
										If false, the mixer is not synchronized to its inputs.
	**/
	AJA_VIRTUAL bool	GetMixerSyncStatus (const UWord inWhichMixer, bool & outIsSyncOK);

	AJA_VIRTUAL bool	ReadLineCount (ULWord *value);
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	WritePanControl (ULWord value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadPanControl (ULWord *value);		///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)
	///@}


	/**
		@name	Audio
	**/
	///@{

	/**
		@brief		Sets the number of audio channels to be concurrently captured or played for a given audio system on the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inNumChannels		Specifies the number of audio channels the device will record or play to/from a
										given audio system. For most applications, this should always be set to the maximum
										number of audio channels the device is capable of capturing or playing, which can
										be obtained by calling the NTV2BoardGetMaxAudioChannels function (see ntv2devicefeatures.h).
		@param[in]	inAudioSystem	Optionally specifies the audio system of interest. Defaults to NTV2_AUDIOSYSTEM_1.
	**/
	AJA_VIRTUAL bool	SetNumberAudioChannels (const ULWord inNumChannels, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Returns the current number of audio channels being captured or played by a given audio system on the AJA device.
		@return		True if successful; otherwise false.
		@param[out]	outNumChannels		Receives the number of audio channels that the AJA device hardware is currently capturing
										or playing for the given audio system. If the function result is true, the variable's
										contents will be valid, and for most AJA devices will be 6, 8, or 16.
		@param[in]	inAudioSystem		Optionally specifies the audio system of interest. Defaults to NTV2_AUDIOSYSTEM_1.
		@details	This function allows client applications to determine how many audio channels the AJA hardware is
					currently capturing/playing into/from the given audio system on the device.
	**/
	AJA_VIRTUAL bool	GetNumberAudioChannels (ULWord & outNumChannels, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Sets the current audio sample rate for the given audio system on the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inRate			Specifies the desired audio sample rate for the given audio system.
									The specified rate value must be NTV2_AUDIO_48K or NTV2_AUDIO_96K.
		@param[in]	inAudioSystem	Optionally specifies the audio system of interest. Defaults to NTV2_AUDIOSYSTEM_1.
		@details	AJA devices generally use a 48 kHz audio sample rate. Many devices also support a 96 kHz sample rate,
					which is useful for "double speed" ingest or playout applications from tape equipment that is capable
					of playing or recording at twice the normal rate. This function call allows the client application to
					change the AJA device's audio sample rate for a given audio system.
	**/
	AJA_VIRTUAL bool	SetAudioRate (const NTV2AudioRate inRate, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Retrieves the current audio sample rate for the given audio system on the AJA device.
		@return		True if successful; otherwise false.
		@param[out]	outRate			Receives the current audio sample rate for the given audio system. If the function
									result is true, the variable will contain one of the following values: NTV2_AUDIO_48K,
									or NTV2_AUDIO_96K.
		@param[in]	inAudioSystem	Optionally specifies the audio system of interest. Defaults to NTV2_AUDIOSYSTEM_1.
		@details	AJA devices usually use a 48 kHz audio sample rate, but many also support a 96 kHz sample rate, which
					is useful for "double speed" ingest or playout applications from tape equipment that is capable of
					playing or recording at twice the normal rate. This function call allows the client application to
					discover the AJA device's audio sample rate that's currently being used for a given audio system.
	**/
	AJA_VIRTUAL bool	GetAudioRate (NTV2AudioRate & outRate, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Changes the size of the audio buffer that is used for a given audio system in the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inValue			Specifies the desired size of the capture/playout audio buffer to be used on the AJA device.
									All modern AJA devices use NTV2_AUDIO_BUFFER_BIG (4 MB).
		@param[in]	inAudioSystem	Optionally specifies the audio system of interest. Defaults to NTV2_AUDIOSYSTEM_1.
	**/
	AJA_VIRTUAL bool	SetAudioBufferSize (const NTV2AudioBufferSize inValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		Retrieves the size of the input or output audio buffer being used for a given audio system on the AJA device.
		@return		True if successful; otherwise false.
		@param[out]	outSize			Receives the size of the capture/playout audio buffer for the given audio system on the AJA device.
		@param[in]	inAudioSystem	Optionally specifies the audio system of interest. Defaults to NTV2_AUDIOSYSTEM_1.
	**/
	AJA_VIRTUAL bool	GetAudioBufferSize (NTV2AudioBufferSize & outSize, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);


	AJA_VIRTUAL bool	SetAudioAnalogLevel (const NTV2AudioLevel value, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	GetAudioAnalogLevel (NTV2AudioLevel & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	SetAudioLoopBack (const NTV2AudioLoopBack value, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	GetAudioLoopBack (NTV2AudioLoopBack & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	SetEncodedAudioMode (const NTV2EncodedAudioMode value, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	GetEncodedAudioMode (NTV2EncodedAudioMode & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	SetEmbeddedAudioInput (const NTV2EmbeddedAudioInput value, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	GetEmbeddedAudioInput (NTV2EmbeddedAudioInput & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	SetEmbeddedAudioClock (const NTV2EmbeddedAudioClock value, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);
	AJA_VIRTUAL bool	GetEmbeddedAudioClock (NTV2EmbeddedAudioClock & outValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		For the given audio system, answers with the wrap address, the threshold at which input/record or output/play transfers
					will likely need to account for "wrapping" around the end of the buffer back to the start of the buffer to continue
					recording/playing.
		@note		Since input/record wrap detection is simple -- i.e., the value returned from CNTV2Card::ReadAudioLastIn has decreased --
					this function is provided to detect when an output wrap will occur.
		@return		True if successful; otherwise false.
		@param[out]	outWrapAddress	Receives the byte offset in the audio output buffer at which a "wrap" will occur.
									This is typically 16KB from the end of the buffer. If the current offset plus the audio bytes to be
									written will go past this position, the caller should split the DMA transfer into two separate ones:
									one to fill to the end of the buffer, and the remainder from the start of the buffer.
		@param[in]	inAudioSystem	Optionally specifies the audio system of interest. Defaults to NTV2_AUDIOSYSTEM_1.
	**/
	AJA_VIRTUAL bool	GetAudioWrapAddress (ULWord & outWrapAddress, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief		For the given audio system, answers with the byte offset from the start of the audio buffer to the first byte
					of the input/capture buffer. (The capture buffer immediately follows the playout buffer in increasing memory
					address order.)
		@return		True if successful; otherwise false.
		@param[out]	outReadOffset	Receives the offset to the audio capture buffer from the start of the audio buffer.
									This will typically be 4MB.
		@param[in]	inAudioSystem	Optionally specifies the audio system of interest. Defaults to NTV2_AUDIOSYSTEM_1.
	**/
	AJA_VIRTUAL bool	GetAudioReadOffset (ULWord & outReadOffset, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	#if !defined (NTV2_DEPRECATE)
		//	These functions dealt exclusively with audio systems, but unfortunately required channels to be passed into them.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetNumberAudioChannels(ULWord numChannels, NTV2Channel channel);			///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetNumberAudioChannels(ULWord *numChannels, NTV2Channel channel = NTV2_CHANNEL1);	///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioRate(NTV2AudioRate value, NTV2Channel channel);						///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioRate(NTV2AudioRate *value, NTV2Channel channel);					///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioBufferSize(NTV2AudioBufferSize value, NTV2Channel channel);			///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioBufferSize(NTV2AudioBufferSize *value, NTV2Channel channel);		///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioAnalogLevel(NTV2AudioLevel value, NTV2Channel channel);				///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioAnalogLevel(NTV2AudioLevel *value, NTV2Channel channel);			///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioLoopBack(NTV2AudioLoopBack value, NTV2Channel channel);				///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioLoopBack(NTV2AudioLoopBack *value, NTV2Channel channel);			///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetEncodedAudioMode(NTV2EncodedAudioMode value, NTV2Channel channel);		///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetEncodedAudioMode(NTV2EncodedAudioMode *value, NTV2Channel channel);		///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetEmbeddedAudioInput(NTV2EmbeddedAudioInput value, NTV2Channel channel);	///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetEmbeddedAudioInput(NTV2EmbeddedAudioInput *value, NTV2Channel channel);	///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetEmbeddedAudioClock(NTV2EmbeddedAudioClock value, NTV2Channel channel);	///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetEmbeddedAudioClock(NTV2EmbeddedAudioClock *value, NTV2Channel channel);	///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioWrapAddress(ULWord *wrapAddress, NTV2Channel channel);				///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioReadOffset(ULWord *readOffset, NTV2Channel channel);				///< @deprecated	Use the equivalent function that accepts an NTV2AudioSystem instead of an NTV2Channel.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAverageAudioLevelChan1_2(ULWord *value);									///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteAudioControl (ULWord inValue, NTV2Channel inChannel = NTV2_CHANNEL1);	///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadAudioControl (ULWord *value, NTV2Channel inChannel = NTV2_CHANNEL1);	///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		For the given audio system, specifies the byte offset in the device's output audio buffer
					where its audio embedder will fetch the next 128-byte audio sample. This essentially moves
					the "play head" for audio output.
		@param[in]	inValue		Specifies the new byte offset into the device's output audio buffer.
		@param[in]	inChannel	Specifies the NTV2Channel (output audio embedder) of interest.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	WriteAudioLastOut (const ULWord inValue, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		For the given audio system, answers with the byte offset of the last 128-byte audio sample
					read by the device's output audio embedder. This is essentially the position of the "play
					"head" during audio output.
		@param[out]	outValue	Receives the byte offset of the last 128-byte audio sample read by the device's output audio
								embedder in its output audio buffer.
		@param[in]	inChannel	Specifies the NTV2Channel (output audio embedder) of interest.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	ReadAudioLastOut (ULWord & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	ReadAudioLastOut (ULWord * pOutValue, const NTV2Channel inChannel = NTV2_CHANNEL1)	{return pOutValue ? ReadAudioLastOut (*pOutValue, inChannel) : false;}

	/**
		@brief		For the given audio system, answers with the byte offset of the last 128-byte audio sample
					written by the device's input audio de-embedder. This is essentially the position of the
					"write head" during audio capture.
		@param[out]	outValue	Receives the byte offset of the last 128-byte audio sample written by the device's input audio
								de-embedder in its input audio buffer.
		@param[in]	inChannel	Specifies the NTV2Channel (input audio de-embedder) of interest.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	ReadAudioLastIn (ULWord & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	ReadAudioLastIn (ULWord *pOutValue, const NTV2Channel inChannel = NTV2_CHANNEL1)	{return pOutValue ? ReadAudioLastIn (*pOutValue, inChannel) : false;}

	AJA_VIRTUAL bool	WriteAudioSource (const ULWord inValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	ReadAudioSource (ULWord & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	ReadAudioSource (ULWord * pOutValue, const NTV2Channel inChannel = NTV2_CHANNEL1)	{return pOutValue ? ReadAudioSource (*pOutValue, inChannel) : false;}

	AJA_VIRTUAL bool	SetAudioOutputMonitorSource (NTV2AudioMonitorSelect inValue, NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetAudioOutputMonitorSource (NTV2AudioMonitorSelect & outValue, NTV2Channel & outChannel);
	AJA_VIRTUAL bool	GetAudioOutputMonitorSource (NTV2AudioMonitorSelect * pOutValue, NTV2Channel * pOutChannel = NULL);	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief		Enables or disables the output of audio samples by the given audio engine, resetting
					the playback position to the start of the audio buffer.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the audio system on the device to be affected.
		@param[in]	inEnable		If true,  audio samples will be produced by the audio system.
									If false, audio sample production is inhibited.
		@note		Calling this funcion with a true parameter has the side effect of resetting
					the current audio buffer pointer (as reported by ReadAudioLastOut) to zero.
					This can be useful for resynchronizing audio and video. If it is desired to
					stop sample production without resetting the pointer, use SetAudioOutputPause instead. 
	**/
	AJA_VIRTUAL bool	SetAudioOutputReset (const NTV2AudioSystem inAudioSystem, const bool inEnable);

	/**
		@brief		Answers whether or not the device's audio system is currently operating in the mode
					in which it is not producing audio output samples and the audio buffer pointer has
					been reset to zero.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the audio system of interest.
		@param[in]	outEnable			A boolean variable that is to receive 'true' if the audio system
										is not producing output samples and the buffer pointer is zero,
										or 'false' if the audio system is operating normally.
	**/
	AJA_VIRTUAL bool	GetAudioOutputReset (const NTV2AudioSystem inAudioSystem, bool & outEnable);

    /**
        @brief		Enables or disables 20 bit mode for the audio engine.
        @return		True if successful; otherwise false.
        @param[in]	inAudioSystem	Specifies the audio system on the device to be affected.
        @param[in]	inEnable		If true,  audio samples will be in 20 bit mode.
                                    If false, audio sample will be in 20 bit mode.
        @note		This is only used by Kona IP.
    **/
    AJA_VIRTUAL bool	SetAudio20BitMode (const NTV2AudioSystem inAudioSystem, const bool inEnable);

    /**
        @brief		Answers whether or not the device's audio system is currently operating in 20 bit
                    mode.  Normally the audio system is in 24 bit mode.
        @return		True if successful; otherwise false.
        @param[in]	inAudioSystem		Specifies the audio system of interest.
        @param[in]	outEnable			Receives 'true' if the audio system is in 20 bit moe, or 'false'
                                        if audio is in 24 bit mode.  This is only used by Kona IP.
    **/
    AJA_VIRTUAL bool	GetAudio20BitMode (const NTV2AudioSystem inAudioSystem, bool & outEnable);

	/**
		@brief		Enables or disables the output of audio samples and advancment of the audio buffer
					pointer ("play head") of the given audio engine.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the audio system on the device to be affected.
		@param[in]	inEnable		If true,  audio samples will be produced by the audio system.
									If false, audio sample production is inhibited.
		@note		If it is desired to reset the audio buffer pointer to the beginning of the
					buffer, use SetAudioOutputReset instead.
	**/
	AJA_VIRTUAL bool	SetAudioOutputPause (const NTV2AudioSystem inAudioSystem, const bool inEnable);

	/**
		@brief		Answers whether or not the device's audio system is currently operating in the mode
					in which it is not producing audio output samples and the audio buffer pointer
					is not advancing.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the audio system of interest.
		@param[in]	outEnable			Receives 'true' if the audio system is paused, or 'false' if audio
										playout is operating normally.
	**/
	AJA_VIRTUAL bool	GetAudioOutputPause (const NTV2AudioSystem inAudioSystem, bool & outEnable);

	/**
		@brief		Enables or disables the input of audio samples by the given audio engine, resetting
					the playback position to the start of the audio buffer.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the audio system on the device to be affected.
		@param[in]	inEnable		If true,  audio samples will be captured by the audio system.
									If false, audio sample capture is inhibited.
		@note		Calling this funcion with a true parameter has the side effect of resetting
					the current audio buffer pointer (as reported by ReadAudioLastIn) to zero.
					This can be useful for resynchronizing audio and video.
	**/
	AJA_VIRTUAL bool	SetAudioInputReset (const NTV2AudioSystem inAudioSystem, const bool inEnable);

	/**
		@brief		Answers whether or not the device's audio system is currently operating in the mode
					in which it is not capturing audio output samples and the audio buffer pointer has
					been reset to zero.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the audio system of interest.
		@param[in]	outEnable			A boolean variable that is to receive 'true' if the audio system
										is not capturing output samples and the buffer pointer is zero,
										or 'false' if the audio system is operating normally.
	**/
	AJA_VIRTUAL bool	GetAudioInputReset (const NTV2AudioSystem inAudioSystem, bool & outEnable);

	/**
		@brief		Determines if the given audio system on the AJA device will configured to capture audio samples.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem	Specifies the audio system of interest.
		@param[in]	inEnable		If true, the audio system will capture samples into memory, if not currently reset.
									If false, the audio system will not capture samples.
		@note		Applications that acquire exclusive use of the AJA device, set its "every frame services" mode
					to NTV2_OEM_TASKS, and use AutoCirculate won't need to call this function, since AutoCirculate
					configures the audio system automatically.
	**/
	AJA_VIRTUAL bool	SetAudioCaptureEnable (const NTV2AudioSystem inAudioSystem, const bool inEnable);

	/**
		@brief		Answers whether the audio system is configured for capturing audio samples
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the audio system of interest.
		@param[in]	outEnable			A boolean variable that is to receive 'true' if the audio system
										will capture samples to memory when not in reset mode,
										or 'false' if the audio system is inhibited from capturing samples.
	**/
	AJA_VIRTUAL bool	GetAudioCaptureEnable (const NTV2AudioSystem inAudioSystem, bool & outEnable);

	/**
		@brief		Enables or disables a special mode for the given audio system whereby its embedder and
					deembedder both start from the audio base address, instead of operating independently.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the audio system on the device to be affected.
		@param[in]	inEnable			If true, de-embedder and embedder both start from the audio base address.
										If false, the audio system operates normally.
	**/
	AJA_VIRTUAL bool	SetAudioPlayCaptureModeEnable (const NTV2AudioSystem inAudioSystem, const bool inEnable);

	/**
		@brief		Answers whether or not the device's audio system is currently operating in a special mode
					in which its embedder and deembedder both start from the audio base address, instead of
					operating independently.
		@return		True if successful; otherwise false.
		@param[in]	inAudioSystem		Specifies the audio system of interest.
		@param[in]	outEnable			A boolean variable that is to receive 'true' if the audio system's
										de-embedder and embedder both start from the audio base address,
										or 'false' if the audio system is operating normally.
	**/
	AJA_VIRTUAL bool	GetAudioPlayCaptureModeEnable (const NTV2AudioSystem inAudioSystem, bool & outEnable);

	/**
		@brief		Sets the audio input delay for the given audio subsystem on the device.
		@param[in]	inAudioSystem	Specifies the audio subsystem whose input delay is to be set.
		@param[in]	inDelay			Specifies the new audio input delay value for the audio subsystem.
									Each increment of this value increases the delay by exactly 512 bytes
									in the audio subsystem's audio buffer, or about 166.7 microseconds.
									Values from 0 thru 8159 are valid, which gives a maximum delay of about
									1.36 seconds.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetAudioInputDelay (const NTV2AudioSystem inAudioSystem, const ULWord inDelay);

	/**
		@brief		Answers with the audio input delay for the given audio subsystem on the device.
		@param[in]	inAudioSystem	Specifies the audio subsystem whose input delay is to be retrieved.
		@param[out]	outDelay		A ULWord variable that is to receive the audio subsystem's current input delay
									value, expressed as an integral number of 512 byte chunks in the audio subsystem's
									audio buffer on the device. This can be translated into microseconds by multiplying
									the received value by 166.7.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetAudioInputDelay (const NTV2AudioSystem inAudioSystem, ULWord & outDelay);


	/**
		@brief		Sets the audio output delay for the given audio subsystem on the device.
		@param[in]	inAudioSystem	Specifies the audio subsystem whose output delay is to be set.
		@param[in]	inDelay			Specifies the new audio output delay value for the audio subsystem.
									Each increment of this value increases the delay by exactly 512 bytes
									in the audio subsystem's audio buffer, or about 166.7 microseconds.
									Values from 0 thru 8159 are valid, which gives a maximum delay of about
									1.36 seconds.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetAudioOutputDelay (const NTV2AudioSystem inAudioSystem, const ULWord inDelay);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioPlayCaptureModeEnable (const NTV2AudioSystem inAudioSystem, bool * pOutEnable);		///< @deprecated	Use GetAudioPlayCaptureModeEnable(NTV2AudioSystem,bool&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioInputDelay (const NTV2AudioSystem inAudioSystem, ULWord * pOutDelay);		///< @deprecated	Use GetAudioInputDelay(NTV2AudioSystem,ULWord&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioOutputDelay (const NTV2AudioSystem inAudioSystem, ULWord * pOutDelay);		///< @deprecated	Use GetAudioOutputDelay(NTV2AudioSystem,ULWord&) instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		Answers with the audio output delay for the given audio subsystem on the device.
		@param[in]	inAudioSystem	Specifies the audio subsystem whose output delay is to be retrieved.
		@param[out]	outDelay		A ULWord variable that is to receive the audio subsystem's current output delay
									value, expressed as an integral number of 512 byte chunks in the audio subsystem's
									audio buffer on the device. This can be translated into microseconds by multiplying
									the received value by 166.7.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool		GetAudioOutputDelay (const NTV2AudioSystem inAudioSystem, ULWord & outDelay);


	/**
		@brief		Determines whether the given audio subsystem on the device is treated as normal PCM audio or not.
		@param[in]	inAudioSystem	Specifies the audio subsystem of interest.
		@param[in]	inIsNonPCM		If true, all audio channels in the audio subsystem are treated as non-PCM;
									otherwise, they're treated as normal PCM audio.
		@return		True if successful;  otherwise false.
		@note		This setting, if non-PCM, overrides per-audio-channel-pair PCM control on those devices that support it.
	**/
	AJA_VIRTUAL bool		SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const bool inIsNonPCM);


	/**
		@brief		Answers whether or not the given audio subsystem on the device is being treated as normal PCM audio.
		@param[in]	inAudioSystem	Specifies the audio subsystem of interest.
		@param[out]	outIsNonPCM		Receives true if all audio channels in the audio subsystem are being treated as non-PCM;
									otherwise false if they're being treated as normal PCM audio.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool		GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, bool & outIsNonPCM);


	/**
		@brief		Determines whether or not the given channel pair in the given audio subsystem on the device is treated as normal PCM audio.
		@param[in]	inAudioSystem	Specifies the audio subsystem of interest.
		@param[in]	inChannelPair	Specifies the channel pair of interest.
		@param[in]	inIsNonPCM		If true, the channel pair is treated as non-PCM;
									otherwise, it's treated as normal PCM audio.
		@return		True if successful;  otherwise false.
		@note		Call ::NTV2DeviceCanDoPCMControl to determine if per-audio-channel-pair PCM control capability is available on this device.
		@note		This function has no effect if the per-audio-subsystem PCM control setting is set to non-PCM.
					(See the overloaded function SetAudioPCMControl(const NTV2AudioSystem, const bool).)
	**/
	AJA_VIRTUAL bool		SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelPair, const bool inIsNonPCM);


	/**
		@brief		Sets which channel pairs are configured for non-PCM in the given audio subsystem on the device.
		@param[in]	inAudioSystem			Specifies the audio subsystem of interest.
		@param[in]	inNonPCMChannelPairs	Specifies the channel pairs that will be configured to emit non-PCM audio data.
		@return		True if successful; otherwise false.
		@note		Call ::NTV2DeviceCanDoPCMControl to determine if this device supports per-audio-channel-pair PCM control.
	**/
	AJA_VIRTUAL bool		SetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPairs & inNonPCMChannelPairs);


	/**
		@brief		Answers whether or not the given channel pair in the given audio subsystem on the device is being treated as normal PCM audio.
		@param[in]	inAudioSystem	Specifies the audio subsystem of interest.
		@param[in]	inChannelPair	Specifies the channel pair of interest.
		@param[out]	outIsNonPCM		Receives true if the channel pair is being treated as non-PCM;
									otherwise false if it's being treated as normal PCM audio.
		@return		True if successful; otherwise false.
		@note		Call ::NTV2DeviceCanDoPCMControl to determine if this device supports per-audio-channel-pair PCM control.
		@note		This function's answer is irrelevant if the per-audio-subsystem PCM control setting is set to non-PCM.
					(See the overloaded function SetAudioPCMControl(const NTV2AudioSystem, const bool).)
	**/
	AJA_VIRTUAL bool		GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelPair, bool & outIsNonPCM);


	/**
		@brief		Answers which channel pairs are configured for non-PCM in the given audio subsystem on the device.
		@param[in]	inAudioSystem			Specifies the audio subsystem of interest.
		@param[out]	outNonPCMChannelPairs	Receives the channel pairs that are currently configured to emit non-PCM audio data.
		@return		True if successful; otherwise false.
		@note		Call ::NTV2DeviceCanDoPCMControl to determine if this device supports per-audio-channel-pair PCM control.
	**/
	AJA_VIRTUAL bool		GetAudioPCMControl (const NTV2AudioSystem inAudioSystem, NTV2AudioChannelPairs & outNonPCMChannelPairs);


	/**
		@brief		Answers whether or not the given audio channel pair in the given audio subsystem on the device is present in the input signal.
		@param[in]	inAudioSystem	Specifies the audio subsystem of interest.
		@param[in]	inChannelPair	Specifies the channel pair of interest.
		@param[out]	outIsPresent	Receives true if the channel pair is present;  otherwise false if it's not present.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool		IsAudioChannelPairPresent (const NTV2AudioSystem inAudioSystem, const NTV2AudioChannelPair inChannelPair, bool & outIsPresent);


	/**
		@brief		Answers which audio channel pairs are present in the given audio subsystem's input stream.
		@param[in]	inAudioSystem				Specifies the audio subsystem of interest.
		@param[out]	outDetectedChannelPairs		Receives the set of unique audio channel pairs that are present in the audio subsystem's input stream.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool		GetDetectedAudioChannelPairs (const NTV2AudioSystem inAudioSystem, NTV2AudioChannelPairs & outDetectedChannelPairs);


	/**
		@brief		Sets the audio source for the given audio system on the device.
		@param[in]	inAudioSystem		Specifies the audio system of interest on the device (e.g., NTV2_AUDIOSYSTEM_1, NTV2_AUDIOSYSTEM_2, etc.).
										(Use the ::NTV2BoardGetNumAudioStreams function to determine how many independent audio systems are available on the device.)
		@param[in]	inAudioSource		Specifies the audio source to use for the given audio system (e.g., NTV2_AUDIO_EMBEDDED, NTV2_AUDIO_AES, NTV2_AUDIO_ANALOG, etc.).
		@param[in]	inEmbeddedInput		If the audio source is set to NTV2_AUDIO_EMBEDDED, and the device has multiple SDI inputs, use inEmbeddedInput
										to specify which NTV2EmbeddedAudioInput to use. This parameter is ignored if the inAudioSource is not NTV2_AUDIO_EMBEDDED.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	SetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, const NTV2AudioSource inAudioSource, const NTV2EmbeddedAudioInput inEmbeddedInput);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, const NTV2AudioSource inAudioSource);
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, const NTV2InputSource inInputSource);
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		Answers with the current audio source for the given audio system on the device.
		@param[in]	inAudioSystem	Specifies the audio system of interest on the device (e.g., NTV2_AUDIOSYSTEM_1, NTV2_AUDIOSYSTEM_2, etc.).
									(Use the ::NTV2BoardGetNumAudioStreams function to determine how many independent audio systems are available on the device.)
		@param[out]	outAudioSource	Receives the audio source that's currently being used for the given audio system (e.g., NTV2_AUDIO_EMBEDDED, NTV2_AUDIO_AES, NTV2_AUDIO_ANALOG, etc.).
		@param[out]	outEmbeddedSource	If the audio source is NTV2_AUDIO_EMBEDDED, outEmbeddedSource will be the SDI input it is configured for.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, NTV2AudioSource & outAudioSource, NTV2EmbeddedAudioInput & outEmbeddedSource);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, NTV2AudioSource & outAudioSource);
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioSystemInputSource (const NTV2AudioSystem inAudioSystem, NTV2AudioSource * pOutAudioSource);
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		Sets the device's audio system that will provide audio for the given SDI output's audio embedder.
		@param[in]	inChannel		Specifies the SDI output as an NTV2Channel (e.g., NTV2_CHANNEL1 == SDIOut1, NTV2_CHANNEL2 == SDIOut2, etc.)
		@param[in]	inAudioSystem	Specifies the audio system that is to be used by the SDI output's embedder (e.g., NTV2_AUDIOSYSTEM_1).
		@return		True if successful; otherwise false.
		@note		Use the NTV2BoardGetNumAudioStreams function to determine how many independent audio systems are available on the device.
	**/
	AJA_VIRTUAL bool	SetSDIOutputAudioSystem (const NTV2Channel inChannel, const NTV2AudioSystem inAudioSystem);

	/**
		@brief		Answers with the device's audio system that is currently providing audio for the given SDI output's audio embedder.
		@param[in]	inChannel		Specifies the SDI output of interest as an NTV2Channel (e.g., NTV2_CHANNEL1 == SDIOut1, NTV2_CHANNEL2 == SDIOut2, etc.)
		@param[in]	outAudioSystem	Receives the audio system that is being used by the SDI output's embedder (e.g., NTV2_AUDIOSYSTEM_1).
		@return		True if successful; otherwise false.
		@note		Use the NTV2BoardGetNumAudioStreams function to determine how many independent audio systems are available on the device.
	**/
	AJA_VIRTUAL bool	GetSDIOutputAudioSystem (const NTV2Channel inChannel, NTV2AudioSystem & outAudioSystem);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetSDIOutAudioSource (const ULWord inValue, const NTV2Channel channel = NTV2_CHANNEL1);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDIOutAudioSource (ULWord & outValue, const NTV2Channel channel = NTV2_CHANNEL1);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI1OutAudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI1OutAudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI2OutAudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI2OutAudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI3OutAudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI3OutAudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI4OutAudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI4OutAudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI5OutAudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI5OutAudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI6OutAudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI6OutAudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI7OutAudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI7OutAudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI8OutAudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI8OutAudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputAudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		Sets the device's audio system that will provide audio for the given SDI output's audio embedder for the 2nd data stream on a dual-link output.
		@param[in]	inChannel		Specifies the SDI output as an NTV2Channel (e.g., NTV2_CHANNEL1 == SDIOut1, NTV2_CHANNEL2 == SDIOut2, etc.)
		@param[in]	inAudioSystem	Specifies the audio system that is to be used by the SDI output's embedder (e.g., NTV2_AUDIOSYSTEM_1).
		@return		True if successful; otherwise false.
		@note		Use the NTV2BoardGetNumAudioStreams function to determine how many independent audio systems are available on the device.
	**/
	AJA_VIRTUAL bool	SetSDIOutputDS2AudioSystem (const NTV2Channel inChannel, const NTV2AudioSystem inAudioSystem);

	/**
		@brief		Answers with the device's audio system that is currently providing audio for the given SDI output's audio embedder for the 2nd data stream on a dual-link output.
		@param[in]	inChannel		Specifies the SDI output of interest as an NTV2Channel (e.g., NTV2_CHANNEL1 == SDIOut1, NTV2_CHANNEL2 == SDIOut2, etc.)
		@param[in]	outAudioSystem	Receives the audio system that is being used by the SDI output's embedder (e.g., NTV2_AUDIOSYSTEM_1).
		@return		True if successful; otherwise false.
		@note		Use the NTV2BoardGetNumAudioStreams function to determine how many independent audio systems are available on the device.
	**/
	AJA_VIRTUAL bool	GetSDIOutputDS2AudioSystem (const NTV2Channel inChannel, NTV2AudioSystem & outAudioSystem);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetSDIOutDS2AudioSource (const ULWord inValue, const NTV2Channel channel = NTV2_CHANNEL1);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDIOutDS2AudioSource (ULWord & outValue, const NTV2Channel channel = NTV2_CHANNEL1);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI1OutDS2AudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI1OutDS2AudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI2OutDS2AudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI2OutDS2AudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI3OutDS2AudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI3OutDS2AudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI4OutDS2AudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI4OutDS2AudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI5OutDS2AudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI5OutDS2AudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI6OutDS2AudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI6OutDS2AudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI7OutDS2AudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI7OutDS2AudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI8OutDS2AudioSource(ULWord value);	///< @deprecated	Use SetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI8OutDS2AudioSource(ULWord* value);	///< @deprecated	Use GetSDIOutputDS2AudioSystem(NTV2Channel,NTV2AudioSystem&) instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		For the given SDI input (specified as a channel number), answers if the specified audio channel pair is currently PCM-encoded or not.
		@param[in]	inSDIInputChannel	Specifies the SDI input of interest.
		@param[in]	inAudioChannelPair	Specifies the audio channel pair of interest.
		@param[out]	outIsPCM			Receives true if the channel pair is currently PCM-encoded;  otherwise false.
		@return		True if successful;  otherwise false.
	**/
	virtual bool InputAudioChannelPairHasPCM (const NTV2Channel inSDIInputChannel, const NTV2AudioChannelPair inAudioChannelPair, bool & outIsPCM);

	/**
		@brief		For the given SDI input (specified as a channel number), returns the set of audio channel pairs that are currently PCM-encoded.
		@param[in]	inSDIInputChannel		Specifies the SDI input of interest.
		@param[out]	outChannelPairs		Receives the channel pairs that are currently PCM-encoded.
		@return		True if successful;  otherwise false.
	**/
	virtual bool GetInputAudioChannelPairsWithPCM (const NTV2Channel inSDIInputChannel, NTV2AudioChannelPairs & outChannelPairs);

	/**
		@brief		For the given SDI input (specified as a channel number), returns the set of audio channel pairs that are currently not PCM-encoded.
		@param[in]	inSDIInputChannel		Specifies the SDI input of interest.
		@param[out]	outChannelPairs		Receives the channel pairs that are not currently PCM-encoded.
		@return		True if successful;  otherwise false.
	**/
	virtual bool GetInputAudioChannelPairsWithoutPCM (const NTV2Channel inSDIInputChannel, NTV2AudioChannelPairs & outChannelPairs);

	/**
		@brief		Answers as to whether or not the host OS audio services for the AJA device (e.g. CoreAudio on MacOS)
					are currently suspended or not.
		@param[out]	outIsSuspended	Receives 'true' if the host OS audio service is currently suspended for the AJA
									device;  otherwise, receives 'false'.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetSuspendHostAudio (bool & outIsSuspended);

	/**
		@brief		Suspends or resumes host OS audio (e.g. CoreAudio on MacOS) for the AJA device.
		@param[in]	inSuspend	If true, suspends host OS audio for the AJA device;  otherwise, resumes it.
		@return		True if successful; otherwise false.
		@note		This function is currently only implemented on MacOS, and is used to suspend or resume CoreAudio
					when an application uses AutoCirculate to capture or play audio, to keep the two audio systems
					from conflicting with each other.
	**/
	AJA_VIRTUAL bool	SetSuspendHostAudio (const bool inSuspend);

	/**
		@brief		Answers with the current audio source for a given quad of AES audio output channels.
					By default, at power-up, for AJA devices that support AES audio output, the content of AES audio output channels 1/2/3/4
					reflect what's being output from audio channels 1/2/3/4 from NTV2_AUDIOSYSTEM_1, likewise with audio channels 5/6/7/8, etc.
		@param[in]	inAESAudioChannels		Specifies the AES audio output channel quad of interest.
		@param[out]	outSrcAudioSystem		Receives the NTV2AudioSystem that is currently driving the given AES audio output channel quad.
		@param[out]	outSrcAudioChannels		Receives the audio channel quad from the audio engine that's sourcing the given AES audio output channel quad.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetAESOutputSource (const NTV2Audio4ChannelSelect inAESAudioChannels, NTV2AudioSystem & outSrcAudioSystem, NTV2Audio4ChannelSelect & outSrcAudioChannels);

	/**
		@brief		Changes the audio source for the given quad of AES audio output channels.
					By default, at power-up, for AJA devices that support AES audio output, the content of AES audio output channels 1/2/3/4
					reflect what's being output from audio channels 1/2/3/4 from NTV2_AUDIOSYSTEM_1, likewise with audio channels 5/6/7/8, etc.
		@param[in]	inAESAudioChannels		Specifies the AES audio output channel quad of interest.
		@param[in]	inSrcAudioSystem		Specifies the NTV2AudioSystem that should drive the given AES audio output channel quad.
		@param[in]	inSrcAudioChannels		Specifies the audio channel quad from the given audio engine that should drive the given AES audio output channel quad.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	SetAESOutputSource (const NTV2Audio4ChannelSelect inAESAudioChannels, const NTV2AudioSystem inSrcAudioSystem, const NTV2Audio4ChannelSelect inSrcAudioChannels);

	/**
		@brief		Answers with the current state of the audio output embedder for the given SDI output connector (specified as a channel number).
					When the embedder is disabled, the device will not embed any SMPTE 299M (HD) or SMPTE 272M (SD) packets in the HANC in the SDI output stream.
		@param[in]	inSDIOutputConnector	Specifies the SDI output of interest.
		@param[out]	outIsEnabled			Receives 'true' if the audio output embedder is enabled;  otherwise 'false' if disabled.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetAudioOutputEmbedderState (const NTV2Channel inSDIOutputConnector, bool & outIsEnabled);

	/**
		@brief		Enables or disables the audio output embedder for the given SDI output connector (specified as a channel number).
					When the embedder is disabled, the device will not embed any SMPTE 299M (HD) or SMPTE 272M (SD) packets in the HANC in the SDI output stream.
		@param[in]	inSDIOutputConnector	Specifies the SDI output of interest.
		@param[out]	inEnable				Specify 'true' to enable the audio output embedder (normal operation).
											Specify 'false' to disable the embedder.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetAudioOutputEmbedderState (const NTV2Channel inSDIOutputConnector, const bool & inEnable);

	///@}

	//
	//	Read/Write Particular Register routines
	//
	AJA_VIRTUAL bool	WriteGlobalControl (ULWord value);
	AJA_VIRTUAL bool	ReadGlobalControl (ULWord *value);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh1Control (ULWord value);				///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh1Control (ULWord *value);				///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh1PCIAccessFrame (ULWord value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh1PCIAccessFrame (ULWord *value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh1OutputFrame (ULWord value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh1OutputFrame (ULWord *value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh1InputFrame (ULWord value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh1InputFrame (ULWord *value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh2Control (ULWord value);				///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh2Control (ULWord *value);				///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh2PCIAccessFrame (ULWord value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh2PCIAccessFrame (ULWord *value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh2OutputFrame (ULWord value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh2OutputFrame (ULWord *value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh2InputFrame (ULWord value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh2InputFrame (ULWord *value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh3Control (ULWord value);				///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh3Control (ULWord *value);				///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh3PCIAccessFrame (ULWord value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh3PCIAccessFrame (ULWord *value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh3OutputFrame (ULWord value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh3OutputFrame (ULWord *value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh3InputFrame (ULWord value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh3InputFrame (ULWord *value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh4Control (ULWord value);				///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh4Control (ULWord *value);				///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh4PCIAccessFrame (ULWord value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh4PCIAccessFrame (ULWord *value);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh4OutputFrame (ULWord value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh4OutputFrame (ULWord *value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WriteCh4InputFrame (ULWord value);			///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadCh4InputFrame (ULWord *value);			///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)


	/**
		@name	Programming
	**/
	///@{
	AJA_VIRTUAL bool	ReadFlashProgramControl(ULWord *value);
	AJA_VIRTUAL bool	IsXilinxProgrammed();
	AJA_VIRTUAL bool	ProgramMainFlash(const char *fileName);
	AJA_VIRTUAL bool	EraseFlashBlock(ULWord numSectors, ULWord sectorSize);
	AJA_VIRTUAL bool	CheckFlashErased(ULWord numSectors);
	AJA_VIRTUAL bool	VerifyMainFlash(const char *fileName);
	AJA_VIRTUAL bool	GetProgramStatus(SSC_GET_FIRMWARE_PROGRESS_STRUCT *statusStruct);
	AJA_VIRTUAL bool	WaitForFlashNOTBusy();
	///@}

	//
	//	OEM Mapping to Userspace Functions
	//
	AJA_VIRTUAL bool	GetBaseAddress (NTV2Channel channel, ULWord **pBaseAddress);
	AJA_VIRTUAL bool	GetBaseAddress (ULWord **pBaseAddress);
	AJA_VIRTUAL bool	GetRegisterBaseAddress (ULWord regNumber, ULWord ** pRegAddress);
	AJA_VIRTUAL bool	GetXena2FlashBaseAddress (ULWord ** pXena2FlashAddress);

	//
	//	Read-Only Status Registers
	//
	AJA_VIRTUAL bool	ReadStatusRegister (ULWord *value);
	AJA_VIRTUAL bool	ReadStatus2Register (ULWord *value);
	AJA_VIRTUAL bool	ReadInputStatusRegister (ULWord *value);
	AJA_VIRTUAL bool	ReadInputStatus2Register (ULWord *value);
	AJA_VIRTUAL bool	ReadInput56StatusRegister (ULWord *value);
	AJA_VIRTUAL bool	ReadInput78StatusRegister (ULWord *value);
	AJA_VIRTUAL bool	Read3GInputStatusRegister(ULWord *value);
	AJA_VIRTUAL bool	Read3GInputStatus2Register(ULWord *value);
	AJA_VIRTUAL bool	Read3GInput5678StatusRegister(ULWord *value);

	AJA_VIRTUAL bool	ReadSDIInVPID (const NTV2Channel channel, ULWord & outValueA, ULWord & outValueB);
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadSDIInVPID(NTV2Channel channel, ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use ReadSDIInVPID(NTV2Channel,ULWord&,ULWord&) instead
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadSDI1InVPID(ULWord* valueA, ULWord* valueB = NULL);						///< @deprecated	Use ReadSDIInVPID(NTV2Channel,ULWord&,ULWord&) instead
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadSDI2InVPID(ULWord* valueA, ULWord* valueB = NULL);						///< @deprecated	Use ReadSDIInVPID(NTV2Channel,ULWord&,ULWord&) instead
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadSDI3InVPID(ULWord* valueA, ULWord* valueB = NULL);						///< @deprecated	Use ReadSDIInVPID(NTV2Channel,ULWord&,ULWord&) instead
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadSDI4InVPID(ULWord* valueA, ULWord* valueB = NULL);						///< @deprecated	Use ReadSDIInVPID(NTV2Channel,ULWord&,ULWord&) instead
	#endif	//	!defined (NTV2_DEPRECATE)

	AJA_VIRTUAL bool	SupportsP2PTransfer ();
	AJA_VIRTUAL bool	SupportsP2PTarget ();


	/**
		@name	On-Device LEDs
	**/
	///@{
	/**
		@brief	The four on-board LEDs can be set by writing 0-15
		@param[in]	inValue		Sets the state of the four on-board LEDs using the least significant
								four bits of the given ULWord value.
		@return	True if successful;  otherwise, false.
	**/
	AJA_VIRTUAL bool	SetLEDState (ULWord inValue);

	/**
		@brief	Answers with the current state of the four on-board LEDs.
		@param[out]	outValue	Receives the current state of the four on-board LEDs.
								Only the least significant four bits of the ULWord have any meaning.
		@return	True if successful;  otherwise, false.
	**/
	AJA_VIRTUAL bool			GetLEDState (ULWord & outValue);

	AJA_VIRTUAL inline  bool	GetLEDState (ULWord * pOutValue)											{return pOutValue ? GetLEDState (*pOutValue) : false;}
	///@}


	/**
		@name	RP-188
	**/
	///@{
	/**
		@brief		Sets the current RP188 mode -- NTV2_RP188_INPUT or NTV2_RP188_OUTPUT -- for the given channel.
		@param[in]	inChannel		Specifies the channel of interest.
		@param[in]	inMode			Specifies the new RP-188 mode for the given channel.
									Must be one of NTV2_RP188_INPUT or NTV2_RP188_OUTPUT. All other values are illegal.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetRP188Mode			(const NTV2Channel inChannel,	const NTV2_RP188Mode inMode);

	/**
		@brief		Returns the current RP188 mode -- NTV2_RP188_INPUT or NTV2_RP188_OUTPUT -- for the given channel.
		@param[in]	inChannel		Specifies the channel of interest.
		@param[out]	outMode			Receives the RP-188 mode for the given channel.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetRP188Mode			(const NTV2Channel inChannel,	NTV2_RP188Mode & outMode);

	/**
		@note		This needs to be documented.
	**/
	AJA_VIRTUAL bool	SetRP188Data			(const NTV2Channel inChannel,	const ULWord frame, const RP188_STRUCT & inRP188Data);

	/**
		@note		This needs to be documented.
	**/
	AJA_VIRTUAL bool	GetRP188Data			(const NTV2Channel inChannel,	const ULWord frame, RP188_STRUCT & outRP188Data);

	/**
		@brief		Sets the RP188 DBB filter for the given SDI output (channel), assuming the channel's RP188 bypass mode is enabled (i.e., for E-E operation).
		@param[in]	inChannel		Specifies the SDI output (channel) of interest.
		@param[out]	inFilterValue	Specifies the new filter value to use. Only the lower 8 bits are used.
		@return		True if successful;  otherwise false.
		@note		AutoCirculate-based applications have no need to use this function, since the driver manages RP188 filtering when AutoCirculateInitForOutput is called.
	**/
	AJA_VIRTUAL bool	SetRP188Source			(const NTV2Channel inChannel,	const ULWord inFilterValue);


	/**
		@brief		Returns the current RP188 filter setting for the given (output) channel, assuming the channel's RP188 bypass mode is enabled (i.e., for E-E operation).
		@param[in]	inChannel		Specifies the SDI output (channel) of interest.
		@param[out]	outFilterValue	Receives the given channel's current RP188 SDI input filter, which will be an 8-bit value.
		@return		True if successful;  otherwise false.
		@note		AutoCirculate-based applications have no need to use this function, since the driver manages RP188 filtering when AutoCirculateInitForOutput is called.
	**/
	AJA_VIRTUAL bool	GetRP188Source			(const NTV2Channel inChannel,	ULWord & outFilterValue);


	/**
		@brief		Answers whether or not the channel's RP-188 bypass mode is in effect.
		@param[in]	inChannel	Specifies the output channel of interest.
		@param[out]	outIsBypassEnabled	Receives true if the output channel's RP188 timecode is currently sourced from the channel's
										RP188 registers (i.e., from calls to SetRP188Data). Receives false if its output timecode is
										currently sourced from the input (specified from a prior call to SetRP188Source).
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	IsRP188BypassEnabled	(const NTV2Channel inChannel,	bool & outIsBypassEnabled);

	/**
		@brief		Normally, if the device channel's NTV2_RP188Mode is NTV2_RP188_OUTPUT, an SDI output will embed RP188 timecode as fed into its three
					DBB/Bits0_31/Bits32_63 registers (via calls to SetRP188Data). These registers can be bypassed to grab RP188 from an SDI input, which
					is useful in E-E mode.
		@param[in]	inChannel			Specifies the (SDI output) channel of interest.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	DisableRP188Bypass		(const NTV2Channel inChannel);

	/**
		@brief		Normally, if the device channel's NTV2_RP188Mode is NTV2_RP188_OUTPUT, an SDI output will embed RP188 timecode as fed into its three
					DBB/Bits0_31/Bits32_63 registers (via calls to SetRP188Data). These registers can be bypassed to grab RP188 from an SDI input, which
					is useful in E-E mode.
		@param[in]	inChannel			Specifies the (SDI output) channel of interest.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	EnableRP188Bypass		(const NTV2Channel inChannel);

		AJA_VIRTUAL NTV2_DEPRECATED inline bool	DisableRP188Bypass	(const NTV2Channel inChannel, const bool inBypassDisabled)
									{(void) inBypassDisabled;  return DisableRP188Bypass (inChannel);}					///< @deprecated	Use DisableRP188Bypass(const NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetRP188Mode (NTV2Channel inChannel, NTV2_RP188Mode * pOutMode)
									{return pOutMode ? GetRP188Mode (inChannel, *pOutMode) : false;}					///< @deprecated	Use GetRP188Mode(const NTV2Channel, NTV2_RP188Mode &) instead.
		AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetRP188Data (NTV2Channel inChannel, ULWord inFrame, RP188_STRUCT * pOutRP188Data)
									{return pOutRP188Data ? GetRP188Data (inChannel, inFrame, *pOutRP188Data) : false;}	///< @deprecated	Use GetRP188Mode(const NTV2Channel, const ULWord, RP188_STRUCT &) instead.
		AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetRP188Source (NTV2Channel inChannel, ULWord * pOutValue)
									{return pOutValue ? GetRP188Source (inChannel, *pOutValue) : false;}				///< @deprecated	Use GetRP188Mode(const NTV2Channel, ULWord &) instead.
	///@}


	//
	//	RegisterAccess Control
	//
	AJA_VIRTUAL bool	SetRegisterWritemode (NTV2RegisterWriteMode inValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetRegisterWritemode (NTV2RegisterWriteMode & outValue, const NTV2Channel inChannel = NTV2_CHANNEL1);
	AJA_VIRTUAL inline bool	GetRegisterWritemode (NTV2RegisterWriteMode * pOutValue, NTV2Channel inChannel = NTV2_CHANNEL1)		{return pOutValue ? GetRegisterWritemode (*pOutValue, inChannel) : false;}


	/**
		@name	Interrupts & Events
	**/
	///@{
	//
	//	Enable Interrupt/Event
	//
	AJA_VIRTUAL bool	EnableInterrupt (const INTERRUPT_ENUMS inEventCode);							//	GENERIC!

	/**
		@brief		Allows the CNTV2Card instance to wait for and respond to vertical blanking interrupts
					originating from the given output channel on the receiver's AJA device.
		@param[in]	channel		Specifies the output channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	EnableOutputInterrupt (const NTV2Channel channel = NTV2_CHANNEL1);

	/**
		@brief		Allows the CNTV2Card instance to wait for and respond to vertical blanking interrupts
					originating from the given input channel on the receiver's AJA device.
		@param[in]	channel		Specifies the input channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	EnableInputInterrupt (const NTV2Channel channel = NTV2_CHANNEL1);


	//
	//	Disable Interrupt/Event
	//
	AJA_VIRTUAL bool	DisableInterrupt (const INTERRUPT_ENUMS inEventCode);						//	GENERIC!

	/**
		@brief		Prevents the CNTV2Card instance from waiting for and responding to vertical blanking
					interrupts originating from the given output channel on the device.
		@param[in]	channel		Specifies the output channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	DisableOutputInterrupt (const NTV2Channel channel = NTV2_CHANNEL1);

	/**
		@brief		Prevents the CNTV2Card instance from waiting for and responding to vertical blanking
					interrupts originating from the given input channel on the device.
		@param[in]	channel		Specifies the input channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	DisableInputInterrupt (const NTV2Channel channel = NTV2_CHANNEL1);

	AJA_VIRTUAL bool	GetCurrentInterruptMasks (NTV2InterruptMask & outIntMask1, NTV2Interrupt2Mask & outIntMask2);


	//
	//	Subscribe to events
	//
	/**
		@brief		Causes me to be notified when the given event/interrupt is triggered for the AJA device.
		@param[in]	inEventCode		Specifies the INTERRUPT_ENUMS of interest.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	SubscribeEvent (const INTERRUPT_ENUMS inEventCode);						//	GENERIC!

	/**
		@brief		Causes me to be notified when an output vertical blanking interrupt is generated for the given output channel.
		@param[in]	inChannel	Specifies the output channel of interest.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	SubscribeOutputVerticalEvent (const NTV2Channel inChannel);


	/**
		@brief		Causes me to be notified when an input vertical blanking interrupt occurs on the given input channel.
		@param[in]	inChannel	Specifies the input channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	SubscribeInputVerticalEvent (const NTV2Channel inChannel = NTV2_CHANNEL1);


	//
	//	Unsubscribe from events
	//
	/**
		@brief		Unregisters me so I'm no longer notified when the given event/interrupt is triggered on the AJA device.
		@param[in]	inEventCode		Specifies the INTERRUPT_ENUMS of interest.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	UnsubscribeEvent (const INTERRUPT_ENUMS inEventCode);						//	GENERIC!

	/**
		@brief		Unregisters me so I'm no longer notified when an output VBI is signaled on the given output channel.
		@param[in]	inChannel	Specifies the output channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@details	This function undoes the effect of a prior call to SubscribeOutputVerticalEvent.
	**/
	AJA_VIRTUAL bool	UnsubscribeOutputVerticalEvent (const NTV2Channel inChannel);

	/**
		@brief		Unregisters me so I'm no longer notified when an input VBI is signaled on the given input channel.
		@param[in]	inChannel	Specifies the input channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@details	This function undoes the effects of a prior call to SubscribeInputVerticalEvent.
	**/
	AJA_VIRTUAL bool	UnsubscribeInputVerticalEvent (const NTV2Channel inChannel = NTV2_CHANNEL1);


	//
	//	Get interrupt counts
	//
	/**
		@brief		Answers with the number of output vertical interrupts handled by the driver for the given output channel.
		@param[out]	outCount	Receives the number of output VBIs handled by the driver since it was loaded.
		@param[in]	inChannel	Specifies the output channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetOutputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the number of input vertical interrupts handled by the driver for the given input channel.
		@param[out]	outCount	Receives the number of input VBIs handled by the driver since it was loaded.
		@param[in]	inChannel	Specifies the input channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetInputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel = NTV2_CHANNEL1);


	/**
		@brief		Answers with the number of interrupt events that I successfully waited for.
		@param[in]	inEventCode		Specifies the interrupt of interest.
		@param[out]	outCount		Receives the number of interrupt events that I successfully waited for.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetInterruptEventCount (const INTERRUPT_ENUMS inEventCode, ULWord & outCount);

	/**
		@brief		Answers with the number of output interrupt events that I successfully waited for on the given channel.
		@param[out]	outCount		Receives the number of output interrupt events that were successfully waited for.
		@param[in]	inChannel		Specifies the NTV2Channel of interest.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetOutputVerticalEventCount (ULWord & outCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Answers with the number of input interrupt events that I successfully waited for on the given channel.
		@param[out]	outCount		Receives the number of input interrupt events that were successfully waited for.
		@param[in]	inChannel		Specifies the NTV2Channel of interest.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetInputVerticalEventCount (ULWord & outCount, const NTV2Channel inChannel = NTV2_CHANNEL1);


	/**
		@brief		Resets my interrupt event tally for the given interrupt type. (This is my count of the number of successful event waits.)
		@param[in]	inEventCode		Specifies the interrupt type.
		@param[in]	inCount			Specifies the new count value. Use zero to reset the tally.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetInterruptEventCount (const INTERRUPT_ENUMS inEventCode, const ULWord inCount);

	/**
		@brief		Resets my output interrupt event tally for the given channel.
		@param[in]	inCount			Specifies the new count value. Use zero to reset the tally.
		@param[in]	inChannel		Specifies the [output] channel.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetOutputVerticalEventCount (const ULWord inCount, const NTV2Channel inChannel = NTV2_CHANNEL1);

	/**
		@brief		Resets my input interrupt event tally for the given channel.
		@param[in]	inCount			Specifies the new count value. Use zero to reset the tally.
		@param[in]	inChannel		Specifies the [input] channel.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetInputVerticalEventCount (const ULWord inCount, const NTV2Channel inChannel = NTV2_CHANNEL1);


	//
	//	Wait for event
	//
	/**
		@brief		Efficiently sleeps the calling thread/process until the next one (or more) output Vertical Blanking
					Interrupt(s) for the given output channel occurs on the AJA device.
		@param[in]	inChannel		Specifies the output channel of interest. Defaults to NTV2_CHANNEL1.
		@param[in]	inRepeatCount	Specifies the number of output VBIs to wait for until returning. Defaults to 1.
		@return		True if successful; otherwise false.
		@note		The device's timing reference source affects how often -- or even if -- the VBI occurs.
		@note		If the wait period exceeds about 50 milliseconds, the function will fail and return false.
	**/
	AJA_VIRTUAL bool	WaitForOutputVerticalInterrupt (const NTV2Channel inChannel = NTV2_CHANNEL1, UWord inRepeatCount = 1);

	/**
		@brief		Efficiently sleeps the calling thread/process until the next output VBI for the given field and output channel.
		@param[in]	inFieldID	Specifies the field identifier of interest.
		@param[in]	channel		Specifies the output channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@note		The device's timing reference source affects how often -- or even if -- the VBI occurs.
		@note		This function assumes an interlaced video format on the given channel on the device.
		@note		If the wait period exceeds about 50 milliseconds, the function will fail and return false.
	**/
	AJA_VIRTUAL bool	WaitForOutputFieldID (const NTV2FieldID inFieldID, const NTV2Channel channel = NTV2_CHANNEL1);

	/**
		@brief		Efficiently sleeps the calling thread/process until the next input Vertical Blanking Interrupt
					for the given input channel occurs on the AJA device.
		@param[in]	inChannel		Specifies the input channel of interest. Defaults to NTV2_CHANNEL1.
		@param[in]	inRepeatCount	Specifies the number of input VBIs to wait for until returning. Defaults to 1.
		@return		True if successful; otherwise false.
		@note		The device's timing reference source affects how often -- or even if -- the VBI occurs.
		@note		If the wait period exceeds about 50 milliseconds, the function will fail and return false.
	**/
	AJA_VIRTUAL bool	WaitForInputVerticalInterrupt (const NTV2Channel inChannel = NTV2_CHANNEL1, UWord inRepeatCount = 1);

	/**
		@brief		Efficiently sleeps the calling thread/process until the next input VBI for the given field and input channel.
		@param[in]	inFieldID	Specifies the field identifier of interest.
		@param[in]	channel		Specifies the input channel of interest. Defaults to NTV2_CHANNEL1.
		@return		True if successful; otherwise false.
		@note		The device's timing reference source affects how often -- or even if -- the VBI occurs.
		@note		This function assumes an interlaced video format on the given channel on the device. Calling this
					function with a progressive signal will give unpredictable results (although 24psf works).
		@note		If the wait period exceeds about 50 milliseconds, the function will fail and return false.
	**/
	AJA_VIRTUAL bool	WaitForInputFieldID (const NTV2FieldID inFieldID, const NTV2Channel channel = NTV2_CHANNEL1);


	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableVerticalInterrupt();					///< @deprecated	Use EnableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableOutput2VerticalInterrupt();			///< @deprecated	Use EnableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableOutput3VerticalInterrupt();			///< @deprecated	Use EnableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableOutput4VerticalInterrupt();			///< @deprecated	Use EnableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableOutput5VerticalInterrupt();			///< @deprecated	Use EnableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableOutput6VerticalInterrupt();			///< @deprecated	Use EnableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableOutput7VerticalInterrupt();			///< @deprecated	Use EnableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableOutput8VerticalInterrupt();			///< @deprecated	Use EnableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableInput1Interrupt();					///< @deprecated	Use EnableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableInput2Interrupt();					///< @deprecated	Use EnableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableInput3Interrupt();					///< @deprecated	Use EnableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableInput4Interrupt();					///< @deprecated	Use EnableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableInput5Interrupt();					///< @deprecated	Use EnableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableInput6Interrupt();					///< @deprecated	Use EnableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableInput7Interrupt();					///< @deprecated	Use EnableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableInput8Interrupt();					///< @deprecated	Use EnableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableAudioInterrupt();						///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableAudioInWrapInterrupt();				///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableAudioOutWrapInterrupt();				///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableUartTxInterrupt();					///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableUart2TxInterrupt();					///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableUartRxInterrupt();					///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableUart2RxInterrupt();					///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableHDMIHotplugInterrupt();				///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	EnableAuxVerticalInterrupt();				///< @deprecated	Use EnableInterrupt(INTERRUPT_ENUMS) instead.

		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableVerticalInterrupt();					///< @deprecated	Use DisableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableOutput2VerticalInterrupt();			///< @deprecated	Use DisableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableOutput3VerticalInterrupt();			///< @deprecated	Use DisableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableOutput4VerticalInterrupt();			///< @deprecated	Use DisableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableOutput5VerticalInterrupt();			///< @deprecated	Use DisableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableOutput6VerticalInterrupt();			///< @deprecated	Use DisableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableOutput7VerticalInterrupt();			///< @deprecated	Use DisableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableOutput8VerticalInterrupt();			///< @deprecated	Use DisableOutputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableInput1Interrupt();					///< @deprecated	Use DisableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableInput2Interrupt();					///< @deprecated	Use DisableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableInput3Interrupt();					///< @deprecated	Use DisableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableInput4Interrupt();					///< @deprecated	Use DisableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableInput5Interrupt();					///< @deprecated	Use DisableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableInput6Interrupt();					///< @deprecated	Use DisableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableInput7Interrupt();					///< @deprecated	Use DisableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableInput8Interrupt();					///< @deprecated	Use DisableInputInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableAudioInterrupt();					///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableAudioInWrapInterrupt();				///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableAudioOutWrapInterrupt();				///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableUartTxInterrupt();					///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableUart2TxInterrupt();					///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableUartRxInterrupt();					///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableUart2RxInterrupt();					///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableHDMIHotplugInterrupt();				///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	DisableAuxVerticalInterrupt();				///< @deprecated	Use DisableInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetCurrentInterruptMask (NTV2InterruptMask * outInterruptMask);		///< @deprecated	Use GetCurrentInterruptMasks instead.

		//	Subscribe to events
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeOutputVerticalEvent ();			///< @deprecated	Use SubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeOutput2VerticalEvent ();			///< @deprecated	Use SubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeOutput3VerticalEvent ();			///< @deprecated	Use SubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeOutput4VerticalEvent ();			///< @deprecated	Use SubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeOutput5VerticalEvent ();			///< @deprecated	Use SubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeOutput6VerticalEvent ();			///< @deprecated	Use SubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeOutput7VerticalEvent ();			///< @deprecated	Use SubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeOutput8VerticalEvent ();			///< @deprecated	Use SubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeInput1VerticalEvent ();			///< @deprecated	Use SubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeInput2VerticalEvent ();			///< @deprecated	Use SubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeInput3VerticalEvent ();			///< @deprecated	Use SubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeInput4VerticalEvent ();			///< @deprecated	Use SubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeInput5VerticalEvent ();			///< @deprecated	Use SubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeInput6VerticalEvent ();			///< @deprecated	Use SubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeInput7VerticalEvent ();			///< @deprecated	Use SubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeInput8VerticalEvent ();			///< @deprecated	Use SubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeAudioInterruptEvent ();			///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeAudioInWrapInterruptEvent ();		///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeAudioOutWrapInterruptEvent ();		///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeUartTxInterruptEvent ();			///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeUartRxInterruptEvent ();			///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeUart2TxInterruptEvent ();			///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeUart2RxInterruptEvent ();			///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeHDMIHotplugInterruptEvent ();		///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeAuxVerticalInterruptEvent ();		///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeDMA1InterruptEvent ();				///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeDMA2InterruptEvent ();				///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeDMA3InterruptEvent ();				///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeDMA4InterruptEvent ();				///< @deprecated	Use SubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SubscribeChangeEvent();	// subscribe to get notified upon any Register changes

		//	Unsubscribe from events
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeOutputVerticalEvent ();			///< @deprecated	Use UnsubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeOutput2VerticalEvent ();			///< @deprecated	Use UnsubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeOutput3VerticalEvent ();			///< @deprecated	Use UnsubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeOutput4VerticalEvent ();			///< @deprecated	Use UnsubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeOutput5VerticalEvent ();			///< @deprecated	Use UnsubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeOutput6VerticalEvent ();			///< @deprecated	Use UnsubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeOutput7VerticalEvent ();			///< @deprecated	Use UnsubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeOutput8VerticalEvent ();			///< @deprecated	Use UnsubscribeOutputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeInput1VerticalEvent ();			///< @deprecated	Use UnsubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeInput2VerticalEvent ();			///< @deprecated	Use UnsubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeInput3VerticalEvent ();			///< @deprecated	Use UnsubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeInput4VerticalEvent ();			///< @deprecated	Use UnsubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeInput5VerticalEvent ();			///< @deprecated	Use UnsubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeInput6VerticalEvent ();			///< @deprecated	Use UnsubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeInput7VerticalEvent ();			///< @deprecated	Use UnsubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeInput8VerticalEvent ();			///< @deprecated	Use UnsubscribeInputVerticalEvent(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeAudioInterruptEvent ();			///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeAudioInWrapInterruptEvent ();	///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeAudioOutWrapInterruptEvent ();	///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeUartTxInterruptEvent ();			///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeUartRxInterruptEvent ();			///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeUart2TxInterruptEvent ();		///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeUart2RxInterruptEvent ();		///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeHDMIHotplugInterruptEvent ();	///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeAuxVerticalInterruptEvent ();	///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeDMA1InterruptEvent ();			///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeDMA2InterruptEvent ();			///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeDMA3InterruptEvent ();			///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeDMA4InterruptEvent ();			///< @deprecated	Use UnsubscribeEvent(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	UnsubscribeChangeEvent ();

		//	Get interrupt counts
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutputVerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetOutputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput2VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetOutputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput3VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetOutputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput4VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetOutputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput5VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetOutputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput6VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetOutputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput7VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetOutputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput8VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetOutputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput1VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetInputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput2VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetInputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput3VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetInputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput4VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetInputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput5VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetInputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput6VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetInputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput7VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetInputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput8VerticalInterruptCount (ULWord *pCount);		///< @deprecated	Use GetInputVerticalInterruptCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioInterruptCount (ULWord *pCount);				///< @deprecated	Use GetInterruptEventCount(INTERRUPT_ENUMS,ULWord&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioInWrapInterruptCount (ULWord *pCount);			///< @deprecated	Use GetInterruptEventCount(INTERRUPT_ENUMS,ULWord&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioOutWrapInterruptCount (ULWord *pCount);			///< @deprecated	Use GetInterruptEventCount(INTERRUPT_ENUMS,ULWord&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAuxVerticalInterruptCount (ULWord *pCount);			///< @deprecated	Use GetInterruptEventCount(INTERRUPT_ENUMS,ULWord&) instead.

		//	Get event counts
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutputVerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetOutputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput2VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetOutputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput3VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetOutputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput4VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetOutputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput5VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetOutputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput6VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetOutputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput7VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetOutputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetOutput8VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetOutputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput1VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetInputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput2VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetInputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput3VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetInputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput4VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetInputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput5VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetInputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput6VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetInputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput7VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetInputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetInput8VerticalEventCount (ULWord *pCount);			///< @deprecated	Use GetInputVerticalEventCount(ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioInterruptEventCount (ULWord *pCount);			///< @deprecated	Use GetInterruptEventCount(INTERRUPT_ENUMS,ULWord&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioInWrapInterruptEventCount (ULWord *pCount);		///< @deprecated	Use GetInterruptEventCount(INTERRUPT_ENUMS,ULWord&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAudioOutWrapInterruptEventCount (ULWord *pCount);	///< @deprecated	Use GetInterruptEventCount(INTERRUPT_ENUMS,ULWord&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAuxVerticalEventCount (ULWord *pCount);				///< @deprecated	Use GetInterruptEventCount(INTERRUPT_ENUMS,ULWord&) instead.

		//	Set event counts
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetOutput2VerticalEventCount (ULWord Count);			///< @deprecated	Use SetOutputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetOutput3VerticalEventCount (ULWord Count);			///< @deprecated	Use SetOutputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetOutput4VerticalEventCount (ULWord Count);			///< @deprecated	Use SetOutputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetOutput5VerticalEventCount (ULWord Count);			///< @deprecated	Use SetOutputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetOutput6VerticalEventCount (ULWord Count);			///< @deprecated	Use SetOutputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetOutput7VerticalEventCount (ULWord Count);			///< @deprecated	Use SetOutputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetOutput8VerticalEventCount (ULWord Count);			///< @deprecated	Use SetOutputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetInput1VerticalEventCount (ULWord Count);				///< @deprecated	Use SetInputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetInput2VerticalEventCount (ULWord Count);				///< @deprecated	Use SetInputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetInput3VerticalEventCount (ULWord Count);				///< @deprecated	Use SetInputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetInput4VerticalEventCount (ULWord Count);				///< @deprecated	Use SetInputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetInput5VerticalEventCount (ULWord Count);				///< @deprecated	Use SetInputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetInput6VerticalEventCount (ULWord Count);				///< @deprecated	Use SetInputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetInput7VerticalEventCount (ULWord Count);				///< @deprecated	Use SetInputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetInput8VerticalEventCount (ULWord Count);				///< @deprecated	Use SetInputVerticalEventCount(ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioInterruptEventCount (ULWord Count);				///< @deprecated	Use SetInterruptEventCount(INTERRUPT_ENUMS,ULWord) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioInWrapInterruptEventCount (ULWord Count);		///< @deprecated	Use SetInterruptEventCount(INTERRUPT_ENUMS,ULWord) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAudioOutWrapInterruptEventCount (ULWord Count);		///< @deprecated	Use SetInterruptEventCount(INTERRUPT_ENUMS,ULWord) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAuxVerticalEventCount (ULWord Count);				///< @deprecated	Use SetInterruptEventCount(INTERRUPT_ENUMS,ULWord) instead.

		//	Wait for event
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForVerticalInterrupt();								///< @deprecated	Use WaitForOutputVerticalInterrupt or WaitForInputVerticalInterrupt, as appropriate, instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput1VerticalInterrupt();						///< @deprecated	Use WaitForOutputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput2VerticalInterrupt();						///< @deprecated	Use WaitForOutputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput3VerticalInterrupt();						///< @deprecated	Use WaitForOutputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput4VerticalInterrupt();						///< @deprecated	Use WaitForOutputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput5VerticalInterrupt();						///< @deprecated	Use WaitForOutputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput6VerticalInterrupt();						///< @deprecated	Use WaitForOutputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput7VerticalInterrupt();						///< @deprecated	Use WaitForOutputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput8VerticalInterrupt();						///< @deprecated	Use WaitForOutputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForFieldID (NTV2FieldID fieldID);					///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput1FieldID (NTV2FieldID fieldID);			///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput2FieldID (NTV2FieldID fieldID);			///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput3FieldID (NTV2FieldID fieldID);			///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput4FieldID (NTV2FieldID fieldID);			///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput5FieldID (NTV2FieldID fieldID);			///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput6FieldID (NTV2FieldID fieldID);			///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput7FieldID (NTV2FieldID fieldID);			///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForOutput8FieldID (NTV2FieldID fieldID);			///< @deprecated	Use WaitForOutputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput1FieldID (NTV2FieldID fieldID);				///< @deprecated	Use WaitForInputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput2FieldID (NTV2FieldID fieldID);				///< @deprecated	Use WaitForInputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput3FieldID (NTV2FieldID fieldID);				///< @deprecated	Use WaitForInputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput4FieldID (NTV2FieldID fieldID);				///< @deprecated	Use WaitForInputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput5FieldID (NTV2FieldID fieldID);				///< @deprecated	Use WaitForInputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput6FieldID (NTV2FieldID fieldID);				///< @deprecated	Use WaitForInputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput7FieldID (NTV2FieldID fieldID);				///< @deprecated	Use WaitForInputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput8FieldID (NTV2FieldID fieldID);				///< @deprecated	Use WaitForInputFieldID(NTV2FieldID,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput1Vertical();								///< @deprecated	Use WaitForInputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput2Vertical();								///< @deprecated	Use WaitForInputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput3Vertical();								///< @deprecated	Use WaitForInputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput4Vertical();								///< @deprecated	Use WaitForInputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput5Vertical();								///< @deprecated	Use WaitForInputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput6Vertical();								///< @deprecated	Use WaitForInputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput7Vertical();								///< @deprecated	Use WaitForInputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForInput8Vertical();								///< @deprecated	Use WaitForInputVerticalInterrupt(NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForAudioInterrupt();								///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForAudioInWrapInterrupt();							///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForAudioOutWrapInterrupt();							///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForUartTxInterruptEvent(ULWord timeoutMS=15);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForUart2TxInterruptEvent(ULWord timeoutMS=15);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForUartRxInterruptEvent(ULWord timeoutMS=15);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForUart2RxInterruptEvent(ULWord timeoutMS=15);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForHDMIHotplugInterruptEvent(ULWord timeoutMS=15);	///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForAuxVerticalInterrupt();							///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForDMA1Interrupt();									///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForDMA2Interrupt();									///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForDMA3Interrupt();									///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForDMA4Interrupt();									///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForPushButtonChangeInterrupt(ULWord timeoutMS=200);	///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForLowPowerInterrupt(ULWord timeoutMS=1000);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForDisplayFIFOInterrupt(ULWord timeoutMS=1000);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForSATAChangeInterrupt(ULWord timeoutMS=200);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForTemp1HighInterrupt(ULWord timeoutMS=1000);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForTemp2HighInterrupt(ULWord timeoutMS=1000);		///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForPowerButtonChangeInterrupt(ULWord timeoutMS=1000);	///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	WaitForChangeEvent();									///< @deprecated	Use WaitForInterrupt(INTERRUPT_ENUMS) instead.
	#endif	//	!defined (NTV2_DEPRECATE)
	///@}


	//
	// Color Correction Functions (KHD only )
	//
	AJA_VIRTUAL bool	SetColorCorrectionMode(NTV2Channel channel, NTV2ColorCorrectionMode mode);
	AJA_VIRTUAL bool	GetColorCorrectionMode(NTV2Channel channel, NTV2ColorCorrectionMode *mode);
	AJA_VIRTUAL bool	SetColorCorrectionOutputBank (NTV2Channel channel, ULWord bank);
	AJA_VIRTUAL bool	GetColorCorrectionOutputBank (NTV2Channel channel, ULWord *bank);
	AJA_VIRTUAL bool	SetColorCorrectionHostAccessBank (NTV2ColorCorrectionHostAccessBank value);
	AJA_VIRTUAL bool	GetColorCorrectionHostAccessBank (NTV2ColorCorrectionHostAccessBank *value, NTV2Channel channel = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	SetColorCorrectionSaturation (NTV2Channel channel, ULWord value);
	AJA_VIRTUAL bool	GetColorCorrectionSaturation (NTV2Channel channel, ULWord *value);

	AJA_VIRTUAL bool	SetDitherFor8BitInputs (NTV2Channel channel, ULWord dither);
	AJA_VIRTUAL bool	GetDitherFor8BitInputs (NTV2Channel channel, ULWord* dither);

	AJA_VIRTUAL bool	SetForce64(ULWord force64);
	AJA_VIRTUAL bool	GetForce64(ULWord* force64);
	AJA_VIRTUAL bool	Get64BitAutodetect(ULWord* autodetect64);
	AJA_VIRTUAL bool	GetFirmwareRev(ULWord* firmwareRev);


	/**
		@name	AutoCirculate
	**/
	///@{
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool		InitAutoCirculate (NTV2Crosspoint inChannelSpec,
																LWord inStartFrameNumber,
																LWord inEndFrameNumber,
																bool bWithAudio				= false,
																bool bWithRP188				= false,
																bool bFbfChange				= false,
																bool bFboChange				= false,
																bool bWithColorCorrection	= false,
																bool bWithVidProc			= false,
																bool bWithCustomAncData		= false,
																bool bWithLTC				= false,
																bool bUseAudioSystem2		= false);	///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)
	#if !defined (NTV2_DEPRECATE_12_6)
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	InitAutoCirculate (NTV2Crosspoint	inChannelSpec,
																LWord			inStartFrameNumber,
																LWord			inEndFrameNumber,
																LWord			inNumChannels,
																NTV2AudioSystem	inAudioSystem,
																bool			bWithAudio				= false,
																bool			bWithRP188				= false,
																bool			bFbfChange				= false,
																bool			bFboChange				= false,
																bool			bWithColorCorrection	= false,
																bool			bWithVidProc			= false,
																bool			bWithCustomAncData		= false,
																bool			bWithLTC				= false);	///< @deprecated	Use CNTV2Card::AutoCirculateInitForInput or CNTV2Card::AutoCirculateInitForOutput instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	StartAutoCirculate (const NTV2Crosspoint inChannelSpec, const ULWord64 inStartTime = 0);			///< @deprecated	Use CNTV2Card::AutoCirculateStart instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	StartAutoCirculateAtTime (NTV2Crosspoint channelSpec, ULWord64 startTime) {return StartAutoCirculate (channelSpec, startTime);}		///< @deprecated	Use CNTV2Card::AutoCirculateStart instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	StopAutoCirculate (NTV2Crosspoint channelSpec);														///< @deprecated	Use CNTV2Card::AutoCirculateStop instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	AbortAutoCirculate (NTV2Crosspoint channelSpec);													///< @deprecated	Use CNTV2Card::AutoCirculateStop instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	PauseAutoCirculate (NTV2Crosspoint channelSpec, bool bPlay);										///< @deprecated	Use CNTV2Card::AutoCirculatePause or CNTV2Card::AutoCirculateResume instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	GetFrameStampEx2 (NTV2Crosspoint channelSpec, ULWord frameNum,
															FRAME_STAMP_STRUCT* pFrameStamp,
															PAUTOCIRCULATE_TASK_STRUCT pTaskStruct = NULL);											///< @deprecated	Use CNTV2Card::AutoCirculateGetFrame instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	FlushAutoCirculate (NTV2Crosspoint channelSpec);													///< @deprecated	Use CNTV2Card::AutoCirculateFlush instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	SetActiveFrameAutoCirculate (NTV2Crosspoint channelSpec, ULWord lActiveFrame);						///< @deprecated	Use CNTV2Card::AutoCirculateSetActiveFrame instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	PrerollAutoCirculate (NTV2Crosspoint channelSpec, ULWord lPrerollFrames);							///< @deprecated	Use CNTV2Card::AutoCirculatePreRoll instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	TransferWithAutoCirculate (PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
																		 PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct);				///< @deprecated	Use CNTV2Card::AutoCirculateTransfer instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	TransferWithAutoCirculateEx(PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
																		PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct,
																		NTV2RoutingTable* pXena2RoutingTable = NULL);								///< @deprecated	Use CNTV2Card::AutoCirculateTransfer instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	TransferWithAutoCirculateEx2(PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
																		PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct,
																		NTV2RoutingTable* pXena2RoutingTable = NULL,
																		PAUTOCIRCULATE_TASK_STRUCT pTaskStruct = NULL);								///< @deprecated	Use CNTV2Card::AutoCirculateTransfer instead.
		AJA_VIRTUAL NTV2_DEPRECATED_12_6 bool	SetAutoCirculateCaptureTask(NTV2Crosspoint channelSpec, PAUTOCIRCULATE_TASK_STRUCT pTaskStruct);	///< @deprecated	Use CNTV2Card::AutoCirculateTransfer instead.
	#endif	//	!defined (NTV2_DEPRECATE_12_6)
	AJA_VIRTUAL NTV2_DEPRECATED bool	GetFrameStamp (NTV2Crosspoint channelSpec, ULWord frameNum, FRAME_STAMP_STRUCT* pFrameStamp);		///< @deprecated	Use CNTV2Card::AutoCirculateGetFrame instead.
	AJA_VIRTUAL NTV2_DEPRECATED bool	GetAutoCirculate (NTV2Crosspoint channelSpec, AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus);	///< @deprecated	Use CNTV2Card::AutoCirculateGetStatus instead.


	/**
		@brief		Prepares for subsequent AutoCirculate ingest operation by reserving and dedicating a contiguous block
					of frame buffers on the AJA device for exclusive use. It also specifies other optional behaviors.
					Upon successful return, the driver's AutoCirculate status for the given channel will be NTV2_AUTOCIRCULATE_INIT.
					Callers can bypass the default frame buffer allocator by specifying zero for the "inFrameCount" parameter, and
					explicitly specifying starting and ending frame numbers using the "inStartFrameNumber" and "inEndFrameNumber" parameters.

		@return		True if successful; otherwise false.

		@param[in]		inChannel				Specifies the NTV2Channel to use. Some devices will not have all of the possible input
												channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
												are on the device.

		@param[in]		inFrameCount			Specifies the number of contiguous device frame buffers to be continuously cycled through.
												Defaults to 7. Specify zero to explicitly specify the starting and ending frame numbers
												and avoid using the default frame buffer allocator (see "inStartFrameNumber" and "inEndFrameNumber"
												parameters below).

		@param[in]		inAudioSystem			Specifies the audio system to use, if any. Defaults to NTV2_AUDIOSYSTEM_INVALID (no audio).

		@param[in]		inOptionFlags			A bit mask that specifies additional AutoCirculate options (e.g., AUTOCIRCULATE_WITH_RP188,
												AUTOCIRCULATE_WITH_LTC, AUTOCIRCULATE_WITH_ANC, etc.). Defaults to zero (no options).

		@param[in]		inNumChannels			Optionally specifies the number of channels to operate on when CNTV2Card::AutoCirculateStart or
												CNTV2Card::AutoCirculateStop are called. Defaults to 1. Must be greater than zero.

		@param[in]		inStartFrameNumber		Optionally specifies the starting frame number as a zero-based unsigned decimal integer.
												Defaults to zero. This parameter is ignored if "inFrameCount" is non-zero.

		@param[in]		inEndFrameNumber		Optionally specifies the ending frame number as a zero-based unsigned decimal integer.
												Defaults to zero. This parameter is ignored if "inFrameCount" is non-zero.

		@details	If this function returns true, the driver will have reserved a contiguous set of device frame buffers, and placed the
					specified channel into the "initialized" state (NTV2_AUTOCIRCULATE_INIT). The channel will then be ready for a subsequent
					call to CNTV2Card::AutoCirculateStart or CNTV2Card::AutoCirculateTransfer.
					AutoCirculateInitForInput's behavior depends on the device's "every frame task mode" (NTV2EveryFrameTaskMode). This mode can
					be discovered by calling CNTV2Card::GetEveryFrameServices, and can be changed by calling CNTV2Card::SetEveryFrameServices.
					If the device's task mode is set to "OEM Tasks" (NTV2_OEM_TASKS), the driver will perform most of the device setup,
					including setting the input format or output standard, enabling the frame store, setting the frame store's mode, etc.
					The driver will not, however, perform routing changes. All widget routing must be completed prior to calling CNTV2Card::AutoCirculateInitForInput.
					If the device's task mode is set to "Disable Tasks" (NTV2_DISABLE_TASKS), on some platforms, the driver will not
					perform any device setup. In this case, all aspects of the device -- the frame store mode, output or input standards, etc. --
					must be configured properly before calling InitAutoCirculate.
					If the device's task mode is set to "Standard Tasks" (NTV2_STANDARD_TASKS), and the retail mode service is running on the host,
					the device configuration will be dictated by the device's current "AJA ControlPanel" settings. In this case, the ControlPanel
					settings should agree with what CNTV2Card::AutoCirculateInitForInput is being asked to do. For example, setting the device output to
					"Test Pattern" in the Control Panel, then calling CNTV2Card::AutoCirculateInitForInput is contradictory, because AutoCirculate is being
					asked to capture from a device that's configured to playout a test pattern.
		@bug		If "inFrameCount" is non-zero, this function relies on a frame buffer allocator that is not currently thread-safe.
					If this allocator is called from 2 or more threads (or processes), a race condition exists that may result in at least
					two of the channels having overlapping frame buffer ranges. This will be addressed in a future SDK. Meanwhile, clients
					can gate calls to CNTV2Card::AutoCirculateInitForInput and/or CNTV2Card::AutoCirculateInitForOutput using a Mutex or Critical Section.
	**/

	AJA_VIRTUAL bool	AutoCirculateInitForInput (	const NTV2Channel		inChannel,
													const UByte				inFrameCount		= 7,
													const NTV2AudioSystem	inAudioSystem		= NTV2_AUDIOSYSTEM_INVALID,
													const ULWord			inOptionFlags		= 0,
													const UByte				inNumChannels		= 1,
													const UByte				inStartFrameNumber	= 0,
													const UByte				inEndFrameNumber	= 0);

	/**
		@brief		Prepares for subsequent AutoCirculate playout operations by reserving and dedicating a contiguous block
					of frame buffers on the AJA device for exclusive use. It also specifies other optional behaviors.
					Upon successful return, the driver's AutoCirculate status for the given channelSpec will be NTV2_AUTOCIRCULATE_INIT.

		@return		True if successful; otherwise false.

		@param[in]		inChannel				Specifies the NTV2Channel to use. Some devices will not have all of the possible output
												channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
												are on the device.

		@param[in]		inFrameCount			Specifies the number of contiguous device frame buffers to be continuously cycled through.
												Defaults to 7. Specify zero to explicitly specify the starting and ending frame numbers
												and avoid using the default frame buffer allocator (see "inStartFrameNumber" and "inEndFrameNumber"
												parameters below).

		@param[in]		inAudioSystem			Specifies the audio system to use, if any. Defaults to NTV2_AUDIOSYSTEM_INVALID (no audio).

		@param[in]		inOptionFlags			A bit mask that specifies additional AutoCirculate options (e.g., AUTOCIRCULATE_WITH_RP188,
												AUTOCIRCULATE_WITH_LTC, AUTOCIRCULATE_WITH_ANC, etc.). Defaults to zero (no options).

		@param[in]		inNumChannels			Optionally specifies the number of channels to operate on when CNTV2Card::AutoCirculateStart or
												CNTV2Card::AutoCirculateStop are called. Defaults to 1. Must be greater than zero.

		@param[in]		inStartFrameNumber		Optionally specifies the starting frame number as a zero-based unsigned decimal integer.
												Defaults to zero. This parameter is ignored if "inFrameCount" is non-zero.

		@param[in]		inEndFrameNumber		Optionally specifies the ending frame number as a zero-based unsigned decimal integer.
												Defaults to zero. This parameter is ignored if "inFrameCount" is non-zero.

		@details	If this function returns true, the driver will have reserved a contiguous set of device frame buffers, and placed the
					specified channel into the "initialized" state (NTV2_AUTOCIRCULATE_INIT). The channel will then be ready for a subsequent
					call to CNTV2Card::AutoCirculateStart or CNTV2Card::AutoCirculateTransfer.
					CNTV2Card::AutoCirculateInitForOutput's behavior depends on the device's "every frame task mode" (NTV2EveryFrameTaskMode). This mode can
					be discovered by calling CNTV2Card::GetEveryFrameServices, and can be changed by calling CNTV2Card::SetEveryFrameServices.
					If the device's task mode is set to "OEM Tasks" (NTV2_OEM_TASKS), the driver will perform most of the device setup,
					including setting the output format and/or standard, enabling the frame store, setting the frame store's mode to playout, etc.
					The driver will not, however, perform routing changes. All widget routing must be completed prior to calling CNTV2Card::AutoCirculateInitForOutput.
					If the device's task mode is set to "Disable Tasks" (NTV2_DISABLE_TASKS), on some platforms, the driver will not
					perform any device setup. In this case, all aspects of the device -- the frame store mode, output standard, etc. --
					must be configured properly before calling CNTV2Card::AutoCirculateInitForOutput.
					If the device's task mode is set to "Standard Tasks" (NTV2_STANDARD_TASKS), and the retail mode service is running on the host,
					the device configuration will be dictated by the device's current "AJA ControlPanel" settings. In this case, the ControlPanel
					settings should agree with what CNTV2Card::AutoCirculateInitForOutput is being asked to do. For example, setting the device output
					to "Input Pass-Through" in the ControlPanel, then calling CNTV2Card::AutoCirculateInitForOutput is contradictory, because AutoCirculate
					is being asked asking to playout video through a device that's configured to capture/pass-thru incoming video.
		@bug		If "inFrameCount" is non-zero, this function relies on a frame buffer allocator that is not currently thread-safe.
					If this allocator is called from 2 or more threads (or processes), a race condition exists that may result in at least
					two of the channels having overlapping frame buffer ranges. This will be addressed in a future SDK. Meanwhile, clients
					can gate calls to CNTV2Card::AutoCirculateInitForInput and/or CNTV2Card::AutoCirculateInitForOutput using a Mutex or Critical Section.
	**/

	AJA_VIRTUAL bool	AutoCirculateInitForOutput (const NTV2Channel		inChannel,
													const UByte				inFrameCount		= 7,
													const NTV2AudioSystem	inAudioSystem		= NTV2_AUDIOSYSTEM_INVALID,
													const ULWord			inOptionFlags		= 0,
													const UByte				inNumChannels		= 1,
													const UByte				inStartFrameNumber	= 0,
													const UByte				inEndFrameNumber	= 0);

	/**
		@brief		Starts AutoCirculating the specified channel that was previously initialized by AutoCirculateInitForInput or
					AutoCirculateInitForOutput.

		@return		True if successful; otherwise false.

		@param[in]		inChannel		Specifies the NTV2Channel to use. Some devices will not have all of the possible input
										channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
										are on the device.

		@param[in]		inStartTime		Optionally specifies a future start time as an unsigned 64-bit "tick count" value that
										is host-OS-dependent. If set to zero, the default, AutoCirculate will switch to the
										"running" state at the next VBI received by the given channel. If non-zero, AutoCirculate
										will remain in the "starting" state until the system tick clock exceeds this value, at
										which point it will switch to the "running" state. This value is denominated in the same
										time units as the finest-grained time counter available on the host's operating system.

		@details	This function sets the state of the channel in the driver from "initializing" to "starting", then at the next
					VBI, the driver's interrupt service routine (ISR) will check the OS tick clock, and if it exceeds the given
					start time value, it will proceed to start AutoCirculate -- otherwise it will remain in the "starting" phase,
					and recheck the clock at the next VBI. The driver will start tracking which memory frames are available and
					which are empty, and will change the channel's status to "running".
					When capturing, the next frame (to be recorded) is determined, and the current last input audio sample is
					written into the next frame's FRAME_STAMP's acAudioInStartAddress field. Finally, the channel's active frame
					is set to the next frame number.
					During playout, the next frame (to go out the jack) is determined, and the current last output audio sample
					is written into the next frame's FRAME_STAMP's acAudioOutStartAddress field. Finally, the channel's active
					frame is set to the next frame number. Henceforth, the driver will AutoCirculate frames at every VBI on a
					per-channel basis.

		@note		This method will fail if the specified channel's AutoCirculate state is not "initializing".

		@note		Calling AutoCirculateStart while in the "paused" state will not un-pause AutoCirculate, but instead will
					restart it.
	**/
	AJA_VIRTUAL bool	AutoCirculateStart (const NTV2Channel inChannel, const ULWord64 inStartTime = 0);

	/**
		@brief		Stops AutoCirculate for the given channel, and releases the on-device frame buffers that were allocated to it.

		@return		True if successful; otherwise false.

		@param[in]		inChannel		Specifies the NTV2Channel to use. Some devices will not have all of the possible input
										channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
										are on the device.

		@param[in]		inAbort			Specifies if AutoCirculate is to be immediately stopped, not gracefully.
										Defaults to false (graceful stop).

		@details	If asked to stop gracefully (the default), the channel's AutoCirculate state is set to "stopping", and at the
					next VBI, AutoCirculate is explicitly stopped, after which the channel's AutoCirculate state is set to "disabled".
					Once this method has been called, the channel cannot be used until it gets reinitialized by a subsequent call
					to CNTV2Card::AutoCirculateInitForInput or CNTV2Card::AutoCirculateInitForOutput.
					When called with <i>inAbort</i> set to 'true', audio capture or playback is immediately stopped (if a valid audio system
					was specified at initialization time), and the AutoCirculate channel status is changed to "disabled".
	**/
	AJA_VIRTUAL bool	AutoCirculateStop (const NTV2Channel inChannel, const bool inAbort = false);

	/**
		@brief		Pauses AutoCirculate for the given channel. Once paused, AutoCirculate can be resumed later by calling
					CNTV2Card::AutoCirculateResume, picking up at the next frame without any loss of audio synchronization.

		@return		True if successful; otherwise false.

		@param[in]		inChannel		Specifies the NTV2Channel to use. Some devices will not have all of the possible input
										channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
										are on the device.

		@details	When pausing, if the channel is in the "running" state, it will be set to "paused", and at the next VBI, the driver
					will explicitly stop audio circulating.
	**/
	AJA_VIRTUAL bool	AutoCirculatePause (const NTV2Channel inChannel);

	/**
		@brief		Resumes AutoCirculate for the given channel, picking up at the next frame without loss of audio synchronization.

		@return		True if successful; otherwise false.

		@param[in]		inChannel		Specifies the NTV2Channel to use. Some devices will not have all of the possible input
										channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
										are on the device.

		@param[in]		inClearDropCount	Specify 'true' to clear the AUTOCIRCULATE_STATUS::acFramesDropped counter; otherwise
											leaves it unchanged. Defaults to 'false' (don't clear it).

		@details	When resuming, if the channel is in the "paused" state, it will be changed to "running", and at the next VBI, the
					driver will restart audio AutoCirculate.
	**/
	AJA_VIRTUAL bool	AutoCirculateResume (const NTV2Channel inChannel, const bool inClearDropCount = false);

	/**
		@brief		Flushes AutoCirculate for the given channel.

		@return		True if successful; otherwise false.

		@param[in]		inChannel		Specifies the NTV2Channel to use. Some devices will not have all of the possible input
										channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
										are on the device.

        @param[in]		inClearDropCount	Specify 'true' to clear the AUTOCIRCULATE_STATUS::acFramesDropped counter; otherwise
                                            leaves it unchanged. Defaults to 'false' (don't clear it).

		@details	On capture, flushes all recorded frames that haven't yet been transferred to the host.
					On playout, all queued frames that have already been transferred to the device (that haven't yet played)
					are discarded.
					In either mode, this function has no effect on the Active Frame (the frame currently being captured
					or played by the device hardware at the moment the function was called).
					The NTV2AutoCirculateState ("running", etc.) for the given channel will remain unchanged.
	**/
    AJA_VIRTUAL bool	AutoCirculateFlush (const NTV2Channel inChannel, const bool inClearDropCount = false);

	/**
		@brief		Tells AutoCirculate how many frames to skip before playout starts for the given channel.

		@return		True if successful; otherwise false.

		@param[in]		inChannel		Specifies the NTV2Channel to use. Some devices will not have all of the possible input
										channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
										are on the device.

		@param[in]		inPreRollFrames	Specifies the number of frames to skip (ignore) before starting AutoCirculate.

		@details	Normally used for playout, this method instructs the driver to mark the given number of frames as valid.
					It's useful only in the rare case when, after CNTV2Card::AutoCirculateInitForOutput was called, several frames have
					already been transferred to the device (perhaps using CNTV2Card::DMAWrite), and calling CNTV2Card::AutoCirculateStart will
					ignore those pre-rolled frames without an intervening CNTV2Card::AutoCirculateTransfer call.

		@note		This method does nothing if the channel's state is not currently "starting", "running" or "paused",
					or if the channel was initialized by CNTV2Card::AutoCirculateInitForInput.
	**/
	AJA_VIRTUAL bool	AutoCirculatePreRoll (const NTV2Channel inChannel, const ULWord inPreRollFrames);

	/**
		@brief		Returns the current AutoCirculate status for the given channel.

		@return		True if successful; otherwise false.

		@param[in]		inChannel		Specifies the NTV2Channel to use. Some devices will not have all of the possible input
										channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
										are on the device.

		@param[out]		outStatus		Receives the AUTOCIRCULATE_STATUS information for the channel.

		@details	Clients can use the AUTOCIRCULATE_STATUS information to determine if there are sufficient readable frames
					in the driver to safely support a DMA transfer to host memory (for capture);  or to determine if any frames
					have been dropped.
	**/
	AJA_VIRTUAL bool	AutoCirculateGetStatus (const NTV2Channel inChannel, AUTOCIRCULATE_STATUS & outStatus);


	/**
		@brief		Returns precise timing information for the given frame and channel that's currently AutoCirculating.

		@return		True if successful; otherwise false.

		@param[in]		inChannel				Specifies the NTV2Channel to use. Some devices will not have all of the possible input
												channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
												are on the device.

		@param[in]		inFrameNumber			Specifies the zero-based frame number of interest. This value must be no less than <i>acStartFrame</i>
												and no more than <i>acEndFrame</i> for the given channel. For capture/ingest, it should be "behind
												the record head". For playout, it should be "behind the play head."

		@param[out]		outFrameInfo			Receives the FRAME_STAMP information for the given frame number and channel.

		@details		When the given channel is AutoCirculating, the driver will continuously fill in a FRAME_STAMP record for the frame
						it's currently working on, which is intended to give enough information to determine if frames have been dropped
						either on input or output. Moreover, it allows for synchronization of audio and video by time-stamping the audio
						input address at the start and end of a video frame.
	**/
	AJA_VIRTUAL bool	AutoCirculateGetFrameStamp (const NTV2Channel inChannel, const ULWord inFrameNumber, FRAME_STAMP & outFrameInfo);

	/**
		@brief		Immediately changes the Active Frame for the given channel.

		@return		True if successful; otherwise false.

		@param[in]		inChannel			Specifies the NTV2Channel to use. Some devices will not have all of the possible input
											channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
											are on the device.

		@param[in]		inNewActiveFrame	Specifies the zero-based frame number to use. This value must be no less than <i>acStartFrame</i>
											and no more than <i>acEndFrame</i> for the given channel (see AUTOCIRCULATE_STATUS).

		@details	This method, assuming it succeeds, changes the Active Frame for the given channel.
					The device driver accomplishes this by changing the Active Frame register (input or output) for the given channel.

		@note		When one of these registers change on the device, it won't take effect until the next VBI, which ensures, for
					example, that an outgoing frame won't suddenly change mid-frame.

		@note		This method does nothing if the channel's AutoCirculate state is not currently "starting", "running" or "paused".
	**/
	AJA_VIRTUAL bool	AutoCirculateSetActiveFrame (const NTV2Channel inChannel, const ULWord inNewActiveFrame);

	/**
		@brief		Transfers all or part of a frame as specified in the given AUTOCIRCULATE_TRANSFER object to/from the host.

		@param[in]		inChannel				Specifies the NTV2Channel to use. Some devices will not have all of the possible input
												channels. Use the NTV2DeviceGetNumFrameStores function to discover how many frame stores
												are on the device.

		@param[in]		inOutTransferInfo		Specifies the AUTOCIRCULATE_TRANSFER information to use, which specifies the transfer
												details.

		@details	It is recommended that this method be called from inside a loop in a separate execution thread, with a way to gracefully
					exit the loop. Once outside of the loop, CNTV2Card::AutoCirculateStop can then be called. It is the application's responsibility
					to provide valid video, audio and ancillary data pointers (and byte counts) to the transfer object via its AUTOCIRCULATE_TRANSFER::SetBuffers
					function.

		@note		Do not call this method using a channel that was not previously initialized with a call to CNTV2Card::AutoCirculateInitForInput
					or CNTV2Card::AutoCirculateInitForOutput. The channel's AutoCirculate state must not be "disabled".

		@note		The calling thread will block until the transfer completes (or fails).
	**/
	AJA_VIRTUAL bool	AutoCirculateTransfer (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutTransferInfo);

	/**
		@brief		Returns the device frame buffer numbers of the first unallocated contiguous band of frame buffers having the given
					size that are available for use with AutoCirculate.

		@param[in]		inFrameCount			Specifies the desired number of contiguous device frame buffers. Must exceed zero.

		@param[out]		outStartFrameNumber		Receives the starting device frame buffer number.

		@param[out]		outEndFrameNumber		Receives the ending device frame buffer number.

		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	FindUnallocatedFrames (const UByte inFrameCount, LWord & outStartFrameNumber, LWord & outEndFrameNumber);

	/**
		@brief			Reads the register(s) specified by the given NTV2RegInfo sequence.
		@param[in]		inOutValues		Specifies the register(s) to be read, and upon return, receives their values.
		@return			True if all registers were read successfully; otherwise false.
		@note			This operation is not guaranteed to be performed atomically.
	**/
	AJA_VIRTUAL bool	ReadRegisters (NTV2RegisterReads & inOutValues);

	/**
		@brief			Reads the given set of registers.
		@param[in]		inRegisters				Specifies the set of registers to be read.
		@param[out]		outValues				Receives the resulting register/value map. Any registers in the "inRegisters" set that don't
												appear in this map were not able to be read successfully.
		@return			True if all requested registers were successfully read; otherwise false.
		@note			This operation is not guaranteed to be performed atomically. A VBI may occur while the requested registers are being read.
	**/
	AJA_VIRTUAL bool	ReadRegisters (const NTV2RegNumSet & inRegisters,  NTV2RegisterValueMap & outValues);

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
		@brief			For devices that support it (see the ::NTV2DeviceCanDoSDIErrorChecks function in "ntv2devicefeatures.h"),
						this function fetches the SDI statistics for all SDI input spigots.
		@param[out]		outStats	Receives the SDI statistics for all SDI input spigots.
		@return			True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	ReadSDIStatistics (NTV2SDIInStatistics & outStats);
	///@}

	/**
		@brief		Sets the frame size used on the device.
		@param[in]	inChannel	Specifies the frame store to be affected. (Currently ignored -- see note below.)
		@param[in]	inValue		Specifies the new frame size. Must be NTV2_FRAMESIZE_8MB or NTV2_FRAMESIZE_16MB.
		@return		True if successful;  otherwise false.
		@note		Currently, all NTV2 devices globally use an 8MB or 16MB frame size across any/all frame stores.
					When a frame store is told to use a particular frame buffer format and frame geometry, the device will
					automatically switch to the smallest size that will safely accommodate the frame data. You can use this function
					to override the default. For example, channel 1 may be capturing 525i2997 '2vuy' video with AutoCirculate, which only
					requires 8MB frames by default. Starting a second channel to playback 2K1080p RGB10 video would automatically bump
					the device to 16MB frames, which would result in the capture of several "glitched" frames in Channel 1. To prevent
					the glitch, call this function to set 16MB frames before starting capture in Channel 1.
	**/
	AJA_VIRTUAL bool	SetFrameBufferSize(NTV2Channel inChannel, NTV2Framesize inValue);

	/**
		@brief		Answers with the frame size currently being used on the device.
		@param[in]	inChannel	Currently ignored. Use NTV2_CHANNEL1.
		@param[out]	outValue	Receives the device's current frame size.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetFrameBufferSize (NTV2Channel inChannel, NTV2Framesize & outValue);
	AJA_VIRTUAL bool	GetFrameBufferSize (NTV2Channel inChannel, NTV2Framesize * pOutValue)		{return pOutValue ? GetFrameBufferSize (inChannel, *pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.
	using CNTV2DriverInterface::GetFrameBufferSize;		//	Keep CNTV2DriverInterface::GetFrameBufferSize visible after being shadowed by CNTV2Card::GetFrameBufferSize

	/**
		@brief		Disables the given frame store.
		@param[in]	inChannel	Specifies the frame store, as identified by an NTV2Channel value.
		@return		True if successful;  otherwise false.
		@note		It is not an error to disable a frame store that is already disabled.
	**/
	AJA_VIRTUAL bool	DisableChannel (const NTV2Channel inChannel);

	/**
		@brief		Enables the given frame store.
		@param[in]	inChannel	Specifies the frame store, as identified by an NTV2Channel value.
		@return		True if successful;  otherwise false.
		@note		It is not an error to enable a frame store that is already enabled.
	**/
	AJA_VIRTUAL bool	EnableChannel (const NTV2Channel inChannel);

	/**
		@brief		Answers whether or not the given frame store is enabled.
		@param[in]	inChannel	Specifies the frame store, as identified by an NTV2Channel value.
		@param[in]	outEnabled	Specifies a boolean variable that is to receive the value "true" if
								the frame store is enabled, or "false" if the frame store is disabled.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	IsChannelEnabled (const NTV2Channel inChannel, bool & outEnabled);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetChannel2Disable (bool value);								///< @deprecated	Use EnableChannel or DisableChannel instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetChannel2Disable (bool* value);								///< @deprecated	Use IsChannelEnabled instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetChannel3Disable (bool value);								///< @deprecated	Use EnableChannel or DisableChannel instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetChannel3Disable (bool* value);								///< @deprecated	Use IsChannelEnabled instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetChannel4Disable (bool value);								///< @deprecated	Use EnableChannel or DisableChannel instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetChannel4Disable (bool* value);								///< @deprecated	Use IsChannelEnabled instead.
	#endif	//	!defined (NTV2_DEPRECATE)

	AJA_VIRTUAL bool	SetVideoDACMode (NTV2VideoDACMode inValue);
	AJA_VIRTUAL bool	GetVideoDACMode (NTV2VideoDACMode & outValue);
	AJA_VIRTUAL bool	GetVideoDACMode (NTV2VideoDACMode * pOutValue)		{return pOutValue ? GetVideoDACMode (*pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.


	/**
		@name	Timing/Offset Control
	**/
	///@{
	AJA_VIRTUAL bool	GetNominalMinMaxHV (int* nominalH, int* minH, int* maxH, int* nominalV, int* minV, int* maxV);
	AJA_VIRTUAL bool	SetVideoHOffset (int hOffset);
	AJA_VIRTUAL bool	GetVideoHOffset (int* hOffset);
	AJA_VIRTUAL bool	SetVideoVOffset (int vOffset);
	AJA_VIRTUAL bool	GetVideoVOffset (int* vOffset);
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetVideoFinePhase (int fOffset);		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetVideoFinePhase (int* fOffset);		///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)

	AJA_VIRTUAL bool	SetAnalogOutHTiming (ULWord inValue);
	AJA_VIRTUAL bool	GetAnalogOutHTiming (ULWord & outValue);
	AJA_VIRTUAL bool	GetAnalogOutHTiming (ULWord * pOutValue)			{return pOutValue ? GetAnalogOutHTiming (*pOutValue) : false;}	///< @deprecated	Use the alternate function that has the non-constant reference output parameter instead.

	/**
		@brief	Adjusts the output timing for the given SDI output spigot.
		@param[in]	inValue			Specifies the output timing control value to use. The lower 16 bits of this 32-bit value
									control the horizontal timing, while the upper 16 bits control the vertical.
									Each horizontal increment/decrement moves the output relative to the reference by one pixel.
									Each vertical increment/decrement moves the output relative to the reference by one line.
		@param[in]	inOutputSpigot	Optionally specifies the SDI output of interest. Defaults to zero (SDI Out 1).
		@note		The output timing can only be adjusted when the device's reference source is set for external reference.
		@note		The "inOutputSpigot" parameter is respected only if the device is multi-format-capable (see NTV2DeviceCanDoMultiFormat)
					and the device is currently in multi-format mode (see GetMultiFormatMode and SetMultiFormatMode). Otherwise,
					the timing is changed for all SDI outputs.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	WriteOutputTimingControl (const ULWord inValue, const UWord inOutputSpigot = 0);

	/**
		@brief	Returns the current output timing control value for the given SDI output spigot.
		@param[out]	outValue		Receives the current output timing control value.
		@param[in]	inOutputSpigot	Optionally specifies the SDI output spigot of interest. Defaults to 0 (SDI Out 1).
		@note		The "inOutputSpigot" parameter is respected only if the device is multi-format-capable (see NTV2DeviceCanDoMultiFormat)
					and the device is currently in multi-format mode (see GetMultiFormatMode and SetMultiFormatMode). Otherwise,
					this function only reports the timing for SDI Output 1 (i.e., the "global" output timing).
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	ReadOutputTimingControl (ULWord & outValue, const UWord inOutputSpigot = 0);
	AJA_VIRTUAL bool	ReadOutputTimingControl (ULWord * pOutValue, const UWord inOutputSpigot = 0)	{return pOutValue ? ReadOutputTimingControl (*pOutValue, inOutputSpigot) : false;}

	AJA_VIRTUAL bool	SetSDI1OutHTiming (ULWord value);
	AJA_VIRTUAL bool	GetSDI1OutHTiming(ULWord* value);
	AJA_VIRTUAL bool	SetSDI2OutHTiming (ULWord value);
	AJA_VIRTUAL bool	GetSDI2OutHTiming(ULWord* value);
	///@}

	/**
		@brief	Sets the SDI output spigot's video standard.
		@param[in]	inOutputSpigot	Specifies the SDI output spigot of interest as a zero-based index number, where zero is "SDI Out 1".
		@param[in]	inValue			Specifies the video standard.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetSDIOutputStandard (const UWord inOutputSpigot, const NTV2Standard inValue);

	/**
		@brief	Answers with the current video standard of the given SDI output spigot.
		@param[in]	inOutputSpigot	Specifies the SDI output spigot of interest as a zero-based index number, where zero is "SDI Out 1".
		@param[out]	outValue		Receives the video standard.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetSDIOutputStandard (const UWord inOutputSpigot, NTV2Standard & outValue);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetSDIOutStandard (const NTV2Standard value, const NTV2Channel channel = NTV2_CHANNEL1);	///< @deprecated	Use SetSDIOutputStandard instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDIOutStandard (NTV2Standard & outStandard, const NTV2Channel channel = NTV2_CHANNEL1);	///< @deprecated	Use GetSDIOutputStandard instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDIOutStandard (NTV2Standard* value, NTV2Channel channel);								///< @deprecated	Use GetSDIOutputStandard instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI1OutStandard (NTV2Standard value);		///< @deprecated	Use SetSDIOutputStandard(NTV2Channel,NTV2Standard) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI1OutStandard (NTV2Standard* value);		///< @deprecated	Use GetSDIOutputStandard(NTV2Channel,NTV2Standard&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI2OutStandard (NTV2Standard value);		///< @deprecated	Use SetSDIOutputStandard(NTV2Channel,NTV2Standard) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI2OutStandard (NTV2Standard* value);		///< @deprecated	Use GetSDIOutputStandard(NTV2Channel,NTV2Standard&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI3OutStandard (NTV2Standard value);		///< @deprecated	Use SetSDIOutputStandard(NTV2Channel,NTV2Standard) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI3OutStandard (NTV2Standard* value);		///< @deprecated	Use GetSDIOutputStandard(NTV2Channel,NTV2Standard&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI4OutStandard (NTV2Standard value);		///< @deprecated	Use SetSDIOutputStandard(NTV2Channel,NTV2Standard) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI4OutStandard (NTV2Standard* value);		///< @deprecated	Use GetSDIOutputStandard(NTV2Channel,NTV2Standard&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI5OutStandard (NTV2Standard value);		///< @deprecated	Use SetSDIOutputStandard(NTV2Channel,NTV2Standard) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI5OutStandard (NTV2Standard* value);		///< @deprecated	Use GetSDIOutputStandard(NTV2Channel,NTV2Standard&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI6OutStandard (NTV2Standard value);		///< @deprecated	Use SetSDIOutputStandard(NTV2Channel,NTV2Standard) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI6OutStandard (NTV2Standard* value);		///< @deprecated	Use GetSDIOutputStandard(NTV2Channel,NTV2Standard&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI7OutStandard (NTV2Standard value);		///< @deprecated	Use SetSDIOutputStandard(NTV2Channel,NTV2Standard) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI7OutStandard (NTV2Standard* value);		///< @deprecated	Use GetSDIOutputStandard(NTV2Channel,NTV2Standard&) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI8OutStandard (NTV2Standard value);		///< @deprecated	Use SetSDIOutputStandard(NTV2Channel,NTV2Standard) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI8OutStandard (NTV2Standard* value);		///< @deprecated	Use GetSDIOutputStandard(NTV2Channel,NTV2Standard&) instead.
	#endif	//	!NTV2_DEPRECATE

	AJA_VIRTUAL bool	SetSDIOutVPID (const ULWord inValueA, const ULWord inValueB, const UWord inOutputSpigot = NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetSDIOutVPID (ULWord & outValueA, ULWord & outValueB, const UWord inOutputSpigot = NTV2_CHANNEL1);
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI1OutVPID(ULWord valueA, ULWord valueB = 0);			///< @deprecated	Use SetSDIOutVPID(ULWord,ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI1OutVPID(ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use GetSDIOutVPID(ULWord&,ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI2OutVPID(ULWord valueA, ULWord valueB = 0);			///< @deprecated	Use SetSDIOutVPID(ULWord,ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI2OutVPID(ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use GetSDIOutVPID(ULWord&,ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI3OutVPID(ULWord valueA, ULWord valueB = 0);			///< @deprecated	Use SetSDIOutVPID(ULWord,ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI3OutVPID(ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use GetSDIOutVPID(ULWord&,ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI4OutVPID(ULWord valueA, ULWord valueB = 0);			///< @deprecated	Use SetSDIOutVPID(ULWord,ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI4OutVPID(ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use GetSDIOutVPID(ULWord&,ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI5OutVPID(ULWord valueA, ULWord valueB = 0);			///< @deprecated	Use SetSDIOutVPID(ULWord,ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI5OutVPID(ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use GetSDIOutVPID(ULWord&,ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI6OutVPID(ULWord valueA, ULWord valueB = 0);			///< @deprecated	Use SetSDIOutVPID(ULWord,ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI6OutVPID(ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use GetSDIOutVPID(ULWord&,ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI7OutVPID(ULWord valueA, ULWord valueB = 0);			///< @deprecated	Use SetSDIOutVPID(ULWord,ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI7OutVPID(ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use GetSDIOutVPID(ULWord&,ULWord&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2SDI8OutVPID(ULWord valueA, ULWord valueB = 0);			///< @deprecated	Use SetSDIOutVPID(ULWord,ULWord,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2SDI8OutVPID(ULWord* valueA, ULWord* valueB = NULL);	///< @deprecated	Use GetSDIOutVPID(ULWord&,ULWord&,NTV2Channel) instead.
	#endif	//	!defined (NTV2_DEPRECATE)


	/**
		@name	Up/Down Conversion
	**/
	///@{
	AJA_VIRTUAL bool	SetUpConvertMode (NTV2UpConvertMode value);
	AJA_VIRTUAL bool	GetUpConvertMode(NTV2UpConvertMode* value);
	AJA_VIRTUAL bool	SetConverterOutStandard (NTV2Standard value);
	AJA_VIRTUAL bool	GetConverterOutStandard(NTV2Standard* value);
	AJA_VIRTUAL bool	SetConverterOutRate (NTV2FrameRate value);
	AJA_VIRTUAL bool	GetConverterOutRate(NTV2FrameRate* value);
	AJA_VIRTUAL bool	SetConverterInStandard (NTV2Standard value);
	AJA_VIRTUAL bool	GetConverterInStandard(NTV2Standard* value);
	AJA_VIRTUAL bool	SetConverterInRate (NTV2FrameRate value);
	AJA_VIRTUAL bool	GetConverterInRate(NTV2FrameRate* value);
	AJA_VIRTUAL bool	SetConverterPulldown(ULWord value);
	AJA_VIRTUAL bool	GetConverterPulldown(ULWord* value);
	AJA_VIRTUAL bool	SetUCPassLine21 (ULWord value);
	AJA_VIRTUAL bool	GetUCPassLine21(ULWord* value);
	AJA_VIRTUAL bool	SetUCAutoLine21 (ULWord value);
	AJA_VIRTUAL bool	GetUCAutoLine21(ULWord* value);

	AJA_VIRTUAL bool	SetDownConvertMode (NTV2DownConvertMode value);
	AJA_VIRTUAL bool	GetDownConvertMode(NTV2DownConvertMode* value);
	AJA_VIRTUAL bool	SetIsoConvertMode (NTV2IsoConvertMode value);
	AJA_VIRTUAL bool	GetIsoConvertMode(NTV2IsoConvertMode* value);
	AJA_VIRTUAL bool	SetEnableConverter(bool value);
	AJA_VIRTUAL bool	GetEnableConverter(bool* value);
	AJA_VIRTUAL bool	SetDeinterlaceMode(ULWord value);
	AJA_VIRTUAL bool	GetDeinterlaceMode(ULWord* value);

	AJA_VIRTUAL bool	SetSecondConverterOutStandard (NTV2Standard value);
	AJA_VIRTUAL bool	GetSecondConverterOutStandard(NTV2Standard* value);
	AJA_VIRTUAL bool	SetSecondConverterInStandard (NTV2Standard value);
	AJA_VIRTUAL bool	GetSecondConverterInStandard(NTV2Standard* value);
	AJA_VIRTUAL bool	SetSecondDownConvertMode (NTV2DownConvertMode value);
	AJA_VIRTUAL bool	GetSecondDownConvertMode(NTV2DownConvertMode* value);
	AJA_VIRTUAL bool	SetSecondIsoConvertMode (NTV2IsoConvertMode value);
	AJA_VIRTUAL bool	GetSecondIsoConvertMode(NTV2IsoConvertMode* value);
	AJA_VIRTUAL bool	SetSecondConverterPulldown (ULWord value);
	AJA_VIRTUAL bool	GetSecondConverterPulldown(ULWord* value);

	AJA_VIRTUAL bool	SetConversionMode(NTV2ConversionMode inConversionMode);
	AJA_VIRTUAL bool	GetConversionMode(NTV2ConversionMode * pOutConversionMode)			{return pOutConversionMode ? GetConversionMode (*pOutConversionMode) : false;}
	AJA_VIRTUAL bool	GetConversionMode(NTV2ConversionMode & outConversionMode);
	///@}

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2FrameSyncControlFrameDelay (NTV2FrameSyncSelect select, ULWord value);						///< @deprecated	This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2FrameSyncControlFrameDelay (NTV2FrameSyncSelect select, ULWord *value);					///< @deprecated	This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2FrameSyncControlStandard (NTV2FrameSyncSelect select, NTV2Standard value);					///< @deprecated	This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2FrameSyncControlStandard (NTV2FrameSyncSelect select, NTV2Standard *value);				///< @deprecated	This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2FrameSyncControlGeometry (NTV2FrameSyncSelect select, NTV2FrameGeometry value);			///< @deprecated	This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2FrameSyncControlGeometry (NTV2FrameSyncSelect select, NTV2FrameGeometry *value);			///< @deprecated	This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetK2FrameSyncControlFrameFormat (NTV2FrameSyncSelect select, NTV2FrameBufferFormat value);		///< @deprecated	This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetK2FrameSyncControlFrameFormat (NTV2FrameSyncSelect select, NTV2FrameBufferFormat *value);	///< @deprecated	This SDK no longer supports the FS1.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@name	Color Space Conversion & LUTs
	**/
	///@{

	/**
		@brief		Selects the color space converter operation method.
		@param[in]	inCSCMethod		Specifies the method by which the color space converter will transform its input into its output.
		@param[in]	inChannel		Specifies the CSC of interest.
		@return		True if the call was successful; otherwise false. 
		@note		When selecting NTV2_CSC_Method_Enhanced_4K as the method, the channel must be NTV2_CHANNEL1 or NTV2_CHANNEL5.
					This will group four CSCs together to process the 4K image. To leave 4K, take CSC 1 (or CSC 5) out of 4K mode. 
	**/
	AJA_VIRTUAL bool					SetColorSpaceMethod (const NTV2ColorSpaceMethod inCSCMethod, const NTV2Channel inChannel);

	/**
		@brief		Returns the color space converter operation method.
		@param[in]	inChannel		Specifies the CSC of interest.
		@return		An enum value indicating the operationg mode of the color space converter. 
	**/
	AJA_VIRTUAL NTV2ColorSpaceMethod	GetColorSpaceMethod (const NTV2Channel inChannel);

	AJA_VIRTUAL bool	SetColorSpaceMatrixSelect (NTV2ColorSpaceMatrixType  type, NTV2Channel channel= NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceMatrixSelect (NTV2ColorSpaceMatrixType* type, NTV2Channel channel= NTV2_CHANNEL1);

	AJA_VIRTUAL bool	GenerateGammaTable(NTV2LutType lutType, int bank, double *table);
	AJA_VIRTUAL bool	DownloadLUTToHW(const double * pInTable, const NTV2Channel inChannel, const int inBank);
	AJA_VIRTUAL bool	DownloadLUTToHW (const NTV2DoubleArray & inRedLUT, const NTV2DoubleArray & inGreenLUT, const NTV2DoubleArray & inBlueLUT, const NTV2Channel inChannel, const int inBank);
	AJA_VIRTUAL bool	SetLUTEnable(bool enable, NTV2Channel channel);

	/**
		@brief		Sends the given color lookup table (LUT) to the device.
		@param[in]	pInTable	A valid, non-null pointer to an array of 1,024 double-precision floating-point values.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	LoadLUTTable(const double * pInTable);
	AJA_VIRTUAL bool	LoadLUTTables (const NTV2DoubleArray & inRedLUT, const NTV2DoubleArray & inGreenLUT, const NTV2DoubleArray & inBlueLUT);
	AJA_VIRTUAL void	GetLUTTables (NTV2DoubleArray & outRedLUT, NTV2DoubleArray & outGreenLUT, NTV2DoubleArray & outBlueLUT);
	AJA_VIRTUAL bool	SetLUTV2HostAccessBank (NTV2ColorCorrectionHostAccessBank value);
	AJA_VIRTUAL bool	GetLUTV2HostAccessBank (NTV2ColorCorrectionHostAccessBank *value, NTV2Channel channel);
	AJA_VIRTUAL bool	SetLUTV2OutputBank (NTV2Channel channel, ULWord bank);
	AJA_VIRTUAL bool	GetLUTV2OutputBank (NTV2Channel channel, ULWord *bank);

	AJA_VIRTUAL bool	SetColorSpaceRGBBlackRange(NTV2RGBBlackRange rgbBlackRange,NTV2Channel channel= NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceRGBBlackRange(NTV2RGBBlackRange* rgbBlackRange,NTV2Channel channel= NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetColorSpaceUseCustomCoefficient(ULWord useCustomCoefficient, NTV2Channel channel= NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceUseCustomCoefficient(ULWord* useCustomCoefficient, NTV2Channel channel= NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetColorSpaceMakeAlphaFromKey(ULWord makeAlphaFromKey, NTV2Channel channel= NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceMakeAlphaFromKey(ULWord* makeAlphaFromKey, NTV2Channel channel= NTV2_CHANNEL1);

	AJA_VIRTUAL bool	GetColorSpaceVideoKeySyncFail(bool* videoKeySyncFail, NTV2Channel channel= NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetColorSpaceCustomCoefficients(ColorSpaceConverterCustomCoefficients useCustomCoefficient, NTV2Channel channel= NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceCustomCoefficients(ColorSpaceConverterCustomCoefficients* useCustomCoefficient, NTV2Channel channel= NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetColorSpaceCustomCoefficients12Bit(ColorSpaceConverterCustomCoefficients useCustomCoefficient, NTV2Channel channel= NTV2_CHANNEL1);
	AJA_VIRTUAL bool	GetColorSpaceCustomCoefficients12Bit(ColorSpaceConverterCustomCoefficients* useCustomCoefficient, NTV2Channel channel= NTV2_CHANNEL1);

	AJA_VIRTUAL bool	SetLUTControlSelect(NTV2LUTControlSelect inLUTSelect);
	AJA_VIRTUAL bool	GetLUTControlSelect(NTV2LUTControlSelect * pOutLUTSelect)			{return pOutLUTSelect ? GetLUTControlSelect (*pOutLUTSelect) : false;}
	AJA_VIRTUAL bool	GetLUTControlSelect(NTV2LUTControlSelect & outLUTSelect);
	///@}


	AJA_VIRTUAL bool	SetSecondaryVideoFormat(NTV2VideoFormat inFormat);			//	RETAIL USE ONLY
	AJA_VIRTUAL bool	GetSecondaryVideoFormat(NTV2VideoFormat * pOutFormat)				{return pOutFormat ? GetSecondaryVideoFormat (*pOutFormat) : false;}		//	RETAIL USE ONLY
	AJA_VIRTUAL bool	GetSecondaryVideoFormat(NTV2VideoFormat & outFormat);		//	RETAIL USE ONLY

	AJA_VIRTUAL bool	SetInputVideoSelect (NTV2InputVideoSelect inInputSelect);	//	RETAIL USE ONLY
	AJA_VIRTUAL bool	GetInputVideoSelect(NTV2InputVideoSelect * pOutInputSelect)			{return pOutInputSelect ? GetInputVideoSelect (*pOutInputSelect) : false;}	//	RETAIL USE ONLY
	AJA_VIRTUAL bool	GetInputVideoSelect(NTV2InputVideoSelect & outInputSelect);	//	RETAIL USE ONLY

	//	--------------------------------------------
	//	GetNTV2VideoFormat functions
	//		@deprecated		These static functions don't work correctly, and will be deprecated.
	//		For a given frame rate, geometry and transport, there may be 2 (or more!) possible matching video formats.
	//		The improved GetNTV2VideoFormat function may return a new CNTV2SDIVideoInfo object that can be interrogated about many things.
	//		@note			This function originated in CNTV2Status.
	static NTV2VideoFormat		GetNTV2VideoFormat (NTV2FrameRate frameRate, UByte inputGeometry, bool progressiveTransport, bool isThreeG, bool progressivePicture=false);
	static NTV2VideoFormat		GetNTV2VideoFormat (NTV2FrameRate frameRate, NTV2Standard standard, bool isThreeG, UByte inputGeometry=0, bool progressivePicture=false);
	//	--------------------------------------------

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetNTV2VideoFormat(UByte status, UByte frameRateHiBit);											///< @deprecated	Does not support progressivePicture, 3G, 2K, etc.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetNTV2VideoFormat(NTV2FrameRate frameRate, NTV2Standard standard);								///< @deprecated	Does not support progressivePicture, 3G, 2K, etc.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetNTV2VideoFormat(NTV2FrameRate frameRate, UByte inputGeometry, bool progressiveTransport);	///< @deprecated	Does not support progressivePicture, 3G, etc.
	#else	//	else defined (NTV2_DEPRECATE)
protected:
	#endif	//	else defined (NTV2_DEPRECATE)
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInput1VideoFormat (bool progressivePicture = false);		///< @deprecated	Use GetInputVideoFormat or GetSDIInputVideoFormat instead.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInput2VideoFormat (bool progressivePicture = false);		///< @deprecated	Use GetInputVideoFormat or GetSDIInputVideoFormat instead.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInput3VideoFormat (bool progressivePicture = false);		///< @deprecated	Use GetInputVideoFormat or GetSDIInputVideoFormat instead.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInput4VideoFormat (bool progressivePicture = false);		///< @deprecated	Use GetInputVideoFormat or GetSDIInputVideoFormat instead.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInput5VideoFormat (bool progressivePicture = false);		///< @deprecated	Use GetInputVideoFormat or GetSDIInputVideoFormat instead.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInput6VideoFormat (bool progressivePicture = false);		///< @deprecated	Use GetInputVideoFormat or GetSDIInputVideoFormat instead.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInput7VideoFormat (bool progressivePicture = false);		///< @deprecated	Use GetInputVideoFormat or GetSDIInputVideoFormat instead.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInput8VideoFormat (bool progressivePicture = false);		///< @deprecated	Use GetInputVideoFormat or GetSDIInputVideoFormat instead.
	#endif

public:
	/**
		@brief		Returns the video format of the signal that is present on the given input source.
		@param[in]	inVideoSource		Specifies the video input source.
		@param[in]	inIsProgressive		Optionally specifies if the video format is expected to be progressive or not.
		@return		A valid NTV2VideoFormat if successful; otherwise returns NTV2_FORMAT_UNKNOWN.
		@details	This function allows client applications to determine the kind of video signal, if any, is being presented
					to a given input source of the device. Because the hardware has no way of knowing if the incoming signal
					is progressive or interlaced (e.g., 525/29.97fps progressive versus 525/59.94fps interlaced),
					the function assumes interlaced, but the caller can override the function's "interlace" assumption.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetInputVideoFormat (const NTV2InputSource inVideoSource = NTV2_INPUTSOURCE_SDI1, const bool inIsProgressive = false);

	/**
		@brief		Returns the video format of the signal that is present on the given SDI input source.
		@param[in]	inChannel			Specifies the input channel of interest.
		@param[in]	inIsProgressive		Optionally specifies if the video format is expected to be progressive or not.
		@return		A valid NTV2VideoFormat if successful; otherwise returns NTV2_FORMAT_UNKNOWN.
		@details	This function allows client applications to determine the kind of video signal, if any, is being presented
					to a given input source of the device. Because the hardware has no way of knowing if the incoming signal
					is progressive or interlaced (e.g., 525/29.97fps progressive versus 525/59.94fps interlaced),
					the function assumes interlaced, but the caller can override the function's "interlace" assumption.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetSDIInputVideoFormat (NTV2Channel inChannel, bool inIsProgressive = false);

	/**
		@brief		Returns the video format of the signal that is present on the device's HDMI input.
		@return		A valid NTV2VideoFormat if successful; otherwise returns NTV2_FORMAT_UNKNOWN.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetHDMIInputVideoFormat (void);

	/**
		@brief		Returns the video format of the signal that is present on the device's analog video input.
		@return		A valid NTV2VideoFormat if successful; otherwise returns NTV2_FORMAT_UNKNOWN.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetAnalogInputVideoFormat (void);

	/**
		@brief		Returns the video format of the signal that is present on the device's composite video input.
		@return		A valid NTV2VideoFormat if successful; otherwise returns NTV2_FORMAT_UNKNOWN.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetAnalogCompositeInputVideoFormat (void);

	/**
		@brief		Returns the video format of the signal that is present on the device's reference input.
		@return		A valid NTV2VideoFormat if successful; otherwise returns NTV2_FORMAT_UNKNOWN.
	**/
	AJA_VIRTUAL NTV2VideoFormat GetReferenceVideoFormat (void);

	AJA_VIRTUAL bool	GetSDIInput3GPresent (bool & outValue, const NTV2Channel channel);
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetInputVideoFormat (int inputNum, bool progressivePicture = false);	///< @deprecated	Use GetInputVideoFormat(NTV2InputSource...) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDIInput3GPresent (bool* value, NTV2Channel channel);	///< @deprecated	Use GetSDIInput3GPresent(bool&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDI1Input3GPresent (bool* value);						///< @deprecated	Use GetSDIInput3GPresent(bool&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDI2Input3GPresent (bool* value);						///< @deprecated	Use GetSDIInput3GPresent(bool&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDI3Input3GPresent (bool* value);						///< @deprecated	Use GetSDIInput3GPresent(bool&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDI4Input3GPresent (bool* value);						///< @deprecated	Use GetSDIInput3GPresent(bool&,NTV2Channel) instead.
	#endif	//	!NTV2_DEPRECATE

	AJA_VIRTUAL bool	GetSDIInput3GbPresent (bool & outValue, const NTV2Channel channel);
	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDIInput3GbPresent (bool* value, NTV2Channel channel);	///< @deprecated	Use GetSDIInput3GbPresent(bool&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDI1Input3GbPresent (bool* value);				///< @deprecated		Use GetSDIInput3GbPresent(bool&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDI2Input3GbPresent (bool* value);				///< @deprecated		Use GetSDIInput3GbPresent(bool&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDI3Input3GbPresent (bool* value);				///< @deprecated		Use GetSDIInput3GbPresent(bool&,NTV2Channel) instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetSDI4Input3GbPresent (bool* value);				///< @deprecated		Use GetSDIInput3GbPresent(bool&,NTV2Channel) instead.

		// Kona/Xena LS specific
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetLSVideoADCMode(NTV2LSVideoADCMode value);		///< @deprecated		The Kona/Xena LS is obsolete and unsupported.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetLSVideoADCMode(NTV2LSVideoADCMode* value);		///< @deprecated		The Kona/Xena LS is obsolete and unsupported.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetKLSInputSelect(NTV2InputSource value);			///< @deprecated		The Kona/Xena LS is obsolete and unsupported.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetKLSInputSelect(NTV2InputSource* value);			///< @deprecated		The Kona/Xena LS is obsolete and unsupported.

		// Kona/Xena LH specific
		// Used to pick downconverter on inputs(sd bitfile only)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetLHDownconvertInput(bool value);					///< @deprecated		The Kona/Xena LH is obsolete and unsupported.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetLHDownconvertInput(bool* value);					///< @deprecated		The Kona/Xena LH is obsolete and unsupported.

		// Used to pick downconverter on outputs(hd bitfile only)
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetLHSDIOutput1Select(NTV2LHOutputSelect value);	///< @deprecated		The Kona/Xena LH is obsolete and unsupported.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetLHSDIOutput1Select(NTV2LHOutputSelect* value);	///< @deprecated		The Kona/Xena LH is obsolete and unsupported.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetLHSDIOutput2Select(NTV2LHOutputSelect value);	///< @deprecated		The Kona/Xena LH is obsolete and unsupported.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetLHSDIOutput2Select(NTV2LHOutputSelect* value);	///< @deprecated		The Kona/Xena LH is obsolete and unsupported.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetLHAnalogOutputSelect(NTV2LHOutputSelect value);	///< @deprecated		The Kona/Xena LH is obsolete and unsupported.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetLHAnalogOutputSelect(NTV2LHOutputSelect* value);	///< @deprecated		The Kona/Xena LH is obsolete and unsupported.
	#endif	//	!NTV2_DEPRECATE

#if !defined (NTV2_DEPRECATE)

		#define	GETXPTMACRO(_InputXpt_)		if (!pOutOutputXpt)	return false;									\
											NTV2OutputCrosspointID	outputXpt (NTV2_XptBlack);					\
											if (!GetConnectedOutput ((_InputXpt_), outputXpt))	return false;	\
											*pOutOutputXpt = outputXpt;											\
											return true;

	/**
		@brief	Backtraces the current signal routing from the given output channel to determine the video format being used,
				then sets the output standard based on that format.
		@note	This functionality is now performed automatically by the driver when AutoCirculate is initialized.
		@note	This function will be deprecated in a future SDK.
	**/
	AJA_VIRTUAL NTV2_DEPRECATED bool	SetVideoOutputStandard (const NTV2Channel inChannel);				///< @deprecated	This function is obsolete.
	// kRegXptSelectGroup1
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCompressionModInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCompressionModInput,	inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCompressionModInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCompressionModInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptConversionModInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptConversionModInput,	inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptConversionModInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptConversionModInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptColorSpaceConverterInputSelect	(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC1VidInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptColorSpaceConverterInputSelect	(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC1VidInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC1VidInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC1VidInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC1VidInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC1VidInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptLUTInputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptLUT1Input,				inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptLUTInputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptLUT1Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup2
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDuallinkOutInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkOut1Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDuallinkOutInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkOut1Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptFrameSync2InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptFrameSync2Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptFrameSync2InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptFrameSync2Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptFrameSync1InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptFrameSync1Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptFrameSync1InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptFrameSync1Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptFrameBuffer1InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptFrameBuffer1Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptFrameBuffer1InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptFrameBuffer1Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup3
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC1KeyInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC1KeyInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC1KeyInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC1KeyInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut2InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut2Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut2InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut2Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut1InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut1Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut1InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut1Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptAnalogOutInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptAnalogOutInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptAnalogOutInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptAnalogOutInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup4
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptMixer1BGKeyInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptMixer1BGKeyInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptMixer1BGKeyInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptMixer1BGKeyInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptMixer1BGVidInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptMixer1BGVidInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptMixer1BGVidInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptMixer1BGVidInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptMixer1FGKeyInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptMixer1FGKeyInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptMixer1FGKeyInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptMixer1FGKeyInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptMixer1FGVidInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptMixer1FGVidInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptMixer1FGVidInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptMixer1FGVidInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup5
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC2KeyInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC2KeyInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC2KeyInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC2KeyInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC2VidInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC2VidInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC2VidInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC2VidInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptLUT2InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptLUT2Input,				inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptLUT2InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptLUT2Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptFrameBuffer2InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptFrameBuffer2Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptFrameBuffer2InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptFrameBuffer2Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	//	kRegXptSelectGroup6
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptWaterMarkerInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptWaterMarker1Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptWaterMarkerInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptWaterMarker1Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptIICTInputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptIICT1Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptIICTInputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptIICT1Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptHDMIOutInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptHDMIOutInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptHDMIOutInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptHDMIOutInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSecondConverterInputSelect		(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptConversionMod2Input,	inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSecondConverterInputSelect		(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptConversionMod2Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	//	kRegXptSelectGroup7
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptWaterMarker2InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptWaterMarker2Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptWaterMarker2InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptWaterMarker2Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptIICT2InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptIICT2Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptIICT2InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptIICT2Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDuallinkOut2InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkOut2Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDuallinkOut2InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkOut2Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	//	kRegXptSelectGroup8
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut3InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut3Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut3InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut3Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut4InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut4Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut4InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut4Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut5InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut5Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut5InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut5Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	//	kRegXptSelectGroup9
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptMixer2BGKeyInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptMixer2BGKeyInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptMixer2BGKeyInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptMixer2BGKeyInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptMixer2BGVidInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptMixer2BGVidInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptMixer2BGVidInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptMixer2BGVidInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptMixer2FGKeyInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptMixer2FGKeyInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptMixer2FGKeyInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptMixer2FGKeyInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptMixer2FGVidInputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptMixer2FGVidInput,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptMixer2FGVidInputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptMixer2FGVidInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup10
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut1DS2InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut1InputDS2,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut1DS2InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut1InputDS2);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut2DS2InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut2InputDS2,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut2DS2InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut2InputDS2);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup11
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDualLinkIn1Select					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkIn1Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDualLinkIn1Select					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkIn1Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDualLinkIn1DSSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkIn1DSInput,	inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDualLinkIn1DSSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkIn1DSInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDualLinkIn2Select					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkIn2Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDualLinkIn2Select					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkIn2Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDualLinkIn2DSSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkIn2DSInput,	inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDualLinkIn2DSSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkIn2DSInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup12
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptLUT3InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptLUT3Input,				inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptLUT3InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptLUT3Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptLUT4InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptLUT4Input,				inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptLUT4InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptLUT4Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptLUT5InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptLUT5Input,				inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptLUT5InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptLUT5Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup13
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptFrameBuffer3InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptFrameBuffer3Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptFrameBuffer3InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptFrameBuffer3Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptFrameBuffer4InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptFrameBuffer4Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptFrameBuffer4InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptFrameBuffer4Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup14
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut3DS2InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut3InputDS2,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut3DS2InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut3InputDS2);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut4DS2InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut4InputDS2,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut4DS2InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut4InputDS2);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptSDIOut5DS2InputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptSDIOut5InputDS2,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptSDIOut5DS2InputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptSDIOut5InputDS2);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup15
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDualLinkIn3Select					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkIn3Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDualLinkIn3Select					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkIn3Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDualLinkIn3DSSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkIn3DSInput,	inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDualLinkIn3DSSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkIn3DSInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDualLinkIn4Select					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkIn4Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDualLinkIn4Select					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkIn4Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDualLinkIn4DSSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkIn4DSInput,	inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDualLinkIn4DSSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkIn4DSInput);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup16
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDuallinkOut3InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkOut3Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDuallinkOut3InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkOut3Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDuallinkOut4InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkOut4Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDuallinkOut4InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkOut4Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptDuallinkOut5InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptDualLinkOut5Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptDuallinkOut5InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptDualLinkOut5Input);}						///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup17
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC3VidInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC3VidInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC3VidInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC3VidInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC3KeyInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC3KeyInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC3KeyInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC3KeyInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC4VidInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC4VidInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC4VidInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC4VidInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC4KeyInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC4KeyInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC4KeyInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC4KeyInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup18
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC5VidInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC5VidInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC5VidInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC5VidInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptCSC5KeyInputSelect				(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptCSC5KeyInput,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptCSC5KeyInputSelect				(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptCSC5KeyInput);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup19
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXpt4KDCQ1InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_Xpt4KDCQ1Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXpt4KDCQ1InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_Xpt4KDCQ1Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXpt4KDCQ2InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_Xpt4KDCQ2Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXpt4KDCQ2InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_Xpt4KDCQ2Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXpt4KDCQ3InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_Xpt4KDCQ3Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXpt4KDCQ3InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_Xpt4KDCQ3Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXpt4KDCQ4InputSelect					(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_Xpt4KDCQ4Input,			inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXpt4KDCQ4InputSelect					(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_Xpt4KDCQ4Input);}								///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	// kRegXptSelectGroup20
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptHDMIOutV2Q1InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptHDMIOutQ1Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptHDMIOutV2Q1InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptHDMIOutQ1Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptHDMIOutV2Q2InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptHDMIOutQ2Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptHDMIOutV2Q2InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptHDMIOutQ2Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptHDMIOutV2Q3InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptHDMIOutQ3Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptHDMIOutV2Q3InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptHDMIOutQ3Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	SetXptHDMIOutV2Q4InputSelect			(const NTV2OutputCrosspointID inOutputXpt)		{return Connect (NTV2_XptHDMIOutQ4Input,		inOutputXpt);}		///< @deprecated	Use CNTV2Card::Connect or CNTV2Card::Disconnect instead.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool	GetXptHDMIOutV2Q4InputSelect			(NTV2OutputCrosspointID * pOutOutputXpt)		{GETXPTMACRO	(NTV2_XptHDMIOutQ4Input);}							///< @deprecated	Use CNTV2Card::GetConnectedOutput instead.
#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@name	Signal Routing
	**/
	///@{

	/**
		@brief		Answers with the currently connected NTV2OutputCrosspointID for the given NTV2InputCrosspointID.
		@param[in]	inInputXpt		Specifies the input (signal sink) of interest.
		@param[out]	outOutputXpt	Receives the output (signal source) the given input is connected to (if connected),
									or NTV2_XptBlack if not connected.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetConnectedOutput (const NTV2InputCrosspointID inInputXpt, NTV2OutputCrosspointID & outOutputXpt);

	/**
		@brief		Answers with the currently connected NTV2InputCrosspointID for the given NTV2OutputCrosspointID.
		@param[in]	inOutputXpt		Specifies the output (signal source) of interest.
		@param[out]	outInputXpt		Receives the input (signal sink) the given output is connected to (if connected),
									or NTV2_XptBlack if not connected.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetConnectedInput (const NTV2OutputCrosspointID inOutputXpt, NTV2InputCrosspointID & outInputXpt);

	/**
		@brief		Connects the given widget signal input (sink) to the given widget signal output (source).
		@param[in]	inInputXpt		Specifies the input (signal sink) to be connected to the given output.
		@param[in]	inOutputXpt		Specifies the output (signal source) to be connected to the given input.
									Specifying NTV2_XptBlack effects a disconnect.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	Connect (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt);

	/**
		@brief		Disconnects the given widget signal input (sink) from whatever output (source) it may be connected.
		@param[in]	inInputXpt		Specifies the input (signal sink) to be disconnected.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	Disconnect (const NTV2InputCrosspointID inInputXpt);

	/**
		@brief		Answers whether or not the given widget signal input (sink) is connected to another output (source).
		@param[in]	inInputXpt		Specifies the input (signal sink) of interest.
		@param[out]	outIsConnected	Receives true if the input is connected to any other output (other than NTV2_XptBlack).
		@return		True if successful;  otherwise false.
		@note		If the input is connected to NTV2_XptBlack, "outIsConnected" will be "false".
	**/
	AJA_VIRTUAL bool	IsConnected (const NTV2InputCrosspointID inInputXpt, bool & outIsConnected);

	/**
		@brief		Answers whether or not the given widget signal input (sink) is connected to another output (source).
		@param[in]	inInputXpt		Specifies the input (signal sink) of interest.
		@param[in]	inOutputXpt		Specifies the output (signal source) of interest. It's okay to specify NTV2_XptBlack.
		@param[out]	outIsConnected	Receives true if the input is connected to the specified output.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	IsConnectedTo (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, bool & outIsConnected);


	/**
		@brief		Answers whether or not the given widget signal input (sink) can legally be connected to the given signal output (source).
		@param[in]	inInputXpt		Specifies the input (signal sink) of interest.
		@param[in]	inOutputXpt		Specifies the output (signal source) of interest.
		@param[out]	outCanConnect	Receives true if the input can be connected to the specified output;  otherwise false.
		@return		True if successful;  otherwise false.
		@bug		This function is not currently implemented.
		@todo		This needs to be implemented.
	**/
	AJA_VIRTUAL bool	CanConnect (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, bool & outCanConnect);


	/**
		@brief		Applies the given routing table to the AJA device.
		@return		True if successful; otherwise false.
		@param[in]	inRouter		Specifies the CNTV2SignalRouter that contains the routing to be applied to the device.
		@param[in]	inReplace		If true, replaces the device's existing widget routing with the given one.
									If false, augments the device's existing widget routing.
									Defaults to false.
		@details	Most modern AJA devices do not have fixed interconnections between inputs, outputs, frame stores
					and the various video processing widgets (e.g., colorspace converters, mixer/keyers, etc.).
					Instead, these routing configurations are designated by a set of registers, one for each input
					of each widget. The register's value determines which widget output node (crosspoint) the input
					is connected to. A zero value in the register means that the input is not connected to anything.
					To simplify this process of routing widgets on the device, a set of signal paths (i.e., interconnects)
					are built and then applied to the device in this function call.
					This function iterates over each connection that's specified in the given routing table and updates
					the appropriate register in the device.
	**/
	AJA_VIRTUAL bool	ApplySignalRoute (const CNTV2SignalRouter & inRouter, const bool inReplace = false);

	/**
		@brief		Removes all existing signal path connections between any and all widgets on the AJA device.
		@return		True if successful; otherwise false.
		@details	This function writes zeroes into all crosspoint selection registers, effectively
					clearing any existing routing configuration on the device.
	**/
	AJA_VIRTUAL bool	ClearRouting (void);

	/**
		@brief		Answers with the current signal routing between any and all widgets on the AJA device.
		@param[out]	outRouting	Receives the current signal routing.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetRouting (CNTV2SignalRouter & outRouting);

	/**
		@brief		Answers with the current signal routing for the given channel.
		@param[in]	inChannel	Specifies the NTV2Channel of interest.
		@param[out]	outRouting	Receives the current signal routing for the given channel.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetRoutingForChannel (const NTV2Channel inChannel, CNTV2SignalRouter & outRouting);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	OutputRoutingTable (const NTV2RoutingTable * pInRoutingTable);	///< @deprecated	Use the ApplySignalRoute call instead.
	#endif	//	!NTV2_DEPRECATE
	///@}


	/**
		@name	Analog
		@brief	These functions only work on devices with analog inputs.
	**/
	///@{
	AJA_VIRTUAL bool	WriteSDProcAmpControlsInitialized(ULWord value=1);
	AJA_VIRTUAL bool	WriteSDBrightnessAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteSDContrastAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteSDSaturationAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteSDHueAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteSDCbOffsetAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteSDCrOffsetAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteHDProcAmpControlsInitialized(ULWord value=1);
	AJA_VIRTUAL bool	WriteHDBrightnessAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteHDContrastAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteHDSaturationAdjustmentCb(ULWord value);
	AJA_VIRTUAL bool	WriteHDSaturationAdjustmentCr(ULWord value);
	AJA_VIRTUAL bool	WriteHDCbOffsetAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteHDCrOffsetAdjustment(ULWord value);

	AJA_VIRTUAL bool	ReadSDProcAmpControlsInitialized(ULWord *value);
	AJA_VIRTUAL bool	ReadSDBrightnessAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadSDContrastAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadSDSaturationAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadSDHueAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadSDCbOffsetAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadSDCrOffsetAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadHDProcAmpControlsInitialized(ULWord *value);
	AJA_VIRTUAL bool	ReadHDBrightnessAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadHDContrastAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadHDSaturationAdjustmentCb(ULWord *value);
	AJA_VIRTUAL bool	ReadHDSaturationAdjustmentCr(ULWord *value);
	AJA_VIRTUAL bool	ReadHDCbOffsetAdjustment(ULWord *value);
	AJA_VIRTUAL bool	ReadHDCrOffsetAdjustment(ULWord *value);

	// FS1 (and other?) ProcAmp methods
	AJA_VIRTUAL bool	WriteProcAmpC1YAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteProcAmpC1CBAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteProcAmpC1CRAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteProcAmpC2CBAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteProcAmpC2CRAdjustment(ULWord value);
	AJA_VIRTUAL bool	WriteProcAmpOffsetYAdjustment(ULWord value);
	AJA_VIRTUAL bool	ReadProcAmpC1YAdjustment(ULWord* value);
	AJA_VIRTUAL bool	ReadProcAmpC1CBAdjustment(ULWord* value);
	AJA_VIRTUAL bool	ReadProcAmpC1CRAdjustment(ULWord* value);
	AJA_VIRTUAL bool	ReadProcAmpC2CBAdjustment(ULWord* value);
	AJA_VIRTUAL bool	ReadProcAmpC2CRAdjustment(ULWord* value);
	AJA_VIRTUAL bool	ReadProcAmpOffsetYAdjustment(ULWord* value);

	AJA_VIRTUAL bool		SetAnalogInputADCMode (NTV2LSVideoADCMode inValue);
	AJA_VIRTUAL inline bool	GetAnalogInputADCMode (NTV2LSVideoADCMode * pOutValue)						{return pOutValue ? GetAnalogInputADCMode (*pOutValue) : false;}
	AJA_VIRTUAL bool		GetAnalogInputADCMode (NTV2LSVideoADCMode & outValue);
	///@}

	/**
		@name	HDMI
	**/
	///@{
	AJA_VIRTUAL bool		SetHDMIOut3DPresent (bool value);
	AJA_VIRTUAL bool		GetHDMIOut3DPresent (bool & out3DPresent);
	AJA_VIRTUAL inline bool	GetHDMIOut3DPresent (bool * pOut3DPresent)									{return pOut3DPresent ? GetHDMIOut3DPresent (*pOut3DPresent) : false;}

	AJA_VIRTUAL bool		SetHDMIOut3DMode (NTV2HDMIOut3DMode value);
	AJA_VIRTUAL bool		GetHDMIOut3DMode (NTV2HDMIOut3DMode & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOut3DMode (NTV2HDMIOut3DMode * pOutValue)							{return pOutValue ? GetHDMIOut3DMode (*pOutValue) : false;}

	AJA_VIRTUAL bool		GetHDMIInputStatusRegister (ULWord & outValue);
	AJA_VIRTUAL inline bool	GetHDMIInputStatusRegister (ULWord * pOutValue)								{return pOutValue ? GetHDMIInputStatusRegister (*pOutValue) : false;}

	AJA_VIRTUAL bool		GetHDMIInputColor (NTV2LHIHDMIColorSpace & outValue);
	AJA_VIRTUAL inline bool	GetHDMIInputColor (NTV2LHIHDMIColorSpace * pOutValue)						{return pOutValue ? GetHDMIInputColor (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMIV2TxBypass (bool inBypass);

	AJA_VIRTUAL bool		SetHDMIInputRange (NTV2HDMIRange inNewValue);
	AJA_VIRTUAL bool		GetHDMIInputRange (NTV2HDMIRange & outValue);
	AJA_VIRTUAL inline bool	GetHDMIInputRange (NTV2HDMIRange * pOutValue)								{return pOutValue ? GetHDMIInputRange (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMIOutVideoStandard (NTV2Standard inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutVideoStandard (NTV2Standard & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutVideoStandard (NTV2Standard * pOutValue)							{return pOutValue ? GetHDMIOutVideoStandard (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMISampleStructure (NTV2HDMISampleStructure inNewValue);
	AJA_VIRTUAL bool		GetHDMISampleStructure (NTV2HDMISampleStructure & outValue);
	AJA_VIRTUAL inline bool	GetHDMISampleStructure (NTV2HDMISampleStructure * pOutValue)				{return pOutValue ? GetHDMISampleStructure (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMIOutVideoFPS (NTV2FrameRate inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutVideoFPS (NTV2FrameRate & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutVideoFPS (NTV2FrameRate * pOutValue)								{return pOutValue ? GetHDMIOutVideoFPS (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMIOutRange (NTV2HDMIRange inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutRange (NTV2HDMIRange & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutRange (NTV2HDMIRange * pOutValue)									{return pOutValue ? GetHDMIOutRange (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMIOutAudioChannels (NTV2HDMIAudioChannels inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutAudioChannels (NTV2HDMIAudioChannels & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutAudioChannels (NTV2HDMIAudioChannels * pOutValue)					{return pOutValue ? GetHDMIOutAudioChannels (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMIInColorSpace (NTV2HDMIColorSpace inNewValue);
	AJA_VIRTUAL bool		GetHDMIInColorSpace (NTV2HDMIColorSpace & outValue);
	AJA_VIRTUAL inline bool	GetHDMIInColorSpace (NTV2HDMIColorSpace * pOutValue)						{return pOutValue ? GetHDMIInColorSpace (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetHDMIOutColorSpace (NTV2HDMIColorSpace inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutColorSpace (NTV2HDMIColorSpace & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutColorSpace (NTV2HDMIColorSpace * pOutValue)						{return pOutValue ? GetHDMIOutColorSpace (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetLHIHDMIOutColorSpace (NTV2LHIHDMIColorSpace inNewValue);
	AJA_VIRTUAL bool		GetLHIHDMIOutColorSpace (NTV2LHIHDMIColorSpace* value);

	AJA_VIRTUAL bool		SetHDMIOutBitDepth (NTV2HDMIBitDepth inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutBitDepth (NTV2HDMIBitDepth & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutBitDepth (NTV2HDMIBitDepth * pOutValue)							{return pOutValue ? GetHDMIOutBitDepth (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMIOutProtocol (NTV2HDMIProtocol inNewValue);
	AJA_VIRTUAL bool		GetHDMIOutProtocol (NTV2HDMIProtocol & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutProtocol (NTV2HDMIProtocol * pOutValue)							{return pOutValue ? GetHDMIOutProtocol (*pOutValue) : false;}

	AJA_VIRTUAL bool		GetHDMIOutDownstreamBitDepth (NTV2HDMIBitDepth & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutDownstreamBitDepth (NTV2HDMIBitDepth * pOutValue)					{return pOutValue ? GetHDMIOutDownstreamBitDepth (*pOutValue) : false;}

	AJA_VIRTUAL bool		GetHDMIOutDownstreamColorSpace (NTV2LHIHDMIColorSpace & outValue);
	AJA_VIRTUAL inline bool	GetHDMIOutDownstreamColorSpace (NTV2LHIHDMIColorSpace * pOutValue)			{return pOutValue ? GetHDMIOutDownstreamColorSpace (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetHDMIAudioSampleRateConverterEnable (bool inNewValue);
	AJA_VIRTUAL bool		GetHDMIAudioSampleRateConverterEnable (bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetHDMIAudioSampleRateConverterEnable (bool* pOutIsEnabled)					{return pOutIsEnabled ? GetHDMIAudioSampleRateConverterEnable (*pOutIsEnabled) : false;}

	/**
		@brief						Sets the HDMI output's 2-channel audio source.
		@param[in]	inNewValue		Specifies the audio channels from the given audio system to be used.
		@param[in]	inAudioSystem	Specifies the audio system that will supply audio samples to the HDMI output. Defaults to NTV2_AUDIOSYSTEM_1.
		@return						True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetHDMIOutAudioSource2Channel (const NTV2AudioChannelPair inNewValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief						Answers with the HDMI output's current 2-channel audio source.
		@param[out]	outValue		Receives the audio channels that are currently being used.
		@param[out]	outAudioSystem	Receives the audio system that is currently supplying audio samples to the HDMI output.
		@return						True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetHDMIOutAudioSource2Channel (NTV2AudioChannelPair & outValue, NTV2AudioSystem & outAudioSystem);

	/**
		@brief						Changes the HDMI output's 8-channel audio source.
		@param[in]	inNewValue		Specifies the audio channels from the given audio system to be used.
		@param[in]	inAudioSystem	Specifies the audio system that will supply audio samples to the HDMI output. Defaults to NTV2_AUDIOSYSTEM_1.
		@return						True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	SetHDMIOutAudioSource8Channel (const NTV2Audio8ChannelSelect inNewValue, const NTV2AudioSystem inAudioSystem = NTV2_AUDIOSYSTEM_1);

	/**
		@brief						Answers with the HDMI output's current 8-channel audio source.
		@param[out]	outValue		Receives the audio channels that are currently being used.
		@param[out]	outAudioSystem	Receives the audio system that is currently supplying audio samples to the HDMI output.
		@return						True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool	GetHDMIOutAudioSource8Channel (NTV2Audio8ChannelSelect & outValue, NTV2AudioSystem & outAudioSystem);

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool		SetHDMIV2OutVideoStandard (NTV2V2Standard inNewValue);	///< @deprecated	Use GetHDMIOutVideoStandard instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool		GetHDMIV2OutVideoStandard (NTV2V2Standard * pOutValue);	///< @deprecated	Use GetHDMIOutVideoStandard instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool		GetHDMIV2OutVideoStandard (NTV2V2Standard & outValue)	{return GetHDMIV2OutVideoStandard (&outValue);}	///< @deprecated	Use GetHDMIOutVideoStandard instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool		GetHDMIOutAudioSource2Channel (NTV2AudioChannelPair * pOutValue, NTV2Channel * pOutChannel = NULL);		///< @deprecated	Use the GetHDMIOutAudioSource8Channel function that has an NTV2AudioSystem reference.
		AJA_VIRTUAL NTV2_DEPRECATED bool		GetHDMIOutAudioSource2Channel (NTV2AudioChannelPair & outValue, NTV2Channel & outChannel);				///< @deprecated	Use the GetHDMIOutAudioSource8Channel function that has an NTV2AudioSystem reference.
		AJA_VIRTUAL NTV2_DEPRECATED bool		GetHDMIOutAudioSource8Channel (NTV2Audio8ChannelSelect * pOutValue, NTV2Channel * pOutChannel = NULL);	///< @deprecated	Use the GetHDMIOutAudioSource8Channel function that has an NTV2AudioSystem reference.
		AJA_VIRTUAL NTV2_DEPRECATED bool		GetHDMIOutAudioSource8Channel (NTV2Audio8ChannelSelect & outValue, NTV2Channel & outChannel);			///< @deprecated	Use the GetHDMIOutAudioSource8Channel function that has an NTV2AudioSystem reference.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@brief		Enables or disables decimate mode on the device's HDMI rasterizer, which halves the
					output frame rate when enabled. This allows a 60 Hz video stream to be displayed on
					a 30 Hz HDMI montitor.
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, enables decimation mode; otherwise disables decimation mode.
	**/
	AJA_VIRTUAL bool		SetHDMIV2DecimateMode (bool inEnable);

	AJA_VIRTUAL bool		GetHDMIV2DecimateMode (bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetHDMIV2DecimateMode (bool* pOutIsEnabled)					{return pOutIsEnabled ? GetHDMIV2DecimateMode (*pOutIsEnabled) : false;}

	/**
	@brief		Enables or disables two sample interleave I/O mode on the device's HDMI rasterizer.
	@return		True if successful; otherwise false.
	@param[in]	tsiEnable		If true, enables two sample interleave I/O; otherwise disables two sample interleave I/O.
	**/
	AJA_VIRTUAL bool		SetHDMIV2TsiIO (bool tsiEnable);

	AJA_VIRTUAL bool		GetHDMIV2TsiIO (bool & tsiEnabled);
	AJA_VIRTUAL inline bool GetHDMIV2TsiIO (bool * pTsiEnabled)							{return pTsiEnabled ? GetHDMIV2TsiIO (*pTsiEnabled) : false;}



	/**
		@brief		Enables or disables level-B mode on the device's HDMI rasterizer.
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, enables level-B mode; otherwise disables level-B mode.
	**/
	AJA_VIRTUAL bool		SetHDMIV2LevelBMode (bool inEnable);

	AJA_VIRTUAL bool		GetHDMIV2LevelBMode (bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetHDMIV2LevelBMode (bool* pOutIsEnabled)					{return pOutIsEnabled ? GetHDMIV2LevelBMode (*pOutIsEnabled) : false;}

	/**
		@brief		Sets HDMI V2 mode for the device.
		@return		True if successful; otherwise false.
		@param[in]	inMode	Specifies the HDMI V2 operation mode for the device.
							Use NTV2_HDMI_V2_HDSD_BIDIRECTIONAL for HD or SD capture and playback.
							Use NTV2_HDMI_V2_4K_CAPTURE for 4K capture.
							Use NTV2_HDMI_V2_4K_PLAYBACK for 4K playback.
							Note that 4K modes are exclusive.
	**/
	AJA_VIRTUAL bool		SetHDMIV2Mode (NTV2HDMIV2Mode inMode);

	/**
		@brief		Answers with the current HDMI V2 mode of the device.
		@return		True if successful; otherwise false.
		@param[out]	outMode	Receives the current HDMI V2 operation mode for the device.
	**/
	AJA_VIRTUAL bool		GetHDMIV2Mode (NTV2HDMIV2Mode & outMode);
	AJA_VIRTUAL inline bool	GetHDMIV2Mode (NTV2HDMIV2Mode* pOutMode)					{return pOutMode ? GetHDMIV2Mode (*pOutMode) : false;}
	///@}

	#if !defined (NTV2_DEPRECATE)		////	FS1		////////////////////////////////////////
		// Analog (FS1 / MOAB)
		AJA_VIRTUAL NTV2_DEPRECATED NTV2VideoFormat GetFS1AnalogCompositeInputVideoFormat();						///< @deprecated		This SDK no longer supports the FS1.

		// Reg 95 stuff
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1ReferenceSelect(NTV2FS1ReferenceSelect value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1ReferenceSelect(NTV2FS1ReferenceSelect *value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1ColorFIDSubcarrierReset(bool value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1ColorFIDSubcarrierReset(bool *value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1FreezeOutput(NTV2FS1FreezeOutput value);							///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1FreezeOutput(NTV2FS1FreezeOutput *value);							///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1XptProcAmpInputSelect(NTV2OutputCrosspointID value);				///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1XptProcAmpInputSelect(NTV2OutputCrosspointID *value);				///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1XptSecondAnalogOutInputSelect(NTV2OutputCrosspointID value);		///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1XptSecondAnalogOutInputSelect(NTV2OutputCrosspointID *value);		///< @deprecated		This SDK no longer supports the FS1.


		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioDelay(int value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1AudioDelay(int *value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetLossOfInput(ULWord value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioAnalogLevel(NTV2FS1AudioLevel value);						///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1AudioAnalogLevel(NTV2FS1AudioLevel *value);						///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioTone(NTV2FS1AudioTone value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1AudioTone(NTV2FS1AudioTone *value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1OutputTone(NTV2FS1OutputTone value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1OutputTone(NTV2FS1OutputTone *value);								///< @deprecated		This SDK no longer supports the FS1.

		// Audio Channel Mapping registers
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioGain_Ch1(int value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioGain_Ch2(int value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioGain_Ch3(int value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioGain_Ch4(int value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioGain_Ch5(int value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioGain_Ch6(int value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioGain_Ch7(int value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioGain_Ch8(int value);											///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioPhase_Ch1(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioPhase_Ch2(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioPhase_Ch3(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioPhase_Ch4(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioPhase_Ch5(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioPhase_Ch6(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioPhase_Ch7(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioPhase_Ch8(bool value);										///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioSource_Ch1(NTV2AudioChannelMapping value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioSource_Ch2(NTV2AudioChannelMapping value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioSource_Ch3(NTV2AudioChannelMapping value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioSource_Ch4(NTV2AudioChannelMapping value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioSource_Ch5(NTV2AudioChannelMapping value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioSource_Ch6(NTV2AudioChannelMapping value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioSource_Ch7(NTV2AudioChannelMapping value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioSource_Ch8(NTV2AudioChannelMapping value);					///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioMute_Ch1(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioMute_Ch2(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioMute_Ch3(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioMute_Ch4(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioMute_Ch5(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioMute_Ch6(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioMute_Ch7(bool value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1AudioMute_Ch8(bool value);										///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1VideoDAC2Mode (NTV2K2VideoDACMode value);							///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1VideoDAC2Mode (NTV2K2VideoDACMode* value);						///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C1ControlWrite(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C1ControlWrite(ULWord value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C1ControlRead(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C1ControlRead(ULWord value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C1ControlBusy(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C1ControlError(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C1Address(ULWord *value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C1Address(ULWord value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C1SubAddress(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C1SubAddress(ULWord value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C1Data(ULWord *value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C1Data(ULWord value);											///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C2ControlWrite(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C2ControlWrite(ULWord value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C2ControlRead(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C2ControlRead(ULWord value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C2ControlBusy(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C2ControlError(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C2Address(ULWord *value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C2Address(ULWord value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C2SubAddress(ULWord *value);									///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C2SubAddress(ULWord value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1I2C2Data(ULWord *value);											///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1I2C2Data(ULWord value);											///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1DownConvertAFDAutoEnable(bool value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1DownConvertAFDAutoEnable(bool* value);							///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1SecondDownConvertAFDAutoEnable(bool value);						///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1SecondDownConvertAFDAutoEnable(bool* value);						///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1DownConvertAFDDefaultHoldLast(bool value);						///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1DownConvertAFDDefaultHoldLast(bool* value);						///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetFS1SecondDownConvertAFDDefaultHoldLast(bool value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetFS1SecondDownConvertAFDDefaultHoldLast(bool* value);					///< @deprecated		This SDK no longer supports the FS1.

		// Received AFD (Read-only)
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDReceivedCode(ULWord* value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDReceivedAR(ULWord* value);										///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDReceivedVANCPresent(bool* value);									///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAFDInsertMode_SDI1(NTV2AFDInsertMode value);							///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDInsertMode_SDI1(NTV2AFDInsertMode* value);						///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAFDInsertMode_SDI2(NTV2AFDInsertMode value);							///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDInsertMode_SDI2(NTV2AFDInsertMode* value);						///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAFDInsertAR_SDI1(NTV2AFDInsertAspectRatio value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDInsertAR_SDI1(NTV2AFDInsertAspectRatio* value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAFDInsertAR_SDI2(NTV2AFDInsertAspectRatio value);					///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDInsertAR_SDI2(NTV2AFDInsertAspectRatio* value);					///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAFDInsert_SDI1(NTV2AFDInsertCode value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDInsert_SDI1(NTV2AFDInsertCode* value);							///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAFDInsert_SDI2(NTV2AFDInsertCode value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDInsert_SDI2(NTV2AFDInsertCode* value);							///< @deprecated		This SDK no longer supports the FS1.

		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAFDInsertLineNumber_SDI1(ULWord value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDInsertLineNumber_SDI1(ULWord* value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	SetAFDInsertLineNumber_SDI2(ULWord value);								///< @deprecated		This SDK no longer supports the FS1.
		AJA_VIRTUAL NTV2_DEPRECATED bool	GetAFDInsertLineNumber_SDI2(ULWord* value);								///< @deprecated		This SDK no longer supports the FS1.
	#endif	//	!NTV2_DEPRECATE

	AJA_VIRTUAL bool	SetLHIVideoDACStandard(NTV2Standard value);
	AJA_VIRTUAL bool	GetLHIVideoDACStandard(NTV2Standard *value);
	AJA_VIRTUAL bool	SetLHIVideoDACMode(NTV2LHIVideoDACMode value);
	AJA_VIRTUAL bool	GetLHIVideoDACMode(NTV2LHIVideoDACMode* value);
	AJA_VIRTUAL bool	SetLHIVideoDACMode(NTV2VideoDACMode value);	// overloaded
	AJA_VIRTUAL bool	GetLHIVideoDACMode(NTV2VideoDACMode* value);	// overloaded

	/**
		@name	Analog LTC
	**/
	///@{
	/**
		@brief		Enables or disables the ability for the device to read analog LTC on the reference input connector.
		@param[in]	inEnable	If true, the device will read analog LTC from the reference input connector.
								Otherwise, the device cannot read analog LTC from the reference input connector.
		@return		True if successful; otherwise false.
		@note		When enabled, the device cannot genlock to whatever signal is applied to the reference input.
	**/
	AJA_VIRTUAL bool		SetLTCInputEnable (bool inEnable);
	AJA_VIRTUAL bool		GetLTCInputEnable (bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetLTCInputEnable (bool * pOutValue)										{return pOutValue ? GetLTCInputEnable (*pOutValue) : false;}

	/**
		@brief		Answers whether or not a valid analog LTC signal is being applied to the device's analog LTC input connector.
		@param[out]	outIsPresent	Receives 'true' if a valid analog LTC signal is present at the analog LTC input connector;
									otherwise 'false'.
		@return		True if successful; otherwise false.
		@note		Some devices share analog LTC input and reference input on one connector.
					For these devices, this call should be preceded by a call to NTV2Card::SetLTCInputEnable(true).
	**/
	AJA_VIRTUAL bool		GetLTCInputPresent (bool & outIsPresent);
	AJA_VIRTUAL inline bool	GetLTCInputPresent (bool * pOutValue)										{return pOutValue ? GetLTCInputPresent (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetLTCOnReference (bool inNewValue);			//	DEPRECATE??
	AJA_VIRTUAL bool		GetLTCOnReference (bool & outLTCIsOnReference);	//	DEPRECATE??
	AJA_VIRTUAL inline bool	GetLTCOnReference (bool * pOutValue)										{return pOutValue ? GetLTCOnReference (*pOutValue) : false;}

	AJA_VIRTUAL bool		SetLTCEmbeddedOutEnable (bool inNewValue);
	AJA_VIRTUAL bool		GetLTCEmbeddedOutEnable (bool & outValue);
	AJA_VIRTUAL inline bool	GetLTCEmbeddedOutEnable (bool * pOutValue)									{return pOutValue ? GetLTCEmbeddedOutEnable (*pOutValue) : false;}

	/**
		@brief	Reads the current contents of the device's analog LTC input registers.
		@param[in]	inLTCInput		Specifies the device's analog LTC input to use. Use 0 for LTC In 1, or 1 for LTC In 2.
									(Call ::NTV2DeviceGetNumLTCInputs to determine the number of analog LTC inputs.)
		@param[out]	outRP188Data	Receives the timecode read from the device registers. Only the "Low" and "High" fields are set --
									the "DBB" field is set to zero.
		@return		True if successful; otherwise false.
		@note		The registers are read immediately, and should contain stable data if called soon after the VBI.
	**/
	AJA_VIRTUAL bool	ReadAnalogLTCInput (const UWord inLTCInput, RP188_STRUCT & outRP188Data);

	/**
		@brief	Answers with the (SDI) input channel that's providing the clock reference being used by the given device's analog LTC input.
		@param[in]	inLTCInput		Specifies the device's analog LTC input. Use 0 for LTC In 1, or 1 for LTC In 2.
									(Call ::NTV2DeviceGetNumLTCInputs to determine the number of analog LTC inputs.)
		@param[out]	outChannel		Receives the NTV2Channel that is currently providing the clock reference for reading the given analog LTC input.
		@return		True if successful; otherwise false.
		@note		This function is provided for devices that are capable of handling multiple, disparate video formats (see ::NTV2DeviceCanDoMultiFormat
					and GetMultiFormatMode functions). It doesn't make sense to call this function on a device that is running in "UniFormat" mode.
	**/
	AJA_VIRTUAL bool	GetAnalogLTCInClockChannel (const UWord inLTCInput, NTV2Channel & outChannel);

	/**
		@brief	Sets the (SDI) input channel that is to provide the clock reference to be used by the given analog LTC input.
		@param[in]	inLTCInput		Specifies the device's analog LTC input. Use 0 for LTC In 1, or 1 for LTC In 2.
									(Call ::NTV2DeviceGetNumLTCInputs to determine the number of analog LTC inputs.)
		@param[in]	inChannel		Specifies the NTV2Channel that should provide the clock reference for reading the given analog LTC input.
		@return		True if successful; otherwise false.
		@note		This function is provided for devices that are capable of handling multiple, disparate video formats (see ::NTV2DeviceCanDoMultiFormat
					and GetMultiFormatMode functions). It doesn't make sense to call this function on a device that is running in "UniFormat" mode.
	**/
	AJA_VIRTUAL bool	SetAnalogLTCInClockChannel (const UWord inLTCInput, const NTV2Channel inChannel);

	/**
		@brief	Writes the given timecode to the specified analog LTC output register.
		@param[in]	inLTCOutput		Specifies the device's analog LTC output to use. Use 0 for LTC Out 1, 1 for LTC Out 2, etc.
									(Call ::NTV2DeviceGetNumLTCOutputs to determine the number of analog LTC outputs.)
		@param[in]	inRP188Data		Specifies the timecode to write into the device registers. Only the "Low" and "High" fields are used --
									the "DBB" field is ignored.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	WriteAnalogLTCOutput (const UWord inLTCOutput, const RP188_STRUCT & inRP188Data);

	/**
		@brief	Answers with the (SDI) output channel that's providing the clock reference being used by the given device's analog LTC output.
		@param[in]	inLTCOutput		Specifies the device's analog LTC output. Use 0 for LTC Out 1, or 1 for LTC Out 2.
									(Call ::NTV2DeviceGetNumLTCOutputs to determine the number of analog LTC outputs.)
		@param[out]	outChannel		Receives the NTV2Channel that is currently providing the clock reference for writing the given analog LTC output.
		@return		True if successful; otherwise false.
		@note		This function is provided for devices that are capable of handling multiple, disparate video formats (see ::NTV2DeviceCanDoMultiFormat
					and GetMultiFormatMode functions). It doesn't make sense to call this function on a device that is running in "UniFormat" mode.
	**/
	AJA_VIRTUAL bool	GetAnalogLTCOutClockChannel (const UWord inLTCOutput, NTV2Channel & outChannel);

	/**
		@brief	Sets the (SDI) output channel that is to provide the clock reference to be used by the given analog LTC output.
		@param[in]	inLTCOutput		Specifies the device's analog LTC output. Use 0 for LTC Out 1, 1 for LTC Out 2, etc.
									(Call ::NTV2DeviceGetNumLTCOutputs to determine the number of analog LTC outputs.)
		@param[in]	inChannel		Specifies the NTV2Channel that should provide the clock reference for writing the given analog LTC output.
		@return		True if successful; otherwise false.
		@note		This function is provided for devices that are capable of handling multiple, disparate video formats (see ::NTV2DeviceCanDoMultiFormat
					and GetMultiFormatMode functions). It doesn't make sense to call this function on a device that is running in "UniFormat" mode.
	**/
	AJA_VIRTUAL bool	SetAnalogLTCOutClockChannel (const UWord inLTCOutput, const NTV2Channel inChannel);
	///@}

	/**
		@name	Stereo Compression
	**/
	///@{
	AJA_VIRTUAL bool		SetStereoCompressorOutputMode (NTV2StereoCompressorOutputMode inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorOutputMode (NTV2StereoCompressorOutputMode & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorOutputMode (NTV2StereoCompressorOutputMode * pOutValue)	{return pOutValue ? GetStereoCompressorOutputMode (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetStereoCompressorFlipMode (ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipMode (ULWord & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorFlipMode (ULWord * pOutValue)							{return pOutValue ? GetStereoCompressorFlipMode (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetStereoCompressorFlipLeftHorz (ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipLeftHorz (ULWord & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorFlipLeftHorz (ULWord * pOutValue)						{return pOutValue ? GetStereoCompressorFlipLeftHorz (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetStereoCompressorFlipLeftVert (ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipLeftVert (ULWord & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorFlipLeftVert (ULWord * pOutValue)						{return pOutValue ? GetStereoCompressorFlipLeftVert (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetStereoCompressorFlipRightHorz (ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipRightHorz (ULWord & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorFlipRightHorz (ULWord * pOutValue)						{return pOutValue ? GetStereoCompressorFlipRightHorz (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetStereoCompressorFlipRightVert (ULWord inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorFlipRightVert (ULWord & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorFlipRightVert (ULWord * pOutValue)						{return pOutValue ? GetStereoCompressorFlipRightVert (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetStereoCompressorStandard (NTV2Standard inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorStandard (NTV2Standard & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorStandard (NTV2Standard * pOutValue)						{return pOutValue ? GetStereoCompressorStandard (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetStereoCompressorLeftSource (NTV2OutputCrosspointID inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorLeftSource (NTV2OutputCrosspointID & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorLeftSource (NTV2OutputCrosspointID * pOutValue)			{return pOutValue ? GetStereoCompressorLeftSource (*pOutValue) : false;}
	AJA_VIRTUAL bool		SetStereoCompressorRightSource (NTV2OutputCrosspointID inNewValue);
	AJA_VIRTUAL bool		GetStereoCompressorRightSource (NTV2OutputCrosspointID & outValue);
	AJA_VIRTUAL inline bool	GetStereoCompressorRightSource (NTV2OutputCrosspointID * pOutValue)			{return pOutValue ? GetStereoCompressorRightSource (*pOutValue) : false;}
	///@}

	/**
		@name	Bi-directional SDI
	**/
	///@{
	/**
		@brief		Assuming the device has bi-directional SDI connectors, this function determines whether
					a given SDI connector will behave as an input or an output.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the SDI connector to be affected.
		@param[in]	inEnable		If true, specifies that the channel connector is to be used as an output.
									If false, specifies it's to be used as an input.
		@note		After switching a bidirectional SDI connector from output to input (i.e., inEnable = false),
					it may take the hardware on the device up to approximately 10 frames to synchronize with
					the input signal such that the device can accurately report the video format seen at the
					input.
	**/
	AJA_VIRTUAL bool		SetSDITransmitEnable (NTV2Channel inChannel, bool inEnable);

	/**
		@brief		Assuming the device has bi-directional SDI connectors, this function answers if a given SDI
					channel is currently acting as an input or an output.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the channel to be affected, which must be one of NTV2_CHANNEL1,
									NTV2_CHANNEL2, NTV2_CHANNEL3, or NTV2_CHANNEL4.
		@param[in]	outEnabled		Receives true if the SDI channel connector is transmitting, or false if it's acting as an input.
	**/
	AJA_VIRTUAL bool		GetSDITransmitEnable (NTV2Channel inChannel, bool & outEnabled);
	AJA_VIRTUAL inline bool	GetSDITransmitEnable (NTV2Channel inChannel, bool* pOutEnabled)			{return pOutEnabled ? GetSDITransmitEnable (inChannel, *pOutEnabled) : false;}
	///@}

	AJA_VIRTUAL bool		SetSDIOut2Kx1080Enable (NTV2Channel inChannel, const bool inIsEnabled);
	AJA_VIRTUAL bool		GetSDIOut2Kx1080Enable (NTV2Channel inChannel, bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetSDIOut2Kx1080Enable (NTV2Channel inChannel, bool* pOutIsEnabled)		{return pOutIsEnabled ? GetSDIOut2Kx1080Enable (inChannel, *pOutIsEnabled) : false;}

	AJA_VIRTUAL bool		SetSDIOut3GEnable (NTV2Channel inChannel, bool enable);
	AJA_VIRTUAL bool		GetSDIOut3GEnable (NTV2Channel inChannel, bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetSDIOut3GEnable (NTV2Channel inChannel, bool* pOutIsEnabled)			{return pOutIsEnabled ? GetSDIOut3GEnable (inChannel, *pOutIsEnabled) : false;}

	AJA_VIRTUAL bool		SetSDIOut3GbEnable (NTV2Channel inChannel, bool enable);
	AJA_VIRTUAL bool		GetSDIOut3GbEnable (NTV2Channel inChannel, bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetSDIOut3GbEnable (NTV2Channel inChannel, bool* pOutIsEnabled)			{return pOutIsEnabled ? GetSDIOut3GbEnable (inChannel, *pOutIsEnabled) : false;}


	/**
		@name	SDI Bypass Relays
	**/
	///@{
	/**
		@brief	Passes back an enum specifying if the watchdog timer would put
				the SDI relays into bypass or send the signals through the device.
		@return	True if successful; otherwise false.
		@param[out]		outValue	Receives the current state of the watchdog
									timer, either NTV2_BYPASS or NTV2_NORMAL.
		@note	The watchdog timer will not change the state of the relays
				if they are under manual control.
	**/
	AJA_VIRTUAL bool	GetSDIWatchdogStatus (NTV2RelayState & outValue);

	/**
		@brief	Answers if the bypass relays between connectors 1 and 2 are currently
				in bypass or routing the signals through the device.
		@return	True if successful; otherwise false.
		@param[out]		outValue	Receives the current state of the relays (NTV2_BYPASS or NTV2_NORMAL).
	**/
	AJA_VIRTUAL bool	GetSDIRelayPosition12 (NTV2RelayState & outValue);

	/**
		@brief	Answers if the bypass relays between connectors 3 and 4 are currently
				in bypass or routing the signals through the device.
		@return	True if successful; otherwise false.
		@param[out]		outValue	Receives the current state of the relays (NTV2_BYPASS or NTV2_NORMAL).
	**/
	AJA_VIRTUAL bool	GetSDIRelayPosition34 (NTV2RelayState & outValue);

	/**
		@brief	Answers if the bypass relays between connectors 1 and 2 would be in
				bypass or would route signals through the device, if under manual control.
		@return	True if successful; otherwise false.
		@param[out]		outValue	Receives the relay state (NTV2_BYPASS or NTV2_NORMAL).
		@note	Manual control will not change the state of the relays if
				the watchdog timer for the relays is enabled.
	**/
	AJA_VIRTUAL bool	GetSDIRelayManualControl12 (NTV2RelayState & outValue);

	/**
		@brief	Sets the state of the relays between connectors 1 and 2 to
				bypass or through the device, if under manual control.
		@return	True if successful; otherwise false.
		@param[in]	inValue		Specifies the desired relay state (NTV2_BYPASS or NTV2_NORMAL).
		@note	Manual control will not change the state of the relays if
				the watchdog timer for the relays is enabled. Because this
				call modifies the control register, it sends a kick
				sequence, which has the side effect of restarting the
				timeout counter.
	**/
	AJA_VIRTUAL bool	SetSDIRelayManualControl12 (NTV2RelayState inValue);

	/**
		@brief	Answers if the bypass relays between connectors 3 and 4 would be
				in bypass or would route through the device, if under manual control.
		@return	True if successful; otherwise false.
		@param[out]		outValue	Receives the relay state (NTV2_BYPASS or NTV2_NORMAL).
		@note	Manual control will not change the state of the relays if
				the watchdog timer for the relays is enabled.
	**/
	AJA_VIRTUAL bool	GetSDIRelayManualControl34 (NTV2RelayState & outValue);

	/**
		@brief	Sets the state of the relays between connectors 3 and 4 to
				bypass or through the device, if under manual control.
		@return	True if successful; otherwise false.
		@param[in]	inValue		Specifies the relay state (NTV2_BYPASS or NTV2_NORMAL).
		@note	Manual control will not change the state of the relays if
				the watchdog timer for the relays is enabled. Because this
				call modifies the control register, it sends a kick
				sequence, which has the side effect of restarting the
				timeout counter.
	**/
	AJA_VIRTUAL bool	SetSDIRelayManualControl34 (NTV2RelayState inValue);

	/**
		@brief	Answers true if the relays between connectors 1 and 2 are under
				watchdog timer control, or false if they are under manual control.
		@return	True if successful; otherwise false.
		@param[out]		outValue	Receives 'true' if the watchdog timer is in control
				of the relays; otherwise false if the relays are under manual control.
	**/
	AJA_VIRTUAL bool	GetSDIWatchdogEnable12 (bool & outValue);

	/**
		@brief	Specifies if the relays between connectors 1 and 2 should be under
				watchdog timer control or manual control.
		@return	True if successful; otherwise false.
		@param[in] 	inValue	Specify true if if the watchdog timer is to be in control
							of the relays, or false if the relays are to be under
							manual control.
		@note	Because this call modifies the control register, it sends
				a kick sequence, which has the side effect of restarting
				the timeout counter.
	**/
	AJA_VIRTUAL bool	SetSDIWatchdogEnable12 (bool inValue);

	/**
		@brief	Answers true if the relays between connectors 3 and 4 are under
				watchdog timer control, or false if they are under manual control.
		@return	True if successful; otherwise false.
		@param[out]		outValue	Receives 'true' if the watchdog timer is in control
									of the relays; otherwise 'false' if the relays are under
									manual control.
	**/
	AJA_VIRTUAL bool	GetSDIWatchdogEnable34 (bool & outValue);

	/**
		@brief	Specifies if the relays between connectors 3 and 4 should be under
				watchdog timer control or manual control.
		@return	True if successful; otherwise false.
		@param[in]	inValue		Specify true if if the watchdog timer is to be in control
								of the relays, or false if the relays are to be under
								manual control.
		@note	Because this call modifies the control register, it sends
				a kick sequence, which has the side effect of restarting
				the timeout counter.
	**/
	AJA_VIRTUAL bool	SetSDIWatchdogEnable34 (bool inValue);

	/**
		@brief	Answers with the amount of time that must elapse before the watchdog
				timer times out.
		@return	True if successful; otherwise false.
	 	@param[out]		outValue	Receives the time value in units of 8 nanoseconds.
		@note	The timeout interval begins or is reset when a kick
				sequence is received.
	**/
	AJA_VIRTUAL bool	GetSDIWatchdogTimeout (ULWord & outValue);

	/**
		@brief	Specifies the amount of time that must elapse before the watchdog
				timer times out.
		@return	True if successful; otherwise false.
		@param[in]	inValue		Specifies the timeout interval in units of 8 nanoseconds.
		@note	The timeout interval begins or is reset when a kick
				sequence is received. This call resets the timeout counter
				to zero, which will then start counting up until this value
				is reached, triggering the watchdog timer if it's enabled.
	**/
	AJA_VIRTUAL bool	SetSDIWatchdogTimeout (ULWord inValue);

	/**
		@brief	Restarts the countdown timer to prevent the watchdog timer from
				timing out.
		@return	True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	KickSDIWatchdog (void);

	/**
		@brief	Answers with the current state of all the control registers.
		@return	True if successful; otherwise false.
		@param[out]		outState	Receives the state of the control registers.
	**/
	AJA_VIRTUAL bool	GetSDIWatchdogState(NTV2SDIWatchdogState & outState);

	/**
		@brief	Sets all of the control registers to a given state.
		@return	True if successful; otherwise false.
		@param[in]	inState		Specifies the new control register state.
	**/
	AJA_VIRTUAL bool	SetSDIWatchdogState(const NTV2SDIWatchdogState & inState);
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
	AJA_VIRTUAL inline bool	GetEnable4KDCRGBMode(bool* pOutIsEnabled)					{return pOutIsEnabled ? GetEnable4KDCRGBMode (*pOutIsEnabled) : false;}

	/**
		@brief		Sets 4K Down Convert YCC 444 mode
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, specifies YCC 444
									If false, specifies YCC 422
	**/
	AJA_VIRTUAL bool		Enable4KDCYCC444Mode(bool inEnable);

	AJA_VIRTUAL bool		GetEnable4KDCYCC444Mode(bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetEnable4KDCYCC444Mode(bool* pOutIsEnabled)				{return pOutIsEnabled ? GetEnable4KDCYCC444Mode (*pOutIsEnabled) : false;}

	/**
		@brief		Sets 4K Down Convert PSF in mode
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, specifies PSF in
									If false, specifies P in
	**/
	AJA_VIRTUAL bool		Enable4KDCPSFInMode(bool inEnable);

	AJA_VIRTUAL bool		GetEnable4KDCPSFInMode(bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetEnable4KDCPSFInMode(bool* pOutIsEnabled)					{return pOutIsEnabled ? GetEnable4KDCPSFInMode (*pOutIsEnabled) : false;}

	/**
		@brief		Sets 4K Down Convert PSF out Mode
		@return		True if successful; otherwise false.
		@param[in]	inEnable		If true, specifies PSF out
									If false, specifies P out
	**/
	AJA_VIRTUAL bool		Enable4KPSFOutMode(bool inEnable);

	AJA_VIRTUAL bool		GetEnable4KPSFOutMode(bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetEnable4KPSFOutMode(bool* pOutIsEnabled)					{return pOutIsEnabled ? GetEnable4KPSFOutMode (*pOutIsEnabled) : false;}
	///@}


	/**
		@brief		Enables or disables 3G level b to 3G level a conversion at the SDI output widget (assuming the AJA device can do so).
		@return		True if successful; otherwise false.
		@param[in]	inOutputSpigot	Specifies the SDI output spigot to be affected (where 0 is "SDI Out 1").
		@param[in]	inEnable		If true, incomming 3g level b signal converted to 3g level a signal at SDI output widget.
									If false, specifies normal operation.
	**/
	AJA_VIRTUAL bool		SetSDIInLevelBtoLevelAConversion (const UWord inOutputSpigot, const bool inEnable);

	AJA_VIRTUAL bool		GetSDIInLevelBtoLevelAConversion (const UWord inOutputSpigot, bool & outEnable);
	AJA_VIRTUAL inline bool	GetSDIInLevelBtoLevelAConversion (const UWord inOutputSpigot, bool* pOutEnable)	{return pOutEnable ? GetSDIInLevelBtoLevelAConversion (inOutputSpigot, *pOutEnable) : false;}

	/**
		@brief		Enables or disables 3G level a to 3G level b conversion at the SDI output widget (assuming the AJA device can do so).
		@return		True if successful; otherwise false.
		@param[in]	inOutputSpigot	Specifies the SDI output widget to be affected (where 0 == "SDI Out 1").
		@param[in]	inEnable		If true, outgoing 3g level a signal converted to 3g level b signal at SDI output widget.
									If false, specifies normal operation.
	**/
	AJA_VIRTUAL bool		SetSDIOutLevelAtoLevelBConversion (const UWord inOutputSpigot, const bool inEnable);

	AJA_VIRTUAL bool		GetSDIOutLevelAtoLevelBConversion (const UWord inOutputSpigot, bool & outEnable);
	AJA_VIRTUAL inline bool	GetSDIOutLevelAtoLevelBConversion (const UWord inOutputSpigot, bool* pOutEnable)	{return pOutEnable ? GetSDIOutLevelAtoLevelBConversion (inOutputSpigot, *pOutEnable) : false;}

	/**
		@brief		Enables or disables an RGB-over-3G-level-A conversion at the SDI output widget (assuming the AJA device can do so).
		@return		True if successful; otherwise false.
		@param[in]	inOutputSpigot	Specifies the SDI output to be affected (where 0 is "SDI Out 1").
		@param[in]	inEnable		If true, perform the conversion at the output SDI spigot;  otherwise have the SDI output spigot operate normally (no conversion).
	**/
	AJA_VIRTUAL bool		SetSDIOutRGBLevelAConversion (const UWord inOutputSpigot, const bool inEnable);

	/**
		@brief		Answers with the device's current RGB-over-3G-level-A conversion at the given SDI output spigot (assuming the device can do such a conversion).
		@return		True if successful; otherwise false.
		@param[in]	inOutputSpigot	Specifies the SDI output spigot of interest (where 0 is "SDI Out 1").
		@param[out]	outIsEnabled	Receives true if the device's output spigot is currently performing the conversion;  otherwise false (not converting).
	**/
	AJA_VIRTUAL bool		GetSDIOutRGBLevelAConversion (const UWord inOutputSpigot, bool & outIsEnabled);


	/**
		@brief		Answers with the TRS error status from a given input channel.
		@return		True if the input channel is currently reporting TRS errors, otherwise false.
		@param[in]	inChannel		Specifies the channel of interest.
	**/
	AJA_VIRTUAL bool		GetSDITRSError (const NTV2Channel inChannel);

	/**
		@brief		Returns SDI Lock Status from inputs.
		@return		True if locked, false if not
		@param[in]	inChannel		Specifies the channel of interest.
	**/
	AJA_VIRTUAL bool		GetSDILock (const NTV2Channel inChannel);

	/**
	@brief		Returns SDI Unlock count from inputs.
	@return		count
	@param[in]	inChannel		Specifies the channel of interest.
	**/
	AJA_VIRTUAL ULWord		GetSDIUnlockCount(const NTV2Channel inChannel);
	
	/**
	@brief		Returns SDI VPID link A Status from inputs.
	@return		True if valid, false if not
	@param[in]	inChannel		Specifies the channel of interest.
	**/
	AJA_VIRTUAL bool		GetVPIDValidA(const NTV2Channel inChannel);

	/**
	@brief		Returns SDI VPID link B Status from inputs.
	@return		True if valid, false if not
	@param[in]	inChannel		Specifies the channel of interest.
	**/
	AJA_VIRTUAL bool		GetVPIDValidB(const NTV2Channel inChannel);

	/**
	@brief		Returns SDI CRC error count from link A.
	@return		count
	@param[in]	inChannel		Specifies the channel of interest.
	**/
	AJA_VIRTUAL ULWord		GetCRCErrorCountA(const NTV2Channel inChannel);

	/**
	@brief		Returns SDI CRC error count from link B.
	@return		count
	@param[in]	inChannel		Specifies the channel of interest.
	**/
	AJA_VIRTUAL ULWord		GetCRCErrorCountB(const NTV2Channel inChannel);

	/**
		@brief		Enables or disables multi-format (per channel) device operation.
					If enabled, each device channel can handle a different video format (provided it's in the same clock family).
					If disabled, all device channels have the same video format. See \ref clockingandsync for more information.
		@return		True if successful; otherwise false.
		@param[in]	inEnable	If true, sets the device in multi-format mode.
								If false, sets the device in uni-format mode.
	**/
	AJA_VIRTUAL bool	   SetMultiFormatMode (const bool inEnable);

	/**
		@brief		Answers if the device is operating in multi-format (per channel) mode or not.
					If enabled, each device channel can handle a different video format (provided it's in the same clock family).
					If disabled, all device channels have the same video format. See \ref clockingandsync for more information.
		@return		True if successful; otherwise false.
		@param[in]	outIsEnabled	Receives true if the device is currently in multi-format mode,
									or false if it's in uni-format mode.
	**/
	AJA_VIRTUAL bool		GetMultiFormatMode (bool & outIsEnabled);
	AJA_VIRTUAL inline bool	GetMultiFormatMode (bool* pOutEnabled)						{return pOutEnabled ? GetMultiFormatMode (*pOutEnabled) : false;}


public:
	#if !defined (NTV2_DEPRECATE)
		// Functions for cards that support more than one bitfile
		virtual NTV2_DEPRECATED bool			CheckBitfile(NTV2VideoFormat newValue = NTV2_FORMAT_UNKNOWN);	///< @deprecated	This function is obsolete.
		static NTV2_DEPRECATED int				FormatCompare (NTV2VideoFormat fmt1, NTV2VideoFormat fmt2);		///< @deprecated	This function is obsolete.
	#endif	//	!defined (NTV2_DEPRECATE)

	/**
		@name	RS-422
	**/
	///@{
	/**
		@brief		Sets the parity control on the RS422 port specified by inChannel.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the RS422 port to be affected, which must be NTV2_CHANNEL1 or NTV2_CHANNEL2.
		@param[in]	inRS422Parity	Specifies if parity should be used, and if so, whether it should be odd or even.
	**/
	AJA_VIRTUAL bool		SetRS422Parity  (const NTV2Channel inChannel, const NTV2_RS422_PARITY inRS422Parity);

	AJA_VIRTUAL bool		GetRS422Parity (NTV2Channel inChannel, NTV2_RS422_PARITY & outRS422Parity);
	AJA_VIRTUAL inline bool	GetRS422Parity (NTV2Channel inChannel, NTV2_RS422_PARITY * pOutRS422Parity)	{return pOutRS422Parity ? GetRS422Parity (inChannel, *pOutRS422Parity) : false;}

	/**
		@brief		Sets the baud rate the RS422 port specified by inChannel.
		@return		True if successful; otherwise false.
		@param[in]	inChannel		Specifies the RS422 port to be affected, which must be NTV2_CHANNEL1 or NTV2_CHANNEL2.
		@param[in]	inRS422BaudRate	Specifies the baud rate to be used for RS422 communications.
	**/
	AJA_VIRTUAL bool		SetRS422BaudRate  (const NTV2Channel inChannel, const NTV2_RS422_BAUD_RATE inRS422BaudRate);

	AJA_VIRTUAL bool		GetRS422BaudRate (NTV2Channel inChannel, NTV2_RS422_BAUD_RATE & outRS422BaudRate);
	AJA_VIRTUAL inline bool	GetRS422BaudRate (NTV2Channel inChannel, NTV2_RS422_BAUD_RATE * pOutRS422BaudRate)	{return pOutRS422BaudRate ? GetRS422BaudRate (inChannel, *pOutRS422BaudRate) : false;}

	#if !defined (NTV2_DEPRECATE)
		AJA_VIRTUAL NTV2_DEPRECATED bool	ReadUartRxFifoSize (ULWord * pOutSizeInBytes);
	#endif	//	!defined (NTV2_DEPRECATE)
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
		@param[out]	outTemp			Receives the temperature value that was read from the device.
		@param[in]	inTempScale		Specifies the temperature unit scale to use. Defaults to Celsius.
		@return		True if successful;  otherwise false.
	**/
	AJA_VIRTUAL bool		GetDieTemperature (double & outTemp, const NTV2DieTempScale inTempScale = NTV2DieTempScale_Celsius);
	///@}
public:
	#if !defined (NTV2_DEPRECATE)
		//	These functions all came from the CNTV2Status module...
		AJA_VIRTUAL NTV2_DEPRECATED NTV2ButtonState GetButtonState (int buttonBit);														///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED inline Word		GetBoardVersion (void)								{return GetDeviceVersion ();}	///< @deprecated	Use GetDeviceVersion instead.
		AJA_VIRTUAL NTV2_DEPRECATED inline void		GetBoardVersionString (std::string & outString)		{outString = GetDeviceVersionString ();}	///< @deprecated	Use GetDeviceVersionString instead.
		AJA_VIRTUAL NTV2_DEPRECATED inline void		GetFPGAVersionString (std::string & outString, const NTV2XilinxFPGA inFPGA = eFPGAVideoProc)	{outString = GetFPGAVersionString (inFPGA);}	///< @deprecated	Use the GetFPGAVersionString function that returns a std::string instead.
		AJA_VIRTUAL NTV2_DEPRECATED inline void		GetPCIFPGAVersionString (std::string & outString)	{outString = GetPCIFPGAVersionString ();}	///< @deprecated	Use the GetPCIFPGAVersionString function that returns a std::string instead.
		AJA_VIRTUAL NTV2_DEPRECATED inline void		GetBootFPGAVersionString (std::string & outString)	{outString.clear ();}		///< @deprecated	This function is obsolete.
		AJA_VIRTUAL NTV2_DEPRECATED inline void		GetDriverVersionString (std::string & outString)	{outString = GetDriverVersionString ();}	///< @deprecated	Use the GetDriverVersionString function that returns a std::string instead.
		AJA_VIRTUAL NTV2_DEPRECATED void			GetBoardIDString (std::string & outString);//			{outString = GetBoardIDString ();}	///< @deprecated	Obsolete. Convert the result of GetDeviceID() into a hexa string instead.
		AJA_VIRTUAL NTV2_DEPRECATED std::string		GetBoardIDString (void);		///< @deprecated	Obsolete. Convert the result of GetDeviceID() into a hex string instead.
		AJA_VIRTUAL NTV2_DEPRECATED bool			GetBitFileInformation (ULWord & outNumBytes, std::string & outDateStr, std::string & outTimeStr, NTV2XilinxFPGA inFPGA = eFPGAVideoProc);	///< @deprecated	This function is obsolete (from CNTV2Status).
		AJA_VIRTUAL NTV2_DEPRECATED Word			GetFPGAVersion (const NTV2XilinxFPGA inFPGA = eFPGAVideoProc);			///< @deprecated	This function is obsolete (from CNTV2Status).
	#endif	//	!defined (NTV2_DEPRECATE)

	AJA_VIRTUAL std::string		GetFPGAVersionString (const NTV2XilinxFPGA inFPGA = eFPGAVideoProc);

	AJA_VIRTUAL Word			GetPCIFPGAVersion (void);		//	From CNTV2Status
	AJA_VIRTUAL std::string		GetPCIFPGAVersionString (void);

	/**
		@brief		Returns the size and time/date stamp of the device's currently-installed firmware.
		@param[out]	outNumBytes		Receives the size of the installed firmware image, in bytes.
		@param[out]	outDateStr		Receives a human-readable string containing the date the currently-installed firmware was built.
									The string has the format "YYYY/MM/DD", where "YYYY" is the year, "MM" is the month ("00" thru "12"),
									and "DD" is the day of the month ("00" thru "31").
		@param[out]	outTimeStr		Receives a human-readable string containing the time the currently-installed firmware was built
									(in local Pacific time). The string has the format "HH:MM:SS", where HH is "00" thru "23",
									and both MM and SS are "00" thru "59".
		@return		True if successful;  otherwise false.
		@note		This function has nothing to do with the firmware bitfiles that are currently installed on the local host's file system.
	**/
	AJA_VIRTUAL bool			GetInstalledBitfileInfo (ULWord & outNumBytes, std::string & outDateStr, std::string & outTimeStr);


				//////////////////////////////////////////////////////////
	public:		//////////	From CNTV2Status				//////////////
				//////////////////////////////////////////////////////////
	AJA_VIRTUAL NTV2_DEPRECATED bool			GetInput1Autotimed (void);			//	DEPRECATION CANDIDATE
	AJA_VIRTUAL NTV2_DEPRECATED bool			GetInput2Autotimed (void);			//	DEPRECATION CANDIDATE
	AJA_VIRTUAL NTV2_DEPRECATED bool			GetAnalogInputAutotimed (void);		//	DEPRECATION CANDIDATE
	AJA_VIRTUAL NTV2_DEPRECATED bool			GetHDMIInputAutotimed (void);		//	DEPRECATION CANDIDATE
	AJA_VIRTUAL NTV2_DEPRECATED bool			GetInputAutotimed (int inInputNum);	//	DEPRECATION CANDIDATE

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
	} ColorCorrectionColor;	//	From CNTV2ColorCorrection

#if defined (NTV2_DEPRECATE)
				//////////////////////////////////////////////////////////
	public:		//////////	From CNTV2TestPattern			//////////////
				//////////////////////////////////////////////////////////
	AJA_VIRTUAL NTV2_DEPRECATED inline void						SetChannel (NTV2Channel channel)									{_channel = channel;}				///< @deprecated	Originally in CNTV2TestPattern.
    AJA_VIRTUAL NTV2_DEPRECATED inline NTV2Channel				GetChannel (void)													{return _channel;}					///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline void						SetSignalMask (UWord signalMask)									{_signalMask = signalMask & 0x7;}	///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline UWord					GetSignalMask (void) const											{return _signalMask;}				///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline void						SetTestPatternFrameBufferFormat (NTV2FrameBufferFormat fbFormat)	{_fbFormat = fbFormat;}				///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline NTV2FrameBufferFormat	GetTestPatternFrameBufferFormat (void) const						{return _fbFormat;}					///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline void						SetDualLinkTestPatternOutputEnable (bool enable)					{_dualLinkOutputEnable = enable;}	///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool						GetDualLinkTestPatternOutputEnable (void) const						{return _dualLinkOutputEnable;}		///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline const TestPatternList &	GetTestPatternList (void) const										{return _testPatternList;}			///< @deprecated	Originally in CNTV2TestPattern.

	AJA_VIRTUAL NTV2_DEPRECATED inline void						SetClientDownloadBuffer (ULWord * buffer)							{_clientDownloadBuffer = buffer;}	///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline ULWord *					GetClientDownloadBuffer (void) const								{return _clientDownloadBuffer;}		///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline void						SetTestPatternDMAEnable (bool enable)								{_bDMAtoBoard = enable;}			///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool						GetTestPatternDMAEnable (void) const								{return _bDMAtoBoard; }				///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline void						EnableFlipFlipPage (bool enable)									{_flipFlopPage = enable;}			///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool						GetEnableFlipFlipPage (void) const									{return _flipFlopPage;}				///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline void						SetAutoRouteOnXena2 (bool autoRoute)								{_autoRouteOnXena2 = autoRoute;}	///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED inline bool						GetAutoRouteOnXena2 (void) const									{return _autoRouteOnXena2;}			///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED bool							DownloadTestPattern (UWord testPatternNumber);						///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadTestPattern (char * testPatternName);						///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadBlackTestPattern (void);									///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadBorderTestPattern (void);									///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadSlantRampTestPattern (void);								///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadYCbCrSlantRampTestPattern (void);							///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							Download48BitRGBSlantRampTestPattern (void);						///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadVerticalSweepTestPattern (void);							///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadZonePlateTestPattern (void);								///< @deprecated	Originally in CNTV2TestPattern.

	AJA_VIRTUAL NTV2_DEPRECATED void							RenderTestPatternToBuffer (UWord testPatternNumber, ULWord * buffer);					///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED bool							RenderTestPatternBuffer (NTV2Channel channel, UByte * buffer, NTV2VideoFormat videoFormat, NTV2FrameBufferFormat fbFormat, ULWord width, ULWord height, ULWord rowBytes);	///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadTestPatternBuffer (ULWord * buffer, ULWord size = 0);							///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED ULWord							GetPatternBufferSize (ULWord * width = 0, ULWord * height = 0, ULWord * rowBytes = 0, ULWord * firstLine = 0);	///< @deprecated	Originally in CNTV2TestPattern.
	
	AJA_VIRTUAL NTV2_DEPRECATED int								MakeSineWaveVideo (double radians, bool bChroma);										///< @deprecated	Originally in CNTV2TestPattern.
	AJA_VIRTUAL NTV2_DEPRECATED void							ConvertLinePixelFormat (UWord * unPackedBuffer, ULWord * packedBuffer, int numPixels);	///< @deprecated	Originally in CNTV2TestPattern.

#ifdef AJAMac
	AJA_VIRTUAL NTV2_DEPRECATED void							DownloadRGBPicture (char * pSrc, ULWord srcWidthPixels, ULWord srcHeightPixels, ULWord srcRowBytes);	///< @deprecated	Originally in CNTV2TestPattern.
#endif

	//LocalLoadTestPattern allows the generator to build the pattern independent of global controls to generate independent formats when inconverter is on.
	AJA_VIRTUAL NTV2_DEPRECATED void							LocalLoadBarsTestPattern (UWord testPatternNumber, NTV2Standard standard);		///< @deprecated	Originally in CNTV2TestPattern.

	protected:	//	CNTV2TestPattern Data
		void													InitNTV2TestPattern (void);														///< @deprecated	Originally in CNTV2TestPattern.
		AJA_VIRTUAL void										DownloadSegmentedTestPattern (SegmentTestPatternData * pTestPatternSegmentData);///< @deprecated	Originally in CNTV2TestPattern.
		AJA_VIRTUAL void										AdjustFor2048x1080 (ULWord & numPixels, ULWord & linePitch);					///< @deprecated	Originally in CNTV2TestPattern.
		AJA_VIRTUAL void										AdjustFor4096x2160 (ULWord & numPixels, ULWord & linePitch, ULWord & numLines);	///< @deprecated	Originally in CNTV2TestPattern.
		AJA_VIRTUAL void										AdjustFor3840x2160 (ULWord & numPixels, ULWord & linePitch, ULWord & numLines);	///< @deprecated	Originally in CNTV2TestPattern.

		NTV2Channel				_channel;	//	Also in CNTV2ColorCorrection
		TestPatternList			_testPatternList;
		UWord					_signalMask;
		NTV2FrameBufferFormat	_fbFormat;
		bool					_dualLinkOutputEnable;
		bool					_bDMAtoBoard;
		bool					_autoRouteOnXena2;
		bool					_flipFlopPage;
		ULWord *				_clientDownloadBuffer;


				//////////////////////////////////////////////////////////
	public:		//////////	From CNTV2VidProc				//////////////
				//////////////////////////////////////////////////////////

		AJA_VIRTUAL NTV2_DEPRECATED void				SetupDefaultVidProc (void);									///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED void				DisableVidProc (void);										///< @deprecated	Originally in CNTV2VidProc class.

		AJA_VIRTUAL NTV2_DEPRECATED void				SetCh1VidProcMode (NTV2Ch1VidProcMode vidProcMode);			///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2Ch1VidProcMode	GetCh1VidProcMode (void);									///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED void				SetCh2OutputMode (NTV2Ch2OutputMode outputMode);			///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2Ch2OutputMode	GetCh2OutputMode (void);									///< @deprecated	Originally in CNTV2VidProc class.

		AJA_VIRTUAL NTV2_DEPRECATED void				SetForegroundVideoCrosspoint (NTV2Crosspoint crosspoint);	///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED void				SetForegroundKeyCrosspoint (NTV2Crosspoint crosspoint);		///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED void				SetBackgroundVideoCrosspoint (NTV2Crosspoint crosspoint);	///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED void				SetBackgroundKeyCrosspoint (NTV2Crosspoint crosspoint);		///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2Crosspoint		GetForegroundVideoCrosspoint (void);						///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2Crosspoint		GetForegroundKeyCrosspoint (void);							///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2Crosspoint		GetBackgroundVideoCrosspoint (void);						///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2Crosspoint		GetBackgroundKeyCrosspoint (void);							///< @deprecated	Originally in CNTV2VidProc class.

		AJA_VIRTUAL NTV2_DEPRECATED void				SetSplitMode (NTV2SplitMode splitMode);						///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED NTV2SplitMode		GetSplitMode (void);										///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED void				SetSplitParameters (Fixed_ position, Fixed_ softness);		///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED void				SetSlitParameters (Fixed_ start, Fixed_ width);				///< @deprecated	Originally in CNTV2VidProc class.

		AJA_VIRTUAL NTV2_DEPRECATED void				SetMixCoefficient (Fixed_ coefficient);						///< @deprecated	Originally in CNTV2VidProc class.
		AJA_VIRTUAL NTV2_DEPRECATED Fixed_				GetMixCoefficient (void);									///< @deprecated	Originally in CNTV2VidProc class.

		AJA_VIRTUAL NTV2_DEPRECATED void				SetMatteColor (YCbCr10BitPixel ycbcrPixel);					///< @deprecated	Originally in CNTV2VidProc class.

		#ifdef MSWindows
			AJA_VIRTUAL NTV2_DEPRECATED void			SetMatteColor (COLORREF rgbColor);							///< @deprecated	Originally in CNTV2VidProc class.
		#endif   

		#ifdef AJALinux
			AJA_VIRTUAL NTV2_DEPRECATED void			SetMatteColor (AJARgb rgbColor);							///< @deprecated	Originally in CNTV2VidProc class.
		#endif


				//////////////////////////////////////////////////////////
	public:		//////////	From CNTV2ColorCorrection		//////////////
				//////////////////////////////////////////////////////////
#if 0
		AJA_VIRTUAL NTV2_DEPRECATED bool		SetupColorCorrectionPointers (bool ajamac = AJA_RETAIL_DEFAULT);			///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionEnable (bool enable);										///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		PingPongColorCorrectionTable (void);										///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionValues (ColorCorrectionColor color,double gamma, double gain, double offset);	///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionGamma (ColorCorrectionColor colorChoice,double gamma);	///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionGain (ColorCorrectionColor colorChoice,double gain);		///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		SetColorCorrectionOffset (ColorCorrectionColor colorChoice,double offset);	///< @deprecated	Originally in CNTV2ColorCorrection class.

		AJA_VIRTUAL NTV2_DEPRECATED ULWord *	GetHWTableBaseAddress (ColorCorrectionColor colorChoice);					///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED UWord *		GetTableBaseAddress (ColorCorrectionColor colorChoice);						///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		BuildTables (void);															///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		TransferTablesToHardware (void);											///< @deprecated	Originally in CNTV2ColorCorrection class.

		// TransferTablesToBuffer
		// ccBuffer needs to be 512*4*3 bytes long.
		// This is suitable to pass to transferutocirculate
		AJA_VIRTUAL NTV2_DEPRECATED void		TransferTablesToBuffer (ULWord* ccBuffer);									///< @deprecated	Originally in CNTV2ColorCorrection class.

		// Copy external LUTs (each double LUT[1024]) to/from internal buffers
		AJA_VIRTUAL NTV2_DEPRECATED void		SetTables(double *redLUT, double *greenLUT, double *blueLUT);				///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		GetTables(double *redLUT, double *greenLUT, double *blueLUT);				///< @deprecated	Originally in CNTV2ColorCorrection class.

		// Copy external LUTs (each double LUT[1024]) direct to/from hardware
		AJA_VIRTUAL NTV2_DEPRECATED void		SetTablesToHardware  (double *redLUT, double *greenLUT, double *blueLUT);	///< @deprecated	Originally in CNTV2ColorCorrection class.
		AJA_VIRTUAL NTV2_DEPRECATED void		GetTablesFromHardware (double *redLUT, double *greenLUT, double *blueLUT);	///< @deprecated	Originally in CNTV2ColorCorrection class.

	protected:  //	CNTV2ColorCorrection Data
		void									InitNTV2ColorCorrection (void);												///< @deprecated	Originally in CNTV2ColorCorrection class.
		void									FreeNTV2ColorCorrection (void);												///< @deprecated	Originally in CNTV2ColorCorrection class.

		ULWord *		_pHWTableBaseAddress [NUM_COLORS];		//	On-board base address
		UWord *			_pColorCorrectionTable [NUM_COLORS];	//	Tables for holding calculations
		double			_Gamma [NUM_COLORS];
		double			_Gain [NUM_COLORS];
		double			_Offset [NUM_COLORS];
		//NTV2Channel	_channel;	//	Also in CNTV2TestPattern
#endif

#endif	//	defined (NTV2_DEPRECATE)

	//	These functions can't be deprecated until CNTV2VidProc and CNTV2TestPattern go away...
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteVideoProcessingControlCrosspoint (ULWord value)			{return WriteRegister (kRegVidProcXptControl, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadVideoProcessingControlCrosspoint (ULWord *value)			{return ReadRegister (kRegVidProcXptControl, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteSplitControl (ULWord value)								{return WriteRegister (kRegSplitControl, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadSplitControl (ULWord *value)								{return ReadRegister (kRegSplitControl, value);}

	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteVideoProcessingControl (ULWord value)						{return WriteRegister (kRegVidProc1Control, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadVideoProcessingControl (ULWord *value)						{return ReadRegister (kRegVidProc1Control, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteVideoProcessing2Control (ULWord value)						{return WriteRegister (kRegVidProc2Control, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadVideoProcessing2Control (ULWord *value)						{return ReadRegister (kRegVidProc2Control, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteVideoProcessing3Control (ULWord value)						{return WriteRegister (kRegVidProc3Control, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadVideoProcessing3Control (ULWord *value)						{return ReadRegister (kRegVidProc3Control, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteVideoProcessing4Control (ULWord value)						{return WriteRegister (kRegVidProc4Control, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadVideoProcessing4Control (ULWord *value)						{return ReadRegister (kRegVidProc4Control, value);}

	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteMixerCoefficient (ULWord value)							{return WriteRegister (kRegMixer1Coefficient, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadMixerCoefficient (ULWord *value)							{return ReadRegister (kRegMixer1Coefficient, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteMixer2Coefficient (ULWord value)							{return WriteRegister (kRegMixer2Coefficient, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadMixer2Coefficient (ULWord *value)							{return ReadRegister (kRegMixer2Coefficient, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteMixer3Coefficient (ULWord value)							{return WriteRegister (kRegMixer3Coefficient, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadMixer3Coefficient (ULWord *value)							{return ReadRegister (kRegMixer3Coefficient, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteMixer4Coefficient (ULWord value)							{return WriteRegister (kRegMixer4Coefficient, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadMixer4Coefficient (ULWord *value)							{return ReadRegister (kRegMixer4Coefficient, value);}

	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteFlatMatteValue (ULWord value)								{return WriteRegister (kRegFlatMatteValue, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadFlatMatteValue (ULWord *value)								{return ReadRegister (kRegFlatMatteValue, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteFlatMatte2Value (ULWord value)								{return WriteRegister (kRegFlatMatte2Value, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadFlatMatte2Value (ULWord *value)								{return ReadRegister (kRegFlatMatte2Value, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteFlatMatte3Value (ULWord value)								{return WriteRegister (kRegFlatMatte3Value, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadFlatMatte3Value (ULWord *value)								{return ReadRegister (kRegFlatMatte3Value, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	WriteFlatMatte4Value (ULWord value)								{return WriteRegister (kRegFlatMatte4Value, value);}
	AJA_VIRTUAL NTV2_DEPRECATED bool	ReadFlatMatte4Value (ULWord *value)								{return ReadRegister (kRegFlatMatte4Value, value);}

public:
	/**
		@name	HEVC-Specific Functions
	**/
	///@{
	/**
		@brief		Returns the driver version and time/date stamp of the hevc device's currently-installed firmware.
		@param[out]	pInfo			HevcDeviceInfo structure to receive the information.
		@return		True if successful;  otherwise false.
	**/	
	AJA_VIRTUAL bool HevcGetDeviceInfo(HevcDeviceInfo* pInfo);

	/**
		@brief		Write an hevc register.
		@param[in]	address			Hevc register byte address
		@param[in]	value			Hevc register data
		@param[in]	mask			Read bit mask
		@param[in]	shift			Read bit shift
		@return		True if successful;  otherwise false.
	**/	
	AJA_VIRTUAL bool HevcWriteRegister(ULWord address, ULWord value, ULWord mask = 0xffffffff, ULWord shift = 0);

	/**
		@brief		Read an hevc register.
		@param[in]	address			Hevc register byte address
		@param[out]	pValue			Hevc register data
		@param[in]	mask			Read bit mask
		@param[in]	shift			Read bit shift
		@return		True if successful;  otherwise false.
	**/	
	AJA_VIRTUAL bool HevcReadRegister(ULWord address, ULWord* pValue, ULWord mask = 0xffffffff, ULWord shift = 0);

	/**
		@brief		Send a command to the hevc device.  See the hevc codec documentation for details on commands.
		@param[in]	pCommand		HevcDeviceCommand structure with the command parameters.
		@return		True if successful;  otherwise false.
	**/	
	AJA_VIRTUAL bool HevcSendCommand(HevcDeviceCommand* pCommand);

	/**
		@brief		Transfer video to/from the hevc device.
		@param[in]	pTransfer		HevcDeviceTransfer structure with the transfer parameters.
		@return		True if successful;  otherwise false.
	**/	
	AJA_VIRTUAL bool HevcVideoTransfer(HevcDeviceTransfer* pTransfer);

	/**
		@brief		Get the status of the hevc device.
		@param[in]	pStatus			HevcDeviceDebug structure to receive the information.
		@return		True if successful;  otherwise false.
	**/	
	AJA_VIRTUAL bool HevcGetStatus(HevcDeviceStatus* pStatus);

	/**
		@brief		Get debug data from the hevc device.
		@param[in]	pDebug			HevcDeviceStatus structure to receive the information.  This is an expanded version
									of the device status that contains performance information.  This structure may change
									more often.
		@return		True if successful;  otherwise false.
	**/	
	AJA_VIRTUAL bool HevcDebugInfo(HevcDeviceDebug* pDebug);
	///@}

	/**
		@name	HDR Support
	**/
	///@{
	/**
		@brief		Enables or disables HDMI HDR.
		@param[in]	inEnableHDMIHDR		If true, sets the device to output HDMI HDR Metadata; otherwise sets the device to not output HDMI HDR Metadata.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool EnableHDMIHDR (const bool inEnableHDMIHDR);
	AJA_VIRTUAL bool GetHDMIHDREnabled (void);	///< @return	True if HDMI HDR metadata output is enabled for the device;  otherwise false.

    /**
        @brief		Enables or disables HDMI HDR Dolby Vision.
        @param[in]	inEnable		If true, sets the device to output HDMI HDR Dolby Vision; otherwise sets the device to not output HDMI HDR Dolby Vision.
        @return		True if successful; otherwise false.
    **/
    AJA_VIRTUAL bool EnableHDMIHDRDolbyVision (const bool inEnable);
    AJA_VIRTUAL bool GetHDMIHDRDolbyVisionEnabled (void);	///< @return	True if HDMI HDR Dolby Vision output is enabled for the device;  otherwise false.


	/**
		@brief		Enables or disables BT.2020 Y'cC'bcC'rc versus BT.2020 Y'C'bC'r or R'G'B'.
		@param[in]	inEnableConstantLuminance	If true, sets the device to BT.2020 Y'cC'bcC'rc; otherwise sets the device to BT.2020 Y'C'bC'r or R'G'B'.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRConstantLuminance (const bool inEnableConstantLuminance);
	AJA_VIRTUAL bool GetHDMIHDRConstantLuminanceSet (void);		///< @return	True if BT.2020 Y'cC'bcC'rc is enabled; otherwise false for BT.2020 Y'C'bC'r or R'G'B'.

	/**
		@brief		Sets the Display Mastering data for Green Primary X as defined in SMPTE ST 2086. This is Byte 3 and 4 of SMDT Type 1.
		@param[in]	inGreenPrimaryX		Specifies the Green Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRGreenPrimaryX (const uint16_t inGreenPrimaryX);
	/**
		@brief		Answers with the Display Mastering data for Green Primary X as defined in SMPTE ST 2086. This is Byte 3 and 4 of SMDT Type 1.
		@param[out]	outGreenPrimaryX		Receives the Green Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRGreenPrimaryX (uint16_t & outGreenPrimaryX);

	/**
		@brief		Sets the Display Mastering data for Green Primary Y as defined in SMPTE ST 2086. This is Byte 5 and 6 of SMDT Type 1.
		@param[in]	inGreenPrimaryY		Specifies the Green Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRGreenPrimaryY (const uint16_t inGreenPrimaryY);
	/**
		@brief		Answers with the Display Mastering data for Green Primary Y as defined in SMPTE ST 2086. This is Byte 5 and 6 of SMDT Type 1.
		@param[out]	outGreenPrimaryY		Receives the Green Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRGreenPrimaryY (uint16_t & outGreenPrimaryY);

	/**
		@brief		Sets the Display Mastering data for Blue Primary X as defined in SMPTE ST 2086. This is Byte 7 and 8 of SMDT Type 1.
		@param[in]	inBluePrimaryX		Specifies the Blue Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRBluePrimaryX (const uint16_t inBluePrimaryX);
	/**
		@brief		Answers with the Display Mastering data for Blue Primary X as defined in SMPTE ST 2086. This is Byte 7 and 8 of SMDT Type 1.
		@param[out]	outBluePrimaryX		Receives the Blue Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRBluePrimaryX (uint16_t & outBluePrimaryX);

	/**
		@brief		Sets the Display Mastering data for Blue Primary Y as defined in SMPTE ST 2086. This is Byte 9 and 10 of SMDT Type 1.
		@param[in]	inBluePrimaryY		Specifies the Blue Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRBluePrimaryY (const uint16_t inBluePrimaryY);
	/**
		@brief		Answers with the Display Mastering data for Blue Primary Y as defined in SMPTE ST 2086. This is Byte 9 and 10 of SMDT Type 1.
		@param[out]	outBluePrimaryY		Receives the Blue Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRBluePrimaryY (uint16_t & outBluePrimaryY);

	/**
		@brief		Sets the Display Mastering data for Red Primary X as defined in SMPTE ST 2086. This is Byte 11 and 12 of SMDT Type 1.
		@param[in]	inRedPrimaryX		Specifies the Red Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRRedPrimaryX (const uint16_t inRedPrimaryX);
	/**
		@brief		Answers with the Display Mastering data for Red Primary X as defined in SMPTE ST 2086. This is Byte 11 and 12 of SMDT Type 1.
		@param[out]	outRedPrimaryX		Receives the Red Primary X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRRedPrimaryX (uint16_t & outRedPrimaryX);

	/**
		@brief		Sets the Display Mastering data for Red Primary Y as defined in SMPTE ST 2086. This is Byte 13 and 14 of SMDT Type 1.
		@param[in]	inRedPrimaryY		Specifies the Red Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRRedPrimaryY (const uint16_t inRedPrimaryY);
	/**
		@brief		Answers with the Display Mastering data for Red Primary Y as defined in SMPTE ST 2086. This is Byte 13 and 14 of SMDT Type 1.
		@param[out]	outRedPrimaryY		Receives the Red Primary Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRRedPrimaryY (uint16_t & outRedPrimaryY);

	/**
		@brief		Sets the Display Mastering data for White Point X as defined in SMPTE ST 2086. This is Byte 15 and 16 of SMDT Type 1.
		@param[in]	inWhitePointX		Specifies the White Point X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRWhitePointX (const uint16_t inWhitePointX);
	/**
		@brief		Answers with the Display Mastering data for White Point X as defined in SMPTE ST 2086. This is Byte 15 and 16 of SMDT Type 1.
		@param[out]	outWhitePointX		Receives the White Point X value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRWhitePointX (uint16_t & outWhitePointX);

	/**
		@brief		Sets the Display Mastering data for White Point Y as defined in SMPTE ST 2086. This is Byte 17 and 18 of SMDT Type 1.
		@param[in]	inWhitePointY		Specifies the White Point Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRWhitePointY (const uint16_t inWhitePointY);
	/**
		@brief		Answers with the Display Mastering data for White Point Y as defined in SMPTE ST 2086. This is Byte 17 and 18 of SMDT Type 1.
		@param[out]	outWhitePointY		Receives the White Point Y value as defined in SMPTE ST 2086.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRWhitePointY (uint16_t & outWhitePointY);

	/**
		@brief		Sets the Display Mastering data for the Max Mastering Luminance value as defined in SMPTE ST 2086. This is Byte 19 and 20 of SMDT Type 1.
		@param[in]	inMaxMasteringLuminance		Specifies the Max Mastering Luminance value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRMaxMasteringLuminance (const uint16_t inMaxMasteringLuminance);
	/**
		@brief		Answers with the Display Mastering data for the Max Mastering Luminance value as defined in SMPTE ST 2086. This is Byte 19 and 20 of SMDT Type 1.
		@param[out]	outMaxMasteringLuminance		Receives the Max Mastering Luminance value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRMaxMasteringLuminance (uint16_t & outMaxMasteringLuminance);

	/**
		@brief		Sets the Display Mastering data for the Min Mastering Luminance value as defined in SMPTE ST 2086. This is Byte 21 and 22 of SMDT Type 1.
		@param[in]	inMinMasteringLuminance		Specifies the Min Mastering Luminance value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRMinMasteringLuminance (const uint16_t inMinMasteringLuminance);
	/**
		@brief		Answers with the Display Mastering data for the Min Mastering Luminance value as defined in SMPTE ST 2086. This is Byte 21 and 22 of SMDT Type 1.
		@param[out]	outMinMasteringLuminance		Receives the Min Mastering Luminance value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRMinMasteringLuminance (uint16_t & outMinMasteringLuminance);

	/**
		@brief		Sets the Display Mastering data for the Max Content Light Level(Max CLL) value. This is Byte 23 and 24 of SMDT Type 1.
		@param[in]	inMaxContentLightLevel		Specifies the Max Content Light Level value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRMaxContentLightLevel (const uint16_t inMaxContentLightLevel);
	/**
		@brief		Answers with the Display Mastering data for the Max Content Light Level(Max CLL) value. This is Byte 23 and 24 of SMDT Type 1.
		@param[out]	outMaxContentLightLevel		Receives the Max Content Light Level value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRMaxContentLightLevel (uint16_t & outMaxContentLightLevel);

	/**
		@brief		Sets the Display Mastering data for the Max Frame Average Light Level(Max FALL) value. This is Byte 25 and 26 of SMDT Type 1.
		@param[in]	inMaxFrameAverageLightLevel		Specifies the Max Frame Average Light Level value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool SetHDMIHDRMaxFrameAverageLightLevel (const uint16_t inMaxFrameAverageLightLevel);
	/**
		@brief		Answers with the Display Mastering data for the Max Frame Average Light Level(Max FALL) value. This is Byte 25 and 26 of SMDT Type 1.
		@param[out]	outMaxFrameAverageLightLevel		Receives the Max Frame Average Light Level value.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool GetHDMIHDRMaxFrameAverageLightLevel (uint16_t & outMaxFrameAverageLightLevel);


	AJA_VIRTUAL bool SetHDMIHDRElectroOpticalTransferFunction (const uint8_t inEOTFByte);
	AJA_VIRTUAL bool GetHDMIHDRElectroOpticalTransferFunction (uint8_t & outEOTFByte);
	AJA_VIRTUAL bool SetHDMIHDRStaticMetadataDescriptorID (const uint8_t inSMDId);
	AJA_VIRTUAL bool GetHDMIHDRStaticMetadataDescriptorID (uint8_t & outSMDId);

	AJA_VIRTUAL bool SetHDRData (const HDRFloatValues & inFloatValues);
	AJA_VIRTUAL bool SetHDRData (const HDRRegValues & inRegisterValues);
    AJA_VIRTUAL bool GetHDRData (HDRFloatValues & outFloatValues);
    AJA_VIRTUAL bool GetHDRData (HDRRegValues & outRegisterValues);
	AJA_VIRTUAL bool SetHDMIHDRBT2020 (void);
	AJA_VIRTUAL bool SetHDMIHDRDCIP3 (void);
	///@}

protected:
	AJA_VIRTUAL ULWord			GetSerialNumberLow (void);			//	From CNTV2Status
	AJA_VIRTUAL ULWord			GetSerialNumberHigh (void);			//	From CNTV2Status
	AJA_VIRTUAL bool			IS_CHANNEL_INVALID (const NTV2Channel inChannel) const;
	AJA_VIRTUAL bool			IS_OUTPUT_SPIGOT_INVALID (const UWord inOutputSpigot) const;
	AJA_VIRTUAL bool			IS_INPUT_SPIGOT_INVALID (const UWord inInputSpigot) const;

private:
	// frame buffer sizing helpers
	AJA_VIRTUAL bool	GetLargestFrameBufferFormatInUse(NTV2FrameBufferFormat* format);
	AJA_VIRTUAL bool	GetFrameInfo(NTV2Channel channel, NTV2FrameGeometry* geometry, NTV2FrameBufferFormat* format );
	AJA_VIRTUAL bool	IsBufferSizeChangeRequired(NTV2Channel channel, NTV2FrameGeometry currentGeometry, NTV2FrameGeometry newGeometry,
													NTV2FrameBufferFormat format);
	AJA_VIRTUAL bool	IsBufferSizeChangeRequired(NTV2Channel channel, NTV2FrameGeometry geometry,
									NTV2FrameBufferFormat currentFormat, NTV2FrameBufferFormat newFormat);
	AJA_VIRTUAL bool	GetFBSizeAndCountFromHW(ULWord* size, ULWord* count);

	AJA_VIRTUAL bool	IsMultiFormatActive (void);	///< @return	True if the device supports the multi format feature and it's enabled; otherwise false.
	/**
		@brief		Answers with the NTV2RegInfo of the register associated with the given boolean (i.e., "Can Do") device feature.
		@param[in]	inParamID		Specifies the device features parameter of interest.
		@param[out]	outRegInfo		Receives the associated NTV2RegInfo.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetRegInfoForBoolParam (const NTV2BoolParamID inParamID, NTV2RegInfo & outRegInfo);
	/**
		@brief		Answers with the NTV2RegInfo of the register associated with the given numeric (i.e., "Get Num") device feature.
		@param[in]	inParamID		Specifies the device features parameter of interest.
		@param[out]	outRegInfo		Receives the associated NTV2RegInfo.
		@return		True if successful; otherwise false.
	**/
	AJA_VIRTUAL bool	GetRegInfoForNumericParam (const NTV2NumericParamID inParamID, NTV2RegInfo & outRegInfo);

};	//	CNTV2Card


typedef CNTV2Card	CNTV2Device;	///< @brief	Instances of this class are able to interrogate and control an NTV2 AJA video/audio capture/playout device.

#define SetTablesToHardware						LoadLUTTables
#define GetTablesFromHardware					GetLUTTables

/////////////////////////////////////////////////////////////////////////////


#if !defined (NTV2_DEPRECATE)
	#define	GetK2AnalogOutHTiming					GetAnalogOutHTiming						///< @deprecated		Use GetAnalogOutHTiming instead.
	#define	GetK2ColorSpaceCustomCoefficients		GetColorSpaceCustomCoefficients			///< @deprecated		Use GetColorSpaceCustomCoefficients instead.
	#define	GetK2ColorSpaceCustomCoefficients12Bit	GetColorSpaceCustomCoefficients12Bit	///< @deprecated		Use GetColorSpaceCustomCoefficients12Bit instead.
	#define	GetK2ColorSpaceMakeAlphaFromKey			GetColorSpaceMakeAlphaFromKey			///< @deprecated		Use GetColorSpaceMakeAlphaFromKey instead.
	#define	GetK2ColorSpaceMatrixSelect				GetColorSpaceMatrixSelect				///< @deprecated		Use GetColorSpaceMatrixSelect instead.
	#define	GetK2ColorSpaceRGBBlackRange			GetColorSpaceRGBBlackRange				///< @deprecated		Use GetColorSpaceRGBBlackRange instead.
	#define	GetK2ColorSpaceUseCustomCoefficient		GetColorSpaceUseCustomCoefficient		///< @deprecated		Use GetColorSpaceUseCustomCoefficient instead.
	#define	GetK2ColorSpaceVideoKeySyncFail			GetColorSpaceVideoKeySyncFail			///< @deprecated		Use GetColorSpaceVideoKeySyncFail instead.
	#define	GetK2ConversionMode						GetConversionMode						///< @deprecated		Use GetConversionMode instead.
	#define	GetK2ConverterInRate					GetConverterInRate						///< @deprecated		Use GetConverterInRate instead.
	#define	GetK2ConverterInStandard				GetConverterInStandard					///< @deprecated		Use GetConverterInStandard instead.
	#define	GetK2ConverterOutRate					GetConverterOutRate						///< @deprecated		Use GetConverterOutRate instead.
	#define	GetK2ConverterOutStandard				GetConverterOutStandard					///< @deprecated		Use GetConverterOutStandard instead.
	#define	GetK2ConverterPulldown					GetConverterPulldown					///< @deprecated		Use GetConverterPulldown instead.
	#define	GetK2DeinterlaceMode					GetDeinterlaceMode						///< @deprecated		Use GetDeinterlaceMode instead.
	#define	GetK2DownConvertMode					GetDownConvertMode						///< @deprecated		Use GetDownConvertMode instead.
	#define	GetK2EnableConverter					GetEnableConverter						///< @deprecated		Use GetEnableConverter instead.
	#define	GetK2FrameBufferSize					GetFrameBufferSize						///< @deprecated		Use GetFrameBufferSize instead.
	#define	GetK2InputVideoSelect					GetInputVideoSelect						///< @deprecated		Use GetInputVideoSelect instead.
	#define	GetK2IsoConvertMode						GetIsoConvertMode						///< @deprecated		Use GetIsoConvertMode instead.
	#define	GetK2PulldownMode						GetPulldownMode							///< @deprecated		Use GetPulldownMode instead.
	#define	GetK2SDI1OutHTiming						GetSDI1OutHTiming						///< @deprecated		Use GetSDI1OutHTiming instead.
	#define	GetK2SDI2OutHTiming						GetSDI2OutHTiming						///< @deprecated		Use GetSDI2OutHTiming instead.
	#define	GetK2SecondaryVideoFormat				GetSecondaryVideoFormat					///< @deprecated		Use GetSecondaryVideoFormat instead.
	#define	GetK2SecondConverterInStandard			GetSecondConverterInStandard			///< @deprecated		Use GetSecondConverterInStandard instead.
	#define	GetK2SecondConverterOutStandard			GetSecondConverterOutStandard			///< @deprecated		Use GetSecondConverterOutStandard instead.
	#define	GetK2SecondConverterPulldown			GetSecondConverterPulldown				///< @deprecated		Use GetSecondConverterPulldown instead.
	#define	GetK2SecondDownConvertMode				GetSecondDownConvertMode				///< @deprecated		Use GetSecondDownConvertMode instead.
	#define	GetK2SecondIsoConvertMode				GetSecondIsoConvertMode					///< @deprecated		Use GetSecondIsoConvertMode instead.
	#define	GetK2UCAutoLine21						GetUCAutoLine21							///< @deprecated		Use GetUCAutoLine21 instead.
	#define	GetK2UCPassLine21						GetUCPassLine21							///< @deprecated		Use GetUCPassLine21 instead.
	#define	GetK2UpConvertMode						GetUpConvertMode						///< @deprecated		Use GetUpConvertMode instead.
	#define	GetK2VideoDACMode						GetVideoDACMode							///< @deprecated		Use GetVideoDACMode instead.
	#define	GetK2Xpt1ColorSpaceConverterInputSelect	GetXptColorSpaceConverterInputSelect	///< @deprecated		Use GetXptColorSpaceConverterInputSelect instead.
	#define	GetK2Xpt1CompressionModInputSelect		GetXptCompressionModInputSelect			///< @deprecated		Use GetXptCompressionModInputSelect instead.
	#define	GetK2Xpt1ConversionModInputSelect		GetXptConversionModInputSelect			///< @deprecated		Use GetXptConversionModInputSelect instead.
	#define	GetK2Xpt1CSC1VidInputSelect				GetXptCSC1VidInputSelect				///< @deprecated		Use GetXptCSC1VidInputSelect instead.
	#define	GetK2Xpt1LUTInputSelect					GetXptLUTInputSelect					///< @deprecated		Use GetXptLUTInputSelect instead.
	#define	GetK2Xpt2DuallinkOutInputSelect			GetXptDuallinkOutInputSelect			///< @deprecated		Use GetXptDuallinkOutInputSelect instead.
	#define	GetK2Xpt2FrameBuffer1InputSelect		GetXptFrameBuffer1InputSelect			///< @deprecated		Use GetXptFrameBuffer1InputSelect instead.
	#define	GetK2Xpt2FrameSync1InputSelect			GetXptFrameSync1InputSelect				///< @deprecated		Use GetXptFrameSync1InputSelect instead.
	#define	GetK2Xpt2FrameSync2InputSelect			GetXptFrameSync2InputSelect				///< @deprecated		Use GetXptFrameSync2InputSelect instead.
	#define	GetK2Xpt3AnalogOutInputSelect			GetXptAnalogOutInputSelect				///< @deprecated		Use GetXptAnalogOutInputSelect instead.
	#define	GetK2Xpt3CSC1KeyInputSelect				GetXptCSC1KeyInputSelect				///< @deprecated		Use GetXptCSC1KeyInputSelect instead.
	#define	GetK2Xpt3SDIOut1InputSelect				GetXptSDIOut1InputSelect				///< @deprecated		Use GetXptSDIOut1InputSelect instead.
	#define	GetK2Xpt3SDIOut2InputSelect				GetXptSDIOut2InputSelect				///< @deprecated		Use GetXptSDIOut2InputSelect instead.
	#define	GetK2Xpt4KDCQ1InputSelect				GetXpt4KDCQ1InputSelect					///< @deprecated		Use GetXpt4KDCQ1InputSelect instead.
	#define	GetK2Xpt4KDCQ2InputSelect				GetXpt4KDCQ2InputSelect					///< @deprecated		Use GetXpt4KDCQ2InputSelect instead.
	#define	GetK2Xpt4KDCQ3InputSelect				GetXpt4KDCQ3InputSelect					///< @deprecated		Use GetXpt4KDCQ3InputSelect instead.
	#define	GetK2Xpt4KDCQ4InputSelect				GetXpt4KDCQ4InputSelect					///< @deprecated		Use GetXpt4KDCQ4InputSelect instead.
	#define	GetK2Xpt4MixerBGKeyInputSelect			GetXptMixerBGKeyInputSelect				///< @deprecated		Use GetXptMixerBGKeyInputSelect instead.
	#define	GetK2Xpt4MixerBGVidInputSelect			GetXptMixerBGVidInputSelect				///< @deprecated		Use GetXptMixerBGVidInputSelect instead.
	#define	GetK2Xpt4MixerFGKeyInputSelect			GetXptMixerFGKeyInputSelect				///< @deprecated		Use GetXptMixerFGKeyInputSelect instead.
	#define	GetK2Xpt4MixerFGVidInputSelect			GetXptMixerFGVidInputSelect				///< @deprecated		Use GetXptMixerFGVidInputSelect instead.
	#define	GetK2Xpt5CSC2KeyInputSelect				GetXptCSC2KeyInputSelect				///< @deprecated		Use GetXptCSC2KeyInputSelect instead.
	#define	GetK2Xpt5CSC2VidInputSelect				GetXptCSC2VidInputSelect				///< @deprecated		Use GetXptCSC2VidInputSelect instead.
	#define	GetK2Xpt5FrameBuffer2InputSelect		GetXptFrameBuffer2InputSelect			///< @deprecated		Use GetXptFrameBuffer2InputSelect instead.
	#define	GetK2Xpt5XptLUT2InputSelect				GetXptLUT2InputSelect					///< @deprecated		Use GetXptLUT2InputSelect instead.
	#define	GetK2Xpt6HDMIOutInputSelect				GetXptHDMIOutInputSelect				///< @deprecated		Use GetXptHDMIOutInputSelect instead.
	#define	GetK2Xpt6IICTInputSelect				GetXptIICTInputSelect					///< @deprecated		Use GetXptIICTInputSelect instead.
	#define	GetK2Xpt6SecondConverterInputSelect		GetXptSecondConverterInputSelect		///< @deprecated		Use GetXptSecondConverterInputSelect instead.
	#define	GetK2Xpt6WaterMarkerInputSelect			GetXptWaterMarkerInputSelect			///< @deprecated		Use GetXptWaterMarkerInputSelect instead.
	#define	GetK2Xpt7DuallinkOut2InputSelect		GetXptDuallinkOut2InputSelect			///< @deprecated		Use GetXptDuallinkOut2InputSelect instead.
	#define	GetK2Xpt7IICT2InputSelect				GetXptIICT2InputSelect					///< @deprecated		Use GetXptIICT2InputSelect instead.
	#define	GetK2Xpt7WaterMarker2InputSelect		GetXptWaterMarker2InputSelect			///< @deprecated		Use GetXptWaterMarker2InputSelect instead.
	#define	GetK2Xpt8SDIOut3InputSelect				GetXptSDIOut3InputSelect				///< @deprecated		Use GetXptSDIOut3InputSelect instead.
	#define	GetK2Xpt8SDIOut4InputSelect				GetXptSDIOut4InputSelect				///< @deprecated		Use GetXptSDIOut4InputSelect instead.
	#define	GetK2Xpt8SDIOut5InputSelect				GetXptSDIOut5InputSelect				///< @deprecated		Use GetXptSDIOut5InputSelect instead.
	#define	GetK2Xpt9Mixer2BGKeyInputSelect			GetXptMixer2BGKeyInputSelect			///< @deprecated		Use GetXptMixer2BGKeyInputSelect instead.
	#define	GetK2Xpt9Mixer2BGVidInputSelect			GetXptMixer2BGVidInputSelect			///< @deprecated		Use GetXptMixer2BGVidInputSelect instead.
	#define	GetK2Xpt9Mixer2FGKeyInputSelect			GetXptMixer2FGKeyInputSelect			///< @deprecated		Use GetXptMixer2FGKeyInputSelect instead.
	#define	GetK2Xpt9Mixer2FGVidInputSelect			GetXptMixer2FGVidInputSelect			///< @deprecated		Use GetXptMixer2FGVidInputSelect instead.
	#define	GetK2Xpt10SDIOut1DS2InputSelect			GetXptSDIOut1DS2InputSelect				///< @deprecated		Use GetXptSDIOut1DS2InputSelect instead.
	#define	GetK2Xpt10SDIOut2DS2InputSelect			GetXptSDIOut2DS2InputSelect				///< @deprecated		Use GetXptSDIOut2DS2InputSelect instead.
	#define	GetK2Xpt11DualLinkIn1DSSelect			GetXptDualLinkIn1DSSelect				///< @deprecated		Use GetXptDualLinkIn1DSSelect instead.
	#define	GetK2Xpt11DualLinkIn1Select				GetXptDualLinkIn1Select					///< @deprecated		Use GetXptDualLinkIn1Select instead.
	#define	GetK2Xpt11DualLinkIn2DSSelect			GetXptDualLinkIn2DSSelect				///< @deprecated		Use GetXptDualLinkIn2DSSelect instead.
	#define	GetK2Xpt11DualLinkIn2Select				GetXptDualLinkIn2Select					///< @deprecated		Use GetXptDualLinkIn2Select instead.
	#define	GetK2Xpt12LUT3InputSelect				GetXptLUT3InputSelect					///< @deprecated		Use GetXptLUT3InputSelect instead.
	#define	GetK2Xpt12LUT4InputSelect				GetXptLUT4InputSelect					///< @deprecated		Use GetXptLUT4InputSelect instead.
	#define	GetK2Xpt12LUT5InputSelect				GetXptLUT5InputSelect					///< @deprecated		Use GetXptLUT5InputSelect instead.
	#define	GetK2Xpt13FrameBuffer3InputSelect		GetXptFrameBuffer3InputSelect			///< @deprecated		Use GetXptFrameBuffer3InputSelect instead.
	#define	GetK2Xpt13FrameBuffer4InputSelect		GetXptFrameBuffer4InputSelect			///< @deprecated		Use GetXptFrameBuffer4InputSelect instead.
	#define	GetK2Xpt14SDIOut3DS2InputSelect			GetXptSDIOut3DS2InputSelect				///< @deprecated		Use GetXptSDIOut3DS2InputSelect instead.
	#define	GetK2Xpt14SDIOut4DS2InputSelect			GetXptSDIOut4DS2InputSelect				///< @deprecated		Use GetXptSDIOut4DS2InputSelect instead.
	#define	GetK2Xpt14SDIOut5DS2InputSelect			GetXptSDIOut5DS2InputSelect				///< @deprecated		Use GetXptSDIOut5DS2InputSelect instead.
	#define	GetK2Xpt15DualLinkIn3DSSelect			GetXptDualLinkIn3DSSelect				///< @deprecated		Use GetXptDualLinkIn3DSSelect instead.
	#define	GetK2Xpt15DualLinkIn3Select				GetXptDualLinkIn3Select					///< @deprecated		Use GetXptDualLinkIn3Select instead.
	#define	GetK2Xpt15DualLinkIn4DSSelect			GetXptDualLinkIn4DSSelect				///< @deprecated		Use GetXptDualLinkIn4DSSelect instead.
	#define	GetK2Xpt15DualLinkIn4Select				GetXptDualLinkIn4Select					///< @deprecated		Use GetXptDualLinkIn4Select instead.
	#define	GetK2Xpt16DuallinkOut3InputSelect		GetXptDuallinkOut3InputSelect			///< @deprecated		Use GetXptDuallinkOut3InputSelect instead.
	#define	GetK2Xpt16DuallinkOut4InputSelect		GetXptDuallinkOut4InputSelect			///< @deprecated		Use GetXptDuallinkOut4InputSelect instead.
	#define	GetK2Xpt16DuallinkOut5InputSelect		GetXptDuallinkOut5InputSelect			///< @deprecated		Use GetXptDuallinkOut5InputSelect instead.
	#define	GetK2Xpt17CSC3KeyInputSelect			GetXptCSC3KeyInputSelect				///< @deprecated		Use GetXptCSC3KeyInputSelect instead.
	#define	GetK2Xpt17CSC3VidInputSelect			GetXptCSC3VidInputSelect				///< @deprecated		Use GetXptCSC3VidInputSelect instead.
	#define	GetK2Xpt17CSC4KeyInputSelect			GetXptCSC4KeyInputSelect				///< @deprecated		Use GetXptCSC4KeyInputSelect instead.
	#define	GetK2Xpt17CSC4VidInputSelect			GetXptCSC4VidInputSelect				///< @deprecated		Use GetXptCSC4VidInputSelect instead.
	#define	GetK2XptCSC5KeyInputSelect				GetXptCSC5KeyInputSelect				///< @deprecated		Use GetXptCSC5KeyInputSelect instead.
	#define	GetK2XptCSC5VidInputSelect				GetXptCSC5VidInputSelect				///< @deprecated		Use GetXptCSC5VidInputSelect instead.
	#define	GetK2XptHDMIOutV2Q1InputSelect			GetXptHDMIOutV2Q1InputSelect			///< @deprecated		Use GetXptHDMIOutV2Q1InputSelect instead.
	#define	GetK2XptHDMIOutV2Q2InputSelect			GetXptHDMIOutV2Q2InputSelect			///< @deprecated		Use GetXptHDMIOutV2Q2InputSelect instead.
	#define	GetK2XptHDMIOutV2Q3InputSelect			GetXptHDMIOutV2Q3InputSelect			///< @deprecated		Use GetXptHDMIOutV2Q3InputSelect instead.
	#define	GetK2XptHDMIOutV2Q4InputSelect			GetXptHDMIOutV2Q4InputSelect			///< @deprecated		Use GetXptHDMIOutV2Q4InputSelect instead.
	#define	SetK2AnalogOutHTiming					SetAnalogOutHTiming						///< @deprecated		Use SetAnalogOutHTiming instead.
	#define	SetK2ColorSpaceCustomCoefficients		SetColorSpaceCustomCoefficients			///< @deprecated		Use SetColorSpaceCustomCoefficients instead.
	#define	SetK2ColorSpaceCustomCoefficients12Bit	SetColorSpaceCustomCoefficients12Bit	///< @deprecated		Use SetColorSpaceCustomCoefficients12Bit instead.
	#define	SetK2ColorSpaceMakeAlphaFromKey			SetColorSpaceMakeAlphaFromKey			///< @deprecated		Use SetColorSpaceMakeAlphaFromKey instead.
	#define	SetK2ColorSpaceMatrixSelect				SetColorSpaceMatrixSelect				///< @deprecated		Use SetColorSpaceMatrixSelect instead.
	#define	SetK2ColorSpaceRGBBlackRange			SetColorSpaceRGBBlackRange				///< @deprecated		Use SetColorSpaceRGBBlackRange instead.
	#define	SetK2ColorSpaceUseCustomCoefficient		SetColorSpaceUseCustomCoefficient		///< @deprecated		Use SetColorSpaceUseCustomCoefficient instead.
	#define	SetK2ConversionMode						SetConversionMode						///< @deprecated		Use SetConversionMode instead.
	#define	SetK2ConverterInRate					SetConverterInRate						///< @deprecated		Use SetConverterInRate instead.
	#define	SetK2ConverterInStandard				SetConverterInStandard					///< @deprecated		Use SetConverterInStandard instead.
	#define	SetK2ConverterOutRate					SetConverterOutRate						///< @deprecated		Use SetConverterOutRate instead.
	#define	SetK2ConverterOutStandard				SetConverterOutStandard					///< @deprecated		Use SetConverterOutStandard instead.
	#define	SetK2ConverterPulldown					SetConverterPulldown					///< @deprecated		Use SetConverterPulldown instead.
	#define	SetK2DeinterlaceMode					SetDeinterlaceMode						///< @deprecated		Use SetDeinterlaceMode instead.
	#define	SetK2DownConvertMode					SetDownConvertMode						///< @deprecated		Use SetDownConvertMode instead.
	#define	SetK2EnableConverter					SetEnableConverter						///< @deprecated		Use SetEnableConverter instead.
	#define	SetK2FrameBufferSize					SetFrameBufferSize						///< @deprecated		Use SetFrameBufferSize instead.
	#define	SetK2InputVideoSelect					SetInputVideoSelect						///< @deprecated		Use SetInputVideoSelect instead.
	#define	SetK2IsoConvertMode						SetIsoConvertMode						///< @deprecated		Use SetIsoConvertMode instead.
	#define	SetK2PulldownMode						SetPulldownMode							///< @deprecated		Use SetPulldownMode instead.
	#define	SetK2SDI1OutHTiming						SetSDI1OutHTiming						///< @deprecated		Use SetSDI1OutHTiming instead.
	#define	SetK2SDI2OutHTiming						SetSDI2OutHTiming						///< @deprecated		Use SetSDI2OutHTiming instead.
	#define	SetK2SecondaryVideoFormat				SetSecondaryVideoFormat					///< @deprecated		Use SetSecondaryVideoFormat instead.
	#define	SetK2SecondConverterInStandard			SetSecondConverterInStandard			///< @deprecated		Use SetSecondConverterInStandard instead.
	#define	SetK2SecondConverterOutStandard			SetSecondConverterOutStandard			///< @deprecated		Use SetSecondConverterOutStandard instead.
	#define	SetK2SecondConverterPulldown			SetSecondConverterPulldown				///< @deprecated		Use SetSecondConverterPulldown instead.
	#define	SetK2SecondDownConvertMode				SetSecondDownConvertMode				///< @deprecated		Use SetSecondDownConvertMode instead.
	#define	SetK2SecondIsoConvertMode				SetSecondIsoConvertMode					///< @deprecated		Use SetSecondIsoConvertMode instead.
	#define	SetK2UCAutoLine21						SetUCAutoLine21							///< @deprecated		Use SetUCAutoLine21 instead.
	#define	SetK2UCPassLine21						SetUCPassLine21							///< @deprecated		Use SetUCPassLine21 instead.
	#define	SetK2UpConvertMode						SetUpConvertMode						///< @deprecated		Use SetUpConvertMode instead.
	#define	SetK2VideoDACMode						SetVideoDACMode							///< @deprecated		Use SetVideoDACMode instead.
	#define	SetK2Xpt1ColorSpaceConverterInputSelect	SetXptColorSpaceConverterInputSelect	///< @deprecated		Use SetXptColorSpaceConverterInputSelect instead.
	#define	SetK2Xpt1CompressionModInputSelect		SetXptCompressionModInputSelect			///< @deprecated		Use SetXptCompressionModInputSelect instead.
	#define	SetK2Xpt1ConversionModInputSelect		SetXptConversionModInputSelect			///< @deprecated		Use SetXptConversionModInputSelect instead.
	#define	SetK2Xpt1CSC1VidInputSelect				SetXptCSC1VidInputSelect				///< @deprecated		Use SetXptCSC1VidInputSelect instead.
	#define	SetK2Xpt1LUTInputSelect					SetXptLUTInputSelect					///< @deprecated		Use SetXptLUTInputSelect instead.
	#define	SetK2Xpt2DuallinkOutInputSelect			SetXptDuallinkOutInputSelect			///< @deprecated		Use SetXptDuallinkOutInputSelect instead.
	#define	SetK2Xpt2FrameBuffer1InputSelect		SetXptFrameBuffer1InputSelect			///< @deprecated		Use SetXptFrameBuffer1InputSelect instead.
	#define	SetK2Xpt2FrameSync1InputSelect			SetXptFrameSync1InputSelect				///< @deprecated		Use SetXptFrameSync1InputSelect instead.
	#define	SetK2Xpt2FrameSync2InputSelect			SetXptFrameSync2InputSelect				///< @deprecated		Use SetXptFrameSync2InputSelect instead.
	#define	SetK2Xpt3AnalogOutInputSelect			SetXptAnalogOutInputSelect				///< @deprecated		Use SetXptAnalogOutInputSelect instead.
	#define	SetK2Xpt3CSC1KeyInputSelect				SetXptCSC1KeyInputSelect				///< @deprecated		Use SetXptCSC1KeyInputSelect instead.
	#define	SetK2Xpt3SDIOut1InputSelect				SetXptSDIOut1InputSelect				///< @deprecated		Use SetXptSDIOut1InputSelect instead.
	#define	SetK2Xpt3SDIOut2InputSelect				SetXptSDIOut2InputSelect				///< @deprecated		Use SetXptSDIOut2InputSelect instead.
	#define	SetK2Xpt4KDCQ1InputSelect				SetXpt4KDCQ1InputSelect					///< @deprecated		Use SetXpt4KDCQ1InputSelect instead.
	#define	SetK2Xpt4KDCQ2InputSelect				SetXpt4KDCQ2InputSelect					///< @deprecated		Use SetXpt4KDCQ2InputSelect instead.
	#define	SetK2Xpt4KDCQ3InputSelect				SetXpt4KDCQ3InputSelect					///< @deprecated		Use SetXpt4KDCQ3InputSelect instead.
	#define	SetK2Xpt4KDCQ4InputSelect				SetXpt4KDCQ4InputSelect					///< @deprecated		Use SetXpt4KDCQ4InputSelect instead.
	#define	SetK2Xpt4MixerBGKeyInputSelect			SetXptMixerBGKeyInputSelect				///< @deprecated		Use SetXptMixerBGKeyInputSelect instead.
	#define	SetK2Xpt4MixerBGVidInputSelect			SetXptMixerBGVidInputSelect				///< @deprecated		Use SetXptMixerBGVidInputSelect instead.
	#define	SetK2Xpt4MixerFGKeyInputSelect			SetXptMixerFGKeyInputSelect				///< @deprecated		Use SetXptMixerFGKeyInputSelect instead.
	#define	SetK2Xpt4MixerFGVidInputSelect			SetXptMixerFGVidInputSelect				///< @deprecated		Use SetXptMixerFGVidInputSelect instead.
	#define	SetK2Xpt5CSC2KeyInputSelect				SetXptCSC2KeyInputSelect				///< @deprecated		Use SetXptCSC2KeyInputSelect instead.
	#define	SetK2Xpt5CSC2VidInputSelect				SetXptCSC2VidInputSelect				///< @deprecated		Use SetXptCSC2VidInputSelect instead.
	#define	SetK2Xpt5FrameBuffer2InputSelect		SetXptFrameBuffer2InputSelect			///< @deprecated		Use SetXptFrameBuffer2InputSelect instead.
	#define	SetK2Xpt5XptLUT2InputSelect				SetXptLUT2InputSelect					///< @deprecated		Use SetXptLUT2InputSelect instead.
	#define	SetK2Xpt6HDMIOutInputSelect				SetXptHDMIOutInputSelect				///< @deprecated		Use SetXptHDMIOutInputSelect instead.
	#define	SetK2Xpt6IICTInputSelect				SetXptIICTInputSelect					///< @deprecated		Use SetXptIICTInputSelect instead.
	#define	SetK2Xpt6SecondConverterInputSelect		SetXptSecondConverterInputSelect		///< @deprecated		Use SetXptSecondConverterInputSelect instead.
	#define	SetK2Xpt6WaterMarkerInputSelect			SetXptWaterMarkerInputSelect			///< @deprecated		Use SetXptWaterMarkerInputSelect instead.
	#define	SetK2Xpt7DuallinkOut2InputSelect		SetXptDuallinkOut2InputSelect			///< @deprecated		Use SetXptDuallinkOut2InputSelect instead.
	#define	SetK2Xpt7IICT2InputSelect				SetXptIICT2InputSelect					///< @deprecated		Use SetXptIICT2InputSelect instead.
	#define	SetK2Xpt7WaterMarker2InputSelect		SetXptWaterMarker2InputSelect			///< @deprecated		Use SetXptWaterMarker2InputSelect instead.
	#define	SetK2Xpt8SDIOut3InputSelect				SetXptSDIOut3InputSelect				///< @deprecated		Use SetXptSDIOut3InputSelect instead.
	#define	SetK2Xpt8SDIOut4InputSelect				SetXptSDIOut4InputSelect				///< @deprecated		Use SetXptSDIOut4InputSelect instead.
	#define	SetK2Xpt8SDIOut5InputSelect				SetXptSDIOut5InputSelect				///< @deprecated		Use SetXptSDIOut4InputSelect instead.
	#define	SetK2Xpt9Mixer2BGKeyInputSelect			SetXptMixer2BGKeyInputSelect			///< @deprecated		Use SetXptSDIOut5InputSelect instead.
	#define	SetK2Xpt9Mixer2BGVidInputSelect			SetXptMixer2BGVidInputSelect			///< @deprecated		Use SetXptMixer2BGKeyInputSelect instead.
	#define	SetK2Xpt9Mixer2FGKeyInputSelect			SetXptMixer2FGKeyInputSelect			///< @deprecated		Use SetXptMixer2BGVidInputSelect instead.
	#define	SetK2Xpt9Mixer2FGVidInputSelect			SetXptMixer2FGVidInputSelect			///< @deprecated		Use SetXptMixer2FGKeyInputSelect instead.
	#define	SetK2Xpt10SDIOut1DS2InputSelect			SetXptSDIOut1DS2InputSelect				///< @deprecated		Use SetXptMixer2FGVidInputSelect instead.
	#define	SetK2Xpt10SDIOut2DS2InputSelect			SetXptSDIOut2DS2InputSelect				///< @deprecated		Use SetXptSDIOut1DS2InputSelect instead.
	#define	SetK2Xpt11DualLinkIn1DSSelect			SetXptDualLinkIn1DSSelect				///< @deprecated		Use SetXptSDIOut2DS2InputSelect instead.
	#define	SetK2Xpt11DualLinkIn1Select				SetXptDualLinkIn1Select					///< @deprecated		Use SetXptDualLinkIn1DSSelect instead.
	#define	SetK2Xpt11DualLinkIn2DSSelect			SetXptDualLinkIn2DSSelect				///< @deprecated		Use SetXptDualLinkIn1Select instead.
	#define	SetK2Xpt11DualLinkIn2Select				SetXptDualLinkIn2Select					///< @deprecated		Use SetXptDualLinkIn2DSSelect instead.
	#define	SetK2Xpt12LUT3InputSelect				SetXptLUT3InputSelect					///< @deprecated		Use SetXptDualLinkIn2Select instead.
	#define	SetK2Xpt12LUT4InputSelect				SetXptLUT4InputSelect					///< @deprecated		Use SetXptLUT3InputSelect instead.
	#define	SetK2Xpt12LUT5InputSelect				SetXptLUT5InputSelect					///< @deprecated		Use SetXptLUT4InputSelect instead.
	#define	SetK2Xpt13FrameBuffer3InputSelect		SetXptFrameBuffer3InputSelect			///< @deprecated		Use SetXptLUT5InputSelect instead.
	#define	SetK2Xpt13FrameBuffer4InputSelect		SetXptFrameBuffer4InputSelect			///< @deprecated		Use SetXptFrameBuffer3InputSelect instead.
	#define	SetK2Xpt14SDIOut3DS2InputSelect			SetXptSDIOut3DS2InputSelect				///< @deprecated		Use SetXptFrameBuffer4InputSelect instead.
	#define	SetK2Xpt14SDIOut4DS2InputSelect			SetXptSDIOut4DS2InputSelect				///< @deprecated		Use SetXptSDIOut3DS2InputSelect instead.
	#define	SetK2Xpt14SDIOut5DS2InputSelect			SetXptSDIOut5DS2InputSelect				///< @deprecated		Use SetXptSDIOut4DS2InputSelect instead.
	#define	SetK2Xpt15DualLinkIn3DSSelect			SetXptDualLinkIn3DSSelect				///< @deprecated		Use SetXptSDIOut5DS2InputSelect instead.
	#define	SetK2Xpt15DualLinkIn3Select				SetXptDualLinkIn3Select					///< @deprecated		Use SetXptDualLinkIn3DSSelect instead.
	#define	SetK2Xpt15DualLinkIn4DSSelect			SetXptDualLinkIn4DSSelect				///< @deprecated		Use SetXptDualLinkIn3Select instead.
	#define	SetK2Xpt15DualLinkIn4Select				SetXptDualLinkIn4Select					///< @deprecated		Use SetXptDualLinkIn4DSSelect instead.
	#define	SetK2Xpt16DuallinkOut3InputSelect		SetXptDuallinkOut3InputSelect			///< @deprecated		Use SetXptDualLinkIn4Select instead.
	#define	SetK2Xpt16DuallinkOut4InputSelect		SetXptDuallinkOut4InputSelect			///< @deprecated		Use SetXptDuallinkOut3InputSelect instead.
	#define	SetK2Xpt16DuallinkOut5InputSelect		SetXptDuallinkOut5InputSelect			///< @deprecated		Use SetXptDuallinkOut4InputSelect instead.
	#define	SetK2Xpt17CSC3KeyInputSelect			SetXptCSC3KeyInputSelect				///< @deprecated		Use SetXptDuallinkOut5InputSelect instead.
	#define	SetK2Xpt17CSC3VidInputSelect			SetXptCSC3VidInputSelect				///< @deprecated		Use SetXptCSC3KeyInputSelect instead.
	#define	SetK2Xpt17CSC4KeyInputSelect			SetXptCSC4KeyInputSelect				///< @deprecated		Use SetXptCSC3VidInputSelect instead.
	#define	SetK2Xpt17CSC4VidInputSelect			SetXptCSC4VidInputSelect				///< @deprecated		Use SetXptCSC4KeyInputSelect instead.
	#define	SetK2XptCSC5KeyInputSelect				SetXptCSC5KeyInputSelect				///< @deprecated		Use SetXptCSC4VidInputSelect instead.
	#define	SetK2XptCSC5VidInputSelect				SetXptCSC5VidInputSelect				///< @deprecated		Use SetXptCSC5KeyInputSelect instead.
	#define	SetK2XptHDMIOutV2Q1InputSelect			SetXptHDMIOutV2Q1InputSelect			///< @deprecated		Use SetXptCSC5VidInputSelect instead.
	#define	SetK2XptHDMIOutV2Q2InputSelect			SetXptHDMIOutV2Q2InputSelect			///< @deprecated		Use SetXptHDMIOutV2Q1InputSelect instead.
	#define	SetK2XptHDMIOutV2Q3InputSelect			SetXptHDMIOutV2Q3InputSelect			///< @deprecated		Use SetXptHDMIOutV2Q2InputSelect instead.
	#define	SetK2XptHDMIOutV2Q4InputSelect			SetXptHDMIOutV2Q4InputSelect			///< @deprecated		Use SetXptHDMIOutV2Q4InputSelect instead.
	#define	SetXena2VideoOutputStandard				SetVideoOutputStandard					///< @deprecated		Use SetVideoOutputStandard instead.
	#define	SetXptMixerBGKeyInputSelect				SetXptMixer1BGKeyInputSelect			///< @deprecated		Use SetXptMixer1BGKeyInputSelect instead.
	#define	GetXptMixerBGKeyInputSelect				GetXptMixer1BGKeyInputSelect			///< @deprecated		Use GetXptMixer1BGKeyInputSelect instead.
	#define	SetXptMixerBGVidInputSelect				SetXptMixer1BGVidInputSelect			///< @deprecated		Use SetXptMixer1BGVidInputSelect instead.
	#define	GetXptMixerBGVidInputSelect				GetXptMixer1BGVidInputSelect			///< @deprecated		Use GetXptMixer1BGVidInputSelect instead.
	#define	SetXptMixerFGKeyInputSelect				SetXptMixer1FGKeyInputSelect			///< @deprecated		Use SetXptMixer1FGKeyInputSelect instead.
	#define	GetXptMixerFGKeyInputSelect				GetXptMixer1FGKeyInputSelect			///< @deprecated		Use GetXptMixer1FGKeyInputSelect instead.
	#define	SetXptMixerFGVidInputSelect				SetXptMixer1FGVidInputSelect			///< @deprecated		Use SetXptMixer1FGVidInputSelect instead.
	#define	GetXptMixerFGVidInputSelect				GetXptMixer1FGVidInputSelect			///< @deprecated		Use GetXptMixer1FGVidInputSelect instead.
	#define	SetXptXptLUT2InputSelect				SetXptLUT2InputSelect					///< @deprecated		Use SetXptLUT2InputSelect instead.
	#define	GetXptXptLUT2InputSelect				GetXptLUT2InputSelect					///< @deprecated		Use GetXptLUT2InputSelect instead.
#endif	//	!defined (NTV2_DEPRECATE)

#endif	//	NTV2CARD_H
