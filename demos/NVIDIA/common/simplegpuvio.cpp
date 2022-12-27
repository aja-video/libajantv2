/* SPDX-License-Identifier: MIT */
#include "simplegpuvio.h"
#include "ntv2signalrouter.h"
#include "ajabase/system/systemtime.h"
#include "ntv2vpid.h"

static int s_iIndexFirstSource = 0;									// source board first frame buffer index
static int s_iIndexLastSource = 1;									// source board last frame buffer index (frame mode)
static int s_iIndexFirstTarget = 20;									// target board first frame buffer index
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
	unsigned int   m_uiBoardNumber = 0;
	if (m_uiBoardNumber >= pNTV2DeviceScanner->GetNumDevices())
	{
		return ;
	}

	mBoard = new CNTV2Card((UWord)m_uiBoardNumber);

	// Return if no board compatible board found or opened
	if (!mBoard)
		return;

	// Set video format
	mBoard->SetVideoFormat(desc->videoFormat, false, false, desc->channel);


	// Genlock the output to the input
	mBoard->SetReference(NTV2_REFERENCE_INPUT1);

	// Cache channel
	mChannel = desc->channel;

	// Get source video info
	NTV2VANCMode vancMode;
	mBoard->GetVANCMode(vancMode);
	mActiveVideoSize = GetVideoActiveSize(desc->videoFormat, desc->bufferFormat, vancMode);
	mActiveVideoHeight = GetDisplayHeight(desc->videoFormat);
	mActiveVideoPitch = mActiveVideoSize / mActiveVideoHeight;
	mTransferLines = mActiveVideoHeight / s_iSubFrameCount;
	mTransferSize = mActiveVideoPitch * mTransferLines;


	if (desc->type == VIO_IN) {



		if (NTV2_IS_QUAD_FRAME_FORMAT(desc->videoFormat))
		{
			// Set frame buffer format
			mBoard->SetFrameBufferFormat(mChannel, desc->bufferFormat);
			mBoard->SetFrameBufferFormat((NTV2Channel)(mChannel + 1), desc->bufferFormat);
			mBoard->SetFrameBufferFormat((NTV2Channel)(mChannel + 2), desc->bufferFormat);
			mBoard->SetFrameBufferFormat((NTV2Channel)(mChannel + 3), desc->bufferFormat);

			mBoard->SetMode(mChannel, NTV2_MODE_CAPTURE);
			mBoard->SetMode((NTV2Channel)(mChannel + 1), NTV2_MODE_CAPTURE);
			mBoard->SetMode((NTV2Channel)(mChannel + 2), NTV2_MODE_CAPTURE);
			mBoard->SetMode((NTV2Channel)(mChannel + 3), NTV2_MODE_CAPTURE);

			// Input specific setup
			mBoard->SetSDITransmitEnable(mChannel, false);
			mBoard->SetSDITransmitEnable((NTV2Channel)(mChannel + 1), false);
			mBoard->SetSDITransmitEnable((NTV2Channel)(mChannel + 2), false);
			mBoard->SetSDITransmitEnable((NTV2Channel)(mChannel + 3), false);

			mBoard->Connect (NTV2_XptCSC1VidInput,		NTV2_XptSDIIn1);
			mBoard->Connect (NTV2_XptFrameBuffer1Input,	NTV2_XptCSC1VidRGB);
			mBoard->Connect (NTV2_XptCSC2VidInput,		NTV2_XptSDIIn2);
			mBoard->Connect (NTV2_XptFrameBuffer2Input,	NTV2_XptCSC2VidRGB);
			mBoard->Connect (NTV2_XptCSC3VidInput,		NTV2_XptSDIIn3);
			mBoard->Connect (NTV2_XptFrameBuffer3Input,	NTV2_XptCSC3VidRGB);
			mBoard->Connect (NTV2_XptCSC4VidInput,		NTV2_XptSDIIn4);
			mBoard->Connect (NTV2_XptFrameBuffer4Input,	NTV2_XptCSC4VidRGB);

			mBoard->SetQuadFrameEnable(1, mChannel);
		}
		else
		{
			// Set frame buffer format
			mBoard->SetFrameBufferFormat(mChannel, desc->bufferFormat);

			// Put source channel in capture mode
			mBoard->SetMode(mChannel, NTV2_MODE_CAPTURE);
			mBoard->SetSDITransmitEnable(mChannel, false);

			mBoard->Connect (NTV2_XptCSC1VidInput,		NTV2_XptSDIIn1);
			mBoard->Connect (NTV2_XptFrameBuffer1Input,	NTV2_XptCSC1VidRGB);

			mBoard->SetQuadFrameEnable(0, mChannel);
		}
	


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
			mBoard->SubscribeInputVerticalEvent(NTV2_CHANNEL1);

			// Set the input to output delay for full frame transfers
			mFrameNumber += s_iFrameDelay;
		}

		mBoard->SetInputFrame(mChannel, mFrameNumber);

	} else {


		// Set video format
		mBoard->SetVideoFormat(desc->videoFormat, false, false, desc->channel);


		if (NTV2_IS_QUAD_FRAME_FORMAT(desc->videoFormat))
		{
			mBoard->SetFrameBufferFormat(mChannel, desc->bufferFormat);
			mBoard->SetFrameBufferFormat((NTV2Channel)(mChannel + 1), desc->bufferFormat);
			mBoard->SetFrameBufferFormat((NTV2Channel)(mChannel + 2), desc->bufferFormat);
			mBoard->SetFrameBufferFormat((NTV2Channel)(mChannel + 3), desc->bufferFormat);

			mBoard->SetMode(mChannel, NTV2_MODE_DISPLAY);
			mBoard->SetMode((NTV2Channel)(mChannel + 1), NTV2_MODE_DISPLAY);
			mBoard->SetMode((NTV2Channel)(mChannel + 2), NTV2_MODE_DISPLAY);
			mBoard->SetMode((NTV2Channel)(mChannel + 3), NTV2_MODE_DISPLAY);

			// Input specific setup
			mBoard->SetSDITransmitEnable(mChannel, true);
			mBoard->SetSDITransmitEnable((NTV2Channel)(mChannel + 1), true);
			mBoard->SetSDITransmitEnable((NTV2Channel)(mChannel + 2), true);
			mBoard->SetSDITransmitEnable((NTV2Channel)(mChannel + 3), true);

			mBoard->Connect (NTV2_XptCSC5VidInput, NTV2_XptFrameBuffer5RGB);
			mBoard->Connect (NTV2_XptSDIOut5Input, NTV2_XptCSC5VidYUV);
			mBoard->Connect (NTV2_XptCSC6VidInput, NTV2_XptFrameBuffer6RGB);
			mBoard->Connect (NTV2_XptSDIOut6Input, NTV2_XptCSC6VidYUV);
			mBoard->Connect (NTV2_XptCSC7VidInput, NTV2_XptFrameBuffer7RGB);
			mBoard->Connect (NTV2_XptSDIOut7Input, NTV2_XptCSC7VidYUV);
			mBoard->Connect (NTV2_XptCSC8VidInput, NTV2_XptFrameBuffer8RGB);
			mBoard->Connect (NTV2_XptSDIOut8Input, NTV2_XptCSC8VidYUV);

			mBoard->SetQuadFrameEnable(1, mChannel);
		}
		else
		{
			// Set frame buffer format
			mBoard->SetFrameBufferFormat(mChannel, desc->bufferFormat);

			mBoard->SetSDITransmitEnable(mChannel, true);

			// Put target in display mode
			mBoard->SetMode(mChannel, NTV2_MODE_DISPLAY);

			// Route video out Output 3 via FrameStore 3
			mBoard->Connect (NTV2_XptCSC3VidInput, NTV2_XptFrameBuffer3RGB);
			mBoard->Connect (NTV2_XptSDIOut3Input, NTV2_XptCSC3VidYUV);

			mBoard->SetQuadFrameEnable(0, mChannel);
		}

		// Setup the frame buffer parameters
		mFrameNumber = s_iIndexFirstTarget;

		
		// NOTE: only support progressive 16:9 formats for now'
		//////NOTE::::need to update for QuadHD or 4K.
		CNTV2VPID vpid;
		vpid.SetVPID(desc->videoFormat, desc->bufferFormat, true, true, VPIDChannel_3);
		ULWord vpidValue = vpid.GetVPID();
		mBoard->SetSDIOutVPID(vpidValue, vpidValue, NTV2_CHANNEL3); 

		// Set register update mode to frame
		mBoard->SetRegisterWriteMode(NTV2_REGWRITE_SYNCTOFRAME);

		// Set target to output first frame
		mBoard->SetOutputFrame(mChannel, mFrameNumber);
	}

}

