/**
	@file		ntv2smpteancdata.cpp
	@brief		Implementation of the CNTV2SMPTEAncData class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include "ntv2captionlogging.h"
#include "ntv2smpteancdata.h"
#include "ntv2utils.h"
#include "ajabase/system/debug.h"
#include <iostream>
#include <iomanip>
#if defined (MSWindows)
	#include "ntv2debug.h"
#endif

using namespace std;


#define LOGMYERROR(__x__)	AJA_sREPORT(AJA_DebugUnit_SMPTEAnc, AJA_DebugSeverity_Error,	AJAFUNC << ":	" << __x__)
#define LOGMYWARN(__x__)	AJA_sREPORT(AJA_DebugUnit_SMPTEAnc, AJA_DebugSeverity_Warning,	AJAFUNC << ":	" << __x__)
#define LOGMYNOTE(__x__)	AJA_sREPORT(AJA_DebugUnit_SMPTEAnc, AJA_DebugSeverity_Notice,	AJAFUNC << ":	" << __x__)
#define LOGMYINFO(__x__)	AJA_sREPORT(AJA_DebugUnit_SMPTEAnc, AJA_DebugSeverity_Info,		AJAFUNC << ":	" << __x__)
#define LOGMYDEBUG(__x__)	AJA_sREPORT(AJA_DebugUnit_SMPTEAnc, AJA_DebugSeverity_Debug,	AJAFUNC << ":	" << __x__)


/////////////////////////////////////////////////////////////////////////////
// CaptionEncoder708 definition
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	PRIVATE FUNCTIONS
/////////////////////////////////////////////////////////////////////////////
/**
	@brief	Converts a single line of NTV2_FBF_8BIT_YCBCR data from the given source buffer, stretching it to 10-bits and copying it
			into another output buffer.
	@note	If SMPTE ancillary data is detected in the video, this routine "intelligently" stretches it by copying the 8-bits to
			the LS 8-bits of the 10-bit output, recalculating parity and checksums as needed. (This emulates what NTV2 device
			firmware does during playout of NTV2_FBF_8BIT_YCBCR frame buffers with NTV2_VANCDATA_8BITSHIFT_ENABLE.)
	@param[in]	pInYUV8Buffer		A valid, non-NULL pointer to the start of the VANC line in an NTV2_FBF_8BIT_YCBCR video buffer.
	@param[out] pOutYUV16Buffer		A valid, non-NULL pointer to the start of a 16-bit UWord/component line buffer that is to receive
									the converted 10-bit-per-component values, which will include even parity and valid checksums.
	@param[in]	inNumPixels			Specifies the width of the line to be converted, in pixels.
	@return		True if successful;	 otherwise false.
**/
static bool UnPackAndShiftAnc8BitLineData (const UByte * pInYUV8Buffer, UWord * pOutYUV16Buffer, const ULWord inNumPixels)
{
	if (!pInYUV8Buffer)
		return false;
	if (!pOutYUV16Buffer)
		return false;
	if (inNumPixels < 12)
		return false;
	if (inNumPixels % 6)
		return false;

	UWordSequence	yuv16Line;
	const bool		result	(CNTV2SMPTEAncData::UnpackLine_8BitYUVtoUWordSequence (pInYUV8Buffer, yuv16Line, inNumPixels));
	if (result)
		for (unsigned n(0);	 n < yuv16Line.size();	n++)
			pOutYUV16Buffer[n] = yuv16Line[n];
	return result;

}	//	UnPackAndShiftAnc8BitLineData


bool CNTV2SMPTEAncData::UnpackLine_8BitYUVtoUWordSequence (const void * pInYUV8Line, UWordSequence & out16BitYUVLine, const ULWord inNumPixels)
{
	const UByte *	pInYUV8Buffer	(reinterpret_cast <const UByte *> (pInYUV8Line));
	const ULWord	maxOutElements	(inNumPixels * 2);

	out16BitYUVLine.clear ();
	out16BitYUVLine.reserve (maxOutElements);
	while (out16BitYUVLine.size() < size_t(maxOutElements))
		out16BitYUVLine.push_back(0);

	if (!pInYUV8Buffer)
		return false;	//	NULL pointer
	if (inNumPixels < 12)
		return false;	//	Invalid width
	if (inNumPixels % 4)	//	6)?
		return false;	//	Width not evenly divisible by 4

	//	Since Y and C may have separate/independent ANC data going on, we're going to split the task and do all
	//	the even (C) samples first, the come back and repeat it for all of the odd (Y) samples...
	for (ULWord comp = 0;  comp < 2;  comp++)
	{
		bool	bNoMoreAnc	(false);	//	Assume all ANC packets (if any) begin at the first pixel and are contiguous
										//	(i.e. no gaps between Anc packets). Once we see a "gap" we set this flag and the
										//	rest of the line turns into a copy.

		ULWord	ancCount	(0);		//	Number of bytes to shift and copy (once an ANC packet is found).
										//	0 == at the start of a potential new Anc packet.

		ULWord	pixNum		(0);		//	The current pixel we are currently serving (note: NOT the component or sample number!)
		UWord	checksum	(0);		//	Accumulator for checksum calculations


		while (pixNum < inNumPixels)
		{
			//	We've found a gap in the Anc data - which we interpret to mean there is no more on this line.
			//	Just do a simple 8-bit -> 10-bit expansion with the remaining data on the line...
			if (bNoMoreAnc)
			{
				const ULWord	index		(2 * pixNum	 +	comp);
				const UWord		dataValue	(static_cast <UWord> (pInYUV8Buffer [index] << 2)); //	Pad 2 LSBs with zeros
				NTV2_ASSERT (index <= ULWord(out16BitYUVLine.size()));
				if (index < ULWord(out16BitYUVLine.size()))
					out16BitYUVLine [index] = dataValue;
				else
					out16BitYUVLine.push_back (dataValue);
				pixNum++;
			}
			else
			{
				//	Still processing (possible) Anc data...
				if (ancCount == 0)
				{
					//	AncCount == 0 means we're at the beginning of an Anc packet - or not...
					if ((pixNum + 7) < inNumPixels)
					{
						//	An Anc packet has to be at least 7 pixels long to be real...
						if (   pInYUV8Buffer [(2 * (pixNum+0)) + comp] == 0x00
							&& pInYUV8Buffer [(2 * (pixNum+1)) + comp] == 0xFF
							&& pInYUV8Buffer [(2 * (pixNum+2)) + comp] == 0xFF)
						{
							//	"00-FF-FF" means a new Anc packet is being started...
							out16BitYUVLine [(2 * pixNum++) + comp] = 0x000;
							out16BitYUVLine [(2 * pixNum++) + comp] = 0x3ff;
							out16BitYUVLine [(2 * pixNum++) + comp] = 0x3ff;		//	Stuff a 10-bit "00-FF-FF" into the output buffer

							ancCount = pInYUV8Buffer[(2 * (pixNum+2)) + comp] + 3 + 1;	//	Grab the number of data words + DID + SID + DC + checksum words
							checksum = 0;												//	Reset checksum accumulator
						}
						else
							bNoMoreAnc = true;	//	No anc here -- assume there's no more for the rest of the line
					}
					else
						bNoMoreAnc = true;	//	Not enough room for another anc packet here -- assume no more for the rest of the line
				}	//	if ancCount == 0
				else if (ancCount == 1)
				{
					//	This is the last byte of an anc packet -- the checksum. Since the original conversion to 8 bits
					//	wiped out part of the original checksum, we've been recalculating it all along until now...
					out16BitYUVLine [(2 * pixNum) + comp]  = checksum & 0x1ff;			//	LS 9 bits of checksum
					out16BitYUVLine [(2 * pixNum) + comp] |= (~checksum & 0x100) << 1;	//	bit 9 = ~bit 8;

					pixNum++;
					ancCount--;
				}	//	else if end of valid Anc packet
				else
				{
					//	ancCount > 0 means an Anc packet is being processed.
					//	Copy 8-bit data into LS 8 bits, add even parity to bit 8, and ~bit 8 to bit 9...
					const UByte ancByte (pInYUV8Buffer [(2 * pixNum) + comp]);
					const UWord ancWord (AddEvenParity (ancByte));

					out16BitYUVLine [(2 * pixNum) + comp] = ancWord;

					checksum += (ancWord & 0x1ff);	//	Add LS 9 bits to checksum

					pixNum++;
					ancCount--;
				}	//	else copying actual Anc packet
			}	//	else processing an Anc packet
		}	//	for each pixel in the line
	}	//	for each Y/C component (channel)

	return true;

}	//	UnpackLine_8BitYUVtoUWordSequence


