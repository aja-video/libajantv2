/**
	@file		ntv2captiondecodechannel608.cpp
	@brief		Implementation of the CNTV2CaptionDecodeChannel608 and CNTV2XDSDecodeChannel608 classes.
	@details	This file contains code for two classes: CNTV2CaptionDecodeChannel608 and
				CNTV2XDSDecodeChannel608. The first decodes standard CEA-608 captioning
				data, and the second is used to decode CEA-608 "XDS" (eXtended Data Service)
				data. See below for detailed descriptions of each.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2captiondecodechannel608.h"
#include "ntv2captionencoder608.h"	//	for CUtf8Helpers
#include "ccfont.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"
#include <sstream>
#include <map>
#include <iomanip>

using namespace std;


/////////////////////////////////////////////////////////////////////////////
// Line21Decoder definition
/////////////////////////////////////////////////////////////////////////////

#define LOGMYERROR(__u__,__xpr__)	AJA_sREPORT((__u__), AJA_DebugSeverity_Error,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define LOGMYWARN(__u__,__xpr__)	AJA_sREPORT((__u__), AJA_DebugSeverity_Warning,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define LOGMYNOTE(__u__,__xpr__)	AJA_sREPORT((__u__), AJA_DebugSeverity_Notice,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define LOGMYINFO(__u__,__xpr__)	AJA_sREPORT((__u__), AJA_DebugSeverity_Info,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)
#define LOGMYDEBUG(__u__,__xpr__)	AJA_sREPORT((__u__), AJA_DebugSeverity_Debug,	GetLogLabel() << ": " << AJAFUNC << ": " << __xpr__)

#define	MyScreenLock		reinterpret_cast<AJALock*>(mpScreenLock)	//	Keeps the AJACCLIB headers separate from 'ajalibraries/ajabase'

#define	DEC02(__x__)		dec << setw (2) << setfill ('0') << (__x__)

#define	STAT_ADD(__stat__,__inc__)		do											\
										{											\
											AJAAutoLock __tmpLocker__(&mStatsLock);	\
											uint32_t &	theStat(mStats[__stat__]);	\
											theStat += (__inc__);					\
										} while (false)

#define	STAT_INCREMENT(__stat__)		STAT_ADD(__stat__, 1)


typedef enum _BoxConstants	{	kTopLeft,	kTopRight,	kBotLeft,	kBotRight,	kVertLine,	kHorzLine,	kHorizLine = kHorzLine,	NBoxConstants}	BoxConstants;


static const string		gThinBox [NBoxConstants]	= {	"\xE2\x94\x8C",	"\xE2\x94\x90",	"\xE2\x94\x94",	"\xE2\x94\x98",	"\xE2\x94\x82",	"\xE2\x94\x80"};
static const string		gDoubleBox [NBoxConstants]	= {	"\xE2\x95\x94",	"\xE2\x95\x97",	"\xE2\x95\x9A",	"\xE2\x95\x9D",	"\xE2\x95\x91",	"\xe2\x95\x90"};
static unsigned			gInstanceTally(0);
static NTV2Line21Attrs	gNoAttributes;
static const NTV2Line21AttributePermutations	gAllAttributePermutations;

//	DEBUG BUILDS ONLY:	Static Back-Buffer Test Mode
//	To test using static back-buffers, set sStaticBackBufferTestMode true, then call Reset, then operate me normally.
//	Note that while I continue caption decoding, my back-buffer contents don't ever change from the test pattern
//	set in EraseScreen.
bool CNTV2CaptionDecodeChannel608::sStaticBackBufferTestMode (false);	//	Defaults to false
static NTV2_CC608_CodePoint	gTstScreen	[2][NTV2_CC608_MaxRow+1][NTV2_CC608_MaxCol+1];	//	Fixed-pattern back buffer for testing
static NTV2Line21Attrs		gTstAttrs	[2][NTV2_CC608_MaxRow+1][NTV2_CC608_MaxCol+1];	//	Fixed-pattern back buffer for testing
static bool					gTstInitialized (false);



//****************************************************************************************************************
//
//	CNTV2CaptionDecodeChannel608
//
//****************************************************************************************************************
/*
		This module implements a decoder for a single channel of CEA-608 ("Line 21") closed captioning.
	CEA-608 allows for up to eight "captioning channels" (CC1 - CC4, Text1 - Text4) that can be active
	and receiving data at a given time. Logic outside of this module will typically parse the caption
	data stream and decide which channel to send the data to. This module should only be called with
	data that is pertinent to the channel it has been asked to decode.

		The main high level method for this module is Parse608Data(), which takes two bytes of data
	(the most any one channel can receive in a video frame) and decodes it. This module maintains the
	character buffers and the current state of its channel, which can be queried by outside code.

		Eight of these modules (one per CEA-608 caption channel) are instantiated by CNTV2CaptionDecode608
	and (using the CNTV2CaptionTranslatorChannel608to708 subclass) CNTV2CaptionTranslator608to708. Users
	will typically work with these higher-level classes to implement a full decoder and/or translator.

  
	Implementation Note: this class is subclassed by CNTV2CaptionTranslatorChannel608to708, which
	adds functionality needed for translating CEA-608 captions to CEA-708 format. Be aware that changes
	you make to this module could also affect the subclass.
 */

/////////////////////////////////////////////////////////////////////////////


bool CNTV2CaptionDecodeChannel608::Create (CNTV2CaptionDecodeChannel608Ptr & outInstance)
{
	outInstance = AJA_NULL;
	try
	{
		outInstance = new CNTV2CaptionDecodeChannel608;
	}
	catch (const std::bad_alloc &)
	{
	}
	return outInstance;

}	//	Create


