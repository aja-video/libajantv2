/**
	@file		log.cpp
	@copyright	Copyright (C) 2012-2019 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJATimeLog class.
**/

//#include "ajabase/system/systemtime.h"
#include "ajabase/system/log.h"
#include "ajabase/system/systemtime.h"

bool AJALog::bInitialized = false;

#if defined(AJA_WINDOWS)
#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

void __cdecl log_odprintf(const char *format, ...)
{
	char	buf[4096], *p = buf;
	va_list	args;

	va_start(args, format);
	p += _vsnprintf_s(buf, sizeof buf - 1, format, args);
	va_end(args);

	while ( p > buf  &&  isspace(p[-1]) )
		*--p = '\0';

	*p++ = '\r';
	*p++ = '\n';
	*p   = '\0';

	::OutputDebugStringA(buf);
}
#endif

//---------------------------------------------------------------------------------------------------------------------
//  class AJALog
//---------------------------------------------------------------------------------------------------------------------

// singleton initialization here
AJALog::AJALog()
{
    if (bInitialized == false)
    {
        bInitialized = true;
        
        #if defined(AJA_MAC)

            #if (AJA_LOGTYPE==2)
                AJADebug::Open();

            #endif
        
        #elif defined(AJA_LINUX)
            
            // one-time initialization here as needed
        
        #elif defined(AJA_WINDOWS)
		
			#if (AJA_LOGTYPE==2)
				AJADebug::Open();
			#endif
		
            // one-time initialization here as needed
       
        #endif
    }
}

// perform singleton release here
AJALog::~AJALog()
{
}


//---------------------------------------------------------------------------------------------------------------------
// MARK: - AJARunAverage
//---------------------------------------------------------------------------------------------------------------------

void AJARunAverage::Mark(int val)
{
	int index = _samplesTotal++ % _sampleSize;
	_samples[index] = val;
}

int AJARunAverage::MarkAverage(int val)
{
	Mark(val);
	return Average();
}

int AJARunAverage::LastValue()
{
	if (_samplesTotal <= 0)
		{ _samplesTotal = 0; return -1; }
	int lastIndex = ((_samplesTotal-1) % _sampleSize);
	return _samples[lastIndex];
}

int AJARunAverage::Average()
{
	int sampleSize = _samplesTotal < _sampleSize ? _samplesTotal : _sampleSize;
	int average = 0;
	for (int i=0; i <sampleSize; i++)
		average += _samples[i];
		
	average = average / sampleSize;
	return average;
}

void AJARunAverage::Reset()
{
	_samplesTotal = 0; 
	std::fill(_samples.begin(), _samples.end(), 0);
}

void AJARunAverage::Resize(int sampleSize)
{
	_sampleSize = sampleSize;
	_samples.resize(sampleSize);
	Reset();
}



//---------------------------------------------------------------------------------------------------------------------
// MARK: - AJARunTimeAverage
// calculates a running average of time deltas
//---------------------------------------------------------------------------------------------------------------------

AJARunTimeAverage::AJARunTimeAverage(int sampleSize) : AJARunAverage(sampleSize)
{
	Reset();
}

void AJARunTimeAverage::Resize(int sampleSize)
{
	AJARunAverage::Resize(sampleSize);
	
}

void AJARunTimeAverage::Reset()
{
	AJARunAverage::Reset();
	_lastTime = AJATime::GetSystemMicroseconds();
}

// mark current delta-time
// return delta-time
int AJARunTimeAverage::MarkDeltaTime()
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
	int deltaTime = currTime - _lastTime;
	_lastTime = currTime;
	
	Mark(deltaTime);
	return deltaTime;
}

// mark current delta-time
// return running average delta-time
int AJARunTimeAverage::MarkDeltaAverage()
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
	int deltaTime = currTime - _lastTime;
	_lastTime = currTime;
	
	Mark(deltaTime);
	return Average();
}



//---------------------------------------------------------------------------------------------------------------------
//  MARK: - class AJATimeLog
//---------------------------------------------------------------------------------------------------------------------

AJATimeLog::AJATimeLog()
{
    _tag = "";
    _unit = AJA_DebugUnit_Critical;
    Reset();
}

AJATimeLog::AJATimeLog(const char* tag, int unit)
{	
	_unit = unit;
	_tag = tag;
    Reset();
}

AJATimeLog::AJATimeLog(const std::string& tag, int unit)
{	
	_unit = unit;
	_tag = tag;
    Reset();
}

AJATimeLog::~AJATimeLog()
{
}

// reset time
void AJATimeLog::Reset()
{
    _time = AJATime::GetSystemMicroseconds();
}

// reset time
void AJATimeLog::PrintReset()
{
    #if defined(AJA_DEBUG) && (AJA_LOGTYPE!=2)
		AJA_LOG("%s - Reset\n", _tag.c_str());
	#else
		if (AJADebug::IsActive(_unit))
			AJADebug::Report(_unit, AJA_DebugSeverity_Debug, __FILE__, __LINE__, 
				"%s - Reset\n", _tag.c_str());
	#endif
	
	Reset();
}

// print dela time in micro seconds
void AJATimeLog::PrintDelta(bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
    #if defined(AJA_DEBUG) && (AJA_LOGTYPE != 2)
		AJA_LOG("%s = %lld\n", _tag.c_str(), currTime-_time);
	#else
		if (AJADebug::IsActive(_unit))
			AJADebug::Report(_unit, AJA_DebugSeverity_Debug, __FILE__, __LINE__, 
				"%s = %lld\n", _tag.c_str(), currTime-_time);
	#endif
    if (bReset)
        _time = currTime;
}


// print dela time in micro seconds
int32_t AJATimeLog::GetDelta(bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
	int32_t delta = int32_t(currTime - _time);
	if (bReset)
		_time = currTime;
		
	return delta;
}

// print delta time in micro seconds, use additional tag
void AJATimeLog::PrintDelta(const char* addedTag, bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
    
    #if defined(AJA_DEBUG) && (AJA_LOGTYPE!=2)
		if (_unit == AJA_DebugUnit_Critical)
			AJA_LOG("%s-%s = %lld\n", _tag.c_str(), addedTag, currTime-_time);
	#else
		if (AJADebug::IsActive(_unit))
			AJADebug::Report(_unit, AJA_DebugSeverity_Debug, __FILE__, __LINE__, 
				"%s-%s = %lld\n", _tag.c_str(), addedTag, currTime-_time);
	#endif

    if (bReset)
        _time = currTime;
}

void AJATimeLog::PrintDelta(const std::string& addedTag, bool bReset)
{
	return PrintDelta(addedTag.c_str(), bReset);
}
