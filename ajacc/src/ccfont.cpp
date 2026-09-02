/**
	@file		ccfont.cpp
	@brief		Implementation of the NTV2CCFont.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include "ccfont.h"
#include "ntv2captiondecodechannel608.h"	//	Just for GetUtf8
#include "ntv2enums.h"						//	For NTV2FrameBufferFormat
#include "ntv2publicinterface.h"			//	For xHEX0N macro
#include "ntv2utils.h"						//	For CalcRowBytesForFormat
#include <map>
#include <iomanip>
#include <sstream>

using namespace std;


const int NTV2_CCFont_BitmapCharHeight		= 18;		// bitmap "dots" per character height (NOT including inter-row spacing)
const int NTV2_CCFont_CharDotTopSpace		=  2;		// inter-row spacing (in dots) above each character
const int NTV2_CCFont_CharDotBottomSpace	=  6;		// inter-row spacing (in dots) below each character
const int NTV2_CCFont_CharDotHeight			= (NTV2_CCFont_BitmapCharHeight + NTV2_CCFont_CharDotTopSpace + NTV2_CCFont_CharDotBottomSpace);

const int NTV2_CCFont_BitmapCharWidth		= 16;		// bitmap "dots" per character width (NOT including inter-character spacing)
const int NTV2_CCFont_CharDotLeftSpace		=  1;		// inter-character spacing (in dots) to the left of each character
const int NTV2_CCFont_CharDotRightSpace		=  1;		// inter-character spacing (in dots) to the right of each character
const int NTV2_CCFont_CharDotWidth			= (NTV2_CCFont_BitmapCharWidth + NTV2_CCFont_CharDotLeftSpace + NTV2_CCFont_CharDotRightSpace);

/**
	@brief	The caption font table that contains glyph data for the 114 characters
			that comprise the following EIA-608 character sets:

					Description								Offset		# chars
					--------------------------------------	---------	-------
					Basic North American Character Set		0x00-0x5F	 96
					Special North American Character Set	0x60-0x6F	 16
					Extended NA & Western European Chars	0x70-0xAF	 63
					Placeholders for underlined chars		0xAF-0xB0	  2
					--------------------------------------	---------	-------
															0x00-0xB0	177 total

			The actual rendered characters are 26 dots high X 18 dots wide.
			(The term "dot" is used to represent these glyph pixels, to better
			distinguish them from what will ultimately be rendered in a frame buffer,
			as these dots can be scaled up by independent horizontal and vertical
			scale factors.)

			The left and right border of this 26x18 matrix is 1-dot thick, and always
			black (zero). The top border is 2 dots of black (zero), and the bottom
			border is six dots of black (zero). Thus, this dot glyph table only stores
			the interior 18x16 bitmap.

			Each of the two-byte bitmap entries represents one "row" of a glyph (16 dots)
			starting at the top row.

			For example, entries 1 (exclamation point) and 15 (forward slash):
			+------------------+					+------------------+
			|        ...       |	0x0380			|                  |	0x0000
			|        ...       |	0x0380			|                  |	0x0000
			|        ...       |	0x0380			|              ..  |	0x000C
			|        ...       |	0x0380			|             ...  |	0x001C
			|        ...       |	0x0380			|            ....  |	0x003C
			|        ...       |	0x0380			|           ....   |	0x0078
			|        ...       |	0x0380			|          ....    |	0x00F0
			|        ...       |	0x0380			|         ....     |	0x01E0
			|        ...       |	0x0380			|        ....      |	0x03C0
			|        ...       |	0x0380			|       ....       |	0x0780
			|        ...       |	0x0380			|      ....        |	0x0F00
			|        ...       |	0x0380			|     ....         |	0x1E00
			|        ...       |	0x0380			|    ....          |	0x3C00
			|        ...       |	0x0380			|   ....           |	0x3800
			|                  |	0x0000			|   ...            |	0x3000
			|                  |	0x0000			|                  |	0x0000
			|        ...       |	0x0380			|                  |	0x0000
			|        ...       |	0x0380			|                  |	0x0000
			+------------------+					+------------------+

	@note	Given a CEA-608 ("almost ASCII") character code value, clients should
			subtract 32 to obtain the correct index into me.
**/
static UWord gCCFontDotMap [] [NTV2_CCFont_BitmapCharHeight] =						//	Code	Glyph	Description
{																					//	----	-----	---------------------------------------
	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		//	0x20	0x00	(SP)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380,		//	0x21	0x01	'!' (exclamation point)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0000, 0x0000, 0x0380, 0x0380 },

	{ 0x0E70, 0x0E70, 0x0E70, 0x0E70, 0x0E70, 0x0000, 0x0000, 0x0000, 0x0000,		//	0x22	0x02	'"'
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x0000, 0x0E70, 0x0E70, 0x3FFC, 0x3FFC, 0x0E70, 0x0E70,		//	0x23	0x03	'#'
	  0x0E70, 0x0E70, 0x3FFC, 0x3FFC, 0x0E70, 0x0E70, 0x0000, 0x0000, 0x0000 },

	{ 0x0380, 0x0380, 0x1FF0, 0x3FF8, 0x7BBC, 0x739C, 0x7380, 0x7B80, 0x3FF0,		//	0x24	0x04	'$'
	  0x1FF8, 0x03BC, 0x039C, 0x739C, 0x7BBC, 0x3FF8, 0x1FF0, 0x0380, 0x0380 },

	{ 0x780E, 0x780E, 0x780E, 0x781E, 0x003C, 0x0078, 0x00F0, 0x01E0, 0x03C0,		//	0x25	0x05	'%'
	  0x0780, 0x0F00, 0x1E00, 0x3C00, 0x7800, 0x701E, 0x701E, 0x701E, 0x701E },

	{ 0x0FC0, 0x1FE0, 0x3CF0, 0x7870, 0x7070, 0x70F0, 0x79E0, 0x3FC0, 0x1F80, 		//	0x26	0x06	'&'
	  0x1F00, 0x3F80, 0x73C0, 0x71EE, 0x70FC, 0x7878, 0x3FFC, 0x1FFE, 0x0FDE },

	{ 0x0780, 0x0780, 0x0780, 0x0F80, 0x1F00, 0x0E00, 0x0400, 0x0000, 0x0000,		//	0x27	0x07	'''
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x01E0, 0x03C0, 0x0780, 0x0F00, 0x1E00, 0x1C00, 0x1C00, 0x1C00, 0x1C00,		//	0x28	0x08	'('
	  0x1C00, 0x1C00, 0x1C00, 0x1C00, 0x1E00, 0x0F00, 0x0780, 0x03C0, 0x01E0 },

	{ 0x0780, 0x03C0, 0x01E0, 0x00F0, 0x0078, 0x0038, 0x0038, 0x0038, 0x0038,		//	0x29	0x09	')'
	  0x0038, 0x0038, 0x0038, 0x0038, 0x0078, 0x00F0, 0x01E0, 0x03C0, 0x0780 },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x0000, 0x0000, 0x1FF8, 0x1FFC, 0x001E,	 	//	0x2A	0x0A	'a'' (lower-case a, acute accent)
	  0x000E, 0x1FFE, 0x3FFE, 0x781E, 0x700E, 0x700E, 0x781E, 0x3FFE, 0x1FFE },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0380, 0x0380, 0x0380, 0x0380, 0x3FF8,		//	0x2B	0x0B	'+'
	  0x3FF8, 0x0380, 0x0380, 0x0380, 0x0380, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		//	0x2C	0x0C	','
	  0x0000, 0x0000, 0x1E00, 0x1E00, 0x1E00, 0x3E00, 0x7C00, 0x3800, 0x1000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x7FFE,		//	0x2D	0x0D	'-'
	  0x7FFE, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		//	0x2E	0x0E	'.'
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3E00, 0x3E00, 0x3E00, 0x3E00 },

	{ 0x0000, 0x0000, 0x000C, 0x001C, 0x003C, 0x0078, 0x00F0, 0x01E0, 0x03C0,	 	//	0x2F	0x0F	'/'
	  0x0780, 0x0F00, 0x1E00, 0x3C00, 0x3800, 0x3000, 0x0000, 0x0000, 0x0000 },



	{ 0x1FF8, 0x3FFC, 0x783E, 0x703E, 0x707E, 0x706E, 0x70EE, 0x70CE, 0x71CE,		//	0x30	0x10	'0'
	  0x718E, 0x738E, 0x730E, 0x770E, 0x760E, 0x7E0E, 0x7C1E, 0x3FFC, 0x1FF8 },

	{ 0x01C0, 0x03C0, 0x07C0, 0x0FC0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0,		//	0x31	0x11	'1'
	  0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x0FF8, 0x0FF8 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x000E, 0x001E, 0x003C, 0x0078, 0x00F0,		//	0x32	0x12	'2'
	  0x01E0, 0x03C0, 0x0780, 0x0F00, 0x1E00, 0x3C00, 0x7800, 0x7FFE, 0x7FFE },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x000E, 0x001E, 0x003C, 0x0078, 0x01F8, 		//	0x33	0x13	'3'
	  0x01FC, 0x001E, 0x000E, 0x000E, 0x000E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0038, 0x0078, 0x00F8, 0x01F8, 0x03B8, 0x0738, 0x0E38, 0x1C38, 0x3838, 		//	0x34	0x14	'4'
	  0x7038, 0x7FFE, 0x7FFE, 0x0038, 0x0038, 0x0038, 0x0038, 0x0038, 0x0038 },

	{ 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FF8, 0x7FFC,		//	0x35	0x15	'5'
	  0x001E, 0x000E, 0x000E, 0x000E, 0x000E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0FFE, 0x1FFE, 0x3C00, 0x7800, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FF8, 		//	0x36	0x16	'6'
	  0x7FFC, 0x701E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x7FFE, 0x7FFE, 0x000E, 0x000E, 0x001E, 0x003C, 0x0078, 0x00F0, 0x01E0, 		//	0x37	0x17	'7'
	  0x03C0, 0x0780, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3C3C, 0x1FF8,		//	0x38	0x18	'8'
	  0x3FFC, 0x781E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E, 0x700E, 0x700E, 0x780E, 0x3FFE,		//	0x39	0x19	'9'
	  0x1FFE, 0x000E, 0x000E, 0x000E, 0x000E, 0x001E, 0x003C, 0x7FF8, 0x7FF0 },

	{ 0x0000, 0x0000, 0x0000, 0x07C0, 0x07C0, 0x07C0, 0x07C0, 0x0000, 0x0000,		//	0x3A	0x1A	':'
	  0x0000, 0x0000, 0x07C0, 0x07C0, 0x07C0, 0x07C0, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x0000, 0x03C0, 0x03C0, 0x03C0, 0x03C0, 0x0000, 0x0000,		//	0x3B	0x1B	';'
	  0x0000, 0x0000, 0x03C0, 0x03C0, 0x03C0, 0x07C0, 0x0F80, 0x0700, 0x0200 },

	{ 0x0000, 0x00F0, 0x01E0, 0x03C0, 0x0780, 0x0F00, 0x1E00, 0x3C00, 0x7800,		//	0x3C	0x1C	'<'
	  0x7000, 0x7800, 0x3C00, 0x1E00, 0x0F00, 0x0780, 0x03C0, 0x01E0, 0x00F0 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3FFC, 0x3FFC, 0x0000,		//	0x3D	0x1D	'='
	  0x0000, 0x3FFC, 0x3FFC, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0700, 0x0780, 0x03C0, 0x01E0, 0x00F0, 0x0078, 0x003C, 0x001E,		//	0x3E	0x1E	'>'
	  0x000E, 0x001E, 0x003C, 0x0078, 0x00F0, 0x01E0, 0x03C0, 0x0780, 0x0700 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x001E, 0x003C, 0x0078, 0x00F0, 0x01E0, 		//	0x3F	0x1F	'?'
	  0x03C0, 0x0380, 0x0380, 0x0380, 0x0380, 0x0000, 0x0000, 0x0380, 0x0380 },




	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x70FE, 0x70FE, 0x70CE, 0x70CE, 0x70CE, 		//	0x40	0x20	'@'
	  0x70FC, 0x7078, 0x7030, 0x7000, 0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x7FFE,		//	0x41	0x21	'A'
	  0x7FFE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x7FF8, 0x7FFC, 0x701E, 0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x7FFC,		//	0x42	0x22	'B'
	  0x7FFC, 0x701E, 0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x7FFC, 0x7FF8 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 		//	0x43	0x23	'C'
	  0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x7FC0, 0x7FF0, 0x70F8, 0x703C, 0x701C, 0x700E, 0x700E, 0x700E, 0x700E, 		//	0x44	0x24	'D'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x701C, 0x703C, 0x70F8, 0x7FF0, 0x7FC0 },

	{ 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FF0,		//	0x45	0x25	'E'
	  0x7FF0, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FFE, 0x7FFE },

	{ 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FF0,		//	0x46	0x26	'F'
	  0x7FF0, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x7000, 0x7000, 0x7000, 0x7000, 0x70FE, 		//	0x47	0x27	'G'
	  0x70FE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x7FFE,		//	0x48	0x28	'H'
	  0x7FFE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x1FF0, 0x1FF0, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380,		//	0x49	0x29	'I'
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x1FF0, 0x1FF0 },

	{ 0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000E, 0x000E,		//	0x4A	0x2A	'J'
	  0x000E, 0x000E, 0x000E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },
	  
	{ 0x700E, 0x701E, 0x703C, 0x7078, 0x70F0, 0x71E0, 0x73C0, 0x7780, 0x7F00,		//	0x4B	0x2B	'K'
	  0x7F00, 0x7780, 0x73C0, 0x71E0, 0x70F0, 0x7078, 0x703C, 0x701E, 0x700E },

	{ 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000,		//	0x4C	0x2C	'L'
	  0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FFE, 0x7FFE },

	{ 0x700E, 0x781E, 0x7C3E, 0x7E7E, 0x7FFE, 0x77EE, 0x73CE, 0x718E, 0x700E,		//	0x4D	0x2D	'M'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x780E, 0x7C0E, 0x7C0E, 0x7E0E, 0x7E0E, 0x770E, 0x770E, 0x738E, 0x738E,		//	0x4E	0x2E	'N'
	  0x71CE, 0x71CE, 0x70EE, 0x70EE, 0x707E, 0x707E, 0x703E, 0x703E, 0x701E },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 		//	0x4F	0x2F	'O'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x7FF8, 0x7FFC, 0x701E, 0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x7FFC,		//	0x50	0x30	'P'
	  0x7FF8, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 		//	0x51	0x31	'Q'
	  0x700E, 0x700E, 0x71CE, 0x71EE, 0x70FE, 0x707C, 0x78FC, 0x3FFE, 0x1FCE },

	{ 0x7FF8, 0x7FFC, 0x701E, 0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x7FFC,		//	0x52	0x32	'R'
	  0x7FFC, 0x73C0, 0x71E0, 0x70F0, 0x7078, 0x703C, 0x701E, 0x700E, 0x700E },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x7000, 0x7000, 0x7000, 0x7800, 0x3FF8,		//	0x53	0x33	'S'
	  0x1FFC, 0x001E, 0x000E, 0x000E, 0x000E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x7FFC, 0x7FFC, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380,		//	0x54	0x34	'T'
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0x55	0x35	'U'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x381C,		//	0x56	0x36	'V'
	  0x381C, 0x381C, 0x3C3C, 0x1C38, 0x1E78, 0x0FF0, 0x07E0, 0x03C0, 0x0180 },

	{ 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0x57	0x37	'W'
	  0x700E, 0x718E, 0x73CE, 0x77EE, 0x7FFE, 0x7E7E, 0x7C3E, 0x781E, 0x700E },	

	{ 0x700E, 0x700E, 0x700E, 0x781E, 0x3C3C, 0x1E78, 0x0FF0, 0x07E0, 0x03C0,		//	0x58	0x38	'X'
	  0x03C0, 0x07E0, 0x0FF0, 0x1E78, 0x3C3C, 0x781E, 0x700E, 0x700E, 0x700E },

	{ 0x701C, 0x783C, 0x3838, 0x3C78, 0x1C70, 0x1EF0, 0x0EE0, 0x0FE0, 0x07C0,		//	0x59	0x39	'Y'
	  0x07C0, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x7FFE, 0x7FFE, 0x000E, 0x000E, 0x001C, 0x0038, 0x0070, 0x00E0, 0x01C0,		//	0x5A	0x3A	'Z'
	  0x0380, 0x0700, 0x0E00, 0x1C00, 0x3800, 0x7000, 0x7000, 0x7FFE, 0x7FFE },

	{ 0x07F0, 0x07F0, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700,		//	0x5B	0x3B	'['
	  0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x07F0, 0x07F0 },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0x5C	0x3C	'e'' (lower-case e, acute accent)
	  0x700E, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0FE0, 0x0FE0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0,		//	0x5D	0x3D	']'
	  0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x00E0, 0x0FE0, 0x0FE0 },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x0000, 0x0000, 0x0380, 0x0380, 0x0380,		//	0x5E	0x3E	'i'' (lower-case i, acute accent)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0x5F	0x3F	'o'' (lower-case o, acute accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },


	//	Special North-American Character Set
	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x0000, 0x0000, 0x700E, 0x700E, 0x700E,		//	0x60	0x40	'u'' (lower-case u, acute accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x703E, 0x787E, 0x3FEE, 0x1FCE },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x1FF8, 0x1FFC, 0x001E, 0x000E, 0x000E, 		//	0x61	0x41	'a'
	  0x000E, 0x1FFE, 0x3FFE, 0x780E, 0x700E, 0x700E, 0x780E, 0x3FFE, 0x1FFE },

	{ 0x7000, 0x7000, 0x7000, 0x7000, 0x73F8, 0x77FC, 0x7E1E, 0x7C0E, 0x780E,		//	0x62	0x42	'b'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x7FFC, 0x7FF8 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x7000,		//	0x63	0x43	'c'
	  0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x000E, 0x000E, 0x000E, 0x000E, 0x1FCE, 0x3FEE, 0x787E, 0x703E, 0x701E,		//	0x64	0x44	'd'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x780E, 0x3FFE, 0x1FFE },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,		//	0x65	0x45	'e'
	  0x700E, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x00F8, 0x01FC, 0x03DC, 0x0380, 0x0380, 0x0380, 0x0380, 0x7FFC, 0x7FFC,		//	0x66	0x46	'f'
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x1FFE, 0x3FFE, 0x780E, 0x700E, 0x700E,		//	0x67	0x47	'g'
	  0x780E, 0x3FFE, 0x1FFE, 0x000E, 0x000E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x7000, 0x7000, 0x7000, 0x7000, 0x73F8, 0x77FC, 0x7E1E, 0x7C0E, 0x780E,		//	0x68	0x48	'h'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x0380, 0x0380, 0x0000, 0x0000, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380,		//	0x69	0x49	'i'
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x0380, 0x0380, 0x0000, 0x0000, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380,		//	0x6A	0x4A	'j'
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0780, 0x7F00, 0x7E00 },

	{ 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7070, 0x70E0, 0x71C0, 0x7380,		//	0x6B	0x4B	'k'
	  0x7700, 0x7E00, 0x7C00, 0x7E00, 0x7700, 0x7380, 0x71C0, 0x70E0, 0x7070 },

	{ 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380,		//	0x6C	0x4C	'l'	(pipe)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0xFEF8, 0xFFFC, 0xF7DE, 0xE38E, 0xE38E,		//	0x6D	0x4D	'm'
	  0xE38E, 0xE38E, 0xE38E, 0xE38E, 0xE38E, 0xE38E, 0xE38E, 0xE38E, 0xE38E },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x73F8, 0x77FC, 0x7E1E, 0x7C0E, 0x780E,		//	0x6E	0x4E	'n'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,		//	0x6F	0x4F	'o'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x73F8, 0x77FC, 0x7E1E, 0x7C0E, 0x780E,		//	0x70	0x50	'p'
	  0x700E, 0x780E, 0x7C0E, 0x7E1E, 0x77FC, 0x73F8, 0x7000, 0x7000, 0x7000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x1FCE, 0x3FEE, 0x787E, 0x703E, 0x701E,		//	0x71	0x51	'q'
	  0x700E, 0x701E, 0x703E, 0x787E, 0x3FEE, 0x1FCE, 0x000E, 0x000E, 0x000E },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x73F8, 0x77FC, 0x7E1E, 0x7C0E, 0x7800,		// 	0x72	0x52	'r'
	  0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x7000,		//	0x73	0x53	's'
	  0x7800, 0x3FF8, 0x1FFC, 0x001E, 0x000E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0380, 0x0380, 0x0380, 0x0380, 0x7FFC, 0x7FFC, 0x0380, 0x0380, 0x0380,		//	0x74	0x54	't'
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x03C0, 0x01FC, 0x00FC },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0x75	0x55	'u'
	  0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x703E, 0x787E, 0x3FEE, 0x1FCE },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x700E, 0x700E, 0x700E, 0x700E, 0x381C,		//	0x76	0x56	'v'
	  0x381C, 0x1C38, 0x1C38, 0x0E70, 0x0E70, 0x07E0, 0x07E0, 0x03C0, 0x03C0 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0x77	0x57	'w'
	  0x700E, 0x700E, 0x700E, 0x718E, 0x718E, 0x718E, 0x7BDE, 0x3FFC, 0x1E78 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x700E, 0x700E, 0x781E, 0x3C3C, 0x1E78,		//	0x78	0x58	'x'
	  0x0FF0, 0x07E0, 0x07E0, 0x0FF0, 0x1E78, 0x3C3C, 0x781E, 0x700E, 0x700E },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0x79	0x59	'y'
	  0x780E, 0x3FFE, 0x1FFE, 0x000E, 0x000E, 0x000E, 0x001E, 0x1FFC, 0x1FF8 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x7FFE, 0x7FFE, 0x001C, 0x0038, 0x0070,		//	0x7A	0x5A	'z'
	  0x00E0, 0x01C0, 0x0380, 0x0700, 0x0E00, 0x1C00, 0x3800, 0x7FFE, 0x7FFE },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x7000,		//	0x7B	0x5B	'c' (lower-case c with cedilla)
	  0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8, 0x01C0, 0x01C0, 0x0F80, 0x0F00 },

	{ 0x0000, 0x0000, 0x0000, 0x01C0, 0x01C0, 0x0000, 0x0000, 0x1FFC, 0x1FFC,		//	0x7C	0x5C	'-:' (division sign)
	  0x0000, 0x0000, 0x01C0, 0x01C0, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x1E06, 0x3F8E, 0x71FC, 0x6078, 0x0000, 0x0000, 0x780E, 0x7C0E, 0x7E0E,		//	0x7D	0x5D	'N~' (upper-case N with tilde)
	  0x7F0E, 0x778E, 0x73CE, 0x71EE, 0x70FE, 0x707E, 0x703E, 0x701E, 0x700E },

	{ 0x1E06, 0x3F8E, 0x71FC, 0x6078, 0x0000, 0x0000, 0x73F8, 0x77FC, 0x7E1E,		//	0x7E	0x5E	'n~' (lower-case n with tilde)
	  0x7C0E, 0x780E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE,		//	0x7F	0x5F	(SB) (solid block) (full block)
	  0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE, 0x7FFE },



	{ 0x3FFC, 0x7FFE, 0x6006, 0x4FE2, 0x4FF2, 0x4C32, 0x4C32, 0x4FF2, 0x4FE2,  		//	0x80	0x60	'R' (registered mark) (service mark) (registered sign)
	  0x4DC2, 0x4CE2, 0x4C72, 0x6006, 0x7FFE, 0x3FFC, 0x0000, 0x0000, 0x0000 },

	{ 0x07C0, 0x0FE0, 0x1C70, 0x1830, 0x1830, 0x1C70, 0x0FE0, 0x07C0, 0x0000,		//	0x81	0x61	'o' (degree sign)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x3800, 0x38F8,		//	0x82	0x62	'1/2' (half symbol) (1/2 symbol)
	  0x38FC, 0x001E, 0x000E, 0x001C, 0x0038, 0x0070, 0x00E0, 0x00FE, 0x00FE },

	{ 0x01C0, 0x01C0, 0x0000, 0x0000, 0x01C0, 0x01C0, 0x01C0, 0x01C0, 0x03C0, 		//	0x83	0x63	'?' (inverted question mark)
	  0x0780, 0x0F00, 0x1E00, 0x3C00, 0x7800, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x7E00, 0x7E00, 0x1800, 0x1800, 0x1A02, 0x1B06, 0x1B8E, 0x1BDE, 0x1BFE,  		//	0x84	0x64	'TM' (trade mark sign)
	  0x0376, 0x0326, 0x0306, 0x0306, 0x0306, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x01C0, 0x01C0, 0x0FF8, 0x1FFC, 0x3FFE, 0x79CE, 0x71C0, 0x71C0,		//	0x85	0x65	'c' (cents sign) (cents symbol)
	  0x71C0, 0x71C0, 0x79CE, 0x3FFE, 0x1FFC, 0x0FF8, 0x01C0, 0x01C0, 0x0000 },

	{ 0x03F8, 0x07FC, 0x0F1E, 0x0E0E, 0x0E00, 0x0E00, 0x0E00, 0x0E00, 0x0FF0,		//	0x86	0x66	'L' (pound stirling sign) (pound sterling)
	  0x0FF0, 0x0E00, 0x0E00, 0x0E00, 0x0E00, 0x0E00, 0x1F00, 0x7FFE, 0x79FE },

	{ 0x00E0, 0x00F0, 0x00F8, 0x00FC, 0x00EE, 0x00E6, 0x00E2, 0x00E0, 0x00E0, 		//	0x87	0x67	'b' (music note) (eighth note)
	  0x00E0, 0x0FE0, 0x1FE0, 0x3FE0, 0x3FE0, 0x3FE0, 0x3FE0, 0x1FC0, 0x0F80 },

	{ 0x1C00, 0x0E00, 0x0700, 0x0380, 0x0000, 0x0000, 0x1FF8, 0x1FFC, 0x001E, 		//	0x88	0x68	'a`' (lower-case a, grave accent)
	  0x000E, 0x1FFE, 0x3FFE, 0x781E, 0x700E, 0x700E, 0x781E, 0x3FFE, 0x1FFE },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		//	0x89	0x69	' ' (transparent space)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x1C00, 0x0E00, 0x0700, 0x0380, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0x8A	0x6A	'e`' (lower-case e, grave accent)
	  0x700E, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },
	  
	{ 0x03C0, 0x07E0, 0x0E70, 0x1C38, 0x0000, 0x0000, 0x1FF8, 0x1FFC, 0x001E, 		//	0x8B	0x6B	'a' (lower-case a, circumflex accent)
	  0x000E, 0x1FFE, 0x3FFE, 0x781E, 0x700E, 0x700E, 0x781E, 0x3FFE, 0x1FFE },

	{ 0x03C0, 0x07E0, 0x0E70, 0x1C38, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0x8C	0x6C	'e' (lower-case e, circumflex accent)
	  0x700E, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x03C0, 0x07E0, 0x0E70, 0x1C38, 0x0000, 0x0000, 0x0380, 0x0380, 0x0380,		//	0x8D	0x6D	'i' (lower-case i, circumflex accent)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x03C0, 0x07E0, 0x0E70, 0x1C38, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0x8E	0x6E	'o' (lower-case o, circumflex accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x03C0, 0x07E0, 0x0E70, 0x1C38, 0x0000, 0x0000, 0x700E, 0x700E, 0x700E,		//	0x8F	0x6F	'u' (lower-case u, circumflex accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x703E, 0x787E, 0x3FEE, 0x1FCE },

	//	The following glyphs were added in the 12.3 SDK...
	{ 0x1C00, 0x0E00, 0x0700, 0x0380, 0x0000, 0x0000, 0x0380, 0x0380, 0x0380,		//	0x90	0x70	'i`' (lower-case i, grave accent)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x1C00, 0x0E00, 0x0700, 0x01C0, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0x91	0x71	'o`' (lower-case o, grave accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x1C00, 0x0E00, 0x0700, 0x01C0, 0x0000, 0x0000, 0x700E, 0x700E, 0x700E,		//	0x92	0x72	'u`' (lower-case u, grave accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x703E, 0x787E, 0x3FEE, 0x1FCE },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,	 	//	0x93	0x73	'A'' (upper-case A, acute accent)
	  0x700E, 0x700E, 0x7FFE, 0x7FFE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x7000,		//	0x94	0x74	'E'' (upper-case E, acute accent)
	  0x7000, 0x7FF0, 0x7FF0, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FFE, 0x7FFE },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x1FF0, 0x1FF0, 0x0380, 0x0380, 0x0380,		//	0x95	0x75	'I'' (upper-case I, acute accent)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x1FF0, 0x1FF0 },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,		//	0x96	0x76	'O'' (upper-case O, acute accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0038, 0x0070, 0x00E0, 0x01C0, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0x97	0x77	'U'' (upper-case U, acute accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x1C00, 0x0E00, 0x0700, 0x0380, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,	 	//	0x98	0x78	'A`' (upper-case A, grave accent)
	  0x700E, 0x700E, 0x7FFE, 0x7FFE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x1C00, 0x0E00, 0x0700, 0x0380, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x7000,		//	0x99	0x79	'E`' (upper-case E, grave accent)
	  0x7000, 0x7FF0, 0x7FF0, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FFE, 0x7FFE },

	{ 0x1C00, 0x0E00, 0x0700, 0x0380, 0x1FF0, 0x1FF0, 0x0380, 0x0380, 0x0380,		//	0x9A	0x7A	'I`' (upper-case I, grave accent)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x1FF0, 0x1FF0 },

	{ 0x1C00, 0x0E00, 0x0700, 0x0380, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,		//	0x9B	0x7B	'O`' (upper-case O, grave accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x1C00, 0x0E00, 0x0700, 0x0380, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0x9C	0x7C	'U`' (upper-case U, grave accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x03C0, 0x07E0, 0x0E70, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,	 	//	0x9D	0x7D	'A' (upper-case A, circumflex accent)
	  0x700E, 0x700E, 0x7FFE, 0x7FFE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x03C0, 0x07E0, 0x0E70, 0x0000, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x7000,		//	0x9E	0x7E	'E' (upper-case E, circumflex accent)
	  0x7000, 0x7FF0, 0x7FF0, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FFE, 0x7FFE },

	{ 0x03C0, 0x07E0, 0x0E70, 0x0000, 0x1FF0, 0x1FF0, 0x0380, 0x0380, 0x0380,		//	0x9F	0x7F	'I' (upper-case I, circumflex accent)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x1FF0, 0x1FF0 },

	{ 0x03C0, 0x07E0, 0x0E70, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,		//	0xA0	0x80	'O' (upper-case O, circumflex accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x03C0, 0x07E0, 0x0E70, 0x0000, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0xA1	0x81	'U' (upper-case U, circumflex accent)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0000, 0x0E70, 0x0E70, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,	 	//	0xA2	0x82	'A' (upper-case A, umlaut)
	  0x700E, 0x700E, 0x7FFE, 0x7FFE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x0000, 0x0E70, 0x0E70, 0x0000, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x7000,		//	0xA3	0x83	'E' (upper-case E, umlaut)
	  0x7000, 0x7FF0, 0x7FF0, 0x7000, 0x7000, 0x7000, 0x7000, 0x7FFE, 0x7FFE },

	{ 0x0000, 0x0E70, 0x0E70, 0x0000, 0x1FF0, 0x1FF0, 0x0380, 0x0380, 0x0380,		//	0xA4	0x84	'I' (upper-case I, umlaut)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x1FF0, 0x1FF0 },

	{ 0x0000, 0x0E70, 0x0E70, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,		//	0xA5	0x85	'O' (upper-case O, umlaut)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0000, 0x0E70, 0x0E70, 0x0000, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E,		//	0xA6	0x86	'U' (upper-case U, umlaut)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0000, 0x0000, 0x0E70, 0x0E70, 0x0000, 0x0000, 0x1FF8, 0x1FFC, 0x001E,	 	//	0xA7	0x87	'a' (lower-case a, umlaut)
	  0x000E, 0x1FFE, 0x3FFE, 0x781E, 0x700E, 0x700E, 0x781E, 0x3FFE, 0x1FFE },

	{ 0x0000, 0x0000, 0x0E70, 0x0E70, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0xA8	0x88	'e' (lower-case e, umlaut)
	  0x700E, 0x7FFE, 0x7FFE, 0x7000, 0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0000, 0x0000, 0x0E70, 0x0E70, 0x0000, 0x0000, 0x0380, 0x0380, 0x0380,		//	0xA9	0x89	'i' (lower-case i, umlaut)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x0000, 0x0000, 0x0E70, 0x0E70, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0xAA	0x8A	'o' (lower-case o, umlaut)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0000, 0x0000, 0x0E70, 0x0E70, 0x0000, 0x0000, 0x700E, 0x700E, 0x700E,		//	0xAB	0x8B	'u' (lower-case u, umlaut)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x701E, 0x703E, 0x787E, 0x3FEE, 0x1FCE },

	{ 0x03C0, 0x03C0, 0x03C0, 0x0300, 0x0180, 0x00C0, 0x0000, 0x0000, 0x0000,		//	0xAC	0x8C	'\'' (opening single quote)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x03C0, 0x03C0, 0x03C0, 0x00C0, 0x0180, 0x0300, 0x0000, 0x0000, 0x0000,		//	0xAD	0x8D	'\'' (closing single quote)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x1E78, 0x1E78, 0x1E78, 0x1860, 0x0C30, 0x0618, 0x0000, 0x0000, 0x0000,		//	0xAE	0x8E	'"' (opening double quotes)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x1E78, 0x1E78, 0x1E78, 0x0618, 0x0C30, 0x1860, 0x0000, 0x0000, 0x0000,		//	0xAF	0x8F	'"' (closing double quotes)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0380, 0x0380, 0x0000, 0x0000, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380,		//	0xB0	0x90	'!' (inverted exclamation)
	  0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x7000, 0x7000, 0x7000, 0x7000, 0x7000,		//	0xB1	0x91	'C' (upper-case C with cedilla)
	  0x7000, 0x700E, 0x781E, 0x3FFC, 0x1FF8, 0x01C0, 0x01C0, 0x0F80, 0x0F00 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x038E, 0x071C, 0x0E38, 0x1C70, 0x38E0,		//	0xB2	0x92	'<' (opening guillemet)
	  0x71C0, 0x38E0, 0x1C70, 0x0E38, 0x071C, 0x038E, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x71C0, 0x38E0, 0x1C70, 0x0E38, 0x071C,		//	0xB3	0x93	'>' (closing guillemet)
	  0x038E, 0x071C, 0x0E38, 0x1C70, 0x38E0, 0x71C0, 0x0000, 0x0000, 0x0000 },

	{ 0x1E06, 0x3F8E, 0x71FC, 0x6078, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E,		//	0xB4	0x94	'A~' (upper-case A with tilde)
	  0x700E, 0x700E, 0x700E, 0x7FFE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x1E06, 0x3F8E, 0x71FC, 0x6078, 0x0000, 0x0000, 0x1FF8, 0x1FFC, 0x001E,		//	0xB5	0x95	'a~' (lower-case a with tilde)
	  0x000E, 0x1FFE, 0x3FFE, 0x781E, 0x700E, 0x700E, 0x781E, 0x3FFE, 0x1FFE },

	{ 0x1E06, 0x3F8E, 0x71FC, 0x6078, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E,		//	0xB6	0x96	'O~' (upper-case O with tilde)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x1E06, 0x3F8E, 0x71FC, 0x6078, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0xB7	0x97	'o~' (lower-case o with tilde)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x03C0, 0x0660, 0x03C0, 0x0000, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E,		//	0xB8	0x98	'Ao' (upper-case A with ring)
	  0x700E, 0x700E, 0x700E, 0x7FFE, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E },

	{ 0x0000, 0x0000, 0x03C0, 0x0660, 0x03C0, 0x0000, 0x1FF8, 0x1FFC, 0x001E,		//	0xB9	0x99	'ao' (lower-case a with ring)
	  0x000E, 0x1FFE, 0x3FFE, 0x781E, 0x700E, 0x700E, 0x781E, 0x3FFE, 0x1FFE },

	{ 0x03C0, 0x0660, 0x03C0, 0x0000, 0x1FF8, 0x3FFC, 0x781E, 0x700E, 0x700E,		//	0xBA	0x9A	'Oo' (upper-case O with ring)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x0000, 0x0000, 0x03C0, 0x0660, 0x03C0, 0x0000, 0x1FF8, 0x3FFC, 0x781E,		//	0xBB	0x9B	'oo' (lower-case o with ring)
	  0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x700E, 0x781E, 0x3FFC, 0x1FF8 },

	{ 0x1FFB, 0x3FFF, 0x781E, 0x701E, 0x703E, 0x707E, 0x70EE, 0x71CE, 0x738E,		//	0xBC	0x9C	'O/' (upper-case O with stroke)
	  0x770E, 0x7E0E, 0x7C0E, 0x780E, 0x700E, 0xF00E, 0xF00E, 0xBFFC, 0x1FF8 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0007, 0x1FFE, 0x3FFC, 0x783E, 0x707E,		//	0xBD	0x9D	'o/' (lower-case o with stroke)
	  0x70EE, 0x71CE, 0x738E, 0x770E, 0x7E0E, 0x7C0E, 0x781E, 0x7FFC, 0xFFF8 },

	{ 0x3FC0, 0x7FE0, 0x7070, 0x7038, 0x7038, 0x7038, 0x7070, 0x73E0, 0x73F8,		//	0xBE	0x9E	'B' (small sharp s)
	  0x701C, 0x700E, 0x7007, 0x7007, 0x7007, 0x700E, 0x739C, 0x71F8, 0x70F0 },

	{ 0x03F0, 0x07F0, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x1E00,		//	0xBF	0x9F	'[' (opening brace)
	  0x1E00, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x07F0, 0x03F0 },

	{ 0x07E0, 0x07F0, 0x0070, 0x0070, 0x0070, 0x0070, 0x0070, 0x0070, 0x003C,		//	0xC0	0xA0	']' (closing brace)
	  0x003C, 0x0070, 0x0070, 0x0070, 0x0070, 0x0070, 0x0070, 0x07F0, 0x07E0 },

	{ 0x0000, 0x0000, 0x0000, 0x638C, 0x3398, 0x1BB0, 0x0FE0, 0x07C0, 0x7FFC,		//	0xC1	0xA1	'*' (asterisk)
	  0x7FFC, 0x0FE0, 0x1BB0, 0x3398, 0x638C, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0180, 0x07E0, 0x0FF0,		//	0xC2	0xA2	'o' (round bullet)
	  0x0FF0, 0x07E0, 0x0180, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF,		//	0xC3	0xA3	'-' (em dash)
	  0xFFFF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x3FFC, 0x7FFE, 0x6006, 0x47E2, 0x4FF2, 0x4C32, 0x4C02, 0x4C02, 0x4C02,  		//	0xC4	0xA4	'C' (copyright)
	  0x4C32, 0x4FF2, 0x47E2, 0x6006, 0x7FFE, 0x3FFC, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x3000, 0x3800, 0x3C00, 0x1E00, 0x0F00, 0x0780, 0x03C0,	 	//	0xC5	0xA5	'/' (backslash)
	  0x01E0, 0x00F0, 0x0078, 0x003C, 0x001C, 0x000C, 0x0000, 0x0000, 0x0000 },

	{ 0x03C0, 0x07E0, 0x0E70, 0x1C38, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		//	0xC6	0xA6	'^' (caret)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		//	0xC7	0xA7	'_' (underbar)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xFFFF, 0xFFFF },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1E06, 0x3F8E,		//	0xC8	0xA8	'~' (tilde)
	  0x71FC, 0x6078, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0x701C, 0x783C, 0x3838, 0x3C78, 0x1C70, 0x1EF0, 0x0EE0, 0x07C0, 0x7FFC,		//	0xC9	0xA9	'Y' (yen sign)
	  0x7FFC, 0x0380, 0x0380, 0x7FFC, 0x7FFC, 0x0380, 0x0380, 0x0380, 0x0380 },

	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x2004, 0x17E8, 0x0FF0, 0x1818, 0x1818,		//	0xCA	0xAA	'o' (non-specific currency sign)
	  0x0FF0, 0x17E8, 0x2004, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	{ 0xFFFF, 0xFFFF, 0xFFFF, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000,		//	0xCB	0xAB	(upper-left corner)
	  0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000 },

	{ 0xFFFF, 0xFFFF, 0xFFFF, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F,		//	0xCC	0xAC	(upper-right corner)
	  0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F },

	{ 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000,		//	0xCD	0xAD	(lower-left corner)
	  0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xF000, 0xFFFF, 0xFFFF, 0xFFFF },

	{ 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F,		//	0xCE	0xAE	(lower-right corner)
	  0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0xFFFF, 0xFFFF, 0xFFFF },


	//////////////////////////////////////
	//	NOTE:  INSERT NEW GLYPHS HERE   //
	//////////////////////////////////////


	//	SPECIAL CASE:	We want to be able to display a "blank space" character that
	//					cannot be underlined (e.g. for mid-row codes). This is it...
	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		//	0xCF	0xAF	NTV2_CCFont_NoUnderlineSpaceChar (a blank space, never rendered with an underline)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },

	//	SPECIAL CASE:	Rather than rendering a whole separate set of characters with
	//					underlines, we render ONE character (this "space") with an
	//					underline and then adjust our copy routine to get the last 3
	//					lines from here...
	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,		//	0xD0	0xB0	NTV2_CCFont_UnderlineChar (a blank space, rendered with an underline)
	  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 }

};	//	gCCFontDotMap


const int NTV2_CCFont_NumChars				= sizeof (gCCFontDotMap) / sizeof (UWord) / NTV2_CCFont_BitmapCharHeight;							// number of characters in font map
const int NTV2_CCFont_UnderlineChar			= NTV2_CCFont_NumChars - 1;		// index of "underline" character (blank space, underlined) = last character in map
const int NTV2_CCFont_NoUnderlineSpaceChar	= NTV2_CCFont_NumChars - 2;		// index of a "blank space" character that can not (should not) have an underline applied

const int NTV2_CCFont_CharUnderline1		= (NTV2_CCFont_CharDotTopSpace + NTV2_CCFont_BitmapCharHeight + 2);	// 1st line of underline
const int NTV2_CCFont_CharUnderline2		= (NTV2_CCFont_CharDotTopSpace + NTV2_CCFont_BitmapCharHeight + 3);	// 2nd line of underline


const int NTV2_CCFont_AsciiOffset			= 0x20;	//	The first glyph in the table ("0") corresponds to ASCII character 0x20 ("space")


//
//	Map CEA-608 codepoints to CCFont glyph indices	(see http://en.wikipedia.org/wiki/EIA-608)
//
typedef	map <NTV2_CC608_CodePoint, NTV2GlyphIndex>		CEA608CodePointToGlyphIndexMap;
typedef	pair <NTV2_CC608_CodePoint, NTV2GlyphIndex>		CEA608CodePointToGlyphIndexPair;
typedef CEA608CodePointToGlyphIndexMap::const_iterator	CEA608CodePointToGlyphIndexConstIter;

static CEA608CodePointToGlyphIndexMap					gCodePointToGlyphIndexMap;


#define	CEA608_CODEPOINT(_charset_, _608_1, _608_2)		::Make608CodePoint ((_608_1), (_608_2), (_charset_))
#define	STD_CODEPOINT(_608_1, _608_2)					CEA608_CODEPOINT (NTV2_CC608_DefaultCharacterSet,		(_608_1), (_608_2))
#define	DBLSZ_CODEPOINT(_608_1, _608_2)					CEA608_CODEPOINT (NTV2_CC608_DoubleSizeCharacterSet,	(_608_1), (_608_2))
#define	STD_TO_CCFONT(_608_1, _608_2, _ccf_)			gCodePointToGlyphIndexMap.insert (CEA608CodePointToGlyphIndexPair (STD_CODEPOINT ((_608_1), (_608_2)), (_ccf_)));
#define	DBL_TO_CCFONT(_608_1, _608_2, _ccf_)			gCodePointToGlyphIndexMap.insert (CEA608CodePointToGlyphIndexPair (DBLSZ_CODEPOINT ((_608_1), (_608_2)), (_ccf_)));
#define	STD_DBL_TO_CCFONT(_608_1, _608_2, _ccf_)		STD_TO_CCFONT ((_608_1), (_608_2), (_ccf_));	DBL_TO_CCFONT ((_608_1), (_608_2), (_ccf_));

#define STD_DBL_TO_CCFONT_TBD(_608_1, _608_2, _ccf_)	//	** MISSING GLYPHS THAT NEED TO BE IMPLEMENTED **


class CCFontTableInitializer
{
	public:
		CCFontTableInitializer ()
		{
			//	NORTH AMERICAN CHARACTER SET
			for (UByte ascii (0x20);  ascii <= 0x7F;  ascii++)
			{
				if (gCodePointToGlyphIndexMap.find (STD_CODEPOINT (ascii, 0x00)) == gCodePointToGlyphIndexMap.end ())
					STD_DBL_TO_CCFONT (ascii, 0x00, ascii);
			}

			//	Special North American (channel 1)
			STD_DBL_TO_CCFONT (0x11, 0x30, 0x80);	//	registered sign
			STD_DBL_TO_CCFONT (0x11, 0x31, 0x81);	//	degree sign
			STD_DBL_TO_CCFONT (0x11, 0x32, 0x82);	//	1/2 symbol
			STD_DBL_TO_CCFONT (0x11, 0x33, 0x83);	//	inverted question mark
			STD_DBL_TO_CCFONT (0x11, 0x34, 0x84);	//	trade mark sign
			STD_DBL_TO_CCFONT (0x11, 0x35, 0x85);	//	cents symbol
			STD_DBL_TO_CCFONT (0x11, 0x36, 0x86);	//	pound sterling
			STD_DBL_TO_CCFONT (0x11, 0x37, 0x87);	//	eighth note
			STD_DBL_TO_CCFONT (0x11, 0x38, 0x88);	//	lower-case a, grave accent
			STD_DBL_TO_CCFONT (0x11, 0x39, 0x89);	//	transparent space
			STD_DBL_TO_CCFONT (0x11, 0x3A, 0x8A);	//	lower-case e, grave accent
			STD_DBL_TO_CCFONT (0x11, 0x3B, 0x8B);	//	lower-case a, circumflex accent
			STD_DBL_TO_CCFONT (0x11, 0x3C, 0x8C);	//	lower-case e, circumflex accent
			STD_DBL_TO_CCFONT (0x11, 0x3D, 0x8D);	//	lower-case i, circumflex accent
			STD_DBL_TO_CCFONT (0x11, 0x3E, 0x8E);	//	lower-case o, circumflex accent
			STD_DBL_TO_CCFONT (0x11, 0x3F, 0x8F);	//	lower-case u, circumflex accent

			//	Special North American (channel 2)
			STD_DBL_TO_CCFONT (0x19, 0x30, 0x80);	//	registered sign
			STD_DBL_TO_CCFONT (0x19, 0x31, 0x81);	//	degree sign
			STD_DBL_TO_CCFONT (0x19, 0x32, 0x82);	//	1/2 symbol
			STD_DBL_TO_CCFONT (0x19, 0x33, 0x83);	//	inverted question mark
			STD_DBL_TO_CCFONT (0x19, 0x34, 0x84);	//	trade mark sign
			STD_DBL_TO_CCFONT (0x19, 0x35, 0x85);	//	cents symbol
			STD_DBL_TO_CCFONT (0x19, 0x36, 0x86);	//	pound sterling
			STD_DBL_TO_CCFONT (0x19, 0x37, 0x87);	//	eighth note
			STD_DBL_TO_CCFONT (0x19, 0x38, 0x88);	//	lower-case a, grave accent
			STD_DBL_TO_CCFONT (0x19, 0x39, 0x89);	//	transparent space
			STD_DBL_TO_CCFONT (0x19, 0x3A, 0x8A);	//	lower-case e, grave accent
			STD_DBL_TO_CCFONT (0x19, 0x3B, 0x8B);	//	lower-case a, circumflex accent
			STD_DBL_TO_CCFONT (0x19, 0x3C, 0x8C);	//	lower-case e, circumflex accent
			STD_DBL_TO_CCFONT (0x19, 0x3D, 0x8D);	//	lower-case i, circumflex accent
			STD_DBL_TO_CCFONT (0x19, 0x3E, 0x8E);	//	lower-case o, circumflex accent
			STD_DBL_TO_CCFONT (0x19, 0x3F, 0x8F);	//	lower-case u, circumflex accent

			//	EXTENDED WESTERN EUROPEAN CHARACTER SET

			//	Extended Spanish/Misc, channel 1 or 3
			STD_DBL_TO_CCFONT     (0x12, 0x20, 0x93);		//	upper-case A, acute accent			'A'
			STD_DBL_TO_CCFONT     (0x12, 0x21, 0x94);		//	upper-case E, acute accent			'E'
			STD_DBL_TO_CCFONT     (0x12, 0x22, 0x96);		//	upper-case O, acute accent			'O'
			STD_DBL_TO_CCFONT     (0x12, 0x23, 0x97);		//	upper-case U, acute accent			'U'
			STD_DBL_TO_CCFONT     (0x12, 0x24, 0xA6);		//	upper-case U, umlaut				'U'
			STD_DBL_TO_CCFONT     (0x12, 0x25, 0xAB);		//	lower-case u, umlaut				'u'
			STD_DBL_TO_CCFONT     (0x12, 0x26, 0xAC);		//	opening single quote				'`'
			STD_DBL_TO_CCFONT     (0x12, 0x27, 0xB0);		//	inverted exclamation				'!'
			STD_DBL_TO_CCFONT     (0x12, 0x28, 0xC1);		//	asterisk							'*'
			STD_DBL_TO_CCFONT     (0x12, 0x29, 0xAD);		//	closing single quote				'\''
			STD_DBL_TO_CCFONT     (0x12, 0x2a, 0xC3);		//	em dash								'_'
			STD_DBL_TO_CCFONT     (0x12, 0x2b, 0xC4);		//	copyright							'c'
			STD_DBL_TO_CCFONT     (0x12, 0x2c, 0x80);		//TBD"\u2120"	service mark						's'
			STD_DBL_TO_CCFONT     (0x12, 0x2d, 0xC2);		//	round bullet						'o'
			STD_DBL_TO_CCFONT     (0x12, 0x2e, 0xAE);		//	opening double quotes				'"'
			STD_DBL_TO_CCFONT     (0x12, 0x2f, 0xAF);		//	closing double quotes				'"'

			//	Extended French, channel 1 or 3
			STD_DBL_TO_CCFONT     (0x12, 0x30, 0x98);		//	upper-case A, grave accent			'A'
			STD_DBL_TO_CCFONT     (0x12, 0x31, 0x9D);		//	upper-case A, circumflex accent		'A'
			STD_DBL_TO_CCFONT     (0x12, 0x32, 0xB1);		//	upper-case C with cedilla			'C'
			STD_DBL_TO_CCFONT     (0x12, 0x33, 0x99);		//	upper-case E, grave accent			'E'
			STD_DBL_TO_CCFONT     (0x12, 0x34, 0x9E);		//	upper-case E, circumflex accent		'E'
			STD_DBL_TO_CCFONT     (0x12, 0x35, 0xA3);		//	upper-case E, umlaut				'E'
			STD_DBL_TO_CCFONT     (0x12, 0x36, 0xA8);		//	lower-case e, umlaut				'e'
			STD_DBL_TO_CCFONT     (0x12, 0x37, 0x9F);		//	upper-case I, circumflex accent		'I'
			STD_DBL_TO_CCFONT     (0x12, 0x38, 0xA4);		//	upper-case I, umlaut				'I'
			STD_DBL_TO_CCFONT     (0x12, 0x39, 0xA9);		//	lower-case i, umlaut				'i'
			STD_DBL_TO_CCFONT     (0x12, 0x3a, 0xA0);		//	upper-case O, circumflex accent		'O'
			STD_DBL_TO_CCFONT     (0x12, 0x3b, 0x9C);		//	upper-case U, grave accent			'U'
			STD_DBL_TO_CCFONT     (0x12, 0x3c, 0x92);		//	lower-case u, grave accent			'u'
			STD_DBL_TO_CCFONT     (0x12, 0x3d, 0xA1);		//	upper-case U, circumflex accent		'U'
			STD_DBL_TO_CCFONT     (0x12, 0x3e, 0xB2);		//	opening guillemet					'<'
			STD_DBL_TO_CCFONT     (0x12, 0x3f, 0xB3);		//	closing guillemet					'>'

			//	Extended Spanish/Misc, channel 2 or 4
			STD_DBL_TO_CCFONT     (0x1a, 0x20, 0x93);		//	upper-case A, acute accent			'A'
			STD_DBL_TO_CCFONT     (0x1a, 0x21, 0x94);		//	upper-case E, acute accent			'E'
			STD_DBL_TO_CCFONT     (0x1a, 0x22, 0x96);		//	upper-case O, acute accent			'O'
			STD_DBL_TO_CCFONT     (0x1a, 0x23, 0x97);		//	upper-case U, acute accent			'U'
			STD_DBL_TO_CCFONT     (0x1a, 0x24, 0xA6);		//	upper-case U, umlaut				'U'
			STD_DBL_TO_CCFONT     (0x1a, 0x25, 0xAB);		//	lower-case u, umlaut				'u'
			STD_DBL_TO_CCFONT     (0x1a, 0x26, 0xAC);		//	opening single quote				'`'
			STD_DBL_TO_CCFONT     (0x1a, 0x27, 0xB0);		//	inverted exclamation				'!'
			STD_DBL_TO_CCFONT     (0x1a, 0x28, 0xC1);		//	asterisk							'*'
			STD_DBL_TO_CCFONT     (0x1a, 0x29, 0xAD);		//	closing single quote				'\''
			STD_DBL_TO_CCFONT     (0x1a, 0x2a, 0xC3);		//	em dash								'_'
			STD_DBL_TO_CCFONT     (0x1a, 0x2b, 0xC4);		//	copyright							'c'
			STD_DBL_TO_CCFONT     (0x1a, 0x2c, 0x80);		//TBD"\u2120"	service mark						's'
			STD_DBL_TO_CCFONT     (0x1a, 0x2d, 0xC2);		//	round bullet						'o'
			STD_DBL_TO_CCFONT     (0x1a, 0x2e, 0xAE);		//	opening double quotes				'"'
			STD_DBL_TO_CCFONT     (0x1a, 0x2f, 0xAF);		//	closing double quotes				'"'

			//	Extended French, channel 2 or 4
			STD_DBL_TO_CCFONT     (0x1a, 0x30, 0x98);		//	upper-case A, grave accent			'A'
			STD_DBL_TO_CCFONT     (0x1a, 0x31, 0x9D);		//	upper-case A, circumflex accent		'A'
			STD_DBL_TO_CCFONT     (0x1a, 0x32, 0xB1);		//	upper-case C with cedilla			'C'
			STD_DBL_TO_CCFONT     (0x1a, 0x33, 0x99);		//	upper-case E, grave accent			'E'
			STD_DBL_TO_CCFONT     (0x1a, 0x34, 0x9E);		//	upper-case E, circumflex accent		'E'
			STD_DBL_TO_CCFONT     (0x1a, 0x35, 0xA3);		//	upper-case E, umlaut				'E'
			STD_DBL_TO_CCFONT     (0x1a, 0x36, 0xA8);		//	lower-case e, umlaut				'e'
			STD_DBL_TO_CCFONT     (0x1a, 0x37, 0x9F);		//	upper-case I, circumflex accent		'I'
			STD_DBL_TO_CCFONT     (0x1a, 0x38, 0xA4);		//	upper-case I, umlaut				'I'
			STD_DBL_TO_CCFONT     (0x1a, 0x39, 0xA9);		//	lower-case i, umlaut				'i'
			STD_DBL_TO_CCFONT     (0x1a, 0x3a, 0xA0);		//	upper-case O, circumflex accent		'O'
			STD_DBL_TO_CCFONT     (0x1a, 0x3b, 0x9C);		//	upper-case U, grave accent			'U'
			STD_DBL_TO_CCFONT     (0x1a, 0x3c, 0x92);		//	lower-case u, grave accent			'u'
			STD_DBL_TO_CCFONT     (0x1a, 0x3d, 0xA1);		//	upper-case U, circumflex accent		'U'
			STD_DBL_TO_CCFONT     (0x1a, 0x3e, 0xB2);		//	opening guillemet					'<'
			STD_DBL_TO_CCFONT     (0x1a, 0x3f, 0xB3);		//	closing guillemet					'>'

			//	Extended Portuguese, channels 1 or 3
			STD_DBL_TO_CCFONT     (0x13, 0x20, 0xB4);		//	upper-case A with tilde				'A'
			STD_DBL_TO_CCFONT     (0x13, 0x21, 0xB5);		//	lower-case a with tilde				'E'
			STD_DBL_TO_CCFONT     (0x13, 0x22, 0x95);		//	upper-case I, acute accent			'I'
			STD_DBL_TO_CCFONT     (0x13, 0x23, 0x9A);		//	upper-case I, grave accent			'I'
			STD_DBL_TO_CCFONT     (0x13, 0x24, 0x90);		//	lower-case i, grave accent			'i'
			STD_DBL_TO_CCFONT     (0x13, 0x25, 0x9B);		//	upper-case O, grave accent			'O'
			STD_DBL_TO_CCFONT     (0x13, 0x26, 0x91);		//	lower-case o, grave accent			'o'
			STD_DBL_TO_CCFONT     (0x13, 0x27, 0xB6);		//	upper-case O with tilde				'O'
			STD_DBL_TO_CCFONT     (0x13, 0x28, 0xB7);		//	lower-case o with tilde				'o'
			STD_DBL_TO_CCFONT     (0x13, 0x29, 0xBF);		//	opening brace						'{'
			STD_DBL_TO_CCFONT     (0x13, 0x2a, 0xC0);		//	closing brace						'}'
			STD_DBL_TO_CCFONT     (0x13, 0x2b, 0xC5);		//	backslash							'\\'
			STD_DBL_TO_CCFONT     (0x13, 0x2c, 0xC6);		//	caret								'^'
			STD_DBL_TO_CCFONT     (0x13, 0x2d, 0xC7);		//	underbar							'_'
			STD_DBL_TO_CCFONT     (0x13, 0x2e, 'l');		//	pipe								'|'
			STD_DBL_TO_CCFONT     (0x13, 0x2f, 0xC8);		//	tilde								'~'

			//	Extended German/Danish, channels 1 or 3
			STD_DBL_TO_CCFONT     (0x13, 0x30, 0xA2);		//	upper-case A, umlaut				'A'
			STD_DBL_TO_CCFONT     (0x13, 0x31, 0xA7);		//	lower-case a, umlaut				'a'
			STD_DBL_TO_CCFONT     (0x13, 0x32, 0xA5);		//	upper-case O, umlaut				'O'
			STD_DBL_TO_CCFONT     (0x13, 0x33, 0xAA);		//	lower-case o, umlaut				'o'
			STD_DBL_TO_CCFONT     (0x13, 0x34, 0xBE);		//	small sharp s						's'
			STD_DBL_TO_CCFONT     (0x13, 0x35, 0xC9);		//	yen sign							'Y'
			STD_DBL_TO_CCFONT     (0x13, 0x36, 0xCA);		//	non-specific currency sign			'$'
			STD_DBL_TO_CCFONT     (0x13, 0x37, 'l');		//	vertical bar						'|'
			STD_DBL_TO_CCFONT     (0x13, 0x38, 0xB8);		//	upper-case A with ring				'A'
			STD_DBL_TO_CCFONT     (0x13, 0x39, 0xB9);		//	lower-case a with ring				'a'
			STD_DBL_TO_CCFONT     (0x13, 0x3a, 0xBC);		//	upper-case O with stroke			'O'
			STD_DBL_TO_CCFONT     (0x13, 0x3b, 0xBD);		//	lower-case o with stroke			'o'
			STD_DBL_TO_CCFONT     (0x13, 0x3c, 0xCB);		//	upper-left corner					'F'
			STD_DBL_TO_CCFONT     (0x13, 0x3d, 0xCC);		//	upper-right corner					'T'
			STD_DBL_TO_CCFONT     (0x13, 0x3e, 0xCD);		//	lower-left corner					'L'
			STD_DBL_TO_CCFONT     (0x13, 0x3f, 0xCE);		//	lower-right corner					'J'

			//	Extended Portuguese, channels 2 or 4
			STD_DBL_TO_CCFONT     (0x1b, 0x20, 0xB4);		//	upper-case A with tilde				'A'
			STD_DBL_TO_CCFONT     (0x1b, 0x21, 0xB5);		//	lower-case a with tilde				'E'
			STD_DBL_TO_CCFONT     (0x1b, 0x22, 0x95);		//	upper-case I, acute accent			'I'
			STD_DBL_TO_CCFONT     (0x1b, 0x23, 0x9A);		//	upper-case I, grave accent			'I'
			STD_DBL_TO_CCFONT     (0x1b, 0x24, 0x90);		//	lower-case i, grave accent			'i'
			STD_DBL_TO_CCFONT     (0x1b, 0x25, 0x9B);		//	upper-case O, grave accent			'O'
			STD_DBL_TO_CCFONT     (0x1b, 0x26, 0x91);		//	lower-case o, grave accent			'o'
			STD_DBL_TO_CCFONT     (0x1b, 0x27, 0xB6);		//	upper-case O with tilde				'O'
			STD_DBL_TO_CCFONT     (0x1b, 0x28, 0xB7);		//	lower-case o with tilde				'o'
			STD_DBL_TO_CCFONT     (0x1b, 0x29, 0xBF);		//	opening brace						'{'
			STD_DBL_TO_CCFONT     (0x1b, 0x2a, 0xC0);		//	closing brace						'}'
			STD_DBL_TO_CCFONT     (0x1b, 0x2b, 0xC5);		//	backslash							'\\'
			STD_DBL_TO_CCFONT     (0x1b, 0x2c, 0xC6);		//	caret								'^'
			STD_DBL_TO_CCFONT     (0x1b, 0x2d, 0xC7);		//	underbar							'_'
			STD_DBL_TO_CCFONT     (0x1b, 0x2e, 'l');		//	pipe								'|'
			STD_DBL_TO_CCFONT     (0x1b, 0x2f, 0xC8);		//	tilde								'~'

			//	Extended German/Danish, channels 2 or 4
			STD_DBL_TO_CCFONT     (0x1b, 0x30, 0xA2);		//	upper-case A, umlaut				'A'
			STD_DBL_TO_CCFONT     (0x1b, 0x31, 0xA7);		//	lower-case a, umlaut				'a'
			STD_DBL_TO_CCFONT     (0x1b, 0x32, 0xA5);		//	upper-case O, umlaut				'O'
			STD_DBL_TO_CCFONT     (0x1b, 0x33, 0xAA);		//	lower-case o, umlaut				'o'
			STD_DBL_TO_CCFONT     (0x1b, 0x34, 0xBE);		//	small sharp s						's'
			STD_DBL_TO_CCFONT     (0x1b, 0x35, 0xC9);		//	yen sign							'Y'
			STD_DBL_TO_CCFONT     (0x1b, 0x36, 0xCA);		//	non-specific currency sign			'$'
			STD_DBL_TO_CCFONT     (0x1b, 0x37, 'l');		//	vertical bar						'|'
			STD_DBL_TO_CCFONT     (0x1b, 0x38, 0xB8);		//	upper-case A with ring				'A'
			STD_DBL_TO_CCFONT     (0x1b, 0x39, 0xB9);		//	lower-case a with ring				'a'
			STD_DBL_TO_CCFONT     (0x1b, 0x3a, 0xBC);		//	upper-case O with stroke			'O'
			STD_DBL_TO_CCFONT     (0x1b, 0x3b, 0xBD);		//	lower-case o with stroke			'o'
			STD_DBL_TO_CCFONT     (0x1b, 0x3c, 0xCB);		//	upper-left corner					'F'
			STD_DBL_TO_CCFONT     (0x1b, 0x3d, 0xCC);		//	upper-right corner					'T'
			STD_DBL_TO_CCFONT     (0x1b, 0x3e, 0xCD);		//	lower-left corner					'L'
			STD_DBL_TO_CCFONT     (0x1b, 0x3f, 0xCE);		//	lower-right corner					'J'

			//cerr << "## DEBUG:  gCodePointToGlyphIndexMap initialized:" << endl;
			//for (CEA608CodePointToGlyphIndexConstIter it (gCodePointToGlyphIndexMap.begin ());  it != gCodePointToGlyphIndexMap.end ();  ++it)
				//cerr << "0x" << hex << setw (8) << setfill ('0') << it->first << dec << ":  '" << it->second << "'" << endl;
		}	//	constructor

		virtual ~CCFontTableInitializer ()
		{
			//cerr << "## DEBUG:  gCodePointToGlyphIndexMap deinitialized" << endl;
		}
};	//	CCFontTableInitializer


static CCFontTableInitializer	gCCFontTableInitializer;	//	Singleton to initialize the CCFont translation table(s)


UByte NTV2CCFont::GlyphIndexToCharacterCode (const NTV2GlyphIndex inGlyphIndex)
{
	return static_cast <UByte> (inGlyphIndex + NTV2_CCFont_AsciiOffset);
}


NTV2GlyphIndex NTV2CCFont::CharacterCodeToGlyphIndex (const UByte inCharacterCode)
{
	return inCharacterCode - NTV2_CCFont_AsciiOffset;
}


UByte NTV2CCFont::GetCCFontCharCode (const NTV2_CC608_CodePoint in608CodePoint) const
{
	CEA608CodePointToGlyphIndexConstIter iter (gCodePointToGlyphIndexMap.find (in608CodePoint));
	return iter != gCodePointToGlyphIndexMap.end()  ?  UByte(iter->second)  :  0x00;
}


UByte NTV2CCFont::UnicodeCodePointToCharacterCode (const ULWord inCodePoint)
{
	UByte	result (0);

	switch (inCodePoint)
	{
		//	NORTH AMERICAN CHARACTER SET
		case 0x000000E1:	result = 0x2A;	break;	//	lower-case a, acute accent
		case 0x000000E9:	result = 0x5C;	break;	//	lower-case e, acute accent
		case 0x000000ED:	result = 0x5E;	break;	//	lower-case i, acute accent
		case 0x000000F3:	result = 0x5F;	break;	//	lower-case o, acute accent
		case 0x000000FA:	result = 0x60;	break;	//	lower-case u, acute accent
		case 0x000000E7:	result = 0x7B;	break;	//	lower-case c with cedilla
		case 0x000000F7:	result = 0x7C;	break;	//	division sign
		case 0x000000D1:	result = 0x7D;	break;	//	upper-case N with tilde
		case 0x000000F1:	result = 0x7E;	break;	//	lower-case n with tilde
		case 0x00002588:	result = 0x7F;	break;	//	full block

		//	Special North American channel 1 (and 3?) and channel 2 (and 4?)
		case 0x000000AE:	result = 0x80;	break;	//	registered sign
		case 0x000000B0:	result = 0x81;	break;	//	degree sign
		case 0x000000BD:	result = 0x82;	break;	//	1/2 symbol
		case 0x000000BF:	result = 0x83;	break;	//	inverted question mark
		case 0x00002122:	result = 0x84;	break;	//	trade mark sign
		case 0x000000A2:	result = 0x85;	break;	//	cents symbol
		case 0x000000A3:	result = 0x86;	break;	//	pound sterling
		case 0x0000266A:	result = 0x87;	break;	//	eighth note
		case 0x000000E0:	result = 0x88;	break;	//	lower-case a, grave accent
		case 0x000000A0:	result = 0x89;	break;	//	transparent space
		case 0x000000E8:	result = 0x8A;	break;	//	lower-case e, grave accent
		case 0x000000E2:	result = 0x8B;	break;	//	lower-case a, circumflex accent
		case 0x000000EA:	result = 0x8C;	break;	//	lower-case e, circumflex accent
		case 0x000000EE:	result = 0x8D;	break;	//	lower-case i, circumflex accent
		case 0x000000F4:	result = 0x8E;	break;	//	lower-case o, circumflex accent
		case 0x000000FB:	result = 0x8F;	break;	//	lower-case u, circumflex accent

		//	EXTENDED WESTERN EUROPEAN CHARACTER SET

		//	Extended Spanish/Misc, channels 1/3 and 2/4
		case 0x000000C1:	result = 0x93;	break;	//	upper-case A, acute accent			'A'
		case 0x000000C9:	result = 0x94;	break;	//	upper-case E, acute accent			'E'
		case 0x000000D3:	result = 0x96;	break;	//	upper-case O, acute accent			'O'
		case 0x000000DA:	result = 0x97;	break;	//	upper-case U, acute accent			'U'
		case 0x000000DC:	result = 0xA6;	break;	//	upper-case U, umlaut				'U'
		case 0x000000FC:	result = 0xAB;	break;	//	lower-case u, umlaut				'u'
		case 0x00002018:	result = 0xAC;	break;	//	opening single quote				'`'
		case 0x000000A1:	result = 0xB0;	break;	//	inverted exclamation				'!'
		case 0x0000002A:	result = 0xC1;	break;	//	asterisk							'*'
		case 0x00002019:	result = 0xAD;	break;	//	closing single quote				'\''
		case 0x00002014:	result = 0xC3;	break;	//	em dash								'_'
		case 0x000000A9:	result = 0xC4;	break;	//	copyright							'(C)'
		case 0x00002120:	result = 0x80;	break;	//	service mark						'(R)'
		case 0x00002022:	result = 0xC2;	break;	//	round bullet						'o'
		case 0x0000201C:	result = 0xAE;	break;	//	opening double quotes				'"'
		case 0x0000201D:	result = 0xAF;	break;	//	closing double quotes				'"'

		//	Extended French, channels 1/3 and 2/4
		case 0x000000C0:	result = 0x98;	break;	//	upper-case A, grave accent			'A'
		case 0x000000C2:	result = 0x9D;	break;	//	upper-case A, circumflex accent		'A'
		case 0x000000C7:	result = 0xB1;	break;	//	upper-case C with cedilla			'C'
		case 0x000000C8:	result = 0x99;	break;	//	upper-case E, grave accent			'E'
		case 0x000000CA:	result = 0x9E;	break;	//	upper-case E, circumflex accent		'E'
		case 0x000000CB:	result = 0xA3;	break;	//	upper-case E, umlaut				'E'
		case 0x000000EB:	result = 0xA8;	break;	//	lower-case e, umlaut				'e'
		case 0x000000CE:	result = 0x9F;	break;	//	upper-case I, circumflex accent		'I'
		case 0x000000CF:	result = 0xA4;	break;	//	upper-case I, umlaut				'I'
		case 0x000000EF:	result = 0xA9;	break;	//	lower-case i, umlaut				'i'
		case 0x000000D4:	result = 0xA0;	break;	//	upper-case O, circumflex accent		'O'
		case 0x000000D9:	result = 0x9C;	break;	//	upper-case U, grave accent			'U'
		case 0x000000F9:	result = 0x92;	break;	//	lower-case u, grave accent			'u'
		case 0x000000DB:	result = 0xA1;	break;	//	upper-case U, circumflex accent		'U'
		case 0x000000AB:	result = 0xB2;	break;	//	opening guillemet					'<'
		case 0x000000BB:	result = 0xB3;	break;	//	closing guillemet					'>'

		//	Extended Portuguese, channels 1 or 3
		case 0x000000C3:	result = 0xB4;	break;	//	upper-case A with tilde				'A'
		case 0x000000E3:	result = 0xB5;	break;	//	lower-case a with tilde				'E'
		case 0x000000CD:	result = 0x95;	break;	//	upper-case I, acute accent			'I'
		case 0x000000CC:	result = 0x9A;	break;	//	upper-case I, grave accent			'I'
		case 0x000000EC:	result = 0x90;	break;	//	lower-case i, grave accent			'i'
		case 0x000000D2:	result = 0x9B;	break;	//	upper-case O, grave accent			'O'
		case 0x000000F2:	result = 0x91;	break;	//	lower-case o, grave accent			'o'
		case 0x000000D5:	result = 0xB6;	break;	//	upper-case O with tilde				'O'
		case 0x000000F5:	result = 0xB7;	break;	//	lower-case o with tilde				'o'
		case 0x0000007B:	result = 0xBF;	break;	//	opening brace						'{'
		case 0x0000007D:	result = 0xC0;	break;	//	closing brace						'}'
		case 0x0000005C:	result = 0xC5;	break;	//	backslash							'\\'
		case 0x0000005E:	result = 0xC6;	break;	//	caret								'^'
		case 0x0000005F:	result = 0xC7;	break;	//	underbar							'_'
		case 0x0000007C:	result = 0x6C;	break;	//	pipe								'|'
		case 0x0000007E:	result = 0xC8;	break;	//	tilde								'~'

		//	Extended German/Danish, channels 1 or 3
		case 0x000000C4:	result = 0xA2;	break;	//	upper-case A, umlaut				'A'
		case 0x000000E4:	result = 0xA7;	break;	//	lower-case a, umlaut				'a'
		case 0x000000D6:	result = 0xA5;	break;	//	upper-case O, umlaut				'O'
		case 0x000000F6:	result = 0xAA;	break;	//	lower-case o, umlaut				'o'
		case 0x000000DF:	result = 0xBE;	break;	//	small sharp s						's'
		case 0x000000A5:	result = 0xC9;	break;	//	yen sign							'Y'
		case 0x000000A4:	result = 0xCA;	break;	//	non-specific currency sign			'$'
		//case 0x0000007C:	result = 0x6C;	break;	//	vertical bar						'|'
		case 0x000000C5:	result = 0xB8;	break;	//	upper-case A with ring				'A'
		case 0x000000E5:	result = 0xB9;	break;	//	lower-case a with ring				'a'
		case 0x000000D8:	result = 0xBC;	break;	//	upper-case O with stroke			'O'
		case 0x000000F8:	result = 0xBD;	break;	//	lower-case o with stroke			'o'
		case 0x0000250C:	result = 0xCB;	break;	//	upper-left corner					'F'
		case 0x00002510:	result = 0xCC;	break;	//	upper-right corner					'T'
		case 0x00002514:	result = 0xCD;	break;	//	lower-left corner					'L'
		case 0x00002518:	result = 0xCE;	break;	//	lower-right corner					'J'

		default:
			if (inCodePoint > 0x0000001F && inCodePoint < 0x0000007F)
				result = UByte (inCodePoint & 0x000000FF);
	}
	return result;

}	//	UnicodeCodePointToCharacterCode


string NTV2CCFont::Utf8ToCCFontByteArray (const string & inUtf8Str)
{
	UByte		rawBytes [5]		= {0, 0, 0, 0, 0};
	bool		finished			(false);
	unsigned	charPos				(0);
	ULWord		unicodeCodePoint	(0);
	string		resultStr;

	while (!finished && charPos < inUtf8Str.length ())
	{
		rawBytes [0] = UByte (inUtf8Str.at (charPos++));
		if (charPos >= inUtf8Str.length ())
			finished = true;
		if (rawBytes [0] < 0x80)
		{
			unicodeCodePoint = ULWord (rawBytes [0]);
			resultStr += string (1, char (UnicodeCodePointToCharacterCode (unicodeCodePoint)));
		}
		else if (rawBytes [0] < 0xC2)
			cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Continuation or overlong 2-byte sequence at character position " << (charPos-1) << endl;
		else if (rawBytes [0] < 0xE0)
		{
			//	2-byte sequence
			if (finished)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  EOF before reading 2nd byte 2-byte UTF8 character" << endl;	break;}
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [1] & 0xC0) != 0x80)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Upper two bits of 2nd byte of 2-byte UTF8 character not 0x80, instead got "
						<< xHEX0N(uint16_t(rawBytes[1] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			unicodeCodePoint = ULWord (rawBytes [0] << 6)  +  ULWord (rawBytes [1])  -  0x3080;
			resultStr += char (UnicodeCodePointToCharacterCode (unicodeCodePoint));
		}
		else if (rawBytes [0] < 0xF0)
		{
			//	3-byte sequence
			if (finished)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  EOF before reading 2nd byte of 3-byte UTF8 character" << endl;	break;}
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  EOF before reading 3rd byte of 3-byte UTF8 character" << endl;	break;}
			if ((rawBytes [1] & 0xC0) != 0x80)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Upper two bits of 2nd byte of 3-byte UTF8 character not 0x80, instead got "
						<< xHEX0N(uint16_t(rawBytes[1] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			if (rawBytes [0] == 0xE0 && rawBytes [1] < 0xA0)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Overlong condition at offset " << (charPos-1) << ":  " << xHEX0N(uint16_t(rawBytes[0]),2)
						<< ", " << xHEX0N(uint16_t(rawBytes[1]),2) << endl;	continue;}
			rawBytes [2] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [2] & 0xC0) != 0x80)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Upper two bits of 3rd byte of 3-byte UTF8 character not 0x80, instead got "
						<< xHEX0N(uint16_t(rawBytes[2] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			unicodeCodePoint = ULWord (rawBytes [0] << 12)  +  ULWord (rawBytes [1] << 6)  +  ULWord (rawBytes [2])  -  0xE2080;
			resultStr += char (UnicodeCodePointToCharacterCode (unicodeCodePoint));
		}
		else if (rawBytes [0] < 0xF5)
		{
			//	4-byte sequence */
			if (finished)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  EOF before reading 2nd byte of 4-byte UTF8 character" << endl;	break;}
			rawBytes [1] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  EOF before reading 3rd byte of 4-byte UTF8 character" << endl;	break;}
			if ((rawBytes [1] & 0xC0) != 0x80)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Upper two bits of 2nd byte of 4-byte UTF8 character not 0x80, instead got "
						<< xHEX0N(uint16_t(rawBytes[1] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			if (rawBytes [0] == 0xF0 && rawBytes [1] < 0x90)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Overlong condition at offset " << (charPos-1) << ":  " << xHEX0N(uint16_t(rawBytes[0]),2)
						<< ", " << xHEX0N(uint16_t(rawBytes[1]),2) << endl;	continue;}
			if (rawBytes [0] == 0xF4 && rawBytes [1] >= 0x90)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Unicode codepoint exceeds U+10FFFF at offset " << (charPos-1) << " in 4-byte sequence:  "
						<< xHEX0N(uint16_t(rawBytes[0]),2) << ", " << xHEX0N(uint16_t(rawBytes[1]),2) << endl;	continue;}
			rawBytes [2] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				{cerr << "## WARNING:  Utf8ToCCFontByteArray:  EOF before reading 4th byte of 4-byte UTF8 character" << endl;	break;}
			if ((rawBytes [2] & 0xC0) != 0x80)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Upper two bits of 3rd byte of 4-byte UTF8 character not 0x80, instead got "
						<< xHEX0N(uint16_t(rawBytes[2] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			rawBytes [3] = UByte (inUtf8Str.at (charPos++));
			if (charPos >= inUtf8Str.length ())
				finished = true;
			if ((rawBytes [3] & 0xC0) != 0x80)
				{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Upper two bits of 4th byte of 4-byte UTF8 character not 0x80, instead got "
						<< xHEX0N(uint16_t(rawBytes[3] & 0xC0),2) << ", at offset " << (charPos-1) << endl;	continue;}
			unicodeCodePoint = ULWord (rawBytes [0] << 18)  +  ULWord (rawBytes [1] << 12)  +  ULWord (rawBytes [2] << 6)  +  ULWord (rawBytes [3])  -  0x3C82080;
			resultStr += char (UnicodeCodePointToCharacterCode (unicodeCodePoint));
		}
		else
			{cerr	<< "## WARNING:  Utf8ToCCFontByteArray:  Unicode codepoint exceeds U+10FFFF at offset " << (charPos-1) << ", "
					<< xHEX0N(uint16_t(rawBytes[0]),2) << endl;	continue;}

	}	//	loop til no more characters

	return resultStr;

}	//	Utf8ToCCFontByteArray


UWord *	NTV2CCFont::GetGlyphFor608CodePoint (const NTV2_CC608_CodePoint in608CodePoint) const
{
	UWord *									pResult	(NULL);
	CEA608CodePointToGlyphIndexConstIter	iter	(gCodePointToGlyphIndexMap.find (in608CodePoint));

	if (iter != gCodePointToGlyphIndexMap.end ())
	{
		const NTV2GlyphIndex	ccFontCharCode	(iter->second);
		AJACC_ASSERT (ccFontCharCode < NTV2_CCFont_NumChars);
		pResult = &gCCFontDotMap [ccFontCharCode][0];
	}
	return pResult;
}


bool NTV2CCFont::HasGlyphFor608CodePoint (const NTV2_CC608_CodePoint in608CodePoint) const
{
	return GetGlyphFor608CodePoint (in608CodePoint) ? true : false;
}


UWord NTV2CCFont::GetGlyphRowDots (const NTV2GlyphIndex inGlyphIndex, const unsigned inDotMapRow) const
{
	UWord	result	(0);
	AJACC_ASSERT (inGlyphIndex < GetGlyphCount ());
	AJACC_ASSERT (inDotMapRow < GetDotMapRowCount ());
	result = gCCFontDotMap [inGlyphIndex][inDotMapRow];
	return result;
}


NTV2CodePointSet NTV2CCFont::GetCodePointsForCCFontCharCode (const UByte inCharCode) const
{
	NTV2CodePointSet	result;
	for (CEA608CodePointToGlyphIndexConstIter iter (gCodePointToGlyphIndexMap.begin ());  iter != gCodePointToGlyphIndexMap.end ();  ++iter)
	{
		const NTV2_CC608_CodePoint	codePoint	(iter->first);
		const NTV2GlyphIndex		charCode	(iter->second);
		if (charCode == inCharCode)
			result.insert (codePoint);
	}
	return result;
}


static NTV2CCFont	gCCFont;


const NTV2CCFont &	NTV2CCFont::GetInstance (void)		{return gCCFont;}


NTV2CCFont::NTV2CCFont ()
	:	mGlyphCount					(NTV2_CCFont_NumChars),
		mUnderlineGlyphIndex		(NTV2_CCFont_UnderlineChar),
		mNoUnderlineSpaceGlyphIndex	(NTV2_CCFont_NoUnderlineSpaceChar),
		mDotMapWidth				(NTV2_CCFont_BitmapCharWidth),
		mLeftMarginDotCount			(NTV2_CCFont_CharDotLeftSpace),
		mRightMarginDotCount		(NTV2_CCFont_CharDotRightSpace),
		mDotMapHeight				(NTV2_CCFont_BitmapCharHeight),
		mTopMarginDotCount			(NTV2_CCFont_CharDotTopSpace),
		mBottomMarginDotCount		(NTV2_CCFont_CharDotBottomSpace),
		mFontName					("CCFontNormal")
{
	(void) NTV2_CCFont_CharDotWidth;
	(void) NTV2_CCFont_CharDotHeight;
	(void) NTV2_CCFont_CharUnderline1;
	(void) NTV2_CCFont_CharUnderline2;

}	//	constructor


NTV2CCFont::~NTV2CCFont ()
{
}	//	destructor


string NTV2CCFont::GetGlyphRowDotsAsString (const NTV2GlyphIndex inGlyphIndex, const unsigned inRow) const
{
	const UWord			rowDots		(GetGlyphRowDots (inGlyphIndex, inRow));
	const string		fullBlock	("\xE2\x96\x88");
	const string		space		(" ");
	ostringstream		oss;

	for (unsigned bitCount(0);  bitCount < GetDotMapWidth();  bitCount++)
		oss << ((rowDots & (1 << (GetDotMapWidth() - bitCount - 1))) ? fullBlock : space);

	return oss.str();
}


static string CopyString (const unsigned inCopies, const string & inStringToCopy)
{
	ostringstream	oss;
	for (unsigned nCopy(0);  nCopy < inCopies;  nCopy++)
		oss << inStringToCopy;
	return oss.str();
}

typedef enum		_BoxConstants	{				kTopLeft,		kTopRight,		kBotLeft,		kBotRight,		kVertLine,		kHorzLine,		kHorizLine = kHorzLine,	NBoxConstants}	BoxConstants;
static const string	gThinBox [NBoxConstants]	= {	"\xE2\x94\x8C",	"\xE2\x94\x90",	"\xE2\x94\x94",	"\xE2\x94\x98",	"\xE2\x94\x82",	"\xE2\x94\x80"};


ostream & NTV2CCFont::PrintGlyphs (ostream & inOutStream, const NTV2GlyphIndex inFirstGlyph, const NTV2GlyphIndex inLastGlyph) const
{
	const string		spaceBetween	(CopyString (5, " "));
	if (inFirstGlyph < GetGlyphCount() && inLastGlyph < GetGlyphCount())
	{
		NTV2GlyphIndex	charOffset	(0);

		for (charOffset = inFirstGlyph;  charOffset <= inLastGlyph;  charOffset++)
			inOutStream << spaceBetween << gThinBox[kTopLeft] << CopyString (GetDotMapWidth(), gThinBox[kHorzLine]) << gThinBox[kTopRight];
		inOutStream << endl;
		for (unsigned dotRow(0);  dotRow < GetDotMapHeight();  dotRow++)
		{
			for (charOffset = inFirstGlyph;  charOffset <= inLastGlyph;  charOffset++)
				inOutStream << spaceBetween << gThinBox[kVertLine] << GetGlyphRowDotsAsString (charOffset, dotRow) << gThinBox[kVertLine];
			inOutStream << endl;
		}	//	for each dot row
		for (charOffset = inFirstGlyph;  charOffset <= inLastGlyph;  charOffset++)
			inOutStream << spaceBetween << gThinBox[kBotLeft] << CopyString (GetDotMapWidth(), gThinBox[kHorzLine]) << gThinBox[kBotRight];
		inOutStream << endl;
		for (charOffset = inFirstGlyph;  charOffset <= inLastGlyph;  charOffset++)
		{
			const NTV2CodePointSet		codePoints		(GetCodePointsForCCFontCharCode(static_cast <UByte> (charOffset + 0x20)));
			const NTV2_CC608_CodePoint	firstCodePoint	(codePoints.begin() != codePoints.end() ? *(codePoints.begin()) : 0);
			const string				utf8char		(::NTV2CC608CodePointToUtf8String(firstCodePoint));
			inOutStream	<< spaceBetween << " " << hex << setw(2) << setfill('0') << unsigned(charOffset) << " " << unsigned(charOffset + 0x20) << " "
						<< hex << setw(8) << setfill('0') << firstCodePoint << dec << " " << utf8char << (utf8char.empty() ? " " : "") << " ";
		}
		inOutStream << endl;
	}
	return inOutStream;

}	//	PrintGlyphs


//
//	Map CEA-608 byte codes to UTF-8
//	See	http://en.wikipedia.org/wiki/EIA-608
//

typedef	map <NTV2_CC608_CodePoint, string>				CEA608CodePointToUtf8StringMap;
typedef	pair <NTV2_CC608_CodePoint, string>				CEA608CodePointToUtfStringPair;
typedef CEA608CodePointToUtf8StringMap::const_iterator	CEA608CodePointToUtf8StringConstIter;

typedef	map <NTV2_CC608_CodePoint, UWord>				CEA608CodePointToUtf16CharMap;
typedef	pair <NTV2_CC608_CodePoint, UWord>				CEA608CodePointToUtf16CharPair;
typedef CEA608CodePointToUtf16CharMap::const_iterator	CEA608CodePointToUtf16CharConstIter;


static CEA608CodePointToUtf8StringMap					gCEA608CodePointToUtf8StringMap;
static CEA608CodePointToUtf16CharMap					gCEA608CodePointToUtf16CharMap;


#define	DBLSZ_CODEPOINT(_608_1, _608_2)					CEA608_CODEPOINT (NTV2_CC608_DoubleSizeCharacterSet, (_608_1), (_608_2))
#define	STD_TO_UTF8(_608_1, _608_2, _utf8_)				gCEA608CodePointToUtf8StringMap.insert (CEA608CodePointToUtfStringPair (STD_CODEPOINT ((_608_1), (_608_2)),		\
																																string (_utf8_)));
#define	DBL_TO_UTF8(_608_1, _608_2, _utf8_)				gCEA608CodePointToUtf8StringMap.insert (CEA608CodePointToUtfStringPair (DBLSZ_CODEPOINT ((_608_1), (_608_2)),	\
																																string (_utf8_)));
#define	STD_DBL_TO_UTF8(_608_1, _608_2, _utf8_)			STD_TO_UTF8 ((_608_1), (_608_2), (_utf8_));	DBL_TO_UTF8 ((_608_1), (_608_2), (_utf8_));


class Utf8TableInitializer
{
	public:
		Utf8TableInitializer ()
		{
			//	NORTH AMERICAN CHARACTER SET
			STD_DBL_TO_UTF8 (0x2A, 0x00, "\xC3\xA1");		//	U+00E1	lower-case a, acute accent
			STD_DBL_TO_UTF8 (0x5C, 0x00, "\xC3\xA9");		//	U+00E9	lower-case e, acute accent
			STD_DBL_TO_UTF8 (0x5E, 0x00, "\xC3\xAD");		//	U+00ED	lower-case i, acute accent
			STD_DBL_TO_UTF8 (0x5F, 0x00, "\xC3\xB3");		//	U+00F3	lower-case o, acute accent
			STD_DBL_TO_UTF8 (0x60, 0x00, "\xC3\xBA");		//	U+00FA	lower-case u, acute accent
			STD_DBL_TO_UTF8 (0x7B, 0x00, "\xC3\xA7");		//	U+00E7	lower-case c with cedilla
			STD_DBL_TO_UTF8 (0x7C, 0x00, "\xC3\xB7");		//	U+00F7	division sign
			STD_DBL_TO_UTF8 (0x7D, 0x00, "\xC3\x91");		//	U+00D1	upper-case N with tilde
			STD_DBL_TO_UTF8 (0x7E, 0x00, "\xC3\xB1");		//	U+00F1	lower-case n with tilde
			STD_DBL_TO_UTF8 (0x7F, 0x00, "\xE2\x96\x88");	//	U+2588	full block
			//	And the rest of the regular ASCII characters...
			for (UByte ascii (0x20);  ascii < 0x7B;  ascii++)
			{
				if (gCEA608CodePointToUtf8StringMap.find (STD_CODEPOINT (ascii, 0x00)) == gCEA608CodePointToUtf8StringMap.end ())
					STD_DBL_TO_UTF8 (ascii, 0x00, string (1, static_cast <char> (ascii)));
			}

			//	Special North American (channel 1)
			STD_DBL_TO_UTF8 (0x11, 0x30, "\xC2\xAE");		//	U+00AE	registered sign
			STD_DBL_TO_UTF8 (0x11, 0x31, "\xC2\xB0");		//	U+00B0	degree sign
			STD_DBL_TO_UTF8 (0x11, 0x32, "\xC2\xBD");		//	U+00BD	1/2 symbol
			STD_DBL_TO_UTF8 (0x11, 0x33, "\xC2\xBF");		//	U+00BF	inverted question mark
			STD_DBL_TO_UTF8 (0x11, 0x34, "\xE2\x84\xA2");	//	U+2122	trade mark sign
			STD_DBL_TO_UTF8 (0x11, 0x35, "\xC2\xA2");		//	U+00A2	cents symbol
			STD_DBL_TO_UTF8 (0x11, 0x36, "\xC2\xA3");		//	U+00A3	pound sterling
			STD_DBL_TO_UTF8 (0x11, 0x37, "\xE2\x99\xAA");	//	U+266A	eighth note
			STD_DBL_TO_UTF8 (0x11, 0x38, "\xC3\xA0");		//	U+00E0	lower-case a, grave accent
			STD_DBL_TO_UTF8 (0x11, 0x39, "\xC2\xA0");		//	U+00A0	transparent space
			STD_DBL_TO_UTF8 (0x11, 0x3A, "\xC3\xA8");		//	U+00E8	lower-case e, grave accent
			STD_DBL_TO_UTF8 (0x11, 0x3B, "\xC3\xA2");		//	U+00E2	lower-case a, circumflex accent
			STD_DBL_TO_UTF8 (0x11, 0x3C, "\xC3\xAA");		//	U+00EA	lower-case e, circumflex accent
			STD_DBL_TO_UTF8 (0x11, 0x3D, "\xC3\xAE");		//	U+00EE	lower-case i, circumflex accent
			STD_DBL_TO_UTF8 (0x11, 0x3E, "\xC3\xB4");		//	U+00F4	lower-case o, circumflex accent
			STD_DBL_TO_UTF8 (0x11, 0x3F, "\xC3\xBB");		//	U+00FB	lower-case u, circumflex accent

			//	Special North American (channel 2)
			STD_DBL_TO_UTF8 (0x19, 0x30, "\xC2\xAE");		//	U+00AE	registered sign
			STD_DBL_TO_UTF8 (0x19, 0x31, "\xC2\xB0");		//	U+00B0	degree sign
			STD_DBL_TO_UTF8 (0x19, 0x32, "\xC2\xBD");		//	U+00BD	1/2 symbol
			STD_DBL_TO_UTF8 (0x19, 0x33, "\xC2\xBF");		//	U+00BF	inverted question mark
			STD_DBL_TO_UTF8 (0x19, 0x34, "\xE2\x84\xA2");	//	U+2122	trade mark sign
			STD_DBL_TO_UTF8 (0x19, 0x35, "\xC2\xA2");		//	U+00A2	cents symbol
			STD_DBL_TO_UTF8 (0x19, 0x36, "\xC2\xA3");		//	U+00A3	pound sterling
			STD_DBL_TO_UTF8 (0x19, 0x37, "\xE2\x99\xAA");	//	U+266A	eighth note
			STD_DBL_TO_UTF8 (0x19, 0x38, "\xC3\xA0");		//	U+00E0	lower-case a, grave accent
			STD_DBL_TO_UTF8 (0x19, 0x39, "\xC2\xA0");		//	U+00A0	transparent space
			STD_DBL_TO_UTF8 (0x19, 0x3A, "\xC3\xA8");		//	U+00E8	lower-case e, grave accent
			STD_DBL_TO_UTF8 (0x19, 0x3B, "\xC3\xA2");		//	U+00E2	lower-case a, circumflex accent
			STD_DBL_TO_UTF8 (0x19, 0x3C, "\xC3\xAA");		//	U+00EA	lower-case e, circumflex accent
			STD_DBL_TO_UTF8 (0x19, 0x3D, "\xC3\xAE");		//	U+00EE	lower-case i, circumflex accent
			STD_DBL_TO_UTF8 (0x19, 0x3E, "\xC3\xB4");		//	U+00F4	lower-case o, circumflex accent
			STD_DBL_TO_UTF8 (0x19, 0x3F, "\xC3\xBB");		//	U+00FB	lower-case u, circumflex accent

			//	EXTENDED WESTERN EUROPEAN CHARACTER SET

			//	Extended Spanish/Misc, channel 1 or 3
			STD_DBL_TO_UTF8 (0x12, 0x20, "\xC3\x81");		//	U+00C1	upper-case A, acute accent			'A'
			STD_DBL_TO_UTF8 (0x12, 0x21, "\xC3\x89");		//	U+00C9	upper-case E, acute accent			'E'
			STD_DBL_TO_UTF8 (0x12, 0x22, "\xC3\x93");		//	U+00D3	upper-case O, acute accent			'O'
			STD_DBL_TO_UTF8 (0x12, 0x23, "\xC3\x9A");		//	U+00DA	upper-case U, acute accent			'U'
			STD_DBL_TO_UTF8 (0x12, 0x24, "\xC3\x9C");		//	U+00DC	upper-case U, umlaut				'U'
			STD_DBL_TO_UTF8 (0x12, 0x25, "\xC3\xBC");		//	U+00FC	lower-case u, umlaut				'u'
			STD_DBL_TO_UTF8 (0x12, 0x26, "\xE2\x80\x98");	//	U+2018	opening single quote				'`'
			STD_DBL_TO_UTF8 (0x12, 0x27, "\xC2\xA1");		//	U+00A1	inverted exclamation				'!'
			STD_DBL_TO_UTF8 (0x12, 0x28, "*");				//	U+002A	asterisk							'*'
			STD_DBL_TO_UTF8 (0x12, 0x29, "\xE2\x80\x99");	//	U+2019	closing single quote				'\''
			STD_DBL_TO_UTF8 (0x12, 0x2a, "\xE2\x80\x94");	//	U+2014	em dash								'_'
			STD_DBL_TO_UTF8 (0x12, 0x2b, "\xC2\xA9");		//	U+00A9	copyright							'(C)'
			STD_DBL_TO_UTF8 (0x12, 0x2c, "\xC2\xAE");		//	U+2120	service mark						'(R)'
			STD_DBL_TO_UTF8 (0x12, 0x2d, "\xE2\x80\xA2");	//	U+2022	round bullet						'o'
			STD_DBL_TO_UTF8 (0x12, 0x2e, "\xE2\x80\x9C");	//	U+201C	opening double quotes				'"'
			STD_DBL_TO_UTF8 (0x12, 0x2f, "\xE2\x80\x9D");	//	U+201D	closing double quotes				'"'

			//	Extended Spanish/Misc, channel 2 or 4
			STD_DBL_TO_UTF8 (0x1a, 0x20, "\xC3\x81");		//	U+00C1	upper-case A, acute accent			'A'
			STD_DBL_TO_UTF8 (0x1a, 0x21, "\xC3\x89");		//	U+00C9	upper-case E, acute accent			'E'
			STD_DBL_TO_UTF8 (0x1a, 0x22, "\xC3\x93");		//	U+00D3	upper-case O, acute accent			'O'
			STD_DBL_TO_UTF8 (0x1a, 0x23, "\xC3\x9A");		//	U+00DA	upper-case U, acute accent			'U'
			STD_DBL_TO_UTF8 (0x1a, 0x24, "\xC3\x9C");		//	U+00DC	upper-case U, umlaut				'U'
			STD_DBL_TO_UTF8 (0x1a, 0x25, "\xC3\xBC");		//	U+00FC	lower-case u, umlaut				'u'
			STD_DBL_TO_UTF8 (0x1a, 0x26, "\xE2\x80\x98");	//	U+2018	opening single quote				'`'
			STD_DBL_TO_UTF8 (0x1a, 0x27, "\xC2\xA1");		//	U+00A1	inverted exclamation				'!'
			STD_DBL_TO_UTF8 (0x1a, 0x28, "*");				//	U+002A	asterisk							'*'
			STD_DBL_TO_UTF8 (0x1a, 0x29, "\xE2\x80\x99");	//	U+2019	closing single quote				'\''
			STD_DBL_TO_UTF8 (0x1a, 0x2a, "\xE2\x80\x94");	//	U+2014	em dash								'_'
			STD_DBL_TO_UTF8 (0x1a, 0x2b, "\xC2\xA9");		//	U+00A9	copyright							'(C)'
			STD_DBL_TO_UTF8 (0x1a, 0x2c, "\xC2\xAE");		//	U+2120	service mark						'(R)'
			STD_DBL_TO_UTF8 (0x1a, 0x2d, "\xE2\x80\xA2");	//	U+2022	round bullet						'o'
			STD_DBL_TO_UTF8 (0x1a, 0x2e, "\xE2\x80\x9C");	//	U+201C	opening double quotes				'"'
			STD_DBL_TO_UTF8 (0x1a, 0x2f, "\xE2\x80\x9D");	//	U+201D	closing double quotes				'"'

			//	Extended French, channel 1 or 3
			STD_DBL_TO_UTF8 (0x12, 0x30, "\xC3\x80");		//	U+00C0	upper-case A, grave accent			'A'
			STD_DBL_TO_UTF8 (0x12, 0x31, "\xC3\x82");		//	U+00C2	upper-case A, circumflex accent		'A'
			STD_DBL_TO_UTF8 (0x12, 0x32, "\xC3\x87");		//	U+00C7	upper-case C with cedilla			'C'
			STD_DBL_TO_UTF8 (0x12, 0x33, "\xC3\x88");		//	U+00C8	upper-case E, grave accent			'E'
			STD_DBL_TO_UTF8 (0x12, 0x34, "\xC3\x8A");		//	U+00CA	upper-case E, circumflex accent		'E'
			STD_DBL_TO_UTF8 (0x12, 0x35, "\xC3\x8B");		//	U+00CB	upper-case E, umlaut				'E'
			STD_DBL_TO_UTF8 (0x12, 0x36, "\xC3\xAB");		//	U+00EB	lower-case e, umlaut				'e'
			STD_DBL_TO_UTF8 (0x12, 0x37, "\xC3\x8E");		//	U+00CE	upper-case I, circumflex accent		'I'
			STD_DBL_TO_UTF8 (0x12, 0x38, "\xC3\x8F");		//	U+00CF	upper-case I, umlaut				'I'
			STD_DBL_TO_UTF8 (0x12, 0x39, "\xC3\xAF");		//	U+00EF	lower-case i, umlaut				'i'
			STD_DBL_TO_UTF8 (0x12, 0x3a, "\xC3\x94");		//	U+00D4	upper-case O, circumflex accent		'O'
			STD_DBL_TO_UTF8 (0x12, 0x3b, "\xC3\x99");		//	U+00D9	upper-case U, grave accent			'U'
			STD_DBL_TO_UTF8 (0x12, 0x3c, "\xC3\xB9");		//	U+00F9	lower-case u, grave accent			'u'
			STD_DBL_TO_UTF8 (0x12, 0x3d, "\xC3\x9B");		//	U+00DB	upper-case U, circumflex accent		'U'
			STD_DBL_TO_UTF8 (0x12, 0x3e, "\xC2\xAB");		//	U+00AB	opening guillemet					'<'
			STD_DBL_TO_UTF8 (0x12, 0x3f, "\xC2\xBB");		//	U+00BB	closing guillemet					'>'

			//	Extended French, channel 2 or 4
			STD_DBL_TO_UTF8 (0x1a, 0x30, "\xC3\x80");		//	U+00C0	upper-case A, grave accent			'A'
			STD_DBL_TO_UTF8 (0x1a, 0x31, "\xC3\x82");		//	U+00C2	upper-case A, circumflex accent		'A'
			STD_DBL_TO_UTF8 (0x1a, 0x32, "\xC3\x87");		//	U+00C7	upper-case C with cedilla			'C'
			STD_DBL_TO_UTF8 (0x1a, 0x33, "\xC3\x88");		//	U+00C8	upper-case E, grave accent			'E'
			STD_DBL_TO_UTF8 (0x1a, 0x34, "\xC3\x8A");		//	U+00CA	upper-case E, circumflex accent		'E'
			STD_DBL_TO_UTF8 (0x1a, 0x35, "\xC3\x8B");		//	U+00CB	upper-case E, umlaut				'E'
			STD_DBL_TO_UTF8 (0x1a, 0x36, "\xC3\xAB");		//	U+00EB	lower-case e, umlaut				'e'
			STD_DBL_TO_UTF8 (0x1a, 0x37, "\xC3\x8E");		//	U+00CE	upper-case I, circumflex accent		'I'
			STD_DBL_TO_UTF8 (0x1a, 0x38, "\xC3\x8F");		//	U+00CF	upper-case I, umlaut				'I'
			STD_DBL_TO_UTF8 (0x1a, 0x39, "\xC3\xAF");		//	U+00EF	lower-case i, umlaut				'i'
			STD_DBL_TO_UTF8 (0x1a, 0x3a, "\xC3\x94");		//	U+00D4	upper-case O, circumflex accent		'O'
			STD_DBL_TO_UTF8 (0x1a, 0x3b, "\xC3\x99");		//	U+00D9	upper-case U, grave accent			'U'
			STD_DBL_TO_UTF8 (0x1a, 0x3c, "\xC3\xB9");		//	U+00F9	lower-case u, grave accent			'u'
			STD_DBL_TO_UTF8 (0x1a, 0x3d, "\xC3\x9B");		//	U+00DB	upper-case U, circumflex accent		'U'
			STD_DBL_TO_UTF8 (0x1a, 0x3e, "\xC2\xAB");		//	U+00AB	opening guillemet					'<'
			STD_DBL_TO_UTF8 (0x1a, 0x3f, "\xC2\xBB");		//	U+00BB	closing guillemet					'>'

			//	Extended Portuguese, channels 1 or 3
			STD_DBL_TO_UTF8 (0x13, 0x20, "\xC3\x83");		//	U+00C3	upper-case A with tilde				'A'
			STD_DBL_TO_UTF8 (0x13, 0x21, "\xC3\xA3");		//	U+00E3	lower-case a with tilde				'E'
			STD_DBL_TO_UTF8 (0x13, 0x22, "\xC3\x8D");		//	U+00CD	upper-case I, acute accent			'I'
			STD_DBL_TO_UTF8 (0x13, 0x23, "\xC3\x8C");		//	U+00CC	upper-case I, grave accent			'I'
			STD_DBL_TO_UTF8 (0x13, 0x24, "\xC3\xAC");		//	U+00EC	lower-case i, grave accent			'i'
			STD_DBL_TO_UTF8 (0x13, 0x25, "\xC3\x92");		//	U+00D2	upper-case O, grave accent			'O'
			STD_DBL_TO_UTF8 (0x13, 0x26, "\xC3\xB2");		//	U+00F2	lower-case o, grave accent			'o'
			STD_DBL_TO_UTF8 (0x13, 0x27, "\xC3\x95");		//	U+00D5	upper-case O with tilde				'O'
			STD_DBL_TO_UTF8 (0x13, 0x28, "\xC3\xB5");		//	U+00F5	lower-case o with tilde				'o'
			STD_DBL_TO_UTF8 (0x13, 0x29, "{");				//	U+007B	opening brace						'{'
			STD_DBL_TO_UTF8 (0x13, 0x2a, "}");				//	U+007D	closing brace						'}'
			STD_DBL_TO_UTF8 (0x13, 0x2b, "\\");				//	U+005C	backslash							'\\'
			STD_DBL_TO_UTF8 (0x13, 0x2c, "^");				//	U+005E	caret								'^'
			STD_DBL_TO_UTF8 (0x13, 0x2d, "_");				//	U+005F	underbar							'_'
			STD_DBL_TO_UTF8 (0x13, 0x2e, "|");				//	U+007C	pipe								'|'
			STD_DBL_TO_UTF8 (0x13, 0x2f, "~");				//	U+007E	tilde								'~'

			//	Extended Portuguese, channels 2 or 4
			STD_DBL_TO_UTF8 (0x1b, 0x20, "\xC3\x83");		//	U+00C3	upper-case A with tilde				'A'
			STD_DBL_TO_UTF8 (0x1b, 0x21, "\xC3\xA3");		//	U+00E3	lower-case a with tilde				'E'
			STD_DBL_TO_UTF8 (0x1b, 0x22, "\xC3\x8D");		//	U+00CD	upper-case I, acute accent			'I'
			STD_DBL_TO_UTF8 (0x1b, 0x23, "\xC3\x8C");		//	U+00CC	upper-case I, grave accent			'I'
			STD_DBL_TO_UTF8 (0x1b, 0x24, "\xC3\xAC");		//	U+00EC	lower-case i, grave accent			'i'
			STD_DBL_TO_UTF8 (0x1b, 0x25, "\xC3\x92");		//	U+00D2	upper-case O, grave accent			'O'
			STD_DBL_TO_UTF8 (0x1b, 0x26, "\xC3\xB2");		//	U+00F2	lower-case o, grave accent			'o'
			STD_DBL_TO_UTF8 (0x1b, 0x27, "\xC3\x95");		//	U+00D5	upper-case O with tilde				'O'
			STD_DBL_TO_UTF8 (0x1b, 0x28, "\xC3\xB5");		//	U+00F5	lower-case o with tilde				'o'
			STD_DBL_TO_UTF8 (0x1b, 0x29, "{");				//	U+007B	opening brace						'{'
			STD_DBL_TO_UTF8 (0x1b, 0x2a, "}");				//	U+007D	closing brace						'}'
			STD_DBL_TO_UTF8 (0x1b, 0x2b, "\\");				//	U+005C	backslash							'\\'
			STD_DBL_TO_UTF8 (0x1b, 0x2c, "^");				//	U+005E	caret								'^'
			STD_DBL_TO_UTF8 (0x1b, 0x2d, "_");				//	U+005F	underbar							'_'
			STD_DBL_TO_UTF8 (0x1b, 0x2e, "|");				//	U+007C	pipe								'|'
			STD_DBL_TO_UTF8 (0x1b, 0x2f, "~");				//	U+007E	tilde								'~'

			//	Extended German/Danish, channels 1 or 3
			STD_DBL_TO_UTF8 (0x13, 0x30, "\xC3\x84");		//	U+00C4	upper-case A, umlaut				'A'
			STD_DBL_TO_UTF8 (0x13, 0x31, "\xC3\xA4");		//	U+00E4	lower-case a, umlaut				'a'
			STD_DBL_TO_UTF8 (0x13, 0x32, "\xC3\x96");		//	U+00D6	upper-case O, umlaut				'O'
			STD_DBL_TO_UTF8 (0x13, 0x33, "\xC3\xB6");		//	U+00F6	lower-case o, umlaut				'o'
			STD_DBL_TO_UTF8 (0x13, 0x34, "\xC3\x9F");		//	U+00DF	small sharp s						's'
			STD_DBL_TO_UTF8 (0x13, 0x35, "\xC2\xA5");		//	U+00A5	yen sign							'Y'
			STD_DBL_TO_UTF8 (0x13, 0x36, "\xC2\xA4");		//	U+00A4	non-specific currency sign			'$'
			STD_DBL_TO_UTF8 (0x13, 0x37, "|");				//	U+007C	vertical bar						'|'
			STD_DBL_TO_UTF8 (0x13, 0x38, "\xC3\x85");		//	U+00C5	upper-case A with ring				'A'
			STD_DBL_TO_UTF8 (0x13, 0x39, "\xC3\xA5");		//	U+00E5	lower-case a with ring				'a'
			STD_DBL_TO_UTF8 (0x13, 0x3a, "\xC3\x98");		//	U+00D8	upper-case O with stroke			'O'
			STD_DBL_TO_UTF8 (0x13, 0x3b, "\xC3\xB8");		//	U+00F8	lower-case o with stroke			'o'
			STD_DBL_TO_UTF8 (0x13, 0x3c, "\xE2\x94\x8C");	//	U+250C	upper-left corner					'F'
			STD_DBL_TO_UTF8 (0x13, 0x3d, "\xE2\x94\x90");	//	U+2510	upper-right corner					'T'
			STD_DBL_TO_UTF8 (0x13, 0x3e, "\xE2\x94\x94");	//	U+2514	lower-left corner					'L'
			STD_DBL_TO_UTF8 (0x13, 0x3f, "\xE2\x94\x98");	//	U+2518	lower-right corner					'J'

			//	Extended German/Danish, channels 2 or 4
			STD_DBL_TO_UTF8 (0x1b, 0x30, "\xC3\x84");		//	U+00C4	upper-case A, umlaut				'A'
			STD_DBL_TO_UTF8 (0x1b, 0x31, "\xC3\xA4");		//	U+00E4	lower-case a, umlaut				'a'
			STD_DBL_TO_UTF8 (0x1b, 0x32, "\xC3\x96");		//	U+00D6	upper-case O, umlaut				'O'
			STD_DBL_TO_UTF8 (0x1b, 0x33, "\xC3\xB6");		//	U+00F6	lower-case o, umlaut				'o'
			STD_DBL_TO_UTF8 (0x1b, 0x34, "\xC3\x9F");		//	U+00DF	small sharp s						's'
			STD_DBL_TO_UTF8 (0x1b, 0x35, "\xC2\xA5");		//	U+00A5	yen sign							'Y'
			STD_DBL_TO_UTF8 (0x1b, 0x36, "\xC2\xA4");		//	U+00A4	non-specific currency sign			'$'
			STD_DBL_TO_UTF8 (0x1b, 0x37, "|");				//	U+007C	vertical bar						'|'
			STD_DBL_TO_UTF8 (0x1b, 0x38, "\xC3\x85");		//	U+00C5	upper-case A with ring				'A'
			STD_DBL_TO_UTF8 (0x1b, 0x39, "\xC3\xA5");		//	U+00E5	lower-case a with ring				'a'
			STD_DBL_TO_UTF8 (0x1b, 0x3a, "\xC3\x98");		//	U+00D8	upper-case O with stroke			'O'
			STD_DBL_TO_UTF8 (0x1b, 0x3b, "\xC3\xB8");		//	U+00F8	lower-case o with stroke			'o'
			STD_DBL_TO_UTF8 (0x1b, 0x3c, "\xE2\x94\x8C");	//	U+250C	upper-left corner					'F'
			STD_DBL_TO_UTF8 (0x1b, 0x3d, "\xE2\x94\x90");	//	U+2510	upper-right corner					'T'
			STD_DBL_TO_UTF8 (0x1b, 0x3e, "\xE2\x94\x94");	//	U+2514	lower-left corner					'L'
			STD_DBL_TO_UTF8 (0x1b, 0x3f, "\xE2\x94\x98");	//	U+2518	lower-right corner					'J'

			//Log() << "## DEBUG:  gCEA608CodePointToUtf8StringMap initialized:" << endl;
			//for (CEA608CodePointToUTF8StringConstIter it (gCEA608CodePointToUtf8StringMap.begin ());  it != gCEA608CodePointToUtf8StringMap.end ();  ++it)
				//Log() << "0x" << hex << setw (8) << setfill ('0') << it->first << dec << ":  '" << it->second << "'" << endl;

			if ((sizeof (gCCFontDotMap) / sizeof (UWord) / NTV2_CCFont_BitmapCharHeight) != NTV2_CCFont_NumChars)
				cerr	<< "## ERROR:  Glyph dot map table contains " << (sizeof (gCCFontDotMap) / sizeof (UWord) / NTV2_CCFont_BitmapCharHeight)
						<< " glyphs, but GetGlyphCount returns " << NTV2_CCFont_NumChars << endl;
			AJACC_ASSERT ((sizeof (gCCFontDotMap) / sizeof (UWord) / NTV2_CCFont_BitmapCharHeight) == NTV2_CCFont_NumChars);
		}	//	constructor

		virtual ~Utf8TableInitializer ()
		{
			//Log() << "## DEBUG:  gCEA608CodePointToUtf8StringMap deinitialized" << endl;
		}
};	//	Utf8TableInitializer

static Utf8TableInitializer	gUtf8TableInitializer;	//	Singleton to initialize the UTF8 translation table(s)


string NTV2CC608CodePointToUtf8String (const NTV2_CC608_CodePoint in608CodePoint)
{
	CEA608CodePointToUtf8StringConstIter iter (gCEA608CodePointToUtf8StringMap.find (in608CodePoint));
	return iter != gCEA608CodePointToUtf8StringMap.end()  ?  iter->second  :  string();

}	//	NTV2CC608CodePointToUtf8String


#define	STD_TO_UTF16(_608_1, _608_2, _utf16_)		gCEA608CodePointToUtf16CharMap.insert (CEA608CodePointToUtf16CharPair (STD_CODEPOINT ((_608_1), (_608_2)),		\
																															UWord (_utf16_)));
