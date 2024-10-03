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
#include "ajabase/system/systemtime.h"
#include "ajabase/system/thread.h"
#include <iomanip>
#if !defined(NTV2_PREVENT_PLUGIN_LOAD)
	#include <fstream>
	#include "mbedtls/x509.h"
	#include "mbedtls/error.h"
	#include "mbedtls/md.h"
	#include "mbedtls/ssl.h"
#endif	//	defined(NTV2_PREVENT_PLUGIN_LOAD)
#if defined(AJAMac)
	#include <CoreFoundation/CoreFoundation.h>
	#include <dlfcn.h>
	#define	DLL_EXTENSION	".dylib"
	#define PATH_DELIMITER	"/"
	#define	FIRMWARE_FOLDER	"Firmware"
#elif defined(AJALinux)
	#include <dlfcn.h>
	#define	DLL_EXTENSION	".so"
	#define PATH_DELIMITER	"/"
	#define	FIRMWARE_FOLDER	"firmware"
#elif defined(MSWindows)
	#define	DLL_EXTENSION	".dll"
	#define PATH_DELIMITER	"\\"
	#define	FIRMWARE_FOLDER	"Firmware"
#elif defined(AJABareMetal)
	#define	DLL_EXTENSION	".so"
	#define PATH_DELIMITER	"/"
	#define	FIRMWARE_FOLDER	"firmware"
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

#define	P_FAIL(__x__)		do														\
							{														\
								ostringstream _os_;									\
								_os_ << AJAFUNC << ": " << __x__;					\
								if (useStdout())									\
									cout << "## ERROR: " << _os_.str() << endl;		\
								AJA_sERROR (AJA_DebugUnit_Plugins, _os_.str());		\
								errMsg = _os_.str();								\
							} while (false)
