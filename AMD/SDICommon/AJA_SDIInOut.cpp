
#include "os_include.h"

#include "GL/glew.h"

#include "ntv2formatdescriptor.h"
#include "SyncedBuffer.h"
#include "AJA_SDIInOut.h"

#define FCC(a,b,c,d)  \
( (((uint32_t)(a)) << 24)  + \
  (((uint32_t)(b)) << 16)  + \
  (((uint32_t)(c)) <<  8)  + \
  (((uint32_t)(d)) <<  0) ) 
 
struct AJA_FormatInfo
{
    NTV2FrameBufferFormat   AJA_Format;
    int                     GL_IntFormat;
    int                     GL_ExtFormat;
    int                     GL_Type;
};


// A list of supported formats with the corresponding formats that can be used
// in OpenGL
struct AJA_FormatInfo AJA_FormatTable[] =
{
    { NTV2_FBF_ARGB,        GL_RGBA8,       GL_BGRA,    GL_UNSIGNED_BYTE},
    { NTV2_FBF_RGBA,        GL_RGBA8,       GL_RGBA,    GL_UNSIGNED_BYTE},
    { NTV2_FBF_ABGR,        GL_RGBA8,       GL_RGBA,    GL_UNSIGNED_BYTE},
    { NTV2_FBF_24BIT_RGB,   GL_RGB8,        GL_RGB,     GL_UNSIGNED_BYTE},
    { NTV2_FBF_24BIT_BGR,   GL_RGB8,        GL_BGR,     GL_UNSIGNED_BYTE},
};


AJA_SDIInOut::AJA_SDIInOut(unsigned int uiBoardNumber)
{
    m_bClearRouting = true;
    m_bRunning      = false;

    m_bUseP2P           = true;
    m_bUseAutoCirculate = false;
   
    m_uiBoardNumber     = uiBoardNumber;

    m_uiNumBuffers      = 8;

    m_VideoFormat = NTV2_FORMAT_UNKNOWN;

	m_WaitChannel = NTV2_CHANNEL1;

    m_uiNumChannels = 0;
    m_uiNumThreads  = 0;
    
    m_ReferenceSource   = NTV2_REFERENCE_INPUT1;

    m_VideoFormat = NTV2_FORMAT_UNKNOWN;

    // Define the number of frames thar are accepted to be stored
    // in AutoCirculate buffer before flushing the buffer to catch up.
    m_uiMaxAllowedBufferLevel = 8;
}


AJA_SDIInOut::~AJA_SDIInOut(void)
{
    if (m_bRunning)
        stop();

    closeCard();

    for (unsigned int i = 0; i < m_uiNumChannels; i++)
    {
        if (m_pChannels[i].pVideoBusAddress)
            delete [] m_pChannels[i].pVideoBusAddress;

        if (m_pChannels[i].pMarkerBusAddress)
            delete [] m_pChannels[i].pMarkerBusAddress;
    }
}


unsigned int AJA_SDIInOut::getNumBoards()
{
    CNTV2DeviceScanner ntv2BoardScan;

    return (unsigned int)ntv2BoardScan.GetNumDevices();
}


bool AJA_SDIInOut::openCard(unsigned int uiNumBuffers, bool bUseAutoCirculate, bool bUseP2P)
{
    
    CNTV2DeviceScanner ntv2BoardScan;

    if (m_uiBoardNumber >= ntv2BoardScan.GetNumDevices())
    {
        return false;
    }

    if (!m_ntv2Card.Open((UWord)m_uiBoardNumber))
        return false;

	if (!m_ntv2Card.AcquireStreamForApplication (FCC('D', 'E', 'M', 'O'), static_cast <uint32_t> (GetCurrentProcessId ())))
		return false;		//	Device is in use by another app -- fail

	m_ntv2Card.GetEveryFrameServices (&m_savedTaskMode);	//	Save the current service level
	m_ntv2Card.SetEveryFrameServices(NTV2_OEM_TASKS);	

	// verify the board can be a p2p target
    m_DeviceID = m_ntv2Card.GetDeviceID();

    // define if the AutoCirculate Interface should be used
    m_bUseAutoCirculate = bUseAutoCirculate;

    // define if a peer to peer copy should be used
    m_bUseP2P = bUseP2P;

    // define how many buffers to use
    m_uiNumBuffers = uiNumBuffers;

    return true;
}


void AJA_SDIInOut::closeCard()
{
    if (m_ntv2Card.IsOpen())
    {
        if (m_bUseAutoCirculate)
        {
            m_ntv2Card.AutoCirculateStop(NTV2_CHANNEL1);
			m_ntv2Card.AutoCirculateStop(NTV2_CHANNEL2);
        }

        m_ntv2Card.ClearRouting();

        m_uiNumChannels = 0;

		m_ntv2Card.SetEveryFrameServices (m_savedTaskMode);			//	Restore the previously saved service level
		m_ntv2Card.ReleaseStreamForApplication (FCC('D', 'E', 'M', 'O'), static_cast <uint32_t> (GetCurrentProcessId ()));	//	Release the device

        m_ntv2Card.Close();
	}
}


