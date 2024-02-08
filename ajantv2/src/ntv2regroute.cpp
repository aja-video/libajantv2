/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2regroute.cpp
	@brief		CNTV2Card widget routing function implementations.
	@copyright	(C) 2004-2023 AJA Video Systems, Inc.
**/

#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2registerexpert.h"
#include "ajabase/system/debug.h"
#include <deque>

using namespace std;

#define HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') <<				 uint64_t(__x__)  << dec
#define INSTP(_p_)			HEX16(uint64_t(_p_))
#define LOGGING_ROUTING_CHANGES			(AJADebug::IsActive(AJA_DebugUnit_RoutingGeneric))
#define ROUTEFAIL(__x__)	AJA_sERROR	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define ROUTEWARN(__x__)	AJA_sWARNING(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define ROUTENOTE(__x__)	AJA_sNOTICE (AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define ROUTEINFO(__x__)	AJA_sINFO	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define ROUTEDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)


static const ULWord sMasks[]	=	{	0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000	};
static const ULWord sShifts[]	=	{			 0,			 8,			16,			24	};

bool CNTV2Card::GetConnectedOutput (const NTV2InputCrosspointID inInputXpt, NTV2OutputCrosspointID & outOutputXpt)
{
	const ULWord	maxRegNum	(::NTV2DeviceGetMaxRegisterNumber (_boardID));
	uint32_t		regNum		(0);
	uint32_t		ndx			(0);

	outOutputXpt = NTV2_OUTPUT_CROSSPOINT_INVALID;
	if (!CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (inInputXpt, regNum, ndx))
		return false;

	if (!regNum)
		return false;	//	Register number is zero
	if (ndx > 3)
		return false;	//	Bad index
	if (regNum > maxRegNum)
		return false;	//	This device doesn't have that routing register

	return CNTV2DriverInterface::ReadRegister (regNum, outOutputXpt, sMasks[ndx], sShifts[ndx]);

}	//	GetConnectedOutput


bool CNTV2Card::GetConnectedInput (const NTV2OutputCrosspointID inOutputXpt, NTV2InputCrosspointID & outInputXpt)
{
	for (outInputXpt = NTV2_FIRST_INPUT_CROSSPOINT;
		outInputXpt <= NTV2_LAST_INPUT_CROSSPOINT;
		outInputXpt = NTV2InputCrosspointID(outInputXpt+1))
	{
		NTV2OutputCrosspointID	tmpOutputXpt	(NTV2_OUTPUT_CROSSPOINT_INVALID);
		if (GetConnectedOutput (outInputXpt, tmpOutputXpt))
			if (tmpOutputXpt == inOutputXpt)
				return true;
	}
	outInputXpt = NTV2_INPUT_CROSSPOINT_INVALID;
	return true;
}


bool CNTV2Card::GetConnectedInputs (const NTV2OutputCrosspointID inOutputXpt, NTV2InputCrosspointIDSet & outInputXpts)
{
	outInputXpts.clear();
	if (!NTV2_IS_VALID_OutputCrosspointID(inOutputXpt))
		return false;
	if (inOutputXpt == NTV2_XptBlack)
		return false;
	for (NTV2InputCrosspointID inputXpt(NTV2_FIRST_INPUT_CROSSPOINT);
		inputXpt <= NTV2_LAST_INPUT_CROSSPOINT;
		inputXpt = NTV2InputCrosspointID(inputXpt+1))
	{
		NTV2OutputCrosspointID	tmpOutputXpt(NTV2_OUTPUT_CROSSPOINT_INVALID);
		if (GetConnectedOutput (inputXpt, tmpOutputXpt))
			if (tmpOutputXpt == inOutputXpt)
				outInputXpts.insert(inputXpt);
	}
	return !outInputXpts.empty();
}