#define	P_WARN(__x__)		if (useStdout()) cout << "## WARNING: " << AJAFUNC << ": " << __x__ << endl;	\
							AJA_sWARNING(AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	P_NOTE(__x__)		if (useStdout()) cout << "## NOTE: " << AJAFUNC << ": " << __x__ << endl;		\
							AJA_sNOTICE (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	P_INFO(__x__)		if (useStdout()) cout << "## INFO: " << AJAFUNC << ": " << __x__ << endl;		\
							AJA_sINFO   (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	P_DBG(__x__)		if (useStdout()) cout << "## DEBUG: " << AJAFUNC << ": " << __x__ << endl;		\
							AJA_sDEBUG  (AJA_DebugUnit_Plugins, AJAFUNC << ": " << __x__)
#define	_DEBUGSTATS_		//	Define this to log above construct/destruct & open/close tallies
#if defined(_DEBUGSTATS_)
	#define PDBGX(__x__)	AJA_sDEBUG	(AJA_DebugUnit_Plugins, INSTP(this) << "::" << AJAFUNC << ": " << __x__)
#else
	#define PDBGX(__x__)	
#endif


//	Stats
static uint32_t gBaseConstructCount(0);		//	Number of NTV2RPCBase constructor calls made
static uint32_t gBaseDestructCount(0);		//	Number of NTV2RPCBase destructor calls made
static uint32_t gClientConstructCount(0);	//	Number of NTV2RPCClient constructor calls made
static uint32_t gClientDestructCount(0);	//	Number of NTV2RPCClient destructor calls made
static uint32_t gServerConstructCount(0);	//	Number of NTV2RPCServer constructor calls made
static uint32_t gServerDestructCount(0);	//	Number of NTV2RPCServer destructor calls made
static uint32_t gLoaderConstructCount(0);	//	Number of NTV2PluginLoader constructor calls made
static uint32_t gLoaderDestructCount(0);	//	Number of NTV2PluginLoader destructor calls made
static uint32_t gPluginConstructCount(0);	//	Number of NTV2Plugin constructor calls made
static uint32_t gPluginDestructCount(0);	//	Number of NTV2Plugin destructor calls made


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
	if (IsLocalDevice())
		oss << "local device";
	else if (HasScheme())
		oss << "device '" << Scheme() << "'";
	else
		oss << "device";
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


#if !defined(NTV2_PREVENT_PLUGIN_LOAD)

/*****************************************************************************************************************************************************
	NTV2Plugin
*****************************************************************************************************************************************************/

class NTV2Plugin;
typedef AJARefPtr<NTV2Plugin>	NTV2PluginPtr;

//	Wraps handle returned from dlopen/LoadLibrary, calls dlclose/FreeLibrary upon destruction
class NTV2Plugin
{
	public:
		static bool	LoadPlugin (const string & path, const string & folderPath, NTV2PluginPtr & outPtr, string & outErrMsg, const bool inUseStdout);

	public:
		#if defined(MSWindows)
					NTV2Plugin (HMODULE handle, const string & path, const bool useStdout);
		inline		operator HMODULE() const	{return mHandle;}
		#else
					NTV2Plugin (void * handle, const string & path, const bool useStdout);
		inline 		operator void*() const		{return mHandle;}
		#endif
					~NTV2Plugin (void);
		inline bool	isLoaded (void) const		{return mHandle && !mPath.empty() ? true : false;}
		void *		addressForSymbol (const string & inSymbol, string & outErrorMsg);

	private:
					NTV2Plugin();
					NTV2Plugin(const NTV2Plugin & rhs);
					NTV2Plugin & operator = (const NTV2Plugin & rhs);
		inline bool useStdout(void)	{return mUseStdout;}
	private:
		#if defined(MSWindows)
		HMODULE	mHandle;
		#else
		void *	mHandle;
		#endif
		string	mPath;
		bool	mUseStdout;
};	//	NTV2Plugin

//	One-stop-shop to load a plugin & instantiate its NTV2Plugin instance
bool NTV2Plugin::LoadPlugin (const string & path, const string & folderPath, NTV2PluginPtr & outPtr, string & outErrMsg, const bool inUseStdout)
{
	ostringstream loadErr;
	#if defined(AJABareMetal)
		return false;	// unimplemented
	#elif defined(MSWindows)
		//	Open the DLL (Windows)...
		std::wstring dllsFolderW;
		aja::string_to_wstring(folderPath, dllsFolderW);
		if (!AddDllDirectory(dllsFolderW.c_str()))
		{
			loadErr << "AddDllDirectory '" << path << "' failed: " << WinErrStr(::GetLastError());
			return false;
		}	//	AddDllDirectory failed
		HMODULE h = ::LoadLibraryExA(LPCSTR(path.c_str()), AJA_NULL, LOAD_LIBRARY_SEARCH_USER_DIRS);
		if (!h)
			loadErr << "Unable to open '" << path << "': " << WinErrStr(::GetLastError());
	#else	//	MacOS or Linux
		//	Open the .dylib (MacOS) or .so (Linux)...
		void * h = ::dlopen(path.c_str(), RTLD_LAZY);
		if (!h)
		{
			const char * pErrorStr(::dlerror());
			const string errStr (pErrorStr ? pErrorStr : "");
			loadErr << "Unable to open '" << path << "': " << errStr;
		}	//	dlopen failed
	#endif	//	MacOS or Linux
	if (!loadErr.str().empty())
		outErrMsg = loadErr.str();
	if (!h)
		return false;
	outPtr = new NTV2Plugin(h, path, inUseStdout);
	return outPtr;
}

NTV2Plugin::NTV2Plugin()
{
	NTV2_ASSERT(false);
	AJAAtomic::Increment(&gPluginConstructCount);
}

NTV2Plugin::NTV2Plugin(const NTV2Plugin & rhs)
{
	NTV2_ASSERT(false);
	AJAAtomic::Increment(&gPluginConstructCount);
}

NTV2Plugin & NTV2Plugin::operator = (const NTV2Plugin & rhs)
{
	NTV2_ASSERT(false);
	return *this;
}

#if defined(MSWindows)
NTV2Plugin::NTV2Plugin (HMODULE handle, const string & path, const bool inUseStdout)
#else
NTV2Plugin::NTV2Plugin (void * handle, const string & path, const bool inUseStdout)
#endif
	:	mHandle(handle),
		mPath(path)
{
	NTV2_ASSERT(mHandle);
	NTV2_ASSERT(!mPath.empty());
	AJAAtomic::Increment(&gPluginConstructCount);
	P_NOTE("Dynamic/shared library '" << mPath << "' (" << INSTP(mHandle) << ") loaded, "
			<< DEC(gPluginConstructCount) << " created, " << DEC(gPluginDestructCount) << " destroyed");
}

NTV2Plugin::~NTV2Plugin (void)
{
	if (mHandle)
	#if defined(AJABareMetal)
		;	//	unimplemented
	#elif !defined(MSWindows)
		::dlclose(mHandle);
	#else	//	macOS or Linux
		::FreeLibrary(mHandle);
	#endif
	AJAAtomic::Increment(&gPluginDestructCount);
	P_NOTE("Dynamic/shared library '" << mPath << "' (" << INSTP(mHandle) << ") unloaded, "
			<< DEC(gPluginConstructCount) << " created, " << DEC(gPluginDestructCount) << " destroyed");
	mHandle = AJA_NULL;
	mPath.clear();
}

void * NTV2Plugin::addressForSymbol (const string & inSymbolName, string & outErrorMsg)
{
	outErrorMsg.clear();
	if (!mHandle)
		return AJA_NULL;
	if (inSymbolName.empty())
		return AJA_NULL;
	void * result(AJA_NULL);
	ostringstream err;
	#if defined(AJABareMetal)
		// TODO
	#elif defined(MSWindows)
		result = reinterpret_cast<void*>(::GetProcAddress(reinterpret_cast<HMODULE>(mHandle), inSymbolName.c_str()));
		if (!result)
			err << "'GetProcAddress' failed for '" << inSymbolName << "': " << WinErrStr(::GetLastError());
	#else	//	MacOS or Linux
		result = ::dlsym(mHandle, inSymbolName.c_str());
		if (!result)
		{	const char * pErrorStr(::dlerror());
			const string errStr (pErrorStr ? pErrorStr : "");
			err << "'dlsym' failed for '" << inSymbolName << "': " << errStr;
		}
	#endif	//	MacOS or Linux
	outErrorMsg = err.str();
	return result;
}	//	addressForSymbol


/*****************************************************************************************************************************************************
	@brief	A singleton that tracks and monitors loaded plugins, and frees them when no one is using them.
	@bug	This doesn't work on some platforms when libajantv2 is statically linked into the plugin. When
			the plugin loads, the plugin gets its own separate libajantv2 static globals, and thus, a second
			set of libajantv2 singletons, including this one.
*****************************************************************************************************************************************************/
class PluginRegistry;
typedef AJARefPtr<PluginRegistry>	PluginRegistryPtr;

//	Singleton that tracks plugin use
class PluginRegistry
{
	public:	//	Class Methods
		static PluginRegistry &	Get (void);
		static void				Terminate (void);
		static inline void		EnableDebugging (const bool inEnable = true)	{sDebugRegistry = inEnable;}
		static inline bool		DebuggingEnabled (void)	{return sDebugRegistry;}

	public:	//	Instance Methods
						PluginRegistry();
						~PluginRegistry();
		bool			loadPlugin (const string & path, const string & folderPath, NTV2PluginPtr & outPtr, string & errMsg, const bool useStdout);
		bool			unloadPlugin (const string & path, string & errMsg);
		bool			pluginIsLoaded (const string & path);
		bool			pluginForPath (const string & path, NTV2PluginPtr & outHandle);
		NTV2StringList	loadedPlugins (void);
		NTV2StringList	pluginStats (void);	//	returns string list, each has pluginPath<tab>refCount
		bool			hasPath (const string & path);
		bool			indexForPath (const string & path, size_t & outIndex);
		ULWord			countForPath (const string & path);
		uint32_t *		refConForPath (const string & path);
		inline bool		useStdout (void) const		{return DebuggingEnabled();}

	private:
		typedef map<string, NTV2PluginPtr>	NTV2PluginMap;
		NTV2StringList	mPluginPaths;	//	List of unique known plugins; only grows, never shrinks
		ULWordSequence	mPluginCounts;	//	Per-plugin instance counts: correlates to gPluginPaths
		ULWordSequence	mCompareCounts;	//	For detecting changes to mPluginCounts
		NTV2PluginMap	mPluginMap;		//	Maps each unique plugin path to its corresponding NTV2Plugin instance
		AJALock			mPluginMapLock;	//	Mutex to serialize access to these registry globals
		AJAThread		mMonitor;		//	Thread that monitors plugin utilization
		bool			mQuitMonitor;	//	Set true to terminate monitor
	private:
		static void		Monitor (AJAThread * pThread, void * pContext);
		void			monitor (void);	//	Monitor thread function
		static PluginRegistryPtr	sSingleton;
		static AJALock				sMutex;
		static bool					sDebugRegistry;
};	//	PluginRegistry

PluginRegistryPtr	PluginRegistry::sSingleton;
AJALock				PluginRegistry::sMutex;
bool				PluginRegistry::sDebugRegistry(false);

PluginRegistry & PluginRegistry::Get (void)
{
	AJAAutoLock tmp(&sMutex);
	if (!sSingleton)
		sSingleton = new PluginRegistry;
	return *sSingleton;
}

void PluginRegistry::Terminate (void)
{
	AJAAutoLock tmp(&sMutex);
	PLGWARN("");
	sSingleton = PluginRegistryPtr();
}

void PluginRegistry::Monitor (AJAThread * pThread, void * pContext)
{	(void) pThread;
	PluginRegistry * pObj (reinterpret_cast<PluginRegistry*>(pContext));
	if (pObj)
		pObj->monitor();
}

PluginRegistry::PluginRegistry()
	:	mQuitMonitor(false)
{
	P_NOTE ("PluginRegistry " << INSTP(this) << " constructed");
	mPluginCounts.reserve(256);
	for (size_t num(0);  num < 256;  num++)
		mPluginCounts.push_back(0);
	mCompareCounts = mPluginCounts;
	mMonitor.Attach(Monitor, this);
	mMonitor.SetPriority(AJA_ThreadPriority_Low);
	mMonitor.Start();
}

PluginRegistry::~PluginRegistry()
{
	mQuitMonitor = true;
	while (mMonitor.Active())
		AJATime::Sleep(10);
	P_NOTE("PluginRegistry singleton " << INSTP(this) << " destroyed:" << endl << aja::join(pluginStats(), "\n"));
}

bool PluginRegistry::loadPlugin (const string & path, const string & folderPath, NTV2PluginPtr & outPtr, string & errMsg, const bool inUseStdout)
{
	AJAAutoLock tmp(&mPluginMapLock);
	outPtr = NTV2PluginPtr();
	if (path.empty())
		{P_FAIL("empty path"); return false;}
	if (pluginForPath(path, outPtr))
	{
		NTV2_ASSERT(hasPath(path));
		return true;
	}
	if (hasPath(path))
		{P_WARN(INSTP(this) << ": '" << path << "': 'pluginForPath' returned false, but 'hasPath' returned true, count=" << countForPath(path));}
	string msg;
	if (!NTV2Plugin::LoadPlugin (path, folderPath, outPtr, msg, inUseStdout))
		{P_FAIL(msg);  return false;}
	P_NOTE(INSTP(this) << ": Dynamic/shared library '" << path << "' loaded");
	mPluginMap[path] = outPtr;
	mPluginPaths.push_back(path);
	mPluginCounts.at(mPluginPaths.size()-1) = 0;
	return true;
}

bool PluginRegistry::unloadPlugin (const string & path, string & errMsg)
{
	AJAAutoLock tmp(&mPluginMapLock);
	NTV2PluginPtr ptr;
	if (path.empty())
		return false;
	if (!pluginForPath(path, ptr))
		{P_FAIL(INSTP(this) << ": '" << path << "' requested to unload, but not loaded");  return false;}
	mPluginMap.erase(path);	//	This should cause NTV2Plugin destructor to be called
	P_NOTE(INSTP(this) << ": '" << path << "' unloaded");
	return true;
}

bool PluginRegistry::pluginIsLoaded (const string & path)
{
	AJAAutoLock tmp(&mPluginMapLock);
	return mPluginMap.find(path) != mPluginMap.end();
}

bool PluginRegistry::pluginForPath (const string & path, NTV2PluginPtr & outHandle)
{
	AJAAutoLock tmp(&mPluginMapLock);
	NTV2PluginMap::const_iterator it(mPluginMap.find(path));
	if (it == mPluginMap.end())
		outHandle = NTV2PluginPtr();
	else
		outHandle = it->second;
	return outHandle;
}

NTV2StringList PluginRegistry::loadedPlugins (void)
{
	NTV2StringList result;
	AJAAutoLock tmp(&mPluginMapLock);
	for (NTV2PluginMap::const_iterator it(mPluginMap.begin());  it != mPluginMap.end();  ++it)
		result.push_back(it->first);
	return result;
}

NTV2StringList PluginRegistry::pluginStats (void)
{
	NTV2StringList result;
	AJAAutoLock tmp(&mPluginMapLock);
	for (size_t ndx(0);  ndx < mPluginPaths.size();  ndx++)
	{
		const string path (mPluginPaths.at(ndx));
		ostringstream oss; oss << path << "\t" << DEC(countForPath(path));
		NTV2PluginPtr p;
		if (pluginForPath(path, p))
			oss << "\t" << (p->isLoaded() ? "loaded" : "unloaded");
		else
			oss << "\t" << "---";
		result.push_back(oss.str());
	}
	return result;
}

bool PluginRegistry::hasPath (const string & path)
{
	size_t ndx(0);
	return indexForPath(path,ndx);
}

bool PluginRegistry::indexForPath (const string & path, size_t & outIndex)
{
	AJAAutoLock tmp(&mPluginMapLock);
	for (outIndex = 0;  outIndex < mPluginPaths.size();  outIndex++)
		if (path == mPluginPaths.at(outIndex))
			return true;
	return false;
}

uint32_t * PluginRegistry::refConForPath (const string & path)
{
	AJAAutoLock tmp(&mPluginMapLock);
	size_t ndx(0);
	if (indexForPath(path, ndx))
		return &mPluginCounts[ndx];
	return AJA_NULL;
}

ULWord PluginRegistry::countForPath (const string & path)
{
	size_t ndx(0);
	if (indexForPath(path, ndx))
		return mPluginCounts.at(ndx);
	return 0;
}

void PluginRegistry::monitor (void)
{
	P_NOTE("PluginRegistry " << INSTP(this) << " monitor started");
	while (!mQuitMonitor)
	{
		{
			AJAAutoLock tmp(&mPluginMapLock);
			for (size_t ndx(0);  ndx < mPluginPaths.size();  ndx++)
			{
				const uint32_t oldCount(mCompareCounts.at(ndx)), newCount(mPluginCounts.at(ndx));
				if (newCount != oldCount)
				{
					string errMsg, path(mPluginPaths.at(ndx));
					if (newCount > oldCount)
						{P_NOTE("PluginRegistry " << INSTP(this) << ": Plugin '" << path << "' utilization "
							<< "increased from " << DEC(oldCount) << " to " << DEC(newCount));}
					else
					{
						P_NOTE("PluginRegistry " << INSTP(this) << ": Plugin '" << path << "' utilization "
							<< "decreased from " << DEC(oldCount) << " to " << DEC(newCount));
						if (newCount == 0)
							unloadPlugin(path, errMsg);
					}	//	else count decreased
					mCompareCounts.at(ndx) = newCount;
				}	//	something changed
			}	//	for each plugin
		}
		AJATime::Sleep(250);
	}
	P_NOTE("PluginRegistry " << INSTP(this) << " monitor stopped");
}

/*****************************************************************************************************************************************************
	@brief	Knows how to load & validate a plugin
*****************************************************************************************************************************************************/
class NTV2PluginLoader
{
	public:	//	Instance Methods
						NTV2PluginLoader (NTV2Dictionary & params);
						~NTV2PluginLoader ();
		void *			getFunctionAddress (const string & inFuncName);
		inline string	pluginPath (void) const		{return mDict.valueForKey(kNTV2PluginInfoKey_PluginPath);}
		inline string	pluginSigPath (void) const	{return mDict.valueForKey(kNTV2PluginInfoKey_PluginSigPath);}
		inline string	pluginsPath (void) const	{return mDict.valueForKey(kNTV2PluginInfoKey_PluginsPath);}
		inline string	pluginBaseName (void) const	{return mDict.valueForKey(kNTV2PluginInfoKey_PluginBaseName);}
		bool			isValidated (void) const;
		inline bool		showParams (void) const		{return mQueryParams.hasKey(kQParamShowParams);}
		void *			refCon (void) const;

	protected:	//	Used internally
		bool			validate (void);
		void *			getSymbolAddress (const string & inSymbolName, string & outErrorMsg);
		bool			getPluginsFolder (string & outPath) const;
		bool			getBaseNameFromScheme (string & outName) const;
		inline bool		isOpen (void)					{return mpPlugin ? mpPlugin->isLoaded() : false;}
		inline bool		useStdout (void) const			{return mQueryParams.hasKey(kQParamLogToStdout);}
		inline bool		isVerbose (void) const			{return mQueryParams.hasKey(kQParamVerboseLogging);}
		inline bool		showCertificate (void) const	{return mQueryParams.hasKey(kQParamShowX509Cert);}
		bool			fail (void);

	private:	//	Instance Data
		NTV2Dictionary &	mDict;			///< @brief	Writeable access to caller's config/connect dictionary
		NTV2Dictionary		mQueryParams;	///< @brief	Query parameters
		NTV2PluginPtr		mpPlugin;		///< @brief	Platform-dependent handle to open plugin .dylib/.dll/.so
		bool                mValidated;
		mutable string		errMsg;

	protected:	//	Class Methods
		static bool		ParseQueryParams (const NTV2Dictionary & inParams, NTV2Dictionary & outQueryParams);
		static bool		ExtractCertInfo (NTV2Dictionary & outInfo, const string & inStr);
		static bool		ExtractIssuerInfo (NTV2Dictionary & outInfo, const string & inStr, const string & inParentKey);
		static string	mbedErrStr (const int mbedtlsReturnCode);
};	//	NTV2PluginLoader


//	Constructor -- peforms all preparatory work: determines which plugin to load, then loads & validates it
NTV2PluginLoader::NTV2PluginLoader (NTV2Dictionary & params)
	:	mDict(params),
		mValidated(false)
{
	PluginRegistry::EnableDebugging(mDict.hasKey(kQParamDebugRegistry) || PluginRegistry::DebuggingEnabled());
	AJAAtomic::Increment(&gLoaderConstructCount);
	const NTV2Dictionary originalParams(mDict);
	if (ParseQueryParams (mDict, mQueryParams)  &&  !mQueryParams.empty())
		mDict.addFrom(mQueryParams);
	if (mDict.hasKey(kNTV2PluginInfoKey_Fingerprint))
		mDict.erase(kNTV2PluginInfoKey_Fingerprint);	//	Be sure caller can't cheat
	P_INFO("Loader created for '" << mDict.valueForKey(kConnectParamScheme) << "', " << DEC(gLoaderConstructCount) << " created, "
			<< DEC(gLoaderDestructCount) << " destroyed");

	//	Determine plugin base name & where to find the dylib/dll/so...
	string pluginBaseName, pluginsFolder;
	if (getBaseNameFromScheme(pluginBaseName)  &&  getPluginsFolder(pluginsFolder))
	{
		const string path (pluginsFolder + PATH_DELIMITER + pluginBaseName);
		const string sigPath (path + SIG_EXTENSION), dllPath (path + DLL_EXTENSION);
		mDict.insert(kNTV2PluginInfoKey_PluginPath, dllPath);
		mDict.insert(kNTV2PluginInfoKey_PluginSigPath, sigPath);
		if (showParams())
		{	cout << "## NOTE: Original params for '" << pluginPath() << "':" << endl;
			originalParams.Print(cout, false) << endl;
		}
		//	Validate the plugin...
		validate();
		if (showParams())
		{	cout << "## NOTE: Final params for '" << pluginPath() << "':" << endl;
			mDict.Print(cout, false) << endl;
		}
	}
}

NTV2PluginLoader::~NTV2PluginLoader ()
{
	AJAAtomic::Increment(&gLoaderDestructCount);
	P_INFO("Loader destroyed for '" << pluginBaseName() << "', " << DEC(gLoaderConstructCount) << " created, "
			<< DEC(gLoaderDestructCount) << " destroyed");
}

string NTV2PluginLoader::mbedErrStr (const int mbedtlsReturnCode)
{
	NTV2Buffer errBuff(4096);
	string str;
	mbedtls_strerror (mbedtlsReturnCode, errBuff, errBuff);
	errBuff.GetString (str, /*U8Offset*/0, /*maxSize*/errBuff);
	return str;
}

bool NTV2PluginLoader::ExtractCertInfo (NTV2Dictionary & outInfo, const string & inStr)
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
}	//	ExtractCertInfo

bool NTV2PluginLoader::ExtractIssuerInfo (NTV2Dictionary & outInfo, const string & inStr, const string & inParentKey)
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
}	//	ExtractIssuerInfo

bool NTV2PluginLoader::ParseQueryParams (const NTV2Dictionary & inParams, NTV2Dictionary & outQueryParams)
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
}	//	ParseQueryParams

