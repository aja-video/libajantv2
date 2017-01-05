/**
	@file		performance.h
	@copyright	Copyright (C) 2011-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declaration of the AJAPerformance class.
**/

#ifndef __PERFORMANCE_H
#define __PERFORMANCE_H

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/common/timer.h"
#include <string>
#include <map>

typedef std::map<std::string, uint64_t>         AJAPerformaceExtraMap;

/////////////////////////////
// Declarations
/////////////////////////////
class AJAPerformance
{
	public:
        AJAPerformance(const std::string& name,
                       AJATimerPrecision precision = AJATimerPrecisionMilliseconds);
        AJAPerformance(const std::string& name, const AJAPerformaceExtraMap& values,
                       AJATimerPrecision precision = AJATimerPrecisionMilliseconds);
        AJAPerformance(AJATimerPrecision precision = AJATimerPrecisionMilliseconds);
		~AJAPerformance(void);

        void SetExtras(const AJAPerformaceExtraMap& values);

		void Start(void);
		void Stop(void);

        std::string Name(void);
        uint64_t Entries(void);
        uint64_t TotalTime(void);
        uint64_t MinTime(void);
        uint64_t MaxTime(void);
        const AJAPerformaceExtraMap Extras(void);
        AJATimerPrecision Precision(void);

	private:
        AJATimer                    mTimer;
        std::string                 mName;
        uint64_t                    mTotalTime;
        uint64_t                    mEntries;
        uint64_t                    mMinTime;
        uint64_t                    mMaxTime;

        AJAPerformaceExtraMap       mExtras;
};

// Helper functions to track/report many performance timers and store in a map
typedef std::map<std::string, AJAPerformance>   AJAPerformanceTracking;

extern bool AJAPerformaceTracking_start(AJAPerformanceTracking& stats,
                                        std::string key, const AJAPerformaceExtraMap& extras,
                                        AJATimerPrecision precision = AJATimerPrecisionMilliseconds);

extern bool AJAPerformaceTracking_stop(AJAPerformanceTracking& stats, std::string key);

extern bool AJAPerformaceTracking_report(AJAPerformanceTracking& stats, std::string title = "");


#endif // __PERFORMANCE_H
//////////////////////// End of performance.h ///////////////////////

