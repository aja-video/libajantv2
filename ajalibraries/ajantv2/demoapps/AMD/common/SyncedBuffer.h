#pragma once

#define MAX_BUFFERS 128

#include "Thread.h"

class SyncedBuffer
{
public:

    SyncedBuffer(void);
    virtual ~SyncedBuffer(void);

    // Create uiSize buffers. The buffers will have no memory assigned
    virtual void createSyncedBuffer(unsigned int uiSize);
    // assign memory to the buffer
    virtual void setBufferMemory(unsigned int uiId, void* pData);

    unsigned int getNumFullElements() { return m_uiNumFullElements; };

    // get a buffer for writing -> produce buffer
    unsigned int getBufferForWriting(void* &pBuffer);
    void releaseWriteBuffer();

    // get a buffer for reading -> consume buffer
    unsigned int getBufferForReading(void* &pBuffer);
    // get a buffer only if available this function will not block
    bool getBufferForReadingIfAvailable(void* &pBuffer, unsigned int &uiIdx);
    void releaseReadBuffer();

    unsigned int getNumSyncedBuffers() { return m_uiSize; };

protected:

    typedef struct
    {
        Semaphore       Mutex;
        void*           pData;
        unsigned int    uiIdx;
    } BufferElement;

private:

    BufferElement*      m_pBuffer;

    unsigned int        m_uiSize;
    unsigned int        m_uiHead;
    unsigned int        m_uiTail;
    unsigned int        m_uiNumFullElements;

    Semaphore           m_NumEmpty;
    Semaphore           m_NumFull;
};
