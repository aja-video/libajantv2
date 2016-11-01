/**
	@file	main.cpp
	@copyright	2015 AJA Video Systems. All rights reserved.
**/

#include "ntv2devicefeaturesbft.h"
#include "ntv2bft.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2debug.h"
#include <iomanip>


using namespace std;


//	For reliable (and easier) debug output:
const string gHexLC [256] = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
							"10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
							"20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
							"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
							"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
							"50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
							"60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
							"70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
							"80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
							"90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
							"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
							"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
							"c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
							"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
							"e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
							"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"};

const string gHexUC [256] = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F",
							"10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F",
							"20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
							"30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F",
							"40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F",
							"50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
							"60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F",
							"70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
							"80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
							"90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F",
							"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF",
							"B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
							"C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
							"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF",
							"E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
							"F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"};

const string gOct [256] = {	"000", "001", "002", "003", "004", "005", "006", "007", "010", "011", "012", "013", "014", "015", "016", "017",
							"020", "021", "022", "023", "024", "025", "026", "027", "030", "031", "032", "033", "034", "035", "036", "037",
							"040", "041", "042", "043", "044", "045", "046", "047", "050", "051", "052", "053", "054", "055", "056", "057",
							"060", "061", "062", "063", "064", "065", "066", "067", "070", "071", "072", "073", "074", "075", "076", "077",
							"100", "101", "102", "103", "104", "105", "106", "107", "110", "111", "112", "113", "114", "115", "116", "117",
							"120", "121", "122", "123", "124", "125", "126", "127", "130", "131", "132", "133", "134", "135", "136", "137",
							"140", "141", "142", "143", "144", "145", "146", "147", "150", "151", "152", "153", "154", "155", "156", "157",
							"160", "161", "162", "163", "164", "165", "166", "167", "170", "171", "172", "173", "174", "175", "176", "177",
							"200", "201", "202", "203", "204", "205", "206", "207", "210", "211", "212", "213", "214", "215", "216", "217",
							"220", "221", "222", "223", "224", "225", "226", "227", "230", "231", "232", "233", "234", "235", "236", "237",
							"240", "241", "242", "243", "244", "245", "246", "247", "250", "251", "252", "253", "254", "255", "256", "257",
							"260", "261", "262", "263", "264", "265", "266", "267", "270", "271", "272", "273", "274", "275", "276", "277",
							"300", "301", "302", "303", "304", "305", "306", "307", "310", "311", "312", "313", "314", "315", "316", "317",
							"320", "321", "322", "323", "324", "325", "326", "327", "330", "331", "332", "333", "334", "335", "336", "337",
							"340", "341", "342", "343", "344", "345", "346", "347", "350", "351", "352", "353", "354", "355", "356", "357",
							"360", "361", "362", "363", "364", "365", "366", "367", "370", "371", "372", "373", "374", "375", "376", "377"};

const string gBin [256] = {	"00000000", "00000001", "00000010", "00000011", "00000100", "00000101", "00000110", "00000111", "00001000", "00001001", "00001010", "00001011", "00001100", "00001101", "00001110", "00001111",
							"00010000", "00010001", "00010010", "00010011", "00010100", "00010101", "00010110", "00010111", "00011000", "00011001", "00011010", "00011011", "00011100", "00011101", "00011110", "00011111",
							"00100000", "00100001", "00100010", "00100011", "00100100", "00100101", "00100110", "00100111", "00101000", "00101001", "00101010", "00101011", "00101100", "00101101", "00101110", "00101111",
							"00110000", "00110001", "00110010", "00110011", "00110100", "00110101", "00110110", "00110111", "00111000", "00111001", "00111010", "00111011", "00111100", "00111101", "00111110", "00111111",
							"01000000", "01000001", "01000010", "01000011", "01000100", "01000101", "01000110", "01000111", "01001000", "01001001", "01001010", "01001011", "01001100", "01001101", "01001110", "01001111",
							"01010000", "01010001", "01010010", "01010011", "01010100", "01010101", "01010110", "01010111", "01011000", "01011001", "01011010", "01011011", "01011100", "01011101", "01011110", "01011111",
							"01100000", "01100001", "01100010", "01100011", "01100100", "01100101", "01100110", "01100111", "01101000", "01101001", "01101010", "01101011", "01101100", "01101101", "01101110", "01101111",
							"01110000", "01110001", "01110010", "01110011", "01110100", "01110101", "01110110", "01110111", "01111000", "01111001", "01111010", "01111011", "01111100", "01111101", "01111110", "01111111",
							"10000000", "10000001", "10000010", "10000011", "10000100", "10000101", "10000110", "10000111", "10001000", "10001001", "10001010", "10001011", "10001100", "10001101", "10001110", "10001111",
							"10010000", "10010001", "10010010", "10010011", "10010100", "10010101", "10010110", "10010111", "10011000", "10011001", "10011010", "10011011", "10011100", "10011101", "10011110", "10011111",
							"10100000", "10100001", "10100010", "10100011", "10100100", "10100101", "10100110", "10100111", "10101000", "10101001", "10101010", "10101011", "10101100", "10101101", "10101110", "10101111",
							"10110000", "10110001", "10110010", "10110011", "10110100", "10110101", "10110110", "10110111", "10111000", "10111001", "10111010", "10111011", "10111100", "10111101", "10111110", "10111111",
							"11000000", "11000001", "11000010", "11000011", "11000100", "11000101", "11000110", "11000111", "11001000", "11001001", "11001010", "11001011", "11001100", "11001101", "11001110", "11001111",
							"11010000", "11010001", "11010010", "11010011", "11010100", "11010101", "11010110", "11010111", "11011000", "11011001", "11011010", "11011011", "11011100", "11011101", "11011110", "11011111",
							"11100000", "11100001", "11100010", "11100011", "11100100", "11100101", "11100110", "11100111", "11101000", "11101001", "11101010", "11101011", "11101100", "11101101", "11101110", "11101111",
							"11110000", "11110001", "11110010", "11110011", "11110100", "11110101", "11110110", "11110111", "11111000", "11111001", "11111010", "11111011", "11111100", "11111101", "11111110", "11111111"};


static string print_address_offset (const size_t inRadix, const ULWord64 inOffset)
{
	const streamsize	maxAddrWidth (sizeof (ULWord64) * 2);
	ostringstream		oss;
	if (inRadix == 8)
		oss << oct;
	else if (inRadix == 10)
		oss << dec;
	else
		oss << hex;
	oss << setfill ('0') << setw (maxAddrWidth) << inOffset << ": ";
	return oss.str ();

}


