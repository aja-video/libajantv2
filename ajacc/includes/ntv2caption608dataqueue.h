/**
	@file		ntv2caption608dataqueue.h
	@brief		Declaration for the CNTV2Caption608DataQueue class.
	@copyright	(C) 2007-2022 AJA Video Systems, Inc. All rights reserved.
**/
#ifndef __NTV2_CEA608_DATAQUEUE_
#define __NTV2_CEA608_DATAQUEUE_

#include "ntv2caption608types.h"
#include "ntv2captionlogging.h"
#ifdef MSWindows
	#include "windows.h"
	#include "stdio.h"
#endif
#include <deque>


/**
	@brief	Each queue entry has two caption data bytes and a flag that
			indicates if those bytes contain valid data or not.
**/
typedef struct QueueData608
{
	bool	fGotData;	///	True if my data bytes are valid; otherwise false
	UByte	fChar1;		///	Caption data byte 1
	UByte	fChar2;		///	Caption data byte 2

} QueueData608;


/**
	@brief	I am a simple, thread-safe queue of CEA-608 caption byte pairs.
**/
class AJAExport CNTV2Caption608DataQueue : public CNTV2CaptionLogConfig
{
	//	INSTANCE METHODS
	public:
						CNTV2Caption608DataQueue (const NTV2Line21Field inFieldOfInterest = NTV2_CC608_Field_Invalid);
		virtual			~CNTV2Caption608DataQueue ();

		virtual void	Flush (void);
		virtual void	SetField (const NTV2Line21Field inField)	{mFieldOfInterest = IsValidLine21Field (inField) ? inField : NTV2_CC608_Field_Invalid;}

		virtual bool	Push608Data (const UByte char1, const UByte char2, const bool bGotData);
		virtual bool	Pop608Data (UByte & outChar1, UByte & outChar2, bool & outGotData);
		virtual bool	IsEmpty (void) const;

		virtual size_t	GetCurrentDepth (void) const;
		virtual size_t	GetMaximumDepth (void) const;
		virtual size_t	GetEnqueueTally (void) const;
		virtual size_t	GetDequeueTally (void) const;


	//	Instance Data
	private:
		std::deque <QueueData608>	mDataQueue;				///< @brief	My data queue
		mutable void *				mpQueueLock;			///< @brief	Protects my queue from simultaneous access by more than one execution thread
		NTV2Line21Field				mFieldOfInterest;		///< @brief	For logging

		//	Stats
		size_t						mEnqueueTally;			///< @brief	Total number of enqueues
		size_t						mDequeueTally;			///< @brief	Total number of dequeues
		size_t						mHighestQueueDepth;		///< @brief	Highest queue depth

};	//	CNTV2Caption608DataQueue

#endif	// __NTV2_CEA608_DATAQUEUE_
