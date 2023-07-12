/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2subscriptions.cpp
	@brief		Implementation of CNTV2Card's event notification subscription functions.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2card.h"

using namespace std; 


static INTERRUPT_ENUMS	gChannelToOutputVerticalInterrupt[]	= {eOutput1, eOutput2, eOutput3, eOutput4, eOutput5, eOutput6, eOutput7, eOutput8, eNumInterruptTypes};
static INTERRUPT_ENUMS	gChannelToInputVerticalInterrupt[]	= {eInput1,  eInput2,  eInput3,  eInput4,  eInput5,  eInput6,  eInput7,  eInput8,  eNumInterruptTypes};


//	Subscribe to events

bool CNTV2Card::SubscribeEvent (const INTERRUPT_ENUMS inEventCode)
{
	return NTV2_IS_VALID_INTERRUPT_ENUM(inEventCode)  &&  ConfigureSubscription (true, inEventCode, mInterruptEventHandles[inEventCode]);
}


bool CNTV2Card::SubscribeOutputVerticalEvent (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  SubscribeEvent(gChannelToOutputVerticalInterrupt[inChannel]);
}

bool CNTV2Card::SubscribeOutputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!SubscribeOutputVerticalEvent(*it))
			failures++;
	return !failures;
}


bool CNTV2Card::SubscribeInputVerticalEvent (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  SubscribeEvent(gChannelToInputVerticalInterrupt[inChannel]);
}

bool CNTV2Card::SubscribeInputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!SubscribeInputVerticalEvent(*it))
			failures++;
	return !failures;
}


//	Unsubscribe from events

bool CNTV2Card::UnsubscribeEvent (const INTERRUPT_ENUMS inEventCode)
{
	return NTV2_IS_VALID_INTERRUPT_ENUM(inEventCode)  &&  ConfigureSubscription (false, inEventCode, mInterruptEventHandles[inEventCode]);
}


bool CNTV2Card::UnsubscribeOutputVerticalEvent (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  UnsubscribeEvent(gChannelToOutputVerticalInterrupt[inChannel]);
}

bool CNTV2Card::UnsubscribeOutputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!UnsubscribeOutputVerticalEvent(*it))
			failures++;
	return !failures;
}


bool CNTV2Card::UnsubscribeInputVerticalEvent (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  UnsubscribeEvent(gChannelToInputVerticalInterrupt[inChannel]);
}

bool CNTV2Card::UnsubscribeInputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!UnsubscribeInputVerticalEvent(*it))
			failures++;
	return !failures;
}


//	Get interrupt count

bool CNTV2Card::GetOutputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = 0;
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  GetInterruptCount (gChannelToOutputVerticalInterrupt[inChannel], outCount);
}


bool CNTV2Card::GetInputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = 0;
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  GetInterruptCount (gChannelToInputVerticalInterrupt[inChannel], outCount);
}


//	Get event count

bool CNTV2Card::GetOutputVerticalEventCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = NTV2_IS_VALID_CHANNEL(inChannel)  ?  mEventCounts.at(gChannelToOutputVerticalInterrupt[inChannel])  :  0;
	return NTV2_IS_VALID_CHANNEL(inChannel);
}


bool CNTV2Card::GetInputVerticalEventCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = NTV2_IS_VALID_CHANNEL(inChannel)  ?  mEventCounts.at(gChannelToInputVerticalInterrupt[inChannel])  :  0;
	return NTV2_IS_VALID_CHANNEL(inChannel);
}


//	Set event count

bool CNTV2Card::SetOutputVerticalEventCount (const ULWord inCount, const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  SetInterruptEventCount(gChannelToOutputVerticalInterrupt[inChannel], inCount);
}

bool CNTV2Card::SetInputVerticalEventCount (const ULWord inCount, const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  SetInterruptEventCount(gChannelToInputVerticalInterrupt[inChannel], inCount);
}


bool CNTV2Card::WaitForOutputVerticalInterrupt (const NTV2Channel inChannel, UWord inRepeatCount)
{
	bool	result	(true);
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;
	if (!inRepeatCount)
		return false;
	do
	{
		result = WaitForInterrupt (gChannelToOutputVerticalInterrupt [inChannel]);
	} while (--inRepeatCount && result);
	return result;
}


bool CNTV2Card::WaitForInputVerticalInterrupt (const NTV2Channel inChannel, UWord inRepeatCount)
{
	bool	result	(true);
	if (!NTV2_IS_VALID_CHANNEL (inChannel))
		return false;
	if (!inRepeatCount)
		return false;
	do
	{
		result = WaitForInterrupt (gChannelToInputVerticalInterrupt [inChannel]);
	} while (--inRepeatCount && result);
	return result;
}


bool CNTV2Card::GetOutputFieldID (const NTV2Channel channel, NTV2FieldID & outFieldID)
{
	//           	         	 	   CHANNEL1    CHANNEL2     CHANNEL3     CHANNEL4     CHANNEL5     CHANNEL6     CHANNEL7     CHANNEL8
	static ULWord	regNum []	=	{kRegStatus, kRegStatus,  kRegStatus,  kRegStatus, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, 0};
	static ULWord	bitShift []	=	{        23,          5,           3,           1,           9,           7,           5,           3, 0};

	// Check status register to see if it is the one we want.
	ULWord	statusValue	(0);
	ReadRegister (regNum[channel], statusValue);
	outFieldID = static_cast <NTV2FieldID> ((statusValue >> bitShift [channel]) & 0x1);

	return true;

}	//	GetOutputFieldID


bool CNTV2Card::WaitForOutputFieldID (const NTV2FieldID inFieldID, const NTV2Channel channel)
{
	//	Wait for next field interrupt...
	bool	bInterruptHappened	(WaitForOutputVerticalInterrupt (channel));

	// Check status register to see if it is the one we want.
	NTV2FieldID	currentFieldID (NTV2_FIELD0);
	GetOutputFieldID(channel, currentFieldID);

	//	If not, wait for another field interrupt...
	if (currentFieldID != inFieldID)
		bInterruptHappened = WaitForOutputVerticalInterrupt (channel);

	return bInterruptHappened;

}	//	WaitForOutputFieldID


bool CNTV2Card::GetInputFieldID (const NTV2Channel channel, NTV2FieldID & outFieldID)
{
	//           	         	 	   CHANNEL1    CHANNEL2     CHANNEL3     CHANNEL4     CHANNEL5     CHANNEL6     CHANNEL7     CHANNEL8
	static ULWord	regNum []	=	{kRegStatus, kRegStatus, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, 0};
	static ULWord	bitShift []	=	{        21,         19,          21,          19,          17,          15,          13,           3, 0};

	//	See if the field ID of the last input vertical interrupt is the one of interest...
	ULWord	statusValue (0);
	ReadRegister (regNum[channel], statusValue);
	outFieldID = static_cast <NTV2FieldID> ((statusValue >> bitShift [channel]) & 0x1);

	return true;

}	//	GetInputFieldID


bool CNTV2Card::WaitForInputFieldID (const NTV2FieldID inFieldID, const NTV2Channel channel)
{
	//	Wait for next field interrupt...
	bool	bInterruptHappened	(WaitForInputVerticalInterrupt (channel));

	//	See if the field ID of the last input vertical interrupt is the one of interest...
	NTV2FieldID	currentFieldID (NTV2_FIELD0);
	GetInputFieldID(channel, currentFieldID);

	//	If not, wait for another field interrupt...
	if (currentFieldID != inFieldID)
		bInterruptHappened = WaitForInputVerticalInterrupt (channel);

	return bInterruptHappened;

}	//	WaitForInputFieldID
