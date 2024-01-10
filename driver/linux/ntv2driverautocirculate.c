/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA boards.
//
// Filename:	ntv2driverautocirculate.c
// Purpose: 	Implementation file for autocirculate methods.
// Notes:
//
///////////////////////////////////////////////////////////////


/*needed by kernel 2.6.18*/
#ifndef CONFIG_HZ
#include <linux/autoconf.h>
#endif

#include <linux/version.h>
#include <linux/kernel.h> // for printk() prototype
#include <linux/delay.h>
#include <linux/pci.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/div64.h>
#include <linux/interrupt.h>
#include <linux/compiler.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
#include <linux/timekeeping.h>
#else
#include <linux/time.h>
#endif

#include "ajatypes.h"
#include "ntv2enums.h"

#include "ntv2publicinterface.h"
#include "ntv2linuxpublicinterface.h"
#include "ntv2devicefeatures.h"
#include "ntv2audiodefines.h"

#include "ntv2driver.h"
#include "registerio.h"
#include "ntv2dma.h"
#include "driverdbg.h"
#include "ntv2driverdbgmsgctl.h"
#include "ntv2drivertask.h"
#include "ntv2kona2.h"
#include "ntv2hdmiout4.h"
#include "../ntv2kona.h"


/**************************/
/* Local defines and types */
/***************************/

//#define AUTO_REPORT
void get100nsTime(LWord64 *time)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0))
	struct timespec64 ts64;

	if (time)
	{
		ktime_get_real_ts64(&ts64);
		*time = ((LWord64)ts64.tv_sec * 10000000) + (ts64.tv_nsec / 100);
	}
#else
	struct timeval tv;

	if (time)
	{
		do_gettimeofday(&tv);

		//	*10 to convert from usecs to 100's of nsec
		*time = ((LWord64)tv.tv_sec * 1000000 + tv.tv_usec) * 10;
	}
#endif
}

#define my_rdtscll(x) get100nsTime(&x)

#if !defined (rdtscll) && defined(__powerpc__)     /* 32bit version */
#define rdtscll(val)                                                   \
        do {                                                           \
               uint32_t tbhi, tblo ;                                   \
               __asm__ __volatile__ ("mftbu %0" : "=r" (tbhi));        \
               __asm__ __volatile__ ("mftbl %0" : "=r" (tblo));        \
               val = 1000 * ((uint64_t) tbhi << 32) | tblo;            \
       } while (0)
#endif

static char* CrosspointName[] =
{
	"Channel 1",
	"Channel 2",
	"Input 1",
	"Input 2",
	"Matte",
	"FGKey",
	"Channel 3",
	"Channel 4",
	"Input 3",
	"Input 4",
	"Channel 5",
	"Channel 6",
	"Channel 7",
	"Channel 8",
	"Input 5",
	"Input 6",
	"Input 7",
	"Input 8"
};

#define AUDIO_IN_SAMPLE_SIZE		4
#define AUDIO_IN_GRANULARITY		1
#define AUDIO_OUT_SAMPLE_SIZE		4
#define AUDIO_OUT_GRANULARITY		1

static const ULWord gChannelToTCDelayReg[] = {
	kVRegTimeCodeCh1Delay, kVRegTimeCodeCh2Delay, kVRegTimeCodeCh3Delay, kVRegTimeCodeCh4Delay,
	kVRegTimeCodeCh5Delay, kVRegTimeCodeCh6Delay, kVRegTimeCodeCh7Delay, kVRegTimeCodeCh8Delay, 0 };

static const ULWord	gChannelToOutputFrameReg[] = {
	kRegCh1OutputFrame, kRegCh2OutputFrame, kRegCh3OutputFrame, kRegCh4OutputFrame,
	kRegCh5OutputFrame, kRegCh6OutputFrame, kRegCh7OutputFrame, kRegCh8OutputFrame, 0 };

static const ULWord	gChannelToInputFrameReg[] = {
	kRegCh1InputFrame, kRegCh2InputFrame, kRegCh3InputFrame, kRegCh4InputFrame,
	kRegCh5InputFrame, kRegCh6InputFrame, kRegCh7InputFrame, kRegCh8InputFrame, 0 };

static const ULWord	gChannelToControlRegNum [] = {
	kRegCh1Control, kRegCh2Control, kRegCh3Control, kRegCh4Control, kRegCh5Control, kRegCh6Control,
	kRegCh7Control, kRegCh8Control, 0};

static const ULWord gAudioRateHighMask [] = {
	kRegMaskAud1RateHigh, kRegMaskAud2RateHigh, kRegMaskAud3RateHigh, kRegMaskAud4RateHigh,
	kRegMaskAud5RateHigh, kRegMaskAud6RateHigh, kRegMaskAud7RateHigh, kRegMaskAud8RateHigh };

static const ULWord gAudioRateHighShift [] = {
	kRegShiftAud1RateHigh, kRegShiftAud2RateHigh, kRegShiftAud3RateHigh, kRegShiftAud4RateHigh,
	kRegShiftAud5RateHigh, kRegShiftAud6RateHigh, kRegShiftAud7RateHigh, kRegShiftAud8RateHigh };

/*********************************************/
/* Prototypes for private utility functions. */
/*********************************************/

static long KAUTO_NEXTFRAME(LWord __dwCurFrame_, INTERNAL_AUTOCIRCULATE_STRUCT* __pAuto_);
static long KAUTO_PREVFRAME(LWord __dwCurFrame_, INTERNAL_AUTOCIRCULATE_STRUCT* __pAuto_);

static ULWord OemAutoCirculateGetBufferLevel(INTERNAL_AUTOCIRCULATE_STRUCT *pAuto);
static void OemBeginAutoCirculateTransfer (ULWord deviceNumber,
										   ULWord frameNumber,
										   AUTOCIRCULATE_TRANSFER_STRUCT *pTransferStruct,
										   INTERNAL_AUTOCIRCULATE_STRUCT *pAuto,
										   NTV2RoutingTable* pNTV22RoutingTable,
										   PAUTOCIRCULATE_TASK_STRUCT pTaskInfo);
static void OemBeginAutoCirculateTransfer_Ex (ULWord deviceNumber,
											  ULWord frameNumber,
											  AUTOCIRCULATE_TRANSFER *pTransferStruct,
											  INTERNAL_AUTOCIRCULATE_STRUCT *pAuto);
static void OemCompleteAutoCirculateTransfer(ULWord deviceNumber,
											 ULWord frameNumber,
											 AUTOCIRCULATE_TRANSFER_STATUS_STRUCT *pUserOutBuffer,
											 INTERNAL_AUTOCIRCULATE_STRUCT *pAuto,
											 bool updateValid, bool transferPending);
static void OemCompleteAutoCirculateTransfer_Ex(ULWord deviceNumber,
												ULWord frameNumber,
												AUTOCIRCULATE_TRANSFER_STATUS *pUserOutBuffer,
												INTERNAL_AUTOCIRCULATE_STRUCT *pAuto,
												bool updateValid, bool transferPending);
static bool IsField0(ULWord deviceNumber, NTV2Crosspoint channelSpec);

// Audio routines
static ULWord GetNumAudioChannels(ULWord deviceNumber, NTV2AudioSystem audioSystem);
//static ULWord64 GetAudioClock(ULWord deviceNumber);
static unsigned long GetAudioControlRegisterAddressAndLock(ULWord deviceNumber,
														   NTV2AudioSystem audioSystem,
														   spinlock_t** ppLock);
static ULWord GetAudioControlRegister(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static void StartAudioCapture(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static void StopAudioCapture(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static void StartAudioPlayback(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static void StopAudioPlayback(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static ULWord IsAudioPlaying(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static void PauseAudioPlayback(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static void UnPauseAudioPlayback(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static bool IsAudioPlaybackPaused(ULWord deviceNumber, NTV2AudioSystem audioSystem);
static bool IsAudioPlaybackStopped(ULWord deviceNumber, NTV2AudioSystem audioSystem);

static inline ULWord oemAudioSampleAlignIn (ULWord deviceNumber,
											NTV2AudioSystem audioSystem,
											ULWord ulReadSample);
static inline ULWord oemAudioSampleAlignOut (ULWord deviceNumber,
											 NTV2AudioSystem audioSystem,
											 ULWord ulReadSample);
static LWord GetFramePeriod(ULWord deviceNumber, NTV2Channel channel);

int oemAutoCirculateDmaAudioSetup(ULWord deviceNumber, INTERNAL_AUTOCIRCULATE_STRUCT* pAuto);

static void
OemAutoCirculateSetupVidProc(ULWord deviceNumber,
							 NTV2Crosspoint channelSpec,
							 AutoCircVidProcInfo* vidProcInfo);

static void
OemAutoCirculateSetupColorCorrector(ULWord deviceNumber,
									NTV2Crosspoint channelSpec,
									INTERNAL_ColorCorrectionInfo *ccInfo);

static int
OemAutoCirculateTransferColorCorrectorInfo(ULWord deviceNumber,
										   INTERNAL_ColorCorrectionInfo *ccInternalInfo,
										   NTV2ColorCorrectionInfo *ccTransferInfo);

static void
oemAutoCirculateTaskInit(PAUTOCIRCULATE_TASK_STRUCT pTaskInfo, AutoCircGenericTask* pTaskArray, ULWord maxTasks);

static int
oemAutoCirculateTaskTransfer(PAUTOCIRCULATE_TASK_STRUCT pDriverInfo,
							 PAUTOCIRCULATE_TASK_STRUCT pUserInfo,
							 bool bToDriver);

static bool DropSyncFrame(ULWord deviceNumber, INTERNAL_AUTOCIRCULATE_STRUCT* pAuto);

static bool OemAutoCirculateP2PCopy(PAUTOCIRCULATE_P2P_STRUCT pDriverBuffer,
									PAUTOCIRCULATE_P2P_STRUCT pUserBuffer,
									bool bToDriver);


static inline ULWord
oemAudioSampleAlignIn (ULWord deviceNumber, NTV2AudioSystem audioSystem, ULWord ulReadSample)
{
	ULWord numBytesPerAudioSampleGroup = GetNumAudioChannels(deviceNumber, audioSystem)*AUDIO_IN_SAMPLE_SIZE*AUDIO_IN_GRANULARITY;
	return ((ulReadSample / numBytesPerAudioSampleGroup) * numBytesPerAudioSampleGroup);
}

static inline ULWord
oemAudioSampleAlignOut (ULWord deviceNumber, NTV2AudioSystem audioSystem, ULWord ulReadSample)
{
	ULWord numBytesPerAudioSampleGroup = GetNumAudioChannels(deviceNumber, audioSystem)*AUDIO_OUT_SAMPLE_SIZE*AUDIO_OUT_GRANULARITY;
	return ((ulReadSample / numBytesPerAudioSampleGroup) * numBytesPerAudioSampleGroup);
}

// Initialize all Auto Circulators -- done at driver load time.
int
AutoCirculateInitialize(ULWord deviceNumber)
{
	int channelSpec;
	NTV2PrivateParams* pNTV2Params;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	// Start with all autocirculators disabled.
	for (channelSpec = 0; channelSpec < NUM_CIRCULATORS; channelSpec++)
	{
		memset(&pNTV2Params->_AutoCirculate[channelSpec],
			  0,
			  sizeof (INTERNAL_AUTOCIRCULATE_STRUCT));
		
		pNTV2Params->_AutoCirculate[channelSpec].channelSpec = channelSpec;
		pNTV2Params->_AutoCirculate[channelSpec].state = NTV2_AUTOCIRCULATE_DISABLED;
		pNTV2Params->_AutoCirculate[channelSpec].deviceNumber = deviceNumber;
		pNTV2Params->_LkUpAcChanSpecGivenEngine[channelSpec] = NTV2CROSSPOINT_FGKEY;  // invalid value
	}

	pNTV2Params->_ulNumberOfWrapsOfClockSampleCounter= 0;
	pNTV2Params->_ulLastClockSampleCounter= 0;
	pNTV2Params->deviceNumber = deviceNumber;

	return 0;
}

///////////////////////////////////////////////////////////////////////
// Auto-Circulate routines called from the ISR
///////////////////////////////////////////////////////////////////////

void
OemAutoCirculate(ULWord deviceNumber, NTV2Crosspoint channelSpec)
{
	Ntv2SystemContext systemContext;
	ULWord64 RDTSC = 0;

    ULWord64 audioCounter = 0;
    ULWord audioOutLastAddress = 0;
    ULWord audioInLastAddress = 0;
	ULWord ulActualLastIn = 0;
	ULWord numSamplesPerFrame = 0;
	ULWord newInputAddress = 0;
	ULWord newStartAddress = 0;

	bool changeEvent = false;
	bool dropFrame = false;
	bool validFrame = false;

    INTERNAL_AUTOCIRCULATE_STRUCT* pAuto = NULL;
    LWord lastActiveFrame = 0;
    INTERNAL_FRAME_STAMP_STRUCT* pActiveFrameStamp = NULL;
	INTERNAL_FRAME_STAMP_STRUCT* pLastFrameStamp = NULL;
    NTV2FrameBufferFormat currentFBF;
	NTV2PrivateParams* pNTV2Params;
    NTV2VideoFrameBufferOrientation currentFBO;
	NTV2Channel channel = NTV2_CHANNEL1;
	NTV2Channel syncChannel = NTV2_CHANNEL1;
	NTV2Channel acChannel = NTV2_CHANNEL1;
	NTV2Crosspoint pautoChannelSpec = NTV2CROSSPOINT_INVALID;
	bool syncProgressive = false;
	bool fieldMode = false;
	bool syncField0 = false;
	systemContext.devNum = deviceNumber;

    if (ILLEGAL_CHANNELSPEC(channelSpec))
    {
	    return;
    }

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
	{
		return;
	}

    pAuto = &pNTV2Params->_AutoCirculate[channelSpec];
	acChannel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
	pautoChannelSpec = pAuto->channelSpec;

	syncChannel = NTV2_CHANNEL1;
	if(IsMultiFormatActive(&systemContext))
	{
		syncChannel = GetNTV2ChannelForNTV2Crosspoint(pautoChannelSpec);
	}
	syncProgressive = IsProgressiveStandard(&systemContext, syncChannel);
	fieldMode = false;
	syncField0 = true;
	if (!syncProgressive)
	{
		syncField0 = IsField0(deviceNumber, pAuto->channelSpec);
		if (pAuto->circulateWithFields)
		{
			fieldMode = oemAutoCirculateCanDoFieldMode(deviceNumber, pautoChannelSpec);
		}
	}

	if (syncField0 || fieldMode)
	{
		if (pNTV2Params->_startAudioNextFrame && NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec))
		{
			UnPauseAudioPlayback(deviceNumber, pAuto->audioSystem);
			StartAudioPlayback(deviceNumber, pAuto->audioSystem);
			pNTV2Params->_startAudioNextFrame = false;
		}

		//In band start on interrupt
		if (pAuto->startAudioNextFrame && NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec))
		{
			UnPauseAudioPlayback(deviceNumber, pAuto->audioSystem);
			StartAudioPlayback(deviceNumber, pAuto->audioSystem);
			pAuto->startAudioNextFrame = false;
		}

		//In band stop on interrupt
		if (pAuto->stopAudioNextFrame && NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec))
		{
			StopAudioPlayback(deviceNumber, pAuto->audioSystem);
			pAuto->stopAudioNextFrame = false;
		}

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
		{
			MSG("PT B: pAuto->recording %d for board %d chspec %d\n"
				"pAuto->state is %d\n",
		  		pAuto->recording, deviceNumber, channelSpec,
		  		pAuto->state);
		}
		if (pAuto->state == NTV2_AUTOCIRCULATE_RUNNING ||
		   pAuto->state == NTV2_AUTOCIRCULATE_PAUSED  ||
		   pAuto->state == NTV2_AUTOCIRCULATE_INIT  ||
		   pAuto->state == NTV2_AUTOCIRCULATE_STARTING)
		{
			// Update the last vertical blank time
			pAuto->VBILastRDTSC = pAuto->VBIRDTSC;
			// Update the current vertical blank time
			my_rdtscll(pAuto->VBIRDTSC);	// Pointer not required
			RDTSC = pAuto->VBIRDTSC;
	        audioCounter = GetAudioClock(deviceNumber);
		}

		if (pAuto->state == NTV2_AUTOCIRCULATE_RUNNING ||
		   pAuto->state == NTV2_AUTOCIRCULATE_PAUSED  ||
		   pAuto->state == NTV2_AUTOCIRCULATE_STARTING)
		{
			// Always align
			// Read the audio out time
			pAuto->VBIAudioOut = oemAudioSampleAlignOut(deviceNumber,
														pAuto->audioSystem,
														GetAudioLastOut(deviceNumber, pAuto->audioSystem));
		    audioOutLastAddress   = pAuto->VBIAudioOut;	//ReadAudioLastOut();
			lastActiveFrame = pAuto->activeFrame;
			pAuto->activeFrame = ReadRegister (deviceNumber, pAuto->activeFrameRegister, NO_MASK, NO_SHIFT);
			if (pAuto->activeFrame < pAuto->startFrame ||
				pAuto->activeFrame > pAuto->endFrame)
			{
				pAuto->activeFrame = pAuto->startFrame;
			}
			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
			{
				MSG("OemAutoCirculate(): lastActiveFrame = %d, pAuto->activeFrame = %d\n",
					lastActiveFrame, pAuto->activeFrame);
				MSG("OemAutoCirculate(): audioOutLastAddress=0x%X\n",audioOutLastAddress);
			}
			pActiveFrameStamp = &pAuto->frameStamp[pAuto->activeFrame];
			pAuto->prevInterruptTime = pAuto->lastInterruptTime;
			pAuto->lastInterruptTime = pAuto->VBIRDTSC;
			pAuto->lastAudioClockTimeStamp = audioCounter;
		}

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
		{
			MSG("PT C: pAuto->recording %d for board %d chspec %d\n"
				"pAuto->state is %d\n",
		  		pAuto->recording, deviceNumber, channelSpec,
		  		pAuto->state);
		}

        switch (pAuto->state)
        {
		case NTV2_AUTOCIRCULATE_INIT:
		{
			pAuto->startTimeStamp = pAuto->VBIRDTSC;
		}
		break;
		case NTV2_AUTOCIRCULATE_STARTING_AT_TIME:
		{
		}
        case NTV2_AUTOCIRCULATE_STARTING:
		{
			LWord nextFrame;

			// if there is a start time set then wait for it
			if (pAuto->startTime > pAuto->VBIRDTSC)
			{
				break;
			}

			// When starting an auto circulate, ignore the currently playing
			// or recording frame.  Start the real play or record on the
			// next frame
			changeEvent = true;

			// TimeStamp  NOTE!!! doesn't take into account repeated frames.
			// First valid frame
			nextFrame = KAUTO_NEXTFRAME(pAuto->activeFrame, pAuto);

			if (pAuto->recording)
			{
				if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("State:NTV2_AUTOCIRCULATE_STARTING(record) \n");
				}

				// frame not valid (audio, rp188, etc)
				pAuto->frameStamp[pAuto->activeFrame].validCount = 0;	// Do not use frame
					
				// read rp188 registers to delay queue
				if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
				{
					CopyRP188HardwareToFrameStampTCArray(&systemContext, &pAuto->frameStamp[lastActiveFrame].internalTCArray);
				}

				if (NTV2DeviceCanDoSDIErrorChecks(pNTV2Params->_DeviceID))
				{
					CopySDIStatusHardwareToFrameStampSDIStatusArray(&systemContext, &pAuto->frameStamp[lastActiveFrame].internalSDIStatusArray);
				}

				ulActualLastIn = GetAudioLastIn(deviceNumber, pAuto->audioSystem);
				audioInLastAddress = oemAudioSampleAlignIn(
					deviceNumber,
					pAuto->audioSystem, ulActualLastIn) + GetAudioReadOffset(deviceNumber, pAuto->audioSystem);
				
				pActiveFrameStamp->frameTime = 	pAuto->VBIRDTSC;	// Record will start record NOW for this frame =D->
				pActiveFrameStamp->audioInStartAddress = audioInLastAddress;
				pActiveFrameStamp->audioClockTimeStamp = audioCounter;
				pActiveFrameStamp->ancTransferSize = 0;
				pActiveFrameStamp->ancField2TransferSize = 0;
				pAuto->startAudioClockTimeStamp = audioCounter;
				pAuto->startTimeStamp = pAuto->VBIRDTSC;			// Start of first frame is NOW =D->
				if(fieldMode)
					pActiveFrameStamp->frameFlags = syncField0? AUTOCIRCULATE_FRAME_FIELD0 : AUTOCIRCULATE_FRAME_FIELD1;
				else 
					pActiveFrameStamp->frameFlags = AUTOCIRCULATE_FRAME_FULL;

				if (pAuto->circulateWithAudio)
				{
					numSamplesPerFrame = GetAudioSamplesPerFrame(deviceNumber,
																 pAuto->audioSystem,
																 pAuto->framesProcessed,
																 AUDIO_IN_GRANULARITY,
																 fieldMode);
					newInputAddress = GetAudioTransferInfo(
						deviceNumber,
						pAuto->audioSystem,
						pActiveFrameStamp->audioInStartAddress - GetAudioReadOffset(deviceNumber, pAuto->audioSystem),
						numSamplesPerFrame*GetNumAudioChannels(deviceNumber, pAuto->audioSystem)*AUDIO_IN_SAMPLE_SIZE,
						&pActiveFrameStamp->audioPreWrapBytes,
						&pActiveFrameStamp->audioPostWrapBytes);
					pActiveFrameStamp->audioInStopAddress = newInputAddress + GetAudioReadOffset(deviceNumber, pAuto->audioSystem);
				}

				if (pAuto->circulateWithCustomAncData)
				{
					if (pAuto->circulateWithHDMIAux)
					{
						SetAuxExtWriteParams(&systemContext, acChannel, nextFrame);
					}
					else
					{
						SetAncExtWriteParams(&systemContext, acChannel, nextFrame);
					}
				}

				if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("ISR->AC->recording: numSamplesPerFrame=%d numAudioChannels=%d bytesToXfer=%d newInputAddress=0x%X\n",
						numSamplesPerFrame, GetNumAudioChannels(deviceNumber, pAuto->audioSystem),
						(numSamplesPerFrame*GetNumAudioChannels(deviceNumber, pAuto->audioSystem)*AUDIO_IN_SAMPLE_SIZE),
						newInputAddress);
				}

				// start capture on field 0
				if (syncField0)
				{
					// write next video frame number to active frame register
					WriteRegister(deviceNumber, pAuto->activeFrameRegister, nextFrame, NO_MASK, NO_SHIFT);
					pAuto->nextFrame = nextFrame;
					if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
					{
						MSG("OemAutoCirculate(Rec fn=%d): ActiveFrameStamp->frameTime=%llu\npActiveFrameStamp->audioInStartAddress=%0Xx\npActiveFrameStamp->audioClockTimeStamp=%llu\n",
							pAuto->activeFrame, pAuto->VBIRDTSC, audioInLastAddress, audioCounter);
					}

					// off and running
					pAuto->state = NTV2_AUTOCIRCULATE_RUNNING;
				}
			}
			else  // playback
			{
				if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("State:NTV2_AUTOCIRCULATE_STARTING(play) \n");
				}
				// Mark each frame so timeStamp + (validCount * frametime) is always valid
				pActiveFrameStamp->frameTime = 	pAuto->VBIRDTSC;

				validFrame = false;
				if ((pActiveFrameStamp->validCount > 0) &&
					(!pActiveFrameStamp->videoTransferPending))
				{
					pActiveFrameStamp->validCount--;
					validFrame = true;
				}

				if (pActiveFrameStamp->validCount == 0)
				{
					bool bDropFrame = false;

					pAuto->framesProcessed++;

					// the frame must not be empty
					if ((pAuto->frameStamp[nextFrame].validCount == 0) ||
						(pAuto->frameStamp[nextFrame].videoTransferPending))
					{
						bDropFrame = true;
					}

					// if synced channel is dropping frames then drop this one
					if (!dropFrame && pNTV2Params->_syncChannels != 0)
					{
						if (DropSyncFrame(deviceNumber, pAuto))
						{
							dropFrame = true;
						}
					}

					if (fieldMode)
					{
						// only start when next field is different
						if (syncField0 == ((pAuto->frameStamp[nextFrame].frameFlags & AUTOCIRCULATE_FRAME_FIELD0) != 0))
						{
							bDropFrame = true;
						}
					}
					
					// it is time to go to the next frame
					if (!bDropFrame)
					{
						// The next frame is ready
						pAuto->frameStamp[nextFrame].frameTime = pAuto->VBIRDTSC;
						pAuto->frameStamp[nextFrame].audioClockTimeStamp = audioCounter;
						pAuto->startAudioClockTimeStamp = audioCounter;
						pAuto->startTimeStamp = pAuto->VBIRDTSC;

						// Advancing to the next frame deprecated due to support for preload 7/23/03
						// // Advance to the next frame
						// added back 5/24 (from windriver: and then added back 3/08/04.)
						WriteRegister(deviceNumber, pAuto->activeFrameRegister, nextFrame, NO_MASK, NO_SHIFT);
						pAuto->nextFrame = nextFrame;
						pAuto->framesProcessed++;

						// Frame Buffer Format
						if (pAuto->enableFbfChange)
						{
							currentFBF = GetFrameBufferFormat(&systemContext, acChannel);
							if (currentFBF != pAuto->frameStamp[nextFrame].frameBufferFormat)
							{
								SetFrameBufferFormat(&systemContext, acChannel, pAuto->frameStamp[nextFrame].frameBufferFormat);
							}
						}

						// Frame Buffer Orientation
						if (pAuto->enableFboChange)
						{
							currentFBO = GetFrameBufferOrientation(&systemContext, acChannel);
							if (currentFBO != pAuto->frameStamp[nextFrame].frameBufferOrientation)
							{
								SetFrameBufferOrientation(&systemContext, acChannel,
														  pAuto->frameStamp[nextFrame].frameBufferOrientation);
							}
						}

						// Start audio playback
						if (pAuto->circulateWithAudio)
						{
							if (validFrame)
							{
								if (IsAudioPlaybackPaused(deviceNumber, pAuto->audioSystem))
								{
									UnPauseAudioPlayback(deviceNumber, pAuto->audioSystem);
								}
								StartAudioPlayback(deviceNumber, pAuto->audioSystem);
								pAuto->frameStamp[nextFrame].audioOutStartAddress = audioOutLastAddress;
								pActiveFrameStamp->audioOutStopAddress = audioOutLastAddress;
							}
							else
							{
								// this will start audio when no valid frames on start
								pAuto->audioDropsRequired = 2;
							}
						}
						else
						{
							if (NTV2_IS_OUTPUT_CROSSPOINT(pautoChannelSpec) &&
								pNTV2Params->_globalAudioPlaybackMode == NTV2_AUDIOPLAYBACK_1STAUTOCIRCULATEFRAME)
							{
								// not using autocirculate for audio but want it to be synced....crazy.
								StartAudioPlayback(deviceNumber, pAuto->audioSystem);
							}
						}

						if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
						{
							CopyFrameStampTCArrayToHardware(&systemContext, &pAuto->frameStamp[nextFrame].internalTCArray);
						}

						if (pAuto->circulateWithHDMIAux)
						{
							oemAutoCirculateWriteHDMIAux(deviceNumber,
														 pAuto->frameStamp[nextFrame].auxData,
														 pAuto->frameStamp[nextFrame].auxDataSize);
						}
						if (pAuto->circulateWithColorCorrection)
						{
							OemAutoCirculateSetupColorCorrector(deviceNumber,
																pAuto->channelSpec,
																&pAuto->frameStamp[nextFrame].colorCorrectionInfo);
						}
						if (pAuto->circulateWithVidProc)
						{
							OemAutoCirculateSetupVidProc(deviceNumber,
														 pAuto->channelSpec,
														 &pAuto->frameStamp[nextFrame].vidProcInfo);
						}
						if (pAuto->circulateWithCustomAncData)
						{
							SetAncInsReadParams(&systemContext, acChannel, nextFrame,
												pAuto->frameStamp[nextFrame].ancTransferSize);
						}

						OemAutoCirculateSetupNTV2Routing(deviceNumber, &pAuto->frameStamp[nextFrame].ntv2RoutingTable);
						
						pAuto->state = NTV2_AUTOCIRCULATE_RUNNING;
					}
				}
				else
				{
					// Not a valid frame
				}
			}
		}
		break;

        case NTV2_AUTOCIRCULATE_RUNNING:
		{
			//
			// Change to RDTSC/Audio handling.  We want to know the audio and
			// time information for the vertical blank at which the record or
			// play started.  Previous code set the info at the end of the
			// audio video capture.
			LWord nextFrame = 0;
			LWord64 timeDiff = 0;
			ULWord numSamplesPerFrame = 0;
			LWord framePeriod = 1;
			bool bDropFrame = false;

			changeEvent = true;
			if (pAuto->recording)
			{
				// RECORD
				pLastFrameStamp = &pAuto->frameStamp[lastActiveFrame];
				
				if (pLastFrameStamp->validCount == 0)
				{
					// Maintain count if dropping
					pLastFrameStamp->validCount = 1;
					if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
					{
						MSG("oemAC recording running, setting activeFrame=%d validCount = 1, was 0\n",
							pAuto->activeFrame);
					}
				}

				// calculate time difference of last two frame interrupts
				timeDiff = (LWord64)pAuto->lastInterruptTime - (LWord64)pAuto->prevInterruptTime;

				// get the current interrupt period
				framePeriod = (LWord)GetFramePeriod(deviceNumber, channel);
				if (GetSmpte372(&systemContext, channel) || fieldMode)
				{
					framePeriod /= 2;
				}
				
				// bail if interrupt period was too short
				if (timeDiff < (LWord64)framePeriod / 5)   // < 20% frame period
				{
					MSG("Auto %s: interrupt too fast, bailing on this one...  duration %lld\n",
						CrosspointName[pAuto->channelSpec], timeDiff);
					return;
				}

				newStartAddress = pLastFrameStamp->audioInStopAddress;
				if (pAuto->circulateWithAudio)
				{
					// calculate the error in the video interrupt period
					LWord discontinuityTime = 10000000;
					if (timeDiff > 0 && timeDiff < 10000000)
					{
						discontinuityTime = (LWord)timeDiff - framePeriod;
					}

					// if video interrupt period error less than 30% of audio sync tolerance then check audio sync
					if (abs(discontinuityTime) < (LWord)pNTV2Params->_audioSyncTolerance * 4)
					{
						// get the current audio address
						ULWord actualLastIn = GetAudioLastIn(deviceNumber, pAuto->audioSystem);
						ULWord startAddress = newStartAddress - GetAudioReadOffset(deviceNumber, pAuto->audioSystem);
						// calculate the difference between the expected start address and the actual address
						// Don't call abs with unsigned arguments. If subtract is "negative"
						// delta will be assigned a huge positive value.
						int32_t sgnStart = (int32_t) startAddress;
						int32_t sgnLast  = (int32_t) actualLastIn;
						ULWord64 delta = abs(sgnLast - sgnStart);
						ULWord64 time = 0;
						if (delta > GetAudioWrapAddress(deviceNumber, pAuto->audioSystem) / 2)
						{
							delta = GetAudioWrapAddress(deviceNumber, pAuto->audioSystem) - delta;
						}
						// convert the address delta to a time delta
						time = delta * 10000;
						do_div(time,
							   ((GetAudioSamplesPerSecond(deviceNumber, pAuto->audioSystem)/1000) *
								(GetNumAudioChannels(deviceNumber, pAuto->audioSystem) * AUDIO_IN_SAMPLE_SIZE)));

						if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
						{
							if ((pAuto->framesProcessed % 100) == 0)
							{
								MSG("Auto %s:  frame %d  drops %d  audio sync %lld\n",
									CrosspointName[pAuto->channelSpec], pAuto->framesProcessed, pAuto->droppedFrames, time);
							}
						}
						
						// if the time difference is larger than the tolerance correct the expected start address
						if (time > (ULWord64)pNTV2Params->_audioSyncTolerance * 10)
						{
							newStartAddress = oemAudioSampleAlignIn(
								deviceNumber,
								pAuto->audioSystem, actualLastIn) + GetAudioReadOffset(deviceNumber, pAuto->audioSystem);
							if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
							{
								MSG("Auto %s:  frame %d  correct audio sync start %d  actual %d  time %lld\n",
									CrosspointName[pAuto->channelSpec], pAuto->framesProcessed, startAddress, actualLastIn, time);
							}
						}
					}
				}

				// save timing and audio data
				pActiveFrameStamp->audioInStartAddress = newStartAddress;
				pActiveFrameStamp->audioClockTimeStamp = audioCounter;
				pActiveFrameStamp->frameTime = pAuto->VBIRDTSC;

				// use the correct frame counter to determine number of audio samples per frame
				if (lastActiveFrame != pAuto->activeFrame)
				{
					numSamplesPerFrame = GetAudioSamplesPerFrame(deviceNumber,
																 pAuto->audioSystem,
																 pAuto->framesProcessed,
																 AUDIO_OUT_GRANULARITY,
																 fieldMode);
					}
					else
					{
						numSamplesPerFrame = GetAudioSamplesPerFrame(deviceNumber,
																	 pAuto->audioSystem,
																	 pAuto->droppedFrames,
																	 AUDIO_OUT_GRANULARITY,
																	 fieldMode);
					}

					// calculate the last audio address for the current frame
				newInputAddress = GetAudioTransferInfo(
					deviceNumber,
					pAuto->audioSystem,
					pActiveFrameStamp->audioInStartAddress - GetAudioReadOffset(deviceNumber, pAuto->audioSystem),
					numSamplesPerFrame*GetNumAudioChannels(deviceNumber, pAuto->audioSystem)*AUDIO_IN_SAMPLE_SIZE,
					&pActiveFrameStamp->audioPreWrapBytes,
					&pActiveFrameStamp->audioPostWrapBytes);
				if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("ISR->AC->recording: numSamplesPerFrame=%d numAudioChannels=%d bytesToXfer=%d newInputAddress=0x%X\n",
						numSamplesPerFrame, GetNumAudioChannels(deviceNumber, pAuto->audioSystem),
						(numSamplesPerFrame*GetNumAudioChannels(deviceNumber, pAuto->audioSystem)*AUDIO_IN_SAMPLE_SIZE),
						newInputAddress);
				}
				pActiveFrameStamp->audioInStopAddress = newInputAddress + GetAudioReadOffset(deviceNumber, pAuto->audioSystem);

				// set the frame flags
				if(fieldMode)
					pActiveFrameStamp->frameFlags = syncField0? AUTOCIRCULATE_FRAME_FIELD0 : AUTOCIRCULATE_FRAME_FIELD1;
				else 
					pActiveFrameStamp->frameFlags = AUTOCIRCULATE_FRAME_FULL;

				// if synced channel is dropping frames then drop this one
				dropFrame = false;
				if (pNTV2Params->_syncChannels != 0)
				{
					if (pAuto->channelSpec == pNTV2Params->_syncChannel1)
					{
						if (pNTV2Params->_AutoCirculate[pNTV2Params->_syncChannel1].droppedFrames <
							pNTV2Params->_AutoCirculate[pNTV2Params->_syncChannel2].droppedFrames)
						{
							dropFrame = true;
						}
					}
					if (pAuto->channelSpec == pNTV2Params->_syncChannel2)
					{
						if (pNTV2Params->_AutoCirculate[pNTV2Params->_syncChannel2].droppedFrames <
							pNTV2Params->_AutoCirculate[pNTV2Params->_syncChannel1].droppedFrames)
						{
							dropFrame = true;
						}
					}
				}

				if (fieldMode)
				{
					// do not repeat the same field when dropping
					ULWord fieldFlags = AUTOCIRCULATE_FRAME_FIELD0 | AUTOCIRCULATE_FRAME_FIELD1;
					LWord prevFrame = KAUTO_PREVFRAME(pAuto->activeFrame, pAuto);
					if ((pAuto->frameStamp[prevFrame].frameFlags & fieldFlags) == (pActiveFrameStamp->frameFlags & fieldFlags))
					{
						bDropFrame = true;
					}
				}

				// Frame complete, move on if possible
				nextFrame = KAUTO_NEXTFRAME(pAuto->activeFrame, pAuto);

				if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("ISR->AC->recording: activeFrame=%ul complete, nextFrame=%ul, validCount nextFrame=%d\n",
						pAuto->activeFrame, nextFrame, pAuto->frameStamp[nextFrame].validCount);
				}

				// update rp188 data for captured frame
				if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
				{
					CopyRP188HardwareToFrameStampTCArray(&systemContext, &pAuto->frameStamp[lastActiveFrame].internalTCArray);
				}

				if (NTV2DeviceCanDoSDIErrorChecks(pNTV2Params->_DeviceID))
				{
					CopySDIStatusHardwareToFrameStampSDIStatusArray(&systemContext, &pAuto->frameStamp[lastActiveFrame].internalSDIStatusArray);
				}

				if (pAuto->circulateWithCustomAncData)
				{
					if (pAuto->circulateWithHDMIAux)
					{
						pAuto->frameStamp[lastActiveFrame].ancTransferSize = GetAuxExtField1Bytes(&systemContext, acChannel);
						pAuto->frameStamp[lastActiveFrame].ancField2TransferSize = GetAuxExtField2Bytes(&systemContext, acChannel);
						SetAuxExtWriteParams(&systemContext, acChannel, nextFrame);
					}
					else
					{
						pAuto->frameStamp[lastActiveFrame].ancTransferSize = GetAncExtField1Bytes(&systemContext, acChannel);
						pAuto->frameStamp[lastActiveFrame].ancField2TransferSize = GetAncExtField2Bytes(&systemContext, acChannel);
						SetAncExtWriteParams(&systemContext, acChannel, nextFrame);
					}
				}
				if ((pAuto->frameStamp[nextFrame].validCount == 0) && !dropFrame)
				{
					// Advance to next frame for capture.
					WriteRegister(deviceNumber, pAuto->activeFrameRegister, nextFrame, NO_MASK, NO_SHIFT);
					pAuto->nextFrame = nextFrame;

					// Increment frames processed
					pAuto->framesProcessed++;
				}
				else
				{
					// Application not reading frames fast enough.
					// This may be a temporary state during non record.  Need to keep active status
					// in this case.  User can see droppedFrames increment to indicate problem.
					pAuto->droppedFrames++;
					if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
					{
						MSG("oemAC: dropped a frame, count %d", pAuto->droppedFrames); /* 64bit */
					}
					// Tell user this is the frame the drop occurred at (returned in
					// frameStamp->currentReps
					pActiveFrameStamp->validCount++;

					if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
					{
						MSG("oemAC - recording running: dropped frame, setting nextFrame=%d validCount = %d\n",
							nextFrame, pActiveFrameStamp->validCount);
					}
				}
			}
			else
			{
				// PLAY
				if (pActiveFrameStamp->validCount > 0) {
					pActiveFrameStamp->validCount--;
					if (pActiveFrameStamp->validCount == 0) {
						// Record audio out point the first time we are at 0
						pActiveFrameStamp->audioOutStopAddress = audioOutLastAddress;
					}
					// Mark each frame so timeStamp + (validCount * frametime) is always valid
					pActiveFrameStamp->frameTime = pAuto->VBIRDTSC;
					pActiveFrameStamp->audioClockTimeStamp = audioCounter;
				}

				// calculate time difference of last two frame interrupts
				timeDiff = (LWord64)pAuto->lastInterruptTime - (LWord64)pAuto->prevInterruptTime;

				// If validCount is used to go negative to indicate dropping position,
				// this would have to be <= 0
				if (pActiveFrameStamp->validCount == 0)
				{
					LWord nextFrame = KAUTO_NEXTFRAME(pAuto->activeFrame,pAuto);
					if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
					{
						MSG("ISR->AC->Play: activeFrame=%d, nextFrame=%d, nextValidCount=%d\n",
							pAuto->activeFrame, nextFrame, pAuto->frameStamp[nextFrame].validCount);
					}
					
					// the frame must be empty
					dropFrame = false;
					if ((pAuto->frameStamp[nextFrame].validCount == 0) ||
						(pAuto->frameStamp[nextFrame].videoTransferPending))
					{
						dropFrame = true;
					}

					// if synced channel is dropping frames then drop this one
					if (!dropFrame && pNTV2Params->_syncChannels != 0)
					{
						if (DropSyncFrame(deviceNumber, pAuto))
						{
							dropFrame = true;
						}
					}

					if (fieldMode)
					{
						// only move on when next field is different
						if (syncField0 == ((pAuto->frameStamp[nextFrame].frameFlags & AUTOCIRCULATE_FRAME_FIELD0) != 0))
						{
							bDropFrame = true;
						}
					}

					if (pAuto->circulateWithAudio)
					{
						// record audio address to play during frame
						//pAuto->frameStamp[nextFrame].audioOutStartAddress = audioOutLastAddress;
						
						if (IsAudioPlaybackStopped(deviceNumber, pAuto->audioSystem))
						{
							if ((pActiveFrameStamp->audioExpectedAddress == 0) &&
								(pAuto->audioDropsRequired == pAuto->audioDropsCompleted))
							{
								UnPauseAudioPlayback(deviceNumber, pAuto->audioSystem);
								StartAudioPlayback(deviceNumber, pAuto->audioSystem);
								//pAuto->startAudioNextFrame = true;
							}
						}
					}

					// it is time to go to the next frame
					if (!dropFrame)
					{
						// The next frame is ready
						pAuto->frameStamp[nextFrame].frameTime = pAuto->VBIRDTSC;
						pAuto->frameStamp[nextFrame].audioClockTimeStamp = audioCounter;

						// Move to it
						WriteRegister(deviceNumber, pAuto->activeFrameRegister, nextFrame, NO_MASK, NO_SHIFT);
						pAuto->nextFrame = nextFrame;

						pAuto->framesProcessed++; /// will go on air next vertical.

						// If we are supposed to dynamically change the frameBufferFormat to match
						// what has been sent down with the video frame in the dma
						if (pAuto->enableFbfChange)
						{
							currentFBF = GetFrameBufferFormat(&systemContext, acChannel);
							if (currentFBF != pAuto->frameStamp[nextFrame].frameBufferFormat)
							{
								SetFrameBufferFormat(&systemContext, acChannel, pAuto->frameStamp[nextFrame].frameBufferFormat);
							}
						}

						// frame buffer orientation
						if (pAuto->enableFboChange)
						{
							currentFBO = GetFrameBufferOrientation(&systemContext, acChannel);
							if (currentFBO != pAuto->frameStamp[nextFrame].frameBufferOrientation)
							{
								SetFrameBufferOrientation(&systemContext,
														  acChannel, pAuto->frameStamp[nextFrame].frameBufferOrientation);
							}
						}

						if (pAuto->circulateWithAudio)
						{
							pAuto->frameStamp[nextFrame].audioOutStartAddress = audioOutLastAddress;
							if (IsAudioPlaybackPaused(deviceNumber, pAuto->audioSystem))
							{
								UnPauseAudioPlayback(deviceNumber, pAuto->audioSystem);
							}

							if (IsAudioPlaybackStopped(deviceNumber, pAuto->audioSystem))
							{
								if ((pActiveFrameStamp->audioExpectedAddress == 0) &&
									(pAuto->audioDropsRequired == pAuto->audioDropsCompleted))
								{
									StartAudioPlayback(deviceNumber, pAuto->audioSystem);
								}
							}
							else
							{
								// calculate the error in the video interrupt period
								ULWord64 time = 0;
								LWord framePeriod = (LWord)GetFramePeriod(deviceNumber, syncChannel);
								LWord discontinuityTime = 10000000;
								if (GetSmpte372(&systemContext, syncChannel) || fieldMode)
								{
									framePeriod /= 2;
								}
								if (timeDiff > 0 && timeDiff < 10000000)
								{
									discontinuityTime = (LWord)timeDiff - framePeriod;
								}
								
								// if video interrupt period error less than 30% of audio sync tolerance then check audio sync
								if (abs(discontinuityTime) < (LWord)pNTV2Params->_audioSyncTolerance * 4)   // interrupt discontinuity
								{
									// calculate the difference between the expected start address and the actual address
									ULWord startAddress = pActiveFrameStamp->audioExpectedAddress;
									// Don't call abs with unsigned arguments. If subtract is "negative"
									// delta will be assigned a huge positive value.
									int32_t sgnStart = (int32_t) startAddress;
									int32_t sgnLast  = (int32_t) audioOutLastAddress;
									ULWord64 delta = abs(sgnLast - sgnStart);
									if (delta > GetAudioWrapAddress(deviceNumber, pAuto->audioSystem) / 2)
									{
										delta = GetAudioWrapAddress(deviceNumber, pAuto->audioSystem) - delta;
									}
									// convert the address delta to a time delta
									time = delta * 10000;
									do_div(time, ((GetAudioSamplesPerSecond(deviceNumber, pAuto->audioSystem)/1000) *
												  (GetNumAudioChannels(deviceNumber, pAuto->audioSystem) * AUDIO_OUT_SAMPLE_SIZE)));
									if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
									{
										if ((pAuto->framesProcessed % 100) == 0)
										{
											MSG("Auto %s:  frame %d  drops %d  audio sync %lld\n",
												CrosspointName[pAuto->channelSpec],
												pAuto->framesProcessed, pAuto->droppedFrames, time);
										}
									}

									// if the time difference is larger than the tolerance restart the audio
									if (time > (ULWord64)pNTV2Params->_audioSyncTolerance * 10)
									{
										StopAudioPlayback(deviceNumber, pAuto->audioSystem);
										pAuto->audioDropsRequired++;
										if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
										{
											MSG("Auto %s:  frame %d  correct audio sync start %d  actual %d  time %lld\n",
												CrosspointName[pAuto->channelSpec], pAuto->framesProcessed,
												startAddress, audioOutLastAddress, time);
										}
									}
								}
							}
						}

						if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES)
							|| MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
						{
							// TODO - is this accurate after the new timing/audio update?  -jac 5/24/2006
							MSG("Playframe=%lu,slot=%ld,audioExpected=%lu,audioStart=%lu,audioStop=%lu,total=%lu,diff=%ld\n",
	  							(long unsigned)pActiveFrameStamp->hUser,
								(long)pAuto->activeFrame,
								(long unsigned)pActiveFrameStamp->audioExpectedAddress,
	  							(long unsigned)pActiveFrameStamp->audioOutStartAddress,
								(long unsigned)pActiveFrameStamp->audioOutStopAddress,
	  							(long unsigned)(pActiveFrameStamp->audioOutStopAddress-pActiveFrameStamp->audioOutStartAddress),
	  							(long)pActiveFrameStamp->audioOutStartAddress-(long)pActiveFrameStamp->audioExpectedAddress);
						}

						if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
						{
							CopyFrameStampTCArrayToHardware(&systemContext, &pAuto->frameStamp[nextFrame].internalTCArray);
						}

						if (pAuto->circulateWithHDMIAux)
						{
							oemAutoCirculateWriteHDMIAux(deviceNumber,
														 pAuto->frameStamp[nextFrame].auxData,
														 pAuto->frameStamp[nextFrame].auxDataSize);
						}
						if (pAuto->circulateWithColorCorrection)
						{
							OemAutoCirculateSetupColorCorrector(deviceNumber,
																pAuto->channelSpec,
																&pAuto->frameStamp[nextFrame].colorCorrectionInfo);
						}
						if (pAuto->circulateWithVidProc)
						{
							OemAutoCirculateSetupVidProc(deviceNumber,
														 pAuto->channelSpec,
														 &pAuto->frameStamp[nextFrame].vidProcInfo);
						}
						if (pAuto->circulateWithCustomAncData)
						{
							SetAncInsReadParams(&systemContext, acChannel, nextFrame,
												pAuto->frameStamp[nextFrame].ancTransferSize);
						}
						OemAutoCirculateSetupNTV2Routing(deviceNumber, &pAuto->frameStamp[nextFrame].ntv2RoutingTable);
					}
					else
					{
						// Application not supplying frames fast enough.
						// This may be a temporary state during non record.  Need to keep active status
						// in this case.  User can see droppedFrames increment to indicate problem.
						pAuto->droppedFrames++;
						if (pAuto->circulateWithAudio)
						{
							if (!IsAudioPlaybackStopped(deviceNumber, pAuto->audioSystem))
							{
								StopAudioPlayback(deviceNumber, pAuto->audioSystem);
								pAuto->audioDropsRequired++;
							}
						}
					}
				}
			}
		}
		break;

        case NTV2_AUTOCIRCULATE_STOPPING:
		{
            if (pAuto->recording)
            {
				if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("State:NTV2_AUTOCIRCULATE_STOPPING(record)\n");
				}
				if (pAuto->circulateWithAudio)
				{
	                StopAudioCapture(deviceNumber, pAuto->audioSystem);
					while (pAuto->audioSystemCount)
					{
						ntv2WriteRegisterMS(&systemContext, GetAudioControlRegister(deviceNumber, (NTV2AudioSystem)(pAuto->audioSystemCount - 1)),
											0, kRegMaskMultiLinkAudio, kRegShiftMultiLinkAudio);
						pAuto->audioSystemCount--;
					}
				}
				if (pAuto->circulateWithCustomAncData)
				{
					if (pAuto->circulateWithHDMIAux)
					{
						EnableAuxExtractor(&systemContext, acChannel, false);
					}
					else
					{
						EnableAncExtractor(&systemContext, acChannel, false);
					}
				}
            }
            else
            {
				if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
				{
					MSG("State:NTV2_AUTOCIRCULATE_STOPPING(play)\n");
				}
				if (pAuto->circulateWithAudio)
				{
	                StopAudioPlayback(deviceNumber, pAuto->audioSystem);
					while (pAuto->audioSystemCount)
					{
						ntv2WriteRegisterMS(&systemContext, GetAudioControlRegister(deviceNumber, (NTV2AudioSystem)(pAuto->audioSystemCount - 1)),
											0, kRegMaskMultiLinkAudio, kRegShiftMultiLinkAudio);
						pAuto->audioSystemCount--;
					}
				}
				if (pAuto->circulateWithCustomAncData)
				{
					EnableAncInserter(&systemContext, acChannel, false);
				}
				if (pAuto->circulateWithRP188)
				{
					//Nothing to do here
				}
            }
            pAuto->state = NTV2_AUTOCIRCULATE_DISABLED;
            WriteRegister(deviceNumber, kVRegChannelCrosspointFirst + GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec),
						  NTV2CROSSPOINT_INVALID, NO_MASK, NO_SHIFT);
            break;
		}

		case NTV2_AUTOCIRCULATE_PAUSED:
		{
//            pAuto->activeFrame = lastActiveFrame;
			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
			{
				MSG("PAUSED: OemAutoCirculate(): lastActiveFrame = %d, pAuto->activeFrame = %d\n",
					lastActiveFrame, pAuto->activeFrame);
			}
			if (pAuto->circulateWithAudio)
			{
				if (!pAuto->recording)
				{
					PauseAudioPlayback(deviceNumber, pAuto->audioSystem);
				}
			}
            break;
		}
		
		case NTV2_AUTOCIRCULATE_DISABLED:
        default:
            // nothing needs to be done.
            break;
        }
	}
    else if ((pAuto->state == NTV2_AUTOCIRCULATE_STARTING ||
			  pAuto->state == NTV2_AUTOCIRCULATE_RUNNING) &&
			 pAuto->circulateWithCustomAncData)
	{
		if (pAuto->recording)
		{
			if (pAuto->circulateWithHDMIAux)
			{
				SetAuxExtField2WriteParams(&systemContext, acChannel, pAuto->nextFrame);
			}
			else
			{
				SetAncExtField2WriteParams(&systemContext, acChannel, pAuto->nextFrame);
			}
		}
		else
		{
			SetAncInsReadField2Params(&systemContext, acChannel, pAuto->nextFrame, pAuto->frameStamp[pAuto->nextFrame].ancField2TransferSize);
		}
	}
}


