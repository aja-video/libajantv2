/* SPDX-License-Identifier: MIT */
/**
	@file		crossplatform/testp2p/testp2p.cpp
	@brief		Implements 'testp2p' command-line tool for testing NTV2 P2P DMA transfer via AutoCirculate.
	@copyright	(C) 2012-2022 AJA Video Systems, Inc.
**/

#include <stdio.h>
#include <string>
#include <signal.h>

#include "ntv2card.h"
#include "ntv2devicescanner.h"
#include "ntv2utils.h"
#include "ntv2debug.h"
#include "ntv2devicefeatures.h"


using namespace std;

#ifdef MSWindows
#pragma warning(disable : 4996)
#endif

#ifdef AJALinux
//#define MAX_PATH 1024
#include "ntv2linuxpublicinterface.h"
#endif

//static NTV2VideoFormat s_eVideoFormat = NTV2_FORMAT_720p_5994; 				// board video format
//static NTV2VideoFormat s_eVideoFormat = NTV2_FORMAT_1080p_2398;			// board video format
static NTV2VideoFormat s_eVideoFormat = NTV2_FORMAT_4x1920x1080p_3000;	// board video format
static NTV2FrameBufferFormat s_eFrameFormat = NTV2_FBF_10BIT_YCBCR;			// frame buffer pixel format
//static NTV2FrameBufferFormat s_eFrameFormat = NTV2_FBF_10BIT_RGB;			// frame buffer pixel format
//static NTV2FrameBufferFormat s_eFrameFormat = NTV2_FBF_RGBA;				// frame buffer pixel format

static int s_iBoardSource = 0;				// source board index
static int s_iBoardTarget = 1;				// target board index
static int s_iIndexFirstSource = 0;			// source board first autocirculate frame buffer index
static int s_iIndexLastSource = 3;			// source board last autocirculate frame buffer index
static int s_iIndexFirstTarget = 0;			// target board first autocirculate frame buffer index
static int s_iIndexLastTarget = 3;			// target board last autocirculate frame buffer index

// choose one of the modes
static bool s_bSystem = true;				// transfer using a autocirculate system buffer
static bool s_bPrepare = false;				// transfer using p2p autocirculate prepare/complete
static bool s_bTarget = false;				// transfer using p2p autocirculate target/message
static bool s_bDirect = false;				// transfer using p2p direct dma (no autocirculate)

// audio only works with system and prepare modes
static bool s_bAudio = true;				// transfer audio too
static int s_iAudioChannels = 8;			// Number of audio channels

static int s_iTestCount = 0;				// number of frames to transfer (0 = infinite)

static bool s_bLFNeeded = false;			// true to complete formatting at the end of a test

// video and crosspoint source
static NTV2Channel s_eChannelSource = NTV2_CHANNEL1;

// video and crosspoint target
static NTV2Channel s_eChannelTarget = NTV2_CHANNEL1;

// autocirculate source data structures
static AUTOCIRCULATE_TRANSFER s_TransferSource;
static AUTOCIRCULATE_STATUS s_AutoCirculateStatusSource;

// autocirculate target data structures
static AUTOCIRCULATE_TRANSFER s_TransferTarget;
static AUTOCIRCULATE_STATUS s_AutoCirculateStatusTarget;

