/**
	@file		debug.cpp
	@copyright	Copyright (C) 2009-2017 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJADebug class.
**/

#include "ajabase/system/system.h"
#include "ajabase/common/common.h"
#include "ajabase/system/debug.h"
#include "ajabase/system/memory.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/systemtime.h"
#include "ajabase/system/atomic.h"
#if defined(AJA_LINUX)
#include <stdarg.h>
#endif

#include <string.h>
#include <stdio.h>

static std::vector<std::string> sGroupLabelArray;
static const char* sSeverityString[] = {"emergency",  "alert", "assert", "error", "warning", "notice", "info", "debug"};
static int sRefCount = 0;
static AJALock sLock;
static AJADebugShare* spShare = NULL;
static bool sDebug = false;


AJADebug::AJADebug()
{
}


AJADebug::~AJADebug()
{
}


AJAStatus 
AJADebug::Open()
{
	AJAAutoLock lock(&sLock);

	// set the debug flag
	sDebug = false;
#if defined(AJA_DEBUG)
	sDebug = true;
#endif

	try
	{
		// increment reference count;
		sRefCount++;

		// allocate the shared data structure for messages
		if (spShare == NULL)
		{
			// allocate the shared memory storage
			size_t size = sizeof(AJADebugShare);
			spShare = (AJADebugShare*)AJAMemory::AllocateShared(&size, AJA_DEBUG_SHARE_NAME);
			if (spShare == NULL || spShare == (void*)(-1))
			{
				spShare = NULL;
				Close();
				return AJA_STATUS_FAIL;
			}

			if (size < sizeof(AJADebugShare))
			{
				Close();
				return AJA_STATUS_FAIL;
			}

			// check version
			if (spShare->version == 0)
			{
				memset(spShare, 0, sizeof(AJADebugShare));
				spShare->version = AJA_DEBUG_VERSION;
				spShare->unitArray[AJA_DebugUnit_Critical] = AJA_DEBUG_DESTINATION_CONSOLE;
			}

			// shared data must be the correct version
			if (spShare->version != AJA_DEBUG_VERSION)
			{
				Close();
				return AJA_STATUS_FAIL;
			}
		}
	}
	catch(...)
	{
		Close();
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJADebug::Close()
{
	AJAAutoLock lock(&sLock);

	try
	{
		// decrement reference count
		sRefCount--;
		if(sRefCount <= 0)
		{
			sRefCount = 0;

			// free the shared data structure
			if(spShare != NULL)
			{
				AJAMemory::FreeShared(spShare);
			}
		}
	}
	catch(...)
	{
	}

	spShare = NULL;

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJADebug::Enable(int32_t index, uint32_t destination)
{
	uint32_t currentDestination;
	AJAStatus status;

	status = GetDestination(index, &currentDestination);
	if(status != AJA_STATUS_SUCCESS)
	{
		return status;
	}
	return SetDestination(index, currentDestination | destination);
}

	
AJAStatus 
AJADebug::Disable(int32_t index, uint32_t destination)
{
	uint32_t currentDestination;
	AJAStatus status;

	status = GetDestination(index, &currentDestination);
	if(status != AJA_STATUS_SUCCESS)
	{
		return status;
	}
	return SetDestination(index, currentDestination & ~destination);
}


AJAStatus 
AJADebug::SetDestination(int32_t index, uint32_t destination)
{
	// check for open
	if (spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	// check for valid index
	if ((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
	{
		return AJA_STATUS_RANGE;
	}

	try
	{
		// save the destination
		spShare->unitArray[index] = destination;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJADebug::GetDestination(int32_t index, uint32_t* pDestination)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	if((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
	{
		return AJA_STATUS_RANGE;
	}

	if(pDestination == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*pDestination = spShare->unitArray[index];
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


bool
AJADebug::IsActive(int32_t index)
{
	// check for open
	if (spShare == NULL)
	{
        return false;
	}

	// check for valid index
	if ((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
	{
        return false;
	}

	try
	{
		// if no destination return false
		if (spShare->unitArray[index] == AJA_DEBUG_DESTINATION_NONE)
		{
            return false;
		}
	}
	catch(...)
	{
        return false;
	}

    return true;
}


bool
AJADebug::IsDebugBuild()
{
    return sDebug;
}


void 
AJADebug::Report(int32_t index, int32_t severity, const char* pFileName, int32_t lineNumber, ...)
{
	// check for open
	if (spShare == NULL)
	{
		return;
	}

	try
	{
		// check for valid index
		if ((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
		{
			index = AJA_DebugUnit_Unknown;
		}
		// check for destination
		if (spShare->unitArray[index] == AJA_DEBUG_DESTINATION_NONE)
		{
			return;
		}
		// check for valid severity
		if ((severity < 0) || (severity >= AJA_DebugSeverity_Size))
		{
			severity = AJA_DebugSeverity_Warning;
		}
		// check for valid file name
		if (pFileName == NULL)
		{
			pFileName = (char*) "unknown";
		}

		// increment the message write index
		int32_t writeIndex = AJAAtomic::Increment(&spShare->writeIndex);

		// modulo the ring size to determine the message array index
		int32_t messageIndex = writeIndex % AJA_DEBUG_MESSAGE_RING_SIZE;

		// save the message data
		spShare->messageRing[messageIndex].groupIndex = index;
		spShare->messageRing[messageIndex].destinationMask = spShare->unitArray[index];
		spShare->messageRing[messageIndex].time = DebugTime();
		strncpy(spShare->messageRing[messageIndex].fileName, pFileName, AJA_DEBUG_FILE_NAME_MAX_SIZE);
		spShare->messageRing[messageIndex].lineNumber = lineNumber;
		spShare->messageRing[messageIndex].severity = severity;

		// format the message
		va_list vargs;
		va_start(vargs, lineNumber);
		const char* pFormat = va_arg(vargs, const char*);
		// check for valid message
		if (pFormat == NULL)
		{
			pFormat = (char*) "no message";
		}
		vsnprintf(spShare->messageRing[messageIndex].messageText, AJA_DEBUG_MESSAGE_MAX_SIZE, pFormat, vargs);
		va_end(vargs);

		// set last to indicate message complete
		spShare->messageRing[messageIndex].sequenceNumber = writeIndex;
	}
	catch (...)
	{
	}
}

void 
AJADebug::Assert(const char* pFileName, int32_t lineNumber, const char* pExpression)
{
	// check for open
	if (spShare == NULL)
	{
		throw;
	}

	try
	{
		// check for valid file name
		if (pFileName == NULL)
		{
			pFileName = (char*) "unknown";
		}
		// check for valid expression
		if (pExpression == NULL)
		{
			pExpression = (char*) "no expression";
		}

		// increment the message write index
		int32_t writeIndex = AJAAtomic::Increment(&spShare->writeIndex);

		// modulo the ring size to determine the message array index
		int32_t messageIndex = writeIndex % AJA_DEBUG_MESSAGE_RING_SIZE;

		// save the message data
		spShare->messageRing[messageIndex].groupIndex = AJA_DebugUnit_Critical;
		spShare->messageRing[messageIndex].destinationMask = spShare->unitArray[AJA_DebugUnit_Critical];
		spShare->messageRing[messageIndex].time = DebugTime();
		strncpy(spShare->messageRing[messageIndex].fileName, pFileName, AJA_DEBUG_FILE_NAME_MAX_SIZE);
		spShare->messageRing[messageIndex].lineNumber = lineNumber;
		spShare->messageRing[messageIndex].severity = AJA_DebugSeverity_Assert;

		// format the message
		sprintf(spShare->messageRing[messageIndex].messageText, "assertion failed (file %s, line %d):  %s\n",
			pFileName, lineNumber, pExpression);

		// set last to indicate message complete
		spShare->messageRing[messageIndex].sequenceNumber = writeIndex;
	}
	catch (...)
	{
	}

	throw;
}


AJAStatus 
AJADebug::GetSequenceNumber(int32_t* pSequenceNumber)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	if(pSequenceNumber == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*pSequenceNumber = spShare->writeIndex;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS; 
}


AJAStatus
AJADebug::GetMessageSequenceNumber(int32_t sequenceNumber, int32_t* pSequenceNumber)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	if(sequenceNumber > spShare->writeIndex)
	{
		return AJA_STATUS_RANGE;
	}
	if(pSequenceNumber == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*pSequenceNumber = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].sequenceNumber;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJADebug::GetMessageGroup(int32_t sequenceNumber, int32_t* pGroupIndex)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	if(sequenceNumber > spShare->writeIndex)
	{
		return AJA_STATUS_RANGE;
	}
	if(pGroupIndex == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*pGroupIndex = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].groupIndex;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJADebug::GetMessageDestination(int32_t sequenceNumber, uint32_t* pDestination)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	if(sequenceNumber > spShare->writeIndex)
	{
		return AJA_STATUS_RANGE;
	}
	if(pDestination == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*pDestination = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].destinationMask;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJADebug::GetMessageTime(int32_t sequenceNumber, uint64_t* pTime)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	if(sequenceNumber > spShare->writeIndex)
	{
		return AJA_STATUS_RANGE;
	}
	if(pTime == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*pTime = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].time;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJADebug::GetMessageFileName(int32_t sequenceNumber, const char** ppFileName)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	if(sequenceNumber > spShare->writeIndex)
	{
		return AJA_STATUS_RANGE;
	}
	if(ppFileName == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*ppFileName = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].fileName;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJADebug::GetMessageLineNumber(int32_t sequenceNumber, int32_t* pLineNumber)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	if(sequenceNumber > spShare->writeIndex)
	{
		return AJA_STATUS_RANGE;
	}
	if(pLineNumber == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*pLineNumber = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].lineNumber;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJADebug::GetMessageSeverity(int32_t sequenceNumber, int32_t* pSeverity)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	if(sequenceNumber > spShare->writeIndex)
	{
		return AJA_STATUS_RANGE;
	}
	if(pSeverity == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*pSeverity = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].severity;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJADebug::GetMessageText(int32_t sequenceNumber, const char** ppMessage)
{
	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	if(sequenceNumber > spShare->writeIndex)
	{
		return AJA_STATUS_RANGE;
	}
	if(ppMessage == NULL)
	{
		return AJA_STATUS_NULL;
	}

	try
	{
		*ppMessage = spShare->messageRing[sequenceNumber%AJA_DEBUG_MESSAGE_RING_SIZE].messageText;
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus 
AJADebug::ReadGroupFile(char* pFileName)
{
	FILE* pGroup = NULL;
	char Label[256];
    int32_t index = 0;

	// must have a file name
	if(pFileName == NULL)
	{
		return AJA_STATUS_NULL;
	}

	// open label file
	pGroup = fopen(pFileName, "r");
	if(pGroup == NULL)
	{
		return AJA_STATUS_FAIL;
	}

	try
	{
		// scan until no more groups
		while(true)
		{
			// parse a group label
            if(fscanf(pGroup, " #%d %s", (int32_t *)&index, Label) != 2)
			{
				break;
			}

			// index must be in range
			if((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
			{
				continue;
			}

			// assign label
			sGroupLabelArray[index] = Label;
		}
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	// cleanup
	fclose(pGroup);

	return AJA_STATUS_SUCCESS;
}


const char* 
AJADebug::GetGroupString(int32_t index)
{
	if((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
	{
		return "index range error";
	}

	if(index >= (int32_t)sGroupLabelArray.size())
	{
		return "label range error";
	}

	if(sGroupLabelArray[index].empty())
	{
		return "no label";
	}

	return sGroupLabelArray[index].c_str();
}


const char* 
AJADebug::GetSeverityString(int32_t severity)
{
	if((severity < 0) || (severity >= (int32_t)(sizeof(sSeverityString)/sizeof(sSeverityString[0]))))
	{
		return "severity range error";
	}

	return sSeverityString[severity];
}


AJAStatus
AJADebug::SaveState(char* pFileName)
{
	FILE* pFile;

	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	// open a new state file
	pFile = fopen(pFileName, "w");
	if(pFile == NULL)
	{
		return AJA_STATUS_FAIL;
	}

	try
	{
		// write the header
		fprintf(pFile, "AJADebugVersion: %d\n", spShare->version);
		fprintf(pFile, "AJADebugStateFileVersion: %d\n", AJA_DEBUG_STATE_FILE_VERSION);

		int i;
		for (i = 0; i < AJA_DEBUG_UNIT_ARRAY_SIZE; i++)
		{
			// write groups with destinations enabled
			if (spShare->unitArray[i] != 0)
			{
				fprintf(pFile, "GroupDestination: %6d : %08x\n", i, spShare->unitArray[i]);
			}
		}
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	fclose(pFile);

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJADebug::RestoreState(char* pFileName)
{
	FILE* pFile;

	if(spShare == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	// open an existing file
	pFile = fopen(pFileName, "r");
	if(pFile == NULL)
	{
		return AJA_STATUS_FAIL;
	}

	try
	{
		int32_t count;
		uint32_t version;
		int32_t index;
		uint32_t destination;
		int intVersion;

		// read the header
		count = fscanf(pFile, " AJADebugVersion: %d", &intVersion);
		version = intVersion;
		if((count != 1) || (version != spShare->version))
		{
			return AJA_STATUS_FAIL;
		}
		count = fscanf(pFile, " AJADebugStateFileVersion: %d", &intVersion);
		version = intVersion;
		if((count != 1) || (version != AJA_DEBUG_STATE_FILE_VERSION))
		{
			return AJA_STATUS_FAIL;
		}

		while(true)
		{
			// read groups that have destinations
			count = fscanf(pFile, " GroupDestination: %d : %x", &index, &destination);
			if(count != 2)
			{
				break;
			}

			// index must be in range
			if((index < 0) || (index >= AJA_DEBUG_UNIT_ARRAY_SIZE))
			{
				continue;
			}

			// update the destination
			spShare->unitArray[index] = destination;
		}
	}
	catch(...)
	{
		return AJA_STATUS_FAIL;
	}

	fclose(pFile);

	return AJA_STATUS_SUCCESS;
}


int64_t
AJADebug::DebugTime()
{
	int64_t ticks = AJATime::GetSystemCounter();
	int64_t rate = AJATime::GetSystemFrequency();
	int64_t time = 0;

	time = ticks / rate * AJA_DEBUG_TICK_RATE;
	time += (ticks % rate) * AJA_DEBUG_TICK_RATE / rate;

	return time;
}
