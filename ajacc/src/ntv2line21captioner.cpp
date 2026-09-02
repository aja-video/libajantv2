/**
	@file		ntv2line21captioner.cpp
	@brief		Implementation of the CNTV2Line21Captioner class.
	@copyright	(C) 2005-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2line21captioner.h"
#include "ntv2publicinterface.h"
#include "ajabase/system/debug.h"
#include <sstream>
#include <string.h>	//	for memset
#if defined (_DEBUG)
	#include <stdio.h>	//	for fopen, etc.
#endif


using namespace std;


static unsigned	gInstanceTally	(0);


/////////////////////////////////////////////////////////////////////////////	CNTV2Line21Captioner

#define LOGENCWARN(__xpr__)		AJA_sWARNING(AJA_DebugUnit_CCLine21Encode, AJAFUNC << ": " << GetLogLabel() << ": " << __xpr__)
#define LOGENCNOTE(__xpr__)		AJA_sNOTICE(AJA_DebugUnit_CCLine21Encode, AJAFUNC << ": " << GetLogLabel() << ": " << __xpr__)
#define LOGENCINFO(__xpr__)		AJA_sINFO(AJA_DebugUnit_CCLine21Encode, AJAFUNC << ": " << GetLogLabel() << ": " << __xpr__)
#define LOGENCDBG(__xpr__)		AJA_sDEBUG(AJA_DebugUnit_CCLine21Encode, AJAFUNC << ": " << GetLogLabel() << ": " << __xpr__)

#define LOGDECWARN(__xpr__)		AJA_sWARNING(AJA_DebugUnit_CCLine21Decode, AJAFUNC << ": " << __xpr__)
#define LOGDECNOTE(__xpr__)		AJA_sNOTICE(AJA_DebugUnit_CCLine21Decode, AJAFUNC << ": " << __xpr__)
#define LOGDECINFO(__xpr__)		AJA_sINFO(AJA_DebugUnit_CCLine21Decode, AJAFUNC << ": " << __xpr__)
#define LOGDECDBG(__xpr__)		AJA_sDEBUG(AJA_DebugUnit_CCLine21Decode, AJAFUNC << ": " << __xpr__)


/**
	NOTE!!
	This implementation makes a simplifying assumption that all captioning bits are 27
	pixels wide. This is a cheat. The actual bit duration should be (H / 32), which for
	NTSC video is 858 pixels / 32 = 26.8125 pixels per bit. This difference should be
	within the tolerance of most captioning receivers, but beware -- we may find that in
	practice we need more precision someday. (Note that in PAL the line width is 864 pixels,
	which is exactly 27.0 pixels/bit).
**/

static const unsigned int	CC_DEFAULT_ENCODE_OFFSET	(7);	//	Number of pixels between start of buffer and beginning of first CC bit-cell

static const unsigned int	CC_BIT_WIDTH	(27);		//	= round (858 / 32)
static const UByte			CC_LEVEL_LO		(16);		//	'0' bit level
static const UByte			CC_LEVEL_HI		(126);		//	'1' bit level
static const UByte			CC_LEVEL_MID	(71);		//	"slice" level -- mid-way between '0' and '1'
static const UByte			CC_LEVEL_TRANS	(20);


/**
	Funny math:	There are exactly 7 cycles of Clock Run-In, which is an inverted cosine wave.
	Each cycle is the same duration as one CC bit (27 pixels). HOWEVER, the CC bit edges are
	supposed to be coincident with the 50% point of the trailing edge of each clock cycle
	think of it as 270 degrees through the cycle), while the actual clock run-in cycles are
	generated from 0 degrees to 0 degrees. So the first clock cycle begins 90 degrees into the
	first "bit", and the last clock cycle draws its last 90 degrees overlapping into the first
	zero start bit. This value is the "length" of this 90 degree offset in pixels.
**/
static const unsigned int	CLOCK_RUN_IN_OFFSET	(7);		//	Number of pixels in 90 degrees

/**
	The transition time between low and high ("rise-time") and high to low ("fall time") is
	three samples. Since bit cells start and end at the 50% points, some of the transition
	samples belong to the previous bit, and some belong to the next bit.
**/
static const unsigned int	TRANSITION_PRE		(1);		//	One sample of the transition "belongs" to the previous bit
static const unsigned int	TRANSITION_POST		(2);		//	Two samples of the transition "belong" to the next bit
static const unsigned int	TRANSITION_WIDTH	(TRANSITION_PRE + TRANSITION_POST);

