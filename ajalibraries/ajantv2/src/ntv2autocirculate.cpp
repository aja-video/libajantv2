/**
	@file		ntv2autocirculate.cpp
	@brief		Implements the CNTV2Card AutoCirculate API functions.
	@copyright	(C) 2004-2017 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include <iomanip>
#include <assert.h>
using namespace std;

#if defined(_DEBUG)
	//	Debug builds can clear Anc buffers during A/C capture
	#define	AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER		//	Requires non-zero kVRegZeroDeviceAncPostCapture
	#define AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER	//	Requires non-zero kVRegZeroHostAncPostCapture
#endif	//	_DEBUG

#if !defined (NTV2_DEPRECATE_12_6)
static const NTV2Channel gCrosspointToChannel [] = {	/* NTV2CROSSPOINT_CHANNEL1	==>	*/	NTV2_CHANNEL1,
														/* NTV2CROSSPOINT_CHANNEL2	==>	*/	NTV2_CHANNEL2,
														/* NTV2CROSSPOINT_INPUT1	==>	*/	NTV2_CHANNEL1,
														/* NTV2CROSSPOINT_INPUT2	==>	*/	NTV2_CHANNEL2,
														/* NTV2CROSSPOINT_MATTE		==>	*/	NTV2_MAX_NUM_CHANNELS,
														/* NTV2CROSSPOINT_FGKEY		==>	*/	NTV2_MAX_NUM_CHANNELS,
														/* NTV2CROSSPOINT_CHANNEL3	==>	*/	NTV2_CHANNEL3,
														/* NTV2CROSSPOINT_CHANNEL4	==>	*/	NTV2_CHANNEL4,
														/* NTV2CROSSPOINT_INPUT3	==>	*/	NTV2_CHANNEL3,
														/* NTV2CROSSPOINT_INPUT4	==>	*/	NTV2_CHANNEL4,
														/* NTV2CROSSPOINT_CHANNEL5	==>	*/	NTV2_CHANNEL5,
														/* NTV2CROSSPOINT_CHANNEL6	==>	*/	NTV2_CHANNEL6,
														/* NTV2CROSSPOINT_CHANNEL7	==>	*/	NTV2_CHANNEL7,
														/* NTV2CROSSPOINT_CHANNEL8	==>	*/	NTV2_CHANNEL8,
														/* NTV2CROSSPOINT_INPUT5	==>	*/	NTV2_CHANNEL5,
														/* NTV2CROSSPOINT_INPUT6	==>	*/	NTV2_CHANNEL6,
														/* NTV2CROSSPOINT_INPUT7	==>	*/	NTV2_CHANNEL7,
														/* NTV2CROSSPOINT_INPUT8	==>	*/	NTV2_CHANNEL8,
																							NTV2_MAX_NUM_CHANNELS};
static const bool		gIsCrosspointInput [] = {		/* NTV2CROSSPOINT_CHANNEL1	==> */	false,
														/* NTV2CROSSPOINT_CHANNEL2	==>	*/	false,
														/* NTV2CROSSPOINT_INPUT1	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT2	==>	*/	true,
														/* NTV2CROSSPOINT_MATTE		==>	*/	false,
														/* NTV2CROSSPOINT_FGKEY		==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL3	==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL4	==>	*/	false,
														/* NTV2CROSSPOINT_INPUT3	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT4	==>	*/	true,
														/* NTV2CROSSPOINT_CHANNEL5	==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL6	==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL7	==>	*/	false,
														/* NTV2CROSSPOINT_CHANNEL8	==>	*/	false,
														/* NTV2CROSSPOINT_INPUT5	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT6	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT7	==>	*/	true,
														/* NTV2CROSSPOINT_INPUT8	==>	*/	true,
																							false};
#endif	//	!defined (NTV2_DEPRECATE_12_6)

#if !defined (NTV2_DEPRECATE)
bool   CNTV2Card::InitAutoCirculate(NTV2Crosspoint channelSpec,
                                    LWord startFrameNumber, 
                                    LWord endFrameNumber,
                                    bool bWithAudio,
                                    bool bWithRP188,
                                    bool bFbfChange,
                                    bool bFboChange,
									bool bWithColorCorrection ,
									bool bWithVidProc,
	                                bool bWithCustomAncData,
	                                bool bWithLTC,
									bool bWithAudio2)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );
 
	// If ending frame number has been defaulted, calculate it
	if (endFrameNumber < 0) {	
		endFrameNumber = 10;	// All boards have at least 10 frames	
	}

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eInitAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = startFrameNumber;
	autoCircData.lVal2		 = endFrameNumber;
	autoCircData.lVal3		 = bWithAudio2?1:0;
	autoCircData.lVal4		 = 1;
	autoCircData.bVal1		 = bWithAudio;
	autoCircData.bVal2		 = bWithRP188;
    autoCircData.bVal3       = bFbfChange;
    autoCircData.bVal4       = bFboChange;
    autoCircData.bVal5       = bWithColorCorrection;
    autoCircData.bVal6       = bWithVidProc;
    autoCircData.bVal7       = bWithCustomAncData;
    autoCircData.bVal8       = bWithLTC;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}
#endif	//	!defined (NTV2_DEPRECATE)