CGpuVideoIO::~CGpuVideoIO()
{
	mBoard->Close();
}

void
CGpuVideoIO::WaitForCaptureStart()
{
	NTV2Mode mode;
	mBoard->GetMode(mChannel, mode);
	if (mode == NTV2_MODE_CAPTURE) {

		// Wait for capture to start
		//mBoard->WaitForInput1FieldID(NTV2_FIELD0);
		mBoard->WaitForInputVerticalInterrupt(NTV2_CHANNEL1);

	} else {

		// Return an error in this case?
	}
}

void 
CGpuVideoIO::SetGpuTransfer(CNTV2glTextureTransfer* transfer)
{
	mGPUTransfer = transfer;
}

CNTV2glTextureTransfer*
CGpuVideoIO::GetGpuTransfer()
{
	return mGPUTransfer;
}

void 
CGpuVideoIO::SetGpuCircularBuffer(CNTV2GpuCircularBuffer* gpuCircularBuffer)
{
	mGPUCircularBuffer = gpuCircularBuffer;
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
	mBoard->WaitForInputFieldID(NTV2_FIELD0, NTV2_CHANNEL1);
	ULWord loval, hival;
	mBoard->ReadRegister(kVRegTimeStampLastInput1VerticalLo, loval);
	mBoard->ReadRegister(kVRegTimeStampLastInput1VerticalHi, hival);

	// Get pointer to next GPU buffer
	AVTextureBuffer* frameData = mGPUCircularBuffer->StartProduceNextBuffer();
	uint8_t* buffer = (uint8_t*)frameData->videoBuffer;
	frameData->currentTime = ((uint64_t)hival << 32) + loval;
//	odprintf("Interrupt Perioe %llu", frameData->currentTime - lastTime);
	lastTime = frameData->currentTime;

	//Overlap the Dma with the GPU DMAs by using multiple chunks.
	//make sure the chunks have the same size 
	uint32_t copiedSize = 0;
	uint32_t copiedChunkSize = 0;
	uint32_t numChunks = mGPUTransfer->GetNumChunks();
	uint32_t chunkSize = (uint32_t)((float)frameData->videoBufferSize/(float)numChunks);
    for (uint32_t i = 0; i < numChunks; i++) {
		copiedChunkSize = (frameData->videoBufferSize-copiedSize > chunkSize ? chunkSize : frameData->videoBufferSize-copiedSize);	
		// Prepare for DMA transfer
		mGPUTransfer->BeforeRecordTransfer(buffer, frameData->texture, frameData->renderToTexture);
		// DMA source frame to system memory
		if(!mBoard->DMAReadSegments(mFrameNumber, 
			(ULWord*)(buffer + copiedSize), copiedSize, copiedChunkSize, 1, copiedChunkSize, copiedChunkSize)){
			printf("error: DMAReadSegment failed\n");
			return false;
		}		
		// Signal that DMA transfer is complete and kickoff GPU transfers
		mGPUTransfer->AfterRecordTransfer(buffer, frameData->texture, frameData->renderToTexture);
		copiedSize += copiedChunkSize;
	}

	// Signal done with this buffer.
	mGPUCircularBuffer->EndProduceNextBuffer();

	// Set source frame
	mBoard->SetInputFrame(mChannel, mFrameNumber);

	// Update source frame
	mFrameNumber++;
	if(mFrameNumber > (ULWord)s_iIndexLastSource) {
		mFrameNumber = s_iIndexFirstSource;
	}

	return true;
}