// Constructor
//
CNTV2CaptionDecodeChannel608::CNTV2CaptionDecodeChannel608 (void)
	:	CNTV2CaptionLogConfig ("CNTV2CaptionDecodeChannel608"), 
		mpScreenLock	(AJA_NULL),
		mCurrScreen		(0),
		mRow			(NTV2_CC608_MaxRow),
		mCol			(NTV2_CC608_MinCol),
		mRollBaseRow	(NTV2_CC608_MaxRow),
		mTextRows		(NTV2_CC608_MaxRow),
		mCaptionMode	(NTV2_CC608_CapModePopOn),
		mChannel		(NTV2_CC608_CC1),
		mCharacterSet	(NTV2_CC608_DefaultCharacterSet),
		mpCallback		(AJA_NULL),
		mpUserData		(AJA_NULL)
{
	ostringstream	oss;	oss << "CaptionDecodeChannel608-" << ++gInstanceTally;
	SetLogLabel(oss.str());
	while (mStats.size() < size_t(kMaxTallies))
		mStats.push_back(0);

	mpScreenLock = new AJALock;
	AJACC_ASSERT (mpScreenLock && "must have AJALock!");

	EraseScreen(0);	//	Clear on-screen and off-screen back-buffers
	EraseScreen(1);

	/**	BACK-BUFFER TEST MODE initialization:
		Each row is filled with the same letter, upper-case for Screen 0, lower case for Screen 1.
		The first row contains 'A' (or 'a');  the last row contains 'O' (or 'o').
		Every letter is opaque with a black character background.
		The foreground color varies by row, rotating through green, blue, cyan, red, yellow, magenta, white, then repeats.
		Rows 5 and 10 are italicized from columns 6 thru 14.
		Rows 10 and 12 are underlined from columns 8 thru 16.
		The band of 'H' characters in row 8 columns 11 thru 19 flash.
			R1C1-R1C32		(grn-blk-opq-fiu) 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA'
			R2C1-R2C32		(blu-blk-opq-fiu) 'BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB'
			R3C1-R3C32		(cyn-blk-opq-fiu) 'CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC'
			R4C1-R4C32		(red-blk-opq-fiu) 'DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD'
			R5C1-R5C5		(yel-blk-opq-fiu) 'EEEEE'
			R5C6-R5C14		(yel-blk-opq-fIu) 'EEEEEEEEE'
			R5C15-R5C32		(yel-blk-opq-fiu) 'EEEEEEEEEEEEEEEEEE'
			R6C1-R6C32		(mag-blk-opq-fiu) 'FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF'
			R7C1-R7C32		(wht-blk-opq-fiu) 'GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG'
			R8C1-R8C10		(grn-blk-opq-fiu) 'HHHHHHHHHH'
			R8C11-R8C19		(grn-blk-opq-Fiu) 'HHHHHHHHH'
			R8C20-R8C32		(grn-blk-opq-fiu) 'HHHHHHHHHHHHH'
			R9C1-R9C32		(blu-blk-opq-fiu) 'IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII'
			R10C1-R10C5		(cyn-blk-opq-fiu) 'JJJJJ'
			R10C6-R10C7		(cyn-blk-opq-fIu) 'JJ'
			R10C8-R10C14	(cyn-blk-opq-fIU) 'JJJJJJJ'
			R10C15-R10C16	(cyn-blk-opq-fiU) 'JJ'
			R10C17-R10C32	(cyn-blk-opq-fiu) 'JJJJJJJJJJJJJJJJ'
			R11C1-R11C32	(red-blk-opq-fiu) 'KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK'
			R12C1-R12C7		(yel-blk-opq-fiu) 'LLLLLLL'
			R12C8-R12C16	(yel-blk-opq-fiU) 'LLLLLLLLL'
			R12C17-R12C32	(yel-blk-opq-fiu) 'LLLLLLLLLLLLLLLL'
			R13C1-R13C32	(mag-blk-opq-fiu) 'MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM'
			R14C1-R14C32	(wht-blk-opq-fiu) 'NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN'
			R15C1-R15C32	(grn-blk-opq-fiu) 'OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO'
	**/
	static AJALock gTstLock;
	AJAAutoLock	autoLock (&gTstLock);
	if (!gTstInitialized)
	{	for (UWord scr(0);  scr < 2;  scr++)
			for (UWord row(NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
				for (UWord col(NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
				{
						gTstScreen[scr][row][col] = Make608CodePoint ((scr ? 'a' : 'A') + row - 1, 0x00);
						gTstAttrs [scr][row][col] = NTV2Line21Attributes (	NTV2Line21Color(row % 7),
																			NTV2_CC608_Black,
																			NTV2_CC608_Opaque,
																			/*italic*/(row==5 || row==10) && col>5 && col<15,
																			/*underline*/(row==10 || row==12) && col>7 && col<17,
																			/*flash*/row==8 && col>10 && col<20);
				}
		gTstInitialized = true;
	}
}	//	constructor


CNTV2CaptionDecodeChannel608::~CNTV2CaptionDecodeChannel608 ()
{
	if (mpScreenLock)
	{
		delete MyScreenLock;
		mpScreenLock = AJA_NULL;
	}
}	//	destructor


CNTV2CaptionDecodeChannel608::CNTV2CaptionDecodeChannel608 (const CNTV2CaptionDecodeChannel608 & inObj)
	:	CNTV2CaptionLogConfig ("CNTV2CaptionDecodeChannel608")
{
	(void) inObj;
	AJACC_ASSERT (false && "hidden copy constructor");

}	//	copy constructor


CNTV2CaptionDecodeChannel608 & CNTV2CaptionDecodeChannel608::operator = (const CNTV2CaptionDecodeChannel608 & inObj)
{
	(void) inObj;
	AJACC_ASSERT (false && "hidden assignment operator");
	return *this;

}	//	assignment operator


vector<uint32_t> CNTV2CaptionDecodeChannel608::GetStats (void) const
{
	AJAAutoLock	tmpLocker(&mStatsLock);
	return mStats;
}

string CNTV2CaptionDecodeChannel608::GetStatTitle (const CaptionDecode608Stats inStat)	//	STATIC
{
	static const string sStatTitles[kMaxTallies] = {	"Parse608Data Successes",	//	kParsedOKTally
														"Parse608Data Failures",	//	kParseFailTally
														"Total Tab Offset Cmds",	//	kTotalTabOffsetCmds
														"Total Character Set Cmds",	//	kTotalCharSetCmds
														"Total Attribute Cmds",		//	kTotalAttribCmds
														"Total Misc Cmds",			//	kTotalMiscCmds
														"Total PAC Cmds",			//	kTotalPACCmds
														"Total Mid-Row Cmds",		//	kTotalMidRowCmds
														"Total Special Characters",	//	kTotalSpecialChars
														"Total Plain Characters",	//	kTotalCharData
														"","","","","",""};			//	kMaxTallies
	if (inStat >= kParsedOKTally  &&  inStat < kMaxTallies)
		return sStatTitles[inStat];
	return string();
}


// Reset()
//
//	This can be called to "flush" the system of any in-progress data.
//	Note: this is NOT guaranteed to be thread-safe.
//
void CNTV2CaptionDecodeChannel608::Reset (void)
{
	AJAAutoLock	autoLock (MyScreenLock);
	SetCurrentScreen (0);
	SetRow (NTV2_CC608_MaxRow);
	SetColumn (NTV2_CC608_MinCol);
	SetCaptionMode (NTV2_CC608_CapModePopOn);
	SetChannel (NTV2_CC608_CC1);

	mTextRows	  = NTV2_CC608_MaxRow;				//	Number of displayed rows in Text mode
	mCharacterSet = NTV2_CC608_DefaultCharacterSet;
	mRollBaseRow  = NTV2_CC608_MaxRow;

	EraseScreen(0);	//	Clear on-screen and off-screen back-buffers
	EraseScreen(1);

}	//	Reset


// SetChannel()
//		Set the decode channel ID that this instance is working on
//
bool CNTV2CaptionDecodeChannel608::SetChannel (const NTV2Line21Channel inNewChannel)
{
	AJAAutoLock	autoLock (MyScreenLock);
	const NTV2Line21Channel	oldChannel	(mChannel);
	if (!IsValidLine21Channel (inNewChannel))	//	Sanity check...
		return false;

	if (inNewChannel != oldChannel)
	{
		mChannel = inNewChannel;
		Notify_ChannelChanged (oldChannel, inNewChannel);
	}
	SetLogLabel (GetLogLabel () + "-" + ::NTV2Line21ChannelToStr (mChannel));
	return true;

}	//	SetChannel



// GetCurrentChannel()
//		Check the new field caption data bytes to see if a new channel has been selected.
//		Returns the new (or remains on the current) captioning channel for the designated field.
//
NTV2Line21Channel CNTV2CaptionDecodeChannel608::GetCurrentChannel (UByte inChar608_1, UByte inChar608_2, NTV2Line21Field inField)
{	
	AJAAutoLock	autoLock (MyScreenLock);
	NTV2Line21Channel	newChannel	(mChannel);	//	Assume no change
	bool				bTextChan	(IsTextChannel ());

	//	See if the new input is a command that switches between Caption Mode and Text Mode...
	if (inChar608_1 == 0x14 || inChar608_1 == 0x1c || inChar608_1 == 0x15 || inChar608_1 == 0x1d)
	{
		switch (inChar608_2)
		{
			case 0x2a:							// [TR] - Text Restart
			case 0x2b:	bTextChan = true;		// [RTD] - Resume Text Display
						break;

			case 0x20:							// [RCL] - Resume Caption Loading
			case 0x25:							// [RU2] - Roll-Up Captions (2 Rows)
			case 0x26:							// [RU3] - Roll-Up Captions (3 Rows)
			case 0x27:							// [RU4] - Roll-Up Captions (4 Rows)
			case 0x29:							// [RDC] - Resume Direct Captioning
			case 0x2f:	bTextChan = false;		// [EOC] - End of Captions
						break;
		}
	}

//	byte1 = IsChannel1Or3 (inChannel) ? 0x11 : 0x19;	byte2 = 0x30;	break;	//	registered sign

	//	If the first byte is between 0x10 - 0x17, it is a command word that selects the 1st caption channel...
	if (inChar608_1 >= 0x10 && inChar608_1 <= 0x17)
	{
		if (inField == NTV2_CC608_Field1)
			newChannel = (bTextChan ? NTV2_CC608_Text1 : NTV2_CC608_CC1);
		else
			newChannel = (bTextChan ? NTV2_CC608_Text3 : NTV2_CC608_CC3);
	}
	//	If the first byte is between 0x18 - 0x1f, it is a command word that selects the 2nd caption channel...
	else if (inChar608_1 >= 0x18 && inChar608_1 <= 0x1f)
	{
		if (inField == NTV2_CC608_Field1)
			newChannel = (bTextChan ? NTV2_CC608_Text2 : NTV2_CC608_CC2);
		else
			newChannel = (bTextChan ? NTV2_CC608_Text4 : NTV2_CC608_CC4);
	}
	//	If the first byte is between 0x01 - 0x0f, it is a command word that selects XDS data (Field 2 only)...
	else if ((inField == NTV2_CC608_Field2) && inChar608_1 >= 0x01 && inChar608_1 <= 0x0f)
		newChannel = NTV2_CC608_XDS;
	//else... no change

	if (mChannel != newChannel)
		LOGMYNOTE(AJA_DebugUnit_CC608Decode, "[S" << GetCurrentScreen() << "R" << DEC02(GetRow()) << "C" << DEC02(GetColumn())
					<< "]  Incoming data bytes 0x" << UHEX2(inChar608_1) << " 0x" << UHEX2(inChar608_2) << " " << ::NTV2Line21FieldToStr(inField)
					<< " will redirect from channel " << ::NTV2Line21ChannelToStr(mChannel) << " to " << ::NTV2Line21ChannelToStr(newChannel));
	return newChannel;

}	//	GetCurrentChannel


// Parse608Data()
//		This is the top of the parsing line: call this method any time you have two new bytes of
//		608 caption data for this channel. The caller should filter out any "duplicate" commands
//		(i.e. commands that are sent twice on adjacent frames) before calling this method. There
//		is no need to call this if there is no new data for a given channel.
//		NOTE:  Parity should have already been stripped off the incoming bytes.
//
bool CNTV2CaptionDecodeChannel608::Parse608Data (const UByte inByte1, const UByte inByte2, string & outDebugStr)
{
	bool	bResult	(true);
	
	//	Caption control codes always start with the first byte in the range: 0x10 - 0x1f...
	const bool	bControlCode	(inByte1 >= 0x10 && inByte1 <= 0x1f);

	//	If this is a control (command) code, send it to the appropriate handler...
	if (bControlCode)
	{
		if ((inByte1 == 0x17 || inByte1 == 0x1f) && (inByte2 >= 0x21 && inByte2 <= 0x23))			//	Tab Offset commands?  (0x1721-1723, 0x1f21-1f23)
			bResult = Parse608TabOffsetCommand (inByte1, inByte2, outDebugStr);
		else if ((inByte1 == 0x17 || inByte1 == 0x1f)  &&  (inByte2 >= 0x24 && inByte2 <= 0x2a))	//	Character Set commands?  (0x1724-172a, 0x1f24-1f2a)
			bResult = Parse608CharacterSetCommand (inByte1, inByte2, outDebugStr);
		//	NOTE:	Command codes 0x172b-172c and 0x1f2b-1f2c are unassigned
		else if ((inByte1 == 0x17 || inByte1 == 0x1f)  &&  (inByte2 >= 0x2d && inByte2 <= 0x2f))	//	Foreground/Background Attribute Commands? (Part I)  (0x172d-172f, 0x1f2d-1f2f), see CEA-608-C, Section 6.2
			bResult = Parse608AttributeCommand (inByte1, inByte2, outDebugStr);
		else if ((inByte1 == 0x10 || inByte1 == 0x18)  &&  (inByte2 >= 0x20 && inByte2 <= 0x2f))	//	Foreground/Background Attribute Commands? (Part II)  (0x1020-102f, 0x1820-182f), see CEA-608-C, Section 6.2
			bResult = Parse608AttributeCommand (inByte1, inByte2, outDebugStr);
		else if ((inByte1 == 0x14 || inByte1 == 0x1c || inByte1 == 0x15 || inByte1 == 0x1d)  &&  (inByte2 >= 0x20 && inByte2 <= 0x2f))	//	Misc Control Code?  (0x1420-102f, 0x1520-152f, 0x1c20-1c2f, 0x1d20-1d2f)
			bResult = Parse608MiscCommand (inByte1, inByte2, outDebugStr);
		else if ((inByte1 >= 0x10 && inByte1 <= 0x1f)  &&  (inByte2 >= 0x40 && inByte2 <= 0x7f))	//	Preamble Address Codes?  (0x1040-1f7f)
			bResult = Parse608PACCommand (inByte1, inByte2, outDebugStr);
		else if ((inByte1 == 0x11 || inByte1 == 0x19)  &&  (inByte2 >= 0x20 && inByte2 <= 0x2f))	//	Mid-Row code?  (0x1120-112f, 0x1920-192f)
			bResult = Parse608MidRowCommand (inByte1, inByte2, outDebugStr);
		else if ((inByte1 == 0x11 || inByte1 == 0x19)  &&  (inByte2 >= 0x30 && inByte2 <= 0x3f))	//	Special (2-Byte) Character?  (0x1130-113f, 0x1930-193f)
			bResult = Parse608SpecialCharacter (inByte1, inByte2, outDebugStr);
		else if ((inByte1 == 0x12 || inByte1 == 0x1a)  &&  (inByte2 >= 0x20 && inByte2 <= 0x3f))	//	Special (2-Byte) Character?  (0x1220-123f, 0x1a20-1a3f)
			bResult = Parse608SpecialCharacter (inByte1, inByte2, outDebugStr);
		else if ((inByte1 == 0x13 || inByte1 == 0x1b)  &&  (inByte2 >= 0x20 && inByte2 <= 0x3f))	//	Special (2-Byte) Character?  (0x1320-133f, 0x1b20-1b3f)
			bResult = Parse608SpecialCharacter (inByte1, inByte2, outDebugStr);
		else
			outDebugStr = "Unknown command code!";
	}
	else	//	Not a control code -- just plain ol' character data
		bResult = Parse608CharacterData (inByte1, inByte2, outDebugStr);

	if (!bResult)
	{
		if (inByte1 || inByte2)
		{
			if (outDebugStr.empty())
				LOGMYERROR(AJA_DebugUnit_CC608DecodeChannel, "Incoming data bytes x" << UHEX2(inByte1) << "|x" << UHEX2(inByte2));
			else
				LOGMYERROR(AJA_DebugUnit_CC608DecodeChannel, "Incoming data bytes x" << UHEX2(inByte1) << "|x" << UHEX2(inByte2) << ": " << outDebugStr);
		}
		//else
		//	LOGMYDEBUG(AJA_DebugUnit_CC608DecodeChannel, "Incoming data bytes both zero" << (outDebugStr.empty() ? "" : " -- ") << outDebugStr);
	}
	else if (!outDebugStr.empty())
		LOGMYDEBUG(AJA_DebugUnit_CC608DecodeChannel, "Incoming data bytes x" << UHEX2(inByte1) << "|x" << UHEX2(inByte2) << ": '" << outDebugStr << "'");
	if (bResult)
		STAT_INCREMENT(kParsedOKTally);
	else
		STAT_INCREMENT(kParseFailTally);
	return bResult;

}	//	Parse608Data


// Parse608CharacterData()
//		Deal with incoming character data
//
bool CNTV2CaptionDecodeChannel608::Parse608CharacterData (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	uint32_t	charTally	(0);

	if (char608_1 == 0xFF  &&  char608_2 == 0xFF)
		return false;

	//	Only add non-zero characters...
	if (char608_1 >= NTV2CCFont::GlyphIndexToCharacterCode(0))	//	NTV2_CCFont_AsciiOffset)
	{
		InsertCharacter (char608_1, 0);
		charTally++;
		outDebugStr += char608_1;
	}

	if (char608_2 >= NTV2CCFont::GlyphIndexToCharacterCode(0))	//	NTV2_CCFont_AsciiOffset)
	{
		InsertCharacter (char608_2, 0);
		charTally++;
		outDebugStr += char608_2;
	}
	STAT_INCREMENT(kTotalCharData);
	return charTally > 0;

}	//	Parse608CharacterData


// Parse608TabOffsetCommand()
//		Deal with incoming TabOffset command
//
bool CNTV2CaptionDecodeChannel608::Parse608TabOffsetCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	int   tab	(0);
	(void) char608_1;
	outDebugStr.clear();

	switch (char608_2)
	{
		case 0x21:	tab = 1;	outDebugStr = "[TO1] Tab Offset 1 Column";	break;
		case 0x22:	tab = 2;	outDebugStr = "[TO2] Tab Offset 2 Column";	break;
		case 0x23:	tab = 3;	outDebugStr = "[TO3] Tab Offset 3 Column";	break;
	}
	if (tab)
		STAT_INCREMENT(kTotalTabOffsetCmds);
	return IncrementColumn(tab);

}	//	Parse608TabOffsetCommand


// Parse608CharacterSetCommand()
//		Deal with incoming CharacterSet command
//
bool CNTV2CaptionDecodeChannel608::Parse608CharacterSetCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	NTV2Line21CharacterSet	newSet	(NTV2_CC608_DefaultCharacterSet);
	(void) char608_1;
	outDebugStr.clear();

	switch (char608_2)
	{
		//	Character Set Assignments (see CEA-608-C, Section 6.3 - Non-US Markets only, TBD...)
		//	(no idea what to do with these... )
		case 0x24:	newSet = NTV2_CC608_DefaultCharacterSet;		outDebugStr = "Select NTV2_CC608_DefaultCharacterSet";		break;
		case 0x25:	newSet = NTV2_CC608_DoubleSizeCharacterSet;		outDebugStr = "Select NTV2_CC608_DoubleSizeCharacterSet";	break;
		case 0x26:	newSet = NTV2_CC608_PrivateCharacterSet1;		outDebugStr = "Select NTV2_CC608_PrivateCharacterSet1";		break;
		case 0x27:	newSet = NTV2_CC608_PrivateCharacterSet2;		outDebugStr = "Select NTV2_CC608_PrivateCharacterSet2";		break;
		case 0x28:	newSet = NTV2_CC608_PRChinaCharacterSet;		outDebugStr = "Select NTV2_CC608_PRChinaCharacterSet";		break;
		case 0x29:	newSet = NTV2_CC608_KoreanCharacterSet;			outDebugStr = "Select NTV2_CC608_KoreanCharacterSet";		break;
		case 0x2a:	newSet = NTV2_CC608_RegisteredCharacterSet1;	outDebugStr = "Select NTV2_CC608_RegisteredCharacterSet1";	break;
	}

	//	TBD: what is the scope of a character set change??
	mCharacterSet = newSet;
	STAT_INCREMENT(kTotalCharSetCmds);
	return true;

}	//	Parse608CharacterSetCommand


// Parse608AttributeCommand()
//		Deal with incoming Display Attribute command
//
bool CNTV2CaptionDecodeChannel608::Parse608AttributeCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	AJAAutoLock	autoLock (MyScreenLock);

	//	Get the current attributes...
	UWord	screen	(GetCurrentScreen ());
	if (GetCaptionMode () == NTV2_CC608_CapModePopOn)	//	In PopUp mode, get attributes from offscreen array
		screen = (GetCurrentScreen () == 0 ? 1 : 0);

	NTV2Line21Attrs attr;
	GetCharacterAttributes (screen, GetRow (), GetColumn (), attr);
	outDebugStr.clear();

	//	Attribute commands come in two ranges: 0x172d - 0x17ff and 0x1020 - 0x102f)...
	if (char608_1 == 0x17 || char608_1 == 0x1f)
	{
		//	Foreground/Background Attribute Commands (Part II)
		//	(see CEA-608-C, Section 6.2)
		switch (char608_2)
		{
			case 0x2d:	attr.SetOpacity(NTV2_CC608_Transparent);			outDebugStr = "[BT] ";	break;
			case 0x2e:	attr.SetColor(NTV2_CC608_Black).RemoveUnderline();	outDebugStr = "[FA] ";	break;
			case 0x2f:	attr.SetColor(NTV2_CC608_Black).AddUnderline();		outDebugStr = "[FAU]";	break;
		}
	}
	else if (char608_1 == 0x10 || char608_1 == 0x18)
	{
		//	Foreground/Background Attribute Commands (Part I)
		//	(see CEA-608-C, Section 6.2)
		switch (char608_2)
		{
			case 0x20:	attr.SetBGColor(NTV2_CC608_White  ).SetOpacity(NTV2_CC608_Opaque);			outDebugStr = "[BWO]";	break;
			case 0x21:	attr.SetBGColor(NTV2_CC608_White  ).SetOpacity(NTV2_CC608_SemiTransparent);	outDebugStr = "[BWS]";	break;
			case 0x22:	attr.SetBGColor(NTV2_CC608_Green  ).SetOpacity(NTV2_CC608_Opaque);			outDebugStr = "[BGO]";	break;
			case 0x23:	attr.SetBGColor(NTV2_CC608_Green  ).SetOpacity(NTV2_CC608_SemiTransparent);	outDebugStr = "[BGS]";	break;
			case 0x24:	attr.SetBGColor(NTV2_CC608_Blue   ).SetOpacity(NTV2_CC608_Opaque);			outDebugStr = "[BBO]";	break;
			case 0x25:	attr.SetBGColor(NTV2_CC608_Blue   ).SetOpacity(NTV2_CC608_SemiTransparent);	outDebugStr = "[BBS]";	break;
			case 0x26:	attr.SetBGColor(NTV2_CC608_Cyan   ).SetOpacity(NTV2_CC608_Opaque);			outDebugStr = "[BCO]";	break;
			case 0x27:	attr.SetBGColor(NTV2_CC608_Cyan   ).SetOpacity(NTV2_CC608_SemiTransparent);	outDebugStr = "[BCS]";	break;
			case 0x28:	attr.SetBGColor(NTV2_CC608_Red    ).SetOpacity(NTV2_CC608_Opaque);			outDebugStr = "[BRO]";	break;
			case 0x29:	attr.SetBGColor(NTV2_CC608_Red    ).SetOpacity(NTV2_CC608_SemiTransparent);	outDebugStr = "[BRS]";	break;
			case 0x2a:	attr.SetBGColor(NTV2_CC608_Yellow ).SetOpacity(NTV2_CC608_Opaque);			outDebugStr = "[BYO]";	break;
			case 0x2b:	attr.SetBGColor(NTV2_CC608_Yellow ).SetOpacity(NTV2_CC608_SemiTransparent);	outDebugStr = "[BYS]";	break;
			case 0x2c:	attr.SetBGColor(NTV2_CC608_Magenta).SetOpacity(NTV2_CC608_Opaque);			outDebugStr = "[BMO]";	break;
			case 0x2d:	attr.SetBGColor(NTV2_CC608_Magenta).SetOpacity(NTV2_CC608_SemiTransparent);	outDebugStr = "[BMS]";	break;
			case 0x2e:	attr.SetBGColor(NTV2_CC608_Black  ).SetOpacity(NTV2_CC608_Opaque);			outDebugStr = "[BAO]";	break;
			case 0x2f:	attr.SetBGColor(NTV2_CC608_Black  ).SetOpacity(NTV2_CC608_SemiTransparent);	outDebugStr = "[BAS]";	break;
		}
    }

	//	Change attributes at current location...
	outDebugStr += " " + ::NTV2Line21AttributesToStr (attr);
	SetAttributes (GetRow (), GetColumn (), attr);
	STAT_INCREMENT(kTotalAttribCmds);
	return true;

}	//	Parse608AttributeCommand


// Parse608PACCommand()
//		Deal with incoming Preamble Address Code (PAC) command
//
bool CNTV2CaptionDecodeChannel608::Parse608PACCommand (UByte char608_1, UByte char608_2, string & outDbgStr)
{
	AJAAutoLock		autoLock (MyScreenLock);
	const UByte		c1		((char608_1 >= 0x18) ? (char608_1 - 0x08) : char608_1);	//	Translate "Channel 2" codes to Channel 1
	const UByte		c2		((char608_2 >= 0x60) ? (char608_2 - 0x20) : char608_2);	//	Translate Even Row codes to Odd Row
	UWord			newRow	(1);
	UWord			newCol	(1);
	UWord			attrCol	(0);
	NTV2Line21Attrs	attr;
	outDbgStr.clear();

	//	Calculate new row...
	switch (c1)
	{
		case 0x11:	newRow = 1;		break;
		case 0x12:	newRow = 3;		break;
		case 0x15:	newRow = 5;		break;
		case 0x16:	newRow = 7;		break;
		case 0x17:	newRow = 9;		break;
		case 0x10:	newRow = 11;	break;
		case 0x13:	newRow = 12;	break;
		case 0x14:	newRow = 14;	break;
	}
	
	//	If original code was for Even Row, bump row by one...
	if (char608_2 >= 0x60)	
		newRow += 1;
		
	//	Calculate new column...
	switch (c2)
	{
		case 0x40:	outDbgStr = "PAC: White";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x41:	outDbgStr = "PAC: White";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
		case 0x42:	outDbgStr = "PAC: Green";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Green);									break;
		case 0x43:	outDbgStr = "PAC: Green";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Green)	.AddUnderline ();				break;
		case 0x44:	outDbgStr = "PAC: Blue";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Blue);									break;
		case 0x45:	outDbgStr = "PAC: Blue";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Blue)		.AddUnderline ();				break;
		case 0x46:	outDbgStr = "PAC: Cyan";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Cyan);									break;
		case 0x47:	outDbgStr = "PAC: Cyan";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Cyan)		.AddUnderline ();				break;
		case 0x48:	outDbgStr = "PAC: Red";		newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Red);										break;
		case 0x49:	outDbgStr = "PAC: Red";		newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Red)		.AddUnderline ();				break;
		case 0x4a:	outDbgStr = "PAC: Yellow";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Yellow);									break;
		case 0x4b:	outDbgStr = "PAC: Yellow";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Yellow)	.AddUnderline ();				break;
		case 0x4c:	outDbgStr = "PAC: Magenta";	newCol = 1;						attr.SetColor (NTV2_CC608_Magenta);									break;
		case 0x4d:	outDbgStr = "PAC: Magenta";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_Magenta)	.AddUnderline ();				break;
		case 0x4e:	outDbgStr = "PAC: Italics";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_White)	.AddItalics ();					break;
		case 0x4f:	outDbgStr = "PAC: Italics";	newCol = 1;		attrCol = 0;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ().AddItalics ();	break;
		case 0x50:	outDbgStr = "PAC: Indent";	newCol = 1;		attrCol = 1;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x51:	outDbgStr = "PAC: Indent";	newCol = 1;		attrCol = 1;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
		case 0x52:	outDbgStr = "PAC: Indent";	newCol = 5;		attrCol = 5;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x53:	outDbgStr = "PAC: Indent";	newCol = 5;		attrCol = 5;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
		case 0x54:	outDbgStr = "PAC: Indent";	newCol = 9;		attrCol = 9;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x55:	outDbgStr = "PAC: Indent";	newCol = 9;		attrCol = 9;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
		case 0x56:	outDbgStr = "PAC: Indent";	newCol = 13;	attrCol = 13;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x57:	outDbgStr = "PAC: Indent";	newCol = 13;	attrCol = 13;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
		case 0x58:	outDbgStr = "PAC: Indent";	newCol = 17;	attrCol = 17;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x59:	outDbgStr = "PAC: Indent";	newCol = 17;	attrCol = 17;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
		case 0x5a:	outDbgStr = "PAC: Indent";	newCol = 21;	attrCol = 21;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x5b:	outDbgStr = "PAC: Indent";	newCol = 21;	attrCol = 21;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
		case 0x5c:	outDbgStr = "Indent";		newCol = 25;	attrCol = 25;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x5d:	outDbgStr = "PAC: Indent";	newCol = 25;	attrCol = 25;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
		case 0x5e:	outDbgStr = "PAC: Indent";	newCol = 29;	attrCol = 29;	attr.SetColor (NTV2_CC608_White);									break;
		case 0x5f:	outDbgStr = "PAC: Indent";	newCol = 29;	attrCol = 29;	attr.SetColor (NTV2_CC608_White)	.AddUnderline ();				break;
	}
	
	//	PAC codes ONLY change the Row in Caption channels, not Text channels
	if (!IsTextChannel())
	{
		if (IsLine21RollUpMode(GetCaptionMode()))
		{
			//	HACK --	If the roll window is at the top of the screen, make sure that the roll base row
			//			is at least as high (low) as the size of the roll mode. E.g. if the current roll mode is
			//			2-line, then the base row needs to be at least "2". This would be an error on the part of
			//			the caption author, but we want to do the right thing anyhow...
			UWord	rollRows	(2);
			switch (GetCaptionMode())
			{
				case NTV2_CC608_CapModeRollUp2:	rollRows = 2;	break;
				case NTV2_CC608_CapModeRollUp3:	rollRows = 3;	break;
				case NTV2_CC608_CapModeRollUp4:	rollRows = 4;	break;
				default:										break;
			}			
			if (newRow < rollRows)
				newRow = rollRows;
			
			//	If we're in Roll Mode, the PAC row determines the RollUp "base row". If the base row
			//	changes, we need to copy the current contents of the roll view (2, 3, or 4 lines above
			//	the base row, including the base row) to the new location and erase the previous...
			if (newRow != mRollBaseRow)
				MoveRollUpWindow(newRow);
		}

		//	Set new row...
		SetRow(newRow);
	}

	//	Set new column & attributes...
	SetColumn(newCol);
	SetAttributes (newRow, attrCol, attr);
	STAT_INCREMENT(kTotalPACCmds);
	return true;

}	//	Parse608PACCommand