int
AutoCirculateControl(ULWord deviceNumber, AUTOCIRCULATE_DATA *pACData)
{
	int status;
	unsigned long flags = 0;
	NTV2PrivateParams* pNTV2Params;
	NTV2Crosspoint channel = NTV2CROSSPOINT_FGKEY;
	bool bDoSync = false;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);

	switch (pACData->eCommand)
	{
	case eInitAutoCirc:
		status =  OemAutoCirculateInit(deviceNumber,
									   pACData->channelSpec,
									   pACData->lVal1,
									   pACData->lVal2,
									   (NTV2AudioSystem)(pACData->lVal3 & NTV2AudioSystemRemoveValues),
									   pACData->lVal4,
									   pACData->bVal1,
									   pACData->bVal2,
									   pACData->bVal3,
									   pACData->bVal4,
									   pACData->bVal5,
									   pACData->bVal6,
									   pACData->bVal7,
									   pACData->bVal8,
									   ((pACData->lVal6 & AUTOCIRCULATE_WITH_FIELDS) != 0),
									   ((pACData->lVal6 & AUTOCIRCULATE_WITH_HDMIAUX) != 0),
									   ((pACData->lVal3 & NTV2_AUDIOSYSTEM_Plus1) != 0),
									   ((pACData->lVal3 & NTV2_AUDIOSYSTEM_Plus2) != 0),
									   ((pACData->lVal3 & NTV2_AUDIOSYSTEM_Plus3) != 0));
		break;

	case eStartAutoCirc:
		status = OemAutoCirculateStart(deviceNumber, pACData->channelSpec, 0);
		break;

	case eStartAutoCircAtTime:
	{
		ULWord64 high = (ULWord)pACData->lVal1;
		ULWord64 low = (ULWord)pACData->lVal2;
		status = OemAutoCirculateStart(deviceNumber, pACData->channelSpec, (LWord64)((high << 32) | low));
		break;
	}
	case eStopAutoCirc:
		status = OemAutoCirculateStop(deviceNumber, pACData->channelSpec);
		break;

	case eAbortAutoCirc:
		status = OemAutoCirculateAbort(deviceNumber, pACData->channelSpec);
		break;

	case ePauseAutoCirc:
		status = OemAutoCirculatePause(deviceNumber,
									   pACData->channelSpec,
									   pACData->bVal1,
									   pACData->bVal2);
		break;

	case eFlushAutoCirculate:
        status = OemAutoCirculateFlush(deviceNumber,
									   pACData->channelSpec,
									   pACData->bVal1);
		break;

	case ePrerollAutoCirculate:
		status = OemAutoCirculatePreroll(deviceNumber,
										 pACData->channelSpec,
										 pACData->lVal1);
		break;
		
	default:
		MSG("Unsupported command in AutoCirculateControl() :%d", pACData->eCommand);
		status = -EINVAL;
		break;
	}
	
	// sync channels
	if (pNTV2Params->_syncChannels != 0)
	{
		INTERNAL_AUTOCIRCULATE_STRUCT* pAuto;
		int iChannel;
		for (iChannel  = GetIndexForNTV2Crosspoint(NTV2CROSSPOINT_CHANNEL1);
			iChannel <= GetIndexForNTV2Crosspoint(NTV2CROSSPOINT_INPUT8);
			iChannel++)
		{
			if (iChannel == GetIndexForNTV2Crosspoint(pACData->channelSpec))
			{
				continue;
			}
			pAuto = &pNTV2Params->_AutoCirculate[iChannel];
			if (pAuto->state != NTV2_AUTOCIRCULATE_DISABLED && pAuto->state != NTV2_AUTOCIRCULATE_STOPPING)
			{
				channel = GetNTV2CrosspointForIndex(iChannel);
				pNTV2Params->_syncChannel1 = pACData->channelSpec;
				pNTV2Params->_syncChannel2 = channel;
				bDoSync = true;
				break;
			}
		}
	}
	else
	{
		pNTV2Params->_syncChannel1 = NTV2CROSSPOINT_FGKEY;
		pNTV2Params->_syncChannel2 = NTV2CROSSPOINT_FGKEY;
	}

	if (bDoSync)
	{
		switch (pACData->eCommand)
		{
		case eInitAutoCirc:
			break;

		case eStartAutoCirc:
			status = OemAutoCirculateStart(deviceNumber, channel, 0);
			break;

		case eStartAutoCircAtTime:
		{
			ULWord64 high = (ULWord)pACData->lVal1;
			ULWord64 low = (ULWord)pACData->lVal2;
			status = OemAutoCirculateStart(deviceNumber, channel, (LWord64)((high << 32) | low));
			break;
		}
		case eStopAutoCirc:
			status = OemAutoCirculateStop(deviceNumber, channel);
			break;

		case eAbortAutoCirc:
			status = OemAutoCirculateAbort(deviceNumber, channel);
			break;

		case ePauseAutoCirc:
			status = OemAutoCirculatePause(deviceNumber,
										   channel,
										   pACData->bVal1,
										   pACData->bVal2);
			break;

		case eFlushAutoCirculate:
            status = OemAutoCirculateFlush(deviceNumber,
										   channel,
										   pACData->bVal1);
			break;

		case ePrerollAutoCirculate:
			status = OemAutoCirculatePreroll(deviceNumber,
											 channel,
											 pACData->lVal1);
			break;

		default:
			MSG("Unsupported command in AutoCirculateControl() :%d", pACData->eCommand);
			status = -EINVAL;
			break;
		}
	}

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_autoCirculateLock, flags);

	return status;
}

int
AutoCirculateStatus(ULWord deviceNumber, AUTOCIRCULATE_STATUS_STRUCT *acStatus)
{
	NTV2PrivateParams* pNTV2Params;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(acStatus->channelSpec))
	    return -ECHRNG;

	pAuto = &pNTV2Params->_AutoCirculate[acStatus->channelSpec];

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
	{
		MSG("Received IOCTL_NTV2_AUTOCIRCULATE_STATUS\n"
			"for deviceNumber %d, channel %d\n",
			deviceNumber, acStatus->channelSpec);
		MSG("State of internal autocirculate status:\n");
		SHOW_INTERNAL_AUTOCIRCULATE_STRUCT(pAuto);
	}

	acStatus->state					= pAuto->state;
	acStatus->startFrame			= pAuto->startFrame;
	acStatus->endFrame				= pAuto->endFrame;
	acStatus->activeFrame			= pAuto->activeFrame;
	acStatus->rdtscStartTime		= pAuto->startTimeStamp;
	my_rdtscll(acStatus->rdtscCurrentTime);
	acStatus->audioClockStartTime 	= pAuto->startAudioClockTimeStamp;
	acStatus->audioClockCurrentTime = GetAudioClock(deviceNumber);
	acStatus->framesProcessed		= pAuto->framesProcessed;
	acStatus->framesDropped			= pAuto->droppedFrames;
	acStatus->bufferLevel			= OemAutoCirculateGetBufferLevel(pAuto);
	acStatus->bWithAudio			= pAuto->circulateWithAudio;
	acStatus->bWithRP188			= pAuto->circulateWithRP188;
    acStatus->bFbfChange			= pAuto->enableFbfChange;
    acStatus->bFboChange			= pAuto->enableFboChange ;
	acStatus->bWithColorCorrection	= pAuto->circulateWithColorCorrection;
	acStatus->bWithVidProc			= pAuto->circulateWithVidProc;
	acStatus->bWithCustomAncData	= pAuto->circulateWithCustomAncData;
	return 0;
}

int
AutoCirculateStatus_Ex(ULWord deviceNumber, AUTOCIRCULATE_STATUS *acStatus)
{
	NTV2PrivateParams* pNTV2Params;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(acStatus->acCrosspoint))
	    return -ECHRNG;

	pAuto = &pNTV2Params->_AutoCirculate[acStatus->acCrosspoint];

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
	{
		MSG("Received IOCTL_NTV2_AUTOCIRCULATE_STATUS\n"
			"for deviceNumber %d, channel %d\n",
			deviceNumber, acStatus->acCrosspoint);
		MSG("State of internal autocirculate status:\n");
		SHOW_INTERNAL_AUTOCIRCULATE_STRUCT(pAuto);
	}

	acStatus->acState = pAuto->state;
	acStatus->acStartFrame = pAuto->startFrame;
	acStatus->acEndFrame = pAuto->endFrame;
	acStatus->acActiveFrame = pAuto->activeFrame;
	acStatus->acRDTSCStartTime = pAuto->startTimeStamp;
	acStatus->acAudioClockStartTime = pAuto->startAudioClockTimeStamp;
	my_rdtscll(acStatus->acRDTSCCurrentTime);
	acStatus->acAudioClockCurrentTime = GetAudioClock(deviceNumber);
	acStatus->acFramesProcessed = pAuto->framesProcessed;
	acStatus->acFramesDropped = pAuto->droppedFrames;
	acStatus->acBufferLevel = OemAutoCirculateGetBufferLevel(pAuto);
	
	acStatus->acAudioSystem = pAuto->circulateWithAudio ? pAuto->audioSystem : NTV2_AUDIOSYSTEM_INVALID;
	
	acStatus->acOptionFlags = 0;
	acStatus->acOptionFlags = (
		pAuto->circulateWithRP188 ? AUTOCIRCULATE_WITH_RP188 : 0) |
		(pAuto->circulateWithLTC ? AUTOCIRCULATE_WITH_LTC : 0) |
		(pAuto->enableFbfChange ? AUTOCIRCULATE_WITH_FBFCHANGE : 0) |
		(pAuto->enableFboChange ? AUTOCIRCULATE_WITH_FBOCHANGE : 0) |
		(pAuto->circulateWithColorCorrection ? AUTOCIRCULATE_WITH_COLORCORRECT : 0) |
		(pAuto->circulateWithVidProc ? AUTOCIRCULATE_WITH_VIDPROC : 0) |
		(pAuto->circulateWithCustomAncData ? AUTOCIRCULATE_WITH_ANC : 0) |
		(pAuto->circulateWithFields ? AUTOCIRCULATE_WITH_FIELDS : 0) |
		(pAuto->circulateWithHDMIAux ? AUTOCIRCULATE_WITH_HDMIAUX : 0);
	
	return 0;
}

