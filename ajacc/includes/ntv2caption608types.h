/**
	@file		ntv2caption608types.h
	@brief		Declares several data types used with 608/SD captioning.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CAPTION608TYPES_
#define __NTV2_CAPTION608TYPES_

#include "ajatypes.h"
#include "ajaexport.h"
#include "ntv2utils.h"
#include <ostream>
#include <string>
#include <set>
#include <map>

#if !defined(NULL)
	#define NULL 0
#endif



/**
	@brief	The minimum row index number (located at the top of the screen).
	@note	All CEA-608 row/column indices are 1-based.
**/
const UWord			NTV2_CC608_MinRow	(1);

/**
	@brief	The maximum permissible row index number (located at the bottom of the screen).
	@note	All CEA-608 row/column indices are 1-based.
**/
const UWord			NTV2_CC608_MaxRow	(15);

/**
	@brief	The minimum column index number (located at the left edge of the screen).
	@note	All CEA-608 row/column indices are 1-based.
**/
const UWord			NTV2_CC608_MinCol	(1);

/**
	@brief	The maximum column index number (located at the right edge of the screen).
	@note	All CEA-608 row/column indices are 1-based.
**/
const UWord			NTV2_CC608_MaxCol	(32);


#define IsValidLine21Row(__row__)		((__row__) >= NTV2_CC608_MinRow && (__row__) <= NTV2_CC608_MaxRow)
#define IsValidLine21Column(__col__)	((__col__) >= NTV2_CC608_MinCol && (__col__) <= NTV2_CC608_MaxCol)


/**
	@brief	The two CEA-608 interlace fields.
**/
typedef enum NTV2Line21Field
{
	NTV2_CC608_Field_Invalid	= 0,
	NTV2_CC608_Field1			= 1,
	NTV2_CC608_Field2			= 2,
	NTV2_CC608_Field_Max		= NTV2_CC608_Field2

} NTV2Line21Field;


#define IsValidLine21Field(_field_)		((_field_) == NTV2_CC608_Field1 || (_field_) == NTV2_CC608_Field2)
#define IsLine21Field1(_field_)			((_field_) == NTV2_CC608_Field1)
#define IsLine21Field2(_field_)			((_field_) == NTV2_CC608_Field2)


/**
	@brief		Converts the given NTV2Line21Field value into a human-readable string.
	@param[in]	inLine21Field	Specifies the value to be converted.
	@return		The human-readable string.
**/
AJAExport const std::string &		NTV2Line21FieldToStr (const NTV2Line21Field inLine21Field);


/**
	@brief	The CEA-608 caption channels:  CC1 thru CC4, TX1 thru TX4, plus XDS.
**/
typedef enum NTV2Line21Channel
{
	NTV2_CC608_CC1,		///< @brief	Caption channel 1, the primary caption channel.
	NTV2_CC608_CC2,		///< @brief	Caption channel 2, the secondary caption channel.
	NTV2_CC608_CC3,
	NTV2_CC608_CC4,

	NTV2_CC608_TextChannelOffset,
	NTV2_CC608_Text1 = NTV2_CC608_TextChannelOffset,
	NTV2_CC608_Text2,
	NTV2_CC608_Text3,
	NTV2_CC608_Text4,

	NTV2_CC608_XDS,
	
	NTV2_CC608_ChannelMax,
	NTV2_CC608_ChannelInvalid = NTV2_CC608_ChannelMax
} NTV2Line21Channel;


#define IsValidLine21Channel(_chan_)			((_chan_) >= NTV2_CC608_CC1 && (_chan_) < NTV2_CC608_ChannelMax)
#define	IsLine21CaptionChannel(_chan_)			((_chan_) >= NTV2_CC608_CC1 && (_chan_) <= NTV2_CC608_CC4)
#define	IsLine21TextChannel(_chan_)				((_chan_) >= NTV2_CC608_Text1 && (_chan_) <= NTV2_CC608_Text4)
#define	IsLine21XDSChannel(_chan_)				((_chan_) == NTV2_CC608_XDS)
#define	IsField1Line21CaptionChannel(_chan_)	((_chan_) == NTV2_CC608_CC1 || (_chan_) == NTV2_CC608_CC2 || (_chan_) == NTV2_CC608_Text1 || (_chan_) == NTV2_CC608_Text2)
#define	IsField2Line21CaptionChannel(_chan_)	((_chan_) == NTV2_CC608_CC3 || (_chan_) == NTV2_CC608_CC4 || (_chan_) == NTV2_CC608_Text3 || (_chan_) == NTV2_CC608_Text4)


/**
	@brief		Converts the given NTV2Line21Channel value into a human-readable string.
	@param[in]	inLine21Channel	Specifies the value to be converted.
	@param[in]	inCompact		Specify true for a compact string;  otherwise false for a more verbose string.
								Defaults to true (compact).
	@return		The human-readable string.
**/
AJAExport const std::string &	NTV2Line21ChannelToStr (const NTV2Line21Channel inLine21Channel, const bool inCompact = true);