#if !defined (NTV2_DEPRECATE_12_6)
bool   CNTV2Card::InitAutoCirculate(NTV2Crosspoint channelSpec,
                                    LWord startFrameNumber, 
                                    LWord endFrameNumber,
									LWord channelCount,
									NTV2AudioSystem audioSystem,
                                    bool bWithAudio,
                                    bool bWithRP188,
                                    bool bFbfChange,
                                    bool bFboChange,
									bool bWithColorCorrection ,
									bool bWithVidProc,
	                                bool bWithCustomAncData,
	                                bool bWithLTC)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );
 
	// If ending frame number has been defaulted, calculate it
	if (endFrameNumber < 0) {	
		endFrameNumber = 10;	// All boards have at least 10 frames	
	}

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eInitAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = startFrameNumber;
	autoCircData.lVal2		 = endFrameNumber;
	autoCircData.lVal3		 = audioSystem;
	autoCircData.lVal4		 = channelCount;
	autoCircData.bVal1		 = bWithAudio;
	autoCircData.bVal2		 = bWithRP188;
    autoCircData.bVal3       = bFbfChange;
    autoCircData.bVal4       = bFboChange;
    autoCircData.bVal5       = bWithColorCorrection;
    autoCircData.bVal6       = bWithVidProc;
    autoCircData.bVal7       = bWithCustomAncData;
    autoCircData.bVal8       = bWithLTC;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


bool   CNTV2Card::StartAutoCirculate (NTV2Crosspoint channelSpec, const ULWord64 inStartTime)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

 	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	::memset (&autoCircData, 0, sizeof (AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = inStartTime ? eStartAutoCircAtTime : eStartAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = static_cast <LWord> (inStartTime >> 32);
	autoCircData.lVal2		 = static_cast <LWord> (inStartTime & 0xFFFFFFFF);

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


bool   CNTV2Card::StopAutoCirculate(NTV2Crosspoint channelSpec)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

  	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eStopAutoCirc;
	autoCircData.channelSpec = channelSpec;

	// Call the OS specific method
	bool bRet =  AutoCirculate (autoCircData);

    // Wait to insure driver changes state to DISABLED
    if (bRet)
    {
		const NTV2Channel	channel	(gCrosspointToChannel [channelSpec]);
		bRet = gIsCrosspointInput [channelSpec] ? WaitForInputFieldID (NTV2_FIELD0, channel) : WaitForOutputFieldID (NTV2_FIELD0, channel);
// 		bRet = gIsCrosspointInput [channelSpec] ? WaitForInputVerticalInterrupt (channel) : WaitForOutputVerticalInterrupt (channel);
// 		bRet = gIsCrosspointInput [channelSpec] ? WaitForInputVerticalInterrupt (channel) : WaitForOutputVerticalInterrupt (channel);
		AUTOCIRCULATE_STATUS_STRUCT acStatus;
		memset(&acStatus, 0, sizeof(AUTOCIRCULATE_STATUS_STRUCT));
		GetAutoCirculate(channelSpec, &acStatus );

		if ( acStatus.state != NTV2_AUTOCIRCULATE_DISABLED)
		{
			// something is wrong so abort.
			bRet = AbortAutoCirculate(channelSpec);
		}

    }

    return bRet;
}


bool   CNTV2Card::AbortAutoCirculate(NTV2Crosspoint channelSpec)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

  	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eAbortAutoCirc;
	autoCircData.channelSpec = channelSpec;

	// Call the OS specific method
	bool bRet =  AutoCirculate (autoCircData);

    return bRet;
}


bool   CNTV2Card::PauseAutoCirculate(NTV2Crosspoint channelSpec, bool pPlayToPause /*= true*/)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = ePauseAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.bVal1		 = pPlayToPause;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

bool   CNTV2Card::GetFrameStampEx2 (NTV2Crosspoint channelSpec, ULWord frameNum, 
									FRAME_STAMP_STRUCT* pFrameStamp,
									PAUTOCIRCULATE_TASK_STRUCT pTaskStruct)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eGetFrameStampEx2;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = frameNum;
	autoCircData.pvVal1		 = (PVOID) pFrameStamp;
	autoCircData.pvVal2		 = (PVOID) pTaskStruct;

	// The following is ignored by Windows; it looks at the 
	// channel spec and frame num in the autoCircData instead.
	pFrameStamp->channelSpec = channelSpec;
	pFrameStamp->frame = frameNum;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

bool CNTV2Card::FlushAutoCirculate (NTV2Crosspoint channelSpec)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eFlushAutoCirculate;
	autoCircData.channelSpec = channelSpec;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

bool CNTV2Card::SetActiveFrameAutoCirculate (NTV2Crosspoint channelSpec, ULWord lActiveFrame)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eSetActiveFrame;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1 = lActiveFrame;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

bool CNTV2Card::PrerollAutoCirculate (NTV2Crosspoint channelSpec, ULWord lPrerollFrames)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = ePrerollAutoCirculate;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1 = lPrerollFrames;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}




