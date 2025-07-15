/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2signalrouter.cpp
	@brief		CNTV2SignalRouter implementation.
	@copyright	(C) 2014-2022 AJA Video Systems, Inc.
**/
#include "ntv2signalrouter.h"
#include "ntv2routingexpert.h"
#include "ntv2debug.h"
#include "ntv2utils.h"
#include "ntv2devicefeatures.hh"
#include "ntv2registerexpert.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <algorithm>

using namespace std;

// Logging helpers
#define HEX16(__x__)		"0x" << hex << setw(16) << setfill('0') << uint64_t(__x__)	<< dec
#define INSTP(_p_)			HEX16(uint64_t(_p_))
#define SRiFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define SRiWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define SRiNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define SRiINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define SRiDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_RoutingGeneric, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define SRFAIL(__x__)		AJA_sERROR	(AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)
#define SRWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)
#define SRNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)
#define SRINFO(__x__)		AJA_sINFO	(AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)
#define SRDBG(__x__)		AJA_sDEBUG	(AJA_DebugUnit_RoutingGeneric, AJAFUNC << ": " << __x__)

static NTV2StringList & Tokenize (const string & inString, NTV2StringList & outTokens, const string & inDelimiters = " ", bool inTrimEmpty = false)
{
	string::size_type	pos (0),	lastPos (0);
	outTokens.clear ();
	while (true)
	{
		pos = inString.find_first_of (inDelimiters, lastPos);
		if (pos == string::npos)
		{
			pos = inString.length ();
			if (pos != lastPos || !inTrimEmpty)
				outTokens.push_back (NTV2StringList::value_type (inString.data () + lastPos, NTV2StringList::size_type(pos - lastPos)));
			break;
		}
		else
		{
			if (pos != lastPos || !inTrimEmpty)
				outTokens.push_back (NTV2StringList::value_type (inString.data () + lastPos, NTV2StringList::size_type(pos - lastPos)));
		}
		lastPos = pos + 1;
	}
	return outTokens;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// CNTV2SignalRouter	Begin

bool CNTV2SignalRouter::AddConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput)
{
	mConnections.insert (NTV2SignalConnection (inSignalInput, inSignalOutput));
	SRiDBG(NTV2InputCrosspointIDToString(inSignalInput) << ", " << NTV2OutputCrosspointIDToString(inSignalOutput) << ": " << *this);
	return true;
}


bool CNTV2SignalRouter::HasInput (const NTV2InputXptID inSignalInput) const
{
	return mConnections.find (inSignalInput) != mConnections.end ();
}


NTV2OutputXptID CNTV2SignalRouter::GetConnectedOutput (const NTV2InputXptID inSignalInput) const
{
	NTV2XptConnectionsConstIter it(mConnections.find(inSignalInput));
	return it != mConnections.end()	 ?	it->second	:  NTV2_XptBlack;
}


bool CNTV2SignalRouter::HasConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput) const
{
	NTV2XptConnectionsConstIter iter (mConnections.find (inSignalInput));
	if (iter == mConnections.end())
		return false;
	return iter->second == inSignalOutput;
}


bool CNTV2SignalRouter::RemoveConnection (const NTV2InputXptID inSignalInput, const NTV2OutputXptID inSignalOutput)
{
	NTV2XptConnectionsIter	iter (mConnections.find (inSignalInput));
	if (iter == mConnections.end())
		return false;	//	Not in map
	if (iter->second != inSignalOutput)
		return false;	//	No match
	mConnections.erase (iter);
	return true;
}


static const ULWord sSignalRouterRegMasks[]		=	{	0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000	};
static const ULWord sSignalRouterRegShifts[]	=	{			 0,			 8,			16,			24	};


bool CNTV2SignalRouter::ResetFromRegisters (const NTV2InputXptIDSet & inInputs, const NTV2RegisterReads & inRegReads)
{
	Reset();
	for (NTV2InputXptIDSetConstIter it(inInputs.begin());  it != inInputs.end();  ++it)
	{
		uint32_t	regNum(0),	maskNdx(0);
		CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (*it, regNum, maskNdx);
		NTV2RegisterReadsConstIter	iter	(::FindFirstMatchingRegisterNumber(regNum, inRegReads));
		if (iter == inRegReads.end())
			continue;

		NTV2_ASSERT(iter->registerNumber == regNum);
		NTV2_ASSERT(iter->registerMask == 0xFFFFFFFF);
		NTV2_ASSERT(iter->registerShift == 0);
		NTV2_ASSERT(maskNdx < 4);
		const uint32_t	regValue	(iter->registerValue & sSignalRouterRegMasks[maskNdx]);
		const NTV2OutputXptID	outputXpt	(NTV2OutputXptID(regValue >> sSignalRouterRegShifts[maskNdx]));
		if (outputXpt != NTV2_XptBlack)
			mConnections.insert(NTV2SignalConnection (*it, outputXpt));
	}	//	for each NTV2InputXptID
	return true;
}


bool CNTV2SignalRouter::GetRegisterWrites (NTV2RegisterWrites & outRegWrites) const
{
	outRegWrites.clear ();

	for (NTV2XptConnectionsConstIter iter (mConnections.begin ());	iter != mConnections.end ();  ++iter)
	{
		const NTV2InputXptID	inputXpt(iter->first);
		const NTV2OutputXptID	outputXpt(iter->second);
		uint32_t regNum(0), ndx(999);

		if (!CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (inputXpt, regNum, ndx)	||	!regNum	 ||	 ndx > 3)
		{
			outRegWrites.clear();
			return false;
		}

		const NTV2RegInfo	regInfo (regNum, outputXpt, sSignalRouterRegMasks[ndx], sSignalRouterRegShifts[ndx]);
		try
		{
			outRegWrites.push_back (regInfo);
		}
		catch (const bad_alloc &)
		{
			outRegWrites.clear ();
			return false;
		}
	}
	SRiDBG(outRegWrites);
	return true;
}


bool CNTV2SignalRouter::Compare (const CNTV2SignalRouter & inRHS, NTV2XptConnections & outNew,
								NTV2XptConnections & outChanged, NTV2XptConnections & outMissing) const
{
	outNew.clear();	 outChanged.clear();  outMissing.clear();
	//	Check that my connections are also in RHS:
	for (NTV2XptConnectionsConstIter it(mConnections.begin());	it != mConnections.end();  ++it)
	{
		const NTV2SignalConnection &	connection (*it);
		const NTV2InputXptID			inputXpt(connection.first);
		const NTV2OutputXptID			outputXpt(connection.second);
		if (inRHS.HasConnection(inputXpt, outputXpt))
			;
		else if (inRHS.HasInput(inputXpt))
			outChanged.insert(NTV2Connection(inputXpt, inRHS.GetConnectedOutput(inputXpt)));	//	Connection changed from this
		else
			outNew.insert(connection);		//	Connection is new in me, not in RHS
	}

	//	Check that RHS' connections are also in me...
	const NTV2XptConnections	connectionsRHS(inRHS.GetConnections());
	for (NTV2XptConnectionsConstIter it(connectionsRHS.begin());  it != connectionsRHS.end();  ++it)
	{
		const NTV2SignalConnection &	connectionRHS (*it);
		const NTV2InputXptID			inputXpt(connectionRHS.first);
		const NTV2OutputXptID			outputXpt(connectionRHS.second);
		NTV2XptConnectionsConstIter		pFind (mConnections.find(inputXpt));
		if (pFind == mConnections.end())		//	If not found in me...
			outMissing.insert(connectionRHS);	//	...in RHS, but missing in me
		else if (pFind->second != outputXpt)	//	If output xpt differs...
			outChanged.insert(connectionRHS);	//	...then 'connection' is changed (in RHS, not in me)
	}

	return outNew.empty() && outChanged.empty() && outMissing.empty();	//	Return true if identical
}


