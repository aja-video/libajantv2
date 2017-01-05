/**
	@file		timer.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJATimer class.
**/

#include "ajabase/common/common.h"
#include "ajabase/common/timer.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/system.h"


AJATimer::AJATimer(AJATimerPrecision precision)
{
	mStartTime = 0;
	mStopTime = 0;
	mRun = false;
    mPrecision = precision;
}


AJATimer::~AJATimer()
{
}


void 
AJATimer::Start()
{
	// save the time at start
    switch(mPrecision)
    {
        default:
        case AJATimerPrecisionMilliseconds:
            mStartTime = AJATime::GetSystemMilliseconds();
            break;

        case AJATimerPrecisionMicroseconds:
            mStartTime = AJATime::GetSystemMicroseconds();
            break;
    }

	mRun = true;
}


void 
AJATimer::Stop()
{
	// save the time at stop
    switch(mPrecision)
    {
        default:
        case AJATimerPrecisionMilliseconds:
            mStopTime = AJATime::GetSystemMilliseconds();
            break;

        case AJATimerPrecisionMicroseconds:
            mStopTime = AJATime::GetSystemMicroseconds();
            break;
    }

	mRun = false;
}


void 
AJATimer::Reset()
{
	// clear the start and stop time
	mStartTime = 0;
	mStopTime = 0;
	mRun = false;
}


uint32_t 
AJATimer::ElapsedTime()
{
	uint32_t elapsedTime = 0;

	// compute the elapsed time when running
	if (mRun)
	{
        switch(mPrecision)
        {
            default:
            case AJATimerPrecisionMilliseconds:
                elapsedTime = (uint32_t)(AJATime::GetSystemMilliseconds() - mStartTime);
                break;

            case AJATimerPrecisionMicroseconds:
                elapsedTime = (uint32_t)(AJATime::GetSystemMicroseconds() - mStartTime);
                break;
        }
	}
	// compute the elapsed time when stopped
	else
	{
		elapsedTime = (uint32_t)(mStopTime - mStartTime);
	}

	return elapsedTime;
}


bool 
AJATimer::Timeout(uint32_t interval)
{
	// timeout is true if greater than specified interval
	if (ElapsedTime() >= interval)
	{
		return true;
	}

	return false;
}


bool
AJATimer::IsRunning(void)
{
	return mRun;
}

AJATimerPrecision
AJATimer::Precision(void)
{
    return mPrecision;
}