//  Notes:  This interface (TransferWithAutoCirculate) gives the driver control
//  over advancing the frame # to be transferred.
//  This allows the driver to assign the sequential frame # appropriately,
//  even in the face of dropped frames caused by CPU overload, etc.   
bool CNTV2Card::TransferWithAutoCirculate(PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
                                          PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct)                                        
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

    // Sanity check with new parameter videoOffset
    if (pTransferStruct->videoDmaOffset)
    {
        if (pTransferStruct->videoBufferSize < pTransferStruct->videoDmaOffset)
            return false;
    }
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eTransferAutoCirculate;
	autoCircData.pvVal1		 = (PVOID) pTransferStruct;
	autoCircData.pvVal2		 = (PVOID) pTransferStatusStruct;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

//
// TransferWithAutoCirculateEx
// Same as TransferWithAutoCirculate
// but adds the ability to pass routing register setups for xena2....
// actually you can send over any register setups you want. These
// will be set by the driver at the appropriate frame.
bool CNTV2Card::TransferWithAutoCirculateEx(PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
                                          PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct,
										  NTV2RoutingTable* pXena2RoutingTable)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

    // Sanity check with new parameter videoOffset
    if (pTransferStruct->videoDmaOffset)
    {
        if (pTransferStruct->videoBufferSize < pTransferStruct->videoDmaOffset)
            return false;
    }
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eTransferAutoCirculateEx;
	autoCircData.pvVal1		 = (PVOID) pTransferStruct;
	autoCircData.pvVal2		 = (PVOID) pTransferStatusStruct;
	autoCircData.pvVal3		 = (PVOID) pXena2RoutingTable;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


//
// TransferWithAutoCirculateEx2
// Same as TransferWithAutoCirculate
// adds flexible frame accurate task list
bool CNTV2Card::TransferWithAutoCirculateEx2(PAUTOCIRCULATE_TRANSFER_STRUCT pTransferStruct,
											 PAUTOCIRCULATE_TRANSFER_STATUS_STRUCT pTransferStatusStruct,
											 NTV2RoutingTable* pXena2RoutingTable,
											 PAUTOCIRCULATE_TASK_STRUCT pTaskStruct)                                        
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Sanity check with new parameter videoOffset
	if (pTransferStruct->videoDmaOffset)
	{
		if (pTransferStruct->videoBufferSize < pTransferStruct->videoDmaOffset)
			return false;
	}
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eTransferAutoCirculateEx2;
	autoCircData.pvVal1		 = (PVOID) pTransferStruct;
	autoCircData.pvVal2		 = (PVOID) pTransferStatusStruct;
	autoCircData.pvVal3		 = (PVOID) pXena2RoutingTable;
	autoCircData.pvVal4		 = (PVOID) pTaskStruct;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

//
// SetAutoCirculateCaptureTask
// add frame accurate task list for each captured frame
bool CNTV2Card::SetAutoCirculateCaptureTask(NTV2Crosspoint channelSpec, PAUTOCIRCULATE_TASK_STRUCT pTaskStruct)
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eSetCaptureTask;
	autoCircData.channelSpec = channelSpec;
	autoCircData.pvVal1		 = (PVOID) pTaskStruct;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}
#endif	//	!defined (NTV2_DEPRECATE_12_6)

//GetFrameStamp(NTV2Crosspoint channelSpec, ULONG frameNum, FRAME_STAMP_STRUCT* pFrameStamp)
//When a channelSpec is autocirculating, the ISR or DPC will continously fill in a
// FRAME_STAMP_STRUCT for the frame it is working on.
// The framestamp structure is intended to give enough information to determine if frames
// have been dropped either on input or output. It also allows for synchronization of 
// audio and video by stamping the audioinputaddress at the start and end of a video frame.
bool   CNTV2Card::GetFrameStamp (NTV2Crosspoint channelSpec, ULWord frameNum, FRAME_STAMP_STRUCT* pFrameStamp)
{
	// Insure that the CNTV2Card has been 'open'ed
	if (! _boardOpened )	return false;

	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eGetFrameStamp;
	autoCircData.channelSpec = channelSpec;
	autoCircData.lVal1		 = frameNum;
	autoCircData.pvVal1		 = (PVOID) pFrameStamp;

	// The following is ignored by Windows; it looks at the 
	// channel spec and frame num in the autoCircData instead.
	pFrameStamp->channelSpec = channelSpec;
	pFrameStamp->frame = frameNum;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}

// GetAutoCirculate(NTV2Crosspoint channelSpec,AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus )
// Returns true if communication with the driver was successful.
// Passes back: whether associated channelSpec is currently autocirculating;
//              Frame Range (Start and End); and Current Active Frame.
//
//              Note that Current Active Frame is reliable,
//              whereas reading, for example, the Ch1OutputFrame register is not reliable,
//              because the latest-written value may or may-not have been clocked-in to the hardware.
//              Note also that this value is valid only if bIsCirculating is true.
bool   CNTV2Card::GetAutoCirculate(NTV2Crosspoint channelSpec, AUTOCIRCULATE_STATUS_STRUCT* autoCirculateStatus )
{
	// Insure that the CNTV2Card has been 'open'ed
	assert( _boardOpened );

	// The following is ignored by Windows.
	autoCirculateStatus -> channelSpec = channelSpec;
	
	// Fill in our OS independent data structure 
	AUTOCIRCULATE_DATA autoCircData;
	memset(&autoCircData, 0, sizeof(AUTOCIRCULATE_DATA));
	autoCircData.eCommand	 = eGetAutoCirc;
	autoCircData.channelSpec = channelSpec;
	autoCircData.pvVal1		 = (PVOID) autoCirculateStatus;

	// Call the OS specific method
	return AutoCirculate (autoCircData);
}