int
AutoCirculateFrameStamp(ULWord deviceNumber, AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT *pFrameStampCombo)
{
	// Determine requested frame
	FRAME_STAMP_STRUCT* pFrameStamp = &(pFrameStampCombo->acFrameStamp);
	AUTOCIRCULATE_TASK_STRUCT*	pTask = &(pFrameStampCombo->acTask);
	ULWord ulFrameNum = pFrameStamp->frame;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;
	NTV2PrivateParams* pNTV2Params;
    LWord lCurrentFrame;
    bool bField0;
	NTV2Crosspoint channelSpec = pFrameStamp->channelSpec;
	ULWord64 RDTSC = 0;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	// Validate requested channel number
	if (ILLEGAL_CHANNELSPEC(channelSpec))
	    return -ECHRNG;
	
	pAuto = &pNTV2Params->_AutoCirculate[channelSpec];

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
	{
		INTERNAL_FRAME_STAMP_STRUCT *pInternalFrameStamp;
		MSG("Received IOCTL_NTV2_AUTOCIRCULATE_FRAMESTAMP\n"
			"State of internal frame stamp:\n");

		pInternalFrameStamp = &(pAuto->frameStamp[ulFrameNum]);
		SHOW_INTERNAL_FRAME_STAMP_STRUCT(pInternalFrameStamp);
	}

	if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING &&
        pAuto->state != NTV2_AUTOCIRCULATE_STARTING &&
        pAuto->state != NTV2_AUTOCIRCULATE_PAUSED)
	{
		memset(pFrameStamp, 0, sizeof(FRAME_STAMP_STRUCT));
		pFrameStamp->currentFrame = NTV2_INVALID_FRAME;
		// Always can set these
		pFrameStamp->audioClockCurrentTime = GetAudioClock(deviceNumber);
		my_rdtscll(RDTSC);	// Pointer not required
		pFrameStamp->currentTime = RDTSC;
		pFrameStamp->currentLineCount = ReadRegister(deviceNumber, kRegLineCount, NO_MASK, NO_SHIFT);
		pFrameStamp->currentFrameTime = pAuto->VBILastRDTSC;

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
		{
			MSG("State of returned frame stamp:\n");
			SHOW_FRAME_STAMP_STRUCT(pFrameStamp);
		}
		return 0;
	}

    if (ulFrameNum < pAuto->startFrame && ulFrameNum > pAuto->endFrame)
        ulFrameNum = NTV2_INVALID_FRAME;
	
    if (ulFrameNum != NTV2_INVALID_FRAME /*&& pAuto->frameStamp[ulFrameNum].validCount != 0*/)
	{
		pFrameStamp->frame = ulFrameNum;
        pFrameStamp->frameTime              = pAuto->frameStamp[ulFrameNum].frameTime;
        pFrameStamp->audioClockTimeStamp    = pAuto->frameStamp[ulFrameNum].audioClockTimeStamp;
        pFrameStamp->audioExpectedAddress   = pAuto->frameStamp[ulFrameNum].audioExpectedAddress;
		// For record, use the values captured by the ISR/DPC
        pFrameStamp->audioInStartAddress    = pAuto->frameStamp[ulFrameNum].audioInStartAddress;
        pFrameStamp->audioInStopAddress     = pAuto->frameStamp[ulFrameNum].audioInStopAddress;
		// For play, give the next start in both locations.  The user will
		// decide the amount to send
        pFrameStamp->audioOutStartAddress   = pAuto->frameStamp[ulFrameNum].audioOutStartAddress;
        pFrameStamp->audioOutStopAddress    = pAuto->frameStamp[ulFrameNum].audioOutStopAddress;
		if (pTask != NULL)
		{
			oemAutoCirculateTaskTransfer(&pAuto->frameStamp[ulFrameNum].taskInfo, pTask, false);
		}
	} else {
		// Error
		pFrameStamp->frame = NTV2_INVALID_FRAME;
        pFrameStamp->frameTime              = 0;
        pFrameStamp->audioClockTimeStamp    = 0;
        pFrameStamp->audioExpectedAddress   = 0;
        pFrameStamp->audioInStartAddress    = 0;
        pFrameStamp->audioInStopAddress     = 0;
        pFrameStamp->audioOutStartAddress   = 0;
        pFrameStamp->audioOutStopAddress    = 0;
	}

    lCurrentFrame = pAuto->activeFrame;
    if (lCurrentFrame < pAuto->startFrame || lCurrentFrame > pAuto->endFrame)
        lCurrentFrame = pAuto->startFrame;

    pFrameStamp->currentFrame = lCurrentFrame;
    pFrameStamp->audioClockCurrentTime = GetAudioClock(deviceNumber);
	my_rdtscll(RDTSC);	// Pointer not required
	pFrameStamp->currentTime = RDTSC;
	
    pFrameStamp->currentRP188 = pAuto->frameStamp[lCurrentFrame].rp188;

    if (pAuto->recording) {
		// In record this is correct
		pFrameStamp->currentFrameTime = pAuto->frameStamp[lCurrentFrame].frameTime;
	} else {
		// In play, this has not been set until frame moves on
		pFrameStamp->currentFrameTime = pAuto->VBILastRDTSC;
	}

    bField0 = IsField0(deviceNumber, channelSpec);
    if (bField0)
        pFrameStamp->currentFieldCount = 0;
    else
        pFrameStamp->currentFieldCount = 1;
	
    if (pAuto->recording)
    {
        pFrameStamp->currentAudioExpectedAddress = pAuto->frameStamp[lCurrentFrame].audioInStartAddress;
        pFrameStamp->currentAudioStartAddress    = pAuto->frameStamp[lCurrentFrame].audioInStartAddress;
        pFrameStamp->audioClockCurrentTime       = pAuto->frameStamp[lCurrentFrame].audioClockTimeStamp;
    }
    else
    {
        pFrameStamp->currentAudioExpectedAddress = pAuto->frameStamp[lCurrentFrame].audioExpectedAddress;
        pFrameStamp->currentAudioStartAddress    = pAuto->frameStamp[lCurrentFrame].audioOutStartAddress;
        pFrameStamp->audioClockCurrentTime       = pAuto->frameStamp[lCurrentFrame].audioClockTimeStamp;
    }

    pFrameStamp->currentLineCount = ReadRegister(deviceNumber, kRegLineCount, NO_MASK, NO_SHIFT);
    pFrameStamp->currentReps  = pAuto->frameStamp[lCurrentFrame].validCount;
    pFrameStamp->currenthUser = pAuto->frameStamp[lCurrentFrame].hUser;

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_DEBUG_MESSAGES))
	{
		MSG("State of returned frame stamp:\n");
		SHOW_FRAME_STAMP_STRUCT(pFrameStamp);
	}

	return 0;
}

int
AutoCirculateCaptureTask(ULWord deviceNumber, AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT *pFrameStampCombo)
{
	FRAME_STAMP_STRUCT* pFrameStamp = &(pFrameStampCombo->acFrameStamp);
	AUTOCIRCULATE_TASK_STRUCT*	pTask = &(pFrameStampCombo->acTask);
	NTV2Crosspoint channelSpec = pFrameStamp->channelSpec;
	NTV2PrivateParams* pNTV2Params;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAuto;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	// Validate requested channel number
	if (ILLEGAL_CHANNELSPEC(channelSpec))
	    return -ECHRNG;

	pAuto = &pNTV2Params->_AutoCirculate[channelSpec];

	if (pTask != NULL)
	{
		oemAutoCirculateTaskTransfer(&pAuto->recordTaskInfo, pTask, true);
	}

	return 0;
}



#define SIXTEEN_MB (1024 * 1024 * 16)
#define EIGHT_MB (1024 * 1024 * 8)
#define TWO_MB   (1024 * 1024 * 2)

int
AutoCirculateTransfer(ULWord deviceNumber, AUTOCIRCULATE_TRANSFER_COMBO_STRUCT *acXferCombo)
{
	PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct = &(acXferCombo->acTransfer);
	PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatus = &(acXferCombo->acStatus);
	NTV2RoutingTable* pNTV2RoutingTable = &(acXferCombo->acXena2RoutingTable);
	AUTOCIRCULATE_TASK_STRUCT*	pTask = &(acXferCombo->acTask);

	NTV2Crosspoint channelSpec = pTransferStruct->channelSpec;
	NTV2Channel channel;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoTemp;
	DMA_PARAMS dmaParams;
	ULWord csIndex;
	ULWord stride;
	LWord loopCount;

	int status = 0;
	NTV2PrivateParams* pNTV2Params;
	ULWord frameNumber;
    ULWord ulFrameOffset;
	Ntv2SystemContext systemContext;
	bool updateValid = false;
	bool transferPending = false;
	unsigned long flags = 0;
	
	systemContext.devNum = deviceNumber;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
	    return -ECHRNG;

	channel = GetNTV2ChannelForNTV2Crosspoint(channelSpec);

	pAuto = &pNTV2Params->_AutoCirculate[channelSpec];

	if (pAuto->recording)
	{
		if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING &&
			pAuto->state != NTV2_AUTOCIRCULATE_STARTING) // Play is in start state until first frame
		{
			pTransferStatus->transferFrame = NTV2_INVALID_FRAME;
			return -EPERM;
		}
	}
	else // Should be able to transfer in frames in INIT (preload), when paused etc.
	{
		if (pAuto->state == NTV2_AUTOCIRCULATE_DISABLED)
		{
			pTransferStatus->transferFrame = NTV2_INVALID_FRAME;
			return -EPERM;
		}
	}

	if (pTransferStruct->desiredFrame == NTV2_INVALID_FRAME)
	{
		ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
		frameNumber = OemAutoCirculateFindNextAvailFrame (pAuto);
		ntv2_spin_unlock_irqrestore(&pNTV2Params->_autoCirculateLock, flags);
		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("oemACTransfer: find next availframe found frame %d\n", frameNumber);
		}
	}
	else
	{
	    frameNumber = pTransferStruct->desiredFrame;
	}

	if (frameNumber == NTV2_INVALID_FRAME && pAuto->recording)
	{
		// Must be first transfer
	    frameNumber = pAuto->startFrame;
		MSG("OemAutoCirculateTransfer recording: couldn't find valid frame, setting to %d\n",
			pAuto->startFrame);
	}

    // If doing a partial frame, insure that we have valid settings
    ulFrameOffset = pTransferStruct->videoDmaOffset;
    if (ulFrameOffset)
    {
		if ((ulFrameOffset + pTransferStruct->videoBufferSize >= 
			 GetFrameBufferSize(&systemContext,
								GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec))) &&
            						(ReadRegister(deviceNumber, kVRegAdvancedIndexing, NO_MASK, NO_SHIFT) == 0))
		{
			MSG("Out Of Range vbs(0x%08X) + fo(0x%08X) >= 0x%08X\n",
				pTransferStruct->videoBufferSize,ulFrameOffset,
				GetFrameBufferSize(&systemContext, GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec)));
           	pTransferStatus->transferFrame = NTV2_INVALID_FRAME;
        	return -ENODATA;
		}
    }

	if (frameNumber != NTV2_INVALID_FRAME)
	{
		NTV2DMAEngine eVideoDmaEngine = NTV2_PIO;
		NTV2DMAEngine eAudioDmaEngine = NTV2_PIO;

		memset(&dmaParams, 0, sizeof(DMA_PARAMS));

		// select video dma engine based on flags (mostly for debug)
		if ((pTransferStruct->transferFlags & kTransferFlagVideoDMA1) == kTransferFlagVideoDMA1)
		{
			eVideoDmaEngine = NTV2_DMA1;
		}
		else if ((pTransferStruct->transferFlags & kTransferFlagVideoDMA2) == kTransferFlagVideoDMA2)
		{
			eVideoDmaEngine = NTV2_DMA2;
		}
		else if ((pTransferStruct->transferFlags & kTransferFlagVideoDMA3) == kTransferFlagVideoDMA3)
		{
			eVideoDmaEngine = NTV2_DMA3;
		}
		else if ((pTransferStruct->transferFlags & kTransferFlagVideoDMA4) == kTransferFlagVideoDMA4)
		{
			eVideoDmaEngine = NTV2_DMA4;
		}
		else
		{
			if (pNTV2Params->_dmaMethod == DmaMethodNwl)
			{
				// alternate engine for performance
				switch (channel)
				{
				default:
				case NTV2_CHANNEL1:
				case NTV2_CHANNEL2:
				case NTV2_CHANNEL7:
				case NTV2_CHANNEL8:
					eVideoDmaEngine = NTV2_DMA1;
					break;
				case NTV2_CHANNEL3:
				case NTV2_CHANNEL4:
				case NTV2_CHANNEL5:
				case NTV2_CHANNEL6:
					eVideoDmaEngine = NTV2_DMA2;
					break;
				}
			}
			else
			{
				// best performance with a single engine
				eVideoDmaEngine = NTV2_DMA1;
			}
		}

		// select audio dma engine based on flags (mostly for debug)
		if ((pTransferStruct->transferFlags & kTransferFlagAudioDMA1) == kTransferFlagAudioDMA1)
		{
			eAudioDmaEngine = NTV2_DMA1;
		}
		else if ((pTransferStruct->transferFlags & kTransferFlagAudioDMA2) == kTransferFlagAudioDMA2)
		{
			eAudioDmaEngine = NTV2_DMA2;
		}
		else if ((pTransferStruct->transferFlags & kTransferFlagAudioDMA3) == kTransferFlagAudioDMA3)
		{
			eAudioDmaEngine = NTV2_DMA3;
		}
		else if ((pTransferStruct->transferFlags & kTransferFlagAudioDMA4) == kTransferFlagAudioDMA4)
		{
			eAudioDmaEngine = NTV2_DMA4;
		}
		else
		{
			// default to video engine so we can chain
			eAudioDmaEngine = eVideoDmaEngine;
		}

		updateValid = false;
		transferPending = false;

		// setup for dma
		memset(&dmaParams, 0, sizeof(DMA_PARAMS));
		dmaParams.deviceNumber = deviceNumber;
		dmaParams.toHost = pAuto->recording;
		dmaParams.dmaEngine = eVideoDmaEngine;
		dmaParams.videoChannel = channel;
		dmaParams.audioSystemCount = pAuto->audioSystemCount;

		if (((pTransferStruct->transferFlags & kTransferFlagP2PPrepare) == kTransferFlagP2PPrepare) ||
			((pTransferStruct->transferFlags & kTransferFlagP2PTarget) == kTransferFlagP2PTarget))
		{
			if (pAuto->channelCount != 1)
			{
				MSG("Auto %s: DMA frame %d P2P multiple channels not supported (prepare)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				return -EINVAL;
			}

			OemBeginAutoCirculateTransfer(deviceNumber, frameNumber, pTransferStruct, pAuto, pNTV2RoutingTable, pTask);
			updateValid = true;

			if (pAuto->recording)
			{
				MSG("Auto %s: DMA frame %d P2P video dma from host transfer not supported (prepare)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status =  -EINVAL;
			}
			else if (!pNTV2Params->_FrameApertureBaseAddress)
			{
				MSG("Auto %s: DMA frame %d P2P target not supported (prepare)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status  = -EINVAL;
			}
			else
			{
				ULWord ulVideoOffset;
				ULWord ulVideoSize;
				dma_addr_t paFrameBuffer;
				ULWord apertureSize;

				AUTOCIRCULATE_P2P_STRUCT dmaData;
				memset((void*)&dmaData, 0, sizeof(AUTOCIRCULATE_P2P_STRUCT));
				dmaData.p2pSize = sizeof(AUTOCIRCULATE_P2P_STRUCT);

				ulVideoOffset = frameNumber *
					GetFrameBufferSize(&systemContext, GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec)) + ulFrameOffset;
				ulVideoSize = GetFrameBufferSize(&systemContext, GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec)) - ulFrameOffset;

				paFrameBuffer	= pNTV2Params->_FrameAperturePhysicalAddress;
				apertureSize	= pNTV2Params->_FrameApertureBaseSize;

				if (paFrameBuffer && ((ulVideoOffset + ulVideoSize) <= apertureSize))
				{
					// fill in p2p structure
					dmaData.videoBusAddress = paFrameBuffer + ulVideoOffset;
					dmaData.videoBusSize = ulVideoSize;

					// for target transfers (vs prepare) the source will also do a message transfer
					if ((pTransferStruct->transferFlags & kTransferFlagP2PTarget) == kTransferFlagP2PTarget)
					{
						if (channelSpec == NTV2CROSSPOINT_CHANNEL1)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel1;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL2)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel2;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL3)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel3;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL4)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel4;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL5)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel5;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL6)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel6;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL7)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel7;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL8)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel8;
						}
						else
						{
							MSG("Auto %s: DMA frame %d unsupported P2P crosspoint channel (prepare)\n",
								CrosspointName[pAuto->channelSpec], frameNumber);
							status = -EINVAL;
						}

						if (dmaData.messageBusAddress != 0)
						{
							dmaData.messageData = frameNumber;
						}
						else
						{
							MSG("Auto %s: DMA frame %d message register physical address is 0 (prepare)\n",
								CrosspointName[pAuto->channelSpec], frameNumber);
							status = -EINVAL;
						}
					}
				}
				else
				{
					MSG("Auto %s: DMA frame %d no P2P target memory aperture (prepare)\n",
						CrosspointName[pAuto->channelSpec], frameNumber);
					status = -EINVAL;
				}

				if (OemAutoCirculateP2PCopy(&dmaData, (AUTOCIRCULATE_P2P_STRUCT*)pTransferStruct->videoBuffer, false))
				{
					WriteFrameApertureOffset(deviceNumber, 0);
					transferPending = true;
				}
				else
				{
					MSG("Auto %s: DMA frame %d P2P buffer mapping error (prepare)\n",
						CrosspointName[pAuto->channelSpec], frameNumber);
					status = -EINVAL;
				}
			}
		}
		else if ((pTransferStruct->transferFlags & kTransferFlagP2PComplete) == kTransferFlagP2PComplete)
		{
			if (pAuto->recording)
			{
				MSG("Auto %s: DMA frame %d video dma from host transfer not supported (complete)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status = -EINVAL;
			}
			else if (!pNTV2Params->_FrameApertureBaseAddress)
			{
				MSG("Auto %s: DMA frame %d P2P target not supported (complete)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status = -EINVAL;
			}
			OemBeginAutoCirculateTransfer(deviceNumber, frameNumber, pTransferStruct, pAuto, pNTV2RoutingTable, pTask);
		}
		else if ((pTransferStruct->transferFlags & kTransferFlagP2PTransfer) == kTransferFlagP2PTransfer)
		{
			OemBeginAutoCirculateTransfer(deviceNumber, frameNumber, pTransferStruct, pAuto, pNTV2RoutingTable, pTask);
			updateValid = true;

			if (!pAuto->recording)
			{
				MSG("Auto %s: DMA frame %d video dma from host transfer not supported (transfer)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				return -EINVAL;
			}
			else if (!pNTV2Params->_FrameApertureBaseAddress)
			{
				MSG("Auto %s: DMA frame %d P2P transfer not supported (transfer)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status = -EINVAL;
			}
			else
			{
				AUTOCIRCULATE_P2P_STRUCT dmaData;
				if (OemAutoCirculateP2PCopy(&dmaData, (PAUTOCIRCULATE_P2P_STRUCT)pTransferStruct->videoBuffer, true))
				{
					// setup p2p video dma
					dmaParams.videoBusAddress = dmaData.videoBusAddress;
					dmaParams.videoBusSize = dmaData.videoBusSize;
					dmaParams.messageBusAddress = dmaData.messageBusAddress;
					dmaParams.messageData = dmaData.messageData;
					dmaParams.videoFrame = frameNumber;
					dmaParams.vidNumBytes = pTransferStruct->videoBufferSize;
					dmaParams.frameOffset = ulFrameOffset;
					dmaParams.vidUserPitch = pTransferStruct->videoSegmentHostPitch;
					dmaParams.vidFramePitch = pTransferStruct->videoSegmentCardPitch;
					dmaParams.numSegments = pTransferStruct->videoNumSegments;
				}
				else
				{
					MSG("Auto %s: DMA frame %d P2P buffer mapping error (transfer)\n",
						CrosspointName[pAuto->channelSpec], frameNumber);
					return -EINVAL;
				}
			}
		}
		else
		{
			// Non peer to peer autocirculate
			pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];
			csIndex = GetIndexForNTV2Crosspoint(channelSpec);
			stride = pAutoPrimary->endFrame - pAutoPrimary->startFrame + 1;

			for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
			{
				NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
				csIndex++;

				pAutoTemp = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];

				OemBeginAutoCirculateTransfer(deviceNumber, 
											  (stride * loopCount) + frameNumber,
											  pTransferStruct,
											  pAutoTemp,
											  pNTV2RoutingTable,
											  pTask);
			}
			updateValid = true;

			// setup user buffer video dma
			dmaParams.pVidUserVa = (PVOID)pTransferStruct->videoBuffer;
			dmaParams.videoFrame = frameNumber;
			dmaParams.vidNumBytes = pTransferStruct->videoBufferSize;
			dmaParams.frameOffset = ulFrameOffset;
			dmaParams.vidUserPitch = pTransferStruct->videoSegmentHostPitch;
			dmaParams.vidFramePitch = pTransferStruct->videoSegmentCardPitch;
			dmaParams.numSegments = pTransferStruct->videoNumSegments;
		}

		// configure audio transfer
		if (pAuto->circulateWithAudio &&
			(pTransferStruct->audioBuffer != NULL) &&
			(pTransferStruct->audioBufferSize != 0))
		{
			pAuto->audioTransferSize = pTransferStruct->audioBufferSize;
			status = oemAutoCirculateDmaAudioSetup(deviceNumber, pAuto);
			if (status != 0)
			{
				MSG("Auto %s: DMA frame %d audio setup failed\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				return -EINVAL;
			}

			if (pAuto->audioTransferSize > pTransferStruct->audioBufferSize)
			{
				MSG("Auto %s: DMA frame %d audio buffer size %d too small %d required\n",
					CrosspointName[pAuto->channelSpec], frameNumber,
					pTransferStruct->audioBufferSize, pAuto->audioTransferSize);
				pAuto->audioTransferSize = pTransferStruct->audioBufferSize;
			}
		}
		else
		{
			pAuto->audioTransferSize = 0;
		}
			
		if (pAuto->circulateWithAudio && (eVideoDmaEngine == eAudioDmaEngine))
		{
			// add audio to video dma
			dmaParams.pAudUserVa = (PVOID)pTransferStruct->audioBuffer;
			dmaParams.audioSystem = pAuto->audioSystem;
			dmaParams.audNumBytes = pAuto->audioTransferSize;
			dmaParams.audOffset = pAuto->audioTransferOffset;
		}

		status = dmaTransfer(&dmaParams);
		if (status != 0)
		{
			return status;
		}

		if (pAuto->circulateWithAudio &&
			(eVideoDmaEngine != eAudioDmaEngine) &&
			(pAuto->audioTransferSize != 0))
		{
			// setup separate audio dma
			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.toHost = pAuto->recording;
			dmaParams.dmaEngine = eAudioDmaEngine;
			dmaParams.videoChannel = channel;
			dmaParams.pAudUserVa = (PVOID)pTransferStruct->audioBuffer;
			dmaParams.audioSystem = pAuto->audioSystem;
			dmaParams.audNumBytes = pTransferStruct->audioBufferSize;
			dmaParams.audOffset = pAuto->audioTransferOffset;
			dmaParams.audioSystemCount = pAuto->audioSystemCount;

			status = dmaTransfer(&dmaParams);
			if (status != 0)
			{
				return status;
			}
		}

		pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];
		csIndex = GetIndexForNTV2Crosspoint(channelSpec);
		stride = pAutoPrimary->endFrame - pAutoPrimary->startFrame + 1;

		for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
		{
			NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
			csIndex++;

			pAutoTemp = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];

			OemCompleteAutoCirculateTransfer(deviceNumber,
											 (stride * loopCount) + frameNumber,
											 pTransferStatus,
											 pAutoTemp,
											 updateValid,
											 transferPending);
		}

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("AC_Transfer: Frm(%d),BfLvl(%d),FrmsDrp(%d),FrmsPrc(%d),State(%d)\n",frameNumber,
				pTransferStatus->bufferLevel,
				pTransferStatus->framesDropped,
				pTransferStatus->framesProcessed,
				pAuto->state);
		}

		if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
		{
			if (pTransferStatus->audioBufferSize)
			{
				MSG("__Audio: BufSize=%d, StartSample=%d\n",
					pTransferStatus->audioBufferSize, pTransferStatus->audioStartSample);
			}
		}
	}
	else
	{
	    pTransferStatus->transferFrame = NTV2_INVALID_FRAME;
	    status = -EBUSY;
	}

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
	{
		MSG("State of internal struct after TRANSFER:\n");
			SHOW_INTERNAL_AUTOCIRCULATE_STRUCT(pAuto);
		MSG("AutoCirculateTransfer completed with status %d.\n", status);
	}

	return status;
}

int
AutoCirculateTransfer_Ex(ULWord deviceNumber, PDMA_PAGE_ROOT pPageRoot, AUTOCIRCULATE_TRANSFER *pTransferStruct)
{
	NTV2PrivateParams* pNTV2Params;
	NTV2Crosspoint channelSpec = pTransferStruct->acCrosspoint;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoPrimary;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoTemp;
	DMA_PARAMS dmaParams;
	ULWord csIndex;
	ULWord stride;
	LWord loopCount;
	NTV2Channel channel;
	int status = 0;
	ULWord frameNumber;
    ULWord ulFrameOffset;
	NTV2DMAEngine eVideoDmaEngine = NTV2_PIO;
	NTV2DMAEngine eAudioDmaEngine = NTV2_PIO;
	NTV2DMAEngine eAncDmaEngine = NTV2_PIO;
	Ntv2SystemContext systemContext;
	bool audioTransferDone = false;
	bool ancTransferDone = false;
	bool updateValid = false;
	bool transferPending = false;
	unsigned long flags = 0;

	systemContext.devNum = deviceNumber;
	
#ifdef AUTO_REPORT
	int64_t transferStartTime;
	int64_t transferStopTime;
#endif

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
	    return -ECHRNG;

	channel = GetNTV2ChannelForNTV2Crosspoint(channelSpec);

	pAuto = &pNTV2Params->_AutoCirculate[channelSpec];

	if (pAuto->recording)
	{
		if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING &&
		   pAuto->state != NTV2_AUTOCIRCULATE_STARTING) // Play is in start state until first frame
		{
			pTransferStruct->acTransferStatus.acTransferFrame = NTV2_INVALID_FRAME;
			return -1;
		}
	}
	else // Should be able to transfer in frames in INIT (preload), when paused etc.
	{
		if (pAuto->state == NTV2_AUTOCIRCULATE_DISABLED)
		{
			pTransferStruct->acTransferStatus.acTransferFrame = NTV2_INVALID_FRAME;
			return -1;
		}
	}

	if (pTransferStruct->acDesiredFrame == NTV2_INVALID_FRAME)
	{
		ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);
		frameNumber = OemAutoCirculateFindNextAvailFrame (pAuto);
		ntv2_spin_unlock_irqrestore(&pNTV2Params->_autoCirculateLock, flags);
		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("oemACTransfer: find next availframe found frame %d\n", frameNumber);
		}
	}
	else
	{
	    frameNumber = pTransferStruct->acDesiredFrame;
	}

	if (frameNumber == NTV2_INVALID_FRAME && pAuto->recording)
	{
		// Must be first transfer
	    frameNumber = pAuto->startFrame;
		MSG("OemAutoCirculateTransfer recording: couldn't find valid frame, setting to %d\n",
			pAuto->startFrame);
	}

    // If doing a partial frame, insure that we have valid settings
    ulFrameOffset = pTransferStruct->acInVideoDMAOffset;
    if (ulFrameOffset)
    {
		if ((ulFrameOffset + pTransferStruct->acVideoBuffer.fByteCount >=
			 GetFrameBufferSize(&systemContext, channel)) &&
            (ReadRegister(deviceNumber, kVRegAdvancedIndexing, NO_MASK, NO_SHIFT) == 0))
		{
			MSG("Out Of Range vbs(0x%08X) + fo(0x%08X) >= 0x%08X\n",
					pTransferStruct->acVideoBuffer.fByteCount,ulFrameOffset,
					GetFrameBufferSize(&systemContext, channel));
           	pTransferStruct->acTransferStatus.acTransferFrame = NTV2_INVALID_FRAME;
        	return -ENODATA;
		}
    }

#ifdef AUTO_REPORT
	transferStartTime = ntv2TimeCounter();