void * NTV2PluginLoader::getSymbolAddress (const string & inSymbolName, string & outErrorMsg)
{
	outErrorMsg.clear();
	if (!mpPlugin)
		return AJA_NULL;
	return mpPlugin->addressForSymbol(inSymbolName, outErrorMsg);
}	//	getSymbolAddress

void * NTV2PluginLoader::refCon (void) const
{
	return PluginRegistry::Get().refConForPath(pluginPath());
}

bool NTV2PluginLoader::getPluginsFolder (string & outPath) const
{
	if (!pluginsPath().empty())
		{outPath = pluginsPath();  return true;}	//	already known, assumed to be good

	//	Plugins are expected to be in the "aja" folder (the parent folder of the "aja/firmware" folder)...
	outPath = ::NTV2GetFirmwareFolderPath();
	if (outPath.empty())
		return false;
	PLGDBG("AJA firmware path is '" << outPath << "'");
	if (outPath.find(FIRMWARE_FOLDER) == string::npos)
		{P_FAIL("'" << outPath << "' doesn't end with '" << FIRMWARE_FOLDER << "'");  outPath.clear(); return false;}
	outPath.erase(outPath.find(FIRMWARE_FOLDER), 9);		//	Lop off trailing "Firmware"
	mDict.insert(kNTV2PluginInfoKey_PluginsPath, outPath);	//	Store it in 'PluginsPath'
	if (outPath.back() == PATH_DELIMITER[0])
		outPath.erase(outPath.length() - 1, 1);	//	Lop off trailing path delimiter
	return !outPath.empty();	//	Success if not empty
}