bool CNTV2Card::FindUnallocatedFrames (const UByte inFrameCount, LWord & outStartFrame, LWord & outEndFrame)
{
	//	Look for a contiguous group of available frame buffers...
	static const NTV2Crosspoint	gCrosspoints[] = {	NTV2CROSSPOINT_CHANNEL1,	NTV2CROSSPOINT_CHANNEL2,	NTV2CROSSPOINT_CHANNEL3,	NTV2CROSSPOINT_CHANNEL4,
													NTV2CROSSPOINT_CHANNEL5,	NTV2CROSSPOINT_CHANNEL6,	NTV2CROSSPOINT_CHANNEL7,	NTV2CROSSPOINT_CHANNEL8,
													NTV2CROSSPOINT_INPUT1,		NTV2CROSSPOINT_INPUT2,		NTV2CROSSPOINT_INPUT3,		NTV2CROSSPOINT_INPUT4,
													NTV2CROSSPOINT_INPUT5,		NTV2CROSSPOINT_INPUT6,		NTV2CROSSPOINT_INPUT7,		NTV2CROSSPOINT_INPUT8 };
	static const unsigned		nCrosspoints	(sizeof (gCrosspoints) / sizeof (NTV2Crosspoint));

	AUTOCIRCULATE_STATUS_STRUCT			acStatus;
	typedef	std::set <LWord>			LWordSet;
	typedef LWordSet::const_iterator	LWordSetConstIter;
	LWordSet							allocatedFrameNumbers;
	ULWord								isQuadMode1	(0);
	ULWord								isQuadMode5	(0);

	outStartFrame = outEndFrame = 0;
	if (!_boardOpened)
		return false;
	if (!inFrameCount)
		return false;

	GetQuadFrameEnable (isQuadMode1, NTV2_CHANNEL1);
	GetQuadFrameEnable (isQuadMode5, NTV2_CHANNEL5);

	//	Inventory all allocated frames...
	for (unsigned ndx (0);  ndx < nCrosspoints;  ndx++)
	{
		const NTV2Crosspoint	xpt	(gCrosspoints [ndx]);
		::memset (&acStatus, 0, sizeof (acStatus));

		if (GetAutoCirculate (xpt, &acStatus)  &&  acStatus.state != NTV2_AUTOCIRCULATE_DISABLED)
		{
			LWord	endFrameNum	(acStatus.endFrame);

			//	Quadruple the number of allocated frame buffers if quad mode is enabled and this is channel 1 or 5...
			if (isQuadMode1  &&  (xpt == NTV2CROSSPOINT_INPUT1 || xpt == NTV2CROSSPOINT_CHANNEL1))
				endFrameNum = (acStatus.endFrame - acStatus.startFrame + 1) * 4  +  acStatus.startFrame  -  1;
			else if (isQuadMode5  &&  (xpt == NTV2CROSSPOINT_INPUT5 || xpt == NTV2CROSSPOINT_CHANNEL5))
				endFrameNum = (acStatus.endFrame - acStatus.startFrame + 1) * 4  +  acStatus.startFrame  -  1;

			for (LWord num (acStatus.startFrame);  num <= endFrameNum;  num++)
				allocatedFrameNumbers.insert (num);
		}	//	if A/C active
	}	//	for each AutoCirculate crosspoint

	//	Find a contiguous band of unallocated frame numbers...
	const LWord			finalFrameNumber	(::NTV2DeviceGetNumberFrameBuffers (_boardID) - 1);
	LWord				startFrameNumber	(0);
	LWord				endFrameNumber		(startFrameNumber + inFrameCount - 1);
	LWordSetConstIter	iter				(allocatedFrameNumbers.begin ());

	while (iter != allocatedFrameNumbers.end ())
	{
		LWord	allocatedStartFrame	(*iter);
		LWord	allocatedEndFrame	(allocatedStartFrame);

		//	Find the end of this allocated band...
		while (++iter != allocatedFrameNumbers.end ()  &&  *iter == (allocatedEndFrame + 1))
			allocatedEndFrame = *iter;

		if (startFrameNumber < allocatedStartFrame  &&  endFrameNumber < allocatedStartFrame)
			break;	//	Found a free block!

		//	Not large enough -- move past this allocated block...
		startFrameNumber = allocatedEndFrame + 1;
		endFrameNumber = startFrameNumber + inFrameCount - 1;
	}

	if (startFrameNumber > finalFrameNumber || endFrameNumber > finalFrameNumber)
		return false;

	outStartFrame = startFrameNumber;
	outEndFrame = endFrameNumber;
	return true;

}	//	FindUnallocatedFrames


