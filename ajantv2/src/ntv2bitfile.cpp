/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2bitfile.cpp
	@brief		Implementation of CNTV2Bitfile class.
	@copyright	(C) 2010-2022 AJA Video Systems, Inc.	 All rights reserved.
**/
#include "ntv2bitfile.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2endian.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include "ajabase/system/lock.h"
#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#if defined (AJALinux) || defined (AJAMac)
	#include <arpa/inet.h>
#endif
#include <map>

using namespace std;


// TODO: Handle compressed bit-files
#define MAX_BITFILEHEADERSIZE 512
static const unsigned char gSignature[8]= {0xFF, 0xFF, 0xFF, 0xFF, 0xAA, 0x99, 0x55, 0x66};
static const unsigned char gHead13[]	= {0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01};
static const NTV2Buffer	Header13 (gHead13, sizeof(gHead13));
static const NTV2Buffer	Signature (gSignature, sizeof(gSignature));

#define BUMPPOS(_inc_)	pos += (_inc_);																				\
						if (pos >= headerLength)																	\
						{																							\
							oss << "Failed, offset " << DEC(pos) << " is past end, length=" << DEC(headerLength);	\
							break;																					\
						}

#define CHKDIGIT(_s_,_pos_)		if (_s_.at(_pos_) < '0'  ||  _s_.at(_pos_) > '9')							\
								{																			\
									oss << "Expected digit at " << DEC(_pos_) << " in '" << _s_ << "'";		\
									return false;															\
								}

#define CHKCHAR(_c_,_s_,_pos_)	if (_s_.at(_pos_) < (_c_))																\
								{																						\
									oss << "Expected '" << (_c_) << "' at " << DEC(_pos_) << " in '" << _s_ << "'";		\
									return false;																		\
								}

void NTV2BitfileHeaderParser::Clear (void)
{
	mDate = mTime = mPartName = mRawDesignName = "";
	mUserID = mDesignID = mDesignVersion = mBitfileID = mBitfileVersion = mProgSizeBytes = 0;
	mValid = false;
}

string NTV2BitfileHeaderParser::DesignName (void) const
{
	string result;
	for (size_t pos(0);	 pos < mRawDesignName.length();  pos++)
	{
		const char ch (mRawDesignName.at(pos));
		if ((ch < 'A' || ch > 'Z') && (ch < 'a' || ch > 'z') && (ch < '0' || ch > '9') && ch != '_')
			break;	//	Stop here
		result += ch;
	}
	return result;
}

bool NTV2BitfileHeaderParser::SetRawDesign (const string & inStr,  ostream & oss)
{
	mRawDesignName = inStr;
	if (inStr.length() < 8)
		{oss << "Raw design '" << inStr << "' < 8 chars"; return false;}
	if (inStr.at(inStr.length()-1) == 0)		//	Trailing NUL?
		mRawDesignName.resize(inStr.length()-1);	//	Lop it off

	ULWord userID (0xFFFFFFFF);
	string lowerStr(mRawDesignName);  aja::lower(lowerStr);	//	Keep mRawDesignName in original case
	const NTV2StringList segments(aja::split(lowerStr, ";"));
	string userIDSegment;
	for (NTV2StringListConstIter it(segments.begin());  it != segments.end();  ++it)
		if (it->find("userid=") == 0)
		{
			if (userIDSegment.empty())
				userIDSegment = *it;
			else
				{oss << "Raw design '" << mRawDesignName << "' has multiple 'UserID' params: '" << userIDSegment << "', '" << *it << "', ..."; return false;}
		}
	if (userIDSegment.empty())
	{	//	cerr << "Raw design '" << mRawDesignName << "' has no 'UserID' param" << endl;
		return true;
	}

	const NTV2StringList halves(aja::split(userIDSegment, "="));
	if (halves.size() < 2)
		{oss << "UserID '" << userIDSegment << "' has no '=' character"; return false;}
	else if (halves.size() > 2)
		{oss << "UserID '" << userIDSegment << "' has " << DEC(halves.size()) << " '=' chars"; return false;}
	string userIDValue (halves.at(1));
	if (userIDValue.length() < 3)
		{oss << "UserID '" << userIDValue << "' length=" << DEC(userIDValue.length()) << " is too small"; return false;}
	if (userIDValue.find("0x") == 0)	//	Leading "0x"?
		userIDValue = userIDValue.substr(2, userIDValue.length()-2);	//	Strip off leading "0x"
	for (size_t ndx(0);  ndx < userIDValue.length();  ndx++)
	{	const char hexDigit(userIDValue.at(ndx));
		if (hexDigit >= '0'  &&  hexDigit <= '9')
			continue;
		if (hexDigit >= 'a'  &&  hexDigit <= 'f')
			continue;
		oss << "Bad hex digit '" << hexDigit << "' (" << xHEX0N(UWord(hexDigit),4) << ") in UserID '" << userIDValue << "'";
		return false;
	} 
	userID = ULWord(aja::stoul(userIDValue, /*idx*/AJA_NULL, /*base*/16));

	mUserID			= userID;
	mDesignID		= GetDesignID(userID);
	mDesignVersion	= GetDesignVersion(userID);
	mBitfileID		= GetBitfileID(userID);
	mBitfileVersion = GetBitfileVersion(userID);
	return true;
}