// AncParityOrChecksumFailed()
//		Returns 'true' if received ANC packets fails one of the tests
//
static bool AncParityOrChecksumFailed (const UWord * pAncBuff, const UWord totalCount)
{
	bool bErr = false;

	//	Check parity on all words...
	for (UWord j = 3;  j < totalCount - 1;	j++)	//	Skip ANC Data Flags and Checksum
	{
		if (pAncBuff[j*2] != CNTV2SMPTEAncData::AddEvenParity(pAncBuff[j*2] & 0xFF))
		{
			LOGMYERROR ("Parity error at word " << j << ":	got 0x" << UHEX2(pAncBuff[j*2])
						<< ", expected 0x" << UHEX2(CNTV2SMPTEAncData::AddEvenParity(pAncBuff[j*2] & 0xFF)));
			bErr = true;
		}
	}

	//	The checksum word is different: bit 8 is part of the checksum total, and bit 9 should be ~bit 8...
	UWord checksum = pAncBuff [(totalCount - 1) * 2];
	bool b8 = (checksum & BIT(8)) != 0;
	bool b9 = (checksum & BIT(9)) != 0;
	if (b8 == b9)
	{
		LOGMYERROR ("Checksum word error:  got 0x" << UHEX2 (checksum) << ", expected 0x" << UHEX2 (checksum ^ 0x200));
		bErr = true;
	}

	//	Check the checksum math...
	UWord sum = 0;
	for (UWord k = 3;  k < totalCount - 1;	k++)
		sum += pAncBuff [k * 2] & 0x1FF;

	if ((sum & 0x1FF) != (checksum & 0x1FF))
	{
		LOGMYERROR ("Checksum math error:  got 0x" << UHEX2 (checksum & 0x1FF) << ", expected 0x" << UHEX2 (sum & 0x1FF));
		bErr = true;
	}

	return bErr;

}	//	AncParityOrChecksumFailed


ostream & operator << (ostream & inOutStream, const UWordVANCPacketList & inData)
{
	UWord	num (0);
	for (UWordVANCPacketListConstIter iter (inData.begin ());  iter != inData.end ();  )
	{
		inOutStream << "Pkt" << dec << ++num << ": " << *iter << endl;
		if (++iter != inData.end())
			inOutStream << endl;
	}
	return inOutStream;
}


static bool CheckAncParityAndChecksum (const UWordSequence &	inYUV16Line,
										const UWord				inStartIndex,
										const UWord				inTotalCount,
										const UWord				inIncrement = 2)
{
	bool	bErr	(false);
	UWord	ndx		(0);
	if (inIncrement == 0  ||  inIncrement > 2)
		return true;	//	Increment must be 1 or 2

	//	Check parity on all words...
	for (ndx = 3;  ndx < inTotalCount - 1;	ndx++)	//	Skip ANC Data Flags and Checksum
	{
		const UWord wordValue	(inYUV16Line[inStartIndex + ndx * inIncrement]);
		if (wordValue != CNTV2SMPTEAncData::AddEvenParity(wordValue & 0xFF))
		{
			LOGMYERROR ("Parity error at word " << ndx << ":  got " << xHEX0N(wordValue,2)
						<< ", expected " << xHEX0N(CNTV2SMPTEAncData::AddEvenParity(wordValue & 0xFF),2));
			bErr = true;
		}
	}

	//	The checksum word is different: bit 8 is part of the checksum total, and bit 9 should be ~bit 8...
	const UWord checksum	(inYUV16Line [inStartIndex + (inTotalCount - 1) * inIncrement]);
	const bool	b8			((checksum & BIT(8)) != 0);
	const bool	b9			((checksum & BIT(9)) != 0);
	if (b8 == b9)
	{
		LOGMYERROR ("Checksum word error:  got " << xHEX0N(checksum,2) << ", expected " << xHEX0N(checksum ^ 0x200, 2));
		bErr = true;
	}

	//	Check the checksum math...
	UWord	sum (0);
	for (ndx = 3;  ndx < inTotalCount - 1;	ndx++)
		sum += inYUV16Line [inStartIndex + ndx * inIncrement] & 0x1FF;

	if ((sum & 0x1FF) != (checksum & 0x1FF))
	{
		LOGMYERROR ("Checksum math error:  got " << xHEX0N(checksum & 0x1FF, 2) << ", expected " << xHEX0N(sum & 0x1FF, 2));
		bErr = true;
	}

	return bErr;

}	//	CheckAncParityAndChecksum


bool CNTV2SMPTEAncData::GetAncPacketsFromVANCLine (const UWordSequence &				inYUV16Line,
													const NTV2_SMPTEAncChannelSelect	inChanSelect,
													UWordVANCPacketList &				outRawPackets,
													UWordSequence &						outWordOffsets)
{
	const UWord wordCountMax	(UWord(inYUV16Line.size ()));

	//	If we're only looking at Y samples, start the search 1 word into the line...
	const UWord searchOffset	(inChanSelect == kNTV2SMPTEAncChannel_Y ? 1 : 0);

	//	If we're looking in both channels (e.g. SD video), increment search words by 1, else skip at every other word...
	const UWord searchIncr		(inChanSelect == kNTV2SMPTEAncChannel_Both ? 1 : 2);

	outRawPackets.clear();
	outWordOffsets.clear();

	if (wordCountMax < 12)
		return false;	//	too small

	for (UWord wordNum = searchOffset;	wordNum < (wordCountMax - 12);	wordNum += searchIncr)
	{
		const UWord ancHdr0 (inYUV16Line.at (wordNum + (0 * searchIncr)));	//	0x000
		const UWord ancHdr1 (inYUV16Line.at (wordNum + (1 * searchIncr)));	//	0x3ff
		const UWord ancHdr2 (inYUV16Line.at (wordNum + (2 * searchIncr)));	//	0x3ff
		const UWord ancHdr3 (inYUV16Line.at (wordNum + (3 * searchIncr)));	//	DID
		const UWord ancHdr4 (inYUV16Line.at (wordNum + (4 * searchIncr)));	//	SDID
		const UWord ancHdr5 (inYUV16Line.at (wordNum + (5 * searchIncr)));	//	DC
		if (ancHdr0 == 0x000  &&  ancHdr1 == 0x3ff	&&	ancHdr2 == 0x3ff)
		{
			//	Total words in ANC packet: 6 header words + data count + checksum word...
			UWord	dataCount	(ancHdr5 & 0xFF);
			UWord	totalCount	(kAncHeaderSize + dataCount + kAncFooterSize);

			if (totalCount > wordCountMax)
			{
				totalCount = wordCountMax;
				LOGMYERROR ("packet totalCount " << totalCount << " exceeds max " << wordCountMax);
				return false;
			}

			//	Be sure we don't go past the end of the line buffer...
			if (ULWord (wordNum + totalCount) >= wordCountMax)
			{
				LOGMYDEBUG ("past end of line: " << wordNum << " + " << totalCount << " >= " << wordCountMax);
				return false;	//	Past end of line buffer
			}

			if (CheckAncParityAndChecksum (inYUV16Line, wordNum, totalCount, searchIncr))
				return false;	//	Parity/Checksum error

			UWordVANCPacket packet;
			for (unsigned i = 0;  i < totalCount;  i++)
				packet.push_back (inYUV16Line.at (wordNum + (i * searchIncr)));
			outRawPackets.push_back (packet);
			outWordOffsets.push_back (wordNum);

			LOGMYINFO ("Found ANC packet in " << ::NTV2SMPTEAncChannelSelectToString(inChanSelect)
							<< ": DID=0x" << HEX4(ancHdr3)
							<< " SDID=0x" << HEX4(ancHdr4)
							<< " word=" << wordNum
							<< " DC=" << ancHdr5
							<< " pix=" << (wordNum / searchIncr));
		}	//	if ANC packet found (wildcard or matching DID/SDID)
	}	//	scan the line

	return true;

}	//	GetAncPacketsFromVANCLine


static const string sAncChanSelStrs[]	=	{"Y", "C", "Y+C", ""};


static bool FindAncInLine (UWord						inAncDID,
							UWord						inAncSDID,
							const NTV2Buffer &			inLineBuffer,
							NTV2_SMPTEAncChannelSelect	inSearchChannel,
							UWordSequence &				outWords,
							bool &						outHasParityError,
							UWord &						inOutPixelStart)
{
	bool			bFound				(false);
	const ULWord	inLineBuffWordCount (inLineBuffer.GetByteCount() / sizeof(UWord));
	const UWord *	pInLineBuffer		(reinterpret_cast<const UWord*>(inLineBuffer.GetHostAddress(0)));

	outWords.clear();
	if (inLineBuffer.IsNULL())
		{LOGMYINFO("NULL line buffer");	 return false;}

	//	If we're only looking at Y samples, start the search 1 word into the line...
	const UWord searchOffset	(inSearchChannel == kNTV2SMPTEAncChannel_Y ? 1 : 0);

	//	If we're looking in both channels (e.g. SD video), increment search words by 1, else skip at every other word...
	const UWord searchIncr		(inSearchChannel == kNTV2SMPTEAncChannel_Both ? 1 : 2);

	//	Start searching at zero unless user designated another pixel address...
	const UWord wordStart		(inOutPixelStart * searchIncr);

	for (UWord wordNum(wordStart + searchOffset);	!bFound	 &&	 wordNum < (inLineBuffWordCount - 12);	 wordNum += searchIncr)
	{
		if (	pInLineBuffer [wordNum + (0 * searchIncr)] == 0x000
			&&	pInLineBuffer [wordNum + (1 * searchIncr)] == 0x3ff
			&&	pInLineBuffer [wordNum + (2 * searchIncr)] == 0x3ff
			&& (pInLineBuffer [wordNum + (3 * searchIncr)] == inAncDID	|| inAncDID	 == NTV2_WildCardDID)
			&& (pInLineBuffer [wordNum + (4 * searchIncr)] == inAncSDID || inAncSDID == NTV2_WildCardSDID) )
		{
			//	Total words in ANC packet: 6 header words + data count + checksum word...
			UWord	dataCount	(pInLineBuffer[wordNum + (5 * searchIncr)] & 0x00FF);
			ULWord	totalCount	(kAncHeaderSize + dataCount + kAncFooterSize);

			//	Be sure we don't go past the end of the line buffer...
			if (ULWord(wordNum + totalCount) < inLineBuffWordCount)
			{
				//	Copy out to buffer...
				for (unsigned i = 0;  i < totalCount;  i++)
					outWords.push_back(pInLineBuffer[wordNum + (i * searchIncr)]);

				//	Scan for parity/checksum errors...
				outHasParityError = AncParityOrChecksumFailed (&pInLineBuffer[wordNum + 0], UWord(totalCount));

				//	Report the starting pixel number of the packet...
				inOutPixelStart = (wordNum / searchIncr);

				bFound = true;
				LOGMYINFO (dataCount << "-byte packet with DID=0x" << HEX4(outWords.at(3)) << " SID=0x" << HEX4(outWords.at(4))
							<< " found at wordNum=" << wordNum << " pixel=" << (wordNum / searchIncr));
				break;
			}	//	if still within the line
		}	//	if ANC packet found (wildcard or matching DID/SDID)
	}	//	scan the line

	if (!bFound)
	{
		//ostringstream oss;
		#if 0
			oss << endl;
			CNTV2CaptionLogConfig::DumpMemory (pInLineBuffer, inLineBuffWordCount * sizeof(UWord), oss, 16, //	radix
												2,			//	bytes per group
												64,			//	groups per line
												0,			//	address radix (don't show addresses)
												false);		//	no ascii
		#endif
		//LOGMYDEBUG ("Packet with DID=0x" << HEX4(inAncDID) << " SID=0x" << HEX4(inAncSDID) << " not found in " << sAncChanSelStrs[inSearchChannel] << oss.str());
	}
	return bFound;

}	//	FindAncInLine