//	1 cycle of the Line 21 Clock Run-In (see note in header file about freq. approximation)
static const UByte			cc_clock [27]	=	{ 16,  17,  22,  29,  38,  49,  61,  74,  86,  98, 108, 116, 122, 125,
									  			 125, 122, 116, 108,  98,  86,  74,  61,  49,  38,  29,  22,  17	};

//	4 possible transition types:
//	Low -> Low
static const UByte			cc_trans_lo_lo [TRANSITION_WIDTH]	= { CC_LEVEL_LO,						CC_LEVEL_LO,	CC_LEVEL_LO						};
//	Low -> High
static const UByte			cc_trans_lo_hi [TRANSITION_WIDTH]	= { CC_LEVEL_LO + CC_LEVEL_TRANS,		CC_LEVEL_MID,	CC_LEVEL_HI - CC_LEVEL_TRANS	};
//	High -> Low
static const UByte			cc_trans_hi_lo [TRANSITION_WIDTH]	= { CC_LEVEL_HI - CC_LEVEL_TRANS,		CC_LEVEL_MID,	CC_LEVEL_LO + CC_LEVEL_TRANS	};
//	High -> High
static const UByte			cc_trans_hi_hi [TRANSITION_WIDTH]	= { CC_LEVEL_HI,						CC_LEVEL_HI,	CC_LEVEL_HI						};


//	Encodes from the beginning of the transition of one bit ("from") to the next ("to").
//	Returns the pointer to the next pixel following the transition.
static UByte * EncodeTransition (UByte * pPixelBuffer, const int inFromBit, const int inToBit)
{
	const UByte * pTrans (NULL);
	
	//	Which kind of transition are we talking about?
	if (inFromBit == 0 && inToBit == 0)
		pTrans = cc_trans_lo_lo;
	else if (inFromBit == 0 && inToBit != 0)
		pTrans = cc_trans_lo_hi;
	else if (inFromBit != 0 && inToBit == 0)
		pTrans = cc_trans_hi_lo;
	else
		pTrans = cc_trans_hi_hi;

	for (unsigned int i(0);  i < TRANSITION_WIDTH;  i++)
	{
		*pPixelBuffer++ = 0x80;
		*pPixelBuffer++ = pTrans [i];
	}
	
	return pPixelBuffer;

}	//	EncodeTransition


// Constructor
CNTV2Line21Captioner::CNTV2Line21Captioner (void)
	:	mEncodeBufferInitialized	(false),
		mEncodePixelOffset			(0),
		mEncodeFirstDataBitOffset	(0)
{
	::memset (mEncodeBuffer, 0, sizeof (mEncodeBuffer));
	ostringstream	oss;
	oss << "CNTV2Line21Captioner-" << ++gInstanceTally;
	SetLogLabel(oss.str());

}	//	constructor


CNTV2Line21Captioner::~CNTV2Line21Captioner ()
{
}	//	destructor
	

//	Initialize a prototype Line 21 buffer with the parts that DON'T change:
//	i.e. the Clock Run-In and the Start bits.
void CNTV2Line21Captioner::InitEncodeBuffer (void)
{
	unsigned int i, j;
	UByte *	ptr	(mEncodeBuffer);
	
	mEncodePixelOffset = CC_DEFAULT_ENCODE_OFFSET;	//	Offset here is in pixels, NOT bytes (1 pixel = 2 bytes)
	
	//	Fill Black until beginning of Clock Run-In...
	//	Both the default offset to the first bit-cell, plus the "missing" quarter-cycle of the clock...
	for (i = 0;  i < (CC_DEFAULT_ENCODE_OFFSET + CLOCK_RUN_IN_OFFSET);  i++)
	{
		*ptr++ = 0x80;		//	chroma sample
		*ptr++ = 0x10;		//	luma sample (Black)
	}
	
	//	7 cycles of 503,496 Hz clock run-in...
	for (j = 0;  j < 7;  j++)
	{
		for (i = 0;  i < CC_BIT_WIDTH;  i++)
		{
			*ptr++ = 0x80;			//	chroma sample
			*ptr++ = cc_clock[i];	//	clock
		}
	}

	//	Start bit: 1 CC bits of '0' (reduced width because the last cycle of the clock run-in overlaps)...
	for (i = 0;  i < (CC_BIT_WIDTH - CLOCK_RUN_IN_OFFSET);  i++)
	{
		*ptr++ = 0x80;
		*ptr++ = CC_LEVEL_LO;
	}

	//	Start bit: 1 CC bit of '0' (full width)...
	for (i = 0;  i < CC_BIT_WIDTH - TRANSITION_POST;  i++)
	{
		*ptr++ = 0x80;
		*ptr++ = CC_LEVEL_LO;
	}

	//	Encode transition between low and high...
	ptr = EncodeTransition (ptr, 0, 1);
	
	//	Start bit: 1 CC bit of '1'...
	for (i = 0;  i < CC_BIT_WIDTH - TRANSITION_PRE;  i++)
	{
		*ptr++ = 0x80;
		*ptr++ = CC_LEVEL_HI;
	}

	//	Fill in black for the rest of the line -- this will be overwritten with "real" data bits later...
	UByte *	lastAddr	(mEncodeBuffer + (2 * CC_LINE_WIDTH_PIXELS));
	while (ptr < lastAddr)
	{
		*ptr++ = 0x80;	//	chroma
		*ptr++ = 0x10;	//	luma
	}

	//	The first data bit cell starts 10 bit cells after the initial offset
	//	(note: does NOT include slop for needed rise-time)
	mEncodeFirstDataBitOffset = mEncodePixelOffset + (10 * CC_BIT_WIDTH);

	mEncodeBufferInitialized = true;

}	//	InitEncodeBuffer


