/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//========================================================================
//
//  ntv2dev_autocirc.h
//
//==========================================================================

#ifndef NTV2AUTOCIRC_H
#define NTV2AUTOCIRC_H

#include "ntv2system.h"
#include "ntv2publicinterface.h"
#include "ntv2rp188.h"

#define NUM_CIRCULATORS 18       // number of 'channels' which have auto-circulate capability
#define NUM_CIRCULATE_FRAMES 64

#define ILLEGAL_CHANNELSPEC(channelSpec) (((unsigned)channelSpec > (unsigned)NTV2CROSSPOINT_INPUT8) || (channelSpec == NTV2CROSSPOINT_MATTE) || (channelSpec == NTV2CROSSPOINT_FGKEY))

#define NTV2_INVALID_FRAME 0xFFFFFFFF

typedef struct {
	NTV2ColorCorrectionMode		mode;
	uint32_t		saturationValue;    /// only used in 3way color correction mode
	uint32_t		ccLookupTables[NTV2_COLORCORRECTOR_TABLESIZE/4]; /// R,G,B lookup tables formated for hardware
} INTERNAL_COLOR_CORRECTION_STRUCT;

typedef struct {
	//! Processor RDTSC at time of play or record.
    int64_t			frameTime;
	//! 48kHz clock in reg 28 extended to 64 bits
    uint64_t		audioClockTimeStamp;    // Register 28 with Wrap Logic
	//! The address that was used to transfer
    uint32_t		audioExpectedAddress;
	//! For record - first position in buffer of audio (includes base offset)
    uint32_t		audioInStartAddress;	// AudioInAddress at the time this Frame was stamped.
	//! For record - end position (exclusive) in buffer of audio (includes base offset)
    uint32_t		audioInStopAddress;		// AudioInAddress at the Frame AFTER this Frame was stamped.
	//! For play - first position in buffer of audio
    uint32_t		audioOutStopAddress;	// AudioOutAddress at the time this Frame was stamped.
	//! For play - end position (exclusive) in buffer of audio
    uint32_t		audioOutStartAddress;	// AudioOutAddress at the Frame AFTER this Frame was stamped.
	uint32_t		audioPreWrapBytes; // For DMA Transfer
	uint32_t		audioPostWrapBytes;	
	//! Total audio and video bytes transfered
	uint32_t		bytesRead;
	/** The actaul start sample when this frame was started in VBI
	* This may be used to check sync against audioInStartAddress (Play) or
	* audioOutStartAddress (Record).  In record it will always be equal, but
	* in playback if the clocks drift or the user supplies non aligned 
	* audio sizes, then this will give the current difference from expected 
	* vs actual position.  To be useful, play audio must be clocked in at
	* the correct rate.
	*/
    uint32_t		startSample;
	//! Associated timecode (RP-188)
    RP188_STRUCT	rp188;                  
	//! Valid counts from n..0 in the isr.  Set to n when valid (n being the number of times (VBIs) to play this frame before advancing to the next.
    // So, 0 indicates a not-ready frame, 1 indicates a normal ready frame, and >1 indicates a preroll condition.
    int32_t			validCount;             // Used to throttle record and playback, See AutoCirculate Method in driver
	//! Repeat is set to n at beginning of DMA.  Moved to repeat on completion.
    int32_t			repeatCount;            // Used to throttle record and playback, See AutoCirculate Method in driver
	//! Opaque user variable
	uint64_t		hUser;					// A user cookie returned by frame stamp
	bool			videoTransferPending;	// p2p transfer in progress
	uint32_t		frameFlags;				// p2p and field flags
    NTV2FrameBufferFormat				frameBufferFormat;
    NTV2VideoFrameBufferOrientation		frameBufferOrientation;
	INTERNAL_COLOR_CORRECTION_STRUCT	colorCorrectionInfo;
	AutoCircVidProcInfo					vidProcInfo;
	CUSTOM_ANC_STRUCT					customAncInfo;
	NTV2RoutingTable					xena2RoutingTable;
	INTERNAL_TIMECODE_STRUCT			internalTCArray;
	INTERNAL_SDI_STATUS_STRUCT			internalSDIStatusArray;
	// Anc frame info
	uint32_t		ancTransferSize;
	uint32_t		ancField2TransferSize;
	uint32_t		auxData[NTV2_HDMIAuxMaxFrames*NTV2_HDMIAuxDataSize/4];
	uint32_t		auxDataSize;
} INTERNAL_FRAME_STAMP_STRUCT;

