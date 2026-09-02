/**
	@file		ntv2smpteancdata.h
	@brief		Declares the CNTV2SMPTEAncData class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_SMPTEANCDATA_
#define __NTV2_SMPTEANCDATA_

#include "ntv2utils.h"
#include "ntv2formatdescriptor.h"
#include <vector>
#include <iostream>
#ifdef MSWindows
	#include "windows.h"
#endif


// Some popular ancillary packet DID/SDID's (sans parity)
const UByte NTV2_SMPTEAncRP334DID	= 0x61;		// SMPTE 334 (Closed Captioning)
const UByte NTV2_SMPTEAncRP334SDID	= 0x01;
const UWord NTV2_WildCardDID		= 0x2FF;	// Wild Card - Match any Data ID
const UWord NTV2_WildCardSDID		= 0x2FF;	// Wild Card - Match any Secondary Data ID


// SMPTE 291 Ancillary Packet data structures
typedef struct NTV2_SMPTEAncHeader
{
	UWord	ancDataFlag0;	// 0x000
	UWord	ancDataFlag1;	// 0x3FF
	UWord	ancDataFlag2;	// 0x3FF
	UWord	ancDataID;		// DID
	UWord	ancSecDID;		// SDID (or Data Block Number)
	UWord	ancDataCount;	// DC

	/**
		@brief	Constructs me from the given DID, SDID and DC values.
		@param[in]	inDID			Specifies my DID.
		@param[in]	inSDID			Specifies my SDID.
		@param[in]	inDataCount		Specifies my data count (DC).
	**/
	inline NTV2_SMPTEAncHeader (const UWord inDID = 0, const UWord inSDID = 0, const UWord inDataCount = 0)
		:	ancDataFlag0	(0x000),
			ancDataFlag1	(0x3FF),
			ancDataFlag2	(0x3FF),
			ancDataID		(inDID),
			ancSecDID		(inSDID),
			ancDataCount	(inDataCount)
	{
	}

} NTV2_SMPTEAncHeader, *NTV2_SMPTEAncHeaderPtr;


const UWord kAncHeaderSize	= sizeof (NTV2_SMPTEAncHeader) / sizeof (UWord);


typedef struct NTV2_SMPTEAncFooter
{
	UWord	ancChecksum;	// anc packet checksum

} NTV2_SMPTEAncFooter, * NTV2_SMPTEAncFooterPtr;


const UWord		kAncFooterSize		= sizeof (NTV2_SMPTEAncFooter) / sizeof (UWord);
const ULWord	kMaxAncPacketSize	= kAncHeaderSize + 255 + kAncFooterSize;		// max TOTAL anc packet size (header + payload + footer)


//	Use this enum to specify which channel (luma or chroma) to look for ANC in...
typedef enum NTV2_SMPTEAncChannelSelect
{
	kNTV2SMPTEAncChannel_Y,		///	Only look in luma samples
	kNTV2SMPTEAncChannel_C,		///	Only look in chroma samples
	kNTV2SMPTEAncChannel_Both	///	Look both luma and chroma samples

} NTV2_SMPTEAncChannelSelect;


std::string NTV2SMPTEAncChannelSelectToString (const NTV2_SMPTEAncChannelSelect inChanSelect, const bool inCompactForm = true);


typedef const UByte *	UByteConstPtr;

/**
	@brief	A UWordVANCPacket is identical to a UWordSequence.
**/
typedef	UWordSequence						UWordVANCPacket;

/**
	@brief	An ordered sequence of zero or more UWordVANCPacket elements.
**/
typedef std::vector <UWordVANCPacket>		UWordVANCPacketList;
typedef	UWordVANCPacketList::const_iterator	UWordVANCPacketListConstIter;
typedef	UWordVANCPacketList::iterator		UWordVANCPacketListIter;

AJAExport std::ostream & operator << (std::ostream & inOutStream, const UWordVANCPacketList & inData);


class AJAExport CNTV2SMPTEAncData
{
	//	Class Methods
	public:
		static	UWord	AddEvenParity (UByte dataByte);

