// file: performancecounter.cpp
//
// Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
// 
#include <windows.h>
#include "performancecounter.h"

CPerformanceCounter::CPerformanceCounter()
{
	_beginTime64 =  _endTime64 = 0;
	_ticks = _maxticks = 0;
    _minticks = 0x7FFFFFFFFFFFFFFF;

	QueryPerformanceFrequency(&_performanceFrequency);
}

CPerformanceCounter::~CPerformanceCounter()
{

}

void CPerformanceCounter::Start()
{
	// Get Start Time	
	QueryPerformanceCounter( (LARGE_INTEGER *)&_beginTime64 );
}

__int64 CPerformanceCounter::Stop()
{
	// Get End Time	
	QueryPerformanceCounter( (LARGE_INTEGER *)&_endTime64 );

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
	QueryPerformanceCounter( (LARGE_INTEGER *)&_endTime64 );

	_ticks = (_endTime64 - _beginTime64) ;

	// Convert ticks to milliseconds	
	return (double)((double)(_ticks)/(double)_performanceFrequency.LowPart);

}

double CPerformanceCounter::GetSeconds()
{
	// Convert ticks to milliseconds	
	return (double)((double)(_ticks)/(double)_performanceFrequency.LowPart);

}

double CPerformanceCounter::GetMinSeconds()
{
	// Convert ticks to milliseconds	
	return (double)(((double)_minticks)/(double)_performanceFrequency.LowPart);

}

double CPerformanceCounter::GetMaxSeconds()
{
	// Convert ticks to milliseconds	
	return (double)(((double)_maxticks)/(double)_performanceFrequency.LowPart);

}