bool CNTV2Card::Connect (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, const bool inValidate)
{
	if (inOutputXpt == NTV2_XptBlack)
		return Disconnect (inInputXpt);

	const ULWord	maxRegNum	(::NTV2DeviceGetMaxRegisterNumber(_boardID));
	uint32_t		regNum		(0);
	uint32_t		ndx			(0);
	bool			canConnect	(true);

	if (!CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo(inInputXpt, regNum, ndx))
		{ROUTEFAIL(GetDisplayName() << ": GetCrosspointSelectGroupRegisterInfo failed, inputXpt=" << DEC(inInputXpt));  return false;}
	if (!regNum  ||  regNum > maxRegNum)
		{ROUTEFAIL(GetDisplayName() << ": GetCrosspointSelectGroupRegisterInfo returned bad register number '" << DEC(regNum) << "' for inputXpt=" << DEC(inInputXpt));  return false;}
	if (ndx > 3)
		{ROUTEFAIL(GetDisplayName() << ": GetCrosspointSelectGroupRegisterInfo returned bad index '" << DEC(ndx) << "' for inputXpt=" << DEC(inInputXpt));  return false;}

	if (inValidate)		//	If caller requested xpt validation
		if (CanConnect(inInputXpt, inOutputXpt, canConnect))	//	If answer can be trusted
			if (!canConnect)	//	If route not valid
			{
				ROUTEFAIL (GetDisplayName() << ": Unsupported route " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt)
							<< ": reg=" << DEC(regNum) << " val=" << DEC(inOutputXpt) << " mask=" << xHEX0N(sMasks[ndx],8) << " shift=" << DEC(sShifts[ndx]));
				return false;
			}

	ULWord	outputXpt(0);
	const bool isLogging (LOGGING_ROUTING_CHANGES);
	if (isLogging)
		ReadRegister(regNum, outputXpt, sMasks[ndx], sShifts[ndx]);
	const bool result (WriteRegister(regNum, inOutputXpt, sMasks[ndx], sShifts[ndx]));
	if (isLogging)
	{
		if (!result)
			ROUTEFAIL(GetDisplayName() << ": Failed to connect " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt)
						<< ": reg=" << DEC(regNum) << " val=" << DEC(inOutputXpt) << " mask=" << xHEX0N(sMasks[ndx],8) << " shift=" << DEC(sShifts[ndx]));
		else if (outputXpt	&&	inOutputXpt != outputXpt)
			ROUTENOTE(GetDisplayName() << ": Connected " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt)
						<< " -- was from " << ::NTV2OutputCrosspointIDToString(NTV2OutputXptID(outputXpt)));
		else if (!outputXpt	 &&	 inOutputXpt != outputXpt)
			ROUTENOTE(GetDisplayName() << ": Connected " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt) << " -- was disconnected");
		//else	ROUTEDBG(GetDisplayName() << ": Connection " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(inOutputXpt) << " unchanged -- already connected");
	}
	return result;
}


bool CNTV2Card::Disconnect (const NTV2InputCrosspointID inInputXpt)
{
	const ULWord	maxRegNum	(::NTV2DeviceGetMaxRegisterNumber(_boardID));
	uint32_t		regNum		(0);
	uint32_t		ndx			(0);
	ULWord			outputXpt	(0);

	if (!CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo(inInputXpt, regNum, ndx))
		return false;
	if (!regNum)
		return false;	//	Register number is zero
	if (ndx > 3)
		return false;	//	Bad index
	if (regNum > maxRegNum)
		return false;	//	This device doesn't have that routing register

	const bool isLogging (LOGGING_ROUTING_CHANGES);
	bool changed (false);
	if (isLogging)
		changed = ReadRegister(regNum, outputXpt, sMasks[ndx], sShifts[ndx])  &&  outputXpt;
	const bool result (WriteRegister(regNum, NTV2_XptBlack, sMasks[ndx], sShifts[ndx]));
	if (isLogging)
	{
		if (result && changed)
			ROUTENOTE(GetDisplayName() << ": Disconnected " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(NTV2OutputXptID(outputXpt)));
		else if (!result)
			ROUTEFAIL(GetDisplayName() << ": Failed to disconnect " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(NTV2OutputXptID(outputXpt))
						<< ": reg=" << DEC(regNum) << " val=0 mask=" << xHEX0N(sMasks[ndx],8) << " shift=" << DEC(sShifts[ndx]));
		//else	ROUTEDBG(GetDisplayName() << ": " << ::NTV2InputCrosspointIDToString(inInputXpt) << " <== " << ::NTV2OutputCrosspointIDToString(NTV2OutputXptID(outputXpt)) << " already disconnected");
	}
	return result;
}