bool AJA_SDIInOut::setupInputChannel(unsigned int uiSDIChannel, NTV2FrameBufferFormat FBFormat, bool bQuad)
{
    NTV2VideoFormat VideoFormat;

    if (!m_ntv2Card.IsOpen())
        return false;

    if (m_uiNumChannels >= MAX_CHANNELS)
        return false;

    m_pChannels[m_uiNumChannels].uiSDIChannel = uiSDIChannel;

	if (uiSDIChannel == 0)
    {
        // Use SDI 1 (IN 1) 
        m_pChannels[m_uiNumChannels].SDIChannel  = NTV2_CHANNEL1;

		if (::NTV2DeviceHasBiDirectionalSDI (m_DeviceID))
		{
			m_ntv2Card.SetSDITransmitEnable (m_pChannels[m_uiNumChannels].SDIChannel, false);
			Sleep(500);
		}

        // Get video format on input 1
        VideoFormat = m_ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI1);
    }
    else if (uiSDIChannel == 1)
    {
        // Use SDI 2 (IN 2)
        m_pChannels[m_uiNumChannels].SDIChannel  = NTV2_CHANNEL2;

		if (::NTV2DeviceHasBiDirectionalSDI (m_DeviceID))
		{
			m_ntv2Card.SetSDITransmitEnable (m_pChannels[m_uiNumChannels].SDIChannel, false);
			Sleep(500);
		}

        // Get video format on input 2
        VideoFormat = m_ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI2);
    }
    else if (uiSDIChannel == 2)
    {
        // Use SDI 2 (IN 2)
        m_pChannels[m_uiNumChannels].SDIChannel  = NTV2_CHANNEL3;

		if (::NTV2DeviceHasBiDirectionalSDI (m_DeviceID))
		{
			m_ntv2Card.SetSDITransmitEnable (m_pChannels[m_uiNumChannels].SDIChannel, false);
			Sleep(500);
		}

        // Get video format on input 2
        VideoFormat = m_ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI3);
    }
    else if (uiSDIChannel == 3)
    {
        // Use SDI 2 (IN 2)
        m_pChannels[m_uiNumChannels].SDIChannel  = NTV2_CHANNEL4;

		if (::NTV2DeviceHasBiDirectionalSDI (m_DeviceID))
		{
			m_ntv2Card.SetSDITransmitEnable (m_pChannels[m_uiNumChannels].SDIChannel, false);
			Sleep(500);
		}

        // Get video format on input 2
        VideoFormat = m_ntv2Card.GetInputVideoFormat(NTV2_INPUTSOURCE_SDI4);
    }
    else
    {
        return false;
    }

    // To configure an input channel, a valid video source must be connected
    if (VideoFormat == NTV2_FORMAT_UNKNOWN)
        return false;

	// Get quad format
	if (bQuad)
	{
		VideoFormat = ::GetQuadSizedVideoFormat(VideoFormat);
	}
    if (VideoFormat == NTV2_FORMAT_UNKNOWN)
    return false;

	// Adapt video format to the format detected on the input
	if (!m_ntv2Card.SetVideoFormat(VideoFormat))
		return false;

	m_VideoFormat = VideoFormat;

	m_pChannels[m_uiNumChannels].MinFrame = m_uiNumChannels * m_uiNumBuffers;
    m_pChannels[m_uiNumChannels].MaxFrame = m_uiNumChannels * m_uiNumBuffers + m_uiNumBuffers - 1;
    m_pChannels[m_uiNumChannels].FBFormat = FBFormat;
	
    if (m_bClearRouting)
        m_ntv2Card.ClearRouting();

    m_bClearRouting = false;

	if (NTV2_IS_QUAD_FRAME_FORMAT(VideoFormat))
	{
		for (int i = 0; i < 4; i++)
		{
			// disable transmitter
			if (::NTV2DeviceHasBiDirectionalSDI (m_DeviceID))
			{
				m_ntv2Card.SetSDITransmitEnable ((NTV2Channel)i, false);
			}

			// frame buffer pixel format
			if (!m_ntv2Card.SetFrameBufferFormat((NTV2Channel)i, FBFormat))
				return false;

			// frame buffer capture mode
			if (!m_ntv2Card.SetMode((NTV2Channel)i, NTV2_MODE_CAPTURE))
				return false;

			if (!m_ntv2Card.EnableChannel((NTV2Channel)i))
				return false;

			// cross point routing
			if (::NTV2DeviceNeedsRoutingSetup(m_DeviceID))
			{
				if(isRGBFormat(FBFormat))
				{
					m_ntv2Card.Connect (::GetCSCInputXptFromChannel ((NTV2Channel)i), 
						::GetSDIInputOutputXptFromChannel ((NTV2Channel)i));
					m_ntv2Card.Connect (::GetFrameBufferInputXptFromChannel ((NTV2Channel)i), 
						::GetCSCOutputXptFromChannel ((NTV2Channel)i, false, true));
					m_ntv2Card.SetColorSpaceMakeAlphaFromKey(0, (NTV2Channel)i);
				}
				else
				{
					m_ntv2Card.Connect (::GetFrameBufferInputXptFromChannel ((NTV2Channel)i), 
						::GetSDIInputOutputXptFromChannel ((NTV2Channel)i));
				}
			}
		}
	}
	else
	{
		// frame buffer pixel format
		if (!m_ntv2Card.SetFrameBufferFormat(m_pChannels[m_uiNumChannels].SDIChannel, FBFormat))
			return false;

		// frame buffer capture mode
		if (!m_ntv2Card.SetMode(m_pChannels[m_uiNumChannels].SDIChannel, NTV2_MODE_CAPTURE))
			return false;

		if (!m_ntv2Card.EnableChannel(m_pChannels[m_uiNumChannels].SDIChannel))
			return false;

		// cross point routing
		if (::NTV2DeviceNeedsRoutingSetup(m_DeviceID))
		{
			if(isRGBFormat(FBFormat))
			{
				m_ntv2Card.Connect (::GetCSCInputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel), 
					::GetSDIInputOutputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel));
				m_ntv2Card.Connect (::GetFrameBufferInputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel), 
					::GetCSCOutputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel, false, true));
				m_ntv2Card.SetColorSpaceMakeAlphaFromKey(0, m_pChannels[m_uiNumChannels].SDIChannel);
			}
			else
			{
				m_ntv2Card.Connect (::GetFrameBufferInputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel), 
					::GetSDIInputOutputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel));
			}
		}
	}
    
    m_pChannels[m_uiNumChannels].ioMode = SDI_INPUT;
    
	NTV2FormatDescriptor fd = GetFormatDescriptor(VideoFormat, m_pChannels[m_uiNumChannels].FBFormat);

	m_pChannels[m_uiNumChannels].uiFBWidth   = fd.numPixels;       // FB width in Pixel
	m_pChannels[m_uiNumChannels].uiFBHeight  = fd.numLines;        // number of lines
	m_pChannels[m_uiNumChannels].uiLinePitch = fd.linePitch;       // size of one line in 32 Bit DWORDS