#endif
	if (frameNumber != NTV2_INVALID_FRAME)
	{
		if (((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_PREPARE) != AUTOCIRCULATE_P2P_PREPARE) &&
			((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_TARGET) != AUTOCIRCULATE_P2P_TARGET) &&
			((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_COMPLETE) != AUTOCIRCULATE_P2P_COMPLETE))
		{
			eVideoDmaEngine = NTV2_DMA1;
			eAudioDmaEngine = eVideoDmaEngine;
		}

		if (((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_PREPARE) == AUTOCIRCULATE_P2P_PREPARE) ||
		   ((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_TARGET) == AUTOCIRCULATE_P2P_TARGET))
		{
			if (pAuto->channelCount != 1)
			{
				MSG("Auto %s: DMA frame %d P2P multiple channels not supported (prepare)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				return -EINVAL;
			}

			OemBeginAutoCirculateTransfer_Ex(deviceNumber, frameNumber, pTransferStruct, pAuto);
			updateValid = true;

			if (pAuto->recording)
			{
				MSG("Auto %s: DMA frame %d P2P video dma from host transfer not supported (prepare)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status =  -EINVAL;
			}
			else if (!pNTV2Params->_FrameApertureBaseAddress)
			{
				MSG("Auto %s: DMA frame %d P2P target not supported (prepare)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status  = -EINVAL;
			}
			else
			{
				ULWord ulVideoOffset;
				ULWord ulVideoSize;
				dma_addr_t paFrameBuffer;
				ULWord apertureSize;

				AUTOCIRCULATE_P2P_STRUCT dmaData;
				memset((void*)&dmaData, 0, sizeof(AUTOCIRCULATE_P2P_STRUCT));
				dmaData.p2pSize = sizeof(AUTOCIRCULATE_P2P_STRUCT);

				ulVideoOffset	= frameNumber*GetFrameBufferSize(&systemContext, channel) + ulFrameOffset;
				ulVideoSize		= GetFrameBufferSize(&systemContext, channel) - ulFrameOffset;

				paFrameBuffer	= pNTV2Params->_FrameAperturePhysicalAddress;
				apertureSize	= pNTV2Params->_FrameApertureBaseSize;

				if (paFrameBuffer && ((ulVideoOffset + ulVideoSize) <= apertureSize))
				{
					// fill in p2p structure
					dmaData.videoBusAddress = paFrameBuffer + ulVideoOffset;
					dmaData.videoBusSize = ulVideoSize;

					// for target transfers (vs prepare) the source will also do a message transfer
					if ((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_TARGET) == AUTOCIRCULATE_P2P_TARGET)
					{
						if (channelSpec == NTV2CROSSPOINT_CHANNEL1)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel1;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL2)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel2;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL3)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel3;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL4)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel4;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL5)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel5;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL6)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel6;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL7)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel7;
						}
						else if (channelSpec == NTV2CROSSPOINT_CHANNEL8)
						{
							dmaData.messageBusAddress = pNTV2Params->_pPhysicalMessageChannel8;
						}
						else
						{
							MSG("Auto %s: DMA frame %d unsupported P2P crosspoint channel (prepare)\n",
								CrosspointName[pAuto->channelSpec], frameNumber);
							status = -EINVAL;
						}

						if (dmaData.messageBusAddress != 0)
						{
							dmaData.messageData = frameNumber;
						}
						else
						{
							MSG("Auto %s: DMA frame %d message register physical address is 0 (prepare)\n",
								CrosspointName[pAuto->channelSpec], frameNumber);
							status = -EINVAL;
						}
					}
				}
				else
				{
					MSG("Auto %s: DMA frame %d no P2P target memory aperture (prepare)\n",
						CrosspointName[pAuto->channelSpec], frameNumber);
					status = -EINVAL;
				}

				if (OemAutoCirculateP2PCopy(&dmaData, (AUTOCIRCULATE_P2P_STRUCT*)pTransferStruct->acVideoBuffer.fUserSpacePtr, false))
				{
					WriteFrameApertureOffset(deviceNumber, 0);
					transferPending = true;
				}
				else
				{
					MSG("Auto %s: DMA frame %d P2P buffer mapping error (prepare)\n",
						CrosspointName[pAuto->channelSpec], frameNumber);
					status = -EINVAL;
				}
			}
		}
		else if ((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_COMPLETE) == AUTOCIRCULATE_P2P_COMPLETE)
		{
			if (pAuto->recording)
			{
				MSG("Auto %s: DMA frame %d video dma from host transfer not supported (complete)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status = -EINVAL;
			}
			else if (!pNTV2Params->_FrameApertureBaseAddress)
			{
				MSG("Auto %s: DMA frame %d P2P target not supported (complete)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status = -EINVAL;
			}
			OemBeginAutoCirculateTransfer_Ex(deviceNumber, frameNumber, pTransferStruct, pAuto);
		}
		else if ((pTransferStruct->acPeerToPeerFlags & AUTOCIRCULATE_P2P_TRANSFER) == AUTOCIRCULATE_P2P_TRANSFER)
		{
			OemBeginAutoCirculateTransfer_Ex(deviceNumber, frameNumber, pTransferStruct, pAuto);
			updateValid = true;

			if (!pAuto->recording)
			{
				MSG("Auto %s: DMA frame %d video dma from host transfer not supported (transfer)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				return -EINVAL;
			}
			else if (!pNTV2Params->_FrameApertureBaseAddress)
			{
				MSG("Auto %s: DMA frame %d P2P transfer not supported (transfer)\n",
					CrosspointName[pAuto->channelSpec], frameNumber);
				status = -EINVAL;
			}
			else
			{
				AUTOCIRCULATE_P2P_STRUCT dmaData;
				if (OemAutoCirculateP2PCopy(&dmaData, (PAUTOCIRCULATE_P2P_STRUCT)pTransferStruct->acVideoBuffer.fUserSpacePtr, true))
				{
					// setup p2p video dma
					memset(&dmaParams, 0, sizeof(DMA_PARAMS));
					dmaParams.deviceNumber = deviceNumber;
					dmaParams.pPageRoot = pPageRoot;
					dmaParams.toHost = pAuto->recording;
					dmaParams.dmaEngine = eVideoDmaEngine;
					dmaParams.videoChannel = channel;
					dmaParams.videoBusAddress = dmaData.videoBusAddress;
					dmaParams.videoBusSize = dmaData.videoBusSize;
					dmaParams.messageBusAddress = dmaData.messageBusAddress;
					dmaParams.messageData = dmaData.messageData;
					dmaParams.videoFrame = frameNumber;
					dmaParams.vidNumBytes = pTransferStruct->acVideoBuffer.fByteCount;
					dmaParams.frameOffset = ulFrameOffset;
					dmaParams.vidUserPitch = pTransferStruct->acInSegmentedDMAInfo.acSegmentHostPitch;
					dmaParams.vidFramePitch = pTransferStruct->acInSegmentedDMAInfo.acSegmentDevicePitch;
					dmaParams.numSegments = pTransferStruct->acInSegmentedDMAInfo.acNumSegments;
					dmaParams.audioSystemCount = pAuto->audioSystemCount;

					status = dmaTransfer(&dmaParams);
					if (status != 0)
					{
						return status;
					}
				}
				else
				{
					MSG("Auto %s: DMA frame %d P2P buffer mapping error (transfer)\n",
						CrosspointName[pAuto->channelSpec], frameNumber);
					return -EINVAL;
				}
			}
		}
		else
		{
			// Non peer to peer autocirculate
			pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];
			csIndex = GetIndexForNTV2Crosspoint(channelSpec);
			stride = pAutoPrimary->endFrame - pAutoPrimary->startFrame + 1;

			for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
			{
				NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
				csIndex++;

				pAutoTemp = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];

				OemBeginAutoCirculateTransfer_Ex (deviceNumber, (stride * loopCount)+frameNumber, pTransferStruct, pAutoTemp);
			}
			updateValid = true;

			if (pNTV2Params->_dmaMethod == DmaMethodXlnx)
			{
				eVideoDmaEngine = NTV2_DMA1;
			}
			else
			{
				// alternate engine for performance
				switch (channel)
				{
				default:
				case NTV2_CHANNEL1:
				case NTV2_CHANNEL2:
				case NTV2_CHANNEL7:
				case NTV2_CHANNEL8:
					eVideoDmaEngine = NTV2_DMA1;
					break;
				case NTV2_CHANNEL3:
				case NTV2_CHANNEL4:
				case NTV2_CHANNEL5:
				case NTV2_CHANNEL6:
					eVideoDmaEngine = NTV2_DMA2;
					break;
				}
			}

			// setup user buffer video dma
			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = pPageRoot;
			dmaParams.toHost = pAuto->recording;
			dmaParams.dmaEngine = eVideoDmaEngine;
			dmaParams.videoChannel = channel;
			dmaParams.pVidUserVa = (PVOID)pTransferStruct->acVideoBuffer.fUserSpacePtr;
			dmaParams.videoFrame = frameNumber;
			dmaParams.vidNumBytes = pTransferStruct->acVideoBuffer.fByteCount;
			dmaParams.frameOffset = ulFrameOffset;
			dmaParams.vidUserPitch = pTransferStruct->acInSegmentedDMAInfo.acSegmentHostPitch;
			dmaParams.vidFramePitch = pTransferStruct->acInSegmentedDMAInfo.acSegmentDevicePitch;
			dmaParams.numSegments = pTransferStruct->acInSegmentedDMAInfo.acNumSegments;
			dmaParams.audioSystemCount = pAuto->audioSystemCount;

			if (pAuto->circulateWithAudio &&
				(pTransferStruct->acAudioBuffer.fUserSpacePtr != 0) &&
				(pTransferStruct->acAudioBuffer.fByteCount != 0))
			{
				pAuto->audioTransferSize = pTransferStruct->acAudioBuffer.fByteCount;
				status = oemAutoCirculateDmaAudioSetup(deviceNumber, pAuto);
				if (status != 0)
				{
					MSG("Auto %s: DMA frame %d audio setup failed\n",
						CrosspointName[pAuto->channelSpec], frameNumber);
					return -EINVAL;
				}

				if (pAuto->audioTransferSize > pTransferStruct->acAudioBuffer.fByteCount)
				{
					MSG("Auto %s: DMA frame %d audio buffer size %d too small %d required\n",
						CrosspointName[pAuto->channelSpec], frameNumber,
						pTransferStruct->acAudioBuffer.fByteCount, pAuto->audioTransferSize);
					pAuto->audioTransferSize = pTransferStruct->acAudioBuffer.fByteCount;
				}
			}
			else
			{
				pAuto->audioTransferSize = 0;
			}

			// setup for audio dma
			if (pAuto->circulateWithAudio)
			{
				dmaParams.pAudUserVa = (PVOID)pTransferStruct->acAudioBuffer.fUserSpacePtr;
				dmaParams.audioSystem = pAuto->audioSystem;
				dmaParams.audNumBytes = pAuto->audioTransferSize;
				dmaParams.audOffset = pAuto->audioTransferOffset;
				audioTransferDone = true;
			}

			// setup for anc dma
			if (pAuto->circulateWithCustomAncData)
			{
				dmaParams.pAncF1UserVa = (PVOID)pTransferStruct->acANCBuffer.fUserSpacePtr;
				dmaParams.ancF1Frame = frameNumber;
				dmaParams.ancF1NumBytes = pAuto->ancTransferSize;
				dmaParams.ancF1Offset =	pAuto->ancTransferOffset;
				dmaParams.pAncF2UserVa = (PVOID)pTransferStruct->acANCField2Buffer.fUserSpacePtr;
				dmaParams.ancF2Frame = frameNumber;
				dmaParams.ancF2NumBytes = pAuto->ancField2TransferSize;
				dmaParams.ancF2Offset =	pAuto->ancField2TransferOffset;
				ancTransferDone = true;
			}

			if (pAuto->circulateWithFields)
			{
				DMA_PARAMS params;
			
				if (!pAuto->recording)
				{
					// update playback field data
					pAuto->frameStamp[frameNumber].frameFlags = pTransferStruct->acPeerToPeerFlags;
				}

				// transfer the active field
				params = dmaParams;
				oemAutoCirculateTransferFields(deviceNumber, pAuto, &params, frameNumber, false);
				status = dmaTransfer(&params);

				if (pAuto->recording && (status == 0))
				{
					// update capture field data
					pTransferStruct->acPeerToPeerFlags = pAuto->frameStamp[frameNumber].frameFlags;
				}
				else
				{
					// transfer the drop field
					params = dmaParams;
					params.pAudUserVa = NULL;
					params.pAncF1UserVa = NULL;
					params.pAncF2UserVa = NULL;
					oemAutoCirculateTransferFields(deviceNumber, pAuto, &params, frameNumber, true);
					status = dmaTransfer(&params);
				}
			}
			else
			{
				status = dmaTransfer(&dmaParams);
			}
			if (status != 0)
			{
				return status;
			}
		}

		if (pAuto->circulateWithAudio && !audioTransferDone &&
			(pAuto->audioTransferSize != 0))
		{
			if (eAudioDmaEngine == NTV2_PIO)
			{
				eAudioDmaEngine = NTV2_DMA1;
			}

			// setup for separate audio dma
			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = pPageRoot;
			dmaParams.toHost = pAuto->recording;
			dmaParams.dmaEngine = eAudioDmaEngine;
			dmaParams.videoChannel = channel;
			dmaParams.pAudUserVa = (PVOID)pTransferStruct->acAudioBuffer.fUserSpacePtr;
			dmaParams.audioSystem = pAuto->audioSystem;
			dmaParams.audNumBytes = pAuto->audioTransferSize;
			dmaParams.audOffset = pAuto->audioTransferOffset;
			dmaParams.audioSystemCount = pAuto->audioSystemCount;

			status = dmaTransfer(&dmaParams);
			if (status != 0)
			{
				return status;
			}
		}

		if (pAuto->circulateWithCustomAncData && !ancTransferDone &&
			(((PVOID)pTransferStruct->acANCBuffer.fUserSpacePtr != NULL) ||
			 ((PVOID)pTransferStruct->acANCField2Buffer.fUserSpacePtr != NULL)))
		{
			if (eAncDmaEngine == NTV2_PIO)
			{
				eAncDmaEngine = NTV2_DMA1;
			}

			// dma only anc
			memset(&dmaParams, 0, sizeof(DMA_PARAMS));
			dmaParams.deviceNumber = deviceNumber;
			dmaParams.pPageRoot = pPageRoot;
			dmaParams.toHost = pAuto->recording;
			dmaParams.dmaEngine = eAncDmaEngine;
			dmaParams.videoChannel = channel;
			dmaParams.pAncF1UserVa = (PVOID)pTransferStruct->acANCBuffer.fUserSpacePtr;
			dmaParams.ancF1Frame = frameNumber;
			dmaParams.ancF1NumBytes = pAuto->ancTransferSize;
			dmaParams.ancF1Offset = pAuto->ancTransferOffset;
			dmaParams.pAncF2UserVa = (PVOID)pTransferStruct->acANCField2Buffer.fUserSpacePtr;
			dmaParams.ancF2Frame = frameNumber;
			dmaParams.ancF2NumBytes = pAuto->ancField2TransferSize;
			dmaParams.ancF2Offset = pAuto->ancField2TransferOffset;
			dmaParams.audioSystemCount = pAuto->audioSystemCount;

			status = dmaTransfer(&dmaParams);
			if (status != 0)
			{
				return status;
			}
		}

		pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];
		csIndex = GetIndexForNTV2Crosspoint(channelSpec);
		stride = pAutoPrimary->endFrame - pAutoPrimary->startFrame + 1;

		for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
		{
			NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
			csIndex++;

			pAutoTemp = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];

			OemCompleteAutoCirculateTransfer_Ex(deviceNumber,
												(stride * loopCount)+frameNumber,
												&(pTransferStruct->acTransferStatus),
												pAutoTemp,
												updateValid,
												transferPending);
		}

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("AC_Transfer: Frm(%d),BfLvl(%d),FrmsDrp(%d),FrmsPrc(%d),State(%d)\n",frameNumber,
	                                       pTransferStruct->acTransferStatus.acBufferLevel,
	                                       pTransferStruct->acTransferStatus.acFramesDropped,
	                                       pTransferStruct->acTransferStatus.acFramesProcessed,
	                                       pAuto->state);
		}

		if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
		{
			if (pTransferStruct->acAudioBuffer.fByteCount)
			{
				MSG("__Audio: BufSize=%d, StartSample=%d\n",
					pTransferStruct->acTransferStatus.acAudioTransferSize, pTransferStruct->acTransferStatus.acAudioStartSample);
			}
		}
	}
	else
	{
	    pTransferStruct->acTransferStatus.acTransferFrame = NTV2_INVALID_FRAME;
	    status = -EBUSY;
	}

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
	{
		MSG("State of internal struct after TRANSFER:\n");
			SHOW_INTERNAL_AUTOCIRCULATE_STRUCT(pAuto);
		MSG("AutoCirculateTransfer completed with status %d.\n", status);
	}
	
#ifdef AUTO_REPORT
	transferStopTime = ntv2TimeCounter();
	pNTV2Params->_dmaStatTotalTime[channelSpec] += transferStopTime - transferStartTime;
	pNTV2Params->_dmaStatTransferCount[channelSpec]++;

	if ((pNTV2Params->_dmaStatTransferCount[channelSpec] > 0) &&
		((transferStopTime - pNTV2Params->_dmaStatReportLast[channelSpec]) > pNTV2Params->_dmaStatReportInterval[channelSpec]))
	{
		int64_t transferAvrTime = pNTV2Params->_dmaStatTotalTime[channelSpec] / pNTV2Params->_dmaStatTransferCount[channelSpec];
		MSG("AutoCirculateTransfer channel %d  count %d  avrtime %d us\n",
			channelSpec,
			pNTV2Params->_dmaStatTransferCount[channelSpec],
			(uint32_t)(transferAvrTime * 1000000 / ntv2TimeFrequency()));
		pNTV2Params->_dmaStatReportLast[channelSpec] = transferStopTime;
		pNTV2Params->_dmaStatTotalTime[channelSpec] = 0;
		pNTV2Params->_dmaStatTransferCount[channelSpec] = 0;
	}
#endif

	return status;
}

int
OemAutoCirculateMessage(ULWord deviceNumber, NTV2Crosspoint channelSpec, ULWord frameNumber)
{
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;
	NTV2PrivateParams* pNTV2Params;

//	ntv2Message("message channelSpec %d frameNumber %d\n",
//				channelSpec, frameNumber);

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
	    return -ECHRNG;

	pAuto = &pNTV2Params->_AutoCirculate[channelSpec];

	// update output frame directly if autocirculate disabled
	if (pAuto->state == NTV2_AUTOCIRCULATE_DISABLED)
	{
		if (channelSpec == NTV2CROSSPOINT_CHANNEL1)
		{
			WriteRegister(deviceNumber, kRegCh1OutputFrame, frameNumber, NO_MASK, NO_SHIFT);
		}
		if (channelSpec == NTV2CROSSPOINT_CHANNEL2)
		{
			WriteRegister(deviceNumber, kRegCh2OutputFrame, frameNumber, NO_MASK, NO_SHIFT);
		}
		if (channelSpec == NTV2CROSSPOINT_CHANNEL3)
		{
			WriteRegister(deviceNumber, kRegCh3OutputFrame, frameNumber, NO_MASK, NO_SHIFT);
		}
		if (channelSpec == NTV2CROSSPOINT_CHANNEL4)
		{
			WriteRegister(deviceNumber, kRegCh4OutputFrame, frameNumber, NO_MASK, NO_SHIFT);
		}
		if (channelSpec == NTV2CROSSPOINT_CHANNEL5)
		{
			WriteRegister(deviceNumber, kRegCh5OutputFrame, frameNumber, NO_MASK, NO_SHIFT);
		}
		if (channelSpec == NTV2CROSSPOINT_CHANNEL6)
		{
			WriteRegister(deviceNumber, kRegCh6OutputFrame, frameNumber, NO_MASK, NO_SHIFT);
		}
		if (channelSpec == NTV2CROSSPOINT_CHANNEL7)
		{
			WriteRegister(deviceNumber, kRegCh7OutputFrame, frameNumber, NO_MASK, NO_SHIFT);
		}
		if (channelSpec == NTV2CROSSPOINT_CHANNEL8)
		{
			WriteRegister(deviceNumber, kRegCh8OutputFrame, frameNumber, NO_MASK, NO_SHIFT);
		}
	}

	// check frame number range
	if (frameNumber >= (ULWord)pAuto->startFrame && frameNumber <= (ULWord)pAuto->endFrame)
	{
		if (pAuto->recording)
		{
			// this should not happem
			MSG("Auto %d:%s: DMA frame %d error - P2P for channel in capture mode\n",
				deviceNumber, CrosspointName[pAuto->channelSpec], frameNumber);
		}
		else
		{
			// check the pending flag
			if (pAuto->frameStamp[frameNumber].videoTransferPending)
			{
				pAuto->frameStamp[frameNumber].videoTransferPending = false;
//				MSG("Auto %d:%s: DMA frame %d P2P message completes pending transfer\n",
//					deviceNumber, CrosspointName[pAuto->channelSpec], frameNumber);
			}
			else
			{
				// transfer not pending?
				MSG("Auto %d:%s: DMA frame %d P2P message for frame with no pending transfer\n",
					deviceNumber, CrosspointName[pAuto->channelSpec], frameNumber);
			}
		}
	}
	else
	{
		// frame out of range
		MSG("Auto %d:%s: DMA frame %d error - P2P message for frame number out of range\n",
			deviceNumber, CrosspointName[pAuto->channelSpec], frameNumber);
	}

	return 0;
}

void 
oemAutoCirculateTransferFields(ULWord deviceNumber,
							   INTERNAL_AUTOCIRCULATE_STRUCT* pAuto, 
							   DMA_PARAMS* pDmaParams, 
							   ULWord frameNumber, bool drop)
{
	NTV2Channel syncChannel = NTV2_CHANNEL1;
	bool syncField0 = true;
	bool top = true;
	ULWord pixels = 0;
	ULWord lines = 0;
	ULWord pitch = 0;
	NTV2FrameGeometry fbGeometry = NTV2_FG_720x486;
	NTV2FrameBufferFormat fbFormat = NTV2_FBF_10BIT_YCBCR;
	Ntv2SystemContext systemContext;
	systemContext.devNum = deviceNumber;

	if ((pDmaParams->frameOffset != 0) || 
		(pDmaParams->numSegments > 1))
		return;

	if (!oemAutoCirculateCanDoFieldMode(deviceNumber, pAuto->channelSpec))
		return;

	// get the format channel
	if(IsMultiFormatActive(&systemContext))
	{
		syncChannel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
	}

	// which field to transfer
	syncField0 = (pAuto->frameStamp[frameNumber].frameFlags & AUTOCIRCULATE_FRAME_FIELD0) != 0;
	if (drop)
		syncField0 = !syncField0;

	// default top to first field
	top = syncField0;

	// get pixels and lines
	fbGeometry = GetFrameGeometry(&systemContext, syncChannel);
	switch (fbGeometry)
	{
	case NTV2_FG_720x486:
		pixels = 720;
		lines = 486;
		top = !syncField0;
		break;
	case NTV2_FG_720x514:
		pixels = 720;
		lines = 514;
		top = !syncField0;
		break;
	case NTV2_FG_720x576:
		pixels = 720;
		lines = 576;
		break;
	case NTV2_FG_720x612:
		pixels = 720;
		lines = 612;
		break;
	case NTV2_FG_1920x1080:
		pixels = 1920;
		lines = 1080;
		break;
	case NTV2_FG_1920x1112:
		pixels = 1920;
		lines = 1112;
		break;
	case NTV2_FG_1920x1114:
		pixels = 1920;
		lines = 1114;
		break;
	default:
		return;
	}

	// get line pitch
	fbFormat = GetFrameBufferFormat(&systemContext, syncChannel);
	switch (fbFormat)
	{
	case NTV2_FBF_10BIT_YCBCR:
	case NTV2_FBF_10BIT_YCBCR_DPX:
		pitch = pixels * 16 / 6;
		break;
	case NTV2_FBF_8BIT_YCBCR:
	case NTV2_FBF_8BIT_YCBCR_YUY2:
		pitch = pixels * 2;
		break;
	case NTV2_FBF_ARGB:
	case NTV2_FBF_RGBA:
	case NTV2_FBF_10BIT_RGB:
	case NTV2_FBF_ABGR:
	case NTV2_FBF_10BIT_DPX:
	case NTV2_FBF_10BIT_YCBCRA:
	case NTV2_FBF_10BIT_DPX_LE:
		pitch = pixels * 4;
		break;
	case NTV2_FBF_24BIT_RGB:
	case NTV2_FBF_24BIT_BGR:
		pitch = pixels * 3 / 2;
		break;
	case NTV2_FBF_48BIT_RGB:
		pitch = pixels * 6;
		break;
	default:
		return;
	}
	
	// fill in segment transfer data
	pDmaParams->frameOffset = top? 0 : pitch;
	pDmaParams->vidNumBytes = pitch;
	pDmaParams->vidFramePitch = pitch * 2;
	pDmaParams->vidUserPitch = pitch;
	pDmaParams->numSegments = lines / 2;
}

LWord
OemAutoCirculateFindNextAvailFrame(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto)
{
	LWord lRange = (pAuto->endFrame - pAuto->startFrame) + 1;
	LWord lStart = 0;
	LWord lCurFrame;
	LWord i;

	if (pAuto->state == NTV2_AUTOCIRCULATE_INIT)
	{
		// When pre-loading (NTV2_AUOCIRCULATE_INIT state), start from startFrame. (activeFrame is not valid)
		lCurFrame = pAuto->startFrame-1;	// (will be incremented before use)
		lStart = 0;
	}
	else
	{
		lCurFrame = pAuto->activeFrame;
		if ((lCurFrame < pAuto->startFrame) || (lCurFrame > pAuto->endFrame))
		{
			// frame out of range
			return NTV2_INVALID_FRAME;
		}
		lStart = 1;
	}

	if (pAuto->recording)
	{
		// No frames available to record unless state = NTV2_AUTOCIRCULATE_RUNNING
		if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING)
			return NTV2_INVALID_FRAME;

		// Search forward for a '1' which indicates frame ready to record.
		for (i = 0; i < lRange; i++)
		{
			// Normalize for non zero starts
			lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);
			if (pAuto->frameStamp[lCurFrame].validCount > 0)
			{
				// Found it
				return lCurFrame;
			}
		}
	}
	else // playback
	{
		/* if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING &&
           pAuto->state != NTV2_AUTOCIRCULATE_STARTING &&
           pAuto->state != NTV2_AUTOCIRCULATE_PAUSED &&
           pAuto->state != NTV2_AUTOCIRCULATE_INIT)
           return NTV2_INVALID_FRAME;
		*/
		if (pAuto->state == NTV2_AUTOCIRCULATE_DISABLED)
			return NTV2_INVALID_FRAME;

		// Search forward for a '0' which indicates next available frame to transfer to.
		for (i = lStart; i < lRange; i++)
		{
			// Normalize for non zero starts
			lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);

			if (pAuto->frameStamp[lCurFrame].validCount == 0)
			{
				// Found it
				return lCurFrame;
			}
		}
	}
	return NTV2_INVALID_FRAME;    // None available
}

