/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubaccess.cpp
	@brief		Implementation of NTV2 "nub" client functions.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "ajatypes.h"
#include "rpc/client.h"
#include "rpc/rpc_error.h"
#include "ntv2card.h"
#include "ntv2utils.h"
#include "ntv2nubaccess.h"
#include "ntv2endian.h"
#include "ntv2publicinterface.h"
#include "ntv2testpatterngen.h"
#include "ntv2devicescanner.h"	//	For IsAlphaNumeric, etc.
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include "ajabase/system/info.h"
#include <iomanip>
#if defined(AJAMac)
	#include <CoreFoundation/CoreFoundation.h>
	#include <dlfcn.h>
#endif

using namespace std;

#if defined(NTV2_NUB_CLIENT_SUPPORT)
#define INSTP(_p_)			xHEX0N(uint64_t(_p_),16)
#define	NBFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_RPCClient, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#define	NBCFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RPCClient, AJAFUNC << ": " << __x__)
#define	NBCWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RPCClient, AJAFUNC << ": " << __x__)
#define	NBCNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RPCClient, AJAFUNC << ": " << __x__)
#define	NBCINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_RPCClient, AJAFUNC << ": " << __x__)
#define	NBCDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_RPCClient, AJAFUNC << ": " << __x__)



NTV2DeviceSpecParser::NTV2DeviceSpecParser (const string inSpec)
{
	Reset(inSpec);
}

void NTV2DeviceSpecParser::Reset (const string inSpec)
{
	mErrors.clear();
	mResult.clear();
	mQueryParams.clear();
	mPos = 0;
	mSpec = inSpec;
	if (!mSpec.empty())
		Parse();	//	Go ahead and parse it
}

bool NTV2DeviceSpecParser::HasResult (const string & inKey) const
{
	return mResult.find(inKey) != mResult.end() ? true : false;
}

string NTV2DeviceSpecParser::Result (const string & inKey) const
{
	NTV2ConnectParamsCIter it(mResult.find(inKey));
	if (it != mResult.end())
		return it->second;
	return string();
}

string NTV2DeviceSpecParser::Resource (const bool inStripLeadSlash) const
{
	string rsrc (Result(kConnectParamResource));
	if (rsrc.empty())
		return rsrc;
	if (!inStripLeadSlash)
		return rsrc;
	if (rsrc.at(0) == '/')
		rsrc.erase(0,1);
	return rsrc;
}

void NTV2DeviceSpecParser::Parse (void)
{
	//	A run of 3 consecutive letters that match "ntv" -- probably a scheme
	//	A run of 1 or 2 decimal digits -- probably a local device index number
	//	"0X" or "0x":
	//		-	maybe a hexadecimal 32-bit value -- a local device ID
	//		-	maybe a hexadecimal 64-bit value -- a local device serial number
	//	A run of 8 or 9 alphanumeric chars -- probably a local device serial number
	ostringstream err;
	string	tokDevID, tokIndexNum, tokScheme, tokSerial, tokModelName;
	size_t	posDevID(0), posIndexNum(0), posScheme(0), posSerial(0), posModelName(0);
	bool	isSerial(ParseSerialNum(posSerial, tokSerial)), isScheme(ParseScheme(posScheme, tokScheme));
	bool	isIndexNum(ParseDecNumber(posIndexNum, tokIndexNum)), isDeviceID(ParseDeviceID(posDevID, tokDevID));
	bool	isModelName(ParseModelName(posModelName, tokModelName));
	if (isScheme  &&  tokScheme == kLegalSchemeNTV2Local)
	{	//	Re-parse serial#, index#, deviceID, modelName from just past "://"...
		posDevID = posIndexNum = posSerial = posModelName  = posScheme;
		isSerial = ParseSerialNum(posSerial, tokSerial);
		isIndexNum = ParseDecNumber(posIndexNum, tokIndexNum);
		isDeviceID = ParseDeviceID(posDevID, tokDevID);
		isModelName = ParseModelName(posModelName, tokModelName);
	}
	do
	{
		if (isModelName)
		{
			mPos = posModelName;
			mResult[kConnectParamDevModel] = tokModelName;
			mResult[kConnectParamScheme] = kLegalSchemeNTV2Local;
			break;
		}
		if (isSerial)
		{	//	Final serial number checks...
			bool converted(false);
			mPos = posSerial;
			if (tokSerial.length() == 18)	//	64-bit hex value?
			{
				//	Convert numeric serial number into character string...
				const bool hasLeading0X (tokSerial.find("0X") == 0  ||  tokSerial.find("0x") == 0);
				const string hex64(tokSerial.substr(hasLeading0X ? 2 : 0, 16));
				const ULWord64 serNum64(aja::stoull(hex64, AJA_NULL, 16));
				string serTxt;	//	(CNTV2Card::SerialNum64ToString(serNum64));
				for (size_t ndx(0);  ndx < 8;  ndx++)
					serTxt += char(serNum64 >> ((7-ndx)*8));
				//cerr << "Converted '" << tokSerial << "' into '" << serTxt << "'" << endl;
				tokSerial = serTxt;
				converted = true;
			}
			//	Check for illegal characters in serial number:
			for (size_t ndx(0);  ndx < tokSerial.length();  ndx++)
			{	char ch(tokSerial.at(ndx));
				if ( ! ( ( (ch >= '0') && (ch <= '9') ) ||
						 ( (ch >= 'A') && (ch <= 'Z') ) ||
						 ( (ch >= 'a') && (ch <= 'z') ) ||
						   (ch == ' ') || (ch == '-') ) )
				{
					err << "Illegal serial number character '" << (ch ? ch : '?') << "' (" << xHEX0N(UWord(ch),2) << ")";
					AddError(err.str());
					mPos -= converted ? 16 : 8;  mPos += ndx * (converted ? 2 : 1) + (converted ? 1 : 0);
					break;
				}
			}
			mResult[kConnectParamDevSerial] = tokSerial;
			mResult[kConnectParamScheme] = kLegalSchemeNTV2Local;
			break;
		}
		if (isDeviceID)
		{
			mPos = posDevID;
			mResult[kConnectParamDevID] = tokDevID;
			mResult[kConnectParamScheme] = kLegalSchemeNTV2Local;
			break;
		}
		if (isIndexNum)
		{
			mPos = posIndexNum;
			mResult[kConnectParamDevIndex] = tokIndexNum;
			mResult[kConnectParamScheme] = kLegalSchemeNTV2Local;
			break;
		}
		if (!isScheme  ||  (isScheme  &&  tokScheme == kLegalSchemeNTV2Local))
		{	//	No such local device
			err << "Invalid local device specification";
			AddError(err.str());
			mPos += isScheme ? 12 : 0;
			break;
		}
		if (isScheme)
		{	//	Continue parsing URLspec...
			mPos = posScheme;
			if (!IsSupportedScheme(tokScheme))
				{err << "Unsupported scheme '" << tokScheme << "'";  AddError(err.str());  mPos -= 3;  break;}
			//	"ntv2://swdevice/?"
			//		"nosharedmemory"
			//		"&supportlog=file%3A%2F%2F%2FUsers%2Fdemo%2FDesktop%2FAJAWatcherSupport.log"
			//		"&sdram=file%3A%2F%2F%2FUsers%2Fdemo%2FDesktop%2FSDRAMsnapshot.dat");
			//	Host[port]/[resource[?query]]
			size_t posURL(posScheme), posRsrc(0);
			string	host, port, rsrcPath;
			if (!ParseHostAddressAndPortNumber(posURL, host, port))
				{mPos = posURL;  AddError("Bad host address or port number");  break;}
			mPos = posURL;
			mResult[kConnectParamScheme] = tokScheme;
			mResult[kConnectParamHost] = host;
			if (!port.empty())
				mResult[kConnectParamPort] = port;

			//	Parse resource path...
			posRsrc = mPos;
			if (ParseResourcePath(posRsrc, rsrcPath))
				{mPos = posRsrc;  mResult[kConnectParamResource] = rsrcPath;}
			//	Parse query...
			size_t posQuery(mPos);
			NTV2Dictionary params;
			if (ParseQuery(posQuery, params))
			{
				mResult[kConnectParamQuery] = DeviceSpec().substr(mPos, posQuery-mPos+1);
				mQueryParams = params;
				mPos = posQuery;
			}
			if (mPos < SpecLength())
				{err << "Extra character(s) at " << DEC(mPos);  AddError(err.str());  break;}
		}
	} while (false);
	#if defined(_DEBUG)
		if (Successful())
			{cout << "Success: '" << DeviceSpec() << "'  --  "; Print(cout); cout << endl;}
		else
			{cerr << "Failed: "; PrintErrors(cerr); cerr << endl;}
	#endif	//	defined(_DEBUG)
}	//	Parse

