/**
	@file		ntv2captionlogging.cpp
	@brief		Implementation of the CNTV2CaptionLogConfig class.
	@copyright	(C) 2015-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2captionlogging.h"
#include "ntv2publicinterface.h"
#include <sstream>
#include <iomanip>
#include <stdlib.h>
#if defined (AJALinux)
	#include <string.h>	//	for memset
	#include <stdio.h>	//	for EOF
#endif
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"

using namespace std;


#define	MyLabelLock		reinterpret_cast <AJALock *> (mpLabelLock)	//	Keeps the CCLIB headers separate from 'ajalibraries/ajabase'


#if defined (MSWindows)
	#pragma warning(disable: 4800)	//	Ignore int/bool
#endif

static NTV2CaptionLogMask	gDefaultCaptionLogMask	(0);


string Line21RowSetToString (const Line21RowSet & inRowSet)
{
	ostringstream	oss;
	Line21RowSetConstIter iter (inRowSet.begin ());
	while (iter != inRowSet.end ())
	{
		oss << *iter;
		if (++iter != inRowSet.end ())
			oss << ",";
		else
			break;
	}
	return oss.str ();
}


void SetDefaultCaptionLogMask (const NTV2CaptionLogMask inMask)
{
	gDefaultCaptionLogMask = inMask;
}


NTV2CaptionLogMask GetDefaultCaptionLogMask (void)
{
	return gDefaultCaptionLogMask;
}


static string print_address_offset (const size_t inRadix, const ULWord64 inOffset)
{
	const streamsize	maxAddrWidth (sizeof (ULWord64) * 2);
	ostringstream		oss;
	if (inRadix == 8)
		oss << OCT0N(inOffset,maxAddrWidth) << ": ";
	else if (inRadix == 10)
		oss << DEC0N(inOffset,maxAddrWidth) << ": ";
	else
		oss << HEX0N(inOffset,maxAddrWidth) << ": ";
	return oss.str ();
}


string CNTV2CaptionLogConfig::HexDump32Bytes (const void * pInStartAddress, const size_t inByteCount, const size_t inLimitBytes)
{
	if (pInStartAddress == NULL)
		return string();
	ostringstream	oss;
	size_t			bytesRemaining	(inByteCount > inLimitBytes ? inLimitBytes : inByteCount);
	const UByte *	pBuffer			(reinterpret_cast <const UByte *> (pInStartAddress));
	while (bytesRemaining)
	{
		oss << HEX0N(uint16_t(*pBuffer),2);
		pBuffer++;
		bytesRemaining--;
	}
	if (inByteCount > inLimitBytes)
		oss << "...";
	return oss.str();
}


ostream & CNTV2CaptionLogConfig::DumpMemory (const void * pInStartAddress,
											const size_t inByteCount,
											ostream & inOutStr,
											const size_t inRadix,
											const size_t inBytesPerGroup,
											const size_t inGroupsPerRow,
											const size_t inAddressRadix,
											const bool inShowAscii,
											const size_t inAddrOffset)
{
	if (pInStartAddress == NULL)
		return inOutStr;
	if (inRadix != 8 && inRadix != 10 && inRadix != 16 && inRadix != 2)
		return inOutStr;
	if (inAddressRadix != 0 && inAddressRadix != 8 && inAddressRadix != 10 && inAddressRadix != 16)
		return inOutStr;
	if (inBytesPerGroup == 0)	//	|| inGroupsPerRow == 0)
		return inOutStr;

	{
		size_t			bytesRemaining		(inByteCount);
		size_t			bytesInThisGroup	(0);
		size_t			groupsInThisRow		(0);
		const unsigned	maxByteWidth		(inRadix == 8 ? 4 : (inRadix == 10 ? 3 : (inRadix == 2 ? 8 : 2)));
		const UByte *	pBuffer				(reinterpret_cast <const UByte *> (pInStartAddress));
		const size_t	asciiBufferSize		(inShowAscii && inGroupsPerRow ? (inBytesPerGroup * inGroupsPerRow + 1) * sizeof (UByte) : 0);	//	Size in bytes, not chars
		UByte *			pAsciiBuffer		(asciiBufferSize ? new UByte [asciiBufferSize / sizeof (UByte)] : NULL);

		if (pAsciiBuffer)
			::memset (pAsciiBuffer, 0, asciiBufferSize);

		if (inGroupsPerRow && inAddressRadix)
			inOutStr << print_address_offset (inAddressRadix, ULWord64 (pBuffer) - ULWord64 (pInStartAddress) + ULWord64 (inAddrOffset));
		while (bytesRemaining)
		{
			if (inRadix == 2)
				inOutStr << BIN08(*pBuffer);
			else if (inRadix == 8)
				inOutStr << oOCT(uint16_t(*pBuffer));
			else if (inRadix == 10)
				inOutStr << DEC0N(uint16_t(*pBuffer),maxByteWidth);
			else if (inRadix == 16)
				inOutStr << HEX0N(uint16_t(*pBuffer),2);

			if (pAsciiBuffer)
				pAsciiBuffer [groupsInThisRow * inBytesPerGroup + bytesInThisGroup] = ::isprint (*pBuffer) ? *pBuffer : '.';
			pBuffer++;
			bytesRemaining--;

			bytesInThisGroup++;
			if (bytesInThisGroup >= inBytesPerGroup)
			{
				groupsInThisRow++;
				if (inGroupsPerRow && groupsInThisRow >= inGroupsPerRow)
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
			if (groupsInThisRow < inGroupsPerRow && pAsciiBuffer)
				inOutStr << string (((inGroupsPerRow - groupsInThisRow) * inBytesPerGroup * maxByteWidth + (inGroupsPerRow - groupsInThisRow)), ' ');
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


ostream & CNTV2CaptionLogConfig::DumpYBytes_2vuy (const UByte *	pInVideoLine,
												ostream &				inOutputStream,
												const unsigned			inFromPixel,
												const unsigned			inToPixel,
												const bool				inShowRuler,
												const unsigned			inHiliteRangeFrom,
												const unsigned			inHiliteRangeTo)
{
	const unsigned	spacesPerPixel	(3);
	unsigned		fromPixel		(inFromPixel);
	unsigned		toPixel			(inToPixel);
	unsigned		hiliteRangeFrom	(inHiliteRangeFrom);
	unsigned		hiliteRangeTo	(inHiliteRangeTo);
	if (fromPixel > 719)
		fromPixel = 719;
	if (toPixel > 719)
		toPixel = 719;
	if (fromPixel > toPixel)
		return inOutputStream;
	if (!pInVideoLine)
		return inOutputStream;
	if (hiliteRangeFrom != 9999 && hiliteRangeTo != 9999)
	{
		if (hiliteRangeFrom > toPixel)
			hiliteRangeFrom = hiliteRangeTo = 9999;
		if (hiliteRangeFrom < fromPixel)
			hiliteRangeFrom = hiliteRangeTo = 9999;
		if (hiliteRangeTo > toPixel)
			hiliteRangeFrom = hiliteRangeTo = 9999;
		if (hiliteRangeTo < fromPixel)
			hiliteRangeFrom = hiliteRangeTo = 9999;
	}

	if (inShowRuler)
	{
		//	Hundreds
		if (fromPixel)
			inOutputStream << "   ";
		for (unsigned pixNum (fromPixel);  pixNum <= toPixel;  pixNum++)
			if (pixNum % 100)
				inOutputStream << "   ";
			else
				inOutputStream << "  " << pixNum % 1000 / 100;
		inOutputStream << endl;
		//	Tens
		if (fromPixel)
			inOutputStream << "   ";
		for (unsigned pixNum (fromPixel);  pixNum <= toPixel;  pixNum++)
			if (pixNum % 10)
				inOutputStream << "   ";
			else
				inOutputStream << "  " << pixNum % 100 / 10;
		inOutputStream << endl;
		//	Ones
		if (fromPixel)
			inOutputStream << "   ";
		for (unsigned pixNum (fromPixel);  pixNum <= toPixel;  pixNum++)
			inOutputStream << "  " << pixNum % 10;
		inOutputStream << endl;
	}

	if (fromPixel)
		inOutputStream << "...";
	for (unsigned pixNum (fromPixel);  pixNum <= toPixel;  pixNum++)
		inOutputStream << " " << HEX0N(uint16_t(pInVideoLine[pixNum*2+1]),2);
	if (toPixel < 719)
		inOutputStream << "...";
	inOutputStream << endl;

	if (hiliteRangeFrom != 9999 && hiliteRangeTo != 9999)
	{
		if (fromPixel)
			inOutputStream << "   ";
		if (hiliteRangeFrom == hiliteRangeTo)
			inOutputStream << string (spacesPerPixel * (hiliteRangeFrom - fromPixel), ' ') << " ^^" << endl;
		else
			inOutputStream << string (spacesPerPixel * (hiliteRangeFrom - fromPixel), ' ') << "[^^"
							<< string (spacesPerPixel * (hiliteRangeTo - hiliteRangeFrom - 1), '^') << "^^^]" << endl;
	}
	return inOutputStream;

}	//	DumpYBytes


ostream & CNTV2CaptionLogConfig::DumpYBytes_2vuy (const vector<uint8_t> & inVideoLine,
												ostream &				inOutputStream,
												const size_t			inFromPixel,
												const size_t			inToPixel,
												const bool				inShowRuler,
												const size_t			inHiliteRangeFrom,
												const size_t			inHiliteRangeTo)
{
	const size_t	spacesPerPixel	(3);
	size_t			fromPixel		(inFromPixel);
	size_t			toPixel			(inToPixel);
	size_t			hiliteRangeFrom	(inHiliteRangeFrom);
	size_t			hiliteRangeTo	(inHiliteRangeTo);
	if (fromPixel > 719)
		fromPixel = 719;
	if (toPixel > 719)
		toPixel = 719;
	if (fromPixel > toPixel)
		return inOutputStream;
	if (inVideoLine.size() < 1440)
		return inOutputStream;
	if (hiliteRangeFrom != 9999 && hiliteRangeTo != 9999)
	{
		if (hiliteRangeFrom > toPixel)
			hiliteRangeFrom = hiliteRangeTo = 9999;
		if (hiliteRangeFrom < fromPixel)
			hiliteRangeFrom = hiliteRangeTo = 9999;
		if (hiliteRangeTo > toPixel)
			hiliteRangeFrom = hiliteRangeTo = 9999;
		if (hiliteRangeTo < fromPixel)
			hiliteRangeFrom = hiliteRangeTo = 9999;
	}

	if (inShowRuler)
	{
		//	Hundreds
		if (fromPixel)
			inOutputStream << "   ";
		for (size_t pixNum(fromPixel);  pixNum <= toPixel;  pixNum++)
			if (pixNum % 100)
				inOutputStream << "   ";
			else
				inOutputStream << "  " << pixNum % 1000 / 100;
		inOutputStream << endl;
		//	Tens
		if (fromPixel)
			inOutputStream << "   ";
		for (size_t pixNum(fromPixel);  pixNum <= toPixel;  pixNum++)
			if (pixNum % 10)
				inOutputStream << "   ";
			else
				inOutputStream << "  " << pixNum % 100 / 10;
		inOutputStream << endl;
		//	Ones
		if (fromPixel)
			inOutputStream << "   ";
		for (size_t pixNum(fromPixel);  pixNum <= toPixel;  pixNum++)
			inOutputStream << "  " << pixNum % 10;
		inOutputStream << endl;
	}

	if (fromPixel)
		inOutputStream << "...";
	for (size_t pixNum(fromPixel);  pixNum <= toPixel;  pixNum++)
		inOutputStream << " " << HEX0N(uint16_t(inVideoLine.at(pixNum*2+1)),2);
	if (toPixel < 719)
		inOutputStream << "...";
	inOutputStream << endl;

	if (hiliteRangeFrom != 9999 && hiliteRangeTo != 9999)
	{
		if (fromPixel)
			inOutputStream << "   ";
		if (hiliteRangeFrom == hiliteRangeTo)
			inOutputStream << string(spacesPerPixel * (hiliteRangeFrom - fromPixel), ' ') << " ^^" << endl;
		else
			inOutputStream << string(spacesPerPixel * (hiliteRangeFrom - fromPixel), ' ') << "[^^"
							<< string(spacesPerPixel * (hiliteRangeTo - hiliteRangeFrom - 1), '^') << "^^^]" << endl;
	}
	return inOutputStream;

}	//	DumpYBytes


string CNTV2CaptionLogConfig::GetSeverityLabel(const unsigned inSeverity)
{
	return string(AJADebug::GetSeverityString(int32_t(inSeverity)));
}


CNTV2CaptionLogConfig::CNTV2CaptionLogConfig (const string inLogLabel)
	:	mLogMask	(::GetDefaultCaptionLogMask ()),
		mLogLabel	(inLogLabel)
{
	mpLabelLock = new AJALock;
	AJACC_ASSERT (mpLabelLock && "must have AJALock!");
	static bool	sAJACCDebugEnabled(false);
	{
		AJAAutoLock	autoLock	(MyLabelLock);
		if (!sAJACCDebugEnabled)
		{
			AJADebug::Open();
			sAJACCDebugEnabled = true;
		}
	}
}


CNTV2CaptionLogConfig::~CNTV2CaptionLogConfig ()
{
	if (mpLabelLock)
	{
		delete MyLabelLock;
		mpLabelLock = NULL;
	}
}


void CNTV2CaptionLogConfig::SetLogLabel (const string & inNewLabel)
{
	AJAAutoLock	autoLock	(MyLabelLock);
	mLogLabel = inNewLabel;
}

void CNTV2CaptionLogConfig::AppendToLogLabel (const string & inString)
{
	AJAAutoLock	autoLock	(MyLabelLock);
	mLogLabel += inString;
}

const string & CNTV2CaptionLogConfig::GetLogLabel (void) const
{
	AJAAutoLock	autoLock	(MyLabelLock);
	return mLogLabel;
}



//	BEGIN OBSOLETE
	void CNTV2CaptionLogConfig::SetLogStream (ostream & inOutputStream)
	{
		(void) inOutputStream;
	}

	ostream & CNTV2CaptionLogConfig::LogIf (const NTV2CaptionLogMask inLogMask) const
	{
		(void) inLogMask;
		return Log();
	}
	
	ostream & CNTV2CaptionLogConfig::Log (void) const
	{
		return cout;
	}
	
	void SetDefaultCaptionLogOutputStream (ostream & inOutputStream)
	{
		(void) inOutputStream;
	}
	
	ostream & GetDefaultCaptionLogOutputStream (void)
	{
		return cout;
	}
//	END OBSOLETE



class EnvironmentVariableReader
{
	public:
		EnvironmentVariableReader ()
		{
			//	To allow a little logging flexibility at runtime... set the 'CAPTIONLOG' environment variable to a
			//	hex string value '0x0000000000000000' that will be interpreted as an NTV2CaptionLogMask...
			const string	sEnvCaptionLog	(::getenv("CAPTIONLOG") ? ::getenv("CAPTIONLOG") : "");
			if (sEnvCaptionLog.length() == 18 && sEnvCaptionLog.find("0x") == 0)
			{
				ULWord64			logMask(0);
				std::stringstream	ss;
				ss << hex << sEnvCaptionLog.substr(2, 16);
				ss >> logMask;
				SetDefaultCaptionLogMask(logMask);
			}
		}
};	//	EnvironmentVariableReader

static EnvironmentVariableReader	gEnvironmentVariableReader;

#ifdef MSWindows
	#pragma warning(default: 4800)	//	int/bool warnings
#endif