int
OemAutoCirculateInit (ULWord deviceNumber,
					  NTV2Crosspoint channelSpec,
					  LWord lStartFrameNum,
					  LWord lEndFrameNum,
					  NTV2AudioSystem audioSystem,
					  LWord lChannelCount,
					  bool bWithAudio,
					  bool bWithRP188,
					  bool bFbfChange,
					  bool bFboChange,
					  bool bWithColorCorrection,
					  bool bWithVidProc,
					  bool bWithCustomAncData,
					  bool bWithLTC,
					  bool bWithFields,
					  bool bWithHDMIAux,
					  bool bWithASPlus1,
					  bool bWithASPlus2,
					  bool bWithASPlus3)
{
	Ntv2SystemContext systemContext;
	NTV2PrivateParams* pNTV2Params;
	ULWord channelRange = 0;
	ULWord csIndex = 0;
	LWord loopCount = 0;
	NTV2Channel ACChannel = NTV2_CHANNEL1;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAuto;
	systemContext.devNum = deviceNumber;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
	{
		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("OemAutoCirculateInit: No such board %d\n", deviceNumber);
		}
		return -ENODEV;
	}

	if (ILLEGAL_CHANNELSPEC(channelSpec))
	{
		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("OemAutoCirculateInit: Illegal channel spec %d\n", channelSpec);
		}
	    return -ECHRNG;
	}

	if (lStartFrameNum >= NTV2_MAX_FRAMEBUFFERS)
		return -EINVAL;

	if (lEndFrameNum >= NTV2_MAX_FRAMEBUFFERS)
		return -EINVAL;
    
	channelRange = lEndFrameNum - lStartFrameNum + 1;
	if (channelRange >= NTV2_MAX_FRAMEBUFFERS)
		return -EINVAL;
    
	csIndex = GetIndexForNTV2Crosspoint(channelSpec);
	if (lChannelCount > 1)
		pNTV2Params->_bMultiChannel = true;
	else
		pNTV2Params->_bMultiChannel = false;

	if (!NTV2DeviceCanDoCustomAnc(pNTV2Params->_DeviceID))
		bWithCustomAncData = false;

	for (loopCount = 0; loopCount < lChannelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;
		ACChannel = GetNTV2ChannelForNTV2Crosspoint(channelSpecAtIndex);

		pAuto = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];
		if (pAuto->state != NTV2_AUTOCIRCULATE_DISABLED)
		{
			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
			{
				MSG("OemAutoCirculateInit: Autocirculate is not disabled on channelspec %d\n", channelSpecAtIndex);
			}
			return -EBUSY;
		}

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("Received IOCTL_NTV2_AUTOCIRCULATE_CONTROL: INIT\n"
				"State of internal struct before INIT:\n");
			SHOW_INTERNAL_AUTOCIRCULATE_STRUCT(pAuto);
		}

		OemAutoCirculateReset (deviceNumber, channelSpecAtIndex);  // Reset AutoCirculate Database

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("pAuto->recording is %d for board %d chspec %d\n",
			  	pAuto->recording, deviceNumber, channelSpecAtIndex);
		}

		pAuto->channelSpec	= channelSpecAtIndex;
		pAuto->startFrame	= lStartFrameNum + (loopCount * channelRange);
		pAuto->endFrame		= lEndFrameNum + (loopCount * channelRange);
		pAuto->currentFrame = pAuto->startFrame;
		pAuto->circulateWithAudio = (loopCount == 0) ? bWithAudio : false;
		pAuto->circulateWithRP188 = (loopCount == 0) ? bWithRP188 : false;
	    pAuto->enableFbfChange    = (loopCount == 0) ? bFbfChange : false;
	    pAuto->enableFboChange    = (loopCount == 0) ? bFboChange : false;
	    pAuto->circulateWithColorCorrection	= (loopCount == 0) ? bWithColorCorrection : false;
	    pAuto->circulateWithVidProc			= (loopCount == 0) ? bWithVidProc : false;
	    pAuto->circulateWithCustomAncData	= (loopCount == 0) ? bWithCustomAncData : false;
		pAuto->circulateWithLTC             = (loopCount == 0) ? bWithLTC : false;
		pAuto->circulateWithFields			= bWithFields;
		pAuto->circulateWithHDMIAux			= (loopCount == 0) ? bWithHDMIAux : false;
		pAuto->audioSystem  = audioSystem;
		pAuto->audioSystemCount = 0;
		pAuto->channelCount = (loopCount == 0) ? lChannelCount : 0;
		pAuto->VBIRDTSC = 0;
		pAuto->VBILastRDTSC = 0;
		pAuto->VBIAudioOut = 0;

        SetMode(&systemContext, ACChannel, NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
        if (!NTV2DeviceCanDo12gRouting(pNTV2Params->_DeviceID))
        {
            if (Get425FrameEnable(&systemContext, NTV2_CHANNEL1) &&
				(pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL1 ||
				 pAuto->channelSpec == NTV2CROSSPOINT_INPUT1))
            {
                SetMode(&systemContext,
						NTV2_CHANNEL2,
						NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
            }
            else if (Get425FrameEnable(&systemContext, NTV2_CHANNEL3) &&
					 (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL3 ||
					  pAuto->channelSpec == NTV2CROSSPOINT_INPUT3))
            {
                SetMode(&systemContext,
						NTV2_CHANNEL4,
						NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
            }
            else if (Get425FrameEnable(&systemContext, NTV2_CHANNEL5) &&
					 (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL5 ||
					  pAuto->channelSpec == NTV2CROSSPOINT_INPUT5))
            {
                SetMode(&systemContext,
						NTV2_CHANNEL6,
						NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
            }
            else if (Get425FrameEnable(&systemContext, NTV2_CHANNEL7) &&
					 (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL7 ||
					  pAuto->channelSpec == NTV2CROSSPOINT_INPUT7))
            {
                SetMode(&systemContext,
						NTV2_CHANNEL8,
						NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
            }
        }
		else if(GetQuadQuadFrameEnable(&systemContext, NTV2_CHANNEL1))
		{
			SetMode(&systemContext,
					NTV2_CHANNEL2,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(&systemContext,
					NTV2_CHANNEL3,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(&systemContext,
					NTV2_CHANNEL4,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
		}

        if (Get4kSquaresEnable(&systemContext, ACChannel) &&
				 (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL1 ||
				  pAuto->channelSpec == NTV2CROSSPOINT_INPUT1))
		{
			SetMode(&systemContext,
					NTV2_CHANNEL2,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(&systemContext,
					NTV2_CHANNEL3,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(&systemContext,
					NTV2_CHANNEL4,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
		}
        else if (Get4kSquaresEnable(&systemContext, ACChannel) &&
				 (pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL5 ||
				  pAuto->channelSpec == NTV2CROSSPOINT_INPUT5))
		{
			SetMode(&systemContext,
					NTV2_CHANNEL6,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(&systemContext,
					NTV2_CHANNEL7,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
			SetMode(&systemContext,
					NTV2_CHANNEL8,
					NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) ? NTV2_MODE_CAPTURE : NTV2_MODE_DISPLAY);
		}

		if (pAuto->circulateWithAudio)
		{
			pAuto->audioSystemCount++;
			if (bWithASPlus1)
			{
				ntv2WriteRegisterMS(&systemContext, GetAudioControlRegister(deviceNumber, (NTV2AudioSystem)(pAuto->audioSystem + 1)), 1, kRegMaskMultiLinkAudio, kRegShiftMultiLinkAudio);
				pAuto->audioSystemCount++;
				if (bWithASPlus2)
				{
					ntv2WriteRegisterMS(&systemContext, GetAudioControlRegister(deviceNumber, (NTV2AudioSystem)(pAuto->audioSystem + 2)), 1, kRegMaskMultiLinkAudio, kRegShiftMultiLinkAudio);
					pAuto->audioSystemCount++;
				}
				if (bWithASPlus3)
				{
					ntv2WriteRegisterMS(&systemContext, GetAudioControlRegister(deviceNumber, (NTV2AudioSystem)(pAuto->audioSystem + 3)), 1, kRegMaskMultiLinkAudio, kRegShiftMultiLinkAudio);
					pAuto->audioSystemCount++;
				}
			}
			if (NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec))
			{
				StopAudioCapture(deviceNumber, pAuto->audioSystem);
				StartAudioCapture(deviceNumber, pAuto->audioSystem);
			}
			else
			{
				pAuto->nextAudioOutputAddress = 0;
				pAuto->audioDropsRequired = 0;
				pAuto->audioDropsCompleted = 0;
				StopAudioPlayback(deviceNumber, pAuto->audioSystem);
			}
			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
            {
                MSG("CNTV2Device::oemAutoCirculateInit - Auto %s: circulateWithAudio\n",
                    CrosspointName[pAuto->channelSpec]);
            }
		}

		if (pAuto->circulateWithCustomAncData)
		{
			if (NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec))
			{
				if (pAuto->circulateWithHDMIAux)
				{
					if (NTV2DeviceCanDoCustomAux(pNTV2Params->_DeviceID))
					{
						EnableAuxExtractor(&systemContext, ACChannel, false);
						SetupAuxExtractor(&systemContext, ACChannel);
						EnableAuxExtractor(&systemContext, ACChannel, true);
					}
					else
					{
						pAuto->circulateWithHDMIAux = false;
						pAuto->circulateWithCustomAncData = false;
					}
				}
				else
				{				
					EnableAncInserter(&systemContext, ACChannel, false);
					EnableAncExtractor(&systemContext, ACChannel, false);
					SetupAncExtractor(&systemContext, ACChannel);
					EnableAncExtractor(&systemContext, ACChannel, true);
				}
			}
			else
			{
				EnableAncExtractor(&systemContext, ACChannel, false);
				EnableAncInserter(&systemContext, ACChannel, false);
				SetupAncInserter(&systemContext, ACChannel);
				EnableAncInserter(&systemContext, ACChannel, true);
			}
			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
            {
                MSG("CNTV2Device::oemAutoCirculateInit - Auto %s: circulateWithANC\n",
                    CrosspointName[pAuto->channelSpec]);
            }
		}
		else if (NTV2DeviceCanDoCustomAnc(pNTV2Params->_DeviceID))
		{
			//	Not using custom ANC, so turn off the firmware for the channel
			EnableAncExtractor(&systemContext, ACChannel, false);
			EnableAncInserter(&systemContext, ACChannel, false);
		}

		if (!pAuto->circulateWithHDMIAux)
		{
			if (NTV2_IS_INPUT_CROSSPOINT(pAuto->channelSpec) &&
				NTV2DeviceCanDoCustomAux(pNTV2Params->_DeviceID))
			{
				EnableAuxExtractor(&systemContext, ACChannel, false);
			}
		}

		if (pAuto->circulateWithRP188)
		{
			SetRP188Mode(&systemContext, ACChannel, NTV2_RP188_OUTPUT);
			if (GetQuadFrameEnable(&systemContext, ACChannel) &&
				(pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL1 ||
				 pAuto->channelSpec == NTV2CROSSPOINT_INPUT1))
			{
				SetRP188Mode(&systemContext, NTV2_CHANNEL2, NTV2_RP188_OUTPUT);
				SetRP188Mode(&systemContext, NTV2_CHANNEL3, NTV2_RP188_OUTPUT);
				SetRP188Mode(&systemContext, NTV2_CHANNEL4, NTV2_RP188_OUTPUT);
			}
			if (GetQuadFrameEnable(&systemContext, ACChannel) &&
				(pAuto->channelSpec == NTV2CROSSPOINT_CHANNEL5 ||
				 pAuto->channelSpec == NTV2CROSSPOINT_INPUT5))
			{
				SetRP188Mode(&systemContext, NTV2_CHANNEL6, NTV2_RP188_OUTPUT);
				SetRP188Mode(&systemContext, NTV2_CHANNEL7, NTV2_RP188_OUTPUT);
				SetRP188Mode(&systemContext, NTV2_CHANNEL8, NTV2_RP188_OUTPUT);
			}
			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
            {
                MSG("CNTV2Device::oemAutoCirculateInit - Auto %s: circulateWithRP188\n",
                    CrosspointName[pAuto->channelSpec]);
            }
		}

		pAuto->activeFrame = NTV2_INVALID_FRAME; // No Active Frame until Start is called.
		pAuto->state = NTV2_AUTOCIRCULATE_INIT;
        WriteRegister(deviceNumber, kVRegChannelCrosspointFirst + ACChannel, channelSpecAtIndex, NO_MASK, NO_SHIFT);

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("State of internal struct #%d after INIT:\n", loopCount);
			SHOW_INTERNAL_AUTOCIRCULATE_STRUCT(pAuto);
		}
	}

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
	{
		MSG("AutoCirculateInit completed.\n");
	}

	return 0;
}

int
OemAutoCirculateStart(ULWord deviceNumber, NTV2Crosspoint channelSpec, ULWord64 startTime)
{
	Ntv2SystemContext systemContext;
	NTV2PrivateParams* pNTV2Params;
	char *szMsg;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAutoPrimary;
	ULWord csIndex = 0;
	LWord loopCount = 0;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;
	INTERNAL_FRAME_STAMP_STRUCT *pInternalFrameStamp;
	LWord startFrame = 0;
	NTV2Channel channel = NTV2_CHANNEL1;
	NTV2Channel ACChannel = NTV2_CHANNEL1;
	NTV2Crosspoint pautoChannelSpec;
	systemContext.devNum = deviceNumber;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
		return -ECHRNG;

	pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];

	if (pAutoPrimary->state != NTV2_AUTOCIRCULATE_INIT)
		return -EINVAL;

	csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;

		pAuto = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];
		szMsg = "";
		// Setup register so next frame interrupt will clock in frame values.
		startFrame = pAuto->startFrame;
		pInternalFrameStamp = &pAuto->frameStamp[startFrame];

		// set register update mode for autocirculate
		if (IsMultiFormatActive(&systemContext))
		{
			channel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
		}
		if (IsProgressiveStandard(&systemContext, channel) || pAuto->circulateWithFields)
		{
            if (pAuto->frameStamp[pAuto->startFrame].frameBufferFormat == NTV2_FBF_8BIT_YCBCR_420PL3)
			{
				SetRegisterWriteMode(deviceNumber, channel, NTV2_REGWRITE_SYNCTOFIELD_AFTER10LINES);
			}
			else
			{
				SetRegisterWriteMode(deviceNumber, channel, NTV2_REGWRITE_SYNCTOFIELD);
			}
		}
		else
		{
			SetRegisterWriteMode(deviceNumber, channel, NTV2_REGWRITE_SYNCTOFRAME);
		}

		ACChannel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
		pautoChannelSpec = pAuto->channelSpec;

		if (NTV2_IS_INPUT_CROSSPOINT(pautoChannelSpec))
		{
			SetInputFrame(&systemContext, ACChannel, pAuto->startFrame);
			//InitRP188(pAuto);
			if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
			{
				CopyRP188HardwareToFrameStampTCArray(&systemContext, &pInternalFrameStamp->internalTCArray);
			}

			if (NTV2DeviceCanDoSDIErrorChecks(getNTV2Params(deviceNumber)->_DeviceID))
			{
				CopySDIStatusHardwareToFrameStampSDIStatusArray(&systemContext, &pInternalFrameStamp->internalSDIStatusArray);
			}

			if (pAuto->circulateWithCustomAncData)
			{
				if (pAuto->circulateWithHDMIAux)
				{
					SetAuxExtWriteParams(&systemContext, ACChannel, pAuto->startFrame);
				}
				else
				{
					SetAncExtWriteParams(&systemContext, ACChannel, pAuto->startFrame);
				}
			}
		}
		else // NTV2_IS_INPUT_CROSSPOINT(pautoChannelSpec)
		{
			SetOutputFrame(&systemContext, ACChannel, pAuto->startFrame);
			if (pAuto->circulateWithRP188 || pAuto->circulateWithLTC)
			{
				CopyFrameStampTCArrayToHardware(&systemContext, &pInternalFrameStamp->internalTCArray);
			}
			if (pAuto->circulateWithCustomAncData)
			{
				SetAncInsReadParams(&systemContext, ACChannel, pAuto->startFrame, pInternalFrameStamp->ancTransferSize);
			}
			if (pAuto->circulateWithHDMIAux)
			{
				oemAutoCirculateWriteHDMIAux(deviceNumber,
											 pAuto->frameStamp[pAuto->startFrame].auxData,
											 pAuto->frameStamp[pAuto->startFrame].auxDataSize);
			}
			if (pAuto->circulateWithColorCorrection)
			{
				OemAutoCirculateSetupColorCorrector(deviceNumber,
													pautoChannelSpec,
													&pAuto->frameStamp[pAuto->startFrame].colorCorrectionInfo);
			}
			if (pAuto->circulateWithVidProc)
			{
				OemAutoCirculateSetupVidProc(deviceNumber,
											 pautoChannelSpec,
											 &pAuto->frameStamp[pAuto->startFrame].vidProcInfo);
			}
		}

		if (pAuto->circulateWithLTC)
		{
			if (MsgsEnabled(NTV2_DRIVER_RP188_DEBUG_MESSAGES))
			{
				MSG("%s", szMsg);
			}
		}
		if (pAuto->circulateWithRP188)
		{
			if (MsgsEnabled(NTV2_DRIVER_RP188_DEBUG_MESSAGES))
			{
				MSG("%s", szMsg);
			}
		}
		if (MsgsEnabled(NTV2_DRIVER_CUSTOM_ANC_DATA_DEBUG_MESSAGES))
		{
			MSG("%s", szMsg);
		}

		pAuto->activeFrame = pAuto->startFrame;
		pAuto->nextFrame = pAuto->startFrame;
		pAuto->startTime = startTime;
		pAuto->state = NTV2_AUTOCIRCULATE_STARTING;

		if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
		{
			MSG("OemAutoCirculateStart #%d completed.\n", loopCount);
		}
	}

	return 0;
}

int
OemAutoCirculateStop (ULWord deviceNumber, NTV2Crosspoint channelSpec)
{
	NTV2PrivateParams* pNTV2Params;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAutoPrimary;
	ULWord csIndex = 0;
	LWord loopCount = 0;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
		return -ECHRNG;

	pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];
	csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		csIndex++;

		pAuto = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];

		if (pAuto->state == NTV2_AUTOCIRCULATE_STARTING ||
			pAuto->state == NTV2_AUTOCIRCULATE_INIT ||
			pAuto->state == NTV2_AUTOCIRCULATE_PAUSED ||
			pAuto->state == NTV2_AUTOCIRCULATE_RUNNING)
		{
			pAuto->state = NTV2_AUTOCIRCULATE_STOPPING;
		}
	}

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
	{
		MSG("OemAutoCirculateStop completed.\n");
	}

	return 0;
}

int
OemAutoCirculateAbort (ULWord deviceNumber, NTV2Crosspoint channelSpec)
{
	ULWord csIndex = 0;
	NTV2PrivateParams* pNTV2Params;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAutoPrimary;
	Ntv2SystemContext systemContext;
	LWord loopCount = 0;
	systemContext.devNum = deviceNumber;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
		return -ECHRNG;

	pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];
	csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		INTERNAL_AUTOCIRCULATE_STRUCT *pAuto = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];
		csIndex++;

		if (pAuto->state != NTV2_AUTOCIRCULATE_DISABLED)
		{
		    if (pAuto->recording)
	        {
				if (pAuto->circulateWithAudio)
				{
					StopAudioCapture(deviceNumber, pAuto->audioSystem);
					while (pAuto->audioSystemCount)
					{
						ntv2WriteRegisterMS(&systemContext, GetAudioControlRegister(deviceNumber, (NTV2AudioSystem)(pAuto->audioSystemCount - 1)),
											0, kRegMaskMultiLinkAudio, kRegShiftMultiLinkAudio);
						pAuto->audioSystemCount--;
					}
				}

				if (pAuto->circulateWithCustomAncData)
				{
					if (pAuto->circulateWithHDMIAux)
					{
						EnableAuxExtractor(&systemContext, GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec), false);
					}
					else
					{
						EnableAncExtractor(&systemContext, GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec), false);
					}
				}
	        }
	        else
	        {
				if (pAuto->circulateWithAudio)
				{
					StopAudioPlayback(deviceNumber, pAuto->audioSystem);
					while (pAuto->audioSystemCount)
					{
						ntv2WriteRegisterMS(&systemContext, GetAudioControlRegister(deviceNumber, (NTV2AudioSystem)(pAuto->audioSystemCount - 1)),
											0, kRegMaskMultiLinkAudio, kRegShiftMultiLinkAudio);
						pAuto->audioSystemCount--;
					}
				}
				else
				{
					if (NTV2_IS_OUTPUT_CROSSPOINT(channelSpec) &&
						pNTV2Params->_globalAudioPlaybackMode == NTV2_AUDIOPLAYBACK_1STAUTOCIRCULATEFRAME)
					{
						// not using autocirculate for audio but want it to be synced....crazy.
						StopAudioPlayback(deviceNumber, pAuto->audioSystem);
					}
				}

				if (pAuto->circulateWithCustomAncData)
				{
					EnableAncInserter(&systemContext, GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec), false);
				}
			}
			pAuto->state = NTV2_AUTOCIRCULATE_DISABLED;
            WriteRegister(deviceNumber, kVRegChannelCrosspointFirst + GetNTV2ChannelForNTV2Crosspoint(channelSpecAtIndex),
						  NTV2CROSSPOINT_INVALID, NO_MASK, NO_SHIFT);
		}
	}

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
	{
		MSG("oemAutoCirculateAbort completed.\n");
	}

	return 0;
}

int
OemAutoCirculatePause (ULWord deviceNumber, NTV2Crosspoint channelSpec, bool bPlay, bool bClearDF)
{
	NTV2PrivateParams* pNTV2Params;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAutoPrimary;
	ULWord csIndex = 0;
	LWord loopCount = 0;
	Ntv2SystemContext systemContext;
	systemContext.devNum = deviceNumber;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
	    return -ECHRNG;

	pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];
	csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		INTERNAL_AUTOCIRCULATE_STRUCT *pAuto = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];
		csIndex++;

		if (!bPlay && pAuto->state == NTV2_AUTOCIRCULATE_RUNNING) {
			// Play to pause
			pAuto->state = NTV2_AUTOCIRCULATE_PAUSED;
//			MSG("%s: Pause:    fr(%d) ts(%d)\n", __FUNCTION__, GetFrameRate(&systemContext), pAuto->audioTransferSize);
		} else if (bPlay && (pAuto->state == NTV2_AUTOCIRCULATE_PAUSED)) {
			// Pause to play
			pAuto->state = NTV2_AUTOCIRCULATE_RUNNING;
			if (bClearDF)
				pAuto->droppedFrames = 0;
//			MSG("%s: Un Pause: fr(%d) ts(%d)\n", __FUNCTION__, GetFrameRate(&systemContext), pAuto->audioTransferSize);
		}
	}

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
	{
		MSG("OemAutoCirculatePause, bPlay=%d\n", (int) bPlay);
	}

	return 0;
}

int
OemAutoCirculateFlush (ULWord deviceNumber, NTV2Crosspoint channelSpec, bool bClearDF)
{
	NTV2PrivateParams* pNTV2Params;
	LWord loopCount = 0;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAutoPrimary;
	ULWord csIndex = 0;
	NTV2AutoCirculateState autoState = NTV2_AUTOCIRCULATE_DISABLED;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
	    return -ECHRNG;

	pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];

	if (pAutoPrimary->state != NTV2_AUTOCIRCULATE_RUNNING &&
		pAutoPrimary->state != NTV2_AUTOCIRCULATE_PAUSED &&
		pAutoPrimary->state != NTV2_AUTOCIRCULATE_INIT)
	{
		return 0;
	}

	csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		INTERNAL_AUTOCIRCULATE_STRUCT *pAuto = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];
		// this is usually 1 frame back! pAuto->activeFrame;
		LWord lCurFrame = ReadRegister(deviceNumber, pAuto->activeFrameRegister, NO_MASK, NO_SHIFT);
		LWord lStartFrame = 0;
		csIndex++;

		autoState = pAuto->state;
		pAuto->state = NTV2_AUTOCIRCULATE_PAUSED;
        if (bClearDF)
            pAuto->droppedFrames = 0;

		if ((lCurFrame < (ULWord)pAuto->startFrame) || (lCurFrame > (ULWord)pAuto->endFrame))
		{
			lCurFrame = pAuto->startFrame;
		}
		lStartFrame = lCurFrame;

		if (pAuto->recording)
		{
			// Flush recorded frames
			lCurFrame = KAUTO_PREVFRAME(lCurFrame, pAuto);
			while (lCurFrame != lStartFrame &&
				   pAuto->frameStamp[lCurFrame].validCount != 0)
			{
				// Mark every frame as available for record except
				// the current (active) frame
				pAuto->frameStamp[lCurFrame].validCount = 0;
				lCurFrame = KAUTO_PREVFRAME(lCurFrame, pAuto);
				pAuto->frameStamp[lCurFrame].videoTransferPending = false;
			}
		}
		else
		{
			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
			{
				MSG("Flush active=%ld, first flush=%ld\n", (long) lCurFrame,
					KAUTO_NEXTFRAME(lCurFrame, pAuto));
			}
			// Flush and frames queued for playback (normally
			// occurs in pause mode, but play would work as well
			lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);
			while (lCurFrame != lStartFrame)
			{
				// Mark each frame as empty starting with active frame + 1 and
				// ending (after loop) with active frame which remains valid.  This
				// will be zeroed by the ISR
				pAuto->frameStamp[lCurFrame].validCount = 0;
				pAuto->frameStamp[lCurFrame].videoTransferPending = false;
				lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);
			}

			// clear all if init state
			if (autoState == NTV2_AUTOCIRCULATE_INIT)
			{
				pAuto->frameStamp[lStartFrame].validCount = 0;
				pAuto->frameStamp[lStartFrame].videoTransferPending = false;
			}

			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
			{
				MSG("FlushEnd active=%ld, start flush=%ld\n", (long)pAuto->activeFrame, (long)lStartFrame);
			}

			// flush audio too
			if (pAuto->circulateWithAudio)
			{
				if (!IsAudioPlaybackStopped(deviceNumber, pAuto->audioSystem))
				{
					StopAudioPlayback(deviceNumber, pAuto->audioSystem);
					pAuto->audioDropsRequired++;
				}
			}
		}

		pAuto->state = autoState;
	}

	return 0;
}

int
OemAutoCirculatePreroll (ULWord deviceNumber, NTV2Crosspoint channelSpec, LWord lPrerollFrames)
{
	NTV2PrivateParams* pNTV2Params;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAutoPrimary;
	ULWord csIndex = 0;
	LWord loopCount = 0;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
		return -ECHRNG;

	pAutoPrimary = &pNTV2Params->_AutoCirculate[channelSpec];

	if (pAutoPrimary->state != NTV2_AUTOCIRCULATE_RUNNING &&
		pAutoPrimary->state != NTV2_AUTOCIRCULATE_STARTING &&
		pAutoPrimary->state != NTV2_AUTOCIRCULATE_PAUSED)
	{
		return 0;
	}
	if (pAutoPrimary->activeFrame < pAutoPrimary->startFrame ||
		pAutoPrimary->activeFrame > pAutoPrimary->endFrame)
	{
		return -EINVAL;
	}

	csIndex = GetIndexForNTV2Crosspoint(channelSpec);

	for (loopCount = 0; loopCount < pAutoPrimary->channelCount; loopCount++)
	{
		NTV2Crosspoint channelSpecAtIndex = GetNTV2CrosspointForIndex(csIndex);
		INTERNAL_AUTOCIRCULATE_STRUCT *pAuto = &pNTV2Params->_AutoCirculate[channelSpecAtIndex];
		ULWord lCurFrame = pAuto->activeFrame;
		csIndex++;

		if (pAuto->recording)
		{
			// This would be neg value if supported
		}
		else
		{
			// Always preroll last frame (rev->fwd transitions)
			lCurFrame = OemAutoCirculateFindNextAvailFrame(pAuto);
			if (lCurFrame == NTV2_INVALID_FRAME)
			{
				return -EINVAL;
			}
			if (lCurFrame != pAuto->activeFrame)
			{
				lCurFrame = KAUTO_PREVFRAME(lCurFrame, pAuto);
			}
			if (lCurFrame == (ULWord)pAuto->activeFrame)
			{
				pAuto->state = NTV2_AUTOCIRCULATE_STARTING;
				if (IsAudioPlaying(deviceNumber, pAuto->audioSystem))
				{
					StopAudioPlayback(deviceNumber, pAuto->audioSystem);
				}
				pAuto->nextAudioOutputAddress = 0;
			}
			// Use signed arithmetic to all reduction of preroll as well as addition
			pAuto->frameStamp[lCurFrame].validCount =
				(LWord)pAuto->frameStamp[lCurFrame].validCount + lPrerollFrames;
			if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
			{
				MSG("Preroll %ld frame (%ld) == %ld ",
					(long)lCurFrame, (long)lPrerollFrames,
					(long)pAuto->frameStamp[lCurFrame].validCount);
			}
		}
	}

	return 0;
}

void
OemAutoCirculateReset (ULWord deviceNumber, NTV2Crosspoint channelSpec)
{
	NTV2PrivateParams* pNTV2Params;
	INTERNAL_AUTOCIRCULATE_STRUCT *pAuto;
	NTV2Channel ACChannel = NTV2_CHANNEL1;
	int j = 0;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	if (ILLEGAL_CHANNELSPEC(channelSpec))
	    return;

	pAuto = &pNTV2Params->_AutoCirculate[channelSpec];

	memset(pAuto, 0, sizeof(INTERNAL_AUTOCIRCULATE_STRUCT));
	ACChannel = GetNTV2ChannelForNTV2Crosspoint(channelSpec);
	pAuto->channelSpec = channelSpec;
	pAuto->deviceNumber = deviceNumber;

	if (NTV2_IS_INPUT_CROSSPOINT(channelSpec))
	{
		pAuto->recording = true;
		pAuto->activeFrameRegister = gChannelToInputFrameReg[ACChannel];
	}
	else
	{
		pAuto->recording = false;
		pAuto->activeFrameRegister = gChannelToOutputFrameReg[ACChannel];
	}

	for (j = 0; j < GetNumFrameBuffers(deviceNumber, getNTV2Params(deviceNumber)->_DeviceID); j++)
	{
	    memset(&(pAuto->frameStamp[j]), 0, sizeof(INTERNAL_FRAME_STAMP_STRUCT));
	    memset(&(pAuto->frameStamp[j].internalTCArray), 0xFF, sizeof(INTERNAL_TIMECODE_STRUCT));
	}

	oemAutoCirculateTaskInit(&pAuto->recordTaskInfo,
							 pAuto->recordTaskArray,
							 AUTOCIRCULATE_TASK_MAX_TASKS);

	pAuto->state = NTV2_AUTOCIRCULATE_DISABLED;
    WriteRegister(deviceNumber, kVRegChannelCrosspointFirst + ACChannel, NTV2CROSSPOINT_INVALID, NO_MASK, NO_SHIFT);

	if (MsgsEnabled(NTV2_DRIVER_AUTOCIRCULATE_CONTROL_DEBUG_MESSAGES))
	{
		MSG("oemAutoCirculateReset completed.\n");
	}
}

// This should work with any start/end pair
static long
KAUTO_NEXTFRAME(LWord __dwCurFrame_, INTERNAL_AUTOCIRCULATE_STRUCT* __pAuto_)
{
	// Get around -1 mod
	if (__dwCurFrame_ < __pAuto_->endFrame)
	{
		return __dwCurFrame_ + 1;
	}
	return __pAuto_->startFrame;
};

static long
KAUTO_PREVFRAME(LWord __dwCurFrame_, INTERNAL_AUTOCIRCULATE_STRUCT* __pAuto_)
{
	// Get around -1 mod
	if (__dwCurFrame_ > __pAuto_->startFrame)
	{
		return __dwCurFrame_ - 1;
	}
	return __pAuto_->endFrame;
};

/**
 * Internal
 * recording. how many frames ready to record
 * playback   how many frames buffered up
 */
static ULWord
OemAutoCirculateGetBufferLevel(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto)
{
	LWord lCurFrame = pAuto->activeFrame;
	LWord lRange = (pAuto->endFrame - pAuto->startFrame) + 1;
	ULWord dwBufferLevel = 0;
	LWord i;

	if (pAuto->state == NTV2_AUTOCIRCULATE_INIT)
    {
        // activeFrame is -1;  We want to know how many have been pre-loaded, starting at startFrame
	    lCurFrame = pAuto->startFrame;
    }
    else
    {
        // we want to know how many are valid, starting from the active frame.
	    lCurFrame = pAuto->activeFrame;
    }

	if ((pAuto->state != NTV2_AUTOCIRCULATE_INIT) &&
        ((pAuto->activeFrame < pAuto->startFrame) || (pAuto->activeFrame > pAuto->endFrame)))
    {
        // Bail out if activeFrame is not valid, and we're past the INIT stage.
        // (In INIT state, activeFrame will be -1, so we can't bail out because of that.
        // We want the BufferLevel to represent the number of pre-loaded frames.)
        return 0;
	}

	if (pAuto->recording)
	{
	    // No frames available to record till state = NTV2_AUTOCIRCULATE_RUNNING
	    if (pAuto->state != NTV2_AUTOCIRCULATE_RUNNING)
	        return 0;

	    // Search backward until a '0' is found which indicates the first oldest frame recorded.
		for (i = 1; i < lRange; i++)
	    {
			// Normalize for non zero starts
			lCurFrame = KAUTO_PREVFRAME(lCurFrame, pAuto);
			if (pAuto->frameStamp[lCurFrame].validCount == 0 ||	// Found the next to get record frame
				i >= lRange)	// All buffers are used, (dropping)
	        {
				// Found it
				return dwBufferLevel;
			}
			dwBufferLevel++;	// pAuto->frameStamp[lCurFrame].validCount always 0 or 1 for now
	    }
	}
	else
	{
		// Search forward for a '0' which indicates the next free buffer
		dwBufferLevel = pAuto->frameStamp[lCurFrame].validCount;

		for (i = 1; i < (pAuto->endFrame - pAuto->startFrame) + 1; i++)
	    {
			lCurFrame = KAUTO_NEXTFRAME(lCurFrame, pAuto);
			if (pAuto->frameStamp[lCurFrame].validCount == 0 ||	// Found empty frame
				i >= lRange)	// Buffers completely full
	        {
				// Found it
				return dwBufferLevel;
			}
			dwBufferLevel += pAuto->frameStamp[lCurFrame].validCount;
		}
	}

	return dwBufferLevel;	// None available, or play full
}

static void
OemBeginAutoCirculateTransfer (ULWord deviceNumber,
							   ULWord frameNumber,
							   AUTOCIRCULATE_TRANSFER_STRUCT *pTransferStruct,
							   INTERNAL_AUTOCIRCULATE_STRUCT *pAuto,
							   NTV2RoutingTable* pNTV2RoutingTable,
							   PAUTOCIRCULATE_TASK_STRUCT pTaskInfo)
{
	if (pAuto->recording == 0)	// If not recording
	{
		// Copy PLAY data for frame stamp
		pAuto->frameStamp[frameNumber].rp188 = pTransferStruct->rp188;
		pAuto->frameStamp[frameNumber].repeatCount = pTransferStruct->frameRepeatCount;
		pAuto->frameStamp[frameNumber].hUser = pTransferStruct->hUser;
		pAuto->frameStamp[frameNumber].frameBufferFormat = pTransferStruct->frameBufferFormat;
		pAuto->frameStamp[frameNumber].frameBufferOrientation = pTransferStruct->frameBufferOrientation;

		// VidProc, color correction, custom ancillary data
		if (pAuto->circulateWithColorCorrection)
		{
			OemAutoCirculateTransferColorCorrectorInfo(deviceNumber,
													   &pAuto->frameStamp[frameNumber].colorCorrectionInfo,
													   &pTransferStruct->colorCorrectionInfo);
		}

		if (pAuto->circulateWithVidProc)
		{
			pAuto->frameStamp[frameNumber].vidProcInfo = pTransferStruct->vidProcInfo;
		}

		if (pNTV2RoutingTable)
		{
			pAuto->frameStamp[frameNumber].ntv2RoutingTable = *pNTV2RoutingTable;
		}
		else
		{
			memset(&pAuto->frameStamp[frameNumber].ntv2RoutingTable, 0, sizeof(NTV2RoutingTable));
		}

		oemAutoCirculateTaskInit(&pAuto->frameStamp[frameNumber].taskInfo,
								 pAuto->frameStamp[frameNumber].taskArray,
								 AUTOCIRCULATE_TASK_MAX_TASKS);

		if ((pTaskInfo != NULL) && (pTaskInfo->numTasks > 0))
		{
			oemAutoCirculateTaskTransfer(&pAuto->frameStamp[frameNumber].taskInfo, pTaskInfo, true);
		}
	}
}