static bool FindAncInLine (UWord						ancDID,
							UWord						ancSDID,
							const UWord *				pLineBuffer,
							NTV2_SMPTEAncChannelSelect	chan,
							ULWord						lineBuffWordCount,
							UWord *						pOutBuff,
							ULWord *					pOutWordCount,
							const ULWord				wordCountMax,
							bool *						pbErr,
							UWord *						pPixelStart)
{
	bool bFound = false;
	bool bErr	= false;

	//	If we're only looking at Y samples, start the search 1 word into the line...
	const UWord searchOffset	(chan == kNTV2SMPTEAncChannel_Y ? 1 : 0);

	//	If we're looking in both channels (e.g. SD video), increment search words by 1, else skip at every other word...
	const UWord searchIncr		(chan == kNTV2SMPTEAncChannel_Both ? 1 : 2);

	//	Start searching at zero unless user designated another pixel address...
	const UWord wordStart		(pPixelStart  ?	 (*pPixelStart * searchIncr)  :	 0);

	for (UWord wordNum = wordStart + searchOffset;	wordNum < (lineBuffWordCount - 12) && bFound == false;	wordNum += searchIncr)
	{
		if (	pLineBuffer [wordNum + (0 * searchIncr)] == 0x000
			&&	pLineBuffer [wordNum + (1 * searchIncr)] == 0x3ff
			&&	pLineBuffer [wordNum + (2 * searchIncr)] == 0x3ff
			&& (pLineBuffer [wordNum + (3 * searchIncr)] == ancDID	|| ancDID  == NTV2_WildCardDID)
			&& (pLineBuffer [wordNum + (4 * searchIncr)] == ancSDID || ancSDID == NTV2_WildCardSDID) )
		{
			//	Total words in ANC packet: 6 header words + data count + checksum word...
			UWord	dataCount	(pLineBuffer[wordNum + (5 * searchIncr)] & 0xFF);
			ULWord	totalCount	(kAncHeaderSize + dataCount + kAncFooterSize);

			//	Make sure we don't have more data than the user can swallow...
			if (totalCount > wordCountMax)
			{
				totalCount = wordCountMax;
				bErr = true;
			}

			//	Be sure we don't go past the end of the line buffer...
			if (ULWord(wordNum + totalCount) < lineBuffWordCount)
			{
				//	Copy out to buffer...
				if (pOutBuff)
				{
					for (unsigned i = 0;  i < totalCount;  i++)
						pOutBuff[i] = pLineBuffer[wordNum + (i * searchIncr)];

					//	Optional: scan for parity/checksum errors...
					if (pbErr)
						bErr = AncParityOrChecksumFailed (&pLineBuffer[wordNum + 0], UWord(totalCount));
				}

				//	Total word count...
				if (pOutWordCount)
					*pOutWordCount = totalCount;

				//	Report the starting pixel number of the packet...
				if (pPixelStart)
					*pPixelStart = (wordNum / searchIncr);

				bFound = true;
				LOGMYINFO (dataCount << "-byte packet with DID=0x" << HEX4(ancDID) << " SID=0x" << HEX4(ancSDID) << " found at wordNum=" << wordNum << " pixel=" << (wordNum / searchIncr));
				break;
			}	//	if still within the line
		}	//	if ANC packet found (wildcard or matching DID/SDID)
	}	//	scan the line

	//	If we found an error along the way, tell the user (if not, don't change)...
	if (bFound && pbErr && bErr)
		*pbErr = bErr;

	if (!bFound)
	{
		//ostringstream oss;
		#if 0
			oss << endl;
			CNTV2CaptionLogConfig::DumpMemory (pLineBuffer, lineBuffWordCount * sizeof (UWord), oss, 16,	//	radix
												2,			//	bytes per group
												64,			//	groups per line
												0,			//	address radix (don't show addresses)
												false);		//	no ascii
		#endif
		//LOGMYDEBUG ("Packet with DID=0x" << HEX4(ancDID) << " SID=0x" << HEX4(ancSDID) << " not found in " << sAncChanSelStrs[chan] << oss.str());
	}
	return bFound;

}	//	FindAncInLine



/////////////////////////////////////////////////////////////////////////////
//	CNTV2SMPTEAncData Implementation
/////////////////////////////////////////////////////////////////////////////


string NTV2SMPTEAncChannelSelectToString (const NTV2_SMPTEAncChannelSelect inChanSelect, const bool inCompact)
{
	switch(inChanSelect)
	{
		case kNTV2SMPTEAncChannel_Y:	return inCompact ? "Y" : "kNTV2SMPTEAncChannel_Y";
		case kNTV2SMPTEAncChannel_C:	return inCompact ? "C" : "kNTV2SMPTEAncChannel_C";
		case kNTV2SMPTEAncChannel_Both: return inCompact ? "Y+C" : "kNTV2SMPTEAncChannel_Both";
		default:						break;
	}
	return "?";
}


// MakeAncHeader
//		Initializes an NTV2_SMPTEAncHeader with the given parameters, adding parity bits as needed
bool CNTV2SMPTEAncData::MakeAncHeader (NTV2_SMPTEAncHeaderPtr pHdr, UByte ancDID, UByte ancSDID, UByte ancDC)
{
	bool	bResult (true);

	if (pHdr == NULL)
		return false;

	pHdr->ancDataFlag0	= 0x000;
	pHdr->ancDataFlag1	= 0x3FF;
	pHdr->ancDataFlag2	= 0x3FF;
	pHdr->ancDataID		= AddEvenParity (ancDID);
	pHdr->ancSecDID		= AddEvenParity (ancSDID);
	pHdr->ancDataCount	= AddEvenParity (ancDC);

	return bResult;

}	//	MakeAncHeader


// SetAncHeaderDataCount
//		Sets an NTV2_SMPTEAncHeader "DataCount" param with the given size, adding parity
bool CNTV2SMPTEAncData::SetAncHeaderDataCount (NTV2_SMPTEAncHeaderPtr pHdr, UByte ancDC)
{
	bool	bResult (true);

	if (pHdr == NULL)
		return false;

	pHdr->ancDataCount = AddEvenParity (ancDC);

	return bResult;

}	//	SetAncHeaderDataCount


// SetAncFooterChecksum
//		Sets an NTV2_SMPTEAncHeader "DataCount" param with the given size, adding parity
bool CNTV2SMPTEAncData::SetAncFooterChecksum (NTV2_SMPTEAncFooterPtr pFtr, UWord checksum)
{
	bool	bResult (true);

	if (pFtr == NULL)
		return false;

	//	Per SMPTE 291 Rules:  bits 8:0 = checksum, bit 9 = ~bit 8
	pFtr->ancChecksum = (checksum & 0x1FF) + (~(checksum << 1) & 0x200);

	return bResult;

}	//	SetAncFooterChecksum



// CalculateAncChecksum
//		Given a pointer to an initialized ancillary packet, calculates the 9-bit checksum
//		and inserts it at the end of the packet.
//
// NOTE: either the correct data size (in words) must be provided, or (if zero) this
//		 method will assume that the header DataCount is correct.
bool CNTV2SMPTEAncData::CalculateAncChecksum (NTV2_SMPTEAncHeaderPtr pHdr, UByte dataCount)
{
	bool	bResult (true);

	if (pHdr == NULL)
		return false;

	//	If the dataSize is not explicitly given, get it from the header...
	if (dataCount == 0)
		dataCount = pHdr->ancDataCount & 0xFF;


	//	By SMPTE 291 rules, "the checksum value is equal to the nine least significant bits
	//	of the sum of the nine least significant bits of the data identification (DID), the
	//	data block number (DBN), or the secondary data identification (SDID), the data count
	//	(DC), and all user data words (UDW) in the packet."

	//	Increase the data count by the 3 header words we're going to include...
	dataCount += 3;

	//	Start at the DataID word, and go through to the end of the User Data Words...
	UWord * ptr = &(pHdr->ancDataID);

	UWord	checksum	(0);
	for (int i = 0; i < dataCount; i++)
		checksum += *ptr++;

	//	SetAncChecksum() will take care of packaging the checksum bits...
	bResult = SetAncFooterChecksum ((NTV2_SMPTEAncFooterPtr) ptr, checksum);

	return bResult;

}	//	CalculateAncChecksum


