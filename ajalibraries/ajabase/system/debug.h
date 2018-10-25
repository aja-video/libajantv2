/**
	@file		debug.h
	@copyright	Copyright (C) 2009-2018 AJA Video Systems, Inc.  All rights reserved.
	@brief		Declares the AJADebug class.
**/

#ifndef AJA_DEBUG_H
#define AJA_DEBUG_H

#include <stdio.h>
#include <sstream>
#include "ajabase/common/public.h"
#include "ajabase/system/debugshare.h"



/** @defgroup AJAGroupMacro AJA Debug Macros 
 *	The macros are used to generate messages and assertions.
 *	@ingroup AJAGroupDebug
 *	@{
 */

/** @def AJA_ASSERT(_expression_)
 *	Assert if _expression_ is false.  
 *	@hideinitializer
 *
 *	In Windows, this macro will specify the file name and line number of the assert.
 *
 *  @param[in]	_expression_	Boolean expression that should be true.
 */

/** @def AJA_REPORT(_index_, _severity_, _format_)
 *	Report debug messages to active destinations.  
 *	@hideinitializer
 *
 *	In Windows, this macro will specify the file name and line number of the report.
 *  
 *  @param[in]	_index_		Send message to this destination index.
 *  @param[in]	_severity_	Severity (::AJA_DEBUG_SEVERITY) of the message to report.
 *  @param[in]	_format_	Format parameters passed to vsprintf. The first is the format itself.
 */

/**	@} */