/**
	@brief	The CEA-608 modes:  pop-on, roll-up (2, 3 and 4-line), and paint-on.
**/
typedef enum NTV2Line21Mode
{
	NTV2_CC608_CapModeMin,
	NTV2_CC608_CapModeUnknown = NTV2_CC608_CapModeMin,	///< @brief Unknown or invalid caption mode
	NTV2_CC608_CapModePopOn,	///< @brief	Pop-on caption mode
	NTV2_CC608_CapModeRollUp2,	///< @brief	2-row roll-up from bottom
	NTV2_CC608_CapModeRollUp3,	///< @brief	3-row roll-up from bottom
	NTV2_CC608_CapModeRollUp4,	///< @brief	4-row roll-up from bottom
	NTV2_CC608_CapModePaintOn,	///< @brief	Paint-on caption mode
	NTV2_CC608_CapModeMax
} NTV2Line21Mode;


#define IsValidLine21Mode(_mode_)		((_mode_) >= NTV2_CC608_CapModePopOn && (_mode_) < NTV2_CC608_CapModeMax)
#define IsLine21PopOnMode(_mode_)		((_mode_) == NTV2_CC608_CapModePopOn)
#define IsLine21PaintOnMode(_mode_)		((_mode_) == NTV2_CC608_CapModePaintOn)
#define IsLine21RollUpMode(_mode_)		((_mode_) >= NTV2_CC608_CapModeRollUp2 && (_mode_) <= NTV2_CC608_CapModeRollUp4)


/**
	@brief		Converts the given NTV2Line21Mode value into a human-readable string.
	@param[in]	inLine21Mode	Specifies the value to be converted.
	@return		The human-readable string.
**/
AJAExport const std::string &				NTV2Line21ModeToStr (const NTV2Line21Mode inLine21Mode);


/**
	@brief	The CEA-608 color values:  white, green, blue, cyan, red, yellow, magenta, and black.
**/
typedef enum NTV2Line21Color
{
	NTV2_CC608_White,
	NTV2_CC608_Green,
	NTV2_CC608_Blue,
	NTV2_CC608_Cyan,
	NTV2_CC608_Red,
	NTV2_CC608_Yellow,
	NTV2_CC608_Magenta,
	NTV2_CC608_Black,

	NTV2_CC608_NumColors,
	NTV2_608_INVALID_COLOR = NTV2_CC608_NumColors

} NTV2Line21Color;


#define IsValidLine21Color(_color_)		((_color_) >= NTV2_CC608_White && (_color_) < NTV2_CC608_NumColors)
#define IsLine21WhiteColor(_color_)		((_color_) == NTV2_CC608_White)
#define IsLine21BlackColor(_color_)		((_color_) == NTV2_CC608_Black)


/**
	@brief		Converts the given NTV2Line21Color value into a human-readable string.
	@param[in]	inLine21Color	Specifies the value to be converted.
	@param[in]	inCompact		Specify true for a compact string;  otherwise false for a more verbose string.
								Defaults to true (compact).
	@return		The human-readable string.
**/
AJAExport const std::string &				NTV2Line21ColorToStr (const NTV2Line21Color inLine21Color, const bool inCompact = true);


/**
	@brief	Converts a given CEA-608 color value into three 8-bit Y, Cb, Cr component values.
	@param[in]	inLine21Color	Specifies the NTV2Line21Color to be converted to 8-bit YUV.
	@param[out]	outY			Receives the 8-bit luminance value.
	@param[out]	outCb			Receives the 8-bit (blue) chrominance value.
	@param[out]	outCr			Receives the 8-bit (red) chrominance value.
	@return	True if successful;  otherwise false.
**/
AJAExport bool NTV2Line21ColorToYUV8 (const NTV2Line21Color inLine21Color, UByte & outY, UByte & outCb, UByte & outCr);


/**
	@brief	Converts a given CEA-608 color value into three 8-bit RGB component values.
	@param[in]	inLine21Color	Specifies the NTV2Line21Color to be converted to 8-bit RGB.
	@param[out]	outR			Receives the 8-bit red value.
	@param[out]	outG			Receives the 8-bit blue value.
	@param[out]	outB			Receives the 8-bit green value.
	@param[in]	inIsHD			Specify true to use Rec709 conversion; otherwise false for Rec601.
								Defaults to false (Rec601).
	@return	True if successful;  otherwise false.
	@todo	Currently uses SD color conversion. Needs to account for HD/SD.
**/
AJAExport bool NTV2Line21ColorToRGB8 (const NTV2Line21Color inLine21Color, UByte & outR, UByte & outG, UByte & outB, const bool inIsHD = false);


/**
	@brief	The CEA-608 character opacity values:  opaque, semi-transparent, and transparent.
**/
typedef enum NTV2Line21Opacity
{
	NTV2_CC608_Opaque,
	NTV2_CC608_SemiTransparent,
	NTV2_CC608_Transparent,

	NTV2_CC608_NumOpacities

} NTV2Line21Opacity;


#define IsValidLine21Opacity(_opacity_)			((_opacity_) >= NTV2_CC608_Opaque && (_opacity_) < NTV2_CC608_NumOpacities)
#define IsLine21Transparent(_opacity_)			((_opacity_) == NTV2_CC608_Transparent)
#define IsLine21SemiTransparent(_opacity_)		((_opacity_) == NTV2_CC608_SemiTransparent)
#define IsLine21Opaque(_opacity_)				((_opacity_) == NTV2_CC608_Opaque)


