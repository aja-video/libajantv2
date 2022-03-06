/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2democommon.h
	@brief		This file contains some structures, constants, classes and functions that are used in some of the demo applications.
				There is nothing magical about anything in this file. What's in here simply works well with the demos.
	@copyright	(C) 2013-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2DEMOCOMMON_H
#define _NTV2DEMOCOMMON_H

#include "stdint.h"
#include "ntv2rp188.h"
#include "ntv2publicinterface.h"
#include "ntv2testpatterngen.h"
#include "ntv2card.h"
#include "ajabase/common/options_popt.h"
#include "ajabase/common/timecodeburn.h"
#include "ajabase/system/debug.h"
#include <algorithm>
#include <string>

//	Convenience macros for EZ logging:
#define	CAPFAIL(_expr_)		AJA_sERROR  (AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	CAPWARN(_expr_)		AJA_sWARNING(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	CAPDBG(_expr_)		AJA_sDEBUG	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	CAPNOTE(_expr_)		AJA_sNOTICE	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)
#define	CAPINFO(_expr_)		AJA_sINFO	(AJA_DebugUnit_DemoCapture, AJAFUNC << ": " << _expr_)

#define	PLFAIL(_xpr_)		AJA_sERROR  (AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)
#define	PLWARN(_xpr_)		AJA_sWARNING(AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)
#define	PLNOTE(_xpr_)		AJA_sNOTICE	(AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)
#define	PLINFO(_xpr_)		AJA_sINFO	(AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)
#define	PLDBG(_xpr_)		AJA_sDEBUG	(AJA_DebugUnit_DemoPlayout, AJAFUNC << ": " << _xpr_)

#define	BURNFAIL(_expr_)	AJA_sERROR  (AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	BURNWARN(_expr_)	AJA_sWARNING(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	BURNDBG(_expr_)		AJA_sDEBUG	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	BURNNOTE(_expr_)	AJA_sNOTICE	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)
#define	BURNINFO(_expr_)	AJA_sINFO	(AJA_DebugUnit_Application, AJAFUNC << ": " << _expr_)

#define NTV2_AUDIOSIZE_MAX	(401 * 1024)
#define NTV2_ANCSIZE_MAX	(0x2000)		//	8K


/**
	@brief	This structure encapsulates the video, audio and anc buffers used in the AutoCirculate demos.
			These demos use a fixed number (CIRCULAR_BUFFER_SIZE) of these buffers in an AJACircularBuffer,
			which greatly simplifies processing frames between producer and consumer threads.
**/
typedef struct
{
	uint32_t *		fVideoBuffer;			///< @brief	Pointer to host video buffer
	uint32_t *		fVideoBuffer2;			///< @brief	Pointer to an additional host video buffer, usually field 2
	uint32_t		fVideoBufferSize;		///< @brief	Size of host video buffer, in bytes
	uint32_t *		fAudioBuffer;			///< @brief	Pointer to host audio buffer
	uint32_t		fAudioBufferSize;		///< @brief	Size of host audio buffer, in bytes
	uint32_t *		fAncBuffer;				///< @brief	Pointer to ANC buffer
	uint32_t		fAncBufferSize;			///< @brief	Size of ANC buffer, in bytes
	uint32_t *		fAncF2Buffer;			///< @brief	Pointer to "Field 2" ANC buffer
	uint32_t		fAncF2BufferSize;		///< @brief	Size of "Field 2" ANC buffer, in bytes
	uint32_t		fAudioRecordSize;		///< @brief	For future use
	uint32_t		fAncRecordSize;			///< @brief	For future use
	RP188_STRUCT	fRP188Data;				///< @brief	For future use
	RP188_STRUCT	fRP188Data2;			///< @brief	For future use
	uint8_t *		fVideoBufferUnaligned;	///< @brief	For future use
	uint32_t		fFrameFlags;			///< @brief Frame data flags
} AVDataBuffer;


