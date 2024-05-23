/* SPDX-License-Identifier: MIT */
/**
	@file		ntv2nubaccess.cpp
	@brief		Implementation of NTV2Dictionary, NTV2DeviceSpecParser, NTV2RPCClientAPI & NTV2RPCServerAPI classes.
	@copyright	(C) 2006-2022 AJA Video Systems, Inc.
**/
#include "ajatypes.h"
#include "ntv2utils.h"
#include "ntv2nubaccess.h"
#include "ntv2publicinterface.h"
#include "ntv2version.h"
#include "ajabase/system/debug.h"
#include "ajabase/common/common.h"
#include "ajabase/system/info.h"
#include "ajabase/system/systemtime.h"
#include <iomanip>
#if !defined(NTV2_PREVENT_PLUGIN_LOAD)
	#include <fstream>
	#include "mbedtls/x509.h"
//	#include "mbedtls/platform.h"
	#include "mbedtls/error.h"
	#include "mbedtls/md.h"
//	#include "mbedtls/pk.h"
	#include "mbedtls/ssl.h"
#endif	//	defined(NTV2_PREVENT_PLUGIN_LOAD)
#if defined(AJAMac)
	#include <CoreFoundation/CoreFoundation.h>
	#include <dlfcn.h>
	#define	DLL_EXTENSION	".dylib"
	#define	FIRMWARE_FOLDER	"Firmware/"
#elif defined(AJALinux)
	#include <dlfcn.h>
	#define	DLL_EXTENSION	".so"
	#define	FIRMWARE_FOLDER	"firmware/"
#elif defined(MSWindows)
	#define	DLL_EXTENSION	".dll"
	#define	FIRMWARE_FOLDER	"Firmware\\"
#elif defined(AJABareMetal)
	#define	DLL_EXTENSION	".dll"
	#define	FIRMWARE_FOLDER	"Firmware\\"
#endif
#define	SIG_EXTENSION	".sig"

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

#define	PLGFAIL(__x__)		AJA_sERROR  (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	PLGWARN(__x__)		AJA_sWARNING(AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	PLGNOTE(__x__)		AJA_sNOTICE (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	PLGINFO(__x__)		AJA_sINFO   (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	PLGDBG(__x__)		AJA_sDEBUG  (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)

