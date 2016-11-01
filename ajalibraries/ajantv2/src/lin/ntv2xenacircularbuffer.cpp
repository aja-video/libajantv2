#include "ntv2xenacircularbuffer.h"
#include "ntv2debug.h"

CXenaCircularBuffer::CXenaCircularBuffer()
	: _head(0),
	  _tail(0),
	  _fillIndex(0),
	  _emptyIndex(0),
	  _notFullEvent(NULL),
	  _notEmptyEvent(NULL),
	  _dataBufferMutex(NULL),
	  _circBufferCount(0),
	  _pAbortFlag(NULL)
{
	_dataBufferMutex = CreateMutex(NULL,FALSE,NULL);
	_notFullEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
	_notEmptyEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
}

CXenaCircularBuffer::~CXenaCircularBuffer()
{
	CloseHandle(_dataBufferMutex);
	CloseHandle(_notFullEvent);
	CloseHandle(_notEmptyEvent);
	XenaBufferMutexList::iterator mutIter;
	for ( mutIter = _xenaBufferMutexList.begin(); mutIter != _xenaBufferMutexList.end(); mutIter++ )
	{
		CloseHandle(*mutIter);
	}
	
}


void CXenaCircularBuffer::Init(int* abortFlag)
{
	_pAbortFlag = abortFlag;
	_xenaBufferList.clear();
	_xenaBufferMutexList.clear();
	_head = 0;
	_tail = 0;
	_fillIndex = 0;
	_emptyIndex = 0;
	_circBufferCount = 0;
	
}

bool CXenaCircularBuffer::Add(PVOID buffer)
{
	_xenaBufferList.push_back(buffer);
	HANDLE mutex = CreateMutex(NULL,FALSE,NULL);
	if ( mutex == NULL )
		return false;

	_xenaBufferMutexList.push_back(mutex);
	return true;
}

PVOID CXenaCircularBuffer::StartProduceNextBuffer()
{
	while (1)
	{
		
		WAIT_OR_DIE(_dataBufferMutex,_pAbortFlag,WAIT_TIMEOUT_MS,done);	
		if ( _circBufferCount == _xenaBufferList.size())
		{
			ReleaseMutex(_dataBufferMutex);
			WAIT_OR_DIE(_notFullEvent,_pAbortFlag,WAIT_TIMEOUT_MS,done);
			continue;
		}
		break;
	}
	WAIT_OR_DIE(_xenaBufferMutexList[_head],_pAbortFlag,WAIT_TIMEOUT_MS,done);
	_fillIndex = _head;
	_head = (_head+1)%(_xenaBufferList.size());
	_circBufferCount++;
	ReleaseMutex(_dataBufferMutex);

	return _xenaBufferList[_fillIndex];

done:
	return NULL;
}

void CXenaCircularBuffer::EndProduceNextBuffer()
{
	ReleaseMutex(_xenaBufferMutexList[_fillIndex]);
	PulseEvent(_notEmptyEvent);

}

PVOID CXenaCircularBuffer::StartConsumeNextBuffer()
{
	while (1)
	{
		WAIT_OR_DIE(_dataBufferMutex,_pAbortFlag,WAIT_TIMEOUT_MS,done);	
		if ( _circBufferCount == 0)
		{
			ReleaseMutex(_dataBufferMutex);
			WAIT_OR_DIE(_notEmptyEvent,_pAbortFlag,WAIT_TIMEOUT_MS,done);
			continue;
		}
		break;
	}
	WAIT_OR_DIE(_xenaBufferMutexList[_tail],_pAbortFlag,WAIT_TIMEOUT_MS,done);
	_emptyIndex = _tail;
	_tail = (_tail+1)%(_xenaBufferList.size());
	_circBufferCount--;
	ReleaseMutex(_dataBufferMutex);

	return _xenaBufferList[_emptyIndex];

done:
	return NULL;
}

void  CXenaCircularBuffer::EndConsumeNextBuffer()
{
	ReleaseMutex(_xenaBufferMutexList[_emptyIndex]);
	PulseEvent(_notFullEvent);
}

