// file: performancecounter.cpp
//
// Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
// 
#include <sys/time.h>
#include "performancecounter.h"

CPerformanceCounter::CPerformanceCounter()
{
	_beginTime64 =  _endTime64 = 0;
	_ticks = _maxticks = 0;
    _minticks = 0x7FFFFFFFFFFFFFFFULL;
}

CPerformanceCounter::~CPerformanceCounter()
{

}

void CPerformanceCounter::Start()
{
	// Get Start Time	
	gettimeofday(&_tstart, &tz);
	_beginTime64 = (_tstart.tv_sec * 1000) + _tstart.tv_usec;
}

__int64 CPerformanceCounter::Stop()
{
	// Get End Time	
	gettimeofday(&_tend, &tz);
	_endTime64 = (_tend.tv_sec * 1000) + _tend.tv_usec;

	_ticks = (_endTime64 - _beginTime64) ;

    // Of course don't do this if the overhead is too much for you.
    if ( _ticks < _minticks )
        _minticks = _ticks;

    if ( _ticks > _maxticks )
        _maxticks = _ticks;

	return _ticks;
}

double CPerformanceCounter::GetCurrentSeconds()
{
	// Get End Time	
	gettimeofday(&_tend, &tz);
	_endTime64 = (_tend.tv_sec * 1000) + _tend.tv_usec;

	_ticks = _endTime64 - _beginTime64;

	return (double)_ticks;
}

double CPerformanceCounter::GetSeconds()
{
	return (double)_ticks;
}

double CPerformanceCounter::GetMinSeconds()
{
	// Convert ticks to milliseconds	
	return (double)_minticks;

}

double CPerformanceCounter::GetMaxSeconds()
{
	// Convert ticks to milliseconds	
	return (double)_maxticks;

}