static void
OemBeginAutoCirculateTransfer_Ex (ULWord deviceNumber,
								  ULWord frameNumber,
								  AUTOCIRCULATE_TRANSFER *pTransferStruct,
								  INTERNAL_AUTOCIRCULATE_STRUCT *pAuto)
{
	Ntv2SystemContext systemContext;
	systemContext.devNum = deviceNumber;

	pAuto->transferFrame = frameNumber;
	pAuto->audioTransferOffset = 0;
	pAuto->audioTransferSize = 0;
	pAuto->ancTransferOffset = 0;
	pAuto->ancTransferSize = 0;
	pAuto->ancField2TransferOffset = 0;

	if (pAuto->circulateWithCustomAncData)
	{
		NTV2Channel channel = GetNTV2ChannelForNTV2Crosspoint(pAuto->channelSpec);
		ULWord frameSize = GetFrameBufferSize(&systemContext, (channel < NTV2_CHANNEL5) ? NTV2_CHANNEL1 : NTV2_CHANNEL5);
		pAuto->ancTransferOffset = frameSize - ReadRegister(deviceNumber, kVRegAncField1Offset, NO_MASK, NO_SHIFT);
		pAuto->ancField2TransferOffset = frameSize - ReadRegister(deviceNumber, kVRegAncField2Offset, NO_MASK, NO_SHIFT);
	}

	if (pAuto->recording)
	{
		if (pAuto->circulateWithCustomAncData)
		{
            pAuto->ancTransferSize = pTransferStruct->acANCBuffer.fByteCount;
            pAuto->ancField2TransferSize = pTransferStruct->acANCField2Buffer.fByteCount;
		}
	}
	else
	{
		// Copy PLAY data for frame stamp
		NTV2_RP188		inTCArray[NTV2_MAX_NUM_TIMECODE_INDEXES];
		ULWord			byteCount = pTransferStruct->acOutputTimeCodes.fByteCount;

		memset(inTCArray, 0, sizeof(inTCArray));
		if (byteCount && pTransferStruct->acOutputTimeCodes.fUserSpacePtr)
		{
			if (copy_from_user((void *)inTCArray, (const void *)pTransferStruct->acOutputTimeCodes.fUserSpacePtr, byteCount))
			{
				printk("OemBeginAutoCirculateTransfer_Ex copy_from_user failed\n");
			}
			else
			{
				//	On playout, if the user-space client allocated the acOutputTimeCodes field of the AUTOCIRCULATE_TRANSFER struct,
				//	it's supposed to be large enough to hold up to NTV2_MAX_NUM_TIMECODE_INDEXES of NTV2_RP188 structs. Only "valid"
				//	NTV2_RP188 values will be played out (i.e., those whose DBB, Hi and Lo fields are not 0xFFFFFFFF)...
				CopyNTV2TimeCodeArrayToFrameStampTCArray(&pAuto->frameStamp[frameNumber].internalTCArray, inTCArray, byteCount);
			}
		}

		//	Fill in PLAY data for frame stamp...
		if (byteCount && NTV2_RP188_IS_VALID(inTCArray[NTV2_TCINDEX_DEFAULT]))
		{
			RP188_STRUCT_from_NTV2_RP188(pAuto->frameStamp[frameNumber].rp188, inTCArray[NTV2_TCINDEX_DEFAULT]);
		}
		else if (NTV2_RP188_IS_VALID(pTransferStruct->acRP188))
		{
			//	acRP188 field is deprecated
			RP188_STRUCT_from_NTV2_RP188(pAuto->frameStamp[frameNumber].rp188, pTransferStruct->acRP188);
		}
		pAuto->frameStamp[frameNumber].repeatCount = pTransferStruct->acFrameRepeatCount;
		pAuto->frameStamp[frameNumber].hUser = pTransferStruct->acInUserCookie;
		pAuto->frameStamp[frameNumber].frameBufferFormat = pTransferStruct->acFrameBufferFormat;
		pAuto->frameStamp[frameNumber].frameBufferOrientation = pTransferStruct->acFrameBufferOrientation;

		if (pAuto->circulateWithCustomAncData)
		{
			if ((PULWord)pTransferStruct->acANCBuffer.fUserSpacePtr != NULL)
			{
				pAuto->ancTransferSize = pTransferStruct->acANCBuffer.fByteCount;
			}
            else if (pTransferStruct->acANCBuffer.fUserSpacePtr == 0 || pTransferStruct->acANCBuffer.fByteCount == 0)
            {
                pAuto->ancTransferSize = 0;
            }
			if ((PULWord)pTransferStruct->acANCField2Buffer.fUserSpacePtr != NULL)
			{
				pAuto->ancField2TransferSize = pTransferStruct->acANCField2Buffer.fByteCount;
			}
            else if (pTransferStruct->acANCField2Buffer.fUserSpacePtr == 0 || pTransferStruct->acANCField2Buffer.fByteCount == 0)
            {
                pAuto->ancField2TransferSize = 0;
            }
		}
		pAuto->frameStamp[frameNumber].ancTransferSize = pAuto->ancTransferSize;
		pAuto->frameStamp[frameNumber].ancField2TransferSize = pAuto->ancField2TransferSize;

		if (pAuto->circulateWithHDMIAux)
		{
			ULWord maxCount = NTV2_HDMIAuxMaxFrames * NTV2_HDMIAuxDataSize;
			memset(pAuto->frameStamp[frameNumber].auxData, 0, maxCount);
			if ((PULWord)pTransferStruct->acHDMIAuxData.fUserSpacePtr != NULL)
			{
				void* pInAuxDataArray = (void*)(pTransferStruct->acHDMIAuxData.fUserSpacePtr);
				ULWord byteCount = pTransferStruct->acHDMIAuxData.fByteCount;
				ULWord rc = 0;
					
				if (byteCount > maxCount)
					byteCount = maxCount;
				rc = copy_from_user(pAuto->frameStamp[frameNumber].auxData,	(const void*)pInAuxDataArray, byteCount);
				if (rc)
					byteCount = 0;
				pAuto->frameStamp[frameNumber].auxDataSize = byteCount;
			}
		}
	}
}

static void
OemCompleteAutoCirculateTransfer(ULWord deviceNumber,
								 ULWord frameNumber,
								 AUTOCIRCULATE_TRANSFER_STATUS_STRUCT *pUserOutBuffer,
								 INTERNAL_AUTOCIRCULATE_STRUCT *pAuto,
								 bool updateValid, bool transferPending)
{
	ULWord64 RDTSC = 0;
	ULWord ulCurrentFrame = NTV2_INVALID_FRAME;
	NTV2PrivateParams* pNTV2Params;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	// Settings relevant to the app specified frameNumber
    if (frameNumber >= (ULWord)pAuto->startFrame && frameNumber <= (ULWord)pAuto->endFrame)
    {
        pUserOutBuffer->transferFrame = frameNumber;
        pUserOutBuffer->frameStamp.audioClockTimeStamp =
            pAuto->frameStamp[frameNumber].audioClockTimeStamp;
        pUserOutBuffer->frameStamp.currentFieldCount = 0;
        pUserOutBuffer->frameStamp.currentLineCount = 0;
        pUserOutBuffer->frameStamp.currenthUser = pAuto->frameStamp[frameNumber].hUser;

        // If recording, the app specified frame (just dma'd) has all relevant timing data
        if (pAuto->recording)
        {
			if (updateValid)
			{
				pAuto->frameStamp[frameNumber].validCount = 0;
			}
            pUserOutBuffer->frameStamp.frame = frameNumber;
            pUserOutBuffer->frameStamp.frameTime = pAuto->frameStamp[frameNumber].frameTime;
            pUserOutBuffer->frameStamp.audioInStartAddress =
                pAuto->frameStamp[frameNumber].audioInStartAddress;
            pUserOutBuffer->frameStamp.audioInStopAddress =
                pAuto->frameStamp[frameNumber].audioInStopAddress;
            pUserOutBuffer->frameStamp.startSample =
                pAuto->frameStamp[frameNumber].audioInStartAddress;
            pUserOutBuffer->frameStamp.currentReps =
                pAuto->frameStamp[frameNumber].validCount;	// For drop detect
            pUserOutBuffer->frameStamp.currentRP188 = pAuto->frameStamp[frameNumber].rp188;
            pUserOutBuffer->audioBufferSize = pAuto->audioTransferSize;
            pUserOutBuffer->audioStartSample = pAuto->audioStartSample;
        }
        // if playing, the frame just dma'd to card doesn't have timing data
        else
        {
			pAuto->frameStamp[frameNumber].videoTransferPending = transferPending;
			if (updateValid)
			{
				pAuto->frameStamp[frameNumber].validCount = pAuto->frameStamp[frameNumber].repeatCount;
			}
            pUserOutBuffer->frameStamp.currentReps = pAuto->frameStamp[frameNumber].repeatCount;
        }
    } else {
        // ERROR!
        pUserOutBuffer->transferFrame = NTV2_INVALID_FRAME;
    }

    // Global settings
    my_rdtscll(RDTSC);
	pUserOutBuffer->frameStamp.currentTime = RDTSC;
    pUserOutBuffer->bufferLevel = OemAutoCirculateGetBufferLevel(pAuto);
    pUserOutBuffer->state = pAuto->state;
    pUserOutBuffer->channelSpec = pAuto->channelSpec;
    pUserOutBuffer->framesDropped = pAuto->droppedFrames;
    pUserOutBuffer->framesProcessed = pAuto->framesProcessed;
    pUserOutBuffer->frameStamp.currentAudioExpectedAddress =
        pUserOutBuffer->frameStamp.audioExpectedAddress;

    // Current (active) frame settings
    ulCurrentFrame = pAuto->activeFrame;
	pUserOutBuffer->frameStamp.currentFrame = ulCurrentFrame;
	if (ulCurrentFrame >= (ULWord)pAuto->startFrame && ulCurrentFrame <= (ULWord)pAuto->endFrame)
	{
		pUserOutBuffer->frameStamp.currentFrameTime = pAuto->frameStamp[ulCurrentFrame].frameTime;
		pUserOutBuffer->frameStamp.audioClockCurrentTime = GetAudioClock(deviceNumber);

		// If playing, fill in timing data from the current (active) frame
		if (!pAuto->recording)
		{
			pUserOutBuffer->frameStamp.frame = ulCurrentFrame;
			pUserOutBuffer->frameStamp.frameTime = pAuto->frameStamp[ulCurrentFrame].frameTime;
			pUserOutBuffer->frameStamp.audioOutStartAddress =
				pAuto->frameStamp[ulCurrentFrame].audioOutStartAddress;
			pUserOutBuffer->frameStamp.audioOutStopAddress =
				pAuto->frameStamp[ulCurrentFrame].audioOutStopAddress;
			pUserOutBuffer->frameStamp.startSample =
				pAuto->frameStamp[ulCurrentFrame].audioInStartAddress;
			pUserOutBuffer->frameStamp.audioExpectedAddress =
				pAuto->frameStamp[ulCurrentFrame].audioExpectedAddress;
		}
	}
	else
	{
		pUserOutBuffer->frameStamp.currentFrameTime = 0;
		pUserOutBuffer->frameStamp.audioClockCurrentTime = GetAudioClock(deviceNumber);

		// If playing, fill in timing data from the current (active) frame
		if (!pAuto->recording)
		{
			pUserOutBuffer->frameStamp.frame = ulCurrentFrame;
			pUserOutBuffer->frameStamp.frameTime = 0;
			pUserOutBuffer->frameStamp.audioOutStartAddress = 0;
			pUserOutBuffer->frameStamp.audioOutStopAddress = 0;
			pUserOutBuffer->frameStamp.startSample = 0;
			pUserOutBuffer->frameStamp.audioExpectedAddress = 0;
		}
	}
}

static void
OemCompleteAutoCirculateTransfer_Ex(ULWord deviceNumber,
	  								ULWord frameNumber,
	  								AUTOCIRCULATE_TRANSFER_STATUS *pUserOutBuffer,
									INTERNAL_AUTOCIRCULATE_STRUCT *pAuto,
									bool updateValid, bool transferPending)
{
	ULWord64 RDTSC = 0;
	ULWord ulCurrentFrame = NTV2_INVALID_FRAME;
	NTV2PrivateParams* pNTV2Params;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	// Settings relevant to the app specified frameNumber
    if (frameNumber >= (ULWord)pAuto->startFrame && frameNumber <= (ULWord)pAuto->endFrame)
    {
        pUserOutBuffer->acTransferFrame = frameNumber;
        pUserOutBuffer->acFrameStamp.acAudioClockTimeStamp =
            pAuto->frameStamp[frameNumber].audioClockTimeStamp;
        pUserOutBuffer->acFrameStamp.acCurrentFieldCount = 0;
        pUserOutBuffer->acFrameStamp.acCurrentLineCount = 0;
        pUserOutBuffer->acFrameStamp.acCurrentUserCookie = pAuto->frameStamp[frameNumber].hUser;

        // If recording, the app specified frame (just dma'd) has all relevant timing data
        if (pAuto->recording)
        {
			NTV2_RP188	outTCArray[NTV2_MAX_NUM_TIMECODE_INDEXES];
			memset((void *) outTCArray, 0xFF, sizeof(outTCArray));

			if (updateValid)
			{
				pAuto->frameStamp[frameNumber].validCount = 0;
			}
            pUserOutBuffer->acFrameStamp.acFrame = frameNumber;
            pUserOutBuffer->acFrameStamp.acFrameTime = pAuto->frameStamp[frameNumber].frameTime;
			pUserOutBuffer->acFrameStamp.acAudioInStartAddress =
                pAuto->frameStamp[frameNumber].audioInStartAddress;
            pUserOutBuffer->acFrameStamp.acAudioInStopAddress =
                pAuto->frameStamp[frameNumber].audioInStopAddress;
            pUserOutBuffer->acFrameStamp.acStartSample =
                pAuto->frameStamp[frameNumber].audioInStartAddress;
            pUserOutBuffer->acFrameStamp.acCurrentReps =
                pAuto->frameStamp[frameNumber].validCount;	// For drop detect
            pUserOutBuffer->acAudioTransferSize = pAuto->audioTransferSize;
            pUserOutBuffer->acAudioStartSample = pAuto->audioStartSample;
			NTV2_RP188P_from_RP188_STRUCT (&pUserOutBuffer->acFrameStamp.acRP188, pAuto->frameStamp[frameNumber].rp188);

			if (pUserOutBuffer->acFrameStamp.acTimeCodes.fByteCount &&
				pUserOutBuffer->acFrameStamp.acTimeCodes.fUserSpacePtr)
			{
				CopyFrameStampTCArrayToNTV2TimeCodeArray(&pAuto->frameStamp[frameNumber].internalTCArray, (NTV2_RP188*)outTCArray, pUserOutBuffer->acFrameStamp.acTimeCodes.fByteCount);
				if (copy_to_user((void *) pUserOutBuffer->acFrameStamp.acTimeCodes.fUserSpacePtr,
								 (const void *) outTCArray,
								 pUserOutBuffer->acFrameStamp.acTimeCodes.fByteCount))
				{
					printk("OemCompleteAutoCirculateTransfer_Ex copy_to_user failed\n");
				}
			}

#if 0	//	Don't use until AJA_MESSAGE is fixed to handle SDI status 
			if (pUserOutBuffer->acFrameStamp.acTimeCodes.fByteCount &&
				pUserOutBuffer->acFrameStamp.acTimeCodes.fUserSpacePtr)
			{
				if (copy_to_user((void *) pUserOutBuffer->acFrameStamp.acTimeCodes.fUserSpacePtr,
								 (const void *) outTCArray,
								 pUserOutBuffer->acFrameStamp.acTimeCodes.fByteCount))
				{
					printk("OemCompleteAutoCirculateTransfer_Ex copy_to_user failed\n");
				}
			}
#endif
            pUserOutBuffer->acAncTransferSize = pAuto->frameStamp[frameNumber].ancTransferSize;
            pUserOutBuffer->acAncField2TransferSize = pAuto->frameStamp[frameNumber].ancField2TransferSize;
        }
        // if playing, the frame just dma'd to card doesn't have timing data
        else
        {
			pAuto->frameStamp[frameNumber].videoTransferPending = transferPending;
			if (updateValid)
			{
				pAuto->frameStamp[frameNumber].validCount = pAuto->frameStamp[frameNumber].repeatCount;
			}
            pUserOutBuffer->acFrameStamp.acCurrentReps = pAuto->frameStamp[frameNumber].repeatCount;
        }
    } else {
        // ERROR!
        pUserOutBuffer->acTransferFrame = NTV2_INVALID_FRAME;
    }

    // Global settings
    my_rdtscll(RDTSC);
	pUserOutBuffer->acFrameStamp.acCurrentTime = RDTSC;
    pUserOutBuffer->acBufferLevel = OemAutoCirculateGetBufferLevel(pAuto);
    pUserOutBuffer->acState = pAuto->state;
    pUserOutBuffer->acFramesDropped = pAuto->droppedFrames;
    pUserOutBuffer->acFramesProcessed = pAuto->framesProcessed;
    pUserOutBuffer->acFrameStamp.acCurrentAudioExpectedAddress =
        pUserOutBuffer->acFrameStamp.acAudioExpectedAddress;

    // Current (active) frame settings
    ulCurrentFrame = pAuto->activeFrame;
	pUserOutBuffer->acFrameStamp.acCurrentFrame = ulCurrentFrame;
	if (ulCurrentFrame >= (ULWord)pAuto->startFrame && ulCurrentFrame <= (ULWord)pAuto->endFrame)
	{
		pUserOutBuffer->acFrameStamp.acCurrentFrameTime = pAuto->frameStamp[ulCurrentFrame].frameTime;
		pUserOutBuffer->acFrameStamp.acAudioClockCurrentTime = GetAudioClock(deviceNumber);

		// If playing, fill in timing data from the current (active) frame
		if (!pAuto->recording)
		{
			pUserOutBuffer->acFrameStamp.acFrame = ulCurrentFrame;
			pUserOutBuffer->acFrameStamp.acFrameTime = pAuto->frameStamp[ulCurrentFrame].frameTime;
			pUserOutBuffer->acFrameStamp.acAudioOutStartAddress =
				pAuto->frameStamp[ulCurrentFrame].audioOutStartAddress;
			pUserOutBuffer->acFrameStamp.acAudioOutStopAddress =
				pAuto->frameStamp[ulCurrentFrame].audioOutStopAddress;
			pUserOutBuffer->acFrameStamp.acStartSample =
				pAuto->frameStamp[ulCurrentFrame].audioInStartAddress;
			pUserOutBuffer->acFrameStamp.acAudioExpectedAddress =
				pAuto->frameStamp[ulCurrentFrame].audioExpectedAddress;
		}
	}
	else
	{

		pUserOutBuffer->acFrameStamp.acCurrentFrameTime = 0;
		pUserOutBuffer->acFrameStamp.acAudioClockCurrentTime = GetAudioClock(deviceNumber);

		// If playing, fill in timing data from the current (active) frame
		if (!pAuto->recording)
		{
			pUserOutBuffer->acFrameStamp.acFrame = ulCurrentFrame;
			pUserOutBuffer->acFrameStamp.acFrameTime = 0;
			pUserOutBuffer->acFrameStamp.acAudioOutStartAddress = 0;
			pUserOutBuffer->acFrameStamp.acAudioOutStopAddress = 0;
			pUserOutBuffer->acFrameStamp.acStartSample = 0;
			pUserOutBuffer->acFrameStamp.acAudioExpectedAddress = 0;
		}
	}
}

///////////////////////////////////////////////////////////////////////
// IsField0...assumes interlaced format
///////////////////////////////////////////////////////////////////////
static bool
IsField0(ULWord deviceNumber, NTV2Crosspoint channelSpec)
{
    bool bField0 = true;   // return value        // check for Audio and Vertical Interrupts

	ULWord statusRegister = ReadStatusRegister(deviceNumber);
	ULWord status2Register = ReadStatus2Register(deviceNumber);
	Ntv2SystemContext systemContext;
	systemContext.devNum = deviceNumber;

    switch (channelSpec)
    {
	default:
	case NTV2CROSSPOINT_CHANNEL1:
	{
		if (statusRegister & (BIT_23))  // Output Field ID
		{
			bField0 = false;
		}
	}
	break;

	case NTV2CROSSPOINT_CHANNEL2:
		if (IsMultiFormatActive(&systemContext))
		{
			if (statusRegister & (BIT_5))  // Output 2 Field ID
			{
				bField0 = false;
			}
		}
		else
		{
			if (statusRegister & (BIT_23))  // Output Field ID
			{
				bField0 = false;
			}
		}
		break;

	case NTV2CROSSPOINT_CHANNEL3:
		if (IsMultiFormatActive(&systemContext))
		{
			if (statusRegister & (BIT_3))  // Output 3 Field ID
			{
				bField0 = false;
			}
		}
		else
		{
			if (statusRegister & (BIT_23))  // Output Field ID
			{
				bField0 = false;
			}
		}
		break;

	case NTV2CROSSPOINT_CHANNEL4:
		if (IsMultiFormatActive(&systemContext))
		{
			if (statusRegister & (BIT_1))  // Output 4 Field ID
			{
				bField0 = false;
			}
		}
		else
		{
			if (statusRegister & (BIT_23))  // Output Field ID
			{
				bField0 = false;
			}
		}
		break;

	case NTV2CROSSPOINT_CHANNEL5:
		if (IsMultiFormatActive(&systemContext))
		{
			if (status2Register & (BIT_9))  // Output 5 Field ID
			{
				bField0 = false;
			}
		}
		else
		{
			if (statusRegister & (BIT_23))  // Output Field ID
			{
				bField0 = false;
			}
		}
		break;

	case NTV2CROSSPOINT_CHANNEL6:
		if (IsMultiFormatActive(&systemContext))
		{
			if (status2Register & (BIT_7))  // Output 6 Field ID
			{
				bField0 = false;
			}
		}
		else
		{
			if (statusRegister & (BIT_23))  // Output Field ID
			{
				bField0 = false;
			}
		}
		break;

	case NTV2CROSSPOINT_CHANNEL7:
		if (IsMultiFormatActive(&systemContext))
		{
			if (status2Register & (BIT_5))  // Output 7 Field ID
			{
				bField0 = false;
			}
		}
		else
		{
			if (statusRegister & (BIT_23))  // Output Field ID
			{
				bField0 = false;
			}
		}
		break;

	case NTV2CROSSPOINT_CHANNEL8:
		if (IsMultiFormatActive(&systemContext))
		{
			if (status2Register & (BIT_3))  // Output 8 Field ID
			{
				bField0 = false;
			}
		}
		else
		{
			if (statusRegister & (BIT_23))  // Output Field ID
			{
				bField0 = false;
			}
		}
		break;

	case NTV2CROSSPOINT_INPUT1:
	{
		if (statusRegister & (BIT_21))  // Input 1 Field ID
		{
			bField0 = false;
		}
		break;
	}

	case NTV2CROSSPOINT_INPUT2:
	{
		if (statusRegister & (BIT_19))  // Input 2 Field ID
		{
			bField0 = false;
		}
	break;
	}

	case NTV2CROSSPOINT_INPUT3:
	{
		if (status2Register & (BIT_21))  // Input 3 Field ID
		{
			bField0 = false;
		}
		break;
	}

	case NTV2CROSSPOINT_INPUT4:
	{
		if (status2Register & (BIT_19))  // Input 4 Field ID
		{
			bField0 = false;
		}
		break;
	}

	case NTV2CROSSPOINT_INPUT5:
	{
		if (status2Register & (BIT_17))  // Input 5 Field ID
		{
			bField0 = false;
		}
		break;
	}

	case NTV2CROSSPOINT_INPUT6:
	{
		if (status2Register & (BIT_15))  // Input 6 Field ID
		{
			bField0 = false;
		}
		break;
	}

	case NTV2CROSSPOINT_INPUT7:
	{
		if (status2Register & (BIT_13))  // Input 7 Field ID
		{
			bField0 = false;
		}
		break;
	}

	case NTV2CROSSPOINT_INPUT8:
	{
		if (status2Register & (BIT_11))  // Input 8 Field ID
		{
			bField0 = false;
		}
		break;
	}
    }
	
    return bField0;
}

//static ULWord64
ULWord64
GetAudioClock(ULWord deviceNumber)
{
	ULWord ulCurrentCount;
	ULWord64 ullRetVal;
	unsigned long flags;
	NTV2PrivateParams* pNTV2Params;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	ntv2_spin_lock_irqsave(&pNTV2Params->_audioClockLock, flags);

	ulCurrentCount= ReadAudioSampleCounter(deviceNumber);
	if (ulCurrentCount<pNTV2Params->_ulLastClockSampleCounter) //wrapped?
		pNTV2Params->_ulNumberOfWrapsOfClockSampleCounter++;
	pNTV2Params->_ulLastClockSampleCounter=ulCurrentCount; //update.
	ullRetVal=((ULWord64)ulCurrentCount)+(((ULWord64)pNTV2Params->_ulNumberOfWrapsOfClockSampleCounter)<<32);

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_audioClockLock, flags);

	ullRetVal=ullRetVal*10000;
	if (NTV2DeviceCanDoAudio96K(pNTV2Params->_DeviceID))
		// May need to change if a 2 channel 48/96 board exists in the future
		do_div(ullRetVal, (ULWord64)(GetAudioSamplesPerSecond(deviceNumber, NTV2_AUDIOSYSTEM_1)/1000));
	else
		do_div(ullRetVal, (ULWord64)(48));

	return ullRetVal;
}

static unsigned long
GetAudioControlRegisterAddressAndLock(ULWord deviceNumber, NTV2AudioSystem audioSystem, spinlock_t** ppLock)
{
	NTV2PrivateParams* pBoard = getNTV2Params(deviceNumber);
	ULWord _DeviceID = pBoard->_DeviceID;
	if (ppLock)
        *ppLock = &(pBoard->_registerSpinLock);
	if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL2) && audioSystem == NTV2_AUDIOSYSTEM_2)
	{
		return pBoard->_pAudio2Control;
	}
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL3) && audioSystem == NTV2_AUDIOSYSTEM_3)
	{
		return pBoard->_pAudio3Control;
	}
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL4) && audioSystem == NTV2_AUDIOSYSTEM_4)
	{
		return pBoard->_pAudio4Control;
	}
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL5) && audioSystem == NTV2_AUDIOSYSTEM_5)
	{
		return pBoard->_pAudio5Control;
	}
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL6) && audioSystem == NTV2_AUDIOSYSTEM_6)
	{
		return pBoard->_pAudio6Control;
	}
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL7) && audioSystem == NTV2_AUDIOSYSTEM_7)
	{
		return pBoard->_pAudio7Control;
	}
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL8) && audioSystem == NTV2_AUDIOSYSTEM_8)
	{
		return pBoard->_pAudio8Control;
	}
	else
	{
		return pBoard->_pAudioControl;
	}
}

static ULWord
GetAudioControlRegister(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	ULWord _DeviceID = getNTV2Params(deviceNumber)->_DeviceID;
	if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL2) && audioSystem == NTV2_AUDIOSYSTEM_2)
		return kRegAud2Control;
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL3) && audioSystem == NTV2_AUDIOSYSTEM_3)
		return kRegAud3Control;
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL4) && audioSystem == NTV2_AUDIOSYSTEM_4)
		return kRegAud4Control;
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL5) && audioSystem == NTV2_AUDIOSYSTEM_5)
		return kRegAud5Control;
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL6) && audioSystem == NTV2_AUDIOSYSTEM_6)
		return kRegAud6Control;
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL7) && audioSystem == NTV2_AUDIOSYSTEM_7)
		return kRegAud7Control;
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL8) && audioSystem == NTV2_AUDIOSYSTEM_8)
		return kRegAud8Control;
	else
		return kRegAud1Control;
}

static void
StartAudioCapture(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2PrivateParams* pNTV2Params;
	unsigned long address = 0;
	spinlock_t* pSpinLock;
	ULWord value = 0;
	unsigned long flags = 0;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Starting audio capture on board %d, audioSystem %d\n", deviceNumber, audioSystem);
	}

    pSpinLock = &(pNTV2Params->_registerSpinLock);
	address = GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, &pSpinLock);

	ntv2_spin_lock_irqsave(pSpinLock, flags);
	value = READ_REGISTER_ULWord(address);
	value |= BIT_8;
	value &= ~BIT_0;
	WRITE_REGISTER_ULWord(address, value);

	value = READ_REGISTER_ULWord(address);
	value |= BIT_0;
	value &= ~BIT_8;
	WRITE_REGISTER_ULWord(address, value);
	ntv2_spin_unlock_irqrestore(pSpinLock, flags);
}

static void
StopAudioCapture(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2PrivateParams* pNTV2Params;
	unsigned long address = 0;
	spinlock_t* pSpinLock;
	ULWord value = 0;
	unsigned long flags = 0;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Stopping audio capture on board %d, audioSystem %d\n", deviceNumber, audioSystem);
	}

    pSpinLock = &(pNTV2Params->_registerSpinLock);
	address = GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, &pSpinLock);

	ntv2_spin_lock_irqsave(pSpinLock, flags);
	value = READ_REGISTER_ULWord(address);
	value |= BIT_8;
	value &= ~BIT_0;
	WRITE_REGISTER_ULWord(address, value);
	ntv2_spin_unlock_irqrestore(pSpinLock, flags);
}

static void
StopAudioPlayback(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2PrivateParams* pNTV2Params;
	unsigned long address = 0;
	spinlock_t* pSpinLock;
	ULWord value = 0;
	unsigned long flags = 0;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Stopping audio playback on board %d, audioSystem %d\n", deviceNumber, audioSystem);
	}

    pSpinLock = &(pNTV2Params->_registerSpinLock);
	address = GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, &pSpinLock);

    // Reset Audio Playback... basically stops it.
	ntv2_spin_lock_irqsave(pSpinLock, flags);
	value = READ_REGISTER_ULWord(address);
	value |= BIT_9; //Set the Audio Output reset bit!
	WRITE_REGISTER_ULWord(address, value);
	ntv2_spin_unlock_irqrestore(pSpinLock, flags);
	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Stopped audio playback on board %d, audioSystem %d\n", deviceNumber, audioSystem);
	}
}

static void
StartAudioPlayback(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2PrivateParams* pNTV2Params;
	unsigned long address = 0;
	spinlock_t* pSpinLock;
	ULWord value = 0;
	unsigned long flags = 0;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Starting audio playback on board %d, audioSystem %d... \n", deviceNumber, audioSystem);
	}

    // Take Audio Playback  out of Reset
    //This sequence is to make sure the audio is correctly initialized!
    pSpinLock = &(pNTV2Params->_registerSpinLock);
	address = GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, &pSpinLock);

	ntv2_spin_lock_irqsave(pSpinLock, flags);
	value = READ_REGISTER_ULWord(address);

	value &= (~BIT_9); //Clear the Audio Output reset bit!
	WRITE_REGISTER_ULWord(address, value);
    udelay(30); //30us

	ntv2_spin_unlock_irqrestore(pSpinLock, flags);

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Audio %s playing back on board %d, audioSystem %d \n",
			IsAudioPlaying(deviceNumber, audioSystem) ? "is" : "is not",
			deviceNumber, audioSystem);
	}
}

void
SetAudioPlaybackMode(ULWord deviceNumber, NTV2_GlobalAudioPlaybackMode mode)
{
	NTV2PrivateParams* pNTV2Params;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Setting Audio Playback Mode for %d to %d... \n", deviceNumber, mode);
	}

	pNTV2Params->_globalAudioPlaybackMode = mode;
	switch (pNTV2Params->_globalAudioPlaybackMode)
	{
	case NTV2_AUDIOPLAYBACK_NOW:
		// Hardwire channel to match windriver/vidfilter.cpp
		StartAudioPlayback(deviceNumber, NTV2_AUDIOSYSTEM_1);
		break;
	case NTV2_AUDIOPLAYBACK_NEXTFRAME:
		pNTV2Params->_startAudioNextFrame = true;
		break;
	case NTV2_AUDIOPLAYBACK_NORMALAUTOCIRCULATE:
		break;
	case NTV2_AUDIOPLAYBACK_1STAUTOCIRCULATEFRAME:
		break;
	}
}

NTV2_GlobalAudioPlaybackMode
GetAudioPlaybackMode(ULWord deviceNumber)
{
	NTV2PrivateParams* pNTV2Params;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENXIO;

	return pNTV2Params->_globalAudioPlaybackMode;
}

ULWord
GetFPGARevision(ULWord deviceNumber)
{
    ULWord fpgaRevision=0;
    ULWord fpgaRevisionMask = BIT(4)+BIT(5)+BIT(6)+BIT(7)+BIT(8)+BIT(9)+BIT(10)+BIT(11);
    ULWord fpgaRevisionShift = 4;
    fpgaRevision = ReadRegister(deviceNumber, kRegStatus, fpgaRevisionMask, fpgaRevisionShift);
    return fpgaRevision;
}

