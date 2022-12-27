/* SPDX-License-Identifier: MIT */
#include "simplegpuvio.h"
#include "ntv2signalrouter.h"
#include "ntv2utils.h"
#include "ajabase/system/systemtime.h"

static int s_iIndexFirstSource = 0;									// source board first frame buffer index
static int s_iIndexLastSource = 1;									// source board last frame buffer index (frame mode)
static int s_iIndexFirstTarget = 20;								// target board first frame buffer index
static int s_iIndexLastTarget = 21;									// target board last frame buffer index (frame mode)
static int s_iFrameDelay = 0;	//2									// input/output delay (frame mode)

static bool s_bSubFrame = false;									// do subframe transfers (low latency)
static int s_iSubFrameCount = 10;									// number of subframes per frame


CGpuVideoIO::CGpuVideoIO() :
	mBoard(NULL),
	mGPUTransfer(NULL),
	mGPUCircularBuffer(NULL)

{
}

CGpuVideoIO::CGpuVideoIO(vioDesc *desc) :
	mBoard(NULL),
	mGPUTransfer(NULL),
	mGPUCircularBuffer(NULL)
{
	//  Open first device that supports input and output
	CNTV2DeviceScanner * pNTV2DeviceScanner = new CNTV2DeviceScanner();
	if (desc->deviceIndex >= pNTV2DeviceScanner->GetNumDevices())
	{
		return ;
	}

	mBoard = new CNTV2Card((UWord)desc->deviceIndex);

	// Return if no board compatible board found or opened
	if (!mBoard)
		return;

	// Cache channel
	mChannel = desc->channel;

	// Set video format
	mBoard->SetVideoFormat(desc->videoFormat, false, false, mChannel);
	mBoard->SetQuadFrameEnable(0, mChannel);

	// Get source video info
	NTV2VANCMode vancMode;
	mBoard->GetVANCMode(vancMode);
	mActiveVideoSize = GetVideoActiveSize(desc->videoFormat, desc->bufferFormat, vancMode);
	mActiveVideoHeight = GetDisplayHeight(desc->videoFormat);
	mActiveVideoPitch = mActiveVideoSize / mActiveVideoHeight;
	mTransferLines = mActiveVideoHeight / s_iSubFrameCount;
	mTransferSize = mActiveVideoPitch * mTransferLines;

	if (desc->type == VIO_IN)
	{
		// Genlock the output to the input
		mBoard->SetReference(::NTV2InputSourceToReferenceSource(::NTV2ChannelToInputSource(mChannel)));

		// Set frame buffer format
		mBoard->SetFrameBufferFormat(mChannel, desc->bufferFormat);
	
		// Put source channel in capture mode
		mBoard->SetMode(mChannel, NTV2_MODE_CAPTURE);
		mBoard->SetSDITransmitEnable(mChannel, false);

		// Connect SDI source to frame store
		mBoard->Connect(::GetCSCInputXptFromChannel(mChannel), ::GetInputSourceOutputXpt(::NTV2ChannelToInputSource(mChannel)));
		mBoard->Connect(::GetFrameBufferInputXptFromChannel(mChannel), ::GetCSCOutputXptFromChannel(mChannel, false, true));

		// Get video standard
		NTV2Standard videoStandard;
		mBoard->GetStandard(videoStandard);

		// Allocate the source video buffer
		mpVidBufferSource = (ULWord*)new char[mActiveVideoSize];
		memset(mpVidBufferSource, 0, mActiveVideoSize);

		// Set register update mode to frame
		mBoard->SetRegisterWriteMode(NTV2_REGWRITE_SYNCTOFRAME);

		// Set source to capture first frame
		mFrameNumber = s_iIndexFirstSource;

		if (!s_bSubFrame)
		{
			// Subscribe to video input interrupts
			mBoard->SubscribeInputVerticalEvent(mChannel);

			// Set the input to output delay for full frame transfers
			mFrameNumber += s_iFrameDelay;
		}

		mBoard->SetInputFrame(mChannel, mFrameNumber);
	}
	else
	{
		// Set frame buffer format
		mBoard->SetFrameBufferFormat(mChannel, desc->bufferFormat);

		mBoard->SetSDITransmitEnable(mChannel, true);

		// Put target in display mode
		mBoard->SetMode(mChannel, NTV2_MODE_DISPLAY);

		// Connect frame store to SDI output
		mBoard->Connect(::GetCSCInputXptFromChannel(mChannel), ::GetFrameBufferOutputXptFromChannel(mChannel,  true,  false));
		mBoard->Connect(::GetSDIOutputInputXpt (mChannel, false), ::GetCSCOutputXptFromChannel(mChannel,  false,  false));

		// Setup the frame buffer parameters
		mFrameNumber = s_iIndexFirstTarget;
		
		// Set register update mode to frame
		mBoard->SetRegisterWriteMode(NTV2_REGWRITE_SYNCTOFRAME);

		// Set target to output first frame
		mBoard->SetOutputFrame(mChannel, mFrameNumber);
	}
}