//	Handy function to fetch the NTV2Crosspoint for a given NTV2Channel that works with both pre & post 12.3 drivers.
//	NOTE:  This relies on the channel's NTV2Mode being correct and aligned with the driver's NTV2Crosspoint!
static bool GetCurrentACChannelCrosspoint (CNTV2Card & inDevice, const NTV2Channel inChannel, NTV2Crosspoint & outCrosspoint)
{
	NTV2Mode	mode	(NTV2_MODE_DISPLAY);
	outCrosspoint = NTV2CROSSPOINT_INVALID;
	if (!inDevice.IsOpen ())
		return false;
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;

	if (!inDevice.GetMode (inChannel, mode))
		return false;
	outCrosspoint = (mode == NTV2_MODE_DISPLAY) ? ::NTV2ChannelToOutputCrosspoint (inChannel) : ::NTV2ChannelToInputCrosspoint (inChannel);
	return true;
}


bool CNTV2Card::AutoCirculateInitForInput (	const NTV2Channel		inChannel,
											const UByte				inFrameCount,
											const NTV2AudioSystem	inAudioSystem,
											const ULWord			inOptionFlags,
											const UByte				inNumChannels,
											const UByte				inStartFrameNumber,
											const UByte				inEndFrameNumber)
{
	LWord	startFrameNumber(0);
	LWord	endFrameNumber	(0);

	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;	//	Must be valid channel
	if (inNumChannels == 0)
		return false;	//	At least one channel

	if ((inEndFrameNumber - inStartFrameNumber) != 0)
	{
		startFrameNumber = inStartFrameNumber;
		endFrameNumber = inEndFrameNumber;
	}
	else if (!FindUnallocatedFrames (inFrameCount, startFrameNumber, endFrameNumber))
		return false;

	//	Fill in our OS independent data structure...
	AUTOCIRCULATE_DATA	autoCircData	(eInitAutoCirc);
	autoCircData.channelSpec = ::NTV2ChannelToInputChannelSpec (inChannel);
	autoCircData.lVal1 = startFrameNumber;
	autoCircData.lVal2 = endFrameNumber;
	autoCircData.lVal3 = inAudioSystem;
	autoCircData.lVal4 = inNumChannels;
	if ((inOptionFlags & AUTOCIRCULATE_WITH_AUDIO_CONTROL) != 0)
		autoCircData.bVal1 = false;
	else
		autoCircData.bVal1 = NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem) ? true : false;
	autoCircData.bVal2 = inOptionFlags & AUTOCIRCULATE_WITH_RP188 ? true : false;
	autoCircData.bVal3 = inOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE ? true : false;
	autoCircData.bVal4 = inOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE ? true : false;
	autoCircData.bVal5 = inOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT ? true : false;
	autoCircData.bVal6 = inOptionFlags & AUTOCIRCULATE_WITH_VIDPROC ? true : false;
	autoCircData.bVal7 = inOptionFlags & AUTOCIRCULATE_WITH_ANC ? true : false;
	autoCircData.bVal8 = inOptionFlags & AUTOCIRCULATE_WITH_LTC ? true : false;

	return AutoCirculate (autoCircData);	//	Call the OS-specific method

}	//	AutoCirculateInitForInput


bool CNTV2Card::AutoCirculateInitForOutput (const NTV2Channel		inChannel,
											const UByte				inFrameCount,
											const NTV2AudioSystem	inAudioSystem,
											const ULWord			inOptionFlags,
											const UByte				inNumChannels,
											const UByte				inStartFrameNumber,
											const UByte				inEndFrameNumber)
{
	LWord	startFrameNumber(0);
	LWord	endFrameNumber	(0);

	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;	//	Must be valid channel
	if (inNumChannels == 0)
		return false;	//	At least one channel

	if ((inEndFrameNumber - inStartFrameNumber) != 0)
	{
		startFrameNumber = inStartFrameNumber;
		endFrameNumber = inEndFrameNumber;
	}
	else if (!FindUnallocatedFrames (inFrameCount, startFrameNumber, endFrameNumber))
		return false;

	//	Fill in our OS independent data structure...
	AUTOCIRCULATE_DATA	autoCircData	(eInitAutoCirc);
	autoCircData.channelSpec = NTV2ChannelToOutputChannelSpec (inChannel);
	autoCircData.lVal1 = startFrameNumber;
	autoCircData.lVal2 = endFrameNumber;
	autoCircData.lVal3 = inAudioSystem;
	autoCircData.lVal4 = inNumChannels;
	if ((inOptionFlags & AUTOCIRCULATE_WITH_AUDIO_CONTROL) != 0)
		autoCircData.bVal1 = false;
	else
		autoCircData.bVal1 = NTV2_IS_VALID_AUDIO_SYSTEM (inAudioSystem) ? true : false;
	autoCircData.bVal2 = ((inOptionFlags & AUTOCIRCULATE_WITH_RP188) != 0) ? true : false;
	autoCircData.bVal3 = ((inOptionFlags & AUTOCIRCULATE_WITH_FBFCHANGE) != 0) ? true : false;
	autoCircData.bVal4 = ((inOptionFlags & AUTOCIRCULATE_WITH_FBOCHANGE) != 0) ? true : false;
	autoCircData.bVal5 = ((inOptionFlags & AUTOCIRCULATE_WITH_COLORCORRECT) != 0) ? true : false;
	autoCircData.bVal6 = ((inOptionFlags & AUTOCIRCULATE_WITH_VIDPROC) != 0) ? true : false;
	autoCircData.bVal7 = ((inOptionFlags & AUTOCIRCULATE_WITH_ANC) != 0) ? true : false;
	autoCircData.bVal8 = ((inOptionFlags & AUTOCIRCULATE_WITH_LTC) != 0) ? true : false;

	return AutoCirculate (autoCircData);	//	Call the OS-specific method

}	//	AutoCirculateInitForOutput