typedef struct {
	Ntv2SystemContext*		pSysCon;
	void*					pFunCon;
	NTV2AutoCirculateState	state;
	NTV2Crosspoint			channelSpec;
	NTV2DMAEngine			DMAEngine;
	bool					recording;  
	int32_t					currentFrame; 
	int32_t					startFrame;
	int32_t					endFrame;
	int32_t					activeFrame;  
	int32_t					activeFrameRegister;
	int32_t					nextFrame;
	bool					circulateWithAudio; 
	bool					circulateWithRP188;
	bool					circulateWithColorCorrection;
	bool					circulateWithVidProc;
	bool					circulateWithCustomAncData;
	bool					enableFbfChange;
	bool					enableFboChange;
	int64_t					startTimeStamp;  
	uint64_t				startAudioClockTimeStamp;
	uint32_t				framesProcessed;
	uint32_t				droppedFrames;
	uint32_t				nextTransferFrame;
	INTERNAL_FRAME_STAMP_STRUCT	frameStamp[MAX_FRAMEBUFFERS];
	uint32_t				nextAudioOutputAddress;
	uint32_t				audioStartSample;
	uint32_t				audioDropsRequired;
	uint32_t				audioDropsCompleted;
	int64_t					lastInterruptTime;
	int64_t					prevInterruptTime;
	uint64_t				lastAudioClockTimeStamp;
	int64_t					startTime;
	bool					circulateWithLTC;
	NTV2AudioSystem			audioSystem;
	int32_t					channelCount;
	bool					videoTransferPending;
	bool					startAudioNextFrame;
	bool					stopAudioNextFrame;
	int64_t					VBIRDTSC;
	int64_t					VBILastRDTSC;
	uint32_t				VBIAudioOut;
	uint32_t				transferFrame;
	uint32_t				audioTransferOffset;
	uint32_t				audioTransferSize;
	uint32_t				ancTransferOffset;
	uint32_t				ancField2TransferOffset;
	uint32_t				ancTransferSize;
	uint32_t				ancField2TransferSize;
	bool					circulateWithHDMIAux;
	bool					circulateWithFields;
} INTERNAL_AUTOCIRCULATE_STRUCT;

typedef struct ntv2autocirc
{
	Ntv2SystemContext*	pSysCon;
	void*				pFunCon;
	NTV2DeviceID		deviceID;
	INTERNAL_AUTOCIRCULATE_STRUCT	autoCirculate[NUM_CIRCULATORS];
	NTV2Channel						ancInputChannel[NTV2_MAX_NUM_CHANNELS];
	NTV2_GlobalAudioPlaybackMode	globalAudioPlaybackMode;
	NTV2Crosspoint		syncChannel1;
	NTV2Crosspoint		syncChannel2;
	bool				startAudioNextFrame;
	bool				stopAudioNextFrame;
	bool				bMultiChannel;
} NTV2AutoCirc;

Ntv2Status AutoCirculateControl(NTV2AutoCirc* pAutoCirc, AUTOCIRCULATE_DATA* psControl);
Ntv2Status AutoCirculateInit(NTV2AutoCirc* pAutoCirc,
							 NTV2Crosspoint lChannelSpec, int32_t lStartFrameNum,
							 int32_t lEndFrameNum, NTV2AudioSystem lAudioSystem,
							 int32_t lChannelCount, bool bWithAudio,
							 bool bWithRP188, bool bFbfChange,
							 bool bFboChange , bool bWithColorCorrection,
							 bool bWithVidProc, bool bWithCustomAncData, 
							 bool bWithLTC, bool bWithFields,
							 bool bWithHDMIAux);
