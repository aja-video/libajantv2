/**
	@file	ntv2dma.cpp
	@brief	Implementations of DMA-centric NTV2Card methods.
	@copyright	(C) 2004-2019 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include <assert.h>


bool CNTV2Card::DMARead (const ULWord inFrameNumber, ULWord * pFrameBuffer, const ULWord inOffsetBytes, const ULWord inByteCount)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, inFrameNumber, pFrameBuffer, inOffsetBytes, inByteCount, true);
}


bool CNTV2Card::DMAWrite (const ULWord inFrameNumber, const ULWord * pFrameBuffer, const ULWord inOffsetBytes, const ULWord inByteCount)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), inOffsetBytes, inByteCount, true);
}


bool CNTV2Card::DMAReadFrame (const ULWord inFrameNumber, ULWord * pFrameBuffer, const ULWord inByteCount)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, inFrameNumber, pFrameBuffer, (ULWord) 0, inByteCount, true);
}


bool CNTV2Card::DMAWriteFrame (const ULWord inFrameNumber, const ULWord * pFrameBuffer, const ULWord inByteCount)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), (ULWord) 0, inByteCount, true);
}


bool CNTV2Card::DMAReadSegments (	const ULWord		inFrameNumber,
									ULWord *			pFrameBuffer,
									const ULWord		inOffsetBytes,
									const ULWord		inByteCount,
									const ULWord		inNumSegments,
									const ULWord		inSegmentHostPitch,
									const ULWord		inSegmentCardPitch)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, inFrameNumber, pFrameBuffer, inOffsetBytes, inByteCount,
						inNumSegments, inSegmentHostPitch, inSegmentCardPitch, true);
}


bool CNTV2Card::DMAWriteSegments (	const ULWord		inFrameNumber,
									const ULWord *		pFrameBuffer,
									const ULWord		inOffsetBytes,
									const ULWord		inByteCount,
									const ULWord		inNumSegments,
									const ULWord		inSegmentHostPitch,
									const ULWord		inSegmentCardPitch)
{
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), inOffsetBytes, inByteCount,
						inNumSegments, inSegmentHostPitch, inSegmentCardPitch, true);
}


bool CNTV2Card::DmaP2PTargetFrame(NTV2Channel channel,
								  ULWord frameNumber,
								  ULWord frameOffset,
								  PCHANNEL_P2P_STRUCT pP2PData)
{
	return DmaTransfer (NTV2_PIO,
					   channel,
					   true,
					   frameNumber,
					   frameOffset,
					   0, 0, 0, 0,
					   pP2PData);
}


bool CNTV2Card::DmaP2PTransferFrame(NTV2DMAEngine DMAEngine,
									ULWord frameNumber,
									ULWord frameOffset,
									ULWord transferSize,
									ULWord numSegments,
									ULWord segmentTargetPitch,
									ULWord segmentCardPitch,
									PCHANNEL_P2P_STRUCT pP2PData)
{
	return DmaTransfer(DMAEngine,
					   NTV2_CHANNEL1,
					   false,
					   frameNumber,
					   frameOffset,
					   transferSize,
					   numSegments,
					   segmentTargetPitch,
					   segmentCardPitch,
					   pP2PData);
}


bool CNTV2Card::GetAudioMemoryOffset (const ULWord inOffsetBytes,  ULWord & outAbsByteOffset,  const NTV2AudioSystem inAudioSystem)
{
	outAbsByteOffset = 0;
	const NTV2DeviceID	deviceID	(GetDeviceID());
	if (UWord(inAudioSystem) >= (::NTV2DeviceGetNumAudioSystems(deviceID) + (DeviceCanDoAudioMixer() ? 1 : 0)))
		return false;	//	Invalid audio system

	if (::NTV2DeviceCanDoStackedAudio(deviceID))
	{
		const ULWord	EIGHT_MEGABYTES	(0x800000);
		const ULWord	memSize			(::NTV2DeviceGetActiveMemorySize(deviceID));
		const ULWord	engineOffset	(memSize  -  EIGHT_MEGABYTES * ULWord(inAudioSystem+1));
		outAbsByteOffset = inOffsetBytes + engineOffset;
	}
	else
	{
		NTV2FrameGeometry		fg	(NTV2_FG_INVALID);
		NTV2FrameBufferFormat	fbf	(NTV2_FBF_INVALID);
		if (!GetFrameGeometry (fg, NTV2Channel(inAudioSystem)) || !GetFrameBufferFormat (NTV2Channel(inAudioSystem), fbf))
			return false;

		const ULWord	audioFrameBuffer	(::NTV2DeviceGetNumberFrameBuffers(deviceID, fg, fbf) - 1);
		outAbsByteOffset = inOffsetBytes  +  audioFrameBuffer * ::NTV2DeviceGetFrameBufferSize(deviceID, fg, fbf);
	}
	return true;
}


bool CNTV2Card::DMAReadAudio (	const NTV2AudioSystem	inAudioSystem,
								ULWord *				pOutAudioBuffer,
								const ULWord			inOffsetBytes,
								const ULWord			inByteCount)
{
	if (!pOutAudioBuffer)
		return false;	//	NULL buffer
	if (!inByteCount)
		return false;	//	Nothing to transfer

	ULWord	absoluteByteOffset	(0);
	if (!GetAudioMemoryOffset (inOffsetBytes,  absoluteByteOffset,  inAudioSystem))
		return false;

	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, 0, pOutAudioBuffer, absoluteByteOffset, inByteCount, true);
}


bool CNTV2Card::DMAWriteAudio (	const NTV2AudioSystem	inAudioSystem,
								const ULWord *			pInAudioBuffer,
								const ULWord			inOffsetBytes,
								const ULWord			inByteCount)
{
	if (!pInAudioBuffer)
		return false;	//	NULL buffer
	if (!inByteCount)
		return false;	//	Nothing to transfer

	ULWord	absoluteByteOffset	(0);
	if (!GetAudioMemoryOffset (inOffsetBytes,  absoluteByteOffset,  inAudioSystem))
		return false;

	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, 0, const_cast <ULWord *> (pInAudioBuffer), absoluteByteOffset, inByteCount, true);
}


bool CNTV2Card::DMAReadAnc (const ULWord		inFrameNumber,
							UByte *				pOutAncBuffer,
							const NTV2FieldID	inFieldID,
							const ULWord		inByteCount)
{
	if (!::NTV2DeviceCanDoCustomAnc (GetDeviceID ()))
		return false;
	if (!NTV2_IS_VALID_FIELD(inFieldID))
		return false;

	NTV2_POINTER	ancF1Buffer	(inFieldID ? NULL : pOutAncBuffer, inFieldID ? 0 : inByteCount);
	NTV2_POINTER	ancF2Buffer	(inFieldID ? pOutAncBuffer : NULL, inFieldID ? inByteCount : 0);
	return DMAReadAnc (inFrameNumber, ancF1Buffer, ancF2Buffer);
}


bool CNTV2Card::DMAReadAnc (const ULWord	inFrameNumber,
							NTV2_POINTER & 	outAncF1Buffer,
							NTV2_POINTER & 	outAncF2Buffer)
{
	ULWord			F1Offset(0),  F2Offset(0), inByteCount(0), bytesToTransfer(0), byteOffsetToAncData(0);
	NTV2Framesize	frameSize(NTV2_FRAMESIZE_INVALID);
	bool			result(true);
	if (!::NTV2DeviceCanDoCustomAnc (GetDeviceID ()))
		return false;
	if (!ReadRegister (kVRegAncField1Offset, F1Offset))
		return false;
	if (!ReadRegister (kVRegAncField2Offset, F2Offset))
		return false;
	if (outAncF1Buffer.IsNULL()  &&  outAncF2Buffer.IsNULL())
		return false;
	if (!GetFrameBufferSize (NTV2_CHANNEL1, frameSize))
		return false;

	const ULWord	frameSizeInBytes(::NTV2FramesizeToByteCount(frameSize));

	//	IMPORTANT ASSUMPTION:	F1 data is first (at lower address) in the frame buffer...!
	inByteCount      =  outAncF1Buffer.IsNULL()  ?  0  :  outAncF1Buffer.GetByteCount();
	bytesToTransfer  =  inByteCount > F1Offset  ?  F1Offset  :  inByteCount;
	if (bytesToTransfer)
	{
		byteOffsetToAncData = frameSizeInBytes - F1Offset;
		result = DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, /*isRead*/true, inFrameNumber,
								reinterpret_cast <ULWord *> (outAncF1Buffer.GetHostPointer()),
								byteOffsetToAncData, bytesToTransfer, true);
	}
	inByteCount      =  outAncF2Buffer.IsNULL()  ?  0  :  outAncF2Buffer.GetByteCount();
	bytesToTransfer  =  inByteCount > F2Offset  ?  F2Offset  :  inByteCount;
	if (result  &&  bytesToTransfer)
	{
		byteOffsetToAncData = frameSizeInBytes - F2Offset;
		result = DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, /*isRead*/true, inFrameNumber,
								reinterpret_cast <ULWord *> (outAncF2Buffer.GetHostPointer()),
								byteOffsetToAncData, bytesToTransfer, true);
	}
	return result;
}