#if defined(AJA_WINDOWS) 

	#if defined(AJA_DEBUG)
		#define AJA_ASSERT(_expression_) \
            if (!(_expression_)) AJADebug::AssertWithMessage(__FILE__, __LINE__, #_expression_);
		#define AJA_PRINT(...) \
			AJADebug::Report(0, AJA_DebugSeverity_Debug, NULL, 0, __VA_ARGS__)
	#else
		#define AJA_ASSERT(_expression_)
		#define AJA_PRINT(...)
	#endif

	#define AJA_REPORT(_index_, _severity_, ...) \
		AJADebug::Report(_index_, _severity_, __FILE__, __LINE__, __VA_ARGS__);

#elif defined(AJA_LINUX)

	#if defined(AJA_DEBUG)
		#define AJA_ASSERT(_expression_) \
            if(!(_expression_)) AJADebug::AssertWithMessage(NULL, 0, #_expression_);
		#define AJA_PRINT(...) \
			AJADebug::Report(0, AJA_DebugSeverity_Error, NULL, 0, __VA_ARGS__)
	#else
		#define AJA_ASSERT(_expression_)
		#define AJA_PRINT(...)
	#endif

	#define AJA_REPORT(_index_, _severity_, ...) \
		AJADebug::Report(_index_, _severity_, __FILE__, __LINE__, __VA_ARGS__);

#elif defined(AJA_MAC) 

	#if defined(AJA_DEBUG)
	
		#define AJA_ASSERT(_expression_) \
            if (!(_expression_)) AJADebug::AssertWithMessage(__FILE__, __LINE__, #_expression_);
		#if !defined (AJA_PRINTTYPE)
			#define AJA_PRINTTYPE		0
		#endif	//	if AJA_PRINTTYPE undefined
		#if (AJA_PRINTTYPE==0)
            #define AJA_PRINT(...) \
                AJADebug::Report(0, AJA_DebugSeverity_Error, NULL, 0, __VA_ARGS__)
		#elif (AJA_PRINTTYPE==1)
			#include <stdio.h>
			#define AJA_PRINT(_format_...) printf(_format_)
		#elif (AJA_PRINTTYPE==2)
			#include "/System/Library/Frameworks/FireLog.framework/Versions/A/Headers/FireLog.h"
			#define AJA_PRINT(_format_...) FireLog(_format_)
		#elif (AJA_PRINTTYPE==3)
			#include <stdio.h>
			#define AJA_PRINT(_format_...) fprintf(stderr, _format_)
		#elif (AJA_PRINTTYPE==4)
			#include <syslog.h>
			#include <stdarg.h>
			#define AJA_PRINT(_format_...) syslog(LOG_ERR, _format_)
		#endif

	#else
		#define AJA_ASSERT(_expression_)
		#define AJA_PRINT(...)
	#endif

    #define AJA_REPORT(_index_, _severity_, ...) \
        AJADebug::Report(_index_, _severity_, __FILE__, __LINE__, __VA_ARGS__);

#else

	#if defined(AJA_DEBUG)
		#define AJA_ASSERT(_expression_) \
            if(!(_expression_)) AJADebug::AssertWithMessage(NULL, 0, #_expression_);
		#define AJA_PRINT(_format_,...) \
			AJADebug::Report(0, AJA_DebugSeverity_Error, NULL, 0, _format_)
	#else
		#define AJA_ASSERT(_expression_)
		#define AJA_PRINT(_format_,...)
	#endif

	#define AJA_REPORT(_index_, _severity_, _format_) \
		AJADebug::Report(_index_, _severity_, NULL, 0, _format_);

#endif

//	Handy ostream-based macros...
#define	AJA_sASSERT(_expr_)                 do {std::ostringstream	__ss__;  __ss__ << #_expr_;  AJADebug::AssertWithMessage(__FILE__, __LINE__, __ss__.str());} while (false)
#define	AJA_sREPORT(_ndx_,_sev_,_expr_)		do {std::ostringstream	__ss__;  __ss__ << _expr_;  AJADebug::Report((_ndx_), (_sev_), __FILE__, __LINE__, __ss__.str());} while (false)
#define	AJA_sEMERGENCY(_ndx_,_expr_)		AJA_sREPORT((_ndx_), AJA_DebugSeverity_Emergency,	_expr_)
#define	AJA_sALERT(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Alert,		_expr_)
#define	AJA_sERROR(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Error,		_expr_)
#define	AJA_sWARNING(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Warning,		_expr_)
#define	AJA_sNOTICE(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Notice,		_expr_)
#define	AJA_sINFO(_ndx_,_expr_)				AJA_sREPORT((_ndx_), AJA_DebugSeverity_Info,		_expr_)
#define	AJA_sDEBUG(_ndx_,_expr_)			AJA_sREPORT((_ndx_), AJA_DebugSeverity_Debug,		_expr_)

// forward declarations
class AJAMemory;

/** 
 *  @param[in]  inStatus   The AJAStatus value of interest.
 *	@return		A string containing the given AJAStatus value as human-readable text.
 */
AJA_EXPORT std::string AJAStatusToString (const AJAStatus inStatus);


/** 
 *	Debug class to generate debug output and assertions.
 *	@ingroup AJAGroupDebug
 */
class AJA_EXPORT AJADebug
{
public:

	AJADebug();
	virtual ~AJADebug();

	/** 
	 *	Initialize the debug system.  
	 *
	 *	Invoke before reporting debug messages.
     *
     *  @param[in]  incrementRefCount   If true will increment the ref count in the shared memory,
     *                                  NOTE that this should only be true when used in something that is
     *                                  processing the log messages, like ajalogger.
	 *
	 *	@return		AJA_STATUS_SUCCESS	Debug system initialized
	 *				AJA_STATUS_FAIL		Initialization failed
	 */
    static AJAStatus Open(bool incrementRefCount = false);

	/** 
	 *	Release resources used by the debug system.  
	 *
	 *	Invoke before application terminates. 
	 *
     *  @param[in]  decrementRefCount   If true will decrement the ref count in the shared memory,
     *                                  NOTE that this should only be true when used in something that is
     *                                  processing the log messages, like ajalogger.
     *
	 *	@return		AJA_STATUS_SUCCESS	Debug system closed
	 *				AJA_STATUS_FAIL		Close failed
	 */
    static AJAStatus Close(bool decrementRefCount = false);

	/**
	 *	Enable delivery of messages to the destination index.
	 *
	 *	@param[in]	index						Specify the message destinations for this group index.
	 *	@param[in]	destination					Bit array of message destinations (@link AJA_DEBUG_DESTINATION@endlink). 
	 *	@return		AJA_STATUS_SUCCESS			Destination enabled
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 */
	static AJAStatus Enable(int32_t index, uint32_t destination = AJA_DEBUG_DESTINATION_NONE);

	/**
	 *	Disable delivery of messages to the destination index.
	 *
	 *	@param[in]	index						Specify the message destinations for this group index.
	 *	@param[in]	destination					Bit array of message destinations (@link AJA_DEBUG_DESTINATION@endlink). 
	 *	@return		AJA_STATUS_SUCCESS			Destination disabled
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 */
	static AJAStatus Disable(int32_t index, uint32_t destination = AJA_DEBUG_DESTINATION_NONE);

	/**
	 *	Enable delivery of messages to the destination index.
	 *
	 *	@param[in]	index						Specify the message destinations for this index.
	 *	@param[in]	destination					Bit array of message destinations (@link AJA_DEBUG_DESTINATION@endlink). 
	 *	@return		AJA_STATUS_SUCCESS			Destination disabled
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 */
	static AJAStatus SetDestination(int32_t index, uint32_t destination = AJA_DEBUG_DESTINATION_NONE);

	/**
	 *	Get the destination associated with a debug group.
	 *
	 *	@param[in]	index						Index of the group destination to return.
	 *	@param[out]	pDestination				The current destination
	 *	@return		AJA_STATUS_SUCCESS			Destination disabled
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
	static AJAStatus GetDestination(int32_t index, uint32_t* pDestination);

	/**
	 *	Is the destination index active.
	 *
	 *	@param[in]	index					Query active for this index.
     *	@return		true                    Destination enabled
     *				false                   Destination disabled, Debug system not open, or Index out of range
	 */
    static bool IsActive(int32_t index);

	/**
	 *	Is the debug system open?
	 *
     *	@return		true                    Debug system is open and ready for use.
     *				false                   Debug system not open.
	 */
    static bool IsOpen(void);

	/** 
	 *	Is this class built with AJA_DEBUG defined. 
	 *
     *	@return		true                    Debug build
     *				false                   Release build
	 */
    static bool IsDebugBuild();

	/**
	 *	Report debug message to the specified destination index.
	 *
	 *	@param[in]	index		Report the message to this destination index.
	 *	@param[in]	severity	Severity (::AJA_DEBUG_SEVERITY) of the message to report.
	 *	@param[in]	pFileName	The source filename reporting the message.
	 *	@param[in]	lineNumber	The line number in the source file reporting the message.
	 *  @param[in]	...			Format parameters to be passed to vsprintf. The first is the format itself.
	 */
	static void Report(int32_t index, int32_t severity, const char* pFileName, int32_t lineNumber, ...);

    /**
     *	Report debug message to the specified destination index.
     *
     *	@param[in]	index		Report the message to this destination index.
     *	@param[in]	severity	Severity (::AJA_DEBUG_SEVERITY) of the message to report.
     *	@param[in]	pFileName	The source filename reporting the message.
     *	@param[in]	lineNumber	The line number in the source file reporting the message.
     *  @param[in]	message		The message to report.
     */
    static void Report(int32_t index, int32_t severity, const char* pFileName, int32_t lineNumber, const std::string& message);

	/**
     *	Assert that an unexpected error has occurred.
	 *
	 *	@param[in]	pFileName		The source file name reporting the assertion.
	 *	@param[in]	lineNumber		The line number in the source file reporting the assertion.
	 *  @param[in]	pExpression		Expression that caused the assertion.
	 */
    static void AssertWithMessage(const char* pFileName, int32_t lineNumber, const std::string& pExpression);

    /**
     *	Get the reference count for the number of clients accessing shared debug info
     *
     *	@param[out]	pRefCount                   The current client reference count.
     *	@return		AJA_STATUS_SUCCESS			Reference count returned
     *				AJA_STATUS_OPEN				Debug system not open
     *				AJA_STATUS_RANGE			Index out of range
     *				AJA_STATUS_NULL				Null output pointer
     */
    static AJAStatus GetClientReferenceCount(int32_t* pRefCount);

    /**
     *	Set the reference count for the number of clients accessing shared debug info
     *  NOTE: in normal circumstances this should never be used, if set to 0 or less
     *        the debug system will be closed (shared memory cleaned up)
     *
     *	@param[in]	refCount                    The client reference count to set.
     *	@return		AJA_STATUS_SUCCESS			Reference count set
     *				AJA_STATUS_OPEN				Debug system not open
     *				AJA_STATUS_RANGE			Index out of range
     */
    static AJAStatus SetClientReferenceCount(int32_t refCount);

	/**
	 *	Get the sequence number of the latest message
	 *
	 *	@param[out]	pSequenceNumber				The current sequence number.
	 *	@return		AJA_STATUS_SUCCESS			Sequence number returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetSequenceNumber(uint64_t* pSequenceNumber);

	/**
	 *	Get the sequence number recorded in the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
     *	@param[out]	pSequenceNumber				Sequence number recorded in the message.
	 *	@return		AJA_STATUS_SUCCESS			Group index returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageSequenceNumber(uint64_t sequenceNumber, uint64_t *pSequenceNumber);

	/**
	 *	Get the group index that reported the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	pGroupIndex					Group that reported the message.
	 *	@return		AJA_STATUS_SUCCESS			Group index returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageGroup(uint64_t sequenceNumber, int32_t* pGroupIndex);

	/**
	 *	Get the destination of the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	pDestination				Destination of the message.
	 *	@return		AJA_STATUS_SUCCESS			Destination returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageDestination(uint64_t sequenceNumber, uint32_t* pDestination);

	/**
	 *	Get the time the message was reported.
	 *
	 *	@param[in]	sequenceNumber			Sequence number of the message.
	 *	@param[out]	pTime					Time the message was reported.
	 *	@return		AJA_STATUS_SUCCESS		message time returned
	 *				AJA_STATUS_OPEN			debug system not open
	 *				AJA_STATUS_RANGE		index out of range
	 *				AJA_STATUS_NULL			null output pointer
	 */
    static AJAStatus GetMessageTime(uint64_t sequenceNumber, uint64_t* pTime);

    /**
     *	Get the wall clock time the message was reported.
     *
     *	@param[in]	sequenceNumber			Sequence number of the message.
     *	@param[out]	pTime					Wall clock time the message was reported, as returned from time()
     *	@return		AJA_STATUS_SUCCESS		message time returned
     *				AJA_STATUS_OPEN			debug system not open
     *				AJA_STATUS_RANGE		index out of range
     *				AJA_STATUS_NULL			null output pointer
     */
    static AJAStatus GetMessageWallClockTime(uint64_t sequenceNumber, int64_t *pTime);


	/**
	 *	Get the source file name that reported the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	ppFileName					Source file name that reported the message.
	 *	@return		AJA_STATUS_SUCCESS			Message file name returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageFileName(uint64_t sequenceNumber, const char** ppFileName);

	/**
	 *	Get the source line number that reported the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	pLineNumber					Source line number that reported the message.
	 *	@return		AJA_STATUS_SUCCESS			Sequence number returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageLineNumber(uint64_t sequenceNumber, int32_t* pLineNumber);

	/**
	 *	Get the severity of the reported message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	pSeverity					Severity of the reported message.
	 *	@return		AJA_STATUS_SUCCESS			Message severity returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageSeverity(uint64_t sequenceNumber, int32_t* pSeverity);

	/**
	 *	Get the message.
	 *
	 *	@param[in]	sequenceNumber				Sequence number of the message.
	 *	@param[out]	ppMessage					The message text
	 *	@return		AJA_STATUS_SUCCESS			Message text returned
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_RANGE			Index out of range
	 *				AJA_STATUS_NULL				Null output pointer
	 */
    static AJAStatus GetMessageText(uint64_t sequenceNumber, const char** ppMessage);

    /**
     *	Get the Process Id that reported the message.
     *
     *	@param[in]	sequenceNumber			Sequence number of the message.
     *	@param[out]	pPid                    Process Id that reported the message.
     *	@return		AJA_STATUS_SUCCESS		message time returned
     *				AJA_STATUS_OPEN			debug system not open
     *				AJA_STATUS_RANGE		index out of range
     *				AJA_STATUS_NULL			null output pointer
     */
    static AJAStatus GetProcessId(uint64_t sequenceNumber, uint64_t* pPid);

    /**
     *	Get the Thread Id that reported the message.
     *
     *	@param[in]	sequenceNumber			Sequence number of the message.
     *	@param[out]	pTid                    Thread Id that reported the message.
     *	@return		AJA_STATUS_SUCCESS		message time returned
     *				AJA_STATUS_OPEN			debug system not open
     *				AJA_STATUS_RANGE		index out of range
     *				AJA_STATUS_NULL			null output pointer
     */
    static AJAStatus GetThreadId(uint64_t sequenceNumber, uint64_t* pTid);

    /**
     *	Get the number of messages accepted into the ring since creation.
     *
     *	@param[out]	pCount                      The number of messages
     *	@return		AJA_STATUS_SUCCESS			Number of messages returned
     *				AJA_STATUS_OPEN				Debug system not open
     *				AJA_STATUS_RANGE			Index out of range
     *				AJA_STATUS_NULL				Null output pointer
     */
    static AJAStatus GetMessagesAccepted(uint64_t* pCount);

    /**
     *	Get the number of messages ignored and not put into the ring since creation.
     *
     *	@param[out]	pCount                      The number of messages
     *	@return		AJA_STATUS_SUCCESS			Number of messages returned
     *				AJA_STATUS_OPEN				Debug system not open
     *				AJA_STATUS_RANGE			Index out of range
     *				AJA_STATUS_NULL				Null output pointer
     */
    static AJAStatus GetMessagesIgnored(uint64_t* pCount);

	/**
	 *	Get the string associated with a debug severity.
	 *
	 *	@param[in]	severity	Index of the severity string to return.
	 *	@return				Severity string
	 */
	static const char* GetSeverityString(int32_t severity);

    /**
     *	Get the string associated with a debug message group.
     *
     *	@param[in]	group	Index of the message group string to return.
     *	@return				Group string
     */
    static const char* GetGroupString(int32_t group);

    /**
     *	Get the string associated with a debug message group.
     *
     *	@param[in]	group	Index of the message group string to return.
     *	@return				Group string
     */
    static const std::string & GetGroupName(const int32_t group);

	/**
	 *	Write group state to a file.
	 *
	 *	@param[in]	pFileName					The group state file name.
	 *	@return		AJA_STATUS_SUCCESS			State saved
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_NULL				Null output pointer
	 */
	static AJAStatus SaveState(char* pFileName);

	/**
	 *	Read group state from a file.
	 *
	 *	@param[in]	pFileName					The group state file name.
	 *	@return		AJA_STATUS_SUCCESS			State restored
	 *				AJA_STATUS_OPEN				Debug system not open
	 *				AJA_STATUS_NULL				Null output pointer
	 */
	static AJAStatus RestoreState(char* pFileName);

	/**
	 *	Get the current time at the debug rate.
	 *
	 *	@return		The current time in debug ticks.
	 */
	static int64_t DebugTime();

	static void * GetPrivateDataLoc (void);
	static size_t GetPrivateDataLen (void);
private:
};

#endif	//	AJA_DEBUG_H