/**
	@brief		Converts the given NTV2Line21Opacity value into a human-readable string.
	@param[in]	inLine21Opacity	Specifies the value to be converted.
	@param[in]	inCompact		Specify true for a compact string;  otherwise false for a more verbose string.
								Defaults to true (compact).
	@return		The human-readable string.
**/
AJAExport const std::string &				NTV2Line21OpacityToStr (const NTV2Line21Opacity inLine21Opacity, const bool inCompact = true);


/**
	@brief	The available CEA-608 character sets.
**/
typedef enum NTV2Line21CharacterSet
{
	NTV2_CC608_DefaultCharacterSet,			//	Plain ol' normal U.S./Latin characters
	NTV2_CC608_DoubleSizeCharacterSet,
	NTV2_CC608_PrivateCharacterSet1,
	NTV2_CC608_PrivateCharacterSet2,
	NTV2_CC608_PRChinaCharacterSet,
	NTV2_CC608_KoreanCharacterSet,
	NTV2_CC608_RegisteredCharacterSet1,

	NTV2_CC608_NumCharacterSets

} NTV2Line21CharacterSet, NTV2Line21CharSet;


#define IsValidLine21CharacterSet(_charset_)	((_charset_) >= NTV2_CC608_DefaultCharacterSet && (_charset_) < NTV2_CC608_NumCharacterSets)


/**
	@brief		Converts the given NTV2Line21CharacterSet value into a human-readable string.
	@param[in]	inLine21CharSet		Specifies the value to be converted.
	@return		The human-readable string.
**/
AJAExport const std::string &				NTV2Line21CharacterSetToStr (const NTV2Line21CharacterSet inLine21CharSet);


/**
	@brief	Describes a unique CEA-608 caption character code point in 32 bits:  0xSS00XXYY,
			where...
				SS	==	NTV2Line21CharacterSet
				XX	==	CEA608 byte 1	(parity stripped)
				YY	==	CEA608 byte 2	(parity stripped)
**/
typedef ULWord	NTV2_CC608_CodePoint;


/**
	@brief	A set of unique CEA-608 caption character code points.
**/
typedef std::set <NTV2_CC608_CodePoint>		NTV2CodePointSet;
typedef NTV2CodePointSet::const_iterator	NTV2CodePointSetConstIter;

AJAExport std::string NTV2CodePointSetToString (const NTV2CodePointSet & inSet);	//	conflicts with classes lib ---	std::ostream & operator << (std::ostream & inOutStream, const NTV2CodePointSet & inSet);


/**
	@brief		Constructs a unique CEA-608 caption character code point from its three components.
	@param[in]	in608Byte1	The first CEA-608 byte (with no parity).
	@param[in]	in608Byte2	The second byte (with no parity).
	@param[in]	inCharSet	Specifies the NTV2Line21CharacterSet. Defaults to NTV2_CC608_DefaultCharacterSet (North American).
	@return		The constructed NTV2_CC608_CodePoint.
**/
inline NTV2_CC608_CodePoint		Make608CodePoint (const UByte in608Byte1, const UByte in608Byte2, const NTV2Line21CharacterSet inCharSet = NTV2_CC608_DefaultCharacterSet)
{
	return NTV2_CC608_CodePoint (ULWord (inCharSet << 24) | ULWord (in608Byte1 << 8) | ULWord (in608Byte2));
}

/**
	@brief		Returns a string containing the UTF-8 character sequence that best represents the given CEA-608
				code point.
	@param[in]	in608CodePoint	Specifies the CEA-608 code point of interest.
	@return		A string that contains the UTF8 character sequence that best represents the given CEA-608 character.
				The returned string will be empty upon failure, or if there is no best representation for the character.
**/
AJAExport std::string		NTV2CC608CodePointToUtf8String (const NTV2_CC608_CodePoint in608CodePoint);

/**
	@brief		Returns the UTF-16 character that best represents the given CEA-608 code point.
	@param[in]	in608CodePoint	Specifies the CEA-608 code point of interest.
	@return		The UTF-16 character that best represents the given CEA-608 character (or zero upon failure).
**/
AJAExport UWord				NTV2CC608CodePointToUtf16Char (const NTV2_CC608_CodePoint in608CodePoint);


/**
	@brief		Extracts the NTV2Line21CharacterSet from the given NTV2_CC608_CodePoint.
	@param[in]	inCodePoint		Specifies the CEA-608 code point.
	@return		The code point's NTV2Line21CharacterSet.
**/
inline NTV2Line21CharacterSet	GetLine21CharacterSet (const NTV2_CC608_CodePoint inCodePoint)
{
	return NTV2Line21CharacterSet ((inCodePoint & 0xFF000000) >> 24);
}


/**
	@brief		Extracts the first CEA-608 byte from the given NTV2_CC608_CodePoint.
	@param[in]	inCodePoint		Specifies the CEA-608 code point.
	@return		The first CEA-608 byte from the given code point.
**/
inline UByte	Get608Byte1 (const NTV2_CC608_CodePoint inCodePoint)
{
	return UByte ((inCodePoint & 0x00FF00) >> 8);
}