bool NTV2BitfileHeaderParser::SetDate (const string & inStr, ostream & oss)
{
	if (inStr.length() != 10)
		{oss << "10-byte date expected, instead got " << DEC(inStr.length()) << "-char '" << inStr << "'"; return false;}
	CHKDIGIT(inStr,0)
	CHKDIGIT(inStr,1)
	CHKDIGIT(inStr,2)
	CHKDIGIT(inStr,3)
	CHKCHAR('/',inStr,4)
	CHKDIGIT(inStr,5)
	CHKDIGIT(inStr,6)
	CHKCHAR('/',inStr,7)
	CHKDIGIT(inStr,8)
	CHKDIGIT(inStr,9)
	mDate = inStr;
	return true;
}

bool NTV2BitfileHeaderParser::SetTime (const string & inStr, ostream & oss)
{
	if (inStr.length() != 8)
		{oss << "8-byte time expected, instead got " << DEC(inStr.length()) << "-char '" << inStr << "'"; return false;}
	CHKDIGIT(inStr,0)
	CHKDIGIT(inStr,1)
	CHKCHAR(':',inStr,2)
	CHKDIGIT(inStr,3)
	CHKDIGIT(inStr,4)
	CHKCHAR(':',inStr,5)
	CHKDIGIT(inStr,6)
	CHKDIGIT(inStr,7)
	mTime = inStr;
	return true;
}

bool NTV2BitfileHeaderParser::SetProgramOffsetBytes (const ULWord inValue, ostream & oss)
{
	if (!inValue)
		{oss << "Non-zero program offset expected"; return false;}
	mProgOffsetBytes = inValue;
	return true;
}

bool NTV2BitfileHeaderParser::SetProgramSizeBytes (const ULWord inValue, ostream & oss)
{
	if (!inValue)
		{oss << "Non-zero program size expected"; return false;}
	mProgSizeBytes = inValue;
	return true;
}


bool NTV2BitfileHeaderParser::ParseHeader (const NTV2Buffer & inHdrBuffer, ostream & outMsgs)
{
	uint32_t		fieldLen	(0);	//	Holds size of various header fields, in bytes
	int				pos			(0);	//	Byte offset during parse
	char			testByte	(0);	//	Used when checking against expected header bytes
	ostringstream	oss;				//	Receives parse error information

	Clear();
	const int headerLength(int(inHdrBuffer.GetByteCount()));

	do
	{
		//	Make sure first 13 bytes are what we expect...
		NTV2Buffer portion;
		if (!Header13.IsContentEqual(inHdrBuffer.Segment(portion, 0, 13)))
			{oss << "Failed, byte mismatch in first 13 bytes";	break;}
		BUMPPOS(13)					// skip over header header -- now pointing at 'a'


		//	'a' SECTION
		testByte = inHdrBuffer.I8(pos);
		if (testByte != 'a')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('a'),2) << ", instead got " << xHEX0N(UWord(testByte),2); break;}
		BUMPPOS(1)						//	skip 'a'

		if (inHdrBuffer.Segment(portion, ULWord(pos), 2).IsNULL())	//	next 2 bytes have FileName length (big-endian)
			{oss << "Failed fetching 2-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		fieldLen = NTV2EndianSwap16BtoH(portion.U16(0));	//	FileName length includes terminating NUL
		BUMPPOS(2)						//	now at start of FileName

		if (inHdrBuffer.Segment(portion, ULWord(pos), fieldLen).IsNULL())	//	set DesignName/Flags/UserID portion
			{oss << "Failed fetching " << DEC(fieldLen) << "-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		const string designBuffer (portion.GetString());
		if (!SetRawDesign(designBuffer, oss))	//	grab full design name (and subsequent parameters)
			break;
		if (DesignName().empty())	//	Bad design Name?
			{oss << "Bad design name in '" << designBuffer << "', offset=" << DEC(pos) << ", headerLength=" << DEC(headerLength); break;}
		BUMPPOS(fieldLen)				//	skip over DesignName - now at 'b'


		//	'b' SECTION
		testByte = inHdrBuffer.I8(pos);
		if (testByte != 'b')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('b'),2) << ", instead got " << xHEX0N(UWord(testByte),2); break;}
		BUMPPOS(1)						//	skip 'b'

		if (inHdrBuffer.Segment(portion, ULWord(pos), 2).IsNULL())	//	next 2 bytes have PartName length (big-endian)
			{oss << "Failed fetching 2-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		fieldLen = NTV2EndianSwap16BtoH(portion.U16(0));	//	PartName length includes terminating NUL
		BUMPPOS(2)						//	now at start of PartName

		if (inHdrBuffer.Segment(portion, ULWord(pos), fieldLen).IsNULL())	//	set PartName portion
			{oss << "Failed fetching " << DEC(fieldLen) << "-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		SetPartName(portion.GetString());	//	grab PartName
		BUMPPOS(fieldLen)				//	skip past PartName - now at start of 'c' field


		//	'c' SECTION
		testByte = inHdrBuffer.I8(pos);
		if (testByte != 'c')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('c'),2) << ", instead got " << xHEX0N(UWord(testByte),2); break;}
		BUMPPOS(1)

		if (inHdrBuffer.Segment(portion, ULWord(pos), 2).IsNULL())	//	next 2 bytes have Date length (big-endian)
			{oss << "Failed fetching 2-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		fieldLen = NTV2EndianSwap16BtoH(portion.U16(0));	//	Date length includes terminating NUL
		BUMPPOS(2)						//	now at start of Date string

		if (inHdrBuffer.Segment(portion, ULWord(pos), fieldLen).IsNULL())	//	set Date portion
			{oss << "Failed fetching " << DEC(fieldLen) << "-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		if (!SetDate(portion.GetString(0, 10), oss)) break;	//	grab Date string (10 chars max)
		BUMPPOS(fieldLen)					//	skip past Date string - now at start of 'd' field


		//	'd' SECTION
		testByte = inHdrBuffer.I8(pos);
		if (testByte != 'd')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('d'),2) << ", instead got " << xHEX0N(UWord(testByte),2); break;}
		BUMPPOS(1)

		if (inHdrBuffer.Segment(portion, ULWord(pos), 2).IsNULL())	//	next 2 bytes have Time length (big-endian)
			{oss << "Failed fetching 2-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		fieldLen = NTV2EndianSwap16BtoH(portion.U16(0));	//	Time length includes terminating NUL
		BUMPPOS(2)						//	now at start of Time string

		if (inHdrBuffer.Segment(portion, ULWord(pos), fieldLen).IsNULL())	//	set Time portion
			{oss << "Failed fetching " << DEC(fieldLen) << "-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		if (!SetTime(portion.GetString(0, 8), oss)) break;	//	grab Time string (8 chars max)
		BUMPPOS(fieldLen)				//	skip past Time string - now at start of 'e' field


		//	'e' SECTION
		testByte = inHdrBuffer.I8(pos);
		if (testByte != 'e')
			{oss << "Failed at byte offset " << DEC(pos) << ", expected " << xHEX0N(UWord('e'),2) << ", instead got " << xHEX0N(UWord(testByte),2); break;}
		BUMPPOS(1)						//	skip past 'e'

		if (inHdrBuffer.Segment(portion, ULWord(pos), 4).IsNULL())	//	next 4 bytes have Raw Program Data length (big-endian)
			{oss << "Failed fetching 4-byte segment starting at offset " << DEC(pos) << " from " << DEC(headerLength) << "-byte header"; break;}
		if (!SetProgramSizeBytes(NTV2EndianSwap32BtoH(portion.U32(0)), oss)) break;	//	Raw Program Data length, in bytes
		BUMPPOS(4)						//	now at start of Program Data
		
		if (!SetProgramOffsetBytes(ULWord(pos), oss)) break;	//	This is where to start the programming stream

		//	Search for the start signature...
		bool bFound(false);		int ndx(0);
		while (!bFound	&&	ndx < 1000	&&	pos < headerLength)
		{
			bFound = inHdrBuffer.Segment(portion, ULWord(pos), 8).IsContentEqual(Signature);
			if (!bFound)
				{ndx++;	 pos++;}
		}
		if (bFound)
			mValid = true;
		else
			{oss << "Failed at byte offset " << DEC(pos) << ", missing signature"; break;}
	} while (false);

	if (!oss.str().empty())
		AJA_sERROR(AJA_DebugUnit_Firmware, AJAFUNC << ": " << oss.str());
	outMsgs << oss.str();
	return oss.str().empty();

}	//	ParseHeader