ULWord
GetNumFrameBuffers(ULWord deviceNumber, ULWord boardID)
{
	NTV2FrameGeometry frameGeometry;
	NTV2FrameBufferFormat frameBufferFormat1;
	NTV2FrameBufferFormat frameBufferFormat2;

	ULWord numFrameBuffers;
	ULWord numFrame1;
	ULWord numFrame2;

	ULWord channel1Compressed;
	ULWord channel2Compressed;
	
	Ntv2SystemContext systemContext;
	systemContext.devNum = deviceNumber;
	
	frameGeometry = GetFrameGeometry(&systemContext, NTV2_CHANNEL1);
	frameBufferFormat1 = GetFrameBufferFormat(&systemContext, NTV2_CHANNEL1);
	frameBufferFormat2 = GetFrameBufferFormat(&systemContext, NTV2_CHANNEL2);

	channel1Compressed = ReadRegister(deviceNumber, kRegCh1Control, kRegMaskChannelCompressed, kRegShiftChannelCompressed);
	channel2Compressed = ReadRegister(deviceNumber, kRegCh2Control, kRegMaskChannelCompressed, kRegShiftChannelCompressed);

	if (NTV2DeviceGetNumVideoChannels(boardID) > 2)
	{
		ULWord numFrame3;
		ULWord numFrame4;
		ULWord tempNum1;
		ULWord tempNum2;

		NTV2FrameBufferFormat frameBufferFormat3 = GetFrameBufferFormat(&systemContext, NTV2_CHANNEL3);
		NTV2FrameBufferFormat frameBufferFormat4 = GetFrameBufferFormat(&systemContext, NTV2_CHANNEL4);
		numFrame1 = NTV2DeviceGetNumberFrameBuffers(boardID, frameGeometry, frameBufferFormat1);
		numFrame2 = NTV2DeviceGetNumberFrameBuffers(boardID, frameGeometry, frameBufferFormat2);
		numFrame3 = NTV2DeviceGetNumberFrameBuffers(boardID, frameGeometry, frameBufferFormat3);
		numFrame4 = NTV2DeviceGetNumberFrameBuffers(boardID, frameGeometry, frameBufferFormat4);
		tempNum1 = numFrame1;
		tempNum2 = numFrame3;
		if (numFrame2 < numFrame1)
			tempNum1 = numFrame2;
		if (numFrame4 < numFrame3)
			tempNum2 = numFrame4;
		if (tempNum1 < tempNum2)
			numFrameBuffers = tempNum1;
		else
			numFrameBuffers = tempNum2;

	}
	else
	{
		numFrame1 = NTV2DeviceGetNumberFrameBuffers(boardID, frameGeometry, frameBufferFormat1);
		numFrame2 = NTV2DeviceGetNumberFrameBuffers(boardID, frameGeometry, frameBufferFormat2);
		numFrameBuffers = numFrame1;
		if (numFrame2 < numFrame1)
		{
			numFrameBuffers = numFrame2;
		}
	}

	return numFrameBuffers;
}

ULWord
GetAudioFrameBufferNumber(ULWord deviceNumber, ULWord boardID, NTV2AudioSystem audioSystem)
{
	// This routine is not to be called with boards that have stacked audio,
	// and the channel 1 geometry applies to all channels on non-stacked boards
	ULWord channel1Compressed;
	ULWord channel2Compressed;

	if (NTV2DeviceCanDoStackedAudio(getNTV2Params(deviceNumber)->_DeviceID))
	{
		// Kludge: For stacked boards, return a flag value the DMA code can use to tell a
		// video DMA from an audio DMA.  Not to be used in any calculations!
		return 0x80000000;
	}

	channel1Compressed = ReadRegister(deviceNumber, kRegCh1Control, kRegMaskChannelCompressed, kRegShiftChannelCompressed);
	channel2Compressed = ReadRegister(deviceNumber, kRegCh2Control, kRegMaskChannelCompressed, kRegShiftChannelCompressed);

	if (NTV2DeviceCanDoAudioN(boardID, NTV2_CHANNEL2) && audioSystem == NTV2_AUDIOSYSTEM_2)
	{
		return (GetNumFrameBuffers(deviceNumber, boardID)-2);
	}
	else if (NTV2DeviceCanDoAudioN(boardID, NTV2_CHANNEL3) && audioSystem == NTV2_AUDIOSYSTEM_3)
	{
		return (GetNumFrameBuffers(deviceNumber, boardID)-3);
	}
	else if (NTV2DeviceCanDoAudioN(boardID, NTV2_CHANNEL4) && audioSystem == NTV2_AUDIOSYSTEM_4)
	{
		return (GetNumFrameBuffers(deviceNumber, boardID)-4);
	}
	else
	{
		return (GetNumFrameBuffers(deviceNumber, boardID)-1);
	}
}

ULWord
GetAudioLastIn(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2PrivateParams* pBoard = getNTV2Params(deviceNumber);
	ULWord _DeviceID = pBoard->_DeviceID;
	if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL2) && audioSystem == NTV2_AUDIOSYSTEM_2)
		return ReadAudioLastIn2(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL3) && audioSystem == NTV2_AUDIOSYSTEM_3)
		return ReadAudioLastIn3(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL4) && audioSystem == NTV2_AUDIOSYSTEM_4)
		return ReadAudioLastIn4(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL5) && audioSystem == NTV2_AUDIOSYSTEM_5)
		return ReadAudioLastIn5(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL6) && audioSystem == NTV2_AUDIOSYSTEM_6)
		return ReadAudioLastIn6(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL7) && audioSystem == NTV2_AUDIOSYSTEM_7)
		return ReadAudioLastIn7(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL8) && audioSystem == NTV2_AUDIOSYSTEM_8)
		return ReadAudioLastIn8(deviceNumber);
	else
		return ReadAudioLastIn(deviceNumber);
}

ULWord
GetAudioLastOut(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2PrivateParams* pBoard = getNTV2Params(deviceNumber);
	ULWord _DeviceID = pBoard->_DeviceID;
	if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL2) && audioSystem == NTV2_AUDIOSYSTEM_2)
	{
		return ReadAudioLastOut2(deviceNumber);
	}
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL3) && audioSystem == NTV2_AUDIOSYSTEM_3)
		return ReadAudioLastOut3(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL4) && audioSystem == NTV2_AUDIOSYSTEM_4)
		return ReadAudioLastOut4(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL5) && audioSystem == NTV2_AUDIOSYSTEM_5)
		return ReadAudioLastOut5(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL6) && audioSystem == NTV2_AUDIOSYSTEM_6)
		return ReadAudioLastOut6(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL7) && audioSystem == NTV2_AUDIOSYSTEM_7)
		return ReadAudioLastOut7(deviceNumber);
	else if (NTV2DeviceCanDoAudioN(_DeviceID, NTV2_CHANNEL8) && audioSystem == NTV2_AUDIOSYSTEM_8)
		return ReadAudioLastOut8(deviceNumber);
	else
		return ReadAudioLastOut(deviceNumber);
}

static void
PauseAudioPlayback(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
    // Set pause audio playback bit
	NTV2PrivateParams* pNTV2Params;
	unsigned long address;
	spinlock_t* pSpinLock;
	ULWord value;
	unsigned long flags;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Pausing audio playback on board %d, audioSystem %d... \n", deviceNumber, audioSystem);
	}

    pSpinLock = &(pNTV2Params->_registerSpinLock);
	address = GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, & pSpinLock);

	ntv2_spin_lock_irqsave(pSpinLock, flags);
	value = READ_REGISTER_ULWord(address);

	value |= BIT_11; 	// Set the pause bit
	WRITE_REGISTER_ULWord(address, value);
	ntv2_spin_unlock_irqrestore(pSpinLock, flags);
}

// Clear pause audio playback bit
static void
UnPauseAudioPlayback(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2PrivateParams* pNTV2Params;
	unsigned long address;
	spinlock_t* pSpinLock;
	ULWord value;
	unsigned long flags;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
		return;

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("Unpausing audio playback on board %d... \n", deviceNumber);
	}
    // Take Audio Playback out of pause
    pSpinLock = &(pNTV2Params->_registerSpinLock);
	address = GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, &pSpinLock);

	ntv2_spin_lock_irqsave(pSpinLock, flags);
	value = READ_REGISTER_ULWord(address);

	value &= ~ (BIT_11);  // Clear the pause bit
	WRITE_REGISTER_ULWord(address, value);
	ntv2_spin_unlock_irqrestore(pSpinLock, flags);
}

static bool
IsAudioPlaybackPaused(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	bool paused = 0;

	if ((READ_REGISTER_ULWord(GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, NULL)) & BIT_11))
	{
		paused = 1;
	}

	if (MsgsEnabled(NTV2_DRIVER_AUDIO_DEBUG_MESSAGES))
	{
    	MSG("IsAudioPlaybackPaused says audio pause is %s on board %d, audioSystem %d... \n",
			paused ? "true" : "false",
			deviceNumber, audioSystem);
	}

	return paused;
}

static bool
IsAudioPlaybackStopped(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	// Audio is (supposed) to be playing if BIT_9 is cleared (not in reset)
	if ((READ_REGISTER_ULWord(GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, NULL)) & BIT_9) != 0)
		return 1;

	return 0;
}

static ULWord
IsAudioPlaying(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	// Audio is (supposed) to be playing if BIT_9 is cleared (not in reset)
	if ((READ_REGISTER_ULWord(GetAudioControlRegisterAddressAndLock(deviceNumber, audioSystem, NULL)) & BIT_9) == 0)
		return 1;

	return 0;
}

int
oemAutoCirculateDmaAudioSetup(ULWord deviceNumber, INTERNAL_AUTOCIRCULATE_STRUCT* pAuto)
{
	NTV2PrivateParams * pNTV2Params = getNTV2Params(deviceNumber);
	ULWord ulFrameNumber = pAuto->transferFrame;
	ULWord ulAudioWrapAddress = GetAudioWrapAddress(deviceNumber, pAuto->audioSystem);
	ULWord ulAudioReadOffset = GetAudioReadOffset(deviceNumber, pAuto->audioSystem);
	ULWord ulPreWrapSize = 0;
	ULWord ulPostWrapSize = 0;
	unsigned long flags = 0;

	if (!pAuto->circulateWithAudio)
	{
		return -EPERM;
	}

	ntv2_spin_lock_irqsave(&pNTV2Params->_autoCirculateLock, flags);

	if (pAuto->recording)
	{
		ULWord ulAudioEnd = pAuto->frameStamp[ulFrameNumber].audioInStopAddress;
		ULWord ulAudioStart = pAuto->frameStamp[ulFrameNumber].audioInStartAddress;
		
		pAuto->audioTransferOffset = ulAudioStart - ulAudioReadOffset;
		if (ulAudioEnd < ulAudioStart)
		{
			ulPreWrapSize = ulAudioWrapAddress - (ulAudioStart - ulAudioReadOffset);
			ulPostWrapSize = ulAudioEnd - ulAudioReadOffset;
		}
		else
		{
			ulPreWrapSize = ulAudioEnd - ulAudioStart;
			ulPostWrapSize = 0;
		}

		// save this for oemCompleteAutoCirculateTransfer
		pAuto->audioTransferSize = ulPreWrapSize + ulPostWrapSize;
        pAuto->audioTransferSize *= pAuto->audioSystemCount;
		pAuto->audioStartSample = 0;
	}
	else
	{
		ULWord ulAudioBytes = pAuto->audioTransferSize;
		if (pAuto->audioSystemCount > 1)
		{
			ulAudioBytes = ulAudioBytes/pAuto->audioSystemCount;
		}

		if(pAuto->audioDropsRequired > pAuto->audioDropsCompleted)
		{
			pAuto->nextAudioOutputAddress = 0;
			pAuto->audioDropsCompleted++;
		}

		// Audio start default
		pAuto->audioTransferOffset = pAuto->nextAudioOutputAddress;
		// Remember actual start
		pAuto->frameStamp[ulFrameNumber].audioExpectedAddress = pAuto->nextAudioOutputAddress;

		// NOTE: '<' should be '<=', but when it is '==', we want to set _ulNextAudioOutputAddr to 0 
		if (pAuto->nextAudioOutputAddress + ulAudioBytes < ulAudioWrapAddress)
		{
			// No audio Wrap required so write out all the audio data at once.
			pAuto->nextAudioOutputAddress += ulAudioBytes;
			ulPreWrapSize = ulAudioBytes;
			ulPostWrapSize = 0;
		}
		else 
		{
			// a Wrap will be required to reach the target address.
			ulPreWrapSize = ulAudioWrapAddress - pAuto->nextAudioOutputAddress;
			ulPostWrapSize = ulAudioBytes - ulPreWrapSize;       
			pAuto->nextAudioOutputAddress = ulPostWrapSize;
		}
	}

	ntv2_spin_unlock_irqrestore(&pNTV2Params->_autoCirculateLock, flags);

	return 0;
}

// OemAutoCirculateSetupColorCorrector
// Setup ColorCorrection Hardware for next frame
static void
OemAutoCirculateSetupColorCorrector(ULWord deviceNumber,
									NTV2Crosspoint channelSpec,
									INTERNAL_ColorCorrectionInfo *ccInfo)
{
	if (!NTV2DeviceCanDoColorCorrection(getNTV2Params(deviceNumber)->_DeviceID))
		return;

	// Find current output bank and make host access bank the other bank.
	switch (channelSpec)
	{
	case NTV2CROSSPOINT_CHANNEL1:
		if (GetColorCorrectionOutputBank(deviceNumber, NTV2_CHANNEL1) == 1)
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH1BANK0);  // happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL1, 0)	;				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH1BANK1);  // happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL1, 1)	;				// happens next frame
		}
		SetColorCorrectionSaturation (deviceNumber, NTV2_CHANNEL1, ccInfo->saturationValue);
		SetColorCorrectionMode(deviceNumber, NTV2_CHANNEL1, ccInfo->mode);
		break;
	case NTV2CROSSPOINT_CHANNEL2:
		if (GetColorCorrectionOutputBank(deviceNumber, NTV2_CHANNEL2) == 1)
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH2BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL2, 0)	;				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH2BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL2, 1);				// happens next frame
		}
		SetColorCorrectionSaturation (deviceNumber, NTV2_CHANNEL2, ccInfo->saturationValue);
		SetColorCorrectionMode(deviceNumber, NTV2_CHANNEL2, ccInfo->mode);
		break;
	case NTV2CROSSPOINT_CHANNEL3:
		if (GetColorCorrectionOutputBank(deviceNumber, NTV2_CHANNEL3) == 1)
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH3BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL3, 0)	;				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH3BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL3, 1);				// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL4:
		if (GetColorCorrectionOutputBank(deviceNumber, NTV2_CHANNEL4) == 1)
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH4BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL4, 0)	;				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH4BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL4, 1);				// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL5:
		if (GetColorCorrectionOutputBank(deviceNumber, NTV2_CHANNEL5) == 1)
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH5BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL5, 0)	;				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH5BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL5, 1);				// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL6:
		if (GetColorCorrectionOutputBank(deviceNumber, NTV2_CHANNEL6) == 1)
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH6BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL6, 0)	;				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH6BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL6, 1);				// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL7:
		if (GetColorCorrectionOutputBank(deviceNumber, NTV2_CHANNEL7) == 1)
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH7BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL7, 0)	;				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH7BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL7, 1);				// happens next frame
		}
		break;
	case NTV2CROSSPOINT_CHANNEL8:
		if (GetColorCorrectionOutputBank(deviceNumber, NTV2_CHANNEL8) == 1)
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH8BANK0);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL8, 0)	;				// happens next frame
		}
		else
		{
			SetColorCorrectionHostAccessBank (deviceNumber, NTV2_CCHOSTACCESS_CH8BANK1);	// happens immediatedly
			SetColorCorrectionOutputBank (deviceNumber, NTV2_CHANNEL8, 1);				// happens next frame
		}
		break;
	default:
		MSG("Setup color corrector bad channel %d\n", channelSpec);
		return;
	}


	// Now fill color correction buffer
	// Windows uses a kernel function:
	// WRITE_REGISTER_BUFFER_ULONG(_pGlobalControl+512,(PULONG)ccInfo->ccLookupTables,NTV2_COLORCORRECTOR_TABLESIZE/4);
	WriteRegisterBufferULWord(deviceNumber,
							  512,	// Offset of Red/Cr table  TODO: Magic number
							  ccInfo->ccLookupTables,
							  NTV2_COLORCORRECTOR_TABLESIZE/4);
}

// OemAutoCirculateTransferColorCorrectorInfo
// Copy over information
static int
OemAutoCirculateTransferColorCorrectorInfo(ULWord deviceNumber,
										   INTERNAL_ColorCorrectionInfo *ccInternalInfo,
										   NTV2ColorCorrectionInfo *ccTransferInfo)
{
	ULWord rc = 0;
	if (!NTV2DeviceCanDoColorCorrection(getNTV2Params(deviceNumber)->_DeviceID))
		return -EOPNOTSUPP;

	// Copy over info easy stuff
	ccInternalInfo->mode = ccTransferInfo->mode;
	ccInternalInfo->saturationValue = ccTransferInfo->saturationValue;

	// now for the hard part.
	rc = copy_from_user((void*)ccInternalInfo->ccLookupTables,
						(const void*) ccTransferInfo->ccLookupTables,
						NTV2_COLORCORRECTOR_TABLESIZE);
	if (rc) {
		MSG("OemAutoCirculateTransferColorCorrectorInfo(): copy_from_user returned %d\n", rc);
		return -EFAULT;
	} else {
		return 0;
	}
}

// this assumes we have one 1024-entry LUT that we're going to download to all three channels
ULWord
DownloadLinearLUTToHW (ULWord deviceNumber, NTV2Channel channel, int bank)
{
	ULWord bResult;
	ULWord lutValue;
	ULWord i;
	unsigned long address;
	NTV2ColorCorrectionHostAccessBank savedBank;

	bResult = 1;
	if (NTV2DeviceCanDoColorCorrection(getNTV2Params(deviceNumber)->_DeviceID))
	{
		savedBank = GetColorCorrectionHostAccessBank(deviceNumber, channel);

		SetLUTEnable(deviceNumber, channel, true);
		// setup Host Access
		switch (channel)
		{
		case NTV2_CHANNEL1:
			SetColorCorrectionHostAccessBank(
				deviceNumber, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH1BANK0 + bank));
			break;
		case NTV2_CHANNEL2:
			SetColorCorrectionHostAccessBank(
				deviceNumber, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH2BANK0 + bank));
			break;
		case NTV2_CHANNEL3:
			SetColorCorrectionHostAccessBank(
				deviceNumber, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH3BANK0 + bank));
			break;
		case NTV2_CHANNEL4:
			SetColorCorrectionHostAccessBank(
				deviceNumber, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH4BANK0 + bank));
			break;
		case NTV2_CHANNEL5:
			SetColorCorrectionHostAccessBank(
				deviceNumber, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH5BANK0 + bank));
			break;
		case NTV2_CHANNEL6:
			SetColorCorrectionHostAccessBank(
				deviceNumber, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH6BANK0 + bank));
			break;
		case NTV2_CHANNEL7:
			SetColorCorrectionHostAccessBank(
				deviceNumber, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH7BANK0 + bank));
			break;
		case NTV2_CHANNEL8:
			SetColorCorrectionHostAccessBank(
				deviceNumber, (NTV2ColorCorrectionHostAccessBank)((int)NTV2_CCHOSTACCESS_CH8BANK0 + bank));
			break;
		default:
			MSG("Download linear LUT bad channel %d\n", channel);
			return 0;
		}

		for (i = 0; i < 1024;  i += 2)
		{
			// Tables are already converted to ints and endian swapped for the Mac
			lutValue = ((i+1)<<22) + (i<<6);

			address = (unsigned long)(getNTV2Params(deviceNumber)->_pGlobalControl + (kColorCorrectionLUTOffset_Red)   + (i*2));
			WRITE_REGISTER_ULWord(address, lutValue);

			address = (unsigned long)(getNTV2Params(deviceNumber)->_pGlobalControl + (kColorCorrectionLUTOffset_Green) + (i*2));
			WRITE_REGISTER_ULWord(address, lutValue);

			address = (unsigned long)(getNTV2Params(deviceNumber)->_pGlobalControl + (kColorCorrectionLUTOffset_Blue)  + (i*2));
			WRITE_REGISTER_ULWord(address, lutValue);
		}
		SetLUTEnable(deviceNumber, channel, false);

		SetColorCorrectionHostAccessBank(deviceNumber, savedBank);
	}

	return bResult;
}

static void
oemAutoCirculateTaskInit(PAUTOCIRCULATE_TASK_STRUCT pTaskInfo, AutoCircGenericTask* pTaskArray, ULWord maxTasks)
{
	pTaskInfo->taskVersion = AUTOCIRCULATE_TASK_VERSION;
	pTaskInfo->numTasks = 0;
	pTaskInfo->taskSize = sizeof(AutoCircGenericTask);
	pTaskInfo->taskArray = pTaskArray;
	if (pTaskArray != NULL)
	{
		pTaskInfo->maxTasks = maxTasks;
	}
	else
	{
		pTaskInfo->maxTasks = 0;
	}
}

static int
oemAutoCirculateTaskTransfer(PAUTOCIRCULATE_TASK_STRUCT pDriverInfo,
							 PAUTOCIRCULATE_TASK_STRUCT pUserInfo,
							 bool bToDriver)
{
	int iSize = 0;
	ULWord rc = 0;

	if (bToDriver)
	{
		if ((pDriverInfo->taskVersion != pUserInfo->taskVersion) ||
			(pDriverInfo->taskSize != pUserInfo->taskSize) ||
			(pDriverInfo->maxTasks < pUserInfo->numTasks))
		{
			return 0;
		}

		iSize = pUserInfo->numTasks * pUserInfo->taskSize;

		if (iSize > 0)
		{
			rc = copy_from_user((void*)pDriverInfo->taskArray, (const void*)pUserInfo->taskArray, iSize);
		}

		if (rc)
		{
			MSG("OemAutoCirculateTaskTransfer(): copy_from_user returned %d\n", rc);
			return -EFAULT;
		}

		pDriverInfo->numTasks = pUserInfo->numTasks;
	}
	else
	{
		if ((pDriverInfo->taskVersion != pUserInfo->taskVersion) ||
			(pDriverInfo->taskSize != pUserInfo->taskSize) ||
			(pDriverInfo->numTasks > pUserInfo->maxTasks))
		{
			return 0;
		}

		iSize = pDriverInfo->numTasks * pDriverInfo->taskSize;

		if (iSize > 0)
		{
			rc = copy_to_user((void*)pUserInfo->taskArray, (const void*)pDriverInfo->taskArray, iSize);
		}

		if (rc)
		{
			MSG("OemAutoCirculateTaskTransfer(): copy_to_user returned %d\n", rc);
			return -EFAULT;
		}

		pUserInfo->numTasks = pDriverInfo->numTasks;
	}

	return 0;
}

#include "ntv2videodefines.h"
#include "ntv2fixed.h"
// Setup VidProc Hardare for next Frame
// Unfortunately only updates hardware every frame even in interlaced modes.
static void
OemAutoCirculateSetupVidProc(ULWord deviceNumber,
							 NTV2Crosspoint channelSpec,
							 AutoCircVidProcInfo* vidProcInfo)
{
	ULWord max = 0, offset=0;
	NTV2FrameDimensions frameBufferSize;
	ULWord splitModeValue = 0;

	ULWord positionValue;
	ULWord softnessPixels;
	ULWord softnessSlope;
	ULWord regValue;

	if (!NTV2DeviceCanDoVideoProcessing(getNTV2Params(deviceNumber)->_DeviceID))
		return;

	SetForegroundVideoCrosspoint(deviceNumber, vidProcInfo->foregroundVideoCrosspoint);
	SetForegroundKeyCrosspoint(deviceNumber, vidProcInfo->foregroundKeyCrosspoint);
	SetBackgroundVideoCrosspoint(deviceNumber, vidProcInfo->backgroundVideoCrosspoint);
	SetBackgroundKeyCrosspoint(deviceNumber, vidProcInfo->backgroundKeyCrosspoint);

	regValue = ReadRegister(deviceNumber, kRegVidProc1Control, NO_MASK, NO_SHIFT);
	regValue &= ~(VIDPROCMUX1MASK + VIDPROCMUX2MASK + VIDPROCMUX3MASK);

	GetActiveFrameBufferSize(deviceNumber, &frameBufferSize);
	switch (vidProcInfo->mode)
	{
	case AUTOCIRCVIDPROCMODE_MIX:
		regValue |= (BIT_0+BIT_2);
		WriteRegister(deviceNumber,
					  kRegMixer1Coefficient,
					  vidProcInfo->transitionCoefficient,
					  NO_MASK, NO_SHIFT);
		break;
	case AUTOCIRCVIDPROCMODE_HORZWIPE:
		regValue |= (BIT_0+BIT_3);
        switch (frameBufferSize.mWidth)
        {
		case HD_NUMCOMPONENTPIXELS_1080:
		case HD_NUMCOMPONENTPIXELS_720:
			max = frameBufferSize.mWidth;
			offset = 8;
			break;
		case NUMCOMPONENTPIXELS:
			max = frameBufferSize.mWidth*2;
			offset = 8;
			break;
		default:
			// not supported at this time;
			return;

		}
 		break;
	case AUTOCIRCVIDPROCMODE_VERTWIPE:
		regValue |= (BIT_0+BIT_3);
		splitModeValue = BIT_30;
        switch (frameBufferSize.mHeight)
        {
		case HD_NUMACTIVELINES_1080:
			offset = 19;
			max = frameBufferSize.mHeight+1;
			break;
		case HD_NUMACTIVELINES_720:
			offset = 7;
			max = frameBufferSize.mHeight+1;
			break;
		case NUMACTIVELINES_525:
			max = (frameBufferSize.mHeight/2)+1;
			offset = 8;
			break;
		case NUMACTIVELINES_625:
			max = (frameBufferSize.mHeight/2)+1;
			offset = 8;
			break;
		default:
			// not supported at this time;
			return;
        }
		break;
	case AUTOCIRCVIDPROCMODE_KEY:
		regValue |= (BIT_0);
		break;
	default:
		regValue = 0;
		break;
	}

	WriteRegister(deviceNumber, kRegVidProc1Control, regValue, NO_MASK, NO_SHIFT);

	if (vidProcInfo->mode == AUTOCIRCVIDPROCMODE_MIX ||
		vidProcInfo->mode == AUTOCIRCVIDPROCMODE_KEY)
		return; ///we're done

	positionValue = (Word)FixedMix(0,(Word)max,vidProcInfo->transitionCoefficient);
	softnessPixels = 0x1FFF;
	softnessSlope = 0x1FFF;
	if (vidProcInfo->transitionSoftness == 0)
	{
		softnessSlope = 0x1FFF;
		softnessPixels = 1;
	}
	else
	{
		// need to tame softness to based on position.
		// 1st find out what the maximum softness is
		ULWord maxSoftness = 0;
		if (positionValue > max/2)
			maxSoftness = max - positionValue;
		else
			maxSoftness = positionValue;

		// softness is limited to 1/4 of max
		if (maxSoftness > max/4)
		{
			maxSoftness = max/4;
		}

		if (maxSoftness == 0)
		{
			softnessPixels = 1;
		}
		else
		{
			softnessPixels = (Word)FixedMix(1,(Word)maxSoftness,vidProcInfo->transitionSoftness);

		}
	}
	
	if (softnessPixels == 0)
		softnessPixels = 1; // shouldn't happen but....
	softnessSlope = 0x1FFF/softnessPixels;
	positionValue -= (softnessPixels/2);

	WriteRegister(deviceNumber,
				  kRegSplitControl,
				  splitModeValue | (softnessSlope<<16) | ((positionValue+offset)<<2),
				  NO_MASK, NO_SHIFT);
}


void
OemAutoCirculateSetupNTV2Routing(ULWord deviceNumber, NTV2RoutingTable* pNTV2Routing)
{
	ULWord i;
	ULWord numEntries = pNTV2Routing->numEntries;
	if (numEntries == 0 || numEntries >= MAX_ROUTING_ENTRIES)
	{
		return;
	}

	for (i = 0; i < numEntries; i++)
	{
		NTV2RoutingEntry* entry = &pNTV2Routing->routingEntry[i];
		if (IsSaveRecallRegister(deviceNumber, entry->registerNum))
		{
			WriteRegister(deviceNumber, entry->registerNum,entry->value,entry->mask,entry->shift);
		}
	}
}

void
oemAutoCirculateWriteHDMIAux(ULWord deviceNumber, ULWord* pAuxData, ULWord auxDataSize)
{
	NTV2PrivateParams *pNTV2Params = getNTV2Params(deviceNumber);
	ULWord* pAux = pAuxData;
	ULWord numData = NTV2_HDMIAuxDataSize;
	ULWord numAux = auxDataSize/numData;
	ULWord auxReg;
	ULWord iAux;
	ULWord iData;
	NTV2Channel channel = NTV2_CHANNEL1; 

	if (channel >= NTV2DeviceGetNumHDMIVideoOutputs(pNTV2Params->_DeviceID))
		return;
	if (numAux == 0)
		return;
	
	for (iAux = 0; iAux < numAux; iAux++)
	{
		if (NTV2DeviceGetHDMIVersion(pNTV2Params->_DeviceID) == 2)
		{
			if (iAux == 0)
			{
				auxReg = kRegHDMIOutputAuxData;
				for (iData = 0; iData < numData/4; iData++, auxReg++)
					WriteRegister(deviceNumber, auxReg, pAux[iData], NO_MASK, NO_SHIFT);
			}
		}
		if (NTV2DeviceGetHDMIVersion(pNTV2Params->_DeviceID) == 4)
		{
			if (pNTV2Params->m_pHDMIOut4Monitor[channel] != NULL)
			{
				ntv2_hdmiout4_write_info_frame(pNTV2Params->m_pHDMIOut4Monitor[channel],
											   numData, ((uint8_t*)pAux) + (iAux * numData));
			}
		}
		pAux += numData/4;
	}
}

NTV2VideoFormat
GetNTV2VideoFormat(UByte status, UByte frameRateHiBit)
{
	UByte linesPerFrame = (status>>4)&0x7;
	UByte frameRate = (status&0x7) | (frameRateHiBit<<3);
	NTV2VideoFormat videoFormat = NTV2_FORMAT_UNKNOWN;
	bool progressive = (status>>7)&0x1;

	switch (linesPerFrame)
	{
	case 1:
		// 525
		if (frameRate == 4)
			videoFormat = NTV2_FORMAT_525_5994;

		break;
	case 2:
		// 625
		if (frameRate == 5)
			videoFormat = NTV2_FORMAT_625_5000;
		break;
	case 3:
	{
		// 720p
		if (frameRate == 1)
			videoFormat = NTV2_FORMAT_720p_6000;
		else if (frameRate == 2)
			videoFormat = NTV2_FORMAT_720p_5994;
		else if (frameRate == 8)
			videoFormat = NTV2_FORMAT_720p_5000;
		break;
	}
	case 4:
	{
		// 1080
		if (progressive)
		{
			switch (frameRate)
			{
			case 3:
				videoFormat = NTV2_FORMAT_1080p_3000;
				break;
			case 4:
				videoFormat = NTV2_FORMAT_1080p_2997;
				break;
			case 5:
				videoFormat = NTV2_FORMAT_1080p_2500;
				break;
			case 6:
				videoFormat = NTV2_FORMAT_1080p_2400;
				break;
			case 7:
				videoFormat = NTV2_FORMAT_1080p_2398;
				break;
			}

		}
		else
		{

			switch (frameRate)
			{
			case 3:
				videoFormat = NTV2_FORMAT_1080i_6000;
				break;
			case 4:
				videoFormat = NTV2_FORMAT_1080i_5994;
				break;
			case 5:
				videoFormat = NTV2_FORMAT_1080i_5000;
				break;
			case 6:
				videoFormat = NTV2_FORMAT_1080psf_2400;
				break;
			case 7:
				videoFormat = NTV2_FORMAT_1080psf_2398;
				break;
			}
		}
		break;
	}
	}

	return videoFormat;
}

