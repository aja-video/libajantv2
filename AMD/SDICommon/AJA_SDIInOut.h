#pragma once

#include "defines.h"
#include "ntv2card.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.h"

#include "Thread.h"


#define MAX_CHANNELS 2

class SyncedBuffer;

class AJA_SDIInOut
{
public:

    AJA_SDIInOut(unsigned int uiBoardNumber = 0);
    ~AJA_SDIInOut(void);

    bool            openCard(unsigned int uiNumBuffers, bool bUseAutoCirculate, bool bUseP2P = false);
    void            closeCard();
    
    // Starts the sdi thread. Depending one the channel definition the input/output threads are started
    // Usually one thread per channel is used. If one input and one output is define and the InOutSingleThreadMode
    // was set, one thread will handle input and output. This can be used for lower latency.
    bool            start();

    // Stops thread loop but does not terminate thread
    void            halt();

    // Stops all sdi threads
    bool            stop();

    // Configure channel uiSDIChannel as input channel
    bool            setupInputChannel (unsigned int uiSDIChannel, NTV2FrameBufferFormat FBFormat, bool bQuad);
    // Configure channel uiSDIChannel as output channel
    bool            setupOutputChannel(unsigned int uiSDIChannel, NTV2FrameBufferFormat FBFormat, NTV2VideoFormat VideoFormat);

    // Deletes all existing channels
    void            deleteChannels();

    // Assigns a consumer/producer buffer to channel. This buffer will be used for data exchange
    void            setSyncBuffer(unsigned int uiChannel, SyncedBuffer*  pBuffer);

    // returns the number of SDI boards installed int the system
    static unsigned int getNumBoards(); 

    // Returns the index of the channel uiSDIChannel. uiSDIChannel specifies the physical channel.
    // uiSDIChannel 0 corresponds to NTV2_CHANNEL1 and uiSDIChannel 1 to NTV2_CHANNEL2. 
    unsigned int        getSDIChannelIndex(unsigned int uiSDIChannel);

    // returns the if of the board on which this instance was opened.
    unsigned int        getBoardNumber()                                    { return m_uiBoardNumber; };

    NTV2VideoFormat     getVideoFormat()                                    { return m_VideoFormat; };
    unsigned int        getFramebufferWidth (unsigned int uiChannel)        { return m_pChannels[uiChannel].uiFBWidth;   };
    unsigned int        getFramebufferHeight(unsigned int uiChannel)        { return m_pChannels[uiChannel].uiFBHeight;  };   
    unsigned int        getBytesPerPixel(unsigned int uiChannel);

    // If in p2p mode, this function will return a list of physical addresses of the surfaces on the sdi device
    unsigned int        getBusAddresses(unsigned int uiChannel, unsigned long long* &outVideoBusAddress, unsigned long long* &outMarkerBusAddress);

    // The following functions can be used to get a suggested GL format that corresponds to the SDI FB configuration.
    // This does not work for all FB configurations
    int             getIntFormat(unsigned int uiChannel);
    int             getExtFormat(unsigned int uiChannel);
    int             getType(unsigned int uiChannel);
    bool            isRGBFormat(NTV2FrameBufferFormat format);
   
private:

    enum SDIMode { SDI_NONE, SDI_INPUT, SDI_OUTPUT };

    typedef bool (CNTV2Card::*VERTICALEVENTFUNC)(NTV2FieldID fieldID );

    typedef struct
    {
        SDIMode                                 ioMode;
        SyncedBuffer*                           pSyncBuffer;
        unsigned int                            uiFBHeight;
        unsigned int                            uiFBWidth;
        unsigned int                            uiLinePitch;
        unsigned int                            uiSDIChannel;
        NTV2FrameBufferFormat                   FBFormat;
        NTV2Channel                             SDIChannel;
		AUTOCIRCULATE_TRANSFER                  TransferStruct;
		unsigned int							uiTransferSize;
		LWord                                   MinFrame;
        LWord                                   MaxFrame;
        ULWord64*                               pVideoBusAddress;
        ULWord64*                               pMarkerBusAddress;
    } Channel;

    static DWORD CALLAPI       inputThread(void* pArg);
    static DWORD CALLAPI       outputThread(void* pArg);

    void                        inputLoop();
    void                        outputLoop();

    // Transfers a frame from the SDI framebuffer to the GPU using AutoCirculate (either pinned mem or p2p)
    void                        transferWithAutoCirculateToGPU(unsigned int uiChannel, unsigned int uiTransferid = 0);
    // Transfers a frame from the GPU to the SDI framebuffer using AutoCirculate (either pinned mem or p2p)
    void                        transferWithAutoCirculateToSDI(unsigned int uiChannel);
    // Transfers a frame from the SDI framebuffer to the GPU using drirect dma (either pinned mem or p2p)
    void                        transferToGPU(unsigned int uiChannel, unsigned int uiFrame, unsigned int uiTransferid = 0);
    // Transfers a frame from the SDI framebuffer to the GPU using direct dma (either pinned mem or p2p)
    void                        transferToSDI(unsigned int uiChannel, unsigned int uiFrame = 0);

    // configure AutoCirculate and in case of p2p retreives bus addresses of buffers on the sdi device
    bool                        initAutoCirculate(unsigned int uiChannel);
 
    bool                        m_bUseP2P;
    bool                        m_bUseAutoCirculate;
    bool                        m_bRunning;
    bool                        m_bClearRouting;
  
    CNTV2DeviceScanner          m_ntv2BoardScan;
    CNTV2Card                   m_ntv2Card;
	NTV2EveryFrameTaskMode		m_savedTaskMode;
    UWord                       m_uwBoardNumber;
    NTV2DeviceID                m_DeviceID;
    NTV2ReferenceSource         m_ReferenceSource;
    NTV2VideoFormat             m_VideoFormat;

    NTV2Channel                 m_WaitChannel;
 
    unsigned int                m_uiBoardNumber;
    unsigned int                m_uiNumBuffers;

    unsigned int                m_uiNumChannels;
    Channel                     m_pChannels[MAX_CHANNELS];

    unsigned int                m_uiMaxAllowedBufferLevel;

    unsigned int                m_uiNumThreads;
    Thread                      m_pThread[MAX_CHANNELS];
};