bool NTV2PluginLoader::getBaseNameFromScheme (string & outName) const
{
	if (!pluginBaseName().empty())
		{outName = pluginBaseName();  return true;}	//	already known, assumed to be good

	//	URL scheme determines plugin base name...
	if (!mDict.hasKey(kConnectParamScheme))
		{P_FAIL("Missing scheme -- params: " << mDict);  return false;}	//	No scheme
	string scheme(mDict.valueForKey(kConnectParamScheme));
	if (scheme.find("ntv2") != 0)	//	Scheme must start with "ntv2"
		{P_FAIL("Scheme '" << scheme << "' doesn't start with 'ntv2'");  return false;}	//	Bad scheme
	scheme.erase(0,4);	//	Remove 1st 4 characters;  remainder yields base name
	outName = scheme;
	mDict.insert(kNTV2PluginInfoKey_PluginBaseName, outName);
	return !outName.empty();	//	Success if not empty
}

bool NTV2PluginLoader::fail (void)
{
	if (mDict.hasKey(kNTV2PluginInfoKey_Errors))
	{	const string v(mDict.valueForKey(kNTV2PluginInfoKey_Errors) + "\n" + errMsg);
		mDict.erase(kNTV2PluginInfoKey_Errors);
		mDict.insert(kNTV2PluginInfoKey_Errors, v);
	}
	else
		mDict.insert(kNTV2PluginInfoKey_Errors, errMsg);
	return false;
}