		/**
			@brief		Converts a single line of NTV2_FBF_8BIT_YCBCR data from the given source buffer into an ordered sequence of UWords
						that contain the resulting 10-bit even-parity data.
			@param[in]	pInYUV8Line			A valid, non-NULL pointer to the start of the VANC line in an NTV2_FBF_8BIT_YCBCR video buffer.
			@param[out]	out16BitYUVLine		Receives the converted 10-bit-per-component values as an ordered sequence of UWord values,
											which will include even parity and valid checksums.
			@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
			@return		True if successful;  otherwise false.
			@note		If SMPTE ancillary data is detected in the video, this routine "intelligently" stretches it by copying the 8-bits to
						the LS 8-bits of the 10-bit output, recalculating parity and checksums as needed. (This emulates what NTV2 device
						firmware does during playout of NTV2_FBF_8BIT_YCBCR frame buffers with NTV2_VANCDATA_8BITSHIFT_ENABLE.)
		**/
		static bool		UnpackLine_8BitYUVtoUWordSequence (const void * pInYUV8Line, UWordSequence & out16BitYUVLine, const ULWord inNumPixels);

		/**
			@brief	Searches for ancillary data in a given host frame buffer.
			@param[in]	inAncDID			Specifies the Ancillary data ID to look for in the frame buffer. (The proper parity will
											automatically be added to 8-bit DID values.)
			@param[in]	inAncSDID			Specifies the Ancillary SDID to look for in the frame buffer. (The proper parity will
											automatically be added to 8-bit SDID values.)
			@param[in]	inFrameBuffer		Specifies the host frame buffer to be searched.
			@param[in]	inFormatDesc		Describes the video/pixel format of the host frame buffer.
			@param[in]	inAncChannel		Specifies the ancillary data channel to search, whether luma (Y), chroma (C), or both.
			@param[out]	outWords			Receives the 10-bit packet data (including parity), replacing its previous contents.
			@param[out]	outHasParityErrors	Receives "true" if any parity errors are found, or "false" if none found.
			@param[in]	inLineIncrement		Specifies the line increment to use. Use 1 to scan every VANC line in the buffer.
			@param		inOutLineStart		Upon entry, specifies the zero-based starting line offset at which to start the search.
											Upon return, contains the ending line offset where the search ended.
			@param		inOutPixelStart		Upon entry, specifies the initial zero-based pixel offset from which to start the search.
											Upon exit, contains the ending zero-based pixel offset where the data search ended.
											Ignored if inOutLineStart is non-zero.
			@note		The specified starting pixel number is used only when searching the first line. If the desired Anc data is not
						found in the first line, subsequent line searching will commence with the first pixel.
			@note		To find the next matching ancillary data packet, the caller must increment the returned line and/or pixel
						offsets; otherwise, this function will keep returning the same values.
			@return		True if successful; otherwise false.
		**/
		static	bool	FindAnc (const UWord						inAncDID,
								const UWord							inAncSDID,
								const NTV2Buffer & 					inFrameBuffer,
								const NTV2FormatDescriptor &		inFormatDesc,
								const NTV2_SMPTEAncChannelSelect	inAncChannel,
								UWordSequence &						outWords,
								bool &								outHasParityErrors,
								const UWord							inLineIncrement,
								UWord &								inOutLineStart,
								UWord &								inOutPixelStart);

