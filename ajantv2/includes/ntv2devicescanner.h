/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2devicescanner.h
	@brief		Declares the CNTV2DeviceScanner class.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#ifndef NTV2DEVICESCANNER_H
#define NTV2DEVICESCANNER_H

#include "ajatypes.h"
#if !defined(NTV2_DEPRECATE_17_1)
	#include "ntv2audiodefines.h"
#endif	//	!defined(NTV2_DEPRECATE_17_1)
#include "ntv2card.h"
#include <vector>
#include <algorithm>

#if !defined(NTV2_DEPRECATE_17_1)
typedef std::vector <AudioSampleRateEnum>				NTV2AudioSampleRateList;
typedef NTV2AudioSampleRateList::const_iterator			NTV2AudioSampleRateListConstIter;
typedef NTV2AudioSampleRateList::iterator				NTV2AudioSampleRateListIter;

typedef std::vector <AudioChannelsPerFrameEnum>			NTV2AudioChannelsPerFrameList;
typedef NTV2AudioChannelsPerFrameList::const_iterator	NTV2AudioChannelsPerFrameListConstIter;
typedef NTV2AudioChannelsPerFrameList::iterator			NTV2AudioChannelsPerFrameListIter;

typedef std::vector <AudioSourceEnum>					NTV2AudioSourceList;
typedef NTV2AudioSourceList::const_iterator				NTV2AudioSourceListConstIter;
typedef NTV2AudioSourceList::iterator					NTV2AudioSourceListIter;

typedef std::vector <AudioBitsPerSampleEnum>			NTV2AudioBitsPerSampleList;
typedef NTV2AudioBitsPerSampleList::const_iterator		NTV2AudioBitsPerSampleListConstIter;
typedef NTV2AudioBitsPerSampleList::iterator			NTV2AudioBitsPerSampleListIter;