bool NTV2PluginLoader::validate (void)
{
	//	Load contents of plugin & sig files into sigContent & dllContent buffers...
	NTV2Buffer sigContent, dllContent;
	{	NTV2Buffer tmp(512*1024*1024);	//	no more than 500MB
		ifstream dllF(pluginPath().c_str(), std::ios::in | std::ios::binary);
		if (!dllF.good())
			{P_FAIL("Plugin file '" << pluginPath() << "' missing");  return fail();}
		if (!dllF.read(tmp, tmp.GetByteCount()).eof())
			{P_FAIL("EOF not reached in plugin file '" << pluginPath() << "' -- over 500MB in size?");  return fail();}
		tmp.Truncate(size_t(dllF.gcount()));
		dllContent = tmp;

		tmp.Allocate(512*1024*1024);	//	no more than 500MB
		ifstream sigF(pluginSigPath().c_str(), std::ios::in | std::ios::binary);
		if (!sigF.good())
			{P_FAIL("Signature file '" << pluginSigPath() << "' missing");  return fail();}
		if (!sigF.read(tmp, tmp.GetByteCount()).eof())
			{P_FAIL("EOF not reached in signature file '" << pluginSigPath() << "' -- over 500MB in size?");  return fail();}
		tmp.Truncate(size_t(sigF.gcount()));
		sigContent = tmp;
	}

	//	Decode sigContent...
	NTV2Dictionary dict;
	{	const string dictStr (reinterpret_cast<const char*>(sigContent.GetHostPointer()), size_t(sigContent.GetByteCount()));
		if (!dict.deserialize(dictStr))
			{P_FAIL("Unable to decode signature file '" << pluginSigPath() << "'");  return fail();}
	}
	P_DBG(DEC(dict.keys().size()) << " keys found in signature file '" << pluginSigPath() << "': " << dict.keys());
	NTV2Buffer checksumFromSigFile, x509CertFromSigFile, signature;
	if (!dict.hasKey(kNTV2PluginSigFileKey_X509Certificate))
		{P_FAIL("Signature file '" << pluginSigPath() << "' missing '" << kNTV2PluginSigFileKey_X509Certificate << "' key");  return fail();}
	if (!dict.hasKey(kNTV2PluginSigFileKey_Signature))
		{P_FAIL("Signature file '" << pluginSigPath() << "' missing '" << kNTV2PluginSigFileKey_Signature << "' key");  return fail();}
	if (!x509CertFromSigFile.SetFromHexString(dict.valueForKey(kNTV2PluginSigFileKey_X509Certificate)))
		{P_FAIL("'SetFromHexString' failed to decode X509 certificate extracted from '" << pluginSigPath() << "' key '" << kNTV2PluginSigFileKey_X509Certificate << "'");  return fail();}
	if (!signature.SetFromHexString(dict.valueForKey(kNTV2PluginSigFileKey_Signature)))
		{P_FAIL("'SetFromHexString' failed to decode signature extracted from '" << pluginSigPath() << "' key '" << kNTV2PluginSigFileKey_Signature << "'");  return fail();}

	//	Grab the signing certificate found in the .sig file...
	mbedtls_x509_crt crt;			//	Container for X509 certificate
	mbedtls_x509_crt_init(&crt);	//	Initialize it as empty
	int ret = mbedtls_x509_crt_parse(&crt, x509CertFromSigFile, x509CertFromSigFile);
	if (ret)
	{	P_FAIL("'mbedtls_x509_crt_parse' returned " << ret << " (" << mbedErrStr(ret) << ") for X509 cert found in '" << pluginSigPath() << "'");
		mbedtls_x509_crt_free(&crt);
		return fail();
	}

	//	Extract certificate info...
	NTV2Dictionary certInfo, issuerInfo, subjectInfo;
	{
		NTV2Buffer msgBuff(4096);
		int msgLength (mbedtls_x509_crt_info (msgBuff, msgBuff, /*prefixString*/"", &crt));
		string msg (msgBuff, size_t(msgLength));
		if (msg.empty())
		{	P_FAIL("'mbedtls_x509_crt_info' returned no info for X509 cert found in '" << pluginSigPath() << "'");
			return fail();
		}
		if (showCertificate())
			cout	<< "## DEBUG: Raw X509 certificate info extracted from signature file '" << pluginSigPath() << "':" << endl
					<< "          " << msg << endl;
		if (!ExtractCertInfo (certInfo, msg))
			return false;
		if (isVerbose())
		{	cout << "## NOTE: X509 certificate info extracted from signature file '" << pluginSigPath() << "':" << endl;
			certInfo.Print(cout, false) << endl;
		}
		if (certInfo.hasKey("issuer name"))
			if (!ExtractIssuerInfo (issuerInfo, certInfo.valueForKey("issuer name"), "issuer name"))
				return false;
		if (certInfo.hasKey("subject name"))
			if (!ExtractIssuerInfo (subjectInfo, certInfo.valueForKey("subject name"), "subject name"))
				return false;
		if (!certInfo.hasKey(kNTV2PluginInfoKey_Fingerprint))
		{	P_FAIL("Missing key '" << kNTV2PluginInfoKey_Fingerprint << "' in X509 certificate from '" << pluginSigPath() << "'");
			return fail();
		}
		if (isVerbose()  &&  !issuerInfo.empty())
		{	cout << "## NOTE: 'issuer name' info:" << endl;
			issuerInfo.Print(cout, false) << endl;
		}
		if (isVerbose()  &&  !subjectInfo.empty())
		{	cout << "## NOTE: 'subject name' info:" << endl;
			subjectInfo.Print(cout, false) << endl;
		}
		if (!issuerInfo.hasKey(kNTV2PluginX500AttrKey_CommonName))
		{	P_FAIL("Missing 'Issuer' key '" << kNTV2PluginX500AttrKey_CommonName << "' in X509 certificate from '" << pluginSigPath() << "'");
			return fail();
		}
		if (!issuerInfo.hasKey(kNTV2PluginX500AttrKey_OrganizationName))
		{	P_FAIL("Missing 'Issuer' key '" << kNTV2PluginX500AttrKey_OrganizationName << "' in X509 certificate from '" << pluginSigPath() << "'");
			return fail();
		}
		if (!subjectInfo.hasKey(kNTV2PluginX500AttrKey_OrganizationalUnitName))
		{	P_FAIL("Missing 'Subject' key '" << kNTV2PluginX500AttrKey_OrganizationalUnitName << "' in X509 certificate from '" << pluginSigPath() << "'");
			return fail();
		}
		mDict.addFrom(certInfo);	//	Store certInfo key/value pairs into client/server instance's params...
	}

	//	Compute SHA256 hash of plugin...
	NTV2Buffer checksumFromDLL(32);
	ret = mbedtls_md_file (mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), pluginPath().c_str(), checksumFromDLL);
	if (ret)
	{	P_FAIL("'mbedtls_md_file' returned " << ret << " (" << mbedErrStr(ret) << ") for '" << pluginPath() << "'");
		return fail();
	}
	if (isVerbose())   {string str; if (checksumFromDLL.toHexString(str)) cout << "## DEBUG: Digest: " << str << endl;}

	//	Verify the dylib/DLL/so signature...
	ret = mbedtls_pk_verify (&crt.pk, MBEDTLS_MD_SHA256,
							/*msgHash*/checksumFromDLL, /*msgHashLength*/0,//checksumFromDLL,
							/*signatureToVerify*/signature, /*signatureLength*/signature);
	if (ret)
	{	P_FAIL("'mbedtls_pk_verify' returned " << ret << " (" << mbedErrStr(ret) << ") for '" << pluginSigPath() << "'");
		return fail();
	}
	mbedtls_x509_crt_free(&crt);	//	Done using the mbedtls_x509_crt struct
	P_DBG("'mbedtls_pk_verify' succeeded for '" << pluginPath() << "' -- signature valid");

	//	Load/open the shared library...
	if (!mpPlugin)
		if (!PluginRegistry::Get().loadPlugin (pluginPath(), pluginsPath(), mpPlugin, errMsg, useStdout()))
			return fail();
	PLGDBG("'" << pluginPath() << "' opened");

	//	Obtain AJA Registration Info...
	NTV2Dictionary regInfo;
	string errGetInfo;
	void * pGetInfo = getSymbolAddress(kFuncNameGetRegInfo, errGetInfo);
	if (!pGetInfo)
	{	P_FAIL("'" << pluginPath() << "': '" kFuncNameGetRegInfo "' failed: " << errGetInfo);
		return fail();
	}
	fpGetRegistrationInfo pGetRegInfo = reinterpret_cast<fpGetRegistrationInfo>(pGetInfo);
	if (!(*pGetRegInfo)(uint32_t(AJA_NTV2_SDK_VERSION), regInfo))
		{P_FAIL("'" << pluginPath() << "': '" << kFuncNameGetRegInfo << "' failed");  return fail();}
	PLGDBG("'" << pluginPath() << "': '" << kFuncNameGetRegInfo << "': returned " << regInfo.keys());
	if (regInfo.empty())
		{P_FAIL("'" << pluginPath() << "': no registration info (empty)");  return fail();}

	//	Check for required registration info keys...
	NTV2StringList missingRegInfoKeys;
	static const NTV2StringList reqKeys = {kNTV2PluginRegInfoKey_Vendor, kNTV2PluginRegInfoKey_CommonName,
											kNTV2PluginRegInfoKey_ShortName, kNTV2PluginRegInfoKey_LongName,
											kNTV2PluginRegInfoKey_Description, kNTV2PluginRegInfoKey_Copyright,
											kNTV2PluginRegInfoKey_NTV2SDKVersion, kNTV2PluginRegInfoKey_Version};
	for (size_t ndx(0);  ndx < reqKeys.size();  ndx++)
		if (!regInfo.hasKey(reqKeys.at(ndx)))
			missingRegInfoKeys.push_back(reqKeys.at(ndx));
	if (!missingRegInfoKeys.empty())
	{	P_FAIL("'" << pluginPath() << "': missing key(s) in registration info: '"
				<< aja::join(missingRegInfoKeys, "','") << "'");
		return fail();	//	fail
	}
	mDict.addFrom(regInfo);	//	Add registration info to plugin's dictionary

	//	Check planet alignment...
	const string	cnReg(regInfo.valueForKey(kNTV2PluginRegInfoKey_CommonName)),
					cnCert(issuerInfo.valueForKey(kNTV2PluginX500AttrKey_CommonName));
	const string	onReg(regInfo.valueForKey(kNTV2PluginRegInfoKey_Vendor)),
					onCert(issuerInfo.valueForKey(kNTV2PluginX500AttrKey_OrganizationName));
	const string	ouReg(regInfo.valueForKey(kNTV2PluginRegInfoKey_OrgUnit)),
					ouCert(subjectInfo.valueForKey(kNTV2PluginX500AttrKey_OrganizationalUnitName));
	const string	myVers(NTV2RPCBase::ShortSDKVersion()),
					plVers(regInfo.valueForKey(kNTV2PluginRegInfoKey_NTV2SDKVersion));
	const string	ajaFingerprint(NTV2RPCBase::AJAFingerprint (/*lowerCase*/true, /*stripColons*/false));
	string	fingerprint(mDict.valueForKey(kNTV2PluginInfoKey_Fingerprint));
	aja::lower(fingerprint);	//	since ajaFingerprint is lower case
	if (onReg != onCert)
	{	P_FAIL("Vendor name (key='" << kNTV2PluginRegInfoKey_Vendor << "') \"" << onReg << "\" from plugin \""
				<< pluginPath() << "\" doesn't match organization name (key='" << kNTV2PluginX500AttrKey_OrganizationName
				<< "') \"" << onCert << "\" from X509 certificate 'Issuer' in '" << pluginSigPath() << "'");
		return fail();
	}
	if (cnReg != cnCert)
	{	P_FAIL("Common name (key='" << kNTV2PluginRegInfoKey_CommonName << "') \"" << cnReg << "\" from plugin \""
				<< pluginPath() << "\" doesn't match common name (key='" << kNTV2PluginX500AttrKey_CommonName
				<< "') \"" << cnCert << "\" from X509 certificate 'Issuer' in '" << pluginSigPath() << "'");
		return fail();
	}
	if (ouReg != ouCert)
	{	P_FAIL("Org unit (key='" << kNTV2PluginX500AttrKey_OrganizationalUnitName << "') \"" << ouReg << "\" from plugin \""
				<< pluginPath() << "\" doesn't match org unit (key='" << kNTV2PluginX500AttrKey_OrganizationalUnitName
				<< "') \"" << ouCert << "\" from X509 certificate 'Subject' in '" << pluginSigPath() << "'");
		return fail();
	}
	if (myVers != plVers)
	{	P_FAIL("SDK version '" << plVers << "' from plugin \"" << pluginPath()
				<< "\" doesn't match client SDK version '" << myVers << "'");
		return fail();
	}
	if (fingerprint != ajaFingerprint)
	{	P_FAIL("'" << pluginPath() << "':|Plugin not authorized/signed by AJA:|"
				<< "Issuer serial: " << fingerprint << "|AJA serial: " << ajaFingerprint);
		return fail();	//	fail
	}

	//	Green light
	mValidated = true;
	return true;
}	//	validate