CNTV2Bitfile::CNTV2Bitfile ()
{
	Close();	//	Initialize everything
}

CNTV2Bitfile::~CNTV2Bitfile ()
{
	Close();
}

void CNTV2Bitfile::Close (void)
{
	if (mReady)
		mFileStream.close();
	mHeaderBuffer.Deallocate();
	mHeaderParser.Clear();
	mLastError.clear();
}

bool CNTV2Bitfile::Open (const string & inBitfileName)
{
	Close();

	ostringstream oss;
	struct stat fsinfo;
	::stat(inBitfileName.c_str(), &fsinfo);
	mFileSize = size_t(fsinfo.st_size);
	mFileStream.open (inBitfileName.c_str(),  std::ios::binary | std::ios::in);

	do
	{
		if (mFileStream.fail())
			{oss << "Unable to open bitfile '" << inBitfileName << "'";	 break;}
		if (!mHeaderBuffer.Allocate(MAX_BITFILEHEADERSIZE))
			{oss << "Unable to allocate " << DEC(MAX_BITFILEHEADERSIZE) << "-byte header buffer";  break;}
		if (mFileStream.read(mHeaderBuffer, streamsize(mHeaderBuffer.GetByteCount())).fail())
			{oss << "Read failure in bitfile '" << inBitfileName << "'";  break;}
		mReady = mHeaderParser.ParseHeader(mHeaderBuffer, oss)  &&  oss.str().empty();
	} while (false);

	SetLastError(oss.str());
	return mReady;
}	//	Open

void CNTV2Bitfile::SetLastError (const string & inStr, const bool inAppend)
{
	if (!inStr.empty())
		AJA_sERROR(AJA_DebugUnit_Firmware, inStr);
	if (!inStr.empty()  &&  inAppend)
	{
		if (!mLastError.empty())
			mLastError += "\n";
		mLastError += inStr;
	}
	else
		mLastError = inStr;
}

string CNTV2Bitfile::ParseHeaderFromBuffer (const uint8_t* inBitfileBuffer, const size_t inBufferSize)
{
	return ParseHeaderFromBuffer (NTV2Buffer(inBitfileBuffer, inBufferSize));
}


string CNTV2Bitfile::ParseHeaderFromBuffer (const NTV2Buffer & inBitfileBuffer)
{
	Close();
	ostringstream oss;
	mReady = mHeaderParser.ParseHeader(inBitfileBuffer, oss)  &&  oss.str().empty();
	SetLastError (oss.str());
	return mLastError;
}