//	Encodes the supplied two bytes into the existing encode buffer and returns a pointer to same.
//	NOTE:	Caller should NOT modify the returned buffer in any way.
UByte * CNTV2Line21Captioner::EncodeLine (const UByte char1, const UByte char2)
{
	if (!mEncodeBufferInitialized)
		InitEncodeBuffer ();

	if (AJADebug::IsActive(AJA_DebugUnit_CCLine21Encode))
	{
		ostringstream	oss;
		for (unsigned ndx (0);  ndx < CC_LINE_WIDTH_PIXELS * 2;  ndx++)
			oss << UHEX2(mEncodeBuffer[ndx]);
		LOGENCINFO("BEFORE:  " << oss.str());
	}

	//	Pointer to first data bit, minus room for transition
	UByte *	ptr	(mEncodeBuffer + ((mEncodeFirstDataBitOffset - TRANSITION_PRE) * 2));

	//	Encode transition from last start bit to first bit of first character...
	ptr = EncodeTransition (ptr, 1, (char1 & 0x01));

	//	Encode first byte...
	ptr = EncodeCharacter (ptr, char1);

	//	Encode transition between characters...
	ptr = EncodeTransition (ptr, (char1 & 0x80), (char2 & 0x01));

	//	Encode second byte...
	ptr = EncodeCharacter (ptr, char2);

	//	Encode final transition...
	ptr = EncodeTransition (ptr, (char2 & 0x80), 0);

	if (AJADebug::IsActive(AJA_DebugUnit_CCLine21Encode))
	{
		ostringstream	oss;
		for (unsigned ndx (0);  ndx < CC_LINE_WIDTH_PIXELS * 2;  ndx++)
			oss << UHEX2(mEncodeBuffer[ndx]);
		LOGENCINFO("AFTER:  " << oss.str());
	}

	//	Return pointer to the first byte of my encode buffer...
	return mEncodeBuffer;

}	//	EncodeLine



//	Encodes from the end of the transition to the first bit until the start of the transition from the last bit.
//	Returns the pointer to the next pixel following the character (i.e. the beginning of the transition to the next character)
//	NOTE:	The MSB is supposed to be odd parity for the LS 7 bits. It's up to the caller to make this so.

UByte * CNTV2Line21Captioner::EncodeCharacter (UByte * ptr, const UByte byte)
{
	char	mask	(1);

	//	Do all 8 bits...
	for (int j (0);  j < 8;  j++)
	{
		//	Do the constant samples
		UByte level = ((byte & mask) == 0) ? CC_LEVEL_LO : CC_LEVEL_HI;

		for (unsigned int i (0);  i < (CC_BIT_WIDTH - TRANSITION_WIDTH);  i++)
		{
			*ptr++ = 0x80;		//	chroma
			*ptr++ = level;		//	luma
		}

		//	Do the transition (except following the last bit)
		char nextMask = mask << 1;

		if (j < 7)
			ptr = EncodeTransition (ptr, (byte & mask), (byte & nextMask));

		mask = nextMask;
	}	//	for each of the 8 bits

	return ptr;

}	//	EncodeCharacter


