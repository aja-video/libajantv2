/**
	@file		i_playcorder.h
	@brief		Header file for the interface between a playcorder UI
				and an inplementation class that does the actuall work.
	@copyright	Copyright 2013 AJA Video Systems, Inc. All rights reserved.
**/

#ifndef IPLAYCORDER_H
#define IPLAYCORDER_H


#include <string>
#include "ajastuff/common/circularbuffer.h"
#include "ajastuff/common/types.h"
#include "ajastuff/common/videotypes.h"

class IPlaycorder
{

	public:

		typedef struct
		{
			uint8_t  *	pPreviewFrame;
			uint32_t	previewFrameSize;
		} PreviewFrameInfo;

		static	 		IPlaycorder *		GetIPlaycorder ();
		virtual 							~IPlaycorder () {};

		virtual			AJAStatus			Init (const uint32_t previewWidth,
												  const uint32_t previewHeight)		= 0;

		virtual 		uint32_t			GetDeviceCount	() const				= 0;
		virtual const	std::string &		GetDeviceString (const uint32_t index)	= 0;
		virtual			AJAStatus			SetDevice (const uint32_t index)		= 0;

		virtual			AJA_VideoFormat		GetInputFormat	() const				= 0;
		virtual const	std::string &		GetInputFormatString ()					= 0;

		virtual			bool				IsPlaying () const						= 0;
		virtual			AJAStatus			StartPlaying (uint32_t & clipLength)	= 0;
		virtual			bool				GetPlayPaused () const					= 0;
		virtual			AJAStatus			SetPlayPaused (const bool pause)		= 0;
		virtual			uint32_t			GetPlayFrame () const					= 0;
		virtual			AJAStatus			SetPlayFrame (const uint32_t number)	= 0;
		virtual			AJAStatus			StopPlaying ()							= 0;
		virtual			AJAStatus			SetPlayPath (const std::string path)	= 0;
		virtual			uint32_t			GetPlayDropCount () const				= 0;

		virtual			bool				IsRecording () const					= 0;
		virtual			bool				IsWritingToStorage () const				= 0;
		virtual			AJAStatus			StartRecording ()						= 0;
		virtual			bool				GetRecordPaused () const				= 0;
		virtual			AJAStatus			SetRecordPaused (const bool pause)		= 0;
		virtual			AJAStatus			StopRecording ()						= 0;
		virtual			AJAStatus			SetRecordPath (const std::string path)	= 0;
		virtual			AJAStatus			WriteToStorage (const bool enable)		= 0;
		virtual			uint32_t			GetRecordDropCount () const				= 0;

		virtual			AJAStatus			StopAll ()								= 0;

		virtual			AJACircularBuffer<PreviewFrameInfo *> *
											GetPreviewCircularBuffer ()				= 0;

};	// IPlaycorder

#endif	//	IPLAYCORDER_H