#define	DBL_TO_UTF16(_608_1, _608_2, _utf16_)		gCEA608CodePointToUtf16CharMap.insert (CEA608CodePointToUtf16CharPair (DBLSZ_CODEPOINT ((_608_1), (_608_2)),	\
																															UWord (_utf16_)));
#define	STD_DBL_TO_UTF16(_608_1, _608_2, _utf16_)	STD_TO_UTF16 ((_608_1), (_608_2), (_utf16_));	DBL_TO_UTF16 ((_608_1), (_608_2), (_utf16_));


class Utf16TableInitializer
{
	public:
		Utf16TableInitializer ()
		{
			//	NORTH AMERICAN CHARACTER SET
			STD_DBL_TO_UTF16 (0x2A, 0x00, 0x00E1);		//	U+00E1	lower-case a, acute accent
			STD_DBL_TO_UTF16 (0x5C, 0x00, 0x00E9);		//	U+00E9	lower-case e, acute accent
			STD_DBL_TO_UTF16 (0x5E, 0x00, 0x00ED);		//	U+00ED	lower-case i, acute accent
			STD_DBL_TO_UTF16 (0x5F, 0x00, 0x00F3);		//	U+00F3	lower-case o, acute accent
			STD_DBL_TO_UTF16 (0x60, 0x00, 0x00FA);		//	U+00FA	lower-case u, acute accent
			STD_DBL_TO_UTF16 (0x7B, 0x00, 0x00E7);		//	U+00E7	lower-case c with cedilla
			STD_DBL_TO_UTF16 (0x7C, 0x00, 0x00F7);		//	U+00F7	division sign
			STD_DBL_TO_UTF16 (0x7D, 0x00, 0x00D1);		//	U+00D1	upper-case N with tilde
			STD_DBL_TO_UTF16 (0x7E, 0x00, 0x00F1);		//	U+00F1	lower-case n with tilde
			STD_DBL_TO_UTF16 (0x7F, 0x00, 0x2588);		//	U+2588	full block
			//	And the rest of the regular ASCII characters...
			for (UByte ascii (0x20);  ascii < 0x7B;  ascii++)
			{
				if (gCEA608CodePointToUtf16CharMap.find (STD_CODEPOINT (ascii, 0x00)) == gCEA608CodePointToUtf16CharMap.end ())
					STD_DBL_TO_UTF16 (ascii, 0x00, static_cast <UWord> (ascii));
			}

			//	Special North American (channel 1)
			STD_DBL_TO_UTF16 (0x11, 0x30, 0x00AE);		//	U+00AE	registered sign
			STD_DBL_TO_UTF16 (0x11, 0x31, 0x00B0);		//	U+00B0	degree sign
			STD_DBL_TO_UTF16 (0x11, 0x32, 0x00BD);		//	U+00BD	1/2 symbol
			STD_DBL_TO_UTF16 (0x11, 0x33, 0x00BF);		//	U+00BF	inverted question mark
			STD_DBL_TO_UTF16 (0x11, 0x34, 0x2122);		//	U+2122	trade mark sign
			STD_DBL_TO_UTF16 (0x11, 0x35, 0x00A2);		//	U+00A2	cents symbol
			STD_DBL_TO_UTF16 (0x11, 0x36, 0x00A3);		//	U+00A3	pound sterling
			STD_DBL_TO_UTF16 (0x11, 0x37, 0x266A);		//	U+266A	eighth note
			STD_DBL_TO_UTF16 (0x11, 0x38, 0x00E0);		//	U+00E0	lower-case a, grave accent
			STD_DBL_TO_UTF16 (0x11, 0x39, 0x00A0);		//	U+00A0	transparent space
			STD_DBL_TO_UTF16 (0x11, 0x3A, 0x00E8);		//	U+00E8	lower-case e, grave accent
			STD_DBL_TO_UTF16 (0x11, 0x3B, 0x00E2);		//	U+00E2	lower-case a, circumflex accent
			STD_DBL_TO_UTF16 (0x11, 0x3C, 0x00EA);		//	U+00EA	lower-case e, circumflex accent
			STD_DBL_TO_UTF16 (0x11, 0x3D, 0x00EE);		//	U+00EE	lower-case i, circumflex accent
			STD_DBL_TO_UTF16 (0x11, 0x3E, 0x00F4);		//	U+00F4	lower-case o, circumflex accent
			STD_DBL_TO_UTF16 (0x11, 0x3F, 0x00FB);		//	U+00FB	lower-case u, circumflex accent

			//	Special North American (channel 2)
			STD_DBL_TO_UTF16 (0x19, 0x30, 0x00AE);		//	U+00AE	registered sign
			STD_DBL_TO_UTF16 (0x19, 0x31, 0x00B0);		//	U+00B0	degree sign
			STD_DBL_TO_UTF16 (0x19, 0x32, 0x00BD);		//	U+00BD	1/2 symbol
			STD_DBL_TO_UTF16 (0x19, 0x33, 0x00BF);		//	U+00BF	inverted question mark
			STD_DBL_TO_UTF16 (0x19, 0x34, 0x2122);		//	U+2122	trade mark sign
			STD_DBL_TO_UTF16 (0x19, 0x35, 0x00A2);		//	U+00A2	cents symbol
			STD_DBL_TO_UTF16 (0x19, 0x36, 0x00A3);		//	U+00A3	pound sterling
			STD_DBL_TO_UTF16 (0x19, 0x37, 0x266A);		//	U+266A	eighth note
			STD_DBL_TO_UTF16 (0x19, 0x38, 0x00E0);		//	U+00E0	lower-case a, grave accent
			STD_DBL_TO_UTF16 (0x19, 0x39, 0x00A0);		//	U+00A0	transparent space
			STD_DBL_TO_UTF16 (0x19, 0x3A, 0x00E8);		//	U+00E8	lower-case e, grave accent
			STD_DBL_TO_UTF16 (0x19, 0x3B, 0x00E2);		//	U+00E2	lower-case a, circumflex accent
			STD_DBL_TO_UTF16 (0x19, 0x3C, 0x00EA);		//	U+00EA	lower-case e, circumflex accent
			STD_DBL_TO_UTF16 (0x19, 0x3D, 0x00EE);		//	U+00EE	lower-case i, circumflex accent
			STD_DBL_TO_UTF16 (0x19, 0x3E, 0x00F4);		//	U+00F4	lower-case o, circumflex accent
			STD_DBL_TO_UTF16 (0x19, 0x3F, 0x00FB);		//	U+00FB	lower-case u, circumflex accent

			//	EXTENDED WESTERN EUROPEAN CHARACTER SET

			//	Extended Spanish/Misc, channel 1 or 3
			STD_DBL_TO_UTF16 (0x12, 0x20, 0x00C1);		//	U+00C1	upper-case A, acute accent			'A'
			STD_DBL_TO_UTF16 (0x12, 0x21, 0x00C9);		//	U+00C9	upper-case E, acute accent			'E'
			STD_DBL_TO_UTF16 (0x12, 0x22, 0x00D3);		//	U+00D3	upper-case O, acute accent			'O'
			STD_DBL_TO_UTF16 (0x12, 0x23, 0x00DA);		//	U+00DA	upper-case U, acute accent			'U'
			STD_DBL_TO_UTF16 (0x12, 0x24, 0x00DC);		//	U+00DC	upper-case U, umlaut				'U'
			STD_DBL_TO_UTF16 (0x12, 0x25, 0x00FC);		//	U+00FC	lower-case u, umlaut				'u'
			STD_DBL_TO_UTF16 (0x12, 0x26, 0x2018);		//	U+2018	opening single quote				'`'
			STD_DBL_TO_UTF16 (0x12, 0x27, 0x00A1);		//	U+00A1	inverted exclamation				'!'
			STD_DBL_TO_UTF16 (0x12, 0x28, 0x002A);		//	U+002A	asterisk							'*'
			STD_DBL_TO_UTF16 (0x12, 0x29, 0x2019);		//	U+2019	closing single quote				'\''
			STD_DBL_TO_UTF16 (0x12, 0x2a, 0x2014);		//	U+2014	em dash								'_'
			STD_DBL_TO_UTF16 (0x12, 0x2b, 0x00A9);		//	U+00A9	copyright							'(C)'
			STD_DBL_TO_UTF16 (0x12, 0x2c, 0x2120);		//	U+2120	service mark						'(R)'
			STD_DBL_TO_UTF16 (0x12, 0x2d, 0x2022);		//	U+2022	round bullet						'o'
			STD_DBL_TO_UTF16 (0x12, 0x2e, 0x201C);		//	U+201C	opening double quotes				'"'
			STD_DBL_TO_UTF16 (0x12, 0x2f, 0x201D);		//	U+201D	closing double quotes				'"'

			//	Extended Spanish/Misc, channel 2 or 4
			STD_DBL_TO_UTF16 (0x1a, 0x20, 0x00C1);		//	U+00C1	upper-case A, acute accent			'A'
			STD_DBL_TO_UTF16 (0x1a, 0x21, 0x00C9);		//	U+00C9	upper-case E, acute accent			'E'
			STD_DBL_TO_UTF16 (0x1a, 0x22, 0x00D3);		//	U+00D3	upper-case O, acute accent			'O'
			STD_DBL_TO_UTF16 (0x1a, 0x23, 0x00DA);		//	U+00DA	upper-case U, acute accent			'U'
			STD_DBL_TO_UTF16 (0x1a, 0x24, 0x00DC);		//	U+00DC	upper-case U, umlaut				'U'
			STD_DBL_TO_UTF16 (0x1a, 0x25, 0x00FC);		//	U+00FC	lower-case u, umlaut				'u'
			STD_DBL_TO_UTF16 (0x1a, 0x26, 0x2018);		//	U+2018	opening single quote				'`'
			STD_DBL_TO_UTF16 (0x1a, 0x27, 0x00A1);		//	U+00A1	inverted exclamation				'!'
			STD_DBL_TO_UTF16 (0x1a, 0x28, 0x002A);		//	U+002A	asterisk							'*'
			STD_DBL_TO_UTF16 (0x1a, 0x29, 0x2019);		//	U+2019	closing single quote				'\''
			STD_DBL_TO_UTF16 (0x1a, 0x2a, 0x2014);		//	U+2014	em dash								'_'
			STD_DBL_TO_UTF16 (0x1a, 0x2b, 0x00A9);		//	U+00A9	copyright							'(C)'
			STD_DBL_TO_UTF16 (0x1a, 0x2c, 0x2120);		//	U+2120	service mark						'(R)'
			STD_DBL_TO_UTF16 (0x1a, 0x2d, 0x2022);		//	U+2022	round bullet						'o'
			STD_DBL_TO_UTF16 (0x1a, 0x2e, 0x201C);		//	U+201C	opening double quotes				'"'
			STD_DBL_TO_UTF16 (0x1a, 0x2f, 0x201D);		//	U+201D	closing double quotes				'"'

			//	Extended French, channel 1 or 3
			STD_DBL_TO_UTF16 (0x12, 0x30, 0x00C0);		//	U+00C0	upper-case A, grave accent			'A'
			STD_DBL_TO_UTF16 (0x12, 0x31, 0x00C2);		//	U+00C2	upper-case A, circumflex accent		'A'
			STD_DBL_TO_UTF16 (0x12, 0x32, 0x00C7);		//	U+00C7	upper-case C with cedilla			'C'
			STD_DBL_TO_UTF16 (0x12, 0x33, 0x00C8);		//	U+00C8	upper-case E, grave accent			'E'
			STD_DBL_TO_UTF16 (0x12, 0x34, 0x00CA);		//	U+00CA	upper-case E, circumflex accent		'E'
			STD_DBL_TO_UTF16 (0x12, 0x35, 0x00CB);		//	U+00CB	upper-case E, umlaut				'E'
			STD_DBL_TO_UTF16 (0x12, 0x36, 0x00EB);		//	U+00EB	lower-case e, umlaut				'e'
			STD_DBL_TO_UTF16 (0x12, 0x37, 0x00CE);		//	U+00CE	upper-case I, circumflex accent		'I'
			STD_DBL_TO_UTF16 (0x12, 0x38, 0x00CF);		//	U+00CF	upper-case I, umlaut				'I'
			STD_DBL_TO_UTF16 (0x12, 0x39, 0x00EF);		//	U+00EF	lower-case i, umlaut				'i'
			STD_DBL_TO_UTF16 (0x12, 0x3a, 0x00D4);		//	U+00D4	upper-case O, circumflex accent		'O'
			STD_DBL_TO_UTF16 (0x12, 0x3b, 0x00D9);		//	U+00D9	upper-case U, grave accent			'U'
			STD_DBL_TO_UTF16 (0x12, 0x3c, 0x00F9);		//	U+00F9	lower-case u, grave accent			'u'
			STD_DBL_TO_UTF16 (0x12, 0x3d, 0x00DB);		//	U+00DB	upper-case U, circumflex accent		'U'
			STD_DBL_TO_UTF16 (0x12, 0x3e, 0x00AB);		//	U+00AB	opening guillemet					'<'
			STD_DBL_TO_UTF16 (0x12, 0x3f, 0x00BB);		//	U+00BB	closing guillemet					'>'

			//	Extended French, channel 2 or 4
			STD_DBL_TO_UTF16 (0x1a, 0x30, 0x00C0);		//	U+00C0	upper-case A, grave accent			'A'
			STD_DBL_TO_UTF16 (0x1a, 0x31, 0x00C2);		//	U+00C2	upper-case A, circumflex accent		'A'
			STD_DBL_TO_UTF16 (0x1a, 0x32, 0x00C7);		//	U+00C7	upper-case C with cedilla			'C'
			STD_DBL_TO_UTF16 (0x1a, 0x33, 0x00C8);		//	U+00C8	upper-case E, grave accent			'E'
			STD_DBL_TO_UTF16 (0x1a, 0x34, 0x00CA);		//	U+00CA	upper-case E, circumflex accent		'E'
			STD_DBL_TO_UTF16 (0x1a, 0x35, 0x00CB);		//	U+00CB	upper-case E, umlaut				'E'
			STD_DBL_TO_UTF16 (0x1a, 0x36, 0x00EB);		//	U+00EB	lower-case e, umlaut				'e'
			STD_DBL_TO_UTF16 (0x1a, 0x37, 0x00CE);		//	U+00CE	upper-case I, circumflex accent		'I'
			STD_DBL_TO_UTF16 (0x1a, 0x38, 0x00CF);		//	U+00CF	upper-case I, umlaut				'I'
			STD_DBL_TO_UTF16 (0x1a, 0x39, 0x00EF);		//	U+00EF	lower-case i, umlaut				'i'
			STD_DBL_TO_UTF16 (0x1a, 0x3a, 0x00D4);		//	U+00D4	upper-case O, circumflex accent		'O'
			STD_DBL_TO_UTF16 (0x1a, 0x3b, 0x00D9);		//	U+00D9	upper-case U, grave accent			'U'
			STD_DBL_TO_UTF16 (0x1a, 0x3c, 0x00F9);		//	U+00F9	lower-case u, grave accent			'u'
			STD_DBL_TO_UTF16 (0x1a, 0x3d, 0x00DB);		//	U+00DB	upper-case U, circumflex accent		'U'
			STD_DBL_TO_UTF16 (0x1a, 0x3e, 0x00AB);		//	U+00AB	opening guillemet					'<'
			STD_DBL_TO_UTF16 (0x1a, 0x3f, 0x00BB);		//	U+00BB	closing guillemet					'>'

			//	Extended Portuguese, channels 1 or 3
			STD_DBL_TO_UTF16 (0x13, 0x20, 0x00C3);		//	U+00C3	upper-case A with tilde				'A'
			STD_DBL_TO_UTF16 (0x13, 0x21, 0x00E3);		//	U+00E3	lower-case a with tilde				'E'
			STD_DBL_TO_UTF16 (0x13, 0x22, 0x00CD);		//	U+00CD	upper-case I, acute accent			'I'
			STD_DBL_TO_UTF16 (0x13, 0x23, 0x00CC);		//	U+00CC	upper-case I, grave accent			'I'
			STD_DBL_TO_UTF16 (0x13, 0x24, 0x00EC);		//	U+00EC	lower-case i, grave accent			'i'
			STD_DBL_TO_UTF16 (0x13, 0x25, 0x00D2);		//	U+00D2	upper-case O, grave accent			'O'
			STD_DBL_TO_UTF16 (0x13, 0x26, 0x00F2);		//	U+00F2	lower-case o, grave accent			'o'
			STD_DBL_TO_UTF16 (0x13, 0x27, 0x00D5);		//	U+00D5	upper-case O with tilde				'O'
			STD_DBL_TO_UTF16 (0x13, 0x28, 0x00F5);		//	U+00F5	lower-case o with tilde				'o'
			STD_DBL_TO_UTF16 (0x13, 0x29, 0x007B);		//	U+007B	opening brace						'{'
			STD_DBL_TO_UTF16 (0x13, 0x2a, 0x007D);		//	U+007D	closing brace						'}'
			STD_DBL_TO_UTF16 (0x13, 0x2b, 0x005C);		//	U+005C	backslash							'\\'
			STD_DBL_TO_UTF16 (0x13, 0x2c, 0x005E);		//	U+005E	caret								'^'
			STD_DBL_TO_UTF16 (0x13, 0x2d, 0x005F);		//	U+005F	underbar							'_'
			STD_DBL_TO_UTF16 (0x13, 0x2e, 0x007C);		//	U+007C	pipe								'|'
			STD_DBL_TO_UTF16 (0x13, 0x2f, 0x007E);		//	U+007E	tilde								'~'

			//	Extended Portuguese, channels 2 or 4
			STD_DBL_TO_UTF16 (0x1b, 0x20, 0x00C3);		//	U+00C3	upper-case A with tilde				'A'
			STD_DBL_TO_UTF16 (0x1b, 0x21, 0x00E3);		//	U+00E3	lower-case a with tilde				'E'
			STD_DBL_TO_UTF16 (0x1b, 0x22, 0x00CD);		//	U+00CD	upper-case I, acute accent			'I'
			STD_DBL_TO_UTF16 (0x1b, 0x23, 0x00CC);		//	U+00CC	upper-case I, grave accent			'I'
			STD_DBL_TO_UTF16 (0x1b, 0x24, 0x00EC);		//	U+00EC	lower-case i, grave accent			'i'
			STD_DBL_TO_UTF16 (0x1b, 0x25, 0x00D2);		//	U+00D2	upper-case O, grave accent			'O'
			STD_DBL_TO_UTF16 (0x1b, 0x26, 0x00F2);		//	U+00F2	lower-case o, grave accent			'o'
			STD_DBL_TO_UTF16 (0x1b, 0x27, 0x00D5);		//	U+00D5	upper-case O with tilde				'O'
			STD_DBL_TO_UTF16 (0x1b, 0x28, 0x00F5);		//	U+00F5	lower-case o with tilde				'o'
			STD_DBL_TO_UTF16 (0x1b, 0x29, 0x007B);		//	U+007B	opening brace						'{'
			STD_DBL_TO_UTF16 (0x1b, 0x2a, 0x007D);		//	U+007D	closing brace						'}'
			STD_DBL_TO_UTF16 (0x1b, 0x2b, 0x005C);		//	U+005C	backslash							'\\'
			STD_DBL_TO_UTF16 (0x1b, 0x2c, 0x005E);		//	U+005E	caret								'^'
			STD_DBL_TO_UTF16 (0x1b, 0x2d, 0x005F);		//	U+005F	underbar							'_'
			STD_DBL_TO_UTF16 (0x1b, 0x2e, 0x007C);		//	U+007C	pipe								'|'
			STD_DBL_TO_UTF16 (0x1b, 0x2f, 0x007E);		//	U+007E	tilde								'~'

			//	Extended German/Danish, channels 1 or 3
			STD_DBL_TO_UTF16 (0x13, 0x30, 0x00C4);		//	U+00C4	upper-case A, umlaut				'A'
			STD_DBL_TO_UTF16 (0x13, 0x31, 0x00E4);		//	U+00E4	lower-case a, umlaut				'a'
			STD_DBL_TO_UTF16 (0x13, 0x32, 0x00D6);		//	U+00D6	upper-case O, umlaut				'O'
			STD_DBL_TO_UTF16 (0x13, 0x33, 0x00F6);		//	U+00F6	lower-case o, umlaut				'o'
			STD_DBL_TO_UTF16 (0x13, 0x34, 0x00DF);		//	U+00DF	small sharp s						's'
			STD_DBL_TO_UTF16 (0x13, 0x35, 0x00A5);		//	U+00A5	yen sign							'Y'
			STD_DBL_TO_UTF16 (0x13, 0x36, 0x00A4);		//	U+00A4	non-specific currency sign			'$'
			STD_DBL_TO_UTF16 (0x13, 0x37, 0x007C);		//	U+007C	vertical bar						'|'
			STD_DBL_TO_UTF16 (0x13, 0x38, 0x00C5);		//	U+00C5	upper-case A with ring				'A'
			STD_DBL_TO_UTF16 (0x13, 0x39, 0x00E5);		//	U+00E5	lower-case a with ring				'a'
			STD_DBL_TO_UTF16 (0x13, 0x3a, 0x00D8);		//	U+00D8	upper-case O with stroke			'O'
			STD_DBL_TO_UTF16 (0x13, 0x3b, 0x00F8);		//	U+00F8	lower-case o with stroke			'o'
			STD_DBL_TO_UTF16 (0x13, 0x3c, 0x250C);		//	U+250C	upper-left corner					'F'
			STD_DBL_TO_UTF16 (0x13, 0x3d, 0x2510);		//	U+2510	upper-right corner					'T'
			STD_DBL_TO_UTF16 (0x13, 0x3e, 0x2514);		//	U+2514	lower-left corner					'L'
			STD_DBL_TO_UTF16 (0x13, 0x3f, 0x2518);		//	U+2518	lower-right corner					'J'

			//	Extended German/Danish, channels 2 or 4
			STD_DBL_TO_UTF16 (0x1b, 0x30, 0x00C4);		//	U+00C4	upper-case A, umlaut				'A'
			STD_DBL_TO_UTF16 (0x1b, 0x31, 0x00E4);		//	U+00E4	lower-case a, umlaut				'a'
			STD_DBL_TO_UTF16 (0x1b, 0x32, 0x00D6);		//	U+00D6	upper-case O, umlaut				'O'
			STD_DBL_TO_UTF16 (0x1b, 0x33, 0x00F6);		//	U+00F6	lower-case o, umlaut				'o'
			STD_DBL_TO_UTF16 (0x1b, 0x34, 0x00DF);		//	U+00DF	small sharp s						's'
			STD_DBL_TO_UTF16 (0x1b, 0x35, 0x00A5);		//	U+00A5	yen sign							'Y'
			STD_DBL_TO_UTF16 (0x1b, 0x36, 0x00A4);		//	U+00A4	non-specific currency sign			'$'
			STD_DBL_TO_UTF16 (0x1b, 0x37, 0x007C);		//	U+007C	vertical bar						'|'
			STD_DBL_TO_UTF16 (0x1b, 0x38, 0x00C5);		//	U+00C5	upper-case A with ring				'A'
			STD_DBL_TO_UTF16 (0x1b, 0x39, 0x00E5);		//	U+00E5	lower-case a with ring				'a'
			STD_DBL_TO_UTF16 (0x1b, 0x3a, 0x00D8);		//	U+00D8	upper-case O with stroke			'O'
			STD_DBL_TO_UTF16 (0x1b, 0x3b, 0x00F8);		//	U+00F8	lower-case o with stroke			'o'
			STD_DBL_TO_UTF16 (0x1b, 0x3c, 0x250C);		//	U+250C	upper-left corner					'F'
			STD_DBL_TO_UTF16 (0x1b, 0x3d, 0x2510);		//	U+2510	upper-right corner					'T'
			STD_DBL_TO_UTF16 (0x1b, 0x3e, 0x2514);		//	U+2514	lower-left corner					'L'
			STD_DBL_TO_UTF16 (0x1b, 0x3f, 0x2518);		//	U+2518	lower-right corner					'J'

			//Log() << "## DEBUG:  gCEA608CodePointToUtf16CharMap initialized:" << endl;
			//for (CEA608CodePointToUTF8StringConstIter it (gCEA608CodePointToUtf16CharMap.begin ());  it != gCEA608CodePointToUtf16CharMap.end ();  ++it)
				//Log() << "0x" << hex << setw (8) << setfill ('0') << it->first << dec << ":  '" << it->second << "'" << endl;
		}	//	constructor