bool CNTV2Card::IsConnectedTo (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, bool & outIsConnected)
{
	NTV2OutputCrosspointID	outputID	(NTV2_XptBlack);

	outIsConnected = false;
	if (!GetConnectedOutput (inInputXpt, outputID))
		return false;

	outIsConnected = outputID == inOutputXpt;
	return true;
}


bool CNTV2Card::IsConnected (const NTV2InputCrosspointID inInputXpt, bool & outIsConnected)
{
	bool	isConnectedToBlack	(false);
	if (!IsConnectedTo (inInputXpt, NTV2_XptBlack, isConnectedToBlack))
		return false;

	outIsConnected = !isConnectedToBlack;
	return true;
}


bool CNTV2Card::CanConnect (const NTV2InputCrosspointID inInputXpt, const NTV2OutputCrosspointID inOutputXpt, bool & outCanConnect)
{
	outCanConnect = false;
	if (!IsSupported(kDeviceHasXptConnectROM))
		return false;	//	No CanConnect ROM -- can't say

	//	NOTE:	This is not a very efficient implementation, but CanConnect probably isn't being
	//			called every frame. The good news is that this function now gets its information
	//			"straight from the horse's mouth" via the validated GetRouteROMInfoFromReg function,
	//			so its answer will be trustworthy.

	//	Check for reasonable input xpt...
	if (ULWord(inInputXpt) < ULWord(NTV2_FIRST_INPUT_CROSSPOINT)  ||  ULWord(inInputXpt) > NTV2_LAST_INPUT_CROSSPOINT)
	{
		ROUTEFAIL(GetDisplayName() << ": " << xHEX0N(UWord(inInputXpt),4) << " > "
				<< xHEX0N(UWord(NTV2_LAST_INPUT_CROSSPOINT),4) << " (out of range)");
		return false;
	}

	//	Every input xpt can connect to XptBlack...
	if (inOutputXpt == NTV2_XptBlack)
		{outCanConnect = true;	return true;}

	//	Check for reasonable output xpt...
	if (ULWord(inOutputXpt) >= UWord(NTV2_LAST_OUTPUT_CROSSPOINT))
	{
		ROUTEFAIL(GetDisplayName() << ":  Bad output xpt " << xHEX0N(ULWord(inOutputXpt),4) << " >= "
					<< xHEX0N(UWord(NTV2_LAST_OUTPUT_CROSSPOINT),4));
		return false;
	}

	//	Determine all legal output xpts for this input xpt...
	NTV2OutputXptIDSet legalOutputXpts;
	const uint32_t regBase(uint32_t(kRegFirstValidXptROMRegister)  +  4UL * uint32_t(inInputXpt - NTV2_FIRST_INPUT_CROSSPOINT));
	for (uint32_t ndx(0);  ndx < 4;	 ndx++)
	{
		ULWord regVal(0);
		NTV2InputXptID inputXpt;
		ReadRegister(regBase + ndx, regVal);
		if (!CNTV2SignalRouter::GetRouteROMInfoFromReg (regBase + ndx, regVal, inputXpt, legalOutputXpts, true/*append*/))
			ROUTEWARN(GetDisplayName() << ":  GetRouteROMInfoFromReg failed for register " << DEC(regBase+ndx)
					<< ", input xpt ' " << ::NTV2InputCrosspointIDToString(inInputXpt) << "' " << xHEX0N(UWord(inInputXpt),2));
	}

	//	Is the route implemented?
	outCanConnect = legalOutputXpts.find(inOutputXpt) != legalOutputXpts.end();
	return true;
}


bool CNTV2Card::ApplySignalRoute (const CNTV2SignalRouter & inRouter, const bool inReplace)
{
	if (inReplace)
		if (!ClearRouting ())
			return false;

	NTV2RegisterWrites	registerWrites;
	if (!inRouter.GetRegisterWrites (registerWrites))
		return false;

	return WriteRegisters (registerWrites);
}

