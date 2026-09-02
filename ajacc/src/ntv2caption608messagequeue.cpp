/**
	@file		ntv2caption608messagequeue.cpp
	@brief		Implementation of the CNTV2Caption608MessageQueue class.
	@copyright	(C) 2005-2022 AJA Video Systems, Inc. All rights reserved.
**/


#include "ntv2caption608messagequeue.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"
#include <iomanip>
#include <string.h>	//	for memset


#if defined (MSWindows)
	#pragma warning(disable: 4800)
	#pragma warning(disable:4127)	//	Stop MSVC from complaining about "do{...}while(false)" macros
#endif


//	Logging for this module...
#define LOGMYWARN(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC608MsgQueue, AJA_DebugSeverity_Warning, AJAFUNC << ":  " << __xpr__)
#define LOGMYNOTE(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC608MsgQueue, AJA_DebugSeverity_Notice, AJAFUNC << ":  " << __xpr__)
#define LOGMYINFO(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC608MsgQueue, AJA_DebugSeverity_Info, AJAFUNC << ":  " << __xpr__)
#define LOGMYDBG(__xpr__)		AJA_sREPORT(AJA_DebugUnit_CC608MsgQueue, AJA_DebugSeverity_Debug, AJAFUNC << ":  " << __xpr__)

using namespace std;


//	A lovely hack to keep the AJACCLIB headers separated from 'ajalibraries/ajabase'...
#define	MyQueueLock		reinterpret_cast <AJALock *> (mpQueueLock)



CNTV2Caption608MessageQueue::CNTV2Caption608MessageQueue()
	:	mpQueueLock		(NULL),
		mMaxMsgTally	(0)
{
	mpQueueLock = new AJALock;
	::memset (mEnqueueByteTally, 0, sizeof(mEnqueueByteTally));
	::memset (mDequeueByteTally, 0, sizeof(mDequeueByteTally));
	::memset (mEnqueueMsgTally, 0, sizeof(mEnqueueMsgTally));
	::memset (mDequeueMsgTally, 0, sizeof(mDequeueMsgTally));
	AJACC_ASSERT (mpQueueLock && "must have AJALock!");

}	//	constructor


CNTV2Caption608MessageQueue::~CNTV2Caption608MessageQueue()
{
	if (mpQueueLock)
	{
		delete MyQueueLock;
		mpQueueLock = NULL;
	}

}	//	destructor


CNTV2Caption608MessagePtr CNTV2Caption608MessageQueue::GetNextCaptionMessage (void)
{
	CNTV2Caption608MessagePtr	result;
	AJAAutoLock					autoLock	(MyQueueLock);

	if (!mHiPriorityQueue.empty())
	{
		result = mHiPriorityQueue.front();
		mHiPriorityQueue.pop_front();
		if (result)
			mDequeueByteTally[result->GetChannel()] += result->GetLength();
		mDequeueMsgTally[result->GetChannel()] += 1;
		LOGMYINFO("Popped " << result << ", " << mHiPriorityQueue.size() << " hi + " << mLoPriorityQueue.size() << " lo msgs remain");
	}
	else if (!mLoPriorityQueue.empty())
	{
		result = mLoPriorityQueue.front();
		mLoPriorityQueue.pop_front();
		if (result)
			mDequeueByteTally[result->GetChannel()] += result->GetLength();
		mDequeueMsgTally[result->GetChannel()] += 1;
		LOGMYINFO("Popped " << result << ", " << mHiPriorityQueue.size() << " hi + " << mLoPriorityQueue.size() << " lo msgs remain");
	}
	else
		LOGMYDBG("Queue empty, returning null message");

	return result;

}	//	GetNextCaptionMessage


bool CNTV2Caption608MessageQueue::EnqueueCaptionMessage (CNTV2Caption608MessagePtr inMsg)
{
	bool	result (false);
	if (inMsg)
	{
		AJAAutoLock	autoLock (MyQueueLock);
		if (inMsg->IsHighPriority())
			mHiPriorityQueue.push_back(inMsg);
		else
			mLoPriorityQueue.push_back(inMsg);
		mEnqueueByteTally[inMsg->GetChannel()] += inMsg->GetLength();
		mEnqueueMsgTally[inMsg->GetChannel()] += 1;
		if (mHiPriorityQueue.size() > mMaxMsgTally)
			mMaxMsgTally = mHiPriorityQueue.size();
		else if (mLoPriorityQueue.size() > mMaxMsgTally)
			mMaxMsgTally = mLoPriorityQueue.size();
		LOGMYINFO("Pushed " << inMsg << ", " << mHiPriorityQueue.size() << " hi + " << mLoPriorityQueue.size() << " lo msgs queued");
		result = true;
	}
	else
		LOGMYWARN("Attempt to push empty message");

	return result;

}	//	EnqueueCaptionMessage