		virtual ~Utf16TableInitializer ()
		{
			//Log() << "## DEBUG:  gCEA608CodePointToUtf16CharMap deinitialized" << endl;
		}
};	//	Utf16TableInitializer

static Utf16TableInitializer	gUtf16TableInitializer;	//	Singleton to initialize the UTF16 translation table(s)


UWord NTV2CC608CodePointToUtf16Char (const NTV2_CC608_CodePoint in608CodePoint)
{
	CEA608CodePointToUtf16CharConstIter	iter	(gCEA608CodePointToUtf16CharMap.find(in608CodePoint));
	if (iter != gCEA608CodePointToUtf16CharMap.end())
		return iter->second;
	else
		return 0x0000;

}	//	NTV2CC608CodePointToUtf16Char


void DumpCCFont (const NTV2CCFont & inCCFont)
{
	NTV2GlyphIndex	glyphsPerRow	(8);
	NTV2GlyphIndex	firstGlyph		(0);
	cerr	<< "## DEBUG:  Dump of " << inCCFont.GetGlyphCount() << " glyph(s) of NTV2CCFont '" << inCCFont.GetName() << "'" << endl
			<< "   LEGEND: [charOffset]  [charOffset + 0x20]  [firstCodePoint]  [Utf8char], where [firstCodePoint] is [charSet|00|cc1|cc2]" << endl;
	while (firstGlyph < inCCFont.GetGlyphCount())
	{
		NTV2GlyphIndex	lastGlyph	(firstGlyph + (glyphsPerRow - 1));
		if (lastGlyph >= inCCFont.GetGlyphCount())
			lastGlyph = inCCFont.GetGlyphCount() - 1;
		inCCFont.PrintGlyphs (cerr, firstGlyph, lastGlyph);
		firstGlyph += glyphsPerRow;
	}
}