CGpuVideoIO::~CGpuVideoIO()
{
	mBoard->DMABufferUnlockAll();
	mBoard->Close();
}

void
CGpuVideoIO::WaitForCaptureStart()
{
	NTV2Mode mode;
	mBoard->GetMode(mChannel, mode);
	if (mode == NTV2_MODE_CAPTURE)
	{
		// Wait for capture to start
		mBoard->WaitForInputFieldID(NTV2_FIELD0, mChannel);
	}
	else
	{
		// Return an error in this case?
	}
}

void 
CGpuVideoIO::SetGpuTransfer(CNTV2gpuTextureTransfer* transfer)
{
	mGPUTransfer = transfer;
}

CNTV2gpuTextureTransfer*
CGpuVideoIO::GetGpuTransfer()
{
	return mGPUTransfer;
}

void 
CGpuVideoIO::SetGpuCircularBuffer(CNTV2GpuCircularBuffer* gpuCircularBuffer)
{
	mGPUCircularBuffer = gpuCircularBuffer;

	if (mBoard == NULL)
	{
		printf("error: no board to use for lock\n");
		return;
	}
	
	for (ULWord i = 0; i < gpuCircularBuffer->mNumFrames; i++)
	{
#ifdef AJA_RDMA		
		if (!mBoard->DMABufferLock((ULWord*)gpuCircularBuffer->mAVTextureBuffers[i].videoBufferRDMA,
								   gpuCircularBuffer->mAVTextureBuffers[i].videoBufferSize,
								   true, true))
		{
			printf("error: RDMA buffer index %d size %d lock failed\n",
				   i, gpuCircularBuffer->mAVTextureBuffers[i].videoBufferSize);
			return;
		}		
#else		
		if (!mBoard->DMABufferLock((ULWord*)gpuCircularBuffer->mAVTextureBuffers[i].videoBuffer,
								   gpuCircularBuffer->mAVTextureBuffers[i].videoBufferSize,
								   true, false))
		{
			printf("error: system buffer lock failed\n");
			return;
		}		
#endif		
	}
}

CNTV2GpuCircularBuffer* 
CGpuVideoIO::GetGpuCircularBuffer()
{
	return mGPUCircularBuffer;
}

