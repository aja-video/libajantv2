/**
	@file		ntv2caption608dataqueue.cpp
	@brief		Implementation of the CNTV2Caption608DataQueue class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2caption608dataqueue.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"


using namespace std;


#if defined (MSWindows)
	#pragma warning(disable: 4800)
	#pragma warning(disable:4127)	//	Stop MSVC from complaining about "do{...}while(false)" macros
#endif


//	A lovely hack to keep the AJACCLIB headers separated from 'ajalibraries/ajabase'...
#define	MyQueueLock					reinterpret_cast <AJALock *> (mpQueueLock)

//	Logging for this module...
#define LOGMYWARN(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC608DataQueue, AJA_DebugSeverity_Warning,	\
											AJAFUNC << ": " << GetLogLabel() << "[" << ::NTV2Line21FieldToStr(mFieldOfInterest) << "]: " << __xpr__)
#define LOGMYNOTE(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC608DataQueue, AJA_DebugSeverity_Notice,		\
											AJAFUNC << ": " << GetLogLabel() << "[" << ::NTV2Line21FieldToStr(mFieldOfInterest) << "]: " << __xpr__)
#define LOGMYINFO(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC608DataQueue, AJA_DebugSeverity_Info,		\
											AJAFUNC << ": " << GetLogLabel() << "[" << ::NTV2Line21FieldToStr(mFieldOfInterest) << "]: " << __xpr__)
#define LOGMYDBG(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC608DataQueue, AJA_DebugSeverity_Debug,		\
											AJAFUNC << ": " << GetLogLabel() << "[" << ::NTV2Line21FieldToStr(mFieldOfInterest) << "]: " << __xpr__)


static unsigned	gInstanceTally	(0);



/////////////////////////////////////////////////////////////////////////////
// Constructor
//
CNTV2Caption608DataQueue::CNTV2Caption608DataQueue (const NTV2Line21Field inFieldOfInterest)
	:	mpQueueLock			(new AJALock),
		mEnqueueTally		(0),
		mDequeueTally		(0),
		mHighestQueueDepth	(0)
{
	SetField(inFieldOfInterest);
	ostringstream	oss;
	oss << "Caption608DataQue-" << ++gInstanceTally;
	SetLogLabel(oss.str());

}	//	constructor


CNTV2Caption608DataQueue::~CNTV2Caption608DataQueue ()
{
	if (mpQueueLock)
	{
		delete MyQueueLock;
		mpQueueLock = NULL;
	}

}	//	destructor


// Flush()
//
void CNTV2Caption608DataQueue::Flush (void)
{
	AJAAutoLock		autoLock	(MyQueueLock);
	const size_t	oldDepth	(mDataQueue.size());
	mDataQueue.clear();
	LOGMYINFO("Depth was " << oldDepth << ", now " << mDataQueue.size());

}	//	Flush



// Push608Data()
//
//	Add new 608 data to the end of our output queue
//
bool CNTV2Caption608DataQueue::Push608Data (const UByte inChar1, const UByte inChar2, const bool inGotData)
{
	QueueData608	elementData	= {inGotData, inChar1, inChar2};
	AJAAutoLock		autoLock	(MyQueueLock);

	mDataQueue.push_back(elementData);
	mEnqueueTally++;
	if (mDataQueue.size() > mHighestQueueDepth)
		mHighestQueueDepth = mDataQueue.size();
	if (inGotData)
		LOGMYINFO("byte1=0x" << UHEX2(inChar1) << ", byte2=0x" << UHEX2(inChar2) << ", depth=" << mDataQueue.size());
	else
		LOGMYDBG("No data, depth=" << mDataQueue.size());

	return true;

}	//	Push608Data


// Pop608Data()
//
//	Copies the next 608 data from the queue to the designated pointer (also "pops" queue element).
//	Returns 'true' if new data returned, 'false' if queue is empty.
//	Side effect: if the queue is empty, this method sets the returned characters to "Nulls"
//
bool CNTV2Caption608DataQueue::Pop608Data (UByte & outChar1, UByte & outChar2, bool & outGotData)
{
	bool		bResult	(false);
	AJAAutoLock	autoLock (MyQueueLock);

	//	Sanity check: make sure there is something on the queue...
	if (!mDataQueue.empty())
	{
		//	Get latest from read index location...
		QueueData608	elementData	(mDataQueue.front());
		outChar1	= elementData.fChar1;
		outChar2	= elementData.fChar2;
		outGotData	= elementData.fGotData;

		mDataQueue.pop_front();
		mDequeueTally++;
		bResult = true;
		if (outGotData)
			LOGMYINFO("byte1=0x" << UHEX2(outChar1) << ", byte2=0x" << UHEX2(outChar2) << ", depth=" << mDataQueue.size());
		else
			LOGMYDBG("No data, depth=" << mDataQueue.size());
	}
	else
	{
		outChar1 = 0x80;
		outChar2 = 0x80;
		outGotData = false;
		LOGMYDBG("Queue empty");
	}

	return bResult;

}	//	Pop608Data


// IsEmpty()
//
bool CNTV2Caption608DataQueue::IsEmpty (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mDataQueue.empty();

}	//	IsEmpty


size_t	CNTV2Caption608DataQueue::GetCurrentDepth (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mDataQueue.size();

}	//	GetCurrentDepth


size_t	CNTV2Caption608DataQueue::GetMaximumDepth (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mHighestQueueDepth;

}	//	GetMaximumDepth


size_t	CNTV2Caption608DataQueue::GetEnqueueTally (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mEnqueueTally;

}	//	GetEnqueueTally


size_t	CNTV2Caption608DataQueue::GetDequeueTally (void) const
{
	AJAAutoLock	autoLock (MyQueueLock);
	return mDequeueTally;

}	//	GetDequeueTally