static ostream & DumpMemory (const void *	pInStartAddress,
							const size_t	inByteCount,
							ostream &		inOutStr		= cout,
							const size_t	inRadix			= 16,
							const size_t	inBytesPerGroup	= 4,
							const size_t	inGroupsPerLine	= 8,
							const size_t	inAddressRadix	= 16,
							const bool		inShowAscii		= true,
							const size_t	inAddrOffset	= 0)
{
	if (pInStartAddress == NULL)
		return inOutStr;
	if (inRadix != 8 && inRadix != 10 && inRadix != 16 && inRadix != 2)
		return inOutStr;
	if (inAddressRadix != 0 && inAddressRadix != 8 && inAddressRadix != 10 && inAddressRadix != 16)
		return inOutStr;
	if (inBytesPerGroup == 0)	//	|| inGroupsPerLine == 0)
		return inOutStr;

	{
		size_t			bytesRemaining		(inByteCount);
		size_t			bytesInThisGroup	(0);
		size_t			groupsInThisRow		(0);
		const unsigned	maxByteWidth		(inRadix == 8 ? 4 : (inRadix == 10 ? 3 : (inRadix == 2 ? 8 : 2)));
		const UByte *	pBuffer				(reinterpret_cast <const UByte *> (pInStartAddress));
		const size_t	asciiBufferSize		(inShowAscii && inGroupsPerLine ? (inBytesPerGroup * inGroupsPerLine + 1) * sizeof (UByte) : 0);	//	Size in bytes, not chars
		UByte *			pAsciiBuffer		(asciiBufferSize ? new UByte [asciiBufferSize / sizeof (UByte)] : NULL);

		if (pAsciiBuffer)
			::memset (pAsciiBuffer, 0, asciiBufferSize);

		if (inGroupsPerLine && inAddressRadix)
			inOutStr << print_address_offset (inAddressRadix, ULWord64 (pBuffer) - ULWord64 (pInStartAddress) + ULWord64 (inAddrOffset));
		while (bytesRemaining)
		{
			if (inRadix == 2)
				inOutStr << gBin [*pBuffer];
			else if (inRadix == 8)
				inOutStr << gOct [*pBuffer];
			else if (inRadix == 10)
				inOutStr << dec << setfill ('0') << setw (int (maxByteWidth)) << *pBuffer;
			else if (inRadix == 16)
				inOutStr << gHexUC [*pBuffer];

			if (pAsciiBuffer)
				pAsciiBuffer [groupsInThisRow * inBytesPerGroup + bytesInThisGroup] = ::isprint (*pBuffer) ? *pBuffer : '.';
			pBuffer++;
			bytesRemaining--;

			bytesInThisGroup++;
			if (bytesInThisGroup >= inBytesPerGroup)
			{
				groupsInThisRow++;
				if (inGroupsPerLine && groupsInThisRow >= inGroupsPerLine)
				{
					if (pAsciiBuffer)
					{
						inOutStr << " " << pAsciiBuffer;
						::memset (pAsciiBuffer, 0, asciiBufferSize);
					}
					inOutStr << endl;
					if (inAddressRadix && bytesRemaining)
						inOutStr << print_address_offset (inAddressRadix, reinterpret_cast <ULWord64> (pBuffer) - reinterpret_cast <ULWord64> (pInStartAddress) + ULWord64 (inAddrOffset));
					groupsInThisRow = 0;
				}	//	if time for new row
				else
					inOutStr << " ";
				bytesInThisGroup = 0;
			}	//	if time for new group
		}	//	loop til no bytes remaining

		if (bytesInThisGroup && bytesInThisGroup < inBytesPerGroup && pAsciiBuffer)
		{
			groupsInThisRow++;
			inOutStr << string ((inBytesPerGroup - bytesInThisGroup) * maxByteWidth + 1, ' ');
		}

		if (groupsInThisRow)
		{
			if (groupsInThisRow < inGroupsPerLine && pAsciiBuffer)
				inOutStr << string (((inGroupsPerLine - groupsInThisRow) * inBytesPerGroup * maxByteWidth + (inGroupsPerLine - groupsInThisRow)), ' ');
			if (pAsciiBuffer)
				inOutStr << pAsciiBuffer;
			inOutStr << endl;
		}
		else if (bytesInThisGroup && bytesInThisGroup < inBytesPerGroup)
			inOutStr << endl;

		if (pAsciiBuffer)
			delete [] pAsciiBuffer;
	}	//	else radix is 16, 10, 8 or 2

	return inOutStr;

}	//	DumpMemory


static bool NTV2PointerBFT (void)
{	//							            1         2         3         4
	//							  01234567890123456789012345678901234567890123
	static const string	str1	("The rain in Spain stays mainly on the plain.");
	static const string	str2	("The rain in Japan stays mainly on the plain.");
	static const string	str3	("APWRAPWR in Spain stays mainly on WRAPWRAPWR");
	NTV2_POINTER		spain	(str1.c_str(), str1.length());
	NTV2_POINTER		japan	(str2.c_str(), str2.length());
	NTV2_POINTER		wrap	(str3.c_str(), str3.length());
	ULWord				firstDiff	(0);
	ULWord				lastDiff	(0);
	SHOULD_BE_TRUE (spain.GetRingChangedByteRange (japan, firstDiff, lastDiff));
	SHOULD_BE_TRUE (firstDiff < lastDiff);
	SHOULD_BE_EQUAL (firstDiff, 12);
	SHOULD_BE_EQUAL (lastDiff, 15);
	SHOULD_BE_TRUE (spain.GetRingChangedByteRange (wrap, firstDiff, lastDiff));
	SHOULD_BE_FALSE (firstDiff < lastDiff);
	SHOULD_BE_EQUAL (firstDiff, 34);
	SHOULD_BE_EQUAL (lastDiff, 7);
	return true;
}


static bool NTV2CopyRasterBFT (void)
{
	if (true)
	{
		//	Test CopyRaster
		const UWord	nDstWidthPixels		(32);
		const UWord	nDstHeightLines		(32);
		const UWord	nDstBytesPerLine	(nDstWidthPixels * 2);
		const UWord	nDstBytes			(nDstBytesPerLine * nDstHeightLines);
		UByte *		pDstRaster			(new UByte [nDstBytes]);
		::memset (pDstRaster, 0xAA, nDstBytes);

		const UWord	nSrcWidthPixels		(16);
		const UWord	nSrcHeightLines		(16);
		const UWord	nSrcBytesPerLine	(nSrcWidthPixels * 2);
		const UWord	nSrcBytes			(nSrcBytesPerLine * nSrcHeightLines);
		UByte *		pSrcRaster			(new UByte [nSrcBytes]);
		::memset (pSrcRaster, 0xBB, nSrcBytes);
		//								 DumpMemory (buffer,     nBytes,    stream, radix, bytesPerGroup,   groupsPerLine, addrRadix, showAscii, addrOffset);
		cerr << "SrcRaster:" << endl;  ::DumpMemory (pSrcRaster, nSrcBytes,   cerr,    16,             2, nSrcWidthPixels,        16,     false);
		cerr << "DstRaster:" << endl;  ::DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		//				CopyRaster (NTV2FrameBufferFormat, pDstRaster, dstBytesPerLine,  dstTotalLines,  dstVertLineOffset, dstHorzPixelOffset, pSrcRaster, srcBytesPerLine,  srcTotalLines,  srcVertLineOffset, srcVertLinesToCopy, srcHorzPixelOffset, srcHorzPixelsToCopy)
		SHOULD_BE_FALSE (CopyRaster (NTV2_FBF_10BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));
		SHOULD_BE_FALSE (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pDstRaster, nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));
		SHOULD_BE_FALSE (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, NULL,       nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));
		SHOULD_BE_FALSE (CopyRaster ( NTV2_FBF_8BIT_YCBCR,       NULL, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));

		//	Blit src into dst at vert offset 2, horz offset 4
		SHOULD_BE_TRUE  (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                0,    nSrcHeightLines,                  0, nSrcWidthPixels));
		cerr << "DstRaster:" << endl;  ::DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		::memset (pDstRaster, 0xAA, nDstBytes);

		//	Blit lines 1,2,3,4 from src into dst at vert offset 2, horz offset 4
		::memset (pSrcRaster + 0 * nSrcBytesPerLine, 0x00, nSrcBytesPerLine);
		::memset (pSrcRaster + 1 * nSrcBytesPerLine, 0x11, nSrcBytesPerLine);
		::memset (pSrcRaster + 2 * nSrcBytesPerLine, 0x22, nSrcBytesPerLine);
		::memset (pSrcRaster + 3 * nSrcBytesPerLine, 0x33, nSrcBytesPerLine);
		::memset (pSrcRaster + 4 * nSrcBytesPerLine, 0x44, nSrcBytesPerLine);
		cerr << "SrcRaster:" << endl;  DumpMemory (pSrcRaster, nSrcBytes,   cerr,    16,             2, nSrcWidthPixels,        16,     false);
		SHOULD_BE_TRUE  (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                1,                  4,                  0, nSrcWidthPixels));
		cerr << "DstRaster:" << endl;  DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		::memset (pDstRaster, 0xAA, nDstBytes);

		//	Ask to grab pixels past right edge of src raster -- Blit lines 1,2,3,4 from src into dst at vert offset 2, horz offset 4
		SHOULD_BE_TRUE  (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                1,                  4,                  2, nSrcWidthPixels));
		cerr << "DstRaster:" << endl;  DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		::memset (pDstRaster, 0xAA, nDstBytes);
		//	Ask to grab 14 pixels past right edge of src raster -- Blit lines 1,2,3,4 from src into dst at vert offset 2, horz offset 4
		SHOULD_BE_TRUE  (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                1,                  4,                 14, nSrcWidthPixels));
		cerr << "DstRaster:" << endl;  DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		::memset (pDstRaster, 0xAA, nDstBytes);
		//	Request all pixels past right edge of src raster -- fail
		SHOULD_BE_FALSE (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,                1,                  4,                 16, nSrcWidthPixels));
		cerr << "DstRaster:" << endl;  DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		::memset (pDstRaster, 0xAA, nDstBytes);

		//	Ask to grab lines past bottom edge of src raster -- Blit last 4 lines from src into dst at vert offset 2, horz offset 4
		::memset (pSrcRaster, 0xBB, nSrcBytes);
		::memset (pSrcRaster + 11 * nSrcBytesPerLine, 0x11, nSrcBytesPerLine);
		::memset (pSrcRaster + 12 * nSrcBytesPerLine, 0x22, nSrcBytesPerLine);
		::memset (pSrcRaster + 13 * nSrcBytesPerLine, 0x33, nSrcBytesPerLine);
		::memset (pSrcRaster + 14 * nSrcBytesPerLine, 0x44, nSrcBytesPerLine);
		::memset (pSrcRaster + 15 * nSrcBytesPerLine, 0x55, nSrcBytesPerLine);
		cerr << "SrcRaster:" << endl;  DumpMemory (pSrcRaster, nSrcBytes,   cerr,    16,             2, nSrcWidthPixels,        16,     false);
		SHOULD_BE_TRUE  (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,               12,                  4,                  0, nSrcWidthPixels));
		cerr << "DstRaster:" << endl;  DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		::memset (pDstRaster, 0xAA, nDstBytes);
		//	Ask to grab lines past bottom edge of src raster -- Blit last 4 lines from src into dst at vert offset 2, horz offset 4
		SHOULD_BE_TRUE  (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,                2,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,               12,    nSrcHeightLines,                  0, nSrcWidthPixels));
		cerr << "DstRaster:" << endl;  DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		::memset (pDstRaster, 0xAA, nDstBytes);
		//	Blit past bottom of dst raster -- Blit src into dst at vert offset 30, horz offset 4
		SHOULD_BE_TRUE  (CopyRaster ( NTV2_FBF_8BIT_YCBCR, pDstRaster, nDstBytesPerLine, nDstHeightLines,               30,                  4, pSrcRaster, nSrcBytesPerLine, nSrcHeightLines,               12,    nSrcHeightLines,                  0, nSrcWidthPixels));
		cerr << "DstRaster:" << endl;  DumpMemory (pDstRaster, nDstBytes,   cerr,    16,             2, nDstWidthPixels,        16,     false);
		::memset (pDstRaster, 0xAA, nDstBytes);

		delete pSrcRaster;
		delete pDstRaster;
	}
	return true;

}	//	NTV2CopyRasterBFT


