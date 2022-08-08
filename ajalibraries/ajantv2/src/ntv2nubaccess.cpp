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
	#define	DLL_EXTENSION	".dylib"
	#define	FIRMWARE_FOLDER	"Firmware/"
#elif defined(AJALinux)
	#define	DLL_EXTENSION	".so"
	#define	FIRMWARE_FOLDER	"firmware/"
#elif defined(MSWindows)
	#define	DLL_EXTENSION	".DLL"
	#define	FIRMWARE_FOLDER	"Firmware/"
#endif
#if !defined(MSWindows)
	#include <dlfcn.h>
#endif

using namespace std;

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
#define	NBSFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)
#define	NBSWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)
#define	NBSNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)
#define	NBSINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)
#define	NBSDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_RPCServer, AJAFUNC << ": " << __x__)


string NTV2Dictionary::valueForKey (const string & inKey) const
{
	DictConstIter it(mDict.find(inKey));
	if (it == mDict.end())
		return "";
	return it->second;
}

uint16_t NTV2Dictionary::u16ValueForKey (const string & inKey, const uint16_t inDefault) const
{
	string str(valueForKey(inKey));
	if (str.empty())
		return inDefault;
	if (str.find("0x") == 0  ||  str.find("0X") == 0)
	{
		str.erase(0,2);
		if (str.empty())
			return inDefault;
		return aja::stoul(str, AJA_NULL, 16);
	}
	if (str.find("x") == 0  ||  str.find("X") == 0)
	{
		str.erase(0,1);
		if (str.empty())
			return inDefault;
		return aja::stoul(str, AJA_NULL, 16);
	}
	if (str.find("o") == 0  ||  str.find("O") == 0)
	{
		str.erase(0,1);
		if (str.empty())
			return inDefault;
		return aja::stoul(str, AJA_NULL, 8);
	}
	if (str.find("b") == 0  ||  str.find("B") == 0)
	{
		str.erase(0,1);
		if (str.empty())
			return inDefault;
		return aja::stoul(str, AJA_NULL, 2);
	}
	return aja::stoul(str, AJA_NULL, 10);
}

ostream & NTV2Dictionary::Print (ostream & oss, const bool inCompact) const
{
	if (inCompact)
		for (DictConstIter it(mDict.begin());  it != mDict.end();  )
		{
			const string & key(it->first), val(it->second), quote(val.find(' ') != string::npos ? "'" : "");
			oss << key << "=" << quote << val << quote;
			if (++it != mDict.end())
				oss << " ";
		}
	else if (empty())
		oss << "0 entries";
	else
	{
		const int kyWdth(int(largestKeySize()+0)), valWdth(int(largestValueSize()+0));
		oss << string(size_t(kyWdth), '-') << "   " << string(size_t(valWdth), '-') << endl;
		for (DictConstIter it(mDict.begin());  it != mDict.end();  )
		{
			const string & key(it->first), val(it->second);
			oss << std::setw(kyWdth) << key << " : " << val;
			if (++it != mDict.end())
				oss << endl;
		}
	}
	return oss;
}

NTV2StringSet NTV2Dictionary::keys (void) const
{
	NTV2StringSet result;
	for (DictConstIter it(mDict.begin());  it != mDict.end();  ++it)
		result.insert(it->first);
	return result;
}

size_t NTV2Dictionary::largestKeySize (void) const
{
	size_t result(0);
	for (DictConstIter it(mDict.begin());  it != mDict.end();  ++it)
		if (it->first.length() > result)
			result = it->first.length();
	return result;
}

size_t NTV2Dictionary::largestValueSize (void) const
{
	size_t result(0);
	for (DictConstIter it(mDict.begin());  it != mDict.end();  ++it)
		if (it->second.length() > result)
			result = it->second.length();
	return result;
}

size_t NTV2Dictionary::UpdateFrom (const NTV2Dictionary & inDict)
{
	size_t numUpdated(0);
	for (DictConstIter it(inDict.mDict.begin());  it != inDict.mDict.end();  ++it)
		if (hasKey(it->first))
			{mDict[it->first] = it->second;   numUpdated++;}
	return numUpdated;
}