// Parse608MidRowCommand()
//		Deal with incoming Mid-Row command
//
bool CNTV2CaptionDecodeChannel608::Parse608MidRowCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	(void) char608_1;

	//	Get the attributes for the current location...
	AJAAutoLock		autoLock(MyScreenLock);
	UWord			screen	(GetCurrentScreen());
	NTV2Line21Attrs	attr;

	if (GetCaptionMode() == NTV2_CC608_CapModePopOn)	//	In PopUp mode...
		screen = (GetCurrentScreen() == 0 ? 1 : 0);		//	...get attributes from offscreen array

	GetCharacterAttributes (screen, GetRow(), GetColumn(), attr);

	//	All codes turn off flash and reset background to Black & Opaque...
	attr.RemoveFlash()	.SetBGColor (NTV2_CC608_Black)	.SetOpacity (NTV2_CC608_Opaque);

	switch (char608_2)
	{	//			Color								Underlined?				Italicized?
		case 0x20:	attr.SetColor (NTV2_CC608_White)	.SetUnderline (false)	.SetItalics (false);	break;	//	White
		case 0x21:	attr.SetColor (NTV2_CC608_White)	.SetUnderline (true)	.SetItalics (false);	break;	//	White underline
		case 0x22:	attr.SetColor (NTV2_CC608_Green)	.SetUnderline (false)	.SetItalics (false);	break;	//	Green
		case 0x23:	attr.SetColor (NTV2_CC608_Green)	.SetUnderline (true)	.SetItalics (false);	break;	//	Green underline
		case 0x24:	attr.SetColor (NTV2_CC608_Blue)		.SetUnderline (false)	.SetItalics (false);	break;	//	Blue
		case 0x25:	attr.SetColor (NTV2_CC608_Blue)		.SetUnderline (true)	.SetItalics (false);	break;	//	Blue underline
		case 0x26:	attr.SetColor (NTV2_CC608_Cyan)		.SetUnderline (false)	.SetItalics (false);	break;	//	Cyan
		case 0x27:	attr.SetColor (NTV2_CC608_Cyan)		.SetUnderline (true)	.SetItalics (false);	break;	//	Cyan underline
		case 0x28:	attr.SetColor (NTV2_CC608_Red)		.SetUnderline (false)	.SetItalics (false);	break;	//	Red
		case 0x29:	attr.SetColor (NTV2_CC608_Red)		.SetUnderline (true)	.SetItalics (false);	break;	//	Red underline
		case 0x2a:	attr.SetColor (NTV2_CC608_Yellow)	.SetUnderline (false)	.SetItalics (false);	break;	//	Yellow
		case 0x2b:	attr.SetColor (NTV2_CC608_Yellow)	.SetUnderline (true)	.SetItalics (false);	break;	//	Yellow underline
		case 0x2c:	attr.SetColor (NTV2_CC608_Magenta)	.SetUnderline (false)	.SetItalics (false);	break;	//	Magenta
		case 0x2d:	attr.SetColor (NTV2_CC608_Magenta)	.SetUnderline (true)	.SetItalics (false);	break;	//	Magenta underline
		case 0x2e:									attr.SetUnderline (false)	.SetItalics (true);		break;	//	Changes pen mode to "italic" (no underline), no color change
		case 0x2f:									attr.SetUnderline (true)	.SetItalics (true);		break;	//	Changes pen mode to "italic + underline", no color change
	}

	//	Insert a "no underline space" with the new attributes...
	outDebugStr = "Attribute: " + ::NTV2Line21AttributesToStr(attr);
	InsertCharacter (NTV2CCFont::GetInstance().GetNoUnderlineSpaceCharacterCode(), 0, attr);
	STAT_INCREMENT(kTotalMidRowCmds);
	return true;

}	//	Parse608MidRowCommand