#define	P_FAIL(__x__)		if (useStdout) cout << "## ERROR: " << AJAFUNC << ": " << __x__ << endl;    \
							AJA_sERROR  (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	P_WARN(__x__)		if (useStdout) cout << "## WARNING: " << AJAFUNC << ": " << __x__ << endl;    \
							AJA_sWARNING(AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	P_NOTE(__x__)		if (useStdout) cout << "## NOTE: " << AJAFUNC << ": " << __x__ << endl;    \
							AJA_sNOTICE (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	P_INFO(__x__)		if (useStdout) cout << "## INFO: " << AJAFUNC << ": " << __x__ << endl;    \
							AJA_sINFO   (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	P_DBG(__x__)		if (useStdout) cout << "## DEBUG: " << AJAFUNC << ": " << __x__ << endl;    \
							AJA_sDEBUG  (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)


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
		return uint16_t(aja::stoul(str, AJA_NULL, 16));
	}
	if (str.find("x") == 0  ||  str.find("X") == 0)
	{
		str.erase(0,1);
		if (str.empty())
			return inDefault;
		return uint16_t(aja::stoul(str, AJA_NULL, 16));
	}
	if (str.find("o") == 0  ||  str.find("O") == 0)
	{
		str.erase(0,1);
		if (str.empty())
			return inDefault;
		return uint16_t(aja::stoul(str, AJA_NULL, 8));
	}
	if (str.find("b") == 0  ||  str.find("B") == 0)
	{
		str.erase(0,1);
		if (str.empty())
			return inDefault;
		return uint16_t(aja::stoul(str, AJA_NULL, 2));
	}
	return uint16_t(aja::stoul(str, AJA_NULL, 10));
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

bool NTV2Dictionary::deserialize (const string & inStr)
{
	size_t badKVPairs(0), insertFailures(0);
	clear();
	const NTV2StringList keyValPairs (aja::split(inStr, "\n"));
	for (NTV2StringListConstIter it(keyValPairs.begin());  it != keyValPairs.end();  ++it)
	{
		const NTV2StringList keyValPair (aja::split(*it, "\t"));
		if (keyValPair.size() != 2)
			{badKVPairs++;  continue;}
		const string k(keyValPair.at(0)), v(keyValPair.at(1));
		if (!insert(k, v))
			insertFailures++;
	}
	return !empty()  &&  !badKVPairs  &&  !insertFailures;
}

bool NTV2Dictionary::serialize (string & outStr) const
{
	outStr.clear();
	ostringstream oss;
	for (DictConstIter it(mDict.begin());  it != mDict.end();  )
	{
		oss << it->first << "\t" << it->second;
		if (++it != mDict.end())
			oss << "\n";
	}
	outStr = oss.str();
	return !outStr.empty();
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

bool NTV2Dictionary::insert (const string & inKey, const string & inValue)
{
	if (inKey.empty())
		return false;
	if (inKey.find("\t") != string::npos)
		return false;
	if (inKey.find("\n") != string::npos)
		return false;
	if (inValue.find("\t") != string::npos)
		return false;
	if (inValue.find("\n") != string::npos)
		return false;
	mDict[inKey] = inValue;
	return true;
}

size_t NTV2Dictionary::updateFrom (const NTV2Dictionary & inDict)
{
	size_t numUpdated(0);
	for (DictConstIter it(inDict.mDict.begin());  it != inDict.mDict.end();  ++it)
		if (hasKey(it->first))
			{mDict[it->first] = it->second;   numUpdated++;}
	return numUpdated;
}

size_t NTV2Dictionary::addFrom (const NTV2Dictionary & inDict)
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
			{oss << "NTV2DeviceSpecParser::Parse success: '" << DeviceSpec() << "'  --  "; Print(oss); AJA_sDEBUG(AJA_DebugUnit_Application, oss.str());}
		else
			{oss << "NTV2DeviceSpecParser::Parse failed: "; PrintErrors(oss); AJA_sERROR(AJA_DebugUnit_Application, oss.str());}
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
			if (!IsUpperLetter(ch) && !IsDecimalDigit(ch) && ch != '-' && ch != ' ')
				break;
			++posAlphaNum;  tokAlphaNum += ch;
		}
		if (tokAlphaNum.length() < 2)	//	At least 2 upper-case chars
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
	queryPos++;

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

#if defined(MSWindows)
	static string WinErrStr (const DWORD inErr)
	{
		string result("foo");
		LPVOID lpMsgBuf;
		const DWORD res(FormatMessage (	FORMAT_MESSAGE_ALLOCATE_BUFFER
										| FORMAT_MESSAGE_FROM_SYSTEM
										| FORMAT_MESSAGE_IGNORE_INSERTS,	//	dwFlags
										AJA_NULL,							//	lpSource:		n/a
										inErr,								//	dwMessageId:	n/a
										MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	//	dwLanguageId
										(LPTSTR) &lpMsgBuf,					//	output buffer
										0,									//	output buffer size, in TCHARs
										AJA_NULL));							//	user params
		if (lpMsgBuf)
		{
			result = reinterpret_cast<const char *>(lpMsgBuf);
			LocalFree(lpMsgBuf);
		}
		return result;
	}
#endif	//	MSWindows


/*****************************************************************************************************************************************************
	NTV2RPCClientAPI
*****************************************************************************************************************************************************/

NTV2RPCClientAPI::NTV2RPCClientAPI (NTV2ConnectParams inParams)
	:	mConnectParams	(inParams)
{
	AJADebug::Open();
}

NTV2RPCClientAPI::~NTV2RPCClientAPI ()
{
	if (IsConnected())
		NTV2Disconnect();
}

NTV2ConnectParams NTV2RPCClientAPI::ConnectParams (void) const
{
	AJAAutoLock tmp(&mParamLock);
	return mConnectParams;
}

bool NTV2RPCClientAPI::HasConnectParam (const string & inParam) const
{
	AJAAutoLock tmp(&mParamLock);
	return mConnectParams.hasKey(inParam);
}

string NTV2RPCClientAPI::ConnectParam (const string & inParam) const
{
	AJAAutoLock tmp(&mParamLock);
	return mConnectParams.valueForKey(inParam);
}

bool NTV2RPCClientAPI::ConnectHasScheme (void) const
{
	return HasConnectParam(kConnectParamScheme);
}

bool NTV2RPCClientAPI::SetConnectParams (const NTV2ConnectParams & inNewParams, const bool inAugment)
{
	if (IsConnected())
		{NBFAIL("Cannot set connect params while connected");  return false;}
	AJAAutoLock tmp(&mParamLock);
	size_t oldCount(mConnectParams.size()), updated(0), added(0);
	if (inAugment)
	{
		updated = mConnectParams.updateFrom(inNewParams);
		added = mConnectParams.addFrom(inNewParams);
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
												NTV2Buffer & inOutFrameBuffer,	const ULWord inCardOffsetBytes,
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

bool NTV2RPCClientAPI::NTV2GetBoolParamRemote (const ULWord inParamID,  ULWord & outValue)
{	(void) inParamID;
	outValue = 0;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2GetNumericParamRemote (const ULWord inParamID,  ULWord & outValue)
{	(void) inParamID;
	outValue = 0;
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2GetSupportedRemote (const ULWord inEnumsID, ULWordSet & outSupported)
{	(void) inEnumsID;
	outSupported.clear();
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2OpenRemote (void)
{
	return false;	//	UNIMPLEMENTED
}

bool NTV2RPCClientAPI::NTV2CloseRemote (void)
{
	AJAAutoLock tmp(&mParamLock);
	mConnectParams.clear();
	return true;
}

bool NTV2RPCClientAPI::ParseQueryParam (const NTV2Dictionary & inParams, NTV2Dictionary & outQueryParams)
{
	if (!inParams.hasKey(kConnectParamQuery))
		return false;
	string queryStr(inParams.valueForKey(kConnectParamQuery));
	if (!queryStr.empty())
		if (queryStr[0] == '?')
			queryStr.erase(0,1);	//	Remove leading '?'
	const NTV2StringList strs(aja::split(queryStr, "&"));
	for (NTV2StringListConstIter it(strs.begin());  it != strs.end();  ++it)
	{
		string str(*it), key, value;
		if (str.find("=") == string::npos)
		{	//	No assignment (i.e. no '=') --- just insert key with empty value...
			key = aja::lower(str);
			outQueryParams.insert(key, value);
			PLGDBG("'" << key << "' = ''");
			continue;
		}
		NTV2StringList pieces(aja::split(str,"="));
		if (pieces.empty())
			continue;
		key = aja::lower(pieces.at(0));
		if (pieces.size() > 1)
			value = pieces.at(1);
		if (key.empty())
			{PLGWARN("Empty key '" << key << "'");  continue;}
		if (outQueryParams.hasKey(key))
			PLGDBG("Param '" << key << "' value '" << outQueryParams.valueForKey(key) << "' to be replaced with '" << value << "'");
		outQueryParams.insert(key, ::PercentDecode(value));
		PLGDBG("'" << key << "' = '" << outQueryParams.valueForKey(key) << "'");
	}	//	for each &param
	return true;
}


#if !defined(NTV2_PREVENT_PLUGIN_LOAD)
static string mbedErrStr (const int returnCode)
{
	static NTV2Buffer sErrBuff(4096);
	string str;
	mbedtls_strerror(returnCode, sErrBuff, sErrBuff);
	sErrBuff.GetString (str, /*U8Offset*/0, /*maxSize*/sErrBuff);
	return str;
}

static bool ExtractCertInfo (NTV2Dictionary & outInfo, const string & inStr)
{
	outInfo.clear();
	if (inStr.empty())
		return false;
	string keyPrefix;
	NTV2StringList lines(aja::split(inStr, "\n"));
	for (size_t lineNdx(0);  lineNdx < lines.size();  lineNdx++)
	{
		string line (lines.at(lineNdx));
		const bool indented (line.empty() ? false : line.at(0) == ' ');
		aja::strip(line);
		if (line.empty())	//	there shouldn't be empty lines...
			continue;		//	...but skip them anyway if they happen to appear
		NTV2StringList keyValPair (aja::split(line, " : "));
		if (keyValPair.size() != 2)
		{
			if (keyValPair.size() == 1)
			{
				keyPrefix = keyValPair.at(0);
				aja::replace(keyPrefix, ":", "");
				aja::strip(keyPrefix);
				continue;	//	next line
			}
			PLGFAIL("cert info line " << DEC(lineNdx+1) << " '" << line << "' has "
					<< DEC(keyValPair.size()) << " column(s) -- expected 2");
			return false;
		}
		string key(keyValPair.at(0)), val(keyValPair.at(1));
		if (key.empty())
			{PLGFAIL("cert info line " << DEC(lineNdx+1) << " '" << line << "' empty key for value '" << val << "'"); continue;}
		if (indented  &&  !keyPrefix.empty())
			{aja::strip(key); key = keyPrefix + ": " + key;}
		else
			{aja::strip(key);  keyPrefix.clear();}
		if (outInfo.hasKey(key))
			val = outInfo.valueForKey(key) + ", " + val;
		outInfo.insert(key, aja::strip(val));	//	ignore errors for now
	}	//	for each info line
	return true;
}

static bool ExtractIssuerInfo (NTV2Dictionary & outInfo, const string & inStr, const string & inParentKey)
{
	outInfo.clear();
	if (inStr.empty())
		return false;
	string str(inStr);
	NTV2StringList pairs(aja::split(aja::replace(str, "\\,", ","), ", ")), normalized;
	string lastKey;
	for (size_t ndx(0);  ndx < pairs.size();  ndx++)
	{
		string assignment (pairs.at(ndx));
		if (assignment.find('=') == string::npos)
		{
			if (!lastKey.empty())
				outInfo.insert (lastKey, outInfo.valueForKey(lastKey) + ", " + assignment);
		}
		else
		{
			NTV2StringList pieces (aja::split(assignment, "="));
			if (pieces.size() != 2)
				{PLGFAIL("'" << inParentKey << "' assignment '" << assignment << "' has " << pieces.size() << " component(s) -- expected 2"); continue;}
			lastKey = pieces.at(0);
			string val(pieces.at(1));
			outInfo.insert (aja::strip(lastKey), val);
		}
	}	//	for each key/val assignment
	return true;
}
#endif  //  !defined(NTV2_PREVENT_PLUGIN_LOAD)

static uint64_t* GetSymbolAddress (void * pHandle, const string & inSymbolName, string & outErrorMsg)
{
	uint64_t * result(0);
	ostringstream err;
	outErrorMsg.clear();
	if (inSymbolName.empty())
		return result;
	#if defined(MSWindows)
		result = reinterpret_cast<uint64_t*>(::GetProcAddress(
			reinterpret_cast<HMODULE>(pHandle), inSymbolName.c_str()));
		if (!result)
			err << "'GetProcAddress' failed for '" << inSymbolName << "': " << WinErrStr(::GetLastError());
	#elif defined(AJABareMetal)
		// TODO
	#else	//	MacOS or Linux
		result = reinterpret_cast<uint64_t*>(::dlsym(pHandle, inSymbolName.c_str()));
		if (!result)
		{	const char * pErrorStr(::dlerror());
			const string errStr (pErrorStr ? pErrorStr : "");
			err << "'dlsym' failed for '" << inSymbolName << "': " << errStr;
		}
	#endif	//	MacOS or Linux
	outErrorMsg = err.str();
	return result;
}

//	Loads NTV2 plugin (specified in 'inParams'), and returns address of given function
static uint64_t * GetNTV2PluginFunction (NTV2ConnectParams & params, const string & inFuncName)
{
	NTV2Dictionary	queryParams;
	NTV2RPCClientAPI::ParseQueryParam (params, queryParams);
	const bool showX509Cert(queryParams.hasKey("showx509cert"));
	const bool isVerbose(queryParams.hasKey("verbose"));
	const bool useStdout(queryParams.hasKey("cout") || queryParams.hasKey("stdout"));
	if (isVerbose)
	{	cout << "## NOTE: Original params:" << endl;
		params.Print(cout, false) << endl;
	}

	//	URL scheme dictates which plugin to load...
	if (!params.hasKey(kConnectParamScheme))
		{P_FAIL("Missing scheme -- params: " << params);  return AJA_NULL;}	//	No scheme
	string scheme(params.valueForKey(kConnectParamScheme));
	if (scheme.find("ntv2"))	//	Scheme must start with "ntv2"
		{P_FAIL("Scheme '" << params.valueForKey(kConnectParamScheme) << "' results in empty plugin name");  return AJA_NULL;}	//	No "host"
	scheme.erase(0,4);	//	Remainder of scheme yields DLL/dylib/so name...

	//	Look for plugins in the "AJA" folder (parent folder of our "firmware" folder)...
	string pluginName(scheme), pluginPath, sigPath, dllsFolder, errStr;
	AJASystemInfo sysInfo (AJA_SystemInfoMemoryUnit_Megabytes, AJA_SystemInfoSection_Path);
	if (AJA_FAILURE(sysInfo.GetValue(AJA_SystemInfoTag_Path_Firmware, pluginPath)))
		{P_FAIL("AJA_SystemInfoTag_Path_Firmware failed");  return AJA_NULL;}	//	Can't get firmware folder
	PLGDBG("AJA firmware path is '" << pluginPath << "', seeking '" << pluginName << DLL_EXTENSION "'");
	if (pluginPath.find(FIRMWARE_FOLDER) == string::npos)
		{P_FAIL("'" << pluginPath << "' doesn't end with '" << FIRMWARE_FOLDER << "'");  return AJA_NULL;}

	//	Determine full path to plugin & signature files...
	pluginPath.erase(pluginPath.find(FIRMWARE_FOLDER), 9);	//	Lop off trailing "Firmware/"
	dllsFolder = pluginPath;
	dllsFolder.erase(dllsFolder.length()-1,1);	//	Lop off trailing slash or backslash
	pluginPath += pluginName;					//	Append plugin name
	sigPath = pluginPath + SIG_EXTENSION;
	pluginPath += DLL_EXTENSION;				//	Append extension

#if defined(NTV2_PREVENT_PLUGIN_LOAD)
	P_FAIL("This SDK was built without the ability to load 3rd party plugins");
	return AJA_NULL;
#else	//	!defined(NTV2_PREVENT_PLUGIN_LOAD)
	//	Load contents of plugin & sig files into sigContent & dllContent buffers...
	NTV2Buffer sigContent, dllContent;
	{	NTV2Buffer tmp(512*1024*1024);	//	no more than 500MB
		ifstream dllF(pluginPath.c_str(), std::ios::in | std::ios::binary);
		if (!dllF.good())
			{P_FAIL("Plugin file '" << pluginPath << "' missing");  return AJA_NULL;}
		if (!dllF.read(tmp, tmp.GetByteCount()).eof())
			{P_WARN("EOF not reached in plugin file '" << pluginPath << "' -- over 500MB in size?");  return AJA_NULL;}
		tmp.Truncate(size_t(dllF.gcount()));
		dllContent = tmp;

		tmp.Allocate(512*1024*1024);	//	no more than 500MB
		ifstream sigF(sigPath.c_str(), std::ios::in | std::ios::binary);
		if (!sigF.good())
			{P_FAIL("Signature file '" << sigPath << "' missing");  return AJA_NULL;}
		if (!sigF.read(tmp, tmp.GetByteCount()).eof())
			{P_WARN("EOF not reached in signature file '" << sigPath << "' -- over 500MB in size?");  return AJA_NULL;}
		tmp.Truncate(size_t(sigF.gcount()));
		sigContent = tmp;
	}

	//	Decode sigContent...
	NTV2Dictionary dict;
	{	const string dictStr (reinterpret_cast<const char*>(sigContent.GetHostPointer()), size_t(sigContent.GetByteCount()));
		if (!dict.deserialize(dictStr))
			{P_FAIL("Unable to decode signature file '" << sigPath << "'");  return AJA_NULL;}
	}
	PLGDBG(DEC(dict.keys().size()) << " keys found in signature file '" << sigPath << "': " << dict.keys());
	NTV2Buffer checksumFromSigFile, x509CertFromSigFile, signature;
	if (!dict.hasKey(kNTV2PluginSigFileKey_X509Certificate))
		{P_FAIL("Signature file '" << sigPath << "' missing '" << kNTV2PluginSigFileKey_X509Certificate << "' key");  return AJA_NULL;}
	if (!dict.hasKey(kNTV2PluginSigFileKey_Signature))
		{P_FAIL("Signature file '" << sigPath << "' missing '" << kNTV2PluginSigFileKey_Signature << "' key");  return AJA_NULL;}
	if (!x509CertFromSigFile.SetFromHexString(dict.valueForKey(kNTV2PluginSigFileKey_X509Certificate)))
		{P_FAIL("'SetFromHexString' failed to decode X509 certificate extracted from '" << sigPath << "' key '" << kNTV2PluginSigFileKey_X509Certificate << "'");  return AJA_NULL;}
	if (!signature.SetFromHexString(dict.valueForKey(kNTV2PluginSigFileKey_Signature)))
		{P_FAIL("'SetFromHexString' failed to decode signature extracted from '" << sigPath << "' key '" << kNTV2PluginSigFileKey_Signature << "'");  return AJA_NULL;}

	//	Grab the signing certificate found in the .sig file...
	mbedtls_x509_crt crt;			//	Container for X509 certificate
	mbedtls_x509_crt_init(&crt);	//	Initialize it as empty
	int ret = mbedtls_x509_crt_parse(&crt, x509CertFromSigFile, x509CertFromSigFile);
	if (ret)
	{	P_FAIL("'mbedtls_x509_crt_parse' returned " << ret << " (" << mbedErrStr(ret) << ") for X509 cert found in '" << sigPath << "'");
		mbedtls_x509_crt_free(&crt);
		return AJA_NULL;
	}

	//	Extract certificate info...
	NTV2Dictionary certInfo, issuerInfo, subjectInfo;
	{
		NTV2Buffer msgBuff(4096);
		int msgLength (mbedtls_x509_crt_info (msgBuff, msgBuff, /*prefixString*/"", &crt));
		string msg (msgBuff, size_t(msgLength));
		if (msg.empty())
		{	P_FAIL("'mbedtls_x509_crt_info' returned no info for X509 cert found in '" << sigPath << "'");
			return AJA_NULL;
		}
		if (showX509Cert)
			cout	<< "## DEBUG: Raw X509 certificate info extracted from signature file '" << sigPath << "':" << endl
					<< "          " << msg << endl;
		if (!ExtractCertInfo (certInfo, msg))
			return AJA_NULL;
		if (isVerbose)
		{	cout << "## NOTE: X509 certificate info extracted from signature file '" << sigPath << "':" << endl;
			certInfo.Print(cout, false) << endl;
		}
		if (certInfo.hasKey("issuer name"))
			if (!ExtractIssuerInfo (issuerInfo, certInfo.valueForKey("issuer name"), "issuer name"))
				return AJA_NULL;
		if (certInfo.hasKey("subject name"))
			if (!ExtractIssuerInfo (subjectInfo, certInfo.valueForKey("subject name"), "subject name"))
				return AJA_NULL;
		if (isVerbose  &&  !issuerInfo.empty())
		{	cout << "## NOTE: 'issuer name' info:" << endl;
			issuerInfo.Print(cout, false) << endl;
		}
		if (isVerbose  &&  !subjectInfo.empty())
		{	cout << "## NOTE: 'subject name' info:" << endl;
			subjectInfo.Print(cout, false) << endl;
		}
		if (!issuerInfo.hasKey(kNTV2PluginX500AttrKey_CommonName))
		{	P_FAIL("Missing issuer key '" << kNTV2PluginX500AttrKey_CommonName << "' in X509 certificate from '" << sigPath << "'");
			return AJA_NULL;
		}
		if (!issuerInfo.hasKey(kNTV2PluginX500AttrKey_OrganizationName))
		{	P_FAIL("Missing issuer key '" << kNTV2PluginX500AttrKey_OrganizationName << "' in X509 certificate from '" << sigPath << "'");
			return AJA_NULL;
		}
		params.addFrom(certInfo);	//	Store certInfo key/value pairs into client/server instance's params...
	}

	//	Compute SHA256 hash of plugin...
	NTV2Buffer checksumFromDLL(32);
	ret = mbedtls_md_file (mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), pluginPath.c_str(), checksumFromDLL);
	if (ret)
	{	P_FAIL("'mbedtls_md_file' returned " << ret << " (" << mbedErrStr(ret) << ") for '" << pluginPath << "'");
		return AJA_NULL;
	}
	if (isVerbose)   {string str; if (checksumFromDLL.toHexString(str)) cout << "## DEBUG: Digest: " << str << endl;}

	//	Verify the dylib/DLL/so signature...
	ret = mbedtls_pk_verify (&crt.pk, MBEDTLS_MD_SHA256,
							/*msgHash*/checksumFromDLL, /*msgHashLength*/0,//checksumFromDLL,
							/*signatureToVerify*/signature, /*signatureLength*/signature);
	if (ret)
	{	P_FAIL("'mbedtls_pk_verify' returned " << ret << " (" << mbedErrStr(ret) << ") for '" << sigPath << "'");
		return AJA_NULL;
	}
	mbedtls_x509_crt_free(&crt);	//	Done using the mbedtls_x509_crt struct
	P_DBG("'" << pluginPath << "' is properly signed");

	//	Load/open the shared library...
	ostringstream loadErr;
	#if defined(MSWindows)
		//	Open the DLL (Windows)...
		std::wstring dllsFolderW;
		aja::string_to_wstring(dllsFolder, dllsFolderW);
		if (!AddDllDirectory(dllsFolderW.c_str()))
		{	P_FAIL("AddDllDirectory '" << pluginPath << "' failed: " << WinErrStr(::GetLastError()));
			return AJA_NULL;
		}	//	AddDllDirectory failed
		HMODULE pHandle = ::LoadLibraryExA(LPCSTR(pluginPath.c_str()), AJA_NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
		if (!pHandle)
			loadErr << "Unable to open '" << pluginPath << "' in '" << dllsFolder << "': " << WinErrStr(::GetLastError());
	#elif defined(AJABareMetal)
		// TODO
		loadErr << "AJABareMetal unimplemented";
		void *pHandle = AJA_NULL;
	#else	//	MacOS or Linux
		//	Open the .dylib (MacOS) or .so (Linux)...
		void* pHandle = ::dlopen(pluginPath.c_str(), RTLD_LAZY);
		if (!pHandle)
		{	const char * pErrorStr(::dlerror());
			errStr =  pErrorStr ? pErrorStr : "";
			loadErr << "Unable to open '" << pluginPath << "': " << errStr;
		}	//	dlopen failed
	#endif	//	MacOS or Linux
	if (!pHandle)
		{P_FAIL(loadErr.str());  return AJA_NULL;}
	PLGDBG("'" << pluginPath << "' opened");

	uint64_t * pResult (AJA_NULL);
	do	//	do only once
	{
		//	Get address of required GetRegInfo function...
		uint64_t * pInfoJSON(::GetSymbolAddress(pHandle, kFuncNameGetRegInfo, errStr));
		if (!pInfoJSON)
			{P_FAIL("'" << pluginPath << "': " << errStr);  break;}

		//	Call GetRegInfo to obtain registration info...
		NTV2Dictionary regInfo;
		fpGetRegistrationInfo pRegInfo = reinterpret_cast<fpGetRegistrationInfo>(pInfoJSON);
		NTV2_ASSERT(pRegInfo);
		if (!(*pRegInfo)(uint32_t(AJA_NTV2_SDK_VERSION), regInfo))
			{P_FAIL("'" << pluginPath << "': '" << kFuncNameGetRegInfo << "' failed");  break;}
		PLGDBG("'" << pluginPath << "': '" << kFuncNameGetRegInfo << "': " << regInfo);

		//	Check for required registration info keys...
		if (regInfo.empty())
			{P_FAIL("'" << pluginPath << "': no registration info (empty)");  break;}
		NTV2StringList missingRegInfoKeys;
		static const NTV2StringList reqKeys = {kNTV2PluginRegInfoKey_Vendor, kNTV2PluginRegInfoKey_CommonName,
												kNTV2PluginRegInfoKey_ShortName, kNTV2PluginRegInfoKey_LongName,
												kNTV2PluginRegInfoKey_Description, kNTV2PluginRegInfoKey_Copyright};
		for (size_t ndx(0);  ndx < reqKeys.size();  ndx++)
			if (!regInfo.hasKey(reqKeys.at(ndx)))
				missingRegInfoKeys.push_back(reqKeys.at(ndx));
		if (!missingRegInfoKeys.empty())
		{	P_FAIL("'" << pluginPath << "': missing key(s) in registration info: '"
					<< aja::join(missingRegInfoKeys, "','") << "'");
			break;	//	fail
		}

		//	Check that vendor matches common name in signature...
		const string	cnReg(regInfo.valueForKey(kNTV2PluginRegInfoKey_CommonName)),
						cnCert(issuerInfo.valueForKey(kNTV2PluginX500AttrKey_CommonName));
		const string	onReg(regInfo.valueForKey(kNTV2PluginRegInfoKey_Vendor)),
						onCert(issuerInfo.valueForKey(kNTV2PluginX500AttrKey_OrganizationName));
		if (onReg != onCert)
		{	P_FAIL("Vendor name (key='" << kNTV2PluginRegInfoKey_Vendor << "') \"" << onReg << "\" from plugin \""
					<< pluginPath << "\" doesn't match organization name (key='" << kNTV2PluginX500AttrKey_OrganizationName
					<< "') \"" << onCert << "\" from X509 certificate in '" << sigPath << "'");
			break;	//	fail
		}
		if (cnReg != cnCert)
		{	P_FAIL("Common name (key='" << kNTV2PluginRegInfoKey_CommonName << "') \"" << cnReg << "\" from plugin \""
					<< pluginPath << "\" doesn't match common name (key='" << kNTV2PluginX500AttrKey_CommonName
					<< "') \"" << cnCert << "\" from X509 certificate in '" << sigPath << "'");
			break;	//	fail
		}
		params.addFrom(regInfo);	//	Add regInfo key/val pairs into 'params'

		//	Finally, the last step ---- get address of requested function...
		pResult = ::GetSymbolAddress(pHandle, inFuncName, errStr);
		if (!pResult)
			{P_FAIL("'" << pluginPath << "': " << errStr);  break;}
	} while (false);	//	One time only
	if (!pResult)
	{	//	Close the dylib/so/DLL...
		#if defined(AJABareMetal)
			// TODO
		#elif !defined(MSWindows)
			::dlclose(pHandle);
		#else	//	macOS or Linux
			::FreeLibrary(pHandle);
		#endif
		P_DBG("'" << pluginPath << "' closed");
	}
	else
		P_DBG("Calling '" << inFuncName << "' in '" << pluginPath << "'");
	if (isVerbose)
	{	cout << "## NOTE: Final params for '" << pluginPath << "':" << endl;
		params.Print(cout, false) << endl;
	}
	return pResult;
#endif	//	!defined(NTV2_PREVENT_PLUGIN_LOAD)
}	//	GetNTV2PluginFunction


NTV2RPCClientAPI * NTV2RPCClientAPI::CreateClient (NTV2ConnectParams & params)	//	CLASS METHOD
{
	fpCreateClient pFunc (reinterpret_cast<fpCreateClient>(::GetNTV2PluginFunction(params, kFuncNameCreateClient)));
	if (!pFunc)
		return AJA_NULL;

	//	Call plugin's Create function to instantiate the NTV2RPCClientAPI object...
	NTV2RPCClientAPI * pRPCObject = (*pFunc) (AJA_NULL, params, AJA_NTV2_SDK_VERSION);
	if (!pRPCObject)
		NBCFAIL("'" << kFuncNameCreateClient << "' returned NULL client instance from: " << params);
	else
		NBCINFO("'" << kFuncNameCreateClient << "' created client instance " << xHEX0N(uint64_t(pRPCObject),16));
	return pRPCObject;
}	//	CreateClient


/*****************************************************************************************************************************************************
	NTV2RPCServerAPI
*****************************************************************************************************************************************************/

NTV2RPCServerAPI * NTV2RPCServerAPI::CreateServer (NTV2ConfigParams & inParams)	//	CLASS METHOD
{
	fpCreateServer pFunc (reinterpret_cast<fpCreateServer>(::GetNTV2PluginFunction(inParams, kFuncNameCreateServer)));
	if (!pFunc)
		return AJA_NULL;

	//	Call plugin's Create function to instantiate the NTV2RPCServerAPI object...
	NTV2RPCServerAPI * pRPCObject = (*pFunc) (AJA_NULL, inParams, AJA_NTV2_SDK_VERSION);
	if (!pRPCObject)
		NBSFAIL("'" << kFuncNameCreateServer << "' returned NULL server instance from: " << inParams);
	else
		NBSINFO("'" << kFuncNameCreateServer << "' created server instance " << xHEX0N(uint64_t(pRPCObject),16));
	return pRPCObject;	//	It's caller's responsibility to delete pRPCObject
}	//	CreateServer

NTV2RPCServerAPI * NTV2RPCServerAPI::CreateServer (const string & inURL)	//	CLASS METHOD
{
	NTV2DeviceSpecParser parser(inURL);
	if (parser.HasErrors())
		return AJA_NULL;
	NTV2ConfigParams parms(parser.Results());
	return CreateServer(parms);
}

NTV2RPCServerAPI::NTV2RPCServerAPI (NTV2ConnectParams inParams)
	:	mConfigParams	(inParams)
{
	NTV2Buffer spare(&mSpare, sizeof(mSpare));  spare.Fill(0ULL);
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

NTV2ConfigParams NTV2RPCServerAPI::ConfigParams (void) const
{
	AJAAutoLock tmp(&mParamLock);
	return mConfigParams;
}

bool NTV2RPCServerAPI::HasConfigParam (const string & inParam) const
{
	AJAAutoLock tmp(&mParamLock);
	return mConfigParams.hasKey(inParam);
}

string NTV2RPCServerAPI::ConfigParam (const string & inParam) const
{
	AJAAutoLock tmp(&mParamLock);
	return mConfigParams.valueForKey(inParam);
}

bool NTV2RPCServerAPI::SetConfigParams (const NTV2ConnectParams & inNewParams, const bool inAugment)
{
	AJAAutoLock tmp(&mParamLock);
	size_t oldCount(mConfigParams.size()), updated(0), added(0);
	if (inAugment)
	{
		updated = mConfigParams.updateFrom(inNewParams);
		added = mConfigParams.addFrom(inNewParams);
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
