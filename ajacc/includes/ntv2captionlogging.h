/**
	@file		ntv2captionlogging.h
	@brief		Declares the NTV2CaptionLogMask, and the CNTV2CaptionLogConfig class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CAPTIONLOGGING_
#define __NTV2_CAPTIONLOGGING_

#include "ajatypes.h"
#include "ntv2publicinterface.h"
#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#else
	#include <stdint.h>
#endif
#include <string>
#include <iostream>
#include <set>

#if !defined(NULL)
	#define NULL 0
#endif


//	The 'ajacc' library can use whatever 'assert' facility is desired.
#define	AJACC_ASSERT	NTV2_ASSERT

//	For debugging...
#define	UHEX2(_c_)		HEX0N(uint16_t(_c_),2)
#define	HEX4(_uword_)	HEX0N((_uword_),4)


/**
	@brief	Selectors to control what information is logged.
**/
typedef uint64_t	NTV2CaptionLogMask;

const uint64_t	kCaptionLog_All					(0xFFFFFFFFFFFFFFFF);	///< @brief	Log everything possible
const uint64_t	kCaptionLog_Off					(0x0000000000000000);	///< @brief	Don't log anything
const uint64_t	kCaptionLog_Decode608			(0x0000000000000001);	///< @brief	Log decode (input) 608 events
const uint64_t	kCaptionLog_DecodeXDS			(0x0000000000000002);	///< @brief	Log decode (input) XDS info
const uint64_t	kCaptionLog_Line21DetectSuccess	(0x0000000000000100);	///< @brief	Log line 21 waveform detect successes
const uint64_t	kCaptionLog_Line21DetectFail	(0x0000000000000200);	///< @brief	Log line 21 waveform detect failures
const uint64_t	kCaptionLog_Line21DecodeSuccess	(0x0000000000000400);	///< @brief	Log line 21 waveform decode successes
const uint64_t	kCaptionLog_Line21DecodeFail	(0x0000000000000800);	///< @brief	Log line 21 waveform decode failures
const uint64_t	kCaptionLog_608ShowScreen		(0x0000000000008000);	///< @brief	Log screen for 608 channel of interest
const uint64_t	kCaptionLog_608ShowAllScreens	(0x0000000000010000);	///< @brief	Log screens for all 608 channels
const uint64_t	kCaptionLog_608ShowScreenAttrs	(0x0000000000020000);	///< @brief	Log screen attributes for 608 channel of interest
const uint64_t	kCaptionLog_DecodeVANC			(0x0000000000100000);	///< @brief	Log decode (input) VANC data
const uint64_t	kCaptionLog_DecodeCDP			(0x0000000000200000);	///< @brief	Log decode (input) CDP data

//	For debugging rows/columns-of-interest:
typedef std::set <int>						Line21RowSet;				///<	A set of caption row numbers
typedef Line21RowSet						Line21ColumnSet;			///<	A set of caption column numbers
typedef Line21RowSet::const_iterator		Line21RowSetConstIter;		///<	A const iterator for a Line21RowSet
typedef Line21RowSetConstIter				Line21ColumnSetConstIter;	///<	A const iterator for a Line21ColumnSet

AJAExport std::string Line21RowSetToString (const Line21RowSet & inRowSet);	//	std::ostream & operator << (std::ostream & inOutStream, const Line21RowSet & inData);
#define	Line21ColumnSetToString				Line21RowSetToString

/**
	@brief	Sets the default log mask that will be used by newly-created objects in the caption library.
	@param[in]	inMask	A non-constant reference to an output stream that will be used in newly-created
						objects in the caption library.
**/
AJAExport void SetDefaultCaptionLogMask (const NTV2CaptionLogMask inMask);

/**
	@brief	Answers with the default log mask used when creating new objects in the caption library.
	@return	The default log mask used for newly-created objects in the caption library.
**/
AJAExport NTV2CaptionLogMask GetDefaultCaptionLogMask (void);


