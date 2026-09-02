/**
	@file		ntv2captiondecoder608.cpp
	@brief		Implementation of the CNTV2CaptionDecoder608 class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/


/**
	Implementation Notes

	This module instantiates eight CNTV2CaptionDecodeChannel608 class objects (1 each for CC1-CC4 and
	Text1-Text4), and one CNTV2XDSDecodeChannel608 class object for XDS data. Users should not need to
	access the individual "channel" objects directly - all calls should funnel through this top level
	class. As the top-level object in the decoder, this code only worries about how to route incoming
	data (and user method calls) to the appropriate channel(s). All other decoding logic is carried
	in the channel objects.

	Implementation Note: This module is VERY similar to CNTV2CaptionTranslator608to708 in its decoding/
	parsing logic (the Translator doesn't require the burn-in methods). If you make a change here, take
	a look at CNTV2CaptionTranslator608to708 to see if the same logic shouldn't also be changed there...
**/


#include "ntv2captiondecoder608.h"
#include "ntv2line21captioner.h"
#include "ccfont.h"
#include "ntv2utils.h"
#include "ntv2captionrenderer.h"
#include "ntv2transcode.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include <sstream>
#include <stdlib.h>

using namespace std;


/////////////////////////////////////////////////////////////////////////////
// Line21Decoder definition
/////////////////////////////////////////////////////////////////////////////

#define	LOGMYERROR(__xpr__)		AJA_sERROR(AJA_DebugUnit_CC608Decode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define	LOGMYNOTE(__xpr__)		AJA_sNOTICE(AJA_DebugUnit_CC608Decode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define	LOGMYINFO(__xpr__)		AJA_sINFO(AJA_DebugUnit_CC608Decode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define	LOGMYDEBUG(__xpr__)		AJA_sDEBUG(AJA_DebugUnit_CC608Decode,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)


static uint32_t		gInstanceTally				(0);
static const UWord	NTV2_CC608_RollIncrement	(2);	//	Number of raster lines each frame scrolls during a Carriage Return "animation"
bool CNTV2CaptionDecoder608::UseNewBurnCaptionsMethod	(false);	//	Default to old row-by-row, col-by-col rendering method
bool CNTV2CaptionDecoder608::RenderPrePostSpaces		(true);		//	Default to lead & follow char runs with blank space
bool CNTV2CaptionDecoder608::AutoCallIdleFrame			(false);	//	Default to NOT automatically call IdleFrame from ProcessNew608FrameData
bool CNTV2CaptionDecoder608::AutoFlashOnAirChars		(false);	//	Default	to NOT automatically flash returned on-air characters/strings


void CNTV2CaptionDecoder608::NTV2Caption608ChangeHandler (void * pInstance, const NTV2Caption608ChangeInfo & inChangeInfo)
{
	const CNTV2CaptionDecoder608 *	pDecoder	(reinterpret_cast <const CNTV2CaptionDecoder608 *> (pInstance));
	if (pDecoder)
		pDecoder->Handle608ChangeNotification (inChangeInfo);
}


void CNTV2CaptionDecoder608::Handle608ChangeNotification (const NTV2Caption608ChangeInfo & inChangeInfo) const
{
	if (mpChangeSubscriber)
		(*mpChangeSubscriber) (mpSubscriberData, inChangeInfo);
}


/////////////////////////////////////////////////////////////////////////////