/**
	@brief	I encapsulate the video, audio and anc host buffers used in the AutoCirculate demos.
			I'm a more modern version of the AVDataBuffer.
**/
AJAExport class NTV2FrameData
{
	public:
		NTV2_POINTER	fVideoBuffer;		///< @brief	Host video buffer
		NTV2_POINTER	fVideoBuffer2;		///< @brief	Additional host video buffer, usually F2
		NTV2_POINTER	fAudioBuffer;		///< @brief	Host audio buffer
		NTV2_POINTER	fAncBuffer;			///< @brief	Host ancillary data buffer
		NTV2_POINTER	fAncBuffer2;		///< @brief	Additional "F2" host anc buffer
		NTV2TimeCodes	fTimecodes;			///< @brief	Map of TC indexes to NTV2_RP188 values
		ULWord			fNumAudioBytes;		///< @brief	Actual number of captured audio bytes
		ULWord			fNumAncBytes;		///< @brief	Actual number of captured F1 anc bytes
		ULWord			fNumAnc2Bytes;		///< @brief	Actual number of captured F2 anc bytes
		uint32_t		fFrameFlags;		///< @brief Frame data flags
	public:
		explicit inline NTV2FrameData()
			:	fVideoBuffer	(0),
				fVideoBuffer2	(0),
				fAudioBuffer	(0),
				fAncBuffer		(0),
				fAncBuffer2		(0),
				fTimecodes		(),
				fNumAudioBytes	(0),
				fNumAncBytes	(0),
				fNumAnc2Bytes	(0),
				fFrameFlags(0)	{}

		//	Inquiry Methods
		inline NTV2_POINTER &	VideoBuffer (void)			{return fVideoBuffer;}
		inline ULWord	VideoBufferSize (void) const		{return fVideoBuffer.GetByteCount();}

		inline NTV2_POINTER &	AudioBuffer (void)			{return fAudioBuffer;}
		inline ULWord	AudioBufferSize (void) const		{return fAudioBuffer.GetByteCount();}
		inline ULWord	NumCapturedAudioBytes (void) const	{return fNumAudioBytes;}

		inline NTV2_POINTER &	AncBuffer (void)			{return fAncBuffer;}
		inline ULWord	AncBufferSize (void) const			{return fAncBuffer.GetByteCount();}
		inline ULWord	NumCapturedAncBytes (void) const	{return fNumAncBytes;}

		inline NTV2_POINTER &	AncBuffer2 (void)			{return fAncBuffer2;}
		inline ULWord	AncBuffer2Size (void) const			{return fAncBuffer2.GetByteCount();}
		inline ULWord	NumCapturedAnc2Bytes (void) const	{return fNumAnc2Bytes;}

		inline NTV2_POINTER &	VideoBuffer2 (void)			{return fVideoBuffer2;}
		inline ULWord	VideoBufferSize2 (void) const		{return fVideoBuffer2.GetByteCount();}

		inline bool		IsNULL (void) const					{return fVideoBuffer.IsNULL() && fVideoBuffer2.IsNULL()
																	&& fAudioBuffer.IsNULL() && fAncBuffer.IsNULL()
																	&& fAncBuffer2.IsNULL();}
		//	Modifier Methods
		inline void		ZeroBuffers (void)					{	if (fVideoBuffer)
																	fVideoBuffer.Fill(ULWord(0));
																if (fVideoBuffer2)
																	fVideoBuffer2.Fill(ULWord(0));
																if (fAudioBuffer)
																	fAudioBuffer.Fill(ULWord(0));
																if (fAncBuffer)
																	fAncBuffer.Fill(ULWord(0));
																if (fAncBuffer2)
																	fAncBuffer2.Fill(ULWord(0));
																fNumAudioBytes = fNumAncBytes = fNumAnc2Bytes = 0;
															}
		bool			LockAll								(CNTV2Card & inDevice);
		bool			UnlockAll							(CNTV2Card & inDevice);

		bool			Reset (void)						{return fVideoBuffer.Allocate(0) && fVideoBuffer2.Allocate(0)
																	&& fAudioBuffer.Allocate(0) && fAncBuffer.Allocate(0)
																	&& fAncBuffer2.Allocate(0);}
};	//	NTV2FrameData

typedef std::vector<NTV2FrameData>			NTV2FrameDataArray;				///< @brief A vector of NTV2FrameData elements
typedef NTV2FrameDataArray::iterator		NTV2FrameDataArrayIter;			///< @brief Handy non-const iterator
typedef NTV2FrameDataArray::const_iterator	NTV2FrameDataArrayConstIter;	///< @brief Handy const iterator