//	m_ntv2Card.SetFrameBufferOrientation(m_pChannels[m_uiNumChannels].SDIChannel, NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP);
	m_ntv2Card.SetFrameBufferOrientation(m_pChannels[m_uiNumChannels].SDIChannel, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);

	if (m_bUseAutoCirculate)
    {
        if (!initAutoCirculate(m_uiNumChannels))
            return false;
    }
    else
    {
        m_ntv2Card.SetRegisterWritemode(NTV2_REGWRITE_SYNCTOFRAME);
    }

    ++m_uiNumChannels;

    return true;
}


bool AJA_SDIInOut::setupOutputChannel(unsigned int uiSDIChannel, NTV2FrameBufferFormat FBFormat, NTV2VideoFormat VideoFormat)
{
    if (!m_ntv2Card.IsOpen())
        return false;

    if (m_uiNumChannels >= MAX_CHANNELS)
        return false;

    m_pChannels[m_uiNumChannels].uiSDIChannel = uiSDIChannel;

    if (uiSDIChannel == 0)
    {
        // Use SDI 1/3 (Out 1)
        m_pChannels[m_uiNumChannels].SDIChannel  = NTV2_CHANNEL1;
    }
    else if (uiSDIChannel == 1)
    {
        // Use SDI 2/4 (Out 2)
        m_pChannels[m_uiNumChannels].SDIChannel  = NTV2_CHANNEL2;
    }
    else if (uiSDIChannel == 2)
    {
        // Use SDI 3 (Out 3)
        m_pChannels[m_uiNumChannels].SDIChannel  = NTV2_CHANNEL3;
    }
    else if (uiSDIChannel == 3)
    {
        // Use SDI 4 (Out 4)
        m_pChannels[m_uiNumChannels].SDIChannel  = NTV2_CHANNEL4;
    }
    else
    {
        return false;
    }

    m_pChannels[m_uiNumChannels].MinFrame = m_uiNumChannels * m_uiNumBuffers;
    m_pChannels[m_uiNumChannels].MaxFrame = m_uiNumChannels * m_uiNumBuffers + m_uiNumBuffers - 1;
    m_pChannels[m_uiNumChannels].FBFormat = FBFormat;

    if (m_bClearRouting)
        m_ntv2Card.ClearRouting();

    m_bClearRouting = false;

	if (NTV2_IS_QUAD_FRAME_FORMAT(VideoFormat))
	{
		for (int i = 0; i < 4; i++)
		{
			// frame buffer pixel format
			if (!m_ntv2Card.SetFrameBufferFormat((NTV2Channel)i, FBFormat))
				return false;
			
			// frame buffer display mode
			if (!m_ntv2Card.SetMode((NTV2Channel)i, NTV2_MODE_DISPLAY))
				return false;

			if (!m_ntv2Card.EnableChannel((NTV2Channel)i))
				return false;

			// enable transmitter
			if (::NTV2DeviceHasBiDirectionalSDI (m_DeviceID))
				m_ntv2Card.SetSDITransmitEnable ((NTV2Channel)i, true);

			// route video
			if(isRGBFormat(FBFormat))
			{
				m_ntv2Card.Connect (::GetCSCInputXptFromChannel ((NTV2Channel)i, false),  
					::GetFrameBufferOutputXptFromChannel ((NTV2Channel)i,  true,  false));
				m_ntv2Card.Connect (::GetSDIOutputInputXpt ((NTV2Channel)i, false),  
					::GetCSCOutputXptFromChannel ((NTV2Channel)i,  false,  false));
			}
			else
			{
				m_ntv2Card.Connect (::GetSDIOutputInputXpt ((NTV2Channel)i, false),  
					::GetFrameBufferOutputXptFromChannel ((NTV2Channel)i,  false,  false));
			}
		}
	}
	else
	{
		// frame buffer pixel format
	    if (!m_ntv2Card.SetFrameBufferFormat(m_pChannels[m_uiNumChannels].SDIChannel, FBFormat))
			return false;

		// frame buffer display mode
		if (!m_ntv2Card.SetMode(m_pChannels[m_uiNumChannels].SDIChannel, NTV2_MODE_DISPLAY))
			return false;

		if (!m_ntv2Card.EnableChannel(m_pChannels[m_uiNumChannels].SDIChannel))
			return false;

		// enable transmitter
		if (::NTV2DeviceHasBiDirectionalSDI (m_DeviceID))
			m_ntv2Card.SetSDITransmitEnable (m_pChannels[m_uiNumChannels].SDIChannel, true);

		// route video
		if(isRGBFormat(FBFormat))
		{
			m_ntv2Card.Connect (::GetCSCInputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel, false),  
				::GetFrameBufferOutputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel,  true,  false));
			m_ntv2Card.Connect (::GetSDIOutputInputXpt (m_pChannels[m_uiNumChannels].SDIChannel, false),  
				::GetCSCOutputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel,  false,  false));
		}
		else
		{
			m_ntv2Card.Connect (::GetSDIOutputInputXpt (m_pChannels[m_uiNumChannels].SDIChannel, false),  
				::GetFrameBufferOutputXptFromChannel (m_pChannels[m_uiNumChannels].SDIChannel,  false,  false));
		}
	}

    // We might need to define the VideoFormat. In case that an input channel is already configured,
    // we already have a valid video format applied, if only an output is used the format needs to be applied.
    if (m_VideoFormat == NTV2_FORMAT_UNKNOWN && VideoFormat != NTV2_FORMAT_UNKNOWN)
    {
        if (!m_ntv2Card.SetVideoFormat(VideoFormat))
            return false;

        m_VideoFormat = VideoFormat;
    }

    m_pChannels[m_uiNumChannels].ioMode = SDI_OUTPUT;

	NTV2FormatDescriptor fd = GetFormatDescriptor(VideoFormat, m_pChannels[m_uiNumChannels].FBFormat);

	m_pChannels[m_uiNumChannels].uiFBWidth = fd.numPixels;       // FB width in Pixel
	m_pChannels[m_uiNumChannels].uiFBHeight = fd.numLines;        // number of lines
	m_pChannels[m_uiNumChannels].uiLinePitch = fd.linePitch;       // size of one line in 32 Bit DWORDS
	if (NTV2_IS_QUAD_FRAME_FORMAT(VideoFormat))
	{
	    m_ntv2Card.SetFrameBufferOrientation(m_pChannels[m_uiNumChannels].SDIChannel, NTV2_FRAMEBUFFER_ORIENTATION_TOPDOWN);  // quad only topdown for now
	}
	else
	{
	    m_ntv2Card.SetFrameBufferOrientation(m_pChannels[m_uiNumChannels].SDIChannel, NTV2_FRAMEBUFFER_ORIENTATION_BOTTOMUP);
	}


    if (m_bUseAutoCirculate)
    {
        if (!initAutoCirculate(m_uiNumChannels))
            return false;
    }
    else
    {
        m_ntv2Card.SetRegisterWritemode(NTV2_REGWRITE_SYNCTOFRAME);

        if (m_bUseP2P)
        {
            // Allocate space to store bus addresses
            m_pChannels[m_uiNumChannels].pVideoBusAddress  = new ULWord64[m_uiNumBuffers];
            m_pChannels[m_uiNumChannels].pMarkerBusAddress = new ULWord64[m_uiNumBuffers];

            // get the bus addresses
            for (unsigned int i = 0; i < m_uiNumBuffers; ++i)
            {
                AUTOCIRCULATE_P2P_STRUCT p2pData;

			    // get target p2p information
			    if(m_ntv2Card.DmaP2PTargetFrame(m_pChannels[m_uiNumChannels].SDIChannel, m_pChannels[m_uiNumChannels].MinFrame + i, 0, &p2pData))
                {
                    m_pChannels[m_uiNumChannels].pVideoBusAddress[i]  = p2pData.videoBusAddress;
                    m_pChannels[m_uiNumChannels].pMarkerBusAddress[i] = p2pData.messageBusAddress;
                }
                else
                {
                    return false;
                }
            }
        }
    }

    ++m_uiNumChannels;

    return true;
}