size_t NTV2Dictionary::AddFrom (const NTV2Dictionary & inDict)
{
	size_t numAdded(0);
	for (DictConstIter it(inDict.mDict.begin());  it != inDict.mDict.end();  ++it)
		if (!hasKey(it->first))
			{mDict[it->first] = it->second;   numAdded++;}
	return numAdded;
}


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
			mResult.insert(kConnectParamScheme, kLegalSchemeNTV2Local);
			mResult.insert(kConnectParamDevModel, tokModelName);
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
			mResult.insert(kConnectParamDevSerial, tokSerial);
			mResult.insert(kConnectParamScheme, kLegalSchemeNTV2Local);
			break;
		}
		if (isDeviceID)
		{
			mPos = posDevID;
			mResult.insert(kConnectParamDevID, tokDevID);
			mResult.insert(kConnectParamScheme, kLegalSchemeNTV2Local);
			break;
		}
		if (isIndexNum)
		{
			mPos = posIndexNum;
			mResult.insert(kConnectParamDevIndex, tokIndexNum);
			mResult.insert(kConnectParamScheme, kLegalSchemeNTV2Local);
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
			mResult.insert(kConnectParamScheme, tokScheme);
			mResult.insert(kConnectParamHost, host);
			if (!port.empty())
				mResult.insert(kConnectParamPort, port);

			//	Parse resource path...
			posRsrc = mPos;
			if (ParseResourcePath(posRsrc, rsrcPath))
				{mPos = posRsrc;  mResult.insert(kConnectParamResource, rsrcPath);}
			//	Parse query...
			size_t posQuery(mPos);
			NTV2Dictionary params;
			if (ParseQuery(posQuery, params))
			{
				mResult.insert(kConnectParamQuery, DeviceSpec().substr(mPos, posQuery-mPos+1));
				mQueryParams = params;
				mPos = posQuery;
			}
			if (mPos < SpecLength())
				{err << "Extra character(s) at " << DEC(mPos);  AddError(err.str());  break;}
		}
	} while (false);
	#if defined(_DEBUG)
		ostringstream oss;
		if (Successful())
			{oss << "Success: '" << DeviceSpec() << "'  --  "; Print(oss); AJA_sDEBUG(AJA_DebugUnit_Application, oss.str());}
		else
			{oss << "Failed: "; PrintErrors(oss); AJA_sERROR(AJA_DebugUnit_Application, oss.str());}
	#endif	//	defined(_DEBUG)
}	//	Parse