static const size_t CIRCULAR_BUFFER_SIZE	(10);	///< @brief	Number of AVDataBuffers in our ring
static const ULWord	kDemoAppSignature		NTV2_FOURCC('D','E','M','O');


/**
	@brief	A handy class that makes it easy to "bounce" an unsigned integer value between a minimum and maximum value
			using sequential calls to its Next method.
**/
template <typename T> class Bouncer
{
	public:
		inline Bouncer (const T inUpperLimit, const T inLowerLimit = T(0), const T inStartValue = T(0), const bool inStartAscend = true)
			:	mMin		(inLowerLimit),
				mMax		(inUpperLimit),
				mValue		(inStartValue),
				mIncrement	(T(1)),
				mAscend		(inStartAscend)
		{
			if (mMin > mMax)
				std::swap (mMin, mMax);
			else if (mMin == mMax)
				mMax = mMin + mIncrement;
			if (mValue < mMin)
			{
				mValue = mMin;
				mAscend = true;
			}
			if (mValue > mMax)
			{
				mValue = mMax;
				mAscend = false;
			}
		}

		inline T	Next (void)
		{
			if (mAscend)
			{
				if (mValue < mMax)
					mValue += mIncrement;
				else
					mAscend = false;
			}
			else
			{
				if (mValue > mMin)
					mValue -= mIncrement;
				else
					mAscend = true;
			}
			return mValue;
		}

		inline void	SetIncrement (const T inValue)	{mIncrement = inValue;}
		inline T	Value (void) const	{return mValue;}

	private:
		T		mMin, mMax, mValue, mIncrement;
		bool	mAscend;

};	//	Bouncer


typedef enum _NTV2VideoFormatKinds
{
	VIDEO_FORMATS_ALL		= 0xFF,
	VIDEO_FORMATS_NON_4KUHD	= 1,
	VIDEO_FORMATS_4KUHD		= 2,
	VIDEO_FORMATS_8KUHD2	= 3,
	VIDEO_FORMATS_NONE		= 0,
	//	Deprecated old ones:
	VIDEO_FORMATS_UHD2		= VIDEO_FORMATS_8KUHD2,
	BOTH_VIDEO_FORMATS		= VIDEO_FORMATS_ALL,
	NON_UHD_VIDEO_FORMATS	= VIDEO_FORMATS_NON_4KUHD,
	UHD_VIDEO_FORMATS		= VIDEO_FORMATS_4KUHD

} NTV2VideoFormatKinds;


typedef enum _NTV2PixelFormatKinds
{
	PIXEL_FORMATS_ALL		= 0xFF,
	PIXEL_FORMATS_RGB		= 1,
	PIXEL_FORMATS_PLANAR	= 2,
	PIXEL_FORMATS_RAW		= 4,
	PIXEL_FORMATS_PACKED	= 8,
	PIXEL_FORMATS_ALPHA		= 16,
	PIXEL_FORMATS_NONE		= 0
} NTV2PixelFormatKinds;


typedef enum _NTV2TCIndexKinds
{
	TC_INDEXES_ALL		= 0xFF,
	TC_INDEXES_SDI		= 1,
	TC_INDEXES_ANALOG	= 2,
	TC_INDEXES_ATCLTC	= 4,
	TC_INDEXES_VITC1	= 8,
	TC_INDEXES_VITC2	= 16,
	TC_INDEXES_NONE		= 0
} NTV2TCIndexKinds;



/**
	@brief	A set of common convenience functions used by the NTV2 \ref demoapps.
			Most are used for converting a command line argument into ::NTV2VideoFormat,
			::NTV2FrameBufferFormat, etc. types.
**/
class CNTV2DemoCommon
{
	public:
	/**
		@name	Device Functions
	**/
	///@{
		/**
			@param[in]	inDeviceSpec	A string containing a decimal index number, device serial number, or a device model name.
			@return		True if the specified device exists and can be opened.
		**/
		static bool							IsValidDevice (const std::string & inDeviceSpec);