bool AJA_SDIInOut::initAutoCirculate(unsigned int uiChannel)
{
	m_ntv2Card.AutoCirculateStop(m_pChannels[uiChannel].SDIChannel);

	m_pChannels[uiChannel].uiTransferSize = (m_pChannels[uiChannel].uiFBHeight * m_pChannels[uiChannel].uiLinePitch * 4);

	m_pChannels[uiChannel].TransferStruct.SetVideoBuffer(NULL, m_pChannels[uiChannel].uiTransferSize);
	m_pChannels[uiChannel].TransferStruct.SetAudioBuffer(NULL, 0);

	if (m_pChannels[uiChannel].ioMode == SDI_INPUT)
	{
		if (!m_ntv2Card.AutoCirculateInitForInput(m_pChannels[uiChannel].SDIChannel, 0, NTV2_AUDIOSYSTEM_1, 0, 1, m_pChannels[uiChannel].MinFrame, m_pChannels[uiChannel].MaxFrame))
			return false;
	}
	else
	{
		if (!m_ntv2Card.AutoCirculateInitForOutput(m_pChannels[uiChannel].SDIChannel, 0, NTV2_AUDIOSYSTEM_1, 0, 1, m_pChannels[uiChannel].MinFrame, m_pChannels[uiChannel].MaxFrame))
			return false;
	}

    if (m_bUseP2P && m_pChannels[uiChannel].ioMode == SDI_OUTPUT)
    {
        // To obtain the physical bus addresses of the surfaces on teh SDI device in AutoCirculate mode, we need to loop once
        // through the ring buffer and get the address of each element.
        AUTOCIRCULATE_P2P_STRUCT p2pBuffer;
        unsigned int uiNumBuffers = (m_pChannels[uiChannel].MaxFrame - m_pChannels[uiChannel].MinFrame) + 1;

        m_pChannels[uiChannel].pVideoBusAddress  = new ULWord64[uiNumBuffers];
        m_pChannels[uiChannel].pMarkerBusAddress = new ULWord64[uiNumBuffers];

        m_ntv2Card.AutoCirculateFlush(m_pChannels[uiChannel].SDIChannel);

        for (unsigned int i = 0; i < uiNumBuffers; i++)
        {
			m_pChannels[uiChannel].TransferStruct.SetVideoBuffer((ULWord*)(&p2pBuffer), 0);
            m_pChannels[uiChannel].TransferStruct.acPeerToPeerFlags = AUTOCIRCULATE_P2P_TARGET;

			if (m_ntv2Card.AutoCirculateTransfer(m_pChannels[uiChannel].SDIChannel, m_pChannels[uiChannel].TransferStruct))
	        {
		        m_pChannels[uiChannel].pVideoBusAddress[i]  = p2pBuffer.videoBusAddress;
                m_pChannels[uiChannel].pMarkerBusAddress[i] = p2pBuffer.messageBusAddress;
	        }
        }
    }

	m_ntv2Card.AutoCirculateFlush(m_pChannels[uiChannel].SDIChannel);
        
    return true;
}