ostream & NTV2DeviceSpecParser::Print (ostream & oss, const bool inDumpResults) const
{
	if (IsSoftwareDevice())
	{
		oss << "software device '" << Result(kConnectParamHost) << "'";
		if (HasResult(kConnectParamResource))
			oss << " resource '" << Result(kConnectParamResource) << "'";
		if (HasResult(kConnectParamQuery))
			oss << " query '" << Result(kConnectParamQuery) << "'";
	}
	else
	{
		oss << (IsLocalDevice() ? "local" : "remote") << " device";
		if (HasResult(kConnectParamDevSerial))
			oss << " serial '" << DeviceSerial() << "'";
		else if (HasResult(kConnectParamDevModel))
			oss << " model '" << DeviceModel() << "'";
		else if (HasResult(kConnectParamDevID))
			oss << " ID '" << DeviceID() << "'";
		else if (HasResult(kConnectParamDevIndex))
			oss << " " << DeviceIndex();
		if (IsRemoteDevice())
		{
			oss << " at host '" << Result(kConnectParamHost) << "'";
			if (HasResult(kConnectParamPort))
				oss << " port " << Result(kConnectParamPort);
			if (HasResult(kConnectParamResource))
				oss << " resource '" << Result(kConnectParamResource) << "'";
			if (HasResult(kConnectParamQuery))
				oss << " query '" << Result(kConnectParamQuery) << "'";
		}
	}
	if (inDumpResults)
	{
		const int maxKeyWdth(LargestResultKey());
		oss << endl << DEC(Results().size()) << " result param" << (Results().size() == 1 ? "" : "s") << (Results().empty() ? "" : ":");
		if (!Results().empty())
		{
			oss << endl;
			
			for (NTV2DictConstIter it(mResult.begin());  it != mResult.end();  ++it)
			{
				string key(it->first), val(it->second);
				oss << std::setw(maxKeyWdth) << key << "=";
				if (val.find(' ') != string::npos)
					oss << "'" << val << "'" << endl;
				else
					oss << val << endl;
			}
		}
	}
	return oss;
}

string NTV2DeviceSpecParser::InfoString (void) const
{
	ostringstream oss;
	Print(oss);
	return oss.str();
}

uint64_t NTV2DeviceSpecParser::DeviceSerial (void) const
{
	uint64_t result(0);
	StringToSerialNum64 (Result(kConnectParamDevSerial), result);
	return result;
}

NTV2DeviceID NTV2DeviceSpecParser::DeviceID (void) const
{
	string devIDStr (Result(kConnectParamDevID));
	if (devIDStr.find("0X") != string::npos)
		devIDStr.erase(0,2);	//	Delete "0x"
	ULWord u32 = ULWord(aja::stoull(devIDStr, AJA_NULL, 16));
	return NTV2DeviceID(u32);
}

UWord NTV2DeviceSpecParser::DeviceIndex (void) const
{
	string devIDStr (Result(kConnectParamDevIndex));
	UWord u16 = UWord(aja::stoul(devIDStr));
	return u16;
}

string NTV2DeviceSpecParser::QueryParam (const string & inKey) const
{
	NTV2DictConstIter it(mQueryParams.find(inKey));
	if (it != mQueryParams.end())
		return it->second;
	return "";
}

ostream & NTV2DeviceSpecParser::PrintErrors (ostream & oss) const
{
	oss << DEC(ErrorCount()) << (ErrorCount() == 1 ? " error" : " errors") << (HasErrors() ? ":" : "");
	if (HasErrors())
	{
		oss	<< endl
			<< DeviceSpec() << endl
			<< string(mPos ? mPos : 0,' ') << "^" << endl;
		for (size_t num(0);  num < ErrorCount();  )
		{
			oss << Error(num);
			if (++num < ErrorCount())
				oss << endl;
		}
	}
	return oss;
}

int NTV2DeviceSpecParser::LargestResultKey (void) const
{
	size_t result(0);
	for (NTV2DictConstIter it(mResult.begin());  it != mResult.end();  ++it)
		if (it->first.length() > result)
			result = it->first.length();
	return int(result);
}

int NTV2DeviceSpecParser::LargestResultValue (void) const
{
	size_t result(0);
	for (NTV2DictConstIter it(mResult.begin());  it != mResult.end();  ++it)
		if (it->second.length() > result)
			result = it->second.length();
	return int(result);
}

bool NTV2DeviceSpecParser::ParseHexNumber (size_t & pos, string & outToken)
{
	outToken.clear();
	string tokHexNum;
	while (pos < SpecLength())
	{
		const char ch(CharAt(pos));
		if (tokHexNum.length() == 0)
		{
			if (ch != '0')
				break;
			++pos; tokHexNum = ch;
		}
		else if (tokHexNum.length() == 1)
		{
			if (ch != 'x'  &&  ch != 'X')
				break;
			++pos; tokHexNum += ch;
		}
		else
		{
			if (!IsHexDigit(ch))
				break;
			++pos; tokHexNum += ch;
		}
	}
	if (tokHexNum.length() > 2)	//	At least 3 chars
		{aja::upper(tokHexNum);  outToken = tokHexNum;}	//	Force upper-case hex
	return !outToken.empty();
}

bool NTV2DeviceSpecParser::ParseDecNumber (size_t & pos, string & outToken)
{
	outToken.clear();
	string tokDecNum;
	while (pos < SpecLength())
	{
		const char ch(CharAt(pos));
		if (!IsDecimalDigit(ch))
			break;
		++pos;
		if (ch != '0'  ||  tokDecNum != "0")	//	This prevents accumulating more than one leading zero
			tokDecNum += ch;
	}
	if (tokDecNum.length() > 0)	//	At least 1 char
		outToken = tokDecNum;
	return !outToken.empty();
}

bool NTV2DeviceSpecParser::ParseAlphaNumeric (size_t & pos, string & outToken, const bool inUpperCaseOnly)
{
	outToken.clear();
	string tokAlphaNum;
	while (pos < SpecLength())
	{
		const char ch(CharAt(pos));
		if (inUpperCaseOnly)
		{
			if (!IsUpperLetter(ch) && !IsDecimalDigit(ch))
				break;
		}
		else
		{
			if (!IsLetter(ch) && !IsDecimalDigit(ch))
				break;
		}
		++pos;  tokAlphaNum += ch;
	}
	if (tokAlphaNum.length() > 1)	//	At least 2 chars
		outToken = tokAlphaNum;
	return !outToken.empty();
}

bool NTV2DeviceSpecParser::ParseScheme (size_t & pos, string & outToken)
{
	outToken.clear();
	string rawScheme, tokScheme;
	while (ParseAlphaNumeric(pos, rawScheme))
	{
		tokScheme = rawScheme;
		char ch(CharAt(pos));
		if (ch != ':')
			break;
		++pos;  tokScheme += ch;

		ch = CharAt(pos);
		if (ch != '/')
			break;
		++pos;  tokScheme += ch;

		ch = CharAt(pos);
		if (ch != '/')
			break;
		++pos;  tokScheme += ch;
		break;
	}
	if (tokScheme.find("://") != string::npos)	//	Contains "://"
		{aja::lower(rawScheme);  outToken = rawScheme;}	//	Force lower-case
	return !outToken.empty();
}

bool NTV2DeviceSpecParser::ParseSerialNum (size_t & pos, string & outToken)
{
	outToken.clear();
	string tokAlphaNum, tokHexNum;
	size_t posAlphaNum(pos), posHexNum(pos);
	do
	{
		while (posAlphaNum < SpecLength())
		{
			const char ch(CharAt(posAlphaNum));
			if (!IsLetter(ch) && !IsDecimalDigit(ch) && ch != '-' && ch != ' ')
				break;
			++posAlphaNum;  tokAlphaNum += ch;
		}
		if (tokAlphaNum.length() < 2)	//	At least 2 chars
			tokAlphaNum.clear();
		else if (tokAlphaNum.length() == 8  ||  tokAlphaNum.length() == 9)
			{pos = posAlphaNum;   outToken = tokAlphaNum;  break;}

		if (ParseHexNumber(posHexNum, tokHexNum))
			if (tokHexNum.length() == 18)	//	64-bit value!
				{pos = posHexNum;  outToken = tokHexNum;}
	} while (false);
	return !outToken.empty();
}