const UByte * CNTV2Line21Captioner::FindFirstDataBit_NTSC (const void * pIn2VUYLine)
{
	const UByte *	pInVideoLine	(reinterpret_cast<const UByte*>(pIn2VUYLine));
	ostream &		output			(::GetDefaultCaptionLogOutputStream());
	const UByte *	pFirstYSample	(pInVideoLine + 1);	//	Point to first luma sample in line
	const UByte *	pFirstClockEdge	(AJA_NULL);
	const UByte *	pLastClockEdge	(AJA_NULL);
	const UByte *	pFirstDataBit	(AJA_NULL);
	const unsigned	bytesPerPixel	(2);
	const unsigned	numClockCycles	(7);
	if (!pIn2VUYLine)
		return AJA_NULL;

	/**
		The rising edge of the first clock run-in cycle should happen approx 10.5 usecs from the leading
		edge of sync, which translates to approximately 20 pixels from the left-hand edge of active video.
		However, the tolerance on this value is +/- 500 nsec, or +/- 7 pixels, so we must programmatically
		find the first leading clock edge and use that as our reference.
	**/

	//	Start looking for a low->high transition starting at pixel 10.
	//	Give up if we haven't found it by pixel 30...
	unsigned pixelNdx(0),  startPixelNdx(10),  stopPixelNdx(30);
	for (pixelNdx = startPixelNdx;  pixelNdx < stopPixelNdx;  pixelNdx++)
		if (pFirstYSample [pixelNdx * bytesPerPixel] < CC_LEVEL_MID)
			if (pFirstYSample [(pixelNdx + 1) * bytesPerPixel] >= CC_LEVEL_MID)
				break;

	if (pixelNdx >= stopPixelNdx)
	{
		if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectFail)
		{
			CNTV2CaptionLogConfig::DumpYBytes_2vuy (pInVideoLine, output, 0, 50, true, 10, 30);
			output << "## WARNING:  FindFirstDataBit_NTSC:  Missing initial clock edge, luma transition from < "
						<< xHEX0N(uint16_t(CC_LEVEL_MID),2) << " to >= " << xHEX0N(uint16_t(CC_LEVEL_MID),2)
						<< " from pixels 10 thru 30" << endl;
		}
		return AJA_NULL;
	}

	pFirstClockEdge = &pFirstYSample [pixelNdx * bytesPerPixel];
	if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectSuccess)
	{
		CNTV2CaptionLogConfig::DumpYBytes_2vuy (pInVideoLine, output, 0, 50, true, pixelNdx, pixelNdx + 1);
		output << "## NOTE:  FindFirstDataBit_NTSC:  Found first clock edge at pixel " << pixelNdx << endl;
	}

	//	If this is the leading edge of the first sine wave, then the crest of this wave will
	//	be approx 7 pixels from here, and the trough of this wave will be approx 13 clocks after that.
	//	This pattern will repeat every 27 pixels for 7 cycles...
	for (unsigned cycleNum = 0;  cycleNum < numClockCycles;  cycleNum++)
	{
		const unsigned int	hi_pixel	((cycleNum * CC_BIT_WIDTH) + 7);
		const unsigned int	lo_pixel	((cycleNum * CC_BIT_WIDTH) + 20);

		if ((pFirstClockEdge [hi_pixel * bytesPerPixel] < CC_LEVEL_MID) || (pFirstClockEdge [lo_pixel * bytesPerPixel] >= CC_LEVEL_MID))
		{
			//	Failed to find an expected clock crest or trough -- abort
			if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectFail)
			{
				CNTV2CaptionLogConfig::DumpYBytes_2vuy (pInVideoLine, output, hi_pixel - CC_BIT_WIDTH, lo_pixel + CC_BIT_WIDTH, true, hi_pixel, lo_pixel);
				if (pFirstClockEdge [hi_pixel * bytesPerPixel] < CC_LEVEL_MID)
					output	<< "## ERROR:  FindFirstDataBit_NTSC:  Missing clock sine crest at cycle " << cycleNum << " (pixel " << hi_pixel
							<< "), expected at least " << xHEX0N(uint16_t(CC_LEVEL_MID),2) << ", instead got "
							<< xHEX0N(uint16_t(pFirstClockEdge[hi_pixel * bytesPerPixel]),2) << endl;
				else
					output	<< "## ERROR:  FindFirstDataBit_NTSC:  Missing clock sine trough at cycle " << cycleNum << " (pixel " << lo_pixel
							<< "), expected less than " << xHEX0N(uint16_t(CC_LEVEL_MID),2) << ", instead got "
							<< xHEX0N(uint16_t(pFirstClockEdge[lo_pixel * bytesPerPixel]),2) << endl;
			}
			return AJA_NULL;
		}
	}	//	loop over the next 7 cycles

	//	Success!  Found 7 cycles of clock!
	if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectSuccess)
		output << "## NOTE:  FindFirstDataBit_NTSC:  Confirmed " << numClockCycles << " cycles of clock run-in" << endl;

	//	Find the leading edge of the last clock cycle.
	//	This will serve as our reference sample point for the data bits...
	startPixelNdx = ((numClockCycles - 2) * CC_BIT_WIDTH) + 20;		//	The lo point of the 6th cycle
	stopPixelNdx  = ((numClockCycles - 1) * CC_BIT_WIDTH) +  7;		//	The hi point of the 7th cycle
	for (pixelNdx = startPixelNdx;  pixelNdx < stopPixelNdx;  pixelNdx++)
		if ((pFirstClockEdge [pixelNdx * bytesPerPixel] < CC_LEVEL_MID) && (pFirstClockEdge [(pixelNdx + 1) * bytesPerPixel] >= CC_LEVEL_MID))
			break;

	//	The mid-point of each sample bit will occur on bit-cell multiples from here...
	pLastClockEdge = &pFirstClockEdge [(pixelNdx + 1) * bytesPerPixel];
	if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectSuccess)
		output << "## NOTE:  FindFirstDataBit_NTSC:  The leading edge of the last clock cycle is at pixel " << (pLastClockEdge - pFirstYSample) / bytesPerPixel << endl;

	//	The next three bit cells are the start bits, which should be 0, 0, 1...
	if (   (pLastClockEdge [(CC_BIT_WIDTH * 1) * bytesPerPixel] <  CC_LEVEL_MID)
		&& (pLastClockEdge [(CC_BIT_WIDTH * 2) * bytesPerPixel] <  CC_LEVEL_MID)
		&& (pLastClockEdge [(CC_BIT_WIDTH * 3) * bytesPerPixel] >= CC_LEVEL_MID))
	{
		//	Good to go -- return the mid-point of the first data bit past the last start bit...
		pFirstDataBit = &pLastClockEdge [(CC_BIT_WIDTH * 4) * bytesPerPixel];
		if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectSuccess)
		{
			const unsigned	firstDataBitPixel	(unsigned (pFirstDataBit - pFirstYSample) / bytesPerPixel);
			CNTV2CaptionLogConfig::DumpYBytes_2vuy (pInVideoLine, output, firstDataBitPixel, firstDataBitPixel + 40, true, firstDataBitPixel, firstDataBitPixel);
			output << "## NOTE:  FindFirstDataBit_NTSC:  Start bits are correct, first data bit is at pixel " << firstDataBitPixel << endl;
		}
	}
	else if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectFail)
	{
		const UByte *	pFirstStartBit		(&pLastClockEdge[(CC_BIT_WIDTH * 1) * bytesPerPixel]);
		const UByte *	pLastStartBit		(&pLastClockEdge[(CC_BIT_WIDTH * 3) * bytesPerPixel]);
		const unsigned	firstStartBitPixel	(unsigned(pFirstStartBit - pFirstYSample) / bytesPerPixel);
		const unsigned	lastStartBitPixel	(unsigned(pLastStartBit - pFirstYSample) / bytesPerPixel);
		CNTV2CaptionLogConfig::DumpYBytes_2vuy (pInVideoLine, output, firstStartBitPixel - 20, lastStartBitPixel + 20, true, firstStartBitPixel, lastStartBitPixel);
		output	<< "## ERROR:  FindFirstDataBit_NTSC:  Bad start bits, expected 0, 0, 1 sequence, instead got: "
				<< (pLastClockEdge[(CC_BIT_WIDTH * 1) * bytesPerPixel] >= CC_LEVEL_MID) << ", "
				<< (pLastClockEdge[(CC_BIT_WIDTH * 2) * bytesPerPixel] >= CC_LEVEL_MID) << ", "
				<< (pLastClockEdge[(CC_BIT_WIDTH * 3) * bytesPerPixel] >= CC_LEVEL_MID) << endl;
	}

	return pFirstDataBit;

}	//	FindFirstDataBit_NTSC