		/**
			@param[in]	inKinds				Specifies the kinds of devices to be returned. Defaults to all available devices.
			@return		A string that can be printed to show the available supported devices.
			@note		These device identifier strings are mere conveniences for specifying devices in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetDeviceStrings (const NTV2DeviceKinds inKinds = NTV2_DEVICEKIND_ALL);
	///@}

	/**
		@name	Video Format Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of video formats returned. Defaults to non-4K/UHD formats.
			@return		The supported ::NTV2VideoFormatSet.
		**/
		static const NTV2VideoFormatSet &	GetSupportedVideoFormats (const NTV2VideoFormatKinds inKinds = VIDEO_FORMATS_NON_4KUHD);

		/**
			@param[in]	inKinds				Specifies the types of video formats returned. Defaults to non-4K/UHD formats.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the video format is incompatible with that device.
			@return		A string that can be printed to show the supported video formats.
			@note		These video format strings are mere conveniences for specifying video formats in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetVideoFormatStrings (const NTV2VideoFormatKinds inKinds = VIDEO_FORMATS_NON_4KUHD,
																	const std::string inDeviceSpecifier = std::string ());

		/**
			@brief	Returns the ::NTV2VideoFormat that matches the given string.
			@param[in]	inStr		Specifies the string to be converted to an ::NTV2VideoFormat.
			@param[in]	inKinds		Specifies which video format type is expected in "inStr", whether non-4K/UHD (the default),
									exclusively 4K/UHD, or both/all.
			@return		The given string converted to an ::NTV2VideoFormat, or ::NTV2_FORMAT_UNKNOWN if there's no match.
		**/
		static NTV2VideoFormat				GetVideoFormatFromString (const std::string & inStr,  const NTV2VideoFormatKinds inKinds = VIDEO_FORMATS_NON_4KUHD);

		/**
			@brief		Given a video format, if all 4 inputs are the same and promotable to 4K, this function does the promotion.
			@param		inOutVideoFormat	On entry, specifies the wire format;  on exit, receives the 4K video format.
			@return		True if successful;  otherwise false.
		**/
		static bool							Get4KInputFormat (NTV2VideoFormat & inOutVideoFormat);
		
		/**
			@brief		Given a video format, if all 4 inputs are the same and promotable to 8K, this function does the promotion.
			@param		inOutVideoFormat	On entry, specifies the wire format;  on exit, receives the 4K video format.
			@return		True if successful;  otherwise false.
		**/
		static bool							Get8KInputFormat (NTV2VideoFormat & inOutVideoFormat);
	///@}

	/**
		@name	Pixel Format Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of pixel formats returned. Defaults to all formats.
			@return		The supported ::NTV2FrameBufferFormatSet.
		**/
		static NTV2FrameBufferFormatSet		GetSupportedPixelFormats (const NTV2PixelFormatKinds inKinds = PIXEL_FORMATS_ALL);

		/**
			@param[in]	inKinds				Specifies the types of pixel formats returned. Defaults to all formats.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the pixel format is incompatible with that device.
			@return		A string that can be printed to show the available pixel formats (or those that are supported by a given device).
			@note		These pixel format strings are mere conveniences for specifying pixel formats in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetPixelFormatStrings (const NTV2PixelFormatKinds inKinds = PIXEL_FORMATS_ALL,
																	const std::string inDeviceSpecifier = std::string ());

		/**
			@brief	Returns the ::NTV2FrameBufferFormat that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an ::NTV2FrameBufferFormat.
			@return		The given string converted to an ::NTV2FrameBufferFormat, or ::NTV2_FBF_INVALID if there's no match.
		**/
		static NTV2FrameBufferFormat		GetPixelFormatFromString (const std::string & inStr);

		/**
			@return		The equivalent ::AJA_PixelFormat for the given ::NTV2FrameBufferFormat.
			@param[in]	inFormat	Specifies the ::NTV2FrameBufferFormat to be converted into an equivalent ::AJA_PixelFormat.
		**/
		static AJA_PixelFormat				GetAJAPixelFormat (const NTV2FrameBufferFormat inFormat);
	///@}

	/**
		@name	Input Source Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of input sources returned. Defaults to all sources.
			@return		The supported ::NTV2InputSourceSet.
		**/
		static const NTV2InputSourceSet		GetSupportedInputSources (const NTV2InputSourceKinds inKinds = NTV2_INPUTSOURCES_ALL);