/**
	@brief		Extracts the second CEA-608 byte from the given NTV2_CC608_CodePoint.
	@param[in]	inCodePoint		Specifies the CEA-608 code point.
	@return		The second CEA-608 byte from the given code point.
**/
inline UByte	Get608Byte2 (const NTV2_CC608_CodePoint inCodePoint)
{
	return UByte (inCodePoint & 0x000000FF);
}


/**
	@brief	CEA-608 Character Attributes
	@note	The non-bool elements in the struct must be "unsigned", otherwise they'll be
			considered signed. For example, NTV2_CC608_Black (= 7) will be read as "-1" in
			the following case:  int x = attr.bgColor. If it's "unsigned", the value will
			correctly read out as "7".
**/
typedef struct AJAExport NTV2Line21Attributes
{
	public:
		/**
			@brief	Constructs a default NTV2Line21Attributes instance, i.e., no flashing, no italics, no underline,
					white on black with an opaque background.
		**/
		explicit						NTV2Line21Attributes ();

		/**
			@brief	Constructs a NTV2Line21Attributes instance with the given settings.
			@param[in]	inFGColor	Specifies the foreground color (required).
			@param[in]	inBGColor	Optionally specifies the background color. Defaults to black.
			@param[in]	inOpacity	Optionally specifies the background opacity. Defaults to opaque.
			@param[in]	inItalics	Optionally specifies the italics setting. Defaults to normal (no italics).
			@param[in]	inUnderline	Optionally specifies the underline setting. Defaults to normal (no underline).
			@param[in]	inFlash		Optionally specifies the flash setting. Defaults to normal (no flashing).
		**/
		explicit						NTV2Line21Attributes (	const NTV2Line21Color	inFGColor,
																const NTV2Line21Color	inBGColor	= NTV2_CC608_Black,
																const NTV2Line21Opacity	inOpacity	= NTV2_CC608_Opaque,
																const bool				inItalics	= false,
																const bool				inUnderline	= false,
																const bool				inFlash		= false);
		inline							NTV2Line21Attributes (	const NTV2Line21Attributes & inRHS)	{	bFlash		= inRHS.bFlash;
																										bItalic		= inRHS.bItalic;
																										bUnderline	= inRHS.bUnderline;
																										fgColor		= inRHS.fgColor;
																										bgColor		= inRHS.bgColor;
																										bgOpacity	= inRHS.bgOpacity;	}
		inline	NTV2Line21Attributes &	operator = (const NTV2Line21Attributes & inRHS)				{	bFlash		= inRHS.bFlash;
																										bItalic		= inRHS.bItalic;
																										bUnderline	= inRHS.bUnderline;
																										fgColor		= inRHS.fgColor;
																										bgColor		= inRHS.bgColor;
																										bgOpacity	= inRHS.bgOpacity;	return *this;}

		/**
			@brief	Returns true if I'm any different from the default, i.e., if IsFlashing(), IsItalicized(), IsUnderlined()
					return true, or if my foreground color is anything but white, or if my background color is anything but
					black, or if my opacity is anything but opaque.
		**/
		inline bool						IsSet (void) const								{ return bFlash || bItalic || bUnderline
																								|| fgColor != NTV2_CC608_White
																								|| bgColor != NTV2_CC608_Black
																								|| bgOpacity != NTV2_CC608_Opaque; }

		/**
			@brief	Returns my foreground color.
		**/
		inline NTV2Line21Color			GetColor (void) const							{ return NTV2Line21Color(fgColor); }

		/**
			@brief	Returns my background color.
		**/
		inline NTV2Line21Color			GetBGColor (void) const							{ return NTV2Line21Color(bgColor); }

		/**
			@brief	Returns my background opacity.
		**/
		inline NTV2Line21Opacity		GetOpacity (void) const							{ return NTV2Line21Opacity(bgOpacity); }

		/**
			@brief	Returns true if I'm italicized;  otherwise returns false.
		**/
		inline bool						IsItalicized (void) const						{ return bItalic; }

		/**
			@brief	Returns true if I'm underlined;  otherwise returns false.
		**/
		inline bool						IsUnderlined (void) const						{ return bUnderline; }

		/**
			@brief	Returns true if I'm flashing;  otherwise returns false.
		**/
		inline bool						IsFlashing (void) const							{ return bFlash; }

		/**
			@brief	Enables italics.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	AddItalics (void)								{ bItalic = true;			return *this; }

		/**
			@brief	Removes italics.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	RemoveItalics (void)							{ bItalic = false;			return *this; }

		/**
			@brief	Sets my italics setting.
			@param[in]	inItalics	Specifies my new italics setting.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	SetItalics (const bool inItalics)				{ bItalic = inItalics;		return *this; }


		/**
			@brief	Enables my underline attribute setting.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	AddUnderline (void)								{ bUnderline = true;		return *this; }

		/**
			@brief	Disables my underline attribute setting.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	RemoveUnderline (void)							{ bUnderline = false;		return *this; }

		/**
			@brief	Sets my underline attribute setting.
			@param[in]	inUnderline		Specifies my new underline setting.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	SetUnderline (const bool inUnderline)			{ bUnderline = inUnderline;	return *this; }


		/**
			@brief	Enables my flashing attribute setting.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	AddFlash (void)									{ bFlash = true;			return *this; }

		/**
			@brief	Disables my flashing attribute setting.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	RemoveFlash (void)								{ bFlash = false;			return *this; }

		/**
			@brief	Sets my flashing attribute setting.
			@param[in]	inFlash		Specifies my new flashing attribute setting.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	SetFlash (const bool inFlash)					{ bFlash = inFlash;			return *this; }


		/**
			@brief	Sets my foreground color attribute.
			@param[in]	inFGColor	Specifies my new foreground color attribute.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	SetColor (const NTV2Line21Color	inFGColor)		{ fgColor = inFGColor;		return *this; }

		/**
			@brief	Sets my background color attribute.
			@param[in]	inBGColor	Specifies my new background color attribute.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	SetBGColor (const NTV2Line21Color inBGColor)	{ bgColor = inBGColor;		return *this; }

		/**
			@brief	Sets my background opacity attribute.
			@param[in]	inOpacity	Specifies my new background opacity attribute.
			@return	A non-constant reference to me.
		**/
		inline NTV2Line21Attributes &	SetOpacity (const NTV2Line21Opacity inOpacity)	{ bgOpacity = inOpacity;	return *this; }


		/**
			@brief	Clears all of my attributes, returning me to a default state, i.e., no flashing, no italics, no underline,
					white on black with an opaque background.
		**/
		inline void						Clear (void)									{ bFlash = bItalic = bUnderline = false;
																							fgColor = NTV2_CC608_White;
																							bgColor = NTV2_CC608_Black;
																							bgOpacity = NTV2_CC608_Opaque; }

		/**
			@brief	Compares my attributes with those of another, and returns true if my magnitude is less than the right-hand operand's.
			@param[in]	inRHS	Specifies the other NTV2Line21Attributes instance to compare.
			@return	True if my magnitude is less than those of the right-hand operand;  otherwise false.
		**/
		inline bool						operator < (const NTV2Line21Attributes & inRHS) const						{ return GetHashKey () < inRHS.GetHashKey (); }

		/**
			@brief	Compares my attributes with those of another, and returns true if they're identical.
			@param[in]	inRHS	Specifies the other NTV2Line21Attributes instance to compare.
			@return	True if all attributes match.
		**/
		inline bool						operator == (const NTV2Line21Attributes & inRHS) const						{ return GetHashKey () == inRHS.GetHashKey (); }

		/**
			@brief	Compares my attributes with those of another, and returns false if any of them mismatch.
			@param[in]	inRHS	Specifies the other NTV2Line21Attributes instance to compare.
			@return	True if any attribute doesn't match.
		**/
		inline bool						operator != (const NTV2Line21Attributes & inRHS) const						{ return !(*this == inRHS);}

		/**
			@brief	For dumping the attributes backbuffer.
		**/
		std::string						GetHexString (void) const;

		/**
			@brief	Returns my magnitude used to implement operator < for sorting or using a Line21 attribute as an index key.
		**/
		uint16_t						GetHashKey (void) const;


	//	Instance Data
	private:
		bool	 bFlash		:1;		///<	@brief	True for "Flash" mode (blink); otherwise normal
		bool	 bItalic	:1;		///<	@brief	True for Italics; otherwise normal
		bool	 bUnderline	:1;		///<	@brief	True for Underline; otherwise normal
		unsigned fgColor	:3;		///<	@brief	Foreground (character) color (NTV2Line21Color)
		unsigned bgColor	:3;		///<	@brief	Background color (NTV2Line21Color)
		unsigned bgOpacity	:2;		///<	@brief	Background opacity (NTV2Line21Opacity)

} NTV2Line21Attributes, NTV2Line21Attrs;