bool NTV2DeviceSpecParser::ParseDeviceID (size_t & pos, string & outToken)
{
	outToken.clear();
	string tokHexNum;
	if (!ParseHexNumber(pos, tokHexNum))
		return false;
	if (tokHexNum.length() != 10)
		return false;
	aja::upper(tokHexNum);	//	Fold to upper case

	//	Check if it matches a known supported NTV2DeviceID...
	NTV2DeviceIDSet allDevIDs(::NTV2GetSupportedDevices());
	NTV2StringSet devIDStrs;
	for (NTV2DeviceIDSetConstIter it(allDevIDs.begin());  it != allDevIDs.end();  ++it)
	{
		ostringstream devID; devID << xHEX0N(*it,8);
		string devIDStr(devID.str());
		aja::upper(devIDStr);
		devIDStrs.insert(devIDStr);
	}	//	for each known/supported NTV2DeviceID
	if (devIDStrs.find(tokHexNum) != devIDStrs.end())
		outToken = tokHexNum;	//	Valid!
	return !outToken.empty();
}

bool NTV2DeviceSpecParser::ParseModelName (size_t & pos, string & outToken)
{
	outToken.clear();
	string tokName;
	if (!ParseAlphaNumeric(pos, tokName))
		return false;
	aja::lower(tokName);	//	Fold to lower case

	//	Check if it matches a known supported device model name...
	NTV2DeviceIDSet allDevIDs(::NTV2GetSupportedDevices());
	NTV2StringSet modelNames;
	for (NTV2DeviceIDSetConstIter it(allDevIDs.begin());  it != allDevIDs.end();  ++it)
	{
		string modelName(::NTV2DeviceIDToString(*it));
		aja::lower(modelName);
		modelNames.insert(modelName);
	}	//	for each known/supported NTV2DeviceID
	if (modelNames.find(tokName) != modelNames.end())
		outToken = tokName;	//	Valid!
	return !outToken.empty();
}

bool NTV2DeviceSpecParser::ParseDNSName (size_t & pos, string & outDNSName)
{
	outDNSName.clear();
	string dnsName, name;
	size_t dnsPos(pos);
	char ch(0);
	while (ParseAlphaNumeric(dnsPos, name))
	{
		if (!dnsName.empty())
			dnsName += '.';
		dnsName += name;
		ch = CharAt(dnsPos);
		if (ch != '.')
			break;
		++dnsPos;
	}
	if (!dnsName.empty())
		pos = dnsPos;
	outDNSName = dnsName;
	return !outDNSName.empty();
}

bool NTV2DeviceSpecParser::ParseIPv4Address (size_t & pos, string & outIPv4)
{
	outIPv4.clear();
	string ipv4Name, num;
	size_t ipv4Pos(pos);
	char ch(0);
	while (ParseDecNumber(ipv4Pos, num))
	{
		if (!ipv4Name.empty())
			ipv4Name += '.';
		ipv4Name += num;
		ch = CharAt(ipv4Pos);
		if (ch != '.')
			break;
		++ipv4Pos;
	}
	if (!ipv4Name.empty())
		pos = ipv4Pos;
	outIPv4 = ipv4Name;
	return !outIPv4.empty();
}

bool NTV2DeviceSpecParser::ParseHostAddressAndPortNumber (size_t & pos, string & outAddr, string & outPort)
{
	outAddr.clear();  outPort.clear();
	//	Look for a DNSName or an IPv4 dotted quad...
	string dnsName, ipv4, port;
	size_t dnsPos(pos), ipv4Pos(pos), portPos(0);
	bool isDNS(ParseDNSName(dnsPos, dnsName)), isIPv4(ParseIPv4Address(ipv4Pos, ipv4));
	if (!isDNS  &&  !isIPv4)
		{pos = dnsPos < ipv4Pos ? ipv4Pos : dnsPos;  return false;}
	//	NOTE:  It's possible to have both isIPv4 && isDNS true -- in this case, isIPv4 takes precedence:
	if (isIPv4)
		{outAddr = ipv4;  pos = portPos = ipv4Pos;}
	else if (isDNS)
		{outAddr = dnsName;  pos = portPos = dnsPos;}

	//	Check for optional port number
	char ch (CharAt(portPos));
	if (ch != ':')
		return true;
	++portPos;
	if (!ParseDecNumber(portPos, port))
		{pos = portPos;  return false;}	//	Bad port number!
	outPort = port;
	pos = portPos;
	return true;
}

bool NTV2DeviceSpecParser::ParseResourcePath (size_t & pos, string & outRsrc)
{
	outRsrc.clear();
	string rsrc, name;
	size_t rsrcPos(pos);
	char ch(CharAt(rsrcPos));
	while (ch == '/')
	{
		++rsrcPos;
		rsrc += '/';
		if (!ParseAlphaNumeric(rsrcPos, name))
			break;
		rsrc += name;
		ch = CharAt(rsrcPos);
	}
	if (!rsrc.empty())
		pos = rsrcPos;
	outRsrc = rsrc;
	return !outRsrc.empty();
}

bool NTV2DeviceSpecParser::ParseParamAssignment (size_t & pos, string & outKey, string & outValue)
{
	outKey.clear();  outValue.clear();
	string key, value;
	size_t paramPos(pos);
	char ch(CharAt(paramPos));
	if (ch == '&')
		ch = CharAt(++paramPos);
	do
	{
		if (!ParseAlphaNumeric(paramPos, key))
			break;
		ch = CharAt(paramPos);
		if (ch != '=')
			break;
		ch = CharAt(++paramPos);
		while (ch != 0  &&  ch != '&')
		{
			value += ch;
			ch = CharAt(++paramPos);
		}
	} while (false);
	if (!key.empty())
		{pos = paramPos;  outKey = key;  outValue = value;}
	return !key.empty();
}

bool NTV2DeviceSpecParser::ParseQuery (size_t & pos, NTV2Dictionary & outParams)
{
	outParams.clear();
	string key, value;
	size_t queryPos(pos);
	char ch(CharAt(queryPos));
	if (ch != '?')
		return false;
	ch = CharAt(++queryPos);

	while (ParseParamAssignment(queryPos, key, value))
	{
		outParams[key] = value;
		ch = CharAt(queryPos);
		if (ch != '&')
			break;
	}
	if (!outParams.empty())
		pos = queryPos;
	return !outParams.empty();
}

const NTV2StringSet & NTV2DeviceSpecParser::SupportedSchemes (void)
{
	static NTV2StringSet sLegalSchemes;
	if (sLegalSchemes.empty())
	{
		sLegalSchemes.insert(kLegalSchemeNTV2);
		sLegalSchemes.insert(kLegalSchemeNTV2Nub);
		sLegalSchemes.insert(kLegalSchemeNTV2Local);
	}
	return sLegalSchemes;
}

bool NTV2DeviceSpecParser::IsSupportedScheme (const string & inScheme)
{
	const NTV2StringSet & legalSchemes(SupportedSchemes());
	return legalSchemes.find(inScheme) != legalSchemes.end();
}

