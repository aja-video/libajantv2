/**
	@file		ntv2caption608types.cpp
	@brief		Implementation of 608/SD-related utility functions and classes.
	@copyright	(C) 2005-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include "ntv2caption608types.h"
#include "ntv2captionlogging.h"
#include "ntv2transcode.h"
#include "ajabase/common/common.h"
#include <map>
#include <sstream>
#include <algorithm>


using namespace std;


#if defined (MSWindows)
	#pragma warning(disable: 4800)	//	Ignore int/bool
#endif



/////////////////////////////////////////////////////////////////////////////////////////////	CaptionData

CaptionData::CaptionData (const UByte inF1Char1, const UByte inF1Char2)
{
	bGotField1Data = inF1Char1 != 0xFF  &&  inF1Char2 != 0xFF;
	bGotField2Data = bGotField3Data = false;
	f1_char1 = inF1Char1;
	f1_char2 = inF1Char2;
	f2_char1 = f2_char2 = f3_char1 = f3_char2 = 0xFF;
}

CaptionData::CaptionData (const UByte inF1Char1, const UByte inF1Char2, const UByte inF2Char1, const UByte inF2Char2)
{
	bGotField1Data = inF1Char1 != 0xFF  &&  inF1Char2 != 0xFF;
	bGotField2Data = inF2Char1 != 0xFF  &&  inF2Char2 != 0xFF;
	bGotField3Data = false;
	f1_char1 = inF1Char1;
	f1_char2 = inF1Char2;
	f2_char1 = inF2Char1;
	f2_char2 = inF2Char2;
	f3_char1 = f3_char2 = 0xFF;
}

//	Compare
bool CaptionData::operator == (const CaptionData & inRHS) const
{
	//	To be considered identical, LHS/RHS "got data" must match...
	if (bGotField1Data != inRHS.bGotField1Data)
		return false;
	if (bGotField2Data != inRHS.bGotField2Data)
		return false;
	if (bGotField3Data != inRHS.bGotField3Data)
		return false;
	if (bGotField1Data)
		if (f1_char1 != inRHS.f1_char1  ||  f1_char2 != inRHS.f1_char2)
			return false;
	if (bGotField2Data)
		if (f2_char1 != inRHS.f2_char1  ||  f2_char2 != inRHS.f2_char2)
			return false;
	if (bGotField3Data)
		if (f3_char1 != inRHS.f3_char1  ||  f3_char2 != inRHS.f3_char2)
			return false;
	return true;
}

bool CaptionData::operator < (const CaptionData & inRHS) const
{
	uint64_t	me(0), rhs(0);
	if (bGotField1Data)
		me |= (uint64_t(f1_char1) << 40) || (uint64_t(f1_char2) << 32);
	if (bGotField2Data)
		me |= (uint64_t(f2_char1) << 24) || (uint64_t(f2_char2) << 16);
	if (bGotField3Data)
		me |= (uint64_t(f3_char1) <<  8) || (uint64_t(f3_char2)      );

	if (inRHS.bGotField1Data)
		rhs |= (uint64_t(inRHS.f1_char1) << 40) || (uint64_t(inRHS.f1_char2) << 32);
	if (inRHS.bGotField2Data)
		rhs |= (uint64_t(inRHS.f2_char1) << 24) || (uint64_t(inRHS.f2_char2) << 16);
	if (inRHS.bGotField3Data)
		rhs |= (uint64_t(inRHS.f3_char1) <<  8) || (uint64_t(inRHS.f3_char2)      );

	return me < rhs;
}


ostream & operator << (ostream & inOutStream, const CaptionData & inData)
{
	inOutStream << "CapDat[";
	if (inData.bGotField1Data)
		inOutStream	<< ::NTV2Line21FieldToStr(NTV2_CC608_Field1) << ":x" << UHEX2(inData.f1_char1) << "|x" << UHEX2(inData.f1_char2)
					<< "(x" << UHEX2(inData.f1_char1 & 0x7F) << "|x" << UHEX2(inData.f1_char2 & 0x7F) << ")";
	if (inData.bGotField2Data)
		inOutStream	<< (inData.bGotField1Data ? " " : "")
					<< ::NTV2Line21FieldToStr(NTV2_CC608_Field2) << ":x" << UHEX2(inData.f2_char1) << "|x" << UHEX2(inData.f2_char2)
					<< "(x" << UHEX2(inData.f2_char1 & 0x7F) << "|x" << UHEX2(inData.f2_char2 & 0x7F) << ")";
	if (inData.bGotField3Data)
		inOutStream	<< (inData.bGotField1Data || inData.bGotField2Data ? " " : "")
					<< "30/24fps x" << UHEX2(inData.f3_char1) << "|x" << UHEX2(inData.f3_char2)
					<< "(x" << UHEX2 (inData.f3_char1 & 0x7F) << "|x" << UHEX2 (inData.f3_char2 & 0x7F) << ")";
	return inOutStream << "]";

}	//	operator <<


static const string		gLine21FieldToStr []	= {"F?", "F1", "F2", ""};
static const string		gLine21ChannelToStr []	= {"CC1", "CC2", "CC3", "CC4", "Tx1", "Tx2", "Tx3", "Tx4", "XDS", ""};
static const string		gLine21ChannelToStr2[]	= {"NTV2_CC608_CC1", "NTV2_CC608_CC2", "NTV2_CC608_CC3", "NTV2_CC608_CC4", "NTV2_CC608_Text1", "NTV2_CC608_Text2", "NTV2_CC608_Text3", "NTV2_CC608_Text4", "NTV2_CC608_XDS", ""};
static const string		gLine21ModeToStr []		= {"None", "PopOn", "RollUp2", "RollUp3", "RollUp4", "PaintOn", ""};
static const string		gLine21ColorToStr []	= {"wht", "grn", "blu", "cyn", "red", "yel", "mag", "blk", ""};
static const string		gLine21ColorToStr2 []	= {"White", "Green", "Blue", "Cyan", "Red", "Yellow", "Magenta", "Black", ""};
static const string		gLine21OpacityToStr []	= {"opq", "tsl", "tsp", ""};
static const string		gLine21OpacityToStr2 []	= {"Opaque", "Translucent", "Transparent", ""};
static const string		gLine21CharSetToStr []	= {"Default", "DoubleSize", "Private1", "Private2", "PRChina", "Korean", "Registered1", ""};
static const string		gEmptyString;


const string & NTV2Line21FieldToStr (const NTV2Line21Field inEnumVal)
{
	return IsValidLine21Field(inEnumVal) ? gLine21FieldToStr[inEnumVal] : gEmptyString;
}

const string & NTV2Line21ChannelToStr (const NTV2Line21Channel inEnumVal, const bool inCompact)
{
	if IsValidLine21Channel(inEnumVal)
		return inCompact ? gLine21ChannelToStr[inEnumVal] : gLine21ChannelToStr2[inEnumVal];
	return gEmptyString;
}

const string & NTV2Line21ModeToStr (const NTV2Line21Mode inEnumVal)
{
	return IsValidLine21Mode(inEnumVal) ? gLine21ModeToStr[inEnumVal] : gEmptyString;
}

const string & NTV2Line21ColorToStr (const NTV2Line21Color inEnumVal, const bool inCompact)
{
	if (IsValidLine21Color(inEnumVal))
		return inCompact  ?  gLine21ColorToStr[inEnumVal]  :  gLine21ColorToStr2[inEnumVal];
	return gEmptyString;
}

const string & NTV2Line21OpacityToStr (const NTV2Line21Opacity inEnumVal, const bool inCompact)
{
	if (IsValidLine21Opacity(inEnumVal))
		return inCompact ? gLine21OpacityToStr[inEnumVal] : gLine21OpacityToStr2[inEnumVal];
	return gEmptyString;
}

const string & NTV2Line21CharacterSetToStr (const NTV2Line21CharacterSet inEnumVal)
{
	return IsValidLine21CharacterSet(inEnumVal) ? gLine21CharSetToStr[inEnumVal] : gEmptyString;
}


bool NTV2Line21ColorToYUV8 (const NTV2Line21Color inLine21Color, UByte & outY, UByte & outCb, UByte & outCr)
{
	switch (inLine21Color)
	{								//	All values 75% Color Bar, with some exceptions:
		case NTV2_CC608_White:		outY = 180;		outCb = 128;	outCr = 128;	break;
		case NTV2_CC608_Green:		outY = 112;		outCb = 72;		outCr = 58;		break;
		case NTV2_CC608_Blue:		outY = 41;		outCb = 240;	outCr = 110;	break;	// 100% level to increase brightness
		case NTV2_CC608_Cyan:		outY = 131;		outCb = 156;	outCr = 44;		break;
		case NTV2_CC608_Red:		outY = 81;		outCb = 90;		outCr = 240;	break;	// 100% level to increase brightness
		case NTV2_CC608_Yellow:		outY = 162;		outCb = 44;		outCr = 142;	break;
		case NTV2_CC608_Magenta:	outY = 84;		outCb = 184;	outCr = 198;	break;
		case NTV2_CC608_Black:		outY = 10;		outCb = 128;	outCr = 128;	break;

		case NTV2_CC608_NumColors:
		default:																	return false;
	}
	return true;
}


bool NTV2Line21ColorToRGB8 (const NTV2Line21Color inLine21Color, UByte & outR, UByte & outG, UByte & outB, const bool inIsHD)
{
	YCbCrAlphaPixel	YCbCr;
	YCbCr.Alpha = 0;
	if (NTV2Line21ColorToYUV8 (inLine21Color, YCbCr.y, YCbCr.cb, YCbCr.cr))
	{
		RGBAlphaPixel	rgb;
		if (inIsHD)
			::HDConvertYCbCrtoRGBSmpte (&YCbCr, &rgb);
		else
			::SDConvertYCbCrtoRGBSmpte (&YCbCr, &rgb);
		outR = rgb.Red;
		outG = rgb.Green;
		outB = rgb.Blue;
		return true;
	}
	return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////	NTV2Line21Attributes

NTV2Line21Attributes::NTV2Line21Attributes ()
{
	Clear ();
}


NTV2Line21Attributes::NTV2Line21Attributes (const NTV2Line21Color	inFGColor,
											const NTV2Line21Color	inBGColor,
											const NTV2Line21Opacity	inOpacity,
											const bool				inItalics,
											const bool				inUnderline,
											const bool				inFlash)
{
	bFlash		= inFlash;
	bItalic		= inItalics;
	bUnderline	= inUnderline;
	fgColor		= inFGColor;
	bgColor		= inBGColor;
	bgOpacity	= inOpacity;
}


string NTV2Line21Attributes::GetHexString (void) const
{
	if (IsSet())
	{
		ostringstream		oss;
		NTV2Line21Color		colorFG	(GetColor());
		NTV2Line21Color		colorBG	(GetBGColor());
		NTV2Line21Opacity	opacity (GetOpacity());
		uint16_t			value	(0x0000);

		if (!IsLine21WhiteColor(colorFG))	value |= (0x0007 & colorFG);
		if (!IsLine21BlackColor(colorBG))	value |= (0x0070 & (uint16_t(colorBG) << 4));
		if (opacity)		value |= (0x0300 & (uint16_t(opacity) << 8));
		if (IsFlashing())	value |= 0x0800;
		if (IsUnderlined())	value |= 0x0080;
		if (IsItalicized())	value |= 0x0008;
		oss << UHEX2((value & 0xFF00) >> 8)  <<  UHEX2(value & 0x00FF);
		return oss.str().substr(1,3);
	}
	return "   ";
}


string NTV2Line21AttributesToStr (const NTV2Line21Attributes inLine21Attributes)
{
	ostringstream	oss;
	oss << inLine21Attributes;
	return oss.str();
}


uint16_t NTV2Line21Attributes::GetHashKey (void) const
{
	uint16_t				value	(0x0000);
	const NTV2Line21Color	colorFG	(GetColor());
	const NTV2Line21Color	colorBG	(GetBGColor());
	const NTV2Line21Opacity	opacity (GetOpacity());

	value |= (0x0007 & colorFG);
	value |= (0x0070 & (uint16_t(colorBG) << 4));
	if (opacity)		value |= (0x0300 & (uint16_t(opacity) << 8));
	if (IsFlashing())	value |= 0x0800;
	if (IsUnderlined())	value |= 0x0080;
	if (IsItalicized())	value |= 0x0008;
	return value;
}


ostream & operator << (ostream & oss, const NTV2Line21Attributes & inData)
{
	if (IsValidLine21Color(inData.GetColor())
		&&  IsValidLine21Color(inData.GetBGColor())
		&&  IsValidLine21Opacity(inData.GetOpacity()))
			oss	<< "(" << ::NTV2Line21ColorToStr(inData.GetColor()) << "-"
				<< ::NTV2Line21ColorToStr(inData.GetBGColor()) << "-"
				<< ::NTV2Line21OpacityToStr(inData.GetOpacity()) << "-"
				<< (inData.IsFlashing() ? "F" : "f")
				<< (inData.IsItalicized() ? "I" : "i")
				<< (inData.IsUnderlined() ? "U" : "u") << ")";
	return oss;
}


NTV2Line21Attributes StrToNTV2Line21Attributes (const std::string & inStr)
{
	NTV2Line21Attributes result;
	if (inStr.length() != 17)
		return result;
	if (inStr.at(0) != '(' || inStr.at(16) != ')' || inStr.at(4) != '-' || inStr.at(8) != '-' || inStr.at(12) != '-')
		return result;

	const string			fgColorStr	(inStr.substr(1, 3));
	const string			bgColorStr	(inStr.substr(5, 3));
	const string			opacityStr	(inStr.substr(9, 3));
	const string			flagsStr	(inStr.substr(13, 3));

	const NTV2Line21Color	fgColor		(StrToNTV2Line21Color(fgColorStr));
	const NTV2Line21Color	bgColor		(StrToNTV2Line21Color(bgColorStr));
	const NTV2Line21Opacity	opacity		(StrToNTV2Line21Opacity(opacityStr));

	if (IsValidLine21Color(fgColor))	result.SetColor(fgColor);
	if (IsValidLine21Color(bgColor))	result.SetBGColor(bgColor);
	if (IsValidLine21Opacity(opacity))	result.SetOpacity(opacity);
	if (flagsStr.at(0) == 'F')			result.AddFlash();
	if (flagsStr.at(1) == 'I')			result.AddItalics();
	if (flagsStr.at(2) == 'U')			result.AddUnderline();
	return result;
}


typedef map<string, unsigned>	StringToEnumMap;

//	This really should be a template function
#define	StringToLine21Enum(T,FuncName,ToStrFunc,startVal,endVal,invalidVal)			T FuncName (const string & inStr)																			\
																					{																											\
																						static StringToEnumMap	g##T;																			\
																						string	lookupName (inStr);																				\
																						std::transform (lookupName.begin (), lookupName.end (), lookupName.begin (), ::tolower);				\
																						if (g##T.empty ())																						\
																							for (unsigned val (startVal); val < (endVal); val++)												\
																							{																									\
																								const T tmpEnum (static_cast <T> (val));														\
																								string	name	(ToStrFunc (tmpEnum));															\
																								std::transform (name.begin (), name.end (), name.begin (), ::tolower);							\
																								g##T [name] = val;																				\
																							}																									\
																						return (g##T.find (lookupName) != g##T.end ()) ? static_cast <T> (g##T [lookupName]) : (invalidVal);	\
																					}

//					Enum Type				Name of Function				EnumToStr Conversion Function	Start Enum Value				End Enum Value					Invalid Enum Value
StringToLine21Enum (NTV2Line21Field,		StrToNTV2Line21Field,			NTV2Line21FieldToStr,			NTV2_CC608_Field1,				NTV2_CC608_Field_Max,			NTV2_CC608_Field_Invalid)
StringToLine21Enum (NTV2Line21Channel,		StrToNTV2Line21Channel,			NTV2Line21ChannelToStr,			NTV2_CC608_CC1,					NTV2_CC608_ChannelMax,			NTV2_CC608_ChannelMax)
StringToLine21Enum (NTV2Line21Mode,			StrToNTV2Line21Mode,			NTV2Line21ModeToStr,			NTV2_CC608_CapModeUnknown,		NTV2_CC608_CapModeMax,			NTV2_CC608_CapModeUnknown)
StringToLine21Enum (NTV2Line21Color,		StrToNTV2Line21Color,			NTV2Line21ColorToStr,			NTV2_CC608_White,				NTV2_CC608_NumColors,			NTV2_CC608_NumColors)
StringToLine21Enum (NTV2Line21Opacity,		StrToNTV2Line21Opacity,			NTV2Line21OpacityToStr,			NTV2_CC608_Opaque,				NTV2_CC608_NumOpacities,		NTV2_CC608_NumOpacities)
StringToLine21Enum (NTV2Line21CharacterSet,	StrToNTV2Line21CharacterSet,	NTV2Line21CharacterSetToStr,	NTV2_CC608_DefaultCharacterSet,	NTV2_CC608_NumCharacterSets,	NTV2_CC608_NumCharacterSets)


string NTV2CodePointSetToString (const NTV2CodePointSet & inSet)
{
	ostringstream	oss;
	for (NTV2CodePointSetConstIter iter (inSet.begin ());  iter != inSet.end ();  )
	{
		const NTV2_CC608_CodePoint	codePoint	(*iter);
		oss << ::NTV2Line21CharacterSetToStr (::GetLine21CharacterSet (codePoint)) << "|" << UHEX2(::Get608Byte1 (codePoint)) << "|" << UHEX2(::Get608Byte2(codePoint));
		++iter;
		if (iter != inSet.end ())
			oss << ", ";
	}
	return oss.str ();
}


/////////////////////////////////////////////////////////////////////////////////////////////	NTV2Line21AttributePermutations

NTV2Line21AttributePermutations::NTV2Line21AttributePermutations ()
{
	unsigned	ndx	(0);

	//	Note that I'm not permuting with the "flashing" attribute -- it's not needed for glyph rendering
	for (unsigned opacity (NTV2_CC608_Opaque);  opacity < NTV2_CC608_NumOpacities;  opacity++)
		for (unsigned isUnderline (0);  isUnderline < 2;  isUnderline++)
			for (unsigned bgColor (NTV2_CC608_White);  bgColor < NTV2_CC608_NumColors;  bgColor++)
				for (unsigned isItalic (0);  isItalic < 2;  isItalic++)
					for (unsigned fgColor (NTV2_CC608_White);  fgColor < NTV2_CC608_NumColors;  fgColor++)
					{
						AJACC_ASSERT (ndx < 768);
						mAttribs [ndx] = NTV2Line21Attributes (NTV2Line21Color (fgColor), NTV2Line21Color (bgColor), NTV2Line21Opacity (opacity),
																	isItalic ? true : false, isUnderline ? true : false);
						AJACC_ASSERT (ndx == mAttribs [ndx].GetHashKey ());	//	These should match
						//AJACC_ASSERT (mKeyToIndexMap.find (mAttribs [ndx].GetHashKey ()) == mKeyToIndexMap.end ());
						//mKeyToIndexMap [mAttribs [ndx].GetHashKey ()] = ndx;
						//cerr << "## DEBUG:  " << mAttribs [ndx] << "  ndx 0x" << hex << ndx << dec << " (" << ndx << ") == key 0x" << hex << mAttribs [ndx].GetHashKey () << dec << " (" << mAttribs [ndx].GetHashKey () << ")" << endl;
						ndx++;
					}
	AJACC_ASSERT (ndx == 768);
}


const NTV2Line21Attrs & NTV2Line21AttributePermutations::GetPermutation (const size_t inIndex) const
{
	AJACC_ASSERT (inIndex < size ());
	return mAttribs [inIndex];
}


size_t NTV2Line21AttributePermutations::GetIndexFromAttribute (const NTV2Line21Attrs & inAttr) const
{
	NTV2Line21Attrs	attribWithoutFlash (inAttr);
	attribWithoutFlash.RemoveFlash();
	return attribWithoutFlash.GetHashKey();
	//KeyToIndexMapConstIter	iter	(mKeyToIndexMap.find (attribWithoutFlash.GetHashKey ()));
	//AJACC_ASSERT (iter != mKeyToIndexMap.end ());
	//return iter->second;
}


const NTV2Line21AttributePermutations	gAllAttributePermutations;


/////////////////////////////////////////////////////////////////////////////////////////////	NTV2Caption608ChangeInfo

NTV2Caption608ChangeInfo::NTV2Caption608ChangeInfo (const NTV2Line21Channel inOldChannel, const NTV2Line21Channel inNewChannel)
	:	mWhatChanged	(NTV2DecoderChange_CurrentChannel),
		mChannel		(inNewChannel)

{
	u.currentChannel.mOld	= UWord (inOldChannel);
}

NTV2Caption608ChangeInfo::NTV2Caption608ChangeInfo (const NTV2Line21Channel inChannel, const UWord inWhatChanged, const UWord inOldValue, const UWord inNewValue)
	:	mWhatChanged	(inWhatChanged),
		mChannel		(inChannel)

{
	u.currentRow.mOld	= inOldValue;
	u.currentRow.mNew	= inNewValue;
}

NTV2Caption608ChangeInfo::NTV2Caption608ChangeInfo (const NTV2Line21Channel inChannel, const UWord inScreen, const UWord inRow, const UWord inColumn, const NTV2_CC608_CodePoint inOldValue, const NTV2_CC608_CodePoint inNewValue)
	:	mWhatChanged	(NTV2DecoderChange_ScreenCharacter),
		mChannel		(inChannel)
{
	u.screenChar.mScreenNum	= inScreen;
	u.screenChar.mRow		= inRow;
	u.screenChar.mColumn	= inColumn;
	u.screenChar.mOld		= inOldValue;
	u.screenChar.mNew		= inNewValue;
}

NTV2Caption608ChangeInfo::NTV2Caption608ChangeInfo (const NTV2Line21Channel inChannel, const UWord inScreen, const UWord inRow, const UWord inColumn, const NTV2Line21Attributes & inOldValue, const NTV2Line21Attributes & inNewValue)
	:	mWhatChanged	(NTV2DecoderChange_ScreenAttribute),
		mChannel		(inChannel)
{
	u.screenAttr.mScreenNum	= inScreen;
	u.screenAttr.mRow		= inRow;
	u.screenAttr.mColumn	= inColumn;
	u.screenAttr.mOld		= inOldValue.GetHashKey ();
	u.screenAttr.mNew		= inNewValue.GetHashKey ();
}

NTV2Caption608ChangeInfo::NTV2Caption608ChangeInfo (const NTV2Line21Channel inChannel)
	:	mWhatChanged	(NTV2DecoderChange_DrawScreen),
		mChannel		(inChannel)
{
}

ostream & NTV2Caption608ChangeInfo::Print (std::ostream & inOutStrm) const
{
	if (mWhatChanged & NTV2DecoderChange_CurrentChannel)
		return inOutStrm << "{" << ::NTV2Line21ChannelToStr (mChannel) << " chl chgd from " << ::NTV2Line21ChannelToStr (NTV2Line21Channel (u.currentChannel.mOld)) << "}";
	else if (mWhatChanged & NTV2DecoderChange_CurrentRow)
		return inOutStrm << "{" << ::NTV2Line21ChannelToStr (mChannel) << " row chgd  " << u.currentRow.mOld << " => " << u.currentRow.mNew << "}";
	else if (mWhatChanged & NTV2DecoderChange_CurrentColumn)
		return inOutStrm << "{" << ::NTV2Line21ChannelToStr (mChannel) << " col chgd  " << u.currentColumn.mOld << " => " << u.currentColumn.mNew << "}";
	else if (mWhatChanged & NTV2DecoderChange_CurrentScreen)
		return inOutStrm << "{" << ::NTV2Line21ChannelToStr (mChannel) << " scrn chgd  " << u.currentScreen.mOld << " => " << u.currentScreen.mNew << "}";
	else if (mWhatChanged & NTV2DecoderChange_CaptionMode)
		return inOutStrm << "{" << ::NTV2Line21ChannelToStr (mChannel) << " mode chgd  " << u.captionMode.mOld << " => " << u.captionMode.mNew << "}";
	else if (mWhatChanged & NTV2DecoderChange_ScreenCharacter)
		return inOutStrm << "{" << ::NTV2Line21ChannelToStr (mChannel) << " char chgd  [S" << u.screenChar.mScreenNum << "R" << u.screenChar.mRow << "C" << u.screenChar.mColumn << "] '" << NTV2CC608CodePointToUtf8String (u.screenChar.mOld) << "' => '" << NTV2CC608CodePointToUtf8String (u.screenChar.mNew) << "'}";
	else if (mWhatChanged & NTV2DecoderChange_ScreenAttribute)
		return inOutStrm << "{" << ::NTV2Line21ChannelToStr (mChannel) << " attr chgd  [S" << u.screenAttr.mScreenNum << "R" << u.screenAttr.mRow << "C" << u.screenAttr.mColumn << "] " << gAllAttributePermutations.GetPermutation (u.screenAttr.mOld) << " => " << gAllAttributePermutations.GetPermutation (u.screenAttr.mNew) << "}";
	else if (mWhatChanged & NTV2DecoderChange_DrawScreen)
		return inOutStrm << "{" << ::NTV2Line21ChannelToStr (mChannel) << " draw screen}";
	else
		return inOutStrm << "{??}";
}

#ifdef MSWindows
	#pragma warning(default: 4800)	//	int/bool warnings
#endif