//	Returns address of function having given name
void * NTV2PluginLoader::getFunctionAddress (const string & inFuncName)
{
	//	Load/open the shared library...
	if (!isOpen())
		{P_FAIL("'" << inFuncName << "': '" << pluginPath() << "' not loaded");  return AJA_NULL;}
	if (!isValidated())
		{P_FAIL("'" << inFuncName << "': '" << pluginPath() << "' not validated");  return AJA_NULL;}

	//	Finally, the last step ---- get address of requested function...
	string errStr;
	void * pResult = getSymbolAddress(inFuncName, errStr);
	if (!pResult)
		{P_FAIL("'" << inFuncName << "': '" << pluginPath() << "': " << errStr);  return AJA_NULL;}
	P_DBG("Calling '" << inFuncName << "' in '" << pluginPath() << "'");
	return pResult;
}	//	getFunctionAddress

bool NTV2PluginLoader::isValidated (void) const
{
	return mpPlugin  &&  mValidated;
}

void DumpLoadedPlugins (void)
{
	const NTV2StringList paths (PluginRegistry::Get().loadedPlugins());
	if (paths.empty())
		cout << "0 plugins" << endl;
	else if (paths.size() == 1)
		cout << "1 plugin: " << paths.at(0) << endl;
	else cout << DEC(paths.size()) << " plugins:" << endl << aja::join(paths, "\n") << endl;
}
#endif  //  !defined(NTV2_PREVENT_PLUGIN_LOAD)


