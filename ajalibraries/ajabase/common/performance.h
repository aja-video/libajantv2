/**
	@file		performance.h
	@copyright	Copyright (C) 2011-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declaration of the AJAPerformance class.
**/

#ifndef AJA_PERFORMANCE_H
#define AJA_PERFORMANCE_H

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/common/timer.h"
#include <string>
#include <map>

typedef std::map<std::string, uint64_t> AJAPerformaceExtraMap;

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

        /**
         *	Set extra values that can be stored along with performance info
         *
         *  @param[in]  values The extra values to assign to this object
         */
        void SetExtras(const AJAPerformaceExtraMap& values);

        /**
         *	Start the timer of the performance object
         */
		void Start(void);

        /**
         *	Stop the timer of the performance object and updates the performance stats:
         *  number of entries, min time, max time, total time
         */
		void Stop(void);

        /**
         *	Print out a performance report to AJADebug
         *  @param[in]  name Name to use in printout, if empty will use the name passed in constructor
         */
        void Report(const std::string& name = "");

        /**
         *  Returns the name for the performance object that was set in the constructor
         */
        std::string Name(void);

        /**
         *  Returns the Precision units set in the constructor
         */
        AJATimerPrecision Precision(void);

        /**
         *  Returns the number of times that the start/stop pair has been called
         */
        uint64_t Entries(void);

        /**
         *  Returns the total elapsed time between all start/stop pairs (in Precision units)
         */
        uint64_t TotalTime(void);

        /**
         *  Returns the minimum time between all start/stop pairs (in Precision units)
         */
        uint64_t MinTime(void);

        /**
         *  Returns the maximum time between all start/stop pairs (in Precision units)
         */
        uint64_t MaxTime(void);

        /**
         *  Returns the mean (average) time of all start/stop pairs (in Precision units)
         */
        double Mean(void);

        /**
         *  Returns the variance of all start/stop pairs (in Precision units)
         */
        double Variance(void);

        /**
         *  Returns the standard deviation of all start/stop pairs (in Precision units)
         */
        double StandardDeviation(void);

        /**
         *  Returns a map of any extra values stored in the performance object
         */
        const AJAPerformaceExtraMap Extras(void);

    private:
        AJATimer                    mTimer;
        std::string                 mName;
        uint64_t                    mTotalTime;
        uint64_t                    mEntries;
        uint64_t                    mMinTime;
        uint64_t                    mMaxTime;
        double                      mMean;
        double                      mM2;

        AJAPerformaceExtraMap       mExtras;
};

// Helper functions to track/report many performance timers and store in a map
typedef std::map<std::string, AJAPerformance> AJAPerformanceTracking;

extern bool AJAPerformaceTracking_start(AJAPerformanceTracking& stats,
                                        std::string key, const AJAPerformaceExtraMap& extras,
                                        AJATimerPrecision precision = AJATimerPrecisionMilliseconds);

extern bool AJAPerformaceTracking_stop(AJAPerformanceTracking& stats, std::string key);

extern bool AJAPerformaceTracking_report(AJAPerformanceTracking& stats, std::string title = "");


#endif // AJA_PERFORMANCE_H
//////////////////////// End of performance.h ///////////////////////