/**
	@deprecated Use the DeviceCapabilities from CNTV2Card::features instead.
**/
typedef struct NTV2DeviceInfo
{
	NTV2DeviceID					deviceID;							///< @brief Device ID/species	(e.g., DEVICE_ID_KONA3G, DEVICE_ID_IOXT, etc.)
	ULWord							deviceIndex;						///< @brief		Device index number -- this will be phased out someday
	ULWord							pciSlot;							///< @brief PCI slot (if applicable and/or known)
	uint64_t						deviceSerialNumber;					///< @brief Unique device serial number (obsolete)
	std::string						serialNumber;						///< @brief Unique device serial number (new in SDK 17.5)
	std::string						deviceIdentifier;					///< @brief Device name as seen in Control Panel, Watcher, Cables, etc.
	UWord							numVidInputs;						///< @brief Total number of video inputs -- analog, digital, whatever
	UWord							numVidOutputs;						///< @brief Total number of video outputs -- analog, digital, whatever
	UWord							numAnlgVidInputs;					///< @brief Total number of analog video inputs
	UWord							numAnlgVidOutputs;					///< @brief Total number of analog video outputs
	UWord							numHDMIVidInputs;					///< @brief Total number of HDMI inputs
	UWord							numHDMIVidOutputs;					///< @brief Total number of HDMI outputs
	UWord							numInputConverters;					///< @brief Total number of input converters
	UWord							numOutputConverters;				///< @brief Total number of output converters
	UWord							numUpConverters;					///< @brief Total number of up-converters
	UWord							numDownConverters;					///< @brief Total number of down-converters
	UWord							downConverterDelay;
	bool							isoConvertSupport;
	bool							rateConvertSupport;
	bool							dvcproHDSupport;
	bool							qrezSupport;
	bool							hdvSupport;
	bool							quarterExpandSupport;
	bool							vidProcSupport;
	bool							dualLinkSupport;					///< @brief Supports dual-link?
	bool							colorCorrectionSupport;				///< @brief Supports color correction?
	bool							programmableCSCSupport;				///< @brief Programmable color space converter?
	bool							rgbAlphaOutputSupport;				///< @brief Supports RGB alpha channel?
	bool							breakoutBoxSupport;					///< @brief Can support a breakout box?
	bool							procAmpSupport;
	bool							has2KSupport;						///< @brief Supports 2K formats?
	bool							has4KSupport;						///< @brief Supports 4K formats?
	bool							has8KSupport;						///< @brief Supports 8K formats?
	bool							has3GLevelConversion;				///< @brief Supports 3G Level Conversion?
	bool							proResSupport;						///< @brief Supports ProRes?
	bool							sdi3GSupport;						///< @brief Supports 3G?
	bool							sdi12GSupport;						///< @brief Supports 12G?
	bool							ipSupport;							///< @brief Supports IP IO?
	bool							biDirectionalSDI;					///< @brief Supports Bi-directional SDI
	bool							ltcInSupport;						///< @brief Accepts LTC input?
	bool							ltcOutSupport;						///< @brief Supports LTC output?
	bool							ltcInOnRefPort;						///< @brief Supports LTC on reference input?
	bool							stereoOutSupport;					///< @brief Supports stereo output?
	bool							stereoInSupport;					///< @brief Supports stereo input?
	bool							multiFormat;						///< @brief Supports multiple video formats?
	NTV2AudioSampleRateList			audioSampleRateList;				///< @brief My supported audio sample rates
	NTV2AudioChannelsPerFrameList	audioNumChannelsList;				///< @brief My supported number of audio channels per frame
	NTV2AudioBitsPerSampleList		audioBitsPerSampleList;				///< @brief My supported audio bits-per-sample
	NTV2AudioSourceList				audioInSourceList;					///< @brief My supported audio input sources (AES, ADAT, etc.)
	NTV2AudioSourceList				audioOutSourceList;					///< @brief My supported audio output destinations (AES, etc.)
	UWord							numAudioStreams;					///< @brief Maximum number of independent audio streams
	UWord							numAnalogAudioInputChannels;		///< @brief Total number of analog audio input channels
	UWord							numAESAudioInputChannels;			///< @brief Total number of AES audio input channels
	UWord							numEmbeddedAudioInputChannels;		///< @brief Total number of embedded (SDI) audio input channels
	UWord							numHDMIAudioInputChannels;			///< @brief Total number of HDMI audio input channels
	UWord							numAnalogAudioOutputChannels;		///< @brief Total number of analog audio output channels
	UWord							numAESAudioOutputChannels;			///< @brief Total number of AES audio output channels
	UWord							numEmbeddedAudioOutputChannels;		///< @brief Total number of embedded (SDI) audio output channels
	UWord							numHDMIAudioOutputChannels;			///< @brief Total number of HDMI audio output channels
	UWord							numDMAEngines;						///< @brief Total number of DMA engines
	UWord							numSerialPorts;						///< @brief Total number of serial ports
	ULWord							pingLED;
	bool							isVirtualDevice=false;
	std::string						vdevUrl;

	AJAExport	bool operator == (const NTV2DeviceInfo & rhs) const;	///< @return	True if I'm equivalent to another ::NTV2DeviceInfo struct.
	AJAExport	inline bool operator != (const NTV2DeviceInfo & rhs) const	{ return !(*this == rhs); } ///< @return	True if I'm different from another ::NTV2DeviceInfo struct.
	AJAExport	NTV2DeviceInfo();
	AJAExport	NTV2DeviceInfo(const NTV2DeviceInfo & info);

} NTV2DeviceInfo;



//	ostream operators

/**
	@brief	Streams the NTV2AudioSampleRateList to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2AudioSampleRateList is to be streamed.
	@param[in]	inList		Specifies the NTV2AudioSampleRateList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioSampleRateList & inList);

/**
	@brief	Streams the NTV2AudioChannelsPerFrameList to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2AudioChannelsPerFrameList is to be streamed.
	@param[in]	inList		Specifies the NTV2AudioChannelsPerFrameList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioChannelsPerFrameList & inList);

/**
	@brief	Streams the NTV2AudioSourceList to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2AudioSourceList is to be streamed.
	@param[in]	inList		Specifies the NTV2AudioSourceList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioSourceList & inList);

/**
	@brief	Streams the NTV2AudioBitsPerSampleList to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2AudioBitsPerSampleList is to be streamed.
	@param[in]	inList		Specifies the NTV2AudioBitsPerSampleList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioBitsPerSampleList & inList);

/**
	@brief	Streams the NTV2DeviceInfo to the given output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2DeviceInfo is to be streamed.
	@param[in]	inInfo		Specifies the NTV2DeviceInfo to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2DeviceInfo & inInfo);


/**
	@brief	I am an ordered list of NTV2DeviceInfo structs.
**/
typedef std::vector <NTV2DeviceInfo>		NTV2DeviceInfoList;
typedef NTV2DeviceInfoList::const_iterator	NTV2DeviceInfoListConstIter;	//	Const iterator shorthand
typedef NTV2DeviceInfoList::iterator		NTV2DeviceInfoListIter;			//	Iterator shorthand


