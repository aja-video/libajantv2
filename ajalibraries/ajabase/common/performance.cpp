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

using std::string;
using std::map;

/////////////////////////////
// Definitions
/////////////////////////////

// Fix for old Linux systems
#ifndef UINT64_MAX
#define UINT64_MAX      18446744073709551615
#endif


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAPerformance::AJAPerformance(const std::string& name,
                               AJATimerPrecision precision)
    : mTimer(precision)
{
    mName       = name;
    mTotalTime  = 0;
    mEntries    = 0;
    mMinTime    = UINT64_MAX;
    mMaxTime    = 0;
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
}

AJAPerformance::AJAPerformance(AJATimerPrecision precision)
    : mTimer(precision)
{
    mName       = "";
    mTotalTime  = 0;
    mEntries    = 0;
    mMinTime    = UINT64_MAX;
    mMaxTime    = 0;
}


///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
AJAPerformance::~AJAPerformance(void)
{
}

void AJAPerformance::SetExtras(const AJAPerformaceExtraMap &values)
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

const AJAPerformaceExtraMap AJAPerformance::Extras(void)
{
    return mExtras;
}

AJATimerPrecision AJAPerformance::Precision(void)
{
    return mTimer.Precision();
}

///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
void AJAPerformance::Start(void)
{
	mTimer.Start();
}

///////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////
void AJAPerformance::Stop(void)
{
	uint32_t elapsedTime;

	mTimer.Stop();
	elapsedTime = mTimer.ElapsedTime();

    mTotalTime += elapsedTime;
    mEntries++;

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

        AJA_REPORT(AJA_DebugUnit_StatsGeneric, AJA_DebugSeverity_Debug, "%s, tracking %d {", title.c_str(), stats.size());

        string units;
        switch(stats.begin()->second.Precision())
        {
            default:
            case AJATimerPrecisionMilliseconds: units = "milliseconds"; break;
            case AJATimerPrecisionMicroseconds: units = "microseconds"; break;
        }

        AJA_REPORT(AJA_DebugUnit_StatsGeneric, AJA_DebugSeverity_Debug, "time units are in %s", units.c_str());

        AJAPerformanceTracking::iterator foundAt = stats.begin();
        while (foundAt != stats.end())
        {
            std::string key = foundAt->first;
            AJAPerformance perf = foundAt->second;

            uint64_t entries = perf.Entries();
            if (entries > 0)
            {
                int min   = (int)perf.MinTime();
                int max   = (int)perf.MaxTime();
                float avg = ((float)perf.TotalTime()/(float)entries);
                string times = (entries == 1) ? "time,  " : "times, ";

                AJA_REPORT(AJA_DebugUnit_StatsGeneric,
                           AJA_DebugSeverity_Debug,
                           "  [%-23s] called %4d %s min: %4d, avg: %5.2f, max: %4d",
                           key.c_str(),
                           entries,
                           times.c_str(),
                           min,
                           avg,
                           max);
            }
            ++foundAt;
        }
        AJA_REPORT(AJA_DebugUnit_StatsGeneric, AJA_DebugSeverity_Debug, "}");

        return true;
    }
    else
    {
        return false;
    }
}
