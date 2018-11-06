/**
	@file		ntv2autocirculate.cpp
	@brief		Implements the CNTV2Card AutoCirculate API functions.
	@copyright	(C) 2004-2018 AJA Video Systems, Inc.	Proprietary and confidential information.
**/

#include "ntv2card.h"
#include "ntv2devicefeatures.h"
#include "ntv2utils.h"
#include "ajabase/system/lock.h"
#include "ajabase/system/debug.h"
#include "ajaanc/includes/ancillarylist.h"
#include "ajaanc/includes/ancillarydata_timecode_atc.h"
#include "ajabase/common/timecode.h"
#include <iomanip>
#include <assert.h>


using namespace std;


#if defined(_DEBUG)
	//	Debug builds can clear Anc buffers during A/C capture
	#define	AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER		//	Requires non-zero kVRegZeroDeviceAncPostCapture
	#define AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER	//	Requires non-zero kVRegZeroHostAncPostCapture
#endif	//	_DEBUG

//	Logging Macros
#define ACINSTP(_p_)		" inst=" << HEX16(uint64_t(_p_))
#define ACTHIS				ACINSTP(this)

#define	ACFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_AutoCirculate, AJAFUNC << ": " << __x__)
#define	ACWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_AutoCirculate, AJAFUNC << ": " << __x__)
#define	ACNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_AutoCirculate, AJAFUNC << ": " << __x__)
#define	ACINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_AutoCirculate, AJAFUNC << ": " << __x__)
#define	ACDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_AutoCirculate, AJAFUNC << ": " << __x__)