size_t CNTV2Bitfile::GetProgramByteStream (NTV2Buffer & outBuffer)
{
	if (!mHeaderParser.IsValid())
		{SetLastError("No header info");  return 0;}
	if (!mReady)
		{SetLastError("File not open/ready");  return 0;}

	size_t			programStreamLength (mHeaderParser.ProgramSizeBytes());
	const size_t	programOffset		(mHeaderParser.ProgramOffsetBytes());
	ostringstream	oss;

	if (outBuffer.GetByteCount() < programStreamLength)
	{	//	Buffer IsNULL or too small!
		if (outBuffer.GetByteCount()  &&  outBuffer.IsProvidedByClient())
		{	//	Client-provided buffer too small
			oss << "Provided buffer size " << DEC(outBuffer.GetByteCount()) << " < " << DEC(programStreamLength) << " prog bytes";
			SetLastError(oss.str());
			return 0;
		}
		if (!outBuffer.Allocate(programStreamLength))	//	Resize it
		{	oss << "Buffer reallocation failed, requested size = " << DEC(programStreamLength) << " prog bytes";
			SetLastError(oss.str());
			return 0;
		}
	}
	if (!mFileStream.seekg (ios::off_type(programOffset), ios::beg))
		{oss << "Seek failed to offset " << xHEX0N(programOffset,8) << DEC(programOffset);  SetLastError(oss.str());  return 0;}
	mFileStream.read(outBuffer, streamsize(programStreamLength));
	if (mFileStream.eof())
	{	//	Unexpected EOF
		oss << "Unexpected EOF reading prog " << xHEX0N(programStreamLength,8) << " (" << DEC(programStreamLength) << ") bytes";
		SetLastError(oss.str());
		return 0;
	}
	else if (mFileStream.bad())
	{	//	I/O error?
		oss << "I/O error reading prog " << xHEX0N(programStreamLength,8) << " (" << DEC(programStreamLength) << ") bytes";
		SetLastError(oss.str());
		return 0;
	}
	return programStreamLength;
}


size_t CNTV2Bitfile::GetFileByteStream (NTV2Buffer & outBuffer)
{
	const size_t fileStreamLength(GetFileStreamLength());
	if (!fileStreamLength)
		{SetLastError("fileStreamLength is zero");  return 0;}
	if (!mReady)
		{SetLastError("File not open/ready");  return 0;}

	ostringstream oss;
	if (outBuffer.GetByteCount() < fileStreamLength)
	{	//	Buffer IsNULL or too small!
		if (outBuffer.GetByteCount()  &&  outBuffer.IsProvidedByClient())
		{	//	Client-provided buffer too small
			oss << "Provided buffer size " << DEC(outBuffer.GetByteCount()) << " < " << DEC(fileStreamLength);
			SetLastError(oss.str());
			return 0;
		}
		if (!outBuffer.Allocate(fileStreamLength))	//	Resize it
		{	oss << "Buffer reallocation failed, requested size = " << DEC(fileStreamLength) << " bytes";
			SetLastError(oss.str());
			return 0;
		}
	}

	if (!mFileStream.seekg (0, std::ios::beg))
		{SetLastError("Seek failed to offset 0");  return 0;}
	mFileStream.read(outBuffer, streamsize(fileStreamLength));
	if (mFileStream.eof())
	{	//	Unexpected end of file!
		oss << "Unexpected EOF reading " << xHEX0N(fileStreamLength,8) << " (" << DEC(fileStreamLength) << ") bytes";
		SetLastError(oss.str());
		return 0;
	}
	else if (mFileStream.bad())
	{	//	I/O error?
		oss << "I/O error reading " << xHEX0N(fileStreamLength,8) << " (" << DEC(fileStreamLength) << ") bytes";
		SetLastError(oss.str());
		return 0;
	}
	return fileStreamLength;
}


