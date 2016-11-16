/**
	@file		ntv2rawdngwriter.h
	@brief		Header file for the NTV2RawDngWriter class that is used to write frames
				in raw format to files on the storage medium.
	@copyright	Copyright (C) 2013-2014 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef _NTV2RAWDNGWRITER_H
#define _NTV2RAWDNGWRITER_H

#include "ajabase/common/circularbuffer.h"
#include "ajabase/common/types.h"
#include "ajabase/system/thread.h"
#include "ntv2enums.h"
#include "ntv2democommon.h"

class NTV2RawDngWriter	:	public AJAThread
{
	//	Public Instance Methods
	public:

					NTV2RawDngWriter (AJACircularBuffer <AVDataBuffer *> & circularBuffer);
	virtual			~NTV2RawDngWriter (void);

	virtual	bool	SetFileNameBase			(const std::string fileNameBase);
	virtual bool	SetFileNameSequence		(const uint32_t fileNameSequence);
	virtual bool	SetIncrementSequence	(const bool incrementSequence);
	virtual bool	SetRecordState			(const bool recordState);
	virtual	bool	SetFrameRate			(const NTV2FrameRate frameRate);
	virtual bool	SetRasterDimensions		(const uint32_t width, const uint32_t height);

	protected:

	uint32_t		mDuration;
	std::string		mFileNameBase;
	uint32_t		mFileNameSequence;
	bool			mIncrementSequence;
	NTV2FrameRate	mFrameRate;
	uint32_t		mRasterWidth;
	uint32_t		mRasterHeight;
	bool			mRecording;
	uint32_t		mScale;

	AJACircularBuffer <AVDataBuffer *> &	mCircularBuffer;

	virtual	bool	ThreadLoop (void);
};

#endif	//	_NTV2RAWDNGWRITER_H