typedef NTV2Line21Attrs *	NTV2Line21AttrsPtr;
typedef NTV2Line21AttrsPtr	NTV2Line21AttributesPtr;


/**
	@brief	Writes a human-readable rendition of the given NTV2Line21Attributes into the given output stream.
	@param		inOutStream		Specifies the output stream to be written.
	@param[in]	inData			Specifies the NTV2Line21Attributes to be rendered into the output stream.
	@return		A non-constant reference to the specified output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const NTV2Line21Attrs & inData);



//	CONVERSION TO/FROM STRINGS


/**
	@brief		Converts the given NTV2Line21Attributes value into a human-readable string.
	@param[in]	inLine21Attributes	Specifies the value to be converted.
	@return		The human-readable string.
**/
AJAExport std::string					NTV2Line21AttributesToStr (const NTV2Line21Attributes inLine21Attributes);


/**
	@brief		Converts the given string into the equivalent NTV2Line21Field value.
	@param[in]	inStr	Specifies the string to be converted.
	@return		The equivalent NTV2Line21Field value.
**/
AJAExport NTV2Line21Field				StrToNTV2Line21Field (const std::string & inStr);


/**
	@brief		Converts the given string into the equivalent NTV2Line21Channel value.
	@param[in]	inStr	Specifies the string to be converted.
	@return		The equivalent NTV2Line21Channel value.
**/
AJAExport NTV2Line21Channel				StrToNTV2Line21Channel (const std::string & inStr);


/**
	@brief		Converts the given string into the equivalent NTV2Line21Mode value.
	@param[in]	inStr	Specifies the string to be converted.
	@return		The equivalent NTV2Line21Mode value.
**/
AJAExport NTV2Line21Mode				StrToNTV2Line21Mode (const std::string & inStr);