/**
	@brief		Streams the NTV2DeviceInfoList to an output stream in a human-readable format.
	@param		inOutStr	The output stream into which the NTV2DeviceInfoList is to be streamed.
	@param[in]	inList		Specifies the NTV2DeviceInfoList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2DeviceInfoList & inList);


typedef struct {
	ULWord							boardNumber;
	AudioSampleRateEnum				sampleRate;
	AudioChannelsPerFrameEnum		numChannels;
	AudioBitsPerSampleEnum			bitsPerSample;
	AudioSourceEnum					sourceIn;
	AudioSourceEnum					sourceOut;
} NTV2AudioPhysicalFormat;


/**
	@brief		Streams the AudioPhysicalFormat to an output stream in a human-readable format.
	@param		inOutStr	The output stream into which the AudioPhysicalFormat is to be streamed.
	@param[in]	inFormat	Specifies the AudioPhysicalFormat to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioPhysicalFormat & inFormat);


/**
	@brief	I am an ordered list of NTV2AudioPhysicalFormat structs.
**/
typedef std::vector <NTV2AudioPhysicalFormat>			NTV2AudioPhysicalFormatList;
typedef NTV2AudioPhysicalFormatList::const_iterator		NTV2AudioPhysicalFormatListConstIter;	//	Shorthand for const_iterator
typedef NTV2AudioPhysicalFormatList::iterator			NTV2AudioPhysicalFormatListIter;		//	Shorthand for iterator


/**
	@brief		Streams the AudioPhysicalFormatList to an output stream in a human-readable format.
	@param		inOutStr	The output stream into which the AudioPhysicalFormatList is to be streamed.
	@param[in]	inList		Specifies the AudioPhysicalFormatList to be streamed.
	@return		The output stream.
**/
AJAExport	std::ostream &	operator << (std::ostream & inOutStr, const NTV2AudioPhysicalFormatList & inList);

#endif	//	!defined(NTV2_DEPRECATE_17_1)


/**
	@brief	This class is used to enumerate AJA devices that are attached and known to the local host computer.
**/
class AJAExport CNTV2DeviceScanner
{
//	Class Methods
public:
	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the AJA device having the given zero-based index number.
		@return		True if successful; otherwise false.
		@param[in]	inDeviceIndexNumber Specifies the AJA device using a zero-based index number.
		@param[out] outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetDeviceAtIndex (const ULWord inDeviceIndexNumber, CNTV2Card & outDevice);

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the first AJA device found on the host that has the given NTV2DeviceID.
		@return		True if successful; otherwise false.
		@param[in]	inDeviceID			Specifies the device identifier of interest.
		@param[out] outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetFirstDeviceWithID (const NTV2DeviceID inDeviceID, CNTV2Card & outDevice);

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the first AJA device whose device identifier name contains the given substring.
		@note		The name is compared case-insensitively (e.g., "iO4K" == "Io4k").
		@return		True if successful; otherwise false.
		@param[in]	inNameSubString		Specifies a portion of the device name to search for.
		@param[out] outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetFirstDeviceWithName (const std::string & inNameSubString, CNTV2Card & outDevice);

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the first AJA device whose serial number contains the given value.
		@note		The serial value is compared case-sensitively.
		@return		True if successful; otherwise false.
		@param[in]	inSerialStr			Specifies the device serial value to search for.
		@param[out] outDevice			Receives the open, ready-to-use CNTV2Card instance of the first matching device.
	**/
	static bool									GetFirstDeviceWithSerial (const std::string & inSerialStr, CNTV2Card & outDevice);	//	New in SDK 16.0

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the first AJA device whose serial number
					matches the given value.
		@return		True if successful; otherwise false.
		@param[in]	inSerialNumber		Specifies the device serial value to search for.
		@param[out] outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetDeviceWithSerial (const std::string & inSerialNumber, CNTV2Card & outDevice); //	New in SDK 17.5, replaced uint64_t version

	/**
		@brief		Rescans the host, and returns an open CNTV2Card instance for the AJA device that matches a command line argument
					according to the following evaluation sequence:
					-#	1 or 2 digit unsigned decimal integer:	a zero-based device index number;
					-#	8 or 9 character alphanumeric string:	device with a matching serial number string (case-insensitive comparison);
					-#	3-16 character hexadecimal integer, optionally preceded by '0x':  device having a matching 64-bit serial number;
					-#	All other cases:  first device (lowest index number) whose name contains the argument string (compared case-insensitively).
		@return		True if successful; otherwise false.
		@param[in]	inArgument			The argument string. If 'list' or '?', the std::cout stream is sent some
										"help text" showing a list of all available devices.
		@param[out] outDevice			Receives the open, ready-to-use CNTV2Card instance.
	**/
	static bool									GetFirstDeviceFromArgument (const std::string & inArgument, CNTV2Card & outDevice);