// Parse608SpecialCharacter()
//		Deal with incoming "special" (2-Byte) characters in range 0x1130 - 0x113f and 0x1930 - 0x193f,
//		0x1220 - 0x123f and 0x1a20 - 0x1a3f, and 0x1320 - 0x133f and 0x1b20 - 0x1b3f.
//
bool CNTV2CaptionDecodeChannel608::Parse608SpecialCharacter (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	AJAAutoLock	autoLock		(MyScreenLock);
	bool		bDoBackspace	(false);
	outDebugStr = "";

	//	Two-byte printable characters -- we put the characters in the bitmap above the regular ASCII characters...
	//UByte c = ' ';

	//	THIS IS HANDLED IN NTV2CCFont::GetCCFontCharCode NOW...
	if (char608_1 == 0x11 || char608_1 == 0x19)
	{
		bDoBackspace = false;
		/*
		switch (char608_2)
		{
			case 0x30:	outDebugStr = "R: registered trademark";			c = 0x80;		break;
			case 0x31:	outDebugStr = "degree sign";						c = 0x81;		break;
			case 0x32:	outDebugStr = "1/2 symbol";							c = 0x82;		break;
			case 0x33:	outDebugStr = "inverted question mark";				c = 0x83;		break;
			case 0x34:	outDebugStr = "TM: trademark symbol";				c = 0x84;		break;
			case 0x35:	outDebugStr = "cents symbol";						c = 0x85;		break;
			case 0x36:	outDebugStr = "pounds sterling";					c = 0x86;		break;
			case 0x37:	outDebugStr = "music note";							c = 0x87;		break;
			case 0x38:	outDebugStr = "lower-case a, grave accent";			c = 0x88;		break;
			//case 0x39:outDebugStr = "transparent space";					c = ' ';		break;
			case 0x39:	outDebugStr = "transparent space";					c = 0;			break;
			case 0x3a:	outDebugStr = "lower-case e, grave accent";			c = 0x8a;		break;
			case 0x3b:	outDebugStr = "lower-case a, circumflex accent";	c = 0x8b;		break;
			case 0x3c:	outDebugStr = "lower-case e, circumflex accent";	c = 0x8c;		break;
			case 0x3d:	outDebugStr = "lower-case i, circumflex accent";	c = 0x8d;		break;
			case 0x3e:	outDebugStr = "lower-case o, circumflex accent";	c = 0x8e;		break;
			case 0x3f:	outDebugStr = "lower-case u, circumflex accent";	c = 0x8f;		break;
			default:																		break;
		}*/
	}
	else if (char608_1 == 0x12 || char608_1 == 0x1a)
	{
		//	These characters are optional for 608 decoders. This means that caption authors will send
		//	out a "normal" character, followed by one of these extended characters. If the decoder
		//	DOESN'T implement these characters, then it will display the first character and ignore
		//	the following extended character. If the decoder DOES implement this character set, then
		//	it will display the first character, then back up and overwrite it with the following
		//	extended character. So we need to deliver a backspace command before printing these chars...
		bDoBackspace = true;
		/*
		switch (char608_2)
		{
			case 0x20:	outDebugStr = "upper-case A, acute accent";			c = 'A';		break;
			case 0x21:	outDebugStr = "upper-case E, acute accent";			c = 'E';		break;
			case 0x22:	outDebugStr = "upper-case O, acute accent";			c = 'O';		break;
			case 0x23:	outDebugStr = "upper-case U, acute accent";			c = 'U';		break;
			case 0x24:	outDebugStr = "upper-case U, umlaut";				c = 'U';		break;
			case 0x25:	outDebugStr = "lower-case u, umlaut";				c = 'u';		break;
			case 0x26:	outDebugStr = "opening single quote";				c = '`';		break;
			case 0x27:	outDebugStr = "inverted exclamation";				c = '!';		break;
			case 0x28:	outDebugStr = "asterisk";							c = '*';		break;
			case 0x29:	outDebugStr = "single quote";						c = '\'';		break;
			case 0x2a:	outDebugStr = "em dash";							c = '_';		break;
			case 0x2b:	outDebugStr = "copyright";							c = 'c';		break;
			case 0x2c:	outDebugStr = "service mark";						c = 's';		break;
			case 0x2d:	outDebugStr = "round bullet";						c = 'o';		break;
			case 0x2e:	outDebugStr = "opening double quotes";				c = '"';		break;
			case 0x2f:	outDebugStr = "closing double quotes";				c = '"';		break;
			
			case 0x30:	outDebugStr = "upper-case A, grave accent";			c = 'A';		break;
			case 0x31:	outDebugStr = "upper-case A, circumflex accent";	c = 'A';		break;
			case 0x32:	outDebugStr = "upper-case C with cedilla";			c = 'C';		break;
			case 0x33:	outDebugStr = "upper-case E, grave accent";			c = 'E';		break;
			case 0x34:	outDebugStr = "upper-case E, circumflex accent";	c = 'E';		break;
			case 0x35:	outDebugStr = "upper-case E, umlaut";				c = 'E';		break;
			case 0x36:	outDebugStr = "lower-case e, umlaut";				c = 'e';		break;
			case 0x37:	outDebugStr = "upper-case I, circumflex accent";	c = 'I';		break;
			case 0x38:	outDebugStr = "upper-case I, umlaut";				c = 'I';		break;
			case 0x39:	outDebugStr = "lower-case I, umlaut";				c = 'i';		break;
			case 0x3a:	outDebugStr = "upper-case O, circumflex";			c = 'O';		break;
			case 0x3b:	outDebugStr = "upper-case U, grave accent";			c = 'U';		break;
			case 0x3c:	outDebugStr = "lower-case u, grave accent";			c = 'u';		break;
			case 0x3d:	outDebugStr = "upper-case U, circumflex accent";	c = 'U';		break;
			case 0x3e:	outDebugStr = "opening guillemets";					c = '<';		break;
			case 0x3f:	outDebugStr = "closing guillemets";					c = '>';		break;
			default:																		break;
		}*/
	}
	else if (char608_1 == 0x13 || char608_1 == 0x1b)
	{
		//	These characters are optional for 608 decoders. This means that caption authors will send
		//	out a "normal" character, followed by one of these extended characters. If the decoder
		//	DOESN'T implement these characters, then it will display the first character and ignore
		//	the following extended character. If the decoder DOES implement this character set, then
		//	it will display the first character, then back up and overwrite it with the following
		//	extended character. So we need to deliver a backspace command before printing these chars...
		bDoBackspace = true;
		/*
		switch (char608_2)
		{
			case 0x20:	outDebugStr = "upper-case A with tilde";			c = 'A';		break;
			case 0x21:	outDebugStr = "lower-case a with tilde";			c = 'E';		break;
			case 0x22:	outDebugStr = "upper-case I, acute accent";			c = 'I';		break;
			case 0x23:	outDebugStr = "upper-case I, grave";				c = 'I';		break;
			case 0x24:	outDebugStr = "lower-case i, grave accent";			c = 'i';		break;
			case 0x25:	outDebugStr = "upper-case O, grave accent";			c = 'O';		break;
			case 0x26:	outDebugStr = "lower-case o, grave accent";			c = 'o';		break;
			case 0x27:	outDebugStr = "upper-case O with tilde";			c = 'O';		break;
			case 0x28:	outDebugStr = "lower-case o with tilde";			c = 'o';		break;
			case 0x29:	outDebugStr = "opening brace";						c = '{';		break;
			case 0x2a:	outDebugStr = "closing brace";						c = '}';		break;
			case 0x2b:	outDebugStr = "backslash";							c = '\\';		break;
			case 0x2c:	outDebugStr = "caret";								c = '^';		break;
			case 0x2d:	outDebugStr = "underbar";							c = '_';		break;
			case 0x2e:	outDebugStr = "pipe";								c = '|';		break;
			case 0x2f:	outDebugStr = "tilde";								c = '~';		break;
			
			case 0x30:	outDebugStr = "upper-case A, umlaut";				c = 'A';		break;
			case 0x31:	outDebugStr = "lower-case A, umlaut";				c = 'a';		break;
			case 0x32:	outDebugStr = "upper-case O, umlaut";				c = 'O';		break;
			case 0x33:	outDebugStr = "lower-case o, umlaut";				c = 'o';		break;
			case 0x34:	outDebugStr = "small sharp s";						c = 's';		break;
			case 0x35:	outDebugStr = "yen";								c = 'Y';		break;
			case 0x36:	outDebugStr = "non-specific currency sign";			c = '$';		break;
			case 0x37:	outDebugStr = "vertical bar";						c = '|';		break;
			case 0x38:	outDebugStr = "upper-case A with ring";				c = 'A';		break;
			case 0x39:	outDebugStr = "lower-case a with ring";				c = 'a';		break;
			case 0x3a:	outDebugStr = "upper-case O with slash";			c = 'O';		break;
			case 0x3b:	outDebugStr = "lower-case O with slash";			c = 'o';		break;
			case 0x3c:	outDebugStr = "upper-left corner";					c = 'F';		break;
			case 0x3d:	outDebugStr = "upper-right corner";					c = 'T';		break;
			case 0x3e:	outDebugStr = "lower-left corner";					c = 'L';		break;
			case 0x3f:	outDebugStr = "lower-right corner";					c = 'J';		break;
			default:																		break;
		}*/
	}
	
	//	If the special character requires a leading backspace, do it first...
	if (bDoBackspace)
		DoBackspace();

	//	Insert the new character...
	InsertCharacter (char608_1, char608_2);
	STAT_INCREMENT(kTotalSpecialChars);
	return true;

}	//	Parse608SpecialCharacter