unsigned int AJA_SDIInOut::getBusAddresses(unsigned int uiChannel, unsigned long long* &outVideoBusAddress, unsigned long long* &outMarkerBusAddress)
{
    if (uiChannel >= m_uiNumChannels)
        return 0;

    if (!m_bUseP2P || m_pChannels[uiChannel].ioMode != SDI_OUTPUT)
        return 0;

   if (m_pChannels[uiChannel].pVideoBusAddress && m_pChannels[uiChannel].pMarkerBusAddress)
    {
        outVideoBusAddress  = (unsigned long long*)m_pChannels[uiChannel].pVideoBusAddress;
        outMarkerBusAddress = (unsigned long long*)m_pChannels[uiChannel].pMarkerBusAddress;

        return (m_pChannels[uiChannel].MaxFrame - m_pChannels[uiChannel].MinFrame) + 1;
    }

    return 0;
}


// Connect a SDI channel with a synchronize buffer to exchange data with
// the render thread.
void AJA_SDIInOut::setSyncBuffer(unsigned int uiChannel, SyncedBuffer* pBuffer)
{
    if (pBuffer && uiChannel < m_uiNumChannels)
        m_pChannels[uiChannel].pSyncBuffer = pBuffer;
}


// get the index with which this channel is stored. uiSDIChannel specifies the physical channel.
// uiSDIChannel 0 corresponds to NTV2_CHANNEL1 and uiSDIChannel 1 to NTV2_CHANNEL2. Depending on the 
// order that the channels are created the index changes. If  NTV2_CHANNEL2 was created first it will
// be stored at position 0.
unsigned int AJA_SDIInOut::getSDIChannelIndex(unsigned int uiSDIChannel)
{
    for (unsigned int i = 0; i < m_uiNumChannels; ++i)
    {
        if (m_pChannels[i].uiSDIChannel == uiSDIChannel)
        {
            return i;
        }
    }

    return m_uiNumChannels;
}


void AJA_SDIInOut::deleteChannels()
{
    if (m_bRunning)
        stop();

    for (unsigned int i = 0; i < m_uiNumChannels; i++)
    {
        if (m_pChannels[i].pVideoBusAddress)
            delete [] m_pChannels[i].pVideoBusAddress;

        if (m_pChannels[i].pMarkerBusAddress)
            delete [] m_pChannels[i].pMarkerBusAddress;
    }

    ZeroMemory((void*)m_pChannels, MAX_CHANNELS*sizeof(Channel));
    
    m_uiNumChannels = 0;
    m_uiNumThreads  = 0;
}


// Get threads started.
bool AJA_SDIInOut::start()
{
    m_bRunning = true;

    if (m_uiMaxAllowedBufferLevel > m_uiNumBuffers)
        m_uiMaxAllowedBufferLevel = m_uiNumBuffers;

    if (m_bUseAutoCirculate)
    {
        // Start AutoCirculate on each channel
        for (unsigned int i = 0; i < m_uiNumChannels; ++i)
        {
            if (!m_ntv2Card.AutoCirculateStart(m_pChannels[i].SDIChannel))
                return false;
        }
    }

    if (m_uiNumChannels == 1)
    {
        if (m_pChannels[0].ioMode == SDI_INPUT)
        {
            // Select the vertical event to which the input channels subscribes
            if (m_pChannels[0].SDIChannel == NTV2_CHANNEL1)
            {
                m_ntv2Card.SubscribeInputVerticalEvent(NTV2_CHANNEL1);
                m_WaitChannel = NTV2_CHANNEL1;
            }
            else if (m_pChannels[0].SDIChannel == NTV2_CHANNEL2)
            {
                m_ntv2Card.SubscribeInputVerticalEvent(NTV2_CHANNEL2);
                m_WaitChannel = NTV2_CHANNEL2;
            }

            // start thread to capture data
            //m_pThread[0]         = CreateThread(NULL, 0, AJA_SDIInOut::inputThread, this, 0, &m_pThreadId[0]);
            m_pThread[0].create((THREAD_PROC)AJA_SDIInOut::inputThread, this);

            m_uiNumThreads = 1;

            return true;
        }
        else if (m_pChannels[0].ioMode == SDI_OUTPUT)
        {
            // start thread to display data
            m_pThread[0].create((THREAD_PROC)AJA_SDIInOut::outputThread, this);

            m_uiNumThreads = 1;

            return true;
        }
    }
    else if (m_uiNumChannels == 2)
    {
        if (m_pChannels[0].ioMode == SDI_INPUT && m_pChannels[1].ioMode == SDI_INPUT)
        {
            // In case of 2 inputs, input 1 is used as reference. In this case the input needs
            // to subscrive to the vertical event of channel 1.
            m_ntv2Card.SetReference(NTV2_REFERENCE_INPUT1);
            m_ntv2Card.SubscribeInputVerticalEvent(NTV2_CHANNEL1);
            m_WaitChannel = NTV2_CHANNEL1;

            // In case of 2 inputs, we create only one thread than handles both inputs
            m_pThread[0].create((THREAD_PROC)AJA_SDIInOut::inputThread, this);


            m_uiNumThreads = 1;

            return true;
        }
        else if (m_pChannels[0].ioMode != m_pChannels[1].ioMode)
        {  
            unsigned int uiInputIdx = 0;

            if (m_pChannels[1].ioMode == SDI_INPUT)
            {
                uiInputIdx = 1;
            }

            // define vertical interrupt on which the input waits
            if (m_pChannels[uiInputIdx].SDIChannel == NTV2_CHANNEL1)
            {
                m_ntv2Card.SetReference(NTV2_REFERENCE_INPUT1);
                m_ntv2Card.SubscribeInputVerticalEvent(NTV2_CHANNEL1);
                m_WaitChannel = NTV2_CHANNEL1;
            }
            else
            {
                m_ntv2Card.SetReference(NTV2_REFERENCE_INPUT2);
                m_ntv2Card.SubscribeInputVerticalEvent(NTV2_CHANNEL2);
                m_WaitChannel = NTV2_CHANNEL2;
            }

            m_pThread[0].create((THREAD_PROC)AJA_SDIInOut::inputThread,  this);
            m_pThread[1].create((THREAD_PROC)AJA_SDIInOut::outputThread, this);

            m_uiNumThreads = 2;
           
            return true;
        }
        else if (m_pChannels[0].ioMode == SDI_OUTPUT && m_pChannels[1].ioMode == SDI_OUTPUT)
        {
            // In case of two output channels, we also use only one thread. Usually we want to start the output
            // of a new frame on each channel if we also have new frames computed for each channel. The out channels 
            // should be genlocked so it makes sense to start transfer at the same time.
            m_pThread[0].create((THREAD_PROC)AJA_SDIInOut::outputThread, this);

            m_uiNumThreads = 1;

            return true;
        }
    }

    return false;
}