ostream & CNTV2SignalRouter::Print (ostream & oss, const bool inForRetailDisplay) const
{
	if (inForRetailDisplay)
	{
		oss << mConnections.size() << " routing entries:" << endl;
		for (NTV2XptConnectionsConstIter iter (mConnections.begin());  iter != mConnections.end();	++iter)
			oss << ::NTV2InputCrosspointIDToString(iter->first, inForRetailDisplay)
				<< " <== " << ::NTV2OutputCrosspointIDToString(iter->second, inForRetailDisplay) << endl;
	}
	else
		oss << mConnections;
	return oss;
}


bool CNTV2SignalRouter::PrintCode (string & outCode, const PrintCodeConfig & inConfig) const
{
	return ToCodeString(outCode, mConnections, inConfig);

}	//	PrintCode


bool CNTV2SignalRouter::ToCodeString (string & outCode, const NTV2XptConnections & inConnections,
										const PrintCodeConfig & inConfig)
{
	ostringstream	oss;

	outCode.clear ();

	if (inConfig.mShowComments)
	{
		oss << inConfig.mPreCommentText << DEC(inConnections.size()) << " routing ";
		oss << ((inConnections.size () == 1) ? "entry:" : "entries:");
		oss << inConfig.mPostCommentText << inConfig.mLineBreakText;
	}

	if (inConfig.mShowDeclarations)
	{
		if (inConfig.mUseRouter)
			oss << inConfig.mPreClassText << "CNTV2SignalRouter" << inConfig.mPostClassText
				<< "\t"<< inConfig.mPreVariableText << inConfig.mRouterVarName<< inConfig.mPostVariableText;
		else
			oss << inConfig.mPreClassText << "CNTV2Card" << inConfig.mPostClassText
				<< "\t" << inConfig.mPreVariableText << inConfig.mDeviceVarName<< inConfig.mPostVariableText;
		oss << ";" << inConfig.mLineBreakText;
	}

	const string	varName				(inConfig.mUseRouter ? inConfig.mRouterVarName : inConfig.mDeviceVarName);
	const string	variableNameText	(inConfig.mPreVariableText + varName + inConfig.mPostVariableText);
	const string	funcName			(inConfig.mUseRouter ? "AddConnection" : "Connect");
	const string	functionCallText	(inConfig.mPreFunctionText + funcName + inConfig.mPostFunctionText);
	for (NTV2XptConnectionsConstIter iter (inConnections.begin ());	 iter != inConnections.end ();	++iter)
	{
		const string	inXptStr	(inConfig.mPreXptText + ::NTV2InputCrosspointIDToString(iter->first, false) + inConfig.mPostXptText);
		const string	outXptStr	(inConfig.mPreXptText + ::NTV2OutputCrosspointIDToString(iter->second, false) + inConfig.mPostXptText);

		oss << variableNameText << "." << functionCallText << " (" << inXptStr << ", " << outXptStr << ");";

		if (inConfig.mShowComments)
		{
			NTV2XptConnectionsConstIter pNew(inConfig.mNew.find(iter->first));
			NTV2XptConnectionsConstIter pChanged(inConfig.mChanged.find(iter->first));
			if (pNew != inConfig.mNew.end()	 &&	 pNew->second == iter->second)
				oss << inConfig.mFieldBreakText << inConfig.mPreCommentText << "New" << inConfig.mPostCommentText;
			else if (pChanged != inConfig.mChanged.end()  &&  pChanged->second != iter->second)
				oss << inConfig.mFieldBreakText << inConfig.mPreCommentText << "Changed from "
					<< ::NTV2OutputCrosspointIDToString(pChanged->second, false) << inConfig.mPostCommentText;
		}
		oss << inConfig.mLineBreakText;
	}	//	for each connection

	if (inConfig.mShowComments)
		for (NTV2XptConnectionsConstIter pGone(inConfig.mMissing.begin());	pGone != inConfig.mMissing.end();  ++pGone)
			if (inConnections.find(pGone->first) == inConnections.end())
			{
				if (inConfig.mUseRouter)
					oss << inConfig.mPreCommentText << varName << "." << "RemoveConnection" << " ("
						<< ::NTV2InputCrosspointIDToString(pGone->first, false)
						<< ", " << ::NTV2OutputCrosspointIDToString(pGone->second, false)
						<< ");" << inConfig.mPostCommentText
						<< inConfig.mFieldBreakText << inConfig.mPreCommentText << "Deleted" << inConfig.mPostCommentText
						<< inConfig.mLineBreakText;
				else
					oss << inConfig.mPreCommentText << varName << "." << "Disconnect" << " ("
						<< ::NTV2InputCrosspointIDToString(pGone->first, false)
						<< ");" << inConfig.mPostCommentText << inConfig.mFieldBreakText
						<< inConfig.mPreCommentText
							<< "From " << ::NTV2OutputCrosspointIDToString(pGone->second, false)
						<< inConfig.mPostCommentText << inConfig.mLineBreakText;
			}

	outCode = oss.str();
	return true;
}


CNTV2SignalRouter::PrintCodeConfig::PrintCodeConfig ()
	:	mShowComments		(true),
		mShowDeclarations	(true),
		mUseRouter			(false),
		mPreCommentText		("// "),
		mPostCommentText	(),
		mPreClassText		(),
		mPostClassText		(),
		mPreVariableText	(),
		mPostVariableText	(),
		mPreXptText			(),
		mPostXptText		(),
		mPreFunctionText	(),
		mPostFunctionText	(),
		mDeviceVarName		("device"),
		mRouterVarName		("router"),
		mLineBreakText		("\n"),
		mFieldBreakText		("\t"),
		mNew				(),
		mChanged			(),
		mMissing			()
{
}


bool CNTV2SignalRouter::Initialize (void)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? true : false;
}	//	Initialize


bool CNTV2SignalRouter::Deinitialize (void)		//	STATIC
{
	return RoutingExpert::DisposeInstance();
}


bool CNTV2SignalRouter::IsInitialized (void)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance(false));
	return pExpert ? true : false;
}


string CNTV2SignalRouter::NTV2InputCrosspointIDToString (const NTV2InputXptID inInputXpt)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->InputXptToString(inInputXpt) : string();
}


string CNTV2SignalRouter::NTV2OutputCrosspointIDToString (const NTV2OutputXptID inOutputXpt)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->OutputXptToString(inOutputXpt) : string();
}


NTV2InputXptID CNTV2SignalRouter::StringToNTV2InputCrosspointID (const string & inStr)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->StringToInputXpt(inStr) : NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID CNTV2SignalRouter::StringToNTV2OutputCrosspointID (const string & inStr)		//	STATIC
{
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->StringToOutputXpt(inStr) : NTV2_OUTPUT_CROSSPOINT_INVALID;
}


bool CNTV2SignalRouter::GetWidgetIDs (const NTV2DeviceID inDeviceID, NTV2WidgetIDSet & outWidgets)		//	STATIC
{
	outWidgets.clear();
	for (NTV2WidgetID widgetID(NTV2WidgetID(NTV2_WIDGET_FIRST));  NTV2_IS_VALID_WIDGET(widgetID);  widgetID = NTV2WidgetID(widgetID+1))
		if (::NTV2DeviceCanDoWidget (inDeviceID, widgetID))
			outWidgets.insert(widgetID);
	return !outWidgets.empty();
}


