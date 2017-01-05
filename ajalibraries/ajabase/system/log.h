/**
	@file		log.h
	@copyright	Copyright (C) 2012-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJATimeLog class.
**/

#ifndef AJA_LOG_H
#define AJA_LOG_H

#include <stdio.h>
#include "ajabase/common/public.h"
#include "debug.h"

#if defined(AJA_WINDOWS)
extern void __cdecl log_odprintf(const char *format, ...);
#elif defined (AJA_MAC)
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

// use this the select alternate platform specific loggers, 0 is used for no-log
#define AJA_LOGTYPE     1

// define AJA_LOG here

#if defined(AJA_WINDOWS) 

	#if defined(AJA_DEBUG)

		#if (AJA_LOGTYPE==0)
			// no log
			#define AJA_LOG(_format_,...)

		#elif (AJA_LOGTYPE==1)
			#define AJA_LOG(...) log_odprintf(__VA_ARGS__)

		#elif (AJA_LOGTYPE==2)
			#define AJA_LOG(...) AJA_REPORT(AJA_DebugUnit_Critical, AJA_DebugSeverity_Info, __VA_ARGS__)
		#else
			//catch all, so builds won't break
			#define AJA_LOG(_format_,...)
		#endif

	#else
		// no log
		#define AJA_LOG(_format_,...)
	#endif

#elif defined(AJA_LINUX)

    #define AJA_LOG(...)

#elif defined(AJA_MAC) 

	#if defined(AJA_DEBUG)

		#if (AJA_LOGTYPE==0)
            // no log
			#define AJA_LOG(_format_...)
        
        #elif (AJA_LOGTYPE==1)
            // printf
			#include <stdio.h>
			#define AJA_LOG(_format_...) printf(_format_)
		
        #elif (AJA_LOGTYPE==9999)	//	'9999' is an attempt to make it obvious when FireLog is being used, to prevent downstream build failures on MacOS
			//	If this #include fails at compile time, then FireLog isn't installed:
			#include "/System/Library/Frameworks/FireLog.framework/Headers/FireLog.h"
			//	FireLog should not be used in production (committed) code!!
			#define AJA_LOG(_format_...)	FireLog (_format_)
		#endif
			
	#else
        // no log
        #define AJA_LOG(_format_...)
	#endif

#endif


#define Make4CC(my4CC)  ((my4CC < 0x40) ?  ' '						 : ((char*)(&my4CC))[3]), \
						((my4CC < 0x40) ?  ' '						 : ((char*)(&my4CC))[2]), \
						((my4CC < 0x40) ? ('0' + (char)(my4CC / 10)) : ((char*)(&my4CC))[1]), \
						((my4CC < 0x40) ? ('0' + (char)(my4CC % 10)) : ((char*)(&my4CC))[0])


/** 
 *	Supports auto initialization of logger, if needed
 *	@ingroup AJAGroupDebug
 */
class AJA_EXPORT AJALog
{
public:

    /**
	 *	Singleton initialization of logging service.
	 *  @param[in]	...			Variable length parameter list for logging. Same formatting as printf
	 */
	AJALog();
    
    /**
	 *	Singleton release of logging service.
	 *  @param[in]	...			Variable length parameter list for logging. Same formatting as printf
	 */
	virtual ~AJALog();
    
private:
    static bool bInitialized;
};


extern AJALog gLogInit;


/**
 *	Used to generate timer logs.
 *	@ingroup AJAGroupDebug
 */

#define TAG_SIZE    64

class AJA_EXPORT AJATimeLog
{
public:
	AJATimeLog();
	AJATimeLog(const char* tag);
	virtual ~AJATimeLog();
    
    /**
	 *	reset timer.
	 */
    void Reset();
	
    /**
	 *	Print does reset along with print
	 */
    void PrintReset();
    void PrintResetIf(bool bEnable=true)
		{
			if (bEnable)
				PrintReset();
		}
	
    /**
	 *	Print tag and reset message.
	 *  @param[in]	bReset          true if time is reset after print
	 */
    void PrintDelta(bool bReset=true);
	
    /**
	 *	Optional print tag and delta-time since last reset.
	 *  @param[in]	bEnable         true to print, false inhibits printing
	 *  @param[in]	bReset          true if time is reset after print
	 */
	inline void PrintDeltaIf(bool bEnable, bool bReset=true)
		{
			if (bEnable)
				PrintDelta(bReset);
		}
	
    /**
	 *	Get delta-time since last reset.
	 *  @param[in]	bReset          true if time is reset after get
	 */
    int32_t GetDelta(bool bReset=true);
    
    /**
	 *	Print tag, appended tag, and delta-time since last reset.
	 *  @param[in]	addedTag        add this tag to current tag
	 *  @param[in]	bReset          true if time is reset after print
	 */
    void PrintDelta(const char* addedTag, bool bReset=true);
	
    /**
	 *	Optional print tag, appended tag, and delta-time since last reset.
	 *  @param[in]	bEnable         true to print, false inhibits printing
	 *  @param[in]	addedTag        add this tag to current tag
	 *  @param[in]	bReset          true if time is reset after print
	 */
    inline void PrintDeltaIf(bool bEnable, const char* addedTag, bool bReset=true)
		{
			if (bEnable)
				PrintDelta(addedTag, bReset);
		}

	
protected:
    char        mTag[TAG_SIZE];
    uint64_t    mTime;
};


#endif	//	AJA_LOG_H