// AddEvenParity
//		Returns the original data byte in bits 7:0, plus even parity in bit 8 and ~bit 8 in bit 9
UWord CNTV2SMPTEAncData::AddEvenParity (UByte dataByte)
{
	// Parity Table
	static const UWord gAncEvenParityTable [256] =
	{
		/* 0-7 */		0x200,0x101,0x102,0x203,0x104,0x205,0x206,0x107,
		/* 8-15 */		0x108,0x209,0x20A,0x10B,0x20C,0x10D,0x10E,0x20F,
		/* 16-23 */		0x110,0x211,0x212,0x113,0x214,0x115,0x116,0x217,
		/* 24-31 */		0x218,0x119,0x11A,0x21B,0x11C,0x21D,0x21E,0x11F,
		/* 32-39 */		0x120,0x221,0x222,0x123,0x224,0x125,0x126,0x227,
		/* 40-47 */		0x228,0x129,0x12A,0x22B,0x12C,0x22D,0x22E,0x12F,
		/* 48-55 */		0x230,0x131,0x132,0x233,0x134,0x235,0x236,0x137,
		/* 56-63 */		0x138,0x239,0x23A,0x13B,0x23C,0x13D,0x13E,0x23F,
		/* 64-71 */		0x140,0x241,0x242,0x143,0x244,0x145,0x146,0x247,
		/* 72-79 */		0x248,0x149,0x14A,0x24B,0x14C,0x24D,0x24E,0x14F,
		/* 80-87 */		0x250,0x151,0x152,0x253,0x154,0x255,0x256,0x157,
		/* 88-95 */		0x158,0x259,0x25A,0x15B,0x25C,0x15D,0x15E,0x25F,
		/* 96-103 */	0x260,0x161,0x162,0x263,0x164,0x265,0x266,0x167,
		/* 104-111 */	0x168,0x269,0x26A,0x16B,0x26C,0x16D,0x16E,0x26F,
		/* 112-119 */	0x170,0x271,0x272,0x173,0x274,0x175,0x176,0x277,
		/* 120-127 */	0x278,0x179,0x17A,0x27B,0x17C,0x27D,0x27E,0x17F,
		/* 128-135 */	0x180,0x281,0x282,0x183,0x284,0x185,0x186,0x287,
		/* 136-143 */	0x288,0x189,0x18A,0x28B,0x18C,0x28D,0x28E,0x18F,
		/* 144-151 */	0x290,0x191,0x192,0x293,0x194,0x295,0x296,0x197,
		/* 152-159 */	0x198,0x299,0x29A,0x19B,0x29C,0x19D,0x19E,0x29F,
		/* 160-167 */	0x2A0,0x1A1,0x1A2,0x2A3,0x1A4,0x2A5,0x2A6,0x1A7,
		/* 168-175 */	0x1A8,0x2A9,0x2AA,0x1AB,0x2AC,0x1AD,0x1AE,0x2AF,
		/* 176-183 */	0x1B0,0x2B1,0x2B2,0x1B3,0x2B4,0x1B5,0x1B6,0x2B7,
		/* 184-191 */	0x2B8,0x1B9,0x1BA,0x2BB,0x1BC,0x2BD,0x2BE,0x1BF,
		/* 192-199 */	0x2C0,0x1C1,0x1C2,0x2C3,0x1C4,0x2C5,0x2C6,0x1C7,
		/* 200-207 */	0x1C8,0x2C9,0x2CA,0x1CB,0x2CC,0x1CD,0x1CE,0x2CF,
		/* 208-215 */	0x1D0,0x2D1,0x2D2,0x1D3,0x2D4,0x1D5,0x1D6,0x2D7,
		/* 216-223 */	0x2D8,0x1D9,0x1DA,0x2DB,0x1DC,0x2DD,0x2DE,0x1DF,
		/* 224-231 */	0x1E0,0x2E1,0x2E2,0x1E3,0x2E4,0x1E5,0x1E6,0x2E7,
		/* 232-239 */	0x2E8,0x1E9,0x1EA,0x2EB,0x1EC,0x2ED,0x2EE,0x1EF,
		/* 240-247 */	0x2F0,0x1F1,0x1F2,0x2F3,0x1F4,0x2F5,0x2F6,0x1F7,
		/* 248-255 */	0x1F8,0x2F9,0x2FA,0x1FB,0x2FC,0x1FD,0x1FE,0x2FF
	};	//	gAncEvenParityTable
	return gAncEvenParityTable [dataByte];
}


bool CNTV2SMPTEAncData::FindAnc (const UWord						inAncDID,
								const UWord							inAncSDID,
								const NTV2Buffer &					inFrameBuffer,
								const NTV2FormatDescriptor &		inFormatDesc,
								const NTV2_SMPTEAncChannelSelect	inAncChannel,
								UWordSequence &						outWords,
								bool &								outHasParityErrors,
								const UWord							inLineIncrement,
								UWord &								inOutLineStart,
								UWord &								inOutPixelStart)
{
	outHasParityErrors = false;		//	Clear parity error flag -- I'll set it below if I run into a problem
	UWord	ancDID	(inAncDID);
	UWord	ancSDID (inAncSDID);
	if (inFrameBuffer.IsNULL())
		{LOGMYERROR ("NULL frame buffer pointer");	return false;}
	if (!inFormatDesc.IsValid())
		{LOGMYERROR ("Invalid format descriptor");	return false;}
	if (inOutLineStart >= inFormatDesc.GetFirstActiveLine())
		{LOGMYERROR ("Start line " << inOutLineStart << " is in active video -- " << inFormatDesc);	 return false;}
	if (!inLineIncrement)
		{LOGMYERROR ("Zero line increment");  return false;}
	if (inAncChannel > kNTV2SMPTEAncChannel_Both)
		{LOGMYERROR ("Bad channel select value " << int(inAncChannel));	 return false;}
	const NTV2FrameBufferFormat inFBFormat	(inFormatDesc.GetPixelFormat());
	if (inFBFormat != NTV2_FBF_10BIT_YCBCR	&&	inFBFormat != NTV2_FBF_8BIT_YCBCR)
		{LOGMYERROR ("Unimplemented FB format '" << ::NTV2FrameBufferFormatToString(inFBFormat) << "'");  return false;}

	//	If the caller has only given us an 8-bit DID/SDID, add the proper parity (aren't we nice!)
	if ((inAncDID & 0x300) == 0)
		ancDID = AddEvenParity(ancDID & 0xFF);

	if ((inAncSDID & 0x300) == 0)
		ancSDID = AddEvenParity(ancSDID & 0xFF);

	//	See if the caller has specified a line/pixel address from which to start the search...
	UWord		lineStart	(inOutLineStart);
	UWord		pixelStart	(inOutPixelStart);
	UWord		lineNumber	(0);
	bool		bFound		(false);
	NTV2Buffer	lineBuff	(inFormatDesc.GetRasterWidth() * 4 * sizeof(UWord));
	UWord *		pLineData	(reinterpret_cast<UWord*>(lineBuff.GetHostPointer()));

	for (lineNumber = lineStart;  lineNumber < inFormatDesc.GetFirstActiveLine();  lineNumber += inLineIncrement)
	{
		const NTV2Buffer	line	(inFormatDesc.GetRowAddress (inFrameBuffer.GetHostPointer(), lineNumber), inFormatDesc.GetBytesPerRow());
		const ULWord *	pLine	(reinterpret_cast<const ULWord*>(inFormatDesc.GetRowAddress (inFrameBuffer.GetHostPointer(), lineNumber)));

		//	"Unpack" the pixels from the frame buffer line into a common 16-bit format...
		if (inFBFormat == NTV2_FBF_10BIT_YCBCR)
			::UnpackLine_10BitYUVto16BitYUV (pLine, pLineData, inFormatDesc.numPixels); //	Formerly UnPackAndShiftAnc10BitLineData
		else if (inFBFormat == NTV2_FBF_8BIT_YCBCR)
			UnPackAndShiftAnc8BitLineData (reinterpret_cast <const UByte *> (pLine), pLineData, inFormatDesc.numPixels);

		//	Search the line for the designated ANC ID - if found, copy the anc data to pOutBuff...
		//					   (inAncDID,	inAncSDID,	inLineBuffer,	inSearchChannel,	outWords,	outHasParityError,	inOutPixelStart
		bFound = FindAncInLine (ancDID,		ancSDID,	lineBuff,		inAncChannel,		outWords,	outHasParityErrors, pixelStart);
		if (bFound)
			break;	//	Found -- stop searching!

		pixelStart = 0; //	After first line searched, search ALL pixels on subsequent lines
	}	//	for each VANC line

	if (bFound)
	{
		//	Report the packet's line/pixel location, so the caller can start a new search from this point (if needed)...
		inOutLineStart = lineNumber;
		inOutPixelStart = pixelStart;
		LOGMYNOTE(outWords.size() << "-word packet with DID=0x" << HEX4(inAncDID) << " SID=0x" << HEX4(inAncSDID) << " found in " << sAncChanSelStrs[inAncChannel] << " at lineNum=" << lineNumber << " pixel=" << pixelStart);
	}
	else
		LOGMYDEBUG("Packet with DID=0x" << HEX4(inAncDID) << " SID=0x" << HEX4(inAncSDID) << " not found in " << sAncChanSelStrs[inAncChannel] << " in every " << inLineIncrement << " lines " << lineStart << "-" << (inFormatDesc.GetFirstActiveLine()-1));
	return bFound;

}	//	FindAnc