vector<uint8_t>::size_type CNTV2Line21Captioner::FindFirstDataBit_NTSC (const vector<uint8_t> & in2VUYLine)
{
	ostream &		output			(::GetDefaultCaptionLogOutputStream());
	const size_t	firstYSample	(1);	//	First luma sample in line
	size_t			firstClockEdge	(in2VUYLine.max_size());
	size_t			lastClockEdge	(firstClockEdge);
	size_t			firstDataBit	(lastClockEdge);
	const size_t	bytesPerPixel	(2);
	const size_t	numClockCycles	(7);
NTV2_ASSERT(sizeof(size_t) == sizeof(vector<uint8_t>::size_type));

	/**
		The rising edge of the first clock run-in cycle should happen approx 10.5 usecs from the leading
		edge of sync, which translates to approximately 20 pixels from the left-hand edge of active video.
		However, the tolerance on this value is +/- 500 nsec, or +/- 7 pixels, so we must programmatically
		find the first leading clock edge and use that as our reference.
	**/

	//	Start looking for a low->high transition starting at pixel 10.
	//	Give up if we haven't found it by pixel 30...
	size_t	pixelNdx		(0);
	size_t	startPixelNdx	(10);
	size_t	stopPixelNdx	(30);
	for (pixelNdx = startPixelNdx;  pixelNdx < stopPixelNdx;  pixelNdx++)
		if ((in2VUYLine.at(firstYSample + pixelNdx * bytesPerPixel) < CC_LEVEL_MID))
			if (in2VUYLine.at(firstYSample + (pixelNdx + 1) * bytesPerPixel) >= CC_LEVEL_MID)
				break;

	if (pixelNdx >= stopPixelNdx)
	{
		if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectFail)
		{
			CNTV2CaptionLogConfig::DumpYBytes_2vuy (in2VUYLine, output, 0, 50, true, 10, 30);
			output << "## WARNING:  FindFirstDataBit_NTSC:  Missing initial clock edge, luma transition from < "
						<< xHEX0N(uint16_t(CC_LEVEL_MID),2) << " to >= " << xHEX0N(uint16_t(CC_LEVEL_MID),2)
						<< " from pixels 10 thru 30" << endl;
		}
		return in2VUYLine.max_size();
	}

	firstClockEdge = firstYSample  +  pixelNdx * bytesPerPixel;
	if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectSuccess)
	{
		CNTV2CaptionLogConfig::DumpYBytes_2vuy (in2VUYLine, output, 0, 50, true, pixelNdx, pixelNdx + 1);
		output << "## NOTE:  FindFirstDataBit_NTSC:  Found first clock edge at pixel " << pixelNdx << endl;
	}

	//	If this is the leading edge of the first sine wave, then the crest of this wave will
	//	be approx 7 pixels from here, and the trough of this wave will be approx 13 clocks after that.
	//	This pattern will repeat every 27 pixels for 7 cycles...
	for (unsigned cycleNum = 0;  cycleNum < numClockCycles;  cycleNum++)
	{
		const size_t	hi_pixel	((cycleNum * CC_BIT_WIDTH) + 7);
		const size_t	lo_pixel	((cycleNum * CC_BIT_WIDTH) + 20);

		if ((in2VUYLine.at(firstClockEdge + hi_pixel * bytesPerPixel) < CC_LEVEL_MID) || (in2VUYLine.at(firstClockEdge + lo_pixel * bytesPerPixel) >= CC_LEVEL_MID))
		{
			//	Failed to find an expected clock crest or trough -- abort
			if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectFail)
			{
				CNTV2CaptionLogConfig::DumpYBytes_2vuy (in2VUYLine, output, hi_pixel - CC_BIT_WIDTH, lo_pixel + CC_BIT_WIDTH, true, hi_pixel, lo_pixel);
				if (in2VUYLine.at(firstClockEdge + hi_pixel * bytesPerPixel) < CC_LEVEL_MID)
					output	<< "## ERROR:  FindFirstDataBit_NTSC:  Missing clock sine crest at cycle " << cycleNum << " (pixel " << hi_pixel
							<< "), expected at least " << xHEX0N(uint16_t(CC_LEVEL_MID),2) << ", instead got "
							<< xHEX0N(uint16_t(in2VUYLine.at(firstClockEdge + hi_pixel * bytesPerPixel)),2) << endl;
				else
					output	<< "## ERROR:  FindFirstDataBit_NTSC:  Missing clock sine trough at cycle " << cycleNum << " (pixel " << lo_pixel
							<< "), expected less than " << xHEX0N(uint16_t(CC_LEVEL_MID),2) << ", instead got "
							<< xHEX0N(uint16_t(in2VUYLine.at(firstClockEdge + lo_pixel * bytesPerPixel)),2) << endl;
			}
			return in2VUYLine.max_size();
		}
	}	//	loop over the next 7 cycles

	//	Success!  Found 7 cycles of clock!
	if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectSuccess)
		output << "## NOTE:  FindFirstDataBit_NTSC:  Confirmed " << numClockCycles << " cycles of clock run-in" << endl;

	//	Find the leading edge of the last clock cycle.
	//	This will serve as our reference sample point for the data bits...
	startPixelNdx = ((numClockCycles - 2) * CC_BIT_WIDTH) + 20;		//	The lo point of the 6th cycle
	stopPixelNdx  = ((numClockCycles - 1) * CC_BIT_WIDTH) +  7;		//	The hi point of the 7th cycle
	for (pixelNdx = startPixelNdx;  pixelNdx < stopPixelNdx;  pixelNdx++)
		if ((in2VUYLine.at(firstClockEdge + pixelNdx * bytesPerPixel) < CC_LEVEL_MID) && (in2VUYLine.at(firstClockEdge + (pixelNdx + 1) * bytesPerPixel) >= CC_LEVEL_MID))
			break;

	//	The mid-point of each sample bit will occur on bit-cell multiples from here...
	lastClockEdge = firstClockEdge + (pixelNdx + 1) * bytesPerPixel;
	if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectSuccess)
		output << "## NOTE:  FindFirstDataBit_NTSC:  The leading edge of the last clock cycle is at pixel " << (lastClockEdge - firstYSample) / bytesPerPixel << endl;

	//	The next three bit cells are the start bits, which should be 0, 0, 1...
	if (   (in2VUYLine.at(lastClockEdge + (CC_BIT_WIDTH * 1) * bytesPerPixel) <  CC_LEVEL_MID)
		&& (in2VUYLine.at(lastClockEdge + (CC_BIT_WIDTH * 2) * bytesPerPixel) <  CC_LEVEL_MID)
		&& (in2VUYLine.at(lastClockEdge + (CC_BIT_WIDTH * 3) * bytesPerPixel) >= CC_LEVEL_MID))
	{
		//	Good to go -- return the mid-point of the first data bit past the last start bit...
		firstDataBit = lastClockEdge + (CC_BIT_WIDTH * 4) * bytesPerPixel;
		if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectSuccess)
		{
			const unsigned	firstDataBitPixel	(unsigned (firstDataBit - firstYSample) / bytesPerPixel);
			CNTV2CaptionLogConfig::DumpYBytes_2vuy (in2VUYLine, output, firstDataBitPixel, firstDataBitPixel + 40, true, firstDataBitPixel, firstDataBitPixel);
			output << "## NOTE:  FindFirstDataBit_NTSC:  Start bits are correct, first data bit is at pixel " << firstDataBitPixel << endl;
		}
	}
	else if (::GetDefaultCaptionLogMask() & kCaptionLog_Line21DetectFail)
	{
		const size_t	firstStartBit		(lastClockEdge + (CC_BIT_WIDTH * 1) * bytesPerPixel);
		const size_t	lastStartBit		(lastClockEdge + (CC_BIT_WIDTH * 3) * bytesPerPixel);
		const size_t	firstStartBitPixel	((firstStartBit - firstYSample) / bytesPerPixel);
		const size_t	lastStartBitPixel	((lastStartBit - firstYSample) / bytesPerPixel);
		CNTV2CaptionLogConfig::DumpYBytes_2vuy (in2VUYLine, output, firstStartBitPixel - 20, lastStartBitPixel + 20, true, firstStartBitPixel, lastStartBitPixel);
		output	<< "## ERROR:  FindFirstDataBit_NTSC:  Bad start bits, expected 0, 0, 1 sequence, instead got: "
				<< (in2VUYLine.at(lastClockEdge + (CC_BIT_WIDTH * 1) * bytesPerPixel) >= CC_LEVEL_MID) << ", "
				<< (in2VUYLine.at(lastClockEdge + (CC_BIT_WIDTH * 2) * bytesPerPixel) >= CC_LEVEL_MID) << ", "
				<< (in2VUYLine.at(lastClockEdge + (CC_BIT_WIDTH * 3) * bytesPerPixel) >= CC_LEVEL_MID) << endl;
	}
	return firstDataBit;

}	//	FindFirstDataBit_NTSC



