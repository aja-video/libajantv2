/**
	@file		timer.h
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJATimer class.
**/

#ifndef AJA_TIMER_H
#define AJA_TIMER_H

#include "ajabase/common/public.h"

enum AJATimerPrecision
{
    AJATimerPrecisionMilliseconds,
    AJATimerPrecisionMicroseconds
};

/**
 *	Class to support timing of events
 *	@ingroup AJAGroupSystem
 */
class AJA_EXPORT AJATimer
{
public:

    AJATimer(AJATimerPrecision precision = AJATimerPrecisionMilliseconds);
	virtual ~AJATimer();

	/**
	 *	Start the timer.
	 */
	void Start();

	/**
	 *	Stop the timer.
	 */
	void Stop();

	/**
	 *	Reset the timer.
	 */
	void Reset();

	/**
	 *	Get the elapsed time.
	 *
	 *	If the timer is running, return the elapsed time since Start() was called.  If Stop() 
	 *	has been called, return the time between Start() and Stop().
	 *
	 *	@return		The elapsed time in milliseconds.
	 */
	uint32_t ElapsedTime();

	/**
	 *	Check for timeout.
	 *
	 *	Timeout checks the ElapsedTime() and returns true if it is greater than interval.
	 *
	 *	@param	interval	Timeout interval in milliseconds.
	 *	@return				true if elapsed time greater than interval.
	 */
	bool Timeout(uint32_t interval);

	/**
	 *	Is the timer running.
	 *
	 *	@return				true if timer is running.
	 */
	bool IsRunning(void);

    /**
     *	Return the timer precision enum.
     *
     *	@return				precision enum that was used in the constructor.
     */
    AJATimerPrecision Precision(void);

private:

    uint64_t            mStartTime;
    uint64_t            mStopTime;
    bool                mRun;
    AJATimerPrecision   mPrecision;
};

#endif	//	AJA_TIMER_H