void AJA_SDIInOut::halt()
{
    m_bRunning = false;
}


bool AJA_SDIInOut::stop()
{
    m_bRunning = false;

    //WaitForMultipleObjects(m_uiNumThreads, m_pThread, TRUE, INFINITE);
    for (unsigned int i = 0; i < m_uiNumThreads; ++i)
    {
        m_pThread[i].join();
    }

    if (m_bUseAutoCirculate)
    {
        for (unsigned int i = 0; i < m_uiNumChannels; i++)
        {
            m_ntv2Card.AutoCirculateStop(m_pChannels[i].SDIChannel);
        }
    }

    return true;
}


DWORD WINAPI AJA_SDIInOut::inputThread(void *pArg)
{
    if (pArg)
    {
        AJA_SDIInOut*  pInOut = (AJA_SDIInOut*)pArg;

        pInOut->inputLoop();
    }

    return 0;
}


DWORD WINAPI AJA_SDIInOut::outputThread(void *pArg)
{
    if (pArg)
    {
        AJA_SDIInOut*  pInOut = (AJA_SDIInOut*)pArg;

        pInOut->outputLoop();
    }

    return 0;
}


void AJA_SDIInOut::transferWithAutoCirculateToGPU(unsigned int uiChannel, unsigned int uiTransferId)
{
    TransferFrame*                  pFrame;
    AUTOCIRCULATE_P2P_STRUCT        p2pBuffer;
    AUTOCIRCULATE_STATUS            ActStatus;
    
	m_ntv2Card.AutoCirculateGetStatus(m_pChannels[uiChannel].SDIChannel, ActStatus);

	if (ActStatus.GetState() == NTV2_AUTOCIRCULATE_RUNNING && ActStatus.GetBufferLevel() > 0 && ActStatus.GetBufferLevel() < m_uiMaxAllowedBufferLevel)
    {
        if (m_bUseP2P)
        {
            /////////////////////////////////////////////////////////////
            // P2P transfer
            /////////////////////////////////////////////////////////////
            // Get Buffer for that contains the addresses on the GPU memory into
            // which the current frame can be transferred
            m_pChannels[uiChannel].pSyncBuffer->getBufferForWriting((void*&)pFrame);

            if (pFrame)
            {
                p2pBuffer.videoBusAddress   = pFrame->uiGfxBufferBusAddress;
                p2pBuffer.videoBusSize      = m_pChannels[uiChannel].uiTransferSize;
                p2pBuffer.messageBusAddress = pFrame->uiGfxMarkerBusAddress;
                p2pBuffer.messageData       = uiTransferId;
                p2pBuffer.p2pflags          = 0;
                p2pBuffer.p2pSize           = sizeof(p2pBuffer);

                pFrame->uiWaitMarkerValue   = uiTransferId;
            }

            // Unblock rendering with new buffer content. No need to wait until the transfer has finished
            // since in P2P mode the render thread will advise ogl to wait for tha data transfer to be finsihed
            // by calling glWaitMarkerAMD.
            m_pChannels[uiChannel].pSyncBuffer->releaseWriteBuffer();

			m_pChannels[uiChannel].TransferStruct.SetVideoBuffer((ULWord*)&p2pBuffer, m_pChannels[uiChannel].uiTransferSize);
            m_pChannels[uiChannel].TransferStruct.acPeerToPeerFlags = AUTOCIRCULATE_P2P_TRANSFER;

            // Transfer frame to gpu. This call will transfer the data and write the marker to indicate end of transfer
			m_ntv2Card.AutoCirculateTransfer(m_pChannels[uiChannel].SDIChannel, m_pChannels[uiChannel].TransferStruct);
            
            ++uiTransferId;
        }
        else
        {
            /////////////////////////////////////////////////////////////
            // 2 Step copy using pinned memory
            /////////////////////////////////////////////////////////////
            // Get DMA Buffer for current frame
            m_pChannels[uiChannel].pSyncBuffer->getBufferForWriting((void*&)pFrame);

            if (pFrame)
            {
				m_pChannels[uiChannel].TransferStruct.SetVideoBuffer((ULWord*)(pFrame->pData), m_pChannels[uiChannel].uiTransferSize);

				if (pFrame->pData)
		        {
			        // Read current frame into DMA buffer
					m_ntv2Card.AutoCirculateTransfer(m_pChannels[uiChannel].SDIChannel, m_pChannels[uiChannel].TransferStruct);
				}
			}

            // Unblock rendering with new buffer content
            m_pChannels[uiChannel].pSyncBuffer->releaseWriteBuffer();
        }
    }
    else if (ActStatus.GetState() == NTV2_AUTOCIRCULATE_RUNNING && ActStatus.GetBufferLevel() > 0)
    {
        // Max buffer level reached -> flush the buffer to catch up
		m_ntv2Card.AutoCirculateFlush(m_pChannels[uiChannel].SDIChannel);
    }
}