static bool NTV2DebugBFT (void)
{
	{
		const NTV2DeviceType	deviceTypes []	= {DEVICETYPE_UNKNOWN, DEVICETYPE_NTV2};
		const string			devTypeStrs []	= {"DEVICETYPE_UNKNOWN", "DEVICETYPE_NTV2"};
		for (unsigned ndx (0);  ndx < sizeof (deviceTypes) / sizeof (NTV2DeviceType);  ndx++)
		{
			SHOULD_BE_UNEQUAL (::NTV2DeviceTypeString (deviceTypes [ndx]), NULL);	//	never NULL!
			SHOULD_BE_EQUAL (string (::NTV2DeviceTypeString (deviceTypes [ndx])), devTypeStrs [ndx]);
		}
	}
	{
		const NTV2DeviceID		deviceIDs []	= {	DEVICE_ID_NOTFOUND,		DEVICE_ID_CORVID1,		DEVICE_ID_CORVID22,		DEVICE_ID_CORVID24,		DEVICE_ID_CORVID3G,
													DEVICE_ID_CORVID44,		DEVICE_ID_CORVID88,		DEVICE_ID_CORVIDHEVC,	DEVICE_ID_CORVIDHBR,
													DEVICE_ID_IO4K,			DEVICE_ID_IO4KUFC,		DEVICE_ID_IOEXPRESS,	DEVICE_ID_IOXT,
													DEVICE_ID_KONA3G,		DEVICE_ID_KONA3GQUAD,	DEVICE_ID_KONA4,		DEVICE_ID_KONA4UFC,		DEVICE_ID_KONAIP_4CH_1SFP,
													DEVICE_ID_KONAIP_4CH_2SFP,		DEVICE_ID_KONALHEPLUS,	DEVICE_ID_KONALHI,		DEVICE_ID_TTAP,			NTV2DeviceID (1234567)	};
		const string			devIDStrs []	= {	"DEVICE_ID_NOTFOUND",	"DEVICE_ID_CORVID1",	"DEVICE_ID_CORVID22",	"DEVICE_ID_CORVID24",	"DEVICE_ID_CORVID3G",
													"DEVICE_ID_CORVID44",	"DEVICE_ID_CORVID88",	"DEVICE_ID_CORVIDHEVC",	"DEVICE_ID_CORVIDHBR",
													"DEVICE_ID_IO4K",		"DEVICE_ID_IO4KUFC",	"DEVICE_ID_IOEXPRESS",	"DEVICE_ID_IOXT",
													"DEVICE_ID_KONA3G",		"DEVICE_ID_KONA3GQUAD",	"DEVICE_ID_KONA4",		"DEVICE_ID_KONA4UFC",	"DEVICE_ID_KONAIP_4CH_1SFP",
													"DEVICE_ID_KONAIP_4CH_2SFP",	"DEVICE_ID_KONALHEPLUS","DEVICE_ID_KONALHI",	"DEVICE_ID_TTAP",		""						};
		const string			deviceStrs []	= {	"Unknown",				"Corvid1",				"Corvid22",				"Corvid24",				"Corvid3G",
													"Corvid44",				"Corvid88",				"CorvidHEVC",			"CorvidHDBT",
													"Io4K",					"Io4KUfc",				"IoExpress",			"IoXT",
													"Kona3G",				"Kona3GQuad",			"Kona4",				"Kona4Ufc",				"KonaIP_4ch_1sfp",
													"KonaIP_4ch_2sfp",		"KonaLHePlus",			"KonaLHi",				"TTap",					""						};
		for (unsigned ndx (0);  ndx < sizeof (deviceIDs) / sizeof (NTV2DeviceID);  ndx++)
		{
			SHOULD_BE_UNEQUAL	(::NTV2DeviceIDString (deviceIDs [ndx]), NULL);	//	never NULL!
			SHOULD_BE_EQUAL		(string (::NTV2DeviceIDString (deviceIDs [ndx])), devIDStrs [ndx]);
			SHOULD_BE_EQUAL		(string (::NTV2DeviceString (deviceIDs [ndx])), deviceStrs [ndx]);
		}
	}
	{
		const NTV2Standard		standards []	= {	NTV2_STANDARD_1080,			NTV2_STANDARD_720,		NTV2_STANDARD_525,			NTV2_STANDARD_625,
													NTV2_STANDARD_1080p,		NTV2_STANDARD_2K,		NTV2_NUM_STANDARDS,			NTV2_STANDARD_UNDEFINED,
													NTV2_STANDARD_INVALID		};
		const string			stdStrs []		= {	"NTV2_STANDARD_1080",		"NTV2_STANDARD_720",	"NTV2_STANDARD_525",		"NTV2_STANDARD_625",
													"NTV2_STANDARD_1080p",		"NTV2_STANDARD_2K",		"NTV2_STANDARD_INVALID",	"NTV2_STANDARD_INVALID",
													"NTV2_STANDARD_INVALID"	};
		const bool				valid []	= 	{	true,						true,					true,						true,
													true,						true,					false,						false,
													false					};
		const bool				isProg []	= 	{	false,						true,					false,						false,
													true,						false,					false,						false,
													false					};
		for (unsigned ndx (0);  ndx < sizeof (standards) / sizeof (NTV2Standard);  ndx++)
		{
			SHOULD_BE_EQUAL (NTV2_IS_VALID_STANDARD (standards [ndx]), valid [ndx]);
			SHOULD_BE_UNEQUAL (::NTV2StandardString (standards [ndx]), NULL);	//	never NULL!
			SHOULD_BE_EQUAL (string (::NTV2StandardString (standards [ndx])), stdStrs [ndx]);
			SHOULD_BE_EQUAL (NTV2_IS_PROGRESSIVE_STANDARD (standards [ndx]), isProg [ndx]);
		}
	}
	{
		const NTV2FrameBufferFormat	fbfs []	= {	NTV2_FBF_10BIT_YCBCR,		NTV2_FBF_8BIT_YCBCR,		NTV2_FBF_ARGB,				NTV2_FBF_RGBA,
												NTV2_FBF_10BIT_RGB,			NTV2_FBF_8BIT_YCBCR_YUY2,	NTV2_FBF_ABGR,				NTV2_FBF_10BIT_DPX,
												NTV2_FBF_10BIT_YCBCR_DPX,	NTV2_FBF_8BIT_DVCPRO,		NTV2_FBF_8BIT_QREZ,			NTV2_FBF_8BIT_HDV,
												NTV2_FBF_24BIT_RGB,			NTV2_FBF_24BIT_BGR,			NTV2_FBF_10BIT_YCBCRA,		NTV2_FBF_10BIT_DPX_LITTLEENDIAN,
												NTV2_FBF_48BIT_RGB,			NTV2_FBF_PRORES,			NTV2_FBF_PRORES_DVCPRO,		NTV2_FBF_PRORES_HDV,
												NTV2_FBF_10BIT_RGB_PACKED,	NTV2_FBF_10BIT_ARGB,		NTV2_FBF_16BIT_ARGB,		NTV2_FBF_10BIT_RAW_RGB,
												NTV2_FBF_10BIT_RAW_YCBCR,	NTV2_FBF_10BIT_YCBCR_420PL,	NTV2_FBF_10BIT_YCBCR_422PL,	NTV2_FBF_8BIT_YCBCR_420PL,
												NTV2_FBF_8BIT_YCBCR_422PL,	NTV2_FBF_UNUSED_23,			NTV2_FBF_UNUSED_26,			NTV2_FBF_UNUSED_27,
												NTV2_FBF_INVALID	};
		const string			fbfStrs []	= {	"NTV2_FBF_10BIT_YCBCR",			"NTV2_FBF_8BIT_YCBCR",			"NTV2_FBF_ARGB",				"NTV2_FBF_RGBA",
												"NTV2_FBF_10BIT_RGB",			"NTV2_FBF_8BIT_YCBCR_YUY2",		"NTV2_FBF_ABGR",				"NTV2_FBF_10BIT_DPX",
												"NTV2_FBF_10BIT_YCBCR_DPX",		"NTV2_FBF_8BIT_DVCPRO",			"NTV2_FBF_8BIT_QREZ",			"NTV2_FBF_8BIT_HDV",
												"NTV2_FBF_24BIT_RGB",			"NTV2_FBF_24BIT_BGR",			"NTV2_FBF_10BIT_YCBCRA",		"NTV2_FBF_10BIT_DPX_LITTLEENDIAN",
												"NTV2_FBF_48BIT_RGB",			"NTV2_FBF_PRORES",				"NTV2_FBF_PRORES_DVCPRO",		"NTV2_FBF_PRORES_HDV",
												"NTV2_FBF_10BIT_RGB_PACKED",	"NTV2_FBF_10BIT_ARGB",			"NTV2_FBF_16BIT_ARGB",			"NTV2_FBF_10BIT_RAW_RGB",
												"NTV2_FBF_10BIT_RAW_YCBCR",		"NTV2_FBF_10BIT_YCBCR_420PL",	"NTV2_FBF_10BIT_YCBCR_422PL",	"NTV2_FBF_8BIT_YCBCR_420PL",
												"NTV2_FBF_8BIT_YCBCR_422PL",	"NTV2_FBF_INVALID",				"NTV2_FBF_INVALID",				"NTV2_FBF_INVALID",
												"NTV2_FBF_INVALID"	};
		const bool				valid []	= {	true,							true,							true,							true,
												true,							true,							true,							true,
												true,							true,							true,							true,
												true,							true,							true,							true,
												true,							true,							true,							true,
												true,							true,							true,							true,
												true,							true,							true,							true,
												true,							false,							false,							false,
												false		};
		const bool				isPlanar []	= {	false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							true,							true,							true,
												true,							false,							false,							false,
												false		};
		const bool				isProRes []	= {	false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							true,							true,							true,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false		};
		const bool				isRGB []	= {	false,							false,							true,							true,
												true,							false,							true,							true,
												false,							false,							false,							false,
												true,							true,							false,							true,
												true,							false,							false,							false,
												true,							true,							true,							true,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false		};
		const bool				hasAlpha []	= {	false,							false,							true,							true,//3
												false,							false,							true,							false,//7
												false,							false,							false,							false,//11
												false,							false,							true,							false,//15
												false,							false,							false,							false,//19
												false,							true,							true,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false		};
		const bool				isRaw []	= {	false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							false,
												false,							false,							false,							true,
												true,							false,							false,							false,
												false,							false,							false,							false,
												false		};
		for (unsigned ndx (0);  ndx < sizeof (fbfs) / sizeof (NTV2FrameBufferFormat);  ndx++)
		{
			SHOULD_BE_EQUAL (NTV2_IS_VALID_FRAME_BUFFER_FORMAT (fbfs [ndx]), valid [ndx]);
			SHOULD_BE_UNEQUAL (::NTV2FrameBufferFormatString (fbfs [ndx]), NULL);	//	never NULL!
			SHOULD_BE_EQUAL (string (::NTV2FrameBufferFormatString (fbfs [ndx])), fbfStrs [ndx]);
			SHOULD_BE_EQUAL (NTV2_IS_FBF_PLANAR (fbfs [ndx]), isPlanar [ndx]);
			SHOULD_BE_EQUAL (NTV2_IS_FBF_PRORES (fbfs [ndx]), isProRes [ndx]);
			SHOULD_BE_EQUAL (NTV2_IS_FBF_RGB (fbfs [ndx]), isRGB [ndx]);
			SHOULD_BE_EQUAL (NTV2_FBF_HAS_ALPHA (fbfs [ndx]), hasAlpha [ndx]);
			SHOULD_BE_EQUAL (NTV2_FBF_IS_RAW (fbfs [ndx]), isRaw [ndx]);
		}
	}
	{
		const NTV2FrameGeometry	geometries []	= {	NTV2_FG_1920x1080,		NTV2_FG_1280x720,		NTV2_FG_720x486,	NTV2_FG_720x576,
													NTV2_FG_1920x1114,		NTV2_FG_2048x1114,		NTV2_FG_720x508,	NTV2_FG_720x598,
													NTV2_FG_1920x1112,		NTV2_FG_1280x740,		NTV2_FG_2048x1080,	NTV2_FG_2048x1556,
													NTV2_FG_2048x1588,		NTV2_FG_2048x1112,		NTV2_FG_720x514,	NTV2_FG_720x612,
													NTV2_FG_4x1920x1080,	NTV2_FG_4x2048x1080,	NTV2_FG_INVALID		};
		const string			geomStrs []		= {	"NTV2_FG_1920x1080",	"NTV2_FG_1280x720",		"NTV2_FG_720x486",		"NTV2_FG_720x576",
													"NTV2_FG_1920x1114",	"NTV2_FG_2048x1114",	"NTV2_FG_720x508",		"NTV2_FG_720x598",
													"NTV2_FG_1920x1112",	"NTV2_FG_1280x740",		"NTV2_FG_2048x1080",	"NTV2_FG_2048x1556",
													"NTV2_FG_2048x1588",	"NTV2_FG_2048x1112",	"NTV2_FG_720x514",		"NTV2_FG_720x612",
													"NTV2_FG_4x1920x1080",	"NTV2_FG_4x2048x1080",	"NTV2_FG_INVALID"		};
		const bool				valid []	= 	{	true,					true,					true,		true,
													true,					true,					true,		true,
													true,					true,					true,		true,
													true,					true,					true,		true,
													true,					true,					false				};
		const bool				isQuad []	= 	{	false,					false,					false,		false,
													false,					false,					false,		false,
													false,					false,					false,		false,
													false,					false,					false,		false,
													true,					true,					false				};
		const bool				is2K1080 []	= 	{	false,					false,					false,		false,
													false,					true,					false,		false,
													false,					false,					true,		false,
													false,					true,					false,		false,
													false,					false,					false				};
		for (unsigned ndx (0);  ndx < sizeof (geometries) / sizeof (NTV2FrameGeometry);  ndx++)
		{
			SHOULD_BE_EQUAL (NTV2_IS_VALID_NTV2FrameGeometry (geometries [ndx]), valid [ndx]);
			SHOULD_BE_UNEQUAL (::NTV2FrameGeometryString (geometries [ndx]), NULL);	//	never NULL!
			SHOULD_BE_EQUAL (string (::NTV2FrameGeometryString (geometries [ndx])), geomStrs [ndx]);
			SHOULD_BE_EQUAL (NTV2_IS_QUAD_FRAME_GEOMETRY (geometries [ndx]), isQuad [ndx]);
			SHOULD_BE_EQUAL (NTV2_IS_2K_1080_FRAME_GEOMETRY (geometries [ndx]), is2K1080 [ndx]);
		}
	}
	{
		const NTV2FrameRate		rates []		= {	NTV2_FRAMERATE_12000,		NTV2_FRAMERATE_11988,		NTV2_FRAMERATE_6000,		NTV2_FRAMERATE_5994,
													NTV2_FRAMERATE_5000,		NTV2_FRAMERATE_4800,		NTV2_FRAMERATE_4795,		NTV2_FRAMERATE_3000,
													NTV2_FRAMERATE_2997,		NTV2_FRAMERATE_2500,		NTV2_FRAMERATE_2400,		NTV2_FRAMERATE_2398,
													NTV2_FRAMERATE_1900,		NTV2_FRAMERATE_1898,		NTV2_FRAMERATE_1800,		NTV2_FRAMERATE_1798,
													NTV2_FRAMERATE_1500,		NTV2_FRAMERATE_1498,		NTV2_FRAMERATE_INVALID		};
		const string			rateStrs []		= {	"NTV2_FRAMERATE_12000",		"NTV2_FRAMERATE_11988",		"NTV2_FRAMERATE_6000",		"NTV2_FRAMERATE_5994",
													"NTV2_FRAMERATE_5000",		"NTV2_FRAMERATE_4800",		"NTV2_FRAMERATE_4795",		"NTV2_FRAMERATE_3000",
													"NTV2_FRAMERATE_2997",		"NTV2_FRAMERATE_2500",		"NTV2_FRAMERATE_2400",		"NTV2_FRAMERATE_2398",
													"NTV2_FRAMERATE_1900",		"NTV2_FRAMERATE_1898",		"NTV2_FRAMERATE_1800",		"NTV2_FRAMERATE_1798",
													"NTV2_FRAMERATE_1500",		"NTV2_FRAMERATE_1498",		"NTV2_FRAMERATE_INVALID"	};
		const bool				valid []	= 	{	true,						true,						true,						true,
													true,						true,						true,						true,
													true,						true,						true,						true,
													true,						true,						true,						true,
													true,						true,						false						};
		for (unsigned ndx (0);  ndx < sizeof (rates) / sizeof (NTV2FrameRate);  ndx++)
		{
			SHOULD_BE_EQUAL (NTV2_IS_VALID_NTV2FrameRate (rates [ndx]), valid [ndx]);
			SHOULD_BE_UNEQUAL (::NTV2FrameRateString (rates [ndx]), NULL);	//	never NULL!
			SHOULD_BE_EQUAL (string (::NTV2FrameRateString (rates [ndx])), rateStrs [ndx]);
		}
	}
	{
		const NTV2VideoFormat	formats []		= {	NTV2_FORMAT_1080i_5000,											NTV2_FORMAT_1080psf_2500,
													NTV2_FORMAT_1080i_5994,											NTV2_FORMAT_1080psf_2997,
													NTV2_FORMAT_1080i_6000,											NTV2_FORMAT_1080psf_3000,
													NTV2_FORMAT_720p_5994,		NTV2_FORMAT_720p_6000,				NTV2_FORMAT_1080psf_2398,		NTV2_FORMAT_1080psf_2400,
													NTV2_FORMAT_1080p_2997,		NTV2_FORMAT_1080p_3000,				NTV2_FORMAT_1080p_2500,			NTV2_FORMAT_1080p_2398,
													NTV2_FORMAT_1080p_2400,		NTV2_FORMAT_1080p_2K_2398,			NTV2_FORMAT_1080p_2K_2400,		NTV2_FORMAT_1080p_2K_2500,
													NTV2_FORMAT_1080p_2K_2997,	NTV2_FORMAT_1080p_2K_3000,
													NTV2_FORMAT_1080p_2K_4795,										NTV2_FORMAT_1080p_2K_4795_A,
													NTV2_FORMAT_1080p_2K_4800,										NTV2_FORMAT_1080p_2K_4800_A,
													NTV2_FORMAT_1080p_2K_5000,										NTV2_FORMAT_1080p_2K_5000_A,
													NTV2_FORMAT_1080p_2K_5994,										NTV2_FORMAT_1080p_2K_5994_A,
													NTV2_FORMAT_1080p_2K_6000,										NTV2_FORMAT_1080p_2K_6000_A,
													NTV2_FORMAT_1080p_2K_4795_B,	NTV2_FORMAT_1080p_2K_4800_B,	NTV2_FORMAT_1080p_2K_5000_B,	NTV2_FORMAT_1080p_2K_5994_B,	NTV2_FORMAT_1080p_2K_6000_B,
													NTV2_FORMAT_1080psf_2K_2398,	NTV2_FORMAT_1080psf_2K_2400,	NTV2_FORMAT_1080psf_2K_2500,	NTV2_FORMAT_1080psf_2500_2,		NTV2_FORMAT_1080psf_2997_2,
													NTV2_FORMAT_1080psf_3000_2,		NTV2_FORMAT_720p_5000,			NTV2_FORMAT_1080psf_3000_2,		NTV2_FORMAT_720p_5000,
													NTV2_FORMAT_1080p_5000_B,										NTV2_FORMAT_1080p_5000,
													NTV2_FORMAT_1080p_5994_B,										NTV2_FORMAT_1080p_5994,
													NTV2_FORMAT_1080p_6000_B,										NTV2_FORMAT_1080p_6000,
													NTV2_FORMAT_1080p_5000_A,	NTV2_FORMAT_1080p_5994_A,			NTV2_FORMAT_1080p_6000_A,
													NTV2_FORMAT_END_HIGH_DEF_FORMATS,
													NTV2_FORMAT_720p_2398,		NTV2_FORMAT_720p_2500,				NTV2_FORMAT_525_5994,			NTV2_FORMAT_625_5000,
													NTV2_FORMAT_525_2398,		NTV2_FORMAT_525_2400,				NTV2_FORMAT_525psf_2997,		NTV2_FORMAT_625psf_2500,
													NTV2_FORMAT_END_STANDARD_DEF_FORMATS,
													NTV2_FORMAT_2K_1498,		NTV2_FORMAT_2K_1500,				NTV2_FORMAT_2K_2398,			NTV2_FORMAT_2K_2400,			NTV2_FORMAT_2K_2500,
													NTV2_FORMAT_END_2K_DEF_FORMATS,
													NTV2_FORMAT_4x1920x1080psf_2398,	NTV2_FORMAT_4x1920x1080psf_2400,	NTV2_FORMAT_4x1920x1080psf_2500,	NTV2_FORMAT_4x1920x1080psf_2997,
													NTV2_FORMAT_4x1920x1080psf_3000,	NTV2_FORMAT_4x1920x1080p_2398,		NTV2_FORMAT_4x1920x1080p_2400,		NTV2_FORMAT_4x1920x1080p_2500,
													NTV2_FORMAT_4x1920x1080p_2997,		NTV2_FORMAT_4x1920x1080p_3000,		NTV2_FORMAT_4x2048x1080psf_2398,	NTV2_FORMAT_4x2048x1080psf_2400,
													NTV2_FORMAT_4x2048x1080psf_2500,	NTV2_FORMAT_4x2048x1080psf_2997,	NTV2_FORMAT_4x2048x1080psf_3000,	NTV2_FORMAT_4x2048x1080p_2398,
													NTV2_FORMAT_4x2048x1080p_2400,		NTV2_FORMAT_4x2048x1080p_2500,		NTV2_FORMAT_4x2048x1080p_2997,		NTV2_FORMAT_4x2048x1080p_3000,
													NTV2_FORMAT_4x2048x1080p_4795,		NTV2_FORMAT_4x2048x1080p_4800,		NTV2_FORMAT_4x1920x1080p_5000,		NTV2_FORMAT_4x1920x1080p_5994,
													NTV2_FORMAT_4x1920x1080p_6000,		NTV2_FORMAT_4x2048x1080p_5000,		NTV2_FORMAT_4x2048x1080p_5994,		NTV2_FORMAT_4x2048x1080p_6000,
													NTV2_FORMAT_4x2048x1080p_11988,		NTV2_FORMAT_4x2048x1080p_12000,
													NTV2_FORMAT_END_HIGH_DEF_FORMATS2,
													NTV2_FORMAT_UNKNOWN	};
		const string			fmtStrs []		= {	"NTV2_FORMAT_1080i_5000  /  NTV2_FORMAT_1080psf_2500",			"NTV2_FORMAT_1080i_5000  /  NTV2_FORMAT_1080psf_2500",
													"NTV2_FORMAT_1080i_5994  /  NTV2_FORMAT_1080psf_2997",			"NTV2_FORMAT_1080i_5994  /  NTV2_FORMAT_1080psf_2997",
													"NTV2_FORMAT_1080i_6000  /  NTV2_FORMAT_1080psf_3000",			"NTV2_FORMAT_1080i_6000  /  NTV2_FORMAT_1080psf_3000",
													"NTV2_FORMAT_720p_5994",	"NTV2_FORMAT_720p_6000",			"NTV2_FORMAT_1080psf_2398",		"NTV2_FORMAT_1080psf_2400",
													"NTV2_FORMAT_1080p_2997",	"NTV2_FORMAT_1080p_3000",			"NTV2_FORMAT_1080p_2500",		"NTV2_FORMAT_1080p_2398",
													"NTV2_FORMAT_1080p_2400",	"NTV2_FORMAT_1080p_2K_2398",		"NTV2_FORMAT_1080p_2K_2400",	"NTV2_FORMAT_1080p_2K_2500",
													"NTV2_FORMAT_1080p_2K_2997",	"NTV2_FORMAT_1080p_2K_3000",
													"NTV2_FORMAT_1080p_2K_4795  /  NTV2_FORMAT_1080p_2K_4795_A",	"NTV2_FORMAT_1080p_2K_4795  /  NTV2_FORMAT_1080p_2K_4795_A",
													"NTV2_FORMAT_1080p_2K_4800  /  NTV2_FORMAT_1080p_2K_4800_A",	"NTV2_FORMAT_1080p_2K_4800  /  NTV2_FORMAT_1080p_2K_4800_A",
													"NTV2_FORMAT_1080p_2K_5000  /  NTV2_FORMAT_1080p_2K_5000_A",	"NTV2_FORMAT_1080p_2K_5000  /  NTV2_FORMAT_1080p_2K_5000_A",
													"NTV2_FORMAT_1080p_2K_5994  /  NTV2_FORMAT_1080p_2K_5994_A",	"NTV2_FORMAT_1080p_2K_5994  /  NTV2_FORMAT_1080p_2K_5994_A",
													"NTV2_FORMAT_1080p_2K_6000  /  NTV2_FORMAT_1080p_2K_6000_A",	"NTV2_FORMAT_1080p_2K_6000  /  NTV2_FORMAT_1080p_2K_6000_A",
													"NTV2_FORMAT_1080p_2K_4795_B",	"NTV2_FORMAT_1080p_2K_4800_B",	"NTV2_FORMAT_1080p_2K_5000_B",	"NTV2_FORMAT_1080p_2K_5994_B",	"NTV2_FORMAT_1080p_2K_6000_B",
													"NTV2_FORMAT_1080psf_2K_2398",	"NTV2_FORMAT_1080psf_2K_2400",	"NTV2_FORMAT_1080psf_2K_2500",	"NTV2_FORMAT_1080psf_2500_2",	"NTV2_FORMAT_1080psf_2997_2",
													"NTV2_FORMAT_1080psf_3000_2",	"NTV2_FORMAT_720p_5000",		"NTV2_FORMAT_1080psf_3000_2",	"NTV2_FORMAT_720p_5000",
													"NTV2_FORMAT_1080p_5000_B  /  NTV2_FORMAT_1080p_5000",			"NTV2_FORMAT_1080p_5000_B  /  NTV2_FORMAT_1080p_5000",
													"NTV2_FORMAT_1080p_5994_B  /  NTV2_FORMAT_1080p_5994",			"NTV2_FORMAT_1080p_5994_B  /  NTV2_FORMAT_1080p_5994",
													"NTV2_FORMAT_1080p_6000_B  /  NTV2_FORMAT_1080p_6000",			"NTV2_FORMAT_1080p_6000_B  /  NTV2_FORMAT_1080p_6000",
													"NTV2_FORMAT_1080p_5000_A",	"NTV2_FORMAT_1080p_5994_A",			"NTV2_FORMAT_1080p_6000_A",
													"NTV2_FORMAT_UNKNOWN",
													"NTV2_FORMAT_720p_2398",	"NTV2_FORMAT_720p_2500",			"NTV2_FORMAT_525_5994",		"NTV2_FORMAT_625_5000",
													"NTV2_FORMAT_525_2398",		"NTV2_FORMAT_525_2400",				"NTV2_FORMAT_525psf_2997",	"NTV2_FORMAT_625psf_2500",
													"NTV2_FORMAT_UNKNOWN",
													"NTV2_FORMAT_2K_1498",		"NTV2_FORMAT_2K_1500",				"NTV2_FORMAT_2K_2398",		"NTV2_FORMAT_2K_2400",				"NTV2_FORMAT_2K_2500",
													"NTV2_FORMAT_UNKNOWN",
													"NTV2_FORMAT_4x1920x1080psf_2398",	"NTV2_FORMAT_4x1920x1080psf_2400",	"NTV2_FORMAT_4x1920x1080psf_2500",	"NTV2_FORMAT_4x1920x1080psf_2997",
													"NTV2_FORMAT_4x1920x1080psf_3000",	"NTV2_FORMAT_4x1920x1080p_2398",	"NTV2_FORMAT_4x1920x1080p_2400",	"NTV2_FORMAT_4x1920x1080p_2500",
													"NTV2_FORMAT_4x1920x1080p_2997",	"NTV2_FORMAT_4x1920x1080p_3000",	"NTV2_FORMAT_4x2048x1080psf_2398",	"NTV2_FORMAT_4x2048x1080psf_2400",
													"NTV2_FORMAT_4x2048x1080psf_2500",	"NTV2_FORMAT_4x2048x1080psf_2997",	"NTV2_FORMAT_4x2048x1080psf_3000",	"NTV2_FORMAT_4x2048x1080p_2398",
													"NTV2_FORMAT_4x2048x1080p_2400",	"NTV2_FORMAT_4x2048x1080p_2500",	"NTV2_FORMAT_4x2048x1080p_2997",	"NTV2_FORMAT_4x2048x1080p_3000",
													"NTV2_FORMAT_4x2048x1080p_4795",	"NTV2_FORMAT_4x2048x1080p_4800",	"NTV2_FORMAT_4x1920x1080p_5000",	"NTV2_FORMAT_4x1920x1080p_5994",
													"NTV2_FORMAT_4x1920x1080p_6000",	"NTV2_FORMAT_4x2048x1080p_5000",	"NTV2_FORMAT_4x2048x1080p_5994",	"NTV2_FORMAT_4x2048x1080p_6000",
													"NTV2_FORMAT_4x2048x1080p_11988",	"NTV2_FORMAT_4x2048x1080p_12000",
													"NTV2_FORMAT_UNKNOWN",
													"NTV2_FORMAT_UNKNOWN"	};
		const bool				valid []		= {	true,															true,
													true,															true,
													true,															true,
													true,						true,								true,							true,
													true,						true,								true,							true,
													true,						true,								true,							true,
													true,						true,
													true,															true,
													true,															true,
													true,															true,
													true,															true,
													true,															true,
													true,						true,								true,							true,							true,
													true,						true,								true,							true,							true,
													true,						true,								true,							true,
													true,															true,
													true,															true,
													true,															true,
													true,						true,								true,
													false,
													true,						true,								true,							true,
													true,						true,								true,							true,
													false,
													true,						true,								true,							true,							true,
													false,
													true,						true,								true,							true,
													true,						true,								true,							true,
													true,						true,								true,							true,
													true,						true,								true,							true,
													true,						true,								true,							true,
													true,						true,								true,							true,
													true,						true,								true,							true,
													true,						true,
													false,
													false	};
//	TODO:	Validate IS_HD, IS_SD, IS_720P, IS_2K, IS_2K_1080, IS_4K, IS_4K_HFR, IS_QUAD_FRAME, IS_4K_4096, IS_4K_QUADHD, IS_372_DL, IS_525, IS_625, IS_INTERMEDIATE, IS_3G, IS_3Gb, IS_WIRE, IS_PSF, IS_PROGRESSIVE, IS_A
		SHOULD_BE_EQUAL (sizeof (formats) / sizeof (NTV2VideoFormat),  sizeof (fmtStrs) / sizeof (string));
		for (unsigned ndx (0);  ndx < sizeof (formats) / sizeof (NTV2VideoFormat);  ndx++)
		{
			//cerr << " " << ndx << " " << ::NTV2VideoFormatString (formats [ndx]) << " == " << fmtStrs [ndx] << endl;
			SHOULD_BE_EQUAL (NTV2_IS_VALID_VIDEO_FORMAT (formats [ndx]), valid [ndx]);
			SHOULD_BE_UNEQUAL (::NTV2VideoFormatString (formats [ndx]), NULL);	//	never NULL!
			SHOULD_BE_EQUAL (string (::NTV2VideoFormatString (formats [ndx])), fmtStrs [ndx]);
		}
	}

//	TODO:	Validate ::NTV2RegisterNameString
	return true;

}	//	NTV2DebugBFT