size_t CNTV2Caption608MessageQueue::Flush (void)
{
	AJAAutoLock		autoLock	(MyQueueLock);
	size_t			tally		(0);

	while (!mHiPriorityQueue.empty())
	{
		mHiPriorityQueue.pop_front();
		tally++;
	}
	while (!mLoPriorityQueue.empty())
	{
		mLoPriorityQueue.pop_front();
		tally++;
	}
	LOGMYINFO("Flushed " << tally << " msgs");
	return tally;

}	//	Flush


size_t CNTV2Caption608MessageQueue::Flush (const NTV2Line21Channel inChannel)
{
	AJAAutoLock		autoLock	(MyQueueLock);
	size_t			tally		(0);

	if (IsLine21CaptionChannel(inChannel))
	{
		MyQueueType	newQueue	(mHiPriorityQueue);		//	Make a local copy of the existing queue
		mHiPriorityQueue.clear();						//	Clear the existing queue
		while (!newQueue.empty())						//	While local queue not yet empty...
		{
			const CNTV2Caption608MessagePtr	msg(newQueue.back());	//	Copy oldest msg
			newQueue.pop_back();									//	Pop oldest msg
			if (msg->GetChannel() == inChannel)						//	Msg's channel match channel of interest?
				tally++;											//		Yes -- don't enqueue it
			else													//	Else
				mHiPriorityQueue.push_back(msg);					//		No -- enqueue it
		}
	}
	else
	{
		MyQueueType	newQueue	(mLoPriorityQueue);		//	Make a local copy of the existing queue
		mLoPriorityQueue.clear();						//	Clear the existing queue
		while (!newQueue.empty())						//	While local queue not yet empty...
		{
			const CNTV2Caption608MessagePtr	msg	(newQueue.back());	//	Copy oldest msg
			newQueue.pop_back();									//	Pop oldest msg
			if (msg->GetChannel() == inChannel)						//	Msg's channel match channel of interest?
				tally++;											//		Yes -- don't enqueue it
			else													//	Else
				mLoPriorityQueue.push_back(msg);					//		No -- enqueue it
		}
	}
	LOGMYINFO("Flushed " << tally << " " << ::NTV2Line21ChannelToStr(inChannel) << " msgs");
	return tally;

}	//	Flush specific channel


size_t CNTV2Caption608MessageQueue::GetQueuedMessageCount (void) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	return mHiPriorityQueue.size() + mLoPriorityQueue.size();

}	//	GetQueuedMessageCount


size_t CNTV2Caption608MessageQueue::GetQueuedMessageCount (const NTV2Line21Channel inChannel) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	size_t			tally		(0);

	if (IsLine21CaptionChannel(inChannel))
	{
		for (MyQueueTypeConstIter iter(mHiPriorityQueue.begin());  iter != mHiPriorityQueue.end();  ++iter)
			if ((*iter)->GetChannel() == inChannel)
				tally++;
	}
	else
	{
		for (MyQueueTypeConstIter iter(mLoPriorityQueue.begin());  iter != mLoPriorityQueue.end();  ++iter)
			if ((*iter)->GetChannel() == inChannel)
				tally++;
	}
	return tally;

}	//	GetQueuedMessageCount


size_t CNTV2Caption608MessageQueue::GetQueuedByteCount (void) const
{
	size_t		result		(0);
	AJAAutoLock	autoLock	(MyQueueLock);

	for (MyQueueType::const_iterator iter(mHiPriorityQueue.begin());  iter != mHiPriorityQueue.end();  ++iter)
	{
		CNTV2Caption608MessagePtr	pMsg (*iter);
		if (pMsg && pMsg->IsData())
			result += pMsg->GetLength();
		AJACC_ASSERT(pMsg);
	}	//	for each hi-pri queued caption msg

	for (MyQueueType::const_iterator iter(mLoPriorityQueue.begin());  iter != mLoPriorityQueue.end();  ++iter)
	{
		CNTV2Caption608MessagePtr	pMsg (*iter);
		if (pMsg && pMsg->IsData())
			result += pMsg->GetLength();
		AJACC_ASSERT(pMsg);
	}	//	for each lo-pri queued caption msg

	return result;

}	//	GetQueuedByteCount


