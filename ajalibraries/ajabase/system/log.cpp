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
//  class AJATimeLog
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


// print dela time in micro seconds, use additional tag
void AJATimeLog::PrintDelta(const char* addTag, bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
    
    #if defined(AJA_DEBUG) && (AJA_LOGTYPE!=2)
		AJA_LOG("%s-%s = %lld\n", _tag.c_str(), addTag, currTime-_time);
	#else
		if (AJADebug::IsActive(_unit))
			AJADebug::Report(_unit, AJA_DebugSeverity_Debug, __FILE__, __LINE__, 
				"%s-%s = %lld\n", _tag.c_str(), addTag, currTime-_time);
	#endif

    if (bReset)
        _time = currTime;
}