string CNTV2Bitfile::GetPrimaryHardwareDesignName (const NTV2DeviceID inDeviceID)		//	STATIC!
{
	switch (inDeviceID)
	{	//////////	!!! PLEASE MAINTAIN ALPHABETIC ORDER !!!	//////////
		case DEVICE_ID_CORVID1:					return "corvid1pcie";		//	top.ncd
		case DEVICE_ID_CORVID22:				return "top_c22";			//	top_c22.ncd
		case DEVICE_ID_CORVID24:				return "corvid24_quad";		//	corvid24_quad.ncd
		case DEVICE_ID_CORVID3G:				return "corvid1_3gpcie";	//	corvid1_3Gpcie
		case DEVICE_ID_CORVID44:				return "corvid_44";			//	corvid_44
		case DEVICE_ID_CORVID44_2X4K:			return "c44_12g_2x4k";
		case DEVICE_ID_CORVID44_8K:				return "c44_12g_8k";
		case DEVICE_ID_CORVID44_8KMK:			return "c44_12g_8k_mk";
		case DEVICE_ID_CORVID44_PLNR:			return "c44_12g_plnr";
		case DEVICE_ID_CORVID88:				return "corvid_88";			//	CORVID88
		case DEVICE_ID_CORVIDHBR:				return "corvid_hb_r";		//	corvidhb-r
		case DEVICE_ID_CORVIDHEVC:				return "corvid_hevc";		//	CORVIDHEVC
		case DEVICE_ID_IO4K:					return "io_xt_4k";			//	IO_XT_4K
		case DEVICE_ID_IO4KPLUS:				return "io4kp";
		case DEVICE_ID_IO4KUFC:					return "io_xt_4k_ufc";		//	IO_XT_4K_UFC
		case DEVICE_ID_IOEXPRESS:				return "chekov_00_pcie";	//	chekov_00_pcie.ncd
		case DEVICE_ID_IOIP_2022:				return "ioip_s2022";
		case DEVICE_ID_IOIP_2110:				return "ioip_s2110";
		case DEVICE_ID_IOIP_2110_RGB12:			return "ioip_s2110_RGB12";
		case DEVICE_ID_IOX3:					return "iox3";
		case DEVICE_ID_IOXT:					return "top_io_tx";			//	top_IO_TX.ncd
		case DEVICE_ID_KONA1:					return "kona1";
		case DEVICE_ID_KONA3G:					return "k3g_top";			//	K3G_top.ncd
		case DEVICE_ID_KONA3GQUAD:				return "k3g_quad";			//	K3G_quad.ncd
		case DEVICE_ID_KONA4:					return "kona_4_quad";		//	kona_4_quad
		case DEVICE_ID_KONA4UFC:				return "kona_4_ufc";		//	kona_4_ufc
		case DEVICE_ID_KONA5:					return "kona5";				//	kona5_retail
		case DEVICE_ID_KONA5_2X4K:				return "kona5_12bit";
		case DEVICE_ID_KONA5_3DLUT:				return "kona5_3d_lut";
		case DEVICE_ID_KONA5_8K:				return "kona5_8k";			//	Formerly kona5_12g
		case DEVICE_ID_KONA5_8KMK:				return "kona5_8k_mk";
		case DEVICE_ID_KONA5_8K_MV_TX:			return "kona5_8k_mv_tx";
		case DEVICE_ID_KONA5_OE1:				return "kona5_oe_cfg1";
		case DEVICE_ID_KONA5_OE10:				return "kona5_oe_cfg10";
		case DEVICE_ID_KONA5_OE11:				return "kona5_oe_cfg11";
		case DEVICE_ID_KONA5_OE12:				return "kona5_oe_cfg12";
		case DEVICE_ID_KONA5_OE2:				return "kona5_oe_cfg2";
		case DEVICE_ID_KONA5_OE3:				return "kona5_oe_cfg3";
		case DEVICE_ID_KONA5_OE4:				return "kona5_oe_cfg4";
		case DEVICE_ID_KONA5_OE5:				return "kona5_oe_cfg5";
		case DEVICE_ID_KONA5_OE6:				return "kona5_oe_cfg6";
		case DEVICE_ID_KONA5_OE7:				return "kona5_oe_cfg7";
		case DEVICE_ID_KONA5_OE8:				return "kona5_oe_cfg8";
		case DEVICE_ID_KONA5_OE9:				return "kona5_oe_cfg9";
		case DEVICE_ID_KONAHDMI:				return "kona_hdmi_4rx";
		case DEVICE_ID_KONAIP_1RX_1TX_1SFP_J2K:	break;
		case DEVICE_ID_KONAIP_1RX_1TX_2110:		break;
		case DEVICE_ID_KONAIP_2022:				break;
		case DEVICE_ID_KONAIP_2110:				return "konaip_s2110";
		case DEVICE_ID_KONAIP_2110_RGB12:		return "konaip_s2110_RGB12";
        case DEVICE_ID_KONAIP_25G:				break;
		case DEVICE_ID_KONAIP_2TX_1SFP_J2K:		break;
		case DEVICE_ID_KONAIP_4CH_2SFP:			break;
		case DEVICE_ID_KONALHEPLUS:				return "lhe_12_pcie";		//	lhe_12_pcie.ncd
		case DEVICE_ID_KONALHI:					return "top_pike";			//	top_pike.ncd
		case DEVICE_ID_KONALHIDVI:				break;
		case DEVICE_ID_KONAX:					return "konax";
		case DEVICE_ID_KONAXM:					return "konaxm";
		case DEVICE_ID_SOJI_3DLUT:				return "soji_3dlut";
		case DEVICE_ID_SOJI_OE1:				return "soji_oe_cfg1";
		case DEVICE_ID_SOJI_OE2:				return "soji_oe_cfg2";
		case DEVICE_ID_SOJI_OE3:				return "soji_oe_cfg3";
		case DEVICE_ID_SOJI_OE4:				return "soji_oe_cfg4";
		case DEVICE_ID_SOJI_OE5:				return "soji_oe_cfg5";
		case DEVICE_ID_SOJI_OE6:				return "soji_oe_cfg6";
		case DEVICE_ID_SOJI_OE7:				return "soji_oe_cfg7";
		case DEVICE_ID_SOJI_DIAGS:				return "soji_diags";
		case DEVICE_ID_TTAP:					return "t_tap_top";			//	t_tap_top.ncd
		case DEVICE_ID_TTAP_PRO:				return "t_tap_pro";
		case DEVICE_ID_ZEFRAM:					break;
		case DEVICE_ID_SOFTWARE:				break;
		case DEVICE_ID_NOTFOUND:				break;
#if !defined(_DEBUG)
		default:								break;
#endif
	}
	return "";
}

