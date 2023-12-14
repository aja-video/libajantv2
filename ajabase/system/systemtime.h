/* SPDX-License-Identifier: MIT */
/**
	@file		systemtime.h
	@brief		Declares the AJATime class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TIME_H
	#define AJA_TIME_H

#if defined(AJA_USE_CPLUSPLUS11) && !defined(AJA_BAREMETAL)
	// If compiling with C++11, by default, implementation uses STL chrono & thread.
	#ifndef AJA_SLEEP_USE_STL
		#define	AJA_SLEEP_USE_STL	//	Sleep... functions use STL chrono/thread;  comment this out to use native impl
	#endif
	// #ifndef AJA_SYSCLK_USE_STL
	// 	#define	AJA_SYSCLK_USE_STL	//	GetSystem... functions use STL chrono;  comment this out to use native impl (TBD)
	// #endif
#endif	//	AJA_USE_CPLUSPLUS11

//	#define	AJA_COLLECT_SLEEP_STATS		//	Define this to allow thread-specific stat collection for Sleep & SleepInMicroseconds
	#include "ajabase/common/public.h"
	#if defined(AJA_COLLECT_SLEEP_STATS)
		#include <string>
	#endif	//	defined(AJA_COLLECT_SLEEP_STATS)

	/** 
		@brief		Collection of platform-independent host system clock time functions.
		@ingroup	AJAGroupSystem
	**/
	class AJA_EXPORT AJATime
	{
		public:

		/**
			@brief		Returns the current value of the host system's low-resolution clock, in milliseconds.
			@return		The current value of the host system's low-resolution clock, in milliseconds.
		**/
		static int64_t	GetSystemTime (void);

		/**
			@brief		Returns the current value of the host system's high-resolution time counter.
			@return		The current value of the host system's high-resolution time counter.
		**/
		static int64_t	GetSystemCounter (void);

		/**
			@brief		Returns the frequency of the host system's high-resolution time counter.
			@return		The high resolution counter frequency in ticks per second.
		**/
		static int64_t	GetSystemFrequency (void);

		/**
			@brief		Returns the current value of the host's high-resolution clock, in seconds.
			@return		Current value of the host's clock, in seconds, based on GetSystemCounter() and GetSystemFrequency().
		**/
		static double 	GetSystemSeconds (void);

		/**
			@brief		Returns the current value of the host's high-resolution clock, in milliseconds.
			@return		Current value of the host's clock, in milliseconds, based on GetSystemCounter() and GetSystemFrequency().
		**/
		static uint64_t GetSystemMilliseconds (void);

		/**
			@brief		Returns the current value of the host's high-resolution clock, in microseconds.
			@return		Current value of the host's clock, in microseconds, based on GetSystemCounter() and GetSystemFrequency().
		**/
		static uint64_t GetSystemMicroseconds (void);

		/**
			@brief		Returns the current value of the host's high-resolution clock, in nanoseconds.
			@return		Current value of the host's clock, in nanoseconds, based on GetSystemCounter() and GetSystemFrequency().
		**/
		static uint64_t GetSystemNanoseconds (void);

		/**
			@brief		Suspends execution of the current thread for a given number of milliseconds.
			@param		inMilliseconds		Specifies the sleep time, in milliseconds.
		**/
		static void		Sleep (const int32_t inMilliseconds);

		/**
			@brief		Suspends execution of the current thread for a given number of microseconds.
			@param		inMicroseconds		Time to sleep (microseconds).
		**/
		static void		SleepInMicroseconds (const int32_t inMicroseconds);

		/**
			@brief		Suspends execution of the current thread for a given number of nanoseconds.
			@param		inMicroseconds		Time to sleep (nanoseconds).
		**/
		static void		SleepInNanoseconds (const uint64_t inNanoseconds);

		#if defined(AJA_COLLECT_SLEEP_STATS)
			static bool			CollectSleepStats (const bool inEnable = true);
			static std::string	GetSleepStats (void);
		#endif	//	AJA_COLLECT_SLEEP_STATS
	};	//	AJATime

#endif	//	AJA_TIME_H