bool CNTV2Card::AutoCirculateStart (const NTV2Channel inChannel, const ULWord64 inStartTime)
{
	AUTOCIRCULATE_DATA	autoCircData	(inStartTime ? eStartAutoCircAtTime : eStartAutoCirc);
	autoCircData.lVal1 = static_cast <LWord> (inStartTime >> 32);
	autoCircData.lVal2 = static_cast <LWord> (inStartTime & 0xFFFFFFFF);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	return AutoCirculate (autoCircData);
}


bool CNTV2Card::AutoCirculateStop (const NTV2Channel inChannel, const bool inAbort)
{
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;

	const AUTO_CIRC_COMMAND	acCommand	(inAbort ? eAbortAutoCirc : eStopAutoCirc);
	AUTOCIRCULATE_DATA		stopInput	(acCommand,	::NTV2ChannelToInputCrosspoint (inChannel));
	AUTOCIRCULATE_DATA		stopOutput	(acCommand,	::NTV2ChannelToOutputCrosspoint (inChannel));
	NTV2Mode				mode		(NTV2_MODE_INVALID);
	AUTOCIRCULATE_STATUS	acStatus;

	//	Stop input or output A/C using the old driver call...
	const bool	stopInputFailed		(!AutoCirculate (stopInput));
	const bool	stopOutputFailed	(!AutoCirculate (stopOutput));
	if (stopInputFailed && stopOutputFailed)
		return false;	//	Both failed
	if (inAbort)
		return true;	//	In abort case, no more to do!

	//	Wait until driver changes AC state to DISABLED...
	bool result = GetMode (inChannel, mode);
	if (NTV2_IS_INPUT_MODE (mode))		WaitForInputFieldID (NTV2_FIELD0, inChannel);
	if (NTV2_IS_OUTPUT_MODE (mode))		WaitForOutputFieldID (NTV2_FIELD0, inChannel);
	if (AutoCirculateGetStatus (inChannel, acStatus)  &&  acStatus.acState != NTV2_AUTOCIRCULATE_DISABLED)
		return AutoCirculateStop (inChannel, true);	//	something's wrong -- abort (WARNING: RECURSIVE CALL!)
	return result;

}	//	AutoCirculateStop


bool CNTV2Card::AutoCirculatePause (const NTV2Channel inChannel)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData (ePauseAutoCirc);
	autoCircData.bVal1		= false;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	return AutoCirculate (autoCircData);

}	//	AutoCirculatePause


bool CNTV2Card::AutoCirculateResume (const NTV2Channel inChannel, const bool inClearDropCount)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData (ePauseAutoCirc);
	autoCircData.bVal1 = true;
	autoCircData.bVal2 = inClearDropCount;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	return AutoCirculate (autoCircData);

}	//	AutoCirculateResume


bool CNTV2Card::AutoCirculateFlush (const NTV2Channel inChannel, const bool inClearDropCount)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(eFlushAutoCirculate);
    autoCircData.bVal1 = inClearDropCount;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	return AutoCirculate (autoCircData);

}	//	AutoCirculateFlush


bool CNTV2Card::AutoCirculatePreRoll (const NTV2Channel inChannel, const ULWord inPreRollFrames)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(ePrerollAutoCirculate);
	autoCircData.lVal1 = inPreRollFrames;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	return AutoCirculate (autoCircData);

}	//	AutoCirculatePreRoll


bool CNTV2Card::AutoCirculateGetStatus (const NTV2Channel inChannel, AUTOCIRCULATE_STATUS & outStatus)
{
	outStatus.Clear ();
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, outStatus.acCrosspoint))
		return false;

	if (!NTV2_IS_VALID_NTV2CROSSPOINT (outStatus.acCrosspoint))
	{
		AUTOCIRCULATE_STATUS	notRunningStatus (::NTV2ChannelToOutputCrosspoint (inChannel));
		outStatus = notRunningStatus;
		return true;	//	AutoCirculate not running on this channel
	}

#if defined (NTV2_NUB_CLIENT_SUPPORT)
	// Auto circulate status is not implemented in the NUB yet
	if (_remoteHandle != INVALID_NUB_HANDLE)
		return false;