static const char	gFBAllocLockName[]	=	"com.aja.ntv2.mutex.FBAlloc";
static AJALock		gFBAllocLock(gFBAllocLockName);	//	New in SDK 15:	Global mutex to avoid device frame buffer allocation race condition


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
	AUTOCIRCULATE_STATUS			acStatus;
	typedef	std::set <uint16_t>		U16Set;
	typedef U16Set::const_iterator	U16SetConstIter;
	U16Set							allocatedFrameNumbers;
	ULWord							isQuadMode1	(0);
	ULWord							isQuadMode5	(0);

	//	Look for a contiguous group of available frame buffers...
	outStartFrame = outEndFrame = 0;
	if (!_boardOpened)
		return false;
	if (!inFrameCount)
		return false;

	GetQuadFrameEnable (isQuadMode1, NTV2_CHANNEL1);
	GetQuadFrameEnable (isQuadMode5, NTV2_CHANNEL5);

	//	Inventory all allocated frames...
	for (NTV2Channel chan(NTV2_CHANNEL1);  chan < NTV2_MAX_NUM_CHANNELS;  chan = NTV2Channel(chan+1))
		if (AutoCirculateGetStatus(chan, acStatus)  &&  !acStatus.IsStopped())
		{
			uint16_t	endFrameNum	(acStatus.GetEndFrame());

			//	Quadruple the number of allocated frame buffers if quad mode is enabled and this is channel 1 or 5...
			if (isQuadMode1  &&  chan == NTV2_CHANNEL1)
				endFrameNum = acStatus.GetFrameCount() * 4  +  acStatus.GetStartFrame()  -  1;
			else if (isQuadMode5  &&  chan == NTV2_CHANNEL5)
				endFrameNum = acStatus.GetFrameCount() * 4  +  acStatus.GetStartFrame()  -  1;

			for (uint16_t num(acStatus.GetStartFrame());  num <= endFrameNum;  num++)
				allocatedFrameNumbers.insert(num);
		}	//	if A/C active

	//	Find a contiguous band of unallocated frame numbers...
	const uint16_t		finalFrameNumber	(::NTV2DeviceGetNumberFrameBuffers(_boardID) - 1);
	uint16_t			startFrameNumber	(0);
	uint16_t			endFrameNumber		(startFrameNumber + inFrameCount - 1);
	U16SetConstIter		iter				(allocatedFrameNumbers.begin());

	while (iter != allocatedFrameNumbers.end())
	{
		uint16_t	allocatedStartFrame	(*iter);
		uint16_t	allocatedEndFrame	(allocatedStartFrame);

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
	{
		ACFAIL("Cannot find " << DEC(inFrameCount) << " unallocated frames");
		return false;
	}

	outStartFrame = LWord(startFrameNumber);
	outEndFrame = LWord(endFrameNumber);
	ACDBG("Found unused " << DEC(UWord(inFrameCount)) << "-frame block (frames " << DEC(outStartFrame) << "-" << DEC(outEndFrame) << ")");
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
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;	//	Must be valid channel
	if (inNumChannels == 0)
		return false;	//	At least one channel
	if (!gFBAllocLock.IsValid())
		return false;	//	Mutex not ready

	AJAAutoLock	autoLock (&gFBAllocLock);	//	Avoid AutoCirculate buffer collisions
	LWord	startFrameNumber(static_cast <LWord>(inStartFrameNumber));
	LWord	endFrameNumber	(static_cast <LWord>(inEndFrameNumber));
	if (inFrameCount)
	{
		if (inEndFrameNumber - inStartFrameNumber)
			ACWARN ("Start and End frame numbers (" << DEC(startFrameNumber) << ", " << DEC(endFrameNumber)
					<< ") specified with non-zero frame count (" << DEC(uint16_t(inFrameCount)) << ")");

		if (!FindUnallocatedFrames (inFrameCount, startFrameNumber, endFrameNumber))
			return false;
	}
	else if (endFrameNumber == 0  &&  startFrameNumber == 0)
	{
		ACFAIL ("Zero frames requested");
		return false;	//	inFrameCount zero
	}

	if (endFrameNumber < startFrameNumber)
	{
		ACFAIL ("End frame (" << DEC(endFrameNumber) << ") < Start frame (" << DEC(startFrameNumber) << ")");
		return false;	//	endFrame must be > startFrame
	}
	if ((endFrameNumber - startFrameNumber + 1) < 2)
	{
		ACFAIL ("Frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber) << " < 2 frames");
		return false;	//	must be at least 2 frames
	}

	//	Warn about interference from other channels...
	for (UWord chan(0);  chan < ::NTV2DeviceGetNumFrameStores(_boardID);  chan++)
	{
		ULWord		frameNum(0);
		NTV2Mode	mode(NTV2_MODE_INVALID);
		bool		isEnabled(false);
		if (inChannel != chan)
			if (IsChannelEnabled(NTV2Channel(chan), isEnabled)  &&  isEnabled)		//	FrameStore is enabled
				if (GetMode(NTV2Channel(chan), mode)  &&  mode == NTV2_MODE_INPUT)	//	Channel is capturing
					if (GetInputFrame(NTV2Channel(chan), frameNum))
						if (frameNum >= ULWord(startFrameNumber)  &&  frameNum <= ULWord(endFrameNumber))	//	Frame in range
							ACWARN ("FrameStore " << DEC(chan+1) << " is writing frame " << DEC(frameNum)
									<< " -- will corrupt AutoCirculate channel " << DEC(inChannel+1) << " input frames "
									<< DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	}

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

	const bool result (AutoCirculate(autoCircData));	//	Call the OS-specific method
	if (result)
		ACINFO("Channel " << DEC(inChannel+1) << " initialized using frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	else
		ACFAIL("Channel " << DEC(inChannel+1) << " initialization failed");
	return result;

}	//	AutoCirculateInitForInput


bool CNTV2Card::AutoCirculateInitForOutput (const NTV2Channel		inChannel,
											const UByte				inFrameCount,
											const NTV2AudioSystem	inAudioSystem,
											const ULWord			inOptionFlags,
											const UByte				inNumChannels,
											const UByte				inStartFrameNumber,
											const UByte				inEndFrameNumber)
{
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;	//	Must be valid channel
	if (inNumChannels == 0)
		return false;	//	At least one channel
	if (!gFBAllocLock.IsValid())
		return false;	//	Mutex not ready

	AJAAutoLock	autoLock (&gFBAllocLock);	//	Avoid AutoCirculate buffer collisions
	LWord	startFrameNumber(static_cast <LWord>(inStartFrameNumber));
	LWord	endFrameNumber	(static_cast <LWord>(inEndFrameNumber));
	if (inFrameCount)
	{
		if (inEndFrameNumber - inStartFrameNumber)
			ACWARN ("Start and End frame numbers (" << DEC(startFrameNumber) << ", " << DEC(endFrameNumber)
					<< ") specified with non-zero frame count (" << DEC(uint16_t(inFrameCount)) << ")");

		if (!FindUnallocatedFrames (inFrameCount, startFrameNumber, endFrameNumber))
			return false;
	}
	else if (endFrameNumber == 0  &&  startFrameNumber == 0)
	{
		ACFAIL ("Zero frames requested");
		return false;	//	inFrameCount zero
	}

	if (endFrameNumber < startFrameNumber)
	{
		ACFAIL ("End frame (" << DEC(endFrameNumber) << ") < Start frame (" << DEC(startFrameNumber) << ")");
		return false;	//	endFrame must be > startFrame
	}
	if ((endFrameNumber - startFrameNumber + 1) < 2)
	{
		ACFAIL ("Frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber) << " < 2 frames");
		return false;	//	must be at least 2 frames
	}

	for (UWord chan(0);  chan < ::NTV2DeviceGetNumFrameStores(_boardID);  chan++)
	{
		ULWord		frameNum(0);
		NTV2Mode	mode(NTV2_MODE_INVALID);
		bool		isEnabled(false);
		if (inChannel != chan)
			if (IsChannelEnabled(NTV2Channel(chan), isEnabled)  &&  isEnabled)		//	FrameStore is enabled
				if (GetMode(NTV2Channel(chan), mode)  &&  mode == NTV2_MODE_INPUT)	//	Channel is capturing
					if (GetInputFrame(NTV2Channel(chan), frameNum))
						if (frameNum >= ULWord(startFrameNumber)  &&  frameNum <= ULWord(endFrameNumber))	//	Frame in range
							ACWARN("FrameStore " << DEC(chan+1) << " is writing frame " << DEC(frameNum)
								<< " -- will corrupt AutoCirculate channel " << DEC(inChannel+1) << " output frames "
								<< DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	}

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

	const bool result (AutoCirculate(autoCircData));	//	Call the OS-specific method
	if (result)
		ACINFO("Channel " << DEC(inChannel+1) << " initialized using frames " << DEC(startFrameNumber) << "-" << DEC(endFrameNumber));
	else
		ACFAIL("Channel " << DEC(inChannel+1) << " initialization failed");
	return result;

}	//	AutoCirculateInitForOutput


bool CNTV2Card::AutoCirculateStart (const NTV2Channel inChannel, const ULWord64 inStartTime)
{
	AUTOCIRCULATE_DATA	autoCircData	(inStartTime ? eStartAutoCircAtTime : eStartAutoCirc);
	autoCircData.lVal1 = static_cast <LWord> (inStartTime >> 32);
	autoCircData.lVal2 = static_cast <LWord> (inStartTime & 0xFFFFFFFF);
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;
	const bool result (AutoCirculate(autoCircData));
	if (result)
		ACINFO("Started channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to start channel " << DEC(inChannel+1));
	return result;
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
	{
		ACFAIL("Failed to stop channel " << DEC(inChannel+1));
		return false;	//	Both failed
	}
	if (inAbort)
	{
		ACINFO("Aborted channel " << DEC(inChannel+1));
		return true;	//	In abort case, no more to do!
	}

	//	Wait until driver changes AC state to DISABLED...
	bool result (GetMode(inChannel, mode));
	if (NTV2_IS_INPUT_MODE(mode))
		WaitForInputFieldID(NTV2_FIELD0, inChannel);
	if (NTV2_IS_OUTPUT_MODE(mode))
		WaitForOutputFieldID(NTV2_FIELD0, inChannel);
	if (AutoCirculateGetStatus(inChannel, acStatus)  &&  acStatus.acState != NTV2_AUTOCIRCULATE_DISABLED)
	{
		ACWARN("Failed to stop channel " << DEC(inChannel+1) << " -- retrying with ABORT");
		return AutoCirculateStop(inChannel, true);	//	something's wrong -- abort (WARNING: RECURSIVE CALL!)
	}
	ACINFO("Stopped channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateStop


bool CNTV2Card::AutoCirculatePause (const NTV2Channel inChannel)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData (ePauseAutoCirc);
	autoCircData.bVal1		= false;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Paused channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to pause channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculatePause


bool CNTV2Card::AutoCirculateResume (const NTV2Channel inChannel, const bool inClearDropCount)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData (ePauseAutoCirc);
	autoCircData.bVal1 = true;
	autoCircData.bVal2 = inClearDropCount;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Resumed channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to resume channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateResume


bool CNTV2Card::AutoCirculateFlush (const NTV2Channel inChannel, const bool inClearDropCount)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(eFlushAutoCirculate);
    autoCircData.bVal1 = inClearDropCount;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Flushed channel " << DEC(inChannel+1) << ", " << (inClearDropCount?"cleared":"retained") << " drop count");
	else
		ACFAIL("Failed to flush channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateFlush


bool CNTV2Card::AutoCirculatePreRoll (const NTV2Channel inChannel, const ULWord inPreRollFrames)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(ePrerollAutoCirculate);
	autoCircData.lVal1 = inPreRollFrames;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Prerolled " << DEC(inPreRollFrames) << " frame(s) on channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to preroll " << DEC(inPreRollFrames) << " frame(s) on channel " << DEC(inChannel+1));
	return result;

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
	const bool result(NTV2Message(reinterpret_cast<NTV2_HEADER *>(&outStatus)));
	if (result)
		ACDBG("GetStatus successful on channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to get status on channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateGetStatus


bool CNTV2Card::AutoCirculateGetFrameStamp (const NTV2Channel inChannel, const ULWord inFrameNum, FRAME_STAMP & outFrameStamp)
{
	//	Use the new driver call...
	outFrameStamp.acFrameTime = LWord64 (inChannel);
	outFrameStamp.acRequestedFrame = inFrameNum;
	return NTV2Message (reinterpret_cast <NTV2_HEADER *> (&outFrameStamp));

}	//	AutoCirculateGetFrameStamp


bool CNTV2Card::AutoCirculateSetActiveFrame (const NTV2Channel inChannel, const ULWord inNewActiveFrame)
{
	//	Use the old A/C driver call...
	AUTOCIRCULATE_DATA	autoCircData	(eSetActiveFrame);
	autoCircData.lVal1 = inNewActiveFrame;
	if (!GetCurrentACChannelCrosspoint (*this, inChannel, autoCircData.channelSpec))
		return false;

	const bool result(AutoCirculate(autoCircData));
	if (result)
		ACINFO("Set active frame to " << DEC(inNewActiveFrame) << " on channel " << DEC(inChannel+1));
	else
		ACFAIL("Failed to set active frame to " << DEC(inNewActiveFrame) << " on channel " << DEC(inChannel+1));
	return result;

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

	if (NTV2DeviceCanDo2110(_boardID))
		if (NTV2_IS_OUTPUT_CROSSPOINT(crosspoint)  &&  inOutXferInfo.acOutputTimeCodes  &&  (inOutXferInfo.GetAncBuffer(true) || inOutXferInfo.GetAncBuffer(false)))
			S2110AddTimecodesToAncBuffers(inChannel, inOutXferInfo);

	/////////////////////////////////////////////////////////////////////////////
		//	Call the driver...
		bool	result	(false);
		inOutXferInfo.acCrosspoint = crosspoint;
		result = NTV2Message (reinterpret_cast <NTV2_HEADER *> (&inOutXferInfo));
	/////////////////////////////////////////////////////////////////////////////

	if (result  &&  taskMode == NTV2_STANDARD_TASKS  &&  NTV2_IS_INPUT_CROSSPOINT (crosspoint))
	{
		//	After 12.? shipped, we discovered problems with timecode capture in our classic retail stuff.
		//	The acTimeCodes[NTV2_TCINDEX_DEFAULT] was coming up empty.
		//	Rather than fix all three drivers -- the Right, but Difficult Thing To Do --
		//	we decided to do the Easy Thing, here, in user-space.

		//	First, determine the ControlPanel's current Input source (SDIIn1/HDMIIn1 or SDIIn2/HDMIIn2)...
		ULWord	inputSelect	(NTV2_Input1Select);
		ReadRegister (kVRegInputSelect, inputSelect);
		const bool	bIsInput2	(inputSelect == NTV2_Input2Select);

		//	Next, determine the ControlPanel's current TimeCode source (LTC? VITC1? VITC2)...
		RP188SourceSelect TimecodeSource(kRP188SourceEmbeddedLTC);
		CNTV2DriverInterface::ReadRegister(kVRegRP188SourceSelect, TimecodeSource);

		//	Now convert that into an NTV2TCIndex...
		NTV2TCIndex TimecodeIndex = NTV2_TCINDEX_DEFAULT;
		switch (TimecodeSource)
		{
			default:
			case kRP188SourceEmbeddedLTC:		TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2_LTC : NTV2_TCINDEX_SDI1_LTC;	break;
			case kRP188SourceEmbeddedVITC1:		TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2     : NTV2_TCINDEX_SDI1;		break;
			case kRP188SourceEmbeddedVITC2:		TimecodeIndex = bIsInput2 ? NTV2_TCINDEX_SDI2_2   : NTV2_TCINDEX_SDI1_2;	break;
			case kRP188SourceLTCPort:			TimecodeIndex = NTV2_TCINDEX_LTC1;											break;
		}

		//	Fetch the TimeCode value that's in that NTV2TCIndex slot...
		NTV2_RP188	tcValue;
		inOutXferInfo.GetInputTimeCode(tcValue, TimecodeIndex);
		if (TimecodeIndex == NTV2_TCINDEX_LTC1)
		{	//	Special case for external LTC:
			//	Our driver currently returns all-zero DBB values for external LTC.
			//	It should probably at least set DBB BIT(17) "selected RP188 received" if external LTC is present.
			//	Ticket 3367: Our QuickTime 'vdig' relies on DBB BIT(17) being set, or it assumes timecode is invalid
			if (tcValue.fLo  &&  tcValue.fHi  &&  tcValue.fLo != 0xFFFFFFFF  &&  tcValue.fHi != 0xFFFFFFFF)
				tcValue.fDBB |= 0x00020000;
		}

		//	Valid or not, stuff that TimeCode value into inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes[NTV2_TCINDEX_DEFAULT]...
		NTV2_RP188 *	pArray	(reinterpret_cast <NTV2_RP188 *> (inOutXferInfo.acTransferStatus.acFrameStamp.acTimeCodes.GetHostPointer()));
		if (pArray)
			pArray [NTV2_TCINDEX_DEFAULT] = tcValue;
	}	//	if NTV2Message OK && retail mode && capturing

	#if defined (AJA_NTV2_CLEAR_DEVICE_ANC_BUFFER_AFTER_CAPTURE_XFER)
		if (result  &&  NTV2_IS_INPUT_CROSSPOINT(crosspoint))
		{
			ULWord	doZeroing	(0);
			if (ReadRegister(kVRegZeroDeviceAncPostCapture, doZeroing)  &&  doZeroing)
			{	//	Zero out the Anc buffer on the device...
				static NTV2_POINTER	gClearDeviceAncBuffer;
				const LWord		xferFrame	(inOutXferInfo.GetTransferFrameNumber());
				ULWord			ancOffsetF1	(0);
				ULWord			ancOffsetF2	(0);
				NTV2Framesize	fbSize		(NTV2_FRAMESIZE_INVALID);
				ReadRegister(kVRegAncField1Offset, ancOffsetF1);
				ReadRegister(kVRegAncField2Offset, ancOffsetF2);
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
			if (ReadRegister(kVRegZeroHostAncPostCapture, doZeroing)  &&  doZeroing)
			{	//	Zero out everything past the last captured Anc byte in the client's host buffer(s)... 
				NTV2_POINTER &	clientAncBufferF1	(inOutXferInfo.acANCBuffer);
				NTV2_POINTER &	clientAncBufferF2	(inOutXferInfo.acANCField2Buffer);
				const ULWord	ancF1ByteCount		(inOutXferInfo.GetCapturedAncByteCount(false));
				const ULWord	ancF2ByteCount		(inOutXferInfo.GetCapturedAncByteCount(true));
				void *			pF1TailEnd			(clientAncBufferF1.GetHostAddress(ancF1ByteCount));
				void *			pF2TailEnd			(clientAncBufferF2.GetHostAddress(ancF2ByteCount));
				if (pF1TailEnd  &&  clientAncBufferF1.GetByteCount() > ancF1ByteCount)
					::memset (pF1TailEnd, 0, clientAncBufferF1.GetByteCount() - ancF1ByteCount);
				if (pF2TailEnd  &&  clientAncBufferF2.GetByteCount() > ancF2ByteCount)
					::memset (pF2TailEnd, 0, clientAncBufferF2.GetByteCount() - ancF2ByteCount);
			}
		}
	#endif	//	AJA_NTV2_CLEAR_HOST_ANC_BUFFER_TAIL_AFTER_CAPTURE_XFER
	if (result)
		ACDBG("Transfer successful for channel " << DEC(inChannel+1));
	else
		ACFAIL("Transfer failed on channel " << DEC(inChannel+1));
	return result;

}	//	AutoCirculateTransfer


static const AJA_FrameRate	sAJARate2NTV2Rate[] = {	AJA_FrameRate_Unknown,	//	NTV2_FRAMERATE_UNKNOWN	= 0,
													AJA_FrameRate_6000,		//	NTV2_FRAMERATE_6000		= 1,
													AJA_FrameRate_5994,		//	NTV2_FRAMERATE_5994		= 2,
													AJA_FrameRate_3000,		//	NTV2_FRAMERATE_3000		= 3,
													AJA_FrameRate_2997,		//	NTV2_FRAMERATE_2997		= 4,
													AJA_FrameRate_2500,		//	NTV2_FRAMERATE_2500		= 5,
													AJA_FrameRate_2400,		//	NTV2_FRAMERATE_2400		= 6,
													AJA_FrameRate_2398,		//	NTV2_FRAMERATE_2398		= 7,
													AJA_FrameRate_5000,		//	NTV2_FRAMERATE_5000		= 8,
													AJA_FrameRate_4800,		//	NTV2_FRAMERATE_4800		= 9,
													AJA_FrameRate_4795,		//	NTV2_FRAMERATE_4795		= 10,
													AJA_FrameRate_12000,	//	NTV2_FRAMERATE_12000	= 11,
													AJA_FrameRate_11988,	//	NTV2_FRAMERATE_11988	= 12,
													AJA_FrameRate_1500,		//	NTV2_FRAMERATE_1500		= 13,
													AJA_FrameRate_1498,		//	NTV2_FRAMERATE_1498		= 14,
													AJA_FrameRate_1900,		//	NTV2_FRAMERATE_1900		= 15,	// Formerly 09 in older SDKs
													AJA_FrameRate_1898,		//	NTV2_FRAMERATE_1898		= 16, 	// Formerly 10 in older SDKs
													AJA_FrameRate_1800,		//	NTV2_FRAMERATE_1800		= 17,	// Formerly 11 in older SDKs
													AJA_FrameRate_1798};	//	NTV2_FRAMERATE_1798		= 18,	// Formerly 12 in older SDKs

bool CNTV2Card::S2110AddTimecodesToAncBuffers (const NTV2Channel inChannel, AUTOCIRCULATE_TRANSFER & inOutXferInfo)
{
	//	IP 2110 Playout only:	Add relevant transmit timecodes to outgoing Anc
	NTV2FrameRate		ntv2Rate		(NTV2_FRAMERATE_UNKNOWN);
	bool				result			(GetFrameRate(ntv2Rate, inChannel));
	bool				isProgressive	(false);
	NTV2Standard		standard		(NTV2_STANDARD_INVALID);
	NTV2_POINTER &		ancF1			(inOutXferInfo.acANCBuffer);
	NTV2_POINTER &		ancF2			(inOutXferInfo.acANCField2Buffer);
	AJAAncillaryList	pkts;

	if (!result)
		return false;	//	Can't get frame rate
	if (!NTV2_IS_VALID_NTV2FrameRate(ntv2Rate))
		return false;	//	Bad frame rate
	if (!IsProgressiveStandard(isProgressive, inChannel))
		return false;	//	Can't get isProgressive
	if (!GetStandard(standard, inChannel))
		return false;	//	Can't get standard
	if (AJA_FAILURE(AJAAncillaryList::SetFromIPAncData(ancF1, ancF2, pkts)))
		return false;	//	Packet import failed

	const NTV2SmpteLineNumber	smpteLineNumInfo	(::GetSmpteLineNumber(standard));
	const uint32_t				F2StartLine			(smpteLineNumInfo.GetFirstActiveLine(NTV2_FIELD1));
	const AJA_FrameRate			ajaRate				(sAJARate2NTV2Rate[ntv2Rate]);
	const AJATimeBase			ajaTB				(ajaRate);
	const NTV2TCIndexes			tcIndexes			(::GetTCIndexesForSDIConnector(inChannel));
	const size_t				maxNumTCs			(inOutXferInfo.acOutputTimeCodes.GetByteCount() / sizeof(NTV2_RP188));
	NTV2_RP188 *				pTimecodes			(reinterpret_cast<NTV2_RP188*>(inOutXferInfo.acOutputTimeCodes.GetHostPointer()));

	//	For each timecode index for this channel...
	for (NTV2TCIndexesConstIter it(tcIndexes.begin());  it != tcIndexes.end();  ++it)
	{
		const NTV2TCIndex	tcNdx(*it);
		if (size_t(tcNdx) >= maxNumTCs)
			continue;	//	Skip -- not in the array
		if (!NTV2_IS_SDI_TIMECODE_INDEX(tcNdx))
			continue;	//	Skip -- analog or invalid

		const NTV2_RP188	regTC	(pTimecodes[tcNdx]);
		if (!regTC)
			continue;	//	Skip -- invalid timecode (all FFs)

		const bool	isDF (ajaTB.IsNonIntegralRatio());
		AJATimeCode						tc;		tc.SetRP188(regTC.fDBB, regTC.fLo, regTC.fHi, ajaTB);
		AJAAncillaryData_Timecode_ATC	atc;	atc.SetTimecode (tc, ajaTB, isDF);
		atc.AJAAncillaryData_Timecode_ATC::SetDBB (uint8_t(regTC.fDBB & 0x000000FF), uint8_t(regTC.fDBB & 0x0000FF00 >> 8));
		if (NTV2_IS_ATC_VITC2_TIMECODE_INDEX(tcNdx))	//	VITC2?
			atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC2);
		else
		{	//	F1
			if (NTV2_IS_ATC_VITC1_TIMECODE_INDEX(tcNdx))	//	VITC1?
				atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_VITC1);
			else if (NTV2_IS_ATC_LTC_TIMECODE_INDEX(tcNdx))	//	LTC?
				atc.SetDBB1PayloadType(AJAAncillaryData_Timecode_ATC_DBB1PayloadType_LTC);
			else
				continue;
		}
		atc.GeneratePayloadData();
		pkts.AddAncillaryData(atc);
	}	//	for each timecode index value

	pkts.SortListByLocation();
	ancF1.Fill(ULWord(0));	ancF2.Fill(ULWord(0));
	return AJA_SUCCESS(pkts.GetIPTransmitData (ancF1, ancF2, isProgressive, F2StartLine));

}	//	S2110AddTimecodesToAncBuffers
