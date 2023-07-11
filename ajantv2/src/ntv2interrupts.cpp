/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2interrupts.cpp
	@brief		Implementation of CNTV2Card's interrupt functions.
	@copyright	(C) 2004-2022 AJA Video Systems, Inc.
**/

#include "ntv2card.h"


static const INTERRUPT_ENUMS	gChannelToInputInterrupt []	=	{eInput1,	eInput2,	eInput3,	eInput4,	eInput5,	eInput6,	eInput7,	eInput8,	eNumInterruptTypes};
static const INTERRUPT_ENUMS	gChannelToOutputInterrupt [] =	{eOutput1,	eOutput2,	eOutput3,	eOutput4,	eOutput5,	eOutput6,	eOutput7,	eOutput8,	eNumInterruptTypes};


bool CNTV2Card::GetCurrentInterruptMasks (NTV2InterruptMask & outIntMask1, NTV2Interrupt2Mask & outIntMask2)
{
	return CNTV2DriverInterface::ReadRegister(kRegVidIntControl, outIntMask1)  &&  CNTV2DriverInterface::ReadRegister(kRegVidIntControl2, outIntMask2);
}


bool CNTV2Card::EnableInterrupt			(const INTERRUPT_ENUMS inInterruptCode)	{return ConfigureInterrupt (true, inInterruptCode);}
bool CNTV2Card::EnableOutputInterrupt	(const NTV2Channel channel)				{return EnableInterrupt (gChannelToOutputInterrupt [channel]);}
bool CNTV2Card::EnableInputInterrupt	(const NTV2Channel channel)				{return EnableInterrupt (gChannelToInputInterrupt [channel]);}
bool CNTV2Card::EnableInputInterrupt	(const NTV2ChannelSet & inFrameStores)
{
	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inFrameStores.begin());  it != inFrameStores.end();  ++it)
		if (!EnableInputInterrupt (*it))
			failures++;
	return failures == 0;
}

bool CNTV2Card::DisableInterrupt		(const INTERRUPT_ENUMS inInterruptCode)
{
	if(NTV2_IS_INPUT_INTERRUPT(inInterruptCode) || NTV2_IS_OUTPUT_INTERRUPT(inInterruptCode))
		return true;
	return ConfigureInterrupt (false, inInterruptCode);
}
bool CNTV2Card::DisableOutputInterrupt	(const NTV2Channel channel)				{return DisableInterrupt (gChannelToOutputInterrupt [channel]);}
bool CNTV2Card::DisableInputInterrupt	(const NTV2Channel channel)				{return DisableInterrupt (gChannelToInputInterrupt [channel]);}
bool CNTV2Card::DisableInputInterrupt	(const NTV2ChannelSet & inFrameStores)
{
	UWord failures(0);
	for (NTV2ChannelSetConstIter it(inFrameStores.begin());  it != inFrameStores.end();  ++it)
		if (!DisableInputInterrupt (*it))
			failures++;
	return failures == 0;
}