Ntv2Status AutoCirculateStart(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int64_t startTime);
Ntv2Status AutoCirculateStop(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec);
Ntv2Status AutoCirculateAbort(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec);
Ntv2Status AutoCirculatePause(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, bool bPlay, bool bClearDF);
Ntv2Status AutoCirculateFlush(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, bool bClearDF);
Ntv2Status AutoCirculatePreroll(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int32_t lPrerollFrames);
Ntv2Status AutoCirculateSetActiveFrame(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int32_t lActiveFrame);
void AutoCirculateReset(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec);
Ntv2Status AutoCirculateGetStatus(NTV2AutoCirc* pAutoCirc, AUTOCIRCULATE_STATUS* pUserOutBuffer);
Ntv2Status AutoCirculateGetFrameStamp(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int32_t ulFrameNum,
									  FRAME_STAMP_STRUCT *pFrameStamp);
Ntv2Status AutoCirculateTransfer(NTV2AutoCirc* pAutoCirc,
								 AUTOCIRCULATE_TRANSFER* pTransferStruct);
Ntv2Status AutoCirclateAudioPlaybackMode(NTV2AutoCirc* pAutoCirc,
										 NTV2AudioSystem audioSystem,
										 NTV2_GlobalAudioPlaybackMode mode);
uint32_t AutoCirculateGetBufferLevel(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto);
bool AutoCirculateFindNextAvailFrame(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto);
void AutoBeginAutoCirculateTransfer(uint32_t frameNumber,
									AUTOCIRCULATE_TRANSFER *pTransferStruct,
									INTERNAL_AUTOCIRCULATE_STRUCT *pAuto);
void AutoCompleteAutoCirculateTransfer(uint32_t frameNumber, AUTOCIRCULATE_TRANSFER_STATUS *pUserOutBuffer,
									   INTERNAL_AUTOCIRCULATE_STRUCT *pAuto,
									   bool updateValid, bool transferPending);
void AutoCirculateMessage(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, uint32_t frameNumber);
void AutoCirculateTransferFields(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto, 
								 AUTOCIRCULATE_TRANSFER* pTransfer, 
								 uint32_t frameNumber, bool drop);

bool AutoCirculate(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec, int32_t isrTimeStamp);
bool AutoIsAutoCirculateInterrupt(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec);
int32_t KAUTO_NEXTFRAME(int32_t __dwCurFrame_, INTERNAL_AUTOCIRCULATE_STRUCT* __pAuto_);
int32_t KAUTO_PREVFRAME(int32_t __dwCurFrame_, INTERNAL_AUTOCIRCULATE_STRUCT* __pAuto_);
void AutoCirculateSetupColorCorrector(NTV2AutoCirc* pAutoCirc,
									  NTV2Crosspoint channelSpec,
									  INTERNAL_COLOR_CORRECTION_STRUCT *ccInfo);
void AutoCirculateSetupXena2Routing(NTV2AutoCirc* pAutoCirc, NTV2RoutingTable* pXena2Routing);
void AutoCirculateWriteHDMIAux(NTV2AutoCirc* pAutoCirc, uint32_t* pAuxData, uint32_t auxDataSize);
bool AutoCirculateDmaAudioSetup(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto);
bool AutoCirculateCanDoFieldMode(INTERNAL_AUTOCIRCULATE_STRUCT* pAuto);

bool AutoDropSyncFrame(NTV2AutoCirc* pAutoCirc, NTV2Crosspoint channelSpec);
void AutoCirculateSetupVidProc(NTV2AutoCirc* pAutoCirc,
							   NTV2Crosspoint channelSpec,
							   AutoCircVidProcInfo* vidProcInfo);
void AutoCirculateTransferColorCorrectorInfo(NTV2AutoCirc* pAutoCirc,
											 INTERNAL_COLOR_CORRECTION_STRUCT* ccInternalInfo,
											 NTV2ColorCorrectionInfo* ccTransferInfo);
bool AutoCirculateP2PCopy(NTV2AutoCirc* pAutoCirc,
						  PAUTOCIRCULATE_P2P_STRUCT pDriverBuffer,
						  PAUTOCIRCULATE_P2P_STRUCT pUserBuffer,
						  bool bToDriver);
void AutoCopyFrameStampOldToNew(const FRAME_STAMP_STRUCT * pInOldStruct, FRAME_STAMP * pOutNewStruct);
bool AutoCirculateFrameStampImmediate(NTV2AutoCirc* pAutoCirc, FRAME_STAMP * pInOutFrameStamp);

#endif