/*
{
	static const NTV2Standard			gStandards[]= {	NTV2_STANDARD_1080,		NTV2_STANDARD_720,		NTV2_STANDARD_525,		NTV2_STANDARD_625,			NTV2_STANDARD_1080p,
														NTV2_STANDARD_2K,		NTV2_STANDARD_2Kx1080p,	NTV2_STANDARD_2Kx1080i,	NTV2_STANDARD_3840x2160p,	NTV2_STANDARD_4096x2160p,
														NTV2_STANDARD_3840HFR,	NTV2_STANDARD_4096HFR,	NTV2_STANDARD_INVALID	};
	static const NTV2FrameBufferFormat	gFBFs[]		= {	NTV2_FBF_10BIT_YCBCR,	NTV2_FBF_8BIT_YCBCR,	NTV2_FBF_ARGB,			NTV2_FBF_RGBA,
														NTV2_FBF_10BIT_RGB,		NTV2_FBF_ABGR,			NTV2_FBF_24BIT_RGB,		NTV2_FBF_48BIT_RGB,		NTV2_FBF_INVALID	};
	static const NTV2VANCMode			gVancModes[]= {	NTV2_VANCMODE_OFF,		NTV2_VANCMODE_TALL, 	NTV2_VANCMODE_TALLER,	NTV2_VANCMODE_INVALID	};

	cerr	<< setw(20) << "NTV2Standard" << "\t" << setw(22) << "NTV2FrameBufferFormat"
			<< "\t" << " VANC?" << "\t" << setw(5) << "[MB]" << endl;
	for (unsigned stdNdx (0);  stdNdx < NTV2_NUM_STANDARDS;  stdNdx++)
	{
		const NTV2Standard	videoStandard	(gStandards [stdNdx]);
		for (unsigned fbfNdx (0);  NTV2_IS_VALID_FRAME_BUFFER_FORMAT (gFBFs[fbfNdx]);  fbfNdx++)
		{
			const NTV2FrameBufferFormat	fbf	(gFBFs [fbfNdx]);
			for (unsigned vancNdx (0);  vancNdx < 3;  vancNdx++)
			{
				static const NTV2VANCMode	vancMode	(gVancModes [vancNdx]);
				const NTV2FormatDescriptor	fd			(videoStandard, fbf, vancMode);
				if (!fd.IsValid ())
					cerr	<< setw(20) << ::NTV2StandardToString (videoStandard) << "\t" << setw(22) << ::NTV2FrameBufferFormatToString (fbf)
							<< "\t" << ::NTV2VANCModeToString (vancMode) << "\t** FAILED **  NTV2FormatDescriptor invalid" << endl;
				SHOULD_BE_TRUE (fd.IsValid ());
				cerr	<< setw(20) << ::NTV2StandardToString (videoStandard) << "\t" << setw(22) << ::NTV2FrameBufferFormatToString (fbf)
						<< "\t" << ::NTV2VANCModeToString (vancMode) << "\t" << setw(5) << setprecision(2) << (double (fd.GetTotalRasterBytes ()) / double (1024 * 1024)) << endl;
			}	//	for no-VANC, tall-VANC, taller-VANC
		}	//	for each frame buffer format
	}	//	for each video standard

	cerr	<< endl
			<< setw(21) << "NTV2VideoFormat" << "\t" << setw(22) << "NTV2FrameBufferFormat"
			<< "\t" << " VANC?" << "\t" << setw(5) << " [MB]"
			<< "\t" << setw (12) << "FullRaster" << "\t" << setw(12) << "VisRaster" << endl;
	for (NTV2VideoFormat videoFormat (NTV2_FORMAT_UNKNOWN);  videoFormat < NTV2_MAX_NUM_VIDEO_FORMATS;  videoFormat = NTV2VideoFormat (videoFormat + 1))
	{
		if (!NTV2_IS_VALID_VIDEO_FORMAT (videoFormat))
			continue;
		for (unsigned fbfNdx (0);  NTV2_IS_VALID_FRAME_BUFFER_FORMAT (gFBFs[fbfNdx]);  fbfNdx++)
		{
			const NTV2FrameBufferFormat	fbf	(gFBFs [fbfNdx]);
			for (unsigned normalTallTaller (0);  normalTallTaller < 3;  normalTallTaller++)
			{
				static const string	sTall[]	=	{	"normal",	"tall  ",	"taller"};
				const bool	isTall		(normalTallTaller > 0);
				const bool	isTaller	(normalTallTaller > 1);
		 		const NTV2FormatDescriptor	fd	(::GetFormatDescriptor (videoFormat, fbf, isTall, isTaller));
				SHOULD_BE_TRUE (fd.IsValid ());
				SHOULD_BE_EQUAL (fd.IsVANC (), isTall || isTaller);
				cerr	<< setw(21) << ::NTV2VideoFormatToString (videoFormat) << "\t" << setw(22) << ::NTV2FrameBufferFormatToString (fbf)
						<< "\t" << sTall[normalTallTaller] << "\t" << setw(5) << setprecision(2) << (double (fd.GetTotalRasterBytes ()) / double (1024 * 1024))
						<< setw (12) << fd.GetFullRasterDimensions() << "\t" << setw(12) << fd.GetFullRasterDimensions() << endl;
			}	//	for no-VANC, tall-VANC, taller-VANC
		}	//	for each frame buffer format
	}	//	for each video format
}
*/