// handle ctrl-c
void SignalHandler(int signal)
{
    (void)signal;

	s_iTestCount = (-1);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

	try
	{
		// trap ctrl-c
		signal(SIGINT, SignalHandler);

		string frameFormat = NTV2FrameBufferFormatString(s_eFrameFormat);
		string videoFormat = NTV2VideoFormatToString(s_eVideoFormat);

		printf("\nTestP2P - Peer to peer video transfer test\n");
		printf("Video transfer: %s\n", 
			(s_bDirect? "p2p direct" : 
			(s_bSystem? "system buffer (no p2p)" : 
			(s_bPrepare? "p2p prepare/complete" : 
			(s_bTarget? "p2p target/message" : "none")))));
		printf("Audio transfer: %s\n", s_bAudio? "system buffer" : "none");
		printf("Source board:   %d\n", s_iBoardSource);
		printf("Target board:   %d\n", s_iBoardTarget);
		printf("Video format:	%s\n", videoFormat.c_str());
		printf("Frame format:	%s\n", frameFormat.c_str());
		printf("Audio channels: %d\n", s_iAudioChannels);
		printf("\n");

		// scan for video boards
		CNTV2DeviceScanner ntv2DeviceScan;
		CNTV2Card avCardSource;

		// open source board
        if (s_iBoardSource >= (int)ntv2DeviceScan.GetNumDevices())
		{
			printf("error: no source board %d\n", s_iBoardSource);
			throw 0;
		}

		if (!avCardSource.Open((UWord)s_iBoardSource))
		{
			printf("error: opening source board %d failed\n", s_iBoardSource);
			throw 0;
		}

		// verify the board can to p2p transfers
		if(!avCardSource.SupportsP2PTransfer())
		{
			printf("error: board %d can not do p2p transfer\n", s_iBoardSource);
			throw 0;
		}

		NTV2DeviceID eSourceDeviceID = avCardSource.GetDeviceID();

		// set source board video format
		avCardSource.SetVideoFormat(s_eVideoFormat);
		avCardSource.SetEveryFrameServices(NTV2_OEM_TASKS);

		// set source reference
		avCardSource.SetReference(NTV2_REFERENCE_INPUT1);

		// clear routing
		avCardSource.ClearRouting();

		// route video
		if (NTV2_IS_QUAD_FRAME_FORMAT(s_eVideoFormat))
		{
			for (int i = 0; i < 4; i++)
			{
				// disable transmitter
                if (::NTV2DeviceHasBiDirectionalSDI(eSourceDeviceID))
				{
					avCardSource.SetSDITransmitEnable((NTV2Channel)i, false);
				}

				// frame buffer pixel format
				if (!avCardSource.SetFrameBufferFormat((NTV2Channel)i, s_eFrameFormat))
					return __LINE__;

				// frame buffer capture mode
				if (!avCardSource.SetMode((NTV2Channel)i, NTV2_MODE_CAPTURE))
					return __LINE__;

				if (!avCardSource.EnableChannel((NTV2Channel)i))
					return __LINE__;

				// cross point routing
				if (::NTV2DeviceNeedsRoutingSetup(eSourceDeviceID))
				{
					if (IsRGBFormat(s_eFrameFormat))
					{
						avCardSource.Connect(::GetCSCInputXptFromChannel((NTV2Channel)i),
							::GetSDIInputOutputXptFromChannel((NTV2Channel)i));
						avCardSource.Connect(::GetFrameBufferInputXptFromChannel((NTV2Channel)i),
							::GetCSCOutputXptFromChannel((NTV2Channel)i, false, true));
						avCardSource.SetColorSpaceMakeAlphaFromKey(false, (NTV2Channel)i);
					}
					else
					{
						avCardSource.Connect(::GetFrameBufferInputXptFromChannel((NTV2Channel)i),
							::GetSDIInputOutputXptFromChannel((NTV2Channel)i));
					}
				}
			}
		}
		else
		{
			if (::NTV2DeviceHasBiDirectionalSDI(eSourceDeviceID))
			{
				avCardSource.SetSDITransmitEnable(s_eChannelSource, false);
			}

			// frame buffer pixel format
			if (!avCardSource.SetFrameBufferFormat(s_eChannelSource, s_eFrameFormat))
				return __LINE__;

			// frame buffer capture mode
			if (!avCardSource.SetMode(s_eChannelSource, NTV2_MODE_CAPTURE))
				return __LINE__;

			if (!avCardSource.EnableChannel(s_eChannelSource))
				return __LINE__;

			// cross point routing
			if (::NTV2DeviceNeedsRoutingSetup(eSourceDeviceID))
			{
				if (IsRGBFormat(s_eFrameFormat))
				{
					avCardSource.Connect(::GetCSCInputXptFromChannel(s_eChannelSource),
						::GetSDIInputOutputXptFromChannel(s_eChannelSource));
					avCardSource.Connect(::GetFrameBufferInputXptFromChannel(s_eChannelSource),
						::GetCSCOutputXptFromChannel(s_eChannelSource, false, true));
					avCardSource.SetColorSpaceMakeAlphaFromKey(false, s_eChannelSource);
				}
				else
				{
					avCardSource.Connect(::GetFrameBufferInputXptFromChannel(s_eChannelSource),
						::GetSDIInputOutputXptFromChannel(s_eChannelSource));
				}
			}
		}

		// setup audio
		NTV2AudioSystem eSourceAudio = NTV2_AUDIOSYSTEM_INVALID;
		if (s_bAudio)
		{
			eSourceAudio = NTV2AudioSystem(s_eChannelSource);
			avCardSource.SetNumberAudioChannels(s_iAudioChannels, eSourceAudio);
			avCardSource.SetAudioSystemInputSource (eSourceAudio, NTV2_AUDIO_EMBEDDED, NTV2EmbeddedAudioInput(s_eChannelSource));
		}

		// get source video size in bytes
		ULWord ulActiveVideoSize = GetVideoActiveSize(s_eVideoFormat, s_eFrameFormat);
		
		// allocate the source video buffer
		ULWord* pVidBufferSource = (ULWord*)new char[ulActiveVideoSize];
		memset(pVidBufferSource, 0, ulActiveVideoSize);
	
		// define audio buffer size
		ULWord ulAudioBufferSize = 4096*16*4;

		// allocate the audio buffer
		ULWord* pAudBufferSource = (ULWord*)new char[ulAudioBufferSize];
		memset(pAudBufferSource, 0, ulAudioBufferSize);

		// configure target board
		CNTV2Card avCardTarget;

		// open target board
        if (s_iBoardTarget >= (int)ntv2DeviceScan.GetNumDevices())
		{
			printf("error: no target board %d\n", s_iBoardTarget);
			throw 0;
		}

		if (!avCardTarget.Open((UWord)s_iBoardTarget))
		{
			printf("error: opening target board %d failed\n", s_iBoardTarget);
			throw 0;
		}

		// verify the board can to p2p transfers
		if (!avCardTarget.SupportsP2PTransfer())
		{
			printf("error: board %d can not do p2p transfer\n", s_iBoardSource);
			throw 0;
		}

		NTV2DeviceID eTargetDeviceID = avCardTarget.GetDeviceID();

		// set target board video format
		avCardTarget.SetVideoFormat(s_eVideoFormat);
		avCardTarget.SetEveryFrameServices(NTV2_OEM_TASKS);

		// set target reference
		avCardTarget.SetReference(NTV2_REFERENCE_FREERUN);

		// clear routing
		avCardTarget.ClearRouting();

		// route video
		if (NTV2_IS_QUAD_FRAME_FORMAT(s_eVideoFormat))
		{
			for (NTV2Channel ch (NTV2_CHANNEL1);  ch < NTV2_CHANNEL5;  ch = NTV2Channel(ch+1))
			{
				// frame buffer pixel format
				if (!avCardTarget.SetFrameBufferFormat(ch, s_eFrameFormat))
					return __LINE__;

				// frame buffer display mode
				if (!avCardTarget.SetMode(ch, NTV2_MODE_DISPLAY))
					return __LINE__;

				if (!avCardTarget.EnableChannel(ch))
					return __LINE__;

				// enable transmitter
				if (::NTV2DeviceHasBiDirectionalSDI(eTargetDeviceID))
				{
					avCardTarget.SetSDITransmitEnable(ch, true);
				}

				// route video
				if (IsRGBFormat(s_eFrameFormat))
				{
					avCardTarget.Connect(::GetCSCInputXptFromChannel(ch, false),
						::GetFrameBufferOutputXptFromChannel(ch, true, false));
					avCardTarget.Connect(::GetSDIOutputInputXpt(ch, false),
						::GetCSCOutputXptFromChannel(ch, false, false));
				}
				else
				{
					avCardTarget.Connect(::GetSDIOutputInputXpt(ch, false),
						::GetFrameBufferOutputXptFromChannel(ch, false, false));
				}
			}
		}
		else
		{
			// frame buffer pixel format
			if (!avCardTarget.SetFrameBufferFormat(s_eChannelTarget, s_eFrameFormat))
				return __LINE__;

			// frame buffer display mode
			if (!avCardTarget.SetMode(s_eChannelTarget, NTV2_MODE_DISPLAY))
				return __LINE__;

			if (!avCardTarget.EnableChannel(s_eChannelTarget))
				return __LINE__;

			// enable transmitter
			if (::NTV2DeviceHasBiDirectionalSDI(eTargetDeviceID))
			{
				avCardTarget.SetSDITransmitEnable(s_eChannelTarget, true);
			}

			// route video
			if (IsRGBFormat(s_eFrameFormat))
			{
				avCardTarget.Connect(::GetCSCInputXptFromChannel(s_eChannelTarget, false),
					::GetFrameBufferOutputXptFromChannel(s_eChannelTarget, true, false));
				avCardTarget.Connect(::GetSDIOutputInputXpt(s_eChannelTarget, false),
					::GetCSCOutputXptFromChannel(s_eChannelTarget, false, false));
			}
			else
			{
				avCardTarget.Connect(::GetSDIOutputInputXpt(s_eChannelTarget, false),
					::GetFrameBufferOutputXptFromChannel(s_eChannelTarget, false, false));
			}
		}

		// setup audio
		NTV2AudioSystem eTargetAudio = NTV2_AUDIOSYSTEM_INVALID;
		if (s_bAudio)
		{
			eTargetAudio = NTV2ChannelToAudioSystem(s_eChannelTarget);
			avCardTarget.SetNumberAudioChannels(s_iAudioChannels, eTargetAudio);
			avCardTarget.SetSDIOutputAudioSystem(s_eChannelTarget, eTargetAudio);
		}

		// subscribe to video input interrupts
		avCardSource.SubscribeInputVerticalEvent(s_eChannelSource);

		ULWord ulCaptureFrameNumber = s_iIndexFirstSource;
		ULWord ulSourceFrameNumber = s_iIndexFirstSource;
		ULWord ulTargetFrameNumber = s_iIndexFirstTarget;
		if(s_bDirect)
		{
			// set register update mode to frame
			avCardSource.SetRegisterWriteMode(NTV2_REGWRITE_SYNCTOFRAME);
			avCardTarget.SetRegisterWriteMode(NTV2_REGWRITE_SYNCTOFRAME);
			// set source to capture first frame
			avCardSource.SetInputFrame(s_eChannelSource, ulCaptureFrameNumber);
			// wait for capture to start
			avCardSource.WaitForInputFieldID(NTV2_FIELD0, s_eChannelSource);
			// set source to capture next frame
			ulCaptureFrameNumber++;
			avCardSource.SetInputFrame(s_eChannelSource, ulCaptureFrameNumber);
			// wait for first frame capture to complete
			avCardSource.WaitForInputFieldID(NTV2_FIELD0, s_eChannelSource);
			avCardSource.WaitForInputFieldID(NTV2_FIELD0, s_eChannelSource);
		}
		else
		{
			// initialize autocirculate source
			avCardSource.AutoCirculateStop(s_eChannelSource);
			avCardSource.AutoCirculateInitForInput(s_eChannelSource, 0, eSourceAudio, 0, 1, s_iIndexFirstSource, s_iIndexLastSource);
			avCardSource.AutoCirculateStart(s_eChannelSource);

			// initialize autocirculate target
			avCardTarget.AutoCirculateStop(s_eChannelTarget);
			avCardTarget.AutoCirculateInitForOutput(s_eChannelTarget, 0, eTargetAudio, 0, 1, s_iIndexFirstTarget, s_iIndexLastTarget);
		}

		// setup to transfer video/audio buffers
		int iTest = 0;
		bool bStart = false;
//		ULWord ulNumBuffersSource = s_iIndexLastSource - s_iIndexFirstSource + 1;
		ULWord ulNumBuffersTarget = s_iIndexLastTarget - s_iIndexFirstTarget + 1;

		// get target buffer addresses
		if(s_bDirect)
		{
			ULWord ulFrameNumber;
			for(ulFrameNumber = s_iIndexFirstTarget; ulFrameNumber <= (ULWord)s_iIndexLastTarget; ulFrameNumber++)
			{
				// allocate a p2p data structure
				AUTOCIRCULATE_P2P_STRUCT p2pData;

				// get target p2p information
				if(!avCardTarget.DmaP2PTargetFrame(s_eChannelTarget, ulFrameNumber, 0, &p2pData))
				{
					printf("error: DmaP2PTargetFrame failed\n");
					throw 0;
				}

				printf("Target buffer  address %08x:%08x  size %08x\n",
					(ULWord)(p2pData.videoBusAddress>>32), (ULWord)p2pData.videoBusAddress, p2pData.videoBusSize);
			}
		}
		else if(s_bPrepare || s_bTarget)
		{
			for(iTest = 0; iTest < (int)ulNumBuffersTarget; iTest++)
			{
				// allocate a p2p data structure
				AUTOCIRCULATE_P2P_STRUCT p2pBuffer;

				// first prepare the target for p2p video transfer
				s_TransferTarget.SetVideoBuffer((ULWord*)(&p2pBuffer), 0);			// connect autocirculate to p2p buffer
				s_TransferTarget.SetAudioBuffer(NULL, 0);							// no audio to transfer
				s_TransferTarget.acDesiredFrame = (-1);
				s_TransferTarget.acPeerToPeerFlags = AUTOCIRCULATE_P2P_TARGET;		// specify target for p2p transfer with message

				// write the p2p target data to the p2p structure
				if (!avCardTarget.AutoCirculateTransfer(s_eChannelTarget, s_TransferTarget))
				{
					printf("error: autocirculate target prepare transfer failed\n");
					throw 0;
				}

				printf("Target buffer  address %08x:%08x  size %08x\n",
					(ULWord)(p2pBuffer.videoBusAddress>>32), (ULWord)p2pBuffer.videoBusAddress, p2pBuffer.videoBusSize);
			}
			avCardTarget.AutoCirculateFlush(s_eChannelTarget);
		}

		// transfer until done
		iTest = 0;
		while((s_iTestCount == 0) || (iTest < s_iTestCount))
		{
			if(s_bDirect)
			{
				CHANNEL_P2P_STRUCT p2pData;

				// get target p2p information
				if(!avCardTarget.DmaP2PTargetFrame(s_eChannelTarget, ulTargetFrameNumber, 0, &p2pData))
				{
					printf("error: DmaP2PTargetFrame failed\n");
					throw 0;
				}

//                p2pData.messageBusAddress = 0;
//                p2pData.messageData = 0;

				// transfer source frame to target buffer
				if(!avCardSource.DmaP2PTransferFrame(NTV2_DMA_FIRST_AVAILABLE, ulSourceFrameNumber,
					0, ulActiveVideoSize, 0, 0, 0, &p2pData))
				{
					printf("error: DmaP2PTransferFrame failed\n");
					throw 0;
				}

				// set output frame number for next playback frame
				avCardTarget.SetOutputFrame(s_eChannelTarget, ulTargetFrameNumber);

				// update source frame
				ulSourceFrameNumber = ulCaptureFrameNumber;

				// update capture frame
				ulCaptureFrameNumber++;
				if(ulCaptureFrameNumber > (ULWord)s_iIndexLastSource)
				{
					ulCaptureFrameNumber = s_iIndexFirstSource;
				}

				// set input frame number for next capture frame
				avCardSource.SetInputFrame(s_eChannelSource, ulCaptureFrameNumber);

				// update target frame
				ulTargetFrameNumber++;
				if(ulTargetFrameNumber > (ULWord)s_iIndexLastTarget)
				{
					ulTargetFrameNumber = s_iIndexFirstTarget;
				}

				// count the frames
				iTest++;
				printf("frames %d\r", iTest);
				fflush(stdout);
				s_bLFNeeded = true;

				// wait for next source frame
				if(!avCardSource.WaitForInputFieldID(NTV2_FIELD0, s_eChannelSource))
				{
					printf("error: wait for interrupt failed\n");
				}
			}
			else
			{
				// check if source and target frames available
				avCardSource.AutoCirculateGetStatus(s_eChannelSource, s_AutoCirculateStatusSource);
				avCardTarget.AutoCirculateGetStatus(s_eChannelTarget, s_AutoCirculateStatusTarget);
				if((s_AutoCirculateStatusSource.GetBufferLevel() > 1) && 
                    (s_AutoCirculateStatusTarget.GetBufferLevel() < (ulNumBuffersTarget - 1)))
				{
					// switch on system buffer or p2p transfer
					if(s_bSystem)
					{
						// connect autocirculate to source buffers
						s_TransferSource.SetVideoBuffer(pVidBufferSource, ulActiveVideoSize);		// connect autocirculate to the video system buffer
						s_TransferSource.SetAudioBuffer(pAudBufferSource, ulAudioBufferSize);		// connect autocirculate to the audio system buffer

						// transfer the source video/audio to the system buffers
						if (!avCardSource.AutoCirculateTransfer(s_eChannelSource, s_TransferSource))
						{
							printf("error: autocirculate source transfer failed\n");
							throw 0;
						}

						// connect autocirculate to source buffers
						s_TransferTarget.SetVideoBuffer(pVidBufferSource, ulActiveVideoSize);		// connect autocirculate to the system buffer
						s_TransferTarget.SetAudioBuffer(pAudBufferSource, 
							s_TransferSource.GetCapturedAudioByteCount());							// transfer audio from system buffer

						// transfer video/audio to target from the system buffers
						if (!avCardTarget.AutoCirculateTransfer(s_eChannelTarget, s_TransferTarget))
						{
							printf("error: autocirculate target transfer failed\n");
							throw 0;
						}

						// start the target play once there are several buffers in the queue
						if((!bStart && (iTest > 1)) || ((s_iTestCount != 0) && (s_iTestCount < 3)))
						{
							avCardTarget.AutoCirculateStart(s_eChannelTarget);
							bStart = true;
						}
					}
					else if(s_bPrepare)
					{
						// allocate a p2p data structure
						AUTOCIRCULATE_P2P_STRUCT p2pBuffer;

						// first prepare the target for p2p video transfer
						s_TransferTarget.SetVideoBuffer((ULWord*)(&p2pBuffer), 0);					// connect autocirculate to p2p buffer
						s_TransferTarget.SetAudioBuffer(NULL, 0);									// no audio to transfer
						s_TransferTarget.acDesiredFrame = (-1);
							s_TransferTarget.acPeerToPeerFlags = AUTOCIRCULATE_P2P_PREPARE;				// specify prepare for p2p transfer with completion

						// write the p2p target data to the p2p structure
						if (!avCardTarget.AutoCirculateTransfer(s_eChannelTarget, s_TransferTarget))
						{
							printf("error: autocirculate target prepare transfer failed\n");
							throw 0;
						}

						// transfer video from the source frame buffer directly to the target frame buffer, audio to the system buffer
						s_TransferSource.SetVideoBuffer((ULWord*)(&p2pBuffer), ulActiveVideoSize);	// connect autocirculate to the p2p buffer
						s_TransferSource.SetAudioBuffer(pAudBufferSource, ulAudioBufferSize);		// connect autocirculate to audio buffer
						s_TransferSource.acPeerToPeerFlags = AUTOCIRCULATE_P2P_TRANSFER;			// do p2p video transfer

						// transfer the video/audio
						if (!avCardSource.AutoCirculateTransfer(s_eChannelSource, s_TransferSource))
						{
							printf("error: autocirculate source transfer failed\n");
							throw 0;
						}

						// inform the target that the p2p transfer is complete and transfer audio from the source
						s_TransferTarget.SetVideoBuffer(NULL, 0);									// no video to transfer
						s_TransferTarget.SetAudioBuffer(pAudBufferSource, 
							s_TransferSource.GetCapturedAudioByteCount());							// transfer audio from system buffer
						s_TransferTarget.acDesiredFrame = 
							s_TransferTarget.GetTransferFrameNumber();								// frame buffer that was prepared
						s_TransferTarget.acPeerToPeerFlags = AUTOCIRCULATE_P2P_COMPLETE;			// p2p transfer is complete

						// transfer the audio and declare the p2p video transfer complete
						if (!avCardTarget.AutoCirculateTransfer(s_eChannelTarget, s_TransferTarget))
						{
							printf("error: autocirculate target complete transfer failed\n");
							throw 0;
						}

						// start the target play once there are several buffers in the queue
                        if((!bStart && (iTest > 1)) || ((s_iTestCount != 0) && (s_iTestCount < 3)))
						{
							avCardTarget.AutoCirculateStart(s_eChannelTarget);
							bStart = true;
						}
					}
					else if(s_bTarget)
					{
						// allocate a p2p data structure
						AUTOCIRCULATE_P2P_STRUCT p2pBuffer;

						// first prepare the target for p2p video transfer
						s_TransferTarget.SetVideoBuffer((ULWord*)(&p2pBuffer), 0);					// connect autocirculate to p2p buffer
						s_TransferTarget.SetAudioBuffer(NULL, 0);									// no audio to transfer
						s_TransferTarget.acDesiredFrame = (-1);
						s_TransferTarget.acPeerToPeerFlags = AUTOCIRCULATE_P2P_TARGET;				// specify target for p2p transfer with message

						// write the p2p target data to the p2p structure
						if (!avCardTarget.AutoCirculateTransfer(s_eChannelTarget, s_TransferTarget))
						{
							printf("error: autocirculate target prepare transfer failed\n");
							throw 0;
						}

						// transfer video from the source frame buffer directly to the target frame buffer
						s_TransferSource.SetVideoBuffer((ULWord*)(&p2pBuffer), ulActiveVideoSize);	// connect autocirculate to the p2p buffer
						s_TransferSource.SetAudioBuffer(NULL, 0);									// no audio since no completion
						s_TransferSource.acPeerToPeerFlags = AUTOCIRCULATE_P2P_TRANSFER;			// do p2p video transfer

						// transfer the video/audio
						if (!avCardSource.AutoCirculateTransfer(s_eChannelSource, s_TransferSource))
						{
							printf("error: autocirculate source transfer failed\n");
							throw 0;
						}

						// start the target play once there are several buffers in the queue
						if((!bStart && (iTest > 1)) || ((s_iTestCount != 0) && (s_iTestCount < 3)))
						{
							avCardTarget.AutoCirculateStart(s_eChannelTarget);
							bStart = true;
						}
					}

					// count the frames
					iTest++;
					printf("frames %d\r", iTest);
					fflush(stdout);
					s_bLFNeeded = true;
				}
				else
				{
					// if no frame available wait for next interrupt
					avCardSource.WaitForInputVerticalInterrupt(s_eChannelSource);
				}
			}
		}

		if(s_bLFNeeded)
		{
			printf("\n");
			s_bLFNeeded = false;
		}

		// done so stop the autocirculate queues
		avCardSource.AutoCirculateStop(s_eChannelSource);
		avCardTarget.AutoCirculateStop(s_eChannelTarget);

		// delete the buffers
		if(pVidBufferSource != NULL)
		{
			delete [] pVidBufferSource;
		}
		if(pAudBufferSource != NULL)
		{
			delete [] pAudBufferSource;
		}

		if(s_bDirect)
		{
			avCardSource.SetMode(s_eChannelSource, NTV2_MODE_DISPLAY);
			avCardTarget.SetMode(s_eChannelTarget, NTV2_MODE_DISPLAY);
		}

		// unsubscribe to video input interrupts
		avCardSource.UnsubscribeInputVerticalEvent(s_eChannelSource);

		return 0;
	}
	catch(...)
	{
	}

	return -1;
}