static LWord
GetFramePeriod(ULWord deviceNumber, NTV2Channel channel)
{
	Ntv2SystemContext systemContext;
	NTV2FrameRate frameRate;
	LWord period;
	systemContext.devNum = deviceNumber;
	frameRate = GetFrameRate(&systemContext, channel);

	switch (frameRate)
	{
	case NTV2_FRAMERATE_12000:
		period = 10000000/120;
		break;
	case NTV2_FRAMERATE_11988:
		period = 10010000/120;
		break;
	case NTV2_FRAMERATE_6000:
		period = 10000000/60;
		break;
	case NTV2_FRAMERATE_5994:
		period = 10010000/60;
		break;
	case NTV2_FRAMERATE_4800:
		period = 10000000/48;
		break;
	case NTV2_FRAMERATE_4795:
		period = 10010000/48;
		break;
	case NTV2_FRAMERATE_3000:
		period = 10000000/30;
		break;
	case NTV2_FRAMERATE_2997:
		period = 10010000/30;
		break;
	case NTV2_FRAMERATE_2500:
		period = 10000000/25;
		break;
	case NTV2_FRAMERATE_2400:
		period = 10000000/24;
		break;
	case NTV2_FRAMERATE_2398:
		period = 10010000/24;
		break;
	case NTV2_FRAMERATE_5000:
		period = 10000000/50;
		break;
#if !defined(NTV2_DEPRECATE_16_0)
	case NTV2_FRAMERATE_1900:
		period = 10000000/19;
		break;
	case NTV2_FRAMERATE_1898:
		period = 10010000/19;
		break;
	case NTV2_FRAMERATE_1800:
		period = 10000000/18;
		break;
	case NTV2_FRAMERATE_1798:
		period = 10010000/18;
		break;
#endif	//	!defined(NTV2_DEPRECATE_16_0)
	case NTV2_FRAMERATE_1500:
		period = 10000000/15;
		break;
	case NTV2_FRAMERATE_1498:
		period = 10010000/15;
		break;
	case NTV2_FRAMERATE_UNKNOWN:
	default:
		period = 10000000;
	}

	return period;
}

static ULWord
GetNumAudioChannels(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	ULWord control = GetAudioControlRegister(deviceNumber, audioSystem);

	if ((ReadRegister(deviceNumber, control, NO_MASK, NO_SHIFT) & BIT(20)))
		return 16;

	if ((ReadRegister(deviceNumber, control, NO_MASK, NO_SHIFT) & BIT(16)))
		return 8;
	else
		return 6;
}

ULWord
GetAudioSamplesPerFrame(ULWord deviceNumber, NTV2AudioSystem audioSystem, ULWord cadenceFrame, ULWord granularity, bool fieldMode)
{
	Ntv2SystemContext systemContext;
	NTV2FrameRate frameRate;
	NTV2AudioRate audioRate;
	bool smpte372Enabled;
	ULWord audioSamplesPerFrame=0;
	ULWord cadenceFrame5 = cadenceFrame % 5;
	ULWord cadenceFrame2 = cadenceFrame & 1;
	// do 4 sample granularity if granularity not 1
	bool oneSample = (granularity == 1);

	NTV2Channel channel = NTV2_CHANNEL1;
	
	systemContext.devNum = deviceNumber;
	
	if (IsMultiFormatActive(&systemContext))
	{
		switch (audioSystem)
		{
		case NTV2_AUDIOSYSTEM_Plus1:
			case NTV2_AUDIOSYSTEM_Plus2:
			case NTV2_AUDIOSYSTEM_Plus3:
		case NTV2_NUM_AUDIOSYSTEMS:		// This is probably an error
		case NTV2_AUDIOSYSTEM_1:
			channel = NTV2_CHANNEL1;
			break;
		case NTV2_AUDIOSYSTEM_2:
			channel = NTV2_CHANNEL2;
			break;
		case NTV2_AUDIOSYSTEM_3:
			channel = NTV2_CHANNEL3;
			break;
		case NTV2_AUDIOSYSTEM_4:
			channel = NTV2_CHANNEL4;
			break;
		case NTV2_AUDIOSYSTEM_5:
			channel = NTV2_CHANNEL5;
			break;
		case NTV2_AUDIOSYSTEM_6:
			channel = NTV2_CHANNEL6;
			break;
		case NTV2_AUDIOSYSTEM_7:
			channel = NTV2_CHANNEL7;
			break;
		case NTV2_AUDIOSYSTEM_8:
			channel = NTV2_CHANNEL8;
			break;
		}
	}

	audioRate=GetAudioRate(deviceNumber, audioSystem);
	frameRate=GetFrameRate(&systemContext, channel);
	smpte372Enabled = GetSmpte372(&systemContext, channel);

	if (smpte372Enabled || fieldMode)
	{
		switch (frameRate)
		{
		case NTV2_FRAMERATE_3000:
			frameRate = NTV2_FRAMERATE_6000;
			break;
		case NTV2_FRAMERATE_2997:
			frameRate = NTV2_FRAMERATE_5994;
			break;
		case NTV2_FRAMERATE_2500:
			frameRate = NTV2_FRAMERATE_5000;
			break;
		case NTV2_FRAMERATE_2400:
			frameRate = NTV2_FRAMERATE_4800;
			break;
		case NTV2_FRAMERATE_2398:
			frameRate = NTV2_FRAMERATE_4795;
			break;
		default:
			frameRate = NTV2_FRAMERATE_5994;
			break;
		}
	}

	switch (audioRate)
	{
	default:
	case NTV2_AUDIO_48K:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_12000:
			audioSamplesPerFrame = 400;
			break;
		case NTV2_FRAMERATE_11988:
			switch (cadenceFrame5)
			{
			case 0:
			case 2:
			case 4:
				audioSamplesPerFrame = 400;
				break;
			case 1:
			case 3:
				audioSamplesPerFrame = 401;
				break;
			}
			break;
		case NTV2_FRAMERATE_6000:
			audioSamplesPerFrame = 800;
			break;
		case NTV2_FRAMERATE_5000:
			audioSamplesPerFrame = 1920/2;
			break;
		case NTV2_FRAMERATE_5994:
			switch (cadenceFrame5)
			{
			case 0:
				audioSamplesPerFrame = 800;
				break;
			case 1:
				audioSamplesPerFrame = oneSample? 801 : 800;
				break;
			case 2:
				audioSamplesPerFrame = oneSample? 801 : 800;
				break;
			case 3:
				audioSamplesPerFrame = oneSample? 801 : 800;
				break;
			case 4:
				audioSamplesPerFrame = oneSample? 801 : 804;
				break;
			}
			break;
		case NTV2_FRAMERATE_4800:
			audioSamplesPerFrame = 1000;
			break;
		case NTV2_FRAMERATE_4795:
			audioSamplesPerFrame = 1001;
			break;
		case NTV2_FRAMERATE_3000:
			audioSamplesPerFrame = 1600;
			break;
		case NTV2_FRAMERATE_2997:
			switch (cadenceFrame5)
			{
			case 0:
				audioSamplesPerFrame = oneSample? 1602 : 1600;
				break;
			case 1:
				audioSamplesPerFrame = oneSample? 1601 : 1600;
				break;
			case 2:
				audioSamplesPerFrame = oneSample? 1602 : 1604;
				break;
			case 3:
				audioSamplesPerFrame = oneSample? 1601 : 1600;
				break;
			case 4:
				audioSamplesPerFrame = oneSample? 1602 : 1604;
				break;
			}
			break;
		case NTV2_FRAMERATE_2500:
			audioSamplesPerFrame = 1920;
			break;
		case NTV2_FRAMERATE_2400:
			audioSamplesPerFrame = 2000;
			break;
		case NTV2_FRAMERATE_2398:
			switch (cadenceFrame2)
			{
			case 0:
				audioSamplesPerFrame = oneSample? 2002 : 2000;
				break;
			case 1:
				audioSamplesPerFrame = oneSample? 2002 : 2004;
				break;
			}
			break;
		case NTV2_FRAMERATE_1500:
			audioSamplesPerFrame = 3200;
			break;
		case NTV2_FRAMERATE_1498:
			switch (cadenceFrame5)
			{
			case 0:
				audioSamplesPerFrame = oneSample? 3204 : 3200;
				break;
			case 1:
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = oneSample? 3203 : 3204;
				break;
			}
			break;
		case NTV2_FRAMERATE_UNKNOWN:
			audioSamplesPerFrame = 0;
			break;
		default:
			MSG("ULWordGetAudioSamplesPerFrame(); Unhandled Framerate, setting to 0\n");
			audioSamplesPerFrame = 0;
			break;

		}
		break;
		
	case NTV2_AUDIO_96K:
		switch (frameRate)
		{
		case NTV2_FRAMERATE_12000:
			audioSamplesPerFrame = 800;
			break;
		case NTV2_FRAMERATE_11988:
			switch (cadenceFrame5)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				audioSamplesPerFrame = 901;
				break;
			case 4:
				audioSamplesPerFrame = 800;
				break;
			}
			break;
		case NTV2_FRAMERATE_6000:
			audioSamplesPerFrame = 800*2;
			break;
		case NTV2_FRAMERATE_5000:
			audioSamplesPerFrame = 1920;
			break;
		case NTV2_FRAMERATE_5994:
			switch (cadenceFrame5)
			{
			case 0:
				audioSamplesPerFrame = oneSample? 1602 : 1600;
				break;
			case 1:
				audioSamplesPerFrame = oneSample? 1601 : 1600;
				break;
			case 2:
				audioSamplesPerFrame = oneSample? 1602 : 1604;
				break;
			case 3:
				audioSamplesPerFrame = oneSample? 1601 : 1600;
				break;
			case 4:
				audioSamplesPerFrame = oneSample? 1602 : 1604;
				break;
			}
			break;
		case NTV2_FRAMERATE_4800:
			audioSamplesPerFrame = 2000;
			break;
		case NTV2_FRAMERATE_4795:
			audioSamplesPerFrame = 2002;
			break;
		case NTV2_FRAMERATE_3000:
			audioSamplesPerFrame = 1600*2;
			break;
		case NTV2_FRAMERATE_2997:
			switch (cadenceFrame5)
			{
			case 0:
				audioSamplesPerFrame = oneSample? 3204 : 3200;
				break;
			case 1:
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = oneSample? 3203 : 3204;
				break;
			}
			break;
		case NTV2_FRAMERATE_2500:
			audioSamplesPerFrame = 1920*2;
			break;
		case NTV2_FRAMERATE_2400:
			audioSamplesPerFrame = 2000*2;
			break;
		case NTV2_FRAMERATE_2398:
			audioSamplesPerFrame = 2002*2;
			break;
		case NTV2_FRAMERATE_1500:
			audioSamplesPerFrame = 3200*2;
			break;
		case NTV2_FRAMERATE_1498:
			switch (cadenceFrame5)
			{
			case 0:
				audioSamplesPerFrame = oneSample? 3204*2 : 3200*2;
				break;
			case 1:
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = oneSample? 3203*2 : 3204*2;
				break;
			}
			break;
		case NTV2_FRAMERATE_UNKNOWN:
			audioSamplesPerFrame = 0*2; //haha
			break;
		default:
			MSG("ULWordGetAudioSamplesPerFrame(); Unhandled Framerate, setting to 0\n");
			audioSamplesPerFrame = 0;
			break;

		}

		break;

	case NTV2_AUDIO_192K:
		switch ( frameRate)
		{
		case NTV2_FRAMERATE_12000:
			audioSamplesPerFrame = 1600;
			break;
		case NTV2_FRAMERATE_11988:
			switch ( cadenceFrame )
			{
			case 0:
			case 2:
			case 4:
				audioSamplesPerFrame = 1602;
				break;
			case 1:
			case 3:
				audioSamplesPerFrame = 1601;
				break;
			}
			break;
		case NTV2_FRAMERATE_6000:
			audioSamplesPerFrame = 3200;
			break;
		case NTV2_FRAMERATE_5994:
			switch ( cadenceFrame )
			{
			case 0:
				audioSamplesPerFrame = 3204;
				break;
			case 1:
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = 3203;
				break;
			}
			break;
		case NTV2_FRAMERATE_5000:
			audioSamplesPerFrame = 3840;
			break;
		case NTV2_FRAMERATE_4800:
			audioSamplesPerFrame = 4000;
			break;
		case NTV2_FRAMERATE_4795:
			audioSamplesPerFrame = 4004;
			break;
		case NTV2_FRAMERATE_3000:
			audioSamplesPerFrame = 6400;
			break;
		case NTV2_FRAMERATE_2997:
			// depends on cadenceFrame;
			switch ( cadenceFrame )
			{
			case 0:
			case 1:
				audioSamplesPerFrame = 6407;
				break;
			case 2:
			case 3:
			case 4:
				audioSamplesPerFrame = 6406;
				break;
			}
			break;
		case NTV2_FRAMERATE_2500:
			audioSamplesPerFrame = 7680;
			break;
		case NTV2_FRAMERATE_2400:
			audioSamplesPerFrame = 8000;
			break;
		case NTV2_FRAMERATE_2398:
			audioSamplesPerFrame = 8008;
			break;
		case NTV2_FRAMERATE_1500:
			audioSamplesPerFrame = 12800;
			break;
		case NTV2_FRAMERATE_1498:
			// depends on cadenceFrame;
			switch ( cadenceFrame )
			{
			case 0:
			case 1:
			case 2:
			case 3:
				audioSamplesPerFrame = 12813;
				break;
			case 4:
				audioSamplesPerFrame = 12812;
				break;
			}
			break;
#if !defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_1900:	// Not supported yet
		case NTV2_FRAMERATE_1898:	// Not supported yet
		case NTV2_FRAMERATE_1800: 	// Not supported yet
		case NTV2_FRAMERATE_1798:	// Not supported yet
#endif	//!defined(NTV2_DEPRECATE_16_0)
		case NTV2_FRAMERATE_UNKNOWN:
		case NTV2_NUM_FRAMERATES:
			audioSamplesPerFrame = 0*2; //haha
			break;
		}
		break;
	}

	return audioSamplesPerFrame;
}

// GetAudioTransferInfo()
// Inputs
//  currentOffset;
//  numBytesToTransfer
// Outputs
// by reference
//  preWrapBytes;
//  postWrapBytes
// by return
//  nextOffset
//
ULWord
GetAudioTransferInfo(ULWord deviceNumber,
					 NTV2AudioSystem audioSystem,
					 ULWord currentOffset,
					 ULWord numBytesToTransfer,
					 ULWord* preWrapBytes,
					 ULWord* postWrapBytes)
{
	ULWord nextOffset = currentOffset + numBytesToTransfer;
	if (MsgsEnabled(NTV2_DRIVER_DMA_AUDIO_DEBUG_MESSAGES))
	{
		MSG("GetAudioTransferInfo(pre): bn=%d as=%d co=%u no=%u pre=%u post=%u wrap=0x%X bytexfer=%u\n",
			deviceNumber, audioSystem, currentOffset, nextOffset,
			*preWrapBytes, *postWrapBytes,
			GetAudioWrapAddress(deviceNumber, audioSystem), numBytesToTransfer);
	}
	if (nextOffset >= GetAudioWrapAddress(deviceNumber, audioSystem))
	{
		*preWrapBytes = GetAudioWrapAddress(deviceNumber, audioSystem) - currentOffset;
		*postWrapBytes = nextOffset - GetAudioWrapAddress(deviceNumber, audioSystem);
		nextOffset  = *postWrapBytes;
	}
	else
	{
		*preWrapBytes = nextOffset-currentOffset;
		*postWrapBytes = 0;
	}
	if (MsgsEnabled(NTV2_DRIVER_DMA_AUDIO_DEBUG_MESSAGES))
	{
		MSG("GetAudioTransferInfo(post): bn=%d as=%d co=%u no=%u pre=%u post=%u wrap=0x%X bytexfer=%u\n",
			deviceNumber, audioSystem, currentOffset, nextOffset,
			*preWrapBytes, *postWrapBytes,
			GetAudioWrapAddress(deviceNumber, audioSystem), numBytesToTransfer);
	}

	return nextOffset;
}

// Method: GetAudioWrapAddress
// Input:  NONE
// Output: ULWord
ULWord
GetAudioWrapAddress(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2AudioBufferSize bufferSize;
	bufferSize = GetAudioBufferSize(deviceNumber, audioSystem);

	if (bufferSize == NTV2_AUDIO_BUFFER_BIG)
		return NTV2_AUDIO_WRAPADDRESS_BIG;
	else
		return  NTV2_AUDIO_WRAPADDRESS;
}

// Method: GetAudioReadOffset
// Input:  NONE
// Output: ULWord
ULWord
GetAudioReadOffset(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2AudioBufferSize bufferSize;
	bufferSize = GetAudioBufferSize(deviceNumber, audioSystem);

	if (bufferSize == NTV2_AUDIO_BUFFER_BIG)
		return NTV2_AUDIO_READBUFFEROFFSET_BIG;
	else
		return NTV2_AUDIO_READBUFFEROFFSET;
}

// Method: GetAudioBufferSize
// Input:  NONE
// Output: NTV2AudioBufferSize
NTV2AudioBufferSize
GetAudioBufferSize(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	return ReadRegister (deviceNumber, GetAudioControlRegister(deviceNumber, audioSystem),
						 kK2RegMaskAudioBufferSize,
						 kK2RegShiftAudioBufferSize);
}

NTV2AudioRate
GetAudioRate(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2AudioRate outRate = NTV2_AUDIO_48K;
	ULWord		control;
	ULWord		rateLow;
	ULWord		rateHigh;

	control = GetAudioControlRegister(deviceNumber, audioSystem);
	rateLow = ReadRegister(deviceNumber, control, kRegMaskAudioRate, kRegShiftAudioRate);
	if (rateLow == 1)
		outRate = NTV2_AUDIO_96K;
	
	rateHigh = ReadRegister (deviceNumber, kRegAudioControl2,
							 gAudioRateHighMask[audioSystem], gAudioRateHighShift[audioSystem]);
	if (rateHigh == 1)
		outRate = NTV2_AUDIO_192K;
		
	return outRate;
}

ULWord
GetAudioSamplesPerSecond(ULWord deviceNumber, NTV2AudioSystem audioSystem)
{
	NTV2AudioRate rate;
	ULWord sps = 48000;

	rate = GetAudioRate(deviceNumber, audioSystem);
	if (rate == NTV2_AUDIO_96K)
		sps = 96000;
	if (rate == NTV2_AUDIO_192K)
		sps = 192000;

	return sps;		
}

bool
DropSyncFrame(ULWord deviceNumber, INTERNAL_AUTOCIRCULATE_STRUCT* pAuto)
{
	NTV2PrivateParams* pNTV2Params;
	ULWord syncChannel1 = NTV2CROSSPOINT_FGKEY;
	ULWord syncChannel2 = NTV2CROSSPOINT_FGKEY;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoChannel1 = NULL;
	INTERNAL_AUTOCIRCULATE_STRUCT* pAutoChannel2 = NULL;
	ULWord fpChannel1 = 0;
	ULWord fpChannel2 = 0;
	LWord nextFrame = 0;

	if (!(pNTV2Params = getNTV2Params(deviceNumber)))
	{
		return false;
	}

	syncChannel1 = pNTV2Params->_syncChannel1;
	syncChannel2 = pNTV2Params->_syncChannel2;
	pAutoChannel1 = &pNTV2Params->_AutoCirculate[syncChannel1];
	pAutoChannel2 = &pNTV2Params->_AutoCirculate[syncChannel2];
	fpChannel1 = pAutoChannel1->framesProcessed;
	fpChannel2 = pAutoChannel2->framesProcessed;

	if ((syncChannel1 == NTV2CROSSPOINT_FGKEY) ||
		(syncChannel2 == NTV2CROSSPOINT_FGKEY))
	{
		return false;
	}

	if (pAuto->channelSpec == syncChannel1)
	{
		if (fpChannel1 > fpChannel2)
		{
			return true;
		}
		else if (fpChannel1 == fpChannel2)
		{
			nextFrame = KAUTO_NEXTFRAME(pAutoChannel2->activeFrame, pAutoChannel2);
			if (pAutoChannel2->frameStamp[nextFrame].validCount == 0)
			{
				return true;
			}
		}
		return false;
	}
	if (pAuto->channelSpec == syncChannel2)
	{
		if (fpChannel2 > fpChannel1)
		{
			return true;
		}
		else if (fpChannel2 == fpChannel1)
		{
			nextFrame = KAUTO_NEXTFRAME(pAutoChannel1->activeFrame, pAutoChannel1);
			if (pAutoChannel1->frameStamp[nextFrame].validCount == 0)
			{
				return true;
			}
		}
		return false;
	}

	return false;
}

// DO NOT call this function from contexts that can't sleep
bool
OemAutoCirculateP2PCopy(PAUTOCIRCULATE_P2P_STRUCT pDriverBuffer,
						PAUTOCIRCULATE_P2P_STRUCT pUserBuffer,
						bool bToDriver)
{
	if ((pDriverBuffer == NULL) || (pUserBuffer == NULL))
	{
		MSG("OemAutoCirculateP2PCopy: NULL buffer\n");
		return false;
	}

	if (!bToDriver && (pDriverBuffer->p2pSize != sizeof(AUTOCIRCULATE_P2P_STRUCT)))
	{
		MSG("OemAutoCirculateP2PCopy: bad driver P2P struct id/size\n");
		return false;
	}

	if (bToDriver)
	{
		if (pUserBuffer->p2pSize != sizeof(AUTOCIRCULATE_P2P_STRUCT))
		{
			MSG("OemAutoCirculateP2PCopy: bad user P2P struct id/size\n");
			return false;
		}

		if (copy_from_user((void*)pDriverBuffer, pUserBuffer, sizeof(AUTOCIRCULATE_P2P_STRUCT)))
		{
			MSG("OemAutoCirculateP2PCopy: copy_from_user failed\n");
			return false;
		}
	}
	else
	{
		if (copy_to_user((void*)pUserBuffer, pDriverBuffer, sizeof(AUTOCIRCULATE_P2P_STRUCT)))
		{
			MSG("OemAutoCirculateP2PCopy: copy_to_user failed\n");
			return false;
		}
	}

	return true;
}

ULWord
Get2MFrameSize(ULWord deviceNumber, NTV2Channel channel)
{
	ULWord regNum;
	ULWord regVal = 0;

	if (!BoardIs2MCompatible(deviceNumber))
		return 0;

	switch (channel)
	{
	default:
	case NTV2_MAX_NUM_CHANNELS:		// Out of ramge
	case NTV2_CHANNEL1:
		regNum = kRegCh1Control2MFrame;
		break;
	case NTV2_CHANNEL2:
		regNum = kRegCh2Control2MFrame;
		break;
	case NTV2_CHANNEL3:
		regNum = kRegCh3Control2MFrame;
		break;
	case NTV2_CHANNEL4:
		regNum = kRegCh4Control2MFrame;
		break;
	case NTV2_CHANNEL5:
		regNum = kRegCh5Control2MFrame;
		break;
	case NTV2_CHANNEL6:
		regNum = kRegCh6Control2MFrame;
		break;
	case NTV2_CHANNEL7:
		regNum = kRegCh7Control2MFrame;
		break;
	case NTV2_CHANNEL8:
		regNum = kRegCh8Control2MFrame;
		break;
	}

	regVal = ReadRegister(deviceNumber, regNum, kRegMask2MFrameSize, kRegShift2MFrameSize);

	return 2048 * regVal;
}

bool
BoardIs2MCompatible(ULWord deviceNumber)
{
	ULWord regVal = 0;

	regVal = ReadRegister(deviceNumber, kRegGlobalControl2, kRegMask2MFrameSupport, kRegShift2MFrameSupport);

	if (regVal == 1)
		return true;
	else
		return false;
}

void
CopyFrameStampOldToNew(const FRAME_STAMP_STRUCT * pInOldStruct, FRAME_STAMP * pOutNewStruct)
{
	pOutNewStruct->acFrameTime = pInOldStruct->frameTime;
	pOutNewStruct->acRequestedFrame = pInOldStruct->frame;
	pOutNewStruct->acAudioClockTimeStamp = pInOldStruct->audioClockTimeStamp;
	pOutNewStruct->acAudioExpectedAddress = pInOldStruct->audioExpectedAddress;
	pOutNewStruct->acAudioInStartAddress = pInOldStruct->audioInStartAddress;
	pOutNewStruct->acAudioInStopAddress = pInOldStruct->audioInStopAddress;
	pOutNewStruct->acAudioOutStopAddress = pInOldStruct->audioOutStopAddress;
	pOutNewStruct->acAudioOutStartAddress = pInOldStruct->audioOutStartAddress;
	pOutNewStruct->acTotalBytesTransferred = pInOldStruct->bytesRead;
	pOutNewStruct->acStartSample = pInOldStruct->startSample;
	//	pOutNewStruct->acTimeCodes						= pInOldStruct->???no equivalent???;
	pOutNewStruct->acCurrentTime = pInOldStruct->currentTime;
	pOutNewStruct->acCurrentFrame = pInOldStruct->currentFrame;
	pOutNewStruct->acCurrentFrameTime = pInOldStruct->currentFrameTime;
	pOutNewStruct->acAudioClockCurrentTime = pInOldStruct->audioClockCurrentTime;
	pOutNewStruct->acCurrentAudioExpectedAddress = pInOldStruct->currentAudioExpectedAddress;
	pOutNewStruct->acCurrentAudioStartAddress = pInOldStruct->currentAudioStartAddress;
	pOutNewStruct->acCurrentFieldCount = pInOldStruct->currentFieldCount;
	pOutNewStruct->acCurrentLineCount = pInOldStruct->currentLineCount;
	pOutNewStruct->acCurrentReps = pInOldStruct->currentReps;
	pOutNewStruct->acCurrentUserCookie = pInOldStruct->currenthUser;
	pOutNewStruct->acFrame = pInOldStruct->frame;
	//	pOutNewStruct->acRP188							= pInOldStruct->currentRP188;
	NTV2_RP188_from_RP188_STRUCT(pOutNewStruct->acRP188, pInOldStruct->currentRP188);
}

int
AutoCirculateFrameStampImmediate(ULWord deviceNumber, FRAME_STAMP * pInOutFrameStamp, NTV2_RP188 * pOutTimecodeArray)
{
	//	On entry...
	//		FRAME_STAMP.acFrameTime			== requested NTV2Channel (in least significant byte)
	//		FRAME_STAMP.acRequestedFrame	== requested frame number

	INTERNAL_AUTOCIRCULATE_STRUCT *			pAuto;
	AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT	oldComboStruct;
	NTV2PrivateParams *						pNTV2Params;
	NTV2Crosspoint							crosspoint;
	NTV2Channel								channel;
	ULWord									modeValue;
	ULWord									frameNumber;
	int										returnCode;

	if (!pInOutFrameStamp)
		return -EINVAL;
	if (pInOutFrameStamp->acFrameTime < 0)
		return -ERANGE;

	channel = (NTV2Channel)pInOutFrameStamp->acFrameTime;
	if (!NTV2_IS_VALID_CHANNEL(channel))
		return -EINVAL;

	if (! (pNTV2Params = getNTV2Params(deviceNumber)))
		return -ENODEV;

	modeValue = ReadRegister(deviceNumber, gChannelToControlRegNum[channel], kRegMaskMode, kRegShiftMode);

	crosspoint = (modeValue == NTV2_MODE_DISPLAY) ?
		GetNTV2CrosspointChannelForIndex(channel) : GetNTV2CrosspointInputForIndex(channel);
	if (ILLEGAL_CHANNELSPEC(crosspoint))
		return -EINVAL;

	pAuto = &pNTV2Params->_AutoCirculate[crosspoint];
//	if (pAuto->recording && modeValue != NTV2_MODE_CAPTURE)
//		printk("%s: pAuto->recording=true, but mode=output for crosspoint %d channel %d\n",
//				__FUNCTION__, crosspoint, channel);
//	else if (! pAuto->recording && modeValue != NTV2_MODE_DISPLAY)
//		printk("%s: pAuto->recording=false, but mode=input for crosspoint %d channel %d\n",
//				__FUNCTION__, crosspoint, channel);

	memset(&oldComboStruct, 0, sizeof(oldComboStruct));

	//	Call the old API...
	oldComboStruct.acFrameStamp.channelSpec = crosspoint;
	oldComboStruct.acFrameStamp.frame = pInOutFrameStamp->acRequestedFrame;
	if ((returnCode = AutoCirculateFrameStamp(deviceNumber, &oldComboStruct)) != 0)
	{
		return returnCode;
	}

	//	Convert old FRAME_STAMP_STRUCT to new FRAME_STAMP struct..
	CopyFrameStampOldToNew(&oldComboStruct.acFrameStamp, pInOutFrameStamp);

	//	Grab all available timecodes...
	frameNumber = pInOutFrameStamp->acRequestedFrame;
	if (frameNumber < pAuto->startFrame || frameNumber > pAuto->endFrame)
		frameNumber = pAuto->activeFrame;
	if (frameNumber < pAuto->startFrame || frameNumber > pAuto->endFrame)
		frameNumber = pAuto->startFrame;

	NTV2_RP188_from_RP188_STRUCT(pInOutFrameStamp->acRP188, pAuto->frameStamp[frameNumber].rp188);	//	NOTE:  acRP188 field is deprecated
	if (! CopyFrameStampTCArrayToNTV2TimeCodeArray(&pAuto->frameStamp[frameNumber].internalTCArray,
												   pOutTimecodeArray,
												   pInOutFrameStamp->acTimeCodes.fByteCount))
	{
//		printk("%s: CopyFrameStampTCArrayToNTV2TimeCodeArray failed, frame=%d, byteCount=%d\n",
//			__FUNCTION__,
//			frameNumber,
//			pInOutFrameStamp->acTimeCodes.fByteCount);
		return -EFAULT;
	}

	return 0;
}

bool 
oemAutoCirculateCanDoFieldMode(ULWord deviceNumber, NTV2Crosspoint channelSpec)
{
	Ntv2SystemContext systemContext;
	NTV2Channel syncChannel = NTV2_CHANNEL1;
	bool fieldMode = false;
	NTV2VideoFormat vidFormat = NTV2_FORMAT_525_5994;
	systemContext.devNum = deviceNumber;

	if(IsMultiFormatActive(&systemContext))
	{
		syncChannel = GetNTV2ChannelForNTV2Crosspoint(channelSpec);
	}

	vidFormat = GetDeviceVideoFormat(deviceNumber, syncChannel);
	switch (vidFormat)
	{
	case NTV2_FORMAT_525_5994:
	case NTV2_FORMAT_625_5000:
	case NTV2_FORMAT_1080i_5000:
	case NTV2_FORMAT_1080i_5994:
	case NTV2_FORMAT_1080i_6000:
		fieldMode = true;
		break;
	default:
		break;
	}

	return fieldMode;
}