bool CNTV2Card::DMAWriteAnc (const ULWord		inFrameNumber,
							const UByte *		pInAncBuffer,
							const NTV2FieldID	inFieldID,
							const ULWord		inByteCount)
{
	if (!::NTV2DeviceCanDoCustomAnc (GetDeviceID ()))
		return false;
	if (!NTV2_IS_VALID_FIELD(inFieldID))
		return false;

	const NTV2_POINTER	ancF1Buffer	(inFieldID ? NULL : pInAncBuffer, inFieldID ? 0 : inByteCount);
	const NTV2_POINTER	ancF2Buffer	(inFieldID ? pInAncBuffer : NULL, inFieldID ? inByteCount : 0);
	return DMAWriteAnc (inFrameNumber, ancF1Buffer, ancF2Buffer);
}


bool CNTV2Card::DMAWriteAnc (const ULWord			inFrameNumber,
							const NTV2_POINTER &	inAncF1Buffer,
							const NTV2_POINTER &	inAncF2Buffer)
{
	ULWord			F1Offset(0),  F2Offset(0), inByteCount(0), bytesToTransfer(0), byteOffsetToAncData(0);
	NTV2Framesize	frameSize(NTV2_FRAMESIZE_INVALID);
	bool			result(true);
	if (!::NTV2DeviceCanDoCustomAnc (GetDeviceID ()))
		return false;
	if (!ReadRegister (kVRegAncField1Offset, F1Offset))
		return false;
	if (!ReadRegister (kVRegAncField2Offset, F2Offset))
		return false;
	if (inAncF1Buffer.IsNULL()  &&  inAncF2Buffer.IsNULL())
		return false;
	if (!GetFrameBufferSize (NTV2_CHANNEL1, frameSize))
		return false;

	const ULWord	frameSizeInBytes(::NTV2FramesizeToByteCount(frameSize));

	//	IMPORTANT ASSUMPTION:	F1 data is first (at lower address) in the frame buffer...!
	inByteCount      =  inAncF1Buffer.IsNULL()  ?  0  :  inAncF1Buffer.GetByteCount();
	bytesToTransfer  =  inByteCount > F1Offset  ?  F1Offset  :  inByteCount;
	if (bytesToTransfer)
	{
		byteOffsetToAncData = frameSizeInBytes - F1Offset;
		result = DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, /*isRead*/false, inFrameNumber,
								reinterpret_cast <ULWord *> (inAncF1Buffer.GetHostPointer()),
								byteOffsetToAncData, bytesToTransfer, true);
	}
	inByteCount      =  inAncF2Buffer.IsNULL()  ?  0  :  inAncF2Buffer.GetByteCount();
	bytesToTransfer  =  inByteCount > F2Offset  ?  F2Offset  :  inByteCount;
	if (result  &&  bytesToTransfer)
	{
		byteOffsetToAncData = frameSizeInBytes - F2Offset;
		result = DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, /*isRead*/false, inFrameNumber,
								reinterpret_cast <ULWord *> (inAncF2Buffer.GetHostPointer()),
								byteOffsetToAncData, bytesToTransfer, true);
	}
	return result;
}