bool CNTV2SMPTEAncData::FindAnc (const UWord						inAncDID,
								const UWord							inAncSDID,
								const ULWord *						pInFrameBuffer,
								const NTV2_SMPTEAncChannelSelect	inChannel,
								const NTV2VideoFormat				inVideoFormat,
								const NTV2FrameBufferFormat			inFBFormat,
								UWord *								pOutBuff,
								ULWord &							outWordCount,
								const ULWord						wordCountMax,
								bool &								outHasParityErrors,
								const UWord							inLineIncrement,
								UWord &								inOutLineStart,
								UWord &								inOutPixelStart)
{
	outHasParityErrors = false;		//	Clear parity error flag -- I'll set it below if I run into a problem
	UWord	ancDID	(inAncDID);
	UWord	ancSDID (inAncSDID);
	if (!pInFrameBuffer)
		{LOGMYERROR ("NULL frame buffer pointer");	return false;}
	if (!inLineIncrement)
		{LOGMYERROR ("Zero line increment");  return false;}
	if (inFBFormat != NTV2_FBF_10BIT_YCBCR	&&	inFBFormat != NTV2_FBF_8BIT_YCBCR)
		{LOGMYERROR ("Unimplemented FB format '" << ::NTV2FrameBufferFormatToString(inFBFormat) << "'");  return false;}

	//	If the caller has only given us an 8-bit DID/SDID, add the proper parity (aren't we nice!)
	if ((inAncDID & 0x300) == 0)
		ancDID = AddEvenParity(ancDID & 0xFF);

	if ((inAncSDID & 0x300) == 0)
		ancSDID = AddEvenParity(ancSDID & 0xFF);

	//	See if the caller has specified a line/pixel address from which to start the search...
	UWord						lineStart	(inOutLineStart);
	UWord						pixelStart	(inOutPixelStart);
	UWord						lineNumber	(0);
	const NTV2Standard			standard	(::GetNTV2StandardFromVideoFormat (inVideoFormat));
	const NTV2FormatDescriptor	fd			(standard, inFBFormat, NTV2_VANCMODE_TALL);
	bool						bFound		(false);
	UWord *						pU16Line	(new UWord [fd.numPixels * 4]);
	const ULWord *				pLine		(pInFrameBuffer);
	pLine += (lineStart * fd.linePitch);	//	Jump to specified lineStart

	for (lineNumber = lineStart;  lineNumber < fd.firstActiveLine;	lineNumber += inLineIncrement)
	{
		//	"Unpack" the pixels from the frame buffer line into a common 16-bit format...
		if (inFBFormat == NTV2_FBF_10BIT_YCBCR)
			::UnpackLine_10BitYUVto16BitYUV (pLine, pU16Line, fd.numPixels);	//	Formerly UnPackAndShiftAnc10BitLineData
		else if (inFBFormat == NTV2_FBF_8BIT_YCBCR)
			UnPackAndShiftAnc8BitLineData (reinterpret_cast <const UByte *> (pLine), pU16Line, fd.numPixels);

		//	Search the line for the designated ANC ID - if found, copy the anc data to pOutBuff...
		//					   (ancDID, ancSDID,	pU16Line,	ChannelSelect,	lineBuffWordCount,	pOutBuff,	pOutWordCount,	wordCountMax,	pOutParityErr,			pPixelStart)
		bFound = FindAncInLine (ancDID, ancSDID,	pU16Line,	inChannel,		(fd.numPixels * 2), pOutBuff,	&outWordCount,	wordCountMax,	&outHasParityErrors,	&pixelStart);

		//	Stop after we find the first instance -- report the line/pixel location so the caller can start a new search from this point...
		if (bFound)
			break;	//	Found!

		//	Nothing on that line -- increment to the next line and look again...
		pLine += (inLineIncrement * fd.linePitch);

		//	For search purposes, we only allow start offsets on the FIRST line. After the first line we search ALL pixels on subsequent lines...
		pixelStart = 0;
	}	//	for each VANC line

	delete [] pU16Line;
	if (bFound)
	{
		//	Report back where we found the ANC packet
		//	NOTE:	The purpose of this return is to allow the caller to use these values as the start points for the NEXT search (e.g. when the caller
		//			wants to iterate through ALL instances of a given ANC ID). However, the caller needs to increment the returned pixel/line numbers
		//			before making another call or else this routine will simple re-find the same ANC packet it did before!!
		inOutLineStart = lineNumber;
		inOutPixelStart = pixelStart;
		LOGMYNOTE(outWordCount << "-word packet with DID=0x" << HEX4(inAncDID) << " SID=0x" << HEX4(inAncSDID) << " found at lineNum=" << lineNumber << " pixel=" << pixelStart);
	}
	else
		LOGMYDEBUG("Packet with DID=0x" << HEX4(inAncDID) << " SID=0x" << HEX4(inAncSDID) << " not found in " << sAncChanSelStrs[inChannel] << " in every " << inLineIncrement << " lines " << lineStart << "-" << (fd.firstActiveLine-1));
	return bFound;

}	//	FindAnc


bool CNTV2SMPTEAncData::FindAnc (const UWord						inAncDID,
								const UWord							inAncSDID,
								const ULWord *						pInFrameBuffer,
								const NTV2_SMPTEAncChannelSelect	inAncChannel,
								const NTV2VideoFormat				inVideoFormat,
								const NTV2FrameBufferFormat			inFBFormat,
								UWord *								pOutBuff,
								ULWord &							outWordCount,
								const ULWord						inWordCountMax,
								bool &								outHasParityErrors)
{
	UWord		lineStart		(0);
	UWord		pixelStart		(0);
	const UWord lineIncrement	(1);
	return FindAnc (inAncDID, inAncSDID, pInFrameBuffer, inAncChannel, inVideoFormat, inFBFormat, pOutBuff, outWordCount, inWordCountMax, outHasParityErrors, lineIncrement, lineStart, pixelStart);

}	//	FindAnc


bool CNTV2SMPTEAncData::FindAnc (const UWord						inAncDID,
								const UWord							inAncSDID,
								const ULWord *						pInFrameBuffer,
								const NTV2_SMPTEAncChannelSelect	inChannel,
								const NTV2VideoFormat				inVideoFormat,
								const NTV2FrameBufferFormat			inFBFormat,
								UWord *								pOutBuff,
								ULWord &							outWordCount,
								const ULWord						inWordCountMax)
{
	UWord		lineStart		(0);
	UWord		pixelStart		(0);
	const UWord lineIncrement	(1);
	bool		hasParityErrors (false);
	return FindAnc (inAncDID, inAncSDID, pInFrameBuffer, inChannel, inVideoFormat, inFBFormat, pOutBuff, outWordCount, inWordCountMax, hasParityErrors, lineIncrement, lineStart, pixelStart);

}	//	FindAnc


// ExtractCompressedAnc()
// find all anc packets in VANC, compress them (see ProRes/DVCProHD spec) into output buffer
bool CNTV2SMPTEAncData::ExtractCompressedAnc (const void * pFrameBuffer, void * pAncBuff, ULWord ancBufMax, ULWord & inOutFoundSize, NTV2VideoFormat videoFormat, NTV2FrameBufferFormat fbFormat)
{
	UWord	pixelStart	(0);
	ULWord	ancWordCount (0), compressSize (0);
	bool	bFound		(false);
	bool	bErr		(false);

	NTV2Standard			standard	(::GetNTV2StandardFromVideoFormat (videoFormat));
	NTV2FormatDescriptor	fd			(standard, fbFormat, NTV2_VANCMODE_TALL);
	NTV2SmpteLineNumber		ln			(::GetSmpteLineNumber (standard));

	ULWord					lineWordCount	(fd.numPixels * 4);
	ULWord					rowBytes		(fd.linePitch * 4);
	UWord *					pLineBuf		(new UWord [lineWordCount]);
	UWord *					pAncPacketBuf	(new UWord [lineWordCount]);
	UByte *					pOut			((UByte *) pAncBuff);
	UByte *					pIn				(NULL);

	//	Init channel/line values...
	ULWord						smpteLine (0), line (0);
	NTV2_SMPTEAncChannelSelect	chan;
	FirstVancLineAndChannel (fd, ln, chan, smpteLine, line);

	do
	{
		pIn = (UByte *) pFrameBuffer + (rowBytes * line);

		if (fbFormat == NTV2_FBF_10BIT_YCBCR || fbFormat == NTV2_FBF_8BIT_YCBCR)
		{
			//	"Unpack" the pixels from the frame buffer line into a common 16-bit format...
			if (fbFormat == NTV2_FBF_10BIT_YCBCR)
				::UnpackLine_10BitYUVto16BitYUV ((ULWord *) pIn, pLineBuf, fd.numPixels);	//	Formerly UnPackAndShiftAnc10BitLineData
			else if (fbFormat == NTV2_FBF_8BIT_YCBCR)
				UnPackAndShiftAnc8BitLineData (pIn, pLineBuf, fd.numPixels);

			//	Search Y components in line...
			pixelStart = 0;
			do
			{
				bFound = FindAncInLine (NTV2_WildCardDID, NTV2_WildCardSDID, pLineBuf, chan, (fd.numPixels * 2), pAncPacketBuf, &ancWordCount, lineWordCount, &bErr, &pixelStart);
				if (bFound && ((ancBufMax - inOutFoundSize) > ancWordCount))
				{
					CompressAncPacket (pAncPacketBuf, pOut, ancBufMax - inOutFoundSize, compressSize, chan, UWord (smpteLine));
					pOut += compressSize;
					inOutFoundSize += compressSize;
					pixelStart += UWord (ancWordCount);
				}
			} while (bFound);
		}
		else	//	Unimplemented frame buffer format
		{
			bFound = false;
			break;
		}
	} while (NextVancLineAndChannel (fd, ln, chan, smpteLine, line));

	delete [] pLineBuf;
	delete [] pAncPacketBuf;

	return inOutFoundSize > 0;

}	//	ExtractCompressedAnc


