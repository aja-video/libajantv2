/**
	@file		ntv2line21captioner.h
	@brief		Declares the CNTV2Line21Captioner class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_LINE21_CAPTIONER_
#define __NTV2_LINE21_CAPTIONER_

#include "ntv2captionlogging.h"
#include "ntv2caption608types.h"
#include <vector>

#if !defined(NULL)
	#define NULL 0
#endif

/**
	@brief	Instances of me can encode two ASCII characters into a "line 21" closed-captioning waveform.
			Conversely, my CNTV2Line21Captioner::DecodeLine class method can decode a "line 21" waveform to the two characters it contains.

			In either use case, I assume a 720-pixel line (e.g. Standard Definition "NTSC"), and the closed
			captioning is represented by an EIA-608 waveform. I do not handle other forms of closed captioning
			transmission (e.g., EIA-708, digital ancillary packets, etc.).

	@note	This implementation only handles conversion to/from 8-bit uncompressed ('2vuy') video.

	@note	This implementation makes a simplifying assumption that all captioning bits are 27 pixels wide.
			The actual bit duration should be (H / 32), which for NTSC video is 858 pixels / 32 = 26.8125 pixels per bit.
			This difference should be within the tolerance of most captioning receivers.
			(In PAL, the line width is 864 pixels, which is exactly 27.0 pixels per bit.)
**/

class AJAExport CNTV2Line21Captioner : public CNTV2CaptionLogConfig
{
	//	Class Methods
	public:
		/**
			@brief		Decodes the supplied line of 8-bit uncompressed ('2vuy') data and, if successful, returns the
						two 8-bit characters.
			@param[in]	pLineData	Specifies a valid, non-NULL pointer to the first byte in a frame buffer that contains
									the "line 21" waveform data to be decoded.
			@param[out]	outChar1	Receives the first caption data byte decoded from the line.
			@param[out]	outChar2	Receives the second caption data byte decoded from the line.
			@return		True if the given line appears to contain valid caption data;  otherwise, false.
			@note		Character analysis, including parity checks, are the caller's responsibility. However, this
						method WILL make a reasonable attempt to discover whether captioning data is present or not.
		**/
		static bool						DecodeLine (const UByte * pLineData, UByte & outChar1, UByte & outChar2);

		/**
			@brief		Decodes the supplied line of 8-bit uncompressed ('2vuy') data and, if successful, returns the
						8-bit decoded data.
			@param[in]	inLineData	A line of 8-bit uncompressed '2vuy' data that contains the "line 21" waveform data to be decoded.
			@param[out]	outData		Receives the data bytes (including parity) decoded from the line.
			@return		True if the given line appears to contain valid data;  otherwise, false.
			@note		Character analysis, including parity checks, are the caller's responsibility. However, this
						method WILL make a reasonable attempt to discover whether captioning data is present or not.
		**/
		static bool						DecodeLine (const std::vector<uint8_t> & inLineData, std::vector<uint8_t> & outData);

		/**
			@brief		Searches for a valid captioning clock run-in in the given 8-bit uncompressed ('2vuy') line.
			@param[in]	pInVideoLine	A valid pointer to a buffer containing the line of 8-bit '2vuy' video to be searched.
										Must be at least 1440 bytes in length.
			@return		NULL if unsuccessful; otherwise a pointer to the middle of the first data bit
						(i.e. the one following the last '1' start bit).
		**/
		static const UByte *			FindFirstDataBit_NTSC (const void * pInVideoLine);

		/**
			@brief		Searches for a valid captioning clock run-in in the given 8-bit uncompressed ('2vuy') line.
			@param[in]	in2VUYLine		A buffer containing the line of 8-bit '2vuy' video to be searched.
			@return		in2VUYLine.max_size() if unsuccessful; otherwise an index/offset to the middle of the first data bit
						(i.e. the one following the last '1' start bit).
		**/
		static std::vector<uint8_t>::size_type	FindFirstDataBit_NTSC (const std::vector<uint8_t> & in2VUYLine);


	//	Class Data
	public:
		static const UWord				CC_LINE_WIDTH_PIXELS		=	720;						///	Standard-Definition only -- assume fixed line width of 720 pixels
		static const UWord				ENCODE_LINE_LENGTH_BYTES	=	CC_LINE_WIDTH_PIXELS * 2;	///	720 pixels x 2 bytes per pixel


	//	Instance Methods
	public:
		/**
			@brief		My default constructor.
		**/
										CNTV2Line21Captioner ();

		/**
			@brief		My default destructor.
		**/
		virtual							~CNTV2Line21Captioner ();

		/**
			@brief		Encodes the two given characters as an EIA-608-compliant "line 21" waveform into my private line
						data buffer. Client applications will probably want to copy this waveform data into their own host
						frame buffer for eventual transmission to an SDI output. Returns the starting address of my private
						line buffer.
			@param[in]	inByte1		The first character to be encoded. The data byte MUST include parity.
			@param[in]	inByte2		The second character to be encoded. The data byte MUST include parity.
			@return		A pointer to the first byte of my private buffer, which contains the encoded "line 21" waveform.
			@note		My private line data buffer has a single line containing 720 pixels of '2vuy' (8-bit YCbCr) SD video data.
		**/
		virtual UByte *					EncodeLine (const UByte inByte1, const UByte inByte2);

	private:
		virtual void					InitEncodeBuffer (void);
		virtual UByte *					EncodeCharacter (UByte * pBuffer, const UByte inByte);

	//	Instance Data
	private:
		UByte	mEncodeBuffer [CC_LINE_WIDTH_PIXELS * 2];	///< @brief	My local encode buffer which holds a single line containing
															///<		720 pixels of '2vuy' (8-bit YCbCr) SD video data
		bool	mEncodeBufferInitialized;					///< @brief	True if my local encode buffer is initialized
		ULWord	mEncodePixelOffset;							///< @brief	Pixel offset into my line buffer where encoding starts
		ULWord	mEncodeFirstDataBitOffset;					///< @brief	First data bit cell starting offset, excluding rise-time slop

};	//	CNTV2Line21Captioner

#endif	//	__NTV2_LINE21_CAPTIONER_