// BytesPerPixel()																(static)
//		Returns the number of bytes that <numPixels> occupies for the designated format.
//		Returns zero for unsupported formats
//
static ULWord BytesPerPixel (const NTV2FrameBufferFormat	inFBFormat,
								const ULWord				numPixels)
{
	ULWord	result(0);
	switch (inFBFormat)
	{
		case NTV2_FBF_10BIT_YCBCR_DPX:	//	NOTE:	Rounds up to nearest multiple of 6 pixels:
		case NTV2_FBF_10BIT_YCBCR:		result = ((numPixels * 16) + 5) / 6;
										break;

		default:						result = ::CalcRowBytesForFormat (inFBFormat, numPixels);
										break;
	}
	return result;

}	//	BytesPerPixel


//		Render one character (2vuy format) into the given buffer
//
bool NTV2CCFont::RenderGlyph8BitYCbCr (UByte * pDestBuffer, const ULWord inBytesPerRow, const NTV2GlyphIndex inGlyphIndex,
										const NTV2Line21Attrs & inAttribs, const ULWord inScaledDotWidth, const ULWord inScaledDotHeight) const
{
	UByte	bg_y (0x10),	bg_cb (0x80),	bg_cr (0x80);
	UByte	fg_y (0x00),	fg_cb (0x00),	fg_cr (0x00);
	bool	bResult	(true);

	//	Reality check
	if (inGlyphIndex >= GetGlyphCount())
		return false;

	//	Get scaling factors, and the FG & BG colors...
	if (!::NTV2Line21ColorToYUV8 (inAttribs.GetColor(), fg_y, fg_cb, fg_cr)
		|| !::NTV2Line21ColorToYUV8 (inAttribs.GetBGColor(), bg_y, bg_cb, bg_cr))
			return false;

	const unsigned	underlineDotRow1	(GetUnderlineStartingDotRow());
	const unsigned	underlineDotRow2	(underlineDotRow1 + 1);
	const unsigned	firstOfficialDotRow	(GetTopMarginDotCount());
	const unsigned	lastOfficialDotRow	(firstOfficialDotRow + GetDotMapHeight() - 1);

	//	For each dot row of the full height of the "dot map"...
	for (unsigned y(0);  y < GetTotalHeightInDots();  y++)
	{
		const int	xOffset	(inAttribs.IsItalicized() ? (y / 7) : 3);

		//	Each rendered line is duplicated <inScaledDotHeight> times...
		for (unsigned ydup (0);  ydup < inScaledDotHeight;  ydup++)
		{
			bool	bUnderline	(inAttribs.IsUnderlined() && (y == underlineDotRow1 || y == underlineDotRow2));
			if (inGlyphIndex == GetNoUnderlineSpaceGlyphIndex())
				bUnderline = false;
			ULWord	charDots	(0);

			//	Get the "dots" for this line from the font map...
			//	NOTE:	There are 26 "dots" per character height, however we only store 18 bits
			//			in gCCFontDotMap. The first two and last six rows are always zero (black)...
			if (y >= firstOfficialDotRow  &&  y <= lastOfficialDotRow)
				charDots = GetGlyphRowDots (inGlyphIndex, y - firstOfficialDotRow);

			charDots = charDots << xOffset;
			UByte *	pSavedLineStart	(pDestBuffer);

			//	For each horizontal dot column...
			for (unsigned x(0);  x < GetTotalWidthInDots();  x++)
			{
				//	NOTE:	There are 18 "dots" per character width, however we only store 16 bits
				//			in gCCFontDotMap. The first and last "dots" are always zero (black)...
				//bool bDot = bUnderline ? true : ((charDots & 0x10000) != 0);
				const bool	bDot	(bUnderline ? true : (charDots & 0x80000) != 0);

				//	Each rendered pixel is duplicated <inScaledDotWidth> times...
				for (UWord xdup(0);  xdup < inScaledDotWidth;  xdup++)
				{
					//	Even pixels have all three components...
					if ((reinterpret_cast <uintptr_t> (pDestBuffer) & 0x02) == 0)
					{
						pDestBuffer[0] = bDot ? fg_cb : bg_cb;		// Cb0
						pDestBuffer[1] = bDot ? fg_y  : bg_y;		// Y0
						pDestBuffer[2] = bDot ? fg_cr : bg_cr;		// Cr0
					}
					else	// odd components only have one
						pDestBuffer[1] = bDot ? fg_y : bg_y;		// Y1

					pDestBuffer += ::BytesPerPixel(NTV2_FBF_8BIT_YCBCR, 1);
				}	//	for each scaled horizontal dot

				charDots = charDots << 1;	//	Shift in next "dot"

			}	//	For each horizontal dot column

			pDestBuffer = pSavedLineStart;	//	Restore start of glyph on this line...
			pDestBuffer += inBytesPerRow;	//	...then bump to next line

		}	//	For each rendered (scaled) line/row

	}	//	For each dot row

	return bResult;

}	//	RenderGlyph8BitYCbCr


