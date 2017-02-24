/**
	@file	ntv2dma.cpp
	@brief	Implementations of DMA-centric NTV2Card methods.
	@copyright	(C) 2004-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
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


bool CNTV2Card::DMAReadAudio (	const NTV2AudioSystem	inAudioEngine,
								ULWord *				pOutAudioBuffer,
								const ULWord			inOffsetBytes,
								const ULWord			inByteCount)
{
    NTV2DeviceID deviceID = GetDeviceID();
	WriteRegister (kVRegAdvancedIndexing, 1);	//	Mac only, required for SDK 12.4 or earlier, obsolete after 12.4

    if (::NTV2DeviceCanDoStackedAudio(deviceID))
    {
        const ULWord	memSize			(::NTV2DeviceGetActiveMemorySize(deviceID));
        const ULWord	engineOffset	(memSize - ((ULWord (inAudioEngine) + 1) * 0x800000));
        const ULWord	audioOffset		(inOffsetBytes + engineOffset);

        return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, 0, pOutAudioBuffer, audioOffset, inByteCount, true);
    }
    else
    {
        if (UWord (inAudioEngine) >= ::NTV2DeviceGetNumAudioStreams(deviceID))
            return false;	//	Invalid audio system

        NTV2Channel channel = static_cast <NTV2Channel>(inAudioEngine);

        NTV2FrameGeometry fg;
        NTV2FrameBufferFormat fbf;
        GetFrameGeometry(&fg, channel);
        GetFrameBufferFormat(channel, &fbf);
        ULWord audioFrameBuffer = NTV2DeviceGetNumberFrameBuffers(deviceID, fg, fbf) - 1;
        ULWord audioOffset = inOffsetBytes + (audioFrameBuffer * NTV2DeviceGetFrameBufferSize(deviceID, fg, fbf));

        return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, 0, pOutAudioBuffer, audioOffset, inByteCount, true);
    }
}


bool CNTV2Card::DMAWriteAudio (	const NTV2AudioSystem	inAudioEngine,
								const ULWord *			pInAudioBuffer,
								const ULWord			inOffsetBytes,
								const ULWord			inByteCount)
{
    NTV2DeviceID deviceID = GetDeviceID();
	WriteRegister (kVRegAdvancedIndexing, 1);	//	Mac only, required for SDK 12.4 or earlier, obsolete after 12.4
    if (::NTV2DeviceCanDoStackedAudio (deviceID))
    {
        const ULWord	memSize			(::NTV2DeviceGetActiveMemorySize(deviceID));
        const ULWord	engineOffset	(memSize - ((ULWord (inAudioEngine) + 1) * 0x800000));
        const ULWord	audioOffset		(inOffsetBytes + engineOffset);

        return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, 0, const_cast <ULWord *> (pInAudioBuffer), audioOffset, inByteCount, true);
    }
    else
    {
        if (UWord (inAudioEngine) >= ::NTV2DeviceGetNumAudioStreams(deviceID))
            return false;	//	Invalid audio system

        NTV2Channel channel = static_cast <NTV2Channel>(inAudioEngine);

        NTV2FrameGeometry fg;
        NTV2FrameBufferFormat fbf;
        GetFrameGeometry(&fg, channel);
        GetFrameBufferFormat(channel, &fbf);
        ULWord audioFrameBuffer = NTV2DeviceGetNumberFrameBuffers(deviceID, fg, fbf) - 1;
        ULWord audioOffset = inOffsetBytes + (audioFrameBuffer * NTV2DeviceGetFrameBufferSize(deviceID, fg, fbf));

        return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, 0, const_cast <ULWord *> (pInAudioBuffer), audioOffset, inByteCount, true);
    }
}


bool CNTV2Card::DMAReadAnc (const ULWord		inFrameNumber,
							UByte *				pOutAncBuffer,
							const NTV2FieldID	inFieldID,
							const ULWord		inByteCount)
{
	const ULWord	maxAncBytesPerField	(4096);
	const ULWord	bytesToTransfer		(inByteCount > maxAncBytesPerField ? maxAncBytesPerField : inByteCount);
	NTV2Framesize	frameSize			(NTV2_FRAMESIZE_INVALID);

	if (!::NTV2DeviceCanDoCustomAnc (GetDeviceID ()))
		return false;

	GetFrameBufferSize (NTV2_CHANNEL1, frameSize);

	const ULWord	currentFrameSizeInBytes	(::NTV2FramesizeToByteCount (frameSize));
	const ULWord	byteOffsetToAncData		(currentFrameSizeInBytes - maxAncBytesPerField * (inFieldID == NTV2_FIELD0 ? 2 : 1));
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, true, inFrameNumber, reinterpret_cast <ULWord *> (pOutAncBuffer), byteOffsetToAncData, bytesToTransfer, true);
}


bool CNTV2Card::DMAWriteAnc (const ULWord		inFrameNumber,
							const UByte *		pInAncBuffer,
							const NTV2FieldID	inFieldID,
							const ULWord		inByteCount)
{
	const ULWord	maxAncBytesPerField	(4096);
	const ULWord	bytesToTransfer		(inByteCount > maxAncBytesPerField ? maxAncBytesPerField : inByteCount);
	NTV2Framesize	frameSize			(NTV2_FRAMESIZE_INVALID);

	if (!::NTV2DeviceCanDoCustomAnc (GetDeviceID ()))
		return false;

	GetFrameBufferSize (NTV2_CHANNEL1, frameSize);

	const ULWord	currentFrameSizeInBytes	(::NTV2FramesizeToByteCount (frameSize));
	const ULWord	byteOffsetToAncData		(currentFrameSizeInBytes - maxAncBytesPerField * (inFieldID == NTV2_FIELD0 ? 2 : 1));
	UByte *			pHostBuffer				(const_cast <UByte *> (pInAncBuffer));
	return DmaTransfer (NTV2_DMA_FIRST_AVAILABLE, false, inFrameNumber, reinterpret_cast <ULWord *> (pHostBuffer), byteOffsetToAncData, bytesToTransfer, true);
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
