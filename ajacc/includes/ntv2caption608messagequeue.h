/**
	@file		ntv2caption608messagequeue.h
	@brief		Declares the CNTV2Caption608MessageQueue class.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CAPTION608MESSAGEQUEUE__
#define __NTV2_CAPTION608MESSAGEQUEUE__

#include "ntv2caption608message.h"
#include <deque>


/**
	@brief	I am a thread-safe queue of caption messages.
			Internally, I maintain two queues:  one for high-priority messages, another for low-priority ones.
			Calling CNTV2Caption608MessageQueue::GetNextCaptionMessage will never return a low-priority message
			if a high-priority message is waiting to go out.
**/
class AJAExport CNTV2Caption608MessageQueue : public CNTV2CaptionLogConfig
{
	//	Instance Methods
	public:
										CNTV2Caption608MessageQueue ();
		virtual							~CNTV2Caption608MessageQueue ();

		/**
			@brief		Dequeues and returns my front-most caption message.
						If no messages are waiting to go out, I'll return a NULL (empty) CNTV2Caption608MessagePtr.
		**/
		virtual CNTV2Caption608MessagePtr	GetNextCaptionMessage (void);

		/**
			@brief		Enqueues the given caption message.
						High-priority messages go ahead of any/all low-priority messages.
			@param[in]	inMsg	The message to be enqueued.
		**/
		virtual bool					EnqueueCaptionMessage (CNTV2Caption608MessagePtr inMsg);

		/**
			@brief		Empties me.
			@return		The number of messages that were removed.
		**/
		virtual size_t					Flush (void);

		/**
			@brief		Flushes all of my messages that are associated with the given caption channel.
			@param[in]	inChannel	The caption channel whose messages are to be removed.
			@return		The number of messages that were removed.
		**/
		virtual size_t					Flush (const NTV2Line21Channel inChannel);

		/**
			@brief		Prints a human-readable representation of me to the given output stream in oldest-to-newest order.
			@param		inOutStream		Specifies the output stream to receive my human-readable representation.
			@param[in]	inDumpMessages	If true, also dump my messages;  otherwise don't dump my messages. Defaults to true.
			@return		The output stream that received my human-readable representation.
		**/
		virtual std::ostream &			Print (std::ostream & inOutStream, const bool inDumpMessages = true) const;


		/**
			@brief		Answers with my current depth.
			@return		The current number of queued messages.
		**/
		virtual size_t					GetQueuedMessageCount (void) const;

		/**
			@brief		Returns the number of messages queued for the given caption channel.
			@param[in]	inChannel	Specifies the caption channel of interest.
			@return		The current number of queued messages for the given caption channel.
		**/
		virtual size_t					GetQueuedMessageCount (const NTV2Line21Channel inChannel) const;

		/**
			@brief		Answers with the total number of bytes (including command bytes) that I currently have queued.
			@return		The current number of queued bytes.
		**/
		virtual size_t					GetQueuedByteCount (void) const;

		/**
			@brief		Returns the number of message bytes queued for the given caption channel.
			@param[in]	inChannel	Specifies the caption channel of interest.
			@return		The current number of queued message bytes for the given caption channel.
		**/
		virtual size_t					GetQueuedByteCount (const NTV2Line21Channel inChannel) const;

		/**
			@brief		Returns the total number of bytes (including command bytes) that have been
						enqueued onto me since I was instantiated.
		**/
		virtual size_t					GetEnqueueByteTally (void) const;

		/**
			@brief		Returns the total number of bytes (including command bytes) that have been
						dequeued from me since I was instantiated.
		**/
		virtual size_t					GetDequeueByteTally (void) const;

		/**
			@brief		Returns the total number of messages that have been enqueued onto me since I was instantiated.
		**/
		virtual size_t					GetEnqueueMessageTally (void) const;

		/**
			@brief		Returns the total number of messages that have been dequeued from me since I was instantiated.
		**/
		virtual size_t					GetDequeueMessageTally (void) const;


		/**
			@brief		Returns the total number of bytes (including command bytes) that have been
						enqueued onto me for the given caption channel since I was instantiated.
		**/
		virtual size_t					GetEnqueueByteTally (const NTV2Line21Channel inChannel) const;

		/**
			@brief		Returns the total number of bytes (including command bytes) that have been
						dequeued from me for the given caption channel since I was instantiated.
		**/
		virtual size_t					GetDequeueByteTally (const NTV2Line21Channel inChannel) const;

		/**
			@brief		Returns the total number of messages that have been enqueued onto me for the given caption channel since I was instantiated.
		**/
		virtual size_t					GetEnqueueMessageTally (const NTV2Line21Channel inChannel) const;

		/**
			@brief		Returns the total number of messages that have been dequeued from me for the given caption channel since I was instantiated.
		**/
		virtual size_t					GetDequeueMessageTally (const NTV2Line21Channel inChannel) const;


		/**
			@brief		Returns the highest queue depth -- i.e., the maximum number of messages I held in either of
						my queues -- since I was instantiated.
		**/
		virtual inline size_t			GetHighestQueueDepth (void) const				{return mMaxMsgTally;}

	private:
		CNTV2Caption608MessageQueue (const CNTV2Caption608MessageQueue & inObjToCopy);
		virtual CNTV2Caption608MessageQueue & operator = (const CNTV2Caption608MessageQueue & inRHS);


	//	Instance Data
	private:
		typedef	std::deque <CNTV2Caption608MessagePtr>	MyQueueType;
		typedef MyQueueType::const_iterator				MyQueueTypeConstIter;

		MyQueueType			mHiPriorityQueue;							///< @brief	My hi-priority queue -- for display messages
		MyQueueType			mLoPriorityQueue;							///< @brief	My lo-priority queue -- for non-display messages
		mutable void *		mpQueueLock;								///< @brief	Protects my queues from simultaneous access by more than one execution thread
		size_t				mEnqueueByteTally [NTV2_CC608_ChannelMax];	///< @brief	Total bytes enqueued (per caption channel)
		size_t				mDequeueByteTally [NTV2_CC608_ChannelMax];	///< @brief	Total bytes dequeued (per caption channel)
		size_t				mEnqueueMsgTally [NTV2_CC608_ChannelMax];	///< @brief	Total messages enqueued (per caption channel)
		size_t				mDequeueMsgTally [NTV2_CC608_ChannelMax];	///< @brief	Total messages dequeued (per caption channel)
		size_t				mMaxMsgTally;								///< @brief	Highest queue depth

};	//	CNTV2Caption608MessageQueue


//	Output stream operators:
inline std::ostream & operator << (std::ostream & inOutStream, const CNTV2Caption608MessageQueue & inObj)	{return inObj.Print (inOutStream);}



#endif	//	__NTV2_CAPTION608MESSAGEQUEUE__