/*****************************************************************************************************************************************************
	NTV2RPCBase
*****************************************************************************************************************************************************/

NTV2RPCBase::NTV2RPCBase (NTV2Dictionary params, ULWord * pRefCon)
	:	mParams(params),
		mpRefCon(pRefCon)
{
	NTV2Dictionary queryParams;
	AJAAtomic::Increment(&gBaseConstructCount);
	if (mpRefCon)
		AJAAtomic::Increment(mpRefCon);
	PDBGX("refCnt=" << DEC(mpRefCon ? *mpRefCon : 0) << ", " << DEC(gBaseConstructCount) << " created, "
		<< DEC(gBaseDestructCount) << " destroyed");
	if (mParams.hasKey(kQParamLogToStdout) && (mParams.hasKey(kQParamVerboseLogging) || mParams.hasKey(kQParamShowParams)))
		{cout << __FILE__ << "(" << __LINE__ << "):" << AJAFUNC << ":" << endl;  mParams.Print(cout, false) << endl;}
}

NTV2RPCBase::~NTV2RPCBase ()
{
	AJAAtomic::Increment(&gBaseDestructCount);
	if (mpRefCon)
		AJAAtomic::Decrement(mpRefCon);
	PDBGX("refCnt=" << DEC(mpRefCon ? *mpRefCon : 0) << ", " << DEC(gBaseConstructCount) << " created, "
		<< DEC(gBaseDestructCount) << " destroyed");
}

bool NTV2RPCBase::SetParams (const NTV2ConnectParams & inNewParams, const bool inAugment)
{
	AJAAutoLock tmp(&mParamLock);
	size_t oldCount(mParams.size()), updated(0), added(0);
	if (inAugment)
	{
		updated = mParams.updateFrom(inNewParams);
		added = mParams.addFrom(inNewParams);
        NBSDBG(DEC(updated) << " param(s) updated, " << DEC(added) << " added: " << mParams);
	}
	else
	{
		mParams = inNewParams;
		NBSDBG(DEC(oldCount) << " param(s) removed, replaced with " << inNewParams);
	}
	if (mParams.empty())
		NBSWARN("No params");
	return true;
}

string NTV2RPCBase::ShortSDKVersion (void)
{
	string result(::NTV2Version());
	const NTV2StringList halves(aja::split(result, " "));
	if (halves.empty())
		return result;
	NTV2StringList nums(aja::split(halves.front(), "."));
	while (nums.size() > 3)
		nums.pop_back();
	return aja::join(nums, ".");
}