// EmbedCompressedAnc()
// decompress anc data (see ProRes/DVCProHD spec), embed into uncompressed VANC
bool CNTV2SMPTEAncData::EmbedCompressedAnc (const void * pAncBuff, void * pFrameBuffer, const ULWord ancBufSize, const NTV2VideoFormat videoFormat, const NTV2FrameBufferFormat fbFormat)
{
	bool					embedded	(false);
	UByte *					pIn			(NULL);
	NTV2Standard			standard	(::GetNTV2StandardFromVideoFormat (videoFormat));
	NTV2FormatDescriptor	fd			(standard, fbFormat, NTV2_VANCMODE_TALL);
	NTV2SmpteLineNumber		ln			(::GetSmpteLineNumber (standard));

	ULWord					ancPacketCount	(0);
	ULWord					chanOffset		(0);
	bool					validLoc		(false);
	ULWord					wordsPerLine	(fd.numPixels * 4);
	UWord *					pAncUnpackedBuf (new UWord [wordsPerLine]);

	//	Init channel/line values...
	ULWord						smpteLine (0), line (0);
	NTV2_SMPTEAncChannelSelect	chan;
	FirstVancLineAndChannel (fd, ln, chan, smpteLine, line);

	//	For each ancillary packet...
	for (ULWord ancBytesRead = 0; ancBytesRead < ancBufSize; ancBytesRead += ancPacketCount)
	{
		//	Set offset into input buffer...
		pIn = (UByte *) pAncBuff + ancBytesRead;

		if (fbFormat == NTV2_FBF_10BIT_YCBCR || fbFormat == NTV2_FBF_8BIT_YCBCR)
		{
			//	Decompressed anc data...
			DecompressAncPacket (pIn, pAncUnpackedBuf, ancPacketCount, validLoc, chan, smpteLine);
			if (validLoc)
				line = GetVancLineOffset (fd, ln, smpteLine);

			//	Write anc data...
			chanOffset = (chan == kNTV2SMPTEAncChannel_Y) ? 1 : 0;
			embedded = embedded || InsertAnc (pAncUnpackedBuf, ancPacketCount, line, chanOffset, (ULWord *) pFrameBuffer, videoFormat, fbFormat);

			//	Increment default values...
			if (NextVancLineAndChannel (fd, ln, chan, smpteLine, line) == false)
				break;
		}
		else
		{
			embedded = false;
			break;
		}
	}

	return embedded;

}	//	EmbedCompressedAnc


// FindCompressedAnc()
// Take 8bit compressed anc bundle with possibly multiple anc packages (ProRes/DVCProHD specs) and return a pointer to a specific anc package specified by (DID/SDID)
bool CNTV2SMPTEAncData::FindCompressedAnc (UByte ancDID, UByte ancSDID, const UByte * pSrcAncBuf, const ULWord srcAncSize, UByteConstPtr & outBuffPtr, ULWord & outBufSize)
{
	const UByte *	pIn (pSrcAncBuf);
	UByte			DID (0), SDID (0);
	ULWord			DC	(0);

	if (pSrcAncBuf && srcAncSize)
	{
		//	While there is enough room to look at next anc package...
		while (pIn < (pSrcAncBuf + srcAncSize - 6))
		{
			DID		= pIn[3];
			SDID	= pIn[4];
			DC		= pIn[5];

			if (ancDID == DID && ancSDID == SDID)
			{
				outBuffPtr = pIn;
				outBufSize = DC + 6;
				return true;
			}

			pIn += DC + 6;
		}
	}	//	if pSrcAncBuff != NULL && srcAncSize > 0

	// not found
	outBuffPtr = NULL;
	outBufSize = 0;

	return false;

}	//	FindCompressedAnc


// DecompressAncPacket()
//Take 8bit compressed anc packet (ProRes/DVCProHD specs) and decompress into unpacket buffer
void CNTV2SMPTEAncData::DecompressAncPacket (const UByte * pInCompBuffer, UWord * pOutUnpackedBuffer, ULWord & outCompPacketSize,
											bool & outIsValidLoc, NTV2_SMPTEAncChannelSelect & outChan, ULWord & outSmpteLine)
{
	const UByte *	pIn		(pInCompBuffer);
	UWord *			pOut	(pOutUnpackedBuffer);

	if (!pIn || !pOut)
		return;

	//	See CompressAncPacket notes for explanation...
	UWord LN	= (pIn[0] & 0x7F) + ((pIn[1] & 0xF) << 7);
	UByte PF	= (pIn[0] >> 7) & 0x01;
	UByte YCF	= (pIn[1] >> 6) & 0x01;
	//UByte HF	= (pIn[1] >> 7) & 0x01; // this value is 1 for HANC, we only support VANC
	UByte CS8	= pIn[2] & 0x01;
	UByte DID	= pIn[3];
	UByte SDID	= pIn[4];
	UByte DC	= pIn[5];

	MakeAncHeader ((NTV2_SMPTEAncHeaderPtr) pOut, DID, SDID, DC);

	//	Move to start of data payload...
	pOut += 6;
	pIn += 6;

	//	Copy data...
	for (ULWord i = 0; i < DC; i++)
		*pOut++ = AddEvenParity (*pIn++);

	//	Last word -- reconstruct checksum...
	*pOut = *pIn + (CS8 ? 0x0100 : 0x0200);

	//	Output params...
	outCompPacketSize = 6 + DC + 1;
	outIsValidLoc = PF;
	if (PF)
	{
		outChan = YCF ? kNTV2SMPTEAncChannel_Y : kNTV2SMPTEAncChannel_C;
		outSmpteLine = LN;
	}

}	//	DecompressAncPacket


// CompressAncPacket()
//Take unpacked 16bit words (10 LSB are anc words) and pack them into 8bit words according to ProRes/DVCProHD specs (see below)
bool CNTV2SMPTEAncData::CompressAncPacket (const UWord * packetBuffer, UByte * compBuffer, ULWord maxCompSize, ULWord & outCompPacketSize, NTV2_SMPTEAncChannelSelect chan, UWord smpteLine)
{
	// Incoming data (packetBuffer) is unpacked into 16bit words, with the 10 LSB's of each word containing original anc data

	// Replace 10bit ancillary data flags (0x000, 0x3FF, 0x3FF) with the following compressed 8bit custom header
	// used in ProRes / DVCProHD

	//		   MSB <---------------------> LSB
	// Byte 0:	PF	L6	L5	L4	L3	L2	L1	L0
	// Byte 1:	HF YCF Res Res L10	L9	L8	L7
	// Byte 2: Res Res Res Res Res Res Res CS8


	// the rest of the header is 8bit version of 10bit anc words

	// Byte 3: DID (b0 - b7)
	// Byte 4: SDID (bit0 - bit7)
	// Byte 5: DC (bit0 - bit7)
	// Byte (6-261): 255 byte max, user data
	// Byte Last: CS (bit0 - bit7)

	// Where...
	// PF: 1 bit - set to 1 if LN, YCF are valid/usable, always set to 1
	// LN: 11 bit - SMPTE line number containing vanc packet
	// HF: 1 bit - set to 1 for HANC, 0 for VANC. Since we only use VANC this is always set to 0
	// YCF: 1 bit - set to 1 for Lum channel, 0 for chrom channel
	// CS8: 1 bit - bit 8 of CS of originating ancilary packet
	// DID: Data ID
	// SDID: Secondary Data ID
	// DC: Data Count (0-255)
	// CS: Check Sum

	UByte * pOut		(compBuffer);
	UByte	DC			((UByte) (packetBuffer [5] & 0x00FF));
	ULWord	wordCount	(6 + DC + 1);
	UWord	CS			(packetBuffer [wordCount - 1]);

	if (wordCount > maxCompSize)
	{
		outCompPacketSize = 0;
		return false;																		// not enough room
	}

	pOut[0] = 0x80 + (smpteLine & 0x3F);													// Byte 0: Header 0
	pOut[1] = (chan == kNTV2SMPTEAncChannel_Y ? 0x40 : 0x00) + ((smpteLine >> 7) & 0x0F);	// Byte 1: Header 1
	pOut[2] = (UByte) ((CS >> 8) & 0x0001);													// Byte 2: Header 2
	pOut[3] = (UByte) (packetBuffer[3] & 0x00FF);											// Byte 3: DID
	pOut[4] = (UByte) (packetBuffer[4] & 0x00FF);											// Byte 4: SDID
	pOut[5] = DC;																			// Byte 5: DC

	//	Move to start of data payload...
	pOut += 6;

	//	Copy data...
	for (UWord i = 0; i < DC; i++)															// Byte 6-261: user data (variable, 255 max)
		*pOut++ = (UByte) (packetBuffer [6 + i] & 0x00FF);

	*pOut++ = (UByte) (CS & 0x00FF);														// Byte Last: CS (bit0-bit7)

	outCompPacketSize = wordCount;															// size

	return true;																			// success

}	//	CompressAncPacket


