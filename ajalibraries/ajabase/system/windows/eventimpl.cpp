/**
	@file		windows/eventimpl.cpp
	@copyright	Copyright (C) 2009-2019 AJA Video Systems, Inc.  All rights reserved.
	@brief		Implements the AJAEventImpl class on the Windows platform.
**/

#include "ajabase/system/event.h"
#include "ajabase/system/windows/eventimpl.h"

#define MAX_EVENTS 64

using std::string;

// event implementation class (windows)
AJAEventImpl::AJAEventImpl(bool manualReset, const std::string& name)
	:mManualReset(manualReset)
{
	if (name == "")
	{
		mEvent = CreateEvent(NULL, manualReset? TRUE:FALSE, FALSE, NULL); 
	}
	else
	{
		mEvent = CreateEvent(NULL, manualReset? TRUE:FALSE, FALSE, name.c_str()); 
	}
}


AJAEventImpl::~AJAEventImpl()
{
	// close the handle to the windows event
	if (mEvent != NULL)
	{
		CloseHandle(mEvent);
	}
}


AJAStatus 
AJAEventImpl::Signal()
{
	// check for open
	if (mEvent == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	// signal event
	return ::SetEvent(mEvent) ? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL;
}


AJAStatus
AJAEventImpl::Clear()
{
	// check for open
	if (mEvent == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}

	return ::ResetEvent(mEvent) ? AJA_STATUS_SUCCESS : AJA_STATUS_FAIL;
}


AJAStatus
AJAEventImpl::SetState(bool signaled)
{
	AJAStatus status;

	if (signaled)
	{
		status = Signal();
	}
	else
	{
		status = Clear();
	}

	return status;
}


AJAStatus
AJAEventImpl::GetState(bool* pSignaled)
{
	if (pSignaled != NULL)
	{
		// wait with timeout 0 returns immediately
		AJAStatus status = WaitForSignal(0);
		// the event was signaled
		if (status == AJA_STATUS_SUCCESS)
		{
			*pSignaled = true;
		}
		else if (status == AJA_STATUS_TIMEOUT)
		{
			*pSignaled = false;
		}
		else
		{
			return status;
		}
	}

	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJAEventImpl::SetManualReset(bool manualReset)
{
	mManualReset = manualReset;
	return AJA_STATUS_SUCCESS;
}


AJAStatus
AJAEventImpl::GetManualReset(bool* pManualReset)
{
    if (pManualReset)
	{
		*pManualReset = mManualReset;
		return AJA_STATUS_SUCCESS;
	}
	else
		return AJA_STATUS_FAIL;
}


AJAStatus 
AJAEventImpl::WaitForSignal(uint32_t timeout)
{
	// check for open
	if (mEvent == NULL)
	{
		return AJA_STATUS_INITIALIZE;
	}
	
	// check for infinite timeout
	if (timeout == 0xffffffff)
	{
		timeout = INFINITE;
	}

	// wait for the event to be signaled
	DWORD retCode = WaitForSingleObject(mEvent, (DWORD)timeout);

	// the event was signaled
	if (retCode == WAIT_OBJECT_0)
	{
		return AJA_STATUS_SUCCESS;
	}
	// the wait timed out
	else if (retCode == WAIT_TIMEOUT)
	{
		return AJA_STATUS_TIMEOUT;
	}

	// some type of error occurred
	AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAEvent::WaitForSignal WaitForSingleObject() returns error %08x", retCode);

	return AJA_STATUS_FAIL;
}

AJAStatus 
AJAEventImpl::GetEventObject(uint64_t* pEventObject)
{
	if (pEventObject != NULL)
	{
		if (mEvent != INVALID_HANDLE_VALUE)
		{
			*pEventObject = (uint64_t)mEvent;
		}
		else
		{
			return AJA_STATUS_OPEN;
		}
	}

	return AJA_STATUS_SUCCESS;
}



AJAStatus
AJAWaitForEvents(AJAEvent* pEventList, uint32_t numEvents, bool all, uint32_t timeout)
{
	HANDLE eventArray[MAX_EVENTS];

	if (pEventList == NULL)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAWaitForEvents  event list is NULL");
		return AJA_STATUS_INITIALIZE;
	}

	if (numEvents == 0)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAWaitForEvents  event list is empty");
		return AJA_STATUS_RANGE;
	}

	if (numEvents > MAX_EVENTS)
	{
		AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAWaitForEvents  event list larger that MAX_EVENTS");
		return AJA_STATUS_RANGE;
	}

	uint32_t i;

	// build the array of handles from the AJAEvent(s)
	for (i = 0; i < numEvents; i++)
	{
		AJAEvent* pEvent = &pEventList[i];
		if(pEvent->mpImpl->mEvent == NULL)
		{
			AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAWaitForEvents  event not initialized");
			return AJA_STATUS_INITIALIZE;
		}
		eventArray[i] = pEvent->mpImpl->mEvent;
	}

	// check for infinite timeout
	if (timeout == 0xffffffff)
	{
		timeout = INFINITE;
	}

	// wait for event(s) to be signaled
	DWORD retCode = WaitForMultipleObjects(numEvents, eventArray, all, (DWORD)timeout);

	// an event was signaled
	if ((retCode >= WAIT_OBJECT_0) && (retCode < (WAIT_OBJECT_0 + numEvents)))
	{
		return AJA_STATUS_SUCCESS;
	}
	// the wait timed out
	else if (retCode == WAIT_TIMEOUT)
	{
		return AJA_STATUS_TIMEOUT;
	}

	// some type of error occurred
	AJA_REPORT(0, AJA_DebugSeverity_Error, "AJAEvent()::WaitForEvents WaitForMultipleObjects() returns error %08x", retCode);

	return AJA_STATUS_FAIL;
}