bool CNTV2CaptionDecoder608::Create (CNTV2CaptionDecoder608Ptr & outDecoder)
{
	outDecoder = AJA_NULL;
	try
	{
		outDecoder = new CNTV2CaptionDecoder608;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outDecoder;

}	//	Create


#if !defined(NTV2_DEPRECATE_16_2)
	static bool DecodeLine21 (UByte & outCC1, UByte & outCC2, const void * pInLine, const NTV2PixelFormat inFormat)
	{
		if (!pInLine)
			return false;	//	NULL line buffer pointer

		bool bResult(false);
		if (inFormat == NTV2_FBF_10BIT_YCBCR)
		{	//	Convert 10-bit line to 8-bit...
			NTV2Buffer line8BitYCbCr(720 * 16 / 8);
			if (line8BitYCbCr)
			{
				bResult = ::ConvertLine_v210_to_2vuy (reinterpret_cast<const ULWord*>(pInLine), line8BitYCbCr, 720);
				if (bResult)
					bResult = CNTV2Line21Captioner::DecodeLine (line8BitYCbCr, outCC1, outCC2);
			}
		}	//	if 10-bit YCbCr
		else if (inFormat == NTV2_FBF_8BIT_YCBCR)
			bResult = CNTV2Line21Captioner::DecodeLine (reinterpret_cast<const UByte*>(pInLine), outCC1, outCC2);

		return bResult;

	}	//	DecodeLine21

	CaptionData CNTV2CaptionDecoder608::DecodeCaptionData (	const UByte *			pInFrameData,
															const NTV2PixelFormat	inPixelFormat,
															const NTV2VideoFormat	inVideoFormat,
															const NTV2FrameGeometry	inFrameGeometry)
	{
		CaptionData	result;
		ULWord		lineOffsetF1	(1);

		if (!pInFrameData)
			return result;

		if (inFrameGeometry == NTV2_FG_720x486)			lineOffsetF1 = 1;
		else if (inFrameGeometry == NTV2_FG_720x508)	lineOffsetF1 = 23;
		else if (inFrameGeometry == NTV2_FG_720x514)	lineOffsetF1 = 29;
		else return result;

		const ULWord	lineOffsetF2	(lineOffsetF1 + 1);
		const ULWord	bytesPerRow		(::CalcRowBytesForFormat (inPixelFormat, ::GetDisplayWidth(inVideoFormat)));
		const UByte *	pLine21			(pInFrameData + (lineOffsetF1 * bytesPerRow));	//	F1 (CC1 & CC2)
		const UByte *	pLine284		(pInFrameData + (lineOffsetF2 * bytesPerRow));	//	F2 (CC3 & CC4)
		UByte			char1 (0), char2 (0);

		result.bGotField1Data = DecodeLine21 (char1, char2, pLine21, inPixelFormat);
		result.f1_char1 = char1;
		result.f1_char2 = char2;

		//	Get field2 data...
		result.bGotField2Data = DecodeLine21 (char1, char2, pLine284, inPixelFormat);
		result.f2_char1 = char1;
		result.f2_char2 = char2;

		return result;

	}	//	DecodeCaptionData
#endif	//	!defined(NTV2_DEPRECATE_16_2)


// Constructor
CNTV2CaptionDecoder608::CNTV2CaptionDecoder608 (void)
	:	mChannelDecoders (NTV2_CC608_ChannelMax)
{
	AJAAtomic::Increment(&gInstanceTally);
	ostringstream oss;  oss << "CaptionDecoder608-" << gInstanceTally;
	SetLogLabel(oss.str());
	mDisplayChannel		= NTV2_CC608_CC1;
	mCurrXmitChannel[0]	= NTV2_CC608_CC1;
	mCurrXmitChannel[1]	= NTV2_CC608_CC3;

	for (UWord chan(NTV2_CC608_CC1);  chan <= NTV2_CC608_Text4;  chan++)
	{
		CNTV2CaptionDecodeChannel608Ptr		p;
		CNTV2CaptionDecodeChannel608::Create(p);
		mChannelDecoders[chan] = p;
		if (!p)
		{
			std::bad_alloc	exception;
			throw exception;
		}

		mChannelDecoders[chan]->SetLogLabel(GetLogLabel() + "-DecodeChannel");
		mChannelDecoders[chan]->SetChannel(NTV2Line21Channel(chan));
	}	//	for each CEA-608 channel

	CNTV2XDSDecodeChannel608::Create(mXDSDecode);
	if (!mXDSDecode)
	{
		std::bad_alloc	exception;
		throw exception;
	}

	mLastControlCode[0] = 0;
	mLastControlCode[1] = 0;

	mRollOffset = 0;
	mFlashCount = 0;
	mpChangeSubscriber = AJA_NULL;
	mpSubscriberData = AJA_NULL;

}	//	constructor


CNTV2CaptionDecoder608::CNTV2CaptionDecoder608 (const CNTV2CaptionDecoder608 & inDecoderToCopy)
	:	CNTV2CaptionLogConfig ()
{	(void)inDecoderToCopy;
	gInstanceTally++;
	AJACC_ASSERT(false);
}


CNTV2CaptionDecoder608 & CNTV2CaptionDecoder608::operator = (const CNTV2CaptionDecoder608 & inDecoderToCopy)
{	(void)inDecoderToCopy;
	AJACC_ASSERT(false);
	return *this;
}


CNTV2CaptionDecoder608::~CNTV2CaptionDecoder608 ()
{
}	//	destructor


// Reset()
//
//	This can be called to "flush" the system of any in-progress data.
//	Note: this is NOT guaranteed to be thread-safe.
//
void CNTV2CaptionDecoder608::Reset (void)
{
	for (UWord chan(0);  chan < NTV2_CC608_ChannelMax - 1;  chan++)
	{
		mChannelDecoders [chan]->Reset();
		mChannelDecoders [chan]->SetLogLabel (GetLogLabel () + "-DecodeChannel");
		mChannelDecoders [chan]->SetChannel (NTV2Line21Channel (chan));
	}

	mXDSDecode->Reset();

	mLastControlCode[0] = 0;
	mLastControlCode[1] = 0;

}	//	Reset



// SetDisplayChannel()
//		Sets the current Line 21 Captioning mode (CC1, CC2, Text1, etc). This tells the decoder
//		software which captioning channel to display.
//
bool CNTV2CaptionDecoder608::SetDisplayChannel (const NTV2Line21Channel inChannel)
{
	if (!IsValidLine21Channel(inChannel))
		return false;

	if (IsLine21XDSChannel(mDisplayChannel))
		mXDSDecode->UnsubscribeChangeNotification (NTV2Caption608ChangeHandler, this);
	else
		mChannelDecoders[mDisplayChannel]->UnsubscribeChangeNotification (NTV2Caption608ChangeHandler, this);

	if (inChannel != mDisplayChannel)
	{
		const NTV2Line21Channel	oldChannel	(mDisplayChannel);
		mDisplayChannel = inChannel;
		LOGMYNOTE ("Display channel changed from " << ::NTV2Line21ChannelToStr(oldChannel) << " to " << ::NTV2Line21ChannelToStr(mDisplayChannel));
	}
	if (IsLine21XDSChannel(mDisplayChannel))
		mXDSDecode->SubscribeChangeNotification (NTV2Caption608ChangeHandler, this);
	else
		mChannelDecoders[mDisplayChannel]->SubscribeChangeNotification (NTV2Caption608ChangeHandler, this);

	return true;

}	//	SetDisplayChannel


void CNTV2CaptionDecoder608::IdleFrame (void)
{
	mFlashCount = (mFlashCount + 1) % CharacterFlashCycleFrames;
}


// ProcessNew608FrameData()
//		This is the main "every frame" input for new 608 captioning data. It should be called once
//	per video frame with the four bytes (2 per field) of new captioning data.
//
bool CNTV2CaptionDecoder608::ProcessNew608FrameData (const CaptionData & inCC608Data)
{
	bool bResult(true);

	//	Parse Field 1...
	if (inCC608Data.bGotField1Data)
		bResult = New608FieldData (inCC608Data.f1_char1, inCC608Data.f1_char2, NTV2_CC608_Field1);

	//	Parse Field 2...
	if (inCC608Data.bGotField2Data)
		bResult = New608FieldData (inCC608Data.f2_char1, inCC608Data.f2_char2, NTV2_CC608_Field2);

	if (AutoCallIdleFrame)
		IdleFrame();
	return bResult;

}	//	ProcessNew608FrameData


// New608FieldData()
//	This is called by ProcessNew608FrameData twice per video frame with the two bytes
//	of captioning or XDS data for each field
//
bool CNTV2CaptionDecoder608::New608FieldData (UByte charP1, UByte charP2, NTV2Line21Field field)
{
	bool	bResult	(true);

	//	Which captioning channels are we talking about here?
	const NTV2Line21Channel	currChannel	(GetCaptionChannel (charP1, charP2, field));

	if (currChannel == NTV2_CC608_XDS)
		bResult = ParseXDSData (charP1, charP2, field, currChannel);
	else
		bResult = ParseCaptionData (charP1, charP2, field, currChannel);

	return bResult;

}	//	New608FieldData



// ParseCaptionData()
//		This is called by New608FieldData() when it determines the new field's data is captioning
//	(i.e. not XDS). It determines which captioning channel the data is intended for and calls that
//	channel's handler.
//
bool CNTV2CaptionDecoder608::ParseCaptionData (const UByte inCharP1, const UByte inCharP2, const NTV2Line21Field inField, const NTV2Line21Channel inCurrChannel)
{
	const NTV2CCFont &	ccFont		(NTV2CCFont::GetInstance ());
	const string &		chanStr		(::NTV2Line21ChannelToStr(inCurrChannel));
	const string &		fieldStr	(::NTV2Line21FieldToStr(inField));
	string				str;
	#if defined (_DEBUG)
		string			parityStr;
		const bool		bParityOK	(::Check608Parity (inCharP1, inCharP2, parityStr));
	#endif

	//	Strip the parity bits...
	const UByte	char1	(inCharP1 & 0x7f);
	const UByte	char2	(inCharP2 & 0x7f);

	//	Does the new data look like a control code?
	//	Control codes are two-byte commands, and both bytes are guaranteed to arrive in the same field...
	bool	bControlCode	(false);
	bool	b2ndControlCode	(false);
	bool	bDisplayScreen	(false);
	UWord	newControlCode	(0);

	if (char1 >= 0x10 && char1 < 0x20)
	{
		//	Yep... if the first byte isn't character data (0x20 - 0x7f), assume we have a command...
		bControlCode = true;

		newControlCode = (UWord(char1 << 8) + char2);

		//	Is this a duplicate control code?
		//	(Many caption control codes are sent twice to avoid being missed. Only respond to the FIRST one!)
		UWord lastControlCode = (inField == NTV2_CC608_Field1 ? mLastControlCode[0] : mLastControlCode[1]);

		if (newControlCode == lastControlCode)
			b2ndControlCode = true;
	}	//	if control code

	//	Parse for specific caption commands or character data...

	//	For the most part, we just send the data to the current channel for parsing. However, there are a couple of exceptions...
	NTV2Line21Channel	theChannel	(inCurrChannel);

	//	Erase XXX Memory (EDM and ENM) commands always are applied to Caption channels (i.e. NOT Text channels).
	//	If the "current" channel is Text, send it to the corresponding Caption channel.
	if ((char1 == 0x14 || char1 == 0x1c || char1 == 0x15 || char1 == 0x1d) && (char2 == 0x2c || char2 == 0x2e))
	{
		if (mChannelDecoders [inCurrChannel]->IsTextChannel ())
			theChannel = static_cast<NTV2Line21Channel> (inCurrChannel - NTV2_CC608_TextChannelOffset);
	}

	//	Send the data to the designated channel for parsing...
	if (!b2ndControlCode)
		mChannelDecoders[theChannel]->Parse608Data (char1, char2, str);

	//	More special cases: if this is an CR or EOC command, we may want to start a RollUp and/or display the screen (debug)...
	if (!b2ndControlCode)
	{
		//	More special cases:  Carriage Return  (0x142d, 0x152d, 0x1c2d, 0x1d2d)
		if ((char1 == 0x14 || char1 == 0x1c || char1 == 0x15 || char1 == 0x1d) && (char2 == 0x2d))
		{
			//	Carriage Return:  if this is the selected channel, start the RollUp animation process...
			if (inCurrChannel == mDisplayChannel)
			{
				//	Push the entire display DOWN one row and start scroll animation...
				mRollOffset = ccFont.GetTotalHeightInDots();
				bDisplayScreen = true;
			}
		}

		//	More special cases: End of Captions  (0x142f, 0x152f, 0x1c2f, 0x1d2f)...
		if ( (char1 == 0x14 || char1 == 0x1c || char1 == 0x15 || char1 == 0x1d) && (char2 == 0x2f) )
			bDisplayScreen = true;	//	Debug
	}

	//	Save the control code so I can check for duplicates on the next frame.
	//	If this frame's data was NOT a control code, or was already a duplicate control code, reset "last" to zero...
	UWord	lastControlCode	(bControlCode ? newControlCode : 0);
	if (b2ndControlCode)
		lastControlCode = 0;

	if (inField == NTV2_CC608_Field1)
		mLastControlCode[0] = lastControlCode;
	else
		mLastControlCode[1] = lastControlCode;

	#if defined (_DEBUG)
		if (!bParityOK)
			LOGMYERROR(fieldStr << "[" << chanStr << "]: 0x" << UHEX2(inCharP1) << "|0x" << UHEX2(inCharP2) << " -- parity error: " << parityStr);
	#endif	//	_DEBUG
	if (true)
	{
		const string	dupeStr	(b2ndControlCode ? "(dupe)" : "");
		const bool bPACCommand = (char1 >= 0x10 && char1 <= 0x1f  &&  char2 >= 0x40 && char2 <= 0x7f);		// debug: we print out different stuff for PAC commands
		if (bPACCommand)
			LOGMYNOTE(fieldStr << "[" << chanStr << "]: 0x" << UHEX2(char1) << "|0x" << UHEX2(char2) << "  " << str
					<< " row " << mChannelDecoders[inCurrChannel]->GetRow() << " col " << mChannelDecoders[inCurrChannel]->GetColumn()
					<< " " << ((char2 % 2) ? "(underline)" : "") << "  " << dupeStr);
		else if (char1 >= 0x20  &&  char2 >= 0x20)
			LOGMYINFO(fieldStr << "[" << chanStr << "]: '" << char1 << "'|'" << char2 << "' " << dupeStr);
		else if (char1 >= 0x20  &&  char2 == 0)
			LOGMYINFO(fieldStr << "[" << chanStr << "]: '" << char1 << "'|0x" << UHEX2(char2) << "  " << dupeStr);
		else if (char1 > 0  ||  char2 > 0)
			LOGMYINFO(fieldStr << "[" << chanStr << "]: 0x" << UHEX2(char1) << "|0x" << UHEX2(char2) << " '" << str << "' " << dupeStr);
		else if (char1 == 0  ||  char2 == 0)
			LOGMYDEBUG(fieldStr << "[" << chanStr << "]: 0x" << UHEX2(char1) << "|0x" << UHEX2(char2) << "  " << dupeStr);
	}
	if (bDisplayScreen)
	{
		Handle608ChangeNotification(NTV2Caption608ChangeInfo(inCurrChannel));
		if ((TestLogMask(kCaptionLog_608ShowScreen)  &&  inCurrChannel == mDisplayChannel)  ||  TestLogMask(kCaptionLog_608ShowAllScreens))
			DebugPrintCurrentScreen (TestLogMask(kCaptionLog_608ShowAllScreens), true, IsLine21TextChannel (mDisplayChannel));
		if ((TestLogMask(kCaptionLog_608ShowScreenAttrs)  &&  inCurrChannel == mDisplayChannel))
			DebugPrintCurrentScreen (TestLogMask(kCaptionLog_608ShowAllScreens), false);
	}
	return true;

}	//	ParseCaptionData



// GetCaptionChannel()
//		This is called by ParseCaptionData() to check the new field caption data bytes to see if
//	a new channel has been selected. Returns the current captioning channel for the designated field.
//
NTV2Line21Channel CNTV2CaptionDecoder608::GetCaptionChannel (const UByte inCharP1, const UByte inCharP2, const NTV2Line21Field inField)
{
	//	Strip the parity bits...
	const UByte	char1	(inCharP1 & 0x7f);
	const UByte	char2	(inCharP2 & 0x7f);

	//	Get the (previous) current channel for this field...
	const NTV2Line21Channel	prevChannel	(inField == NTV2_CC608_Field1 ? mCurrXmitChannel[0] : mCurrXmitChannel[1]);

	//	Ask the current channel if it thinks it should remain the current channel for this field --
	//	or whether the new data is designating a new channel...
	NTV2Line21Channel	currChannel	(NTV2_CC608_CC1);
	if (prevChannel == NTV2_CC608_XDS)
		currChannel = mXDSDecode->GetCurrentChannel (char1, char2, inField);
	else
		currChannel = mChannelDecoders[prevChannel]->GetCurrentChannel (char1, char2, inField);

	//	Save and return the new (or unchanged) channel...
	if (inField == NTV2_CC608_Field1)
		mCurrXmitChannel[0] = currChannel;
	else
		mCurrXmitChannel[1] = currChannel;

	if (prevChannel != currChannel)
		LOGMYNOTE("Changing current caption channel from [" << ::NTV2Line21ChannelToStr(prevChannel)
				<< "] to [" << ::NTV2Line21ChannelToStr(currChannel) << "] for F" << inField << ", 0x" << UHEX2(inCharP1) << "/0x" << UHEX2(char1)
				<< ", 0x" << UHEX2(inCharP2) << "/0x" << UHEX2(char2));
	return currChannel;

}	//	GetCaptionChannel



// ParseXDSData()
//		This is called by New608FieldData() when it determines the new field's data is XDS
//	(i.e. not captioning).
//
bool CNTV2CaptionDecoder608::ParseXDSData (UByte charP1, UByte charP2, NTV2Line21Field field, NTV2Line21Channel currChannel)
{
	(void) currChannel;
	//	Strip the parity bits...
	UByte char1 = charP1 & 0x7f;
	UByte char2 = charP2 & 0x7f;
	return mXDSDecode->NewData (char1, char2, field);

}	//	ParseXDSData



// GetOnAirCharacter()
//		For burn-in feature: returns the character at [row][col] from the current display channel's on-air screen.
//		Note: "row" and "col" are 1-based - if row and/or column are out of bounds, this returns 0.
//
UByte CNTV2CaptionDecoder608::GetOnAirCharacterWithAttributes (const UWord			inRow,
																const UWord			inCol,
																NTV2Line21Attrs &	outAttrs) const
{
	UByte result(0);
	if (IsLine21CaptionChannel(mDisplayChannel))
	{
		result = mChannelDecoders[mDisplayChannel]->GetOnAirCharacter (inRow, inCol, outAttrs);
		if (AutoFlashOnAirChars  &&  result  &&  outAttrs.IsFlashing() &&  IsFlashCycleOff())
			result = NTV2CCFont::GetInstance().GetNoUnderlineSpaceCharacterCode();
	}
	return result;

}	//	GetOnAirCharacterWithAttributes


UWord CNTV2CaptionDecoder608::GetOnAirUTF16CharacterWithAttributes (const UWord			inRow,
																	const UWord			inCol,
																	NTV2Line21Attrs &	outAttrs) const
{
	UWord result(0);
	if (IsLine21CaptionChannel(mDisplayChannel))
	{
		result = mChannelDecoders[mDisplayChannel]->GetOnAirUTF16CharacterWithAttributes (inRow, inCol, outAttrs);
		if (AutoFlashOnAirChars  &&  result  &&  outAttrs.IsFlashing() &&  IsFlashCycleOff())
			result = 32;	//	space character
	}
	return result;

}	//	GetOnAirUTF16CharacterWithAttributes


string CNTV2CaptionDecoder608::GetOnAirCharacter (const UWord			inRow,
													const UWord			inCol,
													NTV2Line21Attrs &	outAttrs) const
{
	string result;
	if (IsLine21CaptionChannel(mDisplayChannel))
	{
		result = mChannelDecoders[mDisplayChannel]->GetOnAirCharacterWithAttributes (inRow, inCol, outAttrs);
		if (AutoFlashOnAirChars  &&  !result.empty()  &&  outAttrs.IsFlashing() &&  IsFlashCycleOff())
			result = " ";	//	space character
	}
	return result;
}


string CNTV2CaptionDecoder608::GetOnAirCharacters (const UWord inRowNumber) const
{
	if (!IsLine21CaptionChannel(mDisplayChannel))
		return string();
	if (!inRowNumber)
		return mChannelDecoders[mDisplayChannel]->GetDebugPrintScreen();
	if (inRowNumber <= NTV2_CC608_MaxRow)
		return mChannelDecoders[mDisplayChannel]->GetDebugPrintRow(inRowNumber);
	return string();
}


//*****************************************************************************************

// BurnCaptions()
//		This is the public high-level method for the burn-in feature. It Keys the contents of the current
//	display channel's on-air caption buffer into given video frame buffer.
//
bool CNTV2CaptionDecoder608::BurnCaptions (NTV2Buffer & inFB,
											const NTV2FormatDesc & inFD)
{
	CNTV2CaptionRendererPtr	renderer (CNTV2CaptionRenderer::GetRenderer(inFD));
	if (!renderer)
		return false;	//	No renderer

	unsigned failCount(0);
	//	Inefficient, but guaranteed coherent:	Row-by-row, column-by-column
	for (UWord row(NTV2_CC608_MinRow);  row <= UWord(NTV2_CC608_MaxRow);  row++)
	{
		//	Load the "0th" character (we might want to print a leading space if the first character is non-zero)...
		NTV2Line21Attributes thisAttr, nextAttr;
		UByte lastChar(0), thisChar(0), nextChar(GetOnAirCharacterWithAttributes(row, NTV2_CC608_MinCol, nextAttr));

		for (UWord col(UWord(NTV2_CC608_MinCol)-1);  col <= UWord(NTV2_CC608_MaxCol)+1;  col++)
		{
			UByte burnChar(thisChar);
			UWord yPos(0), xPos(0);
			if (!renderer->GetCharacterRasterOrigin (row, col, yPos, xPos))
				failCount++;

			//	If this character is a zero, see if a "pre-space" or "post-space" burn is required.
			//	If this is the last blank before a real character, or the first blank following a real
			//	character, burn a space...
			if (RenderPrePostSpaces  &&  !thisChar  &&  (nextChar || lastChar))
			{
				burnChar = ' ';
				thisAttr.Clear();	//	Make sure the leading and trailing blanks have no attributes
			}

			//	If it's non-zero, burn it...
			if (burnChar)
			{
				//	Is this character supposed to be "flashed"?
				//	If so, and this is an "off" cycle, substitute a "blank space" character...
				if (thisAttr.IsFlashing() && (mFlashCount > CharacterFlashCycleFrames / 2))
					burnChar = NTV2CCFont::GetInstance().GetNoUnderlineSpaceCharacterCode();
				if (!renderer->BurnChar (burnChar, thisAttr, inFB, inFD, xPos, yPos))
					failCount++;
			}

			//	Shift characters...
			lastChar = thisChar;
			thisChar = nextChar;
			thisAttr = nextAttr;
			nextChar = GetOnAirCharacterWithAttributes (row, col + 2, nextAttr);	//	NOTE:  GetOnAirCharacter must return 0 if <col> is out of bounds!
		}	//	for each column
	}	//	for each row

	//	Assume I'm being called once per frame --- decrement the roll count (if running)...
	if (mRollOffset >= NTV2_CC608_RollIncrement)
		mRollOffset -= NTV2_CC608_RollIncrement;
	else
		mRollOffset = 0;
	return failCount == 0;	//	true/success if no failures
}

#if !defined(NTV2_DEPRECATE_16_0)
	bool CNTV2CaptionDecoder608::BurnCaptions (UByte *						pBaseVideoAddress,
												const NTV2FrameDimensions	inFBPixelDimensions,
												const NTV2FrameBufferFormat	inFBFormat,
												const UWord					inFBBytesPerRow)
	{	(void) inFBBytesPerRow;
		const NTV2FrameGeometry fg (::GetGeometryFromFrameDimensions(inFBPixelDimensions));
		if (!NTV2_IS_VALID_NTV2FrameGeometry(fg))
			return false;	//	Bad geometry
		const NTV2Standard st(::GetStandardFromGeometry(fg));
		if (!NTV2_IS_VALID_STANDARD(st))
			return false;	//	Bad standard
		const NTV2FormatDescriptor fd(st, inFBFormat, ::GetVANCModeForGeometry(fg));
		if (!fd.IsValid())
			return false;	//	Bad descriptor
		NTV2Buffer fb (pBaseVideoAddress, fd.GetTotalRasterBytes());
		if (!fb)
			return false;	//	Bad buffer
		CNTV2CaptionRendererPtr renderer (CNTV2CaptionRenderer::GetRenderer(fd));
		if (!renderer)
			return false;	//	No renderer

		for (UWord row(NTV2_CC608_MinRow);  row <= UWord(NTV2_CC608_MaxRow);  row++)
		{
			//	Load the "0th" character (we might want to print a leading space if the first character is non-zero)...
			NTV2Line21Attributes thisAttr, nextAttr;
			UByte lastChar(0), thisChar(0), nextChar(GetOnAirCharacterWithAttributes (row, NTV2_CC608_MinCol, nextAttr));

			for (UWord col (UWord(NTV2_CC608_MinCol)-1);  col <= UWord(NTV2_CC608_MaxCol) + 1;  col++)
			{
				UByte burnChar(thisChar);
				UWord yPos(0), xPos(0);
				renderer->GetCharacterRasterOrigin (row, col, yPos, xPos);

				//	If this character is a zero, see if a "pre-space" or "post-space" burn is required.
				//	If this is the last blank before a real character, or the first blank following a real
				//	character, burn a space...
				if (thisChar == 0 && (nextChar != 0 || lastChar != 0))
				{
					burnChar = ' ';
					thisAttr.Clear();	//	Make sure the leading and trailing blanks have no attributes
				}

				//	If it's non-zero, burn it...
				if (burnChar)
				{
					//	Is this character supposed to be "flashed"?
					//	If so, and this is an "off" cycle, substitute a "blank space" character...
					if (thisAttr.IsFlashing() && (mFlashCount > CharacterFlashCycleFrames / 2))
						burnChar = NTV2CCFont::GetInstance().GetNoUnderlineSpaceCharacterCode();
					renderer->BurnChar (burnChar, thisAttr, fb, fd, xPos, yPos);
				}

				//	Shift characters...
				lastChar = thisChar;
				thisChar = nextChar;
				thisAttr = nextAttr;
				nextChar = GetOnAirCharacterWithAttributes (row, col + 2, nextAttr);	//	NOTE:  GetOnAirCharacter must return 0 if <col> is out of bounds!
			}	//	for each column
		}	//	for each row

		//	Assuming that I'm being called once per frame (??!), decrement the roll count if running...
		if (mRollOffset >= NTV2_CC608_RollIncrement)
			mRollOffset -= NTV2_CC608_RollIncrement;
		else
			mRollOffset = 0;
		return true;
	}	//	BurnCaptions
#endif	//	!defined(NTV2_DEPRECATE_16_0)


UWord CNTV2CaptionDecoder608::GetTextModeDisplayRowCount (const NTV2Line21Channel inChannel)
{
	if (!IsLine21TextChannel(inChannel))
		return 0;
	if (!mChannelDecoders[inChannel])
		return 0;
	return mChannelDecoders[inChannel]->GetTextModeDisplayRowCount();

}	//	GetTextModeDisplayRowCount


bool CNTV2CaptionDecoder608::SetTextModeDisplayRowCount (const NTV2Line21Channel inChannel, const UWord	inNumRows)
{
	if (!IsLine21TextChannel(inChannel))
		return false;
	if (!mChannelDecoders[inChannel])
		return false;
	return mChannelDecoders[inChannel]->SetTextModeDisplayRowCount(inNumRows);

}	//	SetTextModeDisplayRowCount


const NTV2Line21Attrs & CNTV2CaptionDecoder608::GetTextModeDisplayAttributes (const NTV2Line21Channel inChannel) const
{
	static NTV2Line21Attrs	sNoAttributes;
	if (!IsLine21TextChannel(inChannel))
		return sNoAttributes;
	if (!mChannelDecoders[inChannel])
		return sNoAttributes;
	return mChannelDecoders[inChannel]->GetTextModeDisplayAttributes();

}	//	GetTextModeDisplayAttributes


bool CNTV2CaptionDecoder608::SetTextModeDisplayAttributes (const NTV2Line21Channel inChannel, const NTV2Line21Attrs & inAttributes)
{
	if (!IsLine21TextChannel(inChannel))
		return false;	//	Must be TX1...TX4
	if (!mChannelDecoders[inChannel])
		return false;
	if (inAttributes.GetColor() == inAttributes.GetBGColor())
		return false;	//	Foreground & background colors must differ
	mChannelDecoders[inChannel]->SetTextModeDisplayAttributes(inAttributes);
	return true;

}	//	SetTextModeDisplayAttributes


bool CNTV2CaptionDecoder608::SubscribeChangeNotification (NTV2Caption608Changed * pCallback, void * pUserData)
{
	mpChangeSubscriber = pCallback;
	mpSubscriberData = pUserData;
	return true;
}


bool CNTV2CaptionDecoder608::UnsubscribeChangeNotification (NTV2Caption608Changed *	pCallback,	void * pUserData)
{
	if (mpChangeSubscriber == pCallback  &&  mpSubscriberData == pUserData)
	{
		mpChangeSubscriber = AJA_NULL;
		mpSubscriberData = AJA_NULL;
		return true;
	}
	return false;
}


typedef enum _BoxConstants
{
	kTopLeft,
	kTopTee,
	kTopRight,
	kLeftTee,
	kRightTee,
	kBotLeft,
	kBotTee,
	kBotRight,
	kVertLine,
	kHorzLine,
	kHorizLine = kHorzLine,
	kMidCross,
	NBoxConstants
}	BoxConstants;

//													kTopLeft		kTopTee			kTopRight		kLeftTee		kRightTee		kBotLeft		kBotTee			kBotRight		kVertLine		kHorzLine		kMidCross
static const string gThinBox [NBoxConstants]	= {"\xE2\x94\x8C", "\xE2\x94\xAC", "\xE2\x94\x90", "\xE2\x94\x9C", "\xE2\x94\xA4", "\xE2\x94\x94", "\xE2\x94\xB4", "\xE2\x94\x98", "\xE2\x94\x82", "\xE2\x94\x80", "\xE2\x94\xBC"};
static const string gDoubleBox [NBoxConstants]	= {"\xE2\x95\x94", "\xE2\x95\xA6", "\xE2\x95\x97", "\xE2\x95\xA0", "\xE2\x95\xA3", "\xE2\x95\x9A", "\xE2\x95\xA9", "\xE2\x95\x9D", "\xE2\x95\x91", "\xE2\x95\x90", "\xE2\x95\xAC"};


static string CopyString (const unsigned inCopies, const string & inStringToCopy)
{
	ostringstream	oss;
	for (unsigned nCopy (0);  nCopy < inCopies;  nCopy++)
		oss << inStringToCopy;
	return oss.str();
}


// DebugPrintCurrentScreen()
//		A poor man's "printf" of the current on-air screen to the Console
//
void CNTV2CaptionDecoder608::DebugPrintCurrentScreen (const bool inAllChannels, const bool inShowChars, const bool inShowTextChannels)
{
	ostringstream	oss;
	const string	horzLine	(CopyString (inShowChars ? NTV2_CC608_MaxCol : NTV2_CC608_MaxCol * 4, inAllChannels ? gDoubleBox [kHorzLine] : gThinBox [kHorzLine]));
	if (inAllChannels)
		oss << gDoubleBox[kTopLeft] << horzLine << gDoubleBox[kTopTee] << horzLine << gDoubleBox[kTopTee] << horzLine << gDoubleBox[kTopTee] << horzLine << gDoubleBox[kTopRight] << endl;
	else
		oss << gThinBox[kTopLeft] << horzLine << gThinBox[kTopRight] << "  (" << ::NTV2Line21ChannelToStr(mDisplayChannel) << (inShowChars ? "" : " attrs") << ")" << endl;

	for (UWord row(NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
	{
		if (inAllChannels)
		{
			for (UWord chl (0);  chl < 4;  chl++)
				oss << gDoubleBox[kVertLine] << mChannelDecoders[inShowTextChannels ? 4 + chl : chl]->GetDebugPrintRow (row, inShowChars);
			oss << gDoubleBox[kVertLine] << endl;
		}
		else
			oss << gThinBox[kVertLine] << mChannelDecoders[mDisplayChannel]->GetDebugPrintRow (row, inShowChars) << gThinBox[kVertLine] << endl;
	}

	if (inAllChannels)
		oss << gDoubleBox[kBotLeft] << horzLine << gDoubleBox[kBotTee] << horzLine << gDoubleBox[kBotTee] << horzLine << gDoubleBox[kBotTee] << horzLine << gDoubleBox[kBotRight] << endl;
	else
		oss << gThinBox[kBotLeft] << horzLine << gThinBox[kBotRight] << endl;
	cerr << oss.str ();

}	//	DebugPrintCurrentScreen


void CNTV2CaptionDecoder608::SetDebugRowsOfInterest (const NTV2Line21Channel inChannel, const UWord inFromRow, const UWord inToRow, const bool inAdd)
{
	if (IsValidLine21Channel(inChannel))
		mChannelDecoders [inChannel]->SetDebugRowsOfInterest (inFromRow, inToRow, inAdd);
	else
		for (UWord chl(0);  chl < NTV2_CC608_ChannelMax;  chl++)
			mChannelDecoders[chl]->SetDebugRowsOfInterest (inFromRow, inToRow, inAdd);
}


void CNTV2CaptionDecoder608::SetDebugColumnsOfInterest (const NTV2Line21Channel inChannel, const UWord inFromCol, const UWord inToCol, const bool inAdd)
{
	if (IsValidLine21Channel(inChannel))
		mChannelDecoders [inChannel]->SetDebugColumnsOfInterest (inFromCol, inToCol, inAdd);
	else
		for (UWord chl(0);  chl < NTV2_CC608_ChannelMax;  chl++)
			mChannelDecoders[chl]->SetDebugColumnsOfInterest (inFromCol, inToCol, inAdd);
}


CNTV2CaptionDecodeChannel608Ptr	CNTV2CaptionDecoder608::Get608ChannelDecoder (const NTV2Line21Channel inChannel) const
{
	CNTV2CaptionDecodeChannel608Ptr	result;
	if (IsLine21CaptionChannel(inChannel) || IsLine21TextChannel(inChannel))
		result = mChannelDecoders[inChannel];
	return result;
}


NTV2CaptionLogMask CNTV2CaptionDecoder608::SetLogMask (const NTV2CaptionLogMask inLogMask)
{
	for (UWord i(0);  i < NTV2_CC608_ChannelMax - 1;  i++)
		mChannelDecoders[i]->SetLogMask(inLogMask);
	mXDSDecode->SetLogMask(inLogMask);
	return CNTV2CaptionLogConfig::SetLogMask(inLogMask);

}	//	SetDebugLevel
