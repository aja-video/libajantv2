/* SPDX-License-Identifier: MIT */
/**
	@file		systemtime.cpp
	@brief		Implements the AJATime class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/systemtime.h"
#if defined(AJA_COLLECT_SLEEP_STATS)
	#include "ajabase/common/timer.h"
	#include "ajabase/system/atomic.h"
	#include "ajabase/system/thread.h"
#endif	//	defined(AJA_COLLECT_SLEEP_STATS)

#if defined(AJA_MAC)
	#include <mach/mach_time.h>
	#include <CoreServices/CoreServices.h>
	static int64_t s_PerformanceFrequency;
	static bool s_bPerformanceInit = false;
#endif
#if defined(AJA_USE_CPLUSPLUS11)
	#include <chrono>
	#include <thread>
#endif	//	AJA_USE_CPLUSPLUS11

#if defined(AJA_WINDOWS)
	#include "timeapi.h"
	static LARGE_INTEGER s_PerformanceFrequency;
	static bool s_bPerformanceInit = false;
#endif

#if defined(AJA_LINUX)
	#include <unistd.h>
	#if (_POSIX_TIMERS > 0)
		#ifdef _POSIX_MONOTONIC_CLOCK
			#define AJA_USE_CLOCK_GETTIME
		#else
			#undef AJA_USE_CLOCK_GETTIME
		#endif
	#endif
	
	#ifdef AJA_USE_CLOCK_GETTIME
		#include <time.h>
	#else
		// Use gettimeofday - this is not really desirable
		#include <sys/time.h>
	#endif
#endif


int64_t AJATime::GetSystemTime (void)
{
	// system dependent time function
#if defined(AJA_WINDOWS)
	return (int64_t)::timeGetTime();
#endif

#if defined(AJA_MAC)
	static mach_timebase_info_data_t	sTimebaseInfo;
	uint64_t ticks = ::mach_absolute_time();
	
	if ( sTimebaseInfo.denom == 0 )
	{
		(void) mach_timebase_info(&sTimebaseInfo);
	}
	
	// Do the maths. We hope that the multiplication doesn't
	// overflow; the price you pay for working in fixed point.
	int64_t nanoSeconds = ticks * sTimebaseInfo.numer / sTimebaseInfo.denom;
	
	return nanoSeconds;
#endif

#if defined(AJA_LINUX)
#ifdef AJA_USE_CLOCK_GETTIME
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * ((int64_t)1000)) + (ts.tv_nsec / (int64_t)1000000);
#else
	struct timeval tv;
	struct timezone tz;

	gettimeofday( &tv, &tz );
	return (int64_t)((tv.tv_sec * ((int64_t)1000)) + (int64_t)(tv.tv_usec / 1000));
#endif
#endif
}


int64_t AJATime::GetSystemCounter (void)
{
#if defined(AJA_WINDOWS)
	LARGE_INTEGER performanceCounter;

	performanceCounter.QuadPart = 0;
	if (!QueryPerformanceCounter(&performanceCounter))
	{
		return 0;
	}

	return (int64_t)performanceCounter.QuadPart;
#endif

#if defined(AJA_MAC)
	//return int64_t(::clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW));
	return (mach_absolute_time());
#endif

#if defined(AJA_LINUX)
#ifdef AJA_USE_CLOCK_GETTIME
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * ((int64_t)1000000000)) + (ts.tv_nsec);
#else
	struct timeval tv;
	struct timezone tz;

	gettimeofday( &tv, &tz );
	return (int64_t)((int64_t)tv.tv_sec * (int64_t)1000000 + tv.tv_usec);
#endif
#endif
}


int64_t AJATime::GetSystemFrequency (void)
{
#if defined(AJA_WINDOWS)
	if (!s_bPerformanceInit)
	{
		QueryPerformanceFrequency(&s_PerformanceFrequency);
		s_bPerformanceInit = true;
	}

	return (int64_t)s_PerformanceFrequency.QuadPart;
#endif

#if defined(AJA_MAC)
	if (!s_bPerformanceInit)
	{
		// 1 billion ticks approximately equals 1 sec on a Mac
		static mach_timebase_info_data_t	sTimebaseInfo;
		uint64_t ticks = 1000000000;
		
		if ( sTimebaseInfo.denom == 0 )
		{
			(void) mach_timebase_info(&sTimebaseInfo);
		}
		
		// Do the maths. We hope that the multiplication doesn't
		// overflow; the price you pay for working in fixed point.
		int64_t nanoSeconds = ticks * sTimebaseInfo.numer / sTimebaseInfo.denom;
	   
		// system frequency - ticks per second units
		s_PerformanceFrequency = ticks * 1000000000 / nanoSeconds;	
		s_bPerformanceInit = true;
	}
	
	return s_PerformanceFrequency;
#endif

#if defined(AJA_LINUX)
#ifdef AJA_USE_CLOCK_GETTIME
	return 1000000000;
#else
	return 1000000;
#endif
#endif
}


double AJATime::GetSystemSeconds (void)
{
	double ticks = double(GetSystemCounter());
	double ticksPerSecond = double(GetSystemFrequency());
	double sec = 0.0;
	if (ticksPerSecond)
	{
		sec = ticks / ticksPerSecond;
	}
	return sec;
}


uint64_t AJATime::GetSystemMilliseconds (void)
{				
	uint64_t ticks			= GetSystemCounter();
	uint64_t ticksPerSecond = GetSystemFrequency();
	uint64_t ms				= 0;
	if (ticksPerSecond)
	{
		// floats are being used here to avoid the issue of overflow
		// or inaccuracy when chosing where to apply the '1000' correction
		ms = uint64_t((double(ticks) / double(ticksPerSecond)) * 1000.);
	}
	return ms;
}


uint64_t AJATime::GetSystemMicroseconds (void)
{
	uint64_t ticks			= GetSystemCounter();
	uint64_t ticksPerSecond = GetSystemFrequency();
	uint64_t us				= 0;
	if (ticksPerSecond)
	{
		// floats are being used here to avoid the issue of overflow
		// or inaccuracy when chosing where to apply the '1000000' correction
		us = uint64_t((double(ticks) / double(ticksPerSecond)) * 1000000.);
	}
	return us;
}


uint64_t AJATime::GetSystemNanoseconds (void)
{
	uint64_t ticks			= GetSystemCounter();
	uint64_t ticksPerSecond = GetSystemFrequency();
	uint64_t us				= 0;
	if (ticksPerSecond)
	{
		// floats are being used here to avoid the issue of overflow
		// or inaccuracy when chosing where to apply the '1000000000' correction
		us = uint64_t((double(ticks) / double(ticksPerSecond)) * 1000000000.);
	}
	return us;
}


#if defined(AJA_COLLECT_SLEEP_STATS)
	static uint64_t		sMonThreadID	= 0;//   1    2    3    4    5    6    7     8     9     10     11     12     13      14      15      16
	static const double	sPercentiles[]	= {	1.0, 1.1, 1.2, 1.5, 2.0, 3.0, 6.0, 11.0, 21.0, 51.0, 101.0, 201.0, 501.0, 1001.0, 2001.0, 5001.0, 10001.0};
	static uint32_t		sCounts[]		= {	0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,     0,     0,     0,      0,      0,      0,       0};
	static int			sNumPercentiles	= sizeof(sPercentiles) / sizeof(double);	//	17
	static int			sNumCounts		= sizeof(sCounts) / sizeof(uint32_t);		//	18
	static AJATimer		sTimer (AJATimerPrecisionMicroseconds);

	#define	PRE_STATS	const bool doStats(sMonThreadID == AJAThread::GetThreadId());	\
						if (doStats)									\
						{	sTimer.Reset();								\
							sTimer.Start();								\
						}

	#define	POST_STATS(_req_)	if (doStats)											\
								{	sTimer.Stop();										\
									const double act(sTimer.ETSecs());					\
									const double req(_req_);							\
									for (int n(0);  n < sNumPercentiles;  n++)			\
										if (act < (sPercentiles[n] * req))				\
										{												\
											AJAAtomic::Increment(&sCounts[n]);			\
											return;										\
										}												\
									AJAAtomic::Increment(&sCounts[sNumPercentiles]);	\
								}
#else	//	AJA_COLLECT_SLEEP_STATS
	#define	PRE_STATS
	#define	POST_STATS(__x__)
#endif	//	!defined(AJA_COLLECT_SLEEP_STATS)


// sleep time in milliseconds
void AJATime::Sleep (int32_t inTime)
{
	if (inTime <= 0)
		return;	//	Don't sleep at all

	PRE_STATS
	#if defined(AJA_USE_CPLUSPLUS11)
		std::this_thread::sleep_for(std::chrono::milliseconds(inTime));
	#elif defined(AJA_WINDOWS)
		::Sleep(DWORD(inTime));
	#else	//	POSIX
		#if 0	//	usleep is deprecated in POSIX
			usleep(inTime * 1000);
		#else
			timespec req, rm;
			req.tv_sec = 0;
			req.tv_nsec = long(inTime) * 1000L * 1000L;
			rm.tv_sec = 0; rm.tv_nsec = 0;
			if (::nanosleep(&req, &rm) < 0)
				;	//	failed
		#endif
	#endif
	POST_STATS(double(inTime) / 1000.0);
}


// sleep time in microseconds
void AJATime::SleepInMicroseconds (int32_t inTime)
{
	if (inTime <= 0)
		return;	//	Don't sleep at all

	PRE_STATS
	#if defined(AJA_USE_CPLUSPLUS11)
		std::this_thread::sleep_for(std::chrono::microseconds(inTime));
	#elif defined(AJA_WINDOWS)
		::Sleep(DWORD(inTime) / 1000);	//	Windows Sleep expects millisecs
	#else	//	POSIX
		#if 0	//	usleep is deprecated in POSIX
			usleep(inTime);
		#else
			timespec req, rm;
			req.tv_sec = 0;
			req.tv_nsec = long(inTime) * 1000L;
			rm.tv_sec = 0; rm.tv_nsec = 0;
			if (::nanosleep(&req, &rm) < 0)
				;	//	failed
		#endif
	#endif
	POST_STATS(double(inTime) / 1000000.0);
}


#if defined(AJA_COLLECT_SLEEP_STATS)
	bool AJATime::CollectSleepStats (const bool inEnable)
	{
		if (!inEnable)
			{sMonThreadID = 0;  return true;}

		if (sMonThreadID != AJAThread::GetThreadId())
			for (int n(0);  n < sNumPercentiles;  n++)
				sCounts[n] = 0;
		sMonThreadID = AJAThread::GetThreadId();
		return true;
	}

	std::string	AJATime::GetSleepStats (void)
	{
		std::ostringstream oss;
		for (int n(0);  n < sNumCounts;  n++)
			if (sCounts[n])
			{
				oss << sCounts[n] << "\t< " << sPercentiles[n] << std::endl;
			}
		return oss.str();
	}
#endif	//	defined(AJA_COLLECT_SLEEP_STATS)
