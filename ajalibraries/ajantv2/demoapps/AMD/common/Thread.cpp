

#include "Thread.h"




#if defined( WIN32 )
//----------------------------------------------------------------------------------------------
//  Windows implementation
//----------------------------------------------------------------------------------------------

Thread::Thread() : m_bRunning(false), m_hThread(NULL), m_uiThreadId(0)
{
}

Thread::~Thread()
{
    if (m_bRunning)
    {
        join();
    }
}

bool Thread::create(THREAD_PROC pThreadFunc, void *pArg)
{
    if (pThreadFunc == NULL || m_hThread != NULL)
    {
        return false;
    }

    m_hThread = CreateThread(NULL, 0, pThreadFunc, pArg, 0, &((DWORD)m_uiThreadId));

    if (m_hThread == NULL)
    {
        return false;
    }

    m_bRunning = true;

    return true;
}


void Thread::join()
{
    if (m_hThread)
    {
        WaitForSingleObject(m_hThread, INFINITE);
    }

    m_bRunning = false;
}




Semaphore::Semaphore() : m_Semaphore(NULL), m_uiMax(0)
{
}

Semaphore::~Semaphore()
{
    if (m_Semaphore)
    {
        destroy();
    }
}


bool Semaphore::create(unsigned int uiInitial, unsigned int uiMax)
{
    if (m_Semaphore != NULL)
    {
        return NULL;
    }

    m_Semaphore = CreateSemaphore(NULL, uiInitial, uiMax, NULL);

    if (m_Semaphore == NULL)
    {
        return false;
    }

    m_uiMax = uiMax;

    return true;
}


void Semaphore::destroy()
{
    if (m_Semaphore)
    {
        ReleaseSemaphore(m_Semaphore, 1, 0);
        CloseHandle(m_Semaphore);

        m_Semaphore = NULL;
    }
}


void Semaphore::release()
{
    if (m_Semaphore)
    {
        ReleaseSemaphore(m_Semaphore, 1, 0);
    }
}

bool Semaphore::waitForObject(unsigned long uiTimeOut)
{
    if (!m_Semaphore)
    {
        return false;
    }

    DWORD dwStatus = WaitForSingleObject(m_Semaphore, uiTimeOut);

    if (dwStatus != WAIT_OBJECT_0)
    {
        return false;
    }

    return true;
}


#elif defined ( LINUX )

//----------------------------------------------------------------------------------------------
//  Linux implementation of Thread
//----------------------------------------------------------------------------------------------
#include <pthread.h>

Thread::Thread() : m_bRunning(false), m_uiThreadId(0)
{
}

Thread::~Thread()
{
    if (m_bRunning)
    {
        join();
    }
}

bool Thread::create(THREAD_PROC pThreadFunc, void *pArg)
{
    if (pThreadFunc == NULL)
    {
        return false;
    }

    int res = pthread_create(&m_uiThreadId, NULL, pThreadFunc, pArg);

    if (res != 0)
    {
        return false;
    }

    m_bRunning = true;

    return true;
}

void Thread::join()
{
    pthread_join(m_uiThreadId, NULL);

    m_bRunning = false;
}



Semaphore::Semaphore() : m_uiMax(0)
{
}


Semaphore::~Semaphore() 
{
    //destroy();
}

bool Semaphore::create(unsigned int uiInitial, unsigned int uiMax)
{
    if (pthread_mutex_init(&m_Mutex, NULL) != 0)
    {
        return false;
    }

    if (pthread_cond_init(&m_Condition, NULL))
    {
        return false;
    }

    m_SemCount = uiInitial;
    m_uiMax    = uiMax;

    
    return true;
}


void Semaphore::destroy()
{
    pthread_mutex_destroy(&m_Mutex);
    pthread_cond_destroy(&m_Condition);
}

void Semaphore::release()
{
    pthread_mutex_lock(&m_Mutex);

    if (m_SemCount < m_uiMax)
    {
        m_SemCount++;
    }
    
    pthread_cond_signal(&m_Condition);
    pthread_mutex_unlock(&m_Mutex);
}

bool Semaphore::waitForObject(unsigned long uiTimeOut)
{
    pthread_mutex_lock(&m_Mutex);

    while (m_SemCount < 1)
    {
        pthread_cond_wait(&m_Condition, &m_Mutex);
    }

    m_SemCount--;
    pthread_mutex_unlock(&m_Mutex);
    
    return true;
}

#endif