bool NTV2DeviceSpecParser::IsUpperLetter (const char inChar)
{	static const string sHexDigits("_ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	return sHexDigits.find(inChar) != string::npos;
}

bool NTV2DeviceSpecParser::IsLowerLetter (const char inChar)
{	static const string sHexDigits("abcdefghijklmnopqrstuvwxyz");
	return sHexDigits.find(inChar) != string::npos;
}

bool NTV2DeviceSpecParser::IsLetter (const char inChar, const bool inIncludeUnderscore)
{	return (inIncludeUnderscore  &&  inChar == '_')  ||  IsUpperLetter(inChar)  ||  IsLowerLetter(inChar);
}

bool NTV2DeviceSpecParser::IsDecimalDigit (const char inChar)
{	static const string sDecDigits("0123456789");
	return sDecDigits.find(inChar) != string::npos;
}

bool NTV2DeviceSpecParser::IsHexDigit (const char inChar)
{	static const string sHexDigits("0123456789ABCDEFabcdef");
	return sHexDigits.find(inChar) != string::npos;
}

bool NTV2DeviceSpecParser::IsLegalSerialNumChar (const char inChar)
{	return IsLetter(inChar) || IsDecimalDigit(inChar);
}

#if defined(_DEBUG)
	void NTV2DeviceSpecParser::test (void)
	{
		NTV2DeviceSpecParser specParser;
		specParser.Reset("1");
		specParser.Reset("00000000000000000000000000000000000000000000000000000000000000000000000000000000000001");
		specParser.Reset("corvid24");
		specParser.Reset("corvid88");
		specParser.Reset("konalhi");
		specParser.Reset("alpha");
		specParser.Reset("00T64450");
		specParser.Reset("00t6-450");
		specParser.Reset("BLATZBE0");
		specParser.Reset("0x424C41545A424530");
		specParser.Reset("0x424C415425424530");

		specParser.Reset("badscheme://1");

		specParser.Reset("ntv2local://1");
		specParser.Reset("NtV2lOcAl://00000000000000000000000000000000000000000000000000000000000000000000000000000000000001");
		specParser.Reset("NTV2Local://corvid24");
		specParser.Reset("ntv2local://corvid88");
		specParser.Reset("ntv2local://konalhi");
		specParser.Reset("ntv2local://alpha");
		specParser.Reset("ntv2local://00T64450");
		specParser.Reset("ntv2local://00t6-450");
		specParser.Reset("ntv2local://BLATZBE0");

		specParser.Reset("ntv2nub://1.2.3.4");
		specParser.Reset("ntv2nub://1.2.3.4/doc");
		specParser.Reset("ntv2nub://1.2.3.4/doc/");
		specParser.Reset("ntv2nub://1.2.3.4/doc/alpha?one&two=2&three=&four=4");
		specParser.Reset("ntv2nub://1.2.3.4/doc/?one&two=2&three=&four=4");
		specParser.Reset("ntv2nub://1.2.3.4:badport/doc?one&two=2&three=&four=4");
		specParser.Reset("ntv2nub://1.2.3.4:200/doc?one&two=2&three=&four=4");
		specParser.Reset("ntv2nub://1.2.3.4:200/doc/?one&two=2&three=&four=4");
		specParser.Reset("ntv2nub://1.2.3.4:12345");
		specParser.Reset("ntv2nub://1.2.3.4:65000/doc");
		specParser.Reset("ntv2nub://1.2.3.4:32767/doc/");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc/");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc/?");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc?");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc/?one");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc?one");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc/?one=");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc?one=");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc/?one=1");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc?one=1");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc/?one=1&two");
		specParser.Reset("ntv2nub://1.2.3.4/path/to/doc?one=1&two");
		specParser.Reset("ntv2nub://50.200.250.300");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.com/path/to/doc/?one=1&two");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.edu:badport/path/to/doc/?one=1&two");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.info:5544/path/to/doc/?one=1&two");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.org/path/to/doc/?one=1&two");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.nz:badport/path/to/doc/?one=1&two");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.au:000004/path/to/doc/?one=1&two");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.ch:4/corvid88");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.cn:4/00T64450");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.ru:4/2");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.co.uk:4/00000000000000000000000000000001");
		specParser.Reset("ntv2nub://fully.qualified.domain.name.com:4/0000000000000000000000000000000001");
		specParser.Reset("ntv2://swdevice/?"
							"nosharedmemory"
							"&supportlog=file%3A%2F%2F%2FUsers%2Fdemo%2FDesktop%2FAJAWatcherSupport.log"
							"&sdram=file%3A%2F%2F%2FUsers%2Fdemo%2FDesktop%2FSDRAMsnapshot.dat");
	}
#endif	//	defined(_DEBUG)


/*****************************************************************************************************************************************************
	NTV2RPCAPI
*****************************************************************************************************************************************************/
NTV2RPCAPI::NTV2RPCAPI ()
	:	mConnectParams(),
		mPvt(mInstanceData, sizeof(mInstanceData))
{
	mPvt.Fill(ULWord(0));
}

NTV2RPCAPI::~NTV2RPCAPI ()
{
	if (IsConnected())
		NTV2Disconnect();
}

bool NTV2RPCAPI::HasConnectParam (const string & inParam) const
{
	return mConnectParams.find(inParam) != mConnectParams.end();
}

string NTV2RPCAPI::ConnectParam (const string & inParam) const
{
	NTV2ConnectParamsCIter it(mConnectParams.find(inParam));
	if (it != mConnectParams.end())
		return it->second;
	return string();
}

ostream & NTV2RPCAPI::Print (ostream & oss) const
{
	oss << (IsConnected() ? "Connected" : "Disconnected");
	if (IsConnected() && !Name().empty())
		oss << " to '" << Name() << "'";
	return oss;
}

bool NTV2RPCAPI::NTV2Connect (const NTV2ConnectParams & inParams)
{
	if (IsConnected())
		NTV2Disconnect();
	mConnectParams = inParams;
	return NTV2OpenRemote();
}

bool NTV2RPCAPI::NTV2Disconnect (void)
{
	return NTV2CloseRemote();
}