string NTV2RPCBase::AJAFingerprint (const bool inLowerCase, const bool inStripColons)
{
	static const string	sAJAFingerprint ("70:1A:37:93:FA:4F:34:30:58:55:51:0C:01:4E:45:7C:BE:5B:41:65");
	string result(sAJAFingerprint);
	if (inStripColons)
		aja::replace(result, ":", "");
	if (inLowerCase)
		aja::lower(result);
	return result;
}


/*****************************************************************************************************************************************************
	NTV2RPCClientAPI
*****************************************************************************************************************************************************/

NTV2RPCClientAPI::NTV2RPCClientAPI (NTV2ConnectParams inParams, void * pRefCon)
	:	NTV2RPCBase(inParams, reinterpret_cast<ULWord*>(pRefCon))
{
	AJADebug::Open();
	AJAAtomic::Increment(&gClientConstructCount);
	PDBGX(DEC(gClientConstructCount) << " created, " << DEC(gClientDestructCount) << " destroyed");
}

NTV2RPCClientAPI::~NTV2RPCClientAPI ()
{
	if (IsConnected())
		NTV2Disconnect();
	AJAAtomic::Increment(&gClientDestructCount);
	PDBGX(DEC(gClientConstructCount) << " created, " << DEC(gClientDestructCount) << " destroyed");
}

NTV2ConnectParams NTV2RPCClientAPI::ConnectParams (void) const
{
	AJAAutoLock tmp(&mParamLock);
	return mParams;
}

bool NTV2RPCClientAPI::HasConnectParam (const string & inParam) const
{
	AJAAutoLock tmp(&mParamLock);
	return mParams.hasKey(inParam);
}

string NTV2RPCClientAPI::ConnectParam (const string & inParam) const
{
	AJAAutoLock tmp(&mParamLock);
	return mParams.valueForKey(inParam);
}

bool NTV2RPCClientAPI::ConnectHasScheme (void) const
{
	return HasConnectParam(kConnectParamScheme);
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
//	AJAAutoLock tmp(&mParamLock);
//	mParams.clear();
	return true;
}

NTV2RPCClientAPI * NTV2RPCClientAPI::CreateClient (NTV2ConnectParams & params)	//	CLASS METHOD
{
#if defined(NTV2_PREVENT_PLUGIN_LOAD)
	return AJA_NULL;
#else
	NTV2RPCClientAPI * pRPCObject(AJA_NULL);
	{
		NTV2PluginLoader loader(params);
		fpCreateClient pFunc (reinterpret_cast<fpCreateClient>(loader.getFunctionAddress(kFuncNameCreateClient)));
		if (!pFunc)
			return AJA_NULL;

		//	Call plugin's Create function to instantiate the NTV2RPCClientAPI object...
		pRPCObject = (*pFunc) (loader.refCon(), params, AJA_NTV2_SDK_VERSION);
		if (!pRPCObject)
			NBCFAIL("'" << kFuncNameCreateClient << "' returned NULL client instance from: " << params);
		else
			NBCINFO("'" << kFuncNameCreateClient << "' created client instance " << xHEX0N(uint64_t(pRPCObject),16));
	}	//	loader freed here
	return pRPCObject;
#endif
}	//	CreateClient


/*****************************************************************************************************************************************************
	NTV2RPCServerAPI
*****************************************************************************************************************************************************/

NTV2RPCServerAPI * NTV2RPCServerAPI::CreateServer (NTV2ConfigParams & params)	//	CLASS METHOD
{
#if defined(NTV2_PREVENT_PLUGIN_LOAD)
	return AJA_NULL;
#else
	NTV2RPCServerAPI * pRPCObject(AJA_NULL);
	{
		NTV2PluginLoader loader(params);
		fpCreateServer pFunc = reinterpret_cast<fpCreateServer>(loader.getFunctionAddress(kFuncNameCreateServer));
		if (!pFunc)
			return AJA_NULL;

		//	Call plugin's Create function to instantiate the NTV2RPCServerAPI object...
		pRPCObject = (*pFunc) (loader.refCon(), params, AJA_NTV2_SDK_VERSION);
		if (!pRPCObject)
			NBSFAIL("'" << kFuncNameCreateServer << "' returned NULL server instance from: " << params);
		else
			NBSINFO("'" << kFuncNameCreateServer << "' created server instance " << xHEX0N(uint64_t(pRPCObject),16));
	}
	return pRPCObject;	//	It's caller's responsibility to delete pRPCObject
#endif
}	//	CreateServer

NTV2RPCServerAPI * NTV2RPCServerAPI::CreateServer (const string & inURL)	//	CLASS METHOD
{
	NTV2DeviceSpecParser parser(inURL);
	if (parser.HasErrors())
		return AJA_NULL;
	NTV2ConfigParams parms(parser.Results());
	return CreateServer(parms);
}

NTV2RPCServerAPI::NTV2RPCServerAPI (NTV2ConnectParams inParams, void * pRefCon)
	:	NTV2RPCBase(inParams, reinterpret_cast<ULWord*>(pRefCon))
{
	NTV2Buffer spare(&mSpare, sizeof(mSpare));  spare.Fill(0ULL);
	AJADebug::Open();
	AJAAtomic::Increment(&gServerConstructCount);
	PDBGX(DEC(gServerConstructCount) << " created, " << DEC(gServerDestructCount) << " destroyed");
}

NTV2RPCServerAPI::~NTV2RPCServerAPI()
{
	Stop();
	while (IsRunning())
		AJATime::Sleep(50);
	AJAAtomic::Increment(&gServerDestructCount);
	PDBGX(DEC(gServerConstructCount) << " created, " << DEC(gServerDestructCount) << " destroyed");
}

void NTV2RPCServerAPI::RunServer (void)
{	//	This function normally should never be called;
	//	It's usually overridden by a subclass
	NBSDBG("Started");
	while (!mTerminate)
		AJATime::Sleep(500);
	NBSDBG("Terminated");
}	//	ServerFunction

ostream & NTV2RPCServerAPI::Print (ostream & oss) const
{
	oss << mParams;
	return oss;
}

NTV2ConfigParams NTV2RPCServerAPI::ConfigParams (void) const
{
	AJAAutoLock tmp(&mParamLock);
	return mParams;
}

bool NTV2RPCServerAPI::HasConfigParam (const string & inParam) const
{
	AJAAutoLock tmp(&mParamLock);
	return mParams.hasKey(inParam);
}

string NTV2RPCServerAPI::ConfigParam (const string & inParam) const
{
	AJAAutoLock tmp(&mParamLock);
	return mParams.valueForKey(inParam);
}
