/**
	@file		ntv2caption708serviceblockqueue.h
	@brief		Declares the CNTV2Caption708ServiceBlockQueue class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef __NTV2_CEA708_SERVICEBLOCKQUEUE_
#define __NTV2_CEA708_SERVICEBLOCKQUEUE_

#include "ntv2captionlogging.h"
#include <deque>
#ifdef MSWindows
	#include "windows.h"
#endif


/**
	@brief	A UByteQueue is a std::deque of unsigned byte values.
**/
typedef	std::deque <UByte>			UByteQueue;
typedef	UByteQueue::iterator		UByteQueueIter;
typedef	UByteQueue::const_iterator	UByteQueueConstIter;


/**
	@brief	I am a simple, thread-safe queue of CEA-708 caption service blocks.
**/
class AJAExport CNTV2Caption708ServiceBlockQueue : public CNTV2CaptionLogConfig
{
	//	Instance Methods
	public:
		explicit		CNTV2Caption708ServiceBlockQueue ();
		virtual			~CNTV2Caption708ServiceBlockQueue ();

		/**
			@brief	Flushes me.
		**/
		virtual void	Flush (void);

		/**
			@brief	Returns true if I'm currently empty.
		**/
		virtual bool	IsEmpty (void) const;

		/**
			@brief	Returns the number of Service Blocks that I contain.
		**/
		virtual size_t	GetCurrentDepth (void) const;

		/**
			@brief	Returns the highest number of Service Blocks that I contain or ever have contained since I was instantiated.
		**/
		virtual size_t	GetHighestDepth (void) const;

		/**
			@brief	Returns the number of "push" calls that have been made since I was instantiated.
		**/
		virtual size_t	GetEnqueueTally (void) const;

		/**
			@brief	Returns the number of "pop" calls that have been made since I was instantiated.
		**/
		virtual size_t	GetDequeueTally (void) const;

		/**
			@brief	Returns the total number of bytes that have been enqueued since I was instantiated.
		**/
		virtual size_t	GetEnqueueByteTally (void) const;

		/**
			@brief	Returns the total number of bytes that have been dequeued since I was instantiated.
		**/
		virtual size_t	GetDequeueByteTally (void) const;

		/**
			@brief	Enqueues the given service block.
			@param	pInServiceBlockData			A valid, non-NULL pointer to a buffer containing the service block data, including the header.
			@param	inServiceBlockByteCount		Specifies the exact number of valid bytes in the source buffer to be copied into my queue.
			@return	True if successful; otherwise false.
		**/
		virtual bool	PushServiceBlock (const UByte * pInServiceBlockData, const size_t inServiceBlockByteCount);

		/**
			@brief	Returns information about the [next] Service Block that's sitting at the top of my queue, if any.
					My queue is left unchanged.
			@param	outBlockSize	Receives the size of the service block, in bytes.
			@param	outDataSize		Receives the size of the service block data, in bytes.
			@param	outServiceNum	Receives the Service Number.
			@param	outIsExtended	Receives "true" if the service block is an "extended" one; otherwise, receives "false".
			@return	True if successful; otherwise false.
		**/
		virtual bool	PeekNextServiceBlockInfo (size_t & outBlockSize, size_t & outDataSize, int & outServiceNum, bool & outIsExtended) const;

		/**
			@brief	Pops the next Service Block off my queue and copies it into the byte vector.
			@param[out]	outData		Receives the service block byte(s).
			@note	This method copies the entire Service Block. To copy just the data, use PopServiceBlockData.
			@return	The number of bytes that were copied.
		**/
		virtual size_t	PopServiceBlock (std::vector<UByte> & outData);

		/**
			@brief	Pops the next Service Block off my queue and copies just its data into the designated buffer.
			@param[out]	outData		Receives just the data byte(s) from the service block.
			@note	This method copies just the data byte(s) from the Service Block. To copy the entire Service Block, use PopServiceBlock.
			@return	The number of bytes that were copied.
		**/
		virtual size_t	PopServiceBlockData (std::vector<UByte> & outData);


		virtual size_t	PopServiceBlock (UByte * pOutDataBuffer);		///< @deprecated	Use vector<UByte> version instead.
		virtual size_t	PopServiceBlockData (UByte * pOutDataBuffer);	///< @deprecated	Use vector<UByte> version instead.

		//	DEBUGGING
		/**
			@brief	Sets my channel number (used in log messages).
			@param	inChannelNum	Specifies my channel number used in log messages.
		**/
		virtual void	SetDebugChannel (const int inChannelNum);

		virtual std::ostream &	Print	(std::ostream & inOutStrm, const bool inWithData = false) const;

	private:
		//	Hide Stuff
		explicit		CNTV2Caption708ServiceBlockQueue (const CNTV2Caption708ServiceBlockQueue & inObj);
		virtual			CNTV2Caption708ServiceBlockQueue & operator = (const CNTV2Caption708ServiceBlockQueue & inRHS);

		// Debug
		virtual std::string	GetChannelString (void) const;


	//	Instance Data
	private:
		UByteQueue		mServiceBlockQueue;		///< @brief	My byte queue
		mutable void *	mpQueueLock;			///< @brief	Protects my queue from simultaneous access by more than one execution thread
		int				mDebugChannel;			///< @brief	Identifies my channel in log messages

		//	Stats
		size_t			mEnqueueTally;			///< @brief	Total number of enqueues
		size_t			mDequeueTally;			///< @brief	Total number of dequeues
		size_t			mEnqueueByteTally;		///< @brief	Total number of bytes enqueued
		size_t			mDequeueByteTally;		///< @brief	Total number of bytes dequeued
		size_t			mHighestQueueDepth;		///< @brief	Highest queue depth

};	//	CNTV2Caption708ServiceBlockQueue


AJAExport std::ostream & operator << (std::ostream & inOutStrm, const CNTV2Caption708ServiceBlockQueue & inQueue);

#endif	// __NTV2_CEA708_SERVICEBLOCKQUEUE_
