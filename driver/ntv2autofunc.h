/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2004 - 2022 AJA Video Systems, Inc.
 */
//========================================================================
//
//  ntv2autodma.h
//
//==========================================================================

#ifndef NTV2AUTODMA_H
	#define NTV2AUTODMA_H

	#include "ntv2system.h"
	#include "ntv2publicinterface.h"

	// dma transfer parameters
	typedef struct _autoDmaParams
	{
		bool				toHost;					// transfer to host
		NTV2DMAEngine		dmaEngine;				// dma engine
		NTV2Channel			videoChannel;			// video channel for frame size
		void*				pVidUserVa;				// user video buffer
		uint64_t			videoBusAddress;		// p2p video bus address
		uint32_t			videoBusSize;			// p2p video bus size
		uint64_t			messageBusAddress;		// p2p message bus address
		uint32_t			messageData;			// p2p message data
		uint32_t			videoFrame;				// card video frame
		uint32_t			vidNumBytes;			// number of bytes per segment
		uint32_t			frameOffset; 			// card video offset
		uint32_t			vidUserPitch;			// user buffer pitch
		uint32_t			vidFramePitch;			// card frame pitch
		uint32_t			numSegments;			// number of segments
		void*				pAudUserVa;				// audio user buffer
		NTV2AudioSystem		audioSystem;			// audio system target
		uint32_t			audNumBytes;			// number of audio bytes
		uint32_t			audOffset;				// card audio offset
		void*				pAncF1UserVa;			// anc field 1 user buffer
		uint32_t			ancF1Frame;				// anc field 1 frame
		uint32_t			ancF1NumBytes;			// number of anc field 1 bytes
		uint32_t			ancF1Offset;			// anc field 1 frame offset
		void*				pAncF2UserVa;			// anc field 2 user buffer
		uint32_t			ancF2Frame;				// anc field 2 frame
		uint32_t			ancF2NumBytes;			// number of anc field 2 bytes
		uint32_t			ancF2Offset;			// anc field 2 frame offset
	} AUTO_DMA_PARAMS, *PAUTO_DMA_PARAMS;

	//	STUBS
	//	Real device drivers and fake devices must implement:
	Ntv2Status	AutoDmaTransfer(void* pContext, PAUTO_DMA_PARAMS pDmaParams);
	int64_t		AutoGetAudioClock(void* pContext);
	bool		AutoBoardCanDoP2P(void* pContext);								//	P2P-related
	uint64_t	AutoGetFrameAperturePhysicalAddress(void* pContext);			//	P2P	only
	uint32_t	AutoGetFrameApertureBaseSize(void* pContext);					//	P2P only
	void		AutoWriteFrameApertureOffset(void* pContext, uint32_t value);	//	P2P only
	uint64_t	AutoGetMessageAddress(void* pContext, NTV2Channel channel);		//	P2P only

#endif	//	NTV2AUTODMA_H