		/**
			@param[in]	inKinds				Specifies the types of input sources returned. Defaults to all sources.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the input source is incompatible with that device.
			@return		A string that can be printed to show the available input sources (or those that are supported by a given device).
			@note		These input source strings are mere conveniences for specifying input sources in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetInputSourceStrings (const NTV2InputSourceKinds inKinds = NTV2_INPUTSOURCES_ALL,
																	const std::string inDeviceSpecifier = std::string ());

		/**
			@brief		Returns the ::NTV2InputSource that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an ::NTV2InputSource.
			@return		The given string converted to an ::NTV2InputSource, or ::NTV2_INPUTSOURCE_INVALID if there's no match.
		**/
		static NTV2InputSource				GetInputSourceFromString (const std::string & inStr);
	///@}

	/**
		@name	Output Destination Functions
	**/
	///@{
		/**
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the output destination is incompatible with that device.
			@return		A string that can be printed to show the available output destinations (or those that are supported by a given device).
			@note		These output destination strings are mere conveniences for specifying output destinations in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetOutputDestinationStrings (const std::string inDeviceSpecifier = std::string ());

		/**
			@brief		Returns the ::NTV2OutputDestination that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an ::NTV2OutputDestination.
			@return		The given string converted to an ::NTV2OutputDestination, or ::NTV2_OUTPUTDESTINATION_INVALID if there's no match.
		**/
		static NTV2OutputDestination		GetOutputDestinationFromString (const std::string & inStr);
	///@}

	/**
		@name	Timecode Functions
	**/
	///@{
		/**
			@param[in]	inKinds				Specifies the types of timecode indexes returned. Defaults to all indexes.
			@return		The supported ::NTV2TCIndexes set.
		**/
		static const NTV2TCIndexes			GetSupportedTCIndexes (const NTV2TCIndexKinds inKinds);

		/**
			@param[in]	inKinds				Specifies the types of timecode indexes returned. Defaults to all indexes.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the timecode index is incompatible with that device.
			@param[in]	inIsInputOnly		Optionally specifies if intended for timecode input (capture).
											Defaults to 'true'. Specify 'false' to obtain the list of timecode indexes
											that are valid for the given device for either input (capture) or output
											(playout).
			@return		A string that can be printed to show the available timecode indexes (or those that are supported by a given device).
			@note		These timecode index strings are mere conveniences for specifying timecode indexes in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetTCIndexStrings (const NTV2TCIndexKinds inKinds = TC_INDEXES_ALL,
																const std::string inDeviceSpecifier = std::string(),
																const bool inIsInputOnly = true);

		/**
			@brief		Returns the ::NTV2TCIndex that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an ::NTV2TCIndex.
			@return		The given string converted to an ::NTV2TCIndex, or ::NTV2_TCINDEX_INVALID if there's no match.
		**/
		static NTV2TCIndex					GetTCIndexFromString (const std::string & inStr);
	///@}

	/**
		@name	Audio System Functions
	**/
	///@{
		/**
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											returns the audio systems that are compatible with that device.
			@return		A string that can be printed to show the available audio systems that are supported by a given device.
			@note		These audio system strings are mere conveniences for specifying audio systems in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetAudioSystemStrings (const std::string inDeviceSpecifier = std::string ());

		/**
			@brief	Returns the ::NTV2AudioSystem that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an ::NTV2AudioSystem.
			@return		The given string converted to an ::NTV2AudioSystem, or ::NTV2_AUDIOSYSTEM_INVALID if there's no match.
		**/
		static NTV2AudioSystem				GetAudioSystemFromString (const std::string & inStr);
	///@}

	/**
		@name	Test Pattern Functions
	**/
	///@{
		/**
			@return		A string that can be printed to show the available test pattern and color identifiers.
			@note		These test pattern strings are mere conveniences for specifying test patterns in the command-line-based demo apps,
						and are subject to change without notice. They are not intended to be canonical in any way.
		**/
		static std::string					GetTestPatternStrings (void);

		/**
			@param[in]	inStr	Specifies the string to be converted to a valid test pattern or color name.
			@return		The test pattern or color name that best matches the given string, or an empty string if invalid.
		**/
		static std::string					GetTestPatternNameFromString (const std::string & inStr);
	///@}