/**
	Call this with ptr set to mid-point of first data bit (i.e. the one following the '1' start bit)
	This method will read the two characters and return 'true' if successful.
	Note: this routine will return the parity bit of each character in the MSB position.
	It makes no calculation or value judgement as to the correctness of the parity.
**/
static bool DecodeCCBytes_NTSC (const UByte * pInFirstDataBit, UByte & outChar1, UByte & outChar2)
{
	if (!pInFirstDataBit)
		return false;

	//	First character, LSB first...
	outChar1 = 0;
	for (unsigned int i = 0;  i < 8;  i++)
	{
		UByte	bit	((pInFirstDataBit [(i * CC_BIT_WIDTH) * 2] > CC_LEVEL_MID) ? 1 : 0);
		outChar1 += (bit << i);
	}

	//	Advance pInFirstDataBit to middle of first data bit in second character...
	pInFirstDataBit += (8 * CC_BIT_WIDTH * 2);

	//	Second character, LSB first...
	outChar2 = 0;
	for (unsigned int i = 0;  i < 8;  i++)
	{
		UByte	bit	((pInFirstDataBit [(i * CC_BIT_WIDTH) * 2] > CC_LEVEL_MID) ? 1 : 0);
		outChar2 += (bit << i);
	}

	if (outChar1 == 0x80  &&  outChar2 == 0x80)
		LOGDECDBG("Returned NULL ccData");
	else if (((outChar1 & 0x7f) >= 0x20)  &&  ((outChar2 & 0x7f) >= 0x20))
		LOGDECINFO("Returned ccData " << xHEX0N(uint16_t(outChar1),2) << " ('" << string(1, outChar1 & 0x7f) << "'), "
					<< xHEX0N(uint16_t(outChar2),2) << " ('" << string(1, outChar2 & 0x7f) << "')");
	else
		LOGDECINFO("Returned ccData " << xHEX0N(uint16_t(outChar1),2) << ", " << xHEX0N(uint16_t(outChar2),2));
	return true;

}	//	DecodeCCBytes_NTSC