class AJAExport CNTV2CaptionLogConfig
{
	//	CLASS METHODS
	public:
		/**
			@brief	Dumps a contiguous chunk of memory in hex, octal, decimal, with or without ascii, to the given output stream.
			@param	pInStartAddress		A valid, non-NULL pointer to the start of the host memory to be dumped.
			@param	inByteCount			The number of bytes to be dumped.
			@param	inOutputStream		Output stream that will receive the dump.
			@param	inRadix				16=hex, 10=decimal, 8=octal, 2=binary -- all others disallowed.
			@param	inBytesPerGroup		Number of bytes to dump per contiguous group of numbers.
			@param	inGroupsPerLine		Number of contiguous groups of numbers to dump per output line.
										If zero, no grouping is done, and address & ASCII display is suppressed.
			@param	inAddressRadix		0=omit, 2=binary, 8=octal, 10=decimal, 16=hex -- all others disallowed.
			@param	inShowAscii			True = also dump ASCII characters; false = don't dump ASCII characters.
										Overridden to false if inGroupsPerLine is zero.
			@param	inAddrOffset		Specifies a value to be added to the addresses that appear in the dump.
										Ignored if inGroupsPerLine is zero.
			@return	A non-constant reference to the output stream that received the dump.
		**/
		static std::ostream & DumpMemory (const void *	pInStartAddress,
										const size_t	inByteCount,
										std::ostream &	inOutputStream	= std::cout,
										const size_t	inRadix			= 16,
										const size_t	inBytesPerGroup	= 4,
										const size_t	inGroupsPerLine	= 8,
										const size_t	inAddressRadix	= 16,
										const bool		inShowAscii		= true,
										const size_t	inAddrOffset	= 0);

		/**
			@return	A string containing a hex dump of up to 32 bytes from the given buffer.
			@param	pInStartAddress		A valid, non-NULL pointer to the start of the host memory to be dumped.
			@param	inByteCount			The maximum number of bytes from the buffer to be dumped.
			@param	inLimitBytes		The maximum number of bytes to be dumped. Defaults to 32.
		**/
		static std::string	HexDump32Bytes (const void * pInStartAddress, const size_t inByteCount, const size_t inLimitBytes = 32);

		/**
			@brief	Dumps the luma values in hexadecimal from the given line of '2vuy' video to the given output stream.
			@param	pInVideoLine		A valid, non-NULL pointer to the first byte of the line of '2vuy' video in host memory.
										It is assumed this is SD video, and the line width is at least 720 pixels.
			@param	inOutputStream		Specifies the output stream that will receive the dump.
			@param	inFromPixel			Specifies starting pixel, a zero-based index.
										Defaults to the first pixel in the line.
			@param	inToPixel			Specifies the last pixel to appear in the dump, a zero-based index.
										Defaults to the last pixel in the line (assuming 720 pixel SD).
			@param	inShowRuler			If true, precedes the dump with a "ruler" to indicate pixel locations.
										Defaults to true.
			@param	inHiliteRangeFrom	Specifies the starting pixel to hilight, a zero-based pixel index.
										Defaults to 9999, which indicates no highlighting should appear in the dump.
										If this value matches inHiliteRangeTo, only a single pixel will be highlighted.
			@param	inHiliteRangeTo		Specifies the last pixel to hilight, a zero-based pixel index.
										Defaults to 9999, which indicates no highlighting should appear in the dump.
										If this value matches inHiliteRangeFrom, only a single pixel will be highlighted.
			@return	A non-constant reference to the output stream that received the dump.
		**/
		static std::ostream & DumpYBytes_2vuy (const UByte *	pInVideoLine,
												std::ostream &	inOutputStream,
												const unsigned	inFromPixel			= 0,
												const unsigned	inToPixel			= 719,
												const bool		inShowRuler			= true,
												const unsigned	inHiliteRangeFrom	= 9999,
												const unsigned	inHiliteRangeTo		= 9999);