void AJA_SDIInOut::transferWithAutoCirculateToSDI(unsigned int uiChannel)
{
    TransferFrame*              pFrame = NULL;
    AUTOCIRCULATE_P2P_STRUCT    p2pBuffer;
	AUTOCIRCULATE_STATUS        ActStatus;

    m_ntv2Card.AutoCirculateGetStatus(m_pChannels[uiChannel].SDIChannel, ActStatus);

    if ((ActStatus.GetState() == NTV2_AUTOCIRCULATE_RUNNING || ActStatus.GetState() == NTV2_AUTOCIRCULATE_STARTING) && (ActStatus.GetBufferLevel() < (m_uiNumBuffers -1)))
    {
        if (m_bUseP2P)
        {
            /////////////////////////////////////////////////////////////
            // P2P transfer
            /////////////////////////////////////////////////////////////                   
			m_pChannels[uiChannel].TransferStruct.SetVideoBuffer((ULWord*)(&p2pBuffer), 0);
            m_pChannels[uiChannel].TransferStruct.SetAudioBuffer(NULL, 0);
            m_pChannels[uiChannel].TransferStruct.acPeerToPeerFlags = AUTOCIRCULATE_P2P_TARGET;

            // Just retreive the information on an empty buffer into which the data can be written by
            // the render thread.
			m_ntv2Card.AutoCirculateTransfer(m_pChannels[uiChannel].SDIChannel, m_pChannels[uiChannel].TransferStruct);

            // Now allow the GPU to copy into the buffer specified in p2pBuffer
            // Request frame data in whcih the addresses of the sdi buffer is written
            m_pChannels[uiChannel].pSyncBuffer->getBufferForWriting((void*&)pFrame);

            if (pFrame)
            {
                pFrame->uiSdiBufferBusAddress = p2pBuffer.videoBusAddress;
                pFrame->uiSdiMarkerBusAddress = p2pBuffer.messageBusAddress;
                pFrame->uiWriteMarkerValue    = p2pBuffer.messageData;
            }

            m_pChannels[uiChannel].pSyncBuffer->releaseWriteBuffer();   
        }
        else
        {
            /////////////////////////////////////////////////////////////
            // 2 Step copy using pinned memory
            /////////////////////////////////////////////////////////////
            m_pChannels[uiChannel].pSyncBuffer->getBufferForReading((void*&)pFrame);

            if (pFrame)
            {
				m_pChannels[uiChannel].TransferStruct.SetVideoBuffer((ULWord*)pFrame->pData, m_pChannels[uiChannel].uiTransferSize);

				if (pFrame->pData)
				{
					m_ntv2Card.AutoCirculateTransfer(m_pChannels[uiChannel].SDIChannel, m_pChannels[uiChannel].TransferStruct);
				}
            }

            m_pChannels[uiChannel].pSyncBuffer->releaseReadBuffer();
        }
    }
}


void AJA_SDIInOut::transferToGPU(unsigned int uiChannel, unsigned int uiFrame, unsigned int uiTransferId)
{
    TransferFrame*                  pFrame;

    unsigned int uiFrameSize = (m_pChannels[uiChannel].uiFBHeight * m_pChannels[uiChannel].uiLinePitch*4);

    if (m_bUseP2P)
    {
        /////////////////////////////////////////////////////////////
        // P2P transfer
        /////////////////////////////////////////////////////////////
        CHANNEL_P2P_STRUCT p2pData;
        

        // Get Buffer for that contains the addresses on the GPU memory into
        // which the current frame can be transferred
        m_pChannels[uiChannel].pSyncBuffer->getBufferForWriting((void*&)pFrame);

        if (pFrame)
        {
            p2pData.videoBusAddress   = pFrame->uiGfxBufferBusAddress;
            p2pData.videoBusSize      = uiFrameSize;
            p2pData.messageBusAddress = pFrame->uiGfxMarkerBusAddress;
            p2pData.messageData       = uiTransferId;
            p2pData.p2pflags          = 0;
            p2pData.p2pSize           = sizeof(p2pData);
            pFrame->uiWaitMarkerValue   = uiTransferId; 
        }
       
        // Unblock rendering with new buffer content. No need to wait until the transfer has finished
        // since in P2P mode the render thread will advise ogl to wait for tha data transfer to be finsihed
        // by calling glWaitMarkerAMD.
        m_pChannels[uiChannel].pSyncBuffer->releaseWriteBuffer();

        // Start transfer frame and write marker to indicate that the transfer has completed
        m_ntv2Card.DmaP2PTransferFrame(NTV2_DMA_FIRST_AVAILABLE, m_pChannels[uiChannel].MinFrame + uiFrame, 0, uiFrameSize, 0, 0, 0, &p2pData);
    }   
    else
    {
        /////////////////////////////////////////////////////////////
        // 2 Step copy using pinned memory
        /////////////////////////////////////////////////////////////
        // Get DMA Buffer for current frame
        m_pChannels[uiChannel].pSyncBuffer->getBufferForWriting((void*&)pFrame);

        if (pFrame)
        {
            m_ntv2Card.DMAReadFrame(m_pChannels[uiChannel].MinFrame + uiFrame, (ULWord*)pFrame->pData, uiFrameSize);
        }

        // Unblock rendering with new buffer content
        m_pChannels[uiChannel].pSyncBuffer->releaseWriteBuffer();
    }

    m_ntv2Card.SetInputFrame(m_pChannels[uiChannel].SDIChannel, m_pChannels[uiChannel].MinFrame + uiFrame);
}


