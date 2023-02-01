/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
///////////////////////////////////////////////////////////////
//
// NTV2 Linux v2.6 Device Driver for AJA OEM boards.
//
////////////////////////////////////////////////////////////
//
// Filename: ntv2driverautocirculate.h
// Purpose:  Header file for autocirculate methods.
// Notes:
//
///////////////////////////////////////////////////////////////

#ifndef NTV2DRIVERAUTOCIRCULATE_H
#define NTV2DRIVERAUTOCIRCULATE_H

#include "ntv2dma.h"
#include "../ntv2kona.h"


//////////////////////////////////////////////////////////////////////////////////////
// Defines
//////////////////////////////////////////////////////////////////////////////////////
#define ILLEGAL_CHANNELSPEC(channelSpec) (((unsigned)channelSpec > (unsigned)NTV2CROSSPOINT_INPUT8) || (channelSpec == NTV2CROSSPOINT_MATTE) || (channelSpec == NTV2CROSSPOINT_FGKEY))

//#define NUM_CIRCULATE_FRAMES 128	// intended to exceed highest configuration number; KiPro currently uses 99 frame buffers
#define NUM_CIRCULATORS 18       // number of 'channels' which have auto-circulate capability
#define TIMECODE_ARRAY_SIZE 18
#define NTV2_INVALID_FRAME 0xFFFFFFFF


//////////////////////////////////////////////////////////////////////////////////////
// Typedefs
//////////////////////////////////////////////////////////////////////////////////////

typedef struct {
   NTV2ColorCorrectionMode mode;
   UWord   saturationValue;    /// only used in 3way color correction mode.
   ULWord  ccLookupTables[NTV2_COLORCORRECTOR_TABLESIZE/4]; /// R,G, and B lookup tables already formated for our hardware.
} INTERNAL_ColorCorrectionInfo;

typedef struct
{
	//! Processor RDTSC at time of play or record.
	LWord64			frameTime;
	//! 48kHz clock in reg 28 extended to 64 bits
	ULWord64		audioClockTimeStamp;	// Register 28 with Wrap Logic
	//! The address that was used to transfer
	ULWord			audioExpectedAddress;
	//! For record - first position in buffer of audio (includes base offset)
	ULWord			audioInStartAddress;	// AudioInAddress at the time this Frame was stamped.
	//! For record - end position (exclusive) in buffer of audio (includes base offset)
	ULWord			audioInStopAddress;		// AudioInAddress at the Frame AFTER this Frame was stamped.
	//! For play - first position in buffer of audio
	ULWord			audioOutStopAddress;	// AudioOutAddress at the time this Frame was stamped.
	//! For play - end position (exclusive) in buffer of audio
	ULWord		    audioOutStartAddress;	// AudioOutAddress at the Frame AFTER this Frame was stamped.
  	ULWord          audioPreWrapBytes; // For DMA Transfer

	ULWord          audioPostWrapBytes;

	//! Total audio and video bytes transfered
	ULWord			bytesRead;
	/** The actaul start sample when this frame was started in VBI
	* This may be used to check sync against audioInStartAddress (Play) or
	* audioOutStartAddress (Record).  In record it will always be equal, but
	* in playback if the clocks drift or the user supplies non aligned
	* audio sizes, then this will give the current difference from expected
	* vs actual position.  To be useful, play audio must be clocked in at
	* the correct rate.
	*/
	ULWord			startSample;
	//! Associated timecode (RP-188)
	RP188_STRUCT	rp188;
	//! Valid counts from n..0 in the isr.  Set to n when valid
	LWord			validCount;		// Used to throttle record and playback, See AutoCirculate Method in driver
	//! Repeat is set to n at beginning of DMA.  Moved to repeat on completion.
	LWord			repeatCount;	// Used to throttle record and playback, See AutoCirculate Method in driver
	//! Opaque user variable
	ULWord64		hUser;					// A user cookie returned by frame stamp

    NTV2FrameBufferFormat frameBufferFormat;
    NTV2VideoFrameBufferOrientation frameBufferOrientation;
	INTERNAL_ColorCorrectionInfo colorCorrectionInfo;
	AutoCircVidProcInfo vidProcInfo;
	NTV2RoutingTable	ntv2RoutingTable;
	AUTOCIRCULATE_TASK_STRUCT taskInfo;
	AutoCircGenericTask taskArray[AUTOCIRCULATE_TASK_MAX_TASKS];
	bool videoTransferPending;
	INTERNAL_TIMECODE_STRUCT internalTCArray;
	INTERNAL_SDI_STATUS_STRUCT internalSDIStatusArray;
	ULWord frameFlags;
	ULWord ancTransferSize;
	ULWord ancField2TransferSize;
	ULWord auxData[NTV2_HDMIAuxMaxFrames*NTV2_HDMIAuxDataSize/4];
	ULWord auxDataSize;
} INTERNAL_FRAME_STAMP_STRUCT;