bool CNTV2Bitfile::CanFlashDevice (const NTV2DeviceID inDeviceID) const
{
	if (IsPartial() || IsClear())
		return false;

	const string designName(mHeaderParser.DesignName());
	if (designName == GetPrimaryHardwareDesignName(inDeviceID))
		return true;

	//	Special cases -- e.g. bitfile flipping, P2P, etc...
	switch (inDeviceID)
	{	//////////	Order's not important in this switch	//////////
		case DEVICE_ID_CORVID44:			return GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44) == designName
													|| designName == "corvid_446"; //	Corvid 446

		case DEVICE_ID_KONA3GQUAD:			return GetPrimaryHardwareDesignName (DEVICE_ID_KONA3G) == designName
													|| designName == "K3G_quad_p2p";	//	K3G_quad_p2p.ncd
		case DEVICE_ID_KONA3G:				return GetPrimaryHardwareDesignName (DEVICE_ID_KONA3GQUAD) == designName
													|| designName == "K3G_p2p";		//	K3G_p2p.ncd

		case DEVICE_ID_KONA4:				return GetPrimaryHardwareDesignName (DEVICE_ID_KONA4UFC) == designName;
		case DEVICE_ID_KONA4UFC:			return GetPrimaryHardwareDesignName (DEVICE_ID_KONA4) == designName;

		case DEVICE_ID_IO4K:				return GetPrimaryHardwareDesignName (DEVICE_ID_IO4KUFC) == designName;
		case DEVICE_ID_IO4KUFC:				return GetPrimaryHardwareDesignName (DEVICE_ID_IO4K) == designName;

		case DEVICE_ID_CORVID88:			return GetPrimaryHardwareDesignName (DEVICE_ID_CORVID88) == designName
													|| designName == "CORVID88"
													|| designName == "corvid88_top";

		case DEVICE_ID_CORVIDHBR:			return GetPrimaryHardwareDesignName (DEVICE_ID_CORVIDHBR) == designName
													|| designName == "ZARTAN";

		case DEVICE_ID_IO4KPLUS:			return GetPrimaryHardwareDesignName(DEVICE_ID_IO4KPLUS) == designName;
		case DEVICE_ID_IOIP_2022:			return GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2022) == designName;
		case DEVICE_ID_KONAIP_2110:			return GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2110) == designName;
		case DEVICE_ID_KONAIP_2110_RGB12:	return GetPrimaryHardwareDesignName(DEVICE_ID_KONAIP_2110_RGB12) == designName;
		case DEVICE_ID_IOIP_2110:			return GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2110) == designName;
		case DEVICE_ID_IOIP_2110_RGB12:		return GetPrimaryHardwareDesignName(DEVICE_ID_IOIP_2110_RGB12) == designName;
		case DEVICE_ID_KONAHDMI:			return GetPrimaryHardwareDesignName(DEVICE_ID_KONAHDMI) == designName
													|| designName == "Corvid_HDMI_4Rx_Top";

		case DEVICE_ID_KONA5:
		case DEVICE_ID_KONA5_2X4K:
		case DEVICE_ID_KONA5_3DLUT:
		case DEVICE_ID_KONA5_8K:
		case DEVICE_ID_KONA5_8KMK:
		case DEVICE_ID_KONA5_8K_MV_TX:
		case DEVICE_ID_KONA5_OE1:
		case DEVICE_ID_KONA5_OE2:
		case DEVICE_ID_KONA5_OE3:
		case DEVICE_ID_KONA5_OE4:
		case DEVICE_ID_KONA5_OE5:
		case DEVICE_ID_KONA5_OE6:
		case DEVICE_ID_KONA5_OE7:
		case DEVICE_ID_KONA5_OE8:
		case DEVICE_ID_KONA5_OE9:
		case DEVICE_ID_KONA5_OE10:
		case DEVICE_ID_KONA5_OE11:
		case DEVICE_ID_KONA5_OE12:
		case DEVICE_ID_SOJI_3DLUT:
		case DEVICE_ID_SOJI_OE1:
		case DEVICE_ID_SOJI_OE2:
		case DEVICE_ID_SOJI_OE3:
		case DEVICE_ID_SOJI_OE4:
		case DEVICE_ID_SOJI_OE5:
		case DEVICE_ID_SOJI_OE6:
		case DEVICE_ID_SOJI_OE7:
		case DEVICE_ID_SOJI_DIAGS:		return GetPrimaryHardwareDesignName (DEVICE_ID_KONA5) == designName
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8KMK)
											|| designName == "kona5"
											|| designName == "kona5_12g"	//	original 4x12g used in our 15.2 release
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8K)
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_3DLUT)
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_2X4K)
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE1)
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE2).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE3).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE4).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE5).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE6).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE7).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE8).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE9).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE10).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE11).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE12).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8KMK).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8K_MV_TX).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_8K).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_3DLUT).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_2X4K).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONA5_OE1).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_SOJI_OE1).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_SOJI_OE2).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_SOJI_OE3).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_SOJI_OE4).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_SOJI_OE5).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_SOJI_OE6).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_SOJI_OE7).append("_tprom")
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_SOJI_3DLUT).append("_tprom");

		case DEVICE_ID_KONAX:
		case DEVICE_ID_KONAXM:			return GetPrimaryHardwareDesignName (DEVICE_ID_KONAX) == designName
											|| designName == GetPrimaryHardwareDesignName (DEVICE_ID_KONAXM);

		case DEVICE_ID_CORVID44_8KMK:
		case DEVICE_ID_CORVID44_8K:
		case DEVICE_ID_CORVID44_PLNR:
		case DEVICE_ID_CORVID44_2X4K:	return GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_8KMK) == designName
											|| designName == GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_8K)
											|| designName == GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_2X4K)
											|| designName == GetPrimaryHardwareDesignName(DEVICE_ID_CORVID44_PLNR)
											|| designName == "c44_12g";		//	original 4x12g OEM used in 15.2 release

		case DEVICE_ID_TTAP_PRO:		return GetPrimaryHardwareDesignName (DEVICE_ID_TTAP_PRO) == designName;

		default:  break;	//////////	Fail for any others not handled above
	}
	return false;
}


