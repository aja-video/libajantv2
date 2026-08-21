/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2interrupts.cpp
	@brief		Implementation of CNTV2Card's interrupt enable/disable functions.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2card.h"


static const INTERRUPT_ENUMS	gChannelToInputInterrupt []	=	{eInput1,	eInput2,	eInput3,	eInput4,	eInput5,	eInput6,	eInput7,	eInput8,	eNumInterruptTypes};
static const INTERRUPT_ENUMS	gChannelToOutputInterrupt [] =	{eOutput1,	eOutput2,	eOutput3,	eOutput4,	eOutput5,	eOutput6,	eOutput7,	eOutput8,	eNumInterruptTypes};


bool CNTV2Card::GetCurrentInterruptMasks (NTV2InterruptMask & outIntMask1, NTV2Interrupt2Mask & outIntMask2)
{
	return driverInterface().ReadRegister(kRegVidIntControl, outIntMask1)  &&  driverInterface().ReadRegister(kRegVidIntControl2, outIntMask2);
}

bool CNTV2Card::EnableInterrupt (const INTERRUPT_ENUMS id)
{
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
	{	NTV2ConfigureInterrupt msg;
		return _pRPCAPI->NTV2MessageRemote(msg.doEnable(id))  &&  msg.isSuccess();
	}
#endif// defined(NTV2_NUB_CLIENT_SUPPORT)
	return ConfigureInterrupt (true, id);
}

bool CNTV2Card::EnableOutputInterrupt (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  EnableInterrupt (gChannelToOutputInterrupt [inChannel]);
}

bool CNTV2Card::EnableInputInterrupt (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  EnableInterrupt (gChannelToInputInterrupt [inChannel]);
}

bool CNTV2Card::EnableInputInterrupt (const NTV2ChannelSet & inFrameStores)
{
	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inFrameStores.begin());  it != inFrameStores.end();  ++it)
		if (!EnableInputInterrupt(*it))
			failures++;
	return failures == 0;
}

bool CNTV2Card::DisableInterrupt (const INTERRUPT_ENUMS id)
{
	if (NTV2_IS_INPUT_INTERRUPT(id)  ||  NTV2_IS_OUTPUT_INTERRUPT(id))
		return true;	//	Can't disable input/output interrupts
#if defined(NTV2_NUB_CLIENT_SUPPORT)
	if (IsRemote())
	{	NTV2ConfigureInterrupt msg;
		return _pRPCAPI->NTV2MessageRemote(msg.doDisable(id))  &&  msg.isSuccess();
	}
#endif// defined(NTV2_NUB_CLIENT_SUPPORT)
	return ConfigureInterrupt (false, id);
}

bool CNTV2Card::DisableOutputInterrupt (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  DisableInterrupt (gChannelToOutputInterrupt [inChannel]);
}

bool CNTV2Card::DisableInputInterrupt (const NTV2Channel inChannel)
{
	return NTV2_IS_VALID_CHANNEL(inChannel)  &&  DisableInterrupt (gChannelToInputInterrupt [inChannel]);
}

bool CNTV2Card::DisableInputInterrupt	(const NTV2ChannelSet & inFrameStores)
{
	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inFrameStores.begin());  it != inFrameStores.end();  ++it)
		if (!DisableInputInterrupt(*it))
			failures++;
	return failures == 0;
}