#if defined (_DEBUG)
	static FILE *	pDumpFile		(NULL);
	static bool		dumpCaptionLine	(false);	//	Set this TRUE to capture raw line 21 into a dump file "__line21_dump.txt"
#endif	//	_DEBUG


bool CNTV2Line21Captioner::DecodeLine (const UByte * pInVideoLine, UByte & outChar1, UByte & outChar2)
{
	#if defined (_DEBUG)
		//	Debug code for capturing line 21 into a file...
		if (pInVideoLine && dumpCaptionLine && !pDumpFile)
			pDumpFile = ::fopen ("__line21_dump.txt", "wb");
		if (pInVideoLine && pDumpFile && dumpCaptionLine)
			::fwrite (pInVideoLine, 720 * 2, 1, pDumpFile);
	#endif	//	_DEBUG

	//	See if the line contains a captioning clock run-in, signifying a valid captioning line.
	//	If successful, FindFirstDataBit_* will return a pointer to the middle of the first data bit
	//	(i.e. the one following the last '1' start bit) -- otherwise it will return NULL...
	const UByte *	pFirstDataBit	(pInVideoLine ? FindFirstDataBit_NTSC(pInVideoLine) : AJA_NULL);
	if (pFirstDataBit)
		return ::DecodeCCBytes_NTSC (pFirstDataBit, outChar1, outChar2);

	LOGDECDBG("No valid clock found -- returned ccData 0xFF 0xFF");
	outChar1 = 0xff;	//	No valid clock -- set the returned characters to 0xFF...
	outChar2 = 0xff;
	return false;

}	//	DecodeLine