typedef struct
{
	NTV2AutoCirculateState	state;
	NTV2Crosspoint			channelSpec;
	bool					recording;
	LWord					currentFrame;
	LWord					startFrame;
	LWord					endFrame;
	LWord					activeFrame;
	LWord					activeFrameRegister;
	bool					circulateWithAudio;
	bool					circulateWithRP188;
    bool					circulateWithColorCorrection;
    bool					circulateWithVidProc;
    bool					circulateWithCustomAncData;
	bool					circulateWithHDMIAux;
	bool					circulateWithFields;
    bool                    enableFbfChange;
    bool                    enableFboChange;
	LWord64					startTimeStamp;
	ULWord64				startAudioClockTimeStamp;
	ULWord					framesProcessed;
	ULWord					droppedFrames;
    ULWord                  nextFrame;
	INTERNAL_FRAME_STAMP_STRUCT frameStamp[NTV2_MAX_FRAMEBUFFERS];
	ULWord					nextAudioOutputAddress;
	ULWord					transferFrame;
	ULWord					audioTransferOffset;
	ULWord					audioTransferSize;
	ULWord					audioStartSample;
	ULWord					audioDropsRequired;
	ULWord					audioDropsCompleted;
	ULWord					ancTransferOffset;
	ULWord					ancTransferSize;
	ULWord					ancField2TransferOffset;
	ULWord					ancField2TransferSize;
	LWord64					lastInterruptTime;
	LWord64					prevInterruptTime;
	ULWord64				lastAudioClockTimeStamp;
	LWord64					startTime;
	bool                    circulateWithLTC;
	AUTOCIRCULATE_TASK_STRUCT	recordTaskInfo;
	AutoCircGenericTask			recordTaskArray[AUTOCIRCULATE_TASK_MAX_TASKS];
	INTERNAL_TIMECODE_STRUCT	timeCodeArray[TIMECODE_ARRAY_SIZE];
	LWord						timeCodeIndex;
	LWord						timeCodeDelay;
	ULWord					deviceNumber;
	NTV2AudioSystem			audioSystem;
	LWord					channelCount;

	// Keep track of interrupt timing per channel
	LWord64					VBIRDTSC;
	LWord64					VBILastRDTSC;
	ULWord					VBIAudioOut;
	bool					startAudioNextFrame;
	bool					stopAudioNextFrame;
	ULWord					audioSystemCount;
} INTERNAL_AUTOCIRCULATE_STRUCT;

//////////////////////////////////////////////////////////////////////////////////////
// Prototypes
//////////////////////////////////////////////////////////////////////////////////////

// Initialize all Auto Circulators -- done at driver load time.
int AutoCirculateInitialize(ULWord boardNumber);

int AutoCirculateControl(ULWord boardNumber, AUTOCIRCULATE_DATA *acData);

int AutoCirculateStatus(ULWord boardNumber, AUTOCIRCULATE_STATUS_STRUCT *acStatus);

int AutoCirculateStatus_Ex(ULWord boardNumber, AUTOCIRCULATE_STATUS *acStatus);

int AutoCirculateFrameStamp(ULWord boardNumber, AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT *frameStampCombo);

int AutoCirculateCaptureTask(ULWord boardNumber, AUTOCIRCULATE_FRAME_STAMP_COMBO_STRUCT *pFrameStampCombo);

int AutoCirculateTransfer(ULWord boardNumber, AUTOCIRCULATE_TRANSFER_COMBO_STRUCT *acXferCombo);

int AutoCirculateTransfer_Ex(ULWord boardNumber, PDMA_PAGE_ROOT pPageRoot, AUTOCIRCULATE_TRANSFER *acXferCombo);

//
// Workhorse functions
//

int OemAutoCirculateInit (	ULWord boardNumber,
	  						NTV2Crosspoint channelSpec,
							LWord lStartFrameNum,
							LWord lEndFrameNum,
							NTV2AudioSystem audioSystem,
							LWord lChanelCount,
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
							bool bWithASPlus3
							);