ostream & NTV2DeviceSpecParser::Print (ostream & oss, const bool inDumpResults) const
{
	oss << (IsLocalDevice() ? "local " : "") << "device";
	if (HasResult(kConnectParamDevSerial))
		oss << " serial '" << DeviceSerial() << "'";
	else if (HasResult(kConnectParamDevModel))
		oss << " model '" << DeviceModel() << "'";
	else if (HasResult(kConnectParamDevID))
		oss << " ID '" << DeviceID() << "'";
	else if (HasResult(kConnectParamDevIndex))
		oss << " " << DeviceIndex();
	if (HasResult(kConnectParamHost))
		oss << " host '" << Result(kConnectParamHost) << "'";
	if (HasResult(kConnectParamPort))
		oss << " port " << Result(kConnectParamPort);
	if (HasResult(kConnectParamResource))
		oss << " resource '" << Result(kConnectParamResource) << "'";
	if (HasResult(kConnectParamQuery))
		oss << " query '" << Result(kConnectParamQuery) << "'";
	if (inDumpResults)
		{oss << endl; Results().Print(oss, /*compact?*/false);}
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

bool NTV2DeviceSpecParser::ParseAlphaNumeric (size_t & pos, string & outToken, const std::string & inOtherChars)
{
	outToken.clear();
	string tokAlphaNum;
	while (pos < SpecLength())
	{
		const char ch(CharAt(pos));
		if (!IsLetter(ch) && !IsDecimalDigit(ch) && inOtherChars.find(ch) == string::npos)
			break;
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
	while (ParseAlphaNumeric(dnsPos, name, "_-"))	//	also allow '_' and '-'
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
		outParams.insert(key, value);
		ch = CharAt(queryPos);
		if (ch != '&')
			break;
	}
	if (!outParams.empty())
		pos = queryPos;
	return !outParams.empty();
}

bool NTV2DeviceSpecParser::IsSupportedScheme (const string & inScheme)
{
	return inScheme.find("ntv2") == 0;	//	Starts with "ntv2"
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
	NTV2RPCClientAPI
*****************************************************************************************************************************************************/
NTV2RPCClientAPI::NTV2RPCClientAPI (const NTV2ConnectParams & inParams)
	:	mConnectParams	(inParams)
{
	AJADebug::Open();
}

NTV2RPCClientAPI::~NTV2RPCClientAPI ()
{
	if (IsConnected())
		NTV2Disconnect();
}

bool NTV2RPCClientAPI::SetConnectParams (const NTV2ConnectParams & inNewParams, const bool inAugment)
{
	if (IsConnected())
		{NBFAIL("Cannot set connect params while connected");  return false;}
	size_t oldCount(mConnectParams.size()), updated(0), added(0);
	if (inAugment)
	{
		updated = mConnectParams.UpdateFrom(inNewParams);
		added = mConnectParams.AddFrom(inNewParams);
		NBDBG(DEC(updated) << " connect param(s) updated, " << DEC(added) << " added: " << mConnectParams);
	}
	else
	{
		mConnectParams = inNewParams;
		NBDBG(DEC(oldCount) << " connect param(s) removed, replaced with " << inNewParams);
	}
	if (mConnectParams.empty())
		NBWARN("No connect params");
	return true;
}

ostream & NTV2RPCClientAPI::Print (ostream & oss) const
{
	oss << (IsConnected() ? "Connected" : "Disconnected");
	if (IsConnected() && !Name().empty())
		oss << " to '" << Name() << "'";
	return oss;
}

string NTV2RPCClientAPI::Description (void) const
{
	return "";
}

bool NTV2RPCClientAPI::NTV2Connect (void)
{
	if (IsConnected())
		NTV2Disconnect();
	return NTV2OpenRemote();
}

bool NTV2RPCClientAPI::NTV2Disconnect (void)
{
	return NTV2CloseRemote();
}

bool NTV2RPCClientAPI::NTV2ReadRegisterRemote (const ULWord regNum, ULWord & outRegValue, const ULWord regMask, const ULWord regShift)
{	(void) regNum;  (void) outRegValue; (void) regMask; (void) regShift;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2WriteRegisterRemote	(const ULWord regNum, const ULWord regValue, const ULWord regMask, const ULWord regShift)
{	(void) regNum; (void) regValue; (void) regMask; (void) regShift;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2AutoCirculateRemote (AUTOCIRCULATE_DATA & autoCircData)
{	(void) autoCircData;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2WaitForInterruptRemote (const INTERRUPT_ENUMS eInterrupt, const ULWord timeOutMs)
{	(void) eInterrupt; (void) timeOutMs;
	return false;	//	UNIMPLEMENTED
}

#if !defined(NTV2_DEPRECATE_16_3)
	bool NTV2RPCClientAPI::NTV2DriverGetBitFileInformationRemote (BITFILE_INFO_STRUCT & bitFileInfo, const NTV2BitFileType bitFileType)
	{	(void) bitFileType;
		::memset(&bitFileInfo, 0, sizeof(bitFileInfo));
		return false;	//	UNIMPLEMENTED
	}

	bool NTV2RPCClientAPI::NTV2DriverGetBuildInformationRemote (BUILD_INFO_STRUCT & buildInfo)
	{
		::memset(&buildInfo, 0, sizeof(buildInfo));
		return false;	//	UNIMPLEMENTED
	}

	bool NTV2RPCClientAPI::NTV2DownloadTestPatternRemote (const NTV2Channel channel, const NTV2PixelFormat testPatternFBF,
														const UWord signalMask, const bool testPatDMAEnb, const ULWord testPatNum)
	{	(void) channel;  (void) testPatternFBF; (void) signalMask; (void) testPatDMAEnb; (void) testPatNum;
		return false;	//	UNIMPLEMENTED
	}

	bool NTV2RPCClientAPI::NTV2ReadRegisterMultiRemote (const ULWord numRegs, ULWord & outFailedRegNum, NTV2RegInfo outRegs[])
	{	(void) numRegs; (void) outFailedRegNum; (void) outRegs;
		return false;	//	UNIMPLEMENTED
	}

	bool NTV2RPCClientAPI::NTV2GetDriverVersionRemote (ULWord & outDriverVersion)
	{
		outDriverVersion = 0xFFFFFFFF;
		return false;	//	UNIMPLEMENTED
	}
#endif	//	!defined(NTV2_DEPRECATE_16_3)

bool NTV2RPCClientAPI::NTV2DMATransferRemote (	const NTV2DMAEngine inDMAEngine,	const bool inIsRead,	const ULWord inFrameNumber,
												NTV2_POINTER & inOutFrameBuffer,	const ULWord inCardOffsetBytes,
												const ULWord inNumSegments,			const ULWord inSegmentHostPitch,
												const ULWord inSegmentCardPitch,	const bool inSynchronous)
{	(void) inDMAEngine; (void) inIsRead;	(void) inFrameNumber; (void) inOutFrameBuffer;
	(void) inCardOffsetBytes; (void) inNumSegments; (void) inSegmentHostPitch;
	(void) inSegmentCardPitch; (void) inSynchronous;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2MessageRemote (NTV2_HEADER * pInMessage)
{	(void) pInMessage;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2OpenRemote (void)
{
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2CloseRemote (void)
{
	mConnectParams.clear();
	return true;
}

NTV2RPCClientAPI * NTV2RPCClientAPI::CreateClient (const NTV2ConnectParams & inParams)	//	CLASS METHOD
{
	//	Scheme dictates which plugin to try loading...
	if (!inParams.hasKey(kConnectParamScheme))
		{NBCFAIL("Missing scheme -- params: " << inParams);  return AJA_NULL;}	//	No scheme
	string scheme(inParams.valueForKey(kConnectParamScheme));
	if (scheme.find("ntv2"))	//	scheme must start with "ntv2"
		{NBCFAIL("Scheme '" << inParams.valueForKey(kConnectParamScheme) << "' results in empty plugin name");  return AJA_NULL;}	//	No "host"
	scheme.erase(0,4);	//	Remainder of scheme yields DLL/dylib/so name...

	//	Look for plugins in the "AJA" folder (usually parent folder of our "firmware" folder)...
	string pluginName(scheme), pluginPath, errStr;
	AJASystemInfo sysInfo (AJA_SystemInfoMemoryUnit_Megabytes, AJA_SystemInfoSection_Path);
	if (AJA_FAILURE(sysInfo.GetValue(AJA_SystemInfoTag_Path_Firmware, pluginPath)))
		{NBCFAIL("AJA_SystemInfoTag_Path_Firmware failed");  return AJA_NULL;}	//	Can't get firmware folder
	NBCDBG("AJA firmware path is '" << pluginPath << "', seeking '" << pluginName << ".dylib'");

#if !defined(MSWindows)
	//	On MacOS and Linux, our plugins are dylibs and so's, respectively...
	if (pluginPath.find(FIRMWARE_FOLDER) == string::npos)
		{NBSFAIL("'" << pluginPath << "' doesn't end with '" << FIRMWARE_FOLDER << "'");  return AJA_NULL;}
	pluginPath.erase(pluginPath.find(FIRMWARE_FOLDER), 9);	//	Lop off trailing "Firmware/"
	pluginPath += pluginName + DLL_EXTENSION;				//	Append pluginName.dylib|so|dll

	//	Open the .dylib...
	void* pHandle = ::dlopen(pluginPath.c_str(), RTLD_LAZY);
	if (!pHandle)
	{	const char * pErrorStr(::dlerror());
		errStr =  pErrorStr ? pErrorStr : "";
		NBCFAIL("Unable to open dylib '" << pluginPath << "': " << errStr);
		return AJA_NULL;
	}	//	dlopen failed
	NBCDBG("Dylib '" << pluginPath << "' opened");

	//	Get pointer to its CreateNTV2SoftwareDevice function...
	uint64_t * pFunc = reinterpret_cast<uint64_t*>(::dlsym(pHandle, kFuncNameCreateClient.c_str()));
	if (!pFunc)
	{	const char * pErrStr(::dlerror());
		errStr =  pErrStr ? pErrStr : "";
		NBCFAIL("'dlsym' failed for '" << kFuncNameCreateClient << "' in '" << pluginPath << "': " << errStr);
		::dlclose(pHandle);
		return AJA_NULL;
	}
	NBCDBG("Calling '" << kFuncNameCreateClient << "' in '" << pluginPath << "'");

	//	Call its Create function to instantiate the NTV2RPCClientAPI object...
	fpCreateClient pCreateFunc = reinterpret_cast<fpCreateClient>(pFunc);
	NTV2_ASSERT(pCreateFunc);
	NTV2RPCClientAPI * pRPCObject = (*pCreateFunc) (pHandle, inParams, AJA_NTV2_SDK_VERSION);
	if (!pRPCObject)
	{
		NBCFAIL("'" << kFuncNameCreateClient << "' failed to return NTV2RPCClientAPI instance using: " << inParams);
		::dlclose(pHandle);
		return AJA_NULL;
	}
	//	It's now someone else's responsibility to call ::dlclose
	NBCINFO("'" << kFuncNameCreateClient << "' in '" << pluginPath << "' created instance " << xHEX0N(uint64_t(pRPCObject),16));
	return pRPCObject;
#else
	//	Need Windows implementation
#endif
	return AJA_NULL;
}	//	CreateClient


/*****************************************************************************************************************************************************
	NTV2RPCServerAPI
*****************************************************************************************************************************************************/

NTV2RPCServerAPI * NTV2RPCServerAPI::CreateServer (const NTV2ConfigParams & inParams)	//	CLASS METHOD
{
	//	Scheme dictates which plugin to try loading...
	if (!inParams.hasKey(kConnectParamScheme))
		{NBSFAIL("Missing scheme");  return AJA_NULL;}	//	No scheme
	string scheme(inParams.valueForKey(kConnectParamScheme));
	if (scheme.find("ntv2"))	//	scheme must start with "ntv2"
		{NBSFAIL("Scheme '" << inParams.valueForKey(kConnectParamScheme) << "' results in empty plugin name");  return AJA_NULL;}	//	No "host"
	scheme.erase(0,4);	//	Remainder of scheme yields DLL/dylib/so name...

	//	Look for plugins in the "AJA" folder (usually parent folder of our "firmware" folder)...
	string pluginName(scheme), pluginPath, errStr;
	AJASystemInfo sysInfo (AJA_SystemInfoMemoryUnit_Megabytes, AJA_SystemInfoSection_Path);
	if (AJA_FAILURE(sysInfo.GetValue(AJA_SystemInfoTag_Path_Firmware, pluginPath)))
		{NBSFAIL("AJA_SystemInfoTag_Path_Firmware failed");  return AJA_NULL;}	//	Can't get firmware folder
	NBSDBG("AJA firmware path is '" << pluginPath << "', seeking '" << pluginName << ".dylib'");

#if !defined(MSWindows)
	//	On MacOS and Linux, our plugins are dylibs and so's, respectively...
	if (pluginPath.find(FIRMWARE_FOLDER) == string::npos)
		{NBSFAIL("'" << pluginPath << "' doesn't end with '" << FIRMWARE_FOLDER << "'");  return AJA_NULL;}
	pluginPath.erase(pluginPath.find(FIRMWARE_FOLDER), 9);	//	Lop off trailing "Firmware/"
	pluginPath += pluginName + DLL_EXTENSION;				//	Append pluginName + .dylib|.so|.dll

	//	Open the .dylib...
	void* pHandle = ::dlopen(pluginPath.c_str(), RTLD_LAZY);
	if (!pHandle)
	{	const char * pErrorStr(::dlerror());
		errStr =  pErrorStr ? pErrorStr : "";
		NBSFAIL("Unable to open dylib '" << pluginPath << "': " << errStr);
		return AJA_NULL;
	}	//	dlopen failed
	NBSDBG("Dylib '" << pluginPath << "' opened");

	//	Get pointer to its CreateNTV2SoftwareDevice function...
	uint64_t * pFunc = reinterpret_cast<uint64_t*>(::dlsym(pHandle, kFuncNameCreateServer.c_str()));
	if (!pFunc)
	{	const char * pErrStr(::dlerror());
		errStr =  pErrStr ? pErrStr : "";
		NBSFAIL("'dlsym' failed for '" << kFuncNameCreateServer << "' in '" << pluginPath << "': " << errStr);
		::dlclose(pHandle);
		return AJA_NULL;
	}
	NBSDBG("Calling '" << kFuncNameCreateServer << "' in '" << pluginPath << "'");

	//	Call its Create function to instantiate the NTV2RPCServerAPI object...
	fpCreateServer pCreateFunc = reinterpret_cast<fpCreateServer>(pFunc);
	NTV2_ASSERT(pCreateFunc);
	NTV2RPCServerAPI * pRPCObject = (*pCreateFunc) (pHandle, inParams, AJA_NTV2_SDK_VERSION);
	if (!pRPCObject)
	{
		NBSFAIL("'" << kFuncNameCreateServer << "' failed to return NTV2RPCServerAPI instance using: " << inParams);
		::dlclose(pHandle);
		return AJA_NULL;
	}
	//	It's now someone else's responsibility to call ::dlclose
	NBSINFO("'" << kFuncNameCreateServer << "' in '" << pluginPath << "' created instance " << xHEX0N(uint64_t(pRPCObject),16));
	return pRPCObject;	//	It's caller's responsibility to delete pRPCObject
#else
	//	Missing Windows implementation"
#endif
	return AJA_NULL;
}	//	CreateServer

NTV2RPCServerAPI * NTV2RPCServerAPI::CreateServer (const string & inURL)	//	CLASS METHOD
{
	NTV2DeviceSpecParser parser(inURL);
	if (parser.HasErrors())
		return AJA_NULL;
	return CreateServer(parser.Results());
}

NTV2RPCServerAPI::NTV2RPCServerAPI (const NTV2ConnectParams & inParams)
	:	mConfigParams	(inParams)
{
	NTV2_POINTER spare(&mSpare, sizeof(mSpare));  spare.Fill(0ULL);
	AJADebug::Open();
}

NTV2RPCServerAPI::~NTV2RPCServerAPI()
{
}

void NTV2RPCServerAPI::RunServer (void)
{	//	This function normally should never be called;
	//	It's usually overridden by a subclass
	NBSDBG("Started");
	while (!mSpare[0])
		AJATime::Sleep(500);
	NBSDBG("Terminated");
}	//	ServerFunction

ostream & NTV2RPCServerAPI::Print (ostream & oss) const
{
	oss << mConfigParams;
	return oss;
}

bool NTV2RPCServerAPI::SetConfigParams (const NTV2ConnectParams & inNewParams, const bool inAugment)
{
	size_t oldCount(mConfigParams.size()), updated(0), added(0);
	if (inAugment)
	{
		updated = mConfigParams.UpdateFrom(inNewParams);
		added = mConfigParams.AddFrom(inNewParams);
        NBSDBG(DEC(updated) << " config param(s) updated, " << DEC(added) << " added: " << mConfigParams);
	}
	else
	{
		mConfigParams = inNewParams;
		NBSDBG(DEC(oldCount) << " config param(s) removed, replaced with " << inNewParams);
	}
	if (mConfigParams.empty())
		NBSWARN("No config params");
	return true;
}