// Parse608MiscCommand()
//		Deal with incoming Miscellaneous commands
//
bool CNTV2CaptionDecodeChannel608::Parse608MiscCommand (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	bool	bResult	(true);
	(void) char608_1;
	outDebugStr.clear();

	switch (char608_2)
	{
		case 0x20:	bResult = DoResumeCaptionLoading();		outDebugStr = "[RCL] Resume Caption Loading";			break;
		case 0x21:	bResult = DoBackspace();				outDebugStr = "[BS]  Backspace";						break;
		case 0x22:											outDebugStr = "[AOF] Alarm Off";   						break;	//	Obsolete
		case 0x23:											outDebugStr = "[AON] Alarm On";							break;	//	Obsolete
		case 0x24:	bResult = DoDeleteToEndOfRow();			outDebugStr = "[DER] Delete to End of Row";				break;
		case 0x25:	bResult = DoRollUpCaption(2);			outDebugStr = "[RU2] Roll up Captions 2 Rows";			break;
		case 0x26:	bResult = DoRollUpCaption(3);			outDebugStr = "[RU3] Roll up Captions 3 Rows";			break;
		case 0x27:	bResult = DoRollUpCaption(4);			outDebugStr = "[RU4] Roll up Captions 4 Rows";			break;
		case 0x28:	bResult = DoFlashOn();					outDebugStr = "[FON] Flash On";							break;
		case 0x29:	bResult = DoResumeDirectCaptioning();	outDebugStr = "[RDC] Resume Direct Captioning";			break;
		case 0x2a:	bResult = DoTextRestart();				outDebugStr = "[TR]  Text Restart";						break;
		case 0x2b:	bResult = DoResumeTextDisplay();		outDebugStr = "[RTD] Resume Text Display";				break;
		case 0x2c:	bResult = DoEraseDisplayedMemory();		outDebugStr = "[EDM] Erase Displayed Memory";			break;
		case 0x2d:	bResult = DoCarriageReturn();			outDebugStr = "[CR]  Carriage Return";					break;
		case 0x2e:	bResult = DoEraseNonDisplayedMemory();	outDebugStr = "[ENM] Erase Non-displayed Memory";		break;
		case 0x2f:	bResult = DoEndOfCaption();				outDebugStr = "[EOC] End of Caption (flip memories)";	break;
	}
	STAT_INCREMENT(kTotalMiscCmds);
	return bResult;

}	//	Parse608MiscCommand



//******************************************************************************
//
//	Specific handlers for "Miscellaneous" commands
//
//******************************************************************************

// DoResumeCaptionLoading()
//		Handles a "Resume Caption Loading" [RCL] command
//
bool CNTV2CaptionDecodeChannel608::DoResumeCaptionLoading (void)
{
	SetCaptionMode (NTV2_CC608_CapModePopOn);
	return true;

}	//	DoResumeCaptionLoading


// DoBackspace()
//		Handles a Backspace [BS] command
//
bool CNTV2CaptionDecodeChannel608::DoBackspace (void)
{
	//	Special cases...
	if (GetColumn() > NTV2_CC608_MinCol)	//	If the cursor is in the first column, never mind...
	{
		AJAAutoLock	autoLock (MyScreenLock);
		if (GetColumn() == NTV2_CC608_MaxCol)
		{
			//	This is a problem case: according to CEA-608, some decoders erase both columns 31 and 32,
			//	while others only backup and erase column 31. In the latter ones, the only way to erase
			//	the last column would be to issue a Delete to End of Row [DER]). This implementation does
			//	the former, although the strict FCC interpretation is to do the latter.
			//	(See CEA-608-C, section C.13)
			InsertCharacter(0, 0);	//	Erase column 32
		}

		IncrementColumn(-1);		//	Back up one column
		InsertCharacter(0, 0);		//	Erase that character
		IncrementColumn(-1);		//	Since "Insert()" advances us by one column again, back up again...
	}
	return true;

}	//	DoBackspace


// DoDeleteToEndOfRow()
//		Erase all characters (i.e. clear to "transparent space") in the current edit buffer
//		from the current cursor position to the end of the current row
//
bool CNTV2CaptionDecodeChannel608::DoDeleteToEndOfRow (void)
{
	AJAAutoLock	autoLock	(MyScreenLock);
	UWord		screen		(GetCurrentScreen());

	if (GetCaptionMode() == NTV2_CC608_CapModePopOn)	//	In PopUp mode, use offscreen array
		screen = 1 - GetCurrentScreen();

	for (UWord col(GetColumn());  col <= NTV2_CC608_MaxCol;  col++)
	{
		SetScreenCharacter (screen, GetRow(), col, ::Make608CodePoint(0,0));
		SetScreenAttributes (screen, GetRow(), col, gNoAttributes);
	}

	return true;

}	//	DoDeleteToEndOfRow


bool CNTV2CaptionDecodeChannel608::SetScreenCharacter (const UWord inScreenNum, const UWord inRow, const UWord inCol, const NTV2_CC608_CodePoint inNewCodePoint)
{
	//	This is the only function permitted to directly write to the mScreen array!
	if (inScreenNum != 0 && inScreenNum != 1)
		return false;	//	Bad screen number
	if (!IsValidLine21Row(inRow) || !IsValidLine21Column(inCol))
		return false;	//	Bad row or column number

	AJAAutoLock	autoLock (MyScreenLock);
	const NTV2_CC608_CodePoint oldCodePoint (mScreen[inScreenNum][inRow][inCol]);
	if (oldCodePoint != inNewCodePoint)
	{
		mScreen[inScreenNum][inRow][inCol] = inNewCodePoint;
		Notify_ScreenCharChanged (inScreenNum, inRow, inCol, oldCodePoint, inNewCodePoint);
	}
	return true;
}


NTV2_CC608_CodePoint CNTV2CaptionDecodeChannel608::GetScreenCharacter (const UWord inScreenNum, const UWord inRow, const UWord inCol) const
{
	if (inScreenNum != 0 && inScreenNum != 1)
		return 0x00000000;	//	Bad screen number
	if (!IsValidLine21Row(inRow) || !IsValidLine21Column(inCol))
		return 0x00000000;	//	Bad row or column number

	//	This is the only function permitted to directly read from the mScreen array!
	AJAAutoLock	autoLock (MyScreenLock);
	return sStaticBackBufferTestMode  ?  gTstScreen[inScreenNum][inRow][inCol]  :  mScreen[inScreenNum][inRow][inCol];
}


// SetAttributes()
//		Figure out which screen is the current "edit" screen, based on the current mode.
//		Set the character attributes for the given row and column.
//		Note 1: setting the attributes for "Column 0" is OK!
//		Note 2: this method does NOT advance the cursor - that is left to the caller!
//
void CNTV2CaptionDecodeChannel608::SetAttributes (const UWord inRow, const UWord inCol, const NTV2Line21Attrs inAttr)
{
	AJAAutoLock autoLock (MyScreenLock);
	UWord screen (GetCurrentScreen());

	if (GetCaptionMode() == NTV2_CC608_CapModePopOn)	//	In PopUp mode, put character in offscreen array
		screen = (GetCurrentScreen() == 0 ? 1 : 0);

	if (IsValidLine21Row(inRow) && IsValidLine21Column(inCol))
		SetScreenAttributes (screen, inRow, inCol, inAttr);

}	//	SetAttributes


//	This is the only function permitted to write the mAttrs array!
bool CNTV2CaptionDecodeChannel608::SetScreenAttributes (const UWord inScreenNum, const UWord inRow, const UWord inCol, const NTV2Line21Attrs inNewAttr)
{
	if (inScreenNum != 0 && inScreenNum != 1)
		return false;	//	Bad screen number
	if (!IsValidLine21Row(inRow) || !IsValidLine21Column(inCol))
		return false;	//	Bad row or column number

	AJAAutoLock autoLock (MyScreenLock);
	const NTV2Line21Attrs oldAttrib (mAttrs[inScreenNum][inRow][inCol]);
	if (oldAttrib != inNewAttr)
	{
		mAttrs[inScreenNum][inRow][inCol] = inNewAttr;
		Notify_ScreenAttrChanged (inScreenNum, inRow, inCol, oldAttrib, inNewAttr);
	}
	return true;
}


//	This is the only function permitted to read the mAttrs array!
NTV2Line21Attrs CNTV2CaptionDecodeChannel608::GetScreenAttributes (const UWord inScreenNum, const UWord inRow, const UWord inCol) const
{
	if (inScreenNum > 1)
		return NTV2Line21Attrs();	//	Bad screen num
	if (!IsValidLine21Row(inRow)  ||  !IsValidLine21Column(inCol))
		return NTV2Line21Attrs();	//	Bad row or col num

	AJAAutoLock	autoLock (MyScreenLock);
	return sStaticBackBufferTestMode  ?  gTstAttrs[inScreenNum][inRow][inCol]  :  mAttrs[inScreenNum][inRow][inCol];
}


// InsertCharacter()
//		Figure out which screen is the current "edit" screen, based on the current mode.
//		Insert the designated character and advance the cursor to the next column.
//
void CNTV2CaptionDecodeChannel608::InsertCharacter (const UByte char608_1, const UByte char608_2)
{
	return InsertCharacter (char608_1, char608_2, gNoAttributes);

}	//	InsertCharacter