void AJA_SDIInOut::transferToSDI(unsigned int uiChannel, unsigned int uiFrame)
{
    TransferFrame*                  pFrame;
    
    unsigned int uiFrameSize = (m_pChannels[uiChannel].uiFBHeight * m_pChannels[uiChannel].uiLinePitch*4);

    if (m_bUseP2P)
    {
        /////////////////////////////////////////////////////////////
        // P2P transfer
        /////////////////////////////////////////////////////////////

        // Retreive the address of an empty buffer on sdi. The information is passed as
        // frame data to the render thread that will transfer the data into this buffer.
        m_pChannels[uiChannel].pSyncBuffer->getBufferForWriting((void*&)pFrame);

        if (pFrame)
        {
            pFrame->uiSdiBufferBusAddress = m_pChannels[uiChannel].pVideoBusAddress[uiFrame];
            pFrame->uiSdiMarkerBusAddress = m_pChannels[uiChannel].pMarkerBusAddress[uiFrame];
            pFrame->uiWriteMarkerValue    = m_pChannels[uiChannel].MinFrame + uiFrame;
        }

        m_pChannels[uiChannel].pSyncBuffer->releaseWriteBuffer();
    }
    else
    {
        /////////////////////////////////////////////////////////////
        // 2 Step copy using pinned memory
        /////////////////////////////////////////////////////////////
        m_pChannels[uiChannel].pSyncBuffer->getBufferForReading((void*&)pFrame);

        if (pFrame)
        {
            m_ntv2Card.DMAWriteFrame(m_pChannels[uiChannel].MinFrame + uiFrame, (ULWord*)pFrame->pData, uiFrameSize);
            m_ntv2Card.SetOutputFrame(m_pChannels[uiChannel].SDIChannel, m_pChannels[uiChannel].MinFrame + uiFrame);
        }

        m_pChannels[uiChannel].pSyncBuffer->releaseReadBuffer();
    }
}


// input loop will run continuously and capture data on the input channel.
// Data is written into associated sync buffer.
void AJA_SDIInOut::inputLoop()
{
   unsigned int    uiTransferId  = 0;
   unsigned int    uiFrameNumber = 0;

    while (m_bRunning)
    { 
        for (unsigned int i = 0; i < m_uiNumChannels; ++i)
        {
            if (m_pChannels[i].ioMode == SDI_INPUT && m_pChannels[i].pSyncBuffer)
            {
                if (m_bUseAutoCirculate)
                {
                    transferWithAutoCirculateToGPU(i, uiTransferId);
                }
                else
                {
                    transferToGPU(i, uiFrameNumber, uiTransferId);
                }

                ++uiTransferId;
            }
        }

        ++uiFrameNumber;

        if (uiFrameNumber >= m_uiNumBuffers)
        {
            uiFrameNumber = 0;
        }

		m_ntv2Card.WaitForInputFieldID (NTV2_FIELD0, m_WaitChannel);
    } 
}


// Output loop will run continuously and copy data to the SDI framebuffer for output
void AJA_SDIInOut::outputLoop()
{
    m_ntv2Card.SubscribeOutputVerticalEvent (NTV2_CHANNEL1);

    unsigned int uiFrameNumber = 0;
   
    while (m_bRunning)
    {
        for (unsigned int i = 0; i < m_uiNumChannels; ++i)
        {
            if (m_pChannels[i].ioMode == SDI_OUTPUT && m_pChannels[i].pSyncBuffer)
            {
                if (m_bUseAutoCirculate)
                {
                    transferWithAutoCirculateToSDI(i);
                }
                else
                {
                    transferToSDI(i, uiFrameNumber);
                }
            }
        }

        ++uiFrameNumber;

        if (uiFrameNumber >= m_uiNumBuffers)
        {
            uiFrameNumber = 0;
        }
        
        m_ntv2Card.WaitForOutputVerticalInterrupt();
    }
}



unsigned int AJA_SDIInOut::getBytesPerPixel(unsigned int uiChannel)
{
    int nBPP = 0;

    nBPP = (m_pChannels[uiChannel].uiLinePitch * 4) / m_pChannels[uiChannel].uiFBWidth;
   
    return nBPP;
}


int AJA_SDIInOut::getExtFormat(unsigned int uiChannel)
{
   int nNumFormats = sizeof(AJA_FormatTable)/sizeof(AJA_FormatTable[0]);

   for (int i = 0; i < nNumFormats; ++i)
   {
       if (AJA_FormatTable[i].AJA_Format == m_pChannels[uiChannel].FBFormat)
           return AJA_FormatTable[i].GL_ExtFormat;
   }

    return 0;
}


int AJA_SDIInOut::getIntFormat(unsigned int uiChannel)
{
   int nNumFormats = sizeof(AJA_FormatTable)/sizeof(AJA_FormatTable[0]);

   for (int i = 0; i < nNumFormats; ++i)
   {
       if (AJA_FormatTable[i].AJA_Format == m_pChannels[uiChannel].FBFormat)
           return AJA_FormatTable[i].GL_IntFormat;
   }

    return 0;
}


int AJA_SDIInOut::getType(unsigned int uiChannel)
{
    int nNumFormats = sizeof(AJA_FormatTable)/sizeof(AJA_FormatTable[0]);

    for (int i = 0; i < nNumFormats; ++i)
    {
        if (AJA_FormatTable[i].AJA_Format == m_pChannels[uiChannel].FBFormat)
           return AJA_FormatTable[i].GL_Type;
    }

    return 0;
}


bool AJA_SDIInOut::isRGBFormat(NTV2FrameBufferFormat format)
{
	switch (format)
	{
	case NTV2_FBF_ARGB:
	case NTV2_FBF_RGBA:
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_ABGR:
	case NTV2_FBF_10BIT_DPX:
	case NTV2_FBF_24BIT_RGB:
	case NTV2_FBF_24BIT_BGR:
    case NTV2_FBF_10BIT_DPX_LE:
	case NTV2_FBF_48BIT_RGB:
	case NTV2_FBF_10BIT_RGB_PACKED:
	case NTV2_FBF_10BIT_ARGB:
	case NTV2_FBF_16BIT_ARGB:
		return true;
	default:
		return false;
	}
}