bool NTV2RPCAPI::NTV2ReadRegisterRemote (const ULWord regNum, ULWord & outRegValue, const ULWord regMask, const ULWord regShift)
{	(void) regNum;  (void) outRegValue; (void) regMask; (void) regShift;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift)
{	(void) regNum; (void) regValue; (void) regMask; (void) regShift;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2AutoCirculateRemote (AUTOCIRCULATE_DATA & autoCircData)
{	(void) autoCircData;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2WaitForInterruptRemote (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
{	(void) eInterrupt; (void) timeOutMs;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2DriverGetBitFileInformationRemote (BITFILE_INFO_STRUCT & bitFileInfo, const NTV2BitFileType bitFileType)
{	(void) bitFileInfo; (void) bitFileType;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2DriverGetBuildInformationRemote (BUILD_INFO_STRUCT & buildInfo)
{	(void) buildInfo;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2DownloadTestPatternRemote (const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
												const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum)
{	(void) channel;  (void) testPatternFBF; (void) signalMask; (void) testPatDMAEnb; (void) testPatNum;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2ReadRegisterMultiRemote (const ULWord numRegs, ULWord & outFailedRegNum, NTV2RegInfo outRegs[])
{	(void) numRegs; (void) outFailedRegNum; (void) outRegs;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2GetDriverVersionRemote (ULWord & outDriverVersion)
{
	outDriverVersion = 0xFFFFFFFF;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2DMATransferRemote (	const NTV2DMAEngine inDMAEngine,	const bool inIsRead,	const ULWord inFrameNumber,
										NTV2_POINTER & inOutFrameBuffer,	const ULWord inCardOffsetBytes,
										const ULWord inNumSegments,			const ULWord inSegmentHostPitch,
										const ULWord inSegmentCardPitch,	const bool inSynchronous)
{	(void) inDMAEngine; (void) inIsRead;	(void) inFrameNumber; (void) inOutFrameBuffer;
	(void) inCardOffsetBytes; (void) inNumSegments; (void) inSegmentHostPitch;
	(void) inSegmentCardPitch; (void) inSynchronous;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2MessageRemote (NTV2_HEADER * pInMessage)
{	(void) pInMessage;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2OpenRemote (void)
{
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCAPI::NTV2CloseRemote (void)
{
	mConnectParams.clear();
	return true;
}

using namespace ntv2nub;

/*****************************************************************************************************************************************************
	NTV2NubRPCAPI
*****************************************************************************************************************************************************/
//	Specific NTV2RPCAPI implementation to talk to remote host
class AJAExport NTV2NubRPCAPI : public NTV2RPCAPI
{
	//	Instance Methods
	public:
								NTV2NubRPCAPI()		:	mpRPCClient		(AJA_NULL),
														mConnectionID	(0),
														mDeviceID		(DEVICE_ID_INVALID),
														mSerialNumber	(0),
														mNoSwap			(false)		{}
		AJA_VIRTUAL				~NTV2NubRPCAPI()	{NTV2CloseRemote();}
		AJA_VIRTUAL	bool		IsConnected	(void) const;
		AJA_VIRTUAL	bool		NTV2ReadRegisterRemote	(const ULWord regNum, ULWord & outRegValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		AJA_VIRTUAL	bool		NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask = 0xFFFFFFFF, const ULWord regShift = 0);
		AJA_VIRTUAL	bool		NTV2AutoCirculateRemote	(AUTOCIRCULATE_DATA & autoCircData);
		AJA_VIRTUAL	bool		NTV2WaitForInterruptRemote	(const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs);
		AJA_VIRTUAL	bool		NTV2DriverGetBitFileInformationRemote	(BITFILE_INFO_STRUCT & outInfo, const NTV2BitFileType inType);
		AJA_VIRTUAL	bool		NTV2DriverGetBuildInformationRemote	(BUILD_INFO_STRUCT & outBuildInfo);
		AJA_VIRTUAL	bool		NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
															const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum);
		AJA_VIRTUAL	bool		NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum,  NTV2RegInfo aRegs[]);
		AJA_VIRTUAL	bool		NTV2GetDriverVersionRemote	(ULWord & outDriverVersion)		{(void) outDriverVersion;	return false;}
		AJA_VIRTUAL	bool		NTV2DMATransferRemote		(const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
														const ULWord inFrameNumber,			NTV2_POINTER & inOutBuffer,
														const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
														const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,
														const bool inSynchronous);
		AJA_VIRTUAL	bool		NTV2MessageRemote	(NTV2_HEADER *	pInMessage);
		AJA_VIRTUAL ostream &	Print (ostream & oss) const;

		AJA_VIRTUAL bool		NTV2QueryDevices (NTV2DeviceIDSerialPairs & outDevices);

	protected:
		AJA_VIRTUAL	bool		NTV2OpenRemote (void);
		AJA_VIRTUAL	bool		NTV2CloseRemote (void);
		AJA_VIRTUAL	inline		ULWord	ConnectionID (void) const		{return mConnectionID;}

	//	Instance Data
	private:
		ULWord					mReserved0;
		rpc::client *			mpRPCClient;	///< @brief	My rpclib client object
		ULWord					mConnectionID;	///< @brief	My connection ID on server (valid if connected)
		NTV2DeviceID			mDeviceID;		///< @brief	DeviceID of server device (valid if connected)
		uint64_t				mSerialNumber;	///< @brief	Serial number of server device (valid if connected)
		bool					mNoSwap;		///< @brief	Default: byte-swap if necessary to achieve network-byte-order
		bool					mReserved1[7];
};	//	NTV2NubRPCAPI


//	Factory method to create NTV2NubRPCAPI instance
NTV2RPCAPI * NTV2RPCAPI::MakeNTV2NubRPCAPI (const NTV2ConnectParams & inParams)
{
	NTV2NubRPCAPI * pResult(new NTV2NubRPCAPI);
	if (!pResult)
		return pResult;
	//	Open the device on the remote system...
	if (!pResult->NTV2Connect(inParams))
		{delete pResult;  pResult = AJA_NULL;}
	return pResult;
}


bool NTV2NubRPCAPI::IsConnected	(void) const
{
	rpc::client::connection_state conState(rpc::client::connection_state::disconnected);
	if (mpRPCClient)
		conState = mpRPCClient->get_connection_state();
	return mConnectionID  &&  conState == rpc::client::connection_state::connected;
}

bool NTV2NubRPCAPI::NTV2OpenRemote (void)
{
	ostringstream err;
	do
	{
		const string hostName(ConnectParam(kConnectParamHost)), port(ConnectParam(kConnectParamPort));
		if (hostName.empty())
			{err << "Missing '" << kConnectParamHost << "' parameter";  break;}
		UWord portNum(NTV2NUBPORT);
		if (!port.empty())
			portNum = UWord(aja::stoul(port));

		ostringstream hostPortName;  hostPortName << "'" << hostName << "'";  if (portNum != NTV2NUBPORT) hostPortName << " port " << DEC(portNum);
		vector<uint8_t> request, response;
		try
		{
			mpRPCClient = new rpc::client(hostName, portNum);
		}
		catch (rpc::system_error & e)
		{	//	e.g. "resolve: Host not found (authoritative)"
			err << e.what() << " by server " << hostPortName.str();
		}
		if (!mpRPCClient)
			{err << "Unable to allocate RPC client " << hostPortName.str();}
		if (!err.str().empty())
			break;	//	Bail on error

		//	Build the request...
		request.push_back(NTV2_RPC_PROTOCOL_VERSION);	//	1st byte is my (client) protocol version
		request.push_back(AJA_NTV2_SDK_VERSION_MAJOR);	//	2nd byte is my (client) SDK major version
		request.push_back(AJA_NTV2_SDK_VERSION_MINOR);	//	3rd byte is my (client) SDK minor version
		request.push_back(AJA_NTV2_SDK_VERSION_POINT);	//	4th byte is my (client) SDK point version
		PUSHU32(0xBAADF00D, request, kDisableByteSwap);	//	Bytes 5-8	uint32_t	Can detect byte-order differences
		if (HasConnectParam(kConnectParamResource))
		{	//	Client is requesting a specific resource on the server...
			string resource(ConnectParam(kConnectParamResource));
			if (!resource.empty() && resource[0] == '/')
				resource.erase(0,1);	//	Remove leading '/'
			//	TBD -- MrBill -- may need to URLDecode this, before stuffing it into the request...
			const ULWord lgth(ULWord(resource.length()));
			PUSHU32(lgth, request);								//	uint32_t	length of "resource" to be "opened" on server, in bytes
			for (ULWord chNdx(0);  chNdx < lgth;  chNdx++)
				request.push_back(UByte(resource.at(chNdx)));	//	uint8_t... every byte in "resource" string
		}

		//	Send "open" request...
		try
		{
			response = mpRPCClient->call("open", request).as<vector<uint8_t> >();
		}
		catch (rpc::system_error & e)
		{	//	Nub not running on server host? Wrong port number?
			err << e.what() << " by server " << hostPortName.str();
		}
		if (!err.str().empty())
			break;

		//	Check "open" response from server...
		if (response.size() < 8)
			{err << "Bad " << DEC(response.size()) << "-byte 'open' response, expected 8 bytes";  break;}

		uint32_t srvBAADF00D(0);	size_t ndx(0);
		UWord	srvProtocolVers	(UWord(response.at(ndx++))),	//	1st byte is server protocol version
				srvSDKMajorVers (UWord(response.at(ndx++))),	//	2nd byte is server client SDK major version
				srvSDKMinorVers	(UWord(response.at(ndx++))),	//	3rd byte is server client SDK minor version
				srvSDKPointVers (UWord(response.at(ndx++)));	//	4th byte is server client SDK point version
		POPU32(srvBAADF00D, response, ndx, kDisableByteSwap);	//	Bytes 5-8 as uint32_t can detect byte-order differences
		NBDBG("server=" << xHEX0N(srvBAADF00D,8) << (srvBAADF00D == 0xBAADF00D ? " matches" : " doesn't match") << " client byte-order");

		//	Check protocol version...
		ostringstream srvVers;
		if (!srvSDKMajorVers  &&  !srvSDKMinorVers  && !srvSDKPointVers)
			srvVers << "'dev' (0.0.0)";
		else
			srvVers << DEC(srvSDKMajorVers) << "." << DEC(srvSDKMinorVers) << "." << DEC(srvSDKPointVers);
		if (srvProtocolVers != NTV2_RPC_PROTOCOL_VERSION)
		{	err << "Protocol mismatch:  V" << DEC(NTV2_RPC_PROTOCOL_VERSION) << " client incompatible with V"
				<< DEC(srvProtocolVers) << " server " << hostPortName.str();
			break;
		}

		//	Get connectionID, NTV2DeviceID & serial#...
		uint32_t v32(0);
		if (ndx+16 > response.size())
		{	err << "Expected connectionID, deviceID & serial# from server " << hostPortName.str() << ": "
				<< DEC(ndx+16) << " should equal " << DEC(response.size());
			break;
		}
		POPU32(mConnectionID, response, ndx);	//	uint32_t	Connection ID			(network-byte-order)
		POPU32(v32, response, ndx);				//	uint32_t	NTV2DeviceID			(network-byte-order)
		POPU64(mSerialNumber, response, ndx);	//	uint64_t	Device serial number	(network-byte-order)
		mDeviceID = NTV2DeviceID(v32);
		mNoSwap = (srvBAADF00D == 0xBAADF00D);
		NBNOTE("Connection " << DEC(mConnectionID) << " to '" << ::NTV2DeviceIDToString(mDeviceID) << "' obtained from V"
				<< DEC(srvProtocolVers) << " server " << hostPortName.str() << " running SDK " << srvVers.str()
				<< " byteOrder=" << (mNoSwap ? "Host(LE)" : "Network(BE)"));
	} while (false);	//	once thru
	if (!err.str().empty())	//	FAILED?
	{
		NBFAIL(err.str());
		cerr << "## FAILED: " << err.str() << endl;
		delete mpRPCClient;
		mpRPCClient = AJA_NULL;
		mConnectionID = 0;
		mSerialNumber = 0;
		return false;
	}
	return true;
}	//	NTV2OpenRemote

bool NTV2NubRPCAPI::NTV2CloseRemote (void)
{	/**	MRBILL	MRBILL	MRBILL	CHECK THIS
	//	Encode the 'clos' request to transmit to server...
	vector<uint8_t> request;			//	Data to be transmitted to server
	PUSHU32(ConnectionID(), request);	//	ULWord		ConnectionID

	//	Transmit the request to the server, and wait for the result blob...
	NBDBG("Sending " << DEC(request.size()) << "-byte 'clos' request to server...");
	vector<uint8_t> response (mpRPCClient->call("clos", request).as<vector<uint8_t> >());
	NBDBG("Received " << DEC(response.size()) << "-byte 'clos' response from server");

	//	Decode the server response blob...
	size_t ndx(0);  uint32_t v32(0);
	POPU32(v32, response, ndx);			//	ULWord returnValue 1=success 0=fail
	if (!v32)
		NBNOTE("Connection " << DEC(v32) << "for '" << ::NTV2DeviceIDToString(mDeviceID) << "' closed on server");
	else
		NBWARN("Connection " << DEC(ConnectionID()) << " 'clos' failed on server");
MrBill	MrBILL	MRBILL	CHECK THIS	**/
	delete mpRPCClient;
	mpRPCClient = AJA_NULL;
	mConnectionID	= 0;
	mDeviceID		= DEVICE_ID_INVALID;
	mSerialNumber	= 0;
	mNoSwap			= false;
	return NTV2RPCAPI::NTV2CloseRemote();
}

bool NTV2NubRPCAPI::NTV2ReadRegisterRemote (const ULWord regNum, ULWord & regValue, const ULWord regMask, const ULWord regShift)
{
	regValue = 0;
	if (!IsConnected())
		return false;	//	No connection

	//	Encode the request to transmit to server...
	vector<uint8_t> request;	//	Data to be transmitted to server
	ULWord v32(0x80000000 | regShift);
	request.reserve(sizeof(ULWord)*4);	//	4 ULWords
	PUSHU32(ConnectionID(), request);	//	ULWord	ConnectionID
	PUSHU32(v32, request);				//	ULWord	[31] isRead  |  [6:0] regShift
	PUSHU32(regMask, request);			//	ULWord	regMask
	PUSHU32(regNum, request);			//	ULWord	regNum

	//	Transmit the request to the server, and wait for the response...
	NBDBG("Connection " << DEC(ConnectionID()) << " sending " << DEC(request.size()) << "-byte 'reg0' request...");
	vector<uint8_t> response (mpRPCClient->call("reg0", request).as<vector<uint8_t> >());
	if (response.size() < 8)
		{NBFAIL("Connection " << DEC(ConnectionID()) << " bad " << DEC(response.size()) << "-byte 'reg0' response, expected 8 bytes");  return false;}
	NBDBG("Connection " << DEC(ConnectionID()) << " received " << DEC(response.size()) << "-byte 'reg0' response");

	//	Decode the server response...
	size_t ndx(0);
	POPU32(v32, response, ndx);			//	ULWord	returnValue 1=success 0=fail
	POPU32(regValue, response, ndx);	//	ULWord	regValue that was read
	return bool(v32);
}

bool NTV2NubRPCAPI::NTV2WriteRegisterRemote (const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift)
{
	if (!IsConnected())
		return false;	//	No connection

	//	Encode the request to transmit to server...
	vector<uint8_t> request;	//	Data to be transmitted to server
	ULWord v32(regShift);
	request.reserve(sizeof(ULWord)*5);	//	5 ULWords:
	PUSHU32(ConnectionID(), request);	//	ULWord	ConnectionID
	PUSHU32(v32, request);				//	ULWord	[31] isRead  |  [6:0] regShift
	PUSHU32(regMask, request);			//	ULWord	regMask
	PUSHU32(regNum, request);			//	ULWord	regNum
	PUSHU32(regValue, request);			//	ULWord	regValue	(write only)

	//	Transmit the blob to the server, and wait for the response blob...
	NBDBG("Connection " << DEC(ConnectionID()) << " sending " << DEC(request.size()) << "-byte 'reg0' request...");
	vector<uint8_t> response (mpRPCClient->call("reg0", request).as<vector<uint8_t> >());
	if (response.size() < 4)
		{NBFAIL("Connection " << DEC(ConnectionID()) << " bad " << DEC(response.size()) << "-byte 'reg0' response, expected 4 bytes");  return false;}
	NBDBG("Connection " << DEC(ConnectionID()) << " received " << DEC(response.size()) << "-byte 'reg0' response");

	//	Decode the server response blob...
	size_t ndx(0);
	POPU32(v32, response, ndx);		//	ULWord	returnValue:	1=success 0=fail
	return bool(v32);
}

bool NTV2NubRPCAPI::NTV2AutoCirculateRemote (AUTOCIRCULATE_DATA & autoCircData)
{
	if (!IsConnected())
		return false;

	//	Encode autoCircData as a request to transmit to server...
	vector<uint8_t> request;	//	Data to be transmitted to server
	PUSHU32(ConnectionID(), request);		//	ULWord			ConnectionID
	if (!autoCircData.RPCEncode(request))
		{NBFAIL("Connection " << DEC(ConnectionID()) << "RPCEncode failed");  return false;}

	//	Transmit the request to the server, and wait for the response...
	NBDBG("Connection " << DEC(ConnectionID()) << " sending " << DEC(request.size()) << "-byte 'auto' request...");
	vector<uint8_t> response (mpRPCClient->call("auto", request).as<vector<uint8_t> >());
	if (response.size() < 4)
		{NBFAIL("Connection " << DEC(ConnectionID()) << " bad " << DEC(response.size()) << "-byte 'auto' response, expected 4 bytes");  return false;}
	NBDBG("Connection " << DEC(ConnectionID()) << " received " << DEC(response.size()) << "-byte 'auto' response");

	//	Decode the server response blob...
	size_t ndx(0);  uint32_t ok(0);
	POPU32(ok, response, ndx);	//	uint32_t	returnValue	1=success	0=fail
	if (!autoCircData.RPCDecode(response, ndx))
		{NBFAIL("Connection " << DEC(ConnectionID()) << " AUTOCIRCULATE_DATA::RPCDecode failed");  return false;}	//	Fail!
	return ok;
}

bool NTV2NubRPCAPI::NTV2WaitForInterruptRemote (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
{
	if (!IsConnected())
		return false;	//	No connection

	//	Encode the blob to transmit to server...
	vector<uint8_t> blob;	//	Data to be transmitted to server
	PUSHU32(ConnectionID(), blob);		//	ULWord	ConnectionID
	PUSHU32(ULWord(eInterrupt), blob);	//	ULWord	INTERRUPT_ENUMS
	PUSHU32(timeOutMs, blob);			//	ULWord	timeOutMs

	//	Transmit the blob to the server, and wait for the result blob...
	NBDBG("Connection " << DEC(ConnectionID()) << " sending " << DEC(blob.size()) << "-byte 'wfi0' request...");
	vector<uint8_t> response (mpRPCClient->call("wfi0", blob).as<vector<uint8_t> >());
	if (response.size() < 4)
		{NBFAIL("Connection " << DEC(ConnectionID()) << " bad " << DEC(response.size()) << "-byte 'wfi0' response, expected 4 bytes");  return false;}
	NBDBG("Connection " << DEC(ConnectionID()) << " received " << DEC(response.size()) << "-byte 'wfi0' response");

	//	Decode the server response blob...
	size_t ndx(0);
	ULWord v32(0);
	POPU32(v32, response, ndx);	//	ULWord returnValue 1=success 0=fail
	return bool(v32);
}

bool NTV2NubRPCAPI::NTV2DriverGetBitFileInformationRemote (BITFILE_INFO_STRUCT & outInfo,  const NTV2BitFileType inType)
{	(void) outInfo;  (void) inType;
	if (!IsConnected())
		return false;
	return false;
}

bool NTV2NubRPCAPI::NTV2DriverGetBuildInformationRemote (BUILD_INFO_STRUCT & outBuildInfo)
{	(void) outBuildInfo;
	if (!IsConnected())
		return false;
	return false;
}

bool NTV2NubRPCAPI::NTV2DownloadTestPatternRemote	(const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
													const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum)
{	(void) channel;  (void) testPatternFBF;  (void) signalMask;  (void) testPatDMAEnb;  (void) testPatNum;
	if (!IsConnected())
		return false;
	return false;
}

bool NTV2NubRPCAPI::NTV2ReadRegisterMultiRemote	(const ULWord numRegs, ULWord & outFailedRegNum,  NTV2RegInfo outRegs[])
{
	if (!IsConnected())
		return false;	//	No connection
	if (!numRegs)
		return true;	//	Nothing to do

	//	Let's cheat and use NTV2GetRegisters message.
	//	First, build the NTV2RegisterReads vector...
	NTV2RegisterReads regReads;
	regReads.reserve(numRegs);
	for (ULWord num(0);  num < numRegs;  num++)
		regReads.push_back(outRegs[num]);

	//	Next, do the message RPC to the server...
	NTV2GetRegisters getRegs(regReads);
	int result(NTV2MessageRemote(getRegs));

	//	Next, set the first failed register...
	NTV2RegNumSet failedRegNums;
	if (getRegs.GetBadRegisters(failedRegNums)  &&  !failedRegNums.empty())
		outFailedRegNum = *(failedRegNums.begin());

	//	Finally, set the register values in the outRegs array...
	NTV2RegisterValueMap goodRegs;
	getRegs.GetRegisterValues(goodRegs);
	for (ULWord num(0);  num < numRegs;  num++)
	{
		NTV2RegInfo & regInfo(outRegs[num]);
		NTV2RegValueMapConstIter it(goodRegs.find(regInfo.registerNumber));
		if (it != goodRegs.end())
			regInfo.registerValue = it->second;
	}
	return result;
}

#define AsNTV2GetRegs(_p_)		(reinterpret_cast<NTV2GetRegisters*>(_p_))
#define AsNTV2SetRegs(_p_)		(reinterpret_cast<NTV2SetRegisters*>(_p_))
#define AsNTV2BankGetSet(_p_)	(reinterpret_cast<NTV2BankSelGetSetRegs*>(_p_))
#define AsACStatus(_p_)			(reinterpret_cast<AUTOCIRCULATE_STATUS*>(_p_))
#define AsACFrameStamp(_p_)		(reinterpret_cast<FRAME_STAMP*>(_p_))
#define AsACTransfer(_p_)		(reinterpret_cast<AUTOCIRCULATE_TRANSFER*>(_p_))
#define AsNTV2Bitstream(_p_)	(reinterpret_cast<NTV2Bitstream*>(_p_))

bool NTV2NubRPCAPI::NTV2DMATransferRemote(	const NTV2DMAEngine inDMAEngine,	const bool inIsRead,
											const ULWord inFrameNumber,			NTV2_POINTER & inOutBuffer,
											const ULWord inCardOffsetBytes,		const ULWord inNumSegments,
											const ULWord inSegmentHostPitch,	const ULWord inSegmentCardPitch,
											const bool inSynchronous)
{
	if (!IsConnected())
		return false;	//	No connection

	//	Encode the request blob to transmit to server...
	vector<uint8_t> request;	//	Data to be transmitted to server
	request.reserve(sizeof(UWord)*3  +  sizeof(ULWord)*7  +  (inIsRead ? inOutBuffer.GetByteCount() : 4));
	const ULWord flags((inIsRead ? 0x80000000 : 0x00000000)  |  (inSynchronous ? 0x40000000 : 0x00000000)  |  (ULWord(inDMAEngine) & 0x0000000F));
	PUSHU32(ConnectionID(), request);			//	ULWord			ConnectionID
	PUSHU32(flags, request);					//	inIsRead  |  inSynchronous  |  NTV2DMAEngine	inDMAEngine
	PUSHU32(inFrameNumber, request);			//	ULWord			inFrameNumber
	PUSHU32(inCardOffsetBytes, request);		//	ULWord			inCardOffsetBytes
	PUSHU32(inNumSegments, request);			//	ULWord			inNumSegments
	PUSHU32(inSegmentHostPitch, request);		//	ULWord			inSegmentHostPitch
	PUSHU32(inSegmentCardPitch, request);		//	ULWord			inSegmentCardPitch
	if (inIsRead)
		PUSHU32(inOutBuffer.GetByteCount(), request);	//	ULWord	size of NTV2_POINTER to return after DMARead
	else if (!inOutBuffer.RPCEncode(request))			//	NTV2_POINTER	buffer to send to server for DMAWrite
		return false;

	//	Transmit the request to the server, and wait for the response...
	NBDBG("Connection " << DEC(ConnectionID()) << " sending " << DEC(request.size()) << "-byte 'dma0' request...");
	vector<uint8_t> response (mpRPCClient->call("dma0", request).as<vector<uint8_t> >());
	if (response.size() < 4)
		{NBFAIL("Connection " << DEC(ConnectionID()) << " bad " << DEC(response.size()) << "-byte 'dma0' response, expected 4 bytes");  return false;}
	NBDBG("Connection " << DEC(ConnectionID()) << " received " << DEC(response.size()) << "-byte 'dma0' response");

	//	Decode the server response...
	size_t ndx(0);	ULWord returnValue(0);
	POPU32(returnValue, response, ndx);		//	ULWord returnValue 1=success 0=fail
	if (inIsRead  &&  !inOutBuffer.RPCDecode(response, ndx))	//	Decode NTV2_POINTER containing DMARead data
		{NBFAIL("Connection " << DEC(ConnectionID()) << " NTV2_POINTER::RPCDecode of resulting DMARead data failed");  return false;}	//	Fail!
	return bool(returnValue);
}

bool NTV2NubRPCAPI::NTV2MessageRemote (NTV2_HEADER *	pInMessage)
{
	if (!IsConnected())
		return false;	//	Not connected
	if (!pInMessage)
		return false;	//	NULL message pointer

	const ULWord msgType (pInMessage->GetType());
	const string typeStr (pInMessage->FourCCToString(msgType));
	vector<uint8_t> request;	//	Data to be transmitted to server
	bool encodeOK(false), decodedOK(false);

	//	Encode the specific message as a request (blob) to transmit to server...
	pInMessage->SetConnectionID(ConnectionID());	//	Set the connection ID before encoding the request
	switch (msgType)
	{
		case NTV2_TYPE_GETREGS:			encodeOK = AsNTV2GetRegs(pInMessage)->RPCEncode(request);		break;
		case NTV2_TYPE_SETREGS:			encodeOK = AsNTV2SetRegs(pInMessage)->RPCEncode(request);		break;
		case NTV2_TYPE_BANKGETSET:		encodeOK = AsNTV2BankGetSet(pInMessage)->RPCEncode(request);	break;
		case NTV2_TYPE_ACSTATUS:		encodeOK = AsACStatus(pInMessage)->RPCEncode(request);			break;
		case NTV2_TYPE_ACFRAMESTAMP:	encodeOK = AsACFrameStamp(pInMessage)->RPCEncode(request);		break;
		case NTV2_TYPE_ACXFER:			encodeOK = AsACTransfer(pInMessage)->RPCEncode(request);		break;
		case NTV2_TYPE_AJABITSTREAM:	encodeOK = AsNTV2Bitstream(pInMessage)->RPCEncode(request);		break;

		case NTV2_TYPE_ACTASK:
		case NTV2_TYPE_SDISTATS:		NBWARN("Connection " << DEC(ConnectionID()) << " message " << typeStr << " not implemented");  return false;	//	Fail!

		//	These are not applicable for remote:
		case NTV2_TYPE_AJADEBUGLOGGING:
		case NTV2_TYPE_AJABUFFERLOCK:	NBDBG("Connection " << DEC(ConnectionID()) << " message " << typeStr << " is a 'no-op' for remote devices");  return true;	//	Success!

		default:						NBFAIL("Connection " << DEC(ConnectionID()) << " message " << typeStr << " not handled");  return false;	//	Fail!
	}

	if (!encodeOK)
		{NBFAIL("Message " << typeStr << " RPCEncode failed");  return false;}

	//	Transmit the request to the server, and wait for the response...
	NBDBG("Connection " << DEC(ConnectionID()) << " sending " << DEC(request.size()) << "-byte 'mesg' request " << typeStr << "...");
	vector<uint8_t> response (mpRPCClient->call("mesg", request).as<vector<uint8_t> >());
	NBDBG("Connection " << DEC(ConnectionID()) << " received " << DEC(response.size()) << "-byte 'mesg' response " << typeStr);

	//	Decode the server response...
	size_t ndx(0);
	switch (msgType)
	{
		case NTV2_TYPE_GETREGS:			decodedOK = AsNTV2GetRegs(pInMessage)->RPCDecode(response, ndx);	break;
		case NTV2_TYPE_SETREGS:			decodedOK = AsNTV2SetRegs(pInMessage)->RPCDecode(response, ndx);	break;
		case NTV2_TYPE_BANKGETSET:		decodedOK = AsNTV2BankGetSet(pInMessage)->RPCDecode(response, ndx);	break;
		case NTV2_TYPE_ACSTATUS:		decodedOK = AsACStatus(pInMessage)->RPCDecode(response, ndx);		break;
		case NTV2_TYPE_ACFRAMESTAMP:	decodedOK = AsACFrameStamp(pInMessage)->RPCDecode(response, ndx);	break;
		case NTV2_TYPE_ACXFER:			decodedOK = AsACTransfer(pInMessage)->RPCDecode(response, ndx);		break;
		case NTV2_TYPE_AJABITSTREAM:	decodedOK = AsNTV2Bitstream(pInMessage)->RPCDecode(response, ndx);	break;
		default:						NTV2_ASSERT(false && "Should never get here");
	}
	if (!decodedOK)
		{NBFAIL("Message " << typeStr << " RPCDecode failed");  return false;}	//	Fail!
	return true;	//	Success!
}

ostream & NTV2NubRPCAPI::Print (ostream & oss) const
{
	return oss;
}

bool NTV2NubRPCAPI::NTV2QueryDevices (NTV2DeviceIDSerialPairs & outDevices)
{
	outDevices.clear();
	ostringstream err;
	vector<uint8_t> listRequest, listResponse;
	if (!IsConnected())
		mpRPCClient = new rpc::client(HostName(), NTV2NUBPORT);
	do
	{
		if (!mpRPCClient)
			{err << "Unable to allocate RPC client for '" << HostName() << "'";  break;}
		//	Try initial contact with server -- build "list" request...
		listRequest.push_back(NTV2_RPC_PROTOCOL_VERSION);	//	1st byte is protocol version
		listRequest.push_back(AJA_NTV2_SDK_VERSION_MAJOR);	//	2nd byte is client SDK major version
		listRequest.push_back(AJA_NTV2_SDK_VERSION_MINOR);	//	3rd byte is client SDK minor version
		listRequest.push_back(AJA_NTV2_SDK_VERSION_POINT);	//	4th byte is client SDK point version
		try
		{
			listResponse = mpRPCClient->call("list", listRequest).as<vector<uint8_t> >();
		}
		catch (rpc::system_error & e)
		{	//	Usually "Connection refused" --- ntv2nub not running?
			err << e.what() << " by server '" << HostName() << "'";
		}
		if (!err.str().empty())
			break;

		//	Check "list" response from server...
		if (listResponse.size() < 4)
			{err << "Bad " << DEC(listResponse.size()) << "-byte 'list' response from server, expected 4-byte device count";  break;}
		uint32_t numDevices(0);
		size_t ndx(0);
		POPU32(numDevices, listResponse, ndx);	//	Connection ID	uint32_t
		if (ndx + numDevices * 12 > listResponse.size())
			{err << DEC(listResponse.size()) << "-byte 'list' response too small, numDevices=" << DEC(numDevices);  break;}
		NBINFO(DEC(numDevices) << " device" << (numDevices == 1 ? "" : "s") << " found" << (numDevices ? ":" : ""));
		for (ULWord devNum(0);  devNum < numDevices;  devNum++)
		{
			uint32_t devID(0);  uint64_t serNum(0);
			POPU32(devID, listResponse, ndx);	//	NTV2DeviceID	uint32_t
			POPU64(serNum, listResponse, ndx);	//	Serial Number	uint64_t
			NTV2DeviceIDSerialPair(NTV2DeviceID(devID), serNum);
			NBINFO("'" << HostName() << "' device " << DEC(devNum) << ": " << ::NTV2DeviceIDToString(NTV2DeviceID(devID))
					<< " '" << CNTV2Card::SerialNum64ToString (serNum) << "'");
		}	//	for each device
	} while (false);
	if (!err.str().empty())	//	FAILED?
	{
		NBFAIL(err.str());	cerr << "## FAILED: " << err.str() << endl;
		delete mpRPCClient;
		mpRPCClient = AJA_NULL;
		return false;
	}
	return true;
}

NTV2RPCAPI * NTV2RPCAPI::FindNTV2SoftwareDevice (const NTV2ConnectParams & inParams)
{
	//	Look for a dylib/DLL named inName...
	//	Look for dylibs/DLLs in user persistence store:
	AJASystemInfo sysInfo (AJA_SystemInfoMemoryUnit_Megabytes, AJA_SystemInfoSection_Path);
	string inName, userPath, queryStr, errStr;
	NTV2ConnectParamsCIter iter(inParams.find(kConnectParamHost));
	if (iter == inParams.end())
		{NBCFAIL("Missing or empty '" << kConnectParamHost << "'" << " connect param value");  return AJA_NULL;}	//	No "host"
	inName = iter->second;
	if (AJA_FAILURE(sysInfo.GetValue(AJA_SystemInfoTag_Path_Firmware, userPath)))
		{NBCFAIL("AJA_SystemInfoTag_Path_Firmware failed");  return AJA_NULL;}	//	Can't get firmware folder
	iter = inParams.find(kConnectParamQuery);
	if (iter != inParams.end())
	{
		queryStr = iter->second;
		if (!queryStr.empty())
			if (queryStr[0] == '?')
				queryStr.erase(0,1);	//	Remove leading '?'
	}
	NBCDBG("AJA firmware path is '" << userPath << "', host='" << inName << "', query='" << queryStr << "'");
#if defined(AJAMac)
	//	Open the .dylib...
	if (userPath.find("Firmware/") == string::npos)
		{NBCFAIL("'" << userPath << "' doesn't end with 'Firmware/'");  return AJA_NULL;}
	userPath.erase(userPath.find("Firmware/"), 9);	//	Lop off trailing "Firmware/"
	userPath += inName + ".dylib";					//	Append inName.dylib
	void* pHandle = ::dlopen(userPath.c_str(), RTLD_LAZY);
	if (!pHandle)
	{	const char * pErrorStr(::dlerror());
		errStr =  pErrorStr ? pErrorStr : "";
		NBCFAIL("Unable to open dylib '" << userPath << "': " << errStr);
		return AJA_NULL;
	}	//	dlopen failed
	NBCINFO("Dylib '" << userPath << "' opened");

	//	Get pointer to its CreateNTV2SoftwareDevice function...
	const string exportedFunctionName("CreateNTV2SoftwareDevice");
	uint64_t * pFunc = reinterpret_cast<uint64_t*>(::dlsym(pHandle, exportedFunctionName.c_str()));
	if (!pFunc)
	{	const char * pErrStr(::dlerror());
		errStr =  pErrStr ? pErrStr : "";
		NBCFAIL("'dlsym' failed for '" << exportedFunctionName << "' in '" << userPath << "': " << errStr);
		::dlclose(pHandle);
		return AJA_NULL;
	}
	NBCINFO("Ready to call '" << exportedFunctionName << "' in '" << userPath << "'");

	//	Call its CreateNTV2SoftwareDevice function to instantiate the NTV2RPCAPI object...
	fpCreateNTV2SoftwareDevice pCreateFunc = reinterpret_cast<fpCreateNTV2SoftwareDevice>(pFunc);
	NTV2_ASSERT(pCreateFunc);
	NTV2RPCAPI * pRPCObject = (*pCreateFunc) (pHandle, queryStr, AJA_NTV2_SDK_VERSION);
	//	It's now someone else's responsibility to call ::dlclose
	if (!pRPCObject)
	{
		NBCFAIL("'CreateNTV2SoftwareDevice' returned NULL for query='" << queryStr << "'");
		::dlclose(pHandle);
		return AJA_NULL;
	}
	AJA_sINFO(AJA_DebugUnit_RPCClient, AJAFUNC << ": Success: 'CreateNTV2SoftwareDevice' returned non-NULL object");
	return pRPCObject;	//	It's caller's responsibility to delete pRPCObject
#elif defined(MSWindows)
#elif defined(AJALinux)
#else
#endif
	return AJA_NULL;
}	//	FindNTV2SoftwareDevice

#endif	//	defined(NTV2_NUB_CLIENT_SUPPORT)

ostream & operator << (ostream & oss, const NTV2Dictionary & inDict)
{
	for (NTV2DictConstIter it(inDict.begin());  it != inDict.end();  ++it)
	{
		string key(it->first), val(it->second);
		if (key.find(' ') != string::npos)
			oss << "'" << key << "'=";
		else
			oss << key << "=";
		if (val.find(' ') != string::npos)
			oss << "'" << val << "'" << endl;
		else
			oss << val << endl;
	}
	return oss;
}