bool NTV2CCFont::RenderGlyph8BitRGB (const NTV2PixelFormat inPixelFormat, UByte * pDestBuffer, const ULWord inBytesPerRow, const NTV2GlyphIndex inGlyphIndex,
										const NTV2Line21Attrs & inAttribs, const ULWord inScaledDotWidth, const ULWord inScaledDotHeight, const bool inIsHD) const
{
	const UByte	opaqueAlpha		(0xFF);
	const UByte	semiTranspAlpha	(0x6F);
	const UByte	transpAlpha		(0x00);

	UByte	fg_r (0x00),	fg_g (0x00),	fg_b (0x00);
	UByte	bg_r (0x00),	bg_g (0x00),	bg_b (0x00),	bg_a (opaqueAlpha);
	bool	bResult	(true);

	//	Reality check
	if (inGlyphIndex >= GetGlyphCount())
		return false;
	if (inPixelFormat != NTV2_FBF_ARGB  &&  inPixelFormat != NTV2_FBF_RGBA  &&  inPixelFormat != NTV2_FBF_ABGR)
		return false;

	//	Get scaling factors, and the FG & BG colors...
	if (!::NTV2Line21ColorToRGB8 (inAttribs.GetColor(), fg_r, fg_g, fg_b, inIsHD)
		|| !::NTV2Line21ColorToRGB8 (inAttribs.GetBGColor(), bg_r, bg_g, bg_b, inIsHD))
			return false;

	if (inAttribs.GetOpacity() == NTV2_CC608_SemiTransparent)
		bg_a = semiTranspAlpha;
	else if (inAttribs.GetOpacity() == NTV2_CC608_Transparent)
		bg_a = transpAlpha;

	const unsigned	bytesPerPixel		(::BytesPerPixel(inPixelFormat, 1));
	const unsigned	underlineDotRow1	(GetUnderlineStartingDotRow());
	const unsigned	underlineDotRow2	(underlineDotRow1 + 1);
	const unsigned	firstOfficialDotRow	(GetTopMarginDotCount());
	const unsigned	lastOfficialDotRow	(firstOfficialDotRow + GetDotMapHeight() - 1);

	//	For each dot row of the full height of the "dot map"...
	for (unsigned y(0);  y < GetTotalHeightInDots();  y++)
	{
		const int	xOffset	(inAttribs.IsItalicized() ? (y / 7) : 3);

		//	Each rendered line is duplicated <inScaledDotHeight> times...
		for (unsigned ydup(0);  ydup < inScaledDotHeight;  ydup++)
		{
			bool	bUnderline	(inAttribs.IsUnderlined() && (y == underlineDotRow1 || y == underlineDotRow2));
			if (inGlyphIndex == GetNoUnderlineSpaceGlyphIndex())
				bUnderline = false;
			ULWord	charDots	(0);

			//	Get the "dots" for this line from the font map...
			//	NOTE:	There are 26 "dots" per character height, however we only store 18 bits
			//			in gCCFontDotMap. The first two and last six rows are always zero (black)...
			if (y >= firstOfficialDotRow && y <= lastOfficialDotRow)
				charDots = GetGlyphRowDots (inGlyphIndex, y - firstOfficialDotRow);

			charDots = charDots << xOffset;
			UByte *	pSavedLineStart	(pDestBuffer);

			//	For each horizontal dot column...
			for (unsigned x(0);  x < GetTotalWidthInDots();  x++)
			{
				//	NOTE:	There are 18 "dots" per character width, however we only store 16 bits
				//			in gCCFontDotMap. The first and last "dots" are always zero (black)...
				//bool bDot = bUnderline ? true : ((charDots & 0x10000) != 0);
				const bool	bDot	(bUnderline ? true : (charDots & 0x80000) != 0);

				//	Each rendered pixel is duplicated <inScaledDotWidth> times...
				for (UWord xdup(0);  xdup < inScaledDotWidth;  xdup++)
				{
					switch (inPixelFormat)
					{
						case NTV2_FBF_ARGB:	pDestBuffer[0] = bDot ? fg_b : bg_b;		// B
											pDestBuffer[1] = bDot ? fg_g : bg_g;		// G
											pDestBuffer[2] = bDot ? fg_r : bg_r;		// R
											pDestBuffer[3] = bDot ? opaqueAlpha : bg_a;	// A
											break;
						case NTV2_FBF_RGBA:	pDestBuffer[0] = bDot ? opaqueAlpha : bg_a;	// A
											pDestBuffer[1] = bDot ? fg_r : bg_r;		// R
											pDestBuffer[2] = bDot ? fg_g : bg_g;		// G
											pDestBuffer[3] = bDot ? fg_b : bg_b;		// B
											break;
						case NTV2_FBF_ABGR:	pDestBuffer[0] = bDot ? fg_r : bg_r;		// R
											pDestBuffer[1] = bDot ? fg_g : bg_g;		// G
											pDestBuffer[2] = bDot ? fg_b : bg_b;		// B
											pDestBuffer[3] = bDot ? opaqueAlpha : bg_a;	// A
											break;
						default:			break;
					}
					pDestBuffer += bytesPerPixel;
				}	//	for each scaled horizontal dot

				charDots = charDots << 1;	//	Shift in next "dot"

			}	//	For each horizontal dot column

			pDestBuffer = pSavedLineStart;	//	Restore start of glyph on this line...
			pDestBuffer += inBytesPerRow;	//	...then bump to next line

		}	//	For each rendered (scaled) line/row

	}	//	For each dot row

	return bResult;

}	//	RenderGlyph8BitRGB