#if !defined (NTV2_DEPRECATE)
	bool CNTV2Card::DmaRead (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
							 const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, true, inFrameNumber, pFrameBuffer, inOffsetBytes, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaWrite (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
							  const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), inOffsetBytes, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaReadFrame (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
								  const ULWord inByteCount, const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, true, inFrameNumber, pFrameBuffer, (ULWord) 0, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaWriteFrame (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
									 const ULWord inByteCount, const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), (ULWord) 0, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaReadSegment (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, ULWord * pFrameBuffer,
									const ULWord inOffsetBytes, const ULWord inByteCount,
									const ULWord inNumSegments, const ULWord inSegmentHostPitch, const ULWord inSegmentCardPitch,
									const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, true, inFrameNumber, pFrameBuffer, inOffsetBytes, inByteCount,
							inNumSegments, inSegmentHostPitch, inSegmentCardPitch, inSynchronous);
	}

	bool CNTV2Card::DmaWriteSegment (const NTV2DMAEngine inDMAEngine, const ULWord inFrameNumber, const ULWord * pFrameBuffer,
									const ULWord inOffsetBytes, const ULWord inByteCount,
									const ULWord inNumSegments, const ULWord inSegmentHostPitch, const ULWord inSegmentCardPitch,
									const bool inSynchronous)
	{
		return DmaTransfer (inDMAEngine, false, inFrameNumber, const_cast <ULWord *> (pFrameBuffer), inOffsetBytes, inByteCount,
							inNumSegments, inSegmentHostPitch, inSegmentCardPitch, inSynchronous);
	}

	bool CNTV2Card::DmaAudioRead (const NTV2DMAEngine inDMAEngine, const NTV2AudioSystem inAudioEngine, ULWord * pOutAudioBuffer,
								 const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous)
	{
		WriteRegister (kVRegAdvancedIndexing, 1);	//	Mac only, required for SDK 12.4 or earlier, obsolete after 12.4
		if (!::NTV2DeviceCanDoStackedAudio (GetDeviceID ()))
			return false;

		const ULWord	memSize			(::NTV2DeviceGetActiveMemorySize (GetDeviceID ()));
		const ULWord	engineOffset	(memSize - ((ULWord (inAudioEngine) + 1) * 0x800000));
		const ULWord	audioOffset		(inOffsetBytes + engineOffset);

		return DmaTransfer (inDMAEngine, true, 0, pOutAudioBuffer, audioOffset, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaAudioWrite (const NTV2DMAEngine inDMAEngine, const NTV2AudioSystem inAudioEngine, const ULWord * pInAudioBuffer,
								  const ULWord inOffsetBytes, const ULWord inByteCount, const bool inSynchronous)
	{
		WriteRegister (kVRegAdvancedIndexing, 1);	//	Mac only, required for SDK 12.4 or earlier, obsolete after 12.4
		if (!::NTV2DeviceCanDoStackedAudio (GetDeviceID ()))
			return false;

		const ULWord	memSize			(::NTV2DeviceGetActiveMemorySize (GetDeviceID ()));
		const ULWord	engineOffset	(memSize - ((ULWord (inAudioEngine) + 1) * 0x800000));
		const ULWord	audioOffset		(inOffsetBytes + engineOffset);

		return DmaTransfer (inDMAEngine, false, 0, const_cast <ULWord *> (pInAudioBuffer), audioOffset, inByteCount, inSynchronous);
	}

	bool CNTV2Card::DmaReadField (NTV2DMAEngine DMAEngine, ULWord frameNumber, NTV2FieldID fieldID,
								  ULWord * pFrameBuffer, ULWord bytes, bool bSync)
	{
		const ULWord	ulOffset	(fieldID == NTV2_FIELD0 ? 0 : ULWord (_ulFrameBufferSize / 2));
		return DmaTransfer (DMAEngine, true, frameNumber, (ULWord *) pFrameBuffer, ulOffset, bytes, bSync);
	}

	bool CNTV2Card::DmaWriteField (NTV2DMAEngine DMAEngine, ULWord frameNumber, NTV2FieldID fieldID,
								   ULWord * pFrameBuffer, ULWord bytes, bool bSync)
	{
		const ULWord	ulOffset	(fieldID == NTV2_FIELD0 ? 0 : ULWord (_ulFrameBufferSize / 2));
		return DmaTransfer (DMAEngine, false, frameNumber, pFrameBuffer, ulOffset, bytes, bSync);
	}
#endif	//	!defined (NTV2_DEPRECATE)