		/**
			@brief	Searches for ancillary data in a given host frame buffer.
			@param[in]	inAncDID			Specifies the Ancillary data ID to look for in the frame buffer. (The proper parity will
											automatically be added to 8-bit DID values.)
			@param[in]	inAncSDID			Specifies the Ancillary SDID to look for in the frame buffer. (The proper parity will
											automatically be added to 8-bit SDID values.)
			@param[in]	pInFrameBuffer		Specifies a valid, non-NULL starting address of the host frame buffer to be searched.
			@param[in]	inAncChannel		Specifies the ancillary data channel to search, whether in the luma (Y), the chroma (C), or both.
			@param[in]	inVideoFormat		Specifies the video format of the host frame buffer.
			@param[in]	inFBFormat			Specifies the pixel format of the host frame buffer. Must be NTV2_FBF_8BIT_YCBCR or NTV2_FBF_10BIT_YCBCR.
			@param[out]	pOutBuff			Specifies a valid, non-NULL address of the output buffer that is to receive the packet data.
											This will be 10-bit data, and will include parity.
			@param[out]	outWordCount		Specifies the number of ancillary data words written into the output buffer.
			@param[in]	inWordCountMax		Specifies the capacity, in UWords, of the output buffer.
			@param[out]	outHasParityErrors	Specifies a boolean variable that is to receive "true" if any parity errors are found,
											or "false" if none are found.
			@param[in]	inLineIncrement		Specifies the line increment to use. Use 1 to scan every line in the buffer.
			@param		inOutLineStart		Upon entry, specifies the zero-based starting line number at which to start the search.
											Upon return, contains the ending line number where the search ended.
			@param		inOutPixelStart		Upon entry, specifies the initial zero-based pixel number from which to start the search.
											Upon exit, contains the ending zero-based pixel number where the data search ended.
											Ignored if inOutLineStart is non-zero.
			@note		The specified starting pixel number is used only when searching the first line. If the desired Anc data is not
						found in the first line, subsequent line searching will commence with the first pixel.
			@note		To find the next matching ancillary data, the caller must increment the returned line and/or pixel start values.
						Otherwise, this function will keep returning the same values.
			@return		True if successful; otherwise false.
		**/
		static	bool	FindAnc (const UWord						inAncDID,
								const UWord							inAncSDID,
								const ULWord *						pInFrameBuffer,
								const NTV2_SMPTEAncChannelSelect	inAncChannel,
								const NTV2VideoFormat				inVideoFormat,
								const NTV2FrameBufferFormat			inFBFormat,
								UWord *								pOutBuff,
								ULWord &							outWordCount,
								const ULWord						inWordCountMax,
								bool &								outHasParityErrors,
								const UWord							inLineIncrement,
								UWord &								inOutLineStart,
								UWord &								inOutPixelStart);

		/**
			@brief	Searches for ancillary data in a given host frame buffer.
			@param[in]	inAncDID			Specifies the Ancillary data ID to look for in the frame buffer. The proper parity will
											automatically be added to 8-bit DID values.
			@param[in]	inAncSDID			Specifies the Ancillary SDID to look for in the frame buffer. The proper parity will
											automatically be added to 8-bit SDID values.
			@param[in]	pInFrameBuffer		Specifies a valid, non-NULL starting address of the host frame buffer to be searched.
			@param[in]	inAncChannel		Specifies the ancillary data channel.
			@param[in]	inVideoFormat		Specifies the video format of the host frame buffer.
			@param[in]	inFBFormat			Specifies the pixel format of the host frame buffer.
			@param[out]	pOutBuff			Specifies a valid, non-NULL address of the output buffer.
			@param[out]	outWordCount		Specifies the number of ancillary data words written into the output buffer.
			@param[in]	inWordCountMax		Specifies the capacity, in UWords, of the output buffer.
			@param[out]	outHasParityErrors	Specifies a boolean variable that is to receive true if any parity errors are found,
											or false if none are found.
			@return		True if successful; otherwise false.
			@note		In this release, only 8-bit YCbCr and 10-bit YCbCr frame buffer formats are allowed.
		**/
		static	bool	FindAnc (const UWord						inAncDID,
								const UWord							inAncSDID,
								const ULWord *						pInFrameBuffer,
								const NTV2_SMPTEAncChannelSelect	inAncChannel,
								const NTV2VideoFormat				inVideoFormat,
								const NTV2FrameBufferFormat			inFBFormat,
								UWord *								pOutBuff,
								ULWord &							outWordCount,
								const ULWord						inWordCountMax,
								bool &								outHasParityErrors);

		/**
			@brief	Searches for ancillary data in a given host frame buffer.
			@param[in]	inAncDID			Specifies the Ancillary data ID to look for in the frame buffer. The proper parity will
											automatically be added to 8-bit DID values.
			@param[in]	inAncSDID			Specifies the Ancillary SDID to look for in the frame buffer. The proper parity will
											automatically be added to 8-bit SDID values.
			@param[in]	pInFrameBuffer		Specifies a valid, non-NULL starting address of the host frame buffer to be searched.
			@param[in]	inAncChannel		Specifies the ancillary data channel.
			@param[in]	inVideoFormat		Specifies the video format of the host frame buffer.
			@param[in]	inFBFormat			Specifies the pixel format of the host frame buffer.
			@param[out]	pOutBuff			Specifies a valid, non-NULL address of the output buffer.
			@param[out]	outWordCount		Specifies the number of ancillary data words written into the output buffer.
			@param[in]	inWordCountMax		Specifies the capacity, in UWords, of the output buffer.
			@return		True if successful; otherwise false.
		**/
		static	bool	FindAnc (const UWord						inAncDID,
								const UWord							inAncSDID,
								const ULWord *						pInFrameBuffer,
								const NTV2_SMPTEAncChannelSelect	inAncChannel,
								const NTV2VideoFormat				inVideoFormat,
								const NTV2FrameBufferFormat			inFBFormat,
								UWord *								pOutBuff,
								ULWord &							outWordCount,
								const ULWord						inWordCountMax);