// FirstVancLineAndChannel()
// determine first line numbers and channel when incrementing throught vanc data
// (see NextVancLineAndChannel for details)
//
bool CNTV2SMPTEAncData::FirstVancLineAndChannel (const NTV2FormatDescriptor &	inFD,
												const NTV2SmpteLineNumber &		inLN,
												NTV2_SMPTEAncChannelSelect &	outChannel,
												ULWord &						outSMPTELine,
												ULWord &						outLineOffset)
{
	const bool	isInterlaced	((inLN.smpteFirstActiveLine + 1) != inLN.smpteSecondActiveLine);

	outChannel = kNTV2SMPTEAncChannel_Y;

	if (isInterlaced)
	{
		outSMPTELine = inLN.smpteFirstActiveLine - (inFD.firstActiveLine / 2);
		outLineOffset = inLN.firstFieldTop ? 0 : 1;
	}
	else
	{
		outSMPTELine = inLN.smpteFirstActiveLine - inFD.firstActiveLine;
		outLineOffset = 0;
	}

	return true;

}	//	FirstVancLineAndChannel


// NextVancLineAndChannel()
// increment channel, line in VANC, return true if valid
// channel incrments Y->C->Y->C, lines increment in smpte line order, line numbers are traversed in smpte line number order, earliest first.
//
// Example for 1080i
// fd (i)				format desc
// ln (i)				smpte line number
// chan (i/o)			luminance / chrominance				Y, C, Y, C,.. C		Y,	 C,	  Y,   C,..	  C
// smpteLine (i/o)		smpte line number					5, 5, 6, 6,.. 20, 568, 568, 569, 569,.. 583
// lineOffset (o)		line offset in the frame buffer		0, 0, 2, 2,.. 30,	1,	 1,	  3,   3,..	 31
//
bool CNTV2SMPTEAncData::NextVancLineAndChannel (const NTV2FormatDescriptor &	inFD,
												const NTV2SmpteLineNumber &		inLN,
												NTV2_SMPTEAncChannelSelect &	inOutChannel,
												ULWord &						inOutSMPTELine,
												ULWord &						outLineOffset)
{
	bool	nextValid	(true);
	bool	interlaced	((inLN.smpteFirstActiveLine + 1) != inLN.smpteSecondActiveLine);
	ULWord	fieldHeight (interlaced ? inFD.firstActiveLine / 2 : inFD.firstActiveLine);

	ULWord	firstVancFieldStart		(inLN.smpteFirstActiveLine - fieldHeight);
	ULWord	secondVancFieldStart	(inLN.smpteSecondActiveLine - fieldHeight);

	//	Always increment channel...
	inOutChannel = (inOutChannel == kNTV2SMPTEAncChannel_Y) ? kNTV2SMPTEAncChannel_C : kNTV2SMPTEAncChannel_Y;

	//	Increment line if channel is lum...
	if (inOutChannel == kNTV2SMPTEAncChannel_Y)
	{
		if (interlaced)
		{
			//	First field...
			if (inOutSMPTELine >= firstVancFieldStart  &&  inOutSMPTELine < inLN.smpteFirstActiveLine)
			{
				inOutSMPTELine = inOutSMPTELine + 1;
				if (inOutSMPTELine == inLN.smpteFirstActiveLine)
				{
					inOutSMPTELine = secondVancFieldStart;
					outLineOffset = inLN.firstFieldTop ? 1 : 0;
				}
				else
					outLineOffset = (inLN.firstFieldTop ? 0 : 1) + (inOutSMPTELine - firstVancFieldStart) * 2;
			}	//	if first field
			else if (inOutSMPTELine >= secondVancFieldStart	 &&	 inOutSMPTELine < inLN.smpteSecondActiveLine)
			{
				inOutSMPTELine = inOutSMPTELine + 1;
				if (inOutSMPTELine < inLN.smpteSecondActiveLine)
					outLineOffset = (inLN.firstFieldTop ? 1 : 0) + (inOutSMPTELine - secondVancFieldStart) * 2;
				else
					nextValid = false;
			}	//	else if second field
			else
				nextValid = false;	//	Invalid
		}
		// progressive
		else
		{
			if (inOutSMPTELine >= firstVancFieldStart  &&  inOutSMPTELine < inLN.smpteFirstActiveLine)
			{
				inOutSMPTELine = inOutSMPTELine + 1;
				if (inOutSMPTELine < inLN.smpteFirstActiveLine)
					outLineOffset = inOutSMPTELine - firstVancFieldStart;
				else
					nextValid = false;
			}
			else
				nextValid = false;
		}	//	else progressive
	}	//	if luminence channel

	return nextValid;

}	//	NextVancLineAndChannel


// GetVancLineOffset()
// convert smpte line number to VANC frame buffer line offset
ULWord CNTV2SMPTEAncData::GetVancLineOffset (const NTV2FormatDescriptor & inFormatDesc, const NTV2SmpteLineNumber & inSmpteLineNumbers, const ULWord inSmpteLine)
{
	ULWord	lineOffset	(0);
	bool	interlaced	((inSmpteLineNumbers.smpteFirstActiveLine + 1) != inSmpteLineNumbers.smpteSecondActiveLine);
	ULWord	fieldHeight (interlaced ? inFormatDesc.firstActiveLine / 2 : inFormatDesc.firstActiveLine);

	ULWord	firstVancFieldStart		(inSmpteLineNumbers.smpteFirstActiveLine - fieldHeight);
	ULWord	secondVancFieldStart	(inSmpteLineNumbers.smpteSecondActiveLine - fieldHeight);

	if (interlaced)
	{
		if (inSmpteLine >= firstVancFieldStart && inSmpteLine < inSmpteLineNumbers.smpteFirstActiveLine)
		{
			if (inSmpteLine == inSmpteLineNumbers.smpteFirstActiveLine)
				lineOffset = inSmpteLineNumbers.firstFieldTop ? 1 : 0;
			else
				lineOffset = (inSmpteLineNumbers.firstFieldTop ? 0 : 1) + (inSmpteLine - firstVancFieldStart) * 2;
		}	//	if first field
		else if (inSmpteLine >= secondVancFieldStart && inSmpteLine < inSmpteLineNumbers.smpteSecondActiveLine)
		{
			if (inSmpteLine < inSmpteLineNumbers.smpteSecondActiveLine)
				lineOffset = (inSmpteLineNumbers.firstFieldTop ? 1 : 0) + (inSmpteLine - secondVancFieldStart) * 2;
		}	//	else if second field
	}	//	if interlaced
	else
	{
		if (inSmpteLine >= firstVancFieldStart && inSmpteLine < inSmpteLineNumbers.smpteFirstActiveLine)
		{
			if (inSmpteLine < inSmpteLineNumbers.smpteFirstActiveLine)
				lineOffset = inSmpteLine - firstVancFieldStart;
		}
	}	//	else progressive

	return lineOffset;

}	//	GetVancLineOffset


// InsertAnc - insert anc data at specified frame buffer line offset, channel offset
bool CNTV2SMPTEAncData::InsertAnc (const UWord * pInAncBuff, const size_t inAncWordCount, const ULWord inLineNumber, const ULWord inWordOffset,
									ULWord * pFrameBuffer, const NTV2VideoFormat inVideoFormat, const NTV2FrameBufferFormat inFBFormat)
{
	bool					bResult			(true);
	NTV2Standard			standard		(::GetNTV2StandardFromVideoFormat (inVideoFormat));
	NTV2FormatDescriptor	fd				(standard, inFBFormat, NTV2_VANCMODE_TALL);

	//	Find the beginning of the line we want to insert into...
	ULWord *				pFBLine			(pFrameBuffer + (inLineNumber * fd.linePitch));

	//	Copy/translate line into UWord line buffer...
	ULWord					wordsPerPixel	(1);
	UWord *					pLineBuffer		(new UWord [fd.numPixels * 4]);

	if (pLineBuffer)
	{
		switch (inFBFormat)
		{
			case NTV2_FBF_10BIT_YCBCR:
				::UnpackLine_10BitYUVto16BitYUV (pFBLine, pLineBuffer, fd.numPixels);	//	Formerly UnPackAndShiftAnc10BitLineData
				wordsPerPixel = 2;
				break;

			//	Theoretically you can't use an 8-bit pixel format to carry Anc data because Anc data uses all 10 bits.
			//	However, thanks to the AJA 8-bit VANC format, we can take the ls 8 bits of the VANC data and insert
			//	it into the 8-bit video and extract it at the other end. To do that here, we're going to "unpack"
			//	the 8-bit video to the ls 8-bits of the line buffer, then insert the incoming 10-bit ANC data (which
			//	will line up the ls 8bits of the VANC data with the 8-bits of the video data), then "pack" the ls
			//	8-bits of the line buffer back to the frame buffer. Note: we're assuming that the incoming VANC data
			//	uses the full "0x000-0x3ff-0x3ff" preamble so the ls 8 bits are still "00-FF-FF".
			//
			case NTV2_FBF_8BIT_YCBCR:
			{
				UByte * pFB8 = reinterpret_cast <UByte *> (pFBLine);
				for (ULWord i = 0; i < (fd.numPixels * 2); i++)
					pLineBuffer [i] = static_cast <UWord> (pFB8 [i]);	//	8-bit video ends up in ls 8 bits of line buffer
				wordsPerPixel = 2;
				break;
			}

			default:
				bResult = false;
				break;
		}	//	switch on inFBFormat

		//	Find location in line buffer to begin insert...
		UWord *pInsert = pLineBuffer + inWordOffset;

		//	Copy supplied ANC data into line buffer...
		if (bResult && ( (inWordOffset + (2 * inAncWordCount)) < (fd.numPixels * wordsPerPixel) ) )
		{
			for (size_t ancWordNdx = 0; ancWordNdx < inAncWordCount; ancWordNdx++)
			{
				*pInsert = (*pInAncBuff++) & 0x3ff;
				pInsert += 2;
			}
		}

		//	Translate and replace line into original Frame Buffer...
		switch (inFBFormat)
		{
			case NTV2_FBF_10BIT_YCBCR:
				::PackLine_16BitYUVto10BitYUV (pLineBuffer, pFBLine, fd.numPixels); //	Formerly PackAnc10BitLineData
				break;

			case NTV2_FBF_8BIT_YCBCR:
			{
				UByte * pFB8 = (UByte *) pFBLine;
				for (ULWord i = 0; i < (fd.numPixels * 2); i++)
					pFB8 [i] = static_cast <UByte> (pLineBuffer [i] & 0xff);	//	LS 8 bits of line buffer goes back to frame buffer
				break;
			}

			default:
				bResult = false;
				break;
		}	//	switch on inFBFormat

		delete [] pLineBuffer;
	}
	else
		bResult = false;

	return bResult;

}	//	InsertAnc


