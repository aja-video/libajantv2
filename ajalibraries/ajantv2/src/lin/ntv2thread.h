#if !defined THREAD_H
#define THREAD_H
//------------------------------------
//  (c) Reliable Software, 1997
//  Copyright (C) 2004 AJA Video Systems, Inc.  Proprietary and Confidential information.
//------------------------------------

#include "ajatypes.h"
#if defined(AJALinux)
# include "ntv2winlinhacks.h"
# include <pthread.h>
# include <stdio.h> 
# include <unistd.h>
#endif

typedef void *(*pt_func_pointer)(void *);

class Thread
{
public:

//	Thread ( void *(*pFun)(void *arg), void *pArg )
	Thread ( pt_func_pointer pFunc, void *pArg )
	{ 
		_func_pointer = pFunc;
		_func_args = pArg;
	}

    virtual ~Thread () { }

    void Resume () 
	{ 
		int rc = 0;
//		fprintf(stderr, "%s: func %p\n", __FUNCTION__, _func_pointer);
		rc = pthread_create(&_handle, NULL, _func_pointer, _func_args);
		if (rc) {
			fprintf(stderr, "%s: pthread_create error: %d\n", __FUNCTION__, rc);
		} else {
			fprintf(stderr, "%s: pthread_create OK\n", __FUNCTION__);
		}
	}

	void Suspend() {  }

    DWORD WaitForDeath ()
    {
		Sleep(10);
		fprintf(stderr, "%s: pthread_canceled\n", __FUNCTION__);
        return pthread_cancel(_handle);
    }

private:
    pthread_t _handle;
	pt_func_pointer _func_pointer;
	void *_func_args;
    DWORD  _tid;     // thread id
};

class Mutex
{
    friend class Lock;
public:
    Mutex () {  }
    ~Mutex () {  }
private:
    void Acquire () { }
    void Release () { }

#define CRITICAL_SECTION int
    CRITICAL_SECTION _critSection;
};

class Lock 
{
public:
	// Acquire the state of the semaphore
	Lock ( Mutex& mutex ) 
		: _mutex(mutex) 
	{
		_mutex.Acquire();
	}
	// Release the state of the semaphore
	~Lock ()
	{
		_mutex.Release();
	}
private:
	Mutex& _mutex;
};

class Event
{
public:
    Event () { }
    ~Event () { }

    // put into signaled state
    void Set () { }
    // put into non-signaled state
    void Reset () { }
#define INFINITE 0xFFFFFFFF
    DWORD Wait (DWORD waitTime = INFINITE ) { return 0; } 

private:
    HANDLE _handle;
};

class AtomicCounter
{
public:
    AtomicCounter () : _counter(0) {}

    long Inc () { return 0; }

    long Dec () { return 0; }

    BOOL IsNonZeroReset () { return false; }
  
private:
    long _counter;
};

#endif
