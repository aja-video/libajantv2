/**
	@file		log.cpp
	@copyright	Copyright (C) 2012-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJATimeLog class.
**/

//#include "ajabase/system/systemtime.h"
#include "ajabase/system/log.h"
#include "ajabase/system/systemtime.h"

bool AJALog::bInitialized = false;

#if defined(AJA_WINDOWS)
#include <windows.h>
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
        
            #if (AJA_LOGTYPE==9999)
                CreateFireLogUserClient();
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
    mTag[0] = 0;
    Reset();
}

// create with name tag
AJATimeLog::AJATimeLog(const char* tag)
{
#if defined(AJA_MAC)
    strncpy(mTag, tag, TAG_SIZE);
#else
    mTag[0] = 0;
#endif
    
    Reset();
}


AJATimeLog::~AJATimeLog()
{
}


// reset time
void AJATimeLog::Reset()
{
    mTime = AJATime::GetSystemMicroseconds();
}


// reset time
void AJATimeLog::PrintReset()
{
	AJA_LOG("%s - Reset\n", mTag);
	Reset();
}

// print dela time in micro seconds
void AJATimeLog::PrintDelta(bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
    AJA_LOG("%s = %lld\n", mTag, currTime-mTime);
    
    if (bReset)
        mTime = currTime;
}


// print dela time in micro seconds
int32_t AJATimeLog::GetDelta(bool bReset)
{
    uint64_t currTime = AJATime::GetSystemMicroseconds();
	int32_t delta = (uint32_t)(currTime - mTime);
    if (bReset)
        mTime = currTime;
		
	return delta;
}


// print dela time in micro seconds, use additional tag
void AJATimeLog::PrintDelta(const char* addTag, bool bReset)
{
    (void)addTag;

    uint64_t currTime = AJATime::GetSystemMicroseconds();
    AJA_LOG("%s-%s = %lld\n", mTag, addTag, currTime-mTime);
    
    if (bReset)
        mTime = currTime;
}