ULWord GetAudioFrameBufferNumber(ULWord boardNumber, ULWord boardID, NTV2AudioSystem audioSystem);

ULWord GetAudioTransferInfo(ULWord boardNumber, NTV2AudioSystem audioSystem, ULWord currentOffset,ULWord numBytesToTransfer,ULWord* preWrapBytes,ULWord* postWrapBytes);

ULWord GetAudioSamplesPerFrame(ULWord boardNumber, NTV2AudioSystem audioSystem, ULWord cadenceFrame, ULWord granularity, bool fieldMode);

ULWord GetAudioReadOffset(ULWord boardNumber, NTV2AudioSystem audioSystem);

NTV2AudioRate GetAudioRate(ULWord boardNumber, NTV2AudioSystem audioSystem);

ULWord GetAudioSamplesPerSecond(ULWord boardNumber, NTV2AudioSystem audioSystem);

ULWord GetAudioWrapAddress(ULWord boardNumber, NTV2AudioSystem audioSystem);

NTV2AudioBufferSize GetAudioBufferSize(ULWord boardNumber, NTV2AudioSystem audioSystem);

ULWord GetAudioLastIn(ULWord boardNumber, NTV2AudioSystem audioSystem);

ULWord GetAudioLastOut(ULWord boardNumber, NTV2AudioSystem audioSystem);

int OemAutoCirculateMessage(ULWord boardNumber, NTV2Crosspoint channelSpec, ULWord frameNumber);

void oemAutoCirculateTransferFields(ULWord deviceNumber,
									INTERNAL_AUTOCIRCULATE_STRUCT* pAuto, 
									DMA_PARAMS* pDmaParams, 
									ULWord frameNumber, bool drop);

ULWord Get2MFrameSize(ULWord boardNumber, NTV2Channel channel);

bool BoardIs2MCompatible(ULWord boardNumber);

void OemAutoCirculate(ULWord boardNumber, NTV2Crosspoint channelSpec);

int OemAutoCirculateStart (ULWord boardNumber, NTV2Crosspoint channelSpec, ULWord64 startTime);

int OemAutoCirculateStop (ULWord boardNumber, NTV2Crosspoint channelSpec);

int OemAutoCirculateAbort (ULWord boardNumber, NTV2Crosspoint channelSpec);

int OemAutoCirculatePause (ULWord boardNumber, NTV2Crosspoint channelSpec, bool bPlay, bool bClearDF);

int OemAutoCirculateFlush (ULWord boardNumber, NTV2Crosspoint channelSpec, bool bClearDF);

int OemAutoCirculatePreroll (ULWord boardNumber, NTV2Crosspoint channelSpec, LWord lPrerollFrames);

void OemAutoCirculateReset (ULWord boardNumber, NTV2Crosspoint channelSpec);

LWord OemAutoCirculateFindNextAvailFrame(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto);

void SetAudioPlaybackMode(ULWord boardNumber, NTV2_GlobalAudioPlaybackMode mode);

NTV2_GlobalAudioPlaybackMode GetAudioPlaybackMode(ULWord boardNumber);

ULWord GetNumFrameBuffers(ULWord boardNumber, ULWord boardID);

void OemAutoCirculateSetupNTV2Routing(ULWord boardNumber, NTV2RoutingTable* pNTV2RoutingTable);

void oemAutoCirculateWriteHDMIAux(ULWord deviceNumber, ULWord* pAuxData, ULWord auxDataSize);

NTV2VideoFormat GetNTV2VideoFormat(UByte status, UByte frameRateHiBit);

ULWord DownloadLinearLUTToHW (ULWord boardNumber, NTV2Channel channel, int bank);

ULWord64 GetAudioClock(ULWord boardNumber);

void CopyFrameStampOldToNew(const FRAME_STAMP_STRUCT * pInOldStruct, FRAME_STAMP * pOutNewStruct);

int AutoCirculateFrameStampImmediate(ULWord deviceNumber, FRAME_STAMP * pInOutFrameStamp, NTV2_RP188 * pOutTimecodeArray);

bool oemAutoCirculateCanDoFieldMode(ULWord deviceNumber, NTV2Crosspoint channelSpec);

#endif	//	 NTV2DRIVERAUTOCIRCULATE_H