/**
	@brief		Converts the given string into the equivalent NTV2Line21Color value.
	@param[in]	inStr	Specifies the string to be converted.
	@return		The equivalent NTV2Line21Color value.
**/
AJAExport NTV2Line21Color				StrToNTV2Line21Color (const std::string & inStr);


/**
	@brief		Converts the given string into the equivalent NTV2Line21Opacity value.
	@param[in]	inStr	Specifies the string to be converted.
	@return		The equivalent NTV2Line21Opacity value.
**/
AJAExport NTV2Line21Opacity				StrToNTV2Line21Opacity (const std::string & inStr);


/**
	@brief		Converts the given string into the equivalent NTV2Line21CharacterSet value.
	@param[in]	inStr	Specifies the string to be converted.
	@return		The equivalent NTV2Line21CharacterSet value.
**/
AJAExport NTV2Line21CharacterSet		StrToNTV2Line21CharacterSet (const std::string & inStr);


/**
	@brief		Converts the given string into the equivalent NTV2Line21Attributes value.
	@param[in]	inStr	Specifies the string to be converted.
	@return		The equivalent NTV2Line21Attributes value.
**/
AJAExport NTV2Line21Attributes			StrToNTV2Line21Attributes (const std::string & inStr);



/**
	@brief	This structure encapsulates all possible CEA-608 caption data bytes that may be
			associated with a given frame or field.
	@note	The "field 3" data bytes is used when translating 30fps video to 24fps and removing 3:2 pulldown.
**/
typedef struct AJAExport CaptionData
{
	bool	bGotField1Data;	///< @brief	True if Field 1 bytes have been set;  otherwise false.
	UByte	f1_char1;		///< @brief	Caption Byte 1 of Field 1
	UByte	f1_char2;		///< @brief	Caption Byte 2 of Field 1
	
	bool	bGotField2Data;	///< @brief	True if Field 2 bytes have been set;  otherwise false.
	UByte	f2_char1;		///< @brief	Caption Byte 1 of Field 2
	UByte	f2_char2;		///< @brief	Caption Byte 2 of Field 2
	
	bool	bGotField3Data;	///< @brief	True if Field 3 bytes have been set. This is used only when translating 30fps video to 24fps and removing 3:2 pulldown.
	UByte	f3_char1;		///< @brief	Caption Byte 1 of Field 3
	UByte	f3_char2;		///< @brief	Caption Byte 2 of Field 3

	public:
		/**
			@brief	Default constructor. Sets my caption bytes to 0xFF, and "gotFieldData" values to false.
		**/
		explicit inline		CaptionData ()	{ bGotField1Data = bGotField2Data = bGotField3Data = false;  f1_char1 = f1_char2 = f2_char1 = f2_char2 = f3_char1 = f3_char2 = 0xFF; }

		/**
			@brief	Constructs me from a pair of caption bytes (Field 1).
		**/
		explicit 			CaptionData (const UByte inF1Char1, const UByte inF1Char2);

		/**
			@brief	Constructs me from two pairs of caption bytes (Field 1 and Field 2).
		**/
		explicit 			CaptionData (const UByte inF1Char1, const UByte inF1Char2, const UByte inF2Char1, const UByte inF2Char2);

		/**
			@return		True if Field 1 caption bytes were set and their byte values aren't 0x80.
		**/
		inline bool			HasF1Data (void) const	{return (bGotField1Data && (f1_char1 != 0x80 || f1_char2 != 0x80));}

		/**
			@return		True if Field 2 caption bytes were set and their byte values aren't 0x80.
		**/
		inline bool			HasF2Data (void) const	{return (bGotField2Data && (f2_char1 != 0x80 || f2_char2 != 0x80));}

		/**
			@return		True if either Field 1 or 2 caption bytes were set and their byte values aren't 0x80.
		**/
		inline bool			HasData (void) const				{return HasF1Data() || HasF2Data();}

		/**
			@return		True if Field 1's caption byte values are both 0xFF.
		**/
		inline bool			IsF1Invalid (void) const			{return f1_char1 == 0xFF && f1_char2 == 0xFF;}

		/**
			@return		True if Field 2's caption byte values are both 0xFF.
		**/
		inline bool			IsF2Invalid (void) const			{return f2_char1 == 0xFF && f2_char2 == 0xFF;}

		/**
			@return		True if my Field 1 and 2 caption bytes are all 0xFF.
		**/
		inline bool			IsError (void) const				{return IsF1Invalid() && IsF2Invalid();}

		/**
			@brief		Copies my F1 data bytes from the given CaptionData instance.
			@param[in]	inRHS	The CaptionData instance supplying the F1 data byte values.
			@return		A non-const reference to me.
		**/
		inline CaptionData &	SetF1Data (const CaptionData & inRHS)	{f1_char1 = inRHS.f1_char1; f1_char2 = inRHS.f1_char2; bGotField1Data = inRHS.bGotField1Data; return *this;}

		/**
			@brief		Sets my F1 data bytes.
			@param[in]	inF1Char1	Specifies the first F1 byte value.
			@param[in]	inF1Char2	Specifies the second F1 byte value.
			@return		A non-const reference to me.
		**/
		inline CaptionData &	SetF1Data (const UByte inF1Char1, const UByte inF1Char2)	{f1_char1 = inF1Char1; f1_char2 = inF1Char2; bGotField1Data = inF1Char1 != 0xFF && inF1Char2 != 0xFF; return *this;}

		/**
			@brief		Copies my F2 data bytes from the given CaptionData instance.
			@param[in]	inRHS	The CaptionData instance supplying the F2 data byte values.
			@return		A non-const reference to me.
		**/
		inline CaptionData &	SetF2Data (const CaptionData & inRHS)	{f2_char1 = inRHS.f2_char1; f2_char2 = inRHS.f2_char2; bGotField2Data = inRHS.bGotField2Data; return *this;}

		/**
			@brief		Sets my F2 data bytes.
			@param[in]	inF2Char1	Specifies the first F2 byte value.
			@param[in]	inF2Char2	Specifies the second F2 byte value.
			@return		A non-const reference to me.
		**/
		inline CaptionData &	SetF2Data (const UByte inF2Char1, const UByte inF2Char2)	{f2_char1 = inF2Char1; f2_char2 = inF2Char2; bGotField2Data = inF2Char1 != 0xFF && inF2Char2 != 0xFF; return *this;}

		/**
			@brief		Sets all of my "got data" fields to "false" and all my character values to 0x80.
		**/
		inline void			Clear (void)	{bGotField1Data = bGotField2Data = bGotField3Data = false;  f1_char1 = f1_char2 = f2_char1 = f2_char2 = f3_char1 = f3_char2 = 0x80;}

		/**
			@brief		Sets all of my "got data" fields to "false" and all my character values to zero.
		**/
		inline void			Zero (void)		{bGotField1Data = bGotField2Data = bGotField3Data = false;  f1_char1 = f1_char2 = f2_char1 = f2_char2 = f3_char1 = f3_char2 = 0x00;}

		/**
			@return		True if I'm equal to the right-hand-side CaptionData.
			@param[in]	inRHS	Specifies the right-hand-side CaptionData that will be compared to me.
			@note		To synthesize the other comparison operators (!=, <=, >, >=), add "#include <utility>",
						and "using namespace std::rel_ops;" to the code module.
		**/
		bool				operator == (const CaptionData & inRHS) const;
		bool				operator < (const CaptionData & inRHS) const;
		inline bool			operator != (const CaptionData & inRHS) const	{return !(*this == inRHS);}
} CaptionData;


