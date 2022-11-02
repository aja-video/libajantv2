/* SPDX-License-Identifier: MIT */
/**
	@file		timer.h
	@brief		Declares the AJATimer class.
	@copyright	(C) 2009-2022 AJA Video Systems, Inc.  All rights reserved.
**/

#ifndef AJA_TIMER_H
#define AJA_TIMER_H

#include <ostream>
#include "ajabase/common/public.h"

enum AJATimerPrecision
{
	AJATimerPrecisionMilliseconds,
	AJATimerPrecisionMicroseconds,
	AJATimerPrecisionNanoseconds
};

/**
 *	Class to support timing of events
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJATimer
{
	public:
									AJATimer (const AJATimerPrecision precision = AJATimerPrecisionMilliseconds);
		virtual						~AJATimer()		{}

		/**
		 *	Starts me. If already running, re-starts me.
		 */
		void						Start (void);

		/**
		 *	Stops me. If already stopped, re-stops me.
		 */
		void						Stop (void);

		/**
		 *	Resets me to a stopped state with no elapsed time, without changing my precision.
		 */
		void						Reset (void);

		/**
		 *	Get the elapsed time.
		 *
		 *	If the timer is running, return the elapsed time since Start() was called.	If Stop() 
		 *	has been called, return the time between Start() and Stop().
		 *
		 *	@return		The elapsed time in selected timer precision units
		 */
		uint32_t					ElapsedTime (void) const;

		/**
		 *	Get the elapsed time, in seconds, as a double-precision floating point value.
		 *
		 *	If the timer is running, return the elapsed time since Start() was called.	If Stop() 
		 *	has been called, return the time elapsed between Start() and Stop().
		 *
		 *	@return		The elapsed time, in seconds
		 */
		inline double				ETSecs (void) const							{return double(ElapsedTime()) * PrecisionSecs(Precision());}

		/**
		 *	Check for timeout.
		 *
		 *	Timeout checks the ElapsedTime() and returns true if it is greater than interval.
		 *
		 *	@param	interval	Timeout interval in selected timer precision units.
		 *	@return				true if elapsed time greater than interval.
		 */
		inline bool					Timeout (const uint32_t interval) const		{return (ElapsedTime() >= interval);}

		/**
		 *	Is the timer running.
		 *
		 *	@return				true if timer is running.
		 */
		inline bool					IsRunning (void) const						{return mRun;}

		/**
		 *	Return the timer precision enum.
		 *
		 *	@return				precision enum that was used in the constructor.
		 */
		inline AJATimerPrecision	Precision (void) const						{return mPrecision;}


		/**
		 *	Prints my elapsed time into the given output stream.
		 *
		 *	@param		oss		the output stream to use
		 *	@return				the output stream being used
		 */
		std::ostream &				Print (std::ostream & oss) const;


		/**
		 *	Return the display string for the given timer precision enum.
		 *
		 *	@param	precision	The precision enum to get the display string for.
		 *	@param	longName	If true the string is set to a long description, otherwise an abbreviation.
		 *	@return				string description
		 */
		static std::string			PrecisionName (const AJATimerPrecision precision, const bool longName = true);

		/**
		 *	Return the fraction of a second as a double-precision floating point value for the given timer precision.
		 *
		 *	@param	precision	The precision enum to get the display string for.
		 *	@return				double-precision value
		 */
		static double				PrecisionSecs (const AJATimerPrecision precision);

	private:
		uint64_t			mStartTime;
		uint64_t			mStopTime;
		bool				mRun;
		AJATimerPrecision	mPrecision;

	#if defined(DOCTEST_LIBRARY_INCLUDED)
		public: inline void	SetStopTime (const uint64_t inIncr)	{mStartTime = 0; mStopTime = inIncr;};
	#endif
};	//	AJATimer

AJA_EXPORT inline std::ostream & operator << (std::ostream & oss, const AJATimer & inObj)	{return inObj.Print(oss);}

#endif	//	AJA_TIMER_H