		static	bool	MakeAncHeader (NTV2_SMPTEAncHeaderPtr pHdr, UByte ancDID, UByte ancSDID, UByte ancDC);
		static	bool	SetAncHeaderDataCount (NTV2_SMPTEAncHeaderPtr pHdr, UByte ancDC);

		static	bool	SetAncFooterChecksum (NTV2_SMPTEAncFooterPtr pFtr, UWord checksum);
		static	bool	CalculateAncChecksum (NTV2_SMPTEAncHeaderPtr pHdr, UByte dataCount = 0);

		static	bool	ExtractCompressedAnc (const void *				pFrameBuffer,
												void *					pAncBuff,
												ULWord					ancBufMax,
												ULWord &				inOutFoundSize,
												NTV2VideoFormat			videoFormat,
												NTV2FrameBufferFormat	fbFormat);

		static	bool	EmbedCompressedAnc (const void *				pAncBuff,
											void *						pFrameBuffer,
											const ULWord				ancBufSize,
											const NTV2VideoFormat		videoFormat,
											const NTV2FrameBufferFormat	fbFormat);

		/**
			@brief	Inserts ancillary data at a given line and channel offset into the given host frame buffer.
			@param[in]	pInAncBuff		Specifies a valid, non-NULL starting address of the buffer that contains the ancillary
										data to be inserted into the host frame buffer.
			@param[in]	inAncWordCount	Specifies the number of two-byte words of ancillary data in the pAncBuff to be inserted
										into the host frame buffer.
			@param[in]	inLineOffset	Specifies the line offset into the host frame buffer into which the ancillary data will
										be inserted.
			@param[in]	inWordOffset	Specifies the word offset in the host frame buffer into which the ancillary data will
										be inserted. This effectively determines the anc channel in which to embed the data
										(e.g., 1 uses the Y/luma channel).
			@param[in]	pFrameBuffer	Specifies a valid, non-NULL starting address of the host frame buffer that is to have
										the ancillary data inserted into it.
			@param[in]	inVideoFormat	Specifies the video format.
			@param[in]	inFBFormat		Specifies the pixel format of the host frame buffer.
			@return		True if successful; otherwise false.
			@bug	This function doesn't work for SD video formats, since SD packets use both luma and chroma channels.
		**/
		static	bool	InsertAnc (const UWord *				pInAncBuff,
									const size_t				inAncWordCount,
									const ULWord				inLineOffset,
									const ULWord				inWordOffset,
									ULWord *					pFrameBuffer,
									const NTV2VideoFormat		inVideoFormat,
									const NTV2FrameBufferFormat	inFBFormat);

		/**
			@brief	Inserts ancillary data at a given line and channel offset into the given host frame buffer.
			@param[in]	pInAncBuff		Specifies a valid, non-NULL starting address of the buffer that contains the ancillary
										data to be inserted into the host frame buffer.
			@param[in]	inAncWordCount	Specifies the number of two-byte words of ancillary data in the pAncBuff to be inserted
										into the host frame buffer.
			@param[in]	inSMPTELineNum	Specifies the SMPTE line number into which the ancillary data will be inserted in the
										host frame buffer.
			@param[in]	inWordOffset	Specifies the word offset in the host frame buffer into which the ancillary data will
										be inserted. This effectively determines the anc channel in which to embed the data
										(e.g., 1 uses the Y/luma channel).
			@param[in]	pFrameBuffer	Specifies a valid, non-NULL starting address of the host frame buffer that is to have
										the ancillary data inserted into it.
			@param[in]	inVideoFormat	Specifies the video format.
			@param[in]	inFBFormat		Specifies the pixel format of the host frame buffer.
			@return		True if successful; otherwise false.
			@bug	This function doesn't work for SD video formats, since SD packets use both luma and chroma channels.
		**/
		static	bool	InsertAncAtSmpteLine (const UWord *					pInAncBuff,
												const ULWord				inAncWordCount,
												const ULWord				inSMPTELineNum,
												const ULWord				inWordOffset,
												ULWord *					pFrameBuffer,
												const NTV2VideoFormat		inVideoFormat,
												const NTV2FrameBufferFormat	inFBFormat);

