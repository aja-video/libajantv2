
#include "os_include.h"

#include "SyncedBuffer.h"

#define SB_TIMEOUT 250


SyncedBuffer::SyncedBuffer(void) : m_pBuffer(NULL), m_uiSize(0), m_uiHead(0), m_uiTail(0), m_uiNumFullElements(0)
{
}


SyncedBuffer::~SyncedBuffer(void)
{
    m_NumFull.destroy();
    m_NumEmpty.destroy();

    // Release all semaphores
    for (unsigned int i = 0; i < m_uiSize; i++)
    {
        m_pBuffer[i].Mutex.destroy();
    }

    if (m_pBuffer)
    {
        delete [] m_pBuffer;
    }
}

void SyncedBuffer::createSyncedBuffer(unsigned int uiSize)
{
    if (!m_pBuffer && uiSize < MAX_BUFFERS)
    {
        m_pBuffer = new BufferElement[uiSize];

        m_uiSize   = uiSize;
        m_uiHead   = 0;
        m_uiTail   = 0;

        //m_hNumFull  = CreateSemaphore(NULL,      0, uiSize, NULL);
        //m_hNumEmpty = CreateSemaphore(NULL, uiSize, uiSize, NULL);

        // Create Semaphore to monitor the nuber of full elements
        // initial value is 0, the max value is uiSize
        m_NumFull.create(0, uiSize);

        // Create Semaphore to monitor the nuber of empty elements
        // initial value is uiSize, the max value is uiSize
        m_NumEmpty.create(uiSize, uiSize);

        m_uiNumFullElements = 0;

        for (unsigned int i = 0; i < m_uiSize; i++)
        {
            m_pBuffer[i].Mutex.create(1, 1); //     = CreateSemaphore(NULL, 1, 1, NULL);
            m_pBuffer[i].pData      = NULL;
        }
    }
}


// assigns memory to this buffer.
void SyncedBuffer::setBufferMemory(unsigned int uiId, void* pData)
{
    if (m_pBuffer && uiId < m_uiSize)
    {
        //WaitForSingleObject(m_pBuffer[uiId].hMutex, INFINITE);
        m_pBuffer[uiId].Mutex.waitForObject(SB_TIMEOUT);
        m_pBuffer[uiId].pData  = pData;

        //ReleaseSemaphore(m_pBuffer[uiId].hMutex, 1, NULL);
        m_pBuffer[uiId].Mutex.release();
    }
}

// Produces an entry
// get a buffer for writing. Buffer will be filled
unsigned int SyncedBuffer::getBufferForWriting(void* &pBuffer)
{
    // Wait until an emty slot is available
    //WaitForSingleObject(m_hNumEmpty, INFINITE);
    m_NumEmpty.waitForObject(SB_TIMEOUT);

    // Enter critical section
    //WaitForSingleObject(m_pBuffer[m_uiHead].hMutex, INFINITE);
    m_pBuffer[m_uiHead].Mutex.waitForObject(SB_TIMEOUT);

    pBuffer = m_pBuffer[m_uiHead].pData;

    return m_uiHead;
}


// Mark buffer as full, ready to be consumed
void SyncedBuffer::releaseWriteBuffer()
{
    // Leave critical section
    //ReleaseSemaphore(m_pBuffer[m_uiHead].hMutex, 1, 0);
    m_pBuffer[m_uiHead].Mutex.release();

    // Increment the number of full buffers
    //ReleaseSemaphore(m_hNumFull, 1, NULL);
    m_NumFull.release();

    ++m_uiNumFullElements;

    // switch to next buffer
    m_uiHead = (m_uiHead + 1) % m_uiSize;
}


// get a buffer for reading
unsigned int SyncedBuffer::getBufferForReading(void* &pBuffer)
{
    // Wait until the buffer is available
    //WaitForSingleObject(m_hNumFull, INFINITE);
    m_NumFull.waitForObject(SB_TIMEOUT);

    // Block buffer
    //WaitForSingleObject(m_pBuffer[m_uiTail].hMutex, INFINITE);
    m_pBuffer[m_uiTail].Mutex.waitForObject(SB_TIMEOUT);

    pBuffer = m_pBuffer[m_uiTail].pData;

    return m_uiTail;
}   


// Check if a full buffer is ready but do not block in case no buffer is available
bool SyncedBuffer::getBufferForReadingIfAvailable(void* &pBuffer, unsigned int &uiIdx)
{
    //DWORD dwStatus = WaitForSingleObject(m_hNumFull, 0);
    bool bStatus = m_NumFull.waitForObject(0);

    if (bStatus)
    {
        // Block buffer
        //WaitForSingleObject(m_pBuffer[m_uiTail].hMutex, INFINITE);
        m_pBuffer[m_uiTail].Mutex.waitForObject();

        pBuffer = m_pBuffer[m_uiTail].pData;

        uiIdx = m_uiTail;

        return true;
    }

    return false;
}



// Mark buffer as empty, ready to be filled
void SyncedBuffer::releaseReadBuffer()
{
    // Release buffer
    //ReleaseSemaphore(m_pBuffer[m_uiTail].hMutex, 1, NULL);
    m_pBuffer[m_uiTail].Mutex.release();

    // Increase number of empty buffers
    //ReleaseSemaphore(m_hNumEmpty, 1, NULL);
    m_NumEmpty.release();

    --m_uiNumFullElements;

    // switch to next buffer
    m_uiTail = (m_uiTail + 1) % m_uiSize;
}