#endif	//	defined (NTV2_NUB_CLIENT_SUPPORT)
	if (!NTV2Message (reinterpret_cast <NTV2_HEADER *> (&outStatus)))
	{
		//	Try it the old way...
		AUTOCIRCULATE_STATUS_STRUCT	tmpStruct;
		::memset (&tmpStruct, 0, sizeof (tmpStruct));

		tmpStruct.channelSpec = ::NTV2ChannelToInputCrosspoint (inChannel);
		GetAutoCirculate (tmpStruct.channelSpec, &tmpStruct);
		if (tmpStruct.state != NTV2_AUTOCIRCULATE_DISABLED)
			return outStatus.CopyFrom (tmpStruct);

		tmpStruct.channelSpec = ::NTV2ChannelToOutputCrosspoint (inChannel);
		GetAutoCirculate (tmpStruct.channelSpec, &tmpStruct);
		if (tmpStruct.state != NTV2_AUTOCIRCULATE_DISABLED)
			return outStatus.CopyFrom (tmpStruct);

		AUTOCIRCULATE_STATUS	notRunning (::NTV2ChannelToOutputCrosspoint (inChannel));
		outStatus = notRunning;
		return true;	//	AutoCirculate not running on this channel
	}
	return true;

}	//	AutoCirculateGetStatus


bool CNTV2Card::AutoCirculateGetFrameStamp (const NTV2Channel inChannel, const ULWord inFrameNum, FRAME_STAMP & outFrameStamp)
{
	//	Try the new driver call first...
	outFrameStamp.acFrameTime = LWord64 (inChannel);
	outFrameStamp.acRequestedFrame = inFrameNum;
	if (!NTV2Message (reinterpret_cast <NTV2_HEADER *> (&outFrameStamp)))
	{
		//	Fail -- try it the old way...
		NTV2Crosspoint		crosspoint	(NTV2CROSSPOINT_INVALID);
		if (!GetCurrentACChannelCrosspoint (*this, inChannel, crosspoint))
			return false;

		//	Use the old A/C driver call...
		FRAME_STAMP_STRUCT	oldFrameStampStruct;
		return GetFrameStamp (crosspoint, inFrameNum, &oldFrameStampStruct)  &&  outFrameStamp.SetFrom (oldFrameStampStruct);
	}
	return true;

}	//	AutoCirculateGetFrameStamp


bool CNTV2Card::AutoCirculateSetActiveFrame (const NTV2Channel inChannel, const ULWord inNewActiveFrame)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(eSetActiveFrame);
	autoCircData.lVal1 = inNewActiveFrame;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	return AutoCirculate (autoCircData);

}	//	AutoCirculateSetActiveFrame