void CNTV2CaptionDecodeChannel608::InsertCharacter (const UByte char608_1, const UByte char608_2, const NTV2Line21Attrs inAttr)
{
	AJAAutoLock	autoLock	(MyScreenLock);
	UWord		screen		(GetCurrentScreen());

	if (IsTextChannel())	//	Text channel is handled differently...
	{
		//AJACC_ASSERT(char608_2 == 0);
		if (char608_1)
			return InsertTextCharacter(char608_1);
	}

	if (IsLine21PopOnMode(GetCaptionMode()))	//	In PopUp mode, put character in offscreen array
		screen = (GetCurrentScreen() == 0 ? 1 : 0);

	SetScreenCharacter (screen, GetRow(), GetColumn(), ::Make608CodePoint(char608_1, char608_2, GetCurrentCharacterSet()));
	SetAttributes (GetRow(), GetColumn(), inAttr);

	if (GetColumn() < NTV2_CC608_MaxCol)
		IncrementColumn();

}	//	InsertCharacter


static NTV2_CC608_CodePoint ASCIICharToCEA608CodePoint (const UByte inASCIIChar)
{
	if (inASCIIChar & 0x80)	return 0;	//	Must be less than 128
	if (inASCIIChar < 0x20)	return 0;	//	Must be ' ' or greater

	switch (inASCIIChar)
	{
		case 0x2A:	return ::Make608CodePoint	(0x12, 0x28);	//	'*'	(asterisk)				Txt1/Txt3
		case 0x5C:	return ::Make608CodePoint	(0x13, 0x2b);	//	'\'	(backslash)				Txt1/Txt3
		case 0x5E:	return ::Make608CodePoint	(0x13, 0x2c);	//	'^'	(caret)					Txt1/Txt3
		case 0x5F:	return ::Make608CodePoint	(0x13, 0x2d);	//	'_'	(underbar)				Txt1/Txt3
		case 0x60:	return ::Make608CodePoint	(0x12, 0x26);	//	'`'	(opening single quote)	Txt1/Txt3	//	Really need back-tick here, not opening single quote
		case 0x7B:	return ::Make608CodePoint	(0x13, 0x29);	//	'{'	(opening brace)			Txt1/Txt3
		case 0x7C:	return ::Make608CodePoint	(0x13, 0x2e);	//	'|'	(pipe)					Txt1/Txt3
		case 0x7D:	return ::Make608CodePoint	(0x13, 0x2a);	//	'}'	(closing brace)			Txt1/Txt3
		case 0x7E:	return ::Make608CodePoint	(0x13, 0x2f);	//	'~'	(tilde)					Txt1/Txt3
	}
	return ::Make608CodePoint(inASCIIChar, 0);

}	//	ASCIICharToCEA608CodePoint


void CNTV2CaptionDecodeChannel608::InsertTextCharacter (const UByte inASCIIChar)
{
	AJAAutoLock	autoLock	(MyScreenLock);
	UWord		currentCol	(GetColumn());

	if (!IsTextChannel()) return;

	//	Certain specific ASCII characters must be translated into different CEA608 codepoints that produce the correct
	//	glyphs when they get displayed, which is why ASCICharToCEA608CodePoint is called before SetScreenCharacter...
	SetScreenCharacter (GetCurrentScreen(), GetRow(), currentCol, ::ASCIICharToCEA608CodePoint(inASCIIChar));
	if (mTextModeStyle.IsSet())
		SetScreenAttributes (GetCurrentScreen(), GetRow(), currentCol, mTextModeStyle);

	//	This implementation of Text Mode display draws decoded characters left-to-right, top-to-bottom,
	//	rolling the entire screen up one row if the bottom-right corner is reached...
	if (currentCol < NTV2_CC608_MaxCol)
		IncrementColumn();		//	Next column over
	else
		DoCarriageReturn();		//	Next row down, maybe roll up if necessary

}	//	InsertTextCharacter


// DoRollUpCaption()
//		Handles a "RollUp Captions 2/3/4" [RU2/RU3/RU4] command
//
bool CNTV2CaptionDecodeChannel608::DoRollUpCaption (const UWord inRowCount)
{
	switch (inRowCount)
	{
		case 2:	return SetCaptionMode(NTV2_CC608_CapModeRollUp2);
		case 3:	return SetCaptionMode(NTV2_CC608_CapModeRollUp3);
		case 4:	return SetCaptionMode(NTV2_CC608_CapModeRollUp4);
	}
	return false;

}	//	DoRollUpCaption


// DoFlashOn()
//		Handles a Flash On command
//
bool CNTV2CaptionDecodeChannel608::DoFlashOn (void)
{
	AJAAutoLock		autoLock	(MyScreenLock);
	UWord			screen		(GetCurrentScreen());		//	Get the current attributes
	NTV2Line21Attrs	attr;

	if (GetCaptionMode() == NTV2_CC608_CapModePopOn)	//	In PopUp mode, get attributes from offscreen array
		screen = (GetCurrentScreen() == 0 ? 1 : 0);

	GetCharacterAttributes (screen, GetRow(), GetColumn(), attr);

	attr.AddFlash();	//	Turn on the "Flash" bit...

	//	Insert a "no underline space" with the new attributes...
	UByte	ch	(NTV2CCFont::GetInstance().GetNoUnderlineSpaceCharacterCode ());
	InsertCharacter (ch, 0, attr);
	return true;

}	//	DoFlashOn


// DoResumeDirectCaptioning()
//		Handles a "Resume Direct Captioning" [RDC] command
//
bool CNTV2CaptionDecodeChannel608::DoResumeDirectCaptioning (void)
{
	SetCaptionMode(NTV2_CC608_CapModePaintOn);
	return true;
}


// DoTextRestart()
//		Handles a Text Restart [TRS] command
//
bool CNTV2CaptionDecodeChannel608::DoTextRestart (void)
{
	AJAAutoLock	autoLock (MyScreenLock);

	if (!IsTextChannel())
		return false;

	//	Erase screen, then reset cursor to top-left corner...
	EraseScreen(GetCurrentScreen());
	SetRow(NTV2_CC608_MinRow);
	SetColumn(NTV2_CC608_MinCol);
	return true;
}


// DoResumeTextDisplay()
//		Handles a Resume Text Display [RTD] command
//
bool CNTV2CaptionDecodeChannel608::DoResumeTextDisplay (void)
{
	//	Nothing to do...
	//	if (IsTextChannel ())
	//		;
	return true;

}	//	DoResumeTextDisplay


// DoEraseDisplayedMemory()
//		Erase all characters (i.e. clear to "transparent space") in on-screen buffer
//
bool CNTV2CaptionDecodeChannel608::DoEraseDisplayedMemory (void)
{
	AJAAutoLock	autoLock (MyScreenLock);
	return EraseScreen(GetCurrentScreen());

}	//	DoEraseDisplayedMemory


// DoCarriageReturn()
//		Handles a carriage return command
//
bool CNTV2CaptionDecodeChannel608::DoCarriageReturn (void)
{
	AJAAutoLock	autoLock (MyScreenLock);
	UWord		rollRows (0);

	if (IsTextChannel())
	{
		//	This is a Text channel!
		if (GetRow() < mTextRows)
		{
			//	If we're not on the bottom-most row yet, just do a carriage return to the beginning of the next row...
			SetColumn(NTV2_CC608_MinCol);
			SetRow(GetRow() + 1);
		}
		else
			rollRows = mTextRows - 1;	//	On the last row -- roll all above rows up one...
	}
	else
	{
		//	This is a Caption channel -- Carriage Returns are only valid in RollUp mode(s)...
		switch (GetCaptionMode())
		{
			case NTV2_CC608_CapModeRollUp2:		rollRows = 1;	break;
			case NTV2_CC608_CapModeRollUp3:		rollRows = 2;	break;
			case NTV2_CC608_CapModeRollUp4:		rollRows = 3;	break;
			default:											break;
		}
	}

	if (rollRows)
	{
		//	Sanity check...
		if (rollRows >= mRollBaseRow)
			rollRows = mRollBaseRow - 1;

		//	Move current row and <rollRows-1> previous rows up one...
		for (UWord scrollRow(mRollBaseRow - rollRows);  scrollRow < mRollBaseRow;  scrollRow++)
		{
			for (UWord scrollCol(NTV2_CC608_MinCol);  scrollCol <= NTV2_CC608_MaxCol;  scrollCol++)
			{
				SetScreenCharacter (GetCurrentScreen(), scrollRow, scrollCol, GetScreenCharacter(GetCurrentScreen(), scrollRow + 1, scrollCol));
				SetScreenAttributes (GetCurrentScreen(), scrollRow, scrollCol, GetScreenAttributes(GetCurrentScreen(), scrollRow + 1, scrollCol));
			}
		}

		//	Erase current row and reset column to 1...
		for (UWord col(0);  col <= NTV2_CC608_MaxCol;  col++)
		{
			SetScreenCharacter (GetCurrentScreen(), mRollBaseRow, col, 0);
			SetScreenAttributes (GetCurrentScreen(), mRollBaseRow, col, NTV2Line21Attrs());
		}

		SetColumn(NTV2_CC608_MinCol);
	}	//	if rollRows > 0
	//else
		//	FCC Specs say "Carriage Returns have no effect on cursor location" in PopUp or PaintOn modes

	return true;

}	//	DoCarriageReturn


// DoEraseNonDisplayedMemory()
//		Erase all characters (i.e. clear to "transparent space") in off-screen buffer
//
bool CNTV2CaptionDecodeChannel608::DoEraseNonDisplayedMemory (void)
{
	AJAAutoLock	autoLock (MyScreenLock);
	return EraseScreen(GetCurrentScreen() == 0  ?  1  :  0);

}	//	DoEraseNonDisplayedMemory


// DoEndOfCaption()
//		Make the offscreen onscreen, and the onscreen offscreen
//
bool CNTV2CaptionDecodeChannel608::DoEndOfCaption (void)
{
	AJAAutoLock	autoLock (MyScreenLock);
	SetCaptionMode(NTV2_CC608_CapModePopOn);
	SetCurrentScreen(GetCurrentScreen() == 0 ? 1 : 0);

	return true;

}	//	DoEndOfCaption


