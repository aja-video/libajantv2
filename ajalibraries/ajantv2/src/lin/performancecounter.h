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

#if defined (MSWindows)
#  include "mmsystem.h"
#endif

#if defined (AJALinux)
#  include "ajatypes.h"
#  include <sys/time.h>
#  define __int64 LWord64
#endif

class CPerformanceCounter
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
#if defined (MSWindows)
	unsigned __int64 _beginTime64, _endTime64;
	LARGE_INTEGER _performanceFrequency;	
#endif
#if defined (AJALinux)
	ULWord64 _beginTime64, _endTime64;
	struct timeval _tstart, _tend;
	struct timezone tz;
#endif
#if defined (AJAMac)
	__int64 _beginTime64, _endTime64;
	struct timeval _tstart, _tend;
	struct timezone tz;
#endif
	__int64 _ticks;
    __int64 _minticks;
    __int64 _maxticks;
};

#endif
