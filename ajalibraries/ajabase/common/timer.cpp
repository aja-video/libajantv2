/* SPDX-License-Identifier: MIT */
/**
	@file		timer.cpp
	@brief		Implements the AJATimer class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/common/common.h"
#include "ajabase/common/timer.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/system.h"
#include <iomanip>


AJATimer::AJATimer (const AJATimerPrecision inPrecision)
	:	mPrecision (inPrecision)
{
	Reset();
}


void AJATimer::Start (void)
{
	//	Save the time at start
	switch (mPrecision)
	{
		default:
		case AJATimerPrecisionMilliseconds:	mStartTime = AJATime::GetSystemMilliseconds();	break;
		case AJATimerPrecisionMicroseconds:	mStartTime = AJATime::GetSystemMicroseconds();	break;
		case AJATimerPrecisionNanoseconds:	mStartTime = AJATime::GetSystemNanoseconds();	break;
	}
	mRun = true;
}


void AJATimer::Stop (void)
{
	//	Save the time at stop...
	switch (mPrecision)
	{
		default:
		case AJATimerPrecisionMilliseconds:	mStopTime = AJATime::GetSystemMilliseconds();	break;
		case AJATimerPrecisionMicroseconds:	mStopTime = AJATime::GetSystemMicroseconds();	break;
		case AJATimerPrecisionNanoseconds:	mStopTime = AJATime::GetSystemNanoseconds();	break;
	}
	mRun = false;
}


void AJATimer::Reset (void)
{
	//	Clear the start and stop time
	mStartTime = mStopTime = 0;
	mRun = false;
}


uint32_t AJATimer::ElapsedTime (void) const
{
	if (IsRunning())	//	Running:
		switch (mPrecision)
		{
			default:
			case AJATimerPrecisionMilliseconds:	return uint32_t(AJATime::GetSystemMilliseconds() - mStartTime);
			case AJATimerPrecisionMicroseconds:	return uint32_t(AJATime::GetSystemMicroseconds() - mStartTime);
			case AJATimerPrecisionNanoseconds:	return uint32_t(AJATime::GetSystemNanoseconds() - mStartTime);
		}
	//	Stopped:
	return uint32_t(mStopTime - mStartTime);
}


#define __fDEC(__x__,__w__,__p__) std::dec << std::fixed << std::setw(__w__) << std::setprecision(__p__) << (__x__)

std::ostream & AJATimer::Print (std::ostream & oss) const
{
	const double secs(ETSecs());
	if (secs > 60.0)
	{
		const double mins (secs / 60.0);
		if (mins > 60.0)
		{
			const double hrs (mins / 60.0);
			if (hrs > 24.0)
			{
				const double days (hrs / 24.0);
				oss << __fDEC(days,4,1) << " days";
			}
			else
				oss << __fDEC(hrs,4,1) << " hrs";
		}
		else
			oss << __fDEC(mins,4,1) << " mins";
	}
	else if (secs >= 1.0)
		oss << __fDEC(secs,4,1) << " secs";
	else if (secs >= 1.0E-03)
		oss << __fDEC(secs*1.0E+3,4,1) << " msec";
	else if (secs >= 1.0E-06)
		oss << __fDEC(secs*1.0E+6,4,1) << " usec";
	else if (secs >= 1.0E-09)
		oss << __fDEC(secs*1.0E+9,4,1) << " nsec";
	else if (secs >= 1.0E-12)
		oss << __fDEC(secs*1.0E+12,4,1) << " psec";
	else
		oss << "0 sec";
	return oss;
}


std::string AJATimer::PrecisionName (const AJATimerPrecision precision, const bool longName)		//	STATIC
{
	if (longName)
		switch (precision)
		{
			default:
			case AJATimerPrecisionMilliseconds: return "milliseconds";
			case AJATimerPrecisionMicroseconds: return "microseconds";
			case AJATimerPrecisionNanoseconds:	return "nanoseconds";
		}
	else
		switch (precision)
		{
			default:
			case AJATimerPrecisionMilliseconds: return "ms";
			case AJATimerPrecisionMicroseconds: return "us";
			case AJATimerPrecisionNanoseconds:	return "ns";
		}
}

double AJATimer::PrecisionSecs (const AJATimerPrecision precision)		//	STATIC
{
	switch (precision)
	{
		default:
		case AJATimerPrecisionMilliseconds:	return 1.0E-03;
		case AJATimerPrecisionMicroseconds:	return 1.0E-06;
		case AJATimerPrecisionNanoseconds:	return 1.0E-09;
	}
}