size_t CNTV2Caption608MessageQueue::GetQueuedByteCount (const NTV2Line21Channel inChannel) const
{
	size_t		result		(0);
	AJAAutoLock	autoLock	(MyQueueLock);

	if (IsLine21CaptionChannel(inChannel))
	{
		for (MyQueueTypeConstIter iter(mHiPriorityQueue.begin());  iter != mHiPriorityQueue.end();  ++iter)
			if ((*iter)->GetChannel() == inChannel)
				result += (*iter)->GetLength();
	}
	else
	{
		for (MyQueueTypeConstIter iter(mLoPriorityQueue.begin());  iter != mLoPriorityQueue.end();  ++iter)
			if ((*iter)->GetChannel() == inChannel)
				result += (*iter)->GetLength();
	}
	return result;

}	//	GetQueuedByteCount


size_t CNTV2Caption608MessageQueue::GetEnqueueByteTally (void) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	size_t			result		(0);
	for (unsigned chan(NTV2_CC608_CC1);  chan < NTV2_CC608_ChannelMax;  chan++)
		result += mEnqueueByteTally[chan];
	return result;
}


size_t CNTV2Caption608MessageQueue::GetEnqueueByteTally (const NTV2Line21Channel inChannel) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	return inChannel < NTV2_CC608_ChannelMax  ?  mEnqueueByteTally[inChannel]  :  0;
}


size_t CNTV2Caption608MessageQueue::GetDequeueByteTally (void) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	size_t			result		(0);
	for (unsigned chan(NTV2_CC608_CC1);  chan < NTV2_CC608_ChannelMax;  chan++)
		result += mDequeueByteTally[chan];
	return result;
}


size_t CNTV2Caption608MessageQueue::GetDequeueByteTally (const NTV2Line21Channel inChannel) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	return inChannel < NTV2_CC608_ChannelMax  ?  mDequeueByteTally[inChannel]  :  0;
}


size_t CNTV2Caption608MessageQueue::GetEnqueueMessageTally (void) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	size_t			result		(0);
	for (unsigned chan(NTV2_CC608_CC1);  chan < NTV2_CC608_ChannelMax;  chan++)
		result += mEnqueueMsgTally[chan];
	return result;
}


size_t CNTV2Caption608MessageQueue::GetEnqueueMessageTally (const NTV2Line21Channel inChannel) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	return inChannel < NTV2_CC608_ChannelMax  ?  mEnqueueMsgTally[inChannel]  :  0;
}


size_t CNTV2Caption608MessageQueue::GetDequeueMessageTally (void) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	size_t			result		(0);
	for (unsigned chan(NTV2_CC608_CC1);  chan < NTV2_CC608_ChannelMax;  chan++)
		result += mDequeueMsgTally[chan];
	return result;
}


size_t CNTV2Caption608MessageQueue::GetDequeueMessageTally (const NTV2Line21Channel inChannel) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	return inChannel < NTV2_CC608_ChannelMax  ?  mDequeueMsgTally[inChannel]  :  0;
}


std::ostream & CNTV2Caption608MessageQueue::Print (std::ostream & inOutStream, const bool inDumpMessages) const
{
	AJAAutoLock		autoLock	(MyQueueLock);
	const size_t	nBytes		(GetQueuedByteCount());
	inOutStream	<< GetLogLabel() << " has " << nBytes << " byte" << (nBytes != 1 ? "s" : "") << " in "
				<< mHiPriorityQueue.size() << " msgs" << (inDumpMessages && (!mHiPriorityQueue.empty() || !mLoPriorityQueue.empty()) ? ":" : "") << endl;
	if (inDumpMessages)
	{
		unsigned ndx (0);
		for (MyQueueType::const_iterator iter(mHiPriorityQueue.begin());  iter != mHiPriorityQueue.end();  ++iter)
			inOutStream << ++ndx << ":  " << *iter << endl;
		ndx = 0;
		for (MyQueueType::const_iterator iter(mLoPriorityQueue.begin());  iter != mLoPriorityQueue.end();  ++iter)
			inOutStream << ++ndx << ":  " << *iter << endl;
	}
	return inOutStream;
}


CNTV2Caption608MessageQueue::CNTV2Caption608MessageQueue (const CNTV2Caption608MessageQueue & inObjToCopy)
	:	CNTV2CaptionLogConfig ()
{
	(void) inObjToCopy;
	AJACC_ASSERT(false);
}


CNTV2Caption608MessageQueue & CNTV2Caption608MessageQueue::operator = (const CNTV2Caption608MessageQueue & inRHS)
{	(void) inRHS;
	AJACC_ASSERT(false);
	return *this;
}


#ifdef MSWindows
	#pragma warning(default: 4800)
#endif