	/**
		@name	Miscellaneous Functions
	**/
	///@{
		/**
			@brief	Returns the given string after converting it to lower case.
			@param[in]	inStr	Specifies the string to be converted to lower case.
			@return		The given string converted to lower-case.
			@note		Only works with ASCII characters!
		**/
		static std::string					ToLower (const std::string & inStr);

		/**
			@param[in]	inStr	Specifies the string to be stripped.
			@return		The given string after stripping all spaces, periods, and "00"s.
			@note		Only works with ASCII character strings!
		**/
		static std::string					StripFormatString (const std::string & inStr);

		/**
			@brief	Returns the character that represents the last key that was pressed on the keyboard
					without waiting for Enter or Return to be pressed.
		**/
		static char							ReadCharacterPress (void);

		/**
			@brief	Prompts the user (via stdout) to press the Return or Enter key, then waits for it to happen.
		**/
		static void							WaitForEnterKeyPress (void);

		/**
		@return		The equivalent TimecodeFormat for a given NTV2FrameRate.
		@param[in]	inFrameRate		Specifies the NTV2FrameRate to be converted into an equivalent TimecodeFormat.
		**/
		static TimecodeFormat				NTV2FrameRate2TimecodeFormat(const NTV2FrameRate inFrameRate);

		/**
			@return		The equivalent AJA_FrameRate for the given NTV2FrameRate.
			@param[in]	inFrameRate	Specifies the NTV2FrameRate to be converted into an equivalent AJA_FrameRate.
		**/
		static AJA_FrameRate				GetAJAFrameRate (const NTV2FrameRate inFrameRate);

		/**
			@return		A pointer to a 'C' string containing the name of the AJA NTV2 demonstration application global mutex.
		**/
		static const char *					GetGlobalMutexName (void);

		/**
			@return		The TSIMuxes to use given the first FrameStore on the device and a count.
			@param[in]	inDeviceID		Specifies the device being used.
			@param[in]	in1stFrameStore	Specifies the first FrameStore of interest.
			@param[in]	inCount			Specifies the number of Muxes.
		**/
		static NTV2ChannelList				GetTSIMuxesForFrameStore (const NTV2DeviceID inDeviceID, const NTV2Channel in1stFrameStore, const UWord inCount);
	///@}


	/**
		@brief	AutoCirculate Frame Range
	**/
	class ACFrameRange
	{
		public:
			explicit inline	ACFrameRange (const UWord inFrameCount = 0)
						:	mIsCountOnly	(true),
							mFrameCount		(inFrameCount),
							mFirstFrame		(0),
							mLastFrame		(0)
						{}
			explicit inline	ACFrameRange (const UWord inFirstFrame, const UWord inLastFrame)
						:	mIsCountOnly	(true),
							mFrameCount		(0),
							mFirstFrame		(inFirstFrame),
							mLastFrame		(inLastFrame)
						{}
			inline bool		isCount(void) const			{return mIsCountOnly;}
			inline bool		isFrameRange(void) const	{return !isCount();}
			inline UWord	count(void) const			{return isCount() ? mFrameCount : 0;}
			inline UWord	firstFrame(void) const		{return mFirstFrame;}
			inline UWord	lastFrame(void) const		{return mLastFrame;}
			inline bool		valid(void) const
							{
								if (isCount())
									return count() > 0;
								return lastFrame() >= firstFrame();
							}
			inline ACFrameRange &	makeInvalid(void)	{mIsCountOnly = true;  mFrameCount = mFirstFrame = mLastFrame = 0; return *this;}
			std::string		setFromString(const std::string & inStr);
			std::string		toString(void) const;
		private:
			bool	mIsCountOnly;	///< @brief	Frame count only? If false, specifies absolute frame range.
			UWord	mFrameCount;	///< @brief	Frame count (mIsCountOnly == true).
			UWord	mFirstFrame;	///< @brief	First frame (mIsCountOnly == false).
			UWord	mLastFrame;		///< @brief	Last frame (mIsCountOnly == false).
	};