// InsertAncAtSmpteLine()
// same as InsertAnc - alternately takes smpte line number instead of a frame buffer line offset
bool CNTV2SMPTEAncData::InsertAncAtSmpteLine (const UWord *					pInAncBuff,
												const ULWord				inAncWordCount,
												const ULWord				inSMPTELineNum,
												const ULWord				inWordOffset,
												ULWord *					pFrameBuffer,
												const NTV2VideoFormat		inVideoFormat,
												const NTV2FrameBufferFormat inFBFormat)
{
	NTV2Standard			standard	(GetNTV2StandardFromVideoFormat (inVideoFormat));
	NTV2FormatDescriptor	formatDesc	(standard, inFBFormat, NTV2_VANCMODE_TALL);
	NTV2SmpteLineNumber		lineNumbers (GetSmpteLineNumber (standard));
	ULWord					vancLineNum (GetVancLineOffset (formatDesc, lineNumbers, inSMPTELineNum));
	const bool				embedded	(InsertAnc (pInAncBuff, inAncWordCount, vancLineNum, inWordOffset, pFrameBuffer, inVideoFormat, inFBFormat));

	return embedded;

}	//	InsertAncAtSmpteLine


ULWord CNTV2SMPTEAncData::GetCaptionAncLineNumber (const NTV2VideoFormat inVideoFormat, const bool inIsField1)
{
	switch (inVideoFormat)
	{
		case NTV2_FORMAT_525_2398:
		case NTV2_FORMAT_525_2400:
		case NTV2_FORMAT_525_5994:				return inIsField1 ? 11 : 274;

		case NTV2_FORMAT_525psf_2997:			return 14;

		case NTV2_FORMAT_625_5000:				return inIsField1 ? 8 : 321;
		case NTV2_FORMAT_625psf_2500:			return 8;

		case NTV2_FORMAT_720p_5000:
		case NTV2_FORMAT_720p_5994:
		case NTV2_FORMAT_720p_6000:
		case NTV2_FORMAT_720p_2398:
		case NTV2_FORMAT_720p_2500:				return 9;

		case NTV2_FORMAT_1080i_5000:
		case NTV2_FORMAT_1080i_5994:
		case NTV2_FORMAT_1080i_6000:			return inIsField1 ? 9 : 572;

		case NTV2_FORMAT_1080psf_2398:
		case NTV2_FORMAT_1080psf_2400:
		case NTV2_FORMAT_1080psf_2500_2:
		case NTV2_FORMAT_1080psf_2997_2:
		case NTV2_FORMAT_1080psf_3000_2:
		case NTV2_FORMAT_1080p_2997:
		case NTV2_FORMAT_1080p_3000:
		case NTV2_FORMAT_1080p_2500:
		case NTV2_FORMAT_1080p_2398:
		case NTV2_FORMAT_1080p_2400:
		case NTV2_FORMAT_1080p_5000_A:
		case NTV2_FORMAT_1080p_5994_A:
		case NTV2_FORMAT_1080p_6000_A:
		case NTV2_FORMAT_1080p_5000_B:
		case NTV2_FORMAT_1080p_5994_B:
		case NTV2_FORMAT_1080p_6000_B:			return 10;

		case NTV2_FORMAT_1080p_2K_2398:
		case NTV2_FORMAT_1080p_2K_2400:
		case NTV2_FORMAT_1080p_2K_2500:
		case NTV2_FORMAT_1080p_2K_2997:
		case NTV2_FORMAT_1080p_2K_3000:
		case NTV2_FORMAT_1080psf_2K_2398:
		case NTV2_FORMAT_1080psf_2K_2400:
		case NTV2_FORMAT_1080psf_2K_2500:		return 8;

		case NTV2_FORMAT_1080p_2K_4795_B:
		case NTV2_FORMAT_1080p_2K_4800_B:
		case NTV2_FORMAT_1080p_2K_5000_B:
		case NTV2_FORMAT_1080p_2K_5994_B:
		case NTV2_FORMAT_1080p_2K_6000_B:		break;		//	ZERO??

		case NTV2_FORMAT_2K_1498:
		case NTV2_FORMAT_2K_1500:
		case NTV2_FORMAT_2K_2398:
		case NTV2_FORMAT_2K_2400:
		case NTV2_FORMAT_2K_2500:				return inIsField1 ? 8 : 998;

		case NTV2_FORMAT_4x1920x1080psf_2398:
		case NTV2_FORMAT_4x1920x1080psf_2400:
		case NTV2_FORMAT_4x1920x1080psf_2500:
		case NTV2_FORMAT_4x1920x1080psf_2997:
		case NTV2_FORMAT_4x1920x1080psf_3000:
		case NTV2_FORMAT_4x1920x1080p_2398:
		case NTV2_FORMAT_4x1920x1080p_2400:
		case NTV2_FORMAT_4x1920x1080p_2500:
		case NTV2_FORMAT_4x1920x1080p_2997:
		case NTV2_FORMAT_4x1920x1080p_3000:
		case NTV2_FORMAT_4x2048x1080psf_2398:
		case NTV2_FORMAT_4x2048x1080psf_2400:
		case NTV2_FORMAT_4x2048x1080psf_2500:
		case NTV2_FORMAT_4x2048x1080psf_2997:
		case NTV2_FORMAT_4x2048x1080psf_3000:
		case NTV2_FORMAT_4x2048x1080p_2398:
		case NTV2_FORMAT_4x2048x1080p_2400:
		case NTV2_FORMAT_4x2048x1080p_2500:
		case NTV2_FORMAT_4x2048x1080p_2997:
		case NTV2_FORMAT_4x2048x1080p_3000:
		case NTV2_FORMAT_4x1920x1080p_5000:
		case NTV2_FORMAT_4x1920x1080p_5994:
		case NTV2_FORMAT_4x1920x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_4795:
		case NTV2_FORMAT_4x2048x1080p_4800:
		case NTV2_FORMAT_4x2048x1080p_5000:
		case NTV2_FORMAT_4x2048x1080p_5994:
		case NTV2_FORMAT_4x2048x1080p_6000:
		case NTV2_FORMAT_4x2048x1080p_11988:
		case NTV2_FORMAT_4x2048x1080p_12000:
		case NTV2_FORMAT_3840x2160psf_2398:
		case NTV2_FORMAT_3840x2160psf_2400:
		case NTV2_FORMAT_3840x2160psf_2500:
		case NTV2_FORMAT_3840x2160p_2398:
		case NTV2_FORMAT_3840x2160p_2400:
		case NTV2_FORMAT_3840x2160p_2500:
		case NTV2_FORMAT_3840x2160p_2997:
		case NTV2_FORMAT_3840x2160p_3000:
		case NTV2_FORMAT_3840x2160psf_2997:
		case NTV2_FORMAT_3840x2160psf_3000:
		case NTV2_FORMAT_3840x2160p_5000:
		case NTV2_FORMAT_3840x2160p_5994:
		case NTV2_FORMAT_3840x2160p_6000:
		case NTV2_FORMAT_4096x2160psf_2398:
		case NTV2_FORMAT_4096x2160psf_2400:
		case NTV2_FORMAT_4096x2160psf_2500:
		case NTV2_FORMAT_4096x2160p_2398:
		case NTV2_FORMAT_4096x2160p_2400:
		case NTV2_FORMAT_4096x2160p_2500:
		case NTV2_FORMAT_4096x2160p_2997:
		case NTV2_FORMAT_4096x2160p_3000:
		case NTV2_FORMAT_4096x2160psf_2997:
		case NTV2_FORMAT_4096x2160psf_3000:
		case NTV2_FORMAT_4096x2160p_4795:
		case NTV2_FORMAT_4096x2160p_4800:
		case NTV2_FORMAT_4096x2160p_5000:
		case NTV2_FORMAT_4096x2160p_5994:
		case NTV2_FORMAT_4096x2160p_6000:
		case NTV2_FORMAT_4096x2160p_11988:
		case NTV2_FORMAT_4096x2160p_12000:			return 10;

		default:
		case NTV2_FORMAT_UNKNOWN:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS:
		case NTV2_FORMAT_END_STANDARD_DEF_FORMATS:
		case NTV2_FORMAT_END_2K_DEF_FORMATS:
		case NTV2_FORMAT_END_HIGH_DEF_FORMATS2: break;
	}
	return 0;

}	//	GetCaptionAncLineNumber