	static size_t		GetNumDevices (void);	///< @deprecated	Do not use

	/**
		@param[in]	inDevice			The CNTV2Card instance that's open for the device of interest.
		@return		A string containing the device name that will find the same given device using CNTV2DeviceScanner::GetFirstDeviceFromArgument.
	**/
	static std::string							GetDeviceRefName (CNTV2Card & inDevice);	//	New in SDK 16.0

	/**
		@return True if the string contains a legal serial number.
		@param[in]	inStr	The string to be tested.
	**/
	static bool			IsLegalSerialNumber (const std::string & inStr);	//	New in SDK 16.0

#if !defined(NTV2_DEPRECATE_17_1)
	static NTV2_DEPRECATED_f(bool IsLegalDecimalNumber (const std::string & inStr, const size_t maxLen = 2)); ///< @deprecated	Use aja::is_legal_decimal_number instead
	static NTV2_DEPRECATED_f(uint64_t IsLegalHexSerialNumber (const std::string & inStr)); ///< @deprecated	Use aja::is_legal_hex_serial_number instead
	static NTV2_DEPRECATED_f(bool IsHexDigit (const char inChr));	///< @deprecated	Use aja::is_hex_digit instead
	static NTV2_DEPRECATED_f(bool IsDecimalDigit (const char inChr));	///< @deprecated	Use aja::is_decimal_digit instead
	static NTV2_DEPRECATED_f(bool IsAlphaNumeric (const char inStr));	///< @deprecated	Use aja::is_alpha_numeric instead
	static NTV2_DEPRECATED_f(bool IsAlphaNumeric (const std::string & inStr)); ///< @deprecated	Use aja::is_alpha_numeric instead
#endif	//	!defined(NTV2_DEPRECATE_17_1)
#if !defined(NTV2_DEPRECATE_17_5)
	static NTV2_MUST_DEPRECATE(bool GetDeviceWithSerial (const uint64_t sn, CNTV2Card & dev)); ///< @deprecated	Use the string version of this function instead
#endif	//	!defined(NTV2_DEPRECATE_17_5)

#if !defined(NTV2_DEPRECATE_17_1)
//	Instance Methods
public:
	explicit			CNTV2DeviceScanner (const bool inScanNow = true);
#if !defined(NTV2_DEPRECATE_16_3)
	explicit			CNTV2DeviceScanner (bool inScanNow, UWord inDeviceMask);
#endif	//	!defined(NTV2_DEPRECATE_16_3)


	static void			ScanHardware (void);	///< @deprecated	Do not use
#if !defined(NTV2_DEPRECATE_17_1)
	static NTV2_DEPRECATED_f(void ScanHardware (const UWord inMask))	{(void)inMask;  ScanHardware();}	///< @deprecated	Do not use
#endif	//	!defined(NTV2_DEPRECATE_17_1)
	static bool		DeviceIDPresent (const NTV2DeviceID inDeviceID, const bool inRescan = false);	///< @deprecated	Do not use
	static bool		GetDeviceInfo (const ULWord inDeviceIndexNumber, NTV2DeviceInfo & outDeviceInfo, const bool inRescan = false);	///< @deprecated	Do not use
	static NTV2DeviceInfoList	GetDeviceInfoList (void);	///< @deprecated	Do not use
	static void		SortDeviceInfoList (void)	{}	///< @deprecated	Obsolete
	static bool		CompareDeviceInfoLists (const NTV2DeviceInfoList & inOldList,
											const NTV2DeviceInfoList & inNewList,
											NTV2DeviceInfoList & outDevicesAdded,
											NTV2DeviceInfoList & outDevicesRemoved);
private:
	static void		SetAudioAttributes (NTV2DeviceInfo & inDeviceInfo, CNTV2Card & inDevice);
	static bool		GetVirtualDeviceList(NTV2DeviceInfoList& outVirtualDevList);
#endif	//	!defined(NTV2_DEPRECATE_17_1)
};	//	CNTV2DeviceScanner

#endif	//	NTV2DEVICESCANNER_H