bool 
CGpuVideoIO::Playout()
{

	// Get next GPU buffer
	AVTextureBuffer* frameData = mGPUCircularBuffer->StartConsumeNextBuffer();
	uint8_t* buffer = (uint8_t*)(frameData->videoBuffer);

	//Overlap the Dma with the GPU DMAs by using multiple chunks.
	//make sure the chunks have the same size 
	uint32_t copiedSize = 0;
	uint32_t copiedChunkSize = 0;
	uint32_t numChunks = mGPUTransfer->GetNumChunks();
	uint32_t chunkSize = (uint32_t)((float)frameData->videoBufferSize/(float)numChunks);
    for (uint32_t i = 0; i < numChunks; i++) {
		copiedChunkSize = (frameData->videoBufferSize-copiedSize > chunkSize ? chunkSize : frameData->videoBufferSize-copiedSize);
		// Kickoff DMA from GPU and prepare for DMA from system memory to video I/O device	
		mGPUTransfer->BeforePlaybackTransfer(buffer, frameData->texture, frameData->renderToTexture);
		if(!mBoard->DMAWriteSegments(mFrameNumber, 
			(ULWord*)(buffer + copiedSize), copiedSize, copiedChunkSize, 1, copiedChunkSize, copiedChunkSize)){
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
	if(mFrameNumber > (ULWord)s_iIndexLastTarget) {
		mFrameNumber = s_iIndexFirstTarget;
	}

	return true;
}