bool CNTV2Line21Captioner::DecodeLine (const vector<uint8_t> & inLineData, vector<uint8_t> & outData)
{
	#if defined (_DEBUG)
		//	Debug code for capturing line 21 into a file...
		if (dumpCaptionLine && !pDumpFile)
			pDumpFile = ::fopen ("__line21_dump.txt", "wb");
		if (pDumpFile && dumpCaptionLine)
			::fwrite (&inLineData[0], 720 * 2, 1, pDumpFile);
	#endif	//	_DEBUG
	outData.clear();
	outData.push_back(0xFF);
	outData.push_back(0xFF);

	//	See if the line contains a captioning clock run-in, signifying a valid captioning line.
	//	If successful, FindFirstDataBit_* will return a pointer to the middle of the first data bit
	//	(i.e. the one following the last '1' start bit) -- otherwise it will return NULL...
	size_t	firstDataBit	(FindFirstDataBit_NTSC(inLineData));
	if (firstDataBit < inLineData.max_size())
		return ::DecodeCCBytes_NTSC (&inLineData[firstDataBit], outData.at(0), outData.at(1));

	LOGDECDBG("No valid clock found -- returned ccData 0xFF 0xFF");
	return false;
}	//	DecodeLine


#ifdef MSWindows
	#pragma warning(default: 4800)	//	int/bool warnings
#endif