bool CNTV2Card::AutoCirculateTransfer (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo)
{
	if (!_boardOpened)
		return false;
	#if defined (_DEBUG)
		NTV2_ASSERT (inOutXferInfo.NTV2_IS_STRUCT_VALID ());
	#endif

	NTV2Crosspoint			crosspoint	(NTV2CROSSPOINT_INVALID);
	NTV2EveryFrameTaskMode	taskMode	(NTV2_OEM_TASKS);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, crosspoint))
		return false;
	if (!NTV2_IS_VALID_NTV2CROSSPOINT (crosspoint))
		return false;
	GetEveryFrameServices (taskMode);

	if (NTV2_IS_INPUT_CROSSPOINT (crosspoint))
		inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes.Fill (UByte (0xFF));	//	Invalidate old timecodes
	else if (NTV2_IS_OUTPUT_CROSSPOINT (crosspoint))
	{
		if (inOutXferInfo.acRP188.IsValid ())
			inOutXferInfo.SetAllOutputTimeCodes (inOutXferInfo.acRP188);

		const NTV2_RP188 *	pArray	(reinterpret_cast <const NTV2_RP188 *> (inOutXferInfo.acOutputTimeCodes.GetHostPointer ()));
		if (pArray  &&  pArray [NTV2_TCINDEX_DEFAULT].IsValid ())
			inOutXferInfo.SetAllOutputTimeCodes (pArray [NTV2_TCINDEX_DEFAULT]);
	}

	//	Use the new A/C driver call...
	bool	result	(false);
	inOutXferInfo.acCrosspoint = crosspoint;
	result = NTV2Message (reinterpret_cast <NTV2_HEADER *> (&inOutXferInfo));
	if (result  &&  taskMode == NTV2_STANDARD_TASKS  &&  NTV2_IS_INPUT_CROSSPOINT (crosspoint))
	{
		//	Hack for retail mode capture
		ULWord	inputSelect	(NTV2_Input1Select);
		ReadRegister (kVRegInputSelect, &inputSelect);
		if (inputSelect == NTV2_Input2Select)
		{
			//	Copy input 2 tc into default location
			RP188SourceSelect TimecodeSource;
			ReadRegister(kVRegRP188SourceSelect, (ULWord*)&TimecodeSource);
			NTV2TCIndex TimecodeIndex = NTV2_TCINDEX_DEFAULT;
			switch (TimecodeSource)
			{
				case kRP188SourceEmbeddedLTC:		TimecodeIndex = NTV2_TCINDEX_SDI2_LTC;		break;
				case kRP188SourceEmbeddedVITC1:		TimecodeIndex = NTV2_TCINDEX_SDI2;			break;
				case kRP188SourceEmbeddedVITC2:		TimecodeIndex = NTV2_TCINDEX_SDI2_2;		break;
				case kRP188SourceLTCPort:			TimecodeIndex = NTV2_TCINDEX_LTC1;			break;
				default:							TimecodeIndex = NTV2_TCINDEX_SDI2_LTC;		break;
			}
			NTV2_RP188	tcValue;
			inOutXferInfo.GetInputTimeCode(tcValue, TimecodeIndex);
			NTV2_RP188 *	pArray	(reinterpret_cast <NTV2_RP188 *> (inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes.GetHostPointer()));
			if (pArray)
				pArray [NTV2_TCINDEX_DEFAULT] = tcValue;
		}
		else
		{
			//	Copy input 2 tc into input 1...
			RP188SourceSelect TimecodeSource;
			ReadRegister(kVRegRP188SourceSelect, (ULWord*)&TimecodeSource);
			NTV2TCIndex TimecodeIndex = NTV2_TCINDEX_DEFAULT;
			switch (TimecodeSource)
			{
				case kRP188SourceEmbeddedLTC:		TimecodeIndex = NTV2_TCINDEX_SDI1_LTC;		break;
				case kRP188SourceEmbeddedVITC1:		TimecodeIndex = NTV2_TCINDEX_SDI1;			break;
				case kRP188SourceEmbeddedVITC2:		TimecodeIndex = NTV2_TCINDEX_SDI1_2;		break;
				case kRP188SourceLTCPort:			TimecodeIndex = NTV2_TCINDEX_LTC1;			break;
				default:							TimecodeIndex = NTV2_TCINDEX_SDI1_LTC;		break;
			}
			NTV2_RP188	tcValue;
			inOutXferInfo.GetInputTimeCode(tcValue, TimecodeIndex);
			NTV2_RP188 *	pArray(reinterpret_cast <NTV2_RP188 *> (inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes.GetHostPointer()));
			if (pArray)
				pArray[NTV2_TCINDEX_DEFAULT] = tcValue;
		}
	}	//	if NTV2Message OK && retail mode && capturing
	#if defined (AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER)
		if (result  &&  NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		{
			ULWord	doZeroing	(0);
			if (ReadRegister(kVRegZeroDeviceAncPostCapture, &doZeroing)  &&  doZeroing)
			{	//	Zero out the Anc buffer on the device...
				static NTV2_POINTER	gClearDeviceAncBuffer;
				const LWord		xferFrame	(inOutXferInfo.GetTransferFrameNumber());
				ULWord			ancOffsetF1	(0);
				ULWord			ancOffsetF2	(0);
				NTV2Framesize	fbSize		(NTV2_FRAMESIZE_INVALID);
				ReadRegister(kVRegAncField1Offset, &ancOffsetF1);
				ReadRegister(kVRegAncField2Offset, &ancOffsetF2);
				GetFrameBufferSize(inChannel, fbSize);
				const ULWord	fbByteCount	(::NTV2FramesizeToByteCount(fbSize));
				const ULWord	ancOffset	(ancOffsetF2 > ancOffsetF1  ?  ancOffsetF2  :  ancOffsetF1);	//	Use whichever is larger
				NTV2_ASSERT (xferFrame != -1);
				if (gClearDeviceAncBuffer.IsNULL() || (gClearDeviceAncBuffer.GetByteCount() != ancOffset))
				{
					gClearDeviceAncBuffer.Allocate(ancOffset);	//	Allocate it
					gClearDeviceAncBuffer.Fill (ULWord(0));		//	Clear it
				}
				if (xferFrame != -1  &&  fbByteCount  &&  !gClearDeviceAncBuffer.IsNULL())
					DMAWriteSegments (xferFrame,
									(ULWord *) gClearDeviceAncBuffer.GetHostPointer(),	//	host buffer
									fbByteCount - ancOffset,				//	device memory offset, in bytes
									gClearDeviceAncBuffer.GetByteCount(),	//	total number of bytes to xfer
									1,										//	numSegments -- one chunk of 'ancOffset'
									gClearDeviceAncBuffer.GetByteCount(),	//	segmentHostPitch
									gClearDeviceAncBuffer.GetByteCount());	//	segmentCardPitch
			}
		}
	#endif	//	AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER
	#if defined (AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER)
		if (result  &&  NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		{
			ULWord	doZeroing	(0);
			if (ReadRegister(kVRegZeroHostAncPostCapture, &doZeroing)  &&  doZeroing)
			{	//	Zero out everything past the last captured Anc byte in the client's host buffer(s)... 
				NTV2_POINTER &	clientAncBufferF1	(inOutXferInfo.acANCBuffer);
				NTV2_POINTER &	clientAncBufferF2	(inOutXferInfo.acANCField2Buffer);
				const ULWord	ancF1ByteCount		(inOutXferInfo.GetAncByteCount(false));
				const ULWord	ancF2ByteCount		(inOutXferInfo.GetAncByteCount(true));
				void *			pF1TailEnd			(clientAncBufferF1.GetHostAddress(ancF1ByteCount));
				void *			pF2TailEnd			(clientAncBufferF2.GetHostAddress(ancF2ByteCount));
				if (pF1TailEnd  &&  clientAncBufferF1.GetByteCount() > ancF1ByteCount)
					::memset (pF1TailEnd, 0, clientAncBufferF1.GetByteCount() - ancF1ByteCount);
				if (pF2TailEnd  &&  clientAncBufferF2.GetByteCount() > ancF2ByteCount)
					::memset (pF2TailEnd, 0, clientAncBufferF2.GetByteCount() - ancF2ByteCount);
			}
		}
	#endif	//	AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER
	return result;

}	//	AutoCirculateTransfer