// EraseScreen()
//		Erase all characters (i.e. clear to "transparent space") in designated buffer
//
bool CNTV2CaptionDecodeChannel608::EraseScreen (const UWord inScreenNum)
{
	NTV2Line21Attrs	attr;
	AJAAutoLock		autoLock(MyScreenLock);

	if (inScreenNum > 1)
		return false;

	for (UWord row(NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
		for (UWord col(NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
		{
			SetScreenCharacter (inScreenNum, row, col, 0);
			SetScreenAttributes (inScreenNum, row, col, attr);
		}
	return true;

}	//	EraseScreen


// SetRow()
//		Sets current row
//
bool CNTV2CaptionDecodeChannel608::SetRow (const UWord inNewRow)
{
	AJAAutoLock	autoLock	(MyScreenLock);
	const UWord	oldRow		(GetRow());
	UWord		newRow		(inNewRow);

	if (newRow < NTV2_CC608_MinRow)
		newRow = NTV2_CC608_MinRow;	//	Clamp to MinRow -- not an error
	if (newRow > NTV2_CC608_MaxRow)
		newRow = NTV2_CC608_MaxRow;	//	Clamp to MaxRow -- not an error
	if (oldRow != newRow)
	{
		mRow = newRow;
		Notify_CurrentRowChanged(oldRow, newRow);
	}
	return true;

}	//	SetRow


// IncrementRow()
//		Adds <delta> to the current row
//		A negative <delta> will decrement the row.
//
bool CNTV2CaptionDecodeChannel608::IncrementRow (const int inDelta)
{
	AJAAutoLock	autoLock (MyScreenLock);
	return SetRow(UWord(int(GetRow()) + inDelta));	//	Add new increment

}	//	IncrementRow


// SetColumn()
//		Sets current column
//
bool CNTV2CaptionDecodeChannel608::SetColumn (const UWord inNewCol)
{
	AJAAutoLock	autoLock	(MyScreenLock);
	const UWord	oldCol		(GetColumn());
	UWord		newCol		(inNewCol);

	if (newCol < NTV2_CC608_MinCol)
		newCol = NTV2_CC608_MinCol;	//	Clamp to MinCol -- not an error
	if (newCol > NTV2_CC608_MaxCol)
		newCol = NTV2_CC608_MaxCol;	//	Clamp to MaxCol -- not an error
	if (oldCol != newCol)
	{
		mCol = newCol;
		Notify_CurrentColumnChanged (oldCol, newCol);
	}
	return true;

}	//	SetColumn


// IncrementColumn()
//		Adds <delta> to the current column.
//		A negative <delta> will decrement the column.
//
bool CNTV2CaptionDecodeChannel608::IncrementColumn (const int inDelta)
{
	AJAAutoLock	autoLock (MyScreenLock);
	return SetColumn(UWord(int(GetColumn()) + inDelta));	//	Add new increment

}	//	IncrementColumn


// SetCaptionMode()
//		Sets current caption mode (Pop-On, Roll-Up, Paint-On, etc)
//
bool CNTV2CaptionDecodeChannel608::SetCaptionMode (const NTV2Line21Mode inNewCaptionMode)
{
	AJAAutoLock				autoLock		(MyScreenLock);
	const NTV2Line21Mode	oldCaptionMode	(GetCaptionMode());

	if (!IsValidLine21Mode(inNewCaptionMode))
		return false;

	//	The rules say that switching to a RollUp mode should erase any previous PopOn or PaintOn captions,
	//	and put the cursor back to the first column (but there should be a PAC code following this anyway...?)
	//	(see CEA-608-C, section C.10)
	if (IsLine21RollUpMode(inNewCaptionMode))
	{
		if (IsLine21PopOnMode(oldCaptionMode) || IsLine21PaintOnMode(oldCaptionMode))
		{
			EraseScreen(0);
			EraseScreen(1);

			SetColumn(NTV2_CC608_MinCol);
			SetRow(mRollBaseRow);
		}
	}

	if (oldCaptionMode != inNewCaptionMode)
	{
		mCaptionMode = inNewCaptionMode;
		Notify_CaptionModeChanged (oldCaptionMode, inNewCaptionMode);
	}
	return true;

}	//	SetCaptionMode


bool CNTV2CaptionDecodeChannel608::SetCurrentScreen (const UWord inNewScreen)
{
	AJAAutoLock	autoLock	(MyScreenLock);
	const UWord	oldScreen	(mCurrScreen);
	if (inNewScreen && inNewScreen != 1)
		return false;

	if (inNewScreen != oldScreen)
	{
		mCurrScreen = inNewScreen;
		Notify_CurrentScreenChanged (oldScreen, inNewScreen);
	}
	return true;
}


// MoveRollUpWindow()
//		Moves RollUp mode "window" to new base row
//
void CNTV2CaptionDecodeChannel608::MoveRollUpWindow (const UWord inNewBaseRow)
{
	AJAAutoLock	autoLock (MyScreenLock);
	UWord		numRows (0);

	//	Sanity check...
	if (IsValidLine21Row(inNewBaseRow) && IsLine21RollUpMode(GetCaptionMode()))
	{
		//	How big is the "window"?
		switch (GetCaptionMode())
		{
			case NTV2_CC608_CapModeRollUp2: numRows = 2;	break;
			case NTV2_CC608_CapModeRollUp3:	numRows = 3;	break;
			case NTV2_CC608_CapModeRollUp4:	numRows = 4;	break;
			default:						AJACC_ASSERT(false);	break;
		}

		//	Make sure we don't have a situation where the base row is higher than the number of rows in the current mode...
		if (numRows > mRollBaseRow)
			numRows = mRollBaseRow;

		//	The simplest (aka brute force...) thing is to copy the current "window" to
		//	a temp buffer, erase the screen, then copy them back to the new location...
		ULWord			tmpScreen	[4][NTV2_CC608_MaxCol+1];
		NTV2Line21Attrs	tmpAttrs	[4][NTV2_CC608_MaxCol+1];
		UWord			row(0),  col(0);

		//	Copy current RollUp "window"...
		//	Note that we're copying from the bottom up: that way, if we clip the upper rows...
		for (row = 0;  row < numRows;  row++)
		{
			for (col = NTV2_CC608_MinCol;  col <= NTV2_CC608_MaxCol;  col++)
			{
				tmpScreen	[row][col] = GetScreenCharacter(GetCurrentScreen(), mRollBaseRow - row, col);
				tmpAttrs	[row][col] = GetScreenAttributes(GetCurrentScreen(), mRollBaseRow - row, col);
			}
		}

		//	Erase the screen...
		EraseScreen(GetCurrentScreen());

		//	Make sure our new base row has enough room "above" it to hold the RollUp window...
		if (numRows > inNewBaseRow)
			numRows = inNewBaseRow;

		//	Copy RollUp "window" to new location  (like the first copy, we're copying from bottom up)...
		for (row = 0;  row < numRows;  row++)
			for (col = NTV2_CC608_MinCol;  col <= NTV2_CC608_MaxCol;  col++)
			{
				SetScreenCharacter(GetCurrentScreen(), inNewBaseRow - row, col, tmpScreen[row][col]);
				SetScreenAttributes(GetCurrentScreen(), inNewBaseRow - row, col, tmpAttrs[row][col]);
			}

		mRollBaseRow = inNewBaseRow;
	}	//	if valid row && roll-up mode
}	//	MoveRollUpWindow


UByte CNTV2CaptionDecodeChannel608::GetOnAirCharacter (const UWord inRow, const UWord inCol, NTV2Line21Attrs & outAttr) const
{
	UByte	result	(0);

	outAttr.Clear ();
	if (IsValidLine21Row(inRow) && IsValidLine21Column(inCol))
	{
		AJAAutoLock					autoLock	(MyScreenLock);
		const NTV2_CC608_CodePoint	codePoint	(GetScreenCharacter(GetCurrentScreen(), inRow, inCol));
		result = NTV2CCFont::GetInstance().GetCCFontCharCode(codePoint);
		GetCharacterAttributes (GetCurrentScreen(), inRow, inCol, outAttr);
	}
	return result;

}	//	GetOnAirCharacter (UByte)


// GetOnAirCharacter()
//		Returns the UTF8 character string that represents the character at [row][col] in the current on-air screen.
//		Note: "row" and "col" are 1-based - if row and/or column are out of bounds, this returns an empty string.
//
string CNTV2CaptionDecodeChannel608::GetOnAirCharacter (const UWord inRow, const UWord inCol) const
{
	string	result;

	if (IsValidLine21Row(inRow) && IsValidLine21Column(inCol))
	{
		AJAAutoLock					autoLock	(MyScreenLock);
		const NTV2_CC608_CodePoint	codePoint	(GetScreenCharacter (GetCurrentScreen(), inRow, inCol));
		result = ::NTV2CC608CodePointToUtf8String(codePoint);
		if (result.empty())
			result = " ";
		//Log() << "## " << GetLogLabel () << "[" << ::NTV2Line21ChannelToStr (mChannel) << "]:  GetOnAirCharacter row " << inRow << ", inCol " << inCol << ", theChar=0x" << hex << unsigned (theChar) << ", 608a=0x" << hex << unsigned (c608_1) << ", 608b=0x" << hex << unsigned (c608_2) << dec << ", result='" << result << "'" << endl;
	}
	return result;

}	//	GetOnAirCharacter (string)


string CNTV2CaptionDecodeChannel608::GetOnAirCharacterWithAttributes (const UWord inRow, const UWord inCol, NTV2Line21Attrs & outAttr) const
{
	string	result;

	outAttr.Clear();
	if (IsValidLine21Row(inRow) && IsValidLine21Column(inCol))
	{
		AJAAutoLock					autoLock	(MyScreenLock);
		const NTV2_CC608_CodePoint	codePoint	(GetScreenCharacter (GetCurrentScreen(), inRow, inCol));
		result = ::NTV2CC608CodePointToUtf8String(codePoint);
		if (result.empty())
			result = " ";
		GetCharacterAttributes (GetCurrentScreen(), inRow, inCol, outAttr);
		//Log() << "## " << GetLogLabel () << "[" << ::NTV2Line21ChannelToStr (mChannel) << "]:  GetOnAirCharacter row " << inRow << ", inCol " << inCol << ", theChar=0x" << hex << unsigned (theChar) << ", 608a=0x" << hex << unsigned (c608_1) << ", 608b=0x" << hex << unsigned (c608_2) << dec << ", result='" << result << "'" << endl;
	}
	return result;

}	//	GetOnAirCharacterWithAttributes


UWord CNTV2CaptionDecodeChannel608::GetOnAirUTF16CharacterWithAttributes (const UWord inRow, const UWord inCol, NTV2Line21Attrs & outAttr) const
{
	UWord	result	(0x0000);

	outAttr.Clear();
	if (IsValidLine21Row(inRow) && IsValidLine21Column(inCol))
	{
		AJAAutoLock					autoLock	(MyScreenLock);
		const NTV2_CC608_CodePoint	codePoint	(GetScreenCharacter (GetCurrentScreen(), inRow, inCol));
		result = ::NTV2CC608CodePointToUtf16Char(codePoint);
		GetCharacterAttributes (GetCurrentScreen(), inRow, inCol, outAttr);
	}
	return result;
}


NTV2Line21CharacterSet CNTV2CaptionDecodeChannel608::GetOnAirCharacterSet (const UWord inRow, const UWord inCol) const
{
	if (IsValidLine21Row(inRow) && IsValidLine21Column(inCol))
	{
		AJAAutoLock	autoLock(MyScreenLock);
		return ::GetLine21CharacterSet(GetScreenCharacter (GetCurrentScreen(), inRow, inCol));
	}
	else
		return NTV2_CC608_NumCharacterSets;		//	invalid
}


// GetOnAirCharacterAttributes()
//		Returns the attributes for the character at [row][col] from the designated screen.
//		Note: "row" and "col" are 1-based - if row and/or column are out of bounds, this returns 'false'
//			  and leaves the passed Attributes struct untouched.
//
bool CNTV2CaptionDecodeChannel608::GetCharacterAttributes (UWord inScreen, const UWord inRow, const UWord inCol, NTV2Line21Attrs & outAttr) const
{
	outAttr.Clear();
	if (!IsValidLine21Row(inRow) || !IsValidLine21Column(inCol))
		return false;

	//	Starting with the current column, search backwards through this row until an "IsSet" attribute is found.
	//	If none found, use the default (White)...
	AJAAutoLock	autoLock	(MyScreenLock);
	UWord		col			(inCol);
	while (col >= NTV2_CC608_MinCol)
	{
		const NTV2Line21Attrs attr (GetScreenAttributes(inScreen, inRow, col));
		if (attr.IsSet())
		{
			outAttr = attr;
			//Log() << "## " << GetLogLabel () << "[" << ::NTV2Line21ChannelToStr (mChannel) << "]:  GetCharacterAttributes:  Found attributes at row " << inRow << " col " << col << ":  " << outAttr << endl;
			return true;	//	Done!
		}
		col--;
	}

	outAttr.Clear();	//	None found -- use the default
	return true;

}	//	GetCharacterAttributes


bool CNTV2CaptionDecodeChannel608::SetTextModeDisplayRowCount (const UWord inNumRows)
{
	if (!IsValidLine21Row(inNumRows))
		return false;

	mTextRows = inNumRows;
	return true;
}


static string CopyString (const UWord inCopies, const string & inStringToCopy)
{
	ostringstream oss;
	for (UWord nCopy(0);  nCopy < inCopies;  nCopy++)
		oss << inStringToCopy;
	return oss.str();
}


string CNTV2CaptionDecodeChannel608::GetDebugPrintRow (const UWord inRow, const bool inShowChars) const
{
	AJAAutoLock		autoLock (MyScreenLock);
	ostringstream	oss;
	for (UWord col(NTV2_CC608_MinCol);  col <= NTV2_CC608_MaxCol;  col++)
	{
		if (inShowChars)
		{
			const NTV2_CC608_CodePoint	codePoint	(GetScreenCharacter(GetCurrentScreen(), inRow, col));
			const string				utf8str		(::NTV2CC608CodePointToUtf8String(codePoint));
			oss << (utf8str.empty() ? " " : utf8str);
		}
		else
		{
			NTV2Line21Attrs	attr;
			const string	hexStr	(GetCharacterAttributes (GetCurrentScreen(), inRow, col, attr) ? attr.GetHexString() : "xxx");
			AJACC_ASSERT (hexStr.length() == 3);
			oss << " " << hexStr;
		}
	}	//	for each column
	return oss.str ();
}


string CNTV2CaptionDecodeChannel608::GetDebugPrintScreen (const bool inShowChars) const
{
	AJAAutoLock		autoLock (MyScreenLock);
	ostringstream	oss;
	oss << gThinBox[kTopLeft] << CopyString (inShowChars ? NTV2_CC608_MaxCol : NTV2_CC608_MaxCol * 4, gThinBox[kHorzLine]) << gThinBox[kTopRight] << endl;

	for (UWord row(NTV2_CC608_MinRow);  row <= NTV2_CC608_MaxRow;  row++)
		oss << gThinBox[kVertLine] << GetDebugPrintRow(row, inShowChars) << gThinBox[kVertLine] << endl;

	oss << gThinBox[kBotLeft] << CopyString (inShowChars ? NTV2_CC608_MaxCol : NTV2_CC608_MaxCol * 4, gThinBox[kHorzLine]) << gThinBox[kBotRight] << endl;
	return oss.str();

}	//	DebugPrintScreen


void CNTV2CaptionDecodeChannel608::SetDebugRowsOfInterest (const UWord inFromRow, const UWord inToRow, const bool inAdd)
{
	if (!inAdd)
		mDebugRows.clear();
	for (UWord rowNum(inFromRow);  rowNum <= inToRow;  rowNum++)
		mDebugRows.insert(rowNum);
	if (mDebugRows.empty())
		LOGMYDEBUG(AJA_DebugUnit_CC608DecodeScreen, "Not filtering rows");
	else
		LOGMYDEBUG(AJA_DebugUnit_CC608DecodeScreen, "Filtering rows " << Line21RowSetToString(mDebugRows));
}


void CNTV2CaptionDecodeChannel608::SetDebugColumnsOfInterest (const UWord inFromCol, const UWord inToCol, const bool inAdd)
{
	if (!inAdd)
		mDebugCols.clear();
	for (UWord colNum(inFromCol);  colNum <= inToCol;  colNum++)
		mDebugCols.insert(colNum);
	if (mDebugCols.empty())
		LOGMYDEBUG(AJA_DebugUnit_CC608DecodeScreen, "Not filtering columns");
	else
		LOGMYDEBUG(AJA_DebugUnit_CC608DecodeScreen, "Filtering columns " << Line21ColumnSetToString(mDebugCols));
}


// Check608Parity()
//		Check parity of both incoming characters - return true if BOTH good
//
bool Check608Parity (UByte char608_1, UByte char608_2, string & outDebugStr)
{
	const bool	bParity1_OK	(char608_1 == ::Add608OddParity(char608_1 & 0x7f));
	const bool	bParity2_OK	(char608_2 == ::Add608OddParity(char608_2 & 0x7f));

	if (!bParity1_OK && bParity2_OK)
		outDebugStr = "Chr1 parity bad";
	else if ( bParity1_OK && !bParity2_OK)
		outDebugStr = "Chr2 parity bad";
	else if (!bParity1_OK && !bParity2_OK)
		outDebugStr = "Chr1 and Chr2 parity bad";
	return bParity1_OK & bParity2_OK;

}	//	Check608Parity


// Add608OddParity()
//		Add odd parity to 608 character
//
UByte Add608OddParity (UByte ch)
{
	UByte	result	(ch);

	//	Count the number of ones in the ls 7 bits...
	unsigned	ones	(0);
	for (int i(0);  i < 7;  i++)
	{
		if (ch & 0x01)
			ones++;
		ch = ch >> 1;
	}

	//	If there are an even number of ones, add another in bit 7...
	if (ones % 2)
		result &= 0x7F;	//	Odd -- no parity bit
	else
		result |= 0x80;	//	Even -- add parity bit

	return result;

}	//	Add608OddParity


bool CNTV2CaptionDecodeChannel608::SubscribeChangeNotification (NTV2Caption608Changed * pCallback, void * pUserData)
{
	mpCallback = pCallback;
	mpUserData = pUserData;
	return true;
}


bool CNTV2CaptionDecodeChannel608::UnsubscribeChangeNotification (NTV2Caption608Changed * pCallback, void * pUserData)
{
	if (mpCallback == pCallback && mpUserData == pUserData)
	{
		mpCallback = AJA_NULL;
		mpUserData = AJA_NULL;
		return true;
	}
	return false;
}


void CNTV2CaptionDecodeChannel608::Notify_ChannelChanged (const NTV2Line21Channel inOldChannel, const NTV2Line21Channel inNewChannel) const
{
	if (mpCallback)
		(*mpCallback) (mpUserData, NTV2Caption608ChangeInfo (inOldChannel, inNewChannel));
	LOGMYINFO(AJA_DebugUnit_CC608DecodeChannel, "[S" << GetCurrentScreen () << "R" << DEC02(GetRow()) << "C" << DEC02(GetColumn())
				<< "]  Channel changed:  " << ::NTV2Line21ChannelToStr(inOldChannel) << "  ==>  " << ::NTV2Line21ChannelToStr(inNewChannel));
}


void CNTV2CaptionDecodeChannel608::Notify_CurrentRowChanged (const UWord inOldRow, const UWord inNewRow) const
{
	if (mpCallback)
		(*mpCallback) (mpUserData, NTV2Caption608ChangeInfo (mChannel, NTV2Caption608ChangeInfo::NTV2DecoderChange_CurrentRow, inOldRow, inNewRow));
	if (!mDebugRows.empty() && mDebugRows.find(inOldRow) == mDebugRows.end() && mDebugRows.find(inNewRow) == mDebugRows.end())
		return;	//	skip if debugging rows-of-interest, and old or new row wasn't of interest
	LOGMYINFO(AJA_DebugUnit_CC608DecodeChannel, "[S" << GetCurrentScreen() << "R" << DEC02(GetRow()) << "C" << DEC02(GetColumn())
													<< "]  Row changed:  " << inOldRow << "  ==>  " << inNewRow);
}


void CNTV2CaptionDecodeChannel608::Notify_CurrentColumnChanged (const UWord inOldCol, const UWord inNewCol) const
{
	if (mpCallback)
		(*mpCallback) (mpUserData, NTV2Caption608ChangeInfo (mChannel, NTV2Caption608ChangeInfo::NTV2DecoderChange_CurrentColumn, inOldCol, inNewCol));
	if (!mDebugCols.empty() && mDebugCols.find(inOldCol) == mDebugCols.end() && mDebugCols.find(inNewCol) == mDebugCols.end())
		return;	//	skip if debugging columns-of-interest, and old or new column wasn't of interest
	LOGMYINFO(AJA_DebugUnit_CC608DecodeChannel, "[S" << GetCurrentScreen() << "R" << DEC02(GetRow()) << "C" << DEC02(GetColumn())
													<< "]  Column changed:  " << inOldCol << "  ==>  " << inNewCol);
}


void CNTV2CaptionDecodeChannel608::Notify_CaptionModeChanged (const NTV2Line21Mode inOldMode, const NTV2Line21Mode inNewMode) const
{
	if (mpCallback)
		(*mpCallback) (mpUserData, NTV2Caption608ChangeInfo (mChannel, NTV2Caption608ChangeInfo::NTV2DecoderChange_CaptionMode, UWord(inOldMode), UWord(inNewMode)));
	LOGMYNOTE(AJA_DebugUnit_CC608DecodeChannel, "[S" << GetCurrentScreen() << "R" << DEC02(GetRow()) << "C" << DEC02(GetColumn())
										<< "]  Mode changed:  " << NTV2Line21ModeToStr(inOldMode) << "  ==>  " << NTV2Line21ModeToStr(inNewMode));
}


void CNTV2CaptionDecodeChannel608::Notify_CurrentScreenChanged	(const UWord inOldScreen, const UWord inNewScreen) const
{
	if (mpCallback)
		(*mpCallback) (mpUserData, NTV2Caption608ChangeInfo (mChannel, NTV2Caption608ChangeInfo::NTV2DecoderChange_CurrentScreen, inOldScreen, inNewScreen));
	LOGMYNOTE(AJA_DebugUnit_CC608DecodeScreen, "[S" << GetCurrentScreen() << "R" << DEC02(GetRow()) << "C" << DEC02(GetColumn())
										<< "]  Screen changed:  " << inOldScreen << "  ==>  " << inNewScreen);
}


void CNTV2CaptionDecodeChannel608::Notify_ScreenCharChanged (const UWord inScreenNum, const UWord inRow, const UWord inCol,
																const NTV2_CC608_CodePoint inOldCodePoint, const NTV2_CC608_CodePoint inNewCodePoint) const
{
	if (mpCallback)
		(*mpCallback) (mpUserData, NTV2Caption608ChangeInfo (mChannel, GetCurrentScreen (), inRow, inCol, inOldCodePoint, inNewCodePoint));
	if (AJADebug::IsActive(AJA_DebugUnit_CC608DecodeScreen))
	{
		if (!mDebugRows.empty() && mDebugRows.find(inRow) == mDebugRows.end())
			return;	//	skip if debugging rows-of-interest, and row isn't of interest
		if (!mDebugCols.empty() && mDebugCols.find(inCol) == mDebugCols.end())
			return;	//	skip if debugging columns-of-interest, and column isn't of interest

		const string	newChar	(::NTV2CC608CodePointToUtf8String(inNewCodePoint));
		const string	oldChar	(::NTV2CC608CodePointToUtf8String(inOldCodePoint));
		const string	newCharStr	(newChar.empty()  ?  "    "  :  " '" + newChar + "'");
		const string	oldCharStr	(oldChar.empty()  ?  "    "  :  " '" + oldChar + "'");

		LOGMYNOTE(AJA_DebugUnit_CC608DecodeScreen, "[S" << GetCurrentScreen() << "R" << DEC02(GetRow()) << "C" << DEC02(GetColumn())
				<< "]  Character changed at [S" << inScreenNum << "R" << DEC02(inRow) << "C" << DEC02(inCol)
				<< "]:  0x" << UHEX2(GetLine21CharacterSet(inOldCodePoint)) << UHEX2(Get608Byte1(inOldCodePoint)) << UHEX2(Get608Byte2(inOldCodePoint))
				<< oldCharStr
				<< "  ==>  0x" << UHEX2(GetLine21CharacterSet(inNewCodePoint)) << UHEX2(Get608Byte1(inNewCodePoint)) << UHEX2(Get608Byte2(inNewCodePoint))
				<< newCharStr);
	}
}


void CNTV2CaptionDecodeChannel608::Notify_ScreenAttrChanged (const UWord inScreenNum, const UWord inRow, const UWord inCol,
																const NTV2Line21Attrs & inOldAttr, const NTV2Line21Attrs & inNewAttr) const
{
	AJACC_ASSERT (&inOldAttr != &inNewAttr);
	if (mpCallback)
		(*mpCallback) (mpUserData, NTV2Caption608ChangeInfo (mChannel, GetCurrentScreen(), inRow, inCol, inOldAttr, inNewAttr));
	if (AJADebug::IsActive(AJA_DebugUnit_CC608DecodeScreen))
	{
		if (!mDebugRows.empty() && mDebugRows.find(inRow) == mDebugRows.end())
			return;	//	skip if debugging rows-of-interest, and old or new row wasn't of interest
		if (!mDebugCols.empty() && mDebugCols.find(inCol) == mDebugCols.end())
			return;	//	skip if debugging columns-of-interest, and old or new column wasn't of interest

		LOGMYNOTE(AJA_DebugUnit_CC608DecodeScreen, "[S" << GetCurrentScreen() << "R" << DEC02(GetRow()) << "C" << DEC02(GetColumn())
				<< "]  Screen " << inScreenNum << " Row " << DEC02(inRow) << " Col " << DEC02(inCol) << " attribute changed to '"
				<< inNewAttr << "' (was '" << inOldAttr << "')");
	}
}