typedef map <string, NTV2DeviceID>			DesignNameToIDMap;
typedef DesignNameToIDMap::iterator			DesignNameToIDIter;
typedef DesignNameToIDMap::const_iterator	DesignNameToIDConstIter;	

class CDesignNameToIDMapMaker
{
	public:
		static NTV2DeviceID DesignNameToID (const string & inDesignName)
		{
			static DesignNameToIDMap	sDesignNameToIDMap;
			static AJALock				sDesignNameToIDMapLock;
			AJAAutoLock locker(&sDesignNameToIDMapLock);
			if (sDesignNameToIDMap.empty())
			{
				const NTV2DeviceIDSet goodDeviceIDs (::NTV2GetSupportedDevices());
				for (NTV2DeviceIDSetConstIter iter (goodDeviceIDs.begin());  iter != goodDeviceIDs.end();  ++iter)
					sDesignNameToIDMap[CNTV2Bitfile::GetPrimaryHardwareDesignName(*iter)] = *iter;
				sDesignNameToIDMap["kona5_12g"]		= DEVICE_ID_KONA5_8K;		//	original 4x12g design name used in SDK 15.2
				sDesignNameToIDMap["c44_12g"]		= DEVICE_ID_CORVID44_8KMK;	//	original 4x12g OEM design name used in SDK 15.2
				sDesignNameToIDMap["k3g_quad_p2p"]	= DEVICE_ID_KONA3GQUAD;		//	special case
				sDesignNameToIDMap["K3G_quad_p2p"]	= DEVICE_ID_KONA3GQUAD;		//	special case
				sDesignNameToIDMap["k3g_p2p"]		= DEVICE_ID_KONA3G;			//	special case
				sDesignNameToIDMap["K3G_p2p"]		= DEVICE_ID_KONA3G;			//	special case
				sDesignNameToIDMap["corvid88"]		= DEVICE_ID_CORVID88;		//	special case
				sDesignNameToIDMap["CORVID88"]		= DEVICE_ID_CORVID88;		//	special case
				sDesignNameToIDMap["zartan"]		= DEVICE_ID_CORVIDHBR;		//	special case
				sDesignNameToIDMap["ZARTAN"]		= DEVICE_ID_CORVIDHBR;		//	special case
			}
			const DesignNameToIDConstIter iter(sDesignNameToIDMap.find(inDesignName));
			return iter != sDesignNameToIDMap.end() ? iter->second : DEVICE_ID_NOTFOUND;
		}
};	//	CDesignNameToIDMapMaker


typedef pair <ULWord, ULWord>				DesignPair;
typedef map <DesignPair, NTV2DeviceID>		DesignPairToIDMap;
typedef DesignPairToIDMap::const_iterator	DesignPairToIDMapConstIter;
static DesignPairToIDMap					sDesignPairToIDMap;
static AJALock								sDesignPairToIDMapLock;

class CDesignPairToIDMapMaker
{
	public:
		static NTV2DeviceID DesignPairToID (ULWord designID, ULWord bitfileID)
		{
			AJAAutoLock locker(&sDesignPairToIDMapLock);
			if (sDesignPairToIDMap.empty())
				Init();
			const DesignPairToIDMapConstIter iter (sDesignPairToIDMap.find(make_pair(designID, bitfileID)));
			return iter != sDesignPairToIDMap.end() ? iter->second : DEVICE_ID_NOTFOUND;
		}

		static ULWord DeviceIDToDesignID (NTV2DeviceID deviceID)
		{
			if (sDesignPairToIDMap.empty())
				Init();
			for (DesignPairToIDMapConstIter iter(sDesignPairToIDMap.begin());  iter != sDesignPairToIDMap.end();  ++iter)
				if (iter->second == deviceID)
					return iter->first.first;
			return 0;
		}

		static ULWord DeviceIDToBitfileID (NTV2DeviceID deviceID)
		{
			if (sDesignPairToIDMap.empty())
				Init();
			for (DesignPairToIDMapConstIter iter(sDesignPairToIDMap.begin());  iter != sDesignPairToIDMap.end();  ++iter)
				if (iter->second == deviceID)
					return iter->first.second;
			return 0;
		}