bool CNTV2Card::ApplySignalRoute (const NTV2XptConnections & inConnections, const bool inReplace)
{
	if (inReplace)
		if (!ClearRouting ())
			return false;

	unsigned failures(0);
	for (NTV2XptConnectionsConstIter iter(inConnections.begin());  iter != inConnections.end();	 ++iter)
		if (!Connect(iter->first, iter->second, IsSupported(kDeviceHasXptConnectROM)))
			failures++;
	return failures == 0;
}

bool CNTV2Card::RemoveConnections (const NTV2XptConnections & inConnections)
{
	unsigned failures(0);
	for (NTV2XptConnectionsConstIter iter(inConnections.begin());  iter != inConnections.end();	 ++iter)
		if (!Disconnect(iter->first))
			failures++;
	return failures == 0;
}


bool CNTV2Card::ClearRouting (void)
{
	const NTV2RegNumSet routingRegisters	(CNTV2RegisterExpert::GetRegistersForClass (kRegClass_Routing));
	const ULWord		maxRegisterNumber	(::NTV2DeviceGetMaxRegisterNumber (_boardID));
	unsigned			nFailures			(0);
	ULWord				tally				(0);

	for (NTV2RegNumSetConstIter it(routingRegisters.begin());  it != routingRegisters.end();  ++it) //	for each routing register
		if (*it <= maxRegisterNumber)																	//		if it's valid for this board
		{	ULWord	num(0);
			if (ReadRegister (*it, num))
				tally += num;
			if (!WriteRegister (*it, 0))																//			then if WriteRegister fails
				nFailures++;																			//				then bump the failure tally
		}

	if (tally && !nFailures)
		ROUTEINFO(GetDisplayName() << ": Routing cleared");
	else if (!nFailures)
		ROUTEDBG(GetDisplayName() << ": Routing already clear, nothing changed");
	else
		ROUTEFAIL(GetDisplayName() << ": " << DEC(nFailures) << " register write(s) failed");
	return nFailures == 0;

}	//	ClearRouting


bool CNTV2Card::GetRouting (CNTV2SignalRouter & outRouting)
{
	outRouting.Reset ();

	//	First, compile a set of NTV2WidgetIDs that are legit for this device...
	NTV2WidgetIDSet validWidgets;
	if (!CNTV2SignalRouter::GetWidgetIDs (GetDeviceID(), validWidgets))
		return false;

	ROUTEDBG(GetDisplayName() << ": '" << ::NTV2DeviceIDToString(GetDeviceID()) << "' has " << validWidgets.size() << " widgets: " << validWidgets);

	//	Inspect every input of every widget...
	for (NTV2WidgetIDSetConstIter pWidgetID (validWidgets.begin ());  pWidgetID != validWidgets.end ();	 ++pWidgetID)
	{
		const NTV2WidgetID	curWidgetID (*pWidgetID);
		NTV2InputXptIDSet	inputs;

		CNTV2SignalRouter::GetWidgetInputs (curWidgetID, inputs);
		ROUTEDBG(GetDisplayName() << ": " << ::NTV2WidgetIDToString(curWidgetID) << " (" << ::NTV2WidgetIDToString(curWidgetID, true) << ") has " << inputs.size() << " input(s):  " << inputs);

		for (NTV2InputCrosspointIDSetConstIter pInputID (inputs.begin ());	pInputID != inputs.end ();	++pInputID)
		{
			NTV2OutputCrosspointID	outputID	(NTV2_XptBlack);
			if (!GetConnectedOutput (*pInputID, outputID))
				ROUTEDBG(GetDisplayName() << ": 'GetConnectedOutput' failed for input " << ::NTV2InputCrosspointIDToString(*pInputID) << " (" << ::NTV2InputCrosspointIDToString(*pInputID, true) << ")");
			else if (outputID == NTV2_XptBlack)
				ROUTEDBG(GetDisplayName() << ": 'GetConnectedOutput' returned XptBlack for input '" << ::NTV2InputCrosspointIDToString(*pInputID, true) << "' (" << ::NTV2InputCrosspointIDToString(*pInputID, false) << ")");
			else
			{
				outRouting.AddConnection (*pInputID, outputID);		//	Record this connection...
				ROUTEDBG(GetDisplayName() << ": Connection found -- from input '" << ::NTV2InputCrosspointIDToString(*pInputID, true) << "' (" << ::NTV2InputCrosspointIDToString(*pInputID, false)
						<< ") <== to output '" << ::NTV2OutputCrosspointIDToString(outputID, true) << "' (" << ::NTV2OutputCrosspointIDToString(outputID, false) << ")");
			}
		}	//	for each input
	}	//	for each valid widget
	ROUTEDBG(GetDisplayName() << ": Returning " << outRouting);
	return true;

}	//	GetRouting