uint64_t lastTime = 0;
bool 
CGpuVideoIO::Capture()
{
	// Wait for frame interrupt
	mBoard->WaitForInputFieldID(NTV2_FIELD0, mChannel);
	
	ULWord loval, hival;
	mBoard->ReadRegister(kVRegTimeStampLastInput1VerticalLo, loval);
	mBoard->ReadRegister(kVRegTimeStampLastInput1VerticalHi, hival);

	// Get pointer to next GPU buffer
	AVTextureBuffer* frameData = mGPUCircularBuffer->StartProduceNextBuffer();
#ifdef AJA_RDMA
	uint8_t* buffer = (uint8_t*)frameData->videoBufferRDMA;
#else
	uint8_t* buffer = (uint8_t*)frameData->videoBuffer;
#endif
	frameData->currentTime = ((uint64_t)hival << 32) + loval;
//	odprintf("Interrupt Perioe %llu", frameData->currentTime - lastTime);
	lastTime = frameData->currentTime;

	//Overlap the Dma with the GPU DMAs by using multiple chunks.
	//make sure the chunks have the same size 
	uint32_t copiedSize = 0;
	uint32_t copiedChunkSize = 0;
	uint32_t numChunks = mGPUTransfer->GetNumChunks();
	uint32_t chunkSize = (uint32_t)((float)frameData->videoBufferSize/(float)numChunks);
    for (uint32_t i = 0; i < numChunks; i++)
	{
		copiedChunkSize = (frameData->videoBufferSize-copiedSize > chunkSize ? chunkSize : frameData->videoBufferSize-copiedSize);	
		// Prepare for DMA transfer
		mGPUTransfer->BeforeRecordTransfer(buffer, frameData->texture, frameData->renderToTexture);
		// DMA source frame to system memory
		if(!mBoard->DMAReadSegments(mFrameNumber, 
			(ULWord*)(buffer + copiedSize), copiedSize, copiedChunkSize, 1, copiedChunkSize, copiedChunkSize))
		{
			printf("error: DMAReadSegment failed\n");
			return false;
		}		
		// Signal that DMA transfer is complete and kickoff GPU transfers
		mGPUTransfer->AfterRecordTransfer(buffer, frameData->texture, frameData->renderToTexture);
		copiedSize += copiedChunkSize;
	}

	// Signal done with this buffer.
	mGPUCircularBuffer->EndProduceNextBuffer();

	// Update source frame
	mFrameNumber++;
	if(mFrameNumber > (ULWord)s_iIndexLastSource)
	{
		mFrameNumber = s_iIndexFirstSource;
	}

	// Set source frame
	mBoard->SetInputFrame(mChannel, mFrameNumber);

	return true;
}

bool 
CGpuVideoIO::Playout()
{
	// Get next GPU buffer
	AVTextureBuffer* frameData = mGPUCircularBuffer->StartConsumeNextBuffer();
#ifdef AJA_RDMA
	uint8_t* buffer = (uint8_t*)frameData->videoBufferRDMA;
#else
	uint8_t* buffer = (uint8_t*)frameData->videoBuffer;
#endif

	//Overlap the Dma with the GPU DMAs by using multiple chunks.
	//make sure the chunks have the same size 
	uint32_t copiedSize = 0;
	uint32_t copiedChunkSize = 0;
	uint32_t numChunks = mGPUTransfer->GetNumChunks();
	uint32_t chunkSize = (uint32_t)((float)frameData->videoBufferSize/(float)numChunks);
    for (uint32_t i = 0; i < numChunks; i++)
	{
		copiedChunkSize = (frameData->videoBufferSize-copiedSize > chunkSize ? chunkSize : frameData->videoBufferSize-copiedSize);
		// Kickoff DMA from GPU and prepare for DMA from system memory to video I/O device	
		mGPUTransfer->BeforePlaybackTransfer(buffer, frameData->texture, frameData->renderToTexture);
		if(!mBoard->DMAWriteSegments(mFrameNumber, 
			(ULWord*)(buffer + copiedSize), copiedSize, copiedChunkSize, 1, copiedChunkSize, copiedChunkSize))
		{
			printf("error: DMAWriteSegment failed\n");
			return false;
		}				
		// Signal DMA to video I/O device is complete
		mGPUTransfer->AfterPlaybackTransfer(buffer, frameData->texture, frameData->renderToTexture);
		copiedSize += copiedChunkSize;
	}		
		// Signal done with this buffer.
	mGPUCircularBuffer->EndConsumeNextBuffer();

	ULWord loval, hival;
	mBoard->ReadRegister(kVRegTimeStampLastInput1VerticalLo, loval);
	mBoard->ReadRegister(kVRegTimeStampLastInput1VerticalHi, hival);
//	uint64_t currentTime = ((uint64_t)hival << 32) + loval;
//	odprintf("Latency Time %llu ", currentTime - frameData->currentTime);

	// Set target frame
	mBoard->SetOutputFrame(mChannel, mFrameNumber);

	// Update target frame
	mFrameNumber++;
	if(mFrameNumber > (ULWord)s_iIndexLastTarget)
	{
		mFrameNumber = s_iIndexFirstTarget;
	}

	return true;
}
