/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2subscriptions.cpp
	@brief		Implementation of CNTV2Card's event notification subscription functions.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ajabase/system/debug.h"

using namespace std; 

#define INSTP(_p_)			HEX0N(uint64_t(_p_),16)
#define DIFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DINOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define DIDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_DriverInterface, INSTP(this) << "::" << AJAFUNC << ": " << __x__)


static INTERRUPT_ENUMS	gChannelToOutputVerticalInterrupt[]	= {eOutput1, eOutput2, eOutput3, eOutput4, eOutput5, eOutput6, eOutput7, eOutput8, eNumInterruptTypes};
static INTERRUPT_ENUMS	gChannelToInputVerticalInterrupt[]	= {eInput1,  eInput2,  eInput3,  eInput4,  eInput5,  eInput6,  eInput7,  eInput8,  eNumInterruptTypes};


//	Subscribe/Unsubscribe to/from events

bool CNTV2Card::SubscribeEvent (const INTERRUPT_ENUMS id)
{
	if (!IsOpen())
		{DIFAIL("Cannot subscribe to '" << NTV2CfgInterrupt::IntName(id) << "' -- device not open");  return false;}
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(id))
		{DIFAIL("Cannot subscribe to '" << NTV2CfgInterrupt::IntName(id) << "' -- bad interrupt ID");  return false;}
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
	{	NTV2ConfigureInterrupt msg(this);
		return _pRPCAPI->NTV2MessageRemote(msg.doSubscribe(id))  &&  msg.isSuccess();
	}
#endif// defined(NTV2_NUB_CLIENT_SUPPORT)
#if defined(MSWindows)
	return WinConfigureSubscription (true, id);
#else
	DIDBG("Subscribe to '" << NTV2CfgInterrupt::IntName(id) << "' is a no-op on this platform");
	return true;	//	Non-Windows platforms reply "good to go"
#endif
}

bool CNTV2Card::SubscribeOutputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!SubscribeOutputVerticalEvent(*it))
			failures++;
	return !failures;
}

bool CNTV2Card::SubscribeInputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!SubscribeInputVerticalEvent(*it))
			failures++;
	return !failures;
}

bool CNTV2Card::UnsubscribeEvent (const INTERRUPT_ENUMS id)
{
	if (!IsOpen())
		{DIFAIL("Cannot unsubscribe from '" << NTV2CfgInterrupt::IntName(id) << "' -- device not open");  return false;}
	if (!NTV2_IS_VALID_INTERRUPT_ENUM(id))
		{DIFAIL("Cannot unsubscribe from '" << NTV2CfgInterrupt::IntName(id) << "' -- bad interrupt ID");  return false;}
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
	{	NTV2ConfigureInterrupt msg(this);
		return _pRPCAPI->NTV2MessageRemote(msg.doUnsubscribe(id))  &&  msg.isSuccess();
	}
#endif// defined(NTV2_NUB_CLIENT_SUPPORT)
#if defined(MSWindows)
	return WinConfigureSubscription (false, id);
#else
	DIDBG("Unsubscribe from '" << NTV2CfgInterrupt::IntName(id) << "' is a no-op on this platform");
	return true;	//	Non-Windows platforms reply "good to go"
#endif
}

bool CNTV2Card::UnsubscribeOutputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!UnsubscribeOutputVerticalEvent(*it))
			failures++;
	return !failures;
}

bool CNTV2Card::UnsubscribeInputVerticalEvent (const NTV2ChannelSet & inChannels)
{	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inChannels.begin());  it != inChannels.end();  ++it)
		if (!UnsubscribeInputVerticalEvent(*it))
			failures++;
	return !failures;
}

//	NOTE: There's currently no API call to inquire which interrupts are subscribed or not


//	Get interrupt count from driver

bool CNTV2Card::GetOutputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = 0;
	return GetInterruptCount(::NTV2ChannelToOutputInterrupt(inChannel), outCount);
}


bool CNTV2Card::GetInputVerticalInterruptCount (ULWord & outCount, const NTV2Channel inChannel)
{
	outCount = 0;
	return GetInterruptCount (::NTV2ChannelToInputInterrupt(inChannel), outCount);
}

//	NOTE: There's currently no driver API call to reset the driver's interrupt counters


bool CNTV2Card::WaitForOutputVerticalInterrupt (const NTV2Channel inChannel, UWord inRepeatCount)
{
	bool	result	(true);
	if (!NTV2_IS_VALID_CHANNEL(inChannel))
		return false;
	if (!inRepeatCount)
		return false;
	do
	{
		result = WaitForInterrupt (::NTV2ChannelToOutputInterrupt(inChannel));
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
		result = WaitForInterrupt (::NTV2ChannelToInputInterrupt(inChannel));
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
	outFieldID = ReadRegister (regNum[channel], statusValue)
					?  NTV2FieldID((statusValue >> bitShift[channel]) & 0x1)
					:  NTV2_FIELD_INVALID;
	return NTV2_IS_VALID_FIELD(outFieldID);

}	//	GetOutputFieldID


bool CNTV2Card::WaitForOutputFieldID (const NTV2FieldID inFieldID, const NTV2Channel channel)
{
	//	Wait for next field interrupt...
	bool bInterruptHappened	(WaitForOutputVerticalInterrupt(channel));

	// Check status register to see if it is the one we want.
	NTV2FieldID	currentFieldID (NTV2_FIELD0);
	GetOutputFieldID(channel, currentFieldID);

	//	If not, wait for another field interrupt...
	if (currentFieldID != inFieldID)
		bInterruptHappened = WaitForOutputVerticalInterrupt(channel);

	return bInterruptHappened;

}	//	WaitForOutputFieldID


bool CNTV2Card::GetInputFieldID (const NTV2Channel channel, NTV2FieldID & outFieldID)
{
	//           	         	 	   CHANNEL1    CHANNEL2     CHANNEL3     CHANNEL4     CHANNEL5     CHANNEL6     CHANNEL7     CHANNEL8
	static ULWord	regNum []	=	{kRegStatus, kRegStatus, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, kRegStatus2, 0};
	static ULWord	bitShift []	=	{        21,         19,          21,          19,          17,          15,          13,           3, 0};

	//	See if the field ID of the last input vertical interrupt is the one of interest...
	ULWord	statusValue (0);
	outFieldID = ReadRegister (regNum[channel], statusValue)
                	?  NTV2FieldID((statusValue >> bitShift[channel]) & 0x1)
                	:  NTV2_FIELD_INVALID;
	return NTV2_IS_VALID_FIELD(outFieldID);

}	//	GetInputFieldID


bool CNTV2Card::WaitForInputFieldID (const NTV2FieldID inFieldID, const NTV2Channel channel)
{
	//	Wait for next field interrupt...
	bool bInterruptHappened	(WaitForInputVerticalInterrupt(channel));

	//	See if the field ID of the last input vertical interrupt is the one of interest...
	NTV2FieldID	currentFieldID (NTV2_FIELD0);
	GetInputFieldID(channel, currentFieldID);

	//	If not, wait for another field interrupt...
	if (currentFieldID != inFieldID)
		bInterruptHappened = WaitForInputVerticalInterrupt(channel);

	return bInterruptHappened;

}	//	WaitForInputFieldID