		/**
			@brief	Dumps the luma values in hexadecimal from the given line of '2vuy' video to the given output stream.
			@param	inVideoLine			A vector of 8-bit values comprising the '2vuy' video line.
										It is assumed this is SD video, and the line width is at least 720 pixels (1440 elements).
			@param	inOutputStream		Specifies the output stream that will receive the dump.
			@param	inFromPixel			Specifies starting pixel, a zero-based index.
										Defaults to the first pixel in the line.
			@param	inToPixel			Specifies the last pixel to appear in the dump, a zero-based index.
										Defaults to the last pixel in the line (assuming 720 pixel SD).
			@param	inShowRuler			If true, precedes the dump with a "ruler" to indicate pixel locations.
										Defaults to true.
			@param	inHiliteRangeFrom	Specifies the starting pixel to hilight, a zero-based pixel index.
										Defaults to 9999, which indicates no highlighting should appear in the dump.
										If this value matches inHiliteRangeTo, only a single pixel will be highlighted.
			@param	inHiliteRangeTo		Specifies the last pixel to hilight, a zero-based pixel index.
										Defaults to 9999, which indicates no highlighting should appear in the dump.
										If this value matches inHiliteRangeFrom, only a single pixel will be highlighted.
			@return	A non-constant reference to the output stream that received the dump.
		**/
		static std::ostream & DumpYBytes_2vuy (const std::vector<uint8_t> & inVideoLine,
												std::ostream &	inOutputStream,
												const size_t	inFromPixel			= 0,
												const size_t	inToPixel			= 719,
												const bool		inShowRuler			= true,
												const size_t	inHiliteRangeFrom	= 9999,
												const size_t	inHiliteRangeTo		= 9999);

		static std::string	GetSeverityLabel(const unsigned inSeverity);


	//	INSTANCE METHODS
	public:
		/**
			@brief		Default constructor.
			@param[in]	inLogLabel	Optionally specifies a label for this instance's log output.
		**/
											CNTV2CaptionLogConfig (const std::string inLogLabel = std::string ());

		virtual 							~CNTV2CaptionLogConfig ();

		//	Debug Output Control
		/**
			@brief		Specifies what, if any, debug information I will write to my log stream.
			@param[in]	inLogMask	A bit mask that specifies what information will be logged.
		**/
		virtual inline NTV2CaptionLogMask	SetLogMask (const NTV2CaptionLogMask inLogMask)				{const NTV2CaptionLogMask tmp (mLogMask);  mLogMask = inLogMask;  return tmp;}

		/**
			@brief		Answers with my current caption logging bit mask.
			@return		My current caption logging mask.
		**/
		virtual inline NTV2CaptionLogMask	GetLogMask (void) const										{return mLogMask;}

		/**
			@brief		Answers true if the given log mask bits are set in my current log mask.
			@param[in]	inLogMask	Specifies the log mask bits of interest.
			@param[in]	inExact		If true, the log mask must match exactly;  otherwise any matching
									bit will constitute a match. Defaults to false (any matching bit).
			@return		True if all given log mask bits are set;  otherwise false.
		**/
		virtual inline bool					TestLogMask (const NTV2CaptionLogMask inLogMask, const bool inExact = false) const		{if (inExact) return (GetLogMask() & inLogMask) == inLogMask; else return (GetLogMask() & inLogMask) ? true : false;}

		/**
			@brief		Specifies my logging label.
			@param[in]	inNewLabel	Specifies my new logging label.
		**/
		virtual void						SetLogLabel (const std::string & inNewLabel);

		/**
			@brief		Appends the given string to my current log label.
			@param[in]	inString	Specifies the string to append to my current log label.
		**/
		virtual void						AppendToLogLabel (const std::string & inString);

		/**
			@brief		Answers with my current logging label.
			@return		My current logging label.
		**/
		virtual const std::string &			GetLogLabel (void) const;


		//	OBSOLETE FUNCTIONS
		virtual void						SetLogStream (std::ostream & inOutputStream);		///< @deprecated	Obsolete -- now uses the AJALogger
		virtual std::ostream &				LogIf (const NTV2CaptionLogMask inLogMask) const;	///< @deprecated	Obsolete -- now uses the AJALogger
		virtual std::ostream &				Log (void) const;									///< @deprecated	Obsolete -- now uses the AJALogger


	//	INSTANCE DATA
	protected:		
		NTV2CaptionLogMask		mLogMask;		///< @brief	Determines what messages are logged
		std::string				mLogLabel;		///< @brief	My debug label
		mutable void *			mpLabelLock;	///< @brief	Protects my debug label from simultaneous access by more than one thread

};	//	CNTV2CaptionLogConfig


//	OBSOLETE
AJAExport void SetDefaultCaptionLogOutputStream (std::ostream & inOutputStream);	///< @deprecated	Obsolete -- now uses the AJALogger
AJAExport std::ostream & GetDefaultCaptionLogOutputStream (void);					///< @deprecated	Obsolete -- now uses the AJALogger

#endif	// __NTV2_CAPTIONLOGGING_
