/**
	@file		ntv2democommon.h
	@brief		This file contains some structures, constants, classes and functions that are used in some of the demo applications.
				There is nothing magical about anything in this file. In your applications you may use a different
				number of circular buffers, or store different data in the AVDataBuffer. What's listed below
				are simply values that work well with the demos.
	@copyright	Copyright (C) 2013-2018 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2DEMOCOMMON_H
#define _NTV2DEMOCOMMON_H

#include "stdint.h"
#include "ntv2rp188.h"
#include "ajabase/common/timecodeburn.h"
#include <algorithm>
#include <string>


/**
	@brief	This structure encapsulates the video and audio buffers used by AutoCirculate. The demo programs
			that employ producer/consumer threads use a fixed number (CIRCULAR_BUFFER_SIZE) of these buffers.
			The AJACircularBuffer template class greatly simplifies implementing this approach to efficiently
			process frames.
	@note	The unused fields in this structure are in preparation for adding features to these demo apps,
			e.g., embedding additional timecode (e.g., VITC) to the output video that's sourced from the input video.
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
	uint32_t		fAudioRecordSize;		///< @brief	For future use -- (see note)
	uint32_t		fAncRecordSize;			///< @brief	For future use -- (see note)
	RP188_STRUCT	fRP188Data;				///< @brief	For future use -- (see note)
	RP188_STRUCT	fRP188Data2;			///< @brief	For future use -- (see note)
	uint8_t *		fVideoBufferUnaligned;	///< @brief	For future use -- (see note)
} AVDataBuffer;



const unsigned int	CIRCULAR_BUFFER_SIZE	(10);		///< @brief	Specifies how many AVDataBuffers constitute the circular buffer


/**
	@brief	A handy class that makes it easy to "bounce" an unsigned integer value between a minimum and maximum value.
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
	VIDEO_FORMATS_NONE		= 0,
	//	Deprecated old ones:
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



class CNTV2DemoCommon
{
	public:
	/**
		@name	Video Format Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of video formats returned. Defaults to non-4K/UHD formats.
			@return		The supported NTV2VideoFormatSet.
		**/
		static const NTV2VideoFormatSet &	GetSupportedVideoFormats (const NTV2VideoFormatKinds inKinds = VIDEO_FORMATS_NON_4KUHD);

		/**
			@param[in]	inKinds				Specifies the types of video formats returned. Defaults to non-4K/UHD formats.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the video format is incompatible with that device.
			@return		A string that can be printed to show the supported video formats.
		**/
		static std::string					GetVideoFormatStrings (const NTV2VideoFormatKinds inKinds = VIDEO_FORMATS_NON_4KUHD,
																	const std::string inDeviceSpecifier = std::string ());

		/**
			@brief	Returns the NTV2VideoFormat that matches the given string.
			@param[in]	inStr		Specifies the string to be converted to an NTV2VideoFormat.
			@param[in]	inKinds		Specifies which video format type is expected in "inStr", whether non-4K/UHD (the default),
									exclusively 4K/UHD, or both/all.
			@return		The given string converted to an NTV2VideoFormat, or NTV2_FORMAT_UNKNOWN if there's no match.
		**/
		static NTV2VideoFormat				GetVideoFormatFromString (const std::string & inStr,  const NTV2VideoFormatKinds inKinds = VIDEO_FORMATS_NON_4KUHD);

		/**
			@brief		Given a video format, if all 4 inputs are the same and promotable to 4K, this function does the promotion.
			@param		inOutVideoFormat	On entry, specifies the wire format;  on exit, receives the 4K video format.
			@return		True if successful;  otherwise false.
		**/
		static bool							Get4KInputFormat (NTV2VideoFormat & inOutVideoFormat);
	///@}

	/**
		@name	Pixel Format Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of pixel formats returned. Defaults to all formats.
			@return		The supported NTV2PixelFormatSet.
		**/
		static NTV2FrameBufferFormatSet		GetSupportedPixelFormats (const NTV2PixelFormatKinds inKinds = PIXEL_FORMATS_ALL);

		/**
			@param[in]	inKinds				Specifies the types of pixel formats returned. Defaults to all formats.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the pixel format is incompatible with that device.
			@return		A string that can be printed to show the available pixel formats (or those that are supported by a given device).
		**/
		static std::string					GetPixelFormatStrings (const NTV2PixelFormatKinds inKinds = PIXEL_FORMATS_ALL,
																	const std::string inDeviceSpecifier = std::string ());

		/**
			@brief	Returns the NTV2FrameBufferFormat that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an NTV2FrameBufferFormat.
			@return		The given string converted to an NTV2FrameBufferFormat, or NTV2_FBF_INVALID if there's no match.
		**/
		static NTV2FrameBufferFormat		GetPixelFormatFromString (const std::string & inStr);

		/**
			@return		The equivalent AJA_PixelFormat for the given NTV2FrameBufferFormat.
			@param[in]	inFormat	Specifies the NTV2FrameBufferFormat to be converted into an equivalent AJA_PixelFormat.
		**/
		static AJA_PixelFormat				GetAJAPixelFormat (const NTV2FrameBufferFormat inFormat);
	///@}

	/**
		@name	Input Source Functions
	**/
	///@{
		/**
			@param[in]	inKinds		Specifies the types of input sources returned. Defaults to all sources.
			@return		The supported NTV2InputSourceSet.
		**/
		static const NTV2InputSourceSet		GetSupportedInputSources (const NTV2InputSourceKinds inKinds = NTV2_INPUTSOURCES_ALL);

		/**
			@param[in]	inKinds				Specifies the types of input sources returned. Defaults to all sources.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the input source is incompatible with that device.
			@return		A string that can be printed to show the available input sources (or those that are supported by a given device).
		**/
		static std::string					GetInputSourceStrings (const NTV2InputSourceKinds inKinds = NTV2_INPUTSOURCES_ALL,
																	const std::string inDeviceSpecifier = std::string ());

		/**
			@brief		Returns the NTV2InputSource that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an NTV2InputSource.
			@return		The given string converted to an NTV2InputSource, or NTV2_INPUTSOURCE_INVALID if there's no match.
		**/
		static NTV2InputSource				GetInputSourceFromString (const std::string & inStr);
	///@}

	/**
		@name	Timecode Functions
	**/
	///@{
		/**
			@param[in]	inKinds				Specifies the types of timecode indexes returned. Defaults to all indexes.
			@return		The supported NTV2TCIndexes set.
		**/
		static const NTV2TCIndexes			GetSupportedTCIndexes (const NTV2TCIndexKinds inKinds);

		/**
			@param[in]	inKinds				Specifies the types of timecode indexes returned. Defaults to all indexes.
			@param[in]	inDeviceSpecifier	An optional device specifier. If non-empty, and resolves to a valid, connected AJA device,
											warns if the timecode index is incompatible with that device.
			@return		A string that can be printed to show the available timecode indexes (or those that are supported by a given device).
		**/
		static std::string					GetTCIndexStrings (const NTV2TCIndexKinds inKinds = TC_INDEXES_ALL,
																const std::string inDeviceSpecifier = std::string ());

		/**
			@brief		Returns the NTV2TCIndex that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an NTV2TCIndex.
			@return		The given string converted to an NTV2TCIndex, or NTV2_TCINDEX_INVALID if there's no match.
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
		**/
		static std::string					GetAudioSystemStrings (const std::string inDeviceSpecifier = std::string ());

		/**
			@brief	Returns the NTV2AudioSystem that matches the given string.
			@param[in]	inStr	Specifies the string to be converted to an NTV2AudioSystem.
			@return		The given string converted to an NTV2AudioSystem, or NTV2_AUDIOSYSTEM_INVALID if there's no match.
		**/
		static NTV2AudioSystem				GetAudioSystemFromString (const std::string & inStr);
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
	///@}

};	//	CNTV2DemoCommon


#endif	//	_NTV2DEMOCOMMON_H
