////////////////////////////////////////////////////////////
//
// file: performancecounter.h
// requires winmm.lib 
//
////////////////////////////////////////////////////////////
//
//	Copyright (C) 2003, 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//
////////////////////////////////////////////////////////////
#ifndef PERFORMANCECOUNTER_H
#define PERFORMANCECOUNTER_H
#include "ajaexport.h"
#if defined(MSWindows)
#include "mmsystem.h"
#include "ajatypes.h"
#else
// May need to change for Mac
#include <stdint.h>
#endif

class  AJAExport CPerformanceCounter
{
public:
    CPerformanceCounter();
    ~CPerformanceCounter();

    void   Start();
    __int64  Stop();
    double GetSeconds();
    double GetMinSeconds();
    double GetMaxSeconds();
	double GetCurrentSeconds();

private:
	uint64_t _beginTime64, _endTime64;
	LARGE_INTEGER _performanceFrequency;	
	__int64 _ticks;
    __int64 _minticks;
    __int64 _maxticks;
};

#endif