		static	bool	FirstVancLineAndChannel (const NTV2FormatDescriptor &	inFD,
												const NTV2SmpteLineNumber &		inLN,
												NTV2_SMPTEAncChannelSelect &	inOutChannel,
												ULWord &						inOutSMPTELine,
												ULWord &						outLineOffset);

		static	bool	NextVancLineAndChannel (const NTV2FormatDescriptor &	inFD,
												const NTV2SmpteLineNumber &		inLN,
												NTV2_SMPTEAncChannelSelect &	inOutChannel,
												ULWord &						inOutSMPTELine,
												ULWord &						outLineOffset);

		static	ULWord	GetVancLineOffset (const NTV2FormatDescriptor &	inFormatDesc,
											const NTV2SmpteLineNumber &	inSmpteLineNumbers,
											const ULWord				inSmpteLine);

		static	bool	CompressAncPacket (const UWord *		packetBuffer,
											UByte *		targetBuffer,
											ULWord		maxTargetSize,
											ULWord &	outCompPacketSize,
											NTV2_SMPTEAncChannelSelect	chan,
											UWord		smpteLine);

		static	void	DecompressAncPacket (const UByte *					pInCompBuffer,
											UWord *							pOutUnpackedBuffer,
											ULWord &						outCompPacketSize,
											bool &							outIsValidLoc,
											NTV2_SMPTEAncChannelSelect &	outChan,
											ULWord &						outSmpteLine);

		static	bool	FindCompressedAnc (UByte			ancDID,
											UByte			ancSDID,
											const UByte *	pSrcAncBuf,
											const ULWord	srcAncSize,
											UByteConstPtr &	outBuffPtr,
											ULWord &		outBufSize);

		/**
			@brief		Extracts whatever VANC packets are found inside the given 16-bit YUV line buffer.
			@param[in]	inYUV16Line			Specifies the UWord sequence containing the 10-bit YUV VANC line data components.
											(Use ::UnpackLine_10BitYUVtoUWordSequence to convert a VANC line from an NTV2_FBF_10BIT_YCBCR
											frame buffer into this format. Use CNTV2SMPTEAncData::UnpackLine_8BitYUVtoUWordSequence) to
											convert a VANC line from an NTV2_FBF_8BIT_YCBCR frame buffer into this format.)
			@param[in]	inChanSelect		Specifies the ancillary data channel to search, whether in the luma (kNTV2SMPTEAncChannel_Y)
											or chroma (kNTV2SMPTEAncChannel_C). Use kNTV2SMPTEAncChannel_Both for SD video.
			@param[out]	outRawPackets		Receives the UWordVancPacketList, which will have one UWordSequence per extracted packet.
											Each UWordSequence in the returned list will start with the 0x000/0x3FF/0x3FF/DID/SDID/DC
											sequence, followed by each 10-bit packet data word, and ending with the checksum word.
			@param[out]	outWordOffsets		Receives the horizontal word offsets into the line, one for each packet found.
											This should have the same number of elements as "outRawPackets".
											These offsets can also be used to discern which channel each packet originated in (Y or C).
			@return		True if successful;  false if failed.
			@note		This function will not finish parsing the line once a parity, checksum, or overrun error is discovered in the line.
		**/
		static bool		GetAncPacketsFromVANCLine (const UWordSequence &				inYUV16Line,
													const NTV2_SMPTEAncChannelSelect	inChanSelect,
													UWordVANCPacketList &				outRawPackets,
													UWordSequence &						outWordOffsets);
		/**
			@brief		Answers with the SMPTE raster line number into which NTSC caption CDPs should be inserted.
			@param[in]	inVideoFormat	Specifies the video format.
			@param[in]	inIsField1		Specify true for Field 1;  otherwise false for Field 2.
			@return		The SMPTE raster line number;  or zero upon failure.
		**/
		static ULWord	GetCaptionAncLineNumber (const NTV2VideoFormat inVideoFormat, const bool inIsField1 = true);
};	//	CNTV2SMPTEAncData

#endif	// __NTV2_SMPTEANCDATA_