bool CNTV2SignalRouter::GetWidgetsForInput (const NTV2InputXptID inInputXpt, NTV2WidgetIDSet & outWidgetIDs)		//	STATIC
{
	outWidgetIDs.clear();
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->GetWidgetsForInput(inInputXpt, outWidgetIDs) : false;
}


bool CNTV2SignalRouter::GetWidgetForInput (const NTV2InputXptID inInputXpt, NTV2WidgetID & outWidgetID, const NTV2DeviceID inDeviceID)		//	STATIC
{
	outWidgetID = NTV2_WIDGET_INVALID;
	NTV2WidgetIDSet wgts;
	if (!GetWidgetsForInput(inInputXpt, wgts))
		return false;
	if (inDeviceID == DEVICE_ID_NOTFOUND)
		outWidgetID = *(wgts.begin());
	else
		for (NTV2WidgetIDSetConstIter it(wgts.begin());	 it != wgts.end();	++it)
			if (::NTV2DeviceCanDoWidget(inDeviceID, *it))
			{
				outWidgetID = *it;
				break;
			}
	return outWidgetID != NTV2_WIDGET_INVALID;
}


bool CNTV2SignalRouter::GetWidgetsForOutput (const NTV2OutputXptID inOutputXpt, NTV2WidgetIDSet & outWidgetIDs)		//	STATIC
{
	outWidgetIDs.clear();
	AJAAutoLock		locker(&gRoutingExpertLock);
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->GetWidgetsForOutput(inOutputXpt, outWidgetIDs) : false;
}


bool CNTV2SignalRouter::GetWidgetForOutput (const NTV2OutputXptID inOutputXpt, NTV2WidgetID & outWidgetID, const NTV2DeviceID inDeviceID)		//	STATIC
{
	outWidgetID = NTV2_WIDGET_INVALID;
	NTV2WidgetIDSet wgts;
	{
		AJAAutoLock		locker(&gRoutingExpertLock);
		if (!GetWidgetsForOutput(inOutputXpt, wgts))
			return false;
	}
	if (inDeviceID == DEVICE_ID_NOTFOUND)
		outWidgetID = *(wgts.begin());
	else
		for (NTV2WidgetIDSetConstIter it(wgts.begin());	 it != wgts.end();	++it)
			if (::NTV2DeviceCanDoWidget(inDeviceID, *it))
			{
				outWidgetID = *it;
				break;
			}
	return outWidgetID != NTV2_WIDGET_INVALID;
}


bool CNTV2SignalRouter::GetWidgetInputs (const NTV2WidgetID inWidgetID, NTV2InputXptIDSet & outInputs)		//	STATIC
{
	outInputs.clear();
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->GetWidgetInputs(inWidgetID, outInputs) : false;
}


bool CNTV2SignalRouter::GetAllWidgetInputs (const NTV2DeviceID inDeviceID, NTV2InputXptIDSet & outInputs)		//	STATIC
{
	outInputs.clear();
	NTV2WidgetIDSet widgetIDs;
	if (!GetWidgetIDs (inDeviceID, widgetIDs))
		return false;	//	Fail

	for (NTV2WidgetIDSetConstIter iter(widgetIDs.begin());	iter != widgetIDs.end ();  ++iter)
	{
		NTV2InputXptIDSet inputs;
		CNTV2SignalRouter::GetWidgetInputs (*iter, inputs);
		for (NTV2InputXptIDSetConstIter it(inputs.begin());  it != inputs.end();  ++it)
		{
			if (WidgetIDToType(*iter) == NTV2WidgetType_FrameStore)
				if (!::NTV2DeviceCanDo425Mux(inDeviceID))
					if (!::NTV2DeviceCanDo8KVideo(inDeviceID))
						if (::NTV2InputCrosspointIDToString(*it, false).find("DS2") != string::npos)	//	is DS2 input?
							continue;	//	do not include FrameStore DS2 inputs for IP25G
			outInputs.insert(*it);
		}
	}
	return true;
}


bool CNTV2SignalRouter::GetAllRoutingRegInfos (const NTV2InputXptIDSet & inInputs, NTV2RegisterWrites & outRegInfos)	//	STATIC
{
	outRegInfos.clear();

	set<uint32_t>	regNums;
	uint32_t		regNum(0),	maskNdx(0);
	for (NTV2InputXptIDSetConstIter it(inInputs.begin());  it != inInputs.end();  ++it)
		if (CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (*it, regNum, maskNdx))
			if (regNums.find(regNum) == regNums.end())
				regNums.insert(regNum);
	for (set<uint32_t>::const_iterator iter(regNums.begin());  iter != regNums.end();  ++iter)
		outRegInfos.push_back(NTV2RegInfo(*iter));

	return true;
}


bool CNTV2SignalRouter::GetWidgetOutputs (const NTV2WidgetID inWidgetID, NTV2OutputXptIDSet & outOutputs)		//	STATIC
{
	outOutputs.clear();
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->GetWidgetOutputs(inWidgetID, outOutputs) : false;
}

bool CNTV2SignalRouter::GetAllWidgetOutputs (const NTV2DeviceID inDeviceID, NTV2OutputXptIDSet & outOutputs)	//	STATIC
{
	outOutputs.clear();
	NTV2WidgetIDSet widgetIDs;
	if (!GetWidgetIDs (inDeviceID, widgetIDs))
		return false;	//	Fail

	for (NTV2WidgetIDSetConstIter iter(widgetIDs.begin());	iter != widgetIDs.end ();  ++iter)
	{
		NTV2OutputXptIDSet outputs;
		CNTV2SignalRouter::GetWidgetOutputs (*iter, outputs);
		for (NTV2OutputXptIDSetConstIter it(outputs.begin());  it != outputs.end();  ++it)
		{
			if (WidgetIDToType(*iter) == NTV2WidgetType_FrameStore)
				if (!::NTV2DeviceCanDo425Mux(inDeviceID))
					if (!::NTV2DeviceCanDo8KVideo(inDeviceID))
						if (::NTV2OutputCrosspointIDToString(*it, false).find("DS2") != string::npos)	//	is DS2 output?
							continue;	//	do not include FrameStore DS2 outputs for IP25G
			outOutputs.insert(*it);
		}
	}
	return true;
}

bool CNTV2SignalRouter::IsRGBOnlyInputXpt (const NTV2InputXptID inInputXpt)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsRGBOnlyInputXpt(inInputXpt) : false;
}

bool CNTV2SignalRouter::IsYUVOnlyInputXpt (const NTV2InputXptID inInputXpt)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsYUVOnlyInputXpt(inInputXpt) : false;
}

bool CNTV2SignalRouter::IsKeyInputXpt (const NTV2InputXptID inInputXpt)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsKeyInputXpt(inInputXpt) : false;
}

NTV2Channel CNTV2SignalRouter::WidgetIDToChannel(const NTV2WidgetID inWidgetID)
{
	RoutingExpertPtr pExpert(RoutingExpert::GetInstance());
	if (pExpert)
		return pExpert->WidgetIDToChannel(inWidgetID);
	return NTV2_CHANNEL_INVALID;
}

NTV2WidgetID CNTV2SignalRouter::WidgetIDFromTypeAndChannel(const NTV2WidgetType inWidgetType, const NTV2Channel inChannel)
{
	RoutingExpertPtr pExpert(RoutingExpert::GetInstance());
	if (pExpert)
		return pExpert->WidgetIDFromTypeAndChannel(inWidgetType, inChannel);
	return NTV2_WIDGET_INVALID;
}