/**
	@brief		Streams a human-readable representation of the given CaptionData into the given output stream.
	@param[in]	inOutStream		The output stream to receive the human-readable representation.
	@param[in]	inData			The CaptionData to be streamed.
	@return		A reference to the given output stream.
**/
AJAExport std::ostream & operator << (std::ostream & inOutStream, const CaptionData & inData);


/**
	@brief	This class is used to respond to dynamic events that occur during CEA-608 caption decoding.
**/
class AJAExport NTV2Caption608ChangeInfo
{
	public:
		/**
			@brief	Used to determine what changed. Also can be used to choose which changes to pay attention to.
		**/
		typedef enum NTV2Caption608Change
		{
			NTV2DecoderChange_None				= 0,		///< @brief	Invalid.
			NTV2DecoderChange_CurrentChannel	= 1,		///< @brief	The current caption channel of interest changed (e.g., CC1 to CC3).
			NTV2DecoderChange_CurrentRow		= 2,		///< @brief	The current caption row changed.
			NTV2DecoderChange_CurrentColumn		= 4,		///< @brief	The current caption column changed.
			NTV2DecoderChange_CurrentScreen		= 8,		///< @brief	The current caption screen changed.
			NTV2DecoderChange_CaptionMode		= 16,		///< @brief	The current caption mode changed (e.g., pop-on to roll-up).
			NTV2DecoderChange_ScreenCharacter	= 32,		///< @brief	The character displayed at a specific screen and position changed.
			NTV2DecoderChange_ScreenAttribute	= 64,		///< @brief	The display attributes at a specific screen and position changed.
			NTV2DecoderChange_DrawScreen		= 128,		///< @brief	If drawing entire caption screen, do it now.
			NTV2DecoderChange_All				= 0xFFFF	///< @brief	All possible changes.
		} NTV2Caption608Change;

	public:
		explicit	NTV2Caption608ChangeInfo (const NTV2Line21Channel inChannel);
		explicit	NTV2Caption608ChangeInfo (const NTV2Line21Channel inOldChannel, const NTV2Line21Channel inNewChannel);
		explicit	NTV2Caption608ChangeInfo (const NTV2Line21Channel inChannel, const UWord inWhatChanged, const UWord inOldValue, const UWord inNewValue);
		explicit	NTV2Caption608ChangeInfo (const NTV2Line21Channel inChannel, const UWord inScreen, const UWord inRow, const UWord inColumn, const NTV2_CC608_CodePoint inOldValue, const NTV2_CC608_CodePoint inNewValue);
		explicit	NTV2Caption608ChangeInfo (const NTV2Line21Channel inChannel, const UWord inScreen, const UWord inRow, const UWord inColumn, const NTV2Line21Attributes & inOldValue, const NTV2Line21Attributes & inNewValue);
		std::ostream &	Print (std::ostream & inOutStrm) const;

