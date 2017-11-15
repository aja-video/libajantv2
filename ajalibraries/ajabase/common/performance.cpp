/////////////////////////////////////////////////////////////////////
//                          performance.cpp
//
// Description: This module will monitor the operational performance,
// the timing, and the statistics of an arbitrary module.
//
// Copyright (C) 2011 AJA Video Systems, Inc.
// Proprietary and Confidential information.  All rights reserved.
//
/////////////////////////////////////////////////////////////////////

/////////////////////////////
// Includes
/////////////////////////////
#include "ajabase/common/performance.h"
#include "ajabase/system/debug.h"

#include <iomanip>
#include <math.h>

using std::string;
using std::map;

/////////////////////////////
// Definitions
/////////////////////////////

// Fix for old Linux systems
#ifndef UINT64_MAX
#define UINT64_MAX      18446744073709551615
#endif

AJAPerformance::AJAPerformance(const std::string& name,
                               AJATimerPrecision precision)
    : mTimer(precision)
{
    mName       = name;
    mTotalTime  = 0;
    mEntries    = 0;
    mMinTime    = UINT64_MAX;
    mMaxTime    = 0;
    mMean       = 0.0;
    mM2         = 0.0;
}

AJAPerformance::AJAPerformance(const std::string& name,
                               const AJAPerformaceExtraMap &values,
                               AJATimerPrecision precision)
    : mTimer(precision)
{
    mName       = name;
    mTotalTime  = 0;
    mEntries    = 0;
    mMinTime    = UINT64_MAX;
    mMaxTime    = 0;
    mExtras     = values;
    mMean       = 0.0;
    mM2         = 0.0;
}

AJAPerformance::AJAPerformance(AJATimerPrecision precision)
    : mTimer(precision)
{
    mName       = "";
    mTotalTime  = 0;
    mEntries    = 0;
    mMinTime    = UINT64_MAX;
    mMaxTime    = 0;
    mMean       = 0.0;
    mM2         = 0.0;
}

AJAPerformance::~AJAPerformance(void)
{
    // If not already stopped then stop and output report
    if (mTimer.IsRunning())
    {
        Stop();
        Report();
    }
}

void AJAPerformance::SetExtras(const AJAPerformaceExtraMap& values)
{
    mExtras = values;
}

std::string AJAPerformance::Name(void)
{
    return mName;
}

uint64_t AJAPerformance::Entries(void)
{
    return mEntries;
}

uint64_t AJAPerformance::TotalTime(void)
{
    return mTotalTime;
}

uint64_t AJAPerformance::MinTime(void)
{
    return mMinTime;
}

uint64_t AJAPerformance::MaxTime(void)
{
    return mMaxTime;
}

double AJAPerformance::Mean(void)
{
    return mMean;
}

double AJAPerformance::Variance(void)
{
    uint64_t entries = Entries();
    if (entries > 1)
    {
        return mM2/(entries-1);
    }
    else
        return 0.0;
}

double AJAPerformance::StandardDeviation(void)
{
    return sqrt(Variance());
}

const AJAPerformaceExtraMap AJAPerformance::Extras(void)
{
    return mExtras;
}

AJATimerPrecision AJAPerformance::Precision(void)
{
    return mTimer.Precision();
}

void AJAPerformance::Start(void)
{
	mTimer.Start();
}

void AJAPerformance::Stop(void)
{
	uint32_t elapsedTime;

	mTimer.Stop();
	elapsedTime = mTimer.ElapsedTime();

    mTotalTime += elapsedTime;
    mEntries++;

    // calculate the running mean and sum of squares of differences from the current mean (mM2)
    // mM2 is needed to calculate the variance and the standard deviation
    // see: https://stackoverflow.com/a/17053010
    //      http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Online_algorithm
    double delta1 = elapsedTime - mMean;
    mMean += delta1/mEntries;
    double delta2 = elapsedTime - mMean;
    mM2 += delta1 * delta2;

    if (elapsedTime > mMaxTime)
	{
        mMaxTime = elapsedTime;

		// First-time assignment in the event that a
		// test run has times that always monotonically
		// increase.
        if (elapsedTime < mMinTime)
		{
            mMinTime = elapsedTime;
		}
	}
    else if (elapsedTime < mMinTime)
	{
        mMinTime = elapsedTime;
	}
}

void AJAPerformance::Report(const std::string& name)
{
    int entries = (int)Entries();
    if (entries > 0)
    {
        int min      = (int)MinTime();
        int max      = (int)MaxTime();
        double mean  = Mean();
        double stdev = StandardDeviation();
        string times = (entries == 1) ? "time,  " : "times, ";
        string reportName = name.empty() ? Name() : name;

        AJA_sREPORT(AJA_DebugUnit_StatsGeneric,
                    AJA_DebugSeverity_Debug,
                    "  ["     << std::left  << std::setw(23) << std::setfill(' ') << reportName << "] " <<
                    "called " << std::right << std::setw(4)  << entries << " "  << times <<
                    "min: "   << std::right << std::setw(4)  << min     << ", " <<
                    "mean: "  << std::right << std::setw(5)  << std::fixed << std::setprecision(2) << mean  << ", " <<
                    "stdev: " << std::right << std::setw(5)  << std::fixed << std::setprecision(2) << stdev << ", " <<
                    "max: "   << std::right << std::setw(4)  << max);
    }
}

bool AJAPerformaceTracking_start(AJAPerformanceTracking& stats,
                                 std::string key, const AJAPerformaceExtraMap& extras, AJATimerPrecision precision)
{
    if(stats.find(key) == stats.end())
    {
        // not already in map
        AJAPerformance newStatsGroup(key, extras, precision);
        stats[key] = newStatsGroup;
    }

    AJAPerformanceTracking::iterator foundAt = stats.find(key);
    if(foundAt != stats.end())
    {
        foundAt->second.Start();
        return true;
    }
    else
    {
        return false;
    }
}

bool AJAPerformaceTracking_stop(AJAPerformanceTracking& stats, std::string key)
{
    AJAPerformanceTracking::iterator foundAt = stats.find(key);
    if(foundAt != stats.end())
    {
        foundAt->second.Stop();
        return true;
    }
    else
    {
        return false;
    }
}

bool AJAPerformaceTracking_report(AJAPerformanceTracking& stats, std::string title)
{
    if (stats.size() > 0)
    {
        if (title.empty())
        {
            title = "stats_report";
        }

        AJA_sREPORT(AJA_DebugUnit_StatsGeneric, AJA_DebugSeverity_Debug,
                    title << ", tracking " << stats.size() << " {");

        string units = AJATimer::PrecisionName(stats.begin()->second.Precision(), true);

        AJA_sREPORT(AJA_DebugUnit_StatsGeneric, AJA_DebugSeverity_Debug,
                    "time units are in " << units);

        AJAPerformanceTracking::iterator foundAt = stats.begin();
        while (foundAt != stats.end())
        {
            std::string key = foundAt->first;
            AJAPerformance perf = foundAt->second;

            perf.Report(key);

            ++foundAt;
        }
        AJA_sREPORT(AJA_DebugUnit_StatsGeneric, AJA_DebugSeverity_Debug, "}");

        return true;
    }
    else
    {
        return false;
    }
}