NTV2WidgetType CNTV2SignalRouter::WidgetIDToType (const NTV2WidgetID inWidgetID)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	if (pExpert)
		return pExpert->WidgetIDToType(inWidgetID);
	return NTV2WidgetType_Invalid;
}

bool CNTV2SignalRouter::IsSDIWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsSDIWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsSDIInputWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsSDIInWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsSDIOutputWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsSDIOutWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::Is3GSDIWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->Is3GSDIWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::Is12GSDIWidgetType (const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->Is12GSDIWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsDualLinkWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsDualLinkWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsDualLinkInWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsDualLinkInWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsDualLinkOutWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsDualLinkOutWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsHDMIWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsHDMIWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsHDMIInWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsHDMIInWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::IsHDMIOutWidgetType(const NTV2WidgetType inWidgetType)
{
	RoutingExpertPtr	pExpert(RoutingExpert::GetInstance());
	return pExpert ? pExpert->IsHDMIOutWidget(inWidgetType) : false;
}

bool CNTV2SignalRouter::GetConnectionsFromRegs (const NTV2InputXptIDSet & inInputXptIDs, const NTV2RegisterReads & inRegValues, NTV2XptConnections & outConnections)
{
	outConnections.clear();
	for (NTV2InputXptIDSetConstIter it(inInputXptIDs.begin());	it != inInputXptIDs.end();	++it)
	{
		uint32_t	regNum(0),	maskNdx(0);
		CNTV2RegisterExpert::GetCrosspointSelectGroupRegisterInfo (*it, regNum, maskNdx);
		NTV2RegisterReadsConstIter	iter	(::FindFirstMatchingRegisterNumber(regNum, inRegValues));
		if (iter == inRegValues.end())
			continue;

		if (iter->registerNumber != regNum)
			return false;	//	Register numbers must match here
		if (iter->registerMask != 0xFFFFFFFF)
			return false;	//	Mask must be 0xFFFFFFFF
		if (iter->registerShift)
			return false;	//	Shift must be zero
		NTV2_ASSERT(maskNdx < 4);
		const uint32_t	regValue	(iter->registerValue & sSignalRouterRegMasks[maskNdx]);
		const NTV2OutputXptID	outputXpt	(NTV2OutputXptID(regValue >> sSignalRouterRegShifts[maskNdx]));
		if (outputXpt != NTV2_XptBlack)
			outConnections.insert(NTV2SignalConnection (*it, outputXpt));
	}	//	for each NTV2InputXptID
	return true;
}


bool CNTV2SignalRouter::CompareConnections (const NTV2XptConnections & inLHS,
											const NTV2XptConnections & inRHS,
											NTV2XptConnections & outNew,
											NTV2XptConnections & outMissing)
{
	outNew.clear();	 outMissing.clear();
	//	Check that LHS connections are also in RHS:
	for (NTV2XptConnectionsConstIter it(inLHS.begin());	 it != inLHS.end();	 ++it)
	{
		const NTV2SignalConnection &	LHSconnection(*it);
		const NTV2InputXptID			inputXpt(LHSconnection.first);
		const NTV2OutputXptID			outputXpt(LHSconnection.second);
		NTV2XptConnectionsConstIter		RHSit(inRHS.find(inputXpt));
		if (RHSit == inRHS.end())
			outMissing.insert(LHSconnection);	//	LHSConnection's inputXpt missing from RHS
		else if (RHSit->second == outputXpt)
			;	//	LHS's input xpt connected to same output xpt as RHS
		else
		{
			outMissing.insert(LHSconnection);	//	LHS connection missing from RHS
			outNew.insert(*RHSit);				//	RHS connection is new
		}
	}

	//	Check that RHS connections are also in LHS...
	for (NTV2XptConnectionsConstIter it(inRHS.begin());	 it != inRHS.end();	 ++it)
	{
		const NTV2SignalConnection &	connectionRHS (*it);
		const NTV2InputXptID			inputXpt(connectionRHS.first);
		const NTV2OutputXptID			outputXpt(connectionRHS.second);
		NTV2XptConnectionsConstIter		LHSit(inLHS.find(inputXpt));
		if (LHSit == inLHS.end())				//	If RHS input xpt not in LHS...
			outNew.insert(connectionRHS);		//	...then RHS connection is new
		else if (LHSit->second != outputXpt)	//	Else if output xpt changed...
			//	Should've already been handled in previous for loop
			NTV2_ASSERT(outMissing.find(LHSit->first) != outMissing.end()  &&  outNew.find(LHSit->first) != outNew.end());
	}

	return outNew.empty() && outMissing.empty();	//	Return true if identical
}


bool CNTV2SignalRouter::CreateFromString (const string & inString, NTV2XptConnections & outConnections) //	STATIC
{
	NTV2StringList	lines;
	string	stringToParse(inString);	aja::strip(aja::lower(stringToParse));
	aja::replace(stringToParse, " ", "");
	aja::replace(stringToParse, "\t", "");
	aja::replace(stringToParse, "&lt;","<");	//	in case uuencoded

	outConnections.clear();
	if (Tokenize(stringToParse, lines, "\n\r", true).empty())	//	Split the string at line breaks
	{
		SRWARN("No lines resulted from input string '" << stringToParse << "'");
		return true;	//	Nothing there
	}

	if (lines.front().find("<==") != string::npos)
	{
//		SRDBG(lines.size() << " lines");
		for (NTV2StringListConstIter pEachLine(lines.begin());	pEachLine != lines.end();  ++pEachLine)
		{
//			SRDBG("	 line '" << *pEachLine << "'");
			size_t	pos (pEachLine->find("<=="));
			if (pos == string::npos)
				{SRFAIL("Parse error: '<==' missing in line '" << *pEachLine << "'");  return false;}
			string	leftPiece	(pEachLine->substr(0, pos));	aja::strip(leftPiece);
			string	rightPiece	(pEachLine->substr(pos + 3, pEachLine->length()));	aja::strip(rightPiece);
			NTV2InputXptID	inputXpt	(StringToNTV2InputCrosspointID(leftPiece));
			NTV2OutputXptID outputXpt	(StringToNTV2OutputCrosspointID(rightPiece));
			//SRDBG(" L'" << leftPiece << "',  R'" << rightPiece << "'");
			if (inputXpt == NTV2_INPUT_CROSSPOINT_INVALID)
				{SRFAIL("Parse error: invalid input crosspoint from '" << leftPiece << "' from line '" << *pEachLine << "'");	return false;}
			if (outConnections.find(inputXpt) != outConnections.end())
				SRWARN("Overwriting " << ::NTV2InputCrosspointIDToString(inputXpt) << "-" << ::NTV2OutputCrosspointIDToString(outConnections[inputXpt])
						<< " with " << ::NTV2InputCrosspointIDToString(inputXpt) << "-" << ::NTV2OutputCrosspointIDToString(outputXpt));
			outConnections.insert(NTV2Connection(inputXpt, outputXpt));
		}	//	for each line
	}
	else if (lines.front().find("connect(") != string::npos)
	{
		for (NTV2StringListConstIter pLine(lines.begin());	pLine != lines.end();  ++pLine)
		{
			string line(*pLine);  aja::strip(line);
			if (line.empty())
				continue;
			if (line.find("//") == 0)	//	starts with "//"
				continue;
//			SRDBG("	 line '" << line << "'");
			size_t	openParenPos(line.find("(")), closedParenPos(line.find(");"));
			if (openParenPos == string::npos  ||  closedParenPos == string::npos  ||  openParenPos > closedParenPos)
				{SRFAIL("Parse error: '(' or ');' missing in line '" << line << "'");  return false;}
			string remainder(line.substr(openParenPos+1, closedParenPos - openParenPos - 1));
			NTV2StringList xptNames;
			aja::split(remainder, ',', xptNames);
			if (xptNames.size() < 2	 ||	 xptNames.size() > 2)
				{SRFAIL("Parse error: " << DEC(xptNames.size()) << " 'Connect' parameter(s) found, expected 2");  return false;}
			NTV2InputXptID	inputXpt (StringToNTV2InputCrosspointID(xptNames.at(0)));
			NTV2OutputXptID outputXpt (StringToNTV2OutputCrosspointID(xptNames.at(1)));
			//SRDBG(" L'" << xptNames.at(0) << "',	R'" << xptNames.at(1) << "'");
			if (inputXpt == NTV2_INPUT_CROSSPOINT_INVALID)
				{SRFAIL("Parse error: invalid input crosspoint from '" << xptNames.at(0) << "' from line '" << *pLine << "'");	return false;}
			if (outputXpt == NTV2_OUTPUT_CROSSPOINT_INVALID)
				{SRFAIL("Parse error: invalid output crosspoint from '" << xptNames.at(1) << "' from line '" << *pLine << "'");	 return false;}
			if (outConnections.find(inputXpt) != outConnections.end())
				SRWARN("Overwriting " << ::NTV2InputCrosspointIDToString(inputXpt) << "-" << ::NTV2OutputCrosspointIDToString(outConnections[inputXpt])
						<< " with " << ::NTV2InputCrosspointIDToString(inputXpt) << "-" << ::NTV2OutputCrosspointIDToString(outputXpt));
			outConnections.insert(NTV2Connection(inputXpt, outputXpt));
		}	//	for each line
	}
	else
		{SRFAIL("Unable to parse '" << lines.front() << "' -- expected '.contains(' or '<=='");	 return false;}
	SRINFO(DEC(outConnections.size()) << " connection(s) created from input string");
	return true;
}


bool CNTV2SignalRouter::CreateFromString (const string & inString, CNTV2SignalRouter & outRouter)	//	STATIC
{
	NTV2XptConnections	connections;
	outRouter.Reset();
	if (!CreateFromString(inString, connections))
		return false;
	return outRouter.ResetFrom(connections);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// CNTV2SignalRouter	End



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Crosspoint Utils Begin

NTV2InputXptID GetFrameStoreInputXptFromChannel (const NTV2Channel inChannel, const bool inIsBInput)
{
	static const NTV2InputXptID gFrameBufferInputs []	=	{	NTV2_XptFrameBuffer1Input,	NTV2_XptFrameBuffer2Input,	NTV2_XptFrameBuffer3Input,	NTV2_XptFrameBuffer4Input,
																NTV2_XptFrameBuffer5Input,	NTV2_XptFrameBuffer6Input,	NTV2_XptFrameBuffer7Input,	NTV2_XptFrameBuffer8Input};
	static const NTV2InputXptID gFrameBufferBInputs []	=	{	NTV2_XptFrameBuffer1DS2Input, NTV2_XptFrameBuffer2DS2Input, NTV2_XptFrameBuffer3DS2Input, NTV2_XptFrameBuffer4DS2Input,
																NTV2_XptFrameBuffer5DS2Input, NTV2_XptFrameBuffer6DS2Input, NTV2_XptFrameBuffer7DS2Input, NTV2_XptFrameBuffer8DS2Input};
	if (NTV2_IS_VALID_CHANNEL (inChannel))
		return inIsBInput ? gFrameBufferBInputs [inChannel] : gFrameBufferInputs [inChannel];
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetCSCInputXptFromChannel (const NTV2Channel inChannel, const bool inIsKeyInput)
{
	static const NTV2InputXptID gCSCVideoInput [] = {	NTV2_XptCSC1VidInput,	NTV2_XptCSC2VidInput,	NTV2_XptCSC3VidInput,	NTV2_XptCSC4VidInput,
														NTV2_XptCSC5VidInput,	NTV2_XptCSC6VidInput,	NTV2_XptCSC7VidInput,	NTV2_XptCSC8VidInput};
	static const NTV2InputXptID gCSCKeyInput [] = {		NTV2_XptCSC1KeyInput,	NTV2_XptCSC2KeyInput,	NTV2_XptCSC3KeyInput,	NTV2_XptCSC4KeyInput,
														NTV2_XptCSC5KeyInput,	NTV2_XptCSC6KeyInput,	NTV2_XptCSC7KeyInput,	NTV2_XptCSC8KeyInput};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inIsKeyInput ? gCSCKeyInput[inChannel] : gCSCVideoInput[inChannel];
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetLUTInputXptFromChannel (const NTV2Channel inLUT)
{
	static const NTV2InputXptID gLUTInput[] = { NTV2_XptLUT1Input,	NTV2_XptLUT2Input,	NTV2_XptLUT3Input,	NTV2_XptLUT4Input,
												NTV2_XptLUT5Input,	NTV2_XptLUT6Input,	NTV2_XptLUT7Input,	NTV2_XptLUT8Input};
	return NTV2_IS_VALID_CHANNEL(inLUT) ? gLUTInput[inLUT] : NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetDLInInputXptFromChannel (const NTV2Channel inChannel, const bool inLinkB)
{
	static const NTV2InputXptID gDLInputs[] = { NTV2_XptDualLinkIn1Input, NTV2_XptDualLinkIn2Input, NTV2_XptDualLinkIn3Input, NTV2_XptDualLinkIn4Input,
												NTV2_XptDualLinkIn5Input, NTV2_XptDualLinkIn6Input, NTV2_XptDualLinkIn7Input, NTV2_XptDualLinkIn8Input };
	static const NTV2InputXptID gDLBInputs[] = {NTV2_XptDualLinkIn1DSInput, NTV2_XptDualLinkIn2DSInput, NTV2_XptDualLinkIn3DSInput, NTV2_XptDualLinkIn4DSInput,
												NTV2_XptDualLinkIn5DSInput, NTV2_XptDualLinkIn6DSInput, NTV2_XptDualLinkIn7DSInput, NTV2_XptDualLinkIn8DSInput };
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inLinkB ? gDLBInputs[inChannel] : gDLInputs[inChannel];
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}

NTV2InputXptID GetDLOutInputXptFromChannel (const NTV2Channel inChannel)
{
	static const NTV2InputXptID gDLOutInputs[] = {	NTV2_XptDualLinkOut1Input, NTV2_XptDualLinkOut2Input, NTV2_XptDualLinkOut3Input, NTV2_XptDualLinkOut4Input,
													NTV2_XptDualLinkOut5Input, NTV2_XptDualLinkOut6Input, NTV2_XptDualLinkOut7Input, NTV2_XptDualLinkOut8Input };
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return gDLOutInputs[inChannel];
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID GetCSCOutputXptFromChannel (const NTV2Channel inChannel, const bool inIsKey, const bool inIsRGB)
{
	static const NTV2OutputXptID gCSCKeyOutputs [] = {	NTV2_XptCSC1KeyYUV,		NTV2_XptCSC2KeyYUV,		NTV2_XptCSC3KeyYUV,		NTV2_XptCSC4KeyYUV,
														NTV2_XptCSC5KeyYUV,		NTV2_XptCSC6KeyYUV,		NTV2_XptCSC7KeyYUV,		NTV2_XptCSC8KeyYUV};
	static const NTV2OutputXptID gCSCRGBOutputs [] = {	NTV2_XptCSC1VidRGB,		NTV2_XptCSC2VidRGB,		NTV2_XptCSC3VidRGB,		NTV2_XptCSC4VidRGB,
														NTV2_XptCSC5VidRGB,		NTV2_XptCSC6VidRGB,		NTV2_XptCSC7VidRGB,		NTV2_XptCSC8VidRGB};
	static const NTV2OutputXptID gCSCYUVOutputs [] = {	NTV2_XptCSC1VidYUV,		NTV2_XptCSC2VidYUV,		NTV2_XptCSC3VidYUV,		NTV2_XptCSC4VidYUV,
														NTV2_XptCSC5VidYUV,		NTV2_XptCSC6VidYUV,		NTV2_XptCSC7VidYUV,		NTV2_XptCSC8VidYUV};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
	{
		if (inIsKey)
			return gCSCKeyOutputs[inChannel];
		else
			return inIsRGB	?  gCSCRGBOutputs[inChannel]  :	 gCSCYUVOutputs[inChannel];
	}
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetLUTOutputXptFromChannel (const NTV2Channel inLUT)
{
	static const NTV2OutputXptID gLUTRGBOutputs[] = {	NTV2_XptLUT1Out, NTV2_XptLUT2Out, NTV2_XptLUT3Out, NTV2_XptLUT4Out,
														NTV2_XptLUT5Out, NTV2_XptLUT6Out, NTV2_XptLUT7Out, NTV2_XptLUT8Out};
	return NTV2_IS_VALID_CHANNEL(inLUT) ? gLUTRGBOutputs[inLUT] : NTV2_OUTPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetFrameStoreOutputXptFromChannel (const NTV2Channel inChannel, const bool inIsRGB, const bool inIs425)
{
	static const NTV2OutputXptID gFrameBufferYUVOutputs[] = {		NTV2_XptFrameBuffer1YUV,		NTV2_XptFrameBuffer2YUV,		NTV2_XptFrameBuffer3YUV,		NTV2_XptFrameBuffer4YUV,
																	NTV2_XptFrameBuffer5YUV,		NTV2_XptFrameBuffer6YUV,		NTV2_XptFrameBuffer7YUV,		NTV2_XptFrameBuffer8YUV};
	static const NTV2OutputXptID gFrameBufferRGBOutputs[] = {		NTV2_XptFrameBuffer1RGB,		NTV2_XptFrameBuffer2RGB,		NTV2_XptFrameBuffer3RGB,		NTV2_XptFrameBuffer4RGB,
																	NTV2_XptFrameBuffer5RGB,		NTV2_XptFrameBuffer6RGB,		NTV2_XptFrameBuffer7RGB,		NTV2_XptFrameBuffer8RGB};
	static const NTV2OutputXptID gFrameBufferYUV425Outputs[] = {	NTV2_XptFrameBuffer1_DS2YUV,	NTV2_XptFrameBuffer2_DS2YUV,	NTV2_XptFrameBuffer3_DS2YUV,	NTV2_XptFrameBuffer4_DS2YUV,
																	NTV2_XptFrameBuffer5_DS2YUV,	NTV2_XptFrameBuffer6_DS2YUV,	NTV2_XptFrameBuffer7_DS2YUV,	NTV2_XptFrameBuffer8_DS2YUV};
	static const NTV2OutputXptID gFrameBufferRGB425Outputs[] = {	NTV2_XptFrameBuffer1_DS2RGB,	NTV2_XptFrameBuffer2_DS2RGB,	NTV2_XptFrameBuffer3_DS2RGB,	NTV2_XptFrameBuffer4_DS2RGB,
																	NTV2_XptFrameBuffer5_DS2RGB,	NTV2_XptFrameBuffer6_DS2RGB,	NTV2_XptFrameBuffer7_DS2RGB,	NTV2_XptFrameBuffer8_DS2RGB};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		if (inIs425)
			return inIsRGB ? gFrameBufferRGB425Outputs[inChannel] : gFrameBufferYUV425Outputs[inChannel];
		else
			return inIsRGB ? gFrameBufferRGBOutputs[inChannel] : gFrameBufferYUVOutputs[inChannel];
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID GetInputSourceOutputXpt (const NTV2InputSource inInputSource, const bool inIsSDI_DS2, const bool inIsHDMI_RGB, const UWord inHDMI_Quadrant)
{
	static const NTV2OutputXptID gHDMIInputOutputs [4][4] =		{	{	NTV2_XptHDMIIn1,	NTV2_XptHDMIIn1Q2,		NTV2_XptHDMIIn1Q3,		NTV2_XptHDMIIn1Q4		},
																	{	NTV2_XptHDMIIn2,	NTV2_XptHDMIIn2Q2,		NTV2_XptHDMIIn2Q3,		NTV2_XptHDMIIn2Q4		},
																	{	NTV2_XptHDMIIn3,	NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack			},
																	{	NTV2_XptHDMIIn4,	NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack			}	};
	static const NTV2OutputXptID gHDMIInputRGBOutputs [4][4] =	{	{	NTV2_XptHDMIIn1RGB, NTV2_XptHDMIIn1Q2RGB,	NTV2_XptHDMIIn1Q3RGB,	NTV2_XptHDMIIn1Q4RGB	},
																	{	NTV2_XptHDMIIn2RGB, NTV2_XptHDMIIn2Q2RGB,	NTV2_XptHDMIIn2Q3RGB,	NTV2_XptHDMIIn2Q4RGB	},
																	{	NTV2_XptHDMIIn3RGB, NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack			},
																	{	NTV2_XptHDMIIn4RGB, NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack			}	};

	if (NTV2_INPUT_SOURCE_IS_SDI (inInputSource))
		return ::GetSDIInputOutputXptFromChannel (::NTV2InputSourceToChannel (inInputSource), inIsSDI_DS2);
	else if (NTV2_INPUT_SOURCE_IS_HDMI (inInputSource))
	{
		NTV2Channel channel = ::NTV2InputSourceToChannel (inInputSource);
		if (inHDMI_Quadrant < 4)
			return inIsHDMI_RGB	 ?	gHDMIInputRGBOutputs [channel][inHDMI_Quadrant]	 :	gHDMIInputOutputs [channel][inHDMI_Quadrant];
		else
			return NTV2_OUTPUT_CROSSPOINT_INVALID;
	}
	else if (NTV2_INPUT_SOURCE_IS_ANALOG (inInputSource))
		return NTV2_XptAnalogIn;
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID GetSDIInputOutputXptFromChannel (const NTV2Channel inChannel,  const bool inIsDS2)
{
	static const NTV2OutputXptID gSDIInputOutputs []	=	{	NTV2_XptSDIIn1,		NTV2_XptSDIIn2,		NTV2_XptSDIIn3,		NTV2_XptSDIIn4,
																NTV2_XptSDIIn5,		NTV2_XptSDIIn6,		NTV2_XptSDIIn7,		NTV2_XptSDIIn8};
	static const NTV2OutputXptID gSDIInputDS2Outputs [] =	{	NTV2_XptSDIIn1DS2,	NTV2_XptSDIIn2DS2,	NTV2_XptSDIIn3DS2,	NTV2_XptSDIIn4DS2,
																NTV2_XptSDIIn5DS2,	NTV2_XptSDIIn6DS2,	NTV2_XptSDIIn7DS2,	NTV2_XptSDIIn8DS2};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inIsDS2	?  gSDIInputDS2Outputs[inChannel]  :  gSDIInputOutputs[inChannel];
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetDLOutOutputXptFromChannel(const NTV2Channel inChannel, const bool inIsLinkB)
{
	static const NTV2OutputXptID gDLOutOutputs[] = {	NTV2_XptDuallinkOut1, NTV2_XptDuallinkOut2, NTV2_XptDuallinkOut3, NTV2_XptDuallinkOut4,
														NTV2_XptDuallinkOut5, NTV2_XptDuallinkOut6, NTV2_XptDuallinkOut7, NTV2_XptDuallinkOut8 };
	static const NTV2OutputXptID gDLOutDS2Outputs[] = { NTV2_XptDuallinkOut1DS2, NTV2_XptDuallinkOut2DS2, NTV2_XptDuallinkOut3DS2, NTV2_XptDuallinkOut4DS2,
														NTV2_XptDuallinkOut5DS2, NTV2_XptDuallinkOut6DS2, NTV2_XptDuallinkOut7DS2, NTV2_XptDuallinkOut8DS2 };
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inIsLinkB ? gDLOutDS2Outputs[inChannel] : gDLOutOutputs[inChannel];
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetDLInOutputXptFromChannel(const NTV2Channel inChannel)
{
	static const NTV2OutputXptID gDLInOutputs[] = { NTV2_XptDuallinkIn1, NTV2_XptDuallinkIn2, NTV2_XptDuallinkIn3, NTV2_XptDuallinkIn4,
													NTV2_XptDuallinkIn5, NTV2_XptDuallinkIn6, NTV2_XptDuallinkIn7, NTV2_XptDuallinkIn8 };
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return gDLInOutputs[inChannel];
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetOutputDestInputXpt (const NTV2OutputDestination inOutputDest,	 const bool inIsSDI_DS2,  const UWord inHDMI_Quadrant)
{
	static const NTV2InputXptID gHDMIOutputInputs[] = { NTV2_XptHDMIOutQ1Input, NTV2_XptHDMIOutQ2Input, NTV2_XptHDMIOutQ3Input, NTV2_XptHDMIOutQ4Input};
	if (NTV2_OUTPUT_DEST_IS_SDI(inOutputDest))
		return ::GetSDIOutputInputXpt (::NTV2OutputDestinationToChannel(inOutputDest), inIsSDI_DS2);
	else if (NTV2_OUTPUT_DEST_IS_HDMI(inOutputDest))
		return inHDMI_Quadrant > 3	?  NTV2_XptHDMIOutInput :  gHDMIOutputInputs[inHDMI_Quadrant];
	else if (NTV2_OUTPUT_DEST_IS_ANALOG(inOutputDest))
		return NTV2_XptAnalogOutInput;
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetSDIOutputInputXpt (const NTV2Channel inChannel,  const bool inIsDS2)
{
	static const NTV2InputXptID gSDIOutputInputs []		=	{	NTV2_XptSDIOut1Input,		NTV2_XptSDIOut2Input,		NTV2_XptSDIOut3Input,		NTV2_XptSDIOut4Input,
																NTV2_XptSDIOut5Input,		NTV2_XptSDIOut6Input,		NTV2_XptSDIOut7Input,		NTV2_XptSDIOut8Input};
	static const NTV2InputXptID gSDIOutputDS2Inputs []	=	{	NTV2_XptSDIOut1InputDS2,	NTV2_XptSDIOut2InputDS2,	NTV2_XptSDIOut3InputDS2,	NTV2_XptSDIOut4InputDS2,
																NTV2_XptSDIOut5InputDS2,	NTV2_XptSDIOut6InputDS2,	NTV2_XptSDIOut7InputDS2,	NTV2_XptSDIOut8InputDS2};
	if (NTV2_IS_VALID_CHANNEL (inChannel))
		return inIsDS2	?  gSDIOutputDS2Inputs [inChannel]	:  gSDIOutputInputs [inChannel];
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2OutputXptID GetMixerOutputXptFromChannel (const NTV2Channel inChannel, const bool inIsKey)
{
	static const NTV2OutputXptID gMixerVidYUVOutputs [] = { NTV2_XptMixer1VidYUV,	NTV2_XptMixer1VidYUV,	NTV2_XptMixer2VidYUV,	NTV2_XptMixer2VidYUV,
															NTV2_XptMixer3VidYUV,	NTV2_XptMixer3VidYUV,	NTV2_XptMixer4VidYUV,	NTV2_XptMixer4VidYUV};
	static const NTV2OutputXptID gMixerKeyYUVOutputs [] = { NTV2_XptMixer1KeyYUV,	NTV2_XptMixer1KeyYUV,	NTV2_XptMixer2KeyYUV,	NTV2_XptMixer2KeyYUV,
															NTV2_XptMixer3KeyYUV,	NTV2_XptMixer3KeyYUV,	NTV2_XptMixer4KeyYUV,	NTV2_XptMixer4KeyYUV};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inIsKey	?  gMixerKeyYUVOutputs[inChannel]  :  gMixerVidYUVOutputs[inChannel];
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetMixerFGInputXpt (const NTV2Channel inChannel,	 const bool inIsKey)
{
	static const NTV2InputXptID gMixerFGVideoInputs []	= { NTV2_XptMixer1FGVidInput,	NTV2_XptMixer1FGVidInput,	NTV2_XptMixer2FGVidInput,	NTV2_XptMixer2FGVidInput,
															NTV2_XptMixer3FGVidInput,	NTV2_XptMixer3FGVidInput,	NTV2_XptMixer4FGVidInput,	NTV2_XptMixer4FGVidInput};
	static const NTV2InputXptID gMixerFGKeyInputs []	= { NTV2_XptMixer1FGKeyInput,	NTV2_XptMixer1FGKeyInput,	NTV2_XptMixer2FGKeyInput,	NTV2_XptMixer2FGKeyInput,
															NTV2_XptMixer3FGKeyInput,	NTV2_XptMixer3FGKeyInput,	NTV2_XptMixer4FGKeyInput,	NTV2_XptMixer4FGKeyInput};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inIsKey	?  gMixerFGKeyInputs[inChannel]	 :	gMixerFGVideoInputs[inChannel];
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}


NTV2InputXptID GetMixerBGInputXpt (const NTV2Channel inChannel,	 const bool inIsKey)
{
	static const NTV2InputXptID gMixerBGVideoInputs []	= { NTV2_XptMixer1BGVidInput,	NTV2_XptMixer1BGVidInput,	NTV2_XptMixer2BGVidInput,	NTV2_XptMixer2BGVidInput,
															NTV2_XptMixer3BGVidInput,	NTV2_XptMixer3BGVidInput,	NTV2_XptMixer4BGVidInput,	NTV2_XptMixer4BGVidInput};
	static const NTV2InputXptID gMixerBGKeyInputs []	= { NTV2_XptMixer1BGKeyInput,	NTV2_XptMixer1BGKeyInput,	NTV2_XptMixer2BGKeyInput,	NTV2_XptMixer2BGKeyInput,
															NTV2_XptMixer3BGKeyInput,	NTV2_XptMixer3BGKeyInput,	NTV2_XptMixer4BGKeyInput,	NTV2_XptMixer4BGKeyInput};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inIsKey	?  gMixerBGKeyInputs[inChannel]	 :	gMixerBGVideoInputs[inChannel];
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}

NTV2InputXptID GetTSIMuxInputXptFromChannel (const NTV2Channel inChannel, const bool inLinkB)
{
	static const NTV2InputXptID gDLInputs[] = { NTV2_Xpt425Mux1AInput, NTV2_Xpt425Mux2AInput, NTV2_Xpt425Mux3AInput, NTV2_Xpt425Mux4AInput,
												NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID};
	static const NTV2InputXptID gDLBInputs[]= { NTV2_Xpt425Mux1BInput, NTV2_Xpt425Mux2BInput, NTV2_Xpt425Mux3BInput, NTV2_Xpt425Mux4BInput,
												NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID,NTV2_INPUT_CROSSPOINT_INVALID};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
		return inLinkB ? gDLBInputs[inChannel] : gDLInputs[inChannel];
	else
		return NTV2_INPUT_CROSSPOINT_INVALID;
}

NTV2OutputXptID GetTSIMuxOutputXptFromChannel (const NTV2Channel inChannel, const bool inLinkB, const bool inIsRGB)
{
	static const NTV2OutputXptID gMuxARGBOutputs[] = {	NTV2_Xpt425Mux1ARGB,	NTV2_Xpt425Mux2ARGB,	NTV2_Xpt425Mux3ARGB,	NTV2_Xpt425Mux4ARGB,
														NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack};
	static const NTV2OutputXptID gMuxAYUVOutputs[] = {	NTV2_Xpt425Mux1AYUV,	NTV2_Xpt425Mux2AYUV,	NTV2_Xpt425Mux3AYUV,	NTV2_Xpt425Mux4AYUV,
														NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack};
	static const NTV2OutputXptID gMuxBRGBOutputs[] = {	NTV2_Xpt425Mux1BRGB,	NTV2_Xpt425Mux2BRGB,	NTV2_Xpt425Mux3BRGB,	NTV2_Xpt425Mux4BRGB,
														NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack};
	static const NTV2OutputXptID gMuxBYUVOutputs[] = {	NTV2_Xpt425Mux1BYUV,	NTV2_Xpt425Mux2BYUV,	NTV2_Xpt425Mux3BYUV,	NTV2_Xpt425Mux4BYUV,
														NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack,			NTV2_XptBlack};
	if (NTV2_IS_VALID_CHANNEL(inChannel))
	{
		if (inLinkB)
			return inIsRGB ? gMuxBRGBOutputs[inChannel] : gMuxBYUVOutputs[inChannel];
		else
			return inIsRGB	?  gMuxARGBOutputs[inChannel]  :  gMuxAYUVOutputs[inChannel];
	}
	else
		return NTV2_OUTPUT_CROSSPOINT_INVALID;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Crosspoint Utils End


// static
bool CNTV2SignalRouter::GetRouteROMInfoFromReg (const ULWord inRegNum, const ULWord inRegVal,
							NTV2InputXptID & outInputXpt, NTV2OutputXptIDSet & outOutputXpts, const bool inAppendOutputXpts)
{
	static const ULWord firstROMReg(kRegFirstValidXptROMRegister);
	static const ULWord firstInpXpt(NTV2_FIRST_INPUT_CROSSPOINT);
	if (!inAppendOutputXpts)
		outOutputXpts.clear();
	outInputXpt = NTV2_INPUT_CROSSPOINT_INVALID;
	if (inRegNum < uint32_t(kRegFirstValidXptROMRegister))
		return false;
	if (inRegNum >= uint32_t(kRegInvalidValidXptROMRegister))
		return false;

	const ULWord regOffset(inRegNum - firstROMReg);
	const ULWord bitOffset((regOffset % 4) * 32);
	outInputXpt = NTV2InputXptID(firstInpXpt  +	 regOffset / 4UL);	//	4 regs per inputXpt
	if (!inRegVal)
		return true;	//	No bits set

	RoutingExpertPtr pExpert(RoutingExpert::GetInstance());
	NTV2_ASSERT(pExpert);
	for (UWord bitNdx(0);  bitNdx < 32;	 bitNdx++)
		if (inRegVal & ULWord(1UL << bitNdx))
		{
			const NTV2OutputXptID yuvOutputXpt (NTV2OutputXptID((bitOffset + bitNdx) & 0x0000007F));
			const NTV2OutputXptID rgbOutputXpt (NTV2OutputXptID(yuvOutputXpt | 0x80));
			if (pExpert	 &&	 pExpert->IsOutputXptValid(yuvOutputXpt))
				outOutputXpts.insert(yuvOutputXpt);
			if (pExpert	 &&	 pExpert->IsOutputXptValid(rgbOutputXpt))
				outOutputXpts.insert(rgbOutputXpt);
		}
	return true;
}

// static
bool CNTV2SignalRouter::GetPossibleConnections (const NTV2RegReads & inROMRegs, NTV2PossibleConnections & outConnections)
{
	outConnections.clear();
	for (NTV2RegReadsConstIter iter(inROMRegs.begin());	 iter != inROMRegs.end();  ++iter)
	{
		if (iter->registerNumber < kRegFirstValidXptROMRegister	 ||	 iter->registerNumber >= kRegInvalidValidXptROMRegister)
			continue;	//	Skip -- not a ROM reg
		NTV2InputXptID inputXpt(NTV2_INPUT_CROSSPOINT_INVALID);
		NTV2OutputXptIDSet	outputXpts;
		if (GetRouteROMInfoFromReg (iter->registerNumber, iter->registerValue, inputXpt, outputXpts, true))
			for (NTV2OutputXptIDSetConstIter it(outputXpts.begin());  it != outputXpts.end();  ++it)
				outConnections.insert(NTV2Connection(inputXpt, *it));
	}
	return !outConnections.empty();
}

// static
bool CNTV2SignalRouter::MakeRouteROMRegisters (NTV2RegReads & outROMRegs)
{
	outROMRegs.clear();
	for (uint32_t regNum(kRegFirstValidXptROMRegister);	 regNum < kRegInvalidValidXptROMRegister;  regNum++)
		outROMRegs.push_back(NTV2RegInfo(regNum));
	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Stream Operators Begin

ostream & operator << (ostream & oss, const CNTV2SignalRouter & inObj)
{
	return inObj.Print(oss);
}


ostream & operator << (ostream & oss, const NTV2OutputXptIDSet & inObj)
{
	NTV2OutputXptIDSetConstIter iter(inObj.begin());
	while (iter != inObj.end())
	{
		oss << ::NTV2OutputCrosspointIDToString(*iter, false);
		if (++iter == inObj.end())
			break;
		oss << ", ";
	}
	return oss;
}

ostream & operator << (ostream & oss, const NTV2InputXptIDSet & inObj)
{
	NTV2InputXptIDSetConstIter	iter(inObj.begin());
	while (iter != inObj.end())
	{
		oss << ::NTV2InputCrosspointIDToString(*iter, false);
		if (++iter == inObj.end())
			break;
		oss << ", ";
	}
	return oss;
}

NTV2RoutingEntry & NTV2RoutingEntry::operator = (const NTV2RegInfo & inRHS)
{
	registerNum = inRHS.registerNumber;
	mask		= inRHS.registerMask;
	shift		= inRHS.registerShift;
	value		= inRHS.registerValue;
	return *this;
}

ostream & operator << (ostream & inOutStream, const NTV2WidgetIDSet & inObj)
{
	for (NTV2WidgetIDSetConstIter iter (inObj.begin ());  iter != inObj.end ();	 )
	{
		inOutStream << ::NTV2WidgetIDToString (*iter, true);
		if (++iter != inObj.end ())
			inOutStream << ",";
	}
	return inOutStream;
}

ostream & operator << (ostream & oss, const NTV2XptConnection & inObj)
{
	oss << ::NTV2InputCrosspointIDToString(inObj.first) << " <== " << ::NTV2OutputCrosspointIDToString(inObj.second);
	return oss;
}

ostream & operator << (ostream & oss, const NTV2XptConnections & inObj)
{
	for (NTV2XptConnectionsConstIter it(inObj.begin());	 it != inObj.end();	 )
	{
		oss << *it;
		if (++it != inObj.end())
			oss << endl;
	}
	return oss;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Stream Operators End