	public:
		union _u
		{
			struct _currentChannel
			{
				UWord					mOld;		///< @brief	Caption channel change. This is the old NTV2Line21Channel value. (The new, current value is in my mChannel member.)
			}	currentChannel;
			struct _currentRow
			{
				UWord					mOld;		///< @brief	Current row change. This is the old row number.
				UWord					mNew;		///< @brief	Current row change. This is the new row number.
			}	currentRow;
			struct _currentColumn
			{
				UWord					mOld;		///< @brief	Current column change. This is the old column number.
				UWord					mNew;		///< @brief	Current column change. This is the new column number.
			}	currentColumn;
			struct _currentScreen
			{
				UWord					mOld;		///< @brief	Current screen change. This is the old screen number.
				UWord					mNew;		///< @brief	Current screen change. This is the new screen number.
			}	currentScreen;
			struct _captionMode
			{
				UWord					mOld;		///< @brief	Current NTV2Line21Mode change. This is the old NTV2Line21Mode value.
				UWord					mNew;		///< @brief	Current NTV2Line21Mode change. This is the new NTV2Line21Mode value.
			}	captionMode;
			struct _screenChar
			{
				UWord					mScreenNum;	///< @brief	Display character change. This is the number of the screen backbuffer that's changing.
				UWord					mRow;		///< @brief	Display character change. The row number that contains the character that's being changed.
				UWord					mColumn;	///< @brief	Display character change. The column number that contains the character that's being changed.
				NTV2_CC608_CodePoint	mOld;		///< @brief	Display character change. The old character NTV2_CC608_CodePoint value.
				NTV2_CC608_CodePoint	mNew;		///< @brief	Display character change. The new character NTV2_CC608_CodePoint value.
			}	screenChar;
			struct _screenAttr
			{
				UWord					mScreenNum;	///< @brief	The number of the screen backbuffer that's changing.
				UWord					mRow;		///< @brief	The row number that contains the character whose attributes are changing.
				UWord					mColumn;	///< @brief	The column number that contains the character whose attributes are changing.
				uint16_t				mOld;		///< @brief	The old character NTV2Line21Attributes.
				uint16_t				mNew;		///< @brief	The new character NTV2Line21Attributes.
			}	screenAttr;
		}	u;

		UWord					mWhatChanged;		///< @brief	Bit mask that indicates what changed
		NTV2Line21Channel		mChannel;			///< @brief	Caption channel being changed

};	//	NTV2Caption608ChangeInfo


/**
	@brief		Streams a human-readable representation of the given NTV2Caption608ChangeInfo into the given output stream.
	@param		oss		The output stream to receive the human-readable representation.
	@param[in]	inInfo	The NTV2Caption608ChangeInfo to be streamed.
	@return		A reference to the given output stream.
**/
inline std::ostream & operator << (std::ostream & oss, const NTV2Caption608ChangeInfo & inInfo)	{return inInfo.Print(oss);}


/**
	@brief	This callback is used to respond to dynamic events that occur during CEA-608 caption decoding.
	@param[in]	pInstance		An instance pointer.
	@param[in]	inChangeInfo	The details about the change that transpired.
**/
typedef void (NTV2Caption608Changed) (void * pInstance, const NTV2Caption608ChangeInfo & inChangeInfo);


/**
	@brief	An ordered set of all possible NTV2Line21Attributes permutations (excluding the "flashing" attribute,
			which is not required for glyph rendering).
			Call size() to discover how many there are.
			Call GetPermutation with an index >= 0 and < size() to get a specific attribute permutation.
			Call GetIndexFromAttribute to find out which permutation index corresponds to the given attribute.
**/
class AJAExport NTV2Line21AttributePermutations
{
	public:
		/**
			@brief	My constructor.
		**/
		NTV2Line21AttributePermutations ();

		/**
			@brief	Returns a const reference to the attribute that corresponds to the given permutation index.
			@param[in]	inIndex		Specifies the permutation index. Must be less than my size().
			@return		The attribute that corresponds to the given permutation index.
		**/
		const NTV2Line21Attrs &		GetPermutation (const size_t inIndex) const;

		/**
			@brief	Returns my size (the total number of attribute permutations that I have).
			@return	The total number of attribute permutations that I have.
		**/
		inline ULWord				size (void) const							{return 3 * 2 * 8 * 2 * 8;}

		/**
			@brief	Returns the permutation index that corresponds to the given attribute.
			@param[in]	inAttribute		The display attribute of interest.
			@return	The permutation index that corresponds to the given attribute.
		**/
		size_t						GetIndexFromAttribute (const NTV2Line21Attributes & inAttribute) const;

		/**
			@brief	Returns a const reference to the attribute that corresponds to the given permutation index.
			@param[in]	inIndex		Specifies the permutation index. Must be less than my size().
			@return		The attribute that corresponds to the given permutation index.
		**/
		inline const NTV2Line21Attrs &	operator [] (const size_t inIndex) const	{return GetPermutation(inIndex);}

	private:
		NTV2Line21Attrs	mAttribs [3 * 2 * 8 * 2 * 8];	//	3 opacities  x  w/wo underline  x  8 BG colors  x  w/wo italics  x  8 FG colors

};	//	NTV2Line21AttributePermutations

#endif	// __NTV2_CAPTION608TYPES_
