#pragma once

#include "os_include.h"

#if defined (WIN32)

typedef LPTHREAD_START_ROUTINE  THREAD_PROC;
#define CALLAPI WINAPI

#elif defined ( LINUX )

typedef void *(*THREAD_PROC)(void*);
#define INFINITE            0xFFFFFFFF 
#define CALLAPI
#endif


class Thread
{
public:
    Thread();
    virtual ~Thread();

    bool            create(THREAD_PROC pThreadFunc, void* pArg);
    void            join();

    //HANDLE          getThreadHandle()   { return m_hThread; };
    unsigned int    getThreadId()       { return m_uiThreadId; };

private:

    bool            m_bRunning;
    unsigned long   m_uiThreadId;

#if defined ( WIN32 )
    HANDLE          m_hThread;
#endif

};



class Semaphore
{
public:
    Semaphore();
    virtual ~Semaphore();

    bool                create(unsigned int uiInitial, unsigned int uiMax);   
    void                destroy();
    bool                waitForObject(unsigned long uiTimeOut = INFINITE);
    void                release();

private:

    unsigned int        m_uiMax;

#if defined ( WIN32 )
    HANDLE              m_Semaphore;
#elif defined (LINUX)

    pthread_mutex_t     m_Mutex; 
    pthread_cond_t      m_Condition; 
    volatile int        m_SemCount; 

#endif
};