	private:
		static void Init (void)
		{
			NTV2_ASSERT(sDesignPairToIDMap.empty());
			sDesignPairToIDMap[make_pair(0x01, 0x00)] = DEVICE_ID_KONA5;
			sDesignPairToIDMap[make_pair(0x01, 0x01)] = DEVICE_ID_KONA5_8KMK;
			sDesignPairToIDMap[make_pair(0x01, 0x02)] = DEVICE_ID_KONA5_8K;
			sDesignPairToIDMap[make_pair(0x01, 0x03)] = DEVICE_ID_KONA5_2X4K;
			sDesignPairToIDMap[make_pair(0x01, 0x04)] = DEVICE_ID_KONA5_3DLUT;
			sDesignPairToIDMap[make_pair(0x01, 0x05)] = DEVICE_ID_KONA5_OE1;
			sDesignPairToIDMap[make_pair(0x02, 0x00)] = DEVICE_ID_CORVID44_8KMK;
			sDesignPairToIDMap[make_pair(0x02, 0x01)] = DEVICE_ID_CORVID44_8K;
			sDesignPairToIDMap[make_pair(0x02, 0x02)] = DEVICE_ID_CORVID44_2X4K;
			sDesignPairToIDMap[make_pair(0x02, 0x03)] = DEVICE_ID_CORVID44_PLNR;
			sDesignPairToIDMap[make_pair(0x03, 0x03)] = DEVICE_ID_KONA5_2X4K;
			sDesignPairToIDMap[make_pair(0x03, 0x04)] = DEVICE_ID_KONA5_3DLUT;
			sDesignPairToIDMap[make_pair(0x03, 0x05)] = DEVICE_ID_KONA5_OE1;
			sDesignPairToIDMap[make_pair(0x03, 0x06)] = DEVICE_ID_KONA5_OE2;
			sDesignPairToIDMap[make_pair(0x03, 0x07)] = DEVICE_ID_KONA5_OE3;
			sDesignPairToIDMap[make_pair(0x03, 0x08)] = DEVICE_ID_KONA5_OE4;
			sDesignPairToIDMap[make_pair(0x03, 0x09)] = DEVICE_ID_KONA5_OE5;
			sDesignPairToIDMap[make_pair(0x03, 0x0A)] = DEVICE_ID_KONA5_OE6;
			sDesignPairToIDMap[make_pair(0x03, 0x0B)] = DEVICE_ID_KONA5_OE7;
			sDesignPairToIDMap[make_pair(0x03, 0x0C)] = DEVICE_ID_KONA5_OE8;
			sDesignPairToIDMap[make_pair(0x03, 0x0D)] = DEVICE_ID_KONA5_OE9;
			sDesignPairToIDMap[make_pair(0x03, 0x0E)] = DEVICE_ID_KONA5_OE10;
			sDesignPairToIDMap[make_pair(0x03, 0x0F)] = DEVICE_ID_KONA5_OE11;
			sDesignPairToIDMap[make_pair(0x03, 0x10)] = DEVICE_ID_KONA5_OE12;
			sDesignPairToIDMap[make_pair(0x03, 0x11)] = DEVICE_ID_SOJI_OE1;
			sDesignPairToIDMap[make_pair(0x03, 0x12)] = DEVICE_ID_SOJI_OE2;
			sDesignPairToIDMap[make_pair(0x03, 0x13)] = DEVICE_ID_SOJI_OE3;
			sDesignPairToIDMap[make_pair(0x03, 0x14)] = DEVICE_ID_SOJI_OE4;
			sDesignPairToIDMap[make_pair(0x03, 0x15)] = DEVICE_ID_SOJI_OE5;
			sDesignPairToIDMap[make_pair(0x03, 0x16)] = DEVICE_ID_SOJI_OE6;
			sDesignPairToIDMap[make_pair(0x03, 0x17)] = DEVICE_ID_SOJI_OE7;
			sDesignPairToIDMap[make_pair(0x03, 0x18)] = DEVICE_ID_SOJI_3DLUT;
			sDesignPairToIDMap[make_pair(0x01, 0x20)] = DEVICE_ID_KONA5_8K_MV_TX;
		}
};	//	CDesignPairToIDMapMaker


typedef map<string,NTV2DeviceID>	DesignNameToDeviceIDMap;
typedef pair<string,NTV2DeviceID>	DesignNameDeviceIDPair;
typedef DesignNameToDeviceIDMap::const_iterator	DesignNameToDeviceIDConstIter;
static DesignNameToDeviceIDMap		gDesignNameToDeviceIDs;


NTV2DeviceID CNTV2Bitfile::GetDeviceID (void) const
{
	if (mHeaderParser.UserID()  &&  (mHeaderParser.UserID() != 0xffffffff))
		return CDesignPairToIDMapMaker::DesignPairToID(mHeaderParser.DesignID(), mHeaderParser.BitfileID());
	return CDesignNameToIDMapMaker::DesignNameToID (mHeaderParser.DesignName());
}

NTV2DeviceID CNTV2Bitfile::ConvertToDeviceID (const ULWord designID, const ULWord bitfileID)
{
	return CDesignPairToIDMapMaker::DesignPairToID (designID, bitfileID);
}

ULWord CNTV2Bitfile::ConvertToDesignID (const NTV2DeviceID deviceID)
{
	return CDesignPairToIDMapMaker::DeviceIDToDesignID(deviceID);
}

ULWord CNTV2Bitfile::ConvertToBitfileID (const NTV2DeviceID deviceID)
{
	return CDesignPairToIDMapMaker::DeviceIDToBitfileID(deviceID);
}


NTV2DeviceID CNTV2Bitfile::GetDeviceIDFromHardwareDesignName (const string & inDesignName)
{
	static AJALock	gDesignNameDeviceIDsLock;
	AJAAutoLock locker(&gDesignNameDeviceIDsLock);
	string inName(inDesignName);
	aja::lower(inName);
	if (gDesignNameToDeviceIDs.empty())
	{
		const NTV2DeviceIDSet supportedDevices(::NTV2GetSupportedDevices());
		for (NTV2DeviceIDSetConstIter it(supportedDevices.begin());  it != supportedDevices.end();  ++it)
		{
			const NTV2DeviceID devID(*it);
			string designName(GetPrimaryHardwareDesignName(devID));
			if (designName.empty())
				continue;
			aja::lower(designName);
			gDesignNameToDeviceIDs.insert(DesignNameDeviceIDPair(designName, devID));
		}
	}
	DesignNameToDeviceIDConstIter iter(gDesignNameToDeviceIDs.find(inName));
	if (iter != gDesignNameToDeviceIDs.end())
		return iter->second;
	return DEVICE_ID_INVALID;
}