static bool NTV2AudioChannelsPairQuadOctetTest (void)
{
	{
		NTV2AudioChannelPairs	nonPcmPairs, oldNonPcmPairs;
		NTV2AudioChannelPairs	whatsNew, whatsGone;
		SHOULD_BE_TRUE (nonPcmPairs.empty ());
		SHOULD_BE_TRUE (equal (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin()));
		oldNonPcmPairs.insert (NTV2_AudioChannel7_8);
		oldNonPcmPairs.insert (NTV2_AudioChannel11_12);
		oldNonPcmPairs.insert (NTV2_AudioChannel57_58);
		nonPcmPairs.insert (NTV2_AudioChannel3_4);

		set_difference (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin(), nonPcmPairs.end(),  inserter (whatsGone, whatsGone.begin()));
		set_difference (nonPcmPairs.begin(), nonPcmPairs.end(),  oldNonPcmPairs.begin(), oldNonPcmPairs.end(),  inserter (whatsNew, whatsNew.begin()));
		//if (!whatsNew.empty ())
		//	cerr << "Whats new:  " << whatsNew << endl;
		SHOULD_BE_FALSE (whatsNew.empty ());
		SHOULD_BE_EQUAL (whatsNew.size (), 1);
		SHOULD_BE_EQUAL (*whatsNew.begin (), NTV2_AudioChannel3_4);
		//if (!whatsGone.empty ())
		//	cerr << "Whats gone:  " << whatsGone.size() << ":  " << whatsGone << endl;
		SHOULD_BE_FALSE (whatsGone.empty ());
		SHOULD_BE_EQUAL (whatsGone.size (), 3);
		SHOULD_BE_EQUAL (*whatsGone.begin (), NTV2_AudioChannel7_8);
		SHOULD_BE_EQUAL (*whatsGone.rbegin (), NTV2_AudioChannel57_58);
	}
	{
		NTV2AudioChannelQuads	nonPcmPairs, oldNonPcmPairs;
		NTV2AudioChannelQuads	whatsNew, whatsGone;
		SHOULD_BE_TRUE (nonPcmPairs.empty ());
		SHOULD_BE_TRUE (equal (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin()));
		oldNonPcmPairs.insert (NTV2_AudioChannel5_8);
		oldNonPcmPairs.insert (NTV2_AudioChannel37_40);
		oldNonPcmPairs.insert (NTV2_AudioChannel125_128);
		nonPcmPairs.insert (NTV2_AudioChannel1_4);

		set_difference (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin(), nonPcmPairs.end(),  inserter (whatsGone, whatsGone.begin()));
		set_difference (nonPcmPairs.begin(), nonPcmPairs.end(),  oldNonPcmPairs.begin(), oldNonPcmPairs.end(),  inserter (whatsNew, whatsNew.begin()));
		//if (!whatsNew.empty ())
		//	cerr << "Whats new:  " << whatsNew << endl;
		SHOULD_BE_FALSE (whatsNew.empty ());
		SHOULD_BE_EQUAL (whatsNew.size (), 1);
		SHOULD_BE_EQUAL (*whatsNew.begin (), NTV2_AudioChannel1_4);
		//if (!whatsGone.empty ())
		//	cerr << "Whats gone:  " << whatsGone << endl;
		SHOULD_BE_FALSE (whatsGone.empty ());
		SHOULD_BE_EQUAL (whatsGone.size (), 3);
		SHOULD_BE_EQUAL (*whatsGone.begin (), NTV2_AudioChannel5_8);
		SHOULD_BE_EQUAL (*whatsGone.rbegin (), NTV2_AudioChannel125_128);
	}
	{
		NTV2AudioChannelOctets	nonPcmPairs, oldNonPcmPairs;
		NTV2AudioChannelOctets	whatsNew, whatsGone;
		SHOULD_BE_TRUE (nonPcmPairs.empty ());
		SHOULD_BE_TRUE (equal (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin()));
		oldNonPcmPairs.insert (NTV2_AudioChannel9_16);
		oldNonPcmPairs.insert (NTV2_AudioChannel73_80);
		oldNonPcmPairs.insert (NTV2_AudioChannel121_128);
		nonPcmPairs.insert (NTV2_AudioChannel1_8);

		set_difference (oldNonPcmPairs.begin(), oldNonPcmPairs.end(), nonPcmPairs.begin(), nonPcmPairs.end(),  inserter (whatsGone, whatsGone.begin()));
		set_difference (nonPcmPairs.begin(), nonPcmPairs.end(),  oldNonPcmPairs.begin(), oldNonPcmPairs.end(),  inserter (whatsNew, whatsNew.begin()));
		//if (!whatsNew.empty ())
		//	cerr << "Whats new:  " << whatsNew << endl;
		SHOULD_BE_FALSE (whatsNew.empty ());
		SHOULD_BE_EQUAL (whatsNew.size (), 1);
		SHOULD_BE_EQUAL (*whatsNew.begin (), NTV2_AudioChannel1_8);
		//if (!whatsGone.empty ())
		//	cerr << "Whats gone:  " << whatsGone << endl;
		SHOULD_BE_FALSE (whatsGone.empty ());
		SHOULD_BE_EQUAL (whatsGone.size (), 3);
		SHOULD_BE_EQUAL (*whatsGone.begin (), NTV2_AudioChannel9_16);
		SHOULD_BE_EQUAL (*whatsGone.rbegin (), NTV2_AudioChannel121_128);
	}

	//	Spot-check the Pair/Quad/Octet-To-String conversions...
	SHOULD_BE_EQUAL (::NTV2AudioChannelPairToString (NTV2_AudioChannel57_58), "NTV2_AudioChannel57_58");
	SHOULD_BE_EQUAL (::NTV2AudioChannelQuadToString (NTV2_AudioChannel37_40), "NTV2_AudioChannel37_40");
	SHOULD_BE_EQUAL (::NTV2AudioChannelOctetToString (NTV2_AudioChannel121_128), "NTV2_AudioChannel121_128");
	return true;
}	//	NTV2AudioChannelsPairQuadOctetTest


static bool NTV2UtilsBFT (void)
{
	SHOULD_BE_TRUE (NTV2CopyRasterBFT ());
	for (NTV2FrameBufferFormat fbf (NTV2_FBF_10BIT_YCBCR);  fbf < NTV2_FBF_NUMFRAMEBUFFERFORMATS;  fbf = NTV2FrameBufferFormat (fbf + 1))
		if (NTV2_IS_VALID_FRAME_BUFFER_FORMAT (fbf))
		{
			SHOULD_BE_EQUAL (::CalcRowBytesForFormat (fbf, 0), 0);
		}
	return true;
}


static bool NTV2BFT (void)
{
	SHOULD_BE_TRUE (::NTV2DeviceFeaturesBFT ());
	SHOULD_BE_TRUE (::NTV2PointerBFT ());
	SHOULD_BE_TRUE (::NTV2DebugBFT ());
	SHOULD_BE_TRUE (::NTV2UtilsBFT ());
	SHOULD_BE_TRUE (::NTV2AudioChannelsPairQuadOctetTest ());
	return true;
}


int main (int argc, const char * argv [])
{
    return ::NTV2BFT () ? 0 : 2;
}
