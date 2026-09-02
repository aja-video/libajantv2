/**
	@file		ntv2caption708serviceblockqueue.cpp
	@brief		Implementation of the CNTV2Caption708ServiceBlockQueue class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/

#include "ntv2caption708serviceblockqueue.h"
#include "ntv2caption608types.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"
#include <sstream>

using namespace std;

#if defined (MSWindows)
	#pragma warning(disable: 4800) 
	#pragma warning(disable:4127)	//	Stop MSVC from complaining about "do{...}while(false)" macros
#endif


//	A lovely hack to keep the AJACCLIB headers separated from 'ajalibraries/ajabase'...
#define	MyQueueLock		reinterpret_cast <AJALock *> (mpQueueLock)

//	Logging for this module...
#define LOGMYERR(__xpr__)		AJA_sERROR(AJA_DebugUnit_CC708ServiceBlockQueue,	GetChannelString() << ": " << AJAFUNC << ": " << __xpr__)
#define LOGMYWARN(__xpr__)		AJA_sWARNING(AJA_DebugUnit_CC708ServiceBlockQueue,	GetChannelString() << ": " << AJAFUNC << ": " << __xpr__)
#define LOGMYNOTE(__xpr__)		AJA_sNOTICE(AJA_DebugUnit_CC708ServiceBlockQueue,	GetChannelString() << ": " << AJAFUNC << ": " << __xpr__)
#define LOGMYINFO(__xpr__)		AJA_sINFO(AJA_DebugUnit_CC708ServiceBlockQueue,		GetChannelString() << ": " << AJAFUNC << ": " << __xpr__)
#define LOGMYDBG(__xpr__)		AJA_sDEBUG(AJA_DebugUnit_CC708ServiceBlockQueue,	GetChannelString() << ": " << AJAFUNC << ": " << __xpr__)


static unsigned	gInstanceTally	(0);



/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2Caption708ServiceBlockQueue::CNTV2Caption708ServiceBlockQueue (void)
	:	mpQueueLock			(new AJALock),
		mDebugChannel		(-1),	//	undefined
		mEnqueueTally		(0),
		mDequeueTally		(0),
		mEnqueueByteTally	(0),
		mDequeueByteTally	(0),
		mHighestQueueDepth	(0)
{
	ostringstream	oss;	oss << "Capt708SvcBlkQue-" << ++gInstanceTally;
	SetLogLabel(oss.str());

}	//	constructor


CNTV2Caption708ServiceBlockQueue::~CNTV2Caption708ServiceBlockQueue ()
{
	if (mpQueueLock)
	{
		delete MyQueueLock;
		mpQueueLock = NULL;
	}

}	//	destructor


// Flush()
//
void CNTV2Caption708ServiceBlockQueue::Flush (void)
{
	AJAAutoLock		autoLock	(MyQueueLock);
	mServiceBlockQueue.clear();
	LOGMYNOTE("Queue flushed");

}	//	Flush


// IsEmpty()
//
bool CNTV2Caption708ServiceBlockQueue::IsEmpty (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mServiceBlockQueue.empty();

}	//	IsEmpty


size_t	CNTV2Caption708ServiceBlockQueue::GetCurrentDepth (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mServiceBlockQueue.size();

}	//	GetCurrentDepth


size_t	CNTV2Caption708ServiceBlockQueue::GetHighestDepth (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mHighestQueueDepth;

}	//	GetHighestDepth


size_t	CNTV2Caption708ServiceBlockQueue::GetEnqueueTally (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mEnqueueTally;

}	//	GetEnqueueTally


size_t	CNTV2Caption708ServiceBlockQueue::GetDequeueTally (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mDequeueTally;

}	//	GetDequeueTally


size_t	CNTV2Caption708ServiceBlockQueue::GetEnqueueByteTally (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mEnqueueByteTally;

}	//	GetEnqueueByteTally


size_t	CNTV2Caption708ServiceBlockQueue::GetDequeueByteTally (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mDequeueByteTally;

}	//	GetDequeueByteTally



// PushServiceBlock()
//
//	Add a new Service Block to the end of our output queue
// 
bool CNTV2Caption708ServiceBlockQueue::PushServiceBlock (const UByte * pInServiceBlockData, const size_t inServiceBlockByteCount)
{
	if (pInServiceBlockData && inServiceBlockByteCount)
	{
		AJAAutoLock	autoLock (MyQueueLock);

		try
		{
			for (size_t ndx(0);  ndx < inServiceBlockByteCount;  ndx++)
				mServiceBlockQueue.push_back(pInServiceBlockData[ndx]);
		}
		catch (const bad_alloc &)
		{
			return false;	//	Fail
		}

		//	Update my stats...
		mEnqueueByteTally += inServiceBlockByteCount;
		mEnqueueTally++;
		if (mServiceBlockQueue.size() > mHighestQueueDepth)
			mHighestQueueDepth = mServiceBlockQueue.size();
		LOGMYINFO(inServiceBlockByteCount << "-byte Service Block, now " << *this);
		return true;	//	Success!
	}
	LOGMYERR("byteCount=" << inServiceBlockByteCount << (pInServiceBlockData ? "" : ", NULL svcBlkData pointer"));
	return false;	//	Fail

}	//	PushServiceBlock


// PeekNextServiceBlockInfo()
//
//	Returns the Block Size, Data Size, and Service Number of the next Service Block on the queue (returns false if empty).
//	This is a "peek" operation - the service block is left on the queue.
//
bool CNTV2Caption708ServiceBlockQueue::PeekNextServiceBlockInfo (size_t & outBlockByteCount, size_t & outDataByteCount, int & outServiceNum, bool & outIsExtended) const
{
	AJAAutoLock	autoLock	(MyQueueLock);

	outIsExtended = false;
	outServiceNum = 0;
	outDataByteCount = 0;
	outBlockByteCount = 0;

	if (mServiceBlockQueue.empty())
		return false;	//	Don't log anything here, we're only peeking

	//	If there's a Service Block on the queue, the first byte will give us the Service Number and dataSize...
	outDataByteCount = mServiceBlockQueue.front() & 0x1f;
	outBlockByteCount = outDataByteCount + (outIsExtended ? 2 : 1);
	outServiceNum = (mServiceBlockQueue.front() & 0xe0) >> 5;

	if (outServiceNum == 7)
	{
		//	It's an Extended Service Block -- get the number from the second byte...
		std::deque <UByte>	queueCopy	(mServiceBlockQueue);	//	Make a temporary copy of my queue
		AJACC_ASSERT (!queueCopy.empty());						//	It better not be empty!
		queueCopy.pop_front();									//	Skip the first byte
		if (queueCopy.empty())
			return false;	//	Fail if empty

		outServiceNum = queueCopy.front() & 0x3f;				//	Recompute the service number
		outIsExtended = true;
	}

	LOGMYINFO(outDataByteCount << "-byte Service Block pending:  Svc " << outServiceNum << (outIsExtended ? " +Extended" : ""));
	return true;

}	//	PeekNextServiceBlockInfo



// PopServiceBlock()
//
size_t CNTV2Caption708ServiceBlockQueue::PopServiceBlock (vector<UByte> & outData)
{
	AJAAutoLock	autoLock	(MyQueueLock);

	if (mServiceBlockQueue.empty())
	{
		LOGMYDBG("Queue empty");
		return 0;		//	Return zero if queue empty
	}

	outData.clear();

	//	The first byte will give us the data size and whether or not we have an extended service block header...
	const UByte		header		(mServiceBlockQueue.front());
	const size_t	dataSize	(header & 0x1f);						//	This is the number of bytes in the Service Block NOT including the header
	const size_t	headerSize	(((header & 0xe0) == 0xe0) ? 2 : 1);	//	Header size, in bytes (extended = 2 bytes, standard = 1)
	const size_t	blockSize	(dataSize + headerSize);

	//	Pop off each queued byte, copying it into the target buffer...
	for (size_t ndx(0);  ndx < blockSize;  ndx++)
	{
		if (mServiceBlockQueue.empty())
			{LOGMYERR("Queue emptied prematurely"); return 0;}
		outData.push_back(mServiceBlockQueue.front());
		mServiceBlockQueue.pop_front();
	}

	LOGMYINFO("Popped " << blockSize << "-byte Service Block, remaining: " << *this);
	return blockSize;

}	//	PopServiceBlock


size_t CNTV2Caption708ServiceBlockQueue::PopServiceBlock (UByte * pOutDataBuffer)
{
	AJAAutoLock	autoLock	(MyQueueLock);

	if (mServiceBlockQueue.empty())
	{
		LOGMYDBG("Queue empty");
		return 0;		//	Return zero if queue empty
	}

	if (pOutDataBuffer == NULL)
		return 0;		//	NULL output buffer pointer

	//	The first byte will give us the data size and whether or not we have an extended service block header...
	const UByte		header		(mServiceBlockQueue.front());
	const size_t	dataSize	(header & 0x1f);						//	This is the number of bytes in the Service Block NOT including the header
	const size_t	headerSize	(((header & 0xe0) == 0xe0) ? 2 : 1);	//	Header size, in bytes (extended = 2 bytes, standard = 1)
	const size_t	blockSize	(dataSize + headerSize);

	//	Pop off each queued byte, copying it into the target buffer...
	for (size_t ndx(0);  ndx < blockSize;  ndx++)
	{
		AJACC_ASSERT (!mServiceBlockQueue.empty() && "PopServiceBlock -- queue empty prematurely");
		pOutDataBuffer [ndx] = mServiceBlockQueue.front();
		mServiceBlockQueue.pop_front();
	}

	LOGMYINFO("Popped " << blockSize << "-byte Service Block, remaining: " << *this);
	return blockSize;

}	//	PopServiceBlock



// PopServiceBlockData()
//
//	Copies the next Service Block (data only) from the queue to the designated pointer (also "pops" queue element).
//	This method only copies the Service Block payload - to copy the entire Service Block, call PopServiceBlock().
//	Returns the size of the copied data.
//
size_t CNTV2Caption708ServiceBlockQueue::PopServiceBlockData (vector<UByte> & outData)
{
	AJAAutoLock	autoLock	(MyQueueLock);

	if (mServiceBlockQueue.empty())
	{
		LOGMYERR("Queue empty");
		return 0;		//	Return zero if queue empty
	}

	outData.clear();

	//	The first byte will give us the data size and whether or not we have an extended service block header...
	const UByte		header		(mServiceBlockQueue.front());
	const size_t	dataSize	(header & 0x1f);						//	This is the number of bytes in the Service Block NOT including the header
	const size_t	headerSize	(((header & 0xe0) == 0xe0) ? 2 : 1);	//	Header size, in bytes (extended = 2 bytes, standard = 1)

	//	Skip the header bytes...
	for (size_t skip(0);  skip < headerSize;  skip++)
	{
		AJACC_ASSERT (!mServiceBlockQueue.empty() && "PopServiceBlockData -- queue empty prematurely");
		mServiceBlockQueue.pop_front();
	}	//	for each header byte

	//	Pop off each data byte, copying it into the target buffer...
	for (size_t ndx(0);  ndx < dataSize;  ndx++)
	{
		if (mServiceBlockQueue.empty())
			{LOGMYERR("Queue emptied prematurely"); return 0;}
		outData.push_back(mServiceBlockQueue.front());
		mServiceBlockQueue.pop_front();
	}	//	for each data byte

	LOGMYINFO("Popped " << size_t(dataSize + headerSize) << "-byte Service Block, remaining: " << *this);
	return dataSize;

}	//	PopServiceBlockData

size_t CNTV2Caption708ServiceBlockQueue::PopServiceBlockData (UByte * pOutDataBuffer)
{
	AJAAutoLock	autoLock	(MyQueueLock);

	if (mServiceBlockQueue.empty())
	{
		LOGMYERR("Queue empty");
		return 0;		//	Return zero if queue empty
	}

	if (pOutDataBuffer == NULL)
		return 0;

	//	The first byte will give us the data size and whether or not we have an extended service block header...
	const UByte		header		(mServiceBlockQueue.front());
	const size_t	dataSize	(header & 0x1f);						//	This is the number of bytes in the Service Block NOT including the header
	const size_t	headerSize	(((header & 0xe0) == 0xe0) ? 2 : 1);	//	Header size, in bytes (extended = 2 bytes, standard = 1)

	//	Skip the header bytes...
	for (size_t skip(0);  skip < headerSize;  skip++)
	{
		AJACC_ASSERT (!mServiceBlockQueue.empty() && "PopServiceBlockData -- queue empty prematurely");
		mServiceBlockQueue.pop_front();
	}	//	for each header byte

	//	Pop off each data byte, copying it into the target buffer...
	for (size_t ndx(0);  ndx < dataSize;  ndx++)
	{
		AJACC_ASSERT (!mServiceBlockQueue.empty() && "PopServiceBlockData -- queue empty prematurely");
		pOutDataBuffer[ndx] = mServiceBlockQueue.front();
		mServiceBlockQueue.pop_front();
	}	//	for each data byte

	LOGMYINFO("Popped " << size_t(dataSize + headerSize) << "-byte Service Block, remaining: " << *this);
	return dataSize;

}	//	PopServiceBlockData



// Debug Stuff...
//

// SetChannel()
//		Set the decode channel ID that this instance is working on
//
void CNTV2Caption708ServiceBlockQueue::SetDebugChannel (const int inChannel)
{
	const int	oldDebugChannel	(mDebugChannel);
	if (oldDebugChannel != inChannel)
	{
		const string	oldLabel	(GetChannelString());
		mDebugChannel = inChannel;
		LOGMYNOTE("Log channel changed -- was " << oldLabel);
	}
}

ostream & CNTV2Caption708ServiceBlockQueue::Print (ostream & inOutStrm, const bool inWithData) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	const size_t	numBytes	(mServiceBlockQueue.size());
	inOutStrm << numBytes << " byte(s) queued";
	if (inWithData)
	{
		for (size_t ndx(0);  ndx < numBytes;  ndx++)
		{
			inOutStrm << " " << HEX0N(uint16_t(mServiceBlockQueue.at(ndx)),2);
			if (ndx > 31)
				{inOutStrm << "...";	break;}
		}
	}
	return inOutStrm;
}


// GetChannelString()
//		For debug: returns a short name string for the given channel.
//
string CNTV2Caption708ServiceBlockQueue::GetChannelString (void) const
{
	ostringstream	oss;
	if (IsValidLine21Channel(mDebugChannel))
		oss << "[" << ::NTV2Line21ChannelToStr(NTV2Line21Channel(mDebugChannel)) << "]";
	else
		oss << "[" << (mDebugChannel - NTV2_CC608_ChannelMax) << "]";
	return oss.str();

}	//	GetChannelString


CNTV2Caption708ServiceBlockQueue::CNTV2Caption708ServiceBlockQueue (const CNTV2Caption708ServiceBlockQueue & inObj)
	:	CNTV2CaptionLogConfig ()
{
	(void) inObj;
	AJACC_ASSERT (false);
}

CNTV2Caption708ServiceBlockQueue & CNTV2Caption708ServiceBlockQueue::operator = (const CNTV2Caption708ServiceBlockQueue & inRHS)
{	(void) inRHS;
	AJACC_ASSERT (false);
	return *this;
}


ostream & operator << (std::ostream & inOutStrm, const CNTV2Caption708ServiceBlockQueue & inQueue)
{
	return inQueue.Print(inOutStrm, true);
}