bool CNTV2Card::GetConnections (NTV2XptConnections & outConnections)
{
	outConnections.clear();
	NTV2RegisterReads regInfos;
	NTV2InputCrosspointIDSet inputXpts;
	return CNTV2SignalRouter::GetAllWidgetInputs (_boardID, inputXpts)
			&&	CNTV2SignalRouter::GetAllRoutingRegInfos (inputXpts, regInfos)
			&&	ReadRegisters(regInfos)
			&&	CNTV2SignalRouter::GetConnectionsFromRegs (inputXpts, regInfos, outConnections);
}


typedef deque <NTV2InputCrosspointID>				NTV2InputCrosspointQueue;
typedef NTV2InputCrosspointQueue::const_iterator	NTV2InputCrosspointQueueConstIter;
typedef NTV2InputCrosspointQueue::iterator			NTV2InputCrosspointQueueIter;


bool CNTV2Card::GetRoutingForChannel (const NTV2Channel inChannel, CNTV2SignalRouter & outRouting)
{
	NTV2InputCrosspointQueue			inputXptQueue;	//	List of inputs to trace backward from
	static const NTV2InputCrosspointID	SDIOutInputs [] = { NTV2_XptSDIOut1Input,	NTV2_XptSDIOut2Input,	NTV2_XptSDIOut3Input,	NTV2_XptSDIOut4Input,
															NTV2_XptSDIOut5Input,	NTV2_XptSDIOut6Input,	NTV2_XptSDIOut7Input,	NTV2_XptSDIOut8Input};
	outRouting.Reset ();

	if (IS_CHANNEL_INVALID (inChannel))
		return false;

	//	Seed the input crosspoint queue...
	inputXptQueue.push_back (SDIOutInputs [inChannel]);

	//	Process all queued inputs...
	while (!inputXptQueue.empty ())
	{
		NTV2InputCrosspointID		inputXpt	(inputXptQueue.front ());
		NTV2OutputCrosspointID		outputXpt	(NTV2_XptBlack);
		NTV2WidgetID				widgetID	(NTV2_WIDGET_INVALID);
		NTV2InputCrosspointIDSet	inputXpts;

		inputXptQueue.pop_front ();

		if (inputXpt == NTV2_INPUT_CROSSPOINT_INVALID)
			continue;

		//	Find out what this input is connected to...
		if (!GetConnectedOutput (inputXpt, outputXpt))
			continue;	//	Keep processing input crosspoints, even if this fails

		if (outputXpt != NTV2_XptBlack)
		{
			//	Make a note of this connection...
			outRouting.AddConnection (inputXpt, outputXpt);

			//	Find out what widget this output belongs to...
			CNTV2SignalRouter::GetWidgetForOutput (outputXpt, widgetID);
			assert (NTV2_IS_VALID_WIDGET (widgetID));	//	FIXFIXFIX	I want to know of any missing NTV2OutputCrosspointID ==> NTV2WidgetID links
			if (!NTV2_IS_VALID_WIDGET (widgetID))
				continue;	//	Keep processing input crosspoints, even if no such widget
			if (!::NTV2DeviceCanDoWidget (GetDeviceID (), widgetID))
				continue;	//	Keep processing input crosspoints, even if no such widget on this device

			//	Add every input of the output's widget to the queue...
			CNTV2SignalRouter::GetWidgetInputs (widgetID, inputXpts);
			for (NTV2InputCrosspointIDSetConstIter it (inputXpts.begin ());	 it != inputXpts.end ();  ++it)
				inputXptQueue.push_back (*it);
		}	//	if connected to something other than "black" output crosspoint
	}	//	loop til inputXptQueue empty

	ROUTEDBG(GetDisplayName() << ": Channel " << DEC(inChannel+1) << " routing: " << outRouting);
	return true;

}	//	GetRoutingForChannel