	typedef struct poptOption	PoptOpts;
	class Popt
	{
		public:
			Popt (const int inArgc, const char ** pArgs, const PoptOpts * pInOptionsTable);
			virtual								~Popt();
			virtual inline int					parseResult(void) const		{return mResult;}
			virtual inline bool					isGood (void) const			{return parseResult() == -1;}
			virtual inline						operator bool() const		{return isGood();}
			virtual inline const std::string &	errorStr (void) const		{return mError;}
		private:
			poptContext	mContext;
			int			mResult;
			std::string	mError;
	};

	static bool	BFT(void);

};	//	CNTV2DemoCommon


//	These AJA_NTV2_AUDIO_RECORD* macros can, if enabled, record audio samples into a file in the current directory.
//	Optionally used in the CNTV2Capture demo.
#if defined(AJA_RAW_AUDIO_RECORD)
	#include "ntv2debug.h"					//	For NTV2DeviceString
	#include <fstream>						//	For ofstream
	//	To open the raw audio file in Audacity -- see http://audacity.sourceforge.net/ ...
	//		1)	Choose File => Import => Raw Data...
	//		2)	Select "Signed 32 bit PCM", Little/No/Default Endian, "16 Channels" (or 8 if applicable), "48000" sample rate.
	//		3)	Click "Import"
	#define		AJA_NTV2_AUDIO_RECORD_BEGIN		ostringstream	_filename;														\
												_filename	<< ::NTV2DeviceString(mDeviceID) << "-" << mDevice.GetIndexNumber()	\
															<< "." << ::NTV2ChannelToString(mConfig.fInputChannel,true)					\
															<< "." << ::NTV2InputSourceToString(mConfig.fInputSource, true)				\
															<< "." << ::NTV2VideoFormatToString(mVideoFormat)					\
															<< "." << ::NTV2AudioSystemToString(mAudioSystem, true)				\
															<< "." << AJAProcess::GetPid()										\
															<< ".raw";															\
												ofstream _ostrm(_filename.str(), ios::binary);

	#define		AJA_NTV2_AUDIO_RECORD_DO		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))									\
													if (pFrameData->fAudioBuffer)												\
														_ostrm.write(pFrameData->AudioBytes(),									\
																	streamsize(pFrameData->NumCapturedAudioBytes()));

	#define		AJA_NTV2_AUDIO_RECORD_END		
#elif defined(AJA_WAV_AUDIO_RECORD)
	#include "ntv2debug.h"					//	For NTV2DeviceString
	#include "ajabase/common/wavewriter.h"	//	For AJAWavWriter
	#define		AJA_NTV2_AUDIO_RECORD_BEGIN		ostringstream	_wavfilename;														\
												_wavfilename	<< ::NTV2DeviceString(mDeviceID) << "-" << mDevice.GetIndexNumber()	\
																<< "." << ::NTV2ChannelToString(mConfig.fInputChannel,true)					\
																<< "." << ::NTV2InputSourceToString(mConfig.fInputSource, true)				\
																<< "." << ::NTV2VideoFormatToString(mVideoFormat)					\
																<< "." << ::NTV2AudioSystemToString(mAudioSystem, true)				\
																<< "." << AJAProcess::GetPid()										\
																<< ".wav";															\
												const int		_wavMaxNumAudChls(::NTV2DeviceGetMaxAudioChannels(mDeviceID));		\
												AJAWavWriter	_wavWriter (_wavfilename.str(),										\
																			AJAWavWriterAudioFormat(_wavMaxNumAudChls, 48000, 32));	\
												_wavWriter.open();

	#define		AJA_NTV2_AUDIO_RECORD_DO		if (NTV2_IS_VALID_AUDIO_SYSTEM(mAudioSystem))										\
													if (pFrameData->fAudioBuffer)													\
														if (_wavWriter.IsOpen())													\
															_wavWriter.write(pFrameData->AudioBytes(), pFrameData->NumCapturedAudioBytes());

	#define		AJA_NTV2_AUDIO_RECORD_END		if (_wavWriter.IsOpen())															\
													_wavWriter.close();
#else
	#define		AJA_NTV2_AUDIO_RECORD_BEGIN		
	#define		AJA_NTV2_AUDIO_RECORD_DO			
	#define		AJA_NTV2_AUDIO_RECORD_END		
#endif

#endif	//	_NTV2DEMOCOMMON_H
