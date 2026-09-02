/**
	@file		ntv2caption608message.cpp
	@brief		Implementation of the CNTV2Caption608Message class.
	@copyright	(C) 2005-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2caption608message.h"
#include "ntv2endian.h"
#include "ntv2publicinterface.h"
#include <string.h>	//	for memset


#if defined (MSWindows)
	#pragma warning(disable: 4800)
#endif

using namespace std;


//	A lovely hack to keep the AJACCLIB headers separated from 'ajalibraries/ajabase'...
#define	MyQueueLock		reinterpret_cast <AJALock *> (mpQueueLock)


static unsigned	gInstanceTally	(0);



/////////////////////////////////////////////////////////////////////////////

bool CNTV2Caption608Message::Create (CNTV2Caption608MessagePtr & outNewInstance, const NTV2Line21Channel inChannel, const NTV2_CC608_CaptionMessageType inType)
{
	CNTV2Caption608MessagePtr	result;
	try
	{
		result = new CNTV2Caption608Message (inChannel, inType);
	}
	catch (const std::bad_alloc &)
	{
		result = NULL;
	}
	outNewInstance = result;
	return outNewInstance;

}	//	Create


CNTV2Caption608Message::CNTV2Caption608Message (const NTV2Line21Channel inChannel, const NTV2_CC608_CaptionMessageType inType)
	:	mType			(inType),
		mLength			(0),
		mReadPosition	(0),
		mChannel		(inChannel)
{
	gInstanceTally++;
	::memset (mData, 0, NTV2_CC608_CaptionMsgMaxBytes);

}	//	constructor


UByte CNTV2Caption608Message::ReadNext (void)
{
	return (mReadPosition < mLength) ? mData[mReadPosition++] : 0;

}	//	ReadNext


bool CNTV2Caption608Message::Add608Command (const UWord inCommand)
{
	bool	bResult		(true);
	bool	bPadByte	(false);

	//	Commands get transmitted twice...
	UWord	byteCount	(4);

	if (GetLength() % 2 == 1)
	{
		//	I already have an odd number of bytes, so I'll need to add an extra "null"
		//	byte to ensure both bytes of the command transmit on the same field...
		byteCount++;
		bPadByte = true;
	}

	//	Room for two more bytes?
	if (GetLength() + byteCount > NTV2_CC608_CaptionMsgMaxBytes)
		return false;	//	Fail

	if (bPadByte)
		mData[mLength++] = 0x00;		//	Add the "null" pad byte

	//	Add command bytes...
	mData[mLength++] = (inCommand & 0xFF00) >> 8;
	mData[mLength++] = (inCommand & 0x00FF);

	//	Duplicate...
	mData[mLength++] = (inCommand & 0xFF00) >> 8;
	mData[mLength++] = (inCommand & 0x00FF);

	return bResult;

}	//	Add608Command


bool CNTV2Caption608Message::Add608String (const std::string & inMessageStr)
{
	const UWord	byteCount	(static_cast <UWord> (inMessageStr.length()));

	//	Make sure we have room for the whole string...
	if ((GetLength() + byteCount) > NTV2_CC608_CaptionMsgMaxBytes)
		return false;	//	Fail

	//	Copy data...
	for (unsigned charNdx(0);  charNdx < byteCount;  charNdx++)
		mData[mLength++] = static_cast <UByte> (inMessageStr[charNdx]);

	return true;

}	//	Add608String


bool CNTV2Caption608Message::AddBytePair (const UByte inByte1, const UByte inByte2)
{
	const UWord	byteCount	(2);

	//	Make sure we have room for the whole string...
	if ((GetLength() + byteCount) > NTV2_CC608_CaptionMsgMaxBytes)
		return false;	//	Fail

	//	Add the bytes...
	mData[mLength++] = inByte1;
	mData[mLength++] = inByte2;
	return true;

}	//	AddBytePair


std::ostream & CNTV2Caption608Message::Print (std::ostream & inOutStream) const
{
	inOutStream << "[MSG" << (IsHighPriority() ? "+" : "-") << ::NTV2Line21ChannelToStr(GetChannel()) << (IsHighPriority() ? "+" : "-") << (IsDelay () ? "DEL:" : "DAT:");
	if (IsData())
	{
		inOutStream << " at " << GetReadPosition() << ", " << GetLength() << (GetLength() == 1 ? " byte" : " bytes");
		if (HasData())
		{
			inOutStream << ": ";
			for (unsigned ndx(0);  ndx < GetLength();  ndx++)
				inOutStream << HEX0N(uint16_t(mData[ndx]),2);
		}
	}
	else
		inOutStream << GetLength () << (GetLength () == 1 ? " frame" : " frames");

	return inOutStream << "]";

}	//	Print


CNTV2Caption608Message & CNTV2Caption608Message::operator = (const CNTV2Caption608Message & inRHS)
{
	(void) inRHS;
	return *this;
}


std::ostream & operator << (std::ostream & inOutStream, CNTV2Caption608MessagePtr inMsgPtr)
{
	if (inMsgPtr)
		return inOutStream << *inMsgPtr;
	else
		return inOutStream << "[null MSG-DAT]";
}


#ifdef MSWindows
	#pragma warning(default: 4800)
#endif
